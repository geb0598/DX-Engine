#pragma once
#include "SWidget.h"
#include "WidgetAnimation.h"
#include <vector>

/**
 * @file SImage.h
 * @brief 이미지를 표시하는 위젯
 *
 * WIC를 사용하여 파일에서 직접 D2D 비트맵을 로드하여 렌더링합니다.
 */

struct ID2D1Bitmap;

class SImage : public SWidget
{
public:
    SImage() = default;
    virtual ~SImage() override;

    /**
     * 위젯 렌더링
     * @param Renderer D2D 렌더러
     * @param Geometry 위젯의 화면 좌표 및 크기 정보
     * @note ID2D1Bitmap을 화면에 그림
     */
    void Paint(FD2DRenderer& Renderer, const FGeometry& Geometry) override;

    /**
     * 위젯 업데이트 (애니메이션 처리)
     * @param DeltaTime 델타 타임
     */
    void Update(float DeltaTime) override;

    /**
     * 파일에서 텍스처 로드 (WIC 사용)
     * @param FilePath 이미지 파일 경로 (예: L"Data/Textures/UI/Image.png")
     */
    SImage& SetTexture(const FWideString& FilePath);

    /**
     * 색상 틴트 (곱셈 블렌딩)
     * @note 현재는 투명도만 적용됨 (색상은 추후 Effect로 구현 가능)
     */
    SImage& SetTint(const FSlateColor& Color);

    /**
     * 투명도 설정
     * @param InOpacity 0.0 (완전 투명) ~ 1.0 (완전 불투명)
     */
    SImage& SetOpacity(float InOpacity);

    /**
     * 종횡비 유지 설정
     * @param bMaintain true = 비율 유지, false = 위젯 크기에 맞게 늘림
     */
    SImage& SetMaintainAspectRatio(bool bMaintain);

    /** 텍스처 로드 여부 */
    bool IsTextureLoaded() const { return Bitmap != nullptr; }

    /** 원본 이미지 크기 */
    FVector2D GetImageSize() const { return ImageSize; }

    // =====================================================
    // 애니메이션
    // =====================================================

    /**
     * Fade In 애니메이션 재생
     * @param Duration 지속 시간
     * @param Easing 이징 타입
     */
    SImage& PlayFadeIn(float Duration = 0.3f, EEasingType Easing = EEasingType::EaseOutQuad);

    /**
     * Fade Out 애니메이션 재생
     * @param Duration 지속 시간
     * @param Easing 이징 타입
     */
    SImage& PlayFadeOut(float Duration = 0.3f, EEasingType Easing = EEasingType::EaseInQuad);

    /**
     * Scale 애니메이션 재생
     * @param ToScale 목표 스케일
     * @param Duration 지속 시간
     * @param Easing 이징 타입
     */
    SImage& PlayScaleAnimation(FVector2D ToScale, float Duration = 0.3f, EEasingType Easing = EEasingType::EaseOutQuad);

    /**
     * 스케일 설정
     * @param InScale 스케일 값
     */
    SImage& SetScale(FVector2D InScale);

    /** 모든 애니메이션 중지 */
    void StopAllAnimations();

private:
    /** D2D 비트맵 (소유) */
    ID2D1Bitmap* Bitmap = nullptr;

    /** 색상 틴트 */
    FSlateColor TintColor = FSlateColor::White();

    /** 투명도 */
    float Opacity = 1.0f;

    /** 종횡비 유지 여부 */
    bool bMaintainAspectRatio = true;

    /** 원본 이미지 크기 */
    FVector2D ImageSize = FVector2D(0.f, 0.f);

    /** 스케일 (애니메이션용) */
    FVector2D Scale = FVector2D(1.f, 1.f);

    /** 실행 중인 애니메이션 */
    std::vector<TSharedPtr<FWidgetAnimation>> Animations;
};