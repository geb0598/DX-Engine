#pragma once

#include <algorithm>
#include <optional>

#include "Math/Math.h"
#include "Containers/Containers.h"
#include "Component/Public/SphereComponent.h"
#include "Component/Public/CubeComponent.h"
#include "Component/Public/TriangleComponent.h"
#include "Component/Public/PlaneComponent.h"

#define DONT_INTERSECT -1.0f

// [추가]
class UStaticMeshComponent;

class URayCaster
{
public:
	// [수정] CurrentRay를 public으로 이동
	struct Ray
	{
		FVector Point;
		FVector Vector;
	};
	Ray CurrentRay;


	// resolve floating point error
	static const float EPSILON;

private:
	void SetRayWithMouseAndMVP(int MouseX, int MouseY, int ScreenWidth, int ScreenHeight ,FMatrix Modeling, FMatrix View, FMatrix Projection)
	{
		float NDCX = 2.0f * MouseX / ScreenWidth - 1.0f;
		float NDCY = 1.0f - 2.0f * MouseY / ScreenHeight;

		// near/far in NDC
		M = Modeling;
		V = View;
		P = Projection;
		FMatrix InverseMVP = (M * V * P).Inverse();
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
private:
	FMatrix M;
	FMatrix V;
	FMatrix P;
private:
	URayCaster() = default;
	~URayCaster() = default;

	std::optional<float> RayCastToSphere()
	{
		const float Radius = 1.0f;

		// Components for Quadratic Formula
		FVector V = CurrentRay.Vector;
		FVector P = CurrentRay.Point;

		float A = V.Dot(V);
		float B = 2.0f * P.Dot(V);
		float C = P.Dot(P) - Radius * Radius;

		// Discriminant
		float D = B * B - 4 * A * C;

		if (D >= 0.0f)
		{
			float T1 = (-B + std::sqrtf(D)) / (2.0f * A);
			float T2 = (-B - std::sqrtf(D)) / (2.0f * A);

			if (T1 < 0.0f && T2 < 0.0f)
				return std::nullopt;
			else if (T1 >= 0.0f && T2 < 0.0f)
				return T1;
			else if (T1 < 0.0f && T2 >= 0.0f)
				return T2;
			else
				return (std::min)(T1, T2);
		}
		else
			return std::nullopt;
	}

	std::optional<float> RayCastToCube()
	{
		const float XHalf = 0.5f;
		const float YHalf = 0.5f;
		const float ZHalf = 0.5f;

		float T1, T2, T3, T4, T5, T6;

		FVector P = CurrentRay.Point;
		FVector V = CurrentRay.Vector;


		// when x = +-cube's x len * 0.5
		if (V.X > EPSILON || V.X < -EPSILON)
		{
			T1 = (XHalf - P.X) / V.X;
			// is point in valid yz range?
			if (P.Y + T1 * V.Y < -YHalf || P.Y + T1 * V.Y > YHalf)
				T1 = DONT_INTERSECT;
			if (P.Z + T1 * V.Z < -ZHalf || P.Z + T1 * V.Z > ZHalf)
				T1 = DONT_INTERSECT;
			if (T1 < 0.0f)
				T1 = DONT_INTERSECT;
			
			T2 = (-XHalf - P.X) / V.X;
			if (P.Y + T2 * V.Y < -YHalf || P.Y + T2 * V.Y > YHalf)
				T2 = DONT_INTERSECT;
			if (P.Z + T2 * V.Z < -ZHalf || P.Z + T2 * V.Z > ZHalf)
				T2 = DONT_INTERSECT;
			if (T2 < 0.0f)
				T2 = DONT_INTERSECT;
		}
		else
		{
			T1 = DONT_INTERSECT;
			T2 = DONT_INTERSECT;
		}

		if (V.Y > EPSILON || V.Y < -EPSILON)
		{
			// when y = +-cube's y len * 0.5
			T3 = (YHalf - P.Y) / V.Y;
			T4 = (-YHalf - P.Y) / V.Y;

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
		}
		else
		{
			T3 = DONT_INTERSECT;
			T4 = DONT_INTERSECT;
		}

		if (V.Z > EPSILON || V.Z < -EPSILON)
		{
			// when z = +-cube's z len * 0.5
			T5 = (ZHalf - P.Z) / V.Z;
			T6 = (-ZHalf - P.Z) / V.Z;

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
		}
		else
		{
			T5 = DONT_INTERSECT;
			T6 = DONT_INTERSECT;
		}

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

		// TODO: It's too hard to change all vars into std::optional, 
		//		 so I left this one for further modification
		
		if (Closest == DONT_INTERSECT)
		{
			return std::nullopt;
		}
		else
		{
			return Closest;
		}
	}

	std::optional<float> RayCastToTriangle()
	{
		FVector P = CurrentRay.Point;
		FVector V = CurrentRay.Vector;

		// Points of Triangle is fixed in Object Space
		FVector T0(0.0f, 1.0f, 0.0f);
		FVector T1(1.0f, -1.0f, 0.0f);
		FVector T2(-1.0f, -1.0f, 0.0f);

		// Normal of Triangle
		FVector N = (T1 - T0).Cross(T2 - T0);

		// when ray is parellel to triangle
		if (V.Dot(N) >= -EPSILON && V.Dot(N) <= EPSILON)
			return std::nullopt;

		// find t when ray intersect with Plane.
		// Plane includes triangle
		float T = -(P - T0).Dot(N) / V.Dot(N);

		// when triangle is behind camera
		if (T < 0.0f)
			return std::nullopt;

		// R is intersection point between ray and plane
		FVector R = P + T * V;

		if ((T1 - T0).Cross(R - T0).Dot(N) < 0.0f ||
			(T2 - T1).Cross(R - T1).Dot(N) < 0.0f ||
			(T0 - T2).Cross(R - T2).Dot(N) < 0.0f)
			return std::nullopt;

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

		T1 = (-B + std::sqrtf(D)) / (2.0f * A);
		T2 = (-B - std::sqrtf(D)) / (2.0f * A);

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

	std::optional<float> RayCastToQuad()
	{
		const float XHalf = 0.5f;
		const float YHalf = 0.5f;

		FVector P = CurrentRay.Point;
		FVector V = CurrentRay.Vector;

		// find T value of ray equation when z = 0

		if (P.Z >= -EPSILON && P.Z <= EPSILON)
			return std::nullopt;

		float T = -P.Z / V.Z;
		
		if (T <= 0.0f)
			return std::nullopt;

		float RayX = P.X + T * V.X;
		if (RayX > XHalf || RayX < -XHalf)
			return std::nullopt;

		float RayY = P.Y + T * V.Y;
		if (RayY > YHalf || RayY < -YHalf)
			return std::nullopt;

		return T;
	}

	float RayCastToAnalogousTorus()
	{
		const float InnerRadius = 0.1f;
		const float OuterRadius = 1.0f;
		const float Height = 0.1f;

		float TVerticalAnnulus = RayCastToCylinderSide((InnerRadius + OuterRadius) / 2.0f, Height);
		float THorizontalAnnulus = RayCastToAnnulus(InnerRadius, OuterRadius, Height);

		return GetCloserT(TVerticalAnnulus, THorizontalAnnulus);
	}

	std::optional<float> GetRealWorldDistance(std::optional<float> T)
	{
		if (!T)
			return T;
		FVector CameraPos = CurrentRay.Point * M;
		FVector IntersectionPos = (CurrentRay.Point + *T * CurrentRay.Vector) * M;

		return (IntersectionPos).Distance(CameraPos);
	}
	// [추가] 일반 메시에 대한 레이캐스팅 함수 구현
	std::optional<float> RayCastToMesh(UMesh* Mesh)
	{
		if (!Mesh) return std::nullopt;

	}
public:
	static URayCaster& Instance()
	{
		static URayCaster RayCaster;
		return RayCaster;
	}

public:
	std::optional<float> GetHitResultAtScreenPosition(
		UPrimitiveComponent& PrimitiveComponent,
		int32 MouseX,
		int32 MouseY,
		int32 ScreenWidth,
		int32 ScreenHeight,
		const FMatrix& ModelingMatrix,
		const FMatrix& ViewMatrix,
		const FMatrix& ProjectionMatrix
	);

	std::optional<float> GetHitResultAtScreenPosition(
		UTriangleComponent& TriangleComponent,
		int32 MouseX,
		int32 MouseY,
		int32 ScreenWidth,
		int32 ScreenHeight,
		const FMatrix& ModelingMatrix,
		const FMatrix& ViewMatrix,
		const FMatrix& ProjectionMatrix
	);

	std::optional<float> GetHitResultAtScreenPosition(
		UCubeComponent& CubeComponent,
		int32 MouseX,
		int32 MouseY,
		int32 ScreenWidth,
		int32 ScreenHeight,
		const FMatrix& ModelingMatrix,
		const FMatrix& ViewMatrix,
		const FMatrix& ProjectionMatrix
	);

	std::optional<float> GetHitResultAtScreenPosition(
		USphereComponent& SphereComponent,
		int32 MouseX,
		int32 MouseY,
		int32 ScreenWidth,
		int32 ScreenHeight,
		const FMatrix& ModelingMatrix,
		const FMatrix& ViewMatrix,
		const FMatrix& ProjectionMatrix
	);
	// UStaticMeshComponent를 위한 오버로드
	std::optional<float> GetHitResultAtScreenPosition(
		UStaticMeshComponent& StaticMeshComponent,
		int32 MouseX,
		int32 MouseY,
		int32 ScreenWidth,
		int32 ScreenHeight,
		const FMatrix& ModelingMatrix,
		const FMatrix& ViewMatrix,
		const FMatrix& ProjectionMatrix
	);
};