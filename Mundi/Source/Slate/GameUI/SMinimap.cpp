#include "pch.h"
#include "SMinimap.h"
#include "SGameHUD.h"
#include <d2d1.h>

#define PI 3.14159265359f

void SMinimap::Paint(FD2DRenderer& Renderer, const FGeometry& Geometry)
{
	if (!IsVisible())
		return;

	FVector2D Position = Geometry.AbsolutePosition;
	FVector2D Size = Geometry.GetAbsoluteSize();

	// 배경 (검은색 원형)
	ID2D1DeviceContext* Context = Renderer.GetContext();

	// 원형 클리핑 영역 설정
	ID2D1EllipseGeometry* EllipseGeometry = nullptr;
	if (Context)
	{
		ID2D1Factory* Factory = nullptr;
		Context->GetFactory(&Factory);

		if (Factory)
		{
			D2D1_ELLIPSE Ellipse;
			Ellipse.point.x = Position.X + Size.X * 0.5f;
			Ellipse.point.y = Position.Y + Size.Y * 0.5f;
			Ellipse.radiusX = Size.X * 0.5f;
			Ellipse.radiusY = Size.Y * 0.5f;

			Factory->CreateEllipseGeometry(Ellipse, &EllipseGeometry);
			Factory->Release();

			if (EllipseGeometry)
			{
				// 원형 클리핑 적용
				Context->PushLayer(
					D2D1::LayerParameters(
						D2D1::InfiniteRect(),
						EllipseGeometry
					),
					nullptr
				);
			}
		}
	}

	// 배경 원 그리기 (클리핑 테스트용)
	Renderer.DrawFilledRect(Position, Size, FSlateColor(0.1f, 0.1f, 0.1f, 0.9f));

	// 1. 현재 플레이어 위치 중심으로 맵을 크롭해서 그리기
	if (MapBitmap)
	{
		// 맵 비트맵 크기
		D2D1_SIZE_F BitmapSize = MapBitmap->GetSize();

		// 월드 바운드 (4 꼭짓점 기준)
		// 이미지 좌하단(0, 627) = 월드 (224, 172)
		// 이미지 우하단(1497, 627) = 월드 (224, -386)
		// 이미지 좌상단(0, 0) = 월드 (-9.5, 172)
		// 이미지 우상단(1497, 0) = 월드 (-9.5, -386)
		float WorldMinX = -9.5f;
		float WorldMaxX = 224.0f;
		float WorldMinY = -386.0f;
		float WorldMaxY = 172.0f;

		float WorldWidth = WorldMaxX - WorldMinX;   // 233.5
		float WorldHeight = WorldMaxY - WorldMinY;  // 558.0

		// 플레이어 월드 위치를 정규화 (0~1)
		float NormalizedX = (PlayerWorldPos.X - WorldMinX) / WorldWidth;
		float NormalizedY = (PlayerWorldPos.Y - WorldMinY) / WorldHeight;

		// 이미지 좌표계 변환 (이미지 좌상단이 원점 0,0)
		// 이미지에서: 위쪽 = 월드 -X, 오른쪽 = 월드 -Y
		// 이미지 X(가로, 0→1497) = 월드 Y가 감소하는 방향 (172→-386)
		// 이미지 Y(세로, 0→627) = 월드 X가 감소하는 방향 (224→-9.5)
		float ImageX = (1.0f - NormalizedY) * BitmapSize.width;   // 월드 Y 반전 → 이미지 X
		float ImageY = NormalizedX * BitmapSize.height;            // 월드 X → 이미지 Y (반전 제거)

		float CenterX = ImageX;
		float CenterY = ImageY;

		// 정사각형 크롭 크기 (작은 쪽 기준)
		float CropSize = std::min(BitmapSize.width, BitmapSize.height) * 0.5f;

		// 크롭 영역 계산 (현재 플레이어 위치 중심 정사각형)
		FSlateRect SourceRect;
		SourceRect.Left = CenterX - CropSize * 0.5f;
		SourceRect.Top = CenterY - CropSize * 0.5f;
		SourceRect.Right = CenterX + CropSize * 0.5f;
		SourceRect.Bottom = CenterY + CropSize * 0.5f;

		// 범위 제한 (텍스처 밖으로 나가지 않도록)
		SourceRect.Left = std::max(0.f, SourceRect.Left);
		SourceRect.Top = std::max(0.f, SourceRect.Top);
		SourceRect.Right = std::min(BitmapSize.width, SourceRect.Right);
		SourceRect.Bottom = std::min(BitmapSize.height, SourceRect.Bottom);

		// ========== 맵 회전 기능 (회전 비활성화하려면 이 블록 전체를 주석처리) ==========
		// 맵 회전을 위한 Transform 설정
		/*ID2D1DeviceContext* Context = Renderer.GetContext();
		if (Context)
		{
			// 현재 Transform 저장
			D2D1::Matrix3x2F OldTransform;
			Context->GetTransform(&OldTransform);

			// 미니맵 중심점 계산
			D2D1_POINT_2F Center = D2D1::Point2F(
				Position.X + Size.X * 0.5f,
				Position.Y + Size.Y * 0.5f
			);

			// 회전 Transform 생성 (플레이어 회전의 반대 방향)
			// PlayerRotation은 Yaw 각도 (도 단위)
			D2D1::Matrix3x2F RotationTransform =
				D2D1::Matrix3x2F::Rotation(-PlayerRotation, Center);

			// Transform 적용
			Context->SetTransform(RotationTransform * OldTransform);

			// 크롭된 맵 그리기 (회전 적용됨)
			Renderer.DrawImage(
				MapBitmap,
				Position,
				Size,
				FSlateColor::White(),
				1.0f,
				SourceRect
			);

			// Transform 복원
			Context->SetTransform(OldTransform);
		}
		else
		{
			// Context가 없으면 회전 없이 그리기
			Renderer.DrawImage(
				MapBitmap,
				Position,
				Size,
				FSlateColor::White(),
				1.0f,
				SourceRect
			);
		}*/
		// ========== 맵 회전 기능 끝 ==========

		// ========== 회전 없는 버전 (회전 기능을 끄려면 위 블록을 주석처리하고 이 주석을 해제) ==========
		
		// 크롭된 맵 그리기 (회전 없음)
		Renderer.DrawImage(
			MapBitmap,
			Position,
			Size,
			FSlateColor::White(),
			1.0f,
			SourceRect
		);
		
		// ========== 회전 없는 버전 끝 ==========
	}

	// 2. 플레이어 마커는 항상 미니맵 중앙에 고정
	FVector2D MarkerPos;
	MarkerPos.X = Position.X + Size.X * 0.5f - MarkerSize * 0.5f;
	MarkerPos.Y = Position.Y + Size.Y * 0.5f - MarkerSize * 0.5f;

	if (PlayerMarkerBitmap)
	{
		// 플레이어 마커 회전 적용
		if (Context)
		{
			// 현재 Transform 저장
			D2D1::Matrix3x2F OldTransform;
			Context->GetTransform(&OldTransform);

			// 마커 중심점 계산
			D2D1_POINT_2F MarkerCenter = D2D1::Point2F(
				MarkerPos.X + MarkerSize * 0.5f,
				MarkerPos.Y + MarkerSize * 0.5f
			);

			// 회전 각도 계산
			// PlayerRotation: -180° = 월드 -X (이미지 위), 좌회전 시 증가 (-180 → 170 → 20)
			// 이미지 좌표계: 위쪽 = 월드 -X, 오른쪽 = 월드 -Y
			// PlayerRotation이 -180일 때 이미지 0도(위쪽)을 향해야 함
			// 좌회전(PlayerRotation 증가) 시 이미지도 반시계방향으로 회전해야 함
			// 공식: 이미지 각도 = PlayerRotation + 180°
			float MarkerAngle = PlayerRotation + 180.0f;

			// 회전 Transform 생성
			D2D1::Matrix3x2F RotationTransform =
				D2D1::Matrix3x2F::Rotation(MarkerAngle, MarkerCenter);

			// Transform 적용
			Context->SetTransform(RotationTransform * OldTransform);

			// 마커 그리기 (회전 적용됨)
			Renderer.DrawImage(
				PlayerMarkerBitmap,
				MarkerPos,
				FVector2D(MarkerSize, MarkerSize),
				FSlateColor::White(),
				1.0f
			);

			// Transform 복원
			Context->SetTransform(OldTransform);
		}
		else
		{
			// Context가 없으면 회전 없이 그리기
			Renderer.DrawImage(
				PlayerMarkerBitmap,
				MarkerPos,
				FVector2D(MarkerSize, MarkerSize),
				FSlateColor::White(),
				1.0f
			);
		}
	}
	else
	{
		// 마커 이미지가 없으면 빨간 점으로 표시
		Renderer.DrawFilledRect(MarkerPos, FVector2D(MarkerSize, MarkerSize), FSlateColor(1.f, 0.f, 0.f, 1.f));
	}

	// 원형 클리핑 해제
	if (EllipseGeometry)
	{
		Context->PopLayer();
		EllipseGeometry->Release();
	}

	// 링 텍스처 그리기 (미니맵 위에 오버레이)
	if (RingBitmap)
	{
		// 링은 미니맵과 같은 크기로 렌더링
		Renderer.DrawImage(
			RingBitmap,
			Position,
			Size,
			FSlateColor::White(),
			1.0f,
			bUseHighQualityRing  // 고품질 보간 옵션
		);
	}
	else
	{
		// 링 텍스처가 없으면 기본 원형 테두리 그리기
		if (Context)
		{
			ID2D1SolidColorBrush* Brush = nullptr;
			Context->CreateSolidColorBrush(
				D2D1::ColorF(0.3f, 0.3f, 0.3f, 1.0f),
				&Brush
			);

			if (Brush)
			{
				D2D1_ELLIPSE Ellipse;
				Ellipse.point.x = Position.X + Size.X * 0.5f;
				Ellipse.point.y = Position.Y + Size.Y * 0.5f;
				Ellipse.radiusX = Size.X * 0.5f;
				Ellipse.radiusY = Size.Y * 0.5f;

				Context->DrawEllipse(Ellipse, Brush, 2.0f);
				Brush->Release();
			}
		}
	}

	// 3. 플레이어 위치 및 회전 디버그 출력 (미니맵 위에 표시)
	FWideString DebugText = L"Pos: (" +
		std::to_wstring(static_cast<int>(PlayerWorldPos.X)) + L", " +
		std::to_wstring(static_cast<int>(PlayerWorldPos.Y)) + L") Rot: " +
		std::to_wstring(static_cast<int>(PlayerRotation)) + L"°";

	Renderer.DrawText(
		DebugText,
		FVector2D(Position.X, Position.Y - 30.f),  // 미니맵 바로 위
		FVector2D(400.f, 30.f),
		FSlateColor(1.f, 1.f, 0.f, 1.f),  // 노란색
		16.f,
		ETextHAlign::Left,
		ETextVAlign::Top
	);
}

SMinimap& SMinimap::SetMapTexture(const FWideString& InTexturePath)
{
	MapTexturePath = InTexturePath;

	// 기존 비트맵 해제
	if (MapBitmap)
	{
		MapBitmap->Release();
		MapBitmap = nullptr;
	}

	// 새 비트맵 로드
	if (SGameHUD::Get().IsInitialized())
	{
		FD2DRenderer* Renderer = SGameHUD::Get().GetRenderer();
		if (Renderer)
		{
			MapBitmap = Renderer->LoadBitmapFromFile(InTexturePath);
		}
	}

	return *this;
}

SMinimap& SMinimap::SetPlayerMarkerTexture(const FWideString& InTexturePath)
{
	PlayerMarkerTexturePath = InTexturePath;

	// 기존 비트맵 해제
	if (PlayerMarkerBitmap)
	{
		PlayerMarkerBitmap->Release();
		PlayerMarkerBitmap = nullptr;
	}

	// 새 비트맵 로드
	if (SGameHUD::Get().IsInitialized())
	{
		FD2DRenderer* Renderer = SGameHUD::Get().GetRenderer();
		if (Renderer)
		{
			PlayerMarkerBitmap = Renderer->LoadBitmapFromFile(InTexturePath);
		}
	}

	return *this;
}

SMinimap& SMinimap::SetRingTexture(const FWideString& InTexturePath)
{
	RingTexturePath = InTexturePath;

	// 기존 비트맵 해제
	if (RingBitmap)
	{
		RingBitmap->Release();
		RingBitmap = nullptr;
	}

	// 새 비트맵 로드
	if (SGameHUD::Get().IsInitialized())
	{
		FD2DRenderer* Renderer = SGameHUD::Get().GetRenderer();
		if (Renderer)
		{
			RingBitmap = Renderer->LoadBitmapFromFile(InTexturePath);
		}
	}

	return *this;
}

void SMinimap::UpdatePlayerPosition(const FVector& WorldPosition)
{
	// 초기 위치만 한 번 저장 (이후에는 업데이트 안함)
	if (!bInitialPosSet)
	{
		InitialPlayerPos = WorldPosition;
		bInitialPosSet = true;
	}

	// 현재 위치는 마커 표시용으로만 저장
	PlayerWorldPos = WorldPosition;
}

FVector2D SMinimap::WorldToMinimap(const FVector& WorldPos, const FVector2D& MinimapSize) const
{
	float NormalizedX = (WorldPos.X - WorldMin.X) / (WorldMax.X - WorldMin.X);
	float NormalizedY = 1.0f - (WorldPos.Y - WorldMin.Y) / (WorldMax.Y - WorldMin.Y);

	return FVector2D(
		NormalizedX * MinimapSize.X,
		NormalizedY * MinimapSize.Y
	);
}
