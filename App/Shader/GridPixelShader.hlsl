cbuffer CameraData : register(b1)
{
    float fadeDistance; // 최대 fade 거리
    float2 screenSize; // 화면 크기 (width, height)
    float3 gridColor; // grid 색상
    row_major float4x4 invViewProj; // 역뷰프로젝션 행렬
};

float4 GridPS(float4 screenPos : SV_POSITION) : SV_Target
{
    // ------------------------
    // 1. 화면 좌표 -> NDC [-1,1]
    // ------------------------
    float2 screenUV;
    screenUV.x = 2.0 * screenPos.x / screenSize.x - 1.0; // -1~1
    screenUV.y = 1.0 - 2.0 * screenPos.y / screenSize.y;
    
    // ------------------------
    // 2. World Position & Ray Direction 계산
    // ------------------------
    float4 worldNear = mul(float4(screenUV, 0.0, 1.0), invViewProj);
    float4 worldFar = mul(float4(screenUV, 1.0, 1.0), invViewProj);
    
    worldNear /= worldNear.w;
    worldFar /= worldFar.w;
    
    float3 rayOrigin = worldNear.xyz;
    float3 rayDir = normalize((worldFar - worldNear).xyz);
    
    // ------------------------
    // 3. Plane(y=0) intersection
    // ------------------------
    if (abs(rayDir.y) < 1e-6)
        discard; // 평면과 평행하면 제외
    
    float t = -rayOrigin.y / rayDir.y;
    if (t < 0)
        discard; // 카메라 뒤쪽이면 제외
    
    
    float3 hitPos = rayOrigin + t * rayDir;
    
    // ------------------------
    // 4. Grid 계산
    // ------------------------
    float gridSize = 1.0;
    float2 gridCoord = hitPos.xz / gridSize;
    
    // 그리드 선 계산 (derivative 사용)
    float2 grid = abs(frac(gridCoord)) / (max(fwidth(gridCoord), 1e-6));
    float gridLine = saturate(1.0 - min(grid.x, grid.y));
    
    // 대안: step 함수 사용 (더 선명한 선)
    // float2 gridStep = step(0.95, 1.0 - abs(frac(gridCoord) - 0.5) * 2.0);
    // float gridLine = max(gridStep.x, gridStep.y);
    
    // ------------------------
    // 5. Fade 계산
    // ------------------------
    float distanceFromCamera = length(hitPos - rayOrigin);
    float fade = saturate(1.0 - smoothstep(0.0, fadeDistance, distanceFromCamera));
    
    // 카메라 근처에서도 페이드 적용 (선택사항)
    // float nearFade = smoothstep(1.0, 5.0, distanceFromCamera);
    // fade *= nearFade;

    // ------------------------
    // 6. 최종 색상
    // ------------------------
    float alpha = gridLine * fade;
    
    // 투명한 픽셀은 버리기
    if (alpha < 0.01)
        discard;
    
    float axisRange = 3e-3 * distanceFromCamera;
    
    float3 xAxis = float3(1.0, 0.0, 0.0);
    float3 zAxis = float3(0.0, 0.0, 1.0);
    
    // color is red when x axis
    if (dot(hitPos, zAxis) > -axisRange && dot(hitPos, zAxis) < axisRange)
        return float4(1.0, 0.0, 0.0, alpha);
    
    // color is blue when z axis
    if (dot(hitPos, xAxis) > -axisRange && dot(hitPos, xAxis) < axisRange)
        return float4(0.0, 0.0, 1.0, alpha);
    
    return float4(gridColor.rgb, alpha);
}