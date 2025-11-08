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
#include "Core/Public/NewObject.h"
#include "Editor/Public/BatchLines.h"
#include "Render/UI/Widget/Public/SkeletalMeshViewerToolbarWidget.h"

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
		ViewerBatchLines->UpdateUGridVertices(100.0f); // 100 unit 간격으로 그리드 생성
		ViewerBatchLines->UpdateVertexBuffer();
	}

	// 툴바 위젯 생성 및 초기화
	ToolbarWidget = new USkeletalMeshViewerToolbarWidget();
	if (ToolbarWidget)
	{
		ToolbarWidget->Initialize();
		ToolbarWidget->SetViewportClient(ViewerViewportClient);
	}

	bIsInitialized = true;
	bIsCleanedUp = false;

	UE_LOG("SkeletalMeshViewerWindow: 독립적인 뷰포트 및 카메라 초기화 완료");
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
