#pragma once
#include "Vector.h"
#include "../Public/MathUtilities.h"

__declspec(align(16)) struct FMatrix
{
    float M[4][4];

    static const FMatrix Identity;
    static const FMatrix Zero;

    FMatrix()
    {
        SetIdentity();
    }
    FMatrix(const float InM[4][4])
    {
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                M[i][j] = InM[i][j];
    }
    FMatrix(
        float M00, float M01, float M02, float M03,
        float M10, float M11, float M12, float M13,
        float M20, float M21, float M22, float M23,
        float M30, float M31, float M32, float M33
    )
    {
        M[0][0] = M00; M[0][1] = M01; M[0][2] = M02; M[0][3] = M03;
        M[1][0] = M10; M[1][1] = M11; M[1][2] = M12; M[1][3] = M13;
        M[2][0] = M20; M[2][1] = M21; M[2][2] = M22; M[2][3] = M23;
        M[3][0] = M30; M[3][1] = M31; M[3][2] = M32; M[3][3] = M33;
    }

    float* operator[](int RowIndex) 
    { 
        return M[RowIndex];
    }
    const float* operator[](int RowIndex) const 
    {
        return M[RowIndex]; 
    }
    FMatrix operator*(const FMatrix& Other) const
    {
        FMatrix Result;
        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                Result.M[i][j] = 0.0f;
                for (int k = 0; k < 4; ++k)
                {
					Result.M[i][j] += M[i][k] * Other.M[k][j];
                }
            }
        }
        return Result;
    }
    FMatrix& operator*=(const FMatrix& Other)
    {
        *this = *this * Other;
        return *this;
    }
    FVector4 operator*(const FVector4& V) const
    {
        return FVector4(
            M[0][0] * V.X + M[0][1] * V.Y + M[0][2] * V.Z + M[0][3] * V.W,
            M[1][0] * V.X + M[1][1] * V.Y + M[1][2] * V.Z + M[1][3] * V.W,
            M[2][0] * V.X + M[2][1] * V.Y + M[2][2] * V.Z + M[2][3] * V.W,
            M[3][0] * V.X + M[3][1] * V.Y + M[3][2] * V.Z + M[3][3] * V.W
        );
    }
    friend FVector4 operator*(const FVector4& V, const FMatrix& M)
    {
        return FVector4(
            V.X * M[0][0] + V.Y * M[1][0] + V.Z * M[2][0] + V.W * M[3][0],
            V.X * M[0][1] + V.Y * M[1][1] + V.Z * M[2][1] + V.W * M[3][1],
            V.X * M[0][2] + V.Y * M[1][2] + V.Z * M[2][2] + V.W * M[3][2],
            V.X * M[0][3] + V.Y * M[1][3] + V.Z * M[2][3] + V.W * M[3][3]
        );
    }
    FMatrix Inverse() const;
    void SetIdentity()
    {
        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                M[i][j] = (i == j) ? 1.0f : 0.0f;
            }
        }
    }
	// 행렬을 이용해 위치 벡터 변환 (평행 이동 포함)
    FVector TransformPosition(const FVector& Vector) const
    {
        FVector4 V4 = *this * FVector4(Vector.X, Vector.Y, Vector.Z, 1.0f);
        return FVector(V4.X, V4.Y, V4.Z);
    }
	// 행렬을 이용해 방향 벡터 변환 (평행 이동 미포함)
    FVector TransformDirection(const FVector& Vector) const
    {
        FVector4 V4 = *this * FVector4(Vector.X, Vector.Y, Vector.Z, 0.0f);
        return FVector(V4.X, V4.Y, V4.Z);
    }
    FMatrix Transpose() const
    {
        FMatrix Result;
        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                Result.M[i][j] = M[j][i];
            }
        }
        return Result;
    }
    float Determinant() const
    {
        return
                M[0][0] * (M[1][1] * (M[2][2] * M[3][3] - M[2][3] * M[3][2]) -
                M[1][2] * (M[2][1] * M[3][3] - M[2][3] * M[3][1]) +
                M[1][3] * (M[2][1] * M[3][2] - M[2][2] * M[3][1])) -
                M[0][1] * (M[1][0] * (M[2][2] * M[3][3] - M[2][3] * M[3][2]) -
                M[1][2] * (M[2][0] * M[3][3] - M[2][3] * M[3][0]) +
                M[1][3] * (M[2][0] * M[3][2] - M[2][2] * M[3][0])) +
                M[0][2] * (M[1][0] * (M[2][1] * M[3][3] - M[2][3] * M[3][1]) -
                M[1][1] * (M[2][0] * M[3][3] - M[2][3] * M[3][0]) +
                M[1][3] * (M[2][0] * M[3][1] - M[2][1] * M[3][0])) -
                M[0][3] * (M[1][0] * (M[2][1] * M[3][2] - M[2][2] * M[3][1]) -
                M[1][1] * (M[2][0] * M[3][2] - M[2][2] * M[3][0]) +
                M[1][2] * (M[2][0] * M[3][1] - M[2][1] * M[3][0]));
    }

    static FMatrix CreateTranslation(const FVector& Translation)
    {
        FMatrix Result;
        Result.SetIdentity();
        Result.M[3][0] = Translation.X;
        Result.M[3][1] = Translation.Y;
        Result.M[3][2] = Translation.Z;
        return Result;
    }
    static FMatrix CreateScale(const FVector& Scale)
    {
        FMatrix Result;
        Result.SetIdentity();
        Result.M[0][0] = Scale.X;
        Result.M[1][1] = Scale.Y;
        Result.M[2][2] = Scale.Z;
        return Result;
    }
    static FMatrix CreateRotationX(float Angle)
    {
        FMatrix Result;
        Result.SetIdentity();
        float Cos = cosf(Angle);
        float Sin = sinf(Angle);
        Result.M[1][1] = Cos;  
        Result.M[1][2] = Sin;
        Result.M[2][1] = -Sin;  
        Result.M[2][2] = Cos;
        return Result;
    }
    static FMatrix CreateRotationY(float Angle)
    {
        FMatrix Result;
        Result.SetIdentity();
        float Cos = cosf(Angle);
        float Sin = sinf(Angle);
        Result.M[0][0] = Cos;   
        Result.M[0][2] = -Sin;
        Result.M[2][0] = Sin;  
        Result.M[2][2] = Cos;
        return Result;
    }
    static FMatrix CreateRotationZ(float Angle)
    {
        FMatrix Result;
        Result.SetIdentity();
        float Cos = cosf(Angle);
        float Sin = sinf(Angle);
        Result.M[0][0] = Cos;  
        Result.M[0][1] = Sin;
        Result.M[1][0] = -Sin;  
        Result.M[1][1] = Cos;
        return Result;
    }
    // NOTE: Yaw -> Pitch -> Roll
    static FMatrix CreateRotationFromEuler(const FVector& EulerDegrees)
    {
        float X = DEG_TO_RAD(EulerDegrees.X);
        float Y = DEG_TO_RAD(EulerDegrees.Y);
        float Z = DEG_TO_RAD(EulerDegrees.Z);

        FMatrix RotX = CreateRotationX(X);
        FMatrix RotY = CreateRotationY(Y);
        FMatrix RotZ = CreateRotationZ(Z);

        // TODO: Is this correct? -> Junyong Lee
        // Z-Up, Left-Hand = Yaw(Z) → Pitch(Y) → Roll(X)
        //return RotZ * RotY * RotX;

        return RotY * RotX * RotZ;
    }
	// 쿼터니언으로부터 회전 행렬 생성
    static FMatrix CreateRotationFromQuaternion(const FVector4& Quat)
    {
        float X = Quat.X, Y = Quat.Y, Z = Quat.Z, W = Quat.W;
        float X2 = X * 2.0f, Y2 = Y * 2.0f, Z2 = Z * 2.0f;
        float XX = X * X2, XY = X * Y2, XZ = X * Z2;
        float YY = Y * Y2, YZ = Y * Z2, ZZ = Z * Z2;
        float WX = W * X2, WY = W * Y2, WZ = W * Z2;

        FMatrix Result;
        Result.M[0][0] = 1.0f - (YY + ZZ); Result.M[0][1] = XY + WZ;           Result.M[0][2] = XZ - WY;           Result.M[0][3] = 0.0f;
        Result.M[1][0] = XY - WZ;          Result.M[1][1] = 1.0f - (XX + ZZ);  Result.M[1][2] = YZ + WX;           Result.M[1][3] = 0.0f;
        Result.M[2][0] = XZ + WY;          Result.M[2][1] = YZ - WX;           Result.M[2][2] = 1.0f - (XX + YY);  Result.M[2][3] = 0.0f;
        Result.M[3][0] = 0.0f;             Result.M[3][1] = 0.0f;              Result.M[3][2] = 0.0f;              Result.M[3][3] = 1.0f;

        return Result;
    }
	// 주어진 축과 각도로 회전 행렬 생성
    static FMatrix CreateRotationAxis(const FVector& Axis, float Angle)
    {
        FVector NormalizedAxis = Axis.GetNormalized();
        float Cos = cosf(Angle);
        float Sin = sinf(Angle);
        float OneMinusCos = 1.0f - Cos;

        float X = NormalizedAxis.X;
        float Y = NormalizedAxis.Y;
        float Z = NormalizedAxis.Z;

        FMatrix Result;
        Result.M[0][0] = Cos + X * X * OneMinusCos;
        Result.M[0][1] = X * Y * OneMinusCos + Z * Sin;
        Result.M[0][2] = X * Z * OneMinusCos - Y * Sin;
        Result.M[0][3] = 0.0f;

        Result.M[1][0] = Y * X * OneMinusCos - Z * Sin;
        Result.M[1][1] = Cos + Y * Y * OneMinusCos;
        Result.M[1][2] = Y * Z * OneMinusCos + X * Sin;
        Result.M[1][3] = 0.0f;

        Result.M[2][0] = Z * X * OneMinusCos + Y * Sin;
        Result.M[2][1] = Z * Y * OneMinusCos - X * Sin;
        Result.M[2][2] = Cos + Z * Z * OneMinusCos;
        Result.M[2][3] = 0.0f;

        Result.M[3][0] = 0.0f;
        Result.M[3][1] = 0.0f;
        Result.M[3][2] = 0.0f;
        Result.M[3][3] = 1.0f;

        return Result;
    }
    static FMatrix CreateModelTransform(const FVector& Translation, const FVector& RotationDegrees, const FVector& Scale)
    {
        FMatrix ScaleMatrix = CreateScale(Scale);
        FMatrix RotationMatrix = CreateRotationFromEuler(RotationDegrees);
        FMatrix TranslationMatrix = CreateTranslation(Translation);

        return ScaleMatrix * RotationMatrix * TranslationMatrix;
    }
    static FMatrix CreateLookAt(const FVector& Eye, const FVector& Target, const FVector& Up)
    {
		FVector OldUp = Up;
		FVector N = (Target - Eye).GetNormalized(); // Forward
        FVector U = OldUp.Cross(N).GetNormalized(); // RIGHT
        FVector V = N.Cross(U);                     // UP

        FMatrix Result;
        Result.M[0][0] = U.X;   Result.M[0][1] = U.Y;   Result.M[0][2] = U.Z;   Result.M[0][3] = -U.Dot(Eye);
        Result.M[1][0] = V.X;   Result.M[1][1] = V.Y;   Result.M[1][2] = V.Z;   Result.M[1][3] = -V.Dot(Eye);
        Result.M[2][0] = N.X;   Result.M[2][1] = N.Y;   Result.M[2][2] = N.Z;   Result.M[2][3] = -N.Dot(Eye);
        Result.M[3][0] = 0.0f;  Result.M[3][1] = 0.0f;  Result.M[3][2] = 0.0f;  Result.M[3][3] = 1.0f;

        // TODO: Must Check whether this transpose is required or not
        return Result.Transpose();
    }
    static FMatrix CreateViewMatrix(const FVector& CamLocation, const FVector& CamRotation)
    {
		return CreateModelTransform(CamLocation, CamRotation, FVector(1.0f, 1.0f, 1.0f)).Inverse();
    }

    static FMatrix CreatePerspective(float FieldOfViewRad, float AspectRatio, float Near, float Far)
    {
        float TanHalfFOV = tanf(FieldOfViewRad * 0.5f);

        FMatrix Result;
        Result.M[0][0] = 1.0f / (AspectRatio * TanHalfFOV);
        Result.M[0][1] = 0.0f;
        Result.M[0][2] = 0.0f;
        Result.M[0][3] = 0.0f;

        Result.M[1][0] = 0.0f;
        Result.M[1][1] = 1.0f / TanHalfFOV;
        Result.M[1][2] = 0.0f;
        Result.M[1][3] = 0.0f;

        Result.M[2][0] = 0.0f;
        Result.M[2][1] = 0.0f;
        Result.M[2][2] = Far / (Far - Near);
        Result.M[2][3] = -(Far * Near) / (Far - Near);

        Result.M[3][0] = 0.0f;
        Result.M[3][1] = 0.0f;
        Result.M[3][2] = 1.0f;
        Result.M[3][3] = 0.0f;

        // TODO: Must Check whether this transpose is required or not
        return Result.Transpose();
    }
    static FMatrix CreateOrthographic(float Left, float Right, float Bottom, float Top, float Near, float Far)
    {
        FMatrix Result;
        Result.M[0][0] = 2.0f / (Right - Left);
        Result.M[0][1] = 0.0f;
        Result.M[0][2] = 0.0f;
        Result.M[0][3] = -(Right + Left) / (Right - Left);

        Result.M[1][0] = 0.0f;
        Result.M[1][1] = 2.0f / (Top - Bottom);
        Result.M[1][2] = 0.0f;
        Result.M[1][3] = -(Top + Bottom) / (Top - Bottom);

        Result.M[2][0] = 0.0f;
        Result.M[2][1] = 0.0f;
		Result.M[2][2] = 1.0f / (Far - Near); // z range [0, 1]
        Result.M[2][3] = -Near / (Far - Near);

        Result.M[3][0] = 0.0f;
        Result.M[3][1] = 0.0f;
        Result.M[3][2] = 0.0f;
        Result.M[3][3] = 1.0f;

        // TODO: Must Check whether this transpose is required or not
        return Result.Transpose();
    }
};

inline FVector operator*(const FVector& V, const FMatrix& M)
{
    return FVector(
        M[0][0] * V.X + M[1][0] * V.Y + M[2][0] * V.Z,
        M[0][1] * V.X + M[1][1] * V.Y + M[2][1] * V.Z,
        M[0][2] * V.X + M[1][2] * V.Y + M[2][2] * V.Z
    );
}

inline FVector4 operator*(const FVector4& V, const FMatrix& M)
{
    return FVector4(
        M[0][0] * V.X + M[1][0] * V.Y + M[2][0] * V.Z + M[3][0] * V.W,
        M[0][1] * V.X + M[1][1] * V.Y + M[2][1] * V.Z + M[3][1] * V.W,
        M[0][2] * V.X + M[1][2] * V.Y + M[2][2] * V.Z + M[3][2] * V.W,
        M[0][3] * V.X + M[1][3] * V.Y + M[2][3] * V.Z + M[3][3] * V.W
    );
}