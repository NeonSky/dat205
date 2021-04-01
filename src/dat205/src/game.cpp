#include "game.hpp"

#include <random>

using namespace optix;

PongGame::PongGame(float width, float depth)
      : m_table_width(width),
        m_table_depth(depth),
        m_paddle_width(0.5f),
        m_paddle_height(0.6f),
        m_paddle_depth(2.0f),
        m_paddle_x_offset(7.5f),
        m_paddle_speed(16.0f),
        m_initial_ball_speed(15.0f),
        m_score_to_win(3) {

  reset();
}

void PongGame::create_geometry(OptixScene &scene, Group &parent_group) {
  Context& ctx = scene.context();

  m_paddle_material = ctx->createMaterial();
  m_paddle_material->setClosestHitProgram(0, ctx->createProgramFromPTXFile(ptxPath("closesthit.cu"), "closesthit"));

  m_ball_material = ctx->createMaterial();
  m_ball_material->setClosestHitProgram(0, ctx->createProgramFromPTXFile(ptxPath("closesthit.cu"), "closesthit"));

  // Create the two paddles.
  {
    Acceleration acceleration = ctx->createAcceleration(ACC_TYPE);
    set_acceleration_properties(acceleration);

    GeometryGroup geometry_group = ctx->createGeometryGroup();
    geometry_group->setAcceleration(acceleration);

    // Let the paddles also share geometry.
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

  // Create the ball.
  {
    Acceleration acceleration = ctx->createAcceleration(ACC_TYPE);
    set_acceleration_properties(acceleration);

    GeometryGroup geometry_group = ctx->createGeometryGroup();
    geometry_group->setAcceleration(acceleration);

    GeometryInstance geometry_instance = ctx->createGeometryInstance();
    geometry_instance->setMaterialCount(1);
    geometry_instance->setMaterial(0, m_ball_material);
    geometry_group->addChild(geometry_instance);

    Geometry geometry = scene.create_sphere(18, 9, m_ball.radius, M_PIf);
    geometry_instance->setGeometry(geometry);

    Matrix4x4 M = Matrix<4, 4>::translate(make_float3(0.0f, m_ball.radius, 0.0f));

    m_ball_transform = ctx->createTransform();
    m_ball_transform->setChild(geometry_group);
    m_ball_transform->setMatrix(false, M.getData(), M.inverse().getData());

    parent_group->addChild(m_ball_transform);
  }
}

void PongGame::update(float dt, float paddle1_dz, float paddle2_dz) {
  // Nothing to update if we already have a winner.
  if (winner() != 0) {
    return;
  }

  update_paddles(dt, m_paddle_speed * paddle1_dz, m_paddle_speed * paddle2_dz);
  update_ball(dt);
}

void PongGame::update_ball(float dt) {
    // Wall constraints.
    float x_min = -m_table_width + m_ball.radius;
    float x_max =  m_table_width - m_ball.radius;
    float z_min = -m_table_depth + m_ball.radius;
    float z_max =  m_table_depth - m_ball.radius;

    // Potential future positions.
    float fx = m_ball.x + dt * m_ball.vx;
    float fz = m_ball.z + dt * m_ball.vz;

    // Update position along x-axis.
    if (fx <= x_min) {
      m_player2.score++;
      reset_ball();
      return;
    }
    else if (fx >= x_max) {
      m_player1.score++;
      reset_ball();
      return;
    }

    m_ball.x = fx;

    // Update position along z-axis.
    if (fz <= z_min) {
      m_ball.z = z_min + -(fz - z_min);
      m_ball.vz = -m_ball.vz;
    }
    else if (fz >= z_max) {
      m_ball.z = z_max - (fz - z_max);
      m_ball.vz = -m_ball.vz;
    }
    else {
      m_ball.z = fz;
    }

    // Bounce against paddles if intersection occurs.
    // We'll give the ball a square collision box for simplicity.
    auto check_paddle_collision = [&](float paddle_x, float paddle_z, float dir) {
      bool x_close = abs(m_ball.x - paddle_x) < m_ball.radius + m_paddle_width / 2.0f;
      bool z_close = abs(m_ball.z - paddle_z) < m_ball.radius + m_paddle_depth / 2.0f;

      if (x_close && z_close) {
        m_ball.vx = dir * abs(m_ball.vx);

        // Slightly disturb the ball's direction to spice up the gameplay.
        static std::random_device rd;
        static std::mt19937 rng(rd());
        static std::uniform_real_distribution<> dist(-0.5, 0.5);

        float vz_disturbance = dist(rng);
        m_ball.vz += m_initial_ball_speed * vz_disturbance;

        float2 vel = m_initial_ball_speed * normalize(make_float2(m_ball.vx, m_ball.vz));
        m_ball.vx = vel.x;
        m_ball.vz = vel.y;
      }
    };

    check_paddle_collision(-m_paddle_x_offset, m_player1.paddle_z, 1.0f);
    check_paddle_collision(m_paddle_x_offset, m_player2.paddle_z, -1.0f);

    Matrix4x4 M = optix::Matrix4x4::translate(make_float3(m_ball.x, m_ball.radius, m_ball.z));
    m_ball_transform->setMatrix(false, M.getData(), M.inverse().getData());
}

void PongGame::update_paddles(float dt, float paddle1_dz, float paddle2_dz) {
  float z_min = -m_table_depth + m_paddle_depth / 2.0f;
  float z_max = m_table_depth - m_paddle_depth / 2.0f;

  // Paddle 1
  {
    m_player1.paddle_z = clamp(m_player1.paddle_z + dt * paddle1_dz, z_min, z_max);
    Matrix4x4 M = optix::Matrix4x4::translate(make_float3(-m_paddle_x_offset, m_paddle_height, m_player1.paddle_z));
    m_paddle1_transform->setMatrix(false, M.getData(), M.inverse().getData());
  }

  // Paddle 2
  {
    m_player2.paddle_z = clamp(m_player2.paddle_z + dt * paddle2_dz, z_min, z_max);
    Matrix4x4 M = optix::Matrix4x4::translate(make_float3(m_paddle_x_offset, m_paddle_height, m_player2.paddle_z));
    m_paddle2_transform->setMatrix(false, M.getData(), M.inverse().getData());
  }
}

void PongGame::reset() {
  m_player1 = {};
  m_player2 = {};

  reset_ball();
}

int PongGame::player1_score() {
  return m_player1.score;
}

int PongGame::player2_score() {
  return m_player2.score;
}

int PongGame::winner() {
  if (m_player1.score >= m_score_to_win) {
    return 1;
  }
  if (m_player2.score >= m_score_to_win) {
    return 2;
  }
  return 0;
}

#include <iostream>

void PongGame::reset_ball() {
  m_ball = {};
  m_ball.radius = 0.5f;

  static std::random_device rd;
  static std::mt19937 rng(rd());
  static std::uniform_real_distribution<> dist(-M_PIf / 4.0f,  M_PIf / 4.0f);

  int dir = 2*(rng() % 2) - 1;
  float init_angle = dir * dist(rng);

  m_ball.vx = m_initial_ball_speed * cos(init_angle);
  m_ball.vz = m_initial_ball_speed * sin(init_angle);
}
