#pragma once
#include <cmath>

struct FVector4;

struct FVector
{
	float X;
	float Y;
	float Z;

	static const FVector Zero;
	static const FVector One;
	static const FVector Up;      // (0, 0, 1)
	static const FVector Forward; // (1, 0, 0)
	static const FVector Right;   // (0, 1, 0)

	FVector(float X = 0.0f, float Y = 0.0f, float Z = 0.0f) : X(X), Y(Y), Z(Z) {}
	FVector(const FVector& Other) : X(Other.X), Y(Other.Y), Z(Other.Z) {}
	//FVector(const FVector4& Vec4);

	FVector operator=(const FVector& Other)
	{
		X = Other.X;
		Y = Other.Y;
		Z = Other.Z;
		return *this;
	}

	FVector operator*(float Scalar) const 
	{ 
		return FVector(X * Scalar, Y * Scalar, Z * Scalar); 
	}
	friend FVector operator*(float Scalar, FVector Other)
	{
		return FVector(Other.X * Scalar, Other.Y * Scalar, Other.Z * Scalar);
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
	FVector Cross(const FVector& Other) const
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
			*this /= Len;
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
	FVector ProjectOnto(const FVector& Other) const
	{
		return Other * (Dot(Other) / Other.LengthSquared());
	}
	float AngleBetween(const FVector& Other) const
	{
		return acosf(Dot(Other) / (Length() * Other.Length()));
	}
};

struct FVector2D
{
	float X;
	float Y;

	FVector2D(float InX = 0.0f, float InY = 0.0f) : X(InX), Y(InY) {}

	// 내적 (Dot Product)
	float Dot(const FVector2D& Other) const
	{
		return X * Other.X + Y * Other.Y;
	}

	// 벡터의 길이
	float Length() const
	{
		return sqrtf(X * X + Y * Y);
	}

	// 벡터 정규화 (자신을 변경)
	void Normalize()
	{
		float Len = Length();
		if (Len > 1e-6f) // 0으로 나누는 것을 방지
		{
			X /= Len;
			Y /= Len;
		}
	}

	// 정규화된 벡터를 새로 만들어 반환
	FVector2D GetNormalized() const
	{
		FVector2D Result = *this;
		Result.Normalize();
		return Result;
	}
};


struct FVector4
{
	float X;
	float Y;
	float Z;
	float W;

	static const FVector4 Zero;
	static const FVector4 One;
	static const FVector4 Up;      // (0, 0, 1, 0)
	static const FVector4 Forward; // (1, 0, 0, 0)
	static const FVector4 Right;   // (0, 1, 0, 0)

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

//inline FVector::FVector(const FVector4& Vec4) : X(Vec4.X), Y(Vec4.Y), Z(Vec4.Z) {}