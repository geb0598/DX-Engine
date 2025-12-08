#include "pch.h"
#include "PhysicsProjectileMovementComponent.h"
#include "Actor.h"
#include "PrimitiveComponent.h"
#include "PhysXPublic.h"
#include "ParticleSystemComponent.h"
#include "Source/Runtime/Engine/GameFramework/Vehicle.h"
#include "PhysScene.h"
#include "BodyInstance.h"
#include <random>

using namespace std;

UPhysicsProjectileMovementComponent::UPhysicsProjectileMovementComponent()
	: MaxVelocity(FVector(0.0f, 0.0f, 0.0f))
	, MinVelocity(FVector(0.0f, 0.0f, 0.0f))
	, DestroyDelay(1.0f)
	, bPendingUpdateVelocity(false)
	, PendingVelocityValue(FVector(0.0f, 0.0f, 0.0f))
	, bLoggedMissingBodyInstance(false)
{
	bCanEverTick = true;
}

void UPhysicsProjectileMovementComponent::BeginPlay()
{
	UMovementComponent::BeginPlay();

	// 자신을 소유한 액터의 RootComponent를 찾습니다.
	if (AActor* Owner = GetOwner())
	{
		if (USceneComponent* Root = Owner->GetRootComponent())
		{
			ProjectileBody = Cast<UPrimitiveComponent>(Root);
		}

		// 루트가 아니면 UpdatedComponent나 첫 번째 프리미티브를 선택
		if (!ProjectileBody)
		{
			ProjectileBody = Cast<UPrimitiveComponent>(GetUpdatedComponent());
		}
		if (!ProjectileBody)
		{
			for (USceneComponent* Comp : Owner->GetSceneComponents())
			{
				if (auto* Prim = Cast<UPrimitiveComponent>(Comp))
				{
					ProjectileBody = Prim;
					break;
				}
			}
		}
	}

	if (!ProjectileBody)
	{
		UE_LOG("[UPhysicsProjectileMovementComponent] 소유한 액터의 루트 컴포넌트나 업데이트 대상 컴포넌트로부터 UPrimitiveComponent를 찾지 못했습니다.");
		return;
	}

	// 에디터 배치 시 충돌을 피하기 위해 기본으로 끈 물리 시뮬레이션을 BeginPlay에서 켜준다.
	ProjectileBody->SetSimulatePhysics(true);

	// 충돌 이벤트 바인딩
	ProjectileBody->OnComponentHit.AddDynamic(this, &UPhysicsProjectileMovementComponent::OnHit);


	// 축별 랜덤 초기 속도 생성
	auto MakeRandomInRange = [](float Min, float Max)
	{
		if (Min > Max)
		{
			std::swap(Min, Max);
		}
		std::random_device Rd;
		std::mt19937 Gen(Rd());
		std::uniform_real_distribution<float> Dist(Min, Max);
		return Dist(Gen);
	};

	PendingVelocityValue = FVector(
		MakeRandomInRange(MinVelocity.X, MaxVelocity.X),
		MakeRandomInRange(MinVelocity.Y, MaxVelocity.Y),
		MakeRandomInRange(MinVelocity.Z, MaxVelocity.Z));

	bPendingUpdateVelocity = true;
}

void UPhysicsProjectileMovementComponent::EndPlay()
{
	ProjectileBody = nullptr;
	bPendingUpdateVelocity = false;
	bLoggedMissingBodyInstance = false;

	Super::EndPlay();
}

void UPhysicsProjectileMovementComponent::TickComponent(float DeltaTime)
{
	if (bDestroyTimerTriggered)
	{
		DestroyCountdown -= DeltaTime;
		if (DestroyCountdown <= 0.0f)
		{
			if (AActor* Owner = GetOwner())
			{
				Owner->Destroy();
			}
			bDestroyTimerTriggered = false;
		}
	}

	if (!ProjectileBody)
		return;

	if (bPendingUpdateVelocity)
	{
		if (FBodyInstance* BodyInstance = ProjectileBody->GetBodyInstance())
		{
			// 질량 오버라이드가 설정되어 있으면 반영
			if (BodyMass > 0.0f)
			{
				BodyInstance->bOverrideMass = true;
				BodyInstance->MassInKgOverride = BodyMass;

				// 이미 생성된 바디에도 질량을 재적용
				if (BodyInstance->IsDynamic() && BodyInstance->PhysScene && BodyInstance->RigidActor)
				{
					std::weak_ptr<bool> WeakHandle = BodyInstance->LifeHandle;
					FPhysScene* PhysScene = BodyInstance->PhysScene;
					PxRigidActor* RigidActor = BodyInstance->RigidActor;
					const float MassValue = BodyMass;

					PhysScene->EnqueueCommand([RigidActor, MassValue, WeakHandle]()
					{
						if (WeakHandle.expired())
						{
							return;
						}

						if (PxRigidDynamic* DynamicActor = RigidActor->is<PxRigidDynamic>())
						{
							PxRigidBodyExt::setMassAndUpdateInertia(*DynamicActor, MassValue);
							DynamicActor->wakeUp();
						}
					});
				}
			}

			BodyInstance->SetLinearVelocity(PendingVelocityValue);

			// 초기 속도 방향에 맞춰 회전 설정 (Up(0,0,1)과 속도 벡터의 외적을 회전축으로 사용)
			if (PendingVelocityValue.SizeSquared() > KINDA_SMALL_NUMBER)
			{
				const FVector Up(0.0f, 0.0f, 1.0f);
				FVector Dir = PendingVelocityValue.GetNormalized();
				FVector Axis = FVector::Cross(Up, Dir);
				float AxisLen = Axis.Size();

				if (AxisLen > KINDA_SMALL_NUMBER)
				{
					Axis.Normalize();
					float Angle = std::atan2(AxisLen, FVector::Dot(Up, Dir)); // rad
					FQuat Rot = FQuat::FromAxisAngle(Axis, Angle);
					ProjectileBody->SetWorldRotation(Rot);

					// 원하는 각속도로 지속 회전하도록 초기 각속도(토크) 부여
					if (AngularSpeed > 0.0f)
					{
						BodyInstance->AddAngularImpulse(Axis * AngularSpeed, true /*bVelChange*/);
					}
				}
			}

			bPendingUpdateVelocity = false;
			bLoggedMissingBodyInstance = false;
			UE_LOG("[UPhysicsProjectileMovementComponent] 물리 투사체 초기 속도 설정 완료");
		}
		else
		{
			if (!bLoggedMissingBodyInstance)
			{
				UE_LOG("[UPhysicsProjectileMovementComponent] 루트 컴포넌트의 바디 인스턴스가 초기화되지 않았습니다.");
				bLoggedMissingBodyInstance = true;
			}
		}
	}
}

void UPhysicsProjectileMovementComponent::DuplicateSubObjects()
{
	Super::DuplicateSubObjects();

	// 런타임 상태 초기화 (설정 값은 기본 복사에 맡김)
	ProjectileBody = nullptr;
	bPendingUpdateVelocity = false;
	PendingVelocityValue = FVector(0.0f, 0.0f, 0.0f);
	bDestroyTimerTriggered = false;
	DestroyCountdown = 0.0f;
	bLoggedMissingBodyInstance = false;
}

void UPhysicsProjectileMovementComponent::DestroyWithDelay(float Delay)
{
	// 이미 트리거가 설정된 상태면 바로 종료.
	if (bDestroyTimerTriggered)
		return;

	if (Delay <= 0.0f)
	{
		if (AActor* Owner = GetOwner())
		{
			Owner->Destroy();
		}
		return;
	}

	bDestroyTimerTriggered = true;
	DestroyCountdown = Delay;
}

void UPhysicsProjectileMovementComponent::OnHit(UPrimitiveComponent* HitComp
	, AActor* OtherActor
	, UPrimitiveComponent* OtherComp
	, FVector NormalImpulse
	, const FHitResult& Hit)
{
	// Vehicle에 힘 가하기 (프로젝타일 속도 방향으로)
	if (AVehicle* Vehicle = Cast<AVehicle>(OtherActor))
	{
		if (ProjectileBody && OtherComp && OtherComp == Vehicle->GetRootComponent())
		{
			if (FBodyInstance* MyBody = ProjectileBody->GetBodyInstance())
			{
				FVector Vel = MyBody->GetLinearVelocity();
				if (Vel.SizeSquared() > KINDA_SMALL_NUMBER)
				{
					UPrimitiveComponent* TargetPrim = Cast<UPrimitiveComponent>(OtherComp);
					if (TargetPrim)
					{
						if (FBodyInstance* TargetBody = TargetPrim->GetBodyInstance())
						{
							TargetBody->AddForce(Vel * 120.0f, true /*bAccelChange*/);
						}
					}
				}
			}
		}
	}

	if (bUseHitParticle)
	{
		if (AActor* Owner = GetOwner())
		{
			for (USceneComponent* Comp : Owner->GetSceneComponents())
			{
				if (auto* ParticleComp = Cast<UParticleSystemComponent>(Comp))
				{
					ParticleComp->ActivateSystem();
				}
			}
		}
	}

	// 충돌 시 발사체 파괴 트리거
	DestroyWithDelay(DestroyDelay);
}
