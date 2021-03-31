#include "game.hpp"

using namespace optix;

PongGame::PongGame(float width, float depth)
      : m_table_width(width),
        m_table_depth(depth),
        m_paddle_width(0.5f),
        m_paddle_height(0.6f),
        m_paddle_depth(2.0f),
        m_paddle_x_offset(4.5f) {

  m_winner = nullptr;
}

void PongGame::create_geometry(OptixScene &scene, Group &parent_group) {
  Context& ctx = scene.context();

  m_paddle_material = ctx->createMaterial();
  m_paddle_material->setClosestHitProgram(0, ctx->createProgramFromPTXFile(ptxPath("closesthit.cu"), "closesthit"));

  m_ball_material = ctx->createMaterial();
  m_ball_material->setClosestHitProgram(0, ctx->createProgramFromPTXFile(ptxPath("closesthit.cu"), "closesthit"));

  // Create a sared geometry group between all game objects.
  Acceleration acceleration = ctx->createAcceleration(ACC_TYPE);
  set_acceleration_properties(acceleration);

  GeometryGroup geometry_group = ctx->createGeometryGroup();
  geometry_group->setAcceleration(acceleration);

  // Create the two paddles.
  {
    // Let the paddles share geometry.
    GeometryGroup geometry_group = ctx->createGeometryGroup();
    geometry_group->setAcceleration(acceleration);

    GeometryInstance geometry_instance = ctx->createGeometryInstance();
    geometry_instance->setMaterialCount(1);
    geometry_instance->setMaterial(0, m_paddle_material);
    geometry_group->addChild(geometry_instance);

    Geometry geometry = scene.create_cuboid(m_paddle_width, m_paddle_height, m_paddle_depth);
    geometry_instance->setGeometry(geometry);

    // Create individual transforms.
    auto create_paddle_transform = [&](Transform &t, int xscale) {
      Matrix4x4 M = Matrix<4, 4>::translate(make_float3(xscale * m_paddle_x_offset, m_paddle_height, 0.0f));

      t = ctx->createTransform();
      t->setChild(geometry_group);
      t->setMatrix(false, M.getData(), M.inverse().getData());

      parent_group->addChild(t);
    };

    create_paddle_transform(m_paddle1_transform, -1);
    create_paddle_transform(m_paddle2_transform, 1);
  }

  // Create the ball. TODO
}

void PongGame::update(float dt, float paddle1_dz, float paddle2_dz) {
  float z_min = -m_table_depth / 2.0f;
  float z_max = m_table_depth / 2.0f;

  // Paddle 1
  m_player1.paddle_z = clamp(m_player1.paddle_z + paddle1_dz, z_min, z_max);

  // Paddle 2
  m_player2.paddle_z = clamp(m_player2.paddle_z + paddle2_dz, z_min, z_max);
}