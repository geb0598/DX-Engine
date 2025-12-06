#include "pch.h"
#include "SImage.h"
#include "FD2DRenderer.h"
#include "SGameHUD.h"

SImage::~SImage()
{
    // 애니메이션 해제
    for (auto* Anim : Animations)
    {
        delete Anim;
    }
    Animations.Empty();

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

    FVector2D DrawSize = Geometry.GetAbsoluteSize();
    FVector2D DrawPosition = Geometry.AbsolutePosition;

    // 스케일 적용
    DrawSize.X *= Scale.X;
    DrawSize.Y *= Scale.Y;

    // 종횡비 유지 모드
    if (bMaintainAspectRatio && ImageSize.X > 0.f && ImageSize.Y > 0.f)
    {
        float WidgetAspect = DrawSize.X / DrawSize.Y;
        float ImageAspect = ImageSize.X / ImageSize.Y;

        if (ImageAspect > WidgetAspect)
        {
            // 이미지가 더 넓음 - 가로 맞춤, 세로 중앙 정렬
            float NewHeight = DrawSize.X / ImageAspect;
            DrawPosition.Y += (DrawSize.Y - NewHeight) * 0.5f;
            DrawSize.Y = NewHeight;
        }
        else
        {
            // 이미지가 더 높음 - 세로 맞춤, 가로 중앙 정렬
            float NewWidth = DrawSize.Y * ImageAspect;
            DrawPosition.X += (DrawSize.X - NewWidth) * 0.5f;
            DrawSize.X = NewWidth;
        }
    }

    // 이미지 렌더링
    Renderer.DrawImage(
        Bitmap,
        DrawPosition,
        DrawSize,
        TintColor,
        Opacity
    );
}

void SImage::Update(float DeltaTime)
{
    // 애니메이션 업데이트 (완료된 것은 제거)
    for (int32 i = Animations.Num() - 1; i >= 0; --i)
    {
        FWidgetAnimation* Anim = Animations[i];
        if (Anim && Anim->Update(DeltaTime))
        {
            delete Anim;
            Animations.RemoveAt(i);
        }
    }
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

    ImageSize = FVector2D(0.f, 0.f);

    // WIC로 직접 D2D 비트맵 로드 (UTexture 우회)
    if (SGameHUD::Get().IsInitialized())
    {
        FD2DRenderer* Renderer = SGameHUD::Get().GetRenderer();
        if (Renderer)
        {
            Bitmap = Renderer->LoadBitmapFromFile(FilePath);

            if (!Bitmap)
            {
                UE_LOG("[SImage] Failed to load bitmap from file: %ls", FilePath.c_str());
            }
            else
            {
                // 비트맵 크기 저장
                D2D1_SIZE_F Size = Bitmap->GetSize();
                ImageSize = FVector2D(Size.width, Size.height);
            }
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

SImage& SImage::SetMaintainAspectRatio(bool bMaintain)
{
    bMaintainAspectRatio = bMaintain;
    return *this;
}

// ────────────────────────────────────────────────────────────────────────────
// 애니메이션
// ────────────────────────────────────────────────────────────────────────────

SImage& SImage::PlayFadeIn(float Duration, EEasingType Easing)
{
    FFadeAnimation* FadeAnim = new FFadeAnimation(&Opacity, 0.0f, 1.0f, Duration);
    FadeAnim->SetEasingType(Easing);
    FadeAnim->Play();
    Animations.Add(FadeAnim);
    return *this;
}

SImage& SImage::PlayFadeOut(float Duration, EEasingType Easing)
{
    FFadeAnimation* FadeAnim = new FFadeAnimation(&Opacity, Opacity, 0.0f, Duration);
    FadeAnim->SetEasingType(Easing);
    FadeAnim->Play();
    Animations.Add(FadeAnim);
    return *this;
}

SImage& SImage::PlayScaleAnimation(FVector2D ToScale, float Duration, EEasingType Easing)
{
    FScaleAnimation* ScaleAnim = new FScaleAnimation(&Scale, Scale, ToScale, Duration);
    ScaleAnim->SetEasingType(Easing);
    ScaleAnim->Play();
    Animations.Add(ScaleAnim);
    return *this;
}

SImage& SImage::SetScale(FVector2D InScale)
{
    Scale = InScale;
    return *this;
}

void SImage::StopAllAnimations()
{
    for (FWidgetAnimation* Anim : Animations)
    {
        if (Anim)
        {
            Anim->Stop();
            delete Anim;
        }
    }
    Animations.Empty();
}