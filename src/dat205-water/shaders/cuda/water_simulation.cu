#include "common.cuh"

rtDeclareVariable(uint, launch_index, rtLaunchIndex, );
rtDeclareVariable(uint, launch_dim,   rtLaunchDim, );

rtDeclareVariable(float, dt, , ); // Delta time
rtDeclareVariable(float, g , , ); // Gravity acceleration

// Simulated particles.
rtBuffer<Particle> particles_buffer;


RT_PROGRAM void update() {
    // rtPrintf("Launch index %d \n", launch_index);

    particles_buffer[launch_index].position += make_float3(dt);

    const float ratio = (float)launch_index / (float)launch_dim;
    particles_buffer[launch_index].velocity = make_float3(ratio);
}