#include "common.cuh"

// Root object of the scene.
rtDeclareVariable(rtObject, root, , );

// The current ray and its payload.
rtDeclareVariable(optix::Ray, ray, rtCurrentRay , );
rtDeclareVariable(RayPayload, payload, rtPayload, );

// Attributes from intersection test.
rtDeclareVariable(optix::float3, attr_geo_normal, attribute GEO_NORMAL, );
rtDeclareVariable(optix::float3, attr_tangent,    attribute TANGENT   , );
rtDeclareVariable(optix::float3, attr_normal,     attribute NORMAL    , );
rtDeclareVariable(optix::float3, attr_uv,         attribute TEX_UV    , );

RT_PROGRAM void closest_hit() {

  // Transform the (unnormalized) object space normals into world space.
  float3 geo_normal = optix::normalize(rtTransformNormal(RT_OBJECT_TO_WORLD, attr_geo_normal));
  float3 normal     = optix::normalize(rtTransformNormal(RT_OBJECT_TO_WORLD, attr_normal));

  // Flip the shading normal if we hit the backface of the triangle.
  if (0.0f < optix::dot(ray.direction, geo_normal)) {
    normal = -normal;
  }

  // Transform the normal components from [-1.0f, 1.0f] to the range [0.0f, 1.0f] and visualize as radiance.
  payload.radiance = normal * 0.5f + 0.5f;
}
