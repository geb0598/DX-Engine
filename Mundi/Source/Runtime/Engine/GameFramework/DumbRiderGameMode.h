// ────────────────────────────────────────────────────────────────────────────
// DumbRiderGameMode.h
// DumbRider 게임용 게임 모드 - SImage 테스트용
// ────────────────────────────────────────────────────────────────────────────
#pragma once

#include "GameModeBase.h"
#include "Source/Runtime/Core/Memory/PointerTypes.h"
#include "ADumbRiderGameMode.generated.h"

class SImage;
class STextBlock;

/**
 * ADumbRiderGameMode
 *
 * DumbRider 레이싱 게임용 게임 모드
 * SImage를 사용한 텍스처 기반 UI 테스트
 */
UCLASS(DisplayName = "DumbRider 게임 모드", Description = "DumbRider 레이싱 게임 모드입니다.")
class ADumbRiderGameMode : public AGameModeBase
{
public:
    GENERATED_REFLECTION_BODY()

        ADumbRiderGameMode();

protected:
    ~ADumbRiderGameMode() override = default;

public:
    // ────────────────────────────────────────────────
    // 생명주기
    // ────────────────────────────────────────────────

    void BeginPlay() override;
    void Tick(float DeltaSeconds) override;

protected:
    // ────────────────────────────────────────────────
    // UI 위젯
    // ────────────────────────────────────────────────

    /** 테스트용 이미지 위젯 */
    TSharedPtr<SImage> TestImage;

    /** 디버그 텍스트 */
    TSharedPtr<STextBlock> DebugText;

    /** 박스 아이콘 */
    TSharedPtr<SImage> BoxesIcon;

    /** "x BOXES LEFT" 텍스트 이미지 */
    TSharedPtr<SImage> BoxesLeftText;
};