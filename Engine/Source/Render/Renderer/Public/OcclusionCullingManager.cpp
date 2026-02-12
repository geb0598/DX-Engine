#include "pch.h"
#include "OcclusionCullingManager.h"

#include "Component/Public/PrimitiveComponent.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "OcclusionRenderer.h"
#include "Physics/Public/AABB.h"
#include "Renderer.h"

void FOcclusionCullingManager::Initialize(ID3D11Device* InDevice, ID3D11DeviceContext* InDeviceContext, uint32 InWidth, uint32 InHeight)
{
	if (IsInitialized())
	{
		return;
	}

	Device = InDevice;
	DeviceContext = InDeviceContext;

	D3D11_DEPTH_STENCIL_DESC DepthStencilDesc = {};
	DepthStencilDesc.DepthEnable = true;
	DepthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	DepthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
	DepthStencilDesc.StencilEnable = false;

	HRESULT HResult = Device->CreateDepthStencilState(&DepthStencilDesc, &DepthStencilState);
	if (FAILED(HResult))
	{
		return;
	}

	CreateShader();

	Resize(InWidth, InHeight);

	bIsInitialized = true;
}

void FOcclusionCullingManager::Resize(uint32 InWidth, uint32 InHeight)
{
	if (!Device)
	{
		return;
	}

	if (Width == InWidth && Height == InHeight)
	{
		return;
	}

	Width = InWidth;
	Height = InHeight;

	ReleaseResources();

	CreateResources();
}

void FOcclusionCullingManager::Release()
{
	ReleaseResources();

	ReleaseShader();

	ReleaseOcclusionCullingBuffer();

	if (DepthStencilState)
	{
		DepthStencilState->Release();
		DepthStencilState = nullptr;
	}

	Device = nullptr;
	DeviceContext = nullptr;
	bIsInitialized = false;
}

void FOcclusionCullingManager::Clear()
{
	PrimitiveVisibilities.clear();
}

void FOcclusionCullingManager::UpdateOcclusionCulling(const TArray<UPrimitiveComponent*>& InPrimitiveComponents,  const FMatrix& InViewMatrix, const FMatrix& InProjectionMatrix)
{
	if (!IsInitialized())
	{
		return;
	}

	Clear();

	BuildNDCAABBs(InPrimitiveComponents, InViewMatrix, InProjectionMatrix);

	if (NDCAABBs.empty())
	{
		return;
	}

	GenerateHiZMipMap();

	ExecuteOcclusionCulling();

	FetchOcclusionCulling();
}

FOcclusionCullingManager::~FOcclusionCullingManager()
{
	Release();
}

void FOcclusionCullingManager::CreateResources()
{
	if (Width == 0 || Height == 0)
	{
		return;
	}

	{
		D3D11_TEXTURE2D_DESC DepthTextureDesc = {};
		DepthTextureDesc.Width = Width;
		DepthTextureDesc.Height = Height;
		DepthTextureDesc.MipLevels = 1;
		DepthTextureDesc.ArraySize = 1;
		DepthTextureDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
		DepthTextureDesc.SampleDesc.Count = 1;
		DepthTextureDesc.Usage = D3D11_USAGE_DEFAULT;
		DepthTextureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

		if (FAILED(Device->CreateTexture2D(&DepthTextureDesc, nullptr, &DepthTexture)))
		{
			return;
		}

		D3D11_DEPTH_STENCIL_VIEW_DESC DepthStencilViewDesc = {};
		DepthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		DepthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;

		if (FAILED(Device->CreateDepthStencilView(DepthTexture, &DepthStencilViewDesc, &DepthStencilView)))
		{
			return;
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC DepthShaderResourceViewDesc = {};
		DepthShaderResourceViewDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		DepthShaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		DepthShaderResourceViewDesc.Texture2D.MipLevels = 1;
		if (FAILED(Device->CreateShaderResourceView(DepthTexture, &DepthShaderResourceViewDesc, &DepthShaderResourceView)))
		{
			return;
		}
	}

	{
		uint32 MipLevels = CalculateMipLevels(Width, Height);

		D3D11_TEXTURE2D_DESC HiZTextureDesc = {};
		HiZTextureDesc.Width = Width;
		HiZTextureDesc.Height = Height;
		HiZTextureDesc.MipLevels = MipLevels;
		HiZTextureDesc.ArraySize = 1;
		HiZTextureDesc.Format = DXGI_FORMAT_R32_FLOAT;
		HiZTextureDesc.SampleDesc.Count = 1;
		HiZTextureDesc.Usage = D3D11_USAGE_DEFAULT;
		HiZTextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;

		if (FAILED(Device->CreateTexture2D(&HiZTextureDesc, nullptr, &HiZTexture)))
		{
			return;
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC HiZShaderResourceViewDesc = {};
		HiZShaderResourceViewDesc.Format = DXGI_FORMAT_R32_FLOAT;
		HiZShaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		HiZShaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
		HiZShaderResourceViewDesc.Texture2D.MipLevels = MipLevels;

		if (FAILED(Device->CreateShaderResourceView(HiZTexture, &HiZShaderResourceViewDesc, &HiZShaderResourceView)))
		{
			return;
		}

		HiZShaderResourceViews.resize(MipLevels);
		HiZUnorderedAccessViews.resize(MipLevels);

		for (uint32 i = 0; i < MipLevels; ++i)
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC ShaderResourceViewDesc = {};
			ShaderResourceViewDesc.Format = DXGI_FORMAT_R32_FLOAT;
			ShaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			ShaderResourceViewDesc.Texture2D.MostDetailedMip = i;
			ShaderResourceViewDesc.Texture2D.MipLevels = 1;

			if (FAILED(Device->CreateShaderResourceView(HiZTexture, &ShaderResourceViewDesc, &HiZShaderResourceViews[i])))
			{
				return;
			}

			D3D11_UNORDERED_ACCESS_VIEW_DESC UnorderedAccessViewDesc = {};
			UnorderedAccessViewDesc.Format = DXGI_FORMAT_R32_FLOAT;
			UnorderedAccessViewDesc.Format = DXGI_FORMAT_R32_FLOAT;
			UnorderedAccessViewDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
			UnorderedAccessViewDesc.Texture2D.MipSlice = i;

			if (FAILED(Device->CreateUnorderedAccessView(HiZTexture, &UnorderedAccessViewDesc, &HiZUnorderedAccessViews[i])))
			{
				return;
			}
		}
	}
}

void FOcclusionCullingManager::CreateShader()
{
	ReleaseShader();

	URenderer& Renderer = URenderer::GetInstance();

	Renderer.CreateComputeShader(L"Asset/Shader/HiZDepthCopyCS.hlsl", &HiZDepthCopyShader);
	Renderer.CreateComputeShader(L"Asset/Shader/HiZDownSampleCS.hlsl", &HiZDownSampleShader);
	Renderer.CreateComputeShader(L"Asset/Shader/HiZOcclusionCullingCS.hlsl", &HiZOcclusionCullingShader);

	D3D11_SAMPLER_DESC HiZSamplerDesc = {};
	HiZSamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	HiZSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	HiZSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	HiZSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	HiZSamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	HiZSamplerDesc.MinLOD = 0.0f;
	HiZSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	if (FAILED(Device->CreateSamplerState(&HiZSamplerDesc, &HiZSamplerState)))
	{
		return;
	}

	D3D11_BUFFER_DESC OcclusionCullingConstantBufferDesc = {};
	OcclusionCullingConstantBufferDesc.ByteWidth = sizeof(FOcclusionCullingConstantsData);
	OcclusionCullingConstantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	OcclusionCullingConstantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	OcclusionCullingConstantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	if (FAILED(Device->CreateBuffer(&OcclusionCullingConstantBufferDesc, nullptr, &OcclusionCullingConstantBuffer)))
	{
		return;
	}
}

void FOcclusionCullingManager::CreateOcclusionCullingBuffer(uint32 NumAABBs)
{
	if (CachedNumAABBs >= NumAABBs)
	{
		return;
	}

	ReleaseOcclusionCullingBuffer();

	CachedNumAABBs = NumAABBs;

	{
		D3D11_BUFFER_DESC AABBBufferDesc = {};
		AABBBufferDesc.ByteWidth = sizeof(FAABBProxy) * CachedNumAABBs;
		AABBBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		AABBBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		AABBBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		AABBBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		AABBBufferDesc.StructureByteStride = sizeof(FAABBProxy);

		if (FAILED(Device->CreateBuffer(&AABBBufferDesc, nullptr, &NDCAABBsBuffer)))
		{
			return;
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC AABBShaderResourceViewDesc = {};
		AABBShaderResourceViewDesc.Format = DXGI_FORMAT_UNKNOWN;
		AABBShaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		AABBShaderResourceViewDesc.Buffer.FirstElement = 0;
		AABBShaderResourceViewDesc.Buffer.NumElements = CachedNumAABBs;

		if (FAILED(Device->CreateShaderResourceView(NDCAABBsBuffer, &AABBShaderResourceViewDesc, &NDCAABBsShaderResourceView)))
		{
			return;
		}
	}

	{
		D3D11_BUFFER_DESC VisibilitiesBufferDesc = {};
		VisibilitiesBufferDesc.ByteWidth = sizeof(uint32) * CachedNumAABBs;
		VisibilitiesBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		VisibilitiesBufferDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
		VisibilitiesBufferDesc.CPUAccessFlags = 0;
		VisibilitiesBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		VisibilitiesBufferDesc.StructureByteStride = sizeof(uint32);

		if (FAILED(Device->CreateBuffer(&VisibilitiesBufferDesc, nullptr, &VisibilitiesBuffer)))
		{
			return;
		}

		D3D11_UNORDERED_ACCESS_VIEW_DESC VisibilitiesUnorderedAccessViewDesc = {};
		VisibilitiesUnorderedAccessViewDesc.Format = DXGI_FORMAT_UNKNOWN;
		VisibilitiesUnorderedAccessViewDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		VisibilitiesUnorderedAccessViewDesc.Buffer.FirstElement = 0;
		VisibilitiesUnorderedAccessViewDesc.Buffer.NumElements = CachedNumAABBs;

		if (Device->CreateUnorderedAccessView(VisibilitiesBuffer, &VisibilitiesUnorderedAccessViewDesc, &VisibilitiesUnorderedAccessView))
		{
			return;
		}

		D3D11_BUFFER_DESC VisibilitiesReadbackBufferDesc = {};
		VisibilitiesReadbackBufferDesc.ByteWidth = sizeof(uint32) * CachedNumAABBs;
		VisibilitiesReadbackBufferDesc.Usage = D3D11_USAGE_STAGING;
		VisibilitiesReadbackBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		VisibilitiesReadbackBufferDesc.MiscFlags = 0;

		if (Device->CreateBuffer(&VisibilitiesReadbackBufferDesc, nullptr, &VisibilitiesReadbackBuffer))
		{
			return;
		}
	}
}

void FOcclusionCullingManager::ReleaseResources()
{
	if (DepthTexture)
	{
		DepthTexture->Release();
		DepthTexture = nullptr;
	}

	if (DepthStencilView)
	{
		DepthStencilView->Release();
		DepthStencilView = nullptr;
	}

	if (DepthShaderResourceView)
	{
		DepthShaderResourceView->Release();
		DepthShaderResourceView = nullptr;
	}

	if (HiZTexture)
	{
		HiZTexture->Release();
		HiZTexture = nullptr;
	}

	if (HiZShaderResourceView)
	{
		HiZShaderResourceView->Release();
		HiZShaderResourceView = nullptr;
	}

	for (ID3D11ShaderResourceView* ShaderResourceView : HiZShaderResourceViews)
	{
		if (ShaderResourceView)
		{
			ShaderResourceView->Release();
			ShaderResourceView = nullptr;
		}
	}

	HiZShaderResourceViews.clear();

	for (ID3D11UnorderedAccessView* UnorderedAccessView : HiZUnorderedAccessViews)
	{
		if (UnorderedAccessView)
		{
			UnorderedAccessView->Release();
			UnorderedAccessView = nullptr;
		}
	}

	HiZUnorderedAccessViews.clear();
}

void FOcclusionCullingManager::ReleaseShader()
{
	if (HiZDepthCopyShader)
	{
		HiZDepthCopyShader->Release();
		HiZDepthCopyShader = nullptr;
	}

	if (HiZDownSampleShader)
	{
		HiZDownSampleShader->Release();
		HiZDownSampleShader = nullptr;
	}

	if (HiZOcclusionCullingShader)
	{
		HiZOcclusionCullingShader->Release();
		HiZOcclusionCullingShader = nullptr;
	}

	if (HiZSamplerState)
	{
		HiZSamplerState->Release();
		HiZSamplerState = nullptr;
	}
}

void FOcclusionCullingManager::ReleaseOcclusionCullingBuffer()
{
	if (NDCAABBsBuffer)
	{
		NDCAABBsBuffer->Release();
		NDCAABBsBuffer = nullptr;
	}

	if (NDCAABBsShaderResourceView)
	{
		NDCAABBsShaderResourceView->Release();
		NDCAABBsShaderResourceView = nullptr;
	}

	if (VisibilitiesBuffer)
	{
		VisibilitiesBuffer->Release();
		VisibilitiesBuffer = nullptr;
	}

	if (VisibilitiesUnorderedAccessView)
	{
		VisibilitiesUnorderedAccessView->Release();
		VisibilitiesUnorderedAccessView = nullptr;
	}

	if (VisibilitiesReadbackBuffer)
	{
		VisibilitiesReadbackBuffer->Release();
		VisibilitiesReadbackBuffer = nullptr;
	}
}

void FOcclusionCullingManager::BuildNDCAABB(const UPrimitiveComponent* InPrimitiveComponent, const FMatrix& InViewMatrix, const FMatrix& InProjectionMatrix)
{
	if (!InPrimitiveComponent)
	{
		return;
	}

	if (!InPrimitiveComponent->IsVisible())
	{
		PrimitiveVisibilities[InPrimitiveComponent] = false;
		return;
	}

	FVector WorldMin;
	FVector WorldMax;
	if (!InPrimitiveComponent->GetWorldAABB(WorldMin, WorldMax))
	{
		PrimitiveVisibilities[InPrimitiveComponent] = false;
		return;
	}

	FVector4 Corners[8] = {
		{ WorldMin.X, WorldMin.Y, WorldMin.Z, 1.0f },
		{ WorldMax.X, WorldMin.Y, WorldMin.Z, 1.0f },
		{ WorldMin.X, WorldMax.Y, WorldMin.Z, 1.0f },
		{ WorldMin.X, WorldMin.Y, WorldMax.Z, 1.0f },
		{ WorldMax.X, WorldMax.Y, WorldMin.Z, 1.0f },
		{ WorldMax.X, WorldMin.Y, WorldMax.Z, 1.0f },
		{ WorldMin.X, WorldMax.Y, WorldMax.Z, 1.0f },
		{ WorldMax.X, WorldMax.Y, WorldMax.Z, 1.0f }
	};

	FMatrix ViewProjMatrix = InViewMatrix * InProjectionMatrix;

	FVector4 NDCMin(FLT_MAX, FLT_MAX, FLT_MAX, 1.0f);
	FVector4 NDCMax(-FLT_MAX, -FLT_MAX, -FLT_MAX, 1.0f);
	for (const FVector4& Corner : Corners)
	{
		FVector4 ScreenPos = FMatrix::VectorMultiply(Corner, ViewProjMatrix);

		if (ScreenPos.W <= 0.0f)
		{
			continue;
		}

		FVector4 NDCPos(ScreenPos.X / ScreenPos.W, ScreenPos.Y / ScreenPos.W, ScreenPos.Z / ScreenPos.W, 1.0f);

		NDCPos.X = std::clamp(NDCPos.X, -1.0f, 1.0f);
		NDCPos.Y = std::clamp(NDCPos.Y, -1.0f, 1.0f);
		NDCPos.Z = std::clamp(NDCPos.Z, 0.0f, 1.0f);

		NDCMin.X = std::min(NDCMin.X, NDCPos.X);
		NDCMin.Y = std::min(NDCMin.Y, NDCPos.Y);
		NDCMin.Z = std::min(NDCMin.Z, NDCPos.Z);

		NDCMax.X = std::max(NDCMax.X, NDCPos.X);
		NDCMax.Y = std::max(NDCMax.Y, NDCPos.Y);
		NDCMax.Z = std::max(NDCMax.Z, NDCPos.Z);
	}

	if (NDCMin.X >= FLT_MAX || NDCMin.Y >= FLT_MAX || NDCMax.Z >= FLT_MAX ||
		NDCMax.X <= -FLT_MAX || NDCMax.Y <= -FLT_MAX || NDCMax.Z <= -FLT_MAX)
	{
		PrimitiveVisibilities[InPrimitiveComponent] = false;
		return;
	}

	FAABBProxy NDCAABB;
	NDCAABB.Min = NDCMin;
	NDCAABB.Max = NDCMax;

	NDCAABBs.push_back(NDCAABB);
	PrimitiveComponents.push_back(InPrimitiveComponent);
}

void FOcclusionCullingManager::BuildNDCAABBs(const TArray<UPrimitiveComponent*>& InPrimitiveComponents, const FMatrix& InViewMatrix, const FMatrix& InProjectionMatrix)
{
	NDCAABBs.clear();
	NDCAABBs.reserve(InPrimitiveComponents.size());

	PrimitiveComponents.clear();
	PrimitiveComponents.reserve(InPrimitiveComponents.size());

	for (const auto PrimitiveComponent : InPrimitiveComponents)
	{
		if (PrimitiveComponent)
		{
			BuildNDCAABB(PrimitiveComponent, InViewMatrix, InProjectionMatrix);
		}
	}
}

void FOcclusionCullingManager::GenerateHiZMipMap()
{
	if (!HiZTexture || !DepthShaderResourceView)
	{
		return;
	}

	DeviceContext->CSSetShader(HiZDepthCopyShader, nullptr, 0);
	DeviceContext->CSSetShaderResources(0, 1, &DepthShaderResourceView);
	DeviceContext->CSSetUnorderedAccessViews(0, 1, &HiZUnorderedAccessViews[0], nullptr);
	DeviceContext->Dispatch((Width + DOWN_SAMPLE_THREAD_GROUP_NUM - 1) / DOWN_SAMPLE_THREAD_GROUP_NUM, (Height + DOWN_SAMPLE_THREAD_GROUP_NUM - 1) / DOWN_SAMPLE_THREAD_GROUP_NUM, 1);

	ID3D11ShaderResourceView* NullShaderResourceView = nullptr;
	ID3D11UnorderedAccessView* NullUnorderedAccessView = nullptr;
	DeviceContext->CSSetShaderResources(0, 1, &NullShaderResourceView);
	DeviceContext->CSSetUnorderedAccessViews(0, 1, &NullUnorderedAccessView, nullptr);

	DeviceContext->CSSetShader(HiZDownSampleShader, nullptr, 0);
	DeviceContext->CSSetSamplers(0, 1, &HiZSamplerState);

	uint32 MipLevels = CalculateMipLevels(Width, Height);

	for (uint32 MipLevel = 1; MipLevel < MipLevels; ++ MipLevel)
	{
		uint32 MipWidth = std::max(1u, Width >> MipLevel);
		uint32 MipHeight = std::max(1u, Height >> MipLevel);

		ID3D11ShaderResourceView* InputShaderResourceView = HiZShaderResourceViews[MipLevel - 1];
		DeviceContext->CSSetShaderResources(0, 1, &InputShaderResourceView);

		ID3D11UnorderedAccessView* OutputUnorderedAccessView = HiZUnorderedAccessViews[MipLevel];
		DeviceContext->CSSetUnorderedAccessViews(0, 1, &OutputUnorderedAccessView, nullptr);

		DeviceContext->Dispatch((MipWidth + DOWN_SAMPLE_THREAD_GROUP_NUM - 1) / DOWN_SAMPLE_THREAD_GROUP_NUM, (MipHeight + DOWN_SAMPLE_THREAD_GROUP_NUM - 1) / DOWN_SAMPLE_THREAD_GROUP_NUM, 1);

		DeviceContext->CSSetShaderResources(0, 1, &NullShaderResourceView);
		DeviceContext->CSSetUnorderedAccessViews(0, 1, &NullUnorderedAccessView, nullptr);
	}

	DeviceContext->CSSetShader(nullptr, nullptr, 0);
}

void FOcclusionCullingManager::ExecuteOcclusionCulling()
{
	uint32 NumAABBs = NDCAABBs.size();

	CreateOcclusionCullingBuffer(NumAABBs);

	if (NumAABBs == 0)
	{
		return;
	}

	D3D11_MAPPED_SUBRESOURCE MappedSubresource;
	if (SUCCEEDED(DeviceContext->Map(NDCAABBsBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubresource)))
	{
		memcpy(MappedSubresource.pData, NDCAABBs.data(), sizeof(FAABBProxy) * NumAABBs);
		DeviceContext->Unmap(NDCAABBsBuffer, 0);
	}

	FOcclusionCullingConstantsData OcclusionCullingConstantsData;
	OcclusionCullingConstantsData.NumAABBs = NumAABBs;
	OcclusionCullingConstantsData.MipLevels = CalculateMipLevels(Width, Height);
	OcclusionCullingConstantsData.ScreenSize = FVector2(Width, Height);

	if (SUCCEEDED(DeviceContext->Map(OcclusionCullingConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubresource)))
	{
		memcpy(MappedSubresource.pData, &OcclusionCullingConstantsData, sizeof(FOcclusionCullingConstantsData));
		DeviceContext->Unmap(OcclusionCullingConstantBuffer, 0);
	}

	DeviceContext->CSSetShader(HiZOcclusionCullingShader, nullptr, 0);

	ID3D11ShaderResourceView* HiZOcclusionCullingShaderResourceViews[] = { HiZShaderResourceView, NDCAABBsShaderResourceView };
	DeviceContext->CSSetShaderResources(0, 2, HiZOcclusionCullingShaderResourceViews);

	DeviceContext->CSSetUnorderedAccessViews(0, 1, &VisibilitiesUnorderedAccessView, nullptr);

	DeviceContext->CSSetConstantBuffers(0, 1, &OcclusionCullingConstantBuffer);
	DeviceContext->CSSetSamplers(0, 1, &HiZSamplerState);

	uint32 ThreadGroups = (NumAABBs + OCCLUSION_CULLING_THREAD_GROUP_NUM - 1) / OCCLUSION_CULLING_THREAD_GROUP_NUM;
	DeviceContext->Dispatch(ThreadGroups, 1, 1);

	ID3D11ShaderResourceView* NullShaderResourceViews[] = { nullptr, nullptr };
	ID3D11UnorderedAccessView* NullUnorderedAccessView = nullptr;
	DeviceContext->CSSetShaderResources(0, 2, NullShaderResourceViews);
	DeviceContext->CSSetUnorderedAccessViews(0, 1, &NullUnorderedAccessView, nullptr);
	DeviceContext->CSSetShader(nullptr, nullptr, 0);
}

void FOcclusionCullingManager::FetchOcclusionCulling()
{
	uint32 NumAABBs = NDCAABBs.size();

	if (NumAABBs == 0)
	{
		return;
	}

	DeviceContext->CopyResource(VisibilitiesReadbackBuffer, VisibilitiesBuffer);

	D3D11_MAPPED_SUBRESOURCE MappedSubresource;

	if (SUCCEEDED(DeviceContext->Map(VisibilitiesReadbackBuffer, 0, D3D11_MAP_READ, 0, &MappedSubresource)))
	{
		uint32* Results = static_cast<uint32*>(MappedSubresource.pData);

		uint32 CulledCount = 0;

		for (uint32 i = 0; i < NumAABBs; ++i)
		{
			const UPrimitiveComponent* PrimitiveComponent = PrimitiveComponents[i];

			bool bIsVisible = Results[i] != 0;
			PrimitiveVisibilities[PrimitiveComponent] = bIsVisible;

			if (!bIsVisible)
			{
				++CulledCount;
			}
		}

		DeviceContext->Unmap(VisibilitiesReadbackBuffer, 0);

		UE_LOG("[Occlusion Stats] - Input: %u, Culled: %u, Visible: %u", NumAABBs, CulledCount, NumAABBs - CulledCount);
	}
}
