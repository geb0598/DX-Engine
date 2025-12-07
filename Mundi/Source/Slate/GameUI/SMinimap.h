#pragma once

/**
 * @file SMinimap.h
 * @brief 미니맵 위젯
 *
 * 2D 맵 이미지와 플레이어 마커를 표시합니다.
 */

#include "SWidget.h"

/**
 * @class SMinimap
 * @brief 미니맵 위젯 (배경 맵 + 플레이어 마커)
 */
class SMinimap : public SWidget
{
public:
	SMinimap() = default;
	virtual ~SMinimap() = default;

	// =====================================================
	// 렌더링
	// =====================================================

	void Paint(FD2DRenderer& Renderer, const FGeometry& Geometry) override;

	// =====================================================
	// 속성 설정
	// =====================================================

	/**
	 * 맵 텍스처 설정
	 * @param InTexturePath 맵 이미지 경로
	 * @return 체이닝용 참조
	 */
	SMinimap& SetMapTexture(const FWideString& InTexturePath);

	/**
	 * 플레이어 마커 텍스처 설정
	 * @param InTexturePath 마커 이미지 경로
	 * @return 체이닝용 참조
	 */
	SMinimap& SetPlayerMarkerTexture(const FWideString& InTexturePath);

	/**
	 * 월드 좌표 범위 설정
	 * @param InMin 월드 최소 좌표
	 * @param InMax 월드 최대 좌표
	 * @return 체이닝용 참조
	 */
	SMinimap& SetWorldBounds(const FVector& InMin, const FVector& InMax)
	{
		WorldMin = InMin;
		WorldMax = InMax;
		return *this;
	}

	/**
	 * 플레이어 월드 위치 업데이트
	 * @param WorldPosition 플레이어의 월드 좌표
	 */
	void UpdatePlayerPosition(const FVector& WorldPosition);

	/**
	 * 플레이어 회전 업데이트
	 * @param Yaw 플레이어의 Yaw 회전값 (도 단위)
	 */
	void UpdatePlayerRotation(float Yaw)
	{
		PlayerRotation = Yaw;
	}

	/**
	 * 마커 크기 설정
	 * @param InSize 마커 크기 (픽셀)
	 * @return 체이닝용 참조
	 */
	SMinimap& SetMarkerSize(float InSize)
	{
		MarkerSize = InSize;
		return *this;
	}

	/**
	 * 미니맵 줌 레벨 설정 (플레이어 주변 보이는 범위)
	 * @param InZoomLevel 월드 단위 반경 (예: 500 = 플레이어 주변 500 유닛)
	 * @return 체이닝용 참조
	 */
	SMinimap& SetZoomLevel(float InZoomLevel)
	{
		ZoomLevel = InZoomLevel;
		return *this;
	}

private:
	// =====================================================
	// 내부 헬퍼
	// =====================================================

	/** 월드 좌표를 미니맵 로컬 좌표로 변환 */
	FVector2D WorldToMinimap(const FVector& WorldPos, const FVector2D& MinimapSize) const;

	// =====================================================
	// 속성
	// =====================================================

	/** 맵 텍스처 경로 */
	FWideString MapTexturePath;

	/** 맵 비트맵 (Direct2D) */
	ID2D1Bitmap* MapBitmap = nullptr;

	/** 플레이어 마커 텍스처 경로 */
	FWideString PlayerMarkerTexturePath;

	/** 플레이어 마커 비트맵 (Direct2D) */
	ID2D1Bitmap* PlayerMarkerBitmap = nullptr;

	/** 월드 좌표 범위 */
	FVector WorldMin = FVector(-1000.f, -1000.f, 0.f);
	FVector WorldMax = FVector(1000.f, 1000.f, 0.f);

	/** 플레이어 월드 위치 */
	FVector PlayerWorldPos = FVector::Zero();

	/** 플레이어 회전 (Yaw, 도 단위) */
	float PlayerRotation = 0.f;

	/** 마커 크기 */
	float MarkerSize = 16.f;

	/** 줌 레벨 (플레이어 주변 보이는 월드 단위 반경) */
	float ZoomLevel = 500.f;

	/** 초기 플레이어 위치 (한 번만 저장) */
	FVector InitialPlayerPos = FVector::Zero();

	/** 초기 위치 설정 여부 */
	bool bInitialPosSet = false;
};
