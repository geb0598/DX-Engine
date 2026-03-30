#pragma once

#include "insights/insights_d3d11.h"

/*-----------------------------------------------------------------------------
    Stat Groups
 -----------------------------------------------------------------------------*/

INSIGHTS_DECLARE_STATGROUP("Rendering", GStatGroup_Rendering)
INSIGHTS_DECLARE_STATGROUP("LightCulling", GStatGroup_LightCulling)

/*-----------------------------------------------------------------------------
    CPU Stats
 -----------------------------------------------------------------------------*/

// Top-level frame
INSIGHTS_DECLARE_STAT("RenderFrame",          GStat_RenderFrame,          GStatGroup_Rendering)

// Render passes
INSIGHTS_DECLARE_STAT("RenderScene",          GStat_RenderScene,          GStatGroup_Rendering)
INSIGHTS_DECLARE_STAT("RenderSceneDepthPass", GStat_RenderSceneDepthPass, GStatGroup_Rendering)
INSIGHTS_DECLARE_STAT("CullLights",           GStat_CullLights,           GStatGroup_LightCulling)
INSIGHTS_DECLARE_STAT("RenderBasePass",       GStat_RenderBasePass,       GStatGroup_Rendering)
INSIGHTS_DECLARE_STAT("RenderFogPass",        GStat_RenderFogPass,        GStatGroup_Rendering)
INSIGHTS_DECLARE_STAT("RenderOverlayPass",    GStat_RenderOverlayPass,    GStatGroup_Rendering)

/*-----------------------------------------------------------------------------
    GPU Stats
 -----------------------------------------------------------------------------*/

INSIGHTS_DECLARE_STAT("GPU_RenderScene",          GStat_GPU_RenderScene,          GStatGroup_Rendering)
INSIGHTS_DECLARE_STAT("GPU_RenderSceneDepthPass", GStat_GPU_RenderSceneDepthPass, GStatGroup_Rendering)
INSIGHTS_DECLARE_STAT("GPU_CullLights",           GStat_GPU_CullLights,           GStatGroup_LightCulling)
INSIGHTS_DECLARE_STAT("GPU_RenderBasePass",       GStat_GPU_RenderBasePass,       GStatGroup_Rendering)
INSIGHTS_DECLARE_STAT("GPU_RenderFogPass",        GStat_GPU_RenderFogPass,        GStatGroup_Rendering)
