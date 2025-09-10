#pragma once

#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <memory>

#include "Types/Types.h"

using namespace DirectX;

struct PSConstants
{
	float FadeDistance;      // 4
	float ScreenSize[2];     // 8
	float pad0;           // 4 -> padding

	float GridColor[3];      // 12
	float pad1;              // 4 -> padding

	float InvViewProj[4][4]; // 64
};

// Forward declarations
class UVertexShader;
class UPixelShader;

// didn't use VertexShader and PixelShader because this function use shader differently.
// for example, this function does not use constant buffer on vertex shader
// but use it on pixel shader
class UGridManager
{
private:
	ID3D11Device* Device;
	ID3D11DeviceContext* DeviceContext;

	// Use shared_ptr to reference shaders from AssetManager
	std::shared_ptr<UVertexShader> GridVertexShader;
	std::shared_ptr<UPixelShader> GridPixelShader;
	ID3D11Buffer* VertexBuffer;
	ID3D11Buffer* PSConstantBuffer;
	ID3D11DepthStencilState* GridDepthStencilState;
	ID3D11DepthStencilState* ObjectDepthStencilState;
	
	UINT VertexCount;
	UINT Stride;

public:
	UGridManager(ID3D11Device* Device, ID3D11DeviceContext* DeviceContext);
	void Initialize();
	void Bind(PSConstants GridInfo);
	void Render();
	void Release();
};
