#include "pch.h"
#include "Actor/Public/Actor.h"
#include "Component/Public/LightComponent.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Component/Public/PointLightComponent.h"
#include "Component/Public/DirectionalLightComponent.h"
#include "Component/Public/AmbientLightComponent.h"
#include "Component/Public/SpotLightComponent.h"
#include "Core/Public/Object.h"
#include "Editor/Public/Editor.h"
#include "Global/Octree.h"
#include "Global/OverlapInfo.h"
#include "Level/Public/Level.h"
#include "Manager/Config/Public/ConfigManager.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Utility/Public/JsonSerializer.h"
#include "Manager/UI/Public/ViewportManager.h"
#include "Physics/Public/Bounds.h"
#include "Physics/Public/AABB.h"
#include "Physics/Public/CollisionHelper.h"
#include "Physics/Public/HitResult.h"
#include <json.hpp>

IMPLEMENT_CLASS(ULevel, UObject)

ULevel::ULevel()
{
	// Octree covering -32000 to 32000 on each axis (64000 unit range)
	// With MAX_DEPTH=14, minimum node size is ~3.9 units
	StaticOctree = new FOctree(FVector(0, 0, 0), 64000, 0);
}

ULevel::~ULevel()
{
	// LevelActors 배열에 남아있는 모든 액터의 메모리를 해제합니다.
	// 역순 루프를 사용하여 DestroyActor 내부의 Remove 호출로 인한 반복자 무효화 방지
	while (!LevelActors.IsEmpty())
	{
		DestroyActor(LevelActors.Last());
	}
	LevelActors.Empty();

	// 모든 액터 객체가 삭제되었으므로, 포인터를 담고 있던 컨테이너들을 비웁니다.
	SafeDelete(StaticOctree);
}

void ULevel::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	// 불러오기
	if (bInIsLoading)
	{
		// NOTE: 레벨 로드 시 NextUUID를 변경하면 UUID 충돌이 발생하므로 관련 기능 구현을 보류합니다.
		uint32 NextUUID = 0;
		FJsonSerializer::ReadUint32(InOutHandle, "NextUUID", NextUUID);

		JSON ActorsJson;
		if (FJsonSerializer::ReadObject(InOutHandle, "Actors", ActorsJson))
		{
			for (auto& Pair : ActorsJson.ObjectRange())
			{
				JSON& ActorDataJson = Pair.second;

				FString TypeString;
				FJsonSerializer::ReadString(ActorDataJson, "Type", TypeString);

				UClass* ActorClass = UClass::FindClass(TypeString);
				SpawnActorToLevel(ActorClass, &ActorDataJson);
			}
		}

		// 뷰포트 카메라 정보 로드
		UViewportManager::GetInstance().SerializeViewports(bInIsLoading, InOutHandle);
	}
	// 저장
	else
	{
		// NOTE: 레벨 로드 시 NextUUID를 변경하면 UUID 충돌이 발생하므로 관련 기능 구현을 보류합니다.
		InOutHandle["NextUUID"] = 0;

		JSON ActorsJson = json::Object();
		for (AActor* Actor : LevelActors)
		{
			JSON ActorJson;
			ActorJson["Type"] = Actor->GetClass()->GetName().ToString();
			Actor->Serialize(bInIsLoading, ActorJson);

			ActorsJson[std::to_string(Actor->GetUUID())] = ActorJson;
		}
		InOutHandle["Actors"] = ActorsJson;

		// 뷰포트 카메라 정보 저장
		UViewportManager::GetInstance().SerializeViewports(bInIsLoading, InOutHandle);
	}
}

void ULevel::Init()
{
	// BeginPlay 도중 생성되는 액터가 존재할 수 있으므로 인덱스 기반
	for (int32 Idx = 0; Idx < LevelActors.Num(); ++Idx)
	{
		AActor* Actor = LevelActors[Idx];
		if (Actor)
		{
			Actor->BeginPlay();
		}
	}
}

AActor* ULevel::SpawnActorToLevel(UClass* InActorClass, JSON* ActorJsonData)
{
	if (!InActorClass)
	{
		return nullptr;
	}

	// TODO: BeginPlay 타이밍 문제
	// - 현재: 항상 BeginPlay() 호출 -> Level::Init()에서도 호출 (중복)
	// - Level::Serialize에서 호출 시 레벨 로드된 액터는 BeginPlay 2번 호출됨
	// - 제안: bCallBeginPlay 파라미터 추가 또는 deferred beginplay 재도입 검토
	AActor* NewActor = Cast<AActor>(NewObject(InActorClass, this));
	if (NewActor)
	{
		LevelActors.Add(NewActor);
		if (ActorJsonData != nullptr)
		{
			NewActor->Serialize(true, *ActorJsonData);
		}
		else
		{
			NewActor->InitializeComponents();
		}

		NewActor->BeginPlay();
		AddLevelComponent(NewActor);

		// 템플릿 액터면 캐시에 추가
		if (NewActor->IsTemplate())
		{
			RegisterTemplateActor(NewActor);
		}

		return NewActor;
	}

	return nullptr;
}

void ULevel::RegisterComponent(UActorComponent* InComponent)
{
	if (!InComponent)
	{
		return;
	}

	if (!StaticOctree)
	{
		return;
	}

	if (auto PrimitiveComponent = Cast<UPrimitiveComponent>(InComponent))
	{
		// StaticOctree에 먼저 삽입 시도
		if (!(StaticOctree->Insert(PrimitiveComponent)))
		{
			// 실패하면 DynamicPrimitiveQueue 목록에 추가
			OnPrimitiveUpdated(PrimitiveComponent);
		}

		// Note: Initial overlaps will be detected in next Level::UpdateAllOverlaps() call
	}
	else if (auto LightComponent = Cast<ULightComponent>(InComponent))
	{
		if (auto PointLightComponent = Cast<UPointLightComponent>(LightComponent))
		{
			if (auto SpotLightComponent = Cast<USpotLightComponent>(PointLightComponent))
			{
				LightComponents.Add(SpotLightComponent);
			}
			else
			{
				LightComponents.Add(PointLightComponent);
			}
		}
		if (auto DirectionalLightComponent = Cast<UDirectionalLightComponent>(LightComponent))
		{
			LightComponents.Add(DirectionalLightComponent);
		}
		if (auto AmbientLightComponent = Cast<UAmbientLightComponent>(LightComponent))
		{
			LightComponents.Add(AmbientLightComponent);
		}


	}
	UE_LOG("Level: '%s' 컴포넌트를 씬에 등록했습니다.", InComponent->GetName().ToString().data());
}

void ULevel::UnregisterComponent(UActorComponent* InComponent)
{
	if (!InComponent)
	{
		return;
	}

	if (!StaticOctree)
	{
		return;
	}

	if (auto PrimitiveComponent = Cast<UPrimitiveComponent>(InComponent))
	{
		// StaticOctree에서 제거 시도
		StaticOctree->Remove(PrimitiveComponent);

		OnPrimitiveUnregistered(PrimitiveComponent);
	}
	else if (auto LightComponent = Cast<ULightComponent>(InComponent))
	{
		LightComponents.Remove(LightComponent);
	}

}

void ULevel::AddActorToLevel(AActor* InActor)
{
	if (!InActor)
	{
		return;
	}

	LevelActors.Add(InActor);
}

void ULevel::RegisterTemplateActor(AActor* InActor)
{
	if (!InActor)
	{
		return;
	}

	// 중복 등록 방지
	if (TemplateActors.Contains(InActor))
	{
		return;
	}

	TemplateActors.Add(InActor);
}

void ULevel::UnregisterTemplateActor(AActor* InActor)
{
	if (!InActor)
	{
		return;
	}

	TemplateActors.Remove(InActor);
}

void ULevel::AddLevelComponent(AActor* Actor)
{
	if (!Actor)
	{
		return;
	}

	for (auto& Component : Actor->GetOwnedComponents())
	{
		if (auto PrimitiveComponent = Cast<UPrimitiveComponent>(Component))
		{
			// Octree Insert 시도
			if (!(StaticOctree->Insert(PrimitiveComponent)))
			{
				// 실패하면 DynamicPrimitiveQueue에 추가
				OnPrimitiveUpdated(PrimitiveComponent);
			}
		}
		else if (auto LightComponent = Cast<ULightComponent>(Component))
		{
			if (auto PointLightComponent = Cast<UPointLightComponent>(LightComponent))
			{
				if (auto SpotLightComponent = Cast<USpotLightComponent>(PointLightComponent))
				{
					LightComponents.Add(SpotLightComponent);
				}
				else
				{
					LightComponents.Add(PointLightComponent);
				}
			}
			if (auto DirectionalLightComponent = Cast<UDirectionalLightComponent>(LightComponent))
			{
				LightComponents.Add(DirectionalLightComponent);
			}
			if (auto AmbientLightComponent = Cast<UAmbientLightComponent>(LightComponent))
			{
				LightComponents.Add(AmbientLightComponent);
			}

		}
	}
}

// Level에서 Actor 제거하는 함수
bool ULevel::DestroyActor(AActor* InActor)
{
	if (!InActor)
	{
		return false;
	}

	InActor->EndPlay();

	// 템플릿 액터면 캐시에서 제거
	if (InActor->IsTemplate())
	{
		UnregisterTemplateActor(InActor);
	}

	// 컴포넌트들을 옥트리에서 제거
	for (auto& Component : InActor->GetOwnedComponents())
	{
		UnregisterComponent(Component);
	}

	// LevelActors 리스트에서 제거
	LevelActors.Remove(InActor);

	// Remove Actor Selection
	UEditor* Editor = GEditor->GetEditorModule();
	if (Editor->GetSelectedActor() == InActor)
	{
		Editor->SelectActor(nullptr);
		Editor->SelectComponent(nullptr);
	}

	// Remove
	SafeDelete(InActor);

	return true;
}

void ULevel::UpdatePrimitiveInOctree(UPrimitiveComponent* InComponent)
{
	if (!StaticOctree->Remove(InComponent))
	{
		return;
	}
	OnPrimitiveUpdated(InComponent);
}

UObject* ULevel::Duplicate()
{
	ULevel* Level = Cast<ULevel>(Super::Duplicate());
	Level->ShowFlags = ShowFlags;
	return Level;
}

void ULevel::DuplicateSubObjects(UObject* DuplicatedObject)
{
	Super::DuplicateSubObjects(DuplicatedObject);
	ULevel* DuplicatedLevel = Cast<ULevel>(DuplicatedObject);

	for (AActor* Actor : LevelActors)
	{
		// Template actor는 PIE World로 복제하지 않음 (Editor World에만 존재)
		if (Actor->IsTemplate())
		{
			continue;
		}

		AActor* DuplicatedActor = Cast<AActor>(Actor->Duplicate());
		DuplicatedActor->SetOuter(DuplicatedLevel);  // Actor의 Outer를 Level로 설정
		DuplicatedLevel->LevelActors.Add(DuplicatedActor);
		DuplicatedLevel->AddLevelComponent(DuplicatedActor);
	}
}

/*-----------------------------------------------------------------------------
	Octree Management
-----------------------------------------------------------------------------*/

void ULevel::UpdateOctree()
{
	if (!StaticOctree)
	{
		return;
	}

	uint32 Count = 0;
	FDynamicPrimitiveQueue NotInsertedQueue;

	while (!DynamicPrimitiveQueue.empty() && Count < MAX_OBJECTS_TO_INSERT_PER_FRAME)
	{
		auto [Component, TimePoint] = DynamicPrimitiveQueue.front();
		DynamicPrimitiveQueue.pop();

		auto* FoundTimestampPtr = DynamicPrimitiveMap.Find(Component);
		if (FoundTimestampPtr)
		{
			if (*FoundTimestampPtr <= TimePoint)
			{
				// 큐에 기록된 오브젝트의 마지막 변경 시간 이후로 변경이 없었다면 Octree에 재삽입한다.
				if (StaticOctree->Insert(Component))
				{
					DynamicPrimitiveMap.Remove(Component);
				}
				// 삽입이 안됐다면 다시 Queue에 들어가기 위해 저장
				else
				{
					NotInsertedQueue.push({Component, *FoundTimestampPtr});
				}
				// TODO: 오브젝트의 유일성을 보장하기 위해 StaticOctree->Remove(Component)가 필요한가?
				++Count;
			}
			else
			{
				// 큐에 기록된 오브젝트의 마지막 변경 이후 새로운 변경이 존재했다면 다시 큐에 삽입한다.
				DynamicPrimitiveQueue.push({Component, *FoundTimestampPtr});
			}
		}
	}

	DynamicPrimitiveQueue = NotInsertedQueue;
	if (Count != 0)
	{
		// UE_LOG("UpdateOctree: %d개의 컴포넌트가 업데이트 되었습니다.", Count);
	}
}

void ULevel::UpdateOctreeImmediate()
{
	if (!StaticOctree)
	{
		return;
	}

	uint32 TotalCount = 0;
	FDynamicPrimitiveQueue NotInsertedQueue;

	// MAX_OBJECTS_TO_INSERT_PER_FRAME 제한 없이 모든 큐 비우기
	while (!DynamicPrimitiveQueue.empty())
	{
		auto [Component, TimePoint] = DynamicPrimitiveQueue.front();
		DynamicPrimitiveQueue.pop();

		auto* FoundTimestampPtr = DynamicPrimitiveMap.Find(Component);
		if (FoundTimestampPtr)
		{
			if (*FoundTimestampPtr <= TimePoint)
			{
				// 큐에 기록된 오브젝트의 마지막 변경 시간 이후로 변경이 없었다면 Octree에 재삽입
				if (StaticOctree->Insert(Component))
				{
					DynamicPrimitiveMap.Remove(Component);
					++TotalCount;
				}
				// 삽입이 안됐다면 다시 Queue에 들어가기 위해 저장
				else
				{
					NotInsertedQueue.push({Component, *FoundTimestampPtr});
				}
			}
			else
			{
				// 큐에 기록된 오브젝트의 마지막 변경 이후 새로운 변경이 존재했다면 다시 큐에 삽입
				DynamicPrimitiveQueue.push({Component, *FoundTimestampPtr});
			}
		}
	}

	DynamicPrimitiveQueue = NotInsertedQueue;

	if (TotalCount > 0)
	{
		UE_LOG("Level: Octree 즉시 구축 완료 (%u개 컴포넌트)", TotalCount);
	}
}

void ULevel::OnPrimitiveUpdated(UPrimitiveComponent* InComponent)
{
	if (!InComponent)
	{
		return;
	}

	float GameTime = UTimeManager::GetInstance().GetGameTime();
	float* FoundTimePtr = DynamicPrimitiveMap.Find(InComponent);

	if (FoundTimePtr)
	{
		*FoundTimePtr = GameTime;
	}
	else
	{
		DynamicPrimitiveMap.Add(InComponent, GameTime);

		DynamicPrimitiveQueue.push({InComponent, GameTime});
	}
}

void ULevel::OnPrimitiveUnregistered(UPrimitiveComponent* InComponent)
{
	if (!InComponent)
	{
		return;
	}

	DynamicPrimitiveMap.Remove(InComponent);
}

AActor* ULevel::FindTemplateActorByName(const FName& InName) const
{
	for (AActor* TemplateActor : TemplateActors)
	{
		if (TemplateActor && TemplateActor->GetName() == InName)
		{
			return TemplateActor;
		}
	}
	return nullptr;
}

TArray<AActor*> ULevel::FindTemplateActorsByClass(UClass* InClass) const
{
	TArray<AActor*> Result;
	if (!InClass)
	{
		return Result;
	}

	for (AActor* TemplateActor : TemplateActors)
	{
		if (TemplateActor && TemplateActor->GetClass() == InClass)
		{
			Result.Add(TemplateActor);
		}
	}
	return Result;
}

/*-----------------------------------------------------------------------------
	Centralized Overlap Management (Unreal-style)
-----------------------------------------------------------------------------*/

void ULevel::UpdateAllOverlaps()
{
	if (!StaticOctree)
		return;

	// 1. 모든 primitive component 수집
	TArray<UPrimitiveComponent*> AllPrimitives;
	for (AActor* Actor : LevelActors)
	{
		if (!Actor)
			continue;

		TArray<UActorComponent*>& Components = Actor->GetOwnedComponents();
		for (UActorComponent* Comp : Components)
		{
			if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(Comp))
			{
				AllPrimitives.Add(PrimComp);
			}
		}
	}

	// 중복 pair 방지용
	TSet<TPair<UPrimitiveComponent*, UPrimitiveComponent*>, PairHash> ProcessedPairs;

	// 2. 각 component의 Broad Phase + Narrow Phase
	for (UPrimitiveComponent* A : AllPrimitives)
	{
		if (!A)
			continue;

		// Skip if overlap events are disabled (Unreal-style)
		if (!A->GetGenerateOverlapEvents())
			continue;

		// === BROAD PHASE: Octree 쿼리로 후보 수집 ===
		FBounds ABounds = A->CalcBounds();
		FAABB AAABB(ABounds.Min, ABounds.Max);

		TArray<UPrimitiveComponent*> Candidates;
		StaticOctree->QueryAABB(AAABB, Candidates);

		// Dynamic primitives도 추가
		TArray<UPrimitiveComponent*> DynamicPrims = GetDynamicPrimitives();
		Candidates.Append(DynamicPrims);

		for (UPrimitiveComponent* B : Candidates)
		{
			if (!B)
				continue;

			// 자기 자신 스킵
			if (B == A)
				continue;

			// 같은 액터 스킵
			if (B->GetOwner() == A->GetOwner())
				continue;

			// Skip if B also has overlap events disabled
			// Both components must have overlap events enabled (Unreal behavior)
			if (!B->GetGenerateOverlapEvents())
				continue;

			// 중복 pair 방지 (정렬된 pair 생성)
			TPair<UPrimitiveComponent*, UPrimitiveComponent*> Pair =
				(A < B) ? TPair(A, B) : TPair(B, A);

			if (ProcessedPairs.Contains(Pair))
				continue;

			ProcessedPairs.Add(Pair);

			// === NARROW PHASE: 정밀 충돌 테스트 ===
			FBounds BBounds = B->CalcBounds();

			// AABB 레벨 rejection
			if (!ABounds.Overlaps(BBounds))
			{
				// AABB가 겹치지 않음 → overlap이 아님
				bool bWasOverlapping = PreviousOverlapState.FindRef(Pair);
				if (bWasOverlapping)
				{
					// EndOverlap 발생
					A->RemoveOverlapInfo(B);
					B->RemoveOverlapInfo(A);
					A->NotifyComponentEndOverlap(B);
					B->NotifyComponentEndOverlap(A);
				}
				PreviousOverlapState[Pair] = false;
				continue;
			}

			// 정밀 shape 테스트
			const IBoundingVolume* ShapeA = A->GetCollisionShape();
			const IBoundingVolume* ShapeB = B->GetCollisionShape();

			if (!ShapeA || !ShapeB)
			{
				// Shape가 없음 → overlap이 아님
				bool bWasOverlapping = PreviousOverlapState.FindRef(Pair);
				if (bWasOverlapping)
				{
					// EndOverlap 발생
					A->RemoveOverlapInfo(B);
					B->RemoveOverlapInfo(A);
					A->NotifyComponentEndOverlap(B);
					B->NotifyComponentEndOverlap(A);
				}
				PreviousOverlapState[Pair] = false;
				continue;
			}

			bool bIsOverlapping = FCollisionHelper::TestOverlap(ShapeA, ShapeB);

			// 3. 이전 프레임 상태 비교
			bool bWasOverlapping = PreviousOverlapState.FindRef(Pair);

			// === EVENT HANDLING ===
			if (bIsOverlapping && !bWasOverlapping)
			{
				// BeginOverlap - 양방향 상태 업데이트 (using public API)
				A->AddOverlapInfo(B);
				B->AddOverlapInfo(A);

				// 양방향 delegate 브로드캐스트 (using public API)
				FHitResult HitResult;
				HitResult.Actor = B->GetOwner();
				HitResult.Component = B;
				A->NotifyComponentBeginOverlap(B, HitResult);

				HitResult.Actor = A->GetOwner();
				HitResult.Component = A;
				B->NotifyComponentBeginOverlap(A, HitResult);
			}
			else if (!bIsOverlapping && bWasOverlapping)
			{
				// EndOverlap - 양방향 상태 업데이트 (using public API)
				A->RemoveOverlapInfo(B);
				B->RemoveOverlapInfo(A);

				// 양방향 delegate 브로드캐스트 (using public API)
				A->NotifyComponentEndOverlap(B);
				B->NotifyComponentEndOverlap(A);
			}

			// 상태 저장
			PreviousOverlapState[Pair] = bIsOverlapping;
		}
	}

	// 4. EndOverlap 정리: 이전 프레임에 overlap이었지만 현재 프레임에 체크되지 않은 pair
	//    (두 컴포넌트가 멀어져서 Octree 쿼리 결과에 나타나지 않은 경우)
	TArray<TPair<UPrimitiveComponent*, UPrimitiveComponent*>> PairsToRemove;

	for (auto& [Pair, bWasOverlapping] : PreviousOverlapState)
	{
		// 이번 프레임에 체크되지 않았지만 이전에는 overlap이었던 pair
		if (bWasOverlapping && !ProcessedPairs.Contains(Pair))
		{
			UPrimitiveComponent* A = Pair.first;
			UPrimitiveComponent* B = Pair.second;

			// 컴포넌트가 여전히 유효한지 확인
			if (A && B)
			{
				// 이벤트 발생 시에만 로깅
				UE_LOG("[UpdateAllOverlaps] Octree query missed - EndOverlap: [%s]%s <-> [%s]%s",
					A->GetOwner() ? A->GetOwner()->GetName().ToString().c_str() : "None",
					A->GetName().ToString().c_str(),
					B->GetOwner() ? B->GetOwner()->GetName().ToString().c_str() : "None",
					B->GetName().ToString().c_str());

				// EndOverlap - 양방향 상태 업데이트
				A->RemoveOverlapInfo(B);
				B->RemoveOverlapInfo(A);

				// 양방향 delegate 브로드캐스트
				A->NotifyComponentEndOverlap(B);
				B->NotifyComponentEndOverlap(A);
			}

			// 제거할 pair 목록에 추가
			PairsToRemove.Add(Pair);
		}
	}

	// 더 이상 overlap이 아닌 pair 제거
	for (const auto& Pair : PairsToRemove)
	{
		PreviousOverlapState.Remove(Pair);
	}
}

bool ULevel::SweepComponentSingle(
	UPrimitiveComponent* Component,
	const FVector& TargetLocation,
	FHitResult& OutHit)
{
	if (!Component || !StaticOctree)
		return false;

	// 1. Component의 현재 위치 저장
	FVector OriginalLocation = Component->GetWorldLocation();

	// 2. 임시로 목표 위치로 이동 (변환 행렬 계산용)
	FVector LocalOffset = TargetLocation - OriginalLocation;

	// 3. 목표 위치에서의 Bounds 계산
	FBounds ComponentBounds = Component->CalcBounds();
	FBounds TargetBounds(
		ComponentBounds.Min + LocalOffset,
		ComponentBounds.Max + LocalOffset
	);

	// === BROAD PHASE: Octree 쿼리 ===
	FAABB TargetAABB(TargetBounds.Min, TargetBounds.Max);
	TArray<UPrimitiveComponent*> Candidates;
	StaticOctree->QueryAABB(TargetAABB, Candidates);

	// Dynamic primitives도 추가
	TArray<UPrimitiveComponent*> DynamicPrims = GetDynamicPrimitives();
	Candidates.Append(DynamicPrims);

	// 4. 목표 위치에서의 collision shape 준비
	const IBoundingVolume* ComponentShape = Component->GetCollisionShape();
	if (!ComponentShape)
		return false;

	// Temporary transform for target location
	FMatrix TargetTransform = Component->GetWorldTransformMatrix();
	TargetTransform.Data[3][0] = TargetLocation.X;
	TargetTransform.Data[3][1] = TargetLocation.Y;
	TargetTransform.Data[3][2] = TargetLocation.Z;

	// === NARROW PHASE: 첫 번째 충돌만 찾기 ===
	float ClosestDistance = FLT_MAX;
	bool bFoundHit = false;

	for (UPrimitiveComponent* Candidate : Candidates)
	{
		if (!Candidate)
			continue;

		// 자기 자신 스킵
		if (Candidate == Component)
			continue;

		// 같은 액터 스킵
		if (Candidate->GetOwner() == Component->GetOwner())
			continue;

		// AABB rejection
		FBounds CandidateBounds = Candidate->CalcBounds();
		if (!TargetBounds.Overlaps(CandidateBounds))
			continue;

		// 정밀 shape 테스트 (임시 transform 사용)
		const IBoundingVolume* CandidateShape = Candidate->GetCollisionShape();
		if (!CandidateShape)
			continue;

		// Create temporary shape at target location for testing
		IBoundingVolume* TempShape = const_cast<IBoundingVolume*>(ComponentShape);
		TempShape->Update(TargetTransform);

		if (FCollisionHelper::TestOverlap(TempShape, CandidateShape))
		{
			// 충돌 거리 계산 (원래 위치에서 candidate까지)
			FVector CandidateCenter = (CandidateBounds.Min + CandidateBounds.Max) * 0.5f;
			float Distance = (CandidateCenter - OriginalLocation).Length();

			if (Distance < ClosestDistance)
			{
				ClosestDistance = Distance;
				OutHit.Component = Candidate;
				OutHit.Actor = Candidate->GetOwner();
				OutHit.Location = TargetLocation;
				bFoundHit = true;
			}
		}

		// Restore original transform
		TempShape->Update(Component->GetWorldTransformMatrix());
	}

	return bFoundHit;
}

bool ULevel::SweepComponentMulti(
	UPrimitiveComponent* Component,
	const FVector& TargetLocation,
	TArray<UPrimitiveComponent*>& OutOverlappingComponents)
{
	if (!Component || !StaticOctree)
		return false;

	OutOverlappingComponents.Empty();

	// 1. Component의 현재 위치 저장
	FVector OriginalLocation = Component->GetWorldLocation();

	// 2. 임시로 목표 위치로 이동 (변환 행렬 계산용)
	FVector LocalOffset = TargetLocation - OriginalLocation;

	// 3. 목표 위치에서의 Bounds 계산
	FBounds ComponentBounds = Component->CalcBounds();
	FBounds TargetBounds(
		ComponentBounds.Min + LocalOffset,
		ComponentBounds.Max + LocalOffset
	);

	// === BROAD PHASE: Octree 쿼리 ===
	FAABB TargetAABB(TargetBounds.Min, TargetBounds.Max);
	TArray<UPrimitiveComponent*> Candidates;
	StaticOctree->QueryAABB(TargetAABB, Candidates);

	// Dynamic primitives도 추가
	TArray<UPrimitiveComponent*> DynamicPrims = GetDynamicPrimitives();
	Candidates.Append(DynamicPrims);

	// 4. 목표 위치에서의 collision shape 준비
	const IBoundingVolume* ComponentShape = Component->GetCollisionShape();
	if (!ComponentShape)
		return false;

	// Temporary transform for target location
	FMatrix TargetTransform = Component->GetWorldTransformMatrix();
	TargetTransform.Data[3][0] = TargetLocation.X;
	TargetTransform.Data[3][1] = TargetLocation.Y;
	TargetTransform.Data[3][2] = TargetLocation.Z;

	// === NARROW PHASE: 모든 충돌 찾기 ===
	for (UPrimitiveComponent* Candidate : Candidates)
	{
		if (!Candidate)
			continue;

		// 자기 자신 스킵
		if (Candidate == Component)
			continue;

		// 같은 액터 스킵
		if (Candidate->GetOwner() == Component->GetOwner())
			continue;

		// AABB rejection
		FBounds CandidateBounds = Candidate->CalcBounds();
		if (!TargetBounds.Overlaps(CandidateBounds))
			continue;

		// 정밀 shape 테스트 (임시 transform 사용)
		const IBoundingVolume* CandidateShape = Candidate->GetCollisionShape();
		if (!CandidateShape)
			continue;

		// Create temporary shape at target location for testing
		IBoundingVolume* TempShape = const_cast<IBoundingVolume*>(ComponentShape);
		TempShape->Update(TargetTransform);

		if (FCollisionHelper::TestOverlap(TempShape, CandidateShape))
		{
			OutOverlappingComponents.Add(Candidate);
		}

		// Restore original transform
		TempShape->Update(Component->GetWorldTransformMatrix());
	}

	return OutOverlappingComponents.Num() > 0;
}

bool ULevel::SweepActorSingle(
	AActor* Actor,
	const FVector& TargetLocation,
	FHitResult& OutHit,
	ECollisionTag FilterTag)
{
	if (!Actor)
		return false;

	// Actor의 모든 PrimitiveComponent를 가져옴
	TArray<UActorComponent*>& Components = Actor->GetOwnedComponents();

	float ClosestDistance = FLT_MAX;
	bool bFoundHit = false;
	FHitResult ClosestHit;

	// 각 PrimitiveComponent에 대해 sweep 테스트
	for (UActorComponent* Comp : Components)
	{
		UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(Comp);
		if (!PrimComp)
			continue;

		// 이 컴포넌트에 대해 sweep 테스트
		FHitResult TempHit;
		if (SweepComponentSingle(PrimComp, TargetLocation, TempHit))
		{
			// CollisionTag 필터링
			if (FilterTag != ECollisionTag::None && TempHit.Actor)
			{
				if (TempHit.Actor->GetCollisionTag() != FilterTag)
					continue;  // 태그가 일치하지 않으면 스킵
			}

			// 거리 계산 (Actor 중심에서 충돌 지점까지)
			FVector ActorLocation = Actor->GetActorLocation();
			float Distance = (TempHit.Location - ActorLocation).Length();

			if (Distance < ClosestDistance)
			{
				ClosestDistance = Distance;
				ClosestHit = TempHit;
				bFoundHit = true;
			}
		}
	}

	if (bFoundHit)
	{
		OutHit = ClosestHit;
		return true;
	}

	return false;
}

bool ULevel::SweepActorMulti(
	AActor* Actor,
	const FVector& TargetLocation,
	TArray<UPrimitiveComponent*>& OutOverlappingComponents,
	ECollisionTag FilterTag)
{
	if (!Actor)
		return false;

	OutOverlappingComponents.Empty();

	// Actor의 모든 PrimitiveComponent를 가져옴
	TArray<UActorComponent*>& Components = Actor->GetOwnedComponents();

	// 중복 제거용 (여러 컴포넌트가 같은 대상과 충돌할 수 있음)
	TSet<UPrimitiveComponent*> UniqueCollisions;

	// 각 PrimitiveComponent에 대해 sweep 테스트
	for (UActorComponent* Comp : Components)
	{
		UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(Comp);
		if (!PrimComp)
			continue;

		// 이 컴포넌트에 대해 sweep 테스트
		TArray<UPrimitiveComponent*> TempOverlaps;
		if (SweepComponentMulti(PrimComp, TargetLocation, TempOverlaps))
		{
			// 결과를 UniqueCollisions에 추가 (필터링 적용)
			for (UPrimitiveComponent* OverlapComp : TempOverlaps)
			{
				// CollisionTag 필터링
				if (FilterTag != ECollisionTag::None)
				{
					AActor* OverlapActor = OverlapComp->GetOwner();
					if (OverlapActor && OverlapActor->GetCollisionTag() != FilterTag)
						continue;  // 태그가 일치하지 않으면 스킵
				}

				UniqueCollisions.Add(OverlapComp);
			}
		}
	}

	// TSet을 TArray로 변환
	for (UPrimitiveComponent* Comp : UniqueCollisions)
	{
		OutOverlappingComponents.Add(Comp);
	}

	return OutOverlappingComponents.Num() > 0;
}
