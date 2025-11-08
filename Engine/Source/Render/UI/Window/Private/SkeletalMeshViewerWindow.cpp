#include "pch.h"
#include "Render/UI/Window/Public/SkeletalMeshViewerWindow.h"
#include "ImGui/imgui_internal.h"
#include "Render/UI/Viewport/Public/Viewport.h"
#include "Render/UI/Viewport/Public/ViewportClient.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Render/Renderer/Public/SceneView.h"
#include "Render/Renderer/Public/SceneViewFamily.h"
#include "Render/Renderer/Public/SceneRenderer.h"
#include "Level/Public/World.h"
#include "Core/Public/NewObject.h"

IMPLEMENT_CLASS(USkeletalMeshViewerWindow, UUIWindow)

/**
 * @brief SkeletalMeshViewerWindow 생성자
 * 독립적인 플로팅 윈도우로 설정
 */
USkeletalMeshViewerWindow::USkeletalMeshViewerWindow()
{
	FUIWindowConfig Config;
	Config.WindowTitle = "SkeletalMesh Viewer";
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
	// Viewport 생성
	ViewerViewport = new FViewport();
	ViewerViewport->SetSize(ViewerWidth, ViewerHeight);
	ViewerViewport->SetInitialPosition(0, 0);

	// ViewportClient 생성
	ViewerViewportClient = new FViewportClient();
	ViewerViewportClient->SetOwningViewport(ViewerViewport);
	ViewerViewportClient->SetViewType(EViewType::Perspective);
	ViewerViewportClient->SetViewMode(EViewModeIndex::VMI_Gouraud);

	ViewerViewport->SetViewportClient(ViewerViewportClient);

	// 렌더 타겟 생성
	CreateRenderTarget(ViewerWidth, ViewerHeight);

	UE_LOG("SkeletalMeshViewerWindow: 독립적인 뷰포트 및 카메라 초기화 완료");
}

/**
 * @brief 정리 함수 - 뷰포트와 카메라 해제
 */
void USkeletalMeshViewerWindow::Cleanup()
{
	// 렌더 타겟 해제
	ReleaseRenderTarget();

	// ViewportClient를 먼저 삭제 (Viewport를 참조하고 있으므로)
	if (ViewerViewportClient)
	{
		delete ViewerViewportClient;
		ViewerViewportClient = nullptr;
	}

	// 그 다음 Viewport 삭제
	if (ViewerViewport)
	{
		delete ViewerViewport;
		ViewerViewport = nullptr;
	}

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

	// === 좌측 패널: Skeleton Tree ===
	if (ImGui::BeginChild("SkeletonTreePanel", ImVec2(LeftPanelWidth - SplitterWidth * 0.5f, PanelHeight), true))
	{
		RenderSkeletonTreePanel();
	}
	ImGui::EndChild();

	ImGui::SameLine();

	// === 첫 번째 Splitter (좌측-중앙 사이) ===
	RenderVerticalSplitter("##LeftCenterSplitter", LeftPanelWidthRatio, MinPanelRatio, MaxPanelRatio);

	ImGui::SameLine();

	// === 중앙 패널: 3D Viewport ===
	if (ImGui::BeginChild("3DViewportPanel", ImVec2(CenterPanelWidth - SplitterWidth, PanelHeight), true))
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
		RenderEditToolsPanel();
	}
	ImGui::EndChild();
}

/**
 * @brief 좌측 패널: Skeleton Tree (Placeholder)
 */
void USkeletalMeshViewerWindow::RenderSkeletonTreePanel()
{
	ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), "Skeleton Tree");
	ImGui::Separator();
	ImGui::Spacing();

	ImGui::TextWrapped("TODO: 본 계층 구조 트리를 여기에 구현");
	ImGui::Spacing();
	ImGui::BulletText("본 선택 기능");
	ImGui::BulletText("계층 구조 표시");
	ImGui::BulletText("본 검색 기능");
	ImGui::BulletText("본 필터링 옵션");

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::TextDisabled("예시 구조:");

	// 예시 트리 구조 (더미)
	if (ImGui::TreeNode("Root"))
	{
		if (ImGui::TreeNode("Spine"))
		{
			if (ImGui::TreeNode("Chest"))
			{
				ImGui::TreeNode("Neck");
				ImGui::TreePop();
			}
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("L_Leg"))
		{
			ImGui::TreeNode("L_Foot");
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("R_Leg"))
		{
			ImGui::TreeNode("R_Foot");
			ImGui::TreePop();
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
		const float ClearColor[4] = { 0.2f, 0.2f, 0.25f, 1.0f }; // 어두운 회색 배경
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

				// View 정리
				delete View;
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

		// 뷰포트 상태 표시 (디버그 정보) - 좌측 상단 오버레이
		ImDrawList* DrawList = ImGui::GetWindowDrawList();
		ImVec2 WindowPos = ImGui::GetCursorScreenPos();
		WindowPos.y -= ViewportSize.y; // Image가 그려진 위치로 이동

		ImVec2 InfoPos = ImVec2(WindowPos.x + 10, WindowPos.y + 10);
		ImVec2 InfoSize = ImVec2(200, 80);

		// 반투명 배경
		DrawList->AddRectFilled(InfoPos, ImVec2(InfoPos.x + InfoSize.x, InfoPos.y + InfoSize.y),
		                        IM_COL32(0, 0, 0, 180), 4.0f);

		// 텍스트
		DrawList->AddText(ImVec2(InfoPos.x + 10, InfoPos.y + 10), IM_COL32(80, 200, 200, 255), "3D Viewport");
		DrawList->AddText(ImVec2(InfoPos.x + 10, InfoPos.y + 30), IM_COL32(200, 200, 200, 255),
		                  (FString("Size: ") + std::to_string(ViewerWidth) + " x " + std::to_string(ViewerHeight)).c_str());
		DrawList->AddText(ImVec2(InfoPos.x + 10, InfoPos.y + 50), IM_COL32(200, 200, 200, 255), "Camera: Active");
		DrawList->AddText(ImVec2(InfoPos.x + 10, InfoPos.y + 65), IM_COL32(200, 200, 200, 255), "Mode: Perspective");
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
void USkeletalMeshViewerWindow::RenderEditToolsPanel()
{
	ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.8f, 1.0f), "Edit Tools");
	ImGui::Separator();
	ImGui::Spacing();

	ImGui::TextWrapped("TODO: 편집 도구 UI를 여기에 구현");
	ImGui::Spacing();

	// Transform 섹션 (검정 테마 적용)
	ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));

	if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::TextDisabled("선택된 본: (없음)");
		ImGui::Spacing();

		// Drag 필드 색상을 검은색으로 설정
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));

		ImDrawList* DrawList = ImGui::GetWindowDrawList();
		float DummyTransform[3] = { 0.0f, 0.0f, 0.0f };

		// Position
		ImGui::Text("Position");
		ImVec2 PosX = ImGui::GetCursorScreenPos();
		ImGui::SetNextItemWidth(75.0f);
		ImGui::DragFloat("##PosX", &DummyTransform[0], 0.1f, 0.0f, 0.0f, "%.3f");
		ImVec2 SizeX = ImGui::GetItemRectSize();
		DrawList->AddLine(ImVec2(PosX.x + 5, PosX.y + 2), ImVec2(PosX.x + 5, PosX.y + SizeX.y - 2),
		                  IM_COL32(255, 0, 0, 255), 2.0f);
		if (ImGui::IsItemHovered()) { ImGui::SetTooltip("X: %.3f", DummyTransform[0]); }
		ImGui::SameLine();

		ImVec2 PosY = ImGui::GetCursorScreenPos();
		ImGui::SetNextItemWidth(75.0f);
		ImGui::DragFloat("##PosY", &DummyTransform[1], 0.1f, 0.0f, 0.0f, "%.3f");
		ImVec2 SizeY = ImGui::GetItemRectSize();
		DrawList->AddLine(ImVec2(PosY.x + 5, PosY.y + 2), ImVec2(PosY.x + 5, PosY.y + SizeY.y - 2),
		                  IM_COL32(0, 255, 0, 255), 2.0f);
		if (ImGui::IsItemHovered()) { ImGui::SetTooltip("Y: %.3f", DummyTransform[1]); }
		ImGui::SameLine();

		ImVec2 PosZ = ImGui::GetCursorScreenPos();
		ImGui::SetNextItemWidth(75.0f);
		ImGui::DragFloat("##PosZ", &DummyTransform[2], 0.1f, 0.0f, 0.0f, "%.3f");
		ImVec2 SizeZ = ImGui::GetItemRectSize();
		DrawList->AddLine(ImVec2(PosZ.x + 5, PosZ.y + 2), ImVec2(PosZ.x + 5, PosZ.y + SizeZ.y - 2),
		                  IM_COL32(0, 0, 255, 255), 2.0f);
		if (ImGui::IsItemHovered()) { ImGui::SetTooltip("Z: %.3f", DummyTransform[2]); }

		ImGui::Spacing();

		// Rotation
		ImGui::Text("Rotation");
		ImVec2 RotX = ImGui::GetCursorScreenPos();
		ImGui::SetNextItemWidth(75.0f);
		ImGui::DragFloat("##RotX", &DummyTransform[0], 1.0f, 0.0f, 0.0f, "%.3f");
		ImVec2 SizeRotX = ImGui::GetItemRectSize();
		DrawList->AddLine(ImVec2(RotX.x + 5, RotX.y + 2), ImVec2(RotX.x + 5, RotX.y + SizeRotX.y - 2),
		                  IM_COL32(255, 0, 0, 255), 2.0f);
		if (ImGui::IsItemHovered()) { ImGui::SetTooltip("X: %.3f", DummyTransform[0]); }
		ImGui::SameLine();

		ImVec2 RotY = ImGui::GetCursorScreenPos();
		ImGui::SetNextItemWidth(75.0f);
		ImGui::DragFloat("##RotY", &DummyTransform[1], 1.0f, 0.0f, 0.0f, "%.3f");
		ImVec2 SizeRotY = ImGui::GetItemRectSize();
		DrawList->AddLine(ImVec2(RotY.x + 5, RotY.y + 2), ImVec2(RotY.x + 5, RotY.y + SizeRotY.y - 2),
		                  IM_COL32(0, 255, 0, 255), 2.0f);
		if (ImGui::IsItemHovered()) { ImGui::SetTooltip("Y: %.3f", DummyTransform[1]); }
		ImGui::SameLine();

		ImVec2 RotZ = ImGui::GetCursorScreenPos();
		ImGui::SetNextItemWidth(75.0f);
		ImGui::DragFloat("##RotZ", &DummyTransform[2], 1.0f, 0.0f, 0.0f, "%.3f");
		ImVec2 SizeRotZ = ImGui::GetItemRectSize();
		DrawList->AddLine(ImVec2(RotZ.x + 5, RotZ.y + 2), ImVec2(RotZ.x + 5, RotZ.y + SizeRotZ.y - 2),
		                  IM_COL32(0, 0, 255, 255), 2.0f);
		if (ImGui::IsItemHovered()) { ImGui::SetTooltip("Z: %.3f", DummyTransform[2]); }

		ImGui::Spacing();

		// Scale
		ImGui::Text("Scale");
		ImVec2 ScaleX = ImGui::GetCursorScreenPos();
		ImGui::SetNextItemWidth(75.0f);
		ImGui::DragFloat("##ScaleX", &DummyTransform[0], 0.01f, 0.0f, 0.0f, "%.3f");
		ImVec2 SizeScaleX = ImGui::GetItemRectSize();
		DrawList->AddLine(ImVec2(ScaleX.x + 5, ScaleX.y + 2), ImVec2(ScaleX.x + 5, ScaleX.y + SizeScaleX.y - 2),
		                  IM_COL32(255, 0, 0, 255), 2.0f);
		if (ImGui::IsItemHovered()) { ImGui::SetTooltip("X: %.3f", DummyTransform[0]); }
		ImGui::SameLine();

		ImVec2 ScaleY = ImGui::GetCursorScreenPos();
		ImGui::SetNextItemWidth(75.0f);
		ImGui::DragFloat("##ScaleY", &DummyTransform[1], 0.01f, 0.0f, 0.0f, "%.3f");
		ImVec2 SizeScaleY = ImGui::GetItemRectSize();
		DrawList->AddLine(ImVec2(ScaleY.x + 5, ScaleY.y + 2), ImVec2(ScaleY.x + 5, ScaleY.y + SizeScaleY.y - 2),
		                  IM_COL32(0, 255, 0, 255), 2.0f);
		if (ImGui::IsItemHovered()) { ImGui::SetTooltip("Y: %.3f", DummyTransform[1]); }
		ImGui::SameLine();

		ImVec2 ScaleZ = ImGui::GetCursorScreenPos();
		ImGui::SetNextItemWidth(75.0f);
		ImGui::DragFloat("##ScaleZ", &DummyTransform[2], 0.01f, 0.0f, 0.0f, "%.3f");
		ImVec2 SizeScaleZ = ImGui::GetItemRectSize();
		DrawList->AddLine(ImVec2(ScaleZ.x + 5, ScaleZ.y + 2), ImVec2(ScaleZ.x + 5, ScaleZ.y + SizeScaleZ.y - 2),
		                  IM_COL32(0, 0, 255, 255), 2.0f);
		if (ImGui::IsItemHovered()) { ImGui::SetTooltip("Z: %.3f", DummyTransform[2]); }

		ImGui::PopStyleColor(3);
	}

	ImGui::PopStyleColor(3);

	ImGui::Spacing();

	// Gizmo 설정 섹션
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

	// Mesh 정보 섹션
	ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));

	if (ImGui::CollapsingHeader("Mesh Info"))
	{
		ImGui::TextDisabled("Loaded Mesh: (없음)");
		ImGui::TextDisabled("Bones: 0");
		ImGui::TextDisabled("Vertices: 0");
		ImGui::TextDisabled("Triangles: 0");
	}

	ImGui::PopStyleColor(3);

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::TextWrapped("추가 구현 예정:");
	ImGui::BulletText("본 프로퍼티 편집");
	ImGui::BulletText("애니메이션 프리뷰");
	ImGui::BulletText("소켓 편집");
	ImGui::BulletText("LOD 설정");
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
