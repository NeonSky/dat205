#include "common.cuh"

// The current ray and its payload.
rtDeclareVariable(optix::Ray, ray, rtCurrentRay , );
rtDeclareVariable(RayPayload, payload, rtPayload, );

// Simulated particles.
rtBuffer<Particle> particles_buffer;

// Current particle.
rtDeclareVariable(Particle, attr_particle, attribute PARTICLE, );

// RT_PROGRAM void any_hit() {
//   payload.radiance = make_float3(0.0, 0.0, 0.0f);
// }

RT_PROGRAM void closest_hit() {
  // const float speed = optix::length(attr_particle.velocity);
  // payload.radiance = make_float3(speed, speed, 1.0f);

  // payload.radiance = make_float3(0.0f, 0.0f, 1.0f);
  payload.radiance = attr_particle.velocity;
}

RT_PROGRAM void bounding_box(int primitive_index, float result[6]) {
  optix::Aabb *aabb = (optix::Aabb *) result;

  const float3 pos = particles_buffer[primitive_index].position;

  // Enclose the particle with a cube.
  aabb->m_min = pos - make_float3(PARTICLE_RADIUS);
  aabb->m_max = pos + make_float3(PARTICLE_RADIUS);
}

RT_PROGRAM void ray_intersection(int primitive_index) {

  // Ray: r(t) = o + td
  // Sphere: ||p - c|| = r

  // We solve: dot(r(t) - c, r(t) - c) - r^2 = 0
  //
  // , which results in: t^2 + 2t * dot(o - c, d) + dot(o - c, o - c) - r^2 = 0
  //
  // , whose solution for t is: t = - dot(o - c, d) \pm sqrt(dot(o - c, d) * dot(o - c, d) - dot(o - c, o - c) + r^2)

  const float3 o = ray.origin;
  const float3 d = ray.direction;
  const float3 c = particles_buffer[primitive_index].position;
  const float r = PARTICLE_RADIUS;

  // Compute only once
  const float oc_d = optix::dot(o - c, d);
  const float inside_root_term = oc_d * oc_d- optix::dot(o - c, o - c) + r * r;

  // Ensure that any (intersection) solution exists.
  if (0.0f <= inside_root_term) {

    // Compute only once
    const float sqrt_term = sqrtf(inside_root_term);

    // Consider the two (usually different) possible solutions.
    const float t1 = -oc_d - sqrt_term;

    // Determine whether the reported hit distance is within the valid interval associated with the ray.
    if (rtPotentialIntersection(t1)) {
      attr_particle = particles_buffer[primitive_index];
      rtReportIntersection(0);
      return;
    }

    // NOTE: We might want to ignore the second root for performance reasons.
    const float t2 = -oc_d + sqrt_term;
    if (rtPotentialIntersection(t2)) {
      rtReportIntersection(0);
    }
  }
}