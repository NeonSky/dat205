#include "common.cuh"

// The current ray and its payload.
rtDeclareVariable(optix::Ray, ray, rtCurrentRay , );
rtDeclareVariable(RayPayload, payload, rtPayload, );

// The distance from the ray origin to where the intersection was detected.
rtDeclareVariable(float, ray_t, rtIntersectionDistance, );

// Ideally, we would use eq 5.20, but that would be too slow
rtDeclareVariable(float, particle_radius, , ); // [m]

// Point lights in the scene.
rtBuffer<PointLight> lights;

// Simulated particles.
rtBuffer<Particle> particles_buffer;

// Current particle.
rtDeclareVariable(Particle, attr_particle, attribute PARTICLE, );

// Render each particle as a lambertian surface using only direct illumination from the sun.
RT_PROGRAM void closest_hit() {
  float3 hit = ray.origin + ray_t * ray.direction;
  float3 n = optix::normalize(hit - attr_particle.position);
  float3 wi = optix::normalize(lights[0].position - hit);
  payload.radiance = make_float3(0.0f, 0.0f, 1.0f) * max(optix::dot(n, wi), 0.2f); // 0.2f is used to avoid pitch black pixels.

  // Kinda cool effect (also useful for debugging).
  // payload.radiance = make_float3(abs(attr_particle.velocity.x), abs(attr_particle.velocity.y), abs(attr_particle.velocity.z));
}

RT_PROGRAM void bounding_box(int primitive_index, float result[6]) {
  optix::Aabb *aabb = (optix::Aabb *) result;

  const float3 pos = particles_buffer[primitive_index].position;

  // Enclose the particle with a cube.
  aabb->m_min = pos - make_float3(particle_radius);
  aabb->m_max = pos + make_float3(particle_radius);
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
  const float r = particle_radius;

  // Compute only once
  const float oc_dot_d = optix::dot(o - c, d);
  const float inside_root_term = oc_dot_d * oc_dot_d- optix::dot(o - c, o - c) + r * r;

  // Ensure that any (intersection) solution exists.
  if (0.0f <= inside_root_term) {

    // Compute only once
    const float sqrt_term = sqrtf(inside_root_term);

    // Consider the two (usually different) possible solutions.
    const float t1 = -oc_dot_d - sqrt_term;

    // Determine whether the reported hit distance is within the valid interval associated with the ray.
    if (rtPotentialIntersection(t1)) {
      attr_particle = particles_buffer[primitive_index];
      rtReportIntersection(0);
      return;
    }

    // NOTE: We ignore the second root since t1 <= t2 = -oc_dot_d + sqrt_term, and distance is all that matters here.
  }
}