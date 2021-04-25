#include "common.cuh"

// Root object of the scene.
rtDeclareVariable(rtObject, root, , );

// The current ray and its payload.
rtDeclareVariable(optix::Ray, ray, rtCurrentRay , );
rtDeclareVariable(RayPayload, payload, rtPayload, );

// The distance from the ray origin to where the intersection was detected.
rtDeclareVariable(float, ray_t, rtIntersectionDistance, );

// Point lights in the scene.
rtBuffer<PointLight> lights;

// Properties of the hit surface's material.
rtDeclareVariable(float3, mat_color          , , );
rtDeclareVariable(float, mat_emission        , , ); // 0 = no emission       , 1 = full emission
rtDeclareVariable(float, mat_metalness       , , ); // 0 = dieletric         , 1 = metal
rtDeclareVariable(float, mat_shininess       , , ); // 0 = smeared highlight , 1 = dense highlight
rtDeclareVariable(float, mat_transparency    , , ); // 0 = opaque            , 1 = transparent
rtDeclareVariable(float, mat_reflectivity    , , ); // 0 = diffuse           , 1 = mirror
rtDeclareVariable(float, mat_fresnel         , , ); // 0 = only absorptions  , 1 = only reflections (when looking from directly above)
rtDeclareVariable(float, mat_refractive_index, , ); // 1.0 = air material

// Attributes from intersection test.
rtDeclareVariable(optix::float3, attr_geo_normal, attribute GEO_NORMAL, );
rtDeclareVariable(optix::float3, attr_tangent,    attribute TANGENT   , );
rtDeclareVariable(optix::float3, attr_normal,     attribute NORMAL    , );
rtDeclareVariable(optix::float3, attr_uv,         attribute TEX_UV    , );

RT_FUNCTION float fresnel(float wo_dot_h) {
  float F = mat_fresnel + (1.0f - mat_fresnel) * pow(1.0f - wo_dot_h, 5.0f); // Schlick's approximation.
  return F;
}

RT_FUNCTION float torrance_sparrow_brdf(float n_dot_wi,
                                        float n_dot_wo,
                                        float n_dot_wh,
                                        float wo_dot_wh) {

  float F = fresnel(wo_dot_wh);
  float D = ((mat_shininess + 2.0f) / (2.0f * M_PIf)) * pow(n_dot_wh, mat_shininess);
  float G = min(1.0f, min(2.0f * n_dot_wh * n_dot_wo / wo_dot_wh, 2.0f * n_dot_wh * n_dot_wi / wo_dot_wh));

  float denominator = 4.0f * n_dot_wo * n_dot_wi;

  return F * D * G / denominator;
}

RT_FUNCTION float3 direct_illumination(float3 const& wo,
                                       float3 const& hit,
                                       float3 const& n) {

  float3 illumination = make_float3(0.0f);

  // Do not illuminate the backface of a triangle.
  float n_dot_wo  = optix::dot(n, wo);
  if (n_dot_wo <= 0.0f) {
    return illumination;
  }

  for (int i = 0; i < lights.size(); i++) {
    PointLight light = lights[i];

    // Ensure that the light could illuminate the front face.
    float3 wi = optix::normalize(light.position - hit);
    float n_dot_wi = optix::dot(n, wi);
    if (n_dot_wi <= 0.0f) {
      continue;
    }

    // Setup a shadow ray from the hit/intersection point, towards the current point light.
    float dist_to_light = optix::length(light.position - hit);
    optix::Ray shadow_ray(hit, wi, 1, EPSILON, dist_to_light);

    // Shoot the shadow ray
    ShadowRayPayload shadow_payload;
    shadow_payload.attenuation = make_float3(1.0f);

    rtTrace(root, shadow_ray, shadow_payload);

    // Ensure that the shadow ray is not entirely absorbed along the way.
    if (fmaxf(shadow_payload.attenuation) <= 0.0f) {
      continue;
    }

    // Compute direct illumination from the light
    float3 light_illumination = shadow_payload.attenuation * (light.intensity / pow(dist_to_light, 2.0f)) * light.color;

    // Compute BRDF
    float3 wh = optix::normalize(wo + wi);

    float n_dot_wh  = optix::dot(n, wh);
    float wo_dot_wh = optix::dot(wo, wh);

    float F = fresnel(wo_dot_wh);
    float D = ((mat_shininess + 2.0f) / (2.0f * M_PIf)) * pow(n_dot_wh, mat_shininess);
    float G = min(1.0f, min(2.0f * n_dot_wh * n_dot_wo / wo_dot_wh, 2.0f * n_dot_wh * n_dot_wi / wo_dot_wh));
    float denominator = 4.0f * n_dot_wo * n_dot_wi;

    float brdf = F * D * G / denominator;

    // Material models
    float3 diffuse_model    = mat_color * M_1_PIf * n_dot_wi * light_illumination;
    float3 dieletric_model  = brdf * n_dot_wi * light_illumination + (1.0f - F) * diffuse_model;
    float3 metal_model      = brdf * mat_color * n_dot_wi * light_illumination;
    float3 microfacet_model = mat_metalness * metal_model + (1.0f - mat_metalness) * dieletric_model;

    // Apply a linear blend between a perfectly diffuse surface and a microfacet brdf.
    float3 material_model = mat_reflectivity * microfacet_model + (1.0f - mat_reflectivity) * diffuse_model;

    illumination += material_model;
  }

  return illumination;
}

RT_FUNCTION float3 indirect_illumination(float3 const& wo,
                                         float3 const& hit,
                                         float3 const& n) {

  float3 illumination = make_float3(0.0f);

  // Indirect illumination from reflections
  if (0.0f < mat_reflectivity && payload.recursion_depth < 3) {
    float3 wi = optix::reflect(ray.direction, n);
    float3 wh = optix::normalize(wo + wi);

    // Fresnel
    float wo_dot_wh = max(0.01f, optix::dot(wo, wh));
    float F = mat_reflectivity * fresnel(wo_dot_wh);

    float importance = payload.importance;
    float importance_threshold = 0.1f;
    if (importance_threshold <= importance) {

      // Setup a reflection ray from the hit/intersection point
      optix::Ray reflection_ray(hit, wi, 0, EPSILON, RT_DEFAULT_MAX);

      // Shoot the reflection ray
      RayPayload reflection_payload;
      reflection_payload.importance      = importance;
      reflection_payload.recursion_depth = payload.recursion_depth + 1;

      rtTrace(root, reflection_ray, reflection_payload);

      float3 mirror_model = F * reflection_payload.radiance;
      float3 metal_model  = mat_color * F * reflection_payload.radiance;

      float3 material_model = mat_metalness * metal_model + (1.0f - mat_metalness) * mirror_model;

      illumination += material_model;
    }
  }

  // Indirect illumination from refractions
  int max_recursion_depth = 5;
  if (0.0f < mat_transparency && payload.recursion_depth < max_recursion_depth) {
    float3 wi;
    bool total_internal_reflection = !optix::refract(wi, ray.direction, n, mat_refractive_index); // For optix::refract(), see https://docs.nvidia.com/gameworks/content/gameworkslibrary/optix/optixapireference/optixu__math__namespace_8h_source.html

    // float cos_theta = 0.0f;

    // Refraction with Schlick’s approximation reference (page 4): https://graphics.stanford.edu/courses/cs148-10-summer/docs/2006--degreve--reflection_refraction.pdf

    float F = 1.0f; // Fresnel of TIR

    if (total_internal_reflection) {
      wi = optix::reflect(ray.direction, n); // optix::refract() sets `wi = make_float3(0.0f)` on TIR, so we need to manually define `wi` in this case.
    }
    else {
      // External -> Internal (i.e. n1 <= n2)
      if(optix::dot(wo, n) <= 0.0f) {
        F = fresnel(optix::dot(wi, n));
      }

      // Internal -> External (i.e. n1 > n2)
      else {
        F = fresnel(optix::dot(wo, n));
      }
    }

    float importance = payload.importance * (1.0f - F);

    const float importance_threshold = 0.1f;
    if (importance_threshold <= importance) {
      optix::Ray refraction_ray(hit, wi, 0, EPSILON, RT_DEFAULT_MAX);

      // Shoot the refraction ray
      RayPayload refraction_payload;
      refraction_payload.importance      = payload.importance;
      refraction_payload.recursion_depth = payload.recursion_depth + 1;

      rtTrace(root, refraction_ray, refraction_payload);

      illumination += mat_transparency * (1.0f - F) * refraction_payload.radiance;
    }
  }

  return illumination;
}

RT_PROGRAM void closest_hit() {

  // Transform the (unnormalized) object space normals into world space.
  float3 geo_normal = optix::normalize(rtTransformNormal(RT_OBJECT_TO_WORLD, attr_geo_normal));
  float3 normal     = optix::normalize(rtTransformNormal(RT_OBJECT_TO_WORLD, attr_normal));

  float3 wo = -ray.direction;
  float3 hit = ray.origin + ray_t * ray.direction;

  // Base color, regardless if the surface is exposed to light or not.
  float3 color = make_float3(0.0f);

  color += mat_color * mat_emission;
  color += direct_illumination(wo, hit, normal);
  color += indirect_illumination(wo, hit, normal);

  payload.radiance = color;
}