#pragma once
#include <cmath>

struct FVector
{
	float X;
	float Y;
	float Z;

	FVector(float X = 0.0f, float Y = 0.0f, float Z = 0.0f) : X(X), Y(Y), Z(Z) {}

	FVector operator*(float Scalar) const 
	{ 
		return FVector(X * Scalar, Y * Scalar, Z * Scalar); 
	}
	FVector operator*=(float Scalar) 
	{ 
		X *= Scalar; 
		Y *= Scalar; 
		Z *= Scalar; 
		return *this;
	}
	FVector operator/(float Scalar) const 
	{ 
		return FVector(X / Scalar, Y / Scalar, Z / Scalar); 
	}
	FVector operator/=(float Scalar) 
	{ 
		X /= Scalar; 
		Y /= Scalar; 
		Z /= Scalar; 
		return *this; 
	}
	FVector operator+(const FVector& Other) const 
	{ 
		return FVector(X + Other.X, Y + Other.Y, Z + Other.Z); 
	}
	FVector operator+=(const FVector& Other) 
	{ 
		X += Other.X; 
		Y += Other.Y; 
		Z += Other.Z; 
		return *this; 
	}
	FVector operator-(const FVector& Other) const 
	{ 
		return FVector(X - Other.X, Y - Other.Y, Z - Other.Z);
	}
	FVector operator-=(const FVector& Other) 
	{ 
		X -= Other.X; 
		Y -= Other.Y; 
		Z -= Other.Z; 
		return *this; 
	}
	FVector operator-() const 
	{ 
		return FVector(-X, -Y, -Z); 
	}
	bool operator==(const FVector& Other) const 
	{ 
		return X == Other.X && Y == Other.Y && Z == Other.Z;
	}
	bool operator!=(const FVector& Other) const 
	{
		return !(*this == Other); 
	}

	float Dot(const FVector& Other) const 
	{ 
		return X * Other.X + Y * Other.Y + Z * Other.Z;
	}
	FVector Cross(const FVector& Other) 
	{ 
		return FVector(Y * Other.Z - Z * Other.Y, Z * Other.X - X * Other.Z, X * Other.Y - Y * Other.X );
	}
	float Length() const 
	{
		return sqrtf(X * X + Y * Y + Z * Z);
	}
	float LengthSquared() const 
	{ 
		return X * X + Y * Y + Z * Z;
	}
	void Normalize() 
	{ 
		float Len = Length(); 
		if (Len > 0) 
		{ 
			X /= Len; 
			Y /= Len; 
			Z /= Len;
		} 
	}
	FVector GetNormalized() const 
	{ 
		FVector Result = *this; 
		Result.Normalize(); 
		return Result;
	}
	float Distance(const FVector& Other) const
	{
		return (*this - Other).Length();
	}
};

struct FVector4
{
	float X;
	float Y;
	float Z;
	float W;

	FVector4(float X = 0.0f, float Y = 0.0f, float Z = 0.0f, float W = 0.0f) : X(X), Y(Y), Z(Z), W(W) {}
	FVector4(const FVector& Vec, float W = 1.0f) : X(Vec.X), Y(Vec.Y), Z(Vec.Z), W(W) {}

	FVector4 operator*(float Scalar) const 
	{ 
		return FVector4(X * Scalar, Y * Scalar, Z * Scalar, W * Scalar);
	}
	FVector4 operator*=(float Scalar)
	{ 
		X *= Scalar;
		Y *= Scalar; 
		Z *= Scalar;
		W *= Scalar; 
		return *this;
	}
	FVector4 operator/(float Scalar) const
	{ 
		return FVector4(X / Scalar, Y / Scalar, Z / Scalar, W / Scalar);
	}
	FVector4 operator/=(float Scalar) 
	{
		X /= Scalar;
		Y /= Scalar;
		Z /= Scalar;
		W /= Scalar;
		return *this; 
	}
	FVector4 operator+(const FVector4& Other) const
	{
		return FVector4(X + Other.X, Y + Other.Y, Z + Other.Z, W + Other.W);
	}
	FVector4 operator+=(const FVector4& Other) 
	{
		X += Other.X;
		Y += Other.Y;
		Z += Other.Z; 
		W += Other.W; 
		return *this;
	}
	FVector4 operator-(const FVector4& Other) const
	{
		return FVector4(X - Other.X, Y - Other.Y, Z - Other.Z, W - Other.W);
	}
	FVector4 operator-=(const FVector4& Other) 
	{
		X -= Other.X; 
		Y -= Other.Y;
		Z -= Other.Z; 
		W -= Other.W; 
		return *this;
	}
	bool operator==(const FVector4& Other) const
	{
		return X == Other.X && Y == Other.Y && Z == Other.Z && W == Other.W;
	}
	bool operator!=(const FVector4& Other) const 
	{ 
		return !(*this == Other);
	}
	FVector4 operator-() const 
	{ 
		return FVector4(-X, -Y, -Z, -W);
	}

	float Dot(const FVector4& Other) const 
	{
		return X * Other.X + Y * Other.Y + Z * Other.Z + W * Other.W;
	}
	float Length() const 
	{ 
		return sqrtf(X * X + Y * Y + Z * Z + W * W);
	}
	float LengthSquared() const 
	{ 
		return X * X + Y * Y + Z * Z + W * W;
	}
	void Normalize() 
	{
		float Len = Length(); 
		if (Len > 0) 
		{
			X /= Len;
			Y /= Len;
			Z /= Len;
			W /= Len;
		}
	}
	FVector4 GetNormalized() const
	{
		FVector4 Result = *this;
		Result.Normalize();
		return Result;
	}
	FVector ToVector3() const 
	{ 
		return FVector(X, Y, Z);
	}
};