[← Week 07][link-week07] | [Week 09 →][link-week09]

![preview][img-preview]

# DX-Engine — Week 08: Shadow Map Filtering

> Shadow Map 기반 그림자 필터링 기법 3종(PCF · VSM · SAVSM)을 DirectX 11과 HLSL Compute Shader로 구현

![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus&logoColor=white)
![DirectX 11](https://img.shields.io/badge/DirectX-11-0078D4?logo=microsoft&logoColor=white)
![HLSL](https://img.shields.io/badge/HLSL-SM_5.0-5C2D91)
![Windows](https://img.shields.io/badge/Windows-0078D6?logo=windows&logoColor=white)

---

## Features

- **PCF (Percentage Closer Filtering)** — 주변 텍셀을 다중 샘플링해 그림자 경계 앨리어싱을 완화하는 기본 필터링 기법
- **VSM (Variance Shadow Maps)** — 깊이 1·2차 모멘트를 저장하고 체비쇼프 부등식으로 그림자 인자를 수학적으로 계산. ddx/ddy Analytic Variance Bias로 실루엣 아티팩트 억제
- **SAVSM (Summed-Area VSM)** — Hillis-Steele parallel prefix scan을 Compute Shader로 구현해 필터 크기에 무관한 O(1) 그림자 쿼리 실현
- **런타임 Shadow Mode 전환** — 라이트별 PCF / VSM / VSM+Box / VSM+Gaussian / SAVSM을 에디터에서 실시간 선택

---

![shadow-comparison][img-comparison]

---

## Pipeline Overview

![pipeline][img-pipeline]

---

## Key Systems

### 1. PCF - Percentage Closer Filtering

기본 Shadow Map은 라이트 공간 깊이를 단순 비교해 픽셀이 그림자에 있는지 판단한다. 텍셀 해상도에 의해 경계가 계단 현상(앨리어싱 아티팩트) 형태로 나타난다.

PCF는 주변 N×N 텍셀을 샘플링해 그림자 여부를 평균 내는 방식으로 경계를 완화한다.

```hlsl
// PCF: 3×3 커널 샘플링
float ShadowFactor = 0.0f;
for (int y = -1; y <= 1; ++y)
    for (int x = -1; x <= 1; ++x)
    {
        float2 Offset = float2(x, y) * TexelSize;
        ShadowFactor += ShadowMap.SampleCmpLevelZero(
            ShadowSampler, ShadowUV + Offset, CurrentDepth);
    }
ShadowFactor /= 9.0f;
```

**PCF의 한계**: 그림자 경계를 더 부드럽게 만들려면 커널 크기를 키워야 하고, 샘플 수는 커널 크기의 제곱에 비례해 증가한다.

---

### 2. VSM - Variance Shadow Maps

PCF의 다중 샘플링 대신, 섀도우 맵에 깊이의 통계적 모멘트를 저장하고 확률론으로 그림자 인자를 계산한다.

**핵심 아이디어**: 깊이 값의 평균(M1)과 분산(Var)을 알면, 체비쇼프 부등식으로 현재 픽셀 깊이 `t`가 차폐물보다 뒤에 있을 확률 상한을 단 두 채널 샘플링으로 얻을 수 있다.

<details>
<summary><b>구현 상세 — 클릭해서 펼치기</b></summary>

<br>

**모멘트 저장**

깊이 단일 값 대신 `float2(M1, M2)` — 1차·2차 모멘트를 `DXGI_FORMAT_R32G32_FLOAT` 텍스쳐에 렌더링한다.

```hlsl
// DepthOnlyVS.hlsl — 모멘트 저장 패스
float2 mainPS(PS_INPUT Input) : SV_Target0
{
    float Depth = Input.Position.z;

    // Analytic Variance Bias: ddx/ddy로 깊이 기울기를 추정해 실루엣 아티팩트 억제
    float dx = ddx(Depth);
    float dy = ddy(Depth);
    float AnalyticVarianceBias = 0.25f * (dx * dx + dy * dy);

    return float2(Depth, Depth * Depth + AnalyticVarianceBias);
}
```

PCF는 섀도우 맵 텍셀을 단일 깊이값으로 취급하기 때문에, 라이트에 대해 비스듬히 놓인 표면에서 텍셀 하나가 넓은 범위를 포괄하게 된다. 이때 현재 픽셀의 깊이가 텍셀 깊이보다 미세하게 크면 자기 자신의 그림자를 받는 Shadow Acne가 발생하고, 이를 막으려면 bias를 수동으로 조정해야 한다.

VSM은 이 문제를 구조적으로 해결한다. 텍셀을 단일 깊이가 아니라 국소적으로 평면인 깊이 분포로 모델링하고, 그 분산을 ddx/ddy로 계산한 표면 기울기로부터 구한다.

```
Var = E[d²] - E[d]²  ≈  0.25 * (∂d/∂x)² + 0.25 * (∂d/∂y)²
```

표면이 라이트 투영 평면과 평행하면 편미분이 0이 되어 분산도 0에 수렴하고, 기울어질수록 분산이 커져 해당 텍셀의 깊이 분포 폭을 자동으로 반영한다. 체비쇼프 부등식은 이 분산을 이용해 그림자 인자를 계산하므로, 비스듬한 표면에서도 추가 bias 없이 올바른 결과를 낸다.

---

**그림자 인자 계산 (체비쇼프)**

```hlsl
// UberLit.hlsl
float CalculateChebyshevShadowFactor(float t, float2 Moments)
{
    float M1       = Moments.x;
    float M2       = Moments.y;
    float Variance = max(0.00001f, M2 - M1 * M1);  // Var[X] = E[X²] - E[X]²
    float d        = t - M1;
    return Variance / (Variance + d * d);            // P(x >= t) 상한
}
```

- `t` : 현재 픽셀의 라이트 공간 깊이
- `t > M1` (평균 깊이보다 뒤에 있을수록) → ShadowFactor 감소 → 어두워짐
- PCF의 N² 샘플링과 달리 **두 채널 샘플링 한 번**으로 연속적인 그림자 경계를 얻는다

</details>

**VSM의 새로운 한계**: 모멘트 텍스쳐에 블러를 적용하면 경계가 더 부드러워지지만, 블러의 비용은 커널 반지름에 비례해 증가한다. PCF와 동일한 문제가 필터 단계에서 재발한다.

---

### 3. SAVSM - Summed-Area Variance Shadow Maps

VSM 블러의 비용 문제를 **SAT(Summed-Area Table)** 로 해결한다. 모멘트 텍스쳐 전체의 SAT를 프레임당 한 번 계산해두면, 임의 크기 직사각형 영역의 평균 모멘트를 **4-corner 조회 한 번**으로 상수 시간에 얻는다.

```
VSM + Blur:  샘플 수 = O(커널 크기²)  — 커널이 커질수록 비용 증가
SAVSM:       샘플 수 = O(1)           — 필터 크기와 무관
```

SAT 구축에는 2D 텍스쳐 전체의 행·열 방향 누적합이 필요하다. 이를 Compute Shader에서 **Hillis-Steele parallel prefix scan**으로 구현했다.

<details>
<summary><b>구현 상세 — 클릭해서 펼치기</b></summary>

<br>

**Hillis-Steele 알고리즘**

1024개 스레드 그룹이 텍스쳐의 한 행(Row) 또는 열(Column) 전체를 `groupshared` 메모리에 올린 뒤, stride를 1→2→4→…→512로 증가시키며 log₂1024 = 10 스텝에 inclusive prefix sum을 완성한다.

```hlsl
// SummedAreaTextureFilter.hlsl
groupshared float2 SharedMemory[THREAD_BLOCK_SIZE]; // 1024

[numthreads(THREAD_BLOCK_SIZE, 1, 1)]
void mainCS(uint3 GroupThreadID : SV_GroupThreadID, uint3 GroupID : SV_GroupID)
{
    uint ThreadIndex = GroupThreadID.x;
#ifdef SCAN_DIRECTION_COLUMN
    SharedMemory[ThreadIndex] = InputTexture[uint2(GroupID.x, ThreadIndex)];
#else
    SharedMemory[ThreadIndex] = InputTexture[uint2(ThreadIndex, GroupID.x)];
#endif
    GroupMemoryBarrierWithGroupSync();

    for (uint Stride = 1; Stride < THREAD_BLOCK_SIZE; Stride <<= 1)
    {
        float2 NeighborValue = (ThreadIndex >= Stride)
            ? SharedMemory[ThreadIndex - Stride]
            : float2(0.0f, 0.0f);
        GroupMemoryBarrierWithGroupSync();
        SharedMemory[ThreadIndex] += NeighborValue;
        GroupMemoryBarrierWithGroupSync();
    }
#ifdef SCAN_DIRECTION_COLUMN
    OutputTexture[uint2(GroupID.x, ThreadIndex)] = SharedMemory[ThreadIndex];
#else
    OutputTexture[uint2(ThreadIndex, GroupID.x)] = SharedMemory[ThreadIndex];
#endif
}
```

Row 패스 → Column 패스 2회로 2D SAT를 완성한다. `SCAN_DIRECTION_COLUMN` 매크로 하나로 단일 `.hlsl` 파일에서 두 방향을 분기해 코드 중복을 제거했다.

> **트레이드오프**: Hillis-Steele은 총 연산이 O(n log n)으로 Work-inefficient하지만, GPU의 특성 덕분에 O(log n) 스텝에 완료된다. 1024 해상도에서 스레드 그룹 하나가 정확히 한 행/열을 처리하도록 설계되어 있다.

**O(1) SAT 조회**

```hlsl
// 직사각형 4-corner 차분으로 영역 평균 모멘트 계산 (UberLit.hlsl)
uint2 D = asuint(SATTexture.Load(int3(TileOrigin + RegionMax, 0)).xy);
uint2 B = asuint(SATTexture.Load(int3(TileOrigin + int2(RegionMax.x, RegionMin.y - 1), 0)).xy);
uint2 C = asuint(SATTexture.Load(int3(TileOrigin + int2(RegionMin.x - 1, RegionMax.y), 0)).xy);
uint2 A = asuint(SATTexture.Load(int3(TileOrigin + int2(RegionMin.x - 1, RegionMin.y - 1), 0)).xy);
float2 Moments = float2(D - B - C + A) / (Area * SAT_DEPTH_SCALE);
return CalculateChebyshevShadowFactor(CurrentDepth, Moments);
```

</details>

---

## Troubleshooting

### Catastrophic Cancellation in Float SAT

초기 구현은 float SAT로 작성했으나, 그림자 영역에 노이즈 패턴 같은 불규칙한 아티팩트가 발생했다.

![savsm-artifact][img-savsm-artifact]

원인은 SAT 4-corner 연산 `D - B - C + A`에서의 Catastrophic Cancellation이었다. D, B, C, A는 모두 텍스쳐 전체 누적합에 가까운 큰 값이고, 이 값들의 차이인 실제 영역 합은 훨씬 작을 수 있다. 1024×1024 해상도에서 SAT 값은 최대 ~10⁶ 규모까지 커지는데, float32는 유효숫자가 약 7자리이므로 이 규모의 숫자에서 수십 단위 차이를 구하면 의미 있는 비트가 전혀 남지 않는다.

```
// float SAT — Catastrophic Cancellation 발생
float D = 524287.73f;
float B = 524277.81f;
float C = 524261.45f;
float A = 524251.53f;
float Result = D - B - C + A; // 기댓값: 0.00, 실제: 노이즈
```

**해결**: float [0,1] 깊이 값을 정수로 스케일링해 SAT를 정수로 구성하고, D-B-C+A를 정수 연산으로 수행한다. 정수 뺄셈은 Cancellation이 없어 결과가 정확하다.

```hlsl
// 인코딩 (SummedAreaTextureFilter.hlsl): float [0,1] → uint
uint EncodedValue = uint(round(FloatDepth * float(DEPTH_SCALE)));

// 디코딩 + 차분 (UberLit.hlsl): uint 비트 패턴으로 복원 후 정수 연산
uint2 D = asuint(SATTexture.Load(...).xy);
uint2 B = asuint(SATTexture.Load(...).xy);
uint2 C = asuint(SATTexture.Load(...).xy);
uint2 A = asuint(SATTexture.Load(...).xy);
float2 Moments = float2(D - B - C + A) / (Area * SAT_DEPTH_SCALE); // 마지막에만 float 변환
```

`asfloat(uint)` / `asuint(float)`로 float 텍스쳐에 uint 비트 패턴을 저장·복원하고, 차분 연산만 uint로 수행해 정밀도 손실 없이 정확한 모멘트를 얻는다.

---

## Performance

![performance][img-performance]

**커널 크기별 Total Frame Time (ms)**

| Shadow Mode | 3×3 | 7×7 | 11×11 | 15×15 |
|-------------|-----|-----|-------|-------|
| PCF | 0.73 | 1.27 | 2.54 | **11.04** |
| VSM + Box | 1.66 | 1.68 | 2.01 | 2.64 |
| VSM + Gaussian | 1.67 | 1.70 | 2.07 | 2.70 |
| SAVSM | 3.49 | **3.41** | **3.41** | **3.41** |

- **PCF**: 라이팅 패스에서 픽셀당 k² 샘플을 수행. 샘플 수 25배(3×3→15×15) 증가 시 GPU 시간 **26배** 증가 — O(k²) 이론과 일치
- **VSM**: 분리 가능 컨볼루션(Separable filter)으로 필터 복잡도를 O(k)로 낮추고 라이팅 패스는 O(1)로 고정. **11×11부터 PCF보다 빠르며, 15×15에서 4.2배 우위**
- **SAVSM**: SAT 구성 비용이 해상도에만 의존하므로 커널 크기와 관계없이 **~3.4ms 일정 유지**

---

## References

- Donnelly & Lauritzen, **Variance Shadow Maps**, I3D 2006
- Lauritzen, **Summed-Area Variance Shadow Maps**, GPU Gems 3, Chapter 8, 2007
- Hensley et al., **Fast Summed-Area Table Generation and its Applications**, Eurographics 2005

---

[← Week 07][link-week07] | [Week 09 →][link-week09]

<!-- 이미지 레퍼런스 -->
[img-preview]:        Docs/Images/main.png
[img-pipeline]:       Docs/Images/pipeline.png
[img-comparison]:     Docs/Images/pcf-vsm-savsm.png
[img-savsm-artifact]: Docs/Images/savsm-artifact.png
[img-performance]:    Docs/Images/performance.png

<!-- 링크 레퍼런스 -->
[link-week07]: https://github.com/geb0598/DX-Engine/tree/week-07
[link-week09]: https://github.com/geb0598/DX-Engine/tree/week-09
