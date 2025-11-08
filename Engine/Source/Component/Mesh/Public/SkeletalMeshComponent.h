#pragma once

#include "Runtime/Engine/Public/SkeletalMesh.h"
#include "SkinnedMeshComponent.h"

// class USkeletalMesh;

UCLASS()
class USkeletalMeshComponent : public USkinnedMeshComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(USkeletalMeshComponent, USkinnedMeshComponent)

private:
	/** 이 컴포넌트에 의해 사용되는 스켈레탈 메시 */
	TObjectPtr<USkeletalMesh> SkeletalMeshAsset;

	/*-----------------------------------------------------------------------------
		UObject 인터페이스
	 -----------------------------------------------------------------------------*/
public:
	virtual UObject* Duplicate() override;

	virtual void DuplicateSubObjects(UObject* DuplicatedObject) override;

	virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

	/*-----------------------------------------------------------------------------
		UActorComponent 인터페이스
	 -----------------------------------------------------------------------------*/
public:
	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime) override;

	virtual void EndPlay() override;

	/*-----------------------------------------------------------------------------
		UPrimitiveComponent 인터페이스
	 -----------------------------------------------------------------------------*/

	/** @todo 오버라이딩이 필요한 함수 확인 */

	/*-----------------------------------------------------------------------------
		USkinnedMeshComponent 인터페이스
	 -----------------------------------------------------------------------------*/
public:
	virtual void RefreshBoneTransforms() override;

	virtual void TickPose(float DeltaTime) override;

	/*-----------------------------------------------------------------------------
		USkeletalMeshComponent 인터페이스
	 -----------------------------------------------------------------------------*/
public:
	/** @brief 이 메쉬를 위해 렌더링되는 SkeletalMesh를 반환한다. */
	USkeletalMesh* GetSkeletalMeshAsset() const;

	/** @brief 이 메쉬를 위해 렌더링되는 SkeletalMesh를 설정한다. */
	void SetSkeletalMeshAsset(USkeletalMesh* NewMesh);

	/** @brief 외부(e.g., 에디터)에서 현재 본의 위치를 확인하기 위해 제공되는 함수이다. */
	FTransform GetBoneTransformLocal(int32 BoneIndex);

	/** @brief 애니메이션이 아닌 외부(e.g., 에디터, 기즈모, etc)에서 포즈를 조작하기 위해 제공되는 함수이다. */
	void SetBoneTransformLocal(int32 BoneIndex, const FTransform& NewLocalTransform);

	/** @brief CPU 스키닝을 수행한다. */
	void UpdateSkinnedVertices();

private:
	/** 포즈 및 애니메이션이 적용된 이후의 본 로컬 트랜스폼 */
	TArray<FTransform> BoneSpaceTransforms;

	/** 스키닝용 행렬 (InvBindMatrix * ComponentSpaceTransform) */
	TArray<FMatrix> SkinningMatrices;

	/** CPU 스키닝을 적용한 이후의 정점 정보 */
	TArray<FNormalVertex> SkinnedVertices;

	/** 포즈가 변경되었음을 알리는 플래그 */
	bool bPoseDirty;

	/** SkinnedVertices 갱신 필요여부를 알리는 플래그 */
	bool bSkinningDirty;
};
