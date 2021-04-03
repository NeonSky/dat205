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
rtDeclareVariable(float3, mat_emissive_coefficient, , );
rtDeclareVariable(float3, mat_ambient_coefficient , , );
rtDeclareVariable(float3, mat_diffuse_coefficient , , );
rtDeclareVariable(float3, mat_specular_coefficient, , );
rtDeclareVariable(float, mat_refractive_index     , , );
rtDeclareVariable(float, mat_fresnel              , , );
rtDeclareVariable(float, mat_transparency         , , );

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
  float3 color = mat_emissive_coefficient + mat_ambient_coefficient * ambient_light_color;

  // Flip the shading normal if we hit the backface of the triangle.
  if (optix::dot(-ray.direction, geo_normal) < 0.0f) {
    normal = -normal;
  }

  float3 hit = ray.origin + ray_t * ray.direction;

  // Direct illumination
  for (int i = 0; i < lights.size(); i++) {
    PointLight light = lights[i];
    float3 light_vec = optix::normalize(light.position - hit);

    // Add light from light if the lights is on the same side of the surface.
    float n_dot_l = optix::dot(normal, light_vec);
    if (0.0f < n_dot_l) {

      // Setup a shadow ray from the hit/intersection point, towards the current point light.
      float dist_to_light = optix::length(light.position - hit);
      optix::Ray shadow_ray(hit, light_vec, 1, SHADOW_EPSILON, dist_to_light);

      // Shoot the shadow ray
      ShadowRayPayload shadow_payload;
      shadow_payload.attenuation = make_float3(1.0f);

      rtTrace(root, shadow_ray, shadow_payload);

      if (0.0f < fmaxf(shadow_payload.attenuation)) {
        float3 light_color = shadow_payload.attenuation * light.color;
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

  // Indirect illumination from reflections
  if (0.0f < mat_fresnel) {
    float3 reflection_vec = optix::reflect(ray.direction, normal);
    float3 halfway_vec = optix::normalize((-ray.direction) + reflection_vec);

    // Fresnel
    float wo_dot_h = optix::dot(-ray.direction, halfway_vec);
    float F = mat_fresnel + (1.0f - mat_fresnel) * pow(1.0f - wo_dot_h, 5.0f);

    float importance = payload.importance * optix::luminance(make_float3(F));

    float importance_threshold = 0.01f;
    unsigned int max_depth = 5;
    if (importance_threshold <= importance && payload.recursion_depth < max_depth) {

      // Setup a reflection ray from the hit/intersection point
      optix::Ray reflection_ray(hit, reflection_vec, 0, EPSILON, RT_DEFAULT_MAX);

      // Shoot the reflection ray
      RayPayload reflection_payload;
      reflection_payload.importance      = importance;
      reflection_payload.recursion_depth = payload.recursion_depth + 1;

      rtTrace(root, reflection_ray, reflection_payload);

      color += F * reflection_payload.radiance;
    }
  }

  // Indirect illumination from refractions
  // TODO: randomize between reflection and refractions by sampling the Fresnel term. This will prevent ray branching.
  if (0.0f < mat_transparency) {
    float3 n = optix::normalize(rtTransformNormal(RT_OBJECT_TO_WORLD, attr_normal));

    float3 refraction_vec;
    bool total_internal_reflection = !optix::refract(refraction_vec, ray.direction, n, mat_refractive_index);

    if (!total_internal_reflection) {

      // External or internal reflection?
      float cos_theta = optix::dot(ray.direction, n);
      if (cos_theta < 0.0f) {
        cos_theta = -cos_theta;
      } else {
        cos_theta = optix::dot(refraction_vec, n);
      }

      float F = mat_fresnel + (1.0f - mat_fresnel) * pow(1.0f - cos_theta, 5.0f);

      float importance = payload.importance * (1.0f - F) * optix::luminance(make_float3(1.0f));

      const float importance_threshold = 0.01f;
      const unsigned int max_depth = 5;
      if (importance_threshold <= importance && payload.recursion_depth < max_depth) {
        optix::Ray refraction_ray(hit, refraction_vec, 0, EPSILON, RT_DEFAULT_MAX);

        // Shoot the refraction ray
        RayPayload refraction_payload;
        refraction_payload.importance      = payload.importance;
        refraction_payload.recursion_depth = payload.recursion_depth + 1;

        rtTrace(root, refraction_ray, refraction_payload);

        color += mat_transparency * (1.0f - F) * refraction_payload.radiance;
      }
    }
  }

  payload.radiance = color;
}
