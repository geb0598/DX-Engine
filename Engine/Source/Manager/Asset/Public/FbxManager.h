#pragma once
#include "FbxImporter.h"
#include "Component/Mesh/Public/StaticMesh.h"

class FFbxManager
{
public:
	static FStaticMesh* LoadFbxStaticMeshAsset(
		const FName& FilePath,
		const FFbxImporter::Configuration& Config = {});

	static UStaticMesh* LoadFbxStaticMesh(
		const FName& FilePath,
		const FFbxImporter::Configuration& Config = {});
};
