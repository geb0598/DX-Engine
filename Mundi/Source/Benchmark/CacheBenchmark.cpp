/**
 * CacheBenchmark.cpp
 *
 * Particle memory layout cache efficiency benchmark.
 * Compares: Contiguous Array vs Heap Scattered Pointers.
 *
 * Replicates the exact operations from FParticleEmitterInstance::Tick():
 *   1. ResetParameters      : Velocity = BaseVelocity, RelativeTime += dt * OneOverMaxLifetime
 *   2. AccelerationConstant : BaseVelocity.Z += accel * dt
 *   3. Tick_FinalUpdate     : Location += Velocity * dt, Rotation += RotationRate * dt
 */

#include <vector>
#include <chrono>
#include <random>
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cassert>

// -------------------------------------------------------------------------
// Matches FBaseParticle layout exactly (9 x 16-byte blocks = 144 bytes)
// -------------------------------------------------------------------------
struct alignas(16) Particle
{
    float OldLocation[3];    float _pad0;
    float Location[3];       float _pad1;
    float BaseVelocity[3];   float Rotation;
    float Velocity[3];       float BaseRotationRate;
    float BaseSize[3];       float RotationRate;
    float Size[3];           int   Flags;
    float Color[4];
    float BaseColor[4];
    float RelativeTime;
    float OneOverMaxLifetime;
    float SubImageIndex;
    float _pad2;
};
static_assert(sizeof(Particle) == 144, "Particle size must match FBaseParticle (144 bytes)");

// -------------------------------------------------------------------------
// Same operations as the real particle tick
// -------------------------------------------------------------------------
inline void UpdateParticle(Particle& p, float dt, float accelZ)
{
    // 1. ResetParameters
    p.Velocity[0]   = p.BaseVelocity[0];
    p.Velocity[1]   = p.BaseVelocity[1];
    p.Velocity[2]   = p.BaseVelocity[2];
    p.Size[0]       = p.BaseSize[0];
    p.Size[1]       = p.BaseSize[1];
    p.Size[2]       = p.BaseSize[2];
    p.RotationRate  = p.BaseRotationRate;
    p.RelativeTime += p.OneOverMaxLifetime * dt;

    // 2. AccelerationConstant
    p.BaseVelocity[2] += accelZ * dt;
    p.Velocity[2]     += accelZ * dt;

    // 3. Tick_FinalUpdate
    p.OldLocation[0] = p.Location[0];
    p.OldLocation[1] = p.Location[1];
    p.OldLocation[2] = p.Location[2];
    p.Location[0]   += p.Velocity[0] * dt;
    p.Location[1]   += p.Velocity[1] * dt;
    p.Location[2]   += p.Velocity[2] * dt;
    p.Rotation      += p.RotationRate * dt;
}

static void InitParticle(Particle& p, int index)
{
    float v = (float)index * 0.001f + 0.1f;
    p.BaseVelocity[0]    = v;
    p.BaseVelocity[1]    = v * 0.5f;
    p.BaseVelocity[2]    = v * 2.0f;
    p.Velocity[0]        = p.BaseVelocity[0];
    p.Velocity[1]        = p.BaseVelocity[1];
    p.Velocity[2]        = p.BaseVelocity[2];
    p.Location[0]        = v * 0.1f;
    p.Location[1]        = v * 0.2f;
    p.Location[2]        = v * 0.3f;
    p.OldLocation[0]     = p.Location[0];
    p.OldLocation[1]     = p.Location[1];
    p.OldLocation[2]     = p.Location[2];
    p.BaseSize[0]        = 1.0f;
    p.BaseSize[1]        = 1.0f;
    p.BaseSize[2]        = 1.0f;
    p.Size[0]            = 1.0f;
    p.Size[1]            = 1.0f;
    p.Size[2]            = 1.0f;
    p.BaseRotationRate   = v * 0.01f;
    p.RotationRate       = p.BaseRotationRate;
    p.Rotation           = 0.0f;
    p.RelativeTime       = 0.0f;
    p.OneOverMaxLifetime = 0.2f;
    p.SubImageIndex      = 0.0f;
    p.Flags              = 0;
    p.Color[0] = 1.0f; p.Color[1] = 0.5f;
    p.Color[2] = 0.0f; p.Color[3] = 0.8f;
    p.BaseColor[0] = p.Color[0]; p.BaseColor[1] = p.Color[1];
    p.BaseColor[2] = p.Color[2]; p.BaseColor[3] = p.Color[3];
    p._pad0 = p._pad1 = p._pad2 = 0.0f;
}

using Clock = std::chrono::high_resolution_clock;

// -------------------------------------------------------------------------
// Contiguous array benchmark (current particle system layout)
// -------------------------------------------------------------------------
static double BenchContiguous(int N, int iterations)
{
    std::vector<Particle> particles(N);
    for (int i = 0; i < N; ++i)
        InitParticle(particles[i], i);

    const float dt     = 0.016f;
    const float accelZ = -9.8f;
    volatile float sink = 0.0f;

    for (int i = 0; i < N; ++i)
        UpdateParticle(particles[i], dt, accelZ);

    auto t0 = Clock::now();
    for (int iter = 0; iter < iterations; ++iter)
    {
        for (int i = 0; i < N; ++i)
            UpdateParticle(particles[i], dt, accelZ);
        sink = particles[0].Location[2];
    }
    auto t1 = Clock::now();

    (void)sink;
    return std::chrono::duration<double, std::milli>(t1 - t0).count() / iterations;
}

// -------------------------------------------------------------------------
// Heap scattered benchmark (each particle individually new-ed, then shuffled)
// -------------------------------------------------------------------------
static double BenchScattered(int N, int iterations)
{
    std::vector<Particle*> particles(N);
    for (int i = 0; i < N; ++i)
    {
        particles[i] = new Particle();
        InitParticle(*particles[i], i);
    }

    std::mt19937 rng(42);
    std::shuffle(particles.begin(), particles.end(), rng);

    const float dt     = 0.016f;
    const float accelZ = -9.8f;
    volatile float sink = 0.0f;

    for (int i = 0; i < N; ++i)
        UpdateParticle(*particles[i], dt, accelZ);

    auto t0 = Clock::now();
    for (int iter = 0; iter < iterations; ++iter)
    {
        for (int i = 0; i < N; ++i)
            UpdateParticle(*particles[i], dt, accelZ);
        sink = particles[0]->Location[2];
    }
    auto t1 = Clock::now();

    (void)sink;
    for (int i = 0; i < N; ++i)
        delete particles[i];

    return std::chrono::duration<double, std::milli>(t1 - t0).count() / iterations;
}

// -------------------------------------------------------------------------
int main()
{
    printf("=======================================================\n");
    printf("  Particle Memory Layout Cache Efficiency Benchmark\n");
    printf("  Contiguous Array vs Heap Scattered (shuffled)\n");
    printf("  FBaseParticle identical ops (144B/particle)\n");
    printf("=======================================================\n\n");

    printf("%-10s  %16s  %16s  %10s\n",
        "Count", "Contiguous (ms)", "Scattered (ms)", "Speedup");
    printf("%-10s  %16s  %16s  %10s\n",
        "----------", "----------------", "----------------", "--------");

    struct TestCase { int count; int iters; };
    TestCase cases[] = {
        {   1000, 500 },
        {   5000, 200 },
        {  10000, 100 },
        {  30000,  30 },
        {  50000,  20 },
        { 100000,  10 },
    };

    for (int c = 0; c < 6; ++c)
    {
        int N     = cases[c].count;
        int iters = cases[c].iters;

        double contiguous  = BenchContiguous(N, iters);
        double scattered  = BenchScattered(N, iters);
        double ratio = scattered / contiguous;

        printf("%-10d  %16.3f  %16.3f  %9.2fx\n", N, contiguous, scattered, ratio);
    }

    printf("\nDone.\n");
    return 0;
}
