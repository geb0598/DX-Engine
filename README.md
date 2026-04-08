[← Week 09][link-week09] | [Week 11 →][link-week11]

![preview][img-preview]

# DX-Engine — Week 10: Skeletal Mesh

> UE5 아키텍처를 참조한 CPU 기반 스켈레탈 메시 시스템 — 에셋-컴포넌트 분리 설계, 계층적 본 변환, 노말 스키닝

![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus&logoColor=white)
![DirectX 11](https://img.shields.io/badge/DirectX-11-0078D4?logo=microsoft&logoColor=white)
![HLSL](https://img.shields.io/badge/HLSL-SM_5.0-5C2D91)
![Windows](https://img.shields.io/badge/Windows-0078D6?logo=windows&logoColor=white)

---

## Features

- **UE5 스타일 에셋-컴포넌트 분리** — `USkinnedAsset` 추상 인터페이스로 에셋(불변·공유)과 런타임 상태를 분리. 동일 메시 N개 배치 시 원본 데이터 1벌만 존재
- **CPU 스키닝 파이프라인** — FBX 로딩 → 역바인드 행렬 캐싱 → 계층 합성 → 정점 스키닝 → D3D11 Dynamic 버퍼 업로드까지 완결된 파이프라인
- **2단계 더티 플래그** — `bPoseDirty` / `bSkinningDirty` 분리로 포즈 변화 없는 프레임의 연산 비용 0

---

![viewer][img-viewer]

## Key Systems

### 1. UE5 스타일 에셋-컴포넌트 아키텍처

![structure][img-structure]

`USkeletalMesh`는 FBX 로딩 시 1회 생성되어 여러 컴포넌트가 공유하는 **불변 에셋**이다. `USkeletalMeshComponent`는 씬에 배치된 인스턴스로, 포즈·스키닝 상태만 독립적으로 소유한다. 동일 메시 100개를 배치해도 원본 정점·스킨 가중치·본 계층은 메모리에 1벌만 존재한다.

```cpp
class USkinnedMeshComponent : public UMeshComponent {
    TObjectPtr<USkinnedAsset> SkinnedAsset;  // 구체 타입이 아닌 추상 타입으로 소유
};
```

`USkinnedMeshComponent`가 에셋을 구체 타입(`USkeletalMesh*`)이 아닌 추상 타입(`USkinnedAsset*`)으로 소유하기 때문에, 새로운 스킨드 에셋 타입이 추가되어도 컴포넌트 코드를 수정할 필요가 없다. UE5에서 `USkinnedAsset`의 구체 타입은 `USkeletalMesh`(일반 캐릭터)와 `UGeometryCollection`(파괴 메시) 등 여러 종류가 존재하며, 동일한 설계를 따랐다.

계층 합성 알고리즘 `FillComponentSpaceTransforms`는 `USkinnedAsset`에 구현하고, 내부에서 `GetRefSkeleton()`(순수 가상)을 호출해 파생 클래스의 데이터를 읽는다. 알고리즘은 고정하고 데이터 접근만 파생 클래스가 제공하는 **Template Method 패턴**이다.

```cpp
// LOD 확장점 — 처리할 본 목록을 외부에서 주입
void FillComponentSpaceTransforms(
    const TArray<FTransform>& InBoneSpaceTransforms,
    const TArray<FBoneIndexType>& InRequiredBones,  // 현재는 전체 본, LOD 시 필터링 목록으로 교체 가능
    TArray<FTransform>& OutComponentSpaceTransforms) const;
```

---

### 2. CPU 스키닝 파이프라인

FBX 파일 로딩부터 최종 Draw Call까지 완결된 파이프라인. 2단계 더티 플래그로 포즈 변화가 없는 프레임에서 연산을 건너뛴다.

**역바인드 행렬 — CalculateInvRefMatrices (에셋 로딩 시 1회)**

T포즈 기준 각 본의 컴포넌트 공간 행렬을 계층 합성한 뒤 역행렬을 캐싱한다. 런타임에는 배열 참조만 수행한다.

```
InvBindMatrix[i] = (T포즈 ComponentSpace[i])⁻¹
→ "T포즈 모델 공간 → 본 로컬 공간" 변환. 에셋 로딩 시 1회 계산 후 캐싱.
```

**컴포넌트 공간 합성 — FillComponentSpaceTransforms (bPoseDirty 시)**

BoneSpaceTransforms(부모 기준 로컬 트랜스폼)를 루트부터 순서대로 누적한다. 부모 본이 자식보다 앞선 배열 순서가 보장되어야 한다.

```cpp
ComponentSpace[0] = BoneSpace[0];                           // 루트
ComponentSpace[i] = BoneSpace[i] * ComponentSpace[parent];  // 자식은 부모에 누적
```

본별 스키닝 행렬과 노말용 역전치 행렬을 동시에 계산·캐싱한다.

```cpp
SkinningMatrices[i]         = RefBasesInvMatrix[i] × ComponentSpace[i].ToMatrix();
// (T포즈 → 본 공간) × (본 공간 → 현재 포즈) = T포즈 정점 → 현재 포즈 정점

InvTransSkinningMatrices[i] = SkinningMatrices[i].Inverse().Transpose();
// 노말 변환용 역전치 행렬 — 본 수(Nb) 기준 O(1) 사전 계산
```

**정점 스키닝 — UpdateSkinnedVertices (bSkinningDirty 시)**

정점마다 최대 `MAX_TOTAL_INFLUENCES`개 본의 가중치를 합산한다. 노말은 역전치 행렬로 변환해 비균등 스케일에서도 면과의 수직 관계를 보존하고, 스키닝 후 노말-탄젠트 직교성은 Gram-Schmidt로 복원한다.

```cpp
for (각 정점 i) {
    for (영향 본 j, 최대 MAX_TOTAL_INFLUENCES개) {
        Position += SkinningMatrices[j].TransformPosition(P) × w;
        Normal   += InvTransSkinningMatrices[j].TransformVector(N) × w;  // 역전치
        Tangent  += SkinningMatrices[j].TransformVector(T) × w;
    }
    Normal.Normalize();
    Tangent = Tangent - Dot(Normal, Tangent) * Normal;  // Gram-Schmidt 직교화
    Tangent.Normalize();
}
FRenderResourceFactory::UpdateVertexBufferData(VertexBuffer, SkinnedVertices);
// D3D11_USAGE_DYNAMIC 버퍼에 Map/Unmap으로 GPU 업로드
```

---

## References

- Epic Games, **Unreal Engine 5 Source — USkinnedAsset / USkeletalMeshComponent / FReferenceSkeleton**

---

[← Week 09][link-week09] | [Week 11 →][link-week11]

<!-- 이미지 레퍼런스 -->
[img-preview]:   Docs/Images/viewer-cap.png
[img-viewer]:    Docs/Images/viewer.png
[img-structure]: Docs/Images/structure.png

<!-- 링크 레퍼런스 -->
[link-week09]: https://github.com/geb0598/DX-Engine/tree/week-09
[link-week11]: https://github.com/geb0598/DX-Engine/tree/week-11
