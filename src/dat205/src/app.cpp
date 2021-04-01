#include "app.hpp"

#include "util/opengl.hpp"

#include <iostream>

Application::Application(ApplicationCreateInfo create_info) {
  m_window = create_info.window;
  m_window_width = create_info.window_width;
  m_window_height = create_info.window_height;
  m_camera.setViewport(m_window_width, m_window_height);

  m_show_gui = true;
  m_paused = false;
  m_camera_zoom_speed  = 4.5f;

  m_game = std::unique_ptr<PongGame>(new PongGame(10.0f, 5.0f));

  glViewport(0, 0, m_window_width, m_window_height);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

  glGenBuffers(1, &m_output_pbo);
  // Buffer size must be > 0 or OptiX can't create a buffer from it.
  bind_buffer(GL_PIXEL_UNPACK_BUFFER, m_output_pbo, [&]() {
    glBufferData(GL_PIXEL_UNPACK_BUFFER,
                 m_window_width * m_window_height * sizeof(float) * 4,
                 (void *) 0,
                 GL_STREAM_READ); // RGBA32F from byte offset 0 in the pixel unpack buffer.
  });

  glGenTextures(1, &m_output_texture);
  bind_texture(GL_TEXTURE_2D, m_output_texture, [&]() {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  });

  m_output_program = create_program("output.vert", "output.frag");

  // Associate the sampler with texture image unit 0
  use_program(m_output_program, [&]() {
    glUniform1i(glGetUniformLocation(m_output_program, "samplerHDR"), 0); // texture image unit 0
  });

  // Init OptiX
  m_ctx = optix::Context::create();

  std::vector<int> devices = m_ctx->getEnabledDevices();
  for (size_t i = 0; i < devices.size(); ++i) {
    std::cout << "Using local device " << devices[i] << ": " << m_ctx->getDeviceName(devices[i]) << std::endl;
  }

  // Init Programs
  run_unsafe_optix_code([&]() {
    m_ray_gen_program = m_ctx->createProgramFromPTXFile(ptxPath("raygen.cu"), "raygeneration");
    m_exception_program = m_ctx->createProgramFromPTXFile(ptxPath("exception.cu"), "exception");

    m_miss_program = m_ctx->createProgramFromPTXFile(ptxPath("miss.cu"), "miss_environment_constant");

    m_closest_hit_program = m_ctx->createProgramFromPTXFile(ptxPath("closesthit.cu"), "closesthit");
  });

  // Init renderer
  m_ctx->setEntryPointCount(1); // 0 = render
  m_ctx->setRayTypeCount(1);    // 0 = radiance
  m_ctx->setStackSize(1024);

  // Debugging settings
  m_ctx->setPrintEnabled(true);
  m_ctx->setExceptionEnabled(RT_EXCEPTION_ALL, true);

  m_output_buffer = m_ctx->createBufferFromGLBO(RT_BUFFER_OUTPUT, m_output_pbo);
  m_output_buffer->setFormat(RT_FORMAT_FLOAT4); // RGBA32F
  m_output_buffer->setSize(m_window_width, m_window_height);

  m_ctx["sysOutputBuffer"]->set(m_output_buffer);

  m_ctx->setRayGenerationProgram(0, m_ray_gen_program);
  m_ctx->setExceptionProgram(0, m_exception_program);
  m_ctx->setMissProgram(0, m_miss_program);

  // Default initialization. Will be overwritten on the first frame.
  m_ctx["sysCameraPosition"]->setFloat(0.0f, 0.0f, 1.0f);
  m_ctx["sysCameraU"]->setFloat(1.0f, 0.0f, 0.0f);
  m_ctx["sysCameraV"]->setFloat(0.0f, 1.0f, 0.0f);
  m_ctx["sysCameraW"]->setFloat(0.0f, 0.0f, -1.0f);

  // Init scene
  m_scene = std::unique_ptr<OptixScene>(new OptixScene(m_ctx));
  create_scene();
  m_game->create_geometry(*m_scene, m_root_group);
  m_ctx->validate();
  m_ctx->launch(0, 0, 0); // dummy launch to build everything

  // User input
  glfwSetWindowUserPointer(m_window, reinterpret_cast<void*>(this));
  glfwSetKeyCallback(m_window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
    app->handle_keyboard_input(window, key, scancode, action, mods);
  });
};

void Application::create_scene() {
  run_unsafe_optix_code([&]() {
    m_opaque_mat = m_ctx->createMaterial();
    m_opaque_mat->setClosestHitProgram(0, m_closest_hit_program);

    m_root_group = m_ctx->createGroup();

    m_root_acceleration = m_ctx->createAcceleration(ACC_TYPE);
    m_root_group->setAcceleration(m_root_acceleration);

    // TODO: Rename this object
    m_ctx["sysTopObject"]->set(m_root_group);

    // Plane
    {
      optix::Geometry geoPlane = m_scene->create_plane(1, 1);

      optix::GeometryInstance giPlane = m_ctx->createGeometryInstance();
      giPlane->setGeometry(geoPlane);
      giPlane->setMaterialCount(1);
      giPlane->setMaterial(0, m_opaque_mat);

      optix::Acceleration accPlane = m_ctx->createAcceleration(ACC_TYPE);
      set_acceleration_properties(accPlane);
      
      optix::GeometryGroup ggPlane = m_ctx->createGeometryGroup();
      ggPlane->setAcceleration(accPlane);
      ggPlane->addChild(giPlane);

      float trafoPlane[16] = {
        10.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 5.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
      };
      optix::Matrix4x4 matrixPlane(trafoPlane);

      optix::Transform trPlane = m_ctx->createTransform();
      trPlane->setChild(ggPlane);
      trPlane->setMatrix(false, matrixPlane.getData(), matrixPlane.inverse().getData());
      
      // Add the transform node placeing the plane to the scene's root Group node.
      m_root_group->addChild(trPlane);
    }
  });
}

void Application::run() {
  while (!glfwWindowShouldClose(m_window)) {

    // Adjust viewport to (potentially new) window dimensions.
    update_viewport();

    update_scene();

    glfwPollEvents();
    render_scene();
    render_gui();

    // Swap the front and back buffers of the default (double-buffered) framebuffer.
    glfwSwapBuffers(m_window);
  }
  m_ctx->destroy();
}

void Application::handle_user_input() {
  ImGuiIO const& io = ImGui::GetIO();

  // Toggle GUI visibility with keyboard button.
  if (ImGui::IsKeyPressed('G', false)) {
    m_show_gui = !m_show_gui;
  }

  if (ImGui::IsKeyPressed(' ', false)) {
    m_paused = !m_paused;
  }

  const ImVec2 mousePosition = ImGui::GetMousePos(); // Mouse coordinate window client rect.
  const int x = int(mousePosition.x);
  const int y = int(mousePosition.y);

  // Ensure mouse is not interacting with the GUI.
  if (!io.WantCaptureMouse) {
    if (ImGui::IsMouseDown(0)) { // Left mouse button
      m_camera.orbit(x, y);
    }
    else if (ImGui::IsMouseDown(1)) { // Right mouse button
      m_camera.pan(x, y);
    }
    else if (io.MouseWheel != 0.0f) { // Mouse wheel scroll
      m_camera.zoom(m_camera_zoom_speed * -io.MouseWheel);
    }
    else {
      m_camera.setBaseCoordinates(x, y);
    }
  }
}

void Application::display() {
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, m_output_texture);

  // Render m_output_texture on a quad that fills the current viewport.
  use_program(m_output_program, []() {
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(-1.0f, -1.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(1.0f, -1.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(1.0f, 1.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(-1.0f, 1.0f);
    glEnd();
  });
}

void Application::render_gui() {
  render_gui_frame([&]() {
    handle_user_input();
    if (m_show_gui) {
      ImGui::Begin("DAT205");
      // ImGui::DragFloat("Ball x", &m_ball_x, 0.2f, -10.0f, 10.0f, "%.1f");
      // ImGui::DragFloat("Ball z", &m_ball_z, 0.2f, -10.0f, 10.0f, "%.1f");

      // if (ImGui::ColorEdit3("Background", (float *)&m_bg_color)) {
      //   m_ctx["sysColorBackground"]->setFloat(m_bg_color);
      // }
      ImGui::End();
    }
  });
}

void Application::handle_keyboard_input(GLFWwindow* window, int key, int scancode, int action, int mods) {
  // NOTE: this setup is awkward but reduces input lag
  static bool d_down = false;
  static bool f_down = false;
  static bool j_down = false;
  static bool k_down = false;

  auto update_key_state = [&](bool& is_down, int k) {
    if (key == k) {
      if (action == GLFW_PRESS) {
        is_down = true;
      } else if(action == GLFW_RELEASE) {
        is_down = false;
      }
    }
  };

  update_key_state(d_down, GLFW_KEY_D);
  update_key_state(f_down, GLFW_KEY_F);
  update_key_state(j_down, GLFW_KEY_J);
  update_key_state(k_down, GLFW_KEY_K);

  if (d_down) {
    m_player1_velocity = -1.0f;
  } else if (f_down) {
    m_player1_velocity = 1.0f;
  } else {
    m_player1_velocity = 0.0f;
  }

  if (k_down) {
    m_player2_velocity = -1.0f;
  } else if (j_down) {
    m_player2_velocity = 1.0f;
  } else {
    m_player2_velocity = 0.0f;
  }
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
  optix::float3 cameraU;
  optix::float3 cameraV;
  optix::float3 cameraW;

  bool cameraChanged = m_camera.getFrustum(camera_pos, cameraU, cameraV, cameraW);
  if (cameraChanged) {
    m_ctx["sysCameraPosition"]->setFloat(camera_pos);
    m_ctx["sysCameraU"]->setFloat(cameraU);
    m_ctx["sysCameraV"]->setFloat(cameraV);
    m_ctx["sysCameraW"]->setFloat(cameraW);
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

void Application::update_viewport() {
  int width, height;
  glfwGetFramebufferSize(m_window, &width, &height);

  // Check if new dimensions are valid and differ.
  if ((width != 0 && height != 0) && (width != m_window_width || height != m_window_height)) {

    m_window_width = width;
    m_window_height = height;
    glViewport(0, 0, m_window_width, m_window_height);

    // Unregister buffer while modifying it so CUDA will be notified of the size change (and not crash).
    unregister_buffer(m_output_buffer, [&]() {

      // Resize output buffer to match window dimensions.
      m_output_buffer->setSize(m_window_width, m_window_height);

      // Initialize with empty data.
      bind_buffer(GL_PIXEL_UNPACK_BUFFER, m_output_buffer->getGLBOId(), [&]() {
        glBufferData(GL_PIXEL_UNPACK_BUFFER,
                     m_output_buffer->getElementSize() * m_window_width * m_window_height,
                     nullptr,
                     GL_STREAM_DRAW);
      });
    });

    m_camera.setViewport(m_window_width, m_window_height);
  }
}
