#include "app.hpp"

#include "opengl/shader.hpp"
#include "opengl/util.hpp"
#include "optix_util.hpp"

#include <iostream>

Application::Application(ApplicationCreateInfo create_info) {
  m_window = create_info.window;
  m_window_width = create_info.window_width;
  m_window_height = create_info.window_height;
  m_show_gui = true;

  glViewport(0, 0, m_window_width, m_window_height);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

  glGenBuffers(1, &m_output_pbo);
  // Buffer size must be > 0 or OptiX can't create a buffer from it.
  bind_buffer(GL_PIXEL_UNPACK_BUFFER, m_output_pbo, [&]() {
    glBufferData(GL_PIXEL_UNPACK_BUFFER, m_window_width * m_window_height * sizeof(float) * 4, (void *)0, GL_STREAM_READ); // RGBA32F from byte offset 0 in the pixel unpack buffer.
  });

  glGenTextures(1, &m_output_texture);
  bind_texture(GL_TEXTURE_2D, m_output_texture, [&]() {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  });

  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

  m_output_program = create_program("output.vert", "output.frag");
  // m_output_program = create_program("test.vert", "test.frag");

  // Associate the sampler with texture image unit 0
  use_program(m_output_program, [&]() {
    glUniform1i(glGetUniformLocation(m_output_program, "samplerHDR"), 0); // texture image unit 0
  });

  m_ctx = optix::Context::create();

  std::vector<int> devices = m_ctx->getEnabledDevices();
  for (size_t i = 0; i < devices.size(); ++i) {
    std::cout << "Using local device " << devices[i] << ": " << m_ctx->getDeviceName(devices[i]) << std::endl;
  }

  try {
    m_ray_gen_program = m_ctx->createProgramFromPTXFile(ptxPath("raygen.cu"), "raygeneration");
    m_exception_program = m_ctx->createProgramFromPTXFile(ptxPath("exception.cu"), "exception");
  }
  catch(optix::Exception& e) {
    std::cerr << e.getErrorString() << std::endl;
  }

  m_ctx->setEntryPointCount(1); // 0 = render
  m_ctx->setRayTypeCount(0);
  m_ctx->setStackSize(1024);

  // Debugging settings
  m_ctx->setPrintEnabled(true);
  m_ctx->setExceptionEnabled(RT_EXCEPTION_ALL, true);
  
  m_ctx["sysColorBackground"]->setFloat(0.462745f, 0.72549f, 0.0f);

  m_output_buffer = m_ctx->createBufferFromGLBO(RT_BUFFER_OUTPUT, m_output_pbo);
  m_output_buffer->setFormat(RT_FORMAT_FLOAT4); // RGBA32F
  m_output_buffer->setSize(m_window_width, m_window_height);

  m_ctx["sysOutputBuffer"]->set(m_output_buffer);

  m_ctx->setRayGenerationProgram(0, m_ray_gen_program);
  m_ctx->setExceptionProgram(0, m_exception_program);
};

void Application::run() {
  while (!glfwWindowShouldClose(m_window)) {

    handle_user_input();
    update_viewport();

    render_scene();
    display();

    if (m_show_gui) {
      render_gui();
    }

    // Swap the front and back buffers of the default (double-buffered) framebuffer.
    glfwSwapBuffers(m_window);
  }
}

void Application::handle_user_input() {
  glfwPollEvents();
  ImGuiIO const& io = ImGui::GetIO();

  // Toggle GUI visibility with keyboard button.
  if (ImGui::IsKeyPressed('G', false)) {
    m_show_gui = !m_show_gui;
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
  render_gui_frame([]() {
    ImGui::ShowTestWindow();
  });
}

void Application::render_scene() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  m_ctx->launch(0, m_window_width, m_window_height);

  // Unpack pixel data from CUDA to output texture.
  glActiveTexture(GL_TEXTURE0);
  bind_texture(GL_TEXTURE_2D, m_output_texture, [&]() {
    bind_buffer(GL_PIXEL_UNPACK_BUFFER, m_output_buffer->getGLBOId(), [&]() {
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, (GLsizei)m_window_width, (GLsizei)m_window_height, 0, GL_RGBA, GL_FLOAT, (void *) 0);
    });
  });
}

void Application::update_viewport() {
  int width, height;
  glfwGetFramebufferSize(m_window, &width, &height);

  // Check if new dimensions are valid and differ.
  if ((width != 0 && height != 0) && (width != m_window_width || height != m_window_height)) {

    m_window_width = width;
    m_window_height = height;
    glViewport(0, 0, width, height);

    // Unregister buffer while modifying it so CUDA will be notified of the size change (and not crash).
    unregister_buffer(m_output_buffer, [&]() {

      // Resize output buffer to match window dimensions.
      m_output_buffer->setSize(width, height);

      // Initialize with empty data.
      bind_buffer(GL_PIXEL_UNPACK_BUFFER, m_output_buffer->getGLBOId(), [&]() {
        glBufferData(GL_PIXEL_UNPACK_BUFFER, m_output_buffer->getElementSize() * width * height, nullptr, GL_STREAM_DRAW);
      });
    });
  }
}
