#include "app.hpp"

#include <random>

using namespace optix;

void Application::setup_water_simulation() {
  m_paused = true;
  
  run_unsafe_optix_code([&]() {
    m_water_update_program = m_ctx->createProgramFromPTXFile(ptxPath("water_simulation.cu"), "update");
  });
  // Reserve entry point 1 for water simulation
  m_ctx->setRayGenerationProgram(1, m_water_update_program);

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

  float3 offset = make_float3(- 0.8f * m_box_width, PARTICLE_RADIUS + 0.5f, - 0.8f * m_box_depth);

  // int side_length = 50; // A bit more than 10^5 particles
  int side_length = 50;
  m_particles_count = side_length * side_length * side_length;

  std::random_device rd;
  std::mt19937 rng(rd());
  std::uniform_real_distribution<> dist(-4.0, 4.0);

  std::vector<Particle> particles(m_particles_count);
  for (int x = 0; x < side_length; x++) {
    for (int y = 0; y < side_length; y++) {
      for (int z = 0; z < side_length; z++) {
        Particle& p = particles[x * side_length * side_length + y * side_length + z];
        p.position = offset + make_float3(x, y, z) / (6.0f);
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

  m_ctx["particles_buffer"]->setBuffer(m_particles_buffer);
  m_ctx["g"]->setFloat(-9.82f);
  m_ctx["y_min"]->setFloat(0.0f);
  m_ctx["x_min"]->setFloat(-m_box_width);
  m_ctx["x_max"]->setFloat(m_box_width);
  m_ctx["z_min"]->setFloat(-m_box_depth);
  m_ctx["z_max"]->setFloat(m_box_depth);

  Geometry geometry = m_ctx->createGeometry();
  // geometry["particles_buffer"]->setBuffer(m_particles_buffer);
  geometry->setBoundingBoxProgram(m_ctx->createProgramFromPTXFile(ptxPath("water_rendering.cu"), "bounding_box"));
  geometry->setIntersectionProgram(m_ctx->createProgramFromPTXFile(ptxPath("water_rendering.cu"), "ray_intersection"));
  geometry->setPrimitiveCount(particles.size());

  geometry_instance->setGeometry(geometry);

  m_root_group->addChild(geometry_group);
}

void Application::update_water_simulation(float dt) {
  m_ctx["dt"]->setFloat(0.02f);
  m_ctx->launch(1, m_particles_count);
  m_water_acceleration->markDirty();
}