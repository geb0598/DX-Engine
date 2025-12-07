#pragma once

/**
 * @file SBorderBox.h
 * @brief 테두리가 있는 반투명 박스 위젯
 *
 * 배경색과 테두리를 가진 둥근 사각형 박스입니다.
 */

#include "SWidget.h"

/**
 * @class SBorderBox
 * @brief 테두리가 있는 반투명 배경 박스 위젯
 */
class SBorderBox : public SWidget
{
public:
	SBorderBox() = default;
	virtual ~SBorderBox() = default;

	// =====================================================
	// 렌더링
	// =====================================================

	void Paint(FD2DRenderer& Renderer, const FGeometry& Geometry) override;

	// =====================================================
	// 속성 설정
	// =====================================================

	/**
	 * 배경 색상 설정
	 * @param InColor 배경 색상
	 * @return 체이닝용 참조
	 */
	SBorderBox& SetBackgroundColor(const FSlateColor& InColor)
	{
		BackgroundColor = InColor;
		return *this;
	}

	/**
	 * 테두리 색상 설정
	 * @param InColor 테두리 색상
	 * @return 체이닝용 참조
	 */
	SBorderBox& SetBorderColor(const FSlateColor& InColor)
	{
		BorderColor = InColor;
		return *this;
	}

	/**
	 * 테두리 두께 설정
	 * @param InThickness 테두리 두께 (픽셀)
	 * @return 체이닝용 참조
	 */
	SBorderBox& SetBorderThickness(float InThickness)
	{
		BorderThickness = InThickness;
		return *this;
	}

	/**
	 * 모서리 둥글기 설정
	 * @param InRadius 모서리 반지름 (픽셀)
	 * @return 체이닝용 참조
	 */
	SBorderBox& SetCornerRadius(float InRadius)
	{
		CornerRadius = InRadius;
		return *this;
	}

private:
	// =====================================================
	// 속성
	// =====================================================

	/** 배경 색상 */
	FSlateColor BackgroundColor = FSlateColor(0.0f, 0.0f, 0.0f, 0.5f);  // 기본값: 반투명 검정

	/** 테두리 색상 */
	FSlateColor BorderColor = FSlateColor(1.0f, 1.0f, 1.0f, 0.3f);  // 기본값: 반투명 흰색

	/** 테두리 두께 */
	float BorderThickness = 2.0f;

	/** 모서리 둥글기 */
	float CornerRadius = 8.0f;
};
