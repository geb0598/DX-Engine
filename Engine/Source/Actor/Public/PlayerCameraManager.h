#pragma once
#include "Actor/Public/Actor.h"
#include "Global/CameraTypes.h"

class UCameraComponent;
class UCameraModifier;

/**
 * View Target
 * Represents a camera target (usually an Actor with a CameraComponent)
 */
struct FViewTarget
{
	/** Target Actor */
	AActor* Target = nullptr;

	/** Point of view for this target */
	FMinimalViewInfo POV;

	FViewTarget() = default;

	explicit FViewTarget(AActor* InTarget)
		: Target(InTarget)
	{
	}
};

/**
 * APlayerCameraManager
 * Manages camera for a player, including view targets
 * Simplified initial implementation - blending and modifiers will be added later
 */
UCLASS()
class APlayerCameraManager : public AActor
{
	GENERATED_BODY()
	DECLARE_CLASS(APlayerCameraManager, AActor)

public:
	APlayerCameraManager();
	virtual ~APlayerCameraManager() override;

	void BeginPlay() override;
	void Tick(float DeltaTime) override;

	/**
	 * Get the final camera view
	 * Automatically updates camera if dirty
	 */
	const FMinimalViewInfo& GetCameraCachePOV() const;

	/**
	 * Set a new view target
	 * @param NewViewTarget The new actor to view from
	 */
	void SetViewTarget(AActor* NewViewTarget);

	/**
	 * Get current view target actor
	 */
	AActor* GetViewTarget() const { return ViewTarget.Target; }

protected:
	/**
	 * Main camera update function
	 * Called every frame to update camera position and rotation
	 */
	virtual void UpdateCamera(float DeltaTime);

	/**
	 * Get the POV from the current view target
	 */
	virtual void GetViewTargetPOV(FViewTarget& OutVT);

	/**
	 * Apply camera modifiers (to be implemented)
	 */
	virtual void ApplyCameraModifiers(float DeltaTime, FMinimalViewInfo& InOutPOV);

private:
	/** Current view target */
	FViewTarget ViewTarget;

	/** Final cached camera POV */
	FMinimalViewInfo CameraCachePOV;

	/** Camera fade parameters */
	FVector4 FadeColor = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
	float FadeAmount = 0.0f;
	FVector2 FadeAlpha = FVector2(0.0f, 0.0f);
	float FadeTime = 0.0f;
	float FadeTimeRemaining = 0.0f;

	/** Camera style name (for future use) */
	FName CameraStyle;

	/** List of camera modifiers (for future use) */
	TArray<UCameraModifier*> ModifierList;

	/** Default FOV */
	float DefaultFOV = 90.0f;

	/** Default aspect ratio */
	float DefaultAspectRatio = 1.777f;

public:
	/** Get/Set default FOV */
	float GetDefaultFOV() const { return DefaultFOV; }
	void SetDefaultFOV(float InFOV) { DefaultFOV = InFOV; }

	/** Get/Set default aspect ratio */
	float GetDefaultAspectRatio() const { return DefaultAspectRatio; }
	void SetDefaultAspectRatio(float InAspectRatio)
	{
		if (DefaultAspectRatio != InAspectRatio)
		{
			DefaultAspectRatio = InAspectRatio;
			bCameraDirty = true;
		}
	}

private:
	/** Whether camera needs update */
	mutable bool bCameraDirty = true;

// Post Process

public:
	void EnableLetterBox(float InTargetAspectRatio, float InTransitionTime);
	void DisableLetterBox(float InTransitionTime);

	void SetVignetteIntensity(float InIntensity);

	const FPostProcessSettings& GetPostProcessSettings() const { return CurrentPostProcessSettings; }

private:
	void UpdatePostProcessAnimations(float DeltaTime);

	FPostProcessSettings CurrentPostProcessSettings;

	// --- LetterBox 애니메이션 상태 변수 ---
	float TargetLetterBoxAmount = 0.0f; // 목표 강도
	float LetterBoxTransitionSpeed = 1.0f; // 전환 속도
private:
};
