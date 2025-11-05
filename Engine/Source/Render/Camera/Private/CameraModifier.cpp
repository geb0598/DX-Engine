#include "pch.h"
#include "Render/Camera/Public/CameraModifier.h"
#include "Actor/Public/PlayerCameraManager.h"
#include "Actor/Public/Actor.h"
#include "Global/CurveTypes.h"
#include <algorithm>

IMPLEMENT_CLASS(UCameraModifier, UObject)

UCameraModifier::UCameraModifier()
{
}

UCameraModifier::~UCameraModifier() = default;

void UCameraModifier::AddedToCamera(APlayerCameraManager* Camera)
{
	CameraOwner = Camera;
}

bool UCameraModifier::ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV)
{
	// Unpack view info for native modify function
	FVector ViewLocation = InOutPOV.Location;
	FQuaternion ViewRotation = InOutPOV.Rotation;
	float FOV = InOutPOV.FOV;

	FVector NewViewLocation = ViewLocation;
	FQuaternion NewViewRotation = ViewRotation;
	float NewFOV = FOV;

	// Call protected modify camera (override point for derived classes)
	ModifyCamera(DeltaTime, ViewLocation, ViewRotation, FOV, NewViewLocation, NewViewRotation, NewFOV);

	// Apply alpha blending
	if (Alpha < 1.0f)
	{
		// Blend location
		InOutPOV.Location = FVector::Lerp(ViewLocation, NewViewLocation, Alpha);

		// Blend rotation (quaternion slerp)
		InOutPOV.Rotation = FQuaternion::Slerp(ViewRotation, NewViewRotation, Alpha);

		// Blend FOV
		InOutPOV.FOV = FOV + (NewFOV - FOV) * Alpha;
	}
	else
	{
		InOutPOV.Location = NewViewLocation;
		InOutPOV.Rotation = NewViewRotation;
		InOutPOV.FOV = NewFOV;
	}

	// Modify post-process settings (if camera owner exists)
	if (CameraOwner)
	{
		float PostProcessBlendWeight = 1.0f;
		FPostProcessSettings& PostProcessSettings = CameraOwner->GetPostProcessSettings();
		ModifyPostProcess(DeltaTime, PostProcessBlendWeight, PostProcessSettings);

		// Apply alpha to post-process blend weight
		PostProcessBlendWeight *= Alpha;
	}

	return false; // Continue modifier chain by default
}

void UCameraModifier::ModifyCamera(float DeltaTime, FVector ViewLocation, FQuaternion ViewRotation, float FOV,
                                    FVector& NewViewLocation, FQuaternion& NewViewRotation, float& NewFOV)
{
	// Base implementation does nothing
	// Override in derived classes
	NewViewLocation = ViewLocation;
	NewViewRotation = ViewRotation;
	NewFOV = FOV;
}

void UCameraModifier::ModifyPostProcess(float DeltaTime, float& PostProcessBlendWeight, FPostProcessSettings& PostProcessSettings)
{
	// Base implementation does nothing
	// Override in derived classes
}

void UCameraModifier::UpdateAlpha(float DeltaTime)
{
	float TargetAlpha = GetTargetAlpha();

	if (Alpha < TargetAlpha)
	{
		// Blend in (0 → 1)
		if (!bIsBlendingIn)
		{
			// Started new blend in
			bIsBlendingIn = true;
			AlphaBlendElapsedTime = 0.0f;
		}

		if (AlphaInTime > 0.0f)
		{
			AlphaBlendElapsedTime += DeltaTime;
			float NormalizedTime = AlphaBlendElapsedTime / AlphaInTime;
			NormalizedTime = std::min(NormalizedTime, 1.0f);

			// Evaluate curve or use linear
			if (AlphaInCurve)
			{
				Alpha = AlphaInCurve->Evaluate(NormalizedTime);
			}
			else
			{
				Alpha = NormalizedTime; // Linear
			}

			Alpha = std::min(Alpha, TargetAlpha);
		}
		else
		{
			Alpha = TargetAlpha;
		}
	}
	else if (Alpha > TargetAlpha)
	{
		// Blend out (1 → 0)
		if (bIsBlendingIn)
		{
			// Started new blend out
			bIsBlendingIn = false;
			AlphaBlendElapsedTime = 0.0f;
		}

		if (AlphaOutTime > 0.0f)
		{
			AlphaBlendElapsedTime += DeltaTime;
			float NormalizedTime = AlphaBlendElapsedTime / AlphaOutTime;
			NormalizedTime = std::min(NormalizedTime, 1.0f);

			// Evaluate curve or use linear
			float BlendValue;
			if (AlphaOutCurve)
			{
				BlendValue = AlphaOutCurve->Evaluate(NormalizedTime);
			}
			else
			{
				BlendValue = NormalizedTime; // Linear
			}

			// Blend from 1 to 0
			Alpha = 1.0f - BlendValue;
			Alpha = std::max(Alpha, TargetAlpha);
		}
		else
		{
			Alpha = TargetAlpha;
		}

		// Auto-disable when fully blended out
		if (bPendingDisable && Alpha <= 0.0f)
		{
			DisableModifier(true);
		}
	}
	else
	{
		// Alpha == TargetAlpha, reset timer
		AlphaBlendElapsedTime = 0.0f;
	}
}

float UCameraModifier::GetTargetAlpha()
{
	// If disabled or pending disable, target alpha is 0, otherwise 1
	return (bDisabled || bPendingDisable) ? 0.0f : 1.0f;
}

void UCameraModifier::DisableModifier(bool bImmediate)
{
	if (bImmediate)
	{
		bDisabled = true;
		Alpha = 0.0f;
		bPendingDisable = false;
	}
	else
	{
		// Blend out before disabling
		bPendingDisable = true;
	}
}

void UCameraModifier::EnableModifier()
{
	bDisabled = false;
	bPendingDisable = false;

	// Reset alpha to 0 to start fresh blend-in
	Alpha = 0.0f;
	AlphaBlendElapsedTime = 0.0f;
	bIsBlendingIn = false;
}

AActor* UCameraModifier::GetViewTarget() const
{
	if (CameraOwner)
	{
		return CameraOwner->GetViewTarget();
	}
	return nullptr;
}

void UCameraModifier::SetAlphaInCurve(const FCurve* Curve)
{
	AlphaInCurve = Curve;
}

void UCameraModifier::SetAlphaOutCurve(const FCurve* Curve)
{
	AlphaOutCurve = Curve;
}
