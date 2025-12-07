#include "pch.h"
#include "SGradientBox.h"

void SGradientBox::Paint(FD2DRenderer& Renderer, const FGeometry& Geometry)
{
	if (!IsVisible())
		return;

	// 그라데이션 사각형 그리기
	Renderer.DrawHorizontalGradientRect(
		Geometry.AbsolutePosition,
		Geometry.GetAbsoluteSize(),
		Color,
		FadeWidth
	);
}
