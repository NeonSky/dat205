#include "common.cuh"

// Root object of the scene.
rtDeclareVariable(rtObject, root, , );

// The current ray and its payload.
rtDeclareVariable(optix::Ray, ray, rtCurrentRay , );
rtDeclareVariable(RayPayload, payload, rtPayload, );

// The distance from the ray origin to where the intersection was detected.
rtDeclareVariable(float, ray_t, rtIntersectionDistance, );

// Point lights in the scene.
rtDeclareVariable(float3, ambient_light_color, , );
rtBuffer<PointLight> lights;

// Properties of the hit surface's material.
rtDeclareVariable(float3, mat_ambient_coefficient, , );
rtDeclareVariable(float3, mat_diffuse_coefficient, , );
rtDeclareVariable(float3, mat_specular_coefficient, , );

// Attributes from intersection test.
rtDeclareVariable(optix::float3, attr_geo_normal, attribute GEO_NORMAL, );
rtDeclareVariable(optix::float3, attr_tangent,    attribute TANGENT   , );
rtDeclareVariable(optix::float3, attr_normal,     attribute NORMAL    , );
rtDeclareVariable(optix::float3, attr_uv,         attribute TEX_UV    , );

RT_PROGRAM void closest_hit() {

  // Transform the (unnormalized) object space normals into world space.
  float3 geo_normal = optix::normalize(rtTransformNormal(RT_OBJECT_TO_WORLD, attr_geo_normal));
  float3 normal     = optix::normalize(rtTransformNormal(RT_OBJECT_TO_WORLD, attr_normal));

  // Base color, regardless if the surface is exposed to light or not.
  float3 color = mat_ambient_coefficient * ambient_light_color;

  // Flip the shading normal if we hit the backface of the triangle.
  if (0.0f < optix::dot(ray.direction, geo_normal)) {
    normal = -normal;
  }

  float3 hit = ray.origin + ray_t * ray.direction;

  for (int i = 0; i < lights.size(); i++) {
    PointLight light = lights[i];
    float3 light_vec = optix::normalize(light.position - hit);

    // Add light from light if the lights is on the same side of the surface.
    float n_dot_l = optix::dot(normal, light_vec);
    if (0 < n_dot_l) {

      // Setup a shadow ray from the hit/intersection point, towards the current point light.
      float dist_to_light = optix::length(light.position - hit);
      optix::Ray shadow_ray(hit, light_vec, 1, EPSILON, dist_to_light);

      // Shoot the shadow ray
      RayPayload shadow_payload;
      shadow_payload.radiance = make_float3(1.0f);
      rtTrace(root, shadow_ray, shadow_payload);

      if (0.0f < fmaxf(shadow_payload.radiance)) {
        float3 light_color = shadow_payload.radiance * light.color;
        color += mat_diffuse_coefficient * n_dot_l * light_color;

        // Phong highlight
        float3 halfway_vec = optix::normalize((-ray.direction) + light_vec);
        float n_dot_h = optix::dot(normal, halfway_vec);
        if (0 < n_dot_h) {
          float highlight_sharpness = 88.0f;
          color += mat_specular_coefficient * light.color * pow(n_dot_h, highlight_sharpness);
        }
      }

    }
  }

  payload.radiance = color;
}
