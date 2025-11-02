#include "pch.h"
#include "Component/Public/ShapeComponent.h"

IMPLEMENT_ABSTRACT_CLASS(UShapeComponent, UPrimitiveComponent)

UShapeComponent::UShapeComponent()
{
	bCanEverTick = false;
	// ShapeComponent는 충돌 감지용이므로 에디터에서 선택 불가
	bCanPick = false;
}
