#include "pch.h"
#include "Render/UI/Window/Public/SkeletalMeshViewerWindow.h"
#include "ImGui/imgui_internal.h"
#include "Render/UI/Viewport/Public/Viewport.h"
#include "Render/UI/Viewport/Public/ViewportClient.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Render/Renderer/Public/SceneView.h"
#include "Render/Renderer/Public/SceneViewFamily.h"
#include "Render/Renderer/Public/SceneRenderer.h"
#include "Render/Renderer/Public/Pipeline.h"
#include "Render/Renderer/Public/RenderResourceFactory.h"
#include "Level/Public/World.h"
#include "Level/Public/Level.h"
#include "Core/Public/NewObject.h"
#include "Global/Quaternion.h"
#include "Editor/Public/BatchLines.h"
#include "Render/UI/Widget/Public/SkeletalMeshViewerToolbarWidget.h"
#include "Component/Public/AmbientLightComponent.h"
#include "Component/Public/DirectionalLightComponent.h"

IMPLEMENT_CLASS(USkeletalMeshViewerWindow, UUIWindow)

/**
 * @brief SkeletalMeshViewerWindow 생성자
 * 독립적인 플로팅 윈도우로 설정
 */
USkeletalMeshViewerWindow::USkeletalMeshViewerWindow()
{
	FUIWindowConfig Config;
	Config.WindowTitle = "SkeletalMeshViewer";
	Config.DefaultSize = ImVec2(1400, 800);
	Config.DefaultPosition = ImVec2(100, 100);
	Config.DockDirection = EUIDockDirection::None; // 독립적인 플로팅 윈도우
	Config.Priority = 100;
	Config.bResizable = true;
	Config.bMovable = true;
	Config.bCollapsible = true;
	Config.InitialState = EUIWindowState::Hidden; // 기본적으로 숨김

	// 일반 윈도우 플래그
	Config.WindowFlags = ImGuiWindowFlags_None;

	SetConfig(Config);

	UE_LOG("SkeletalMeshViewerWindow: 생성자 호출됨");
}

/**
 * @brief 소멸자
 */
USkeletalMeshViewerWindow::~USkeletalMeshViewerWindow()
{
	Cleanup();
}

/**
 * @brief 초기화 함수 - 독립적인 뷰포트와 카메라 생성
 */
void USkeletalMeshViewerWindow::Initialize()
{
	if (bIsInitialized)
	{
		UE_LOG_WARNING("SkeletalMeshViewerWindow: 이미 초기화됨 - 중복 초기화 방지");
		return;
	}

	// Viewport 생성
	ViewerViewport = new FViewport();
	ViewerViewport->SetSize(ViewerWidth, ViewerHeight);
	ViewerViewport->SetInitialPosition(0, 0);

	// ViewportClient 생성
	ViewerViewportClient = new FViewportClient();
	ViewerViewportClient->SetOwningViewport(ViewerViewport);
	ViewerViewportClient->SetViewType(EViewType::Perspective);
	ViewerViewportClient->SetViewMode(EViewModeIndex::VMI_BlinnPhong);

	// 뷰어의 카메라는 ImGui 마우스 델타를 직접 사용하므로 Camera::UpdateInput() 비활성화
	if (ViewerViewportClient->GetCamera())
	{
		ViewerViewportClient->GetCamera()->SetInputEnabled(false);
	}

	ViewerViewport->SetViewportClient(ViewerViewportClient);

	// 렌더 타겟 생성
	CreateRenderTarget(ViewerWidth, ViewerHeight);

	// BatchLines 생성 및 Grid 초기화
	ViewerBatchLines = new UBatchLines();
	if (ViewerBatchLines)
	{
		ViewerBatchLines->UpdateUGridVertices(GridCellSize); // 그리드 생성
		ViewerBatchLines->UpdateVertexBuffer();
	}

	// 툴바 위젯 생성 및 초기화
	ToolbarWidget = new USkeletalMeshViewerToolbarWidget();
	if (ToolbarWidget)
	{
		ToolbarWidget->Initialize();
		ToolbarWidget->SetViewportClient(ViewerViewportClient);
		ToolbarWidget->SetOwningWindow(this);
	}

	// 뷰어 전용 라이트 컴포넌트 생성 (World에 추가하지 않음)
	ViewerAmbientLight = NewObject<UAmbientLightComponent>();
	if (ViewerAmbientLight)
	{
		ViewerAmbientLight->SetLightColor(FVector(0.3f, 0.3f, 0.3f)); // 부드러운 앰비언트 라이트
		ViewerAmbientLight->SetIntensity(1.0f);
	}

	ViewerDirectionalLight = NewObject<UDirectionalLightComponent>();
	if (ViewerDirectionalLight)
	{
		ViewerDirectionalLight->SetWorldLocation(FVector(0, 0, 100));
		ViewerDirectionalLight->SetWorldRotation(FVector(-45.0f, 45.0f, 0.0f)); // 대각선 위에서 비추는 라이트
		ViewerDirectionalLight->SetLightColor(FVector(1.0f, 1.0f, 1.0f)); // 흰색 라이트
		ViewerDirectionalLight->SetIntensity(0.8f);
	}

	bIsInitialized = true;
	bIsCleanedUp = false;

	UE_LOG("SkeletalMeshViewerWindow: 독립적인 뷰포트 및 카메라 초기화 완료 (라이트 포함)");
}

/**
 * @brief 정리 함수 - 뷰포트와 카메라 해제
 */
void USkeletalMeshViewerWindow::Cleanup()
{
	if (bIsCleanedUp)
	{
		UE_LOG_WARNING("SkeletalMeshViewerWindow: 이미 정리됨 - 중복 정리 방지");
		return;
	}

	// 렌더 타겟 해제
	ReleaseRenderTarget();

	// BatchLines 삭제
	if (ViewerBatchLines)
	{
		SafeDelete(ViewerBatchLines);
	}

	// 툴바 위젯 삭제
	if (ToolbarWidget)
	{
		delete ToolbarWidget;
		ToolbarWidget = nullptr;
	}

	// 뷰어 전용 라이트 삭제
	if (ViewerAmbientLight)
	{
		delete ViewerAmbientLight;
		ViewerAmbientLight = nullptr;
	}

	if (ViewerDirectionalLight)
	{
		delete ViewerDirectionalLight;
		ViewerDirectionalLight = nullptr;
	}

	// ViewportClient와 Viewport의 연결을 먼저 끊음
	if (ViewerViewportClient && ViewerViewport)
	{
		ViewerViewportClient->SetOwningViewport(nullptr);
		ViewerViewport->SetViewportClient(nullptr);
	}

	// ViewportClient 삭제
	if (ViewerViewportClient)
	{
		SafeDelete(ViewerViewportClient);
	}

	// 그 다음 Viewport 삭제
	if (ViewerViewport)
	{
		SafeDelete(ViewerViewport);
	}

	bIsCleanedUp = true;
	bIsInitialized = false;

	UE_LOG("SkeletalMeshViewerWindow: 정리 완료");
}

/**
 * @brief 렌더 타겟 생성
 */
void USkeletalMeshViewerWindow::CreateRenderTarget(uint32 Width, uint32 Height)
{
	// 기존 렌더 타겟 해제
	ReleaseRenderTarget();

	if (Width == 0 || Height == 0)
	{
		UE_LOG_WARNING("SkeletalMeshViewerWindow: 잘못된 렌더 타겟 크기 (%d x %d)", Width, Height);
		return;
	}

	URenderer& Renderer = URenderer::GetInstance();
	ID3D11Device* Device = Renderer.GetDevice();

	// 렌더 타겟 텍스처 생성
	D3D11_TEXTURE2D_DESC TextureDesc = {};
	TextureDesc.Width = Width;
	TextureDesc.Height = Height;
	TextureDesc.MipLevels = 1;
	TextureDesc.ArraySize = 1;
	TextureDesc.Format = DXGI_FORMAT_R8G8B8A8_TYPELESS;
	TextureDesc.SampleDesc.Count = 1;
	TextureDesc.SampleDesc.Quality = 0;
	TextureDesc.Usage = D3D11_USAGE_DEFAULT;
	TextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	TextureDesc.CPUAccessFlags = 0;
	TextureDesc.MiscFlags = 0;

	HRESULT hr = Device->CreateTexture2D(&TextureDesc, nullptr, &ViewerRenderTargetTexture);
	if (FAILED(hr))
	{
		UE_LOG_ERROR("SkeletalMeshViewerWindow: 렌더 타겟 텍스처 생성 실패");
		return;
	}

	// 렌더 타겟 뷰 생성
	D3D11_RENDER_TARGET_VIEW_DESC RTVDesc = {};
	RTVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	RTVDesc.Texture2D.MipSlice = 0;

	hr = Device->CreateRenderTargetView(ViewerRenderTargetTexture, &RTVDesc, &ViewerRenderTargetView);
	if (FAILED(hr))
	{
		UE_LOG_ERROR("SkeletalMeshViewerWindow: 렌더 타겟 뷰 생성 실패");
		ReleaseRenderTarget();
		return;
	}

	// 셰이더 리소스 뷰 생성
	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.Texture2D.MipLevels = 1;

	hr = Device->CreateShaderResourceView(ViewerRenderTargetTexture, &SRVDesc, &ViewerShaderResourceView);
	if (FAILED(hr))
	{
		UE_LOG_ERROR("SkeletalMeshViewerWindow: 셰이더 리소스 뷰 생성 실패");
		ReleaseRenderTarget();
		return;
	}

	// 깊이 스텐실 텍스처 생성
	D3D11_TEXTURE2D_DESC DepthDesc = {};
	DepthDesc.Width = Width;
	DepthDesc.Height = Height;
	DepthDesc.MipLevels = 1;
	DepthDesc.ArraySize = 1;
	DepthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	DepthDesc.SampleDesc.Count = 1;
	DepthDesc.SampleDesc.Quality = 0;
	DepthDesc.Usage = D3D11_USAGE_DEFAULT;
	DepthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	DepthDesc.CPUAccessFlags = 0;
	DepthDesc.MiscFlags = 0;

	hr = Device->CreateTexture2D(&DepthDesc, nullptr, &ViewerDepthStencilTexture);
	if (FAILED(hr))
	{
		UE_LOG_ERROR("SkeletalMeshViewerWindow: 깊이 스텐실 텍스처 생성 실패");
		ReleaseRenderTarget();
		return;
	}

	// 깊이 스텐실 뷰 생성
	D3D11_DEPTH_STENCIL_VIEW_DESC DSVDesc = {};
	DSVDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	DSVDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	DSVDesc.Texture2D.MipSlice = 0;

	hr = Device->CreateDepthStencilView(ViewerDepthStencilTexture, &DSVDesc, &ViewerDepthStencilView);
	if (FAILED(hr))
	{
		UE_LOG_ERROR("SkeletalMeshViewerWindow: 깊이 스텐실 뷰 생성 실패");
		ReleaseRenderTarget();
		return;
	}

	ViewerWidth = Width;
	ViewerHeight = Height;

	UE_LOG("SkeletalMeshViewerWindow: 렌더 타겟 생성 완료 (%d x %d)", Width, Height);
}

/**
 * @brief 렌더 타겟 해제
 */
void USkeletalMeshViewerWindow::ReleaseRenderTarget()
{
	SafeRelease(ViewerDepthStencilView);
	SafeRelease(ViewerDepthStencilTexture);
	SafeRelease(ViewerShaderResourceView);
	SafeRelease(ViewerRenderTargetView);
	SafeRelease(ViewerRenderTargetTexture);
}

/**
 * @brief 윈도우 렌더링 전 호출
 */
void USkeletalMeshViewerWindow::OnPreRenderWindow(float MenuBarOffset)
{
	// 부모 클래스의 PreRender 호출
	UUIWindow::OnPreRenderWindow(MenuBarOffset);
}

/**
 * @brief 윈도우 렌더링 후 호출 - 여기서 실제 레이아웃을 그림
 */
void USkeletalMeshViewerWindow::OnPostRenderWindow()
{
	RenderLayout();

	// 부모 클래스의 PostRender 호출
	UUIWindow::OnPostRenderWindow();
}

/**
 * @brief 3-패널 레이아웃 렌더링
 * ImGui Child 영역을 사용하여 화면을 3개로 분할하고 Splitter로 크기 조절 가능
 */
void USkeletalMeshViewerWindow::RenderLayout()
{
	const ImVec2 ContentRegion = ImGui::GetContentRegionAvail();
	const float TotalWidth = ContentRegion.x;
	const float PanelHeight = ContentRegion.y;

	// 중앙 패널은 남은 공간 사용
	const float CenterPanelWidthRatio = 1.0f - LeftPanelWidthRatio - RightPanelWidthRatio;

	const float LeftPanelWidth = TotalWidth * LeftPanelWidthRatio;
	const float CenterPanelWidth = TotalWidth * CenterPanelWidthRatio;
	const float RightPanelWidth = TotalWidth * RightPanelWidthRatio;


	// SkeletalMesh 유효성 검사 및 변수 할당
	USkeletalMesh* SkeletalMesh = nullptr;
	FReferenceSkeleton RefSkeleton;
	int32 NumBones = 0;
	bool bValid = CheckSkeletalValidity(SkeletalMesh, RefSkeleton, NumBones, false);
	/*if(!bValid)
	{
		return;
	}*/

	// TempBoneSpaceTransforms 초기화
	if (TempBoneSpaceTransforms.IsEmpty() && SkeletalMeshComponent)
	{
		const int32 NumBones = RefSkeleton.GetRawBoneNum();
		TempBoneSpaceTransforms.SetNum(NumBones);
		for (int32 i = 0; i < NumBones; ++i)
		{
			TempBoneSpaceTransforms[i] = SkeletalMeshComponent->GetBoneTransformLocal(i);
		}
	}

	// SkeletalMeshComponent에 임시 본 트랜스폼 적용
	if (SkeletalMeshComponent)
	{
		SkeletalMeshComponent->RefreshBoneTransformsCustom(TempBoneSpaceTransforms);
	}

	// === 좌측 패널: Skeleton Tree ===
	if (ImGui::BeginChild("SkeletonTreePanel", ImVec2(LeftPanelWidth - SplitterWidth * 0.5f, PanelHeight), true))
	{
		RenderSkeletonTreePanel(SkeletalMesh, RefSkeleton, NumBones);
	}
	ImGui::EndChild();

	ImGui::SameLine();

	// === 첫 번째 Splitter (좌측-중앙 사이) ===
	RenderVerticalSplitter("##LeftCenterSplitter", LeftPanelWidthRatio, MinPanelRatio, MaxPanelRatio);

	ImGui::SameLine();

	// === 중앙 패널: 3D Viewport ===
	if (ImGui::BeginChild("3DViewportPanel", ImVec2(CenterPanelWidth - SplitterWidth, PanelHeight), true, ImGuiWindowFlags_MenuBar))
	{
		Render3DViewportPanel();
	}
	ImGui::EndChild();

	ImGui::SameLine();

	// === 두 번째 Splitter (중앙-우측 사이) ===
	RenderVerticalSplitter("##CenterRightSplitter", RightPanelWidthRatio, MinPanelRatio, MaxPanelRatio, true);

	ImGui::SameLine();

	// === 우측 패널: Edit Tools ===
	if (ImGui::BeginChild("EditToolsPanel", ImVec2(RightPanelWidth - SplitterWidth * 0.5f, PanelHeight), true))
	{
		RenderEditToolsPanel(SkeletalMesh, RefSkeleton, NumBones);
	}
	ImGui::EndChild();
}

/**
 * @brief 좌측 패널: Skeleton Tree (Placeholder)
 */
void USkeletalMeshViewerWindow::RenderSkeletonTreePanel(const USkeletalMesh* InSkeletalMesh, const FReferenceSkeleton& InRefSkeleton, const int32 InNumBones)
{
	/*assert(InSkeletalMesh);
	assert(InNumBones > 0);*/
	bool bValid = CheckSkeletalValidity(const_cast<USkeletalMesh*>(InSkeletalMesh), const_cast<FReferenceSkeleton&>(InRefSkeleton), const_cast<int32&>(InNumBones), true);
	if(bValid == false)
	{
		return;
	}

	ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), "Skeleton Tree");
	ImGui::Separator();
	ImGui::Spacing();

	// 본 정보 표시
	ImGui::Text("Total Bones: %d", InNumBones);
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	// 검색 기능
	static char SearchBuffer[128] = "";
	ImGui::SetNextItemWidth(-1);
	ImGui::InputTextWithHint("##BoneSearch", "Search bones...", SearchBuffer, IM_ARRAYSIZE(SearchBuffer));
	ImGui::Spacing();

	// 스크롤 가능한 영역
	if (ImGui::BeginChild("BoneTreeScroll", ImVec2(0, 0), false))
	{
		const TArray<FMeshBoneInfo>& BoneInfoArray = InRefSkeleton.GetRawRefBoneInfo();
		const TArray<FTransform>& BonePoseArray = InRefSkeleton.GetRawRefBonePose();

		// 검색 필터링
		FString SearchStr = SearchBuffer;
		bool bHasSearchFilter = !SearchStr.empty();

		// 루트 본들을 찾아서 재귀적으로 렌더링
		for (int32 BoneIndex = 0; BoneIndex < InNumBones; ++BoneIndex)
		{
			const FMeshBoneInfo& BoneInfo = BoneInfoArray[BoneIndex];

			// 루트 본만 처리 (부모가 없는 본)
			if (BoneInfo.ParentIndex == INDEX_NONE)
			{
				RenderBoneTreeNode(BoneIndex, InRefSkeleton, SearchStr, bHasSearchFilter);
			}
		}
	}
	ImGui::EndChild();
}

void USkeletalMeshViewerWindow::RenderBoneTreeNode(int32 BoneIndex, const FReferenceSkeleton& RefSkeleton, const FString& SearchFilter, bool bHasSearchFilter)
{
	const TArray<FMeshBoneInfo>& BoneInfoArray = RefSkeleton.GetRawRefBoneInfo();
	//const TArray<FTransform>& BonePoseArray = RefSkeleton.GetRawRefBonePose();
	const FMeshBoneInfo& BoneInfo = BoneInfoArray[BoneIndex];

	FString BoneName = BoneInfo.Name.ToString();

	// 검색 필터 적용: 본인은 매칭되지 않지만 자식 본들 중에 매칭되는 것이 있는지 확인
	if (bHasSearchFilter && !BoneName.Contains(SearchFilter))
	{
		// 자식 본들 중에 매칭되는 것이 있는지 확인
		bool bHasMatchingChild = false;
		for (int32 ChildIndex = 0; ChildIndex < BoneInfoArray.Num(); ++ChildIndex)
		{
			if (BoneInfoArray[ChildIndex].ParentIndex == BoneIndex)
			{
				FString ChildName = BoneInfoArray[ChildIndex].Name.ToString();
				if (ChildName.Contains(SearchFilter))
				{
					bHasMatchingChild = true;
					break;
				}
			}
		}

		// 본인도 매칭 안되고 자식도 매칭 안되면 렌더링 안함
		if (!bHasMatchingChild)
		{
			return;
		}
	}

	// 자식 본들 찾기
	TArray<int32> ChildBoneIndices;
	for (int32 ChildIndex = 0; ChildIndex < BoneInfoArray.Num(); ++ChildIndex)
	{
		if (BoneInfoArray[ChildIndex].ParentIndex == BoneIndex)
		{
			ChildBoneIndices.Add(ChildIndex);
		}
	}

	// 트리 노드 플래그 설정
	ImGuiTreeNodeFlags NodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

	// 자식이 없으면 Leaf 플래그 추가
	if (ChildBoneIndices.IsEmpty())
	{
		NodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
	}

	 // 선택 상태 표시
	 if (SelectedBoneIndex == BoneIndex)
	 {
	     NodeFlags |= ImGuiTreeNodeFlags_Selected;
	 }

	// 검색 필터에 매칭되면 하이라이트
	bool bIsMatching = false;
	if (bHasSearchFilter && BoneName.Contains(SearchFilter))
	{
		bIsMatching = true;
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.4f, 1.0f)); // 노란색
	}

	// 트리 노드 렌더링
	bool bNodeOpen = ImGui::TreeNodeEx(
		(void*)(intptr_t)BoneIndex,
		NodeFlags,
		"[%d] %s",
		BoneIndex,
		BoneName.c_str()
	);

	if (bIsMatching)
	{
		ImGui::PopStyleColor();
	}

	// 툴팁: 본 정보 표시
	if (ImGui::IsItemHovered())
	{
		ImGui::BeginTooltip();
		ImGui::Text("Parent Index: %d", BoneInfo.ParentIndex);

		if (BoneInfo.ParentIndex != INDEX_NONE)
		{
			ImGui::Text("Parent Name: %s", BoneInfoArray[BoneInfo.ParentIndex].Name.ToString().c_str());
		}
		else
		{
			ImGui::Text("Parent Name: (Root)");
		}

		/*ImGui::Separator();
		ImGui::Text("Local Transform:");
		const FTransform& BonePose = BonePoseArray[BoneIndex];
		ImGui::Text("  Location: (%.2f, %.2f, %.2f)", BonePose.Translation.X, BonePose.Translation.Y, BonePose.Translation.Z);

		FVector EulerRot = BonePose.Rotation.ToEuler();
		ImGui::Text("  Rotation: (%.2f, %.2f, %.2f)", EulerRot.X, EulerRot.Y, EulerRot.Z);
		ImGui::Text("  Scale: (%.2f, %.2f, %.2f)", BonePose.Scale.X, BonePose.Scale.Y, BonePose.Scale.Z);*/

		ImGui::EndTooltip();
	}

	// 클릭 시 본 선택 (TODO: 선택 기능 구현)
	// if (ImGui::IsItemClicked())
	// {
	//     SelectedBoneIndex = BoneIndex;
	// }

	// 클릭 시 본 선택
	if (ImGui::IsItemClicked())
	{
		SelectedBoneIndex = BoneIndex;
	}

	// 자식 본들 재귀적으로 렌더링
	if (bNodeOpen && !ChildBoneIndices.IsEmpty())
	{
		for (int32 ChildBoneIndex : ChildBoneIndices)
		{
			RenderBoneTreeNode(ChildBoneIndex, RefSkeleton, SearchFilter, bHasSearchFilter);
		}
		ImGui::TreePop();
	}
}

/**
 * @brief 중앙 패널: 3D Viewport
 * 독립적인 카메라를 사용한 3D 렌더링 뷰포트
 */
void USkeletalMeshViewerWindow::Render3DViewportPanel()
{
	// 툴바 위젯 업데이트 및 렌더링 (맨 처음)
	if (ToolbarWidget)
	{
		ToolbarWidget->Update();
		ToolbarWidget->RenderWidget();
	}

	const ImVec2 ViewportSize = ImGui::GetContentRegionAvail();

	// 뷰포트 크기가 변경되면 렌더 타겟 재생성
	const uint32 NewWidth = static_cast<uint32>(ViewportSize.x);
	const uint32 NewHeight = static_cast<uint32>(ViewportSize.y);

	if (NewWidth > 0 && NewHeight > 0 && (NewWidth != ViewerWidth || NewHeight != ViewerHeight))
	{
		CreateRenderTarget(NewWidth, NewHeight);
		if (ViewerViewport)
		{
			ViewerViewport->SetSize(NewWidth, NewHeight);
		}
	}

	// 독립 렌더 타겟에 씬 렌더링
	if (ViewerRenderTargetView && ViewerDepthStencilView && ViewerViewportClient)
	{
		URenderer& Renderer = URenderer::GetInstance();
		ID3D11DeviceContext* Context = Renderer.GetDeviceContext();

		// 렌더 타겟 클리어
		const float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f }; // 검정 배경
		Context->ClearRenderTargetView(ViewerRenderTargetView, ClearColor);
		Context->ClearDepthStencilView(ViewerDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		// 렌더 타겟 설정
		Context->OMSetRenderTargets(1, &ViewerRenderTargetView, ViewerDepthStencilView);

		// 뷰포트 설정
		D3D11_VIEWPORT D3DViewport = {};
		D3DViewport.TopLeftX = 0.0f;
		D3DViewport.TopLeftY = 0.0f;
		D3DViewport.Width = static_cast<float>(ViewerWidth);
		D3DViewport.Height = static_cast<float>(ViewerHeight);
		D3DViewport.MinDepth = 0.0f;
		D3DViewport.MaxDepth = 1.0f;
		Context->RSSetViewports(1, &D3DViewport);

		// Viewport의 RenderRect 설정
		ViewerViewport->SetRenderRect(D3DViewport);

		// World 확인
		if (GWorld)
		{
			// 뷰어 전용 라이트를 Level에 임시 등록
			ULevel* CurrentLevel = GWorld->GetLevel();
			if (CurrentLevel)
			{
				if (ViewerAmbientLight)
				{
					CurrentLevel->RegisterComponent(ViewerAmbientLight);
				}
				if (ViewerDirectionalLight)
				{
					CurrentLevel->RegisterComponent(ViewerDirectionalLight);
				}
			}

			// 카메라 준비
			ViewerViewportClient->PrepareCamera(D3DViewport);

			// Visible Primitives 업데이트 (Frustum Culling)
			ViewerViewportClient->UpdateVisiblePrimitives(GWorld);

			// SceneViewFamily 생성
			FSceneViewFamily ViewFamily;
			ViewFamily.SetRenderTarget(ViewerViewport);
			ViewFamily.SetCurrentTime(0.0f);
			ViewFamily.SetDeltaWorldTime(0.016f);

			// ViewportClient의 카메라에서 정보 가져오기
			UCamera* Camera = ViewerViewportClient->GetCamera();
			if (Camera)
			{
				// 카메라 상수 가져오기
				const FCameraConstants& CameraConst = Camera->GetCameraConstants();

				// SceneView 생성
				FSceneView* View = new FSceneView();
				View->InitializeWithMatrices(
					CameraConst.View,
					CameraConst.Projection,
					Camera->GetLocation(),
					Camera->GetRotation(),
					ViewerViewport,
					GWorld,
					ViewerViewportClient->GetViewMode(),
					Camera->GetFovY(),
					CameraConst.NearClip,
					CameraConst.FarClip
				);

				View->SetViewport(ViewerViewport);
				ViewFamily.AddView(View);

				// SceneRenderer로 렌더링
				FSceneRenderer* SceneRenderer = FSceneRenderer::CreateSceneRenderer(ViewFamily);
				if (SceneRenderer)
				{
					SceneRenderer->Render();
					delete SceneRenderer;
				}

				// Grid 렌더링 (SceneRenderer 후, 메인 렌더 타겟 복구 전)
				if (ViewerBatchLines)
				{
					// 카메라 상수 버퍼 업데이트
					URenderer& Renderer = URenderer::GetInstance();
					ID3D11Buffer* ConstantBufferViewProj = Renderer.GetConstantBufferViewProj();
					UPipeline* Pipeline = Renderer.GetPipeline();

					if (ConstantBufferViewProj && Pipeline)
					{
						// 카메라 상수 업데이트
						FRenderResourceFactory::UpdateConstantBufferData(ConstantBufferViewProj, CameraConst);

						// 파이프라인에 바인딩
						Pipeline->SetConstantBuffer(1, EShaderType::VS, ConstantBufferViewProj);
					}

					ViewerBatchLines->Render();
				}

				// View 정리
				delete View;
			}

			// 뷰어 전용 라이트를 Level에서 등록 해제
			if (CurrentLevel)
			{
				if (ViewerAmbientLight)
				{
					CurrentLevel->UnregisterComponent(ViewerAmbientLight);
				}
				if (ViewerDirectionalLight)
				{
					CurrentLevel->UnregisterComponent(ViewerDirectionalLight);
				}
			}
		}

		// 렌더 타겟 및 뷰포트 해제 (메인 렌더 타겟으로 복구)
		ID3D11RenderTargetView* MainRTV = Renderer.GetDeviceResources()->GetBackBufferRTV();
		ID3D11DepthStencilView* MainDSV = Renderer.GetDeviceResources()->GetDepthBufferDSV();
		Context->OMSetRenderTargets(1, &MainRTV, MainDSV);

		// 메인 뷰포트 복구
		const D3D11_VIEWPORT& MainViewport = Renderer.GetDeviceResources()->GetViewportInfo();
		Context->RSSetViewports(1, &MainViewport);
	}

	// ImGui에 렌더 타겟 표시
	if (ViewerShaderResourceView)
	{
		ImTextureID TextureID = reinterpret_cast<ImTextureID>(ViewerShaderResourceView);
		ImGui::Image(TextureID, ViewportSize);

		// 뷰포트 위에 마우스가 있을 때만 드래그 시작
		if (ImGui::IsItemHovered())
		{
			// 우클릭이 처음 눌렸을 때 드래그 시작
			if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
			{
				bIsDraggingRightButton = true;
				HWND hwnd = (HWND)ImGui::GetMainViewport()->PlatformHandleRaw;
				if (hwnd)
				{
					SetCapture(hwnd);
				}
			}

			// 중간 버튼이 처음 눌렸을 때 드래그 시작
			if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle))
			{
				bIsDraggingMiddleButton = true;
				HWND hwnd = (HWND)ImGui::GetMainViewport()->PlatformHandleRaw;
				if (hwnd)
				{
					SetCapture(hwnd);
				}
			}
		}

		// 마우스 버튼이 떼어졌을 때 드래그 종료 (어디서든)
		if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
		{
			bIsDraggingRightButton = false;
			ReleaseCapture();
		}

		if (ImGui::IsMouseReleased(ImGuiMouseButton_Middle))
		{
			bIsDraggingMiddleButton = false;
			ReleaseCapture();
		}

		// 드래그 중이거나 뷰포트 위에 있을 때 카메라 조작 처리
		if ((bIsDraggingRightButton || bIsDraggingMiddleButton || ImGui::IsItemHovered()) && ViewerViewportClient)
		{
			UCamera* Camera = ViewerViewportClient->GetCamera();
			if (Camera)
			{
				ImGuiIO& IO = ImGui::GetIO();

				// 우클릭 드래그: 카메라 회전 (Perspective) 또는 패닝 (Orthographic)
				if (bIsDraggingRightButton && ImGui::IsMouseDown(ImGuiMouseButton_Right))
				{
					FVector MouseDelta = FVector(IO.MouseDelta.x, IO.MouseDelta.y, 0);

					if (Camera->GetCameraType() == ECameraType::ECT_Perspective)
					{
						// Perspective: 마우스 드래그로 회전
						const float YawDelta = MouseDelta.X * KeySensitivityDegPerPixel * 2;
						const float PitchDelta = -MouseDelta.Y * KeySensitivityDegPerPixel * 2;

						FRotator CurrentRot = Camera->GetRotationRotator();
						CurrentRot.Yaw += YawDelta;
						CurrentRot.Pitch += PitchDelta;
						CurrentRot.Roll = 0.0f;

						// Pitch 클램핑
						constexpr float MaxPitch = 89.9f;
						CurrentRot.Pitch = clamp(CurrentRot.Pitch, -MaxPitch, MaxPitch);

						Camera->SetRotation(CurrentRot);

						// WASD 키로 이동
						FVector Direction = FVector::Zero();
						if (ImGui::IsKeyDown(ImGuiKey_A)) { Direction += -Camera->GetRight() * 2; }
						if (ImGui::IsKeyDown(ImGuiKey_D)) { Direction += Camera->GetRight() * 2; }
						if (ImGui::IsKeyDown(ImGuiKey_W)) { Direction += Camera->GetForward() * 2; }
						if (ImGui::IsKeyDown(ImGuiKey_S)) { Direction += -Camera->GetForward() * 2; }
						if (ImGui::IsKeyDown(ImGuiKey_Q)) { Direction += FVector(0, 0, -2); }
						if (ImGui::IsKeyDown(ImGuiKey_E)) { Direction += FVector(0, 0, 2); }

						if (Direction.LengthSquared() > MATH_EPSILON)
						{
							Direction.Normalize();
							Camera->SetLocation(Camera->GetLocation() + Direction * Camera->GetMoveSpeed() * DT);
						}
					}
					else if (Camera->GetCameraType() == ECameraType::ECT_Orthographic)
					{
						// Orthographic: 마우스 드래그로 패닝
						const float PanSpeed = 0.1f;
						FVector PanDelta = Camera->GetRight() * -MouseDelta.X * PanSpeed + Camera->GetUp() * MouseDelta.Y * PanSpeed;
						Camera->SetLocation(Camera->GetLocation() + PanDelta);
					}
				}

				// 마우스 휠 클릭 (중간 버튼) 드래그: 패닝
				if (bIsDraggingMiddleButton && ImGui::IsMouseDown(ImGuiMouseButton_Middle))
				{
					FVector MouseDelta = FVector(IO.MouseDelta.x, IO.MouseDelta.y, 0);

					// 모든 카메라 타입에서 패닝 동작
					const float PanSpeed = 0.5f;
					FVector PanDelta = Camera->GetRight() * -MouseDelta.X * PanSpeed + Camera->GetUp() * MouseDelta.Y * PanSpeed;
					Camera->SetLocation(Camera->GetLocation() + PanDelta);
				}

				// 마우스 휠: 줌 또는 카메라 속도 조절
				if (IO.MouseWheel != 0.0f)
				{
					// 우클릭 + 마우스 휠: 카메라 이동 속도 조절
					if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
					{
						float CurrentSpeed = Camera->GetMoveSpeed();
						float NewSpeed = CurrentSpeed + IO.MouseWheel * 1.0f;
						NewSpeed = clamp(NewSpeed, 0.1f, 100.0f);
						Camera->SetMoveSpeed(NewSpeed);
					}
					else
					{
						// 일반 마우스 휠: 줌
						if (Camera->GetCameraType() == ECameraType::ECT_Perspective)
						{
							// Perspective: FOV 조절
							float NewFOV = Camera->GetFovY() - IO.MouseWheel * 5.0f;
							NewFOV = clamp(NewFOV, 10.0f, 120.0f);
							Camera->SetFovY(NewFOV);
						}
						else if (Camera->GetCameraType() == ECameraType::ECT_Orthographic)
						{
							// Orthographic: Zoom 조절
							float NewZoom = Camera->GetOrthoZoom() * (1.0f - IO.MouseWheel * 0.1f);
							NewZoom = clamp(NewZoom, 10.0f, 10000.0f);
							Camera->SetOrthoZoom(NewZoom);
						}
					}
				}
			}
		}

		// 뷰포트 상태 표시 (디버그 정보) - 좌측 상단 오버레이
		ImDrawList* DrawList = ImGui::GetWindowDrawList();
		ImVec2 WindowPos = ImGui::GetCursorScreenPos();
		WindowPos.y -= ViewportSize.y; // Image가 그려진 위치로 이동

		ImVec2 InfoPos = ImVec2(WindowPos.x + 10, WindowPos.y + 10);
		ImVec2 InfoSize = ImVec2(280, 180);

		// 반투명 배경
		DrawList->AddRectFilled(InfoPos, ImVec2(InfoPos.x + InfoSize.x, InfoPos.y + InfoSize.y),
		                        IM_COL32(0, 0, 0, 180), 4.0f);

		// 텍스트
		DrawList->AddText(ImVec2(InfoPos.x + 10, InfoPos.y + 10), IM_COL32(80, 200, 200, 255), "3D Viewport");
		DrawList->AddText(ImVec2(InfoPos.x + 10, InfoPos.y + 30), IM_COL32(200, 200, 200, 255),
		                  (FString("Size: ") + std::to_string(ViewerWidth) + " x " + std::to_string(ViewerHeight)).c_str());

		// 카메라 정보 표시
		if (ViewerViewportClient)
		{
			UCamera* Camera = ViewerViewportClient->GetCamera();
			if (Camera)
			{
				FVector CameraPos = Camera->GetLocation();
				FVector CameraRot = Camera->GetRotation();
				float CameraFOV = Camera->GetFovY();
				float CameraAspect = Camera->GetAspect();

				// 카메라 위치
				char CameraInfoText[256];
				sprintf_s(CameraInfoText, "Pos: %.1f, %.1f, %.1f", CameraPos.X, CameraPos.Y, CameraPos.Z);
				DrawList->AddText(ImVec2(InfoPos.x + 10, InfoPos.y + 55), IM_COL32(200, 200, 100, 255), CameraInfoText);

				// 카메라 회전
				sprintf_s(CameraInfoText, "Rot: %.1f, %.1f, %.1f", CameraRot.X, CameraRot.Y, CameraRot.Z);
				DrawList->AddText(ImVec2(InfoPos.x + 10, InfoPos.y + 75), IM_COL32(200, 200, 100, 255), CameraInfoText);

				// FOV와 종횡비
				sprintf_s(CameraInfoText, "FOV: %.1f | Aspect: %.2f", CameraFOV, CameraAspect);
				DrawList->AddText(ImVec2(InfoPos.x + 10, InfoPos.y + 95), IM_COL32(200, 200, 100, 255), CameraInfoText);

				// 컨트롤 힌트
				DrawList->AddText(ImVec2(InfoPos.x + 10, InfoPos.y + 120), IM_COL32(150, 150, 150, 255), "RMB: Rotate | MMB: Pan");
				DrawList->AddText(ImVec2(InfoPos.x + 10, InfoPos.y + 140), IM_COL32(150, 150, 150, 255), "Wheel: Zoom | Q/W/E/R: Gizmo");
				DrawList->AddText(ImVec2(InfoPos.x + 10, InfoPos.y + 160), IM_COL32(150, 150, 150, 255), "F: Focus | Alt+G: Toggle Grid");

				// 축 방향 표시기 렌더링 (좌측 하단)
				const float AxisCenterX = WindowPos.x + 70.0f;
				const float AxisCenterY = WindowPos.y + ViewportSize.y - 70.0f;
				const float AxisSize = 40.0f;
				const float LineThickness = 3.0f;

				// 카메라의 View 행렬 가져오기
				const FMatrix& ViewMatrix = Camera->GetFViewProjConstants().View;

				// 월드 축 벡터 (X, Y, Z)
				FVector4 WorldX(1, 0, 0, 0);
				FVector4 WorldY(0, 1, 0, 0);
				FVector4 WorldZ(0, 0, 1, 0);

				// View 공간으로 변환
				FVector4 ViewAxisX(
					WorldX.X * ViewMatrix.Data[0][0] + WorldX.Y * ViewMatrix.Data[1][0] + WorldX.Z * ViewMatrix.Data[2][0],
					WorldX.X * ViewMatrix.Data[0][1] + WorldX.Y * ViewMatrix.Data[1][1] + WorldX.Z * ViewMatrix.Data[2][1],
					WorldX.X * ViewMatrix.Data[0][2] + WorldX.Y * ViewMatrix.Data[1][2] + WorldX.Z * ViewMatrix.Data[2][2],
					0.f
				);

				FVector4 ViewAxisY(
					WorldY.X * ViewMatrix.Data[0][0] + WorldY.Y * ViewMatrix.Data[1][0] + WorldY.Z * ViewMatrix.Data[2][0],
					WorldY.X * ViewMatrix.Data[0][1] + WorldY.Y * ViewMatrix.Data[1][1] + WorldY.Z * ViewMatrix.Data[2][1],
					WorldY.X * ViewMatrix.Data[0][2] + WorldY.Y * ViewMatrix.Data[1][2] + WorldY.Z * ViewMatrix.Data[2][2],
					0.f
				);

				FVector4 ViewAxisZ(
					WorldZ.X * ViewMatrix.Data[0][0] + WorldZ.Y * ViewMatrix.Data[1][0] + WorldZ.Z * ViewMatrix.Data[2][0],
					WorldZ.X * ViewMatrix.Data[0][1] + WorldZ.Y * ViewMatrix.Data[1][1] + WorldZ.Z * ViewMatrix.Data[2][1],
					WorldZ.X * ViewMatrix.Data[0][2] + WorldZ.Y * ViewMatrix.Data[1][2] + WorldZ.Z * ViewMatrix.Data[2][2],
					0.f
				);

				// 2D 스크린 좌표로 변환
				ImVec2 AxisEndX(AxisCenterX + ViewAxisX.X * AxisSize, AxisCenterY - ViewAxisX.Y * AxisSize);
				ImVec2 AxisEndY(AxisCenterX + ViewAxisY.X * AxisSize, AxisCenterY - ViewAxisY.Y * AxisSize);
				ImVec2 AxisEndZ(AxisCenterX + ViewAxisZ.X * AxisSize, AxisCenterY - ViewAxisZ.Y * AxisSize);

				// 텍스트 위치 (1.25배 더 멀리)
				const float TextOffset = 1.25f;
				ImVec2 TextPosX(AxisCenterX + ViewAxisX.X * AxisSize * TextOffset, AxisCenterY - ViewAxisX.Y * AxisSize * TextOffset);
				ImVec2 TextPosY(AxisCenterX + ViewAxisY.X * AxisSize * TextOffset, AxisCenterY - ViewAxisY.Y * AxisSize * TextOffset);
				ImVec2 TextPosZ(AxisCenterX + ViewAxisZ.X * AxisSize * TextOffset, AxisCenterY - ViewAxisZ.Y * AxisSize * TextOffset);

				// 축 라인 그리기
				DrawList->AddLine(ImVec2(AxisCenterX, AxisCenterY), AxisEndX, IM_COL32(255, 0, 0, 255), LineThickness); // X축 (빨강)
				DrawList->AddLine(ImVec2(AxisCenterX, AxisCenterY), AxisEndY, IM_COL32(0, 255, 0, 255), LineThickness); // Y축 (초록)
				DrawList->AddLine(ImVec2(AxisCenterX, AxisCenterY), AxisEndZ, IM_COL32(0, 102, 255, 255), LineThickness); // Z축 (파랑)

				// 중심점 그리기
				DrawList->AddCircleFilled(ImVec2(AxisCenterX, AxisCenterY), 3.0f, IM_COL32(51, 51, 51, 255));
				DrawList->AddCircleFilled(ImVec2(AxisCenterX, AxisCenterY), 1.5f, IM_COL32(51, 51, 51, 255));

				// 축 레이블 텍스트
				const bool bIsOrtho = (Camera->GetCameraType() == ECameraType::ECT_Orthographic);
				const float VisibilityThreshold = 0.98f;

				// X축 텍스트
				if (!bIsOrtho || std::abs(ViewAxisX.Z) < VisibilityThreshold)
				{
					DrawList->AddText(ImVec2(TextPosX.x - 8, TextPosX.y - 8), IM_COL32(255, 0, 0, 255), "X");
				}

				// Y축 텍스트
				if (!bIsOrtho || std::abs(ViewAxisY.Z) < VisibilityThreshold)
				{
					DrawList->AddText(ImVec2(TextPosY.x - 8, TextPosY.y - 8), IM_COL32(0, 255, 0, 255), "Y");
				}

				// Z축 텍스트
				if (!bIsOrtho || std::abs(ViewAxisZ.Z) < VisibilityThreshold)
				{
					DrawList->AddText(ImVec2(TextPosZ.x - 8, TextPosZ.y - 8), IM_COL32(0, 102, 255, 255), "Z");
				}
			}
			else
			{
				DrawList->AddText(ImVec2(InfoPos.x + 10, InfoPos.y + 50), IM_COL32(200, 200, 200, 255), "Camera: Not Available");
			}
		}
	}
	else
	{
		// 렌더 타겟이 없을 경우 플레이스홀더
		const char* PlaceholderText = "Render Target Not Available";
		const ImVec2 TextSize = ImGui::CalcTextSize(PlaceholderText);
		ImGui::SetCursorPos(ImVec2((ViewportSize.x - TextSize.x) * 0.5f, ViewportSize.y * 0.5f));
		ImGui::TextDisabled("%s", PlaceholderText);
	}

	// TODO 주석
	// TODO: ViewerViewportClient를 사용한 독립적인 씬 렌더링
	// TODO: SkeletalMesh 렌더링
	// TODO: 선택된 본의 Transform Gizmo 렌더링
	// TODO: 독립적인 카메라 조작 (마우스 드래그로 회전, 줌, 팬)
	// TODO: 그리드 표시
}

/**
 * @brief 우측 패널: Edit Tools (Placeholder)
 */
void USkeletalMeshViewerWindow::RenderEditToolsPanel(const USkeletalMesh* InSkeletalMesh, const FReferenceSkeleton& InRefSkeleton, const int32 InNumBones)
{
	/*assert(InSkeletalMesh);
	assert(InNumBones > 0);*/

	bool bValid = CheckSkeletalValidity(const_cast<USkeletalMesh*>(InSkeletalMesh), const_cast<FReferenceSkeleton&>(InRefSkeleton), const_cast<int32&>(InNumBones), true);
	if (bValid == false)
	{
		return;
	}

	ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.8f, 1.0f), "Edit Tools");
	ImGui::Separator();
	ImGui::Spacing();

	// TempBoneSpaceTransforms 초기화 확인
	/*if (TempBoneSpaceTransforms.IsEmpty())
	{
		TempBoneSpaceTransforms.SetNum(NumBones);
		for (int32 i = 0; i < NumBones; ++i)
		{
			TempBoneSpaceTransforms[i] = SkeletalMeshComponent->GetBoneTransformLocal(i);
		}
	}*/

	// 선택된 본이 없는 경우
	if (SelectedBoneIndex == INDEX_NONE)
	{
		ImGui::TextDisabled("No bone selected");
		ImGui::Spacing();
		ImGui::TextWrapped("Select a bone from the Skeleton Tree to view and edit its properties.");
		return;
	}

	// 선택된 본 인덱스 유효성 검사
	assert((SelectedBoneIndex < 0 || SelectedBoneIndex >= InNumBones) && "Invalid bone index selected");

	const TArray<FMeshBoneInfo>& BoneInfoArray = InRefSkeleton.GetRawRefBoneInfo();
	const TArray<FTransform>& RefBonePoses = InRefSkeleton.GetRawRefBonePose();
	const FMeshBoneInfo& BoneInfo = BoneInfoArray[SelectedBoneIndex];

	// ===================================================================
	// Reference Pose (읽기 전용)
	// ===================================================================
	ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.2f, 0.3f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.3f, 0.3f, 0.4f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.25f, 0.25f, 0.35f, 1.0f));

	if (ImGui::CollapsingHeader("Reference Pose (Read Only)", ImGuiTreeNodeFlags_DefaultOpen))
	{
		const FTransform& RefPose = RefBonePoses[SelectedBoneIndex];

		ImGui::Indent();

		ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), "Translation:");
		ImGui::Text("  X: %.3f", RefPose.Translation.X);
		ImGui::Text("  Y: %.3f", RefPose.Translation.Y);
		ImGui::Text("  Z: %.3f", RefPose.Translation.Z);

		ImGui::Spacing();

		ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), "Rotation (Quaternion):");
		ImGui::Text("  X: %.3f", RefPose.Rotation.X);
		ImGui::Text("  Y: %.3f", RefPose.Rotation.Y);
		ImGui::Text("  Z: %.3f", RefPose.Rotation.Z);
		ImGui::Text("  W: %.3f", RefPose.Rotation.W);

		ImGui::Spacing();

		FVector EulerRot = RefPose.Rotation.ToEuler();
		ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), "Rotation (Euler):");
		ImGui::Text("  Pitch: %.3f", EulerRot.X);
		ImGui::Text("  Yaw: %.3f", EulerRot.Y);
		ImGui::Text("  Roll: %.3f", EulerRot.Z);

		ImGui::Spacing();

		ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), "Scale:");
		ImGui::Text("  X: %.3f", RefPose.Scale.X);
		ImGui::Text("  Y: %.3f", RefPose.Scale.Y);
		ImGui::Text("  Z: %.3f", RefPose.Scale.Z);

		ImGui::Unindent();
	}

	ImGui::PopStyleColor(3);
	ImGui::Spacing();

	// ===================================================================
	// Bone Space Transform (편집 가능)
	// ===================================================================
	ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.3f, 0.2f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.3f, 0.4f, 0.3f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.25f, 0.35f, 0.25f, 1.0f));

	if (ImGui::CollapsingHeader("Bone Space Transform (Editable)", ImGuiTreeNodeFlags_DefaultOpen))
	{
		FTransform& TempTransform = TempBoneSpaceTransforms[SelectedBoneIndex];

		ImGui::Indent();

		// Translation
		ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "Translation:");

		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));

		ImDrawList* DrawList = ImGui::GetWindowDrawList();

		// X
		ImVec2 PosX = ImGui::GetCursorScreenPos();
		ImGui::SetNextItemWidth(80.0f);
		ImGui::DragFloat("##TransX", &TempTransform.Translation.X, 0.1f, 0.0f, 0.0f, "%.3f");
		ImVec2 SizeX = ImGui::GetItemRectSize();
		DrawList->AddLine(ImVec2(PosX.x + 5, PosX.y + 2), ImVec2(PosX.x + 5, PosX.y + SizeX.y - 2),
			IM_COL32(255, 0, 0, 255), 2.0f);
		ImGui::SameLine();

		// Y
		ImVec2 PosY = ImGui::GetCursorScreenPos();
		ImGui::SetNextItemWidth(80.0f);
		ImGui::DragFloat("##TransY", &TempTransform.Translation.Y, 0.1f, 0.0f, 0.0f, "%.3f");
		ImVec2 SizeY = ImGui::GetItemRectSize();
		DrawList->AddLine(ImVec2(PosY.x + 5, PosY.y + 2), ImVec2(PosY.x + 5, PosY.y + SizeY.y - 2),
			IM_COL32(0, 255, 0, 255), 2.0f);
		ImGui::SameLine();

		// Z
		ImVec2 PosZ = ImGui::GetCursorScreenPos();
		ImGui::SetNextItemWidth(80.0f);
		ImGui::DragFloat("##TransZ", &TempTransform.Translation.Z, 0.1f, 0.0f, 0.0f, "%.3f");
		ImVec2 SizeZ = ImGui::GetItemRectSize();
		DrawList->AddLine(ImVec2(PosZ.x + 5, PosZ.y + 2), ImVec2(PosZ.x + 5, PosZ.y + SizeZ.y - 2),
			IM_COL32(0, 0, 255, 255), 2.0f);

		ImGui::Spacing();

		// Rotation (Euler)
		ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "Rotation (Euler):");

		FVector EulerAngles = TempTransform.Rotation.ToEuler();

		// Pitch (X)
		ImVec2 RotX = ImGui::GetCursorScreenPos();
		ImGui::SetNextItemWidth(80.0f);
		ImGui::DragFloat("##RotX", &EulerAngles.X, 1.0f, 0.0f, 0.0f, "%.3f");
		ImVec2 SizeRotX = ImGui::GetItemRectSize();
		DrawList->AddLine(ImVec2(RotX.x + 5, RotX.y + 2), ImVec2(RotX.x + 5, RotX.y + SizeRotX.y - 2),
			IM_COL32(255, 0, 0, 255), 2.0f);
		ImGui::SameLine();

		// Yaw (Y)
		ImVec2 RotY = ImGui::GetCursorScreenPos();
		ImGui::SetNextItemWidth(80.0f);
		ImGui::DragFloat("##RotY", &EulerAngles.Y, 1.0f, 0.0f, 0.0f, "%.3f");
		ImVec2 SizeRotY = ImGui::GetItemRectSize();
		DrawList->AddLine(ImVec2(RotY.x + 5, RotY.y + 2), ImVec2(RotY.x + 5, RotY.y + SizeRotY.y - 2),
			IM_COL32(0, 255, 0, 255), 2.0f);
		ImGui::SameLine();

		// Roll (Z)
		ImVec2 RotZ = ImGui::GetCursorScreenPos();
		ImGui::SetNextItemWidth(80.0f);
		ImGui::DragFloat("##RotZ", &EulerAngles.Z, 1.0f, 0.0f, 0.0f, "%.3f");
		ImVec2 SizeRotZ = ImGui::GetItemRectSize();
		DrawList->AddLine(ImVec2(RotZ.x + 5, RotZ.y + 2), ImVec2(RotZ.x + 5, RotZ.y + SizeRotZ.y - 2),
			IM_COL32(0, 0, 255, 255), 2.0f);

		// Euler를 Quaternion으로 변환
		TempTransform.Rotation = FQuaternion::FromEuler(EulerAngles);

		ImGui::Spacing();

		// Scale
		ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "Scale:");

		// Scale X
		ImVec2 ScaleX = ImGui::GetCursorScreenPos();
		ImGui::SetNextItemWidth(80.0f);
		ImGui::DragFloat("##ScaleX", &TempTransform.Scale.X, 0.01f, 0.0f, 0.0f, "%.3f");
		ImVec2 SizeScaleX = ImGui::GetItemRectSize();
		DrawList->AddLine(ImVec2(ScaleX.x + 5, ScaleX.y + 2), ImVec2(ScaleX.x + 5, ScaleX.y + SizeScaleX.y - 2),
			IM_COL32(255, 0, 0, 255), 2.0f);
		ImGui::SameLine();

		// Scale Y
		ImVec2 ScaleY = ImGui::GetCursorScreenPos();
		ImGui::SetNextItemWidth(80.0f);
		ImGui::DragFloat("##ScaleY", &TempTransform.Scale.Y, 0.01f, 0.0f, 0.0f, "%.3f");
		ImVec2 SizeScaleY = ImGui::GetItemRectSize();
		DrawList->AddLine(ImVec2(ScaleY.x + 5, ScaleY.y + 2), ImVec2(ScaleY.x + 5, ScaleY.y + SizeScaleY.y - 2),
			IM_COL32(0, 255, 0, 255), 2.0f);
		ImGui::SameLine();

		// Scale Z
		ImVec2 ScaleZ = ImGui::GetCursorScreenPos();
		ImGui::SetNextItemWidth(80.0f);
		ImGui::DragFloat("##ScaleZ", &TempTransform.Scale.Z, 0.01f, 0.0f, 0.0f, "%.3f");
		ImVec2 SizeScaleZ = ImGui::GetItemRectSize();
		DrawList->AddLine(ImVec2(ScaleZ.x + 5, ScaleZ.y + 2), ImVec2(ScaleZ.x + 5, ScaleZ.y + SizeScaleZ.y - 2),
			IM_COL32(0, 0, 255, 255), 2.0f);

		ImGui::PopStyleColor(3);

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		//Reset 버튼
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.3f, 0.2f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f, 0.4f, 0.3f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.45f, 0.25f, 0.15f, 1.0f));

		if (ImGui::Button("Reset to Current", ImVec2(-1, 30)))
		{
			// 현재 컴포넌트의 값으로 리셋
			for (int32 i = 0; i < InNumBones; ++i)
			{
				TempBoneSpaceTransforms[i] = SkeletalMeshComponent->GetBoneTransformLocal(i);
			}
			UE_LOG("SkeletalMeshViewerWindow: Reset temp bone transforms to current values");
		}
		ImGui::PopStyleColor(3);

		ImGui::Unindent();
	}

	ImGui::PopStyleColor(3);
	ImGui::Spacing();

	// ===================================================================
	// Gizmo Settings
	// ===================================================================
	ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));

	if (ImGui::CollapsingHeader("Gizmo Settings"))
	{
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));

		static int GizmoMode = 0;
		ImGui::RadioButton("Translate", &GizmoMode, 0);
		ImGui::RadioButton("Rotate", &GizmoMode, 1);
		ImGui::RadioButton("Scale", &GizmoMode, 2);

		ImGui::Spacing();
		static bool bLocalSpace = false;
		ImGui::Checkbox("Local Space", &bLocalSpace);

		ImGui::PopStyleColor(4);
	}

	ImGui::PopStyleColor(3);
	ImGui::Spacing();

	// ===================================================================
	// Mesh Info
	// ===================================================================
	ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));

	if (ImGui::CollapsingHeader("Mesh Info"))
	{
		if (InSkeletalMesh)
		{
			ImGui::Text("Loaded Mesh: %s", InSkeletalMesh->GetName().ToString().c_str());
			ImGui::Text("Bones: %d", InNumBones);
		}
		else
		{
			ImGui::TextDisabled("Loaded Mesh: (없음)");
			ImGui::TextDisabled("Bones: 0");
		}
	}

	ImGui::PopStyleColor(3);

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::TextWrapped("추가 구현 예정:");
	ImGui::BulletText("본 프로퍼티 편집");
	ImGui::BulletText("애니메이션 프리뷰");
	ImGui::BulletText("소켓 편집");
	ImGui::BulletText("LOD 설정");

	// Apply Changes 버튼 - 우측 하단에 고정
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.2f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.6f, 0.3f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.45f, 0.15f, 1.0f));

	if (ImGui::Button("Apply All Changes", ImVec2(-1, ViewerHeight)))
	{
		// TempBoneSpaceTransforms를 실제 BoneSpaceTransforms에 적용
		for (int32 i = 0; i < InNumBones; ++i)
		{
			SkeletalMeshComponent->SetBoneTransformLocal(i, TempBoneSpaceTransforms[i]);
		}
		UE_LOG("SkeletalMeshViewerWindow: Applied bone transform changes");
	}
	ImGui::PopStyleColor(3);
}

/**
 * @brief 카메라 컨트롤 UI 렌더링
 */
void USkeletalMeshViewerWindow::RenderCameraControls(UCamera& InCamera)
{
	ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), "Camera Settings");
	ImGui::Separator();
	ImGui::Spacing();

	// 카메라 타입
	ECameraType CameraType = InCamera.GetCameraType();
	const char* CameraTypeNames[] = { "Perspective", "Orthographic" };
	int CurrentType = static_cast<int>(CameraType);

	if (ImGui::Combo("Camera Type", &CurrentType, CameraTypeNames, 2))
	{
		InCamera.SetCameraType(static_cast<ECameraType>(CurrentType));
	}

	ImGui::Spacing();

	// 위치
	FVector Location = InCamera.GetLocation();
	float Loc[3] = { Location.X, Location.Y, Location.Z };
	if (ImGui::DragFloat3("Location", Loc, 1.0f))
	{
		InCamera.SetLocation(FVector(Loc[0], Loc[1], Loc[2]));
	}

	// 회전
	FVector Rotation = InCamera.GetRotation();
	float Rot[3] = { Rotation.X, Rotation.Y, Rotation.Z };
	if (ImGui::DragFloat3("Rotation", Rot, 1.0f))
	{
		InCamera.SetRotation(FVector(Rot[0], Rot[1], Rot[2]));
	}

	ImGui::Spacing();

	// Perspective 전용 설정
	if (CameraType == ECameraType::ECT_Perspective)
	{
		float FOV = InCamera.GetFovY();
		if (ImGui::SliderFloat("FOV", &FOV, 10.0f, 120.0f, "%.1f"))
		{
			InCamera.SetFovY(FOV);
		}
	}
	// Orthographic 전용 설정
	else
	{
		float OrthoZoom = InCamera.GetOrthoZoom();
		if (ImGui::SliderFloat("Ortho Zoom", &OrthoZoom, 10.0f, 10000.0f, "%.1f"))
		{
			InCamera.SetOrthoZoom(OrthoZoom);
		}
	}

	// Near/Far Clip
	float NearZ = InCamera.GetNearZ();
	float FarZ = InCamera.GetFarZ();

	if (ImGui::DragFloat("Near Clip", &NearZ, 0.1f, 0.1f, 100.0f, "%.2f"))
	{
		InCamera.SetNearZ(NearZ);
	}

	if (ImGui::DragFloat("Far Clip", &FarZ, 10.0f, 100.0f, 100000.0f, "%.1f"))
	{
		InCamera.SetFarZ(FarZ);
	}

	ImGui::Spacing();

	// 이동 속도
	float MoveSpeed = InCamera.GetMoveSpeed();
	if (ImGui::SliderFloat("Move Speed", &MoveSpeed, 1.0f, 100.0f, "%.1f"))
	{
		InCamera.SetMoveSpeed(MoveSpeed);
	}

	ImGui::Spacing();
	ImGui::Separator();

	// 카메라 리셋 버튼
	if (ImGui::Button("Reset Camera", ImVec2(-1, 0)))
	{
		InCamera.SetLocation(FVector(0, -500, 300));
		InCamera.SetRotation(FVector(0, 0, 0));
		InCamera.SetFovY(90.0f);
		InCamera.SetOrthoZoom(1000.0f);
		InCamera.SetMoveSpeed(10.0f);
	}
}

bool USkeletalMeshViewerWindow::CheckSkeletalValidity(USkeletalMesh* OutSkeletalMesh, FReferenceSkeleton& OutRefSkeleton, int32& OutNumBones, bool bLogging) const
{
	// SkeletalMeshComponent 유효성 검사
	if (!SkeletalMeshComponent)
	{
		if (bLogging)
		{
			ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No SkeletalMeshComponent assigned");
			ImGui::Spacing();
			ImGui::TextWrapped("Select a SkeletalMeshComponent from the Detail panel to view its skeleton.");
		}
		return false;
	}

	// SkeletalMesh 가져오기
	OutSkeletalMesh = SkeletalMeshComponent->GetSkeletalMeshAsset();
	if (!OutSkeletalMesh)
	{
		if (bLogging)
		{
			ImGui::TextColored(ImVec4(0.8f, 0.4f, 0.4f, 1.0f), "No SkeletalMesh found");
			ImGui::Spacing();
			ImGui::TextWrapped("The component has no mesh asset assigned.");
		}
		return false;
	}

	// ReferenceSkeleton 가져오기
	OutRefSkeleton = OutSkeletalMesh->GetRefSkeleton();
	OutNumBones = OutRefSkeleton.GetRawBoneNum();

	if (OutNumBones == 0)
	{
		if (bLogging)
		{
			ImGui::TextColored(ImVec4(0.8f, 0.4f, 0.4f, 1.0f), "No bones in skeleton");
		}
		return false;
	}

	return true;
}

/**
 * @brief 수직 Splitter (드래그 가능한 구분선) 렌더링
 * @param bInvertDirection false면 좌측 패널용 (오른쪽 드래그 = 증가), true면 우측 패널용 (왼쪽 드래그 = 증가)
 */
void USkeletalMeshViewerWindow::RenderVerticalSplitter(const char* SplitterID, float& Ratio, float MinRatio, float MaxRatio, bool bInvertDirection)
{
	const ImVec2 ContentRegion = ImGui::GetContentRegionAvail();
	const float SplitterHeight = ContentRegion.y;

	// Splitter 영역 설정
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f, 0.6f, 0.6f, 0.5f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.8f, 0.8f, 0.7f));

	ImGui::Button(SplitterID, ImVec2(SplitterWidth, SplitterHeight));

	ImGui::PopStyleColor(3);

	// 호버 시 커서 변경
	if (ImGui::IsItemHovered())
	{
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
	}

	// 드래그 처리
	if (ImGui::IsItemActive())
	{
		// 마우스 이동량 계산
		const float MouseDeltaX = ImGui::GetIO().MouseDelta.x;
		const float TotalWidth = ImGui::GetWindowWidth();

		// 비율 조정
		// 좌측 패널: 오른쪽 드래그 = 증가 (양수)
		// 우측 패널: 왼쪽 드래그 = 증가 (음수를 곱해서 반전)
		const float DirectionMultiplier = bInvertDirection ? -1.0f : 1.0f;
		const float DeltaRatio = (MouseDeltaX / TotalWidth) * DirectionMultiplier;
		Ratio += DeltaRatio;

		// 비율 제한
		Ratio = ImClamp(Ratio, MinRatio, MaxRatio);
	}
}

/**
 * @brief 그리드 셀 크기 설정
 */
void USkeletalMeshViewerWindow::SetGridCellSize(float NewCellSize)
{
	GridCellSize = NewCellSize;

	// ViewerBatchLines가 존재하면 그리드 업데이트
	if (ViewerBatchLines)
	{
		ViewerBatchLines->UpdateUGridVertices(GridCellSize);
		ViewerBatchLines->UpdateVertexBuffer();
	}
}
