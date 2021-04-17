#include "common.cuh"

rtDeclareVariable(uint, launch_index, rtLaunchIndex, );
rtDeclareVariable(uint, launch_dim,   rtLaunchDim, );

rtDeclareVariable(float, dt    , , ); // Delta time [s]
rtDeclareVariable(float, g     , , ); // Gravity acceleration [m/s^2]

rtDeclareVariable(float, cell_size, , ); // [m]
rtDeclareVariable(float, support_radius, , ); // [m]

rtDeclareVariable(float, particle_mass, , );// [kg]
rtDeclareVariable(float, rest_density, , ); // [kg / m^3]
rtDeclareVariable(float, gass_stiffness, , ); // [Pa * m^3 / kg]
rtDeclareVariable(float, viscosity, , ); // [Pa * s]
rtDeclareVariable(float, l_threshold, , ); // []
rtDeclareVariable(float, surface_tension, , ); // [N / m]

rtDeclareVariable(float, y_min , , ); // The floor's y-level
rtDeclareVariable(float, x_min , , ); // Left wall
rtDeclareVariable(float, x_max , , ); // Right wall
rtDeclareVariable(float, z_min , , ); // Far wall
rtDeclareVariable(float, z_max , , ); // Near wall

rtBuffer<HashCell> hash_table;

// Simulated particles.
rtBuffer<Particle> particles_buffer;


// eq 5.1, 5.2, 5.3
RT_FUNCTION uint hash(int3 pos) {
  // Primes
  static const int p1 = 73856093;
  static const int p2 = 19349663;
  static const int p3 = 83492791;

  // Hash
  return ((pos.x * p1) ^ (pos.y * p2) ^ (pos.z * p3)) % hash_table.size();
}

RT_PROGRAM void reset_nearest_neighbors() {
  Particle& p = particles_buffer[launch_index];
  HashCell& cell = hash_table[p.prev_hash_cell_index];
  cell[0] = 0; // only necessary to reset the count
}

RT_PROGRAM void update_nearest_neighbors() {
  Particle& p = particles_buffer[launch_index];

  int3 cell_position = make_int3(p.position / cell_size);
  uint cell_index = hash(cell_position);
  HashCell& cell = hash_table[cell_index];
  p.prev_hash_cell_index = cell_index;

  uint particles_in_cell = cell[0];
  if (particles_in_cell < HASH_CELL_SIZE-1) {
    atomicAdd(&cell[0], 1);
    cell[particles_in_cell+1] = launch_index;
  }
}

RT_FUNCTION void nearest_neighbor_search(Particle& p,
                                         unsigned int& nn_count,
                                         unsigned int* nn) {

  int3 center_cell_position = make_int3(p.position / cell_size);

  // Search for neighbors in the 3x3x3 grid of cells that is centered on this particle's cell.
  for (int x = -1; x <= 1; x++) {
    for (int y = -1; y <= 1; y++) {
      for (int z = -1; z <= 1; z++) {
        int3 cell_position = center_cell_position + make_int3(x, y, z);
        HashCell& cell = hash_table[hash(cell_position)];

        uint particles_in_cell = cell[0];
        for (int i = 1; i <= particles_in_cell; i++) {
          if (cell[i] != launch_index) {
            nn[nn_count] = cell[i];
            nn_count += 1;
          }
        }
      }
    }
  }
}

// eq 4.3
RT_FUNCTION float poly6_kernel(float distance) {
  if (distance >= support_radius) {
    return 0.0f;
  } else {
    return (315.0f / (64.0f * M_PIf * powf(support_radius, 9.0f))) * powf(powf(support_radius, 2.0f) - powf(distance, 2.0f), 3.0f);
  }
}

// eq 4.4
RT_FUNCTION float3 poly6_kernel_gradient(float3 dist_vec) {
  float distance = optix::length(dist_vec);
  if (distance >= support_radius) {
    return make_float3(0.0f);
  } else {
    return -(945.0f / (32.0f * M_PIf * powf(support_radius, 9.0f))) * dist_vec * powf(powf(support_radius, 2.0f) - powf(distance, 2.0f), 2.0f);
  }
}

// eq 4.5
RT_FUNCTION float poly6_kernel_laplacian(float distance) {
  if (distance >= support_radius) {
    return 0.0f;
  } else {
    return -(945.0f / (32.0f * M_PIf * powf(support_radius, 9.0f))) * (powf(support_radius, 2.0f) - powf(distance, 2.0f)) * (3.0f * powf(support_radius, 2.0f) - 7.0f * powf(distance, 2.0f));
  }
}

// eq 4.6
RT_FUNCTION void update_density(Particle& p,
                                unsigned int nn_count,
                                unsigned int* nn) {

  // Density from this particle alone
  float density = particle_mass * poly6_kernel(0.0f);

  // Density from neighbors
  for (int i = 0; i < nn_count; i++) {
    Particle& pi = particles_buffer[nn[i]];
    float distance = optix::length(p.position - pi.position);

    density += particle_mass * poly6_kernel(distance);
  }

  p.density = density;
}

// eq 4.12
RT_FUNCTION void update_pressure(Particle& p,
                                unsigned int nn_count,
                                unsigned int* nn) {
  p.pressure = gass_stiffness * (p.density - rest_density);
}

RT_PROGRAM void update_particles_data() {
  Particle& p = particles_buffer[launch_index];

  // Find nearest neighbors
  unsigned int nn_count = 0;
  unsigned int nn[3 * 3 * 3 * HASH_CELL_SIZE];
  nearest_neighbor_search(p, nn_count, nn);

  // Update density and pressure for each particle
  update_density(p, nn_count, nn);
  update_pressure(p, nn_count, nn);
}

// eq 4.14
RT_FUNCTION float3 pressure_kernel_gradient(float3 dist_vec) {
  float distance = optix::length(dist_vec);
  if (distance >= support_radius) {
    return make_float3(0.0f);
  } else if (distance < 1e-5) {
    // We'll treat this case as the particles being in the same position.
    // Thus, they won't affect each other in any direction.
    // This is mostly to avoid division by zero.
    return make_float3(0.0f);
  } else {
    return -(45.0f / (M_PIf * powf(support_radius, 6.0f))) * (dist_vec / distance) * powf(support_radius - distance, 2.0f);
  }
}

// eq 4.11
RT_FUNCTION float3 pressure_force(Particle& p,
                                  unsigned int nn_count,
                                  unsigned int* nn) {
  float3 force = make_float3(0.0f);
  for (int i = 0; i < nn_count; i++) {
    Particle& pi = particles_buffer[nn[i]];
    float3 dist_vec = p.position - pi.position;

    force += particle_mass * ((p.pressure + pi.pressure) / (2.0f * pi.density)) * pressure_kernel_gradient(dist_vec);
  }
  force *= -1.0f;
  return force;
}

// eq 4.22
RT_FUNCTION float viscosity_kernel_laplacian(float distance) {
  if (distance >= support_radius) {
    return 0.0f;
  } else {
    return (45.0f / (M_PIf * powf(support_radius, 6.0f))) * (support_radius - distance);
  }
}

// eq 4.17
RT_FUNCTION float3 viscosity_force(Particle& p,
                                   unsigned int nn_count,
                                   unsigned int* nn) {
  float3 force = make_float3(0.0f);
  for (int i = 0; i < nn_count; i++) {
    Particle& pi = particles_buffer[nn[i]];

    force += (pi.velocity - p.velocity) * (particle_mass / pi.density) * viscosity_kernel_laplacian(optix::length(pi.position - p.position));
  }
  force *= viscosity;
  return force;
}

// eq 4.24
RT_FUNCTION float3 gravity_force(float particle_density) {
  return particle_density * make_float3(0.0f, g, 0.0f);
}

RT_FUNCTION float3 surface_tension_force(Particle& p,
                                         unsigned int nn_count,
                                         unsigned int* nn) {

  // eq 4.28
  float3 inward_surface_normal = make_float3(0.0f);
  for (int i = 0; i < nn_count; i++) {
    Particle& pi = particles_buffer[nn[i]];
    float distance = optix::length(p.position - pi.position);

    inward_surface_normal += (particle_mass / pi.density) * poly6_kernel_gradient(p.position - pi.position);
  }

  float normal_dist = optix::length(inward_surface_normal);
  if (normal_dist < l_threshold) {
    return make_float3(0.0f);
  }

  // eq 4.26
  float laplacian = 0.0f;
  for (int i = 0; i < nn_count; i++) {
    Particle& pi = particles_buffer[nn[i]];
    float distance = optix::length(p.position - pi.position);

    laplacian += (particle_mass / pi.density) * poly6_kernel_laplacian(distance);
  }
  float3 force = -surface_tension * laplacian * inward_surface_normal / normal_dist;

  return force;
}

RT_FUNCTION void euler_cromer(Particle& p, float3 force) {
    float3 acceleration = force / p.density; // eq 4.2
    p.velocity += dt * acceleration;
    p.position += dt * p.velocity;
}

// p54
RT_PROGRAM void update() {
    Particle& p = particles_buffer[launch_index];

    // Find nearest neighbors
    unsigned int nn_count = 0;
    unsigned int nn[3 * 3 * 3 * HASH_CELL_SIZE];
    nearest_neighbor_search(p, nn_count, nn);

    float3 tot_force = make_float3(0.0f);

    // Internal forces
    tot_force += pressure_force(p, nn_count, nn);
    tot_force += viscosity_force(p, nn_count, nn);

    // External forces
    tot_force += gravity_force(p.density);
    tot_force += surface_tension_force(p, nn_count, nn);

    // Integrate forces over time
    euler_cromer(p, tot_force);

    // TODO: Remove
    if (p.position.x <= x_min + PARTICLE_RADIUS) {
      p.position.x = x_min + PARTICLE_RADIUS;
      p.velocity.x *= -0.5f;
    }
    else if (p.position.x >= x_max - PARTICLE_RADIUS) {
      p.position.x = x_max - PARTICLE_RADIUS;
      p.velocity.x *= -0.5f;
    }

    if (p.position.z <= z_min + PARTICLE_RADIUS) {
      p.position.z = z_min + PARTICLE_RADIUS;
      p.velocity.z *= -0.5f;
    }
    else if (p.position.z >= z_max - PARTICLE_RADIUS) {
      p.position.z = z_max - PARTICLE_RADIUS;
      p.velocity.z *= -0.5f;
    }

    if (p.position.y <= y_min + PARTICLE_RADIUS) {
      p.position.y = y_min + PARTICLE_RADIUS;
      p.velocity.y = fmaxf(p.velocity.y, 0.0f);
      // p.velocity.y *= -0.7f;
    }
}