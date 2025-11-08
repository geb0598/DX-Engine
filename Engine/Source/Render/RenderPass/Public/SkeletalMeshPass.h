#pragma once

#include "RenderPass.h"

class FSkeletalMeshPass : public FRenderPass
{
public:
	FSkeletalMeshPass(UPipeline* InPipeline,
		ID3D11Buffer* InConstantBufferViewProj,
		ID3D11Buffer* InConstantBufferModel
	);

	virtual void SetRenderTargets(class UDeviceResources* DeviceResources) override;
	virtual void Execute(FRenderingContext& Context) override;
	virtual void Release() override;

private:
	ID3D11VertexShader* VertexShader;
	ID3D11PixelShader* PixelShader;
	ID3D11InputLayout* InputLayout;
	ID3D11DepthStencilState* DepthStencilState;
};
