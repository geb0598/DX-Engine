#pragma once
#include "Core/Public/Object.h"
#include "Global/Enum.h"

class UWorld;
class AActor;
class UPrimitiveComponent;
class UPointLightComponent;
class ULightComponent;
class FOctree;

UCLASS()
class ULevel :
	public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(ULevel, UObject)
public:
	ULevel();
	~ULevel() override;

	virtual void Init();

	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

	const TArray<AActor*>& GetLevelActors() const { return LevelActors; }
	const TArray<AActor*>& GetTemplateActors() const { return TemplateActors; }

	void AddActorToLevel(AActor* InActor);
	void RegisterTemplateActor(AActor* InActor);
	void UnregisterTemplateActor(AActor* InActor);

	// Template Actor 검색 함수
	AActor* FindTemplateActorByName(const FName& InName) const;
	TArray<AActor*> FindTemplateActorsByClass(UClass* InClass) const;

	// Deferred BeginPlay 시스템 (Unreal Engine 방식)
	void AddPendingBeginPlayActor(AActor* InActor);
	void ProcessPendingBeginPlay();

	void AddLevelComponent(AActor* Actor);

	void RegisterComponent(UActorComponent* InComponent);
	void UnregisterComponent(UActorComponent* InComponent);
	bool DestroyActor(AActor* InActor);

	uint64 GetShowFlags() const { return ShowFlags; }
	void SetShowFlags(uint64 InShowFlags) { ShowFlags = InShowFlags; }

	void UpdatePrimitiveInOctree(UPrimitiveComponent* InComponent);

	FOctree* GetStaticOctree() { return StaticOctree; }

	/**
	 * @brief Octree 범위 밖에 있는 동적 프리미티브 목록 반환
	 * @return 동적 프리미티브 배열 (값 복사)
	 * @note 참조 무효화 방지를 위해 값으로 반환 (각 호출자가 독립적인 복사본 획득)
	 * @todo 성능 최적화: DirtyFlag와 캐시된 배열 도입
	 *       - 멤버 변수: TArray<UPrimitiveComponent*> CachedDynamicPrimitives
	 *       - 플래그: bool bIsDynamicPrimitivesCacheDirty
	 *       - OnPrimitiveUpdated/OnPrimitiveUnregistered에서 DirtyFlag 설정
	 *       - GetDynamicPrimitives()에서 캐시가 유효하면 재사용
	 *       - 예상 성능 개선: 매 프레임 복사 비용 제거 (특히 다중 호출 시)
	 */
	TArray<UPrimitiveComponent*> GetDynamicPrimitives() const
	{
		TArray<UPrimitiveComponent*> Result;
		Result.Reserve(DynamicPrimitiveMap.Num());

		for (auto [Component, TimePoint] : DynamicPrimitiveMap)
		{
			Result.Add(Component);
		}
		return Result;
	}

	friend class UWorld;
public:
	virtual UObject* Duplicate() override;

protected:
	virtual void DuplicateSubObjects(UObject* DuplicatedObject) override;

private:
	AActor* SpawnActorToLevel(UClass* InActorClass, JSON* ActorJsonData = nullptr);

	TArray<AActor*> LevelActors;	// 레벨이 보유하고 있는 모든 Actor를 배열로 저장합니다.
	TArray<AActor*> TemplateActors;	// bIsTemplate이 true인 Actor들의 캐시 (빠른 조회용)

	// 지연 삭제를 위한 리스트
	TArray<AActor*> ActorsToDelete;

	// Deferred BeginPlay - 다음 Tick에서 BeginPlay를 호출할 Actor들 (중복 방지를 위해 TSet 사용)
	TSet<AActor*> PendingBeginPlayActors;

	uint64 ShowFlags =
		static_cast<uint64>(EEngineShowFlags::SF_Billboard) |
		static_cast<uint64>(EEngineShowFlags::SF_StaticMesh) |
		static_cast<uint64>(EEngineShowFlags::SF_Text) |
		static_cast<uint64>(EEngineShowFlags::SF_Decal) |
		static_cast<uint64>(EEngineShowFlags::SF_Fog) |
		static_cast<uint64>(EEngineShowFlags::SF_Collision);
	
	/*-----------------------------------------------------------------------------
		Octree Management
	-----------------------------------------------------------------------------*/
public:
	void UpdateOctree();
	void UpdateOctreeImmediate();

private:

	void OnPrimitiveUpdated(UPrimitiveComponent* InComponent);

	void OnPrimitiveUnregistered(UPrimitiveComponent* InComponent);

	/** @brief 한 프레임에 Octree에 삽입할 오브젝트의 최대 크기를 결정해서 부하를 여러 프레임에 분산함. */
	static constexpr uint32 MAX_OBJECTS_TO_INSERT_PER_FRAME = 256;
	
	/** @brief 가장 오래전에 움직인 UPrimitiveComponent를 Octree에 삽입하기 위해 필요한 구조체. */
	struct FDynamicPrimitiveData
	{
		UPrimitiveComponent* Primitive;
		float LastMoveTimePoint;

		bool operator>(const FDynamicPrimitiveData& Other) const
		{
			return LastMoveTimePoint > Other.LastMoveTimePoint;
		}
	};
	
	using FDynamicPrimitiveQueue = TQueue<FDynamicPrimitiveData>;

	FOctree* StaticOctree = nullptr;

	/** @brief 가장 오래전에 움직인 UPrimitiveComponent부터 순서대로 Octree에 삽입할 수 있도록 보관 */
	FDynamicPrimitiveQueue DynamicPrimitiveQueue;

	/** @brief 각 UPrimitiveComponent가 움직인 가장 마지막 시간을 기록 */
	TMap<UPrimitiveComponent*, float> DynamicPrimitiveMap;
	
	/*-----------------------------------------------------------------------------
		Lighting Management
	-----------------------------------------------------------------------------*/
public:
	const TArray<ULightComponent*>& GetLightComponents() const { return LightComponents; } 

private:
	TArray<ULightComponent*> LightComponents;
};
