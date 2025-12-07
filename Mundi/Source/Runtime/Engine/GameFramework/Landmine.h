#pragma once

#include "Actor.h"
#include "ALandmine.generated.h"

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

    // ===== Functions =====

    UStaticMeshComponent* GetMeshComponent() const { return MeshComponent; }

    void DuplicateSubObjects() override;
    void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

protected:
    /** 충돌 시 호출될 콜백 함수 */
    void OnCompHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

protected:
    UStaticMeshComponent* MeshComponent;
};