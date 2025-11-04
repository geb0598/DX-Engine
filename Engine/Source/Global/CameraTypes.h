#pragma once
#include "Global/Vector.h"
#include "Global/Matrix.h"
#include "Global/Quaternion.h"

/**
 * Camera Constants for Rendering
 * View and Projection matrices with clip planes
 */
struct FCameraConstants
{
	FCameraConstants() : NearClip(0), FarClip(0)
	{
		View = FMatrix::Identity();
		Projection = FMatrix::Identity();
	}

	FMatrix View;
	FMatrix Projection;
	FVector ViewWorldLocation;
	float NearClip;
	float FarClip;
};

/**
 * Camera Projection Mode
 */
enum class ECameraProjectionMode : uint8
{
	Perspective,
	Orthographic
};

/**
 * View Target Blend Function
 */
enum class EViewTargetBlendFunction : uint8
{
	VTBlend_Linear,
	VTBlend_Cubic,
	VTBlend_EaseIn,
	VTBlend_EaseOut,
	VTBlend_EaseInOut
};

/**
 * Camera Shake Play Space
 */
enum class ECameraShakePlaySpace : uint8
{
	CameraLocal,
	World,
	UserDefined
};

/**
 * Minimal View Info for Camera
 * Contains essential camera parameters for rendering
 */
struct FMinimalViewInfo
{
	/** Camera location (world space) */
	FVector Location = FVector::ZeroVector();

	/** Camera rotation (world space) */
	FQuaternion Rotation = FQuaternion::Identity();

	/** Field of view (degrees) */
	float FOV = 90.0f;

	/** Desired aspect ratio */
	float AspectRatio = 1.777f;

	/** Near clipping plane */
	float NearClipPlane = 10.0f;

	/** Far clipping plane */
	float FarClipPlane = 10000.0f;

	/** Projection mode */
	ECameraProjectionMode ProjectionMode = ECameraProjectionMode::Perspective;

	/** Ortho width */
	float OrthoWidth = 512.0f;

	/** Cached camera constants (generated from above) */
	FCameraConstants CameraConstants;

	/** Fade amount (0=transparent, 1=fully faded) */
	float FadeAmount = 0.0f;

	/** Fade color (RGBA) */
	FVector4 FadeColor = FVector4(0.0f, 0.0f, 0.0f, 1.0f);

	/**
	 * Generate camera constants from this POV
	 */
	void UpdateCameraConstants();

	/**
	 * Blend between two POVs
	 */
	static FMinimalViewInfo Blend(
		const FMinimalViewInfo& A,
		const FMinimalViewInfo& B,
		float Alpha,
		EViewTargetBlendFunction BlendFunc = EViewTargetBlendFunction::VTBlend_Linear
	);
};
