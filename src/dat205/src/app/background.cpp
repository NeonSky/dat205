#include "app.hpp"

using namespace optix;

void Application::create_background_geometry() {
  std::string env_map_path = std::string(sutil::samplesDir()) + "/data/chinese_garden_2k.hdr";
  float3 default_color = make_float3(1.0f, 1.0f, 1.0f);
  m_ctx["env_map"]->setTextureSampler(sutil::loadTexture(m_ctx, env_map_path, default_color));

  Material mat = m_ctx->createMaterial();
  mat->setClosestHitProgram(0, m_ctx->createProgramFromPTXFile(ptxPath("closest_hit.cu"), "closest_hit"));

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
        10.0f, 0.0f, 0.0f, 0.0f,
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
  });
}