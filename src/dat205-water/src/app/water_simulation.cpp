#include "app.hpp"

#include <random>

using namespace optix;

void Application::setup_water_simulation() {
  m_paused = true;

  setup_water_particles();
  setup_water_geometry();
  setup_water_physics();
}

void Application::setup_water_particles() {
  // Reserve entry point 1 for water simulation, entry point 2 for hash table updates, entry point 3 for updating particle data
  m_ctx->setRayGenerationProgram(1, m_ctx->createProgramFromPTXFile(ptxPath("water_simulation.cu"), "update"));
  m_ctx->setRayGenerationProgram(2, m_ctx->createProgramFromPTXFile(ptxPath("water_simulation.cu"), "update_nearest_neighbors"));
  m_ctx->setRayGenerationProgram(3, m_ctx->createProgramFromPTXFile(ptxPath("water_simulation.cu"), "update_particles_data"));

  float3 offset = make_float3(- 0.8f * m_box_width, PARTICLE_RADIUS + 0.5f, - 0.8f * m_box_depth);

  // int side_length = 50; // A bit more than 10^5 particles
  int side_length = 20; // the current prime is based on this
  m_particles_count = side_length * side_length * side_length;

  std::random_device rd;
  std::mt19937 rng(rd());
  std::uniform_real_distribution<> dist(-1.0, 1.0);

  std::vector<Particle> particles(m_particles_count);
  for (int x = 0; x < side_length; x++) {
    for (int y = 0; y < side_length; y++) {
      for (int z = 0; z < side_length; z++) {
        Particle& p = particles[x * side_length * side_length + y * side_length + z];
        p.position = offset + 2.0f * PARTICLE_RADIUS * make_float3(x, y, z);
        p.velocity = make_float3(dist(rng), dist(rng), dist(rng));
      }
    }
  }

  m_particles_buffer = m_ctx->createBuffer(RT_BUFFER_INPUT_OUTPUT);
  m_particles_buffer->setFormat(RT_FORMAT_USER);
  m_particles_buffer->setElementSize(sizeof(Particle));
  m_particles_buffer->setSize(particles.size());

  memcpy(m_particles_buffer->map(), particles.data(), sizeof(Particle) * particles.size());
  m_particles_buffer->unmap();

  // eq 5.4: nextPrime(2 * m_particles_count)
  unsigned int hash_table_size = 16001; // Prime manually picked from: http://compoasso.free.fr/primelistweb/page/prime/liste_online_en.php

  m_hash_buffer = m_ctx->createBuffer(RT_BUFFER_INPUT_OUTPUT);
  m_hash_buffer->setFormat(RT_FORMAT_USER);
  m_hash_buffer->setElementSize(sizeof(HashCell));
  m_hash_buffer->setSize(hash_table_size);

  reset_hash_table();

  m_ctx["particles_buffer"]->setBuffer(m_particles_buffer);
  m_ctx["hash_table"]->setBuffer(m_hash_buffer);
}

void Application::setup_water_geometry() {
  Material mat = m_ctx->createMaterial();
  mat->setClosestHitProgram(0, m_ctx->createProgramFromPTXFile(ptxPath("water_rendering.cu"), "closest_hit"));

  m_water_acceleration = m_ctx->createAcceleration(ACC_TYPE);

  GeometryGroup geometry_group = m_ctx->createGeometryGroup();
  geometry_group->setAcceleration(m_water_acceleration);

  // Let the paddles also share geometry.
  GeometryInstance geometry_instance = m_ctx->createGeometryInstance();
  geometry_instance->setMaterialCount(1);
  geometry_instance->setMaterial(0, mat);

  geometry_group->addChild(geometry_instance);

  Geometry geometry = m_ctx->createGeometry();
  geometry->setBoundingBoxProgram(m_ctx->createProgramFromPTXFile(ptxPath("water_rendering.cu"), "bounding_box"));
  geometry->setIntersectionProgram(m_ctx->createProgramFromPTXFile(ptxPath("water_rendering.cu"), "ray_intersection"));
  geometry->setPrimitiveCount(m_particles_count);

  geometry_instance->setGeometry(geometry);

  m_root_group->addChild(geometry_group);
}

void Application::setup_water_physics() {
  // Boundaries
  m_ctx["y_min"]->setFloat(0.0f);
  m_ctx["x_min"]->setFloat(-m_box_width);
  m_ctx["x_max"]->setFloat(m_box_width);
  m_ctx["z_min"]->setFloat(-m_box_depth);
  m_ctx["z_max"]->setFloat(m_box_depth);

  m_ctx["g"]->setFloat(-9.82f);

  // Some values from p51

  float support_radius = 0.0457f; // [m]
  float fluid_volume = 1.0f; // [m^3]
  float rest_density = 998.29f; // [kg / m^3]
  float particle_mass = (fluid_volume / m_particles_count) * rest_density; // [kg]

  m_ctx["cell_size"]->setFloat(2.0f * support_radius);
  m_ctx["support_radius"]->setFloat(support_radius); // [m]

  m_ctx["particle_mass"]->setFloat(particle_mass);
  m_ctx["rest_density"]->setFloat(998.29f);
  m_ctx["gass_stiffness"]->setFloat(3.0f);
  m_ctx["viscosity"]->setFloat(3.5f);

}

void Application::reset_hash_table() {
  RTsize size;
  m_hash_buffer->getSize(size);

  std::vector<HashCell> hash_table(size);
  for (auto& cell : hash_table) {
    std::fill(cell, cell + HASH_CELL_SIZE, 0);
  }

  memcpy(m_hash_buffer->map(), hash_table.data(), sizeof(HashCell) * hash_table.size());
  m_hash_buffer->unmap();
}

void Application::update_water_simulation(float dt) {
  m_ctx["dt"]->setFloat(0.02f);

  reset_hash_table();
  m_ctx->launch(2, 1);

  // Update particles' data
  m_ctx->launch(3, m_particles_count);

  m_ctx->launch(1, m_particles_count);
  m_water_acceleration->markDirty();
}
