#pragma once
#include "Vector.h"

#define DEG_TO_RAD(degrees) ((degrees) * 3.14159265359f / 180.0f)
#define RAD_TO_DEG(radians) ((radians) * 180.0f / 3.14159265359f)

constexpr float PI = 3.141592654f;
constexpr float TOWPI = 6.283185307f;
constexpr float ONEDIVPI = 0.318309886f;
constexpr float ONEDIV2PI = 0.159154943f;
constexpr float PIDIV2 = 1.570796327f;
constexpr float PIDIV4 = 0.785398163f;

// 오일러 각도를 쿼터니언으로 변환
inline FVector4 EulerToQuaternion(const FVector& EulerDegrees)
{
    float X = DEG_TO_RAD(EulerDegrees.X) * 0.5f;
    float Y = DEG_TO_RAD(EulerDegrees.Y) * 0.5f;
    float Z = DEG_TO_RAD(EulerDegrees.Z) * 0.5f;

    float CosX = cosf(X);
    float SinX = sinf(X);
    float CosY = cosf(Y);
    float SinY = sinf(Y);
    float CosZ = cosf(Z);
    float SinZ = sinf(Z);

    // Yaw(Z) → Pitch(Y) → Roll(X) 순서
    return FVector4(
        CosZ * CosY * SinX - SinZ * SinY * CosX, // X
        SinZ * CosY * SinX + CosZ * SinY * CosX, // Y
        SinZ * CosY * CosX - CosZ * SinY * SinX, // Z
        CosZ * CosY * CosX + SinZ * SinY * SinX  // W
    );
}

// 쿼터니언을 오일러 각도로 변환
inline FVector QuaternionToEuler(const FVector4& Quat)
{
    float X = Quat.X;
    float Y = Quat.Y;
    float Z = Quat.Z;
    float W = Quat.W;

    // Roll (X-axis rotation)
    float SinRCosP = 2 * (W * X + Y * Z);
    float CosRCosP = 1 - 2 * (X * X + Y * Y);
    float Roll = atan2f(SinRCosP, CosRCosP);

    // Pitch (Y-axis rotation)
    float SinP = 2 * (W * Y - Z * X);
    float Pitch = (fabsf(SinP) >= 1) ? copysignf(PIDIV2, SinP) : asinf(SinP);

    // Yaw (Z-axis rotation)
    float SinYCosP = 2 * (W * Z + X * Y);
    float CosYCosP = 1 - 2 * (Y * Y + Z * Z);
    float Yaw = atan2f(SinYCosP, CosYCosP);

    return FVector(RAD_TO_DEG(Roll), RAD_TO_DEG(Pitch), RAD_TO_DEG(Yaw));
}