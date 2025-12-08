#include "pch.h"
#include "SProgressBar.h"

void SProgressBar::Paint(FD2DRenderer& Renderer, const FGeometry& Geometry)
{
	if (!IsVisible())
		return;

	FVector2D Position = Geometry.AbsolutePosition;
	FVector2D Size = Geometry.GetAbsoluteSize();

	// 배경/테두리의 둥근 정도 계산 (세로 크기의 절반 = 캡슐 모양)
	float BackgroundCornerRadius = Size.Y * 0.5f;

	// 배경 그리기 (채워진 둥근 사각형)
	Renderer.DrawFilledRoundedRect(
		Position,
		Size,
		BackgroundColor,
		BackgroundCornerRadius
	);

	// 진행 바 그리기 (채워진 둥근 사각형, 패딩 적용)
	if (Percent > 0.0f)
	{
		// 패딩 적용 (테두리와 진행 바 사이 공백)
		float Padding = 3.0f;
		FVector2D BarPosition = FVector2D(Position.X + Padding, Position.Y + Padding);
		FVector2D BarSize = FVector2D(Size.X - Padding * 2.0f, Size.Y - Padding * 2.0f);

		if (bHorizontal)
		{
			// 수평 진행 바 (왼쪽에서 오른쪽으로)
			BarSize.X = (Size.X - Padding * 2.0f) * Percent;
		}
		else
		{
			// 수직 진행 바 (아래에서 위로)
			float BarHeight = (Size.Y - Padding * 2.0f) * Percent;
			BarPosition.Y = Position.Y + Padding + (Size.Y - Padding * 2.0f) - BarHeight;
			BarSize.Y = BarHeight;
		}

		// 진행 바 채우기 (배경과 동일한 둥근 정도)
		float BarCornerRadius = (Size.Y - Padding * 2.0f) * 0.5f;  // 세로 크기의 절반 = 완전히 둥근 끝
		Renderer.DrawFilledRoundedRect(
			BarPosition,
			BarSize,
			BarColor,
			BarCornerRadius
		);
	}

	// 테두리 그리기 (선, 배경과 동일한 둥근 정도)
	if (BorderThickness > 0.0f)
	{
		Renderer.DrawRoundedRect(
			Position,
			Size,
			BorderColor,
			BackgroundCornerRadius,
			BorderThickness
		);
	}
}
