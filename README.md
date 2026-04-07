[← Week 08][link-week08] | [Week 10 →][link-week10]

[![preview][img-preview]][link-youtube]

# DX-Engine — Week 09: Last Roll

> 뱀파이어 서바이버류 게임 제작과 이를 위한 엔진 시스템 구현 — Delegate, FWeakObjectPtr, Lua 스크립팅, Post-Processing

![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus&logoColor=white)
![DirectX 11](https://img.shields.io/badge/DirectX-11-0078D4?logo=microsoft&logoColor=white)
![HLSL](https://img.shields.io/badge/HLSL-SM_5.0-5C2D91)
![Lua](https://img.shields.io/badge/Lua-5.4-2C2D72?logo=lua&logoColor=white)
![Windows](https://img.shields.io/badge/Windows-0078D6?logo=windows&logoColor=white)

---

## Game — Last Roll

자체 제작 DX11 엔진 위에서 동작하는 첫 번째 완성 게임. 엔진 코어(렌더링·물리·컴포넌트 시스템)는 C++로, 게임 로직·AI·연출은 Lua(sol2) 스크립트로 작성해 Unity의 C++/C# 분리 구조를 재현했다. 3인 팀이 4일동안 완성한 게임잼 프로젝트다.

주사위를 조작해 끝없이 몰려오는 적을 상대로 가능한 한 오래 버티는 **뱀파이어 서바이버류 탑다운 슈터**.

### Overview

| | |
|---|---|
| **장르** | Vampire Survivors-like, Top-Down Shooter |
| **개발 기간** | 4일 |
| **개발 인원** | 3인 |
| **플랫폼** | Windows (DirectX 11) |

### 게임플레이

- **조작**: WASD 이동, 자동 공격
- **맵**: 무한 스크롤 타일 맵
- **적**: 체스 나이트 홉 이동형(EnemyA), 직선 돌진형(EnemyB) 두 종류를 Lua ActorPool로 관리
- **연출**: 피격 시 카메라 흔들림, 사망 시 슬로우모션 → 빨간 비녜트 → 페이드 아웃

---

## Features

- **UE 스타일 델리게이트** — 5계층 타입 소거 구조로 Static / Lambda / UObject / WeakLambda 등 6종 바인딩과 멀티캐스트 Broadcast 지원
- **FWeakObjectPtr** — GUObjectArray 인덱스 기반 4바이트 약한 참조. UObject 소멸 시 델리게이트 구독 자동 해제
- **C++ / Lua 이중 구조** — 엔진 코어는 C++, 게임 로직·AI·연출은 Lua로 분리. Unity의 C# 스크립팅과 유사한 구조
- **Post-Processing 파이프라인** — Vignette · Fade · Letterbox · Gamma 보정을 템플릿 메서드 패턴으로 체인 구성

---

## Key Systems

### 1. FWeakObjectPtr — Handle-based Weak Reference

`UObject*`를 직접 저장하는 대신 `GUObjectArray`의 인덱스를 저장하고, `Get()` 호출 시 전역 레지스트리에서 유효성을 확인한다. `std::weak_ptr`(16바이트) 대비 4바이트로 동일한 목적을 달성한다.

```cpp
class FWeakObjectPtr {
    uint32 InternalIndex = INDEX_NONE;  // 포인터 대신 인덱스 (4바이트)
public:
    UObject* Get() const {
        if (InternalIndex == INDEX_NONE) return nullptr;
        const auto& Array = GetUObjectArray();
        return (InternalIndex < Array.size()) ? Array[InternalIndex] : nullptr;
    }
    explicit operator bool() const { return IsValid(); }
};
```

> **설계 한계**: SerialNumber 없이 슬롯 재사용 문제에 취약하다. UE는 이를 `FObjectHandlePrivate::SerialNumber`로 해결한다. 헤더에 해당 한계를 주석으로 명시했다.

---

### 2. UE 스타일 타입 소거 델리게이트

게임 이벤트 시스템의 근간. 5계층 인터페이스-구현 구조로 6종의 callable을 균일하게 처리한다.

```
Layer 1  IDelegateInstance          — GetUObject(), IsSafeToExecute()
Layer 2  IBaseDelegateInstance<Sig> — Execute(), ExecuteIfSafe()
Layer 3  구체 인스턴스 6종          — Static / Raw / SharedPtr / Lambda / UObject / WeakLambda
Layer 4  TDelegateBase              — shared_ptr<IDelegateInstance> 소유
Layer 5  TDelegate / TMulticastDelegate — 사용자 API
```

<details>
<summary><b>구현 상세 — 클릭해서 펼치기</b></summary>

<br>

**UObject 안전 실행**

```cpp
bool ExecuteIfSafe(ParamTypes... Params) const final {
    if (UserClass* Obj = static_cast<UserClass*>(UserObject.Get())) {
        (void)(Obj->*MethodPtr)(std::forward<ParamTypes>(Params)...);
        return true;
    }
    return false;  // FWeakObjectPtr 유효성 실패 → 실행 안 함
}
```

**Broadcast — 역순 순회 + Lazy Compaction**

```cpp
// 구독자가 자신을 제거해도 인덱스 무효화 없이 동작
for (int32 i = InvocationList.size() - 1; i >= 0; --i) {
    if (!Instance->ExecuteIfSafe(Params...)) NeedsCompaction = true;
}
if (NeedsCompaction) CompactInvocationList();  // 순회 후 일괄 제거
```

`NeedsCompaction` 더티 플래그로 즉시 삭제 대신 순회 완료 후 일괄 compact해 이터레이터 무효화를 방지한다.

**Lua-C++ 이벤트 브릿지**

```cpp
// ScriptComponent 수명에 Lua 구독을 연동
auto Wrapper = [LuaFunc = std::move(LuaFunc)](const FOverlapInfo& Info) {
    sol::protected_function_result Result = LuaFunc(Info.OverlappingComponent->GetOwner());
    if (!Result.valid())
        UE_LOG_ERROR("Lua Error: %s", sol::error(Result).what());
};
Prim->OnComponentBeginOverlap.AddWeakLambda(ScriptComp, std::move(Wrapper));
```

ScriptComponent가 소멸하면 `FWeakObjectPtr` 유효성 검사에서 걸려 Lua 구독이 자동 해제된다.

</details>

---

### 3. C++ / Lua 이중 구조

엔진 코어(컴포넌트 시스템, 렌더링, 물리)는 C++로, 게임 로직·AI·연출은 Lua(sol2)로 작성하도록 역할을 분리했다. Unity에서 엔진은 C++로 작성하고 게임 로직은 C#으로 작성하는 구조와 같은 맥락이다.

```
C++ (Engine)                    Lua (Game Logic)
────────────────────            ──────────────────────────
AActor / UObject                Player.lua
UScriptComponent      ←bind→   Enemy.lua / EnemyB.lua
USphereComponent                ActorPool.lua  (오브젝트 풀)
SpawnActorByName()   ←call──   GameManager.lua
BeginOverlap Event   ──fire→   BindBeginOverlap() 콜백
```

액터 풀도 Lua 싱글톤(`ActorPool.lua`)으로 구현했다. 풀 미스 시 C++ `SpawnActorByName` API를 호출해 액터를 동적 생성하고, 반납 시 `StopAllCoroutine()`으로 Lua 코루틴 상태를 정리한다.

```lua
function ActorPool:Return(Actor)
    Actor:SetCanTick(false)
    Actor:SetActorHiddenInGame(true)
    Actor:StopAllCoroutine()         -- 반납 시 코루틴 상태 정리
    table.insert(self.InactiveActors[ActorName], Actor)
end
```

---

### 4. Post-Processing 파이프라인

`FPostProcessPass` 기반 클래스가 템플릿 메서드 패턴으로 실행 알고리즘을 고정하고, 파생 클래스는 `UpdateConstants()`와 `IsEnabled()`만 오버라이드한다.

```
씬 렌더 결과
  → Vignette  — HP 80% 미만 시 빨간 비녜트 자동 강화
  → Fade      — Lua StartCameraFade()로 씬 전환·사망 연출 제어
  → Letterbox — 목표 종횡비에 맞춰 상하 마스킹
  → Gamma     — pow(rgb, 1/2.2) sRGB 인코딩
  → Back Buffer
```

<details>
<summary><b>구현 상세 — 클릭해서 펼치기</b></summary>

<br>

**버텍스 버퍼 없는 풀스크린 삼각형**

```hlsl
// SV_VertexID 만으로 풀스크린 삼각형 생성 — 버텍스 버퍼 불필요
PS_INPUT mainVS(uint VertexID : SV_VertexID) {
    Output.TexCoord = float2((VertexID << 1) & 2, VertexID & 2);
    Output.Position = float4(Output.TexCoord * float2(2.0f, -2.0f)
                             + float2(-1.0f, 1.0f), 0.0f, 1.0f);
    return Output;
}
```

모든 PP 셰이더가 이 공통 VS를 재사용해 `Draw(3, 0)` 한 번으로 풀스크린을 처리한다.

**HP 연동 비녜트**

```hlsl
float2 uv          = Input.TexCoord - 0.5f;
float  VignetteMask = dot(uv, uv) * 2.0f;
float3 FinalColor   = lerp(SceneColor.rgb, VignetteColor.rgb,
                           saturate(VignetteMask * VignetteIntensity));
```

`VignetteIntensity`는 HP 80% 미만일 때 `(1 - HPRatio)`에 비례해 C++ 측에서 매 프레임 갱신한다.

**ShowFlags 연동**

```cpp
bool FGammaPass::IsEnabled(FRenderingContext& Context) const {
    return (Context.ShowFlags & EEngineShowFlags::SF_Gamma);
}
```

각 패스를 에디터 ShowFlags로 런타임에 개별 토글할 수 있다.

</details>

---

## References

- Epic Games, **Unreal Engine Source — TDelegate / FWeakObjectPtr**
- Ierusalimschy et al., **Programming in Lua**, sol2 documentation

---

[← Week 08][link-week08] | [Week 10 →][link-week10]

<!-- 이미지 레퍼런스 -->
[img-preview]: Docs/Images/thumbnail-youtube.png

<!-- 유튜브 링크 -->
[link-youtube]: https://youtu.be/aQaqnOtGVc8

<!-- 링크 레퍼런스 -->
[link-week08]: https://github.com/geb0598/DX-Engine/tree/week-08
[link-week10]: https://github.com/geb0598/DX-Engine/tree/week-10
