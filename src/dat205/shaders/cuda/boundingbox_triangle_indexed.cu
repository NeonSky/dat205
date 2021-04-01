#include "common.cuh"

rtBuffer<VertexData> vertex_buffer;
rtBuffer<uint3>      index_buffer;

// Axis Aligned Bounding Box routine for indexed interleaved triangle data.
RT_PROGRAM void boundingbox_triangle_indexed(int primitiveIndex, float result[6]) {
  const uint3 indices = index_buffer[primitiveIndex];

  const float3 v0 = vertex_buffer[indices.x].position;
  const float3 v1 = vertex_buffer[indices.y].position;
  const float3 v2 = vertex_buffer[indices.z].position;

  const float area = optix::length(optix::cross(v1 - v0, v2 - v0));

  optix::Aabb *aabb = (optix::Aabb *) result;

  if (0.0f < area && !isinf(area)) {
    aabb->m_min = fminf(fminf(v0, v1), v2);
    aabb->m_max = fmaxf(fmaxf(v0, v1), v2);
  } else {
    aabb->invalidate();
  }
}
