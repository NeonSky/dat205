#include "app.hpp"

using namespace optix;

void Application::create_scene() {

  // Create root group and acceleration structure.
  run_unsafe_optix_code([&]() {
    m_root_group = m_ctx->createGroup();

    m_root_acceleration = m_ctx->createAcceleration(ACC_TYPE);
    m_root_group->setAcceleration(m_root_acceleration);

    m_ctx["root"]->set(m_root_group);
  });

  // Create game geometry under root node.
  m_game->create_geometry(*m_scene, m_root_group);

  // Add some light sources to the scene.
  create_scene_lights();
}

void Application::update_scene() {
  if (m_paused) {
    return;
  }

  const float dt = 0.02f;
  m_game->update(dt, m_player1_velocity, m_player2_velocity);

  m_root_acceleration->markDirty();
}

void Application::render_scene() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  float3 camera_pos;
  float3 camera_right;
  float3 camera_up;
  float3 camera_forward;

  bool camera_changed = m_camera.getFrustum(camera_pos,
                                           camera_right,
                                           camera_up,
                                           camera_forward);
  if (camera_changed) {
    m_ctx["camera_pos"]->setFloat(camera_pos);
    m_ctx["camera_right"]->setFloat(camera_right);
    m_ctx["camera_up"]->setFloat(camera_up);
    m_ctx["camera_forward"]->setFloat(camera_forward);
  }

  m_ctx->launch(0, m_window_width, m_window_height);

  // Unpack pixel data from CUDA to output texture.
  glActiveTexture(GL_TEXTURE0);
  bind_texture(GL_TEXTURE_2D, m_output_texture, [&]() {
    bind_buffer(GL_PIXEL_UNPACK_BUFFER, m_output_buffer->getGLBOId(), [&]() {
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, (GLsizei)m_window_width, (GLsizei)m_window_height, 0, GL_RGBA, GL_FLOAT, (void *) 0);
    });
  });

  display();
}

void Application::create_scene_lights() {
  std::vector<PointLight> lights;

  // Sun
  {
    PointLight l;
    l.position  = make_float3(-30.0f, 80.0f, -40.0f);
    l.color     = make_float3(0.95f, 0.86f, 0.83f);
    l.intensity = 20000.0f;
    lights.push_back(l);
  }

  // Red Goal Lights
  {
    PointLight l;
    l.position  = make_float3(-8.0f, 4.0f, -3.0f);
    l.color     = make_float3(0.8f, 0.0f, 0.0f);
    l.intensity = 25.0f;
    lights.push_back(l);
    l.position  = make_float3(-8.0f, 4.0f, 3.0f);
    lights.push_back(l);
  }

  // Blue Goal Lights
  {
    PointLight l;
    l.position  = make_float3(8.0f, 4.0f, -3.0f);
    l.color     = make_float3(0.0f, 0.0f, 0.8f);
    l.intensity = 25.0f;
    lights.push_back(l);
    l.position  = make_float3(8.0f, 4.0f, 3.0f);
    lights.push_back(l);
  }

  // Upload data to GPU.
  Buffer buffer = m_ctx->createBuffer(RT_BUFFER_INPUT);
  buffer->setFormat(RT_FORMAT_USER);
  buffer->setElementSize(sizeof(PointLight));
  buffer->setSize(lights.size());

  // Fills the buffer with the lights data.
  memcpy(buffer->map(), lights.data(), sizeof(PointLight) * lights.size());
  buffer->unmap();

  m_ctx["lights"]->set(buffer);
}