#include "app.hpp"

using namespace optix;

// TODO: Move to game
void Application::create_background_geometry() {
  std::string env_map_path = std::string(sutil::samplesDir()) + "/data/chinese_garden_2k.hdr";
  float3 default_color = make_float3(1.0f, 1.0f, 1.0f);
  m_ctx["env_map"]->setTextureSampler(sutil::loadTexture(m_ctx, env_map_path, default_color));

  Material mat = m_ctx->createMaterial();
  mat->setClosestHitProgram(0, m_ctx->createProgramFromPTXFile(ptxPath("closest_hit.cu"), "closest_hit"));
  mat->setAnyHitProgram(1, m_ctx->createProgramFromPTXFile(ptxPath("any_hit.cu"), "any_hit"));
  mat["mat_color"]->setFloat(1.0f, 1.0f, 1.0f);
  mat["mat_emission"]->setFloat(0.3f);
  mat["mat_metalness"]->setFloat(0.4f);
  mat["mat_shininess"]->setFloat(5.0f);
  mat["mat_transparency"]->setFloat(0.0f);
  mat["mat_reflectivity"]->setFloat(0.7f);
  mat["mat_fresnel"]->setFloat(0.2f);
  mat["mat_refractive_index"]->setFloat(1.3f);

  run_unsafe_optix_code([&]() {
    // Floor
    {
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

      float T[16] = {
        8.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 5.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
      };
      Matrix4x4 M(T);

      Transform t = m_ctx->createTransform();
      t->setChild(geometry_group);
      t->setMatrix(false, M.getData(), M.inverse().getData());
      
      // Add the transform node placeing the plane to the scene's root Group node.
      m_root_group->addChild(t);
    }

    // South and north walls
    {
      auto add_wall = [&](float z_offset, float height, float fresnel) {
        Material mat = m_ctx->createMaterial();
        mat->setClosestHitProgram(0, m_ctx->createProgramFromPTXFile(ptxPath("closest_hit.cu"), "closest_hit"));
        mat->setAnyHitProgram(1, m_ctx->createProgramFromPTXFile(ptxPath("any_hit.cu"), "any_hit"));
        mat["mat_color"]->setFloat(1.0f, 1.0f, 1.0f);
        mat["mat_emission"]->setFloat(0.05f);
        mat["mat_metalness"]->setFloat(0.0f);
        mat["mat_shininess"]->setFloat(0.0f);
        mat["mat_transparency"]->setFloat(0.8f);
        mat["mat_reflectivity"]->setFloat(0.8f);
        mat["mat_fresnel"]->setFloat(fresnel);
        mat["mat_refractive_index"]->setFloat(1.45f);

        Geometry geometry = m_scene->create_cuboid(16, height, 1);

        GeometryInstance geometry_instance = m_ctx->createGeometryInstance();
        geometry_instance->setGeometry(geometry);
        geometry_instance->setMaterialCount(1);
        geometry_instance->setMaterial(0, mat);

        Acceleration acceleration = m_ctx->createAcceleration(ACC_TYPE);
        set_acceleration_properties(acceleration);
        
        GeometryGroup geometry_group = m_ctx->createGeometryGroup();
        geometry_group->setAcceleration(acceleration);
        geometry_group->addChild(geometry_instance);

        float T[16] = {
          1.0f, 0.0f, 0.0f, 0.0f,
          0.0f, 1.0f, 0.0f, 0.0f,
          0.0f, 0.0f, 1.0f, z_offset,
          0.0f, 0.0f, 0.0f, 1.0f
        };
        Matrix4x4 M(T);

        Transform t = m_ctx->createTransform();
        t->setChild(geometry_group);
        t->setMatrix(false, M.getData(), M.inverse().getData());

        // Add the transform node placeing the plane to the scene's root Group node.
        m_root_group->addChild(t);
      };

      add_wall(5.5f, 2.0f, 0.2f);
      add_wall(-5.5f, 4.0f, 0.0f);
    }

    // West and east walls
    {
      auto add_wall = [&](float x_offset, float3 color) {
        Material mat = m_ctx->createMaterial();
        mat->setClosestHitProgram(0, m_ctx->createProgramFromPTXFile(ptxPath("closest_hit.cu"), "closest_hit"));
        mat->setAnyHitProgram(1, m_ctx->createProgramFromPTXFile(ptxPath("any_hit.cu"), "any_hit"));
        mat["mat_color"]->setFloat(color);
        mat["mat_emission"]->setFloat(0.8f);
        mat["mat_metalness"]->setFloat(0.0f);
        mat["mat_shininess"]->setFloat(0.0f);
        mat["mat_transparency"]->setFloat(0.0f);
        mat["mat_reflectivity"]->setFloat(0.0f);
        mat["mat_fresnel"]->setFloat(0.0f);
        mat["mat_refractive_index"]->setFloat(1.0f);

        Geometry geometry = m_scene->create_cuboid(1, 1, 12);

        GeometryInstance geometry_instance = m_ctx->createGeometryInstance();
        geometry_instance->setGeometry(geometry);
        geometry_instance->setMaterialCount(1);
        geometry_instance->setMaterial(0, mat);

        Acceleration acceleration = m_ctx->createAcceleration(ACC_TYPE);
        set_acceleration_properties(acceleration);
        
        GeometryGroup geometry_group = m_ctx->createGeometryGroup();
        geometry_group->setAcceleration(acceleration);
        geometry_group->addChild(geometry_instance);

        float T[16] = {
          1.0f, 0.0f, 0.0f, x_offset,
          0.0f, 1.0f, 0.0f, 0.0f,
          0.0f, 0.0f, 1.0f, 0.0f,
          0.0f, 0.0f, 0.0f, 1.0f
        };
        Matrix4x4 M(T);

        Transform t = m_ctx->createTransform();
        t->setChild(geometry_group);
        t->setMatrix(false, M.getData(), M.inverse().getData());

        // Add the transform node placeing the plane to the scene's root Group node.
        m_root_group->addChild(t);
      };

      // NOTE: We apply a small offset to avoid z-fighting.
      add_wall(-8.501f, make_float3(1.0f, 0.0f, 0.0f));
      add_wall(8.501f, make_float3(0.0f, 0.0f, 1.0f));
    }

  });
}