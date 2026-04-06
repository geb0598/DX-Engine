/**
 * @file SummedAreaTextureFilter.hlsl
 * @brief Summed Area Table 계산용 정수 기반 Hillis-Steele 병렬 누적 합 (Inclusive Scan)
 *
 * @details
 * 정수(uint32) 덧셈은 반올림 오차가 없으므로 Two-Sum/Double-Float 기법이 불필요하다.
 * SAT 빌드 과정 전체가 비트 정확(bit-exact)하며, 4-corner 쿼리도 정수 연산으로 수행한다.
 *
 * 인코딩 방식
 * - Row pass 입력: float2 shadow map (depth, depth²+bias) 포맷
 * - 변환: float_val → uint(float_val * DEPTH_SCALE)
 * - 결과: 정수 SAT 값을 asfloat() 비트 재해석으로 float 텍스처에 저장
 * - 복원: UberLit.hlsl에서 asuint()로 비트 재해석 → 정수 4-corner 쿼리 → float 복원
 *
 * 오버플로우 조건
 *   N² × DEPTH_SCALE < 2^32 = 4,294,967,296
 *   DEPTH_SCALE = 4095: N ≤ 1024 안전  (1024² × 4095 = 4,293,918,720 < 2^32 ✓)
 *
 * 패스 구조
 * 1. ROW SCAN (매크로 미정의 시 - Default)
 *    - 입력: float shadow map → 정수 변환 후 행 방향 prefix sum
 *    - C++ Dispatch: Dispatch(1, RegionHeight, 1)
 *
 * 2. COLUMN SCAN (SCAN_DIRECTION_COLUMN 매크로 정의 시)
 *    - 입력: Row pass가 기록한 정수 SAT (asuint 재해석) → 열 방향 prefix sum
 *    - C++ Dispatch: Dispatch(RegionWidth, 1, 1)
 *
 * @author geb0598
 * @date 2025-10-26
 */

// 스레드 그룹 당 스레드 개수 (1D)
#define THREAD_BLOCK_SIZE 1024

// float [0, 1] → uint [0, DEPTH_SCALE] 인코딩 스케일.
// N² × DEPTH_SCALE < 2^32 을 만족해야 오버플로우 없음.
// N = 1024 기준: DEPTH_SCALE ≤ 4095 (12-bit 정밀도, 오차 ≈ 0.000244)
#define DEPTH_SCALE 4095u

// 입력: float 포맷 텍스처
//   Row  pass → 원본 shadow map (float2 depth 인코딩)
//   Col  pass → Row pass 출력 (uint 비트를 float로 재해석한 값)
Texture2D<float2> InputTexture : register(t0);

// 출력: uint 비트를 asfloat()로 재해석하여 float 텍스처에 저장
RWTexture2D<float2> OutputTexture : register(u0);

// 정수형 공유 메모리: 1024 × 8 bytes = 8 KB (< 32 KB 한도) ✓
groupshared uint2 SharedInt[THREAD_BLOCK_SIZE];

cbuffer TextureInfo : register(b0)
{
    uint RegionStartX;
    uint RegionStartY;
    uint RegionWidth;
    uint RegionHeight;
    uint TextureWidth;
    uint TextureHeight;
    uint DownsampleFactor;
    uint Padding;
}

[numthreads(THREAD_BLOCK_SIZE, 1, 1)]
void mainCS(
    uint3 GroupID       : SV_GroupID,
    uint3 GroupThreadID : SV_GroupThreadID,
    uint  GroupIndex    : SV_GroupIndex
    )
{
    // --- 1. 인덱스 계산 ---
    uint ThreadIndex = GroupIndex;

    uint InputRow, InputColumn;
    uint OutputRow, OutputColumn;
    uint MaxElementsInThisPass;

#ifdef SCAN_DIRECTION_COLUMN
    // Column Scan: 1 스레드 그룹 = 1 열
    InputRow             = ThreadIndex + RegionStartY;
    InputColumn          = GroupID.x   + RegionStartX;
    OutputRow            = InputRow;
    OutputColumn         = InputColumn;
    MaxElementsInThisPass = RegionHeight / DownsampleFactor;

#else
    // Row Scan: 1 스레드 그룹 = 1 행
    InputRow             = GroupID.y * DownsampleFactor + RegionStartY;
    InputColumn          = ThreadIndex * DownsampleFactor + RegionStartX;
    OutputRow            = GroupID.y  + RegionStartY;
    OutputColumn         = ThreadIndex + RegionStartX;
    MaxElementsInThisPass = RegionWidth / DownsampleFactor;
#endif

    // --- 2. 데이터 로드 → 정수 변환 ---
    uint2 IntValue = uint2(0u, 0u);

    if (ThreadIndex < MaxElementsInThisPass)
    {
        float2 FloatValue = InputTexture[uint2(InputColumn, InputRow)];

#ifdef SCAN_DIRECTION_COLUMN
        // Column pass: Row pass가 기록한 uint 비트를 float로 재해석한 값
        IntValue = asuint(FloatValue);
#else
        // Row pass: 원본 shadow map float2 → uint2 인코딩
        IntValue = uint2(round(FloatValue * float(DEPTH_SCALE)));
#endif
    }

    SharedInt[ThreadIndex] = IntValue;
    GroupMemoryBarrierWithGroupSync();

    // --- 3. 정수 Hillis-Steele 누적 합 ---
    for (uint Stride = 1u; Stride < THREAD_BLOCK_SIZE; Stride <<= 1u)
    {
        uint2 Neighbor = uint2(0u, 0u);

        if (ThreadIndex >= Stride)
        {
            Neighbor = SharedInt[ThreadIndex - Stride];
        }

        GroupMemoryBarrierWithGroupSync();

        SharedInt[ThreadIndex] += Neighbor;

        GroupMemoryBarrierWithGroupSync();
    }

    // --- 4. uint SAT → asfloat() 비트 재해석으로 float 텍스처에 저장 ---
    // UberLit.hlsl에서 asuint()로 복원하여 4-corner 쿼리를 수행한다.
    if (ThreadIndex < MaxElementsInThisPass)
    {
        OutputTexture[uint2(OutputColumn, OutputRow)] = asfloat(SharedInt[ThreadIndex]);
    }
}
