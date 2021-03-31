#pragma once

#include "util/optix.hpp"

struct Ball {
  float x;
  float z;
  float radius;
};

struct Player {
  int score;
  float paddle_z;
};

class PongGame {
public:
  PongGame(float width, float height);

  void create_geometry(OptixScene &scene, optix::Group &parent_group);
  void update(float dt, float player1_input_dz, float player2_input_dz);
  void render();

private:
  // Model
  const float m_table_width;
  const float m_table_depth;

  const float m_paddle_width;
  const float m_paddle_height;
  const float m_paddle_depth;
  const float m_paddle_x_offset;

  Player *m_winner;

  Player m_player1;
  Player m_player2;
  Ball m_ball;

  // View
  optix::Acceleration m_acceleration;
  optix::GeometryGroup m_geometry_group;

  optix::Material m_paddle_material;
  optix::Material m_ball_material;

  optix::Transform m_paddle1_transform;
  optix::Transform m_paddle2_transform;
  optix::Transform m_ball_transform;
};