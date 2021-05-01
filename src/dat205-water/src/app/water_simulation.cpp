#include "app.hpp"

using namespace optix;

void Application::setup_water_simulation() {
  m_paused = true;

  setup_water_particles();
  setup_water_geometry();
  setup_water_physics();
}

void Application::setup_water_particles() {

  // Setup CUDA functions (we'll use OptiX for convenience).
  m_ctx->setRayGenerationProgram(1, m_ctx->createProgramFromPTXFile(ptxPath("water_simulation.cu"), "reset_nearest_neighbors"));
  m_ctx->setRayGenerationProgram(2, m_ctx->createProgramFromPTXFile(ptxPath("water_simulation.cu"), "update_nearest_neighbors"));
  m_ctx->setRayGenerationProgram(3, m_ctx->createProgramFromPTXFile(ptxPath("water_simulation.cu"), "update_particles_data"));
  m_ctx->setRayGenerationProgram(4, m_ctx->createProgramFromPTXFile(ptxPath("water_simulation.cu"), "update_force"));
  m_ctx->setRayGenerationProgram(5, m_ctx->createProgramFromPTXFile(ptxPath("water_simulation.cu"), "update_particles"));

  // Setup particles.
  int side_length = 20; // ~8k particles
  m_particles_count = side_length * side_length * side_length;
  std::vector<Particle> particles(m_particles_count);

  // We will use a fixed particle radius because the density-based approach (eq 5.20) is quite expensive (involves a cube root).
  // NOTE: should be less than the support radius (not for physical reasons, but for visual reasons).
  m_particles_radius = 0.0135f;
  m_ctx["particle_radius"]->setFloat(m_particles_radius); // [m]

  // Water simulation setup: center setup with no velocity.
  {
    float3 offset = make_float3(0.0f, 1.0f, 0.0f) - make_float3((side_length / 2) * 2.0f * m_particles_radius);
    for (int x = 0; x < side_length; x++) {
      for (int y = 0; y < side_length; y++) {
        for (int z = 0; z < side_length; z++) {
          Particle& p = particles[x * side_length * side_length + y * side_length + z];
          p.position = offset + 2.0f * m_particles_radius * make_float3(x, y, z);
          p.velocity = make_float3(0.0f);
        }
      }
    }
  }

  // Water simulation setup: corner setup with no velocity.
  // {
  //   float3 offset = make_float3(-m_box_width + 2.0f * m_particles_radius, 2.0f * m_particles_radius, -m_box_depth + 2.0f * m_particles_radius);
  //   for (int x = 0; x < side_length; x++) {
  //     for (int y = 0; y < side_length; y++) {
  //       for (int z = 0; z < side_length; z++) {
  //         Particle& p = particles[x * side_length * side_length + y * side_length + z];
  //         p.position = offset + 2.0f * m_particles_radius * make_float3(x, y, z);
  //         p.velocity = make_float3(0.0f);
  //       }
  //     }
  //   }
  // }

  // Water simulation setup: side setup with no velocity.
  // {
  //   float3 offset = make_float3(-m_box_width + 2.0f * m_particles_radius, 2.0f * m_particles_radius, -m_box_depth + 2.0f * m_particles_radius);
  //   for (int x = 0; x < side_length; x++) {
  //     for (int y = 0; y < side_length; y++) {
  //       for (int z = 0; z < side_length; z++) {
  //         Particle& p = particles[x * side_length * side_length + y * side_length + z];
  //         p.position = offset + m_particles_radius * make_float3(x, y, 3.5f * z);
  //         p.velocity = make_float3(0.0f);
  //       }
  //     }
  //   }
  // }

  // Create particles buffer.
  m_particles_buffer = m_ctx->createBuffer(RT_BUFFER_INPUT);
  m_particles_buffer->setFormat(RT_FORMAT_USER);
  m_particles_buffer->setElementSize(sizeof(Particle));
  m_particles_buffer->setSize(particles.size());

  // Upload particles data to the GPU buffer.
  memcpy(m_particles_buffer->map(), particles.data(), sizeof(Particle) * particles.size());
  m_particles_buffer->unmap();

  // Determine suitable hash table size using eq 5.4: nextPrime(2 * m_particles_count)
  std::vector<HashCell> hash_table(54001); // Based on 30^3. Prime manually picked from: http://compoasso.free.fr/primelistweb/page/prime/liste_online_en.php

  // Create hash table buffer.
  m_hash_buffer = m_ctx->createBuffer(RT_BUFFER_INPUT);
  m_hash_buffer->setFormat(RT_FORMAT_USER);
  m_hash_buffer->setElementSize(sizeof(HashCell));
  m_hash_buffer->setSize(hash_table.size());

  // Upload initial (empty) hash table data.
  memcpy(m_hash_buffer->map(), hash_table.data(), sizeof(HashCell) * hash_table.size());
  m_hash_buffer->unmap();

  // Store the buffers in our OptiX context.
  m_ctx["particles_buffer"]->setBuffer(m_particles_buffer);
  m_ctx["hash_table"]->setBuffer(m_hash_buffer);
}

void Application::setup_water_geometry() {

  // Water Geometry and Material
  Geometry geometry = m_ctx->createGeometry();
  geometry->setBoundingBoxProgram(m_ctx->createProgramFromPTXFile(ptxPath("water_rendering.cu"), "bounding_box"));
  geometry->setIntersectionProgram(m_ctx->createProgramFromPTXFile(ptxPath("water_rendering.cu"), "ray_intersection"));
  geometry->setPrimitiveCount(m_particles_count);

  Material mat = m_ctx->createMaterial();
  mat->setClosestHitProgram(0, m_ctx->createProgramFromPTXFile(ptxPath("water_rendering.cu"), "closest_hit"));

  // Minimal OptiX Geometry Group Setup
  GeometryInstance geometry_instance = m_ctx->createGeometryInstance();
  geometry_instance->setMaterialCount(1);
  geometry_instance->setMaterial(0, mat);
  geometry_instance->setGeometry(geometry);

  m_water_acceleration = m_ctx->createAcceleration(ACC_TYPE);

  GeometryGroup geometry_group = m_ctx->createGeometryGroup();
  geometry_group->setAcceleration(m_water_acceleration);
  geometry_group->addChild(geometry_instance);

  m_root_group->addChild(geometry_group);
}

void Application::setup_water_physics() {

  // Simulation Boundaries (i.e. the glass floor and walls)
  float margin = m_particles_radius + 0.01f;
  m_ctx["y_min"]->setFloat(0.0f + margin);
  m_ctx["x_min"]->setFloat(-m_box_width + margin);
  m_ctx["x_max"]->setFloat(m_box_width - margin);
  m_ctx["z_min"]->setFloat(-m_box_depth + margin);
  m_ctx["z_max"]->setFloat(m_box_depth - margin);

  // Gravity Acceleration Constant
  m_ctx["g"]->setFloat(-9.82f); // [m / s^2]

  // Mass-density of water.
  float rest_density = 998.29f; // [kg / m^3]
  m_ctx["rest_density"]->setFloat(rest_density); // [kg / m^3]

  // The volume of water that our particles are together representing.
  // The current setup is a bit arbitrary, but roughly mimics the cube we start with in setup 1.
  float fluid_volume = m_particles_count * pow(2.0f * m_particles_radius, 3.0f); // [m^3]

  // The mass-per-particle amount that follows from this volume.
  float particle_mass = (fluid_volume / m_particles_count) * rest_density; // [kg]
  m_ctx["particle_mass"]->setFloat(particle_mass); // [kg]

  // Upper bound on pair-wise direct interaction range between particles.
  // Will produce values very close to 0.0457f (See table on p51)
  // See eq 5.14 and fig 5.3
  float average_neighbor_count = 30; // The average amount of neighboring particles we use (at rest density).
  float support_radius = pow(3.0f * fluid_volume * average_neighbor_count / (4.0f * M_PI * m_particles_count), 1.0f / 3.0f); // [m]

  m_ctx["support_radius"]->setFloat(support_radius); // [m]

  // The side length of the voxel that each hash cell represents.
  m_ctx["cell_size"]->setFloat(support_radius); // [m], see eq 5.5

  // Visocity is slightly exaggerated due to small particle count compared to reality.
  m_ctx["viscosity"]->setFloat(3.5f); // [Pa * s]
  // m_ctx["viscosity"]->setFloat(5.0f); // Looks pretty good.

  // Assuming water (at ~20 celsius) against air. See https://www.wikiwand.com/en/Surface_tension
  m_ctx["surface_tension"]->setFloat(0.0728f); // [N / m]

  // Minimum magnitude of inward surface normals we will consider for surface tension.
  m_ctx["l_threshold"]->setFloat(7.065f); // []

  // Gass stiffness constant `k` is given by the ideal gas law (eq 5.15): k = PV = nRT
  // However, its true value would be very large and force an unreasonably small step size.
  // So, we instead use it as a design param that should be kept as large as possible, without causing instabilities.
  m_ctx["gass_stiffness"]->setFloat(3.0f); // [J]

  // Conservasion of kinetic energy after collision against boundaries.
  m_ctx["restitution"]->setFloat(0.5f); // []

}

void Application::update_water_simulation(float dt) {
  m_ctx["dt"]->setFloat(dt);

  // Reset the hash table to not contain any particles.
  m_ctx->launch(1, m_particles_count);

  // (Re)build the hash table.
  m_ctx->launch(2, m_particles_count);

  // Update particle data.
  m_ctx->launch(3, m_particles_count);

  // Update particle forces.
  m_ctx->launch(4, m_particles_count);

  // Update simulation by one timestep.
  m_ctx->launch(5, m_particles_count);

  // Mark particle bounding boxes as outdated.
  m_water_acceleration->markDirty();
}
