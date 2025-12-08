#pragma once
#include "MovementComponent.h"
#include "UPhysicsProjectileMovementComponent.generated.h"

class UPrimitiveComponent;

UCLASS(DisplayName = "물리 투사체 이동 컴포넌트", Description = "물리 기반의 투사체 이동을 처리하는 컴포넌트입니다.")
class UPhysicsProjectileMovementComponent : public UMovementComponent
{
public:
	GENERATED_REFLECTION_BODY()

	UPhysicsProjectileMovementComponent();

protected:
	~UPhysicsProjectileMovementComponent() override = default;

public:
	void BeginPlay() override;
	void EndPlay() override;
	void TickComponent(float DeltaTime) override;
	void DuplicateSubObjects() override;

	// DestroyDelay 초만큼 대기 후 소유 액터를 Destroy() 합니다.
	void DestroyWithDelay(float Delay);

	// 초기 속도는 X, Y, Z 값 각각에 대해 [Min, Max] 범위 내에서 정해집니다.

	UPROPERTY(EditAnywhere, Category = "Projectile Movement")
	FVector MinVelocity = FVector(0.0f, 0.0f, 0.0f);

	UPROPERTY(EditAnywhere, Category = "Projectile Movement")
	FVector MaxVelocity = FVector(0.0f, 0.0f, 0.0f);

	UPROPERTY(EditAnywhere, Category = "Projectile Movement")
	float DestroyDelay = 1.0f; // 충돌 발생 후 이 시간	(초) 후에 발사체를 파괴

	UPROPERTY(EditAnywhere, Category = "Projectile Movement")
	bool bUseHitParticle = false; // 충돌 시 파티클 이펙트 재생 여부

	UPROPERTY(EditAnywhere, Category = "Projectile Movement")
	float AngularSpeed = 10.0f; // 초기 속도에 수직인 축으로 회전시킬 각속도(라디안/초), 0이면 회전 안 함

	UPROPERTY(EditAnywhere, Category = "Projectile Movement")
	float BodyMass = 100.0f; // kg 단위 질량 오버라이드 (0 이하면 무시)

protected:
	/** 충돌 시 호출될 콜백 함수 */
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UPrimitiveComponent* ProjectileBody = nullptr;

	bool bPendingUpdateVelocity = false; //Projectile의 BodyInstance가 만들어지지 않았을 경우를 대비해 대기
	FVector PendingVelocityValue = FVector(0.0f, 0.0f, 0.0f);

	bool bDestroyTimerTriggered = false;
	float DestroyCountdown = 0.0f;

	bool bLoggedMissingBodyInstance = false;
};
