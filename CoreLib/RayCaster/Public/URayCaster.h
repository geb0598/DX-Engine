#pragma once

#include <algorithm>

#include "Math/Math.h"
#include "Containers/Containers.h"

#define DONT_INTERSECT -1.0f

class URayCaster
{
private:
	struct Ray
	{
		FVector Point;
		FVector Vector;
	};
private:
	Ray CurrentRay;
private:
	URayCaster() = default;
	~URayCaster() = default;

public:
	void SetRayWithMouseAndMVP(int X, int Y, FMatrix Modeling, FMatrix View, FMatrix Projection)
	{
		const float SCREENWIDTH = 1000.0f;
		const float SCREENHEIGHT = 1000.0f;

		float NDCX = 2.0f * X / SCREENWIDTH - 1.0f;
		float NDCY = 1.0f - 2.0f * Y / SCREENHEIGHT;

		// near/far in NDC
		FMatrix InverseMVP = (Modeling * View * Projection).Inverse();
		FVector4 NDCNear(NDCX, NDCY, 0.0f, 1.0f); // DirectX ��Ÿ�� (0~1)
		FVector4 NDCFar(NDCX, NDCY, 1.0f, 1.0f);

		//Ray.Point = Ray.Point * Projection.Inverse() * View.Inverse() * Modeling.Inverse();
		//Ray.Vector = Ray.Vector * Projection.Inverse() * View.Inverse() * Modeling.Inverse();

		// World space points
		FVector4 WorldNear = NDCNear * InverseMVP;
		FVector4 WorldFar = NDCFar * InverseMVP;

		WorldNear /= WorldNear.W;
		WorldFar /= WorldFar.W;

		// Ray
		// change point and vector because world is mirrored
		CurrentRay.Point = WorldNear.ToVector3();
		CurrentRay.Vector = (WorldFar - WorldNear).ToVector3().GetNormalized();
		return;
	}

	float RayCastToSphere(float Radius)
	{
		// Components for Quadratic Formula
		FVector V = CurrentRay.Vector;
		FVector P = CurrentRay.Point;

		float A = V.Dot(V);
		float B = 2.0f * P.Dot(V);
		float C = P.Dot(P) - Radius * Radius;

		// Discriminant
		float D = B * B - 4 * A * C;

		if (D >= 0.0f)
			return DONT_INTERSECT;

		float T1 = (-B + std::sqrtf(D)) / 2.0f * A;
		float T2 = (-B - std::sqrtf(D)) / 2.0f * A;

		if (T1 < 0.0f && T2 < 0.0f)
			return DONT_INTERSECT;
		else if (T1 >= 0.0f && T2 < 0.0f)
			return T1;
		else if (T1 < 0.0f && T2 >= 0.0f)
			return T2;
		else
			return (std::min)(T1, T2);
	}

	float RayCastToCube()
	{
		float XHalf = 0.5f;
		float YHalf = 0.5f;
		float ZHalf = 0.5f;

		FVector P = CurrentRay.Point;
		FVector V = CurrentRay.Vector;

		// when x = +-cube's x len * 0.5
		float T1 = (XHalf - P.X) / V.X;
		float T2 = (-XHalf - P.X) / V.X;

		// is point in valid yz range?
		if (P.Y + T1 * V.Y < -YHalf || P.Y + T1 * V.Y > YHalf)
			T1 = DONT_INTERSECT;
		if (P.Z + T1 * V.Z < -ZHalf || P.Z + T1 * V.Z > ZHalf)
			T1 = DONT_INTERSECT;
		if (T1 < 0.0f)
			T1 = DONT_INTERSECT;
		if (P.Y + T2 * V.Y < -YHalf || P.Y + T2 * V.Y > YHalf)
			T2 = DONT_INTERSECT;
		if (P.Z + T2 * V.Z < -ZHalf || P.Z + T2 * V.Z > ZHalf)
			T2 = DONT_INTERSECT;
		if (T2 < 0.0f)
			T2 = DONT_INTERSECT;

		// when y = +-cube's y len * 0.5
		float T3 = (YHalf - P.Y) / V.Y;
		float T4 = (-YHalf - P.Y) / V.Y;

		// is point in valid xz range?
		if (P.X + T3 * V.X < -XHalf || P.X + T3 * V.X > XHalf)
			T3 = DONT_INTERSECT;
		if (P.Z + T3 * V.Z < -ZHalf || P.Z + T3 * V.Z > ZHalf)
			T3 = DONT_INTERSECT;
		if (T3 < 0.0f)
			T3 = DONT_INTERSECT;
		if (P.X + T4 * V.X < -XHalf || P.X + T4 * V.X > XHalf)
			T4 = DONT_INTERSECT;
		if (P.Z + T4 * V.Z < -ZHalf || P.Z + T4 * V.Z > ZHalf)
			T4 = DONT_INTERSECT;
		if (T4 < 0.0f)
			T4 = DONT_INTERSECT;

		// when z = +-cube's z len * 0.5
		float T5 = (ZHalf - P.Z) / V.Z;
		float T6 = (-ZHalf - P.Z) / V.Z;

		// is point in valid xy range?
		if (P.X + T5 * V.X < -XHalf || P.X + T5 * V.X > XHalf)
			T5 = DONT_INTERSECT;
		if (P.Y + T5 * V.Y < -YHalf || P.Y + T5 * V.Y > YHalf)
			T5 = DONT_INTERSECT;
		if (T5 < 0.0f)
			T5 = DONT_INTERSECT;
		if (P.X + T6 * V.X < -XHalf || P.X + T6 * V.X > XHalf)
			T6 = DONT_INTERSECT;
		if (P.Y + T6 * V.Y < -YHalf || P.Y + T6 * V.Y > YHalf)
			T6 = DONT_INTERSECT;
		if (T6 < 0.0f)
			T6 = DONT_INTERSECT;

		TArray<float> Answers;

		Answers.push_back(T1);
		Answers.push_back(T2);
		Answers.push_back(T3);
		Answers.push_back(T4);
		Answers.push_back(T5);
		Answers.push_back(T6);

		float Closest = DONT_INTERSECT;

		for (int i = 0; i < 6; i++)
		{
			if (Answers[i] == DONT_INTERSECT)
				continue;

			if (Closest == DONT_INTERSECT)
				Closest = Answers[i];
			else
				Closest = (std::min)(Closest, Answers[i]);
		}

		return Closest;
	}

	float RayCastToTriangle()
	{
		FVector P = CurrentRay.Point;
		FVector V = CurrentRay.Vector;

		// Points of Triangle is fixed in Object Space
		FVector T0(0.0f, 1.0f, 0.0f);
		FVector T1(1.0f, -1.0f, 0.0f);
		FVector T2(-1.0f, -1.0f, 0.0f);

		// Normal of Triangle
		FVector N = (T1 - T0).Cross(T2 - T0);
		
		// resolve floating point error
		const float EPSILON = 1e-5f;

		// when ray is parellel to triangle
		if (V.Dot(N) >= -EPSILON && V.Dot(N) <= EPSILON)
			return DONT_INTERSECT;

		// find t when ray intersect with Plane.
		// Plane includes triangle
		float T = -(P - T0).Dot(N) / V.Dot(N);

		// when triangle is behind camera
		if (T < 0.0f)
			return DONT_INTERSECT;

		// R is intersection point between ray and plane
		FVector R = P + T * V;

		if ((T1 - T0).Cross(R - T0).Dot(N) < 0.0f ||
			(T2 - T1).Cross(R - T1).Dot(N) < 0.0f ||
			(T0 - T2).Cross(R - T2).Dot(N) < 0.0f)
			return DONT_INTERSECT;

		return T;
	}

	float GetCloserT(float T1, float T2)
	{
		if (T1 == DONT_INTERSECT && T2 == DONT_INTERSECT)
			return DONT_INTERSECT;
		else if (T1 == DONT_INTERSECT && T2 != DONT_INTERSECT)
			return T2;
		else if (T1 != DONT_INTERSECT && T2 == DONT_INTERSECT)
			return T1;
		else
			return (std::min)(T1, T2);
	}

	// this function return closest intersection between cylinder side and ray
	float RayCastToCylinderSide(float Radius, float Height)
	{
		const float HalfHeight = Height * 0.5f;

		FVector P = CurrentRay.Point;
		FVector V = CurrentRay.Vector;
		float T1, T2;

		// Remain X Y Component Only
		FVector PXY = FVector(CurrentRay.Point.X, CurrentRay.Point.Y);
		FVector VXY = FVector(CurrentRay.Vector.X, CurrentRay.Vector.Y);

		// Components for Quadratic Formula
		float A = VXY.Dot(VXY);
		float B = 2.0f * PXY.Dot(VXY);
		float C = PXY.Dot(PXY) - Radius * Radius;

		// Discriminant
		float D = B * B - 4 * A * C;

		if (D < 0.0f)
			return DONT_INTERSECT;

		T1 = (-B + std::sqrtf(D)) / 2.0f * A;
		T2 = (-B - std::sqrtf(D)) / 2.0f * A;

		if (T1 > 0.0f)
		{
			// Check that Z is in valid range
			float Z1 = CurrentRay.Point.Z + CurrentRay.Vector.Z * T1;
			if (Z1 > HalfHeight || Z1 < -HalfHeight)
				T1 = DONT_INTERSECT;
		}
		else
			T1 = DONT_INTERSECT;

		if (T2 > 0.0f)
		{
			// Check that Z is in valid range
			float Z2 = CurrentRay.Point.Z + CurrentRay.Vector.Z * T2;
			if (Z2 > HalfHeight || Z2 < -HalfHeight)
				T2 = DONT_INTERSECT;
		}
		else
			T2 = DONT_INTERSECT;

		return GetCloserT(T1, T2);
	}

	float RayCastToCylinderCap(float Radius, float Height)
	{
		const float HalfHeight = Height * 0.5f;

		FVector P = CurrentRay.Point;
		FVector V = CurrentRay.Vector;

		// Find t when Z value of ray is equal to half height
		float PZ = CurrentRay.Point.Z;
		float VZ = CurrentRay.Point.Z;
		float T1 = (HalfHeight - PZ) / VZ;
		float T2 = (HalfHeight - PZ) / VZ;

		if (T1 > 0.0f)
		{
			if (T1 * T1 * V.Dot(V) + 2.0f * T1 * P.Dot(V) + P.Dot(P) - Radius * Radius > 0.0f)
				T1 = DONT_INTERSECT;
		}
		else
			T1 = DONT_INTERSECT;

		if (T2 > 0.0f)
		{
			if (T2 * T2 * V.Dot(V) + 2.0f * T2 * P.Dot(V) + P.Dot(P) - Radius * Radius > 0.0f)
				T2 = DONT_INTERSECT;
		}
		else
			T2 = DONT_INTERSECT;

		return GetCloserT(T1, T2);
	}

	float RayCastToCylinder(float Radius, float Height)
	{
		 float TSide = RayCastToCylinderSide(Radius, Height);
		 float TCap = RayCastToCylinderCap(Radius, Height);

		 return GetCloserT(TSide, TCap);
	}

	float RayCastToAnnulus(float InnerRadius, float OuterRadius, float Height)
	{
		float A, B, CInnerRadius, COuterRadius, D;

		// Remain Z Component only
		float PZ = CurrentRay.Point.Z;
		float VZ = CurrentRay.Vector.Z;

		const float EPSILON = 1e-5f;
		// when ray is parellel to annulus
		if (VZ >= -EPSILON && VZ <= EPSILON)
			return DONT_INTERSECT;

		// T value of ray equation when ray's z equal to annulus z position
		float T = -PZ / VZ;

		// when annulus is behind camera
		if (T < 0.0f)
			return DONT_INTERSECT;

		// Remain XY Component only
		FVector PXY = FVector(CurrentRay.Point.X, CurrentRay.Point.Y);
		FVector VXY = FVector(CurrentRay.Vector.X, CurrentRay.Vector.Y);

		// Components for Quadratic Formula
		A = VXY.Dot(VXY);
		B = 2.0f * PXY.Dot(VXY);
		COuterRadius = PXY.Dot(PXY) - OuterRadius * OuterRadius;

		// Discriminant
		D = B * B - 4 * A * COuterRadius;

		// When there is no intersect
		if (D < 0.0f)
			return DONT_INTERSECT;
		if (A * T * T + B * T + COuterRadius > 0.0f)
			return DONT_INTERSECT;

		// Calculate second equation
		CInnerRadius = PXY.Dot(PXY) - InnerRadius * InnerRadius;

		// Discriminant
		D = B * B - 4 * A * CInnerRadius;
		
		// When there is no intersect
		if (D < 0.0f)
			return DONT_INTERSECT;
		if (A * T * T + B * T + CInnerRadius < 0.0f)
			return DONT_INTERSECT;

		return T;
	}

	bool RayCastToAnalogousTorus()
	{
		const float InnerRadius = 0.1f;
		const float OuterRadius = 1.0f;
		const float Height = 0.1f;

		float TVerticalAnnulus = RayCastToCylinderSide((InnerRadius + OuterRadius) / 2.0f, Height);
		float THorizontalAnnulus = RayCastToAnnulus(InnerRadius, OuterRadius, Height);

		return GetCloserT(TVerticalAnnulus, THorizontalAnnulus);
	}

	static URayCaster& Instance()
	{
		static URayCaster RayCaster;
		return RayCaster;
	}
};