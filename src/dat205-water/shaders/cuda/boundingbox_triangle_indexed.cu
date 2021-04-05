#include "common.cuh"

// The vertex buffer and index buffer of the geometry to test against.
rtBuffer<VertexData> vertex_buffer;
rtBuffer<uint3>      index_buffer;

// Builds an AABB for geometry made of indexed triangle data.
RT_PROGRAM void boundingbox_triangle_indexed(int primitive_index, float result[6]) {
  optix::Aabb *aabb = (optix::Aabb *) result;

  // Triangle vertices.
  const uint3 indices = index_buffer[primitive_index];
  const float3 v0 = vertex_buffer[indices.x].position;
  const float3 v1 = vertex_buffer[indices.y].position;
  const float3 v2 = vertex_buffer[indices.z].position;

  const float area = optix::length(optix::cross(v1 - v0, v2 - v0)) / 2.0f;

  if (area <= 0.0f || isinf(area)) {
    aabb->invalidate();
  }
  
  aabb->m_min = fminf(fminf(v0, v1), v2); // Combine the smallest component values into the min boundary point.
  aabb->m_max = fmaxf(fmaxf(v0, v1), v2); // Combine the greatest component values into the max boundary point.
}
