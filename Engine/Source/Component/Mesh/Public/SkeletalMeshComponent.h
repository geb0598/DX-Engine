#pragma once

#include "SkinnedMeshComponent.h"

class USkeletalMesh;

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
	/**
	 * @brief 이 메쉬를 위해 렌더링되는 SkeletalMesh를 반환한다.
	 */
	USkeletalMesh* GetSkeletalMeshAsset() const;

	// void SetSkeletalMeshAsset(USkeletalMesh* NewMesh)
};
