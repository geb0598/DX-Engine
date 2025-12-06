#include "pch.h"
#include "SImage.h"
#include "FD2DRenderer.h"
#include "SGameHUD.h"
#include "Texture.h"
#include "ResourceManager.h"

SImage::~SImage()
{
    // D2D 비트맵 해제
    if (Bitmap)
    {
        Bitmap->Release();
        Bitmap = nullptr;
    }
}

// ────────────────────────────────────────────────────────────────────────────
// 렌더링
// ────────────────────────────────────────────────────────────────────────────

void SImage::Paint(FD2DRenderer& Renderer, const FGeometry& Geometry)
{
    if (!IsVisible() || !Bitmap)
        return;

    // 이미지 렌더링
    Renderer.DrawImage(
        Bitmap,
        Geometry.AbsolutePosition,
        Geometry.GetAbsoluteSize(),
        TintColor,
        Opacity
    );
}

// ────────────────────────────────────────────────────────────────────────────
// 텍스처 설정
// ────────────────────────────────────────────────────────────────────────────

SImage& SImage::SetTexture(const FWideString& FilePath)
{
    // 기존 비트맵 해제
    if (Bitmap)
    {
        Bitmap->Release();
        Bitmap = nullptr;
    }

    // UTF-16 → UTF-8 변환
    FString FilePathUTF8 = WideToUTF8(FilePath);

    // UTexture로 로드 (AssetManager가 자동 캐싱)
    UTexture* Texture = UResourceManager::GetInstance().Load<UTexture>(FilePathUTF8);
    if (!Texture)
    {
        UE_LOG("[SImage] Failed to load texture: %s", FilePathUTF8.c_str());
        return *this;
    }

    // D2D 비트맵으로 변환
    if (SGameHUD::Get().IsInitialized())
    {
        FD2DRenderer* Renderer = SGameHUD::Get().GetRenderer();
        if (Renderer)
        {
            Bitmap = Renderer->CreateBitmapFromUTexture(Texture);

            if (!Bitmap)
            {
                UE_LOG("[SImage] Failed to convert UTexture to D2D Bitmap: %s", FilePathUTF8.c_str());
            }
        }
    }

    return *this;
}

SImage& SImage::SetTexture(UTexture* InTexture)
{
    // 기존 비트맵 해제
    if (Bitmap)
    {
        Bitmap->Release();
        Bitmap = nullptr;
    }

    if (!InTexture)
        return *this;

    // D2D 비트맵으로 변환
    if (SGameHUD::Get().IsInitialized())
    {
        FD2DRenderer* Renderer = SGameHUD::Get().GetRenderer();
        if (Renderer)
        {
            Bitmap = Renderer->CreateBitmapFromUTexture(InTexture);
        }
    }

    return *this;
}

// ────────────────────────────────────────────────────────────────────────────
// 스타일 설정
// ────────────────────────────────────────────────────────────────────────────

SImage& SImage::SetTint(const FSlateColor& Color)
{
    TintColor = Color;
    return *this;
}

SImage& SImage::SetOpacity(float InOpacity)
{
    Opacity = FMath::Clamp(InOpacity, 0.0f, 1.0f);
    return *this;
}