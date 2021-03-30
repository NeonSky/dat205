#include "app.hpp"

#include <cstring>
#include <iostream>
#include <sstream>

optix::Geometry Application::create_plane(const int tessU, const int tessV) {
  assert(1 <= tessU && 1 <= tessV);

  const float uTile = 2.0f / float(tessU);
  const float vTile = 2.0f / float(tessV);
  
  optix::float3 corner;

  std::vector<VertexAttributes> attributes;
  
  VertexAttributes attrib;

  // Positive y-axis is the geometry normal, create geometry on the xz-plane.
  corner = optix::make_float3(-1.0f, 0.0f, 1.0f); // left front corner of the plane. texcoord (0.0f, 0.0f).

  attrib.tangent = optix::make_float3(1.0f, 0.0f, 0.0f);
  attrib.normal = optix::make_float3(0.0f, 1.0f, 0.0f);

  for (int j = 0; j <= tessV; ++j) {
    const float v = float(j) * vTile;

    for (int i = 0; i <= tessU; ++i) {
      const float u = float(i) * uTile;

      attrib.vertex = corner + optix::make_float3(u, 0.0f, -v);
      attrib.texcoord = optix::make_float3(u * 0.5f, v * 0.5f, 0.0f);

      attributes.push_back(attrib);
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

  std::cout << "createPlane(): Vertices = " << attributes.size() <<  ", Triangles = " << indices.size() / 3 << std::endl;

  return create_geometry(attributes, indices);
}
