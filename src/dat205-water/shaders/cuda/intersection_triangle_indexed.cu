#include "common.cuh"

// The vertex buffer and index buffer of the geometry to test against.
rtBuffer<VertexData> vertex_buffer;
rtBuffer<uint3>      index_buffer;

// The current ray.
rtDeclareVariable(optix::Ray, ray, rtCurrentRay, );

// Output attributes that we will define/write upon an intersection.
rtDeclareVariable(optix::float3, attr_geo_normal, attribute GEO_NORMAL, );
rtDeclareVariable(optix::float3, attr_tangent,    attribute TANGENT   , );
rtDeclareVariable(optix::float3, attr_normal,     attribute NORMAL    , );
rtDeclareVariable(optix::float3, attr_uv,         attribute TEX_UV    , );

// Checks for intersection against geometry made of indexed triangle data.
RT_PROGRAM void intersection_triangle_indexed(int primitive_index) {

  // Triangle vertices.
  const uint3 indices = index_buffer[primitive_index];
  VertexData const& v0 = vertex_buffer[indices.x];
  VertexData const& v1 = vertex_buffer[indices.y];
  VertexData const& v2 = vertex_buffer[indices.z];

  // Ray-triangle intersection test.
  float3 n;
  float  t;
  float  beta;
  float  gamma;

  if (intersect_triangle(ray, v0.position, v1.position, v2.position, n, t, beta, gamma)) { // NOTE: intersect_triangle() is defined in optixu_math_namespace.h. See: https://docs.nvidia.com/gameworks/content/gameworkslibrary/optix/optixapireference/optixu__math__namespace_8h.html
    if (rtPotentialIntersection(t)) {

      // Barycentric interpolation
      const float alpha = 1.0f - beta - gamma;

      // NOTE: We will normalize the results in the hit shaders.
      attr_geo_normal = n;
      attr_tangent    = v0.tangent  * alpha + v1.tangent  * beta + v2.tangent  * gamma;
      attr_normal     = v0.normal   * alpha + v1.normal   * beta + v2.normal   * gamma;
      attr_uv         = v0.uv       * alpha + v1.uv       * beta + v2.uv       * gamma;
      
      // Report intersection for material 0.
      rtReportIntersection(0);
    }
  }
}
