#pragma once
#include "SWidget.h"

/**
 * @file SImage.h
 * @brief 이미지를 표시하는 위젯
 *
 * UTexture를 로드하여 화면에 렌더링합니다.
 */

struct ID2D1Bitmap;
class UTexture;

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
     * 파일에서 텍스처 로드
     * @param FilePath 이미지 파일 경로
     * @note AssetManager가 자동으로 캐싱 관리
     */
    SImage& SetTexture(const FWideString& FilePath);

    /**
     * 이미 로드된 UTexture 설정
     * @param InTexture UTexture 객체
     */
    SImage& SetTexture(UTexture* InTexture);

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

    /** 텍스처 로드 여부 */
    bool IsTextureLoaded() const { return Bitmap != nullptr; }

private:
    /** D2D 비트맵 (소유) */
    ID2D1Bitmap* Bitmap = nullptr;

    /** 색상 틴트 */
    FSlateColor TintColor = FSlateColor::White();

    /** 투명도 */
    float Opacity = 1.0f;
};