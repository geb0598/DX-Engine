[← Week 05][link-week05] | [Week 07 →][link-week07]

![preview](Docs/Images/main.gif)

# DX-Engine — Week 06

> SAT 기반 OBB 충돌 감지, 하이브리드 디퍼드 라이팅, Octree 디버깅 & 시각화, 노이즈 텍스쳐 기반 데칼 페이드 인/아웃

![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus&logoColor=white)
![DirectX 11](https://img.shields.io/badge/DirectX-11-0078D4?logo=microsoft&logoColor=white)
![HLSL](https://img.shields.io/badge/HLSL-SM_5.0-5C2D91)
![Windows](https://img.shields.io/badge/Windows-0078D6?logo=windows&logoColor=white)

---

## Features

- **SAT 기반 OBB 충돌 감지** — SAT를 활용한 OBB간 충돌 판정 자체 구현
- **하이브리드 디퍼드 포인트 라이팅** — 기존 Forward Rendering 파이프라인을 최대한 유지하며 라이팅 실험
- **Octree 디버깅 & 시각화** — 와이어프레임 시각화 도구로 타인 코드의 버그 탐색 및 수정
- **데칼 페이드 인/아웃** — 노이즈 텍스쳐 셰이더 기반 비선형 페이드 효과

---

## Key Systems

### SAT 기반 OBB 충돌 감지

데칼 프로젝션, 피킹, 물리 판정 등 엔진 전반의 공간 쿼리에서 임의 회전된 박스끼리의 교차 여부를 판정하는 핵심 충돌 감지 알고리즘.
최대 15개의 분리축을 순차적으로 검사하며, 하나의 축에서라도 분리가 확인되면 즉시 종료하는 Early Exit 패턴을 적용한다.

```cpp
bool FOBB::Intersects(const FOBB& Other) const
{
    // 두 OBB의 로컬 축 추출
    const FVector AxisLhs[] = { /* ScaleRotation 행렬의 행 벡터 3개 */ };
    const FVector AxisRhs[] = { /* Other.ScaleRotation 행렬의 행 벡터 3개 */ };

    FVector TestAxis[15];
    size_t Count = 0;

    // 1. 면 법선 6개
    TestAxis[Count++] = AxisLhs[0]; TestAxis[Count++] = AxisLhs[1]; TestAxis[Count++] = AxisLhs[2];
    TestAxis[Count++] = AxisRhs[0]; TestAxis[Count++] = AxisRhs[1]; TestAxis[Count++] = AxisRhs[2];

    // 2. Edge 교차축 최대 9개 (영벡터 제외)
    for (size_t i = 0; i < 3; ++i)
        for (size_t j = 0; j < 3; ++j)
        {
            TestAxis[Count] = AxisLhs[i].Cross(AxisRhs[j]);
            if (TestAxis[Count].LengthSquared() > DBL_EPSILON)
                ++Count;
        }

    const FVector Diff = Other.Center - Center;

    for (size_t i = 0; i < Count; ++i)
    {
        float ProjectedDist = abs(Diff.Dot(TestAxis[i]));

        float RadiusLhs =
            Extents.X * abs(AxisLhs[0].Dot(TestAxis[i])) +
            Extents.Y * abs(AxisLhs[1].Dot(TestAxis[i])) +
            Extents.Z * abs(AxisLhs[2].Dot(TestAxis[i]));

        float RadiusRhs =
            Other.Extents.X * abs(AxisRhs[0].Dot(TestAxis[i])) +
            Other.Extents.Y * abs(AxisRhs[1].Dot(TestAxis[i])) +
            Other.Extents.Z * abs(AxisRhs[2].Dot(TestAxis[i]));

        if (ProjectedDist > RadiusLhs + RadiusRhs)
            return false;  // 분리축 발견 → 즉시 종료
    }

    return true;  // 모든 축 통과 → 교차
}
```

OBB-AABB 교차는 AABB를 Identity 회전의 OBB로 변환해 OBB-OBB 경로로 위임한다.

<details>
<summary><b>Technical Details — 클릭해서 펼치기</b></summary>

<br>

**SAT → Octree 공간 쿼리 즉시 연계**

구현 완료 후 DecalPass의 프리미티브 탐색에 바로 활용했다.
기존에는 씬 전체 프리미티브를 순회했지만(`O(N)`), 데칼의 OBB와 교차하지 않는 Octree 서브트리는 통째로 스킵하도록 교체했다.

```cpp
void FDecalPass::Query(FOctree* InOctree, UDecalComponent* InDecal,
                        TArray<UPrimitiveComponent*>& OutPrimitives)
{
    auto BoundingBox = static_cast<const FOBB*>(InDecal->GetBoundingBox());
    if (!BoundingBox->Intersects(InOctree->GetBoundingBox()))
        return;  // OBB-AABB 교차 없으면 서브트리 전체 스킵

    InOctree->GetAllPrimitives(OutPrimitives);

    if (!InOctree->IsLeafNode())
        for (auto Child : InOctree->GetChildren())
            Query(Child, InDecal, OutPrimitives);
}
```

</details>

---

### 하이브리드 디퍼드 포인트 라이팅

![lighting](Docs/Images/hybrid-lighting.gif)

부트캠프 일정상 week-06 주차에 라이팅 구현은 예정되어 있지 않았다. 기존 파이프라인은 포워드 렌더링만 사용했고, 별도의 라이팅 패스가 없었다. 개인적으로 라이팅을 한번 시험해보고 싶어서 추가했는데, 기존 팀 작업물에 영향을 최소화해야 했으므로 기존 포워드 패스는 그대로 두고 필요한 정보(노말)만 추가로 기록한 뒤, 포워드 렌더링이 끝난 후 라이팅을 가산하는 하이브리드 방식을 선택했다.

```
기존 포워드 파이프라인 (변경 없음)
StaticMeshPass → SceneColor RTV

추가된 라이팅 패스
StaticMeshPass → SceneColor(t0) + NormalBuffer(t1)  [MRT만 추가]
PointLightPass → G-buffer 샘플링 → 월드 좌표 역투영 → Blinn-Phong → Additive Blend
```

<details>
<summary><b>Technical Details — 클릭해서 펼치기</b></summary>

<br>

**월드 좌표 역투영**

깊이 버퍼와 InvProjection/InvView 행렬로 픽셀의 월드 좌표를 복원한다.
ImGui 패널이 있는 분할 뷰포트 환경에서도 정확하도록 `SV_Position` 기반 화면 좌표에 viewport offset 보정을 적용했다.

```hlsl
float2 ViewportUV = (ScreenPosition - Viewport.xy) / Viewport.zw;
float2 ClipPos = ViewportUV * 2.0 - 1.0;
ClipPos.y *= -1.0;  // DirectX Y축 반전
float4 ViewPos = mul(float4(ClipPos, Depth, 1.0f), InvProjection);
ViewPos /= ViewPos.w;
float3 WorldPosition = mul(ViewPos, InvView).xyz;
```

</details>

---

### Octree 디버깅 & 시각화

![octree](Docs/Images/octree.gif)

팀원의 Octree 구현에서 버그가 발생해 동작이 올바르지 않았다. 문제를 특정하기 위해 Octree 구조를 에디터에서 실시간으로 확인할 수 있는 와이어프레임 시각화 도구를 먼저 제작했고, 이를 통해 버그를 찾아 수정했다.

**시각화 구조**: `SF_Octree` ShowFlag → `TraverseOctree()` DFS → 각 노드의 AABB를 `UBoundingBoxLines`로 변환 → BatchLines 동적 버퍼에 합산 → MainBar "Octree 표시" 메뉴로 토글

**발견한 버그**: 비리프 노드의 프리미티브를 수집하지 않는 문제와 `Remove()` 시 비리프 노드의 Primitives를 확인하지 않는 문제를 swap-and-pop 방식으로 수정했다.

---

### 데칼 페이드 인/아웃

초기에는 단순 선형 alpha 감소로 구현했다가, 노이즈 텍스처를 마스크로 활용하는 Threshold Dissolve 방식으로 업그레이드했다. FadeTexture의 픽셀 그레이스케일 값이 임계값 역할을 해서, FadeProgress가 해당 픽셀의 밝기를 넘어서는 순간 그 픽셀이 먼저 소멸한다. 어두운 픽셀일수록 먼저 사라지는 비선형 효과로, 단순 alpha fade보다 시각적으로 풍부한 연출을 텍스처 한 장으로 달성한다.

```hlsl
// Before: 단순 선형 alpha
DecalColor.a *= (1.0f - FadeProgress);

// After: Threshold Dissolve — 픽셀마다 사라지는 타이밍이 다름
float FadeValue = FadeTexture.Sample(FadeSampler, DecalUV).r;
DecalColor.a *= 1.0f - saturate(FadeProgress / (FadeValue + 1e-6));  // 1e-6: divide-by-zero 방지
if (DecalColor.a < 0.001f) { discard; }
```

Perlin Noise, Voronoi, Gabor, Techno 등 11종의 노이즈 텍스처를 마스크로 지원해 연출 목적에 따라 다양한 Dissolve 패턴을 선택할 수 있다.

![fade-in-out](Docs/Images/fade-in-out.gif)

페이드 완료 시 `bIsPendingDestroy` 플래그로 지연 삭제해 틱 중 iterator 무효화를 방지한다.

---

[← Week 05][link-week05] | [Week 07 →][link-week07]

<!-- 링크 레퍼런스 -->
[link-week05]: https://github.com/geb0598/DX-Engine/tree/week-05
[link-week07]: https://github.com/geb0598/DX-Engine/tree/week-07
