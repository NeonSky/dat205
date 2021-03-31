#pragma once

#include "util/optix.hpp"

struct Ball {
  float x;
  float z;
  float radius;
  float vx;
  float vz;
};

struct Player {
  int score;
  float paddle_z;
};

class PongGame {
public:
  PongGame(float width, float height);

  void create_geometry(OptixScene &scene, optix::Group &parent_group);
  void update(float dt, float paddle1_dz, float paddle2_dz);
  void render();

private:
  // Model
  const float m_table_width;
  const float m_table_depth;

  const float m_paddle_width;
  const float m_paddle_height;
  const float m_paddle_depth;
  const float m_paddle_x_offset;

  const float m_initial_ball_speed;

  Player *m_winner;

  Player m_player1;
  Player m_player2;
  Ball m_ball;

  // View
  optix::Material m_paddle_material;
  optix::Material m_ball_material;

  optix::Transform m_paddle1_transform;
  optix::Transform m_paddle2_transform;
  optix::Transform m_ball_transform;

  void update_ball(float dt);
  void update_paddles(float dt, float paddle1_dz, float paddle2_dz);

};