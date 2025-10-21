#pragma once
#include "Vector.h"
#include "ActorComponent.h"
#include"Engine.h"


// 부착 시 로컬을 유지할지, 월드를 유지할지
enum class EAttachmentRule
{
    KeepRelative,
    KeepWorld
};

class URenderer;
class USceneComponent : public UActorComponent
{
public:
    DECLARE_CLASS(USceneComponent, UActorComponent)
    USceneComponent();

protected:
    ~USceneComponent() override;


public:
    // ──────────────────────────────
    // Relative Transform API
    // ──────────────────────────────
    void SetRelativeLocation(const FVector& NewLocation);
    FVector GetRelativeLocation() const;

    void SetRelativeRotation(const FQuat& NewRotation);
    FQuat GetRelativeRotation() const;

    void SetRelativeScale(const FVector& NewScale);
    FVector GetRelativeScale() const;

    void AddRelativeLocation(const FVector& DeltaLocation);
    void AddRelativeRotation(const FQuat& DeltaRotation);
    void AddRelativeScale3D(const FVector& DeltaScale);

    // ──────────────────────────────
    // World Transform API
    // ──────────────────────────────
    FTransform GetWorldTransform() const;
    void SetWorldTransform(const FTransform& W);

    void SetWorldLocation(const FVector& L);
    FVector GetWorldLocation() const;

    void SetWorldRotation(const FQuat& R);
    FQuat GetWorldRotation() const;

    void SetWorldScale(const FVector& S);
    FVector GetWorldScale() const;

    void AddWorldOffset(const FVector& Delta);
    void AddWorldRotation(const FQuat& DeltaRot);
    void SetWorldLocationAndRotation(const FVector& L, const FQuat& R);

    void AddLocalOffset(const FVector& Delta);
    void AddLocalRotation(const FQuat& DeltaRot);
    void SetLocalLocationAndRotation(const FVector& L, const FQuat& R);

    FMatrix GetWorldMatrix() const; // ToMatrixWithScale
    /**
    * @brief 컴포넌트의 월드 공간 기준 Forward 벡터(+X)를 반환합니다.
    */
    FVector GetForwardVector() const;

    // ──────────────────────────────
    // Attach/Detach
    // ──────────────────────────────
    void SetupAttachment(USceneComponent* InParent, EAttachmentRule Rule = EAttachmentRule::KeepWorld);
    void DetachFromParent(bool bKeepWorld = true);

    // ──────────────────────────────
    // Hierarchy Access
    // ──────────────────────────────
    USceneComponent* GetAttachParent() const { return AttachParent; }
    const TArray<USceneComponent*>& GetAttachChildren() const { return AttachChildren; }
    UWorld* GetWorld() { return GEngine->GetActiveWorld(); }

    // Duplicate function
    UObject* Duplicate() override;
    void DuplicateSubObjects() override;

protected:
    void UpdateRelativeTransform();

    // Duplicate 헬퍼: 공통 속성 복사 (Transform, AttachChildren)
    /**
     * @brief 부모 속성을 선 복사 후 자신의 속성을 복사합니다.
     * @param InTarget 복사할 오브젝트
    */
    virtual void CopyCommonProperties(UObject* InTarget);

protected:
    // [PIE] 값 복사
    FVector RelativeLocation{ 0,0,0 };
    FQuat   RelativeRotation;
    FVector RelativeScale{ 1,1,1 };

    // Hierarchy
    // [PIE] 외부에서 초기화
    USceneComponent* AttachParent = nullptr;
    // [PIE] 직접 순회를 돌면서 Duplicate 복사
    TArray<USceneComponent*> AttachChildren;

    // 로컬(부모 기준) 트랜스폼
    // [PIE] 값 복사
    FTransform RelativeTransform;
};
