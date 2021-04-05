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

    const float radius_of_concern = 1.0f;
    for (unsigned int i = 0; i < launch_dim; i++) {
      if (i != launch_index) {
        float3 away_vec = p.position - particles_buffer[i].position;
        float dist      = optix::length(away_vec);

        if (radius_of_concern < dist) {
          continue;
        }

        p.velocity += dt * away_vec / (dist * dist * M_PIf);
      }
    }

    if (p.position.x <= x_min + PARTICLE_RADIUS) {
      p.position.x = x_min + PARTICLE_RADIUS;
      p.velocity.x *= -0.8f;
    }
    else if (p.position.x >= x_max - PARTICLE_RADIUS) {
      p.position.x = x_max - PARTICLE_RADIUS;
      p.velocity.x *= -0.8f;
    }

    if (p.position.z <= z_min + PARTICLE_RADIUS) {
      p.position.z = z_min + PARTICLE_RADIUS;
      p.velocity.z *= -0.8f;
    }
    else if (p.position.z >= z_max - PARTICLE_RADIUS) {
      p.position.z = z_max - PARTICLE_RADIUS;
      p.velocity.z *= -0.8f;
    }

    if (p.position.y <= y_min + PARTICLE_RADIUS) {
      p.position.y = y_min + PARTICLE_RADIUS;
      // p.velocity.y = 0.0f;
      p.velocity.y *= -0.7f;
    }
}