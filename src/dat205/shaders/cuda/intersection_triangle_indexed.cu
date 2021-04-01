#include "common.cuh"

rtBuffer<VertexData> vertex_buffer;
rtBuffer<uint3>      index_buffer;

rtDeclareVariable(optix::Ray, ray, rtCurrentRay, );

rtDeclareVariable(optix::float3, attr_geo_normal, attribute GEO_NORMAL, );
rtDeclareVariable(optix::float3, attr_tangent,    attribute TANGENT, );
rtDeclareVariable(optix::float3, attr_normal,     attribute NORMAL, ); 
rtDeclareVariable(optix::float3, attr_uv,         attribute TEXCOORD, ); 

// Intersection routine for indexed interleaved triangle data.
RT_PROGRAM void intersection_triangle_indexed(int primitiveIndex) {
  const uint3 indices = index_buffer[primitiveIndex];

  VertexData const& a0 = vertex_buffer[indices.x];
  VertexData const& a1 = vertex_buffer[indices.y];
  VertexData const& a2 = vertex_buffer[indices.z];

  const float3 v0 = a0.position;
  const float3 v1 = a1.position;
  const float3 v2 = a2.position;

  float3 n;
  float  t;
  float  beta;
  float  gamma;

  if (intersect_triangle(ray, v0, v1, v2, n, t, beta, gamma)) {
    if (rtPotentialIntersection(t)) {
      // Barycentric interpolation:
      const float alpha = 1.0f - beta - gamma;

      // Note: No normalization on the TBN attributes here for performance reasons.
      //       It's done after the transformation into world space anyway.
      attr_geo_normal = n;
      attr_tangent    = a0.tangent  * alpha + a1.tangent  * beta + a2.tangent  * gamma;
      attr_normal     = a0.normal   * alpha + a1.normal   * beta + a2.normal   * gamma;
      attr_uv         = a0.uv       * alpha + a1.uv       * beta + a2.uv       * gamma;
      
      rtReportIntersection(0);
    }
  }
}
