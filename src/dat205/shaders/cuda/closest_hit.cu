#include "common.cuh"

rtDeclareVariable(rtObject, root, , );

rtDeclareVariable(optix::Ray, ray, rtCurrentRay, );
rtDeclareVariable(RayPayload, payload, rtPayload, );

// Attributes.
rtDeclareVariable(optix::float3, attr_geo_normal, attribute GEO_NORMAL, );
//rtDeclareVariable(optix::float3, attr_tangent,   attribute TANGENT, );
rtDeclareVariable(optix::float3, attr_normal,    attribute NORMAL, );
//rtDeclareVariable(optix::float3, attr_uv,  attribute TEXCOORD, ); 

// This closest hit program only uses the geometric normal and the shading normal attributes.
// OptiX will remove all code from the intersection programs for unused attributes automatically.

// Note that the matching between attribute outputs from the intersection program and 
// the inputs in the closesthit and anyhit programs is done with the type (here float3) and
// the user defined attribute semantic (e.g. here NORMAL). 
// The actual variable name doesn't need to match but it's recommended for clarity.

RT_PROGRAM void closest_hit() {

  // Transform the (unnormalized) object space normals into world space.
  float3 geo_normal = optix::normalize(rtTransformNormal(RT_OBJECT_TO_WORLD, attr_geo_normal));
  float3 normal    = optix::normalize(rtTransformNormal(RT_OBJECT_TO_WORLD, attr_normal));

  // Check if the ray hit the geometry on the frontface or the backface.
  // The geometric normal is always defined on the front face of the geometry.
  // In this implementation the coordinate systems are right-handed and the frontface triangle winding is counter-clockwise (matching OpenGL).

  // If ray.direction and geometric normal are in the same hemisphere we're looking at a backface.
  if (0.0f < optix::dot(ray.direction, geo_normal)) {
    // Flip the shading normal to the backface, because only that is used below.
    // (See later examples for more intricate handling of the frontface condition.)
    normal = -normal;
  }

  // Visualize the resulting world space normal on the surface we're looking on.
  // Transform the normal components from [-1.0f, 1.0f] to the range [0.0f, 1.0f] to get colors for negative values.
  payload.radiance = normal * 0.5f + 0.5f;
}
