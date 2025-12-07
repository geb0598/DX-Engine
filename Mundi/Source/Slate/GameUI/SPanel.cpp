#include "pch.h"
#include "SPanel.h"
#include <d2d1.h>

// =====================================================
// 렌더링
// =====================================================

void SPanel::Paint(FD2DRenderer& Renderer, const FGeometry& Geometry)
{
    if (!IsVisible())
        return;

    // 회전 적용
    ID2D1DeviceContext* Context = Renderer.GetContext();
    D2D1::Matrix3x2F OldTransform;
    bool bAppliedRotation = false;

    if (Context && Rotation != 0.f)
    {
        // 현재 Transform 저장
        Context->GetTransform(&OldTransform);

        // 패널 중심점 계산
        FVector2D Position = Geometry.AbsolutePosition;
        FVector2D Size = Geometry.GetAbsoluteSize();
        D2D1_POINT_2F Center = D2D1::Point2F(
            Position.X + Size.X * 0.5f,
            Position.Y + Size.Y * 0.5f
        );

        // 회전 Transform 생성
        D2D1::Matrix3x2F RotationTransform =
            D2D1::Matrix3x2F::Rotation(Rotation, Center);

        // Transform 적용
        Context->SetTransform(RotationTransform * OldTransform);
        bAppliedRotation = true;
    }

    // 자식들 렌더링
    PaintChildren(Renderer, Geometry);

    // Transform 복원
    if (bAppliedRotation && Context)
    {
        Context->SetTransform(OldTransform);
    }
}

void SPanel::PaintChildren(FD2DRenderer& Renderer, const FGeometry& Geometry)
{
    for (const FSlot& Slot : ChildSlots)
    {
        if (!Slot.Widget)
            continue;

        // 자식의 Geometry 계산
        FGeometry ChildGeometry = Geometry.MakeChild(Slot.Position, Slot.Size);

        // 자식 렌더링
        Slot.Widget->Paint(Renderer, ChildGeometry);
    }
}

// =====================================================
// 입력 처리 (자식에게 전파)
// =====================================================

FReply SPanel::OnMouseButtonDown(const FGeometry& Geometry, const FVector2D& MousePosition)
{
    if (!IsVisible() || !bIsEnabled)
        return FReply::Unhandled();

    // 역순으로 순회 (위에 있는 위젯부터)
    for (int32 i = ChildSlots.Num() - 1; i >= 0; --i)
    {
        const FSlot& Slot = ChildSlots[i];
        if (!Slot.Widget)
            continue;

        FGeometry ChildGeometry = Geometry.MakeChild(Slot.Position, Slot.Size);

        // 자식이 해당 위치에 있는지 확인
        if (Slot.Widget->HitTest(ChildGeometry, MousePosition))
        {
            FReply Reply = Slot.Widget->OnMouseButtonDown(ChildGeometry, MousePosition);
            if (Reply.IsEventHandled())
                return Reply;
        }
    }

    return FReply::Unhandled();
}

FReply SPanel::OnMouseButtonUp(const FGeometry& Geometry, const FVector2D& MousePosition)
{
    if (!IsVisible() || !bIsEnabled)
        return FReply::Unhandled();

    // 역순으로 순회 (위에 있는 위젯부터)
    for (int32 i = ChildSlots.Num() - 1; i >= 0; --i)
    {
        const FSlot& Slot = ChildSlots[i];
        if (!Slot.Widget)
            continue;

        FGeometry ChildGeometry = Geometry.MakeChild(Slot.Position, Slot.Size);

        // 자식이 해당 위치에 있는지 확인
        if (Slot.Widget->HitTest(ChildGeometry, MousePosition))
        {
            FReply Reply = Slot.Widget->OnMouseButtonUp(ChildGeometry, MousePosition);
            if (Reply.IsEventHandled())
                return Reply;
        }
    }

    return FReply::Unhandled();
}

FReply SPanel::OnMouseMove(const FGeometry& Geometry, const FVector2D& MousePosition)
{
    if (!IsVisible() || !bIsEnabled)
        return FReply::Unhandled();

    // 호버 상태 업데이트
    UpdateHoveredWidget(Geometry, MousePosition);

    // 역순으로 순회 (위에 있는 위젯부터)
    for (int32 i = ChildSlots.Num() - 1; i >= 0; --i)
    {
        const FSlot& Slot = ChildSlots[i];
        if (!Slot.Widget)
            continue;

        FGeometry ChildGeometry = Geometry.MakeChild(Slot.Position, Slot.Size);

        if (Slot.Widget->HitTest(ChildGeometry, MousePosition))
        {
            FReply Reply = Slot.Widget->OnMouseMove(ChildGeometry, MousePosition);
            if (Reply.IsEventHandled())
                return Reply;
        }
    }

    return FReply::Unhandled();
}

// =====================================================
// 자식 관리
// =====================================================

FSlot& SPanel::AddChild(const TSharedPtr<SWidget>& Child)
{
    FSlot NewSlot;
    NewSlot.Widget = Child;
    ChildSlots.Add(NewSlot);
    return ChildSlots[ChildSlots.Num() - 1];
}

bool SPanel::RemoveChild(const TSharedPtr<SWidget>& Child)
{
    for (int32 i = 0; i < ChildSlots.Num(); ++i)
    {
        if (ChildSlots[i].Widget == Child)
        {
            ChildSlots.RemoveAt(i);
            return true;
        }
    }
    return false;
}

void SPanel::ClearChildren()
{
    ChildSlots.Empty();
}

// =====================================================
// 히트 테스트
// =====================================================

TSharedPtr<SWidget> SPanel::FindWidgetAt(const FGeometry& Geometry, const FVector2D& AbsolutePosition)
{
    if (!IsVisible())
        return nullptr;

    // 역순으로 순회 (위에 있는 위젯부터)
    for (int32 i = ChildSlots.Num() - 1; i >= 0; --i)
    {
        const FSlot& Slot = ChildSlots[i];
        if (!Slot.Widget || !Slot.Widget->IsVisible())
            continue;

        FGeometry ChildGeometry = Geometry.MakeChild(Slot.Position, Slot.Size);

        if (ChildGeometry.IsPointInside(AbsolutePosition))
        {
            // 자식이 SPanel인 경우 재귀적으로 찾기
            if (SPanel* ChildPanel = dynamic_cast<SPanel*>(Slot.Widget.Get()))
            {
                TSharedPtr<SWidget> Found = ChildPanel->FindWidgetAt(ChildGeometry, AbsolutePosition);
                if (Found)
                    return Found;
            }

            return Slot.Widget;
        }
    }

    return nullptr;
}

// =====================================================
// 호버 상태 관리
// =====================================================

void SPanel::UpdateHoveredWidget(const FGeometry& Geometry, const FVector2D& MousePosition)
{
    TSharedPtr<SWidget> NewHovered = nullptr;

    // 현재 마우스 위치의 위젯 찾기
    for (int32 i = ChildSlots.Num() - 1; i >= 0; --i)
    {
        const FSlot& Slot = ChildSlots[i];
        if (!Slot.Widget || !Slot.Widget->IsVisible())
            continue;

        FGeometry ChildGeometry = Geometry.MakeChild(Slot.Position, Slot.Size);

        if (Slot.Widget->HitTest(ChildGeometry, MousePosition))
        {
            NewHovered = Slot.Widget;
            break;
        }
    }

    TSharedPtr<SWidget> OldHovered = HoveredChild.Pin();

    // 호버된 위젯이 변경되었는지 확인
    if (NewHovered != OldHovered)
    {
        // 이전 호버 위젯에 Leave 이벤트
        if (OldHovered)
        {
            OldHovered->OnMouseLeave();
        }

        // 새 호버 위젯에 Enter 이벤트
        if (NewHovered)
        {
            NewHovered->OnMouseEnter();
        }

        HoveredChild = NewHovered;
    }
}
