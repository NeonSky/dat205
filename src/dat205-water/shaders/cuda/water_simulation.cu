#include "common.cuh"

rtDeclareVariable(uint, launch_index, rtLaunchIndex, );
rtDeclareVariable(uint, launch_dim,   rtLaunchDim, );

rtDeclareVariable(float, dt    , , ); // Delta time
rtDeclareVariable(float, g     , , ); // Gravity acceleration
rtDeclareVariable(float, y_min , , ); // The floor's y-level
rtDeclareVariable(float, x_min , , ); // Left wall
rtDeclareVariable(float, x_max , , ); // Right wall
rtDeclareVariable(float, z_min , , ); // Far wall
rtDeclareVariable(float, z_max , , ); // Near wall

// Simulated particles.
rtBuffer<Particle> particles_buffer;


RT_PROGRAM void update() {
    // rtPrintf("Launch index %d \n", launch_index);
    Particle& p = particles_buffer[launch_index];

    p.position += dt * p.velocity;
    p.velocity.y += dt * g;

    if (p.position.x <= x_min + PARTICLE_RADIUS) {
      p.position.x = x_min + PARTICLE_RADIUS;
      p.velocity.x *= -1.0f;
    }
    else if (p.position.x >= x_max - PARTICLE_RADIUS) {
      p.position.x = x_max - PARTICLE_RADIUS;
      p.velocity.x *= -1.0f;
    }

    if (p.position.z <= z_min + PARTICLE_RADIUS) {
      p.position.z = z_min + PARTICLE_RADIUS;
      p.velocity.z *= -1.0f;
    }
    else if (p.position.z >= z_max - PARTICLE_RADIUS) {
      p.position.z = z_max - PARTICLE_RADIUS;
      p.velocity.z *= -1.0f;
    }

    if (p.position.y <= y_min + PARTICLE_RADIUS) {
      p.position.y = y_min + PARTICLE_RADIUS;
      // p.velocity.y = 0.0f;
      p.velocity.y *= -0.7f;
    }
}