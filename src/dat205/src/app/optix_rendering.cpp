#include "app.hpp"

#include <iostream>

void Application::setup_optix_rendering() {
  glViewport(0, 0, m_window_width, m_window_height);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

  // Initialize output buffer with empty data.
  glGenBuffers(1, &m_output_pbo);
  bind_buffer(GL_PIXEL_UNPACK_BUFFER, m_output_pbo, [&]() {
    glBufferData(GL_PIXEL_UNPACK_BUFFER,
                 m_window_width * m_window_height * sizeof(float) * 4,
                 (void *) 0,
                 GL_STREAM_READ);
  });

  // Setup output texture.
  glGenTextures(1, &m_output_texture);
  bind_texture(GL_TEXTURE_2D, m_output_texture, [&]() {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  });

  // Output Shader
  m_output_program = create_program("output.vert", "output.frag");

  // Bind the sampler to texture image unit 0.
  use_program(m_output_program, [&]() {
    glUniform1i(glGetUniformLocation(m_output_program, "tex_sampler"), 0);
  });

  // Init OptiX
  m_ctx = optix::Context::create();
  std::vector<int> devices = m_ctx->getEnabledDevices();
  for (size_t i = 0; i < devices.size(); ++i) {
    std::cout << "Using local device " << devices[i] << ": " << m_ctx->getDeviceName(devices[i]) << std::endl;
  }

  // General OptiX Configurations
  m_ctx->setEntryPointCount(1); // 0 = render
  m_ctx->setRayTypeCount(2);    // 0 = radiance, 1 = shadow
  m_ctx->setStackSize(2048);    // [bytes / thread]

  // Debug Settings
  #if USE_DEBUG_EXCEPTIONS
  m_ctx->setPrintEnabled(true);
  m_ctx->setExceptionEnabled(RT_EXCEPTION_ALL, true);
  #endif

  // Setup OptiX' output buffer.
  m_output_buffer = m_ctx->createBufferFromGLBO(RT_BUFFER_OUTPUT, m_output_pbo);
  m_output_buffer->setFormat(RT_FORMAT_FLOAT4); // RGBA32F
  m_output_buffer->setSize(m_window_width, m_window_height);
  m_ctx["output_buffer"]->set(m_output_buffer);

  // Mandatory OptiX Shaders
  run_unsafe_optix_code([&]() {
    m_ray_gen_program   = m_ctx->createProgramFromPTXFile(ptxPath("ray_generation.cu"), "ray_generation");
    m_exception_program = m_ctx->createProgramFromPTXFile(ptxPath("exception.cu"), "exception");
    m_miss_program      = m_ctx->createProgramFromPTXFile(ptxPath("miss.cu"), "environment_map");
  });
  m_ctx->setRayGenerationProgram(0, m_ray_gen_program);
  m_ctx->setExceptionProgram(0, m_exception_program);
  m_ctx->setMissProgram(0, m_miss_program);
}

void Application::display() {

  // Bind output texture, which has been written to by OptiX through pixel transfer.
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, m_output_texture);

  // Render m_output_texture on a quad that fills the current viewport.
  use_program(m_output_program, []() {
    glBegin(GL_QUADS);

    // Bottom-left Corner
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(-1.0f, -1.0f);

    // Bottom-right Corner
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(1.0f, -1.0f);

    // Top-right Corner
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(1.0f, 1.0f);

    // Top-left Corner
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(-1.0f, 1.0f);

    glEnd();
  });
}
