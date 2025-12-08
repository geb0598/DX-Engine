#pragma once
#include "Actor.h"
#include "ANPC.generated.h"

class UAudioComponent;
class USkeletalMeshComponent;
class UCapsuleComponent; 
class USound;

UCLASS(DisplayName = "NPC", Description = "월드에 배치 가능한 범용 NPC 액터")
class ANPC : public AActor
{
    GENERATED_REFLECTION_BODY()

public:
    ANPC();
    virtual ~ANPC() override;

protected:
    // ====================================================================
    // 라이프사이클
    // ====================================================================
    virtual void BeginPlay() override;
    virtual void DuplicateSubObjects() override;
    virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle);

public:
    // ====================================================================
    // 컴포넌트
    // ====================================================================
    
    /** 충돌 처리를 위한 루트 캡슐 (차량이 얘를 때림) */
    UCapsuleComponent* CapsuleComponent;

    /** 비주얼을 담당하는 스켈레탈 메쉬 */
    USkeletalMeshComponent* MeshComponent;

    /** 충돌 시 발생할 소리 컴포넌트 */
    UAudioComponent* HitSoundComponent;

    // ====================================================================
    // 설정
    // ====================================================================
    
    /** 기본 재생할 애니메이션 경로 */
    UPROPERTY(EditAnywhere, Category = "NPC")
    FString DefaultAnimPath;

    /** 처음에 재생할 애니메이션 */
    class UAnimSequence* IdleAnimation;

    /** 충돌 시 소리 */
    UPROPERTY(EditAnywhere, Category = "NPC")
    FString HitSoundPath;

    /** 래그돌 전환 시 가해지는 힘 계수 */
    UPROPERTY(EditAnywhere, Category = "NPC")
    float HitImpulseMultiplier = 10.0f;

protected:
    /** 충돌 감지 콜백 */
    void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

    /** 래그돌 상태로 전환 */
    void BecomeRagdoll(const FVector& ImpactVelocity);

private:
    bool bIsRagdoll = false;
};