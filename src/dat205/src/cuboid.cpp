#include "app.hpp"

using namespace optix;

#include <iostream>

Geometry Application::create_cuboid(float width, float height, float depth) {
  assert(0 < width && 0 < height && 0 < depth);

  std::vector<VertexAttributes> vertices;

  // The corner is assumed to be the bot-left corner of the face.
  auto add_face = [&vertices](float3 corner, float3 right, float3 up) {
    VertexAttributes v;
    v.normal = cross(right, up);
    v.tangent = right;

    // Left-bot
    v.vertex = corner;
    vertices.push_back(v);

    // Right-bot
    v.vertex = corner + right;
    vertices.push_back(v);

    // Left-top
    v.vertex = corner + up;
    vertices.push_back(v);

    // Right-top
    v.vertex = corner + right + up;
    vertices.push_back(v);
  };

  // Left face
  float3 corner = make_float3(-width / 2.0f, -height / 2.0f, -depth / 2.0f);
  float3 right  = make_float3(0.0f, 0.0f, depth);
  float3 up     = make_float3(0.0f, height, 0.0f);
  add_face(corner, right, up);

  // Right face
  corner = make_float3(width / 2.0f, -height / 2.0f, depth / 2.0f);
  right  = make_float3(0.0f, 0.0f, -depth);
  up     = make_float3(0.0f, height, 0.0f);
  add_face(corner, right, up);

  // Bottom face
  corner = make_float3(-width / 2.0f, -height / 2.0f, -depth / 2.0f);
  right  = make_float3(width, 0.0f, 0.0f);
  up     = make_float3(0.0f, 0.0f, depth);
  add_face(corner, right, up);

  // Top face
  corner = make_float3(-width / 2.0f, height / 2.0f, depth / 2.0f);
  right  = make_float3(width, 0.0f, 0.0f);
  up     = make_float3(0.0f, 0.0f, -depth);
  add_face(corner, right, up);

  // Back face
  corner = make_float3(width / 2.0f, -height / 2.0f, -depth / 2.0f);
  right  = make_float3(-width, 0.0f, 0.0f);
  up     = make_float3(0.0f, height, 0.0f);
  add_face(corner, right, up);

  // Front face
  corner = make_float3(-width / 2.0f, -height / 2.0f, depth / 2.0f);
  right  = make_float3(width, 0.0f, 0.0f);
  up     = make_float3(0.0f, height, 0.0f);
  add_face(corner, right, up);

  std::vector<unsigned int> indices;
  for (int f = 0; f < 6; f++) {
    int offset = 4*f;

    // Bot-right triangle
    indices.push_back(offset + 0);
    indices.push_back(offset + 1);
    indices.push_back(offset + 3);

    // Top-left triangle
    indices.push_back(offset + 0);
    indices.push_back(offset + 3);
    indices.push_back(offset + 2);
  }

  assert(vertices.size() == 4*6);
  assert(indices.size() == 6*6);

  return create_geometry(vertices, indices);
}