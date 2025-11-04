#include "pch.h"
#include "Global/CameraTypes.h"
#include "Global/Vector.h"
#include "Global/Matrix.h"

void FMinimalViewInfo::UpdateCameraConstants()
{
	// Calculate camera basis vectors
	const FVector Forward = Rotation.RotateVector(FVector::ForwardVector());
	const FVector Right = Rotation.RotateVector(FVector::RightVector());
	const FVector Up = Rotation.RotateVector(FVector::UpVector());

	// View Matrix
	CameraConstants.View = FMatrix(
		Right.X, Up.X, Forward.X, 0.0f,
		Right.Y, Up.Y, Forward.Y, 0.0f,
		Right.Z, Up.Z, Forward.Z, 0.0f,
		-Right.Dot(Location), -Up.Dot(Location), -Forward.Dot(Location), 1.0f
	);

	// Projection Matrix
	if (ProjectionMode == ECameraProjectionMode::Perspective)
	{
		// Perspective projection
		const float HalfFOVRadians = (FOV * 0.5f) * (PI / 180.0f);
		const float TanHalfFOV = tanf(HalfFOVRadians);

		CameraConstants.Projection = FMatrix(
			1.0f / (AspectRatio * TanHalfFOV), 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f / TanHalfFOV, 0.0f, 0.0f,
			0.0f, 0.0f, FarClipPlane / (FarClipPlane - NearClipPlane), 1.0f,
			0.0f, 0.0f, -(FarClipPlane * NearClipPlane) / (FarClipPlane - NearClipPlane), 0.0f
		);
	}
	else
	{
		// Orthographic projection
		const float OrthoHeight = OrthoWidth / AspectRatio;

		CameraConstants.Projection = FMatrix(
			2.0f / OrthoWidth, 0.0f, 0.0f, 0.0f,
			0.0f, 2.0f / OrthoHeight, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f / (FarClipPlane - NearClipPlane), 0.0f,
			0.0f, 0.0f, -NearClipPlane / (FarClipPlane - NearClipPlane), 1.0f
		);
	}

	CameraConstants.ViewWorldLocation = Location;
	CameraConstants.NearClip = NearClipPlane;
	CameraConstants.FarClip = FarClipPlane;
}

FMinimalViewInfo FMinimalViewInfo::Blend(
	const FMinimalViewInfo& A,
	const FMinimalViewInfo& B,
	float Alpha,
	EViewTargetBlendFunction BlendFunc)
{
	// Clamp alpha
	Alpha = (Alpha < 0.0f) ? 0.0f : (Alpha > 1.0f) ? 1.0f : Alpha;

	// Apply blend function
	float BlendedAlpha = Alpha;
	switch (BlendFunc)
	{
	case EViewTargetBlendFunction::VTBlend_Linear:
		// Already linear
		break;

	case EViewTargetBlendFunction::VTBlend_Cubic:
		BlendedAlpha = Alpha * Alpha * (3.0f - 2.0f * Alpha);
		break;

	case EViewTargetBlendFunction::VTBlend_EaseIn:
		BlendedAlpha = Alpha * Alpha;
		break;

	case EViewTargetBlendFunction::VTBlend_EaseOut:
		BlendedAlpha = 1.0f - (1.0f - Alpha) * (1.0f - Alpha);
		break;

	case EViewTargetBlendFunction::VTBlend_EaseInOut:
		BlendedAlpha = (Alpha < 0.5f)
			? 2.0f * Alpha * Alpha
			: 1.0f - 2.0f * (1.0f - Alpha) * (1.0f - Alpha);
		break;
	}

	// Blend the view info
	FMinimalViewInfo Result;

	// Lerp location
	Result.Location = FVector(
		A.Location.X + (B.Location.X - A.Location.X) * BlendedAlpha,
		A.Location.Y + (B.Location.Y - A.Location.Y) * BlendedAlpha,
		A.Location.Z + (B.Location.Z - A.Location.Z) * BlendedAlpha
	);

	// Slerp rotation (shortest path)
	Result.Rotation = FQuaternion::SlerpShortestPath(A.Rotation, B.Rotation, BlendedAlpha);

	// Lerp FOV
	Result.FOV = A.FOV + (B.FOV - A.FOV) * BlendedAlpha;

	// Lerp aspect ratio
	Result.AspectRatio = A.AspectRatio + (B.AspectRatio - A.AspectRatio) * BlendedAlpha;

	// Lerp clipping planes
	Result.NearClipPlane = A.NearClipPlane + (B.NearClipPlane - A.NearClipPlane) * BlendedAlpha;
	Result.FarClipPlane = A.FarClipPlane + (B.FarClipPlane - A.FarClipPlane) * BlendedAlpha;

	// Use projection mode from B when blending is > 0.5
	Result.ProjectionMode = (BlendedAlpha < 0.5f) ? A.ProjectionMode : B.ProjectionMode;

	// Lerp ortho width
	Result.OrthoWidth = A.OrthoWidth + (B.OrthoWidth - A.OrthoWidth) * BlendedAlpha;

	// Lerp fade amount
	Result.FadeAmount = A.FadeAmount + (B.FadeAmount - A.FadeAmount) * BlendedAlpha;

	// Lerp fade color
	Result.FadeColor = FVector4(
		A.FadeColor.X + (B.FadeColor.X - A.FadeColor.X) * BlendedAlpha,
		A.FadeColor.Y + (B.FadeColor.Y - A.FadeColor.Y) * BlendedAlpha,
		A.FadeColor.Z + (B.FadeColor.Z - A.FadeColor.Z) * BlendedAlpha,
		A.FadeColor.W + (B.FadeColor.W - A.FadeColor.W) * BlendedAlpha
	);

	// Update camera constants
	Result.UpdateCameraConstants();

	return Result;
}
