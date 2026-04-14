#include "pch.h"
#include "InsightsStats.h"

/*-----------------------------------------------------------------------------
    Stat Group Definitions
 -----------------------------------------------------------------------------*/

insights::Group GStatGroup_Particle("Particle");
insights::Group GStatGroup_Renderer("Renderer");

/*-----------------------------------------------------------------------------
    CPU - Particle Tick
 -----------------------------------------------------------------------------*/

insights::Descriptor GStat_EmitterTick            ("EmitterTick",           GStatGroup_Particle);
insights::Descriptor GStat_Tick_SpawnParticles    ("Tick_SpawnParticles",   GStatGroup_Particle);
insights::Descriptor GStat_Tick_ResetParameters   ("Tick_ResetParameters",  GStatGroup_Particle);
insights::Descriptor GStat_Tick_ModuleUpdate      ("Tick_ModuleUpdate",     GStatGroup_Particle);
insights::Descriptor GStat_Tick_KillParticles     ("Tick_KillParticles",    GStatGroup_Particle);
insights::Descriptor GStat_Tick_FinalUpdate       ("Tick_FinalUpdate",      GStatGroup_Particle);

/*-----------------------------------------------------------------------------
    CPU - Particle Render
 -----------------------------------------------------------------------------*/

insights::Descriptor GStat_ParticleUpdateDynamicData ("ParticleUpdateDynamicData", GStatGroup_Particle);
insights::Descriptor GStat_ParticleCollectBatches    ("ParticleCollectBatches",    GStatGroup_Particle);
insights::Descriptor GStat_ParticleRenderPass        ("ParticleRenderPass",        GStatGroup_Particle);

/*-----------------------------------------------------------------------------
    CPU - Renderer Passes
 -----------------------------------------------------------------------------*/

insights::Descriptor GStat_RenderScene        ("RenderScene",        GStatGroup_Renderer);
insights::Descriptor GStat_RenderGatherProxies("RenderGatherProxies",GStatGroup_Renderer);
insights::Descriptor GStat_RenderShadowMaps   ("RenderShadowMaps",   GStatGroup_Renderer);
insights::Descriptor GStat_RenderLitPath      ("RenderLitPath",      GStatGroup_Renderer);
insights::Descriptor GStat_RenderPostProcessing("RenderPostProcessing",GStatGroup_Renderer);
insights::Descriptor GStat_Present            ("Present",            GStatGroup_Renderer);

/*-----------------------------------------------------------------------------
    GPU - Renderer Passes
 -----------------------------------------------------------------------------*/

insights::Descriptor GStat_GPU_ShadowMaps    ("GPU_ShadowMaps",    GStatGroup_Renderer);
insights::Descriptor GStat_GPU_LitPath       ("GPU_LitPath",       GStatGroup_Renderer);
insights::Descriptor GStat_GPU_ParticlePass  ("GPU_ParticlePass",  GStatGroup_Renderer);
insights::Descriptor GStat_GPU_PostProcessing("GPU_PostProcessing",GStatGroup_Renderer);
