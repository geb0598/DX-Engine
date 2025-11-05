#include "pch.h"
#include "Actor/Public/PlayerCameraManager.h"
#include "Component/Public/CameraComponent.h"
#include "Actor/Public/Actor.h"

IMPLEMENT_CLASS(APlayerCameraManager, AActor)

APlayerCameraManager::APlayerCameraManager()
{
	// Enable tick for camera updates
	bCanEverTick = true;
}

APlayerCameraManager::~APlayerCameraManager() = default;

void APlayerCameraManager::BeginPlay()
{
	Super::BeginPlay();
}

void APlayerCameraManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateCamera(DeltaTime);
}

void APlayerCameraManager::SetViewTarget(AActor* NewViewTarget)
{
	ViewTarget.Target = NewViewTarget;
	bCameraDirty = true;
}

void APlayerCameraManager::UpdateCamera(float DeltaTime)
{
	// Get POV from view target
	GetViewTargetPOV(ViewTarget);

	// Copy to cache
	CameraCachePOV = ViewTarget.POV;

	// Apply camera modifiers (empty for now)
	ApplyCameraModifiers(DeltaTime, CameraCachePOV);

	// Update camera constants for rendering
	CameraCachePOV.UpdateCameraConstants();

	// Camera is now up to date
	bCameraDirty = false;
}

const FMinimalViewInfo& APlayerCameraManager::GetCameraCachePOV() const
{
	// Update camera if dirty
	if (bCameraDirty)
	{
		const_cast<APlayerCameraManager*>(this)->UpdateCamera(0.0f);
	}
	return CameraCachePOV;
}

void APlayerCameraManager::GetViewTargetPOV(FViewTarget& OutVT)
{
	// If we have a view target, try to get camera component from it
	if (OutVT.Target)
	{
		// Find all camera components on the view target
		TArray<UCameraComponent*> CameraComponents = OutVT.Target->GetComponentsByClass<UCameraComponent>();

		// Find first active camera component
		UCameraComponent* ActiveCameraComp = nullptr;
		for (UCameraComponent* CameraComp : CameraComponents)
		{
			if (CameraComp && CameraComp->IsActive())
			{
				ActiveCameraComp = CameraComp;
				break;
			}
		}

		if (ActiveCameraComp)
		{
			// Get camera view from active component
			OutVT.POV = ActiveCameraComp->GetCameraView();
		}
		else
		{
			// No active camera component, use actor's transform as fallback
			OutVT.POV.Location = OutVT.Target->GetActorLocation();
			OutVT.POV.Rotation = OutVT.Target->GetActorRotation();
			OutVT.POV.FOV = DefaultFOV;
			OutVT.POV.AspectRatio = DefaultAspectRatio;
		}
	}
	else
	{
		// No view target, use defaults
		OutVT.POV.FOV = DefaultFOV;
		OutVT.POV.AspectRatio = DefaultAspectRatio;
	}
}

void APlayerCameraManager::ApplyCameraModifiers(float DeltaTime, FMinimalViewInfo& InOutPOV)
{
	// TODO: Implement camera modifiers
	// For now, just apply fade amount if set
	InOutPOV.FadeAmount = FadeAmount;
	InOutPOV.FadeColor = FadeColor;
}
