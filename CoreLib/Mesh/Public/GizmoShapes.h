#pragma once
#include "Mesh/Public/Vertex.h"
#include "Math/Public/Vector.h"
#include <vector>
#include <cmath>

// X축 화살표 메시 (빨간색)
inline FVertexSimple XAxisArrowVertices[] =
{
    // 몸통 부분
    // Front face
    { 0.0f, -0.025f,  0.025f,  1.f, 0.f, 0.f, 1.f }, { 0.8f, -0.025f,  0.025f,  1.f, 0.f, 0.f, 1.f }, { 0.0f,  0.025f,  0.025f,  1.f, 0.f, 0.f, 1.f },
    { 0.0f,  0.025f,  0.025f,  1.f, 0.f, 0.f, 1.f }, { 0.8f, -0.025f,  0.025f,  1.f, 0.f, 0.f, 1.f }, { 0.8f,  0.025f,  0.025f,  1.f, 0.f, 0.f, 1.f },
    // Back face
    { 0.0f, -0.025f, -0.025f,  1.f, 0.f, 0.f, 1.f }, { 0.0f,  0.025f, -0.025f,  1.f, 0.f, 0.f, 1.f }, { 0.8f, -0.025f, -0.025f,  1.f, 0.f, 0.f, 1.f },
    { 0.0f,  0.025f, -0.025f,  1.f, 0.f, 0.f, 1.f }, { 0.8f,  0.025f, -0.025f,  1.f, 0.f, 0.f, 1.f }, { 0.8f, -0.025f, -0.025f,  1.f, 0.f, 0.f, 1.f },
    // Left face
    { 0.0f, -0.025f, -0.025f,  1.f, 0.f, 0.f, 1.f }, { 0.0f, -0.025f,  0.025f,  1.f, 0.f, 0.f, 1.f }, { 0.0f,  0.025f, -0.025f,  1.f, 0.f, 0.f, 1.f },
    { 0.0f,  0.025f, -0.025f,  1.f, 0.f, 0.f, 1.f }, { 0.0f, -0.025f,  0.025f,  1.f, 0.f, 0.f, 1.f }, { 0.0f,  0.025f,  0.025f,  1.f, 0.f, 0.f, 1.f },
    // Right face
    { 0.8f, -0.025f, -0.025f,  1.f, 0.f, 0.f, 1.f }, { 0.8f,  0.025f, -0.025f,  1.f, 0.f, 0.f, 1.f }, { 0.8f, -0.025f,  0.025f,  1.f, 0.f, 0.f, 1.f },
    { 0.8f,  0.025f, -0.025f,  1.f, 0.f, 0.f, 1.f }, { 0.8f,  0.025f,  0.025f,  1.f, 0.f, 0.f, 1.f }, { 0.8f, -0.025f,  0.025f,  1.f, 0.f, 0.f, 1.f },
    // Top face
    { 0.0f,  0.025f, -0.025f,  1.f, 0.f, 0.f, 1.f }, { 0.0f,  0.025f,  0.025f,  1.f, 0.f, 0.f, 1.f }, { 0.8f,  0.025f, -0.025f,  1.f, 0.f, 0.f, 1.f },
    { 0.8f,  0.025f, -0.025f,  1.f, 0.f, 0.f, 1.f }, { 0.0f,  0.025f,  0.025f,  1.f, 0.f, 0.f, 1.f }, { 0.8f,  0.025f,  0.025f,  1.f, 0.f, 0.f, 1.f },
    // Bottom face
    { 0.0f, -0.025f, -0.025f,  1.f, 0.f, 0.f, 1.f }, { 0.8f, -0.025f, -0.025f,  1.f, 0.f, 0.f, 1.f }, { 0.0f, -0.025f,  0.025f,  1.f, 0.f, 0.f, 1.f },
    { 0.0f, -0.025f,  0.025f,  1.f, 0.f, 0.f, 1.f }, { 0.8f, -0.025f, -0.025f,  1.f, 0.f, 0.f, 1.f }, { 0.8f, -0.025f,  0.025f,  1.f, 0.f, 0.f, 1.f },

    // 화살표 머리 부분 (원뿔)
    { 0.8f, -0.05f,  0.05f, 1.f, 0.f, 0.f, 1.f }, { 1.0f,  0.0f,   0.0f,  1.f, 0.f, 0.f, 1.f }, { 0.8f,  0.05f,  0.05f, 1.f, 0.f, 0.f, 1.f },
    { 0.8f,  0.05f,  0.05f, 1.f, 0.f, 0.f, 1.f }, { 1.0f,  0.0f,   0.0f,  1.f, 0.f, 0.f, 1.f }, { 0.8f,  0.05f, -0.05f, 1.f, 0.f, 0.f, 1.f },
    { 0.8f,  0.05f, -0.05f, 1.f, 0.f, 0.f, 1.f }, { 1.0f,  0.0f,   0.0f,  1.f, 0.f, 0.f, 1.f }, { 0.8f, -0.05f, -0.05f, 1.f, 0.f, 0.f, 1.f },
    { 0.8f, -0.05f, -0.05f, 1.f, 0.f, 0.f, 1.f }, { 1.0f,  0.0f,   0.0f,  1.f, 0.f, 0.f, 1.f }, { 0.8f, -0.05f,  0.05f, 1.f, 0.f, 0.f, 1.f },
};

// Y축 화살표 메시 (초록색)
inline FVertexSimple YAxisArrowVertices[] =
{
    // 몸통 부분
    // Front face
    { -0.025f, 0.0f,  0.025f,  0.f, 1.f, 0.f, 1.f }, { 0.025f, 0.0f,  0.025f,  0.f, 1.f, 0.f, 1.f }, { -0.025f,  0.8f,  0.025f,  0.f, 1.f, 0.f, 1.f },
    { -0.025f,  0.8f,  0.025f,  0.f, 1.f, 0.f, 1.f }, { 0.025f, 0.0f,  0.025f,  0.f, 1.f, 0.f, 1.f }, { 0.025f,  0.8f,  0.025f,  0.f, 1.f, 0.f, 1.f },
    // Back face
    { -0.025f, 0.0f, -0.025f,  0.f, 1.f, 0.f, 1.f }, { -0.025f,  0.8f, -0.025f,  0.f, 1.f, 0.f, 1.f }, { 0.025f, 0.0f, -0.025f,  0.f, 1.f, 0.f, 1.f },
    { -0.025f,  0.8f, -0.025f,  0.f, 1.f, 0.f, 1.f }, { 0.025f,  0.8f, -0.025f,  0.f, 1.f, 0.f, 1.f }, { 0.025f, 0.0f, -0.025f,  0.f, 1.f, 0.f, 1.f },
    // Left face
    { -0.025f, 0.0f, -0.025f,  0.f, 1.f, 0.f, 1.f }, { -0.025f, 0.0f,  0.025f,  0.f, 1.f, 0.f, 1.f }, { -0.025f,  0.8f, -0.025f,  0.f, 1.f, 0.f, 1.f },
    { -0.025f,  0.8f, -0.025f,  0.f, 1.f, 0.f, 1.f }, { -0.025f, 0.0f,  0.025f,  0.f, 1.f, 0.f, 1.f }, { -0.025f,  0.8f,  0.025f,  0.f, 1.f, 0.f, 1.f },
    // Right face
    { 0.025f, 0.0f, -0.025f,  0.f, 1.f, 0.f, 1.f }, { 0.025f,  0.8f, -0.025f,  0.f, 1.f, 0.f, 1.f }, { 0.025f, 0.0f,  0.025f,  0.f, 1.f, 0.f, 1.f },
    { 0.025f,  0.8f, -0.025f,  0.f, 1.f, 0.f, 1.f }, { 0.025f,  0.8f,  0.025f,  0.f, 1.f, 0.f, 1.f }, { 0.025f, 0.0f,  0.025f,  0.f, 1.f, 0.f, 1.f },
    // Top face
    { -0.025f,  0.8f, -0.025f,  0.f, 1.f, 0.f, 1.f }, { -0.025f,  0.8f,  0.025f,  0.f, 1.f, 0.f, 1.f }, { 0.025f,  0.8f, -0.025f,  0.f, 1.f, 0.f, 1.f },
    { 0.025f,  0.8f, -0.025f,  0.f, 1.f, 0.f, 1.f }, { -0.025f,  0.8f,  0.025f,  0.f, 1.f, 0.f, 1.f }, { 0.025f,  0.8f,  0.025f,  0.f, 1.f, 0.f, 1.f },
    // Bottom face
    { -0.025f, 0.0f, -0.025f,  0.f, 1.f, 0.f, 1.f }, { 0.025f, 0.0f, -0.025f,  0.f, 1.f, 0.f, 1.f }, { -0.025f, 0.0f,  0.025f,  0.f, 1.f, 0.f, 1.f },
    { -0.025f, 0.0f,  0.025f,  0.f, 1.f, 0.f, 1.f }, { 0.025f, 0.0f, -0.025f,  0.f, 1.f, 0.f, 1.f }, { 0.025f, 0.0f,  0.025f,  0.f, 1.f, 0.f, 1.f },

    // 화살표 머리 부분 (원뿔)
    { -0.05f, 0.8f,  0.05f, 0.f, 1.f, 0.f, 1.f }, { 0.0f,   1.0f,  0.0f,  0.f, 1.f, 0.f, 1.f }, { 0.05f,  0.8f,  0.05f, 0.f, 1.f, 0.f, 1.f },
    { 0.05f,  0.8f,  0.05f, 0.f, 1.f, 0.f, 1.f }, { 0.0f,   1.0f,  0.0f,  0.f, 1.f, 0.f, 1.f }, { 0.05f,  0.8f, -0.05f, 0.f, 1.f, 0.f, 1.f },
    { 0.05f,  0.8f, -0.05f, 0.f, 1.f, 0.f, 1.f }, { 0.0f,   1.0f,  0.0f,  0.f, 1.f, 0.f, 1.f }, { -0.05f, 0.8f, -0.05f, 0.f, 1.f, 0.f, 1.f },
    { -0.05f, 0.8f, -0.05f, 0.f, 1.f, 0.f, 1.f }, { 0.0f,   1.0f,  0.0f,  0.f, 1.f, 0.f, 1.f }, { -0.05f, 0.8f,  0.05f, 0.f, 1.f, 0.f, 1.f },
};

// Z축 화살표 메시 (파란색)
inline FVertexSimple ZAxisArrowVertices[] =
{
    // 몸통 부분
    // Front face
    { -0.025f, -0.025f, 0.8f,  0.f, 0.f, 1.f, 1.f }, { 0.025f, -0.025f, 0.8f,  0.f, 0.f, 1.f, 1.f }, { -0.025f,  0.025f, 0.8f,  0.f, 0.f, 1.f, 1.f },
    { -0.025f,  0.025f, 0.8f,  0.f, 0.f, 1.f, 1.f }, { 0.025f, -0.025f, 0.8f,  0.f, 0.f, 1.f, 1.f }, { 0.025f,  0.025f, 0.8f,  0.f, 0.f, 1.f, 1.f },
    // Back face
    { -0.025f, -0.025f, 0.0f,  0.f, 0.f, 1.f, 1.f }, { -0.025f,  0.025f, 0.0f,  0.f, 0.f, 1.f, 1.f }, { 0.025f, -0.025f, 0.0f,  0.f, 0.f, 1.f, 1.f },
    { -0.025f,  0.025f, 0.0f,  0.f, 0.f, 1.f, 1.f }, { 0.025f,  0.025f, 0.0f,  0.f, 0.f, 1.f, 1.f }, { 0.025f, -0.025f, 0.0f,  0.f, 0.f, 1.f, 1.f },
    // Left face
    { -0.025f, -0.025f, 0.0f,  0.f, 0.f, 1.f, 1.f }, { -0.025f, -0.025f, 0.8f,  0.f, 0.f, 1.f, 1.f }, { -0.025f,  0.025f, 0.0f,  0.f, 0.f, 1.f, 1.f },
    { -0.025f,  0.025f, 0.0f,  0.f, 0.f, 1.f, 1.f }, { -0.025f, -0.025f, 0.8f,  0.f, 0.f, 1.f, 1.f }, { -0.025f,  0.025f, 0.8f,  0.f, 0.f, 1.f, 1.f },
    // Right face
    { 0.025f, -0.025f, 0.0f,  0.f, 0.f, 1.f, 1.f }, { 0.025f,  0.025f, 0.0f,  0.f, 0.f, 1.f, 1.f }, { 0.025f, -0.025f, 0.8f,  0.f, 0.f, 1.f, 1.f },
    { 0.025f,  0.025f, 0.0f,  0.f, 0.f, 1.f, 1.f }, { 0.025f,  0.025f, 0.8f,  0.f, 0.f, 1.f, 1.f }, { 0.025f, -0.025f, 0.8f,  0.f, 0.f, 1.f, 1.f },
    // Top face
    { -0.025f,  0.025f, 0.0f,  0.f, 0.f, 1.f, 1.f }, { -0.025f,  0.025f, 0.8f,  0.f, 0.f, 1.f, 1.f }, { 0.025f,  0.025f, 0.0f,  0.f, 0.f, 1.f, 1.f },
    { 0.025f,  0.025f, 0.0f,  0.f, 0.f, 1.f, 1.f }, { -0.025f,  0.025f, 0.8f,  0.f, 0.f, 1.f, 1.f }, { 0.025f,  0.025f, 0.8f,  0.f, 0.f, 1.f, 1.f },
    // Bottom face
    { -0.025f, -0.025f, 0.0f,  0.f, 0.f, 1.f, 1.f }, { 0.025f, -0.025f, 0.0f,  0.f, 0.f, 1.f, 1.f }, { -0.025f, -0.025f, 0.8f,  0.f, 0.f, 1.f, 1.f },
    { -0.025f, -0.025f, 0.8f,  0.f, 0.f, 1.f, 1.f }, { 0.025f, -0.025f, 0.0f,  0.f, 0.f, 1.f, 1.f }, { 0.025f, -0.025f, 0.8f,  0.f, 0.f, 1.f, 1.f },

    // 화살표 머리 부분 (원뿔)
    { -0.05f,  0.05f, 0.8f, 0.f, 0.f, 1.f, 1.f }, {  0.0f,   0.0f,  1.0f,  0.f, 0.f, 1.f, 1.f }, {  0.05f,  0.05f, 0.8f, 0.f, 0.f, 1.f, 1.f },
    {  0.05f,  0.05f, 0.8f, 0.f, 0.f, 1.f, 1.f }, {  0.0f,   0.0f,  1.0f,  0.f, 0.f, 1.f, 1.f }, {  0.05f, -0.05f, 0.8f, 0.f, 0.f, 1.f, 1.f },
    {  0.05f, -0.05f, 0.8f, 0.f, 0.f, 1.f, 1.f }, {  0.0f,   0.0f,  1.0f,  0.f, 0.f, 1.f, 1.f }, { -0.05f, -0.05f, 0.8f, 0.f, 0.f, 1.f, 1.f },
    { -0.05f, -0.05f, 0.8f, 0.f, 0.f, 1.f, 1.f }, {  0.0f,   0.0f,  1.0f,  0.f, 0.f, 1.f, 1.f }, { -0.05f,  0.05f, 0.8f, 0.f, 0.f, 1.f, 1.f },
};

inline void CreateTorusVertices(
    std::vector<FVertexSimple>& outVertices,
    float majorRadius, float minorRadius,
    int majorSegments, int minorSegments,
    const FVector& color)
{
    outVertices.clear();
    const float PI = 3.1415926535f;
    float majorStep = 2.0f * PI / majorSegments;
    float minorStep = 2.0f * PI / minorSegments;

    for (int i = 0; i < majorSegments; ++i)
    {
        float a0 = i * majorStep;
        float a1 = a0 + majorStep;
        FVector p0(cos(a0) * majorRadius, sin(a0) * majorRadius, 0);
        FVector p1(cos(a1) * majorRadius, sin(a1) * majorRadius, 0);

        for (int j = 0; j < minorSegments; ++j)
        {
            float b0 = j * minorStep;
            float b1 = b0 + minorStep;

            FVector n0(cos(a0) * cos(b0), sin(a0) * cos(b0), sin(b0));
            FVector n1(cos(a0) * cos(b1), sin(a0) * cos(b1), sin(b1));
            FVector n2(cos(a1) * cos(b1), sin(a1) * cos(b1), sin(b1));
            FVector n3(cos(a1) * cos(b0), sin(a1) * cos(b0), sin(b0));

            FVector v0 = p0 + n0 * minorRadius;
            FVector v1 = p0 + n1 * minorRadius;
            FVector v2 = p1 + n2 * minorRadius;
            FVector v3 = p1 + n3 * minorRadius;

            outVertices.push_back({ v0.X, v0.Y, v0.Z, color.X, color.Y, color.Z, 1.0f });
            outVertices.push_back({ v1.X, v1.Y, v1.Z, color.X, color.Y, color.Z, 1.0f });
            outVertices.push_back({ v2.X, v2.Y, v2.Z, color.X, color.Y, color.Z, 1.0f });

            outVertices.push_back({ v0.X, v0.Y, v0.Z, color.X, color.Y, color.Z, 1.0f });
            outVertices.push_back({ v2.X, v2.Y, v2.Z, color.X, color.Y, color.Z, 1.0f });
            outVertices.push_back({ v3.X, v3.Y, v3.Z, color.X, color.Y, color.Z, 1.0f });
        }
    }
}
