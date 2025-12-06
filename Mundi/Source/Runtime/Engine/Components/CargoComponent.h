#pragma once
#include "SceneComponent.h"
#include "UCargoComponent.generated.h"

UENUM()
enum class ECargoSwayMode : uint8
{
    AllAxis,    // 모든 방향으로 짐이 흔들림
    RollOnly,   // Roll (좌/우) 방향으로만 짐이 흔들림
    PitchOnly,  // Pitch (앞/뒤) 방향으로만 짐이 흔들림
    Locked      // 짐이 흔들리지 않음
};

enum class ECargoState
{
    Stable,     // 안정적 (컴포넌트 계층에 속함)
    Collapsed   // 붕괴됨 (물리 시뮬레이션 시작)
};

UCLASS(DisplayName="화물 시스템", Description="차량 적재물의 동적 흔들림 및 붕괴 시스템")
class UCargoComponent : public USceneComponent
{
public:
    GENERATED_REFLECTION_BODY()

    UCargoComponent();
    virtual ~UCargoComponent();

    // ====================================================================
    // 언리얼 엔진 생명주기 (Lifecycle)
    // ====================================================================
    virtual void InitializeComponent() override;
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaSeconds) override;
    virtual void OnRegister(UWorld* InWorld) override;
    virtual void OnUnregister() override;

    // ====================================================================
    // 화물 설정 및 초기화
    // ====================================================================

    /** 흔들림 허용 축 (좌우만 / 앞뒤좌우 / 고정) */
    UPROPERTY(EditAnywhere, Category="Cargo Physics")
    ECargoSwayMode SwayMode = ECargoSwayMode::AllAxis;

    /** 기본 밀림 단위 (m). 위층으로 갈수록 이 값의 배수로 밀려남 */
    UPROPERTY(EditAnywhere, Category="Cargo Physics")
    float BaseSlideAmount = 0.2f;

    /** 흔들림 최소 반응 속도 (기울기가 작을 때) */
    UPROPERTY(EditAnywhere, Category="Cargo Physics")
    float MinSwaySpeed = 2.0f;

    /** 흔들림 최대 반응 속도 (기울기가 클 때) -> 더 빠르게 쏠림 */
    UPROPERTY(EditAnywhere, Category="Cargo Physics")
    float MaxSwaySpeed = 10.0f;

    /** 차량 자체가 이 각도(Degree) 이상 기울면 즉시 붕괴 */
    UPROPERTY(EditAnywhere, Category="Cargo Physics")
    float CriticalAngle = 50.0f;

    /** 화물이 부모의 바닥 면적 대비 벗어날 수 있는 비율 (0.5 = 모서리) */
    UPROPERTY(EditAnywhere, Category="Cargo Physics")
    float FalloffThreshold = 0.6f;

    /** 붕괴 시 화물을 바깥으로 밀어내는 힘의 크기 */
    UPROPERTY(EditAnywhere, Category="Cargo Physics")
    float CollapseImpulse = 5.0f; 

    /** 붕괴 시 화물에 가해지는 랜덤 회전력의 크기 */
    UPROPERTY(EditAnywhere, Category="Cargo Physics")
    float RandomSpinImpulse = 1.0f;

    /** 화물이 모두 붕괴될 때 운전자를 발사시키는 힘의 크기 */
    float EjectionImpulse = 10.0f;

    /** 랜덤으로 선택될 화물 메쉬 경로 목록 */
    UPROPERTY(EditAnywhere, Category="Cargo Gen")
    TArray<FString> CargoMeshOptions;

    /** 화물 회전 최소값 (Z축, 도) */
    UPROPERTY(EditAnywhere, Category="Cargo Gen")
    float MinCargoRotation = -15.0f;

    /** 화물 회전 최대값 */
    UPROPERTY(EditAnywhere, Category="Cargo Gen")
    float MaxCargoRotation = 15.0f;
    
    /** * 화물 박스들을 생성하고 계층 구조로 연결합니다.
     * @param BoxCount 생성할 박스의 개수
     * @param MeshPath 박스에 사용할 스태틱 메시 경로
     */
    void InitializeCargo(int32 BoxCount);

private:
    // ====================================================================
    // 내부 구현 함수
    // ====================================================================

    /** 현재 오프셋이 임계값을 넘었는지 검사 */
    bool CheckCollapseRelative(UStaticMeshComponent* ParentCargo, const FVector& CurrentLocation);

    /** 지정된 인덱스부터의 모든 화물을 액터에서 분리하고 물리 시뮬레이션을 시작 */
    void CollapseFrom(int32 StartIndex);

    /** 모든 화물을 액터에서 분리하고 물리 시뮬레이션을 시작 */
    void CollapseAll();

    /** 현재 화물의 상태 (모든 화물이 추락하면 ECargoState::Collapsed) */
    ECargoState CurrentState = ECargoState::Stable;

    /** 현재 액터에 붙어있는 (붕괴되지 않은) 화물의 개수 */
    int32 ValidCargoCount = 0;

    /** 관리 중인 화물 컴포넌트 목록 (0:최하단, N - 1:최상단) */
    TArray<UStaticMeshComponent*> CargoBoxes;
};
