[← Week 06][link-week06] | [Week 08 →][link-week08]

[![2.5D Culling for Forward+][img-thumbnail]][link-youtube]

# DX-Engine — Week 07: Forward+ 타일 기반 라이트 컬링

> Compute Shader 기반 2.5D 라이트 컬링 파이프라인 E2E 구현 및 런타임 셰이더 핫 리로드

![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus&logoColor=white)
![DirectX 11](https://img.shields.io/badge/DirectX-11-0078D4?logo=microsoft&logoColor=white)
![HLSL](https://img.shields.io/badge/HLSL-SM_5.0-5C2D91)
![Windows](https://img.shields.io/badge/Windows-0078D6?logo=windows&logoColor=white)

---

## Features

- **Forward+ 2.5D 라이트 컬링** — 화면을 32×32 픽셀 타일로 분할, Compute Shader에서 프러스텀 + 깊이 마스크 2단계 컬링으로 픽셀 셰이더가 평가하는 라이트 수를 대폭 축소
- **HLSL 런타임 핫 리로드** — 파일 수정 시간 감시 기반 자동 재컴파일, Temp ComPtr 트랜잭션으로 컴파일 실패 시 이전 셰이더 유지

---

## Key Systems

### Forward+ 타일 기반 2.5D 라이트 컬링

1,000개 포인트 라이트 씬에서 모든 픽셀이 모든 라이트를 평가하면 연산량이 `픽셀 수 × 라이트 수`로 폭발적으로 증가한다.
화면을 타일로 나누고 Compute Shader가 각 타일에 영향을 주는 라이트만 추린 뒤 비트마스크로 PS에 전달한다.
AMD의 SIGGRAPH 2012 Forward+ 발표자료를 바탕으로, 타일의 깊이 분포를 32비트 마스크로 압축해 화면 XY상으로는 겹쳐 보여도 깊이 범위가 맞지 않는 라이트를 추가로 기각하는 **2.5D 컬링**을 구현했다.

![pipeline][img-pipeline]

<details>
<summary><b>구현 상세 — 클릭해서 펼치기</b></summary>

<br>

**Phase 0 — Depth Prepass**

Base Pass 전에 씬을 한 번 그려 Depth Buffer를 채운다. CS는 이 버퍼에서 타일 내 픽셀들의 실제 깊이 분포를 읽는다.

**Phase 1 — TileDepthMask 구축**

스레드 그룹 하나(32×32 = 1024 스레드)가 타일 하나를 담당한다.
각 스레드가 자기 픽셀의 NDC 깊이를 선형 깊이로 역변환한 뒤, Near~Far를 32등분한 슬라이스 중 해당 인덱스에 비트를 원자적으로 기록한다.

```hlsl
float LinearDepth = (NearClip * FarClip) / (FarClip - DepthSample * (FarClip - NearClip));
float NormalizedDepth = saturate((LinearDepth - NearClip) / (FarClip - NearClip));
uint SliceIndex = clamp((uint)(NormalizedDepth * NUM_SLICES), 0, NUM_SLICES - 1);
InterlockedOr(TileDepthMask, 1u << SliceIndex);
```

결과: TileDepthMask는 타일 내 실제 픽셀이 존재하는 깊이 슬라이스만 비트가 켜진 32비트 값.

**Phase 2 — 2단계 라이트 컬링**

Thread(0,0)이 타일의 뷰 프러스텀을 생성한 뒤, 각 라이트에 대해 두 단계로 기각한다.

- **1차 — 프러스텀 컬링**: 라이트 어테뉴에이션 반경을 구(Sphere)로 추상화해 6-평면 교차 판정. 프러스텀 밖이면 기각.
- **2차 — 깊이 마스크 컬링**: 라이트 Z 범위를 동일한 32비트 슬라이스로 변환(`SphereToDepthMask()`) 후 `TileDepthMask`와 AND. 겹치는 슬라이스가 없으면 기각.

```hlsl
void CullPointLight(uint Index, uint FlatTileIndex, FSphere SphereVS, FFrustum Frustum) {
    if (!IsSphereInsideFrustum(SphereVS, Frustum)) return;       // 1차: 프러스텀 밖
    if (!(TileDepthMask & SphereToDepthMask(SphereVS))) return;  // 2차: 실제 픽셀 없는 깊이
    uint BucketIndex = Index / BUCKET_SIZE;
    InterlockedOr(PointLightMask[FlatTileIndex * BUCKET_SIZE + BucketIndex], 1u << (Index % BUCKET_SIZE));
}
```

**Phase 3 — PS 비트마스크 조회**

픽셀 셰이더는 화면 좌표 → 타일 인덱스를 계산해 `PointLightMask`에서 비트마스크를 읽고, `firstbitlow()` 로 켜진 비트만 순회해 가시 라이트만 연산한다.

```hlsl
uint flatIdx = CalculateFlatTileIndex(Input.Position.x, Input.Position.y);
for (uint bucket = 0; bucket < NUM_BUCKETS; ++bucket) {
    uint mask = PointLightMask[flatIdx * NUM_BUCKETS + bucket];
    while (mask != 0u) {
        uint li = bucket * BUCKET_SIZE + firstbitlow(mask);
        CalculatePointLight(PointLights[li], ...);
        mask &= mask - 1u;
    }
}
```

**히트맵 디버그 시각화**

`SF_Heatmap` ShowFlag로 토글되는 디버그 패스를 구현했다.
타일당 라이트 수를 Blue → Cyan → Green → Yellow → Red 그라디언트로 시각화해 컬링 분포를 실시간 확인한다.

▶ [히트맵 데모 영상][link-youtube]

</details>

**성능 측정** — 1,000 Point Lights, GPU RenderScene 기준

![performance][img-performance]

| | Culling OFF | Culling ON |
|---|---|---|
| RenderScene (GPU) | 66.7 ms | 15.8 ms |
| Cull Pass (GPU) | — | 0.817 ms |
| **개선** | | **4.2× (76% 감소)** |

---

### HLSL 런타임 핫 리로드

엔진 실행 중 `.hlsl` 파일을 수정하면 자동으로 재컴파일해 즉시 반영한다.
라이팅·포스트 프로세싱 셰이더 이터레이션 속도를 크게 단축하는 개발 도구다.

**자동 감지** — `ResourceManager::Load<UShader>()`를 템플릿 특수화로 구현해, 기존 캐싱 인터페이스를 변경하지 않고 파일 수정 시간 비교 로직을 셰이더에만 주입한다.

```cpp
template<>
inline UShader* UResourceManager::Load(const FString& InCommandString) {
    if (Iter != Resources[TypeIndex].end()) {
        auto Shader = static_cast<UShader*>(Iter->second);
        FString ShaderPath; std::istringstream(InCommandString) >> ShaderPath;
        if (Shader->GetLastModificationTime() < std::filesystem::last_write_time(ShaderPath))
            Shader->Load(InCommandString, Device);  // 변경 감지 → 재컴파일
        return Shader;
    }
}
```

**트랜잭션 안전 컴파일** — Temp `ComPtr`에 먼저 컴파일하고, 성공 시에만 기존 셰이더와 교체한다. 컴파일 실패 시 이전 셰이더가 유지되어 런타임 크래시나 검은 화면을 방지한다.

```cpp
ComPtr<ID3D11VertexShader> TempVS;
// ... D3DCompileFromFile → 성공 시에만 교체
if (VertexShader) { VertexShader->Release(); VertexShader = nullptr; }
VertexShader = TempVS.Detach();
LastModificationTime = std::filesystem::last_write_time(ShaderFilePath);
```

---

## References

- Harada et al., **Forward+: Bringing Deferred Lighting to the Next Level**, SIGGRAPH 2012

---

[← Week 06][link-week06] | [Week 08 →][link-week08]

<!-- 이미지 레퍼런스 -->
[img-thumbnail]:   Docs/Images/thumbnail-youtube.png
[img-pipeline]:    Docs/Images/Forward+pipeline.png
[img-performance]: Docs/Images/Forward+performance.png

<!-- 링크 레퍼런스 -->
[link-youtube]: https://youtu.be/hzabsv2XB2U
[link-week06]:  https://github.com/geb0598/DX-Engine/tree/week-06
[link-week08]:  https://github.com/geb0598/DX-Engine/tree/week-08
