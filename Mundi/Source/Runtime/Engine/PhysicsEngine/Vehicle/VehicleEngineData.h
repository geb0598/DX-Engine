#pragma once

#include "FVehicleEngineData.generated.h"

USTRUCT()
struct FVehicleEngineData
{
    GENERATED_REFLECTION_BODY()
    
    /** 최대 RPM */
    float MaxRPM = 6000.0f;
    /** 최대 토크 (Nm) */
    float MaxTorque = 600.0f;
    /** 엔진 관성 (kg*m^2) */
    float MOI = 1.0f;
    /** 감속 계수 (악셀 뗐을 때) */
    float DampingRateFullThrottle = 0.15f;
    float DampingRateZeroThrottleClutchEngaged = 2.0f;
    float DampingRateZeroThrottleClutchDisengaged = 0.35f;

    // PhysX 데이터 변환
    PxVehicleEngineData GetPxEngineData() const
    {
        PxVehicleEngineData data;
        data.mPeakTorque                              = MaxTorque;
        data.mMaxOmega                                = MaxRPM * (PxPi / 30.0f); // RPM -> rad/s 변환 (RPM * Pi / 30)
        data.mMOI                                     = MOI;
        data.mDampingRateFullThrottle                 = DampingRateFullThrottle;
        data.mDampingRateZeroThrottleClutchEngaged    = DampingRateZeroThrottleClutchEngaged;
        data.mDampingRateZeroThrottleClutchDisengaged = DampingRateZeroThrottleClutchDisengaged;
        
        // 토크 커브 (하드 코딩)
        data.mTorqueCurve.addPair(0.0f, 0.8f);   
        data.mTorqueCurve.addPair(0.33f, 1.0f);  
        data.mTorqueCurve.addPair(1.0f, 0.8f);   
        
        return data;
    }
};
