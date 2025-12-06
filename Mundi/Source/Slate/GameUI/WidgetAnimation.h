#pragma once

/**
 * @file WidgetAnimation.h
 * @brief 위젯 애니메이션 시스템
 *
 * Fade, Scale 등의 기본 애니메이션을 제공합니다.
 */

#include "SlateTypes.h"
#include <functional>

// =====================================================
// 이징 함수 타입
// =====================================================

enum class EEasingType
{
	Linear,			// 선형
	EaseInQuad,		// 천천히 시작
	EaseOutQuad,	// 천천히 끝
	EaseInOutQuad,	// 천천히 시작하고 끝
	EaseInCubic,	// 더 천천히 시작
	EaseOutCubic,	// 더 천천히 끝
	EaseInOutCubic	// 더 천천히 시작하고 끝
};

// =====================================================
// 이징 함수
// =====================================================

class FEasing
{
public:
	static float Apply(float t, EEasingType Type)
	{
		switch (Type)
		{
		case EEasingType::Linear:
			return t;

		case EEasingType::EaseInQuad:
			return t * t;

		case EEasingType::EaseOutQuad:
			return t * (2.0f - t);

		case EEasingType::EaseInOutQuad:
			return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;

		case EEasingType::EaseInCubic:
			return t * t * t;

		case EEasingType::EaseOutCubic:
		{
			float f = t - 1.0f;
			return f * f * f + 1.0f;
		}

		case EEasingType::EaseInOutCubic:
			return t < 0.5f ? 4.0f * t * t * t : (t - 1.0f) * (2.0f * t - 2.0f) * (2.0f * t - 2.0f) + 1.0f;

		default:
			return t;
		}
	}
};

// =====================================================
// 기본 애니메이션 클래스
// =====================================================

class FWidgetAnimation
{
public:
	FWidgetAnimation() = default;
	virtual ~FWidgetAnimation() = default;

	/**
	 * 애니메이션 업데이트
	 * @param DeltaTime 델타 타임
	 * @return 애니메이션이 완료되었으면 true
	 */
	virtual bool Update(float DeltaTime)
	{
		if (!bIsPlaying)
			return true;

		ElapsedTime += DeltaTime;
		float t = FMath::Clamp(ElapsedTime / Duration, 0.0f, 1.0f);

		// 이징 적용
		float EasedT = FEasing::Apply(t, EasingType);

		// 실제 애니메이션 적용
		ApplyAnimation(EasedT);

		// 완료 체크
		if (ElapsedTime >= Duration)
		{
			bIsPlaying = false;
			if (OnCompleted)
				OnCompleted();
			return true;
		}

		return false;
	}

	/** 애니메이션 시작 */
	void Play()
	{
		bIsPlaying = true;
		ElapsedTime = 0.0f;
	}

	/** 애니메이션 중지 */
	void Stop()
	{
		bIsPlaying = false;
	}

	/** 애니메이션 재설정 */
	void Reset()
	{
		ElapsedTime = 0.0f;
		bIsPlaying = false;
	}

	/** 재생 중인지 확인 */
	bool IsPlaying() const { return bIsPlaying; }

	/** 지속 시간 설정 */
	void SetDuration(float InDuration) { Duration = InDuration; }

	/** 이징 타입 설정 */
	void SetEasingType(EEasingType InType) { EasingType = InType; }

	/** 완료 콜백 설정 */
	void SetOnCompleted(std::function<void()> Callback) { OnCompleted = Callback; }

protected:
	/** 실제 애니메이션 적용 (서브클래스에서 구현) */
	virtual void ApplyAnimation(float t) = 0;

	float Duration = 1.0f;
	float ElapsedTime = 0.0f;
	bool bIsPlaying = false;
	EEasingType EasingType = EEasingType::Linear;
	std::function<void()> OnCompleted;
};

// =====================================================
// Fade 애니메이션
// =====================================================

class FFadeAnimation : public FWidgetAnimation
{
public:
	FFadeAnimation(float* InOpacityPtr, float InFrom, float InTo, float InDuration)
		: OpacityPtr(InOpacityPtr), From(InFrom), To(InTo)
	{
		Duration = InDuration;
	}

protected:
	void ApplyAnimation(float t) override
	{
		if (OpacityPtr)
		{
			*OpacityPtr = FMath::Lerp(From, To, t);
		}
	}

private:
	float* OpacityPtr = nullptr;
	float From = 0.0f;
	float To = 1.0f;
};

// =====================================================
// Scale 애니메이션
// =====================================================

class FScaleAnimation : public FWidgetAnimation
{
public:
	FScaleAnimation(FVector2D* InScalePtr, FVector2D InFrom, FVector2D InTo, float InDuration)
		: ScalePtr(InScalePtr), From(InFrom), To(InTo)
	{
		Duration = InDuration;
	}

protected:
	void ApplyAnimation(float t) override
	{
		if (ScalePtr)
		{
			ScalePtr->X = FMath::Lerp(From.X, To.X, t);
			ScalePtr->Y = FMath::Lerp(From.Y, To.Y, t);
		}
	}

private:
	FVector2D* ScalePtr = nullptr;
	FVector2D From = FVector2D(1.0f, 1.0f);
	FVector2D To = FVector2D(1.0f, 1.0f);
};

// =====================================================
// Position 애니메이션
// =====================================================

class FPositionAnimation : public FWidgetAnimation
{
public:
	FPositionAnimation(FVector2D* InPositionPtr, FVector2D InFrom, FVector2D InTo, float InDuration)
		: PositionPtr(InPositionPtr), From(InFrom), To(InTo)
	{
		Duration = InDuration;
	}

protected:
	void ApplyAnimation(float t) override
	{
		if (PositionPtr)
		{
			PositionPtr->X = FMath::Lerp(From.X, To.X, t);
			PositionPtr->Y = FMath::Lerp(From.Y, To.Y, t);
		}
	}

private:
	FVector2D* PositionPtr = nullptr;
	FVector2D From = FVector2D(0.0f, 0.0f);
	FVector2D To = FVector2D(0.0f, 0.0f);
};
