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

// ============================================
// Post Process Effects
// ============================================
public:
	/**
	 * Enable letterbox with cinematic aspect ratio
	 * @param InTargetAspectRatio Target aspect ratio (e.g., 2.35 for cinemascope)
	 * @param InTransitionTime Time to fade in the letterbox
	 */
	void EnableLetterBox(float InTargetAspectRatio, float InTransitionTime);

	/**
	 * Disable letterbox effect
	 * @param InTransitionTime Time to fade out the letterbox
	 */
	void DisableLetterBox(float InTransitionTime);

	/**
	 * Set vignette intensity
	 * @param InIntensity Vignette intensity (0.0 = none, 1.0 = full)
	 */
	void SetVignetteIntensity(float InIntensity);

	/**
	 * Get current post process settings
	 */
	const FPostProcessSettings& GetPostProcessSettings() const { return CurrentPostProcessSettings; }

// ============================================
// Camera Fade
// ============================================
public:
	/**
	 * Fade camera to/from a color
	 * @param FromAlpha Starting alpha (0.0 = clear, 1.0 = opaque)
	 * @param ToAlpha Ending alpha
	 * @param Duration Fade duration in seconds
	 * @param Color Fade color (RGB)
	 * @param bHoldWhenFinished If true, holds at ToAlpha after fade completes
	 */
	void StartCameraFade(float FromAlpha, float ToAlpha, float Duration, FVector Color, bool bHoldWhenFinished = false);

	/**
	 * Stop camera fade immediately
	 * @param bStopAtCurrent If true, stays at current alpha. If false, goes to 0.0
	 */
	void StopCameraFade(bool bStopAtCurrent = false);

	/**
	 * Set camera fade manually (no animation)
	 * @param InFadeAmount Alpha value (0.0 = clear, 1.0 = opaque)
	 * @param InFadeColor Fade color (RGB)
	 */
	void SetManualCameraFade(float InFadeAmount, FVector InFadeColor);

	/**
	 * Get current fade amount
	 */
	float GetCameraFadeAmount() const { return FadeAmount; }

	/**
	 * Get current fade color
	 */
	FVector GetCameraFadeColor() const { return FVector(FadeColor.X, FadeColor.Y, FadeColor.Z); }

	/**
	 * Check if camera is currently fading
	 */
	bool IsFading() const { return FadeTimeRemaining > 0.0f; }

private:
	void UpdatePostProcessAnimations(float DeltaTime);
	void UpdateCameraFade(float DeltaTime);

	FPostProcessSettings CurrentPostProcessSettings;

	// --- LetterBox Animation ---
	float TargetLetterBoxAmount = 0.0f;
	float LetterBoxTransitionSpeed = 1.0f;

	// --- Camera Fade ---
	FVector4 FadeColor = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
	float FadeAmount = 0.0f;
	float FadeStartAlpha = 0.0f;
	float FadeEndAlpha = 0.0f;
	float FadeDuration = 0.0f;
	float FadeTimeRemaining = 0.0f;
	bool bHoldFadeWhenFinished = false;
};
