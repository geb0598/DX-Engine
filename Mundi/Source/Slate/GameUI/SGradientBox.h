#pragma once

/**
 * @file SGradientBox.h
 * @brief 그라데이션 배경 위젯
 *
 * 중앙에서 좌우로 갈수록 페이드되는 그라데이션 박스입니다.
 */

#include "SWidget.h"

/**
 * @class SGradientBox
 * @brief 가로 그라데이션 배경 위젯
 */
class SGradientBox : public SWidget
{
public:
	SGradientBox() = default;
	virtual ~SGradientBox() = default;

	// =====================================================
	// 렌더링
	// =====================================================

	void Paint(FD2DRenderer& Renderer, const FGeometry& Geometry) override;

	// =====================================================
	// 속성 설정
	// =====================================================

	/**
	 * 중앙 색상 설정
	 * @param InColor 중앙 색상 (좌우는 투명하게 페이드)
	 * @return 체이닝용 참조
	 */
	SGradientBox& SetColor(const FSlateColor& InColor)
	{
		Color = InColor;
		return *this;
	}

	/**
	 * 페이드 폭 설정
	 * @param InFadeWidth 페이드 폭 (픽셀)
	 * @return 체이닝용 참조
	 */
	SGradientBox& SetFadeWidth(float InFadeWidth)
	{
		FadeWidth = InFadeWidth;
		return *this;
	}

private:
	// =====================================================
	// 속성
	// =====================================================

	/** 중앙 색상 */
	FSlateColor Color = FSlateColor(0.0f, 0.0f, 0.0f, 0.5f);  // 기본값: 반투명 검정

	/** 페이드 폭 */
	float FadeWidth = 100.0f;
};
