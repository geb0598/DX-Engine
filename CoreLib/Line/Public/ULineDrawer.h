#pragma once

#include <d3d11.h>

#include <wrl.h>

#include "Containers/Containers.h"
#include "Types/Types.h"
#include "Mesh/Public/Vertex.h"

#define AXIS_LENGTH 10000.0f

// red line
extern FVertexSimple XAxisVertices[];
// green line
extern FVertexSimple YAxisVertices[];
// blue line
extern FVertexSimple ZAxisVertices[];

class ULineDrawer
{
public:
	~ULineDrawer() = default;

	ULineDrawer(ID3D11Device* Device, ID3D11DeviceContext* DeviceContext);

	ULineDrawer(const ULineDrawer&) = delete;
	ULineDrawer(ULineDrawer&&) noexcept = default;

	ULineDrawer& operator=(const ULineDrawer&) = delete;
	ULineDrawer& operator=(ULineDrawer&&) noexcept = default;

	void Render();
	void Release();

	static void InitializeXYZAxis(ID3D11Device* Device);
	static void RenderXYZAxis(ID3D11DeviceContext* DeviceContext);
	static void ReleaseXYZAxis();

private:
	ID3D11Buffer* VertexBuffer;
	ID3D11Device* Device;
	ID3D11DeviceContext* DeviceContext;

	UINT VertexCount;
	UINT Stride;

	// Static resources for XYZ axis rendering
	static ID3D11Buffer* XAxisVertexBuffer;
	static ID3D11Buffer* YAxisVertexBuffer;
	static ID3D11Buffer* ZAxisVertexBuffer;
	static bool bAxisInitialized;
};