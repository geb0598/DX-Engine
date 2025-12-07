#include "pch.h"
#include "STextBlock.h"
#include <d2d1.h>

// =====================================================
// 렌더링
// =====================================================

void STextBlock::Paint(FD2DRenderer& Renderer, const FGeometry& Geometry)
{
    if (!IsVisible())
        return;

    FWideString DisplayText = GetText();
    if (DisplayText.empty())
        return;

    FVector2D Position = Geometry.AbsolutePosition;
    FVector2D Size = Geometry.GetAbsoluteSize();

    // 회전 적용
    ID2D1DeviceContext* Context = Renderer.GetContext();
    D2D1::Matrix3x2F OldTransform;
    bool bAppliedRotation = false;

    if (Context && Rotation != 0.f)
    {
        // 현재 Transform 저장
        Context->GetTransform(&OldTransform);

        // 텍스트 중심점 계산
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

    // 그림자 먼저 그리기
    if (bHasShadow)
    {
        Renderer.DrawText(
            DisplayText,
            Position + ShadowOffset,
            Size,
            ShadowColor,
            FontSize,
            HAlign,
            VAlign
        );
    }

    // 메인 텍스트
    Renderer.DrawText(
        DisplayText,
        Position,
        Size,
        TextColor,
        FontSize,
        HAlign,
        VAlign
    );

    // Transform 복원
    if (bAppliedRotation && Context)
    {
        Context->SetTransform(OldTransform);
    }
}

// =====================================================
// 텍스트 설정
// =====================================================

STextBlock& STextBlock::SetText(const FWideString& InText)
{
    Text = InText;
    TextGetter = nullptr;  // 정적 텍스트 사용 시 바인딩 해제
    return *this;
}

STextBlock& STextBlock::SetText(const FString& InText)
{
    // ANSI -> Wide 변환
    if (InText.empty())
    {
        Text.clear();
    }
    else
    {
        int WideLen = MultiByteToWideChar(CP_UTF8, 0, InText.c_str(), -1, nullptr, 0);
        Text.resize(static_cast<size_t>(WideLen - 1));
        MultiByteToWideChar(CP_UTF8, 0, InText.c_str(), -1, Text.data(), WideLen);
    }
    TextGetter = nullptr;
    return *this;
}

STextBlock& STextBlock::BindText(FTextGetter InGetter)
{
    TextGetter = std::move(InGetter);
    return *this;
}

FWideString STextBlock::GetText() const
{
    if (TextGetter)
    {
        return TextGetter();
    }
    return Text;
}

// =====================================================
// 스타일 설정
// =====================================================

STextBlock& STextBlock::SetColor(const FSlateColor& InColor)
{
    TextColor = InColor;
    return *this;
}

STextBlock& STextBlock::SetFontSize(float InSize)
{
    FontSize = InSize;
    return *this;
}

STextBlock& STextBlock::SetHAlign(ETextHAlign InAlign)
{
    HAlign = InAlign;
    return *this;
}

STextBlock& STextBlock::SetVAlign(ETextVAlign InAlign)
{
    VAlign = InAlign;
    return *this;
}

STextBlock& STextBlock::SetShadow(bool bEnable, const FVector2D& Offset, const FSlateColor& Color)
{
    bHasShadow = bEnable;
    ShadowOffset = Offset;
    ShadowColor = Color;
    return *this;
}

STextBlock& STextBlock::SetRotation(float InRotation)
{
    Rotation = InRotation;
    return *this;
}
