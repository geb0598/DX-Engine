#pragma once
#include "Actor.h"
#include "BoxComponent.h"
#include "AGameVictoryVolume.generated.h"

class UPrimitiveComponent;

/*
* GameVictoryVolume: 플레이어가 접촉시 게임 승리를 트리거하는 볼륨.
* 
* Description:
* 박스 컴포넌트를 루트 컴포넌트로 갖는다.
* 박스 컴포넌트는 트리거 용도로 설정되어 있으며, 플레이어가 접촉 시 게임 승리를 트리거하는 역할을 한다.
* - 델리게이트를 사용해 박스 컴포넌트의 충돌 시작 이벤트 수신
* - 이벤트 처리는 Wolrd의 GameMode를 통해 게임 승리 로직을 호출
* 
* Note: 지금은 AActor를 직접 상속하지만, 나중에 다른 용도의 트리커 볼륨이 추가된다면
* AVolume(충돌 볼륨 전용 Actor) 클래스를 만들어 상속시키는 것을 고려.
* 
*/

UCLASS(DisplayName = "게임 승리 볼륨", Description = "플레이어가 접촉 시 게임 승리를 트리거하는 볼륨입니다.")
class AGameVictoryVolume : public AActor
{
public:
    GENERATED_REFLECTION_BODY()

    AGameVictoryVolume();
    ~AGameVictoryVolume() override = default;

    // 생명주기
    void BeginPlay() override;
    void EndPlay() override;

protected:
	// Overlap 콜백 (ShapeComponent가 브로드캐스트)
	void OnBeginOverlap(UPrimitiveComponent* MyComp, UPrimitiveComponent* OtherComp);

public:
	// 복제/직렬화
	void DuplicateSubObjects() override;
	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

public:
	/** 트리거 영역으로 사용할 Box 콜라이더 */
	UPROPERTY(VisibleAnywhere, Category = "VictoryVolume")
	UBoxComponent* TriggerVolume;

	/** 플레이어를 식별할 태그 (기본: "player") */
	UPROPERTY(EditAnywhere, Category = "VictoryVolume")
	FString PlayerTag = "player";
};
