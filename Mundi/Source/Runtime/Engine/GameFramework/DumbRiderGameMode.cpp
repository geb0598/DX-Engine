// ────────────────────────────────────────────────────────────────────────────
// DumbRiderGameMode.cpp
// DumbRider 게임용 게임 모드 구현
// ────────────────────────────────────────────────────────────────────────────
#include "pch.h"
#include "DumbRiderGameMode.h"
#include "DancingCharacter.h"

#include "GameUI/SGameHUD.h"
#include "GameUI/SImage.h"
#include "GameUI/STextBlock.h"

// ────────────────────────────────────────────────────────────────────────────
// 생성자
// ────────────────────────────────────────────────────────────────────────────

ADumbRiderGameMode::ADumbRiderGameMode()
{
	DefaultPawnClass = ADancingCharacter::StaticClass();
}

// ────────────────────────────────────────────────────────────────────────────
// 생명주기
// ────────────────────────────────────────────────────────────────────────────

void ADumbRiderGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (!SGameHUD::Get().IsInitialized())
		return;

	// 테스트용 이미지 (화면 좌상단)
	TestImage = MakeShared<SImage>();
	TestImage->SetTexture(L"Data/Textures/Dumb/DumbRider.png");

	SGameHUD::Get().AddWidget(TestImage)
		.SetAnchor(0.1f, 0.1f)
		.SetOffset(0.f, 0.f)
		.SetSize(250.f, 250.f);

	// Fade In + Scale 애니메이션 테스트 (더 느리게, 더 명확하게)
	TestImage->SetOpacity(0.0f);  // 완전히 투명하게 시작
	TestImage->SetScale(FVector2D(0.3f, 0.3f));  // 아주 작게 시작
	TestImage->PlayFadeIn(2.0f, EEasingType::EaseOutCubic);  // 2초 동안 Fade In
	TestImage->PlayScaleAnimation(FVector2D(1.0f, 1.0f), 2.0f, EEasingType::EaseOutCubic);  // 2초 동안 확대

	// 디버그 텍스트 (이미지 아래)
	DebugText = MakeShared<STextBlock>();
	DebugText->SetText(L"SImage Animation Test")
		.SetFontSize(24.f)
		.SetColor(FSlateColor::White())
		.SetShadow(true, FVector2D(2.f, 2.f), FSlateColor::Black());

	SGameHUD::Get().AddWidget(DebugText)
		.SetAnchor(0.1f, 0.1f)
		.SetOffset(0.f, 220.f)
		.SetSize(300.f, 40.f);
}

void ADumbRiderGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	SGameHUD::Get().UpdateAnimations(DeltaSeconds);
}
