#pragma once
#include "sol/sol.hpp"

class FLuaBinder
{
public:
	static void BindCoreTypes(sol::state& LuaState);
	static void BindMathTypes(sol::state& LuaState);
	static void BindActorTypes(sol::state& LuaState);
	static void BindComponentTypes(sol::state& LuaState);
	static void BindCoreFunctions(sol::state& LuaState);
};
