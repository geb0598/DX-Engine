#pragma once

/**
 * @file SProgressBar.h
 * @brief 진행률 표시 바 위젯
 *
 * 0.0 ~ 1.0 범위의 진행률을 시각적으로 표시합니다.
 * 목적지까지의 거리, 로딩 진행률 등에 사용됩니다.
 */

#include "SWidget.h"

/**
 * @class SProgressBar
 * @brief 진행률 표시 바 위젯
 */
class SProgressBar : public SWidget
{
public:
	SProgressBar() = default;
	virtual ~SProgressBar() = default;

	// =====================================================
	// 렌더링
	// =====================================================

	void Paint(FD2DRenderer& Renderer, const FGeometry& Geometry) override;

	// =====================================================
	// 속성 설정
	// =====================================================

	/**
	 * 진행률 설정
	 * @param InPercent 진행률 (0.0 ~ 1.0)
	 * @return 체이닝용 참조
	 */
	SProgressBar& SetPercent(float InPercent)
	{
		Percent = FMath::Clamp(InPercent, 0.0f, 1.0f);
		return *this;
	}

	/**
	 * 진행 바 색상 설정
	 * @param InColor 진행 바 색상
	 * @return 체이닝용 참조
	 */
	SProgressBar& SetBarColor(const FSlateColor& InColor)
	{
		BarColor = InColor;
		return *this;
	}

	/**
	 * 배경 색상 설정
	 * @param InColor 배경 색상
	 * @return 체이닝용 참조
	 */
	SProgressBar& SetBackgroundColor(const FSlateColor& InColor)
	{
		BackgroundColor = InColor;
		return *this;
	}

	/**
	 * 테두리 색상 설정
	 * @param InColor 테두리 색상
	 * @return 체이닝용 참조
	 */
	SProgressBar& SetBorderColor(const FSlateColor& InColor)
	{
		BorderColor = InColor;
		return *this;
	}

	/**
	 * 테두리 두께 설정
	 * @param InThickness 테두리 두께 (픽셀)
	 * @return 체이닝용 참조
	 */
	SProgressBar& SetBorderThickness(float InThickness)
	{
		BorderThickness = InThickness;
		return *this;
	}

	/**
	 * 모서리 둥글기 설정
	 * @param InRadius 모서리 반지름 (픽셀)
	 * @return 체이닝용 참조
	 */
	SProgressBar& SetCornerRadius(float InRadius)
	{
		CornerRadius = InRadius;
		return *this;
	}

	/**
	 * 방향 설정 (수평/수직)
	 * @param bInHorizontal true면 수평, false면 수직
	 * @return 체이닝용 참조
	 */
	SProgressBar& SetHorizontal(bool bInHorizontal)
	{
		bHorizontal = bInHorizontal;
		return *this;
	}

	/**
	 * 현재 진행률 가져오기
	 * @return 진행률 (0.0 ~ 1.0)
	 */
	float GetPercent() const { return Percent; }

private:
	// =====================================================
	// 속성
	// =====================================================

	/** 진행률 (0.0 ~ 1.0) */
	float Percent = 0.0f;

	/** 진행 바 색상 */
	FSlateColor BarColor = FSlateColor(0.576f, 0.988f, 0.517f, 1.0f);  // 기본값: 초록색

	/** 배경 색상 */
	FSlateColor BackgroundColor = FSlateColor(0.2f, 0.2f, 0.2f, 0.8f);  // 기본값: 어두운 회색

	/** 테두리 색상 */
	FSlateColor BorderColor = FSlateColor(0.7f, 0.7f, 0.7f, 1.0f);  // 기본값: 회색

	/** 테두리 두께 */
	float BorderThickness = 2.0f;

	/** 모서리 둥글기 */
	float CornerRadius = 4.0f;

	/** 방향 (true: 수평, false: 수직) */
	bool bHorizontal = true;
};
