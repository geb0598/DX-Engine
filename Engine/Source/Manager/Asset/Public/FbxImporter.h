#pragma once
#include "fbxsdk.h"
#include <filesystem>
#include "Global/Vector.h"
#include "Global/Types.h"

struct FFbxMeshInfo
{
	TArray<FVector> VertexList;
	TArray<FVector> NormalList;
	TArray<FVector2> TexCoordList;
	TArray<uint32> Indices;
};

class FFbxImporter
{
public:
	struct Configuration
	{
		bool bConvertToUEBasis = true;
	};

	// 🔸 FBX SDK 세션 관리
	static bool Initialize();
	static void Shutdown();

	static bool LoadFBX(
		const std::filesystem::path& FilePath,
		FFbxMeshInfo* OutMeshInfo,
		Configuration Config = {});

private:
	static inline FbxManager* SdkManager = nullptr;
	static inline FbxIOSettings* IoSettings = nullptr;
};
