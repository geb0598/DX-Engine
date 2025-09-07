#pragma once

#include "Math/Math.h"

class URayCaster
{
private:
	URayCaster() = default;
	~URayCaster() = default;

	struct Ray
	{
		FVector4 Point;
		FVector4 Vector;
	};

	Ray GetRayFromMouseAndObjectLocation(int X, int Y, FMatrix Projection, FMatrix View, FMatrix Modeling)
	{
		Ray Ray;

		Ray.Point = FVector4(static_cast<float>(X), static_cast<float>(Y), 0.0f, 1.0f);
		Ray.Vector = FVector4(0.0f, 0.0f, 1.0f, 0.0f);

		//Ray.Point = Ray.Point * Projection.Inverse() * View.Inverse() * Modeling.Inverse();
		//Ray.Vector = Ray.Vector * Projection.Inverse() * View.Inverse() * Modeling.Inverse();

		return Ray;
	}

	bool RayCastToSphere(const Ray &Ray, float Radius)
	{
		// Components for Quadratic Formula
		FVector4 V = Ray.Vector;
		FVector4 P = Ray.Point;

		float A = V.Dot(V);
		float B = 2.0f * P.Dot(V);
		float C = P.Dot(P) - Radius * Radius;

		// Discriminant
		float D = B * B - 4 * A * C;

		if (D >= 0.0f)
			return true;
		else
			return false;
	}

	bool RayCastToCylinder(const Ray& Ray, float Radius, float Height)
	{
		FVector4 P = Ray.Point;
		FVector4 V = Ray.Vector;
		float T1, T2;
		
		/* Calculate for cylinder side first */
		// Remain X Y Component Only
		FVector4 PXY = FVector4(Ray.Point.X, Ray.Point.Y);
		FVector4 VXY = FVector4(Ray.Vector.X, Ray.Vector.Y);

		// Components for Quadratic Formula
		float A = VXY.Dot(VXY);
		float B = 2.0f * PXY.Dot(VXY);
		float C = PXY.Dot(PXY) - Radius * Radius;

		// Discriminant
		float D = B * B - 4 * A * C;

		if (D < 0.0f)
			return false;

		T1 = (-B + std::sqrtf(D)) / 2 * A;
		T2 = (-B - std::sqrtf(D)) / 2 * A;

		// Check that Z is in valid range
		float Z1 = Ray.Point.Z + Ray.Vector.Z * T1;
		float Z2 = Ray.Point.Z + Ray.Vector.Z * T2;

		if (Z1 > 0.5f * Height && Z1 < -0.5 * Height &&
			Z2 > 0.5f * Height && Z2 < -0.5 * Height)
			return false;

		/* Calculate for cylinder cap next */
		// find t when Z value of ray is equal to height
		float PZ = Ray.Point.Z;
		float VZ = Ray.Point.Z;
		T1 = (0.5f * Height - PZ) / VZ;
		T2 = (-0.5f * Height - PZ) / VZ;

		if (T1 * T1 * V.Dot(V) + 2.0f * T1 * P.Dot(V) + P.Dot(P) - Radius * Radius > 0 &&
			T2 * T2 * V.Dot(V) + 2.0f * T2 * P.Dot(V) + P.Dot(P) - Radius * Radius > 0)
			return false;

		return true;
	}

	bool RayCastToTriangle();

	bool RayCastToTorus();

public:
	URayCaster& Instance()
	{
		static URayCaster RayCaster;
		return RayCaster;
	}
};