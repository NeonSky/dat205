#include "common.cuh"

rtBuffer<VertexData> vertexBuffer;
rtBuffer<uint3>      indicesBuffer;

// Attributes.
rtDeclareVariable(optix::float3, varGeoNormal, attribute GEO_NORMAL, );
rtDeclareVariable(optix::float3, varTangent,   attribute TANGENT, );
rtDeclareVariable(optix::float3, varNormal,    attribute NORMAL, ); 
rtDeclareVariable(optix::float3, varTexCoord,  attribute TEXCOORD, ); 

rtDeclareVariable(optix::Ray, theRay, rtCurrentRay, );

// Intersection routine for indexed interleaved triangle data.
RT_PROGRAM void intersection_triangle_indexed(int primitiveIndex) {
  const uint3 indices = indicesBuffer[primitiveIndex];

  VertexData const& a0 = vertexBuffer[indices.x];
  VertexData const& a1 = vertexBuffer[indices.y];
  VertexData const& a2 = vertexBuffer[indices.z];

  const float3 v0 = a0.position;
  const float3 v1 = a1.position;
  const float3 v2 = a2.position;

  float3 n;
  float  t;
  float  beta;
  float  gamma;

  if (intersect_triangle(theRay, v0, v1, v2, n, t, beta, gamma))
  {
    if (rtPotentialIntersection(t))
    {
      // Barycentric interpolation:
      const float alpha = 1.0f - beta - gamma;

      // Note: No normalization on the TBN attributes here for performance reasons.
      //       It's done after the transformation into world space anyway.
      varGeoNormal      = n;
      varTangent        = a0.tangent  * alpha + a1.tangent  * beta + a2.tangent  * gamma;
      varNormal         = a0.normal   * alpha + a1.normal   * beta + a2.normal   * gamma;
      varTexCoord       = a0.texcoord * alpha + a1.texcoord * beta + a2.texcoord * gamma;
      
      rtReportIntersection(0);
    }
  }
}
