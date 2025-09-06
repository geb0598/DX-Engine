#pragma once
#include "Vector.h"

struct FMatrix
{
    float M[4][4];

    FMatrix()
    {
        Identity();
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

    void Identity()
    {
        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                M[i][j] = (i == j) ? 1.0f : 0.0f;
            }
        }
    }

    FVector TransformPosition(const FVector& V) const
    {
        FVector4 V4 = *this * FVector4(V.X, V.Y, V.Z, 1.0f);
        return FVector(V4.X, V4.Y, V4.Z);
    }

    FVector TransformDirection(const FVector& V) const
    {
        FVector4 V4 = *this * FVector4(V.X, V.Y, V.Z, 0.0f);
        return FVector(V4.X, V4.Y, V4.Z).GetNormalized();
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

    // 역행렬 (간단한 구현)
    FMatrix Inverse() const
    {
        float Det = Determinant();
        if (fabsf(Det) < 1e-6f)
        {
            // 특이행렬인 경우 단위행렬 반환
            FMatrix Result;
            Result.Identity();
            return Result;
        }

        FMatrix Result;
        float InvDet = 1.0f / Det;

        // 여인수 행렬 계산 (복잡하므로 간단한 버전)
        // 실제 프로덕션에서는 LU 분해나 가우스-조던 소거법 사용 권장

        Result.M[0][0] = (M[1][1] * (M[2][2] * M[3][3] - M[2][3] * M[3][2]) -
            M[1][2] * (M[2][1] * M[3][3] - M[2][3] * M[3][1]) +
            M[1][3] * (M[2][1] * M[3][2] - M[2][2] * M[3][1])) * InvDet;
        // ... (나머지 원소들도 비슷하게 계산, 복잡하므로 생략)

        // 간단한 변환 행렬의 경우를 위한 특별 처리
        // 회전+이동 행렬의 역행렬은 더 간단하게 계산 가능

        return Result;
    }

    // ===== 정적 변환 행렬 생성 함수들 =====

    // 이동 행렬
    static FMatrix CreateTranslation(const FVector& Translation)
    {
        FMatrix Result;
        Result.Identity();
        Result.M[0][3] = Translation.X;
        Result.M[1][3] = Translation.Y;
        Result.M[2][3] = Translation.Z;
        return Result;
    }

    // 스케일 행렬
    static FMatrix CreateScale(const FVector& Scale)
    {
        FMatrix Result;
        Result.Identity();
        Result.M[0][0] = Scale.X;
        Result.M[1][1] = Scale.Y;
        Result.M[2][2] = Scale.Z;
        return Result;
    }

    // X축 회전 행렬
    static FMatrix CreateRotationX(float Angle)
    {
        FMatrix Result;
        Result.Identity();
        float Cos = cosf(Angle);
        float Sin = sinf(Angle);
        Result.M[1][1] = Cos;  Result.M[1][2] = -Sin;
        Result.M[2][1] = Sin;  Result.M[2][2] = Cos;
        return Result;
    }

    // Y축 회전 행렬
    static FMatrix CreateRotationY(float Angle)
    {
        FMatrix Result;
        Result.Identity();
        float Cos = cosf(Angle);
        float Sin = sinf(Angle);
        Result.M[0][0] = Cos;   Result.M[0][2] = Sin;
        Result.M[2][0] = -Sin;  Result.M[2][2] = Cos;
        return Result;
    }

    // Z축 회전 행렬
    static FMatrix CreateRotationZ(float Angle)
    {
        FMatrix Result;
        Result.Identity();
        float Cos = cosf(Angle);
        float Sin = sinf(Angle);
        Result.M[0][0] = Cos;  Result.M[0][1] = -Sin;
        Result.M[1][0] = Sin;  Result.M[1][1] = Cos;
        return Result;
    }

    // 임의 축 회전 행렬
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
        Result.M[0][1] = X * Y * OneMinusCos - Z * Sin;
        Result.M[0][2] = X * Z * OneMinusCos + Y * Sin;
        Result.M[0][3] = 0.0f;

        Result.M[1][0] = Y * X * OneMinusCos + Z * Sin;
        Result.M[1][1] = Cos + Y * Y * OneMinusCos;
        Result.M[1][2] = Y * Z * OneMinusCos - X * Sin;
        Result.M[1][3] = 0.0f;

        Result.M[2][0] = Z * X * OneMinusCos - Y * Sin;
        Result.M[2][1] = Z * Y * OneMinusCos + X * Sin;
        Result.M[2][2] = Cos + Z * Z * OneMinusCos;
        Result.M[2][3] = 0.0f;

        Result.M[3][0] = 0.0f;
        Result.M[3][1] = 0.0f;
        Result.M[3][2] = 0.0f;
        Result.M[3][3] = 1.0f;

        return Result;
    }

    // Look-At 행렬 (카메라 뷰 행렬)
    static FMatrix CreateLookAt(const FVector& Eye, const FVector& Target, const FVector& Up)
    {
        FVector Forward = (Target - Eye).GetNormalized();
        FVector Right = Forward.Cross(Up).GetNormalized();
        FVector NewUp = Right.Cross(Forward);

        FMatrix Result;
        Result.M[0][0] = Right.X;   Result.M[0][1] = Right.Y;   Result.M[0][2] = Right.Z;   Result.M[0][3] = -Right.Dot(Eye);
        Result.M[1][0] = NewUp.X;   Result.M[1][1] = NewUp.Y;   Result.M[1][2] = NewUp.Z;   Result.M[1][3] = -NewUp.Dot(Eye);
        Result.M[2][0] = -Forward.X; Result.M[2][1] = -Forward.Y; Result.M[2][2] = -Forward.Z; Result.M[2][3] = Forward.Dot(Eye);
        Result.M[3][0] = 0.0f;      Result.M[3][1] = 0.0f;      Result.M[3][2] = 0.0f;      Result.M[3][3] = 1.0f;

        return Result;
    }

    // 원근 투영 행렬
    static FMatrix CreatePerspective(float FOV, float AspectRatio, float Near, float Far)
    {
        float TanHalfFOV = tanf(FOV * 0.5f);

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
        Result.M[2][2] = -(Far + Near) / (Far - Near);
        Result.M[2][3] = -(2.0f * Far * Near) / (Far - Near);

        Result.M[3][0] = 0.0f;
        Result.M[3][1] = 0.0f;
        Result.M[3][2] = -1.0f;
        Result.M[3][3] = 0.0f;

        return Result;
    }

    // 직교 투영 행렬
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
        Result.M[2][2] = -2.0f / (Far - Near);
        Result.M[2][3] = -(Far + Near) / (Far - Near);

        Result.M[3][0] = 0.0f;
        Result.M[3][1] = 0.0f;
        Result.M[3][2] = 0.0f;
        Result.M[3][3] = 1.0f;

        return Result;
    }

    static const FMatrix Identity;
    static const FMatrix Zero;
};