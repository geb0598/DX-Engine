#include "pch.h"
#include "SBorderBox.h"

void SBorderBox::Paint(FD2DRenderer& Renderer, const FGeometry& Geometry)
{
	if (!IsVisible())
		return;

	// 배경 그리기 (채워진 둥근 사각형)
	Renderer.DrawFilledRoundedRect(
		Geometry.AbsolutePosition,
		Geometry.GetAbsoluteSize(),
		BackgroundColor,
		CornerRadius
	);

	// 테두리 그리기 (둥근 사각형 테두리)
	Renderer.DrawRoundedRect(
		Geometry.AbsolutePosition,
		Geometry.GetAbsoluteSize(),
		BorderColor,
		CornerRadius,
		BorderThickness
	);
}
