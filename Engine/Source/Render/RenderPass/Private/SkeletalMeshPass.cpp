#include "pch.h"

#include "Component/Mesh/Public/SkeletalMeshComponent.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Render/RenderPass/Public/ShadowMapPass.h"
#include "Render/RenderPass/Public/SkeletalMeshPass.h"
#include "Render/Renderer/Public/DeviceResources.h"
#include "Render/Renderer/Public/Pipeline.h"
#include "Render/Renderer/Public/RenderResourceFactory.h"
#include "Render/Renderer/Public/Renderer.h"

struct FShadowMapResource;

FSkeletalMeshPass::FSkeletalMeshPass(UPipeline* InPipeline,
                                     ID3D11Buffer* InConstantBufferViewProj,
                                     ID3D11Buffer* InConstantBufferModel)
		: FRenderPass(InPipeline, InConstantBufferViewProj, InConstantBufferModel)
{
}

void FSkeletalMeshPass::SetRenderTargets(class UDeviceResources* DeviceResources)
{
	ID3D11RenderTargetView* RTVs[] = { DeviceResources->GetDestinationRTV(), DeviceResources->GetNormalBufferRTV() };
	ID3D11DepthStencilView* DSV = DeviceResources->GetDepthBufferDSV();
	Pipeline->SetRenderTargets(2, RTVs, DSV);
}

void FSkeletalMeshPass::Execute(FRenderingContext& Context)
{
	const auto& Renderer = URenderer::GetInstance();
	GPU_EVENT(Renderer.GetDeviceContext(), "SkeletalMeshPass");
	FRenderState RenderState = UStaticMeshComponent::GetClassDefaultRenderState();
	if (Context.ViewMode == EViewModeIndex::VMI_Wireframe)
	{
		/** @todo VS, PS 설정이 없으면 이전 설정에 영향을 받는지 확인하기. Get_Shader 함수가 VMI_Wireframe도 처리하게 확장해야 할 듯. */
		RenderState.CullMode = ECullMode::None;
		RenderState.FillMode = EFillMode::WireFrame;
	}
	else
	{
		VertexShader = Renderer.GetVertexShader(Context.ViewMode);
		PixelShader = Renderer.GetPixelShader(Context.ViewMode);
	}

	ID3D11RasterizerState* RasterizerState = FRenderResourceFactory::GetRasterizerState(RenderState);
	FPipelineInfo PipelineInfo = {
		InputLayout,
		VertexShader,
		RasterizerState,
		DepthStencilState,
		PixelShader,
		nullptr,
		D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
	};
	Pipeline->UpdatePipeline(PipelineInfo);

	Pipeline->SetSamplerState(0, EShaderType::PS, Renderer.GetDefaultSampler());

	Pipeline->SetSamplerState(1, EShaderType::PS, Renderer.GetShadowComparisonSampler());

	Pipeline->SetSamplerState(2, EShaderType::PS, Renderer.GetVarianceShadowSampler());

	Pipeline->SetSamplerState(3, EShaderType::PS, Renderer.GetPointShadowSampler());

	FShadowMapPass* ShadowPass = Renderer.GetShadowMapPass();
	if (ShadowPass)
	{
		FShadowMapResource* ShadowAtlas = ShadowPass->GetShadowAtlas();
		if (ShadowAtlas && ShadowAtlas->IsValid())
		{
			Pipeline->SetShaderResourceView(10, EShaderType::PS, ShadowAtlas->ShadowSRV.Get());
			Pipeline->SetShaderResourceView(11, EShaderType::PS, ShadowAtlas->VarianceShadowSRV.Get());
		}
	}

	Pipeline->SetConstantBuffer(0, EShaderType::VS, ConstantBufferModel);
	Pipeline->SetConstantBuffer(1, EShaderType::VS | EShaderType::PS, ConstantBufferCamera);

	if (!(Context.ShowFlags & EEngineShowFlags::SF_StaticMesh)) { return; }
	TArray<UStaticMeshComponent*>& MeshComponents = Context.StaticMeshes;
	sort(MeshComponents.begin(), MeshComponents.end(),
		[](UStaticMeshComponent* A, UStaticMeshComponent* B) {
			int32 MeshA = A->GetStaticMesh() ? A->GetStaticMesh()->GetAssetPathFileName().GetComparisonIndex() : 0;
			int32 MeshB = B->GetStaticMesh() ? B->GetStaticMesh()->GetAssetPathFileName().GetComparisonIndex() : 0;
			return MeshA < MeshB;
		});

	USkeletalMesh* CurrentMeshAsset = nullptr;
	UMaterial* CurrentMaterial = nullptr;

	for (USkeletalMeshComponent* MeshComp : Context.SkeletalMeshes)
	{
		if (!MeshComp->IsVisible()) { continue; }
		if (!MeshComp->GetSkeletalMeshAsset()) { continue; }
		USkeletalMesh* MeshAsset = MeshComp->GetSkeletalMeshAsset();

		if (CurrentMeshAsset != MeshAsset)
		{
			Pipeline->SetVertexBuffer(MeshComp->GetVertexBuffer(), sizeof(FSkeletalVertex));
			Pipeline->SetIndexBuffer(MeshComp->GetIndexBuffer(), 0);
			CurrentMeshAsset = MeshAsset;
		}

		FRenderResourceFactory::UpdateConstantBufferData(ConstantBufferModel, MeshComp->GetWorldTransformMatrix());
		Pipeline->SetConstantBuffer(0, EShaderType::VS, ConstantBufferModel);

		FSkeletalMeshRenderData* SkeletalMeshRenderData = MeshAsset->GetSkeletalMeshRenderData();
		Pipeline->DrawIndexed(static_cast<uint32>(SkeletalMeshRenderData->StaticMesh.Indices.Num()), 0, 0);
		/** @todo 머티리얼 도입 */
	}
}

void FSkeletalMeshPass::Release()
{
}

