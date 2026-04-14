[← Week 11][link-week11] | [Week Final →][link-week-final]

[![preview][img-preview]](https://youtu.be/53094Xegf2k)

# DX-Engine — Week 12: Cascade Particle System

> UE4 Cascade를 레퍼런스로 설계한 모듈식 CPU 파티클 시스템 — 에셋/런타임 이중 계층, 캐시 친화적 연속 메모리 풀, Base+Payload 확장 구조

![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus&logoColor=white)
![DirectX 11](https://img.shields.io/badge/DirectX-11-0078D4?logo=microsoft&logoColor=white)
![HLSL](https://img.shields.io/badge/HLSL-SM_5.0-5C2D91)
![Windows](https://img.shields.io/badge/Windows-0078D6?logo=windows&logoColor=white)

---

## Features

- **Cascade 3계층 아키텍처** — `UParticleSystem → UParticleEmitter → UParticleModule` 에셋 계층과 `UParticleSystemComponent → FParticleEmitterInstance` 런타임 계층을 분리
- **캐시 친화적 연속 메모리 풀** — `uint8*` 풀 + `uint16*` 인덱스 배열, 16-byte 정렬, swap-and-decrement O(1) 파티클 제거
- **Base + Payload 확장 구조** — `FBaseParticle` 고정 헤더에 모듈별 페이로드 바이트를 동적으로 추가, 모듈 코드 변경 없이 속성 확장
- **UDistribution 전략 패턴** — 상수 / 균등 분포 / 베지어 커브를 동일 인터페이스로 교체해 파티클 속성에 확률적 변이 부여

---

## Key Systems

### 1. Cascade 3계층 아키텍처 + 6단계 Tick 파이프라인

UE4 Cascade 파티클 시스템 소스를 분석해 에셋-런타임 이중 계층 구조로 재구현했다.

```
에셋 계층 (설계 데이터)          런타임 계층 (실행 데이터)
─────────────────────────       ────────────────────────────
UParticleSystem                 UParticleSystemComponent
  └─ UParticleEmitter[]    →      └─ FParticleEmitterInstance[]
       └─ UParticleLODLevel              ↑ Init() 시 에셋 참조
            └─ UParticleModule[]    매 프레임 Tick()
```

에셋 계층은 파티클 동작 규칙을 정의하고, 런타임 계층은 해당 규칙을 매 프레임 시뮬레이션한다. 에셋 하나로 여러 런타임 인스턴스를 생성할 수 있어 메모리 중복 없이 다수의 이펙트를 실행한다.

![architecture][img-architecture]

`FParticleEmitterInstance::Tick()`은 6단계 파이프라인으로 구성된다.

```cpp
void FParticleEmitterInstance::Tick(float DeltaTime)
{
    Tick_EmitterTimeSetup(DeltaTime);  // 1. EmitterTime 전진, 루프 처리
    KillParticles();                   // 2. RelativeTime >= 1.0 파티클 제거
    ResetParticleParameters();         // 3. Velocity = BaseVelocity, Acceleration = 0
    Tick_ModuleUpdate(DeltaTime);      // 4. 각 UParticleModule::Update() 호출
    Tick_SpawnParticles(DeltaTime);    // 5. SpawnRate 기반 신규 파티클 생성 + Spawn() 호출
    Tick_ModuleFinalUpdate(DeltaTime); // 6. 위치 적분, FinalUpdate 처리
}
```

3단계의 `ResetParticleParameters()`가 `Velocity = BaseVelocity`로 매 프레임 속도를 초기화하므로, Update 모듈들은 독립적으로 속도에 힘을 누적할 수 있다. 모듈 간 실행 순서 의존성이 없어진다.

`CacheEmitterModuleInfo()`는 이미터 초기화 시 1회 실행되어 모든 모듈을 `SpawnModules[]` / `UpdateModules[]` / `FinalUpdateModules[]` 리스트로 분류한다. 런타임에는 분류된 리스트만 순회하므로 불필요한 타입 검사가 없다.

![pipeline][img-pipeline]

---

### 2. 캐시 친화적 연속 메모리 풀

파티클 데이터를 `std::vector<Particle>` 대신 수동 관리 연속 메모리 블록에 저장한다.

```cpp
// FParticleEmitterInstance — 풀 초기화
int32 ParticleSize   = (sizeof(FBaseParticle) + ModulePayloadBytes + 15) & (~15); // 16-byte 정렬
uint8*  ParticleData    = new uint8[MaxParticles * ParticleSize];  // 연속 메모리 풀
uint16* ParticleIndices = new uint16[MaxParticles];                // 인덱스 배열

for (int32 i = 0; i < MaxParticles; i++)
    ParticleIndices[i] = i;  // 초기 순열: [0, 1, 2, ..., N]
```

**16-byte 정렬**: `(RawSize + 15) & (~15)` 비트 연산으로 각 파티클 슬롯을 16-byte 경계에 정렬한다. GPU HLSL `cbuffer`의 `float4` 정렬 요구사항을 충족시키며, CPU 캐시 라인 경계와도 일치해 순회 시 캐시 미스를 줄인다.

**O(1) swap-and-decrement 파티클 제거**: 파티클이 수명을 다하면 인덱스 배열에서 해당 슬롯을 마지막 활성 파티클과 교환하고 카운터를 감소시킨다. 메모리 이동이 없다.

```cpp
void FParticleEmitterInstance::KillParticle(int32 Index)
{
    // 죽은 파티클의 인덱스를 마지막 활성 파티클 인덱스로 덮어쓰기
    ParticleIndices[Index] = ParticleIndices[ActiveParticles - 1];
    ActiveParticles--;  // O(1) — 메모리 이동 없음
}
```

역방향 루프 매크로 `BEGIN_UPDATE_LOOP / END_UPDATE_LOOP`로 루프 내에서 `KillParticle(i)`를 안전하게 호출할 수 있다. 인덱스 `i`가 방금 교체된 살아있는 파티클로 채워지므로 스킵 없이 처리된다.

![memory-layout][img-memory-layout]

<details>
<summary><b>구현 상세 — 클릭해서 펼치기</b></summary>

<br>

**역방향 루프 매크로**

```cpp
#define BEGIN_UPDATE_LOOP                                           \
    for (int32 i = ActiveParticles - 1; i >= 0; i--) {            \
        DECLARE_PARTICLE(Particle,                                  \
            ParticleData + ParticleStride * ParticleIndices[i]);

#define END_UPDATE_LOOP \
    }
```

`i = ActiveParticles - 1`부터 역방향으로 순회하므로, 루프 내에서 `KillParticle(i)`를 호출해도 아직 처리하지 않은 `[0 .. i-1]` 슬롯에는 영향을 주지 않는다.

**파티클 접근 매크로**

```cpp
#define DECLARE_PARTICLE(Name, Address) \
    FBaseParticle& Name = *((FBaseParticle*)(Address));

#define DECLARE_PARTICLE_PTR(Name, Address) \
    FBaseParticle* Name = (FBaseParticle*)(Address);
```

인덱스 → 주소 변환을 `ParticleData + ParticleStride * ParticleIndices[i]` 한 줄로 캡슐화한다.

**Sub-frame spawn 분산**

```cpp
// 한 프레임에 N개 파티클을 균등 분산해 생성
float Interp = 0.0f;
float InterpIncrement = (SpawnCount > 0) ? 1.0f / SpawnCount : 0.0f;
for (int32 i = 0; i < SpawnCount; i++, Interp += InterpIncrement) {
    SpawnParticle(SpawnTime - DeltaTime * (1.0f - Interp));
}
```

`Interp`로 생성 시점을 프레임 내에 균등 분산해 파티클이 한 점에 군집되는 현상을 방지한다.

**EmitterDelay 구현**

```cpp
EmitterTime -= EmitterDelay;  // 음수 EmitterTime → 스폰 조건(>= 0) 자연 억제
```

별도 플래그 없이 단 한 줄로 딜레이를 구현한다.

</details>

---

### 3. FBaseParticle + 모듈 페이로드 (Base + Payload 확장 구조)

각 파티클 슬롯은 고정 헤더(`FBaseParticle`)와 모듈이 요청한 가변 페이로드로 구성된다.

```
파티클 슬롯 레이아웃 (ParticleSize = 고정 + 페이로드 합계, 16-byte 정렬)
┌──────────────────────────────────┬────────────────┬────────────────┬─────┐
│  FBaseParticle (고정 헤더)        │ VelocityPayload│ SubUVPayload   │ ... │
│  Location, Velocity, Color,      │  (모듈이 요청) │  (모듈이 요청) │     │
│  RelativeTime, Size, SubImageIdx │                │                │     │
└──────────────────────────────────┴────────────────┴────────────────┴─────┘
                                   ↑ offset[0]       ↑ offset[1]
                             ModuleOffsetMap로 사전 계산
```

모듈은 `RequiredBytes()`로 필요한 바이트 수를 선언하고, 페이로드 접근은 `PARTICLE_ELEMENT` 매크로로 타입 안전하게 처리한다.

```cpp
// UParticleModule::RequiredBytes() — 페이로드 바이트 선언
int32 UParticleModuleSubUV::RequiredBytes() { return sizeof(FSubUVPayload); }
int32 UParticleModuleVelocity::RequiredBytes() { return 0; }  // FBaseParticle만으로 충분

// PARTICLE_ELEMENT 매크로 — 타입 안전 페이로드 접근 + 오프셋 자동 전진
#define PARTICLE_ELEMENT(Type, Name)                                    \
    Type& Name = *((Type*)((uint8*)ParticleBase + CurrentOffset));      \
    CurrentOffset += sizeof(Type);

// 모듈 Update 내에서 페이로드 접근 예시 (UParticleModuleSubUV::Update)
void UParticleModuleSubUV::Update(const FUpdateContext& Context)
{
    BEGIN_UPDATE_LOOP
        int32 CurrentOffset = Offset;  // ModuleOffsetMap에서 가져온 사전 계산 오프셋
        PARTICLE_ELEMENT(FSubUVPayload, SubUV);
        Particle.SubImageIndex = SubUVDistribution.GetValue(Particle.RelativeTime);
    END_UPDATE_LOOP
}
```

**CacheEmitterModuleInfo() — 빌드타임 오프셋 선계산**

```cpp
void UParticleEmitter::CacheEmitterModuleInfo()
{
    int32 RunningOffset = sizeof(FBaseParticle);  // 고정 헤더 이후부터 시작
    for (UParticleModule* Module : LODLevel->Modules) {
        int32 ReqBytes = Module->RequiredBytes();
        if (ReqBytes > 0) {
            ModuleOffsetMap.Add(Module, RunningOffset);  // 오프셋 등록
            RunningOffset += ReqBytes;
        }
        if (Module->bSpawnModule)       SpawnModules.Add(Module);
        if (Module->bUpdateModule)      UpdateModules.Add(Module);
        if (Module->bFinalUpdateModule) FinalUpdateModules.Add(Module);
    }
    ParticleSize = (RunningOffset + 15) & (~15);  // 최종 정렬
}
```

이미터 초기화 시 1회 실행되므로 런타임에는 `ModuleOffsetMap[Module]`로 O(1) 조회 후 포인터 산술만 수행한다. 새 모듈을 추가해도 기존 모듈 코드를 수정할 필요가 없다.

메시 파티클처럼 별도 런타임 인스턴스 타입이 필요한 경우, `UParticleModuleTypeDataMesh::CreateInstance()`가 `FParticleMeshEmitterInstance`를 반환하고 `RequiredBytes()`에서 회전 페이로드(`FMeshRotationPayloadData`, 72바이트)를 추가 요청한다. 스프라이트/메시 파티클이 동일한 파이프라인에서 처리된다.

---

### 4. UDistribution 전략 패턴

파티클 속성(속도·크기·색상·수명)에 대한 값 공급 인터페이스를 추상화했다.

```
UDistributionFloat          GetValue(float T)
  ├─ UDistributionFloatConstant      → 고정 값
  ├─ UDistributionFloatUniform       → [Min, Max] 균등 분포 (std::mt19937)
  └─ UDistributionFloatBezier        → 3차 베지어 커브
```

```cpp
// 베지어 커브 — 수명에 따른 크기 변화 등에 활용
float UDistributionFloatBezier::GetValue(float T) const
{
    float T2 = T * T, T3 = T2 * T;
    float MT = 1.0f - T, MT2 = MT * MT, MT3 = MT2 * MT;
    return MT3*P[0] + 3*MT2*T*P[1] + 3*MT*T2*P[2] + T3*P[3];
}
```

`FRawDistributionFloat`가 `UDistributionFloat*`를 소유하므로, 모듈은 `Distribution->GetValue(T)`만 호출하면 된다. 파티클 모듈 코드 변경 없이 상수·균등·베지어 분포를 런타임에 교체할 수 있다.

---

## Performance

cpp-insights 프로파일러를 통합해 `EmitterTick` / `ParticleUpdateDynamicData` / `ParticleCollectBatches` 구간을 계측했다. 테스트 환경: i5-12600K, 32GB DDR4-3200, Release 빌드.

![performance][img-performance]

**EmitterTick 스케일링 (스프라이트 vs 메시)**

| 파티클 수 | Sprite EmitterTick (ms) | Mesh EmitterTick (ms) |
|----------:|------------------------:|----------------------:|
|     1,000 |                   0.026 |                 0.033 |
|     5,000 |                   0.143 |                 0.208 |
|    10,000 |                   0.313 |                 0.384 |
|    20,000 |                   0.930 |                 0.938 |
|    50,000 |                   2.905 |                 3.798 |
|   100,000 |                   3.565 |                 4.729 |

- 스프라이트 기준 10만 개 EmitterTick **3.5ms** — 60fps 예산(16.6ms)의 **21%**
- 각 이미터의 파티클 데이터(`ParticleStride` = 224B)가 단일 연속 블록에 저장되어 공간 지역성(spatial locality)을 유지, ~20K 이하 구간에서 안정적인 선형 증가를 보임
- 20K→50K 구간에서 기울기가 증가하는데, 이 시점에서 파티클 풀 크기가 L2 캐시(1.25MB/코어)를 초과하기 시작함 (20K×224B ≈ 4.5MB)

**메모리 할당 최적화 — `FParticleDataContainer::Alloc` 버퍼 재사용**

`ParticleUpdateDynamicData`에서 매 프레임 `free → malloc → memcpy`가 발생해 100K 기준 약 4ms의 오버헤드가 있었다. 기존 블록이 충분한 경우 재할당을 건너뛰는 버퍼 재사용 방식으로 변경해 약 **2ms 감소**시켰다.

```cpp
void FParticleDataContainer::Alloc(int32 InParticleDataNumBytes, int32 InParticleIndicesNumShorts)
{
    int32 NewMemBlockSize = InParticleDataNumBytes + (InParticleIndicesNumShorts * sizeof(uint16));
    if (ParticleData != nullptr && MemBlockSize >= NewMemBlockSize)
    {
        // 기존 블록 재사용 — 재할당 없음
        ParticleDataNumBytes     = InParticleDataNumBytes;
        ParticleIndicesNumShorts = InParticleIndicesNumShorts;
        ParticleIndices = (uint16*)(ParticleData + InParticleDataNumBytes);
        return;
    }
    Free();
    // ...
}
```

**캐시 효율 벤치마크 — 연속 배열 vs 힙 분산 포인터**

`FBaseParticle`(144B)과 동일한 Tick 연산으로 측정. 연속 배열이 힙 분산 대비 최대 **4.4배** 빠름(Release, 50K 기준).

| 파티클 수 | Contiguous (ms) | Heap Scattered (ms) | Speedup |
|----------:|----------------:|--------------------:|--------:|
|     1,000 |           0.003 |               0.004 |   1.33x |
|     5,000 |           0.016 |               0.022 |   1.38x |
|    10,000 |           0.041 |               0.070 |   1.71x |
|    30,000 |           0.116 |               0.267 |   2.30x |
|    50,000 |           0.199 |               0.877 |   4.41x |
|   100,000 |           1.135 |               2.748 |   2.42x |

---

## References

- Epic Games, **Unreal Engine 4 Source — Cascade Particle System** (`Engine/Source/Runtime/Engine/Private/Particles/`)
- Tomas Akenine-Möller et al., **Real-Time Rendering** — OBB Slab Method

---

[← Week 11][link-week11] | [Week Final →][link-week-final]

<!-- 이미지 레퍼런스 -->
[img-preview]:       Docs/Images/week-12/thumbnail-youtube.png
[img-architecture]:  Docs/Images/week-12/architecture.png
[img-memory-layout]: Docs/Images/week-12/memory-layout.png
[img-pipeline]:      Docs/Images/week-12/pipeline.png
[img-performance]:   Docs/Images/week-12/performance.png

<!-- 링크 레퍼런스 -->
[link-week11]:      https://github.com/geb0598/DX-Engine/tree/week-11
[link-week-final]:  https://github.com/geb0598/DX-Engine/tree/week-final
