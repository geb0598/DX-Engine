#pragma once
#include "Actor/Public/Pawn.h"

class ULightSensorComponent;

/**
 * @brief 기본 플레이어 Pawn 구현
 * ScriptComponent와 LightSensorComponent를 포함
 */
UCLASS()
class ADefaultPawn :
	public APawn
{
	DECLARE_CLASS(ADefaultPawn, APawn)

public:
	ADefaultPawn();
	~ADefaultPawn() override = default;

private:
	UScriptComponent* ScriptComponent = nullptr;
	ULightSensorComponent* LightSensorComponent = nullptr;
};
