#pragma once

#include "Actor.h"
#include "ALandmine.generated.h"

class UAudioComponent;
class UStaticMeshComponent;

UCLASS(DisplayName="지뢰", Description="밟으면 폭발하여 대상을 날려버리는 액터입니다")
class ALandmine : public AActor
{
public:
    GENERATED_REFLECTION_BODY()

    ALandmine();
    ~ALandmine() override;

public:
    // ===== Properties =====
    
    /** 폭발 힘 (충격량) */
    UPROPERTY(EditAnywhere, Category="Mine")
    float ExplosionImpulse = 50.0f; 

    /** 폭발 회전력 (회전 충격량) */
    UPROPERTY(EditAnywhere, Category="Mine")
    float AngularImpulseStrength = 10.0f;

    /** 위쪽으로 띄우는 힘의 비율 (0.0 ~ 1.0) */
    UPROPERTY(EditAnywhere, Category="Mine")
    float UpwardBias = 0.5f;

    /** 현재 활성화 상태 여부 */
    UPROPERTY(VisibleAnywhere, Category="Mine")
    bool bIsActive = true;

    // ===== Functions =====

    UStaticMeshComponent* GetMeshComponent() const { return MeshComponent; }

    /** 지뢰를 다시 활성화합니다. */
    void Activate();

    /** 지뢰를 비활성화합니다. (밟아도 안 터짐, 이펙트 끔 등) */
    void Deactivate();

    void DuplicateSubObjects() override;
    void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

protected:
    /** 충돌 시 호출될 콜백 함수 */
    void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

protected:
    UStaticMeshComponent* MeshComponent;

    UParticleSystemComponent* FlashingEffect;

    UParticleSystemComponent* ExplosionEffect;

    UAudioComponent* ExplosionSoundComponent;
};