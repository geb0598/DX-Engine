#pragma once
#include "Actor/Public/Pawn.h"

/**
 * @brief 기본 플레이어 Pawn 구현
 * ScriptComponent를 포함하여 Lua 스크립트로 게임 로직 제어
 */
UCLASS()
class ADefaultPawn : public APawn
{
	DECLARE_CLASS(ADefaultPawn, APawn)

public:
	ADefaultPawn();
	~ADefaultPawn() override = default;

private:
	UScriptComponent* ScriptComponent = nullptr;
};
