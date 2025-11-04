#include "pch.h"
#include "Manager/Lua/Public/LuaBinder.h"
#include "Actor/Public/Actor.h"
#include "Actor/Public/GameMode.h"
#include "Actor/Public/EnemySpawnerActor.h"
#include "Demo/Public/Player.h"
#include "Component/Public/ScriptComponent.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Manager/Input/Public/InputManager.h"
#include "Level/Public/Level.h"
#include "Physics/Public/HitResult.h"
#include "Global/Enum.h"
#include "Render/UI/Overlay/Public/D2DOverlayManager.h"

void FLuaBinder::BindCoreTypes(sol::state& LuaState)
{
	// --- UWorld ---
    LuaState.new_usertype<UWorld>("UWorld",
        "SpawnActor", sol::overload(
            sol::resolve<AActor*(const std::string&)>(&UWorld::SpawnActor),
            sol::resolve<AActor*(UClass*, JSON*)>(&UWorld::SpawnActor)
        ),
        "GetTimeSeconds", &UWorld::GetTimeSeconds,
        "DestroyActor", &UWorld::DestroyActor,
        "GetGameMode", &UWorld::GetGameMode,
        "GetLevel", &UWorld::GetLevel,
        "FindTemplateActorOfName", &UWorld::FindTemplateActorOfName,
        "GetSourceEditorWorld", &UWorld::GetSourceEditorWorld
    );

    // --- GWorld 인스턴스 접근자 (전역 함수) ---
    LuaState.set_function("GetWorld", []() -> UWorld* {
    	return GWorld;
    });

	// --- ULevel ---
	LuaState.new_usertype<ULevel>("ULevel",
		// Get all actors in the level (returns Lua table)
		"GetLevelActors", [](ULevel* Level, sol::this_state ts) -> sol::table {
			sol::state_view lua(ts);
			sol::table result = lua.create_table();

			if (!Level)
				return result;

			const TArray<AActor*>& Actors = Level->GetLevelActors();
			for (int i = 0; i < Actors.Num(); ++i)
			{
				result[i + 1] = Actors[i];  // Lua는 1부터 시작
			}
			return result;
		},

		// Get all template actors in the level (returns Lua table)
		"GetTemplateActors", [](ULevel* Level, sol::this_state ts) -> sol::table {
			sol::state_view lua(ts);
			sol::table result = lua.create_table();

			if (!Level)
				return result;

			const TArray<AActor*>& Actors = Level->GetTemplateActors();
			for (int i = 0; i < Actors.Num(); ++i)
			{
				result[i + 1] = Actors[i];
			}
			return result;
		},

		// Find template actor by name
		// Returns AActor* (can be nullptr) - supports polymorphism via sol2 RTTI
		"FindTemplateActorByName", [](ULevel* Level, const std::string& InName) -> AActor* {
			if (!Level)
				return nullptr;
			return Level->FindTemplateActorByName(FName(InName));
		},

		// Find all template actors by class (returns Lua table)
		"FindTemplateActorsOfClass", [](ULevel* Level, UClass* InClass, sol::this_state ts) -> sol::table {
			sol::state_view lua(ts);
			sol::table result = lua.create_table();

			if (!Level || !InClass)
				return result;

			TArray<AActor*> Actors = Level->FindTemplateActorsOfClass(InClass);
			for (int i = 0; i < Actors.Num(); ++i)
			{
				result[i + 1] = Actors[i];
			}
			return result;
		},

		// Find regular actor by name (template actors excluded)
		// Returns AActor* (can be nullptr) - supports polymorphism via sol2 RTTI
		"FindActorByName", [](ULevel* Level, const std::string& InName) -> AActor* {
			if (!Level)
				return nullptr;
			return Level->FindActorByName(FName(InName));
		},

		// Find all regular actors by class (returns Lua table, template actors excluded)
		// 클래스명 문자열로 찾기
		"FindActorsOfClass", [](ULevel* Level, const std::string& ClassName, sol::this_state ts) -> sol::table {
			sol::state_view lua(ts);
			sol::table result = lua.create_table();

			if (!Level)
				return result;

			TArray<AActor*> Actors = Level->FindActorsOfClassByName(FString(ClassName));
			for (int i = 0; i < Actors.Num(); ++i)
			{
				result[i + 1] = Actors[i];
			}
			return result;
		},

		// Sweep single collision test for Actor (returns first hit)
		// Tests all PrimitiveComponents of the Actor
		// Optional FilterTag parameter: only returns hits with matching CollisionTag
		"SweepActorSingle", sol::overload(
			// Without filter
			[](ULevel* Level, AActor* Actor, const FVector& TargetLocation) -> sol::optional<FHitResult> {
				if (!Level || !Actor)
					return sol::nullopt;

				FHitResult HitResult;
				bool bHit = Level->SweepActorSingle(Actor, TargetLocation, HitResult);
				if (bHit)
					return HitResult;
				return sol::nullopt;
			},
			// With filter
			[](ULevel* Level, AActor* Actor, const FVector& TargetLocation, ECollisionTag FilterTag) -> sol::optional<FHitResult> {
				if (!Level || !Actor)
					return sol::nullopt;

				FHitResult HitResult;
				bool bHit = Level->SweepActorSingle(Actor, TargetLocation, HitResult, FilterTag);
				if (bHit)
					return HitResult;
				return sol::nullopt;
			}
		),

		// Sweep multi collision test for Actor (returns all overlapping components)
		// Tests all PrimitiveComponents of the Actor
		// Optional FilterTag parameter: only returns hits with matching CollisionTag
		"SweepActorMulti", sol::overload(
			// Without filter
			[](ULevel* Level, AActor* Actor, const FVector& TargetLocation) -> sol::optional<TArray<UPrimitiveComponent*>> {
				if (!Level || !Actor)
					return sol::nullopt;

				TArray<UPrimitiveComponent*> OverlappingComponents;
				bool bHit = Level->SweepActorMulti(Actor, TargetLocation, OverlappingComponents);
				if (bHit && OverlappingComponents.Num() > 0)
					return OverlappingComponents;
				return sol::nullopt;
			},
			// With filter
			[](ULevel* Level, AActor* Actor, const FVector& TargetLocation, ECollisionTag FilterTag) -> sol::optional<TArray<UPrimitiveComponent*>> {
				if (!Level || !Actor)
					return sol::nullopt;

				TArray<UPrimitiveComponent*> OverlappingComponents;
				bool bHit = Level->SweepActorMulti(Actor, TargetLocation, OverlappingComponents, FilterTag);
				if (bHit && OverlappingComponents.Num() > 0)
					return OverlappingComponents;
				return sol::nullopt;
			}
		)
	);
}

void FLuaBinder::BindMathTypes(sol::state& LuaState)
{
	// -- Vector -- //
	LuaState.new_usertype<FVector>("FVector",
		sol::call_constructor,
		sol::factories(
			[]() { return FVector(); },
			[](float X, float Y, float Z) { return FVector(X, Y, Z); }
		),
		"X", &FVector::X,
		"Y", &FVector::Y,
		"Z", &FVector::Z,

		sol::meta_function::addition, sol::overload(
			[](const FVector& a, const FVector& b) { return a + b; }
		),

		sol::meta_function::subtraction, sol::overload(
			[](const FVector& a, const FVector& b) { return a - b; }
		),

		sol::meta_function::multiplication, sol::overload(
			[](const FVector& a, const FVector& b) { return a * b; },
			[](const FVector& v, float s) { return v * s; },
			[](float s, const FVector& v) { return s * v; }
		),

		sol::meta_function::division, sol::overload(
			[](const FVector& a, const FVector& b) { return a / b; },
			[](const FVector& v, float s) { return v / s; }
		),

		"Normalize", &FVector::Normalize,
		"Length", &FVector::Length
	);

	// -- Quaternion -- //
	LuaState.new_usertype<FQuaternion>("FQuaternion",
		sol::call_constructor,
	    sol::factories(
	       []() { return FQuaternion(); },
	       [](float X, float Y, float Z, float W) { return FQuaternion(X, Y, Z, W); }
	    ),
	    "X", &FQuaternion::X,
	    "Y", &FQuaternion::Y,
	    "Z", &FQuaternion::Z,
	    "W", &FQuaternion::W,

	    sol::meta_function::multiplication, &FQuaternion::operator*,

	    // --- Static Func ---
	    // Lua: local q = FQuaternion.Identity()
	    "Identity", &FQuaternion::Identity,
	    // Lua: local q = FQuaternion.FromAxisAngle(FVector(0,0,1), math.rad(90))
	    "FromAxisAngle", &FQuaternion::FromAxisAngle,
	    // Lua: local q = FQuaternion.FromEuler(FVector(0, 0, 90))
	    "FromEuler", &FQuaternion::FromEuler,
	    // Lua: local q = FQuaternion.FromRotationMatrix(someMatrix)
	    "FromRotationMatrix", &FQuaternion::FromRotationMatrix,
	    // Lua: local q = FQuaternion.MakeFromDirection(FVector(1,0,0))
	    "MakeFromDirection", &FQuaternion::MakeFromDirection,

	    // --- Member Func ---
	    // Lua: local eulerVec = myQuat:ToEuler()
	    "ToEuler", &FQuaternion::ToEuler,
	    // Lua: local matrix = myQuat:ToRotationMatrix()
	    "ToRotationMatrix", &FQuaternion::ToRotationMatrix,
	    // Lua: myQuat:Normalize()
	    "Normalize", &FQuaternion::Normalize,
	    // Lua: local conj = myQuat:Conjugate()
	    "Conjugate", &FQuaternion::Conjugate,
	    // Lua: local inv = myQuat:Inverse()
	    "Inverse", &FQuaternion::Inverse,

	    // --- 6. 오버로드된 함수 (정적 & 멤버) ---
	    "RotateVector", sol::overload(
	        // C++: static FVector RotateVector(const FQuaternion& q, const FVector& v)
	        // Lua: local rotatedVec = FQuaternion.RotateVector(myQuat, myVec)
	        sol::resolve<FVector(const FQuaternion&, const FVector&)>(&FQuaternion::RotateVector),

	        // C++: FVector FQuaternion::RotateVector(const FVector& V) const
	        // Lua: local rotatedVec = myQuat:RotateVector(myVec)
	        sol::resolve<FVector(const FVector&) const>(&FQuaternion::RotateVector)
	    )
	);

	// -- HitResult -- //
	LuaState.new_usertype<FHitResult>("FHitResult",
		sol::call_constructor,
		sol::factories(
			[]() { return FHitResult(); },
			[](const FVector& InLocation, const FVector& InNormal) { return FHitResult(InLocation, InNormal); }
		),
		// Members
		"Location", &FHitResult::Location,
		"Normal", &FHitResult::Normal,
		"PenetrationDepth", &FHitResult::PenetrationDepth,
		"Actor", &FHitResult::Actor,
		"Component", &FHitResult::Component,
		"bBlockingHit", &FHitResult::bBlockingHit
	);
}

void FLuaBinder::BindActorTypes(sol::state& LuaState)
{
	// -- Actor -- //
	LuaState.new_usertype<AActor>("AActor",
		"Name", sol::property(
			[](AActor* Actor) -> std::string { return static_cast<string>(Actor->GetName().ToString()); }
		),
		"Location", sol::property(
			&AActor::GetActorLocation,
			&AActor::SetActorLocation
		),
		"Rotation", sol::property(
			&AActor::GetActorRotation,
			&AActor::SetActorRotation
		),
		"Scale", sol::property(
			&AActor::GetActorScale3D,
			&AActor::SetActorScale3D
		),
		"UUID", sol::property(
			&AActor::GetUUID
		),
		"Tag", sol::property(
			[](AActor* Actor) -> int32 { return static_cast<int32>(Actor->GetCollisionTag()); }
		),
		"IsTemplate", sol::property(
			&AActor::IsTemplate,
			&AActor::SetIsTemplate
		),
		"Duplicate", &AActor::Duplicate,
		"DuplicateFromTemplate", sol::overload(
			// 파라미터 없음 - 템플릿의 Outer Level에 추가, 기본 위치/회전
			[](AActor* Self) {
				return Self->DuplicateFromTemplate();
			},
			// Level 지정 - 지정된 Level에 추가, 기본 위치/회전
			[](AActor* Self, ULevel* TargetLevel) {
				return Self->DuplicateFromTemplate(TargetLevel);
			},
			// Level + Location 지정
			[](AActor* Self, ULevel* TargetLevel, const FVector& InLocation) {
				return Self->DuplicateFromTemplate(TargetLevel, InLocation);
			},
			// Level + Location + Rotation 지정 (완전한 제어)
			[](AActor* Self, ULevel* TargetLevel, const FVector& InLocation, const FQuaternion& InRotation) {
				return Self->DuplicateFromTemplate(TargetLevel, InLocation, InRotation);
			}
		),
		"GetActorForwardVector", sol::resolve<FVector() const>(&AActor::GetActorForwardVector),
		"GetActorUpVector", sol::resolve<FVector() const>(&AActor::GetActorUpVector),
		"GetActorRightVector", sol::resolve<FVector() const>(&AActor::GetActorRightVector),

		// Type-safe casting functions - add new actor types here using the macro
		BIND_ACTOR_CAST(AEnemySpawnerActor),
		BIND_ACTOR_CAST(APlayer)
	);

	// --- AGameMode ---
    LuaState.new_usertype<AGameMode>("AGameMode",
        sol::base_classes, sol::bases<AActor>(),
        "InitGame", &AGameMode::InitGame,
        "StartGame", &AGameMode::StartGame,
        "EndGame", &AGameMode::EndGame,

        "IsGameRunning", sol::property(&AGameMode::IsGameRunning),
        "IsGameEnded", sol::property(&AGameMode::IsGameEnded),

        "OnGameInited", sol::writeonly_property(
            [](AGameMode* Self, const sol::function& LuaFunc) {
                if (!Self || !LuaFunc.valid()) return;
                TWeakObjectPtr<AGameMode> WeakGameMode(Self);
                Self->OnGameInited.Add([WeakGameMode, LuaFunc]()
                {
                    if (WeakGameMode.IsValid())
                    {
                        auto Result = LuaFunc();
                        if (!Result.valid())
                        {
                            sol::error Err = Result;
                            UE_LOG_ERROR("[Lua Error] %s", Err.what());
                        }
                    }
                });
            }
        ),

        "OnGameStarted", sol::writeonly_property(
            [](AGameMode* Self, const sol::function& LuaFunc) {
                if (!Self || !LuaFunc.valid()) return;
                TWeakObjectPtr<AGameMode> WeakGameMode(Self);
                Self->OnGameStarted.Add([WeakGameMode, LuaFunc]()
                {
                    if (WeakGameMode.IsValid())
                    {
                        auto Result = LuaFunc();
                        if (!Result.valid())
                        {
                            sol::error Err = Result;
                            UE_LOG_ERROR("[Lua Error] %s", Err.what());
                        }
                    }
                });
            }
        ),

        "OnGameEnded", sol::writeonly_property(
            [](AGameMode* Self, sol::function LuaFunc) {
                if (!Self || !LuaFunc.valid()) return;
                TWeakObjectPtr<AGameMode> WeakGameMode(Self);
                Self->OnGameEnded.Add([WeakGameMode, LuaFunc]()
                {
                    if (WeakGameMode.IsValid())
                    {
                        auto Result = LuaFunc();
                        if (!Result.valid()) {
                            sol::error Err = Result;
                            UE_LOG_ERROR("[Lua Error] %s", Err.what());
                        }
                    }
                });
            }
        ),
		"GetPlayer", &AGameMode::GetPlayer
    );

	// --- AEnemySpawnerActor ---
	LuaState.new_usertype<AEnemySpawnerActor>("AEnemySpawnerActor",
		sol::base_classes, sol::bases<AActor>(),
		"RequestSpawn", &AEnemySpawnerActor::RequestSpawn
	);

	// --- APlayer ---
	LuaState.new_usertype<APlayer>("APlayer",
		sol::base_classes, sol::bases<AActor>(),

		"OnPlayerTracking", sol::writeonly_property(
			[](APlayer* Self, const sol::function& LuaFunc) {
				if (!Self || !LuaFunc.valid()) return;
				TWeakObjectPtr<APlayer> WeakPlayer(Self);
				Self->OnPlayerTracking.Add([WeakPlayer, LuaFunc](float LightLevel, FVector PlayerLocation)
				{
					if (WeakPlayer.IsValid())
					{
						auto Result = LuaFunc(LightLevel, PlayerLocation);
						if (!Result.valid())
						{
							sol::error Err = Result;
							UE_LOG_ERROR("[Lua Error] OnPlayerTracking: %s", Err.what());
						}
					}
				});
			}
		),

		"BroadcastTracking", &APlayer::BroadcastTracking
	);

	// --- WeakObjectPtr Bindings ---
	// Add TWeakObjectPtr support for safe object tracking from Lua
	BIND_WEAK_PTR(AActor);
	// BIND_WEAK_PTR(AEnemySpawnerActor);  // Example: add more types as needed
}

void FLuaBinder::BindComponentTypes(sol::state& LuaState)
{
	LuaState.new_usertype<UScriptComponent>("ScriptComponent",
		"StartCoroutine", &UScriptComponent::StartCoroutine,
		"StopCoroutine", &UScriptComponent::StopCoroutine,
		"StopAllCoroutines", &UScriptComponent::StopAllCoroutines
	);
}

void FLuaBinder::BindCoreFunctions(sol::state& LuaState)
{
	// -- Log -- //
	LuaState.set_function("Log", [](sol::variadic_args Vars) {
			std::stringstream ss;
			sol::state_view Lua(Vars.lua_state());

			for (auto v : Vars)
			{
				sol::protected_function ToString = Lua["tostring"];
				sol::protected_function_result Result = ToString(v);

				FString Str;
				if (Result.valid())
				{
					Str = Result.get<std::string>();
				}
				else
				{
					Str = "[nil or error]";
				}
				ss << Str << "\t";
			}

			std::string FinalLogMessage = ss.str();
			if (!FinalLogMessage.empty())
			{
				FinalLogMessage.pop_back();
			}

			UE_LOG("%s", FinalLogMessage.c_str());
		}
	);

	// -- InputManager -- //
	UInputManager& InputMgr = UInputManager::GetInstance();
	LuaState.set_function("IsKeyDown", [&InputMgr](int32 Key) {
		// PIE World에서 입력 차단 중이면 false 반환
		if (GWorld && GWorld->IsIgnoringInput())
		{
			return false;
		}
		return InputMgr.IsKeyDown(static_cast<EKeyInput>(Key));
	});
	LuaState.set_function("IsKeyPressed", [&InputMgr](int32 Key) {
		// PIE World에서 입력 차단 중이면 false 반환
		if (GWorld && GWorld->IsIgnoringInput())
		{
			return false;
		}
		return InputMgr.IsKeyPressed(static_cast<EKeyInput>(Key));
	});
	LuaState.set_function("IsKeyReleased", [&InputMgr](int32 Key) {
		// PIE World에서 입력 차단 중이면 false 반환
		if (GWorld && GWorld->IsIgnoringInput())
		{
			return false;
		}
		return InputMgr.IsKeyReleased(static_cast<EKeyInput>(Key));
	});
	LuaState.set_function("GetMouseNDCPosition", [&InputMgr]() {
		return InputMgr.GetMouseNDCPosition();
	});
	LuaState.set_function("GetMousePosition", [&InputMgr]() {
		return InputMgr.GetMousePosition();
	});
	LuaState.set_function("GetMouseDelta", [&InputMgr]() {
		// PIE World에서 입력 차단 중이면 0 벡터 반환
		if (GWorld && GWorld->IsIgnoringInput())
		{
			return FVector(0, 0, 0);
		}

		// PIE 모드에서는 ConsumeMouseDelta 사용
		// 에디터 모드에서는 일반 GetMouseDelta
		if (GEditor && GEditor->IsPIESessionActive() && !GEditor->IsPIEMouseDetached())
		{
			return InputMgr.ConsumeMouseDelta();
		}

		return InputMgr.GetMouseDelta();
	});

	// -- D2D Overlay Manager -- //
    sol::table DebugDraw = LuaState.create_table("DebugDraw");

	// --- Line ---
	// Lua: DebugDraw.Line(startX, startY, endX, endY, r, g, b, a, thickness)
	DebugDraw.set_function("Line",
	    [](float startX, float startY, float endX, float endY,
	       float cR, float cG, float cB, float cA,
	       float Thickness)
	    {
	        D2D1_POINT_2F Start = D2D1::Point2F(startX, startY);
	        D2D1_POINT_2F End = D2D1::Point2F(endX, endY);
	        D2D1_COLOR_F color = D2D1::ColorF(cR, cG, cB, cA);

	        FD2DOverlayManager::GetInstance().AddLine(Start, End, color, Thickness);
	    }
	);

	// --- Ellipse ---
	// Lua: DebugDraw.Ellipse(cX, cY, rX, rY, r, g, b, a, bFilled)
	DebugDraw.set_function("Ellipse",
	    [](float cX, float cY, float RadiusX, float RadiusY,
	       float cR, float cG, float cB, float cA,
	       bool bFilled)
	    {
	        D2D1_POINT_2F Center = D2D1::Point2F(cX, cY);
	        D2D1_COLOR_F Color = D2D1::ColorF(cR, cG, cB, cA);

	        FD2DOverlayManager::GetInstance().AddEllipse(Center, RadiusX, RadiusY, Color, bFilled);
	    }
	);

	// --- Rectangle ---
	// Lua: DebugDraw.Rectangle(l, t, r, b, r, g, b, a, bFilled)
	DebugDraw.set_function("Rectangle",
	    [](float rL, float rT, float rR, float rB,
	       float cR, float cG, float cB, float cA,
	       bool bFilled)
	    {
	        D2D1_RECT_F Rect = D2D1::RectF(rL, rT, rR, rB);
	        D2D1_COLOR_F Color = D2D1::ColorF(cR, cG, cB, cA);

	        FD2DOverlayManager::GetInstance().AddRectangle(Rect, Color, bFilled);
	    }
	);

	// --- Text ---
	// Lua: DebugDraw.Text(text, l, t, r, b, r, g, b, a, fontSize, bBold, bCentered, fontName)
	DebugDraw.set_function("Text",
	    [](const std::string& Text,
	       float rL, float rT, float rR, float rB,
	       float cR, float cG, float cB, float cA,
	       float FontSize, bool bBold, bool bCentered, const std::string& FontName)
	    {
	        D2D1_RECT_F Rect = D2D1::RectF(rL, rT, rR, rB);
	        D2D1_COLOR_F Color = D2D1::ColorF(cR, cG, cB, cA);

	        std::wstring w_text = StringToWideString(Text);
	        std::wstring w_fontName = StringToWideString(FontName);

	        FD2DOverlayManager::GetInstance().AddText(
	            w_text.c_str(), Rect, Color, FontSize, bBold, bCentered, w_fontName.c_str()
	        );
	    }
	);

	// --- GetViewportWidth ---
	// Lua: DebugDraw.GetViewportWidth()
	DebugDraw.set_function("GetViewportWidth",
	    []()
	    {
	        return FD2DOverlayManager::GetInstance().GetViewportWidth();
	    }
	);

	// --- GetViewportHeight ---
	// Lua: DebugDraw.GetViewportHeight()
	DebugDraw.set_function("GetViewportHeight",
	    []()
	    {
	        return FD2DOverlayManager::GetInstance().GetViewportHeight();
	    }
	);
	// -- Math -- //
	LuaState.set_function("Clamp", &Clamp<float>);
}
