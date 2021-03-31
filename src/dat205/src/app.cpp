#include "app.hpp"

#include "util/opengl.hpp"

#include <random>
#include <iostream>

#define ACC_TYPE "Trbvh"

void setAccelerationProperties(optix::Acceleration acceleration) {
  // This requires that the position is the first element and it must be float x, y, z.
  acceleration->setProperty("vertex_buffer_name", "vertexBuffer");
  assert(sizeof(VertexData) == 48);
  acceleration->setProperty("vertex_buffer_stride", "48");

  acceleration->setProperty("index_buffer_name", "indicesBuffer");
  assert(sizeof(optix::uint3) == 12);
  acceleration->setProperty("index_buffer_stride", "12");
}

Application::Application(ApplicationCreateInfo create_info) {
  m_window = create_info.window;
  m_window_width = create_info.window_width;
  m_window_height = create_info.window_height;
  m_camera.setViewport(m_window_width, m_window_height);

  m_show_gui = true;
  m_camera_zoom_speed  = 4.5f;

  m_ball_x = 0;
  m_ball_z = 0;

  std::random_device rd;
  std::mt19937 rng(rd());
  std::uniform_real_distribution<> dist(0.0, 2.0 * M_PIf);

  float init_angle = dist(rng);
  float ball_speed = 20.0f;

  m_ball_vx = ball_speed * cos(init_angle);
  m_ball_vz = ball_speed * sin(init_angle);

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

    // Geometry
    m_boundingbox_triangle_indexed  = m_ctx->createProgramFromPTXFile(ptxPath("boundingbox_triangle_indexed.cu"),  "boundingbox_triangle_indexed");
    m_intersection_triangle_indexed = m_ctx->createProgramFromPTXFile(ptxPath("intersection_triangle_indexed.cu"), "intersection_triangle_indexed");

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
  create_scene();
  m_ctx->validate();
  m_ctx->launch(0, 0, 0); // dummy launch to build everything
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

    // Demo code only!
    // Mind that these local OptiX objects will leak when not cleaning up the scene properly on changes.
    // Destroying the OptiX context will clean them up at program exit though.

    // Plane
    {
      optix::Geometry geoPlane = create_plane(1, 1);

      optix::GeometryInstance giPlane = m_ctx->createGeometryInstance(); // This connects Geometries with Materials.
      giPlane->setGeometry(geoPlane);
      giPlane->setMaterialCount(1);
      giPlane->setMaterial(0, m_opaque_mat);

      optix::Acceleration accPlane = m_ctx->createAcceleration(ACC_TYPE);
      setAccelerationProperties(accPlane);
      
      // This connects GeometryInstances with Acceleration structures. (All OptiX nodes with "Group" in the name hold an Acceleration.)
      optix::GeometryGroup ggPlane = m_ctx->createGeometryGroup();
      ggPlane->setAcceleration(accPlane);
      ggPlane->addChild(giPlane);

      // The original object coordinates of the plane have unit size, from -1.0f to 1.0f in x-axis and z-axis.
      // Scale the plane to go from -5 to 5.
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

    // Sphere
    {
      // Add a tessellated sphere with 180 longitudes and 90 latitudes (32400 triangles) with radius 1.0f around the origin.
      // The last argument is the maximum theta angle, which allows to generate spheres with a whole at the top.
      // (Useful to test thin-walled materials with different materials on the front- and backface.)
      optix::Geometry geoSphere = create_sphere(18, 9, 0.5f, M_PIf);

      optix::Acceleration accSphere = m_ctx->createAcceleration(ACC_TYPE);
      setAccelerationProperties(accSphere);
      
      optix::GeometryInstance giSphere = m_ctx->createGeometryInstance(); // This connects Geometries with Materials.
      giSphere->setGeometry(geoSphere);
      giSphere->setMaterialCount(1);
      giSphere->setMaterial(0, m_opaque_mat);

      optix::GeometryGroup ggSphere = m_ctx->createGeometryGroup();    // This connects GeometryInstances with Acceleration structures. (All OptiX nodes with "Group" in the name hold an Acceleration.)
      ggSphere->setAcceleration(accSphere);
      ggSphere->addChild(giSphere);

      float trafoSphere[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.5f, // Translate the sphere by 1.0f on the y-axis to be above the plane, exactly touching.
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
      };
      optix::Matrix4x4 matrixSphere(trafoSphere);

      trSphere = m_ctx->createTransform();
      trSphere->setChild(ggSphere);
      trSphere->setMatrix(false, matrixSphere.getData(), matrixSphere.inverse().getData());

      m_root_group->addChild(trSphere);
    }

    // Paddle
    {
      optix::Geometry geometry = create_cuboid(2, 1, 3);

      optix::Acceleration acceleration = m_ctx->createAcceleration(ACC_TYPE);
      setAccelerationProperties(acceleration);

      optix::GeometryInstance geometry_instance = m_ctx->createGeometryInstance();
      geometry_instance->setGeometry(geometry);
      geometry_instance->setMaterialCount(1);
      geometry_instance->setMaterial(0, m_opaque_mat);

      optix::GeometryGroup geometry_group = m_ctx->createGeometryGroup();
      geometry_group->setAcceleration(acceleration);
      geometry_group->addChild(geometry_instance);

      float T[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 1.5f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
      };
      optix::Matrix4x4 M(T);

      optix::Transform transform = m_ctx->createTransform();
      transform->setChild(geometry_group);
      transform->setMatrix(false, M.getData(), M.inverse().getData());

      m_root_group->addChild(transform);
    }
  });
}

void Application::run() {
  while (!glfwWindowShouldClose(m_window)) {

    // Adjust viewport to (potentially new) window dimensions.
    update_viewport();

    update_scene();

    render_scene();
    render_gui();

    // Swap the front and back buffers of the default (double-buffered) framebuffer.
    glfwSwapBuffers(m_window);
  }
  m_ctx->destroy();
}

void Application::handle_user_input() {
  glfwPollEvents();
  ImGuiIO const& io = ImGui::GetIO();

  // Toggle GUI visibility with keyboard button.
  if (ImGui::IsKeyPressed('G', false)) {
    m_show_gui = !m_show_gui;
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
      ImGui::DragFloat("Ball x", &m_ball_x, 0.2f, -10.0f, 10.0f, "%.1f");
      ImGui::DragFloat("Ball z", &m_ball_z, 0.2f, -10.0f, 10.0f, "%.1f");

      // if (ImGui::ColorEdit3("Background", (float *)&m_bg_color)) {
      //   m_ctx["sysColorBackground"]->setFloat(m_bg_color);
      // }
      ImGui::End();
    }
  });
}

void Application::update_scene() {
  const float dt = 0.02f;

  float width = 10.0f;
  float depth = 5.0f;
  float radius = 0.5f;

  float xmin = -width + radius;
  float xmax = width - radius;
  float zmin = -depth + radius;
  float zmax = depth - radius;

  // Potential future positions
  float fx = m_ball_x + dt * m_ball_vx;
  float fz = m_ball_z + dt * m_ball_vz;

  // TODO: Should give a score instead of bouncing for the x direction.
  if (fx <= xmin) {
    m_ball_x = xmin + -(fx - xmin);
    m_ball_vx = -m_ball_vx;
  }
  else if (fx >= xmax) {
    m_ball_x = xmax - (fx - xmax);
    m_ball_vx = -m_ball_vx;
  }
  else {
    m_ball_x = fx;
  }

  if (fz <= zmin) {
    m_ball_z = zmin + -(fz - zmin);
    m_ball_vz = -m_ball_vz;
  }
  else if (fz >= zmax) {
    m_ball_z = zmax - (fz - zmax);
    m_ball_vz = -m_ball_vz;
  }
  else {
    m_ball_z = fz;
  }

  optix::Matrix4x4 m = optix::Matrix4x4::translate(optix::make_float3(m_ball_x, radius, m_ball_z));
  trSphere->setMatrix(false, m.getData(), m.inverse().getData());
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

optix::Geometry Application::create_geometry(std::vector<VertexData> const& attributes, std::vector<unsigned int> const& indices) {
  optix::Geometry geometry(nullptr);

  run_unsafe_optix_code([&]() {
    geometry = m_ctx->createGeometry();

    optix::Buffer vertexBuffer = m_ctx->createBuffer(RT_BUFFER_INPUT, RT_FORMAT_USER);
    vertexBuffer->setElementSize(sizeof(VertexData));
    vertexBuffer->setSize(attributes.size());

    void *dst = vertexBuffer->map(0, RT_BUFFER_MAP_WRITE_DISCARD);
    memcpy(dst, attributes.data(), sizeof(VertexData) * attributes.size());
    vertexBuffer->unmap();

    optix::Buffer indicesBuffer = m_ctx->createBuffer(RT_BUFFER_INPUT, RT_FORMAT_UNSIGNED_INT3, indices.size() / 3);
    dst = indicesBuffer->map(0, RT_BUFFER_MAP_WRITE_DISCARD);
    memcpy(dst, indices.data(), sizeof(optix::uint3) * indices.size() / 3);
    indicesBuffer->unmap();

    geometry->setBoundingBoxProgram(m_boundingbox_triangle_indexed);
    geometry->setIntersectionProgram(m_intersection_triangle_indexed);

    geometry["vertexBuffer"]->setBuffer(vertexBuffer);
    geometry["indicesBuffer"]->setBuffer(indicesBuffer);
    geometry->setPrimitiveCount((unsigned int)(indices.size()) / 3);
  });

  return geometry;
}
