#include "app.hpp"

void Application::create_scene() {
  run_unsafe_optix_code([&]() {
    m_root_group = m_ctx->createGroup();

    m_root_acceleration = m_ctx->createAcceleration(ACC_TYPE);
    m_root_group->setAcceleration(m_root_acceleration);

    m_ctx["root"]->set(m_root_group);
  });

  create_background_geometry();
  m_game->create_geometry(*m_scene, m_root_group);
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

  optix::float3 camera_pos;
  optix::float3 camera_right;
  optix::float3 camera_up;
  optix::float3 camera_forward;

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