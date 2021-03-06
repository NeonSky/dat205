#include "common.cuh"

rtDeclareVariable(uint, launch_index, rtLaunchIndex, );
rtDeclareVariable(uint, launch_dim,   rtLaunchDim, );

rtDeclareVariable(float, dt, , ); // Delta time [s]
rtDeclareVariable(float, g , , ); // Gravity acceleration [m / s^2]

rtDeclareVariable(float, cell_size      , , ); // [m]
rtDeclareVariable(float, support_radius , , ); // [m]
rtDeclareVariable(float, particle_radius, , ); // [m]

rtDeclareVariable(float, particle_mass  , , ); // [kg]
rtDeclareVariable(float, rest_density   , , ); // [kg / m^3]
rtDeclareVariable(float, viscosity      , , ); // [Pa * s]
rtDeclareVariable(float, surface_tension, , ); // [N / m]
rtDeclareVariable(float, l_threshold    , , ); // []
rtDeclareVariable(float, gass_stiffness , , ); // [J]
rtDeclareVariable(float, restitution    , , ); // []

rtDeclareVariable(float, y_min, , ); // The floor's y-level
rtDeclareVariable(float, x_min, , ); // Left wall
rtDeclareVariable(float, x_max, , ); // Right wall
rtDeclareVariable(float, z_min, , ); // Far wall
rtDeclareVariable(float, z_max, , ); // Near wall

// A table/array/buffer of hash cells.
// In each cell we will store the particles that occupy that corresponding volume in space.
rtBuffer<HashCell> hash_table;

// Simulated particles.
rtBuffer<Particle> particles_buffer;


// Converts a discretized 3D position into a hash table index.
// We use this to decide where in the hash table to store each particle for neighbor detection.
//
// See: eq 5.1, 5.2, 5.3
RT_FUNCTION uint hash(int3 pos) {
  // Primes
  static const int p1 = 73856093;
  static const int p2 = 19349663;
  static const int p3 = 83492791;

  // Hash
  return ((pos.x * p1) ^ (pos.y * p2) ^ (pos.z * p3)) % hash_table.size();
}

// Resets each hash cell to contain 0 particles.
RT_PROGRAM void reset_nearest_neighbors() {
  Particle& p = particles_buffer[launch_index];

  // Fetch the previous cell we stored this particle in (default is 0).
  HashCell& cell = hash_table[p.prev_hash_cell_index];

  // Reset the particle count of that cell (idempotent operation).
  cell[0] = 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Updates each hash cell to contain the particles that occupy the corresponding physical space (voxel).
RT_PROGRAM void update_nearest_neighbors() {
  Particle& p = particles_buffer[launch_index];

  // Discretize the space into cells of size `cell_size` and compute particle `p`'s coordinates in this space.
  int3 cell_position = make_int3(p.position / cell_size);

  // Hash the discretized position to get its corresponding index in the hash table.
  uint cell_index = hash(cell_position);

  // Fetch the cell and remember its index (will be used the next time we reset the hash table).
  HashCell& cell = hash_table[cell_index];
  p.prev_hash_cell_index = cell_index;

  // Increase the particle occupaciation count.
  uint prev_particle_count = atomicAdd(&cell[0], 1);

  // Were we already at max before trying to add this new particle?
  // NOTE: first entry is for count so there are only `HASH_CELL_SIZE-1` particle slots.
  if (prev_particle_count >= HASH_CELL_SIZE-1) {
    // Revert the count to max (idempotent operation).
    atomicMin(&cell[0], HASH_CELL_SIZE-1);
  }

  // Otherwise, we have now reserved a spot (prev_particle_count) in the hash cell
  else {
    // Store particle `p`'s index in the particles buffer in this cell.
    // NOTE: we skip first entry since that's for the particle count.
    cell[1 + prev_particle_count] = launch_index;
  }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Populates `nn` with the neighboring particles that are potentially within `support_radius` distance.
// Also updates `nn_count` to equal the amount of such particles.
RT_FUNCTION void nearest_neighbor_search(Particle& p,
                                         unsigned int& nn_count,
                                         unsigned int* nn) {

  int3 center_cell_position = make_int3(p.position / cell_size);

  // Search for neighbors in the 3x3x3 grid of cells that is centered on particle `p`.
  for (int x = -1; x <= 1; x++) {
    for (int y = -1; y <= 1; y++) {
      for (int z = -1; z <= 1; z++) {

        // Retrieve the corresponding hash cell.
        int3 cell_position = center_cell_position + make_int3(x, y, z);
        uint cell_index = hash(cell_position);
        HashCell& cell = hash_table[cell_index];

        // Iterate all particles in the cell.
        uint n_particles_in_cell = cell[0];
        for (int i = 1; i <= n_particles_in_cell; i++) {

          if (cell[i] != launch_index) {
            nn[nn_count] = cell[i];
            nn_count += 1;
          }
        }

      }
    }
  }
}

// The Poly 6 Smoothing Kernel
// Behaves similarly to a normal distribution but is quite fast to compute.
// NOTE: this is a normalized smoothing kernel, which means that \int W(|x - x_i|) dx = 1
// See eq 4.3 and fig 4.2
RT_FUNCTION float poly6_kernel(float distance) {
  if (distance >= support_radius) {
    return 0.0f;
  } else {
    return (315.0f / (64.0f * M_PIf * powf(support_radius, 9.0f))) * powf(powf(support_radius, 2.0f) - powf(distance, 2.0f), 3.0f);
  }
}

// The Gradient of the Poly 6 Smoothing Kernel
// See eq 4.4
RT_FUNCTION float3 poly6_kernel_gradient(float3 dist_vec) {
  float distance = optix::length(dist_vec);
  if (distance >= support_radius) {
    return make_float3(0.0f);
  } else {
    return -(945.0f / (32.0f * M_PIf * powf(support_radius, 9.0f))) * dist_vec * powf(powf(support_radius, 2.0f) - powf(distance, 2.0f), 2.0f);
  }
}

// The Laplacian of the Poly 6 Smoothing Kernel
// See eq 4.5
RT_FUNCTION float poly6_kernel_laplacian(float distance) {
  if (distance >= support_radius) {
    return 0.0f;
  } else {
    return -(945.0f / (32.0f * M_PIf * powf(support_radius, 9.0f))) * (powf(support_radius, 2.0f) - powf(distance, 2.0f)) * (3.0f * powf(support_radius, 2.0f) - 7.0f * powf(distance, 2.0f));
  }
}

// Evaluates the mass-density field at particle `p`.
// A smoothing kernel is used to ensure smooth continous densities across the fluid.
// See eq 4.6
RT_FUNCTION void update_density(Particle& p,
                                unsigned int nn_count,
                                unsigned int* nn) {

  // Density based on this particle alone.
  float density = particle_mass * poly6_kernel(0.0f);

  // Density from neighbors.
  for (int i = 0; i < nn_count; i++) {
    Particle& pi = particles_buffer[nn[i]];
    float distance = optix::length(p.position - pi.position);

    density += particle_mass * poly6_kernel(distance);
  }

  p.density = density;
}

// Computes pressure based on density deviation from the fluid's rest density.
// This is a modification of the ideal gas law.
// eq 4.12
RT_FUNCTION void update_pressure(Particle& p) {
  // If density at `p` is relatively high, then so is pressure.
  // If density at `p` is relatively low, then so is pressure.
  p.pressure = gass_stiffness * (p.density - rest_density);
}

// Updates per-particle attributes based on neighboring particles.
RT_PROGRAM void update_particles_data() {
  Particle& p = particles_buffer[launch_index];

  // Find nearest neighbors.
  unsigned int nn_count = 0;
  unsigned int nn[3 * 3 * 3 * HASH_CELL_SIZE];
  nearest_neighbor_search(p, nn_count, nn);

  // Update density and pressure for each particle.
  update_density(p, nn_count, nn);
  update_pressure(p);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

RT_FUNCTION float sign(float x) {
	return x > 0.0f ? 1.0f : -1.0f;
}

// The "spiky" kernel (symmetric).
// This is used to prevent clusters when distance approaches 0.
// See eq 4.14 and fig 4.4
RT_FUNCTION float3 pressure_kernel_gradient(float3 dist_vec) {
  float distance = optix::length(dist_vec);
  if (distance >= support_radius) {
    return make_float3(0.0f);
  } else if (distance < 1e-3) {
    return -(45.0f / (M_PIf * powf(support_radius, 6.0f))) * make_float3(sign(dist_vec.x), sign(dist_vec.y), sign(dist_vec.z)) * powf(support_radius - distance, 2.0f);
  } else {
    return -(45.0f / (M_PIf * powf(support_radius, 6.0f))) * (dist_vec / distance) * powf(support_radius - distance, 2.0f);
  }
}

// Computes the symmetric pressure force acting on this particle (due to neighboring particles).
// See eq 4.10 and fig 4.3
RT_FUNCTION float3 pressure_force(const Particle& p,
                                  unsigned int nn_count,
                                  unsigned int* nn) {
  float3 force = make_float3(0.0f);
  for (int i = 0; i < nn_count; i++) {
    const Particle& pi = particles_buffer[nn[i]];
    float3 dist_vec = p.position - pi.position;

    // Higher pressure between the two particles results in stronger force.
    // The density divisions are used to ensure symmetry.
    // NOTE: eq 4.11 is a bit easier to analyze, but appears to perform worse.
    force += particle_mass * (p.pressure / powf(p.density, 2.0f) + pi.pressure / powf(pi.density, 2.0f)) * pressure_kernel_gradient(dist_vec);
  }
  force *= -1.0f * p.density; // We negate to convert the vector back to facing towards `p` again.
  return force;
}

// Strictly positive, which is required for avoiding introducing of energy and instability into the viscosity system.
// Symmetric, just like the other kernels.
// See eq 4.22 and fig 4.5
RT_FUNCTION float viscosity_kernel_laplacian(float distance) {
  if (distance >= support_radius) {
    return 0.0f;
  } else {
    return (45.0f / (M_PIf * powf(support_radius, 6.0f))) * (support_radius - distance);
  }
}

// Computes the viscosity force which causes particles to gradually conform to the surrounding fluid velocity.
// See eq 4.17
RT_FUNCTION float3 viscosity_force(const Particle& p,
                                   unsigned int nn_count,
                                   unsigned int* nn) {
  float3 force = make_float3(0.0f);
  for (int i = 0; i < nn_count; i++) {
    const Particle& pi = particles_buffer[nn[i]];

    // Apply force that brings our velocity closer to the neighboring particle's.
    force += (pi.velocity - p.velocity) * (particle_mass / pi.density) * viscosity_kernel_laplacian(optix::length(pi.position - p.position));
  }
  force *= viscosity;
  return force;
}

// Computes the gravity force density along the negative y-axis.
// See eq 4.24
RT_FUNCTION float3 gravity_force(float particle_density) {
  // Should be multipled by volume as well, but this conforms to eq 4.24 (which presumably assumes constant volume).
  return particle_density * make_float3(0.0f, g, 0.0f);
}

// Computes the inward facing cohesion force for surface fluid particles.
RT_FUNCTION float3 surface_tension_force(const Particle& p,
                                         unsigned int nn_count,
                                         unsigned int* nn) {

  // Compute the inverse/inward surface normal.
  // See eq 4.28 and fig 4.6
  float3 inward_surface_normal = make_float3(0.0f);
  for (int i = 0; i < nn_count; i++) {
    const Particle& pi = particles_buffer[nn[i]];

    // In eq 4.27 we show that the color field (exact locations of particles) can be written in SPH formulation.
    // We then take the gradient to determine in which direction there are more particles.
    inward_surface_normal += (particle_mass / pi.density) * poly6_kernel_gradient(p.position - pi.position);
  }

  // Use threshold to ensure numerical stability.
  // The most contributing particles are the surface particles anyways.
  float normal_dist = optix::length(inward_surface_normal);
  if (normal_dist < l_threshold) {
    return make_float3(0.0f);
  }

  // Compute the laplacian of the color field.
  // See eq 4.26
  float laplacian = (particle_mass / p.density) * poly6_kernel_laplacian(0.0f);
  for (int i = 0; i < nn_count; i++) {
    Particle& pi = particles_buffer[nn[i]];
    float distance = optix::length(p.position - pi.position);

    laplacian += (particle_mass / pi.density) * poly6_kernel_laplacian(distance);
  }

  // We use the laplacian here because it measures the divergence of the normal (i.e. surface curvature).
  float3 force = -surface_tension * laplacian * (inward_surface_normal / normal_dist); // eq 4.26

  return force;
}

// Updates the simulation by one time step.
// See p54
RT_PROGRAM void update_force() {
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

    p.force = tot_force;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Simulates one simulation step of the provided force onto the given particle.
RT_FUNCTION void euler_cromer(Particle& p, float3 force) {
    float3 acceleration = force / p.density; // eq 4.2
    p.velocity += dt * acceleration;
    p.position += dt * p.velocity;
}

// Modifies particle velocity and position based on potential collisions with boundaries.
RT_FUNCTION void collision_detection(Particle& p) {

  // Early Exit (no collision possible)
  if (x_min <= p.position.x && p.position.x <= x_max &&
      y_min <= p.position.y &&
      z_min <= p.position.z && p.position.z <= z_max) {
    return;
  }

  // Compute a simplified (projected) contact point (as opposed to traversing the particle velocity backwards).
  float3 contact_point = p.position;
  contact_point.x = min(x_max, max(x_min, p.position.x));
  contact_point.y = max(y_min, p.position.y);
  contact_point.z = min(z_max, max(z_min, p.position.z));

  // Determine surface normal.
  // This approach will slightly round the box corners, but is a very short/concise implementation.
  // Not rounding the corners (see a previous commit) seems to make no practical difference anyways.
  float3 surface_normal = optix::normalize(contact_point - p.position);

  // Reflect the particle along the standard reflection vector.
  // When restitution is used, we also the velocity that disappeared to the penetration distance.
  // See eq 4.58
  float penetration_depth = optix::length(p.position - contact_point);
  p.velocity = p.velocity - (1.0f + restitution * penetration_depth / (dt * optix::length(p.velocity))) * optix::dot(p.velocity, surface_normal) * surface_normal;

  // Implementation detail: to not have particles perfectly at the same position in corners, we add a small "random" offset (by using their velocity).
  p.position = contact_point + 0.000001f * p.velocity;
}

// Time integrates each particle and handles boundary collisions.
RT_PROGRAM void update_particles() {
    Particle& p = particles_buffer[launch_index];

    // Integrate forces over time
    euler_cromer(p, p.force);

    // Handle potential collisions
    collision_detection(p);
}