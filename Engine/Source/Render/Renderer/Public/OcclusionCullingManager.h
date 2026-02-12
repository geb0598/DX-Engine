#pragma once
#include "Component/Public/PrimitiveComponent.h"

struct FAABB;
class UCamera;
class UPrimitiveComponent;

class FOcclusionCullingManager
{
public:
	static FOcclusionCullingManager& GetInstance()
	{
		static FOcclusionCullingManager OcclusionCuller;
		return OcclusionCuller;
	}

	FOcclusionCullingManager(const FOcclusionCullingManager&) = delete;
	FOcclusionCullingManager(FOcclusionCullingManager&&)	  = delete;

	FOcclusionCullingManager& operator=(const FOcclusionCullingManager&) = delete;
	FOcclusionCullingManager& operator=(FOcclusionCullingManager&&)		 = delete;

	void Initialize(ID3D11Device* InDevice, ID3D11DeviceContext* InDeviceContext, uint32 InWidth = 0, uint32 InHeight = 0);
	void Resize(uint32 InWidth, uint32 InHeight);
	void Release();
	void Clear();

	bool IsInitialized() const { return bIsInitialized; }

	void UpdateOcclusionCulling(const TArray<UPrimitiveComponent*>& InPrimitiveComponents,  const FMatrix& InViewMatrix, const FMatrix& InProjectionMatrix);

	bool QueryOcclusionCulled(const UPrimitiveComponent* PrimitiveComponent) const
	{
		auto Iter = PrimitiveVisibilities.find(PrimitiveComponent);
		if (Iter != PrimitiveVisibilities.end())
		{
			return !Iter->second;
		}

		return false;
	}

	ID3D11DepthStencilView* GetDepthStencilView() const { return DepthStencilView; }
	ID3D11DepthStencilState* GetDepthStencilState() const { return DepthStencilState; }

private:
	static constexpr uint32 DOWN_SAMPLE_THREAD_GROUP_NUM = 16;
	static constexpr uint32 OCCLUSION_CULLING_THREAD_GROUP_NUM = 64;
	// POD
	struct FAABBProxy
	{
		FVector Min;
		FVector Max;
	};

	struct FOcclusionCullingConstantsData
	{
		uint32 NumAABBs;
		uint32 MipLevels;
		FVector2 ScreenSize;
	};

	static uint32 CalculateMipLevels(uint32 InWidth, uint32 InHeight)
	{
		return static_cast<uint32>(floor(log2(max(InWidth, InHeight)))) + 1;
	}

	FOcclusionCullingManager()									= default;
	~FOcclusionCullingManager();

	void CreateResources();
	void CreateShader();
	void CreateOcclusionCullingBuffer(uint32 NumAABBs);

	void ReleaseResources();
	void ReleaseShader();
	void ReleaseOcclusionCullingBuffer();

	void BuildNDCAABB(const UPrimitiveComponent* InPrimitiveComponent, const FMatrix& InViewMatrix, const FMatrix& InProjectionMatrix);
	void BuildNDCAABBs(const TArray<UPrimitiveComponent*>& InPrimitiveComponents, const FMatrix& InViewMatrix, const FMatrix& InProjectionMatrix);

	void GenerateHiZMipMap();
	void ExecuteOcclusionCulling();
	void FetchOcclusionCulling();

	ID3D11Device* Device											= nullptr;
	ID3D11DeviceContext* DeviceContext								= nullptr;

	ID3D11Texture2D* DepthTexture									= nullptr;
	ID3D11DepthStencilView* DepthStencilView						= nullptr;
	ID3D11ShaderResourceView* DepthShaderResourceView				= nullptr;
	ID3D11DepthStencilState* DepthStencilState						= nullptr;

	ID3D11Texture2D* HiZTexture										= nullptr;
	ID3D11ShaderResourceView* HiZShaderResourceView					= nullptr;
	TArray<ID3D11ShaderResourceView*> HiZShaderResourceViews;
	TArray<ID3D11UnorderedAccessView*> HiZUnorderedAccessViews;

	ID3D11ComputeShader* HiZDepthCopyShader							= nullptr;
	ID3D11ComputeShader* HiZDownSampleShader						= nullptr;
	ID3D11ComputeShader* HiZOcclusionCullingShader					= nullptr;

	ID3D11Buffer* NDCAABBsBuffer										= nullptr;
	ID3D11ShaderResourceView* NDCAABBsShaderResourceView				= nullptr;

	ID3D11Buffer* VisibilitiesBuffer								= nullptr;
	ID3D11UnorderedAccessView* VisibilitiesUnorderedAccessView		= nullptr;

	ID3D11Buffer* VisibilitiesReadbackBuffer						= nullptr;

	ID3D11Buffer* OcclusionCullingConstantBuffer					= nullptr;

	ID3D11SamplerState* HiZSamplerState								= nullptr;

	TArray<FAABBProxy> NDCAABBs;
	TArray<const UPrimitiveComponent*> PrimitiveComponents;

	TMap<const UPrimitiveComponent*, bool> PrimitiveVisibilities;

	uint32 Width	= 0;
	uint32 Height	= 0;

	uint32 CachedNumAABBs = 0;

	bool bIsInitialized = false;
};
