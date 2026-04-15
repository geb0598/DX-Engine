[← Week 12][link-week12]

[![thumbnail][img-thumbnail]](https://youtu.be/VPcq6Q8q_eI)

# DX-Engine — Week Final: PhysX Integration

> NVIDIA PhysX 4.1.2를 엔진에 직접 통합

![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus&logoColor=white)
![DirectX 11](https://img.shields.io/badge/DirectX-11-0078D4?logo=microsoft&logoColor=white)
![PhysX](https://img.shields.io/badge/PhysX-4.1.2-76B900?logo=nvidia&logoColor=white)
![Windows](https://img.shields.io/badge/Windows-0078D6?logo=windows&logoColor=white)

---

## Dumb Rider

쇼핑카트를 조종해 짐을 떨어뜨리지 않고 목적지까지 도달하는 게임. PhysX `PxVehicle4W`로 구현한 쇼핑카트는 Ackermann 조향과 서스펜션 물리를 그대로 적용해, 과적 상태에서 코너를 돌면 카트가 실제로 기울고 짐이 흘러내린다.

---

## Features

- **PhysX SDK 전체 스택 통합** — SDK 초기화부터 Vehicle, 충돌 이벤트, 약한 참조까지 엔진 아키텍처에 맞게 수직 래핑
- **비동기 Physics Frame Pipeline** — `simulate()`와 `Actor::Tick()`을 병렬 실행하는 `StartFrame / EndFrame` 분리 구조
- **FBodyInstance / UBodySetup / FKAggregateGeom** — UE4 구조를 차용한 3단계 엔진↔PhysX 브리지
- **PxVehicle4W 차량 시스템** — Ackermann 조향, 서스펜션, 엔진 토크 커브 5단계 초기화 파이프라인

---

## Key Systems

### 1. Physics Frame Pipeline

![pipeline][img-pipeline]

`UWorld::Tick()` 한 프레임의 물리 실행 흐름이다. `StartFrame`에서 `simulate()`를 발사한 뒤 메인 스레드는 블로킹 없이 `Actor::Tick()`을 진행하고, `EndFrame`에서 `fetchResults()`로 동기화한다. PhysX 워커 스레드와 게임 로직이 겹치는 구간이 이 구조의 핵심이다.

| 함수 | 역할 |
|------|------|
| `FlushDeferredAdds` | 큐에 쌓인 PxActor 추가 요청을 씬에 일괄 등록 |
| `FlushCommands` | 게임 로직의 PhysX 상태 변경 요청(SetTransform, AddForce 등)을 일괄 실행 |
| `SyncComponentsToBodies` | 물리 시뮬레이션 결과를 엔진 컴포넌트 트랜스폼에 반영 |
| `DispatchPhysNotifications` | 충돌 결과를 OnHit / OnOverlap 게임 이벤트로 변환해 전달 |
| `FlushDeferredReleases` | 큐에 쌓인 PxActor 제거 요청을 씬에서 일괄 해제 |

<details>
<summary><b>설계 상세 — 클릭해서 펼치기</b></summary>

<br>

**FlushCommands — 락 보유 시간 최소화**

`SetBodyTransform` / `AddForce` 등 게임 로직의 PhysX 변경 요청은 즉시 실행되지 않고 람다로 `CommandQueue`에 쌓인다. `simulate()` 직전 `FlushCommands()`에서 일괄 실행한다. 이때 큐 전체를 `std::move`로 로컬에 옮긴 뒤 뮤텍스를 먼저 해제하고, `SCOPED_SCENE_WRITE_LOCK` 하에 실행한다. 큐 이동 후 락을 잡으므로 뮤텍스 보유 시간이 최소화된다.

```cpp
void FPhysScene::FlushCommands()
{
    TArray<std::function<void()>> LocalCommands;
    {
        std::lock_guard<std::mutex> Lock(CommandMutex);
        LocalCommands = std::move(CommandQueue);  // 뮤텍스 보유 구간은 이동만
    }
    // 뮤텍스 해제 후 씬 락 하에 실행
    SCOPED_SCENE_WRITE_LOCK(PhysXScene);
    for (auto& Cmd : LocalCommands) { Cmd(); }
}
```

각 람다는 `LifeHandle`(`std::weak_ptr<bool>`)을 캡처한다. 실행 시점에 `FBodyInstance`가 이미 소멸했으면 `WeakHandle.expired()`로 감지해 조용히 skip한다.

```cpp
PhysScene->EnqueueCommand([this, NewTransform, WeakHandle]()
{
    if (WeakHandle.expired()) return;  // 이미 소멸한 바디 → skip
    RigidActor->setGlobalPose(U2PTransform(NewTransform));
});
```

**DispatchPhysNotifications — 데드락 방지**

`onContact` 콜백이 `PendingCollisionNotifies`에 충돌 정보를 쌓으면, `DispatchPhysNotifications()`에서 `OnHit` / `OnOverlap` 델리게이트로 변환해 실행한다. `NotifyMutex`를 잡은 채 델리게이트를 실행하면, 콜백 내부에서 새 충돌이 발생해 `AddPendingCollisionNotify()`가 같은 뮤텍스를 재획득하려다 데드락에 빠진다. 큐를 로컬로 이동한 뒤 락을 해제하고 실행한다.

```cpp
void FPhysScene::DispatchPhysNotifications_AssumesLocked()
{
    std::vector<FCollisionNotifyInfo> LocalNotifies;
    {
        std::lock_guard<std::mutex> Lock(NotifyMutex);
        LocalNotifies = std::move(PendingCollisionNotifies);  // 큐 비우고 락 해제
    }
    for (auto& Notify : LocalNotifies)
    {
        // 뮤텍스 미보유 상태에서 델리게이트 실행
        // → 콜백 내 AddPendingCollisionNotify() 호출해도 데드락 없음
        Notify.Actor0->OnHit.Broadcast(...);
    }
}
```

**SyncComponentsToBodies — SkipPhysicsUpdate로 역방향 루프 차단**

물리 결과를 컴포넌트에 반영할 때 플래그 없이 `SetWorldTransform`을 호출하면 `OnUpdateTransform → SetBodyTransform → EnqueueCommand(setGlobalPose)`로 이어져 물리가 준 값을 PhysX에 다시 써넣는 루프가 매 프레임 반복된다. `SkipPhysicsUpdate`를 전달하면 `OnUpdateTransform` 내부 분기에서 `SetBodyTransform` 호출을 건너뛴다.

```cpp
// UPrimitiveComponent::OnUpdateTransform()
if (BodyInstance.IsValidBodyInstance() &&
    !((int32)UpdateTransformFlags & (int32)EUpdateTransformFlags::SkipPhysicsUpdate))
{
    BodyInstance.SetBodyTransform(...);  // SkipPhysicsUpdate이면 진입 안 함
}

// FPhysScene::SyncComponentsToBodies()
OwnerComp->SetWorldTransform(NewTransform,
    EUpdateTransformFlags::SkipPhysicsUpdate,  // 역방향 루프 차단
    ETeleportType::None);
```

**FlushDeferredAdds / FlushDeferredReleases — 생명주기 대칭 구조**

PhysX는 `simulate()` 중 `addActor()` / `release()` 호출을 금지한다. 추가·제거 요청은 각각 별도 큐에 쌓아두고, Add는 `StartFrame`(simulate 직전), Release는 `EndFrame` 마지막(Sync·Dispatch 완료 후)에 처리한다. Release를 마지막에 두는 이유는 `SyncComponentsToBodies`와 `DispatchPhysNotifications`가 해당 액터의 `userData`를 참조하기 때문이다.

같은 프레임 내 생성·소멸 쌍이 발생하면 `DeferReleaseActor()`가 `DeferredAddQueue`에서 해당 액터를 제거하고 반환해, 씬 진입 자체를 막는다.

```cpp
void FPhysScene::DeferReleaseActor(PxActor* InActor)
{
    if (bPhysXSceneExecuting)  // 시뮬레이션 중
    {
        if (DeferredAddQueue.Contains(InActor))
        {
            DeferredAddQueue.Remove(InActor);
            return;  // 씬에 추가도 release도 없이 소거
        }
    }
    DeferredReleaseQueue.Add(InActor);
}
```

</details>

---

### 2. FBodyInstance / UBodySetup / FKAggregateGeom — 엔진↔PhysX 브리지

UE4 구조를 차용해 엔진 오브젝트와 PhysX 액터 사이의 연결 계층을 3단계로 분리했다.

```
[에디터 / 게임 로직]
UBoxColliderComponent
  └─ UBodySetup                  ← 충돌 형상 정의 (에디터 데이터)
       └─ FKAggregateGeom         ← Box / Sphere / Capsule / Convex 형상 배열
            └─ FKBoxElem 등       ← 개별 형상 (크기, 위치, 회전, 충돌 채널)

[런타임 브리지]
FBodyInstance                    ← PxRigidActor 1:1 래퍼
  ├─ RigidActor->userData = this ← PhysX 콜백 → 엔진 역참조
  └─ LifeHandle                  ← CommandQueue 람다의 수명 추적

[PhysX]
PxRigidActor
  └─ PxShape × N                 ← FKAggregateGeom → AddShapesToRigidActor() 변환 결과
```

`FBodyInstance`는 게임 로직과 PhysX 사이의 단일 진입점이다. `SetBodyTransform` / `AddForce` 등 모든 물리 변경 요청은 PhysX API를 직접 호출하지 않고 `CommandQueue`로 예약된다. 이는 `Actor::Tick()` 중 `simulate()`가 이미 돌고 있을 수 있기 때문이다 — PhysX 워커 스레드가 같은 메모리를 읽는 중에 직접 쓰면 데이터 레이스가 발생한다.

<details>
<summary><b>AddShapesToRigidActor — 형상 변환 상세</b></summary>

<br>

`UBodySetup::AddShapesToRigidActor_AssumesLocked()`가 `FKAggregateGeom`의 각 형상을 PhysX `PxShape`으로 변환해 액터에 붙인다.

```cpp
// FKBoxElem → PxBoxGeometry
PxBoxGeometry BoxGeom(HalfX * Scale.X, HalfY * Scale.Y, HalfZ * Scale.Z);
PxShape* Shape = Physics->createShape(BoxGeom, *Material);
RigidActor->attachShape(*Shape);

// FKSphylElem (Capsule) → 축 보정 필요
// PhysX 캡슐은 X축 방향, 엔진은 Z축 방향
PxQuat FixRot(PxHalfPi, PxVec3(0, 1, 0));  // Z → X 변환
PxCapsuleGeometry CapsuleGeom(Radius * MaxScale, HalfLength * ScaleX);
```

`FBodyShapeCalculator`는 스켈레탈 메시의 버텍스 분포를 분석해 각 본에 맞는 충돌 형상을 자동 생성한다. 버텍스를 본 로컬 좌표로 변환한 뒤 3축 투영으로 가장 긴 축을 길이 방향으로, 66번째 백분위수를 반지름으로 선택한다.

</details>

---

### 3. PxVehicle4W 차량 시스템

`PxVehicle4W`는 단순히 `create()`를 호출하는 것이 아니라 5단계 데이터 구조를 순서대로 구성해야 한다.

```
1. FVehicleEngineData    → PxVehicleEngineData    (토크 커브, RPM 범위)
2. FWheelSetup × 4      → PxVehicleWheelsSimData  (반지름, 질량, 서스펜션)
3. FVehicleDriveData    → PxVehicleDriveSimData4W (차동장치, 클러치)
4. Suspension           → per-wheel SuspensionData (스프링, 댐퍼)
5. Ackermann Steering   → PxVehicleDifferential4WData
                                    ↓
                           PxVehicle4W::create()
```

**SDK 제약 우회 — 바퀴 충돌 이벤트**

PxVehicle의 바퀴 레이캐스트는 `PxSimulationEventCallback::onContact()`를 발생시키지 않는다. PhysX 내부적으로 Vehicle 레이캐스트가 일반 씬 쿼리와 다른 경로로 처리되기 때문이다. 매 프레임 `CheckWheelInteractions()`에서 `SweepSingleSphere()`로 바퀴 접촉을 직접 감지하고, `DispatchBlockingHit()`으로 게임 이벤트를 수동 주입해 해결했다.

```cpp
void AVehiclePawn::CheckWheelInteractions()
{
    for (int32 i = 0; i < 4; i++)
    {
        FHitResult HitResult;
        if (PhysScene->SweepSingleSphere(WheelPos, NextPos, WheelRadius, HitResult))
        {
            // onContact가 오지 않으므로 수동으로 이벤트 발생
            PhysScene->DispatchBlockingHit(*this, HitResult);
        }
    }
}
```

---

## References

- NVIDIA, **PhysX 4.1.2 SDK Documentation**
- NVIDIA, **PhysX Vehicle SDK Guide**
- Epic Games, **Unreal Engine 4 Source — FPhysScene / FBodyInstance / PxVehicle4W integration**

---

[← Week 12][link-week12]

<!-- 이미지 레퍼런스 -->
[img-thumbnail]: Docs/Images/thumbnail-youtube.png
[img-pipeline]:  Docs/Images/pipeline.png

<!-- 링크 레퍼런스 -->
[link-week12]: https://github.com/geb0598/DX-Engine/tree/week-12
