#pragma once

#include "insights/insights_d3d11.h"

namespace insights { class Group; class Descriptor; }

/*-----------------------------------------------------------------------------
    Stat Groups
 -----------------------------------------------------------------------------*/

extern insights::Group GStatGroup_Particle;
extern insights::Group GStatGroup_Renderer;

/*-----------------------------------------------------------------------------
    CPU - Particle Tick
 -----------------------------------------------------------------------------*/

extern insights::Descriptor GStat_EmitterTick;
extern insights::Descriptor GStat_Tick_SpawnParticles;
extern insights::Descriptor GStat_Tick_ResetParameters;
extern insights::Descriptor GStat_Tick_ModuleUpdate;
extern insights::Descriptor GStat_Tick_KillParticles;
extern insights::Descriptor GStat_Tick_FinalUpdate;

/*-----------------------------------------------------------------------------
    CPU - Particle Render
 -----------------------------------------------------------------------------*/

extern insights::Descriptor GStat_ParticleUpdateDynamicData;
extern insights::Descriptor GStat_ParticleCollectBatches;
extern insights::Descriptor GStat_ParticleRenderPass;

/*-----------------------------------------------------------------------------
    CPU - Renderer Passes
 -----------------------------------------------------------------------------*/

extern insights::Descriptor GStat_RenderScene;
extern insights::Descriptor GStat_RenderGatherProxies;
extern insights::Descriptor GStat_RenderShadowMaps;
extern insights::Descriptor GStat_RenderLitPath;
extern insights::Descriptor GStat_RenderPostProcessing;
extern insights::Descriptor GStat_Present;

/*-----------------------------------------------------------------------------
    GPU - Renderer Passes
 -----------------------------------------------------------------------------*/

extern insights::Descriptor GStat_GPU_ShadowMaps;
extern insights::Descriptor GStat_GPU_LitPath;
extern insights::Descriptor GStat_GPU_ParticlePass;
extern insights::Descriptor GStat_GPU_PostProcessing;
