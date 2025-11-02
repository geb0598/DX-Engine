#include "pch.h"
#include "Manager/Lua/Public/LuaBinder.h"
#include "Actor/Public/GameMode.h"
#include "Component/Public/ScriptComponent.h"
#include "Manager/Input/Public/InputManager.h"

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
        "FindTemplateActorOfName", &UWorld::FindTemplateActorOfName
    );

    // --- GWorld 인스턴스 접근자 (전역 함수) ---
    LuaState.set_function("GetWorld", []() -> UWorld* {
    	return GWorld;
    });
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
		"GetActorRightVector", sol::resolve<FVector() const>(&AActor::GetActorRightVector)
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
        )
    );
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
		return InputMgr.GetMouseDelta();
	});
}
