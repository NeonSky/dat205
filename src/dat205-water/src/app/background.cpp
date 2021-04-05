#include "app.hpp"

#define _USE_MATH_DEFINES
 
#include <cmath>

using namespace optix;

void Application::create_background_geometry() {
  std::string env_map_path = std::string(sutil::samplesDir()) + "/data/chinese_garden_2k.hdr";
  float3 default_color = make_float3(1.0f, 1.0f, 1.0f);
  m_ctx["env_map"]->setTextureSampler(sutil::loadTexture(m_ctx, env_map_path, default_color));

  Material mat = m_ctx->createMaterial();
  mat->setClosestHitProgram(0, m_ctx->createProgramFromPTXFile(ptxPath("closest_hit.cu"), "closest_hit"));
  mat->setAnyHitProgram(1, m_ctx->createProgramFromPTXFile(ptxPath("any_hit.cu"), "any_hit"));
  mat["mat_color"]->setFloat(1.0f, 1.0f, 1.0f);
  mat["mat_emission"]->setFloat(0.05f);
  mat["mat_metalness"]->setFloat(0.0f);
  mat["mat_shininess"]->setFloat(0.0f);
  mat["mat_transparency"]->setFloat(0.8f);
  mat["mat_reflectivity"]->setFloat(0.8f);
  mat["mat_fresnel"]->setFloat(0.2f);
  mat["mat_refractive_index"]->setFloat(1.0f);

  run_unsafe_optix_code([&]() {
    Geometry geometry = m_scene->create_plane(1, 1);

    GeometryInstance geometry_instance = m_ctx->createGeometryInstance();
    geometry_instance->setGeometry(geometry);
    geometry_instance->setMaterialCount(1);
    geometry_instance->setMaterial(0, mat);

    Acceleration acceleration = m_ctx->createAcceleration(ACC_TYPE);
    set_acceleration_properties(acceleration);
    
    GeometryGroup geometry_group = m_ctx->createGeometryGroup();
    geometry_group->setAcceleration(acceleration);
    geometry_group->addChild(geometry_instance);

    auto add_glass_pane = [&](Matrix4x4 M) {
      Transform t = m_ctx->createTransform();
      t->setChild(geometry_group);
      t->setMatrix(false, M.getData(), M.inverse().getData());

      m_root_group->addChild(t);
    };

    // Dimensions of water simulation
    float width  = 5.0f;
    float height = 1.5f;
    float depth  = 5.0f;

    // Floor
    add_glass_pane(Matrix<4, 4>::rotate(M_PI, make_float3(0.0f, 0.0f, 1.0f)) * Matrix<4, 4>::scale(make_float3(width, 1.0f, depth)));

    // Left wall
    add_glass_pane(
        Matrix<4, 4>::translate(make_float3(-width, height, 0.0f)) *
        Matrix<4, 4>::rotate(M_PI_2, make_float3(0.0f, 0.0f, 1.0f)) *
        Matrix<4, 4>::scale(make_float3(height, 1.0f, width))
    );

    // Right wall
    add_glass_pane(
        Matrix<4, 4>::translate(make_float3(width, height, 0.0f)) *
        Matrix<4, 4>::rotate(-M_PI_2, make_float3(0.0f, 0.0f, 1.0f)) *
        Matrix<4, 4>::scale(make_float3(height, 1.0f, width))
    );

    // Far wall
    add_glass_pane(
        Matrix<4, 4>::translate(make_float3(0.0f, height, -depth)) *
        Matrix<4, 4>::rotate(-M_PI_2, make_float3(1.0f, 0.0f, 0.0f)) *
        Matrix<4, 4>::scale(make_float3(width, 1.0f, height))
    );

    // Near wall
    add_glass_pane(
        Matrix<4, 4>::translate(make_float3(0.0f, height, depth)) *
        Matrix<4, 4>::rotate(M_PI_2, make_float3(1.0f, 0.0f, 0.0f)) *
        Matrix<4, 4>::scale(make_float3(width, 1.0f, height))
    );

  });
}