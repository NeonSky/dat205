#include "app.hpp"

#include <cstring>
#include <iostream>
#include <sstream>

optix::Geometry Application::create_plane(const int tessU, const int tessV) {
  assert(1 <= tessU && 1 <= tessV);

  const float uTile = 2.0f / float(tessU);
  const float vTile = 2.0f / float(tessV);
  
  optix::float3 corner;

  std::vector<VertexData> vertices;
  
  VertexData v;

  // Positive y-axis is the geometry normal, create geometry on the xz-plane.
  corner = optix::make_float3(-1.0f, 0.0f, 1.0f); // left front corner of the plane. texcoord (0.0f, 0.0f).

  v.tangent = optix::make_float3(1.0f, 0.0f, 0.0f);
  v.normal = optix::make_float3(0.0f, 1.0f, 0.0f);

  for (int j = 0; j <= tessV; ++j) {
    const float tv = float(j) * vTile;

    for (int i = 0; i <= tessU; ++i) {
      const float tu = float(i) * uTile;

      v.position = corner + optix::make_float3(tu, 0.0f, -tv);
      v.texcoord = optix::make_float3(tu * 0.5f, tv * 0.5f, 0.0f);

      vertices.push_back(v);
    }
  }

  std::vector<unsigned int> indices;
  
  const unsigned int stride = tessU + 1;
  for (int j = 0; j < tessV; ++j) {
    for (int i = 0; i < tessU; ++i) {
      indices.push_back( j      * stride + i    );
      indices.push_back( j      * stride + i + 1);
      indices.push_back((j + 1) * stride + i + 1);

      indices.push_back((j + 1) * stride + i + 1);
      indices.push_back((j + 1) * stride + i    );
      indices.push_back( j      * stride + i    );
    }
  }

  std::cout << "createPlane(): Vertices = " << vertices.size() <<  ", Triangles = " << indices.size() / 3 << std::endl;

  return create_geometry(vertices, indices);
}
