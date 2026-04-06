#include "pch.h"
#include "Render/RenderPass/Public/ShadowMapPass.h"
#include "Render/Renderer/Public/Pipeline.h"
#include "Render/Renderer/Public/RenderResourceFactory.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Component/Public/DirectionalLightComponent.h"
#include "Component/Public/SpotLightComponent.h"
#include "Component/Public/PointLightComponent.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"
INSIGHTS_DECLARE_STATGROUP("Shadow", GStatGroupShadow)
INSIGHTS_DECLARE_STAT("Shadow Map Pass", GStatShadowMapPass, GStatGroupShadow)

#define MAX_LIGHT_NUM 8
#define X_OFFSET 1024.0f
#define Y_OFFSET 1024.0f
#define SHADOW_MAP_RESOLUTION 1024.0f

// Helper functions for matrix operations
namespace ShadowMatrixHelper
{
	// Create a look-at view matrix (Left-Handed)
	FMatrix CreateLookAtLH(const FVector& Eye, const FVector& Target, const FVector& Up)
	{
		FVector ZAxis = (Target - Eye).GetNormalized();
		FVector XAxis = Up.Cross(ZAxis).GetNormalized();
		FVector YAxis = ZAxis.Cross(XAxis);

		FMatrix Result;
		Result.Data[0][0] = XAxis.X;  Result.Data[0][1] = YAxis.X;  Result.Data[0][2] = ZAxis.X;  Result.Data[0][3] = 0.0f;
		Result.Data[1][0] = XAxis.Y;  Result.Data[1][1] = YAxis.Y;  Result.Data[1][2] = ZAxis.Y;  Result.Data[1][3] = 0.0f;
		Result.Data[2][0] = XAxis.Z;  Result.Data[2][1] = YAxis.Z;  Result.Data[2][2] = ZAxis.Z;  Result.Data[2][3] = 0.0f;
		Result.Data[3][0] = -XAxis.Dot(Eye);
		Result.Data[3][1] = -YAxis.Dot(Eye);
		Result.Data[3][2] = -ZAxis.Dot(Eye);
		Result.Data[3][3] = 1.0f;

		return Result;
	}

	// Create an orthographic projection matrix (Left-Handed)
	FMatrix CreateOrthoLH(float Left, float Right, float Bottom, float Top, float Near, float Far)
	{
		FMatrix Result;
		Result.Data[0][0] = 2.0f / (Right - Left);
		Result.Data[0][1] = 0.0f;
		Result.Data[0][2] = 0.0f;
		Result.Data[0][3] = 0.0f;

		Result.Data[1][0] = 0.0f;
		Result.Data[1][1] = 2.0f / (Top - Bottom);
		Result.Data[1][2] = 0.0f;
		Result.Data[1][3] = 0.0f;

		Result.Data[2][0] = 0.0f;
		Result.Data[2][1] = 0.0f;
		Result.Data[2][2] = 1.0f / (Far - Near);
		Result.Data[2][3] = 0.0f;

		Result.Data[3][0] = (Left + Right) / (Left - Right);
		Result.Data[3][1] = (Top + Bottom) / (Bottom - Top);
		Result.Data[3][2] = Near / (Near - Far);
		Result.Data[3][3] = 1.0f;

		return Result;
	}

	// Create a perspective projection matrix (Left-Handed)
	// Used for spot light shadow mapping
	FMatrix CreatePerspectiveFovLH(float FovYRadians, float Aspect, float Near, float Far)
	{
		// f = 1 / tan(fovY/2)
		const float F = 1.0f / std::tanf(FovYRadians * 0.5f);

		FMatrix Result = FMatrix::Identity();
		// | f/aspect   0        0         0 |
		// |    0       f        0         0 |
		// |    0       0   zf/(zf-zn)     1 |
		// |    0       0  -zn*zf/(zf-zn)  0 |
		Result.Data[0][0] = F / Aspect;
		Result.Data[1][1] = F;
		Result.Data[2][2] = Far / (Far - Near);
		Result.Data[2][3] = 1.0f;
		Result.Data[3][2] = (-Near * Far) / (Far - Near);
		Result.Data[3][3] = 0.0f;

		return Result;
	}
}

FShadowMapPass::FShadowMapPass(UPipeline* InPipeline,
	ID3D11Buffer* InConstantBufferCamera,
	ID3D11Buffer* InConstantBufferModel,
	ID3D11VertexShader* InDepthOnlyVS,
	ID3D11PixelShader* InDepthOnlyPS,
	ID3D11InputLayout* InDepthOnlyInputLayout,
	ID3D11VertexShader* InPointLightShadowVS,
	ID3D11PixelShader* InPointLightShadowPS,
	ID3D11InputLayout* InPointLightShadowInputLayout)
	: FRenderPass(InPipeline, InConstantBufferCamera, InConstantBufferModel)
	, DepthOnlyVS(InDepthOnlyVS)
	, DepthOnlyPS(InDepthOnlyPS)
	, DepthOnlyInputLayout(InDepthOnlyInputLayout)
	, LinearDepthOnlyVS(InPointLightShadowVS)
	, LinearDepthOnlyPS(InPointLightShadowPS)
	, PointLightShadowInputLayout(InPointLightShadowInputLayout)
{
	ID3D11Device* Device = URenderer::GetInstance().GetDevice();

	// 1. Shadow depth stencil state ?�성
	D3D11_DEPTH_STENCIL_DESC DSDesc = {};
	DSDesc.DepthEnable = TRUE;
	DSDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	DSDesc.DepthFunc = D3D11_COMPARISON_LESS;
	DSDesc.StencilEnable = FALSE;

	HRESULT hr = Device->CreateDepthStencilState(&DSDesc, &ShadowDepthStencilState);
	if (FAILED(hr))
	{
		throw std::runtime_error("Failed to create shadow depth stencil state");
	}

	// 2. Shadow rasterizer state (slope-scale depth bias 지??
	D3D11_RASTERIZER_DESC RastDesc = {};
	RastDesc.FillMode = D3D11_FILL_SOLID;
	RastDesc.CullMode = D3D11_CULL_BACK;  // Back-face culling
	RastDesc.FrontCounterClockwise = FALSE;
	RastDesc.DepthBias = 0;               // ?�적?�로 ?�정
	RastDesc.DepthBiasClamp = 0.0f;
	RastDesc.SlopeScaledDepthBias = 0.0f; // ?�적?�로 ?�정
	RastDesc.DepthClipEnable = TRUE;
	RastDesc.ScissorEnable = FALSE;
	RastDesc.MultisampleEnable = FALSE;
	RastDesc.AntialiasedLineEnable = FALSE;

	hr = Device->CreateRasterizerState(&RastDesc, &ShadowRasterizerState);
	if (FAILED(hr))
	{
		throw std::runtime_error("Failed to create shadow rasterizer state");
	}

	// 3. Shadow view-projection constant buffer ?�성 (DepthOnlyVS.hlsl??PerFrame�??�일)
	ShadowViewProjConstantBuffer = FRenderResourceFactory::CreateConstantBuffer<FShadowViewProjConstant>();

	// 5. Point Light Shadow constant buffer ?�성
	PointLightShadowParamsBuffer = FRenderResourceFactory::CreateConstantBuffer<FPointLightShadowParams>();

	ConstantCascadeData = FRenderResourceFactory::CreateConstantBuffer<FCascadeShadowMapData>();
	
	ShadowAtlas.Initialize(Device, 8192);

	D3D11_BUFFER_DESC BufferDesc = {};
	
	//BufferDesc.Usage = D3D11_USAGE_DEFAULT;
	BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	BufferDesc.ByteWidth = (UINT)(8 * sizeof(FShadowAtlasTilePos));
	BufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	BufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	BufferDesc.StructureByteStride = sizeof(FShadowAtlasTilePos);
	
	// 초기 ?�이??
	
	hr = URenderer::GetInstance().GetDevice()->CreateBuffer(
		&BufferDesc,
		nullptr,
		&ShadowAtlasDirectionalLightTilePosStructuredBuffer
		);
	
	assert(SUCCEEDED(hr));

	hr = URenderer::GetInstance().GetDevice()->CreateBuffer(
		&BufferDesc,
		nullptr,
		&ShadowAtlasSpotLightTilePosStructuredBuffer
		);
	
	assert(SUCCEEDED(hr));

	BufferDesc.ByteWidth = (UINT)(8 * sizeof(FShadowAtlasPointLightTilePos));
	BufferDesc.StructureByteStride = sizeof(FShadowAtlasPointLightTilePos);
	
	hr = URenderer::GetInstance().GetDevice()->CreateBuffer(
		&BufferDesc,
		nullptr,
		&ShadowAtlasPointLightTilePosStructuredBuffer
		);
	
	assert(SUCCEEDED(hr));
	
	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	SRVDesc.Buffer.FirstElement = 0;
	SRVDesc.Buffer.NumElements = (UINT)8;
	
	hr = URenderer::GetInstance().GetDevice()->CreateShaderResourceView(
		ShadowAtlasDirectionalLightTilePosStructuredBuffer,
		&SRVDesc,
		&ShadowAtlasDirectionalLightTilePosStructuredSRV
		);

	assert(SUCCEEDED(hr));

	hr = URenderer::GetInstance().GetDevice()->CreateShaderResourceView(
		ShadowAtlasSpotLightTilePosStructuredBuffer,
		&SRVDesc,
		&ShadowAtlasSpotLightTilePosStructuredSRV
		);
	
	assert(SUCCEEDED(hr));

	hr = URenderer::GetInstance().GetDevice()->CreateShaderResourceView(
		ShadowAtlasPointLightTilePosStructuredBuffer,
		&SRVDesc,
		&ShadowAtlasPointLightTilePosStructuredSRV
		);
	
	assert(SUCCEEDED(hr));

	ShadowAtlasDirectionalLightTilePosArray.resize(8);
	ShadowAtlasPointLightTilePosArray.resize(8);
	ShadowAtlasSpotLightTilePosArray.resize(8);
}

FShadowMapPass::~FShadowMapPass()
{
	Release();
}

void FShadowMapPass::Execute(FRenderingContext& Context)
{
	INSIGHTS_GPU_SCOPE(GStatShadowMapPass);
	// IMPORTANT: Unbind shadow map SRVs before rendering to them as DSV
	// This prevents D3D11 resource hazard warnings
	const auto& Renderer = URenderer::GetInstance();
	ID3D11DeviceContext* DeviceContext = Renderer.GetDeviceContext();
	ID3D11ShaderResourceView* NullSRVs[4] = { nullptr, nullptr, nullptr, nullptr };
	DeviceContext->PSSetShaderResources(10, 4, NullSRVs);  // Unbind t10-t14

	
	// 미렌더링 영역: M1=1.0(far plane) → CurrentDepth <= M1 → 완전 밝음(그림자 없음)
	const float ClearColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	DeviceContext->ClearRenderTargetView(ShadowAtlas.VarianceShadowRTV.Get(), ClearColor);
	DeviceContext->ClearDepthStencilView(ShadowAtlas.ShadowDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	// Phase 1: Directional Lights
	ActiveDirectionalLightCount = 0;
	ActiveDirectionalCascadeCount = 0;
	for (auto DirLight : Context.DirectionalLights)
	{
		if (DirLight->GetCastShadows() && DirLight->GetLightEnabled())
		{
			// ?�효??첫번�?Dir Light�??�용
			RenderDirectionalShadowMap(DirLight, Context.StaticMeshes, Context.CurrentCamera);
			ActiveDirectionalLightCount = 1;
			ActiveDirectionalCascadeCount = UCascadeManager::GetInstance().GetSplitNum();
			break;
		}
	}

	// Phase 2: Spot Lights
	// ?�효??Spot Light�??�집?�다. 8개�? ?�한?�다.
	TArray<USpotLightComponent*> ValidSpotLights;
	for (USpotLightComponent* SpotLight : Context.SpotLights)
	{
		if (ValidSpotLights.size() >= MAX_LIGHT_NUM)
			break;
		
		if (SpotLight->GetCastShadows() && SpotLight->GetLightEnabled())
		{
			ValidSpotLights.push_back(SpotLight);
		}
	}

	ActiveSpotLightCount = static_cast<uint32>(ValidSpotLights.size());
	for (int32 i = 0; i < ValidSpotLights.size(); i++)
	{
		RenderSpotShadowMap(ValidSpotLights[i], i, Context.StaticMeshes);
	}

	// Phase 3: Point Lights
	TArray<UPointLightComponent*> ValidPointLights;
	for (UPointLightComponent* PointLight : Context.PointLights)
	{
		if (ValidPointLights.size() >= MAX_LIGHT_NUM)
			break;
		
		if (PointLight->GetCastShadows() && PointLight->GetLightEnabled())
		{
			ValidPointLights.push_back(PointLight);
		}
	}

	ActivePointLightCount = static_cast<uint32>(ValidPointLights.size());
	for (int32 i = 0; i < ValidPointLights.size(); i++)
	{
		RenderPointShadowMap(ValidPointLights[i], i, Context.StaticMeshes);
	}

	SetShadowAtlasTilePositionStructuredBuffer();
}

void FShadowMapPass::RenderDirectionalShadowMap(
	UDirectionalLightComponent* Light,
	const TArray<UStaticMeshComponent*>& Meshes,
	UCamera* InCamera
	)
{
	// FShadowMapResource* ShadowMap = GetOrCreateShadowMap(Light);
	// if (!ShadowMap || !ShadowMap->IsValid())
	// 	return;

	const auto& Renderer = URenderer::GetInstance();
	ID3D11DeviceContext* DeviceContext = Renderer.GetDeviceContext();
	const auto& DeviceResources = Renderer.GetDeviceResources();

	// 0. ?�재 ?�태 ?�??(복원??
	ID3D11RenderTargetView* OriginalRTV = nullptr;
	ID3D11DepthStencilView* OriginalDSV = nullptr;
	DeviceContext->OMGetRenderTargets(1, &OriginalRTV, &OriginalDSV);

	D3D11_VIEWPORT OriginalViewport;
	UINT NumViewports = 1;
	DeviceContext->RSGetViewports(&NumViewports, &OriginalViewport);

	// 1. Shadow render target ?�정
	// Note: RenderTargets??Pipeline API ?�용, Viewport??Pipeline 미�??�으�?DeviceContext 직접 ?�용
	// ID3D11RenderTargetView* NullRTV = nullptr;
	// Pipeline->SetRenderTargets(1, &NullRTV, ShadowMap->ShadowDSV.Get());
	// DeviceContext->RSSetViewports(1, &ShadowMap->ShadowViewport);
	// DeviceContext->ClearDepthStencilView(ShadowMap->ShadowDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	//ID3D11RenderTargetView* NullRTV = nullptr;
	//Pipeline->SetRenderTargets(1, &NullRTV, ShadowAtlas.ShadowDSV.Get());
	Pipeline->SetRenderTargets(
		1,
		ShadowAtlas.VarianceShadowRTV.GetAddressOf(),
		ShadowAtlas.ShadowDSV.Get()
		);

	// 2. Light�?캐싱??rasterizer state 가?�오�?(DepthBias ?�함)
	ID3D11RasterizerState* RastState = GetOrCreateRasterizerState(
		Light->GetShadowBias(),
		Light->GetShadowSlopeBias()
	);

	// 3. Pipeline???�해 shadow rendering state ?�정
	FPipelineInfo ShadowPipelineInfo = {
		DepthOnlyInputLayout,
		DepthOnlyVS,
		RastState,  // 캐싱??state ?�용 (�??�레???�성/?�제 방�?)
		ShadowDepthStencilState,
		DepthOnlyPS,
		nullptr,  // No blend state
		D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
	};
	Pipeline->UpdatePipeline(ShadowPipelineInfo);

	UCascadeManager& CascadeManager = UCascadeManager::GetInstance();
	FCascadeShadowMapData CascadeShadowMapData = CascadeManager.GetCascadeShadowMapData(InCamera, Light);

	FRenderResourceFactory::UpdateConstantBufferData(ConstantCascadeData, CascadeShadowMapData);
	Pipeline->SetConstantBuffer(6, EShaderType::VS | EShaderType::PS, ConstantCascadeData);
	
	for (int i = 0; i < CascadeManager.GetSplitNum(); i++)
	{
		D3D11_VIEWPORT ShadowViewport;
	
		ShadowViewport.Width = Light->GetShadowResolutionScale();
		ShadowViewport.Height = Light->GetShadowResolutionScale();
		ShadowViewport.MinDepth = 0.0f;
		ShadowViewport.MaxDepth = 1.0f;
		ShadowViewport.TopLeftX = SHADOW_MAP_RESOLUTION * i;
		ShadowViewport.TopLeftY = 0.0f;
	
		DeviceContext->RSSetViewports(1, &ShadowViewport);

		ShadowAtlasDirectionalLightTilePosArray[i] = {{static_cast<uint32>(i), 0}, {}};

		// 4. Light view-projection 계산
		// FMatrix LightView, LightProj;
		// CalculateDirectionalLightViewProj(Light, Meshes, LightView, LightProj);

		// Store the calculated shadow view-projection matrix in the light component
		//FMatrix LightViewProj = LightView * LightProj;
		FMatrix LightView = CascadeShadowMapData.View;
		FMatrix LightProj = CascadeShadowMapData.Proj[i];
		FMatrix LightViewProj = LightView * LightProj;

		// Cascade??ViewProj가 ?�러개라??추후 ?�정?�던가 ?�려????- HSH
		// Light->SetShadowViewProjection(LightViewProj);

		// 5. �?메시 ?�더�?
		for (auto Mesh : Meshes)
		{
			if (Mesh->IsVisible())
			{
				RenderMeshDepth(Mesh, LightView, LightProj);
			}
		}
	}

	// 6. ?�태 복원
	// RenderTarget�?DepthStencil 복원 (Pipeline API ?�용)
	Pipeline->SetRenderTargets(1, &OriginalRTV, OriginalDSV);

	// Viewport 복원 (DeviceContext 직접 ?�용)
	DeviceContext->RSSetViewports(1, &OriginalViewport);

	// ?�시 리소???�제
	if (OriginalRTV)
		OriginalRTV->Release();
	if (OriginalDSV)
		OriginalDSV->Release();

	// Note: RastState??캐싱?��?�??�기???�제?��? ?�음 (Release()?�서 ?�괄 ?�제)
}

void FShadowMapPass::RenderSpotShadowMap(
	USpotLightComponent* Light,
	uint32 AtlasIndex,
	const TArray<UStaticMeshComponent*>& Meshes
	)
{
	// FShadowMapResource* ShadowMap = GetOrCreateShadowMap(Light);
	// if (!ShadowMap || !ShadowMap->IsValid())
	// 	return;

	const auto& Renderer = URenderer::GetInstance();
	ID3D11DeviceContext* DeviceContext = Renderer.GetDeviceContext();

	// 0. ?�재 ?�태 ?�??(복원??
	ID3D11RenderTargetView* OriginalRTV = nullptr;
	ID3D11DepthStencilView* OriginalDSV = nullptr;
	DeviceContext->OMGetRenderTargets(1, &OriginalRTV, &OriginalDSV);

	D3D11_VIEWPORT OriginalViewport;
	UINT NumViewports = 1;
	DeviceContext->RSGetViewports(&NumViewports, &OriginalViewport);

	// // 1. Shadow render target ?�정
	Pipeline->SetRenderTargets(
		1,
		ShadowAtlas.VarianceShadowRTV.GetAddressOf(),
		ShadowAtlas.ShadowDSV.Get()
		);

	D3D11_VIEWPORT ShadowViewport;

	const static float Y_START = SHADOW_MAP_RESOLUTION;

	ShadowViewport.Width = Light->GetShadowResolutionScale();
	ShadowViewport.Height = Light->GetShadowResolutionScale();
	ShadowViewport.MinDepth = 0.0f;
	ShadowViewport.MaxDepth = 1.0f;
	ShadowViewport.TopLeftX = X_OFFSET * AtlasIndex;
	ShadowViewport.TopLeftY = Y_START;

	ShadowAtlasSpotLightTilePosArray[AtlasIndex] = {{AtlasIndex, 1}};
	
	DeviceContext->RSSetViewports(1, &ShadowViewport);
	
	// 2. Light�?캐싱??rasterizer state 가?�오�?(DepthBias ?�함)
	ID3D11RasterizerState* RastState = ShadowRasterizerState;
	if (Light->GetShadowModeIndex() == EShadowModeIndex::SMI_UnFiltered || Light->GetShadowModeIndex() == EShadowModeIndex::SMI_PCF)
	{
		RastState = GetOrCreateRasterizerState(
			Light->GetShadowBias(),
			Light->GetShadowSlopeBias()
		);
	}

	// 3. Pipeline???�해 shadow rendering state ?�정
	FPipelineInfo ShadowPipelineInfo = {
		DepthOnlyInputLayout,
		LinearDepthOnlyVS,
		RastState,  // 캐싱??state ?�용 (�??�레???�성/?�제 방�?)
		ShadowDepthStencilState,
		LinearDepthOnlyPS,
		nullptr,  // No blend state
		D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
	};
	Pipeline->UpdatePipeline(ShadowPipelineInfo);

	// 4. Light view-projection 계산 (Perspective projection for cone-shaped frustum)
	FMatrix LightView, LightProj;
	CalculateSpotLightViewProj(Light, Meshes, LightView, LightProj);

	// Store the calculated shadow view-projection matrix in the light component
	FMatrix LightViewProj = LightView * LightProj;
	Light->SetShadowViewProjection(LightViewProj);  // Will be added to SpotLightComponent in Phase 6
	
	FPointLightShadowParams Params;
	Params.LightPosition = Light->GetWorldLocation();
	Params.LightRange = Light->GetAttenuationRadius();
	FRenderResourceFactory::UpdateConstantBufferData(PointLightShadowParamsBuffer, Params);
	Pipeline->SetConstantBuffer(2, EShaderType::PS, PointLightShadowParamsBuffer);

	// 5. �?메시 ?�더�?
	for (auto Mesh : Meshes)
	{
		if (Mesh->IsVisible())
		{
			RenderMeshDepth(Mesh, LightView, LightProj);
		}
	}

	// 6. ?�태 복원
	// RenderTarget�?DepthStencil 복원 (Pipeline API ?�용)
	Pipeline->SetRenderTargets(1, &OriginalRTV, OriginalDSV);

	// Viewport 복원 (DeviceContext 직접 ?�용)
	DeviceContext->RSSetViewports(1, &OriginalViewport);

	// ?�시 리소???�제
	if (OriginalRTV)
		OriginalRTV->Release();
	if (OriginalDSV)
		OriginalDSV->Release();

	// Note: RastState??캐싱?��?�??�기???�제?��? ?�음 (Release()?�서 ?�괄 ?�제)
}

void FShadowMapPass::RenderPointShadowMap(
	UPointLightComponent* Light,
	uint32 AtlasIndex,
	const TArray<UStaticMeshComponent*>& Meshes
	)
{
	// FCubeShadowMapResource* ShadowMap = GetOrCreateCubeShadowMap(Light);
	// if (!ShadowMap || !ShadowMap->IsValid())
	// 	return;

	const auto& Renderer = URenderer::GetInstance();
	ID3D11DeviceContext* DeviceContext = Renderer.GetDeviceContext();

	// 0. ?�재 ?�태 ?�??(복원??
	ID3D11RenderTargetView* OriginalRTV = nullptr;
	ID3D11DepthStencilView* OriginalDSV = nullptr;
	DeviceContext->OMGetRenderTargets(1, &OriginalRTV, &OriginalDSV);

	D3D11_VIEWPORT OriginalViewport;
	UINT NumViewports = 1;
	DeviceContext->RSGetViewports(&NumViewports, &OriginalViewport);

	// 1. Rasterizer state
	ID3D11RasterizerState* RastState = ShadowRasterizerState;
	if (Light->GetShadowModeIndex() == EShadowModeIndex::SMI_UnFiltered || Light->GetShadowModeIndex() == EShadowModeIndex::SMI_PCF)
	{
		RastState = GetOrCreateRasterizerState(
			Light->GetShadowBias(),
			Light->GetShadowSlopeBias()
		);
	}

	// 2. Pipeline ?�정 (Point Light??linear distance�?depth�??�?�하므�?pixel shader ?�요)
	FPipelineInfo ShadowPipelineInfo = {
		PointLightShadowInputLayout,
		LinearDepthOnlyVS,
		RastState,
		ShadowDepthStencilState,
		LinearDepthOnlyPS,  // Pixel shader for linear distance output
		nullptr,  // No blend state
		D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
	};
	Pipeline->UpdatePipeline(ShadowPipelineInfo);

	// 2.5. Point Light shadow params ?�정 (light position, range)
	FPointLightShadowParams Params;
	Params.LightPosition = Light->GetWorldLocation();
	Params.LightRange = Light->GetAttenuationRadius();
	FRenderResourceFactory::UpdateConstantBufferData(PointLightShadowParamsBuffer, Params);
	Pipeline->SetConstantBuffer(2, EShaderType::PS, PointLightShadowParamsBuffer);

	// 3. 6�?View-Projection 계산
	FMatrix ViewProj[6];
	CalculatePointLightViewProj(Light, ViewProj);

	// ?�나??Atlas??모두 ?�성?��?�?
	// RenderTarget?� 변경될 ?�이 ?�어 먼�? Set?�다.
	Pipeline->SetRenderTargets(1, ShadowAtlas.VarianceShadowRTV.GetAddressOf(), ShadowAtlas.ShadowDSV.Get());
	
	// 4. 6�?�??�더�?(+X, -X, +Y, -Y, +Z, -Z)
	for (int Face = 0; Face < 6; Face++)
	{
		// // 4-1. DSV ?�정 (�?�?
		D3D11_VIEWPORT ShadowViewport;

		static const float Y_START = SHADOW_MAP_RESOLUTION * 2.0f;

		ShadowViewport.Width = Light->GetShadowResolutionScale();
		ShadowViewport.Height = Light->GetShadowResolutionScale();
		ShadowViewport.MinDepth = 0.0f;
		ShadowViewport.MaxDepth = 1.0f;
		ShadowViewport.TopLeftX = X_OFFSET * AtlasIndex;
		ShadowViewport.TopLeftY = Y_START + Y_OFFSET * Face;

		DeviceContext->RSSetViewports(1, &ShadowViewport);

		ShadowAtlasPointLightTilePosArray[AtlasIndex].UV[Face][0] = AtlasIndex;
		ShadowAtlasPointLightTilePosArray[AtlasIndex].UV[Face][1] = 2 + Face;

		// 4-2. Constant buffer ?�데?�트 (�?면의 ViewProj)
		FShadowViewProjConstant CBData;
		CBData.ViewProjection = ViewProj[Face];
		FRenderResourceFactory::UpdateConstantBufferData(ShadowViewProjConstantBuffer, CBData);
		Pipeline->SetConstantBuffer(1, EShaderType::VS, ShadowViewProjConstantBuffer);

		// 4-3. 메시 ?�더�?
		for (auto Mesh : Meshes)
		{
			if (Mesh->IsVisible())
			{
				// Model transform ?�데?�트
				FMatrix WorldMatrix = Mesh->GetWorldTransformMatrix();
				FRenderResourceFactory::UpdateConstantBufferData(ConstantBufferModel, WorldMatrix);
				Pipeline->SetConstantBuffer(0, EShaderType::VS, ConstantBufferModel);

				// Vertex/Index buffer 바인??
				ID3D11Buffer* VertexBuffer = Mesh->GetVertexBuffer();
				ID3D11Buffer* IndexBuffer = Mesh->GetIndexBuffer();
				uint32 IndexCount = Mesh->GetNumIndices();

				if (!VertexBuffer || !IndexBuffer || IndexCount == 0)
					continue;

				Pipeline->SetVertexBuffer(VertexBuffer, sizeof(FNormalVertex));
				Pipeline->SetIndexBuffer(IndexBuffer, 0);

				// Draw call
				Pipeline->DrawIndexed(IndexCount, 0, 0);
			}
		}
	}

	// 5. ?�태 복원
	Pipeline->SetRenderTargets(1, &OriginalRTV, OriginalDSV);
	DeviceContext->RSSetViewports(1, &OriginalViewport);

	if (OriginalRTV)
		OriginalRTV->Release();
	if (OriginalDSV)
		OriginalDSV->Release();

	// Note: 6�?ViewProj�?PointLightComponent???�?�하??것�? 비효?�적?��?�?
	// Shader?�서 Light position 기반?�로 direction??계산?�도�?구현
}

void FShadowMapPass::SetShadowAtlasTilePositionStructuredBuffer()
{
	FRenderResourceFactory::UpdateStructuredBuffer(
		ShadowAtlasDirectionalLightTilePosStructuredBuffer,
		ShadowAtlasDirectionalLightTilePosArray
		);
	Pipeline->SetShaderResourceView(
		12,
		EShaderType::VS | EShaderType::PS,
		ShadowAtlasDirectionalLightTilePosStructuredSRV
		);

	FRenderResourceFactory::UpdateStructuredBuffer(
		ShadowAtlasSpotLightTilePosStructuredBuffer,
		ShadowAtlasSpotLightTilePosArray
		);
	Pipeline->SetShaderResourceView(
		13,
		EShaderType::VS | EShaderType::PS,
		ShadowAtlasSpotLightTilePosStructuredSRV
		);

	FRenderResourceFactory::UpdateStructuredBuffer(
		ShadowAtlasPointLightTilePosStructuredBuffer,
		ShadowAtlasPointLightTilePosArray
		);
	Pipeline->SetShaderResourceView(
		14,
		EShaderType::VS | EShaderType::PS,
		ShadowAtlasPointLightTilePosStructuredSRV
		);
}

void FShadowMapPass::CalculateDirectionalLightViewProj(UDirectionalLightComponent* Light,
	const TArray<UStaticMeshComponent*>& Meshes, FMatrix& OutView, FMatrix& OutProj)
{
	// 1. 모든 메시??AABB�??�함?�는 bounding box 계산
	FVector MinBounds(FLT_MAX, FLT_MAX, FLT_MAX);
	FVector MaxBounds(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	bool bHasValidMeshes = false;
	for (auto Mesh : Meshes)
	{
		if (!Mesh->IsVisible())
			continue;

		// 메시??world AABB 가?�오�?
		FVector WorldMin, WorldMax;
		Mesh->GetWorldAABB(WorldMin, WorldMax);

		MinBounds.X = std::min(MinBounds.X, WorldMin.X);
		MinBounds.Y = std::min(MinBounds.Y, WorldMin.Y);
		MinBounds.Z = std::min(MinBounds.Z, WorldMin.Z);

		MaxBounds.X = std::max(MaxBounds.X, WorldMax.X);
		MaxBounds.Y = std::max(MaxBounds.Y, WorldMax.Y);
		MaxBounds.Z = std::max(MaxBounds.Z, WorldMax.Z);

		bHasValidMeshes = true;
	}

	// 메시가 ?�으�?기본 ?�렬 반환
	if (!bHasValidMeshes)
	{
		OutView = FMatrix::Identity();
		OutProj = FMatrix::Identity();
		return;
	}

	// 2. Light direction 기�??�로 view matrix ?�성
	FVector LightDir = Light->GetForwardVector();
	if (LightDir.Length() < 1e-6f)
		LightDir = FVector(0, 0, -1);
	else
		LightDir = LightDir.GetNormalized();

	FVector SceneCenter = (MinBounds + MaxBounds) * 0.5f;
	float SceneRadius = (MaxBounds - MinBounds).Length() * 0.5f;

	// Light position?� scene 중심?�서 light direction 반�?�?충분??멀�?
	FVector LightPos = SceneCenter - LightDir * (SceneRadius + 50.0f);

	// Up vector 계산 (Z-Up, X-Forward, Y-Right Left-Handed 좌표�?
	FVector Up = FVector(0, 0, 1);  // Z-Up
	if (std::abs(LightDir.Z) > 0.99f)  // Light가 거의 ?�직(Z축과 ?�행)?�면
		Up = FVector(1, 0, 0);  // X-Forward�?fallback?�로

	OutView = ShadowMatrixHelper::CreateLookAtLH(LightPos, SceneCenter, Up);

	// 3. AABB�?light view space�?변?�하??orthographic projection 범위 계산
	FVector LightSpaceMin(FLT_MAX, FLT_MAX, FLT_MAX);
	FVector LightSpaceMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	// AABB??8�?코너�?light view space�?변??
	FVector Corners[8] = {
		FVector(MinBounds.X, MinBounds.Y, MinBounds.Z),
		FVector(MaxBounds.X, MinBounds.Y, MinBounds.Z),
		FVector(MinBounds.X, MaxBounds.Y, MinBounds.Z),
		FVector(MaxBounds.X, MaxBounds.Y, MinBounds.Z),
		FVector(MinBounds.X, MinBounds.Y, MaxBounds.Z),
		FVector(MaxBounds.X, MinBounds.Y, MaxBounds.Z),
		FVector(MinBounds.X, MaxBounds.Y, MaxBounds.Z),
		FVector(MaxBounds.X, MaxBounds.Y, MaxBounds.Z)
	};

	for (int i = 0; i < 8; i++)
	{
		FVector LightSpaceCorner = OutView.TransformPosition(Corners[i]);

		LightSpaceMin.X = std::min(LightSpaceMin.X, LightSpaceCorner.X);
		LightSpaceMin.Y = std::min(LightSpaceMin.Y, LightSpaceCorner.Y);
		LightSpaceMin.Z = std::min(LightSpaceMin.Z, LightSpaceCorner.Z);

		LightSpaceMax.X = std::max(LightSpaceMax.X, LightSpaceCorner.X);
		LightSpaceMax.Y = std::max(LightSpaceMax.Y, LightSpaceCorner.Y);
		LightSpaceMax.Z = std::max(LightSpaceMax.Z, LightSpaceCorner.Z);
	}

	// 4. Orthographic projection ?�성
	// Scene ?�기??비�???padding ?�용 (?�이 ?�면 padding???�게)
	float SceneSizeZ = LightSpaceMax.Z - LightSpaceMin.Z;
	float Padding = std::max(SceneSizeZ * 0.5f, 50.0f);  // 최소 50, ??깊이??50%

	float Left = LightSpaceMin.X;
	float Right = LightSpaceMax.X;
	float Bottom = LightSpaceMin.Y;
	float Top = LightSpaceMax.Y;
	float Near = LightSpaceMin.Z - Padding;
	float Far = LightSpaceMax.Z + Padding;

	// Near???�수가 ?�면 ?�됨 (orthographic?�서??괜찮지�? ?�전???�해)
	Near = std::max(Near, 0.1f);

	OutProj = ShadowMatrixHelper::CreateOrthoLH(Left, Right, Bottom, Top, Near, Far);
}

void FShadowMapPass::CalculateSpotLightViewProj(USpotLightComponent* Light,
	const TArray<UStaticMeshComponent*>& Meshes, FMatrix& OutView, FMatrix& OutProj)
{
	// 1. Light???�치?� 방향 가?�오�?
	FVector LightPos = Light->GetWorldLocation();
	FVector LightDir = Light->GetForwardVector();

	// LightDir??거의 0?�면 기본�??�용
	if (LightDir.Length() < 1e-6f)
		LightDir = FVector(1, 0, 0);  // X-Forward (engine default)
	else
		LightDir = LightDir.GetNormalized();

	// 2. View Matrix ?�성: Light ?�치?�서 Direction 방향?�로
	FVector Target = LightPos + LightDir;

	// Up vector 계산 (Z-Up, X-Forward, Y-Right Left-Handed 좌표�?
	FVector Up = FVector(0, 0, 1);  // Z-Up
	if (std::abs(LightDir.Z) > 0.99f)  // Light가 거의 ?�직(Z축과 ?�행)?�면
		Up = FVector(1, 0, 0);  // X-Forward�?fallback?�로

	OutView = ShadowMatrixHelper::CreateLookAtLH(LightPos, Target, Up);

	// 3. Perspective Projection ?�성: Cone 모양??frustum
	float FovY = Light->GetOuterConeAngle() * 2.0f;  // Full cone angle
	float Aspect = 1.0f;  // Square shadow map
	float Near = 1.0f;    // ?�무 ?�으�?depth precision 문제
	float Far = Light->GetAttenuationRadius();  // Light range

	// FovY가 ?�무 ?�거???�면 clamp (?�효 범위: 0.1 ~ PI - 0.1)
	FovY = std::clamp(FovY, 0.1f, PI - 0.1f);

	// Far가 Near보다 ?�거??같으�?기본�??�용
	if (Far <= Near)
		Far = Near + 10.0f;

	OutProj = ShadowMatrixHelper::CreatePerspectiveFovLH(FovY, Aspect, Near, Far);
}

void FShadowMapPass::CalculatePointLightViewProj(UPointLightComponent* Light, FMatrix OutViewProj[6])
{
	// 1. Light position
	FVector LightPos = Light->GetWorldLocation();

	// 2. Near/Far planes
	float Near = 1.0f;
	float Far = Light->GetAttenuationRadius();
	if (Far <= Near)
		Far = Near + 10.0f;

	// 3. Perspective projection (90 degree FOV for cube faces)
	FMatrix Proj = ShadowMatrixHelper::CreatePerspectiveFovLH(
		PI / 2.0f,  // 90 degrees FOV
		1.0f,       // Aspect ratio 1:1 (square)
		Near,
		Far
	);

	// 4. 6 directions for cube faces (DirectX cube map order)
	// Order: +X, -X, +Y, -Y, +Z, -Z
	FVector Directions[6] = {
		FVector(1.0f, 0.0f, 0.0f),   // +X
		FVector(-1.0f, 0.0f, 0.0f),  // -X
		FVector(0.0f, 1.0f, 0.0f),   // +Y
		FVector(0.0f, -1.0f, 0.0f),  // -Y
		FVector(0.0f, 0.0f, 1.0f),   // +Z
		FVector(0.0f, 0.0f, -1.0f)   // -Z
	};

	// 5. Up vectors for each direction (avoid gimbal lock)
	FVector Ups[6] = {
		FVector(0.0f, 1.0f, 0.0f),   // +X: Y-Up
		FVector(0.0f, 1.0f, 0.0f),   // -X: Y-Up
		FVector(0.0f, 0.0f, -1.0f),  // +Y: -Z-Up (looking up, so up is -Z)
		FVector(0.0f, 0.0f, 1.0f),   // -Y: +Z-Up (looking down, so up is +Z)
		FVector(0.0f, 1.0f, 0.0f),   // +Z: Y-Up
		FVector(0.0f, 1.0f, 0.0f)    // -Z: Y-Up
	};

	// 6. Calculate View-Projection for each face
	for (int i = 0; i < 6; i++)
	{
		FVector Target = LightPos + Directions[i];
		FMatrix View = ShadowMatrixHelper::CreateLookAtLH(LightPos, Target, Ups[i]);
		OutViewProj[i] = View * Proj;
	}
}

FShadowMapResource* FShadowMapPass::GetOrCreateShadowMap(ULightComponent* Light)
{
	FShadowMapResource* ShadowMap = nullptr;
	ID3D11Device* Device = URenderer::GetInstance().GetDevice();

	if (auto DirLight = Cast<UDirectionalLightComponent>(Light))
	{
		auto It = DirectionalShadowMaps.find(DirLight);
		if (It == DirectionalShadowMaps.end())
		{
			// ?�로 ?�성
			ShadowMap = new FShadowMapResource();
			ShadowMap->Initialize(Device, Light->GetShadowMapResolution());
			DirectionalShadowMaps[DirLight] = ShadowMap;
		}
		else
		{
			ShadowMap = It->second;
			// ?�상??변�?체크
			if (ShadowMap->NeedsResize(Light->GetShadowMapResolution()))
			{
				ShadowMap->Initialize(Device, Light->GetShadowMapResolution());
			}
		}
	}
	else if (auto SpotLight = Cast<USpotLightComponent>(Light))
	{
		auto It = SpotShadowMaps.find(SpotLight);
		if (It == SpotShadowMaps.end())
		{
			ShadowMap = new FShadowMapResource();
			ShadowMap->Initialize(Device, Light->GetShadowMapResolution());
			SpotShadowMaps[SpotLight] = ShadowMap;
		}
		else
		{
			ShadowMap = It->second;
			if (ShadowMap->NeedsResize(Light->GetShadowMapResolution()))
			{
				ShadowMap->Initialize(Device, Light->GetShadowMapResolution());
			}
		}
	}

	return ShadowMap;
}

FCubeShadowMapResource* FShadowMapPass::GetOrCreateCubeShadowMap(UPointLightComponent* Light)
{
	FCubeShadowMapResource* ShadowMap = nullptr;
	ID3D11Device* Device = URenderer::GetInstance().GetDevice();

	auto It = PointShadowMaps.find(Light);
	if (It == PointShadowMaps.end())
	{
		// ?�로 ?�성
		ShadowMap = new FCubeShadowMapResource();
		ShadowMap->Initialize(Device, Light->GetShadowMapResolution());
		PointShadowMaps[Light] = ShadowMap;
	}
	else
	{
		ShadowMap = It->second;
		// ?�상??변�?체크
		if (ShadowMap->NeedsResize(Light->GetShadowMapResolution()))
		{
			ShadowMap->Initialize(Device, Light->GetShadowMapResolution());
		}
	}

	return ShadowMap;
}

FCubeShadowMapResource* FShadowMapPass::GetPointShadowMap(UPointLightComponent* Light)
{
	auto It = PointShadowMaps.find(Light);
	return It != PointShadowMaps.end() ? It->second : nullptr;
}

FShadowMapResource* FShadowMapPass::GetShadowAtlas()
{
	return &ShadowAtlas;
}

FShadowAtlasTilePos FShadowMapPass::GetDirectionalAtlasTilePos(uint32 Index) const
{
	return ShadowAtlasDirectionalLightTilePosArray[Index];
}

FShadowAtlasTilePos FShadowMapPass::GetSpotAtlasTilePos(uint32 Index) const
{
	return ShadowAtlasSpotLightTilePosArray[Index];
}

FShadowAtlasPointLightTilePos FShadowMapPass::GetPointAtlasTilePos(uint32 Index) const
{
	return ShadowAtlasPointLightTilePosArray[Index];
}

/**
 * @brief 모든 ?�?�우맵이 ?�용 중인 �?메모리�? 계산
 * @return 바이???�위 메모�??�용??
 */
uint64 FShadowMapPass::GetTotalShadowMapMemory() const
{
	uint64 TotalBytes = 0;

	// Shadow Atlas 메모�?계산 (�?Depth 32bit + Variance Map RGBA 32bit)
	if (ShadowAtlas.IsValid())
	{
		const uint64 DepthBytes = static_cast<uint64>(ShadowAtlas.Resolution) * ShadowAtlas.Resolution * 4;  // 32-bit depth
		const uint64 VarianceBytes = static_cast<uint64>(ShadowAtlas.Resolution) * ShadowAtlas.Resolution * 16;  // RGBA32F (4 channels * 4 bytes)
		TotalBytes += DepthBytes + VarianceBytes;
	}

	// Directional Light ?�?�우�?메모�?
	for (const auto& Pair : DirectionalShadowMaps)
	{
		if (Pair.second && Pair.second->IsValid())
		{
			const uint64 DepthBytes = static_cast<uint64>(Pair.second->Resolution) * Pair.second->Resolution * 4;
			const uint64 VarianceBytes = static_cast<uint64>(Pair.second->Resolution) * Pair.second->Resolution * 16;
			TotalBytes += DepthBytes + VarianceBytes;
		}
	}

	// Spot Light ?�?�우�?메모�?
	for (const auto& Pair : SpotShadowMaps)
	{
		if (Pair.second && Pair.second->IsValid())
		{
			const uint64 DepthBytes = static_cast<uint64>(Pair.second->Resolution) * Pair.second->Resolution * 4;
			const uint64 VarianceBytes = static_cast<uint64>(Pair.second->Resolution) * Pair.second->Resolution * 16;
			TotalBytes += DepthBytes + VarianceBytes;
		}
	}

	// Point Light ?�브�??�?�우�?메모�?(6�?
	for (const auto& Pair : PointShadowMaps)
	{
		if (Pair.second && Pair.second->IsValid())
		{
			const uint64 DepthBytesPerFace = static_cast<uint64>(Pair.second->Resolution) * Pair.second->Resolution * 4;
			TotalBytes += DepthBytesPerFace * 6;
		}
	}

	return TotalBytes;
}

/**
 * @brief ?�용 중인 ?��??�스 ?�??개수�?반환
 * @return ?�재 ?�용 중인 ?�??개수
 */
uint32 FShadowMapPass::GetUsedAtlasTileCount() const
{
	// ?�제 ?�용 중인 ?�??개수 계산
	// Directional CSM Count + Spot * 1 + Point * 6 (?�브�?6�?
	return ActiveDirectionalCascadeCount + ActiveSpotLightCount + (ActivePointLightCount * 6);
}

/**
 * @brief ?��??�스??최�? ?�??개수�?반환
 * @return 최�? ?�??개수 (4x4 = 16)
 */
uint32 FShadowMapPass::GetMaxAtlasTileCount()
{
	// 8x8 ?��??�스 = 64 ?�??(8192 / 1024 = 8)
	// TODO(KHJ): ?�단?� 고정�? ?�요 ???�로 값을 받아??�?
	return 64;
}

/**
 * @brief 메시�?shadow depth�??�더�?
 * @param InMesh Static mesh component
 * @param InView Light space view ?�렬
 * @param InProj Light space projection ?�렬
 */
void FShadowMapPass::RenderMeshDepth(const UStaticMeshComponent* InMesh, const FMatrix& InView, const FMatrix& InProj) const
{
	// Constant buffer ?�데?�트
	FShadowViewProjConstant CBData;
	CBData.ViewProjection = InView * InProj;
	FRenderResourceFactory::UpdateConstantBufferData(ShadowViewProjConstantBuffer, CBData);
	Pipeline->SetConstantBuffer(1, EShaderType::VS, ShadowViewProjConstantBuffer);

	// Model transform ?�데?�트
	FMatrix WorldMatrix = InMesh->GetWorldTransformMatrix();
	FRenderResourceFactory::UpdateConstantBufferData(ConstantBufferModel, WorldMatrix);
	Pipeline->SetConstantBuffer(0, EShaderType::VS, ConstantBufferModel);

	// Vertex/Index buffer 바인??
	ID3D11Buffer* VertexBuffer = InMesh->GetVertexBuffer();
	ID3D11Buffer* IndexBuffer = InMesh->GetIndexBuffer();
	uint32 IndexCount = InMesh->GetNumIndices();

	if (!VertexBuffer || !IndexBuffer || IndexCount == 0)
	{
		return;
	}

	Pipeline->SetVertexBuffer(VertexBuffer, sizeof(FNormalVertex));
	Pipeline->SetIndexBuffer(IndexBuffer, 0);

	// Draw call
	Pipeline->DrawIndexed(IndexCount, 0, 0);
}

void FShadowMapPass::Release()
{
	// Shadow maps ?�제
	for (auto& Pair : DirectionalShadowMaps)
	{
		if (Pair.second)
		{
			Pair.second->Release();
			delete Pair.second;
		}
	}
	DirectionalShadowMaps.clear();

	// // Rasterizer state 캐시 ?�제 (�??�레???�성 방�?�??�해 캐싱?�던 states)
	// for (auto& Pair : DirectionalRasterizerStates)
	// {
	// 	if (Pair.second)
	// 		Pair.second->Release();
	// }
	// DirectionalRasterizerStates.clear();
	//
	// for (auto& Pair : SpotRasterizerStates)
	// {
	// 	if (Pair.second)
	// 		Pair.second->Release();
	// }
	// SpotRasterizerStates.clear();
	//
	// for (auto& Pair : PointRasterizerStates)
	// {
	// 	if (Pair.second)
	// 		Pair.second->Release();
	// }
	// PointRasterizerStates.clear();

	for (auto& Pair : LightRasterizerStates)
	{
		if (Pair.second)
			Pair.second->Release();
	}
	LightRasterizerStates.clear();

	for (auto& Pair : SpotShadowMaps)
	{
		if (Pair.second)
		{
			Pair.second->Release();
			delete Pair.second;
		}
	}
	SpotShadowMaps.clear();

	for (auto& Pair : PointShadowMaps)
	{
		if (Pair.second)
		{
			Pair.second->Release();
			delete Pair.second;
		}
	}
	PointShadowMaps.clear();

	// States ?�제
	if (ShadowDepthStencilState)
	{
		ShadowDepthStencilState->Release();
		ShadowDepthStencilState = nullptr;
	}

	if (ShadowRasterizerState)
	{
		ShadowRasterizerState->Release();
		ShadowRasterizerState = nullptr;
	}

	if (ShadowViewProjConstantBuffer)
	{
		ShadowViewProjConstantBuffer->Release();
		ShadowViewProjConstantBuffer = nullptr;
	}

	if (PointLightShadowParamsBuffer)
	{
		PointLightShadowParamsBuffer->Release();
		PointLightShadowParamsBuffer = nullptr;
	}

	ShadowAtlas.Release();

	SafeRelease(ShadowAtlasDirectionalLightTilePosStructuredBuffer);
	SafeRelease(ShadowAtlasSpotLightTilePosStructuredBuffer);
	SafeRelease(ShadowAtlasPointLightTilePosStructuredBuffer);
	
	SafeRelease(ShadowAtlasDirectionalLightTilePosStructuredSRV);
	SafeRelease(ShadowAtlasSpotLightTilePosStructuredSRV);
	SafeRelease(ShadowAtlasPointLightTilePosStructuredSRV);

	SafeRelease(ConstantCascadeData);
	// Shader?� InputLayout?� Renderer가 ?�유?��?�??�기???�제?��? ?�음
}

FShadowMapResource* FShadowMapPass::GetDirectionalShadowMap(UDirectionalLightComponent* Light)
{
	auto It = DirectionalShadowMaps.find(Light);
	return It != DirectionalShadowMaps.end() ? It->second : nullptr;
}

FShadowMapResource* FShadowMapPass::GetSpotShadowMap(USpotLightComponent* Light)
{
	auto It = SpotShadowMaps.find(Light);
	return It != SpotShadowMaps.end() ? It->second : nullptr;
}

// ID3D11RasterizerState* FShadowMapPass::GetOrCreateRasterizerState(UDirectionalLightComponent* Light)
// {
// 	// ?��? ?�성??state가 ?�으�??�사??
// 	auto It = DirectionalRasterizerStates.find(Light);
// 	if (It != DirectionalRasterizerStates.end())
// 		return It->second;
//
// 	// ?�로 ?�성
// 	const auto& Renderer = URenderer::GetInstance();
// 	D3D11_RASTERIZER_DESC RastDesc = {};
// 	ShadowRasterizerState->GetDesc(&RastDesc);
//
// 	// Light�?DepthBias ?�정
// 	// DepthBias: Shadow acne (?�기 그림???�티?�트) 방�?
// 	//   - 공식: FinalDepth = OriginalDepth + DepthBias*r + SlopeScaledDepthBias*MaxSlope
// 	//   - r: Depth buffer??최소 ?�현 ?�위 (format dependent)
// 	//   - MaxSlope: max(|dz/dx|, |dz/dy|) - ?�면??기울�?
// 	//   - 100000.0f: float ??integer 변???��???
// 	RastDesc.DepthBias = static_cast<INT>(Light->GetShadowBias() * 100000.0f);
// 	RastDesc.SlopeScaledDepthBias = Light->GetShadowSlopeBias();
//
// 	ID3D11RasterizerState* NewState = nullptr;
// 	Renderer.GetDevice()->CreateRasterizerState(&RastDesc, &NewState);
//
// 	// 캐시???�??
// 	DirectionalRasterizerStates[Light] = NewState;
//
// 	return NewState;
// }
//
// ID3D11RasterizerState* FShadowMapPass::GetOrCreateRasterizerState(USpotLightComponent* Light)
// {
// 	// ?��? ?�성??state가 ?�으�??�사??
// 	auto It = SpotRasterizerStates.find(Light);
// 	if (It != SpotRasterizerStates.end())
// 		return It->second;
//
// 	// ?�로 ?�성
// 	const auto& Renderer = URenderer::GetInstance();
// 	D3D11_RASTERIZER_DESC RastDesc = {};
// 	ShadowRasterizerState->GetDesc(&RastDesc);
//
// 	// Light�?DepthBias ?�정
// 	// DepthBias: Shadow acne (?�기 그림???�티?�트) 방�?
// 	//   - 공식: FinalDepth = OriginalDepth + DepthBias*r + SlopeScaledDepthBias*MaxSlope
// 	//   - r: Depth buffer??최소 ?�현 ?�위 (format dependent)
// 	//   - MaxSlope: max(|dz/dx|, |dz/dy|) - ?�면??기울�?
// 	//   - 100000.0f: float ??integer 변???��???
// 	RastDesc.DepthBias = static_cast<INT>(Light->GetShadowBias() * 100000.0f);
// 	RastDesc.SlopeScaledDepthBias = Light->GetShadowSlopeBias();
//
// 	ID3D11RasterizerState* NewState = nullptr;
// 	Renderer.GetDevice()->CreateRasterizerState(&RastDesc, &NewState);
//
// 	// 캐시???�??
// 	SpotRasterizerStates[Light] = NewState;
//
// 	return NewState;
// }
//
// ID3D11RasterizerState* FShadowMapPass::GetOrCreateRasterizerState(UPointLightComponent* Light)
// {
// 	// ?��? ?�성??state가 ?�으�??�사??
// 	auto It = PointRasterizerStates.find(Light);
// 	if (It != PointRasterizerStates.end())
// 		return It->second;
//
// 	// ?�로 ?�성
// 	const auto& Renderer = URenderer::GetInstance();
// 	D3D11_RASTERIZER_DESC RastDesc = {};
// 	ShadowRasterizerState->GetDesc(&RastDesc);
//
// 	// Light�?DepthBias ?�정
// 	// DepthBias: Shadow acne (?�기 그림???�티?�트) 방�?
// 	//   - 공식: FinalDepth = OriginalDepth + DepthBias*r + SlopeScaledDepthBias*MaxSlope
// 	//   - r: Depth buffer??최소 ?�현 ?�위 (format dependent)
// 	//   - MaxSlope: max(|dz/dx|, |dz/dy|) - ?�면??기울�?
// 	//   - 100000.0f: float ??integer 변???��???
// 	RastDesc.DepthBias = static_cast<INT>(Light->GetShadowBias() * 100000.0f);
// 	RastDesc.SlopeScaledDepthBias = Light->GetShadowSlopeBias();
//
// 	ID3D11RasterizerState* NewState = nullptr;
// 	Renderer.GetDevice()->CreateRasterizerState(&RastDesc, &NewState);
//
// 	// 캐시???�??
// 	PointRasterizerStates[Light] = NewState;
//
// 	return NewState;
// }

ID3D11RasterizerState* FShadowMapPass::GetOrCreateRasterizerState(
	float InShadowBias,
	float InShadowSlopBias
	)
{
	// Light�?DepthBias ?�정
	// DepthBias: Shadow acne (?�기 그림???�티?�트) 방�?
	//   - 공식: FinalDepth = OriginalDepth + DepthBias*r + SlopeScaledDepthBias*MaxSlope
	//   - r: Depth buffer??최소 ?�현 ?�위 (format dependent)
	//   - MaxSlope: max(|dz/dx|, |dz/dy|) - ?�면??기울�?
	//   - 100000.0f: float ??integer 변???��???

	auto Quantize = [](float value, float step)
	{
		return roundf(value / step) * step;
	};

	FLOAT QuantizedShadowBias = Quantize(InShadowBias, 0.0001f);     // 0.0001 ?�위
	FLOAT QuantizedSlopeBias  = Quantize(InShadowSlopBias, 0.01f);   // 0.01 ?�위
	INT DepthBias = static_cast<INT>(QuantizedShadowBias * 100000.0f);
	FLOAT SlopeScaledDepthBias = QuantizedSlopeBias;

	FString RasterizeMapKey = to_string(QuantizedShadowBias) + to_string(QuantizedSlopeBias);
	
	// ?��? ?�성??state가 ?�으�??�사??
	auto It = LightRasterizerStates.find(RasterizeMapKey);
	if (It != LightRasterizerStates.end())
		return It->second;

	// ?�로 ?�성
	const auto& Renderer = URenderer::GetInstance();
	D3D11_RASTERIZER_DESC RastDesc = {};
	ShadowRasterizerState->GetDesc(&RastDesc);
	
	RastDesc.DepthBias = DepthBias;
	RastDesc.SlopeScaledDepthBias = SlopeScaledDepthBias;

	ID3D11RasterizerState* NewState = nullptr;
	Renderer.GetDevice()->CreateRasterizerState(&RastDesc, &NewState);

	// 캐시???�??
	LightRasterizerStates[RasterizeMapKey] = NewState;

	return NewState;
}
