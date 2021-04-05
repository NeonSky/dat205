#include "common.cuh"

// The 2D color (RGBA32F) buffer we will render our result to.
rtBuffer<float4,  2> output_buffer;

// Root object of the scene.
rtDeclareVariable(rtObject, root, , );

// Output buffer/screen dimensionality and current pixel index.
rtDeclareVariable(uint2, launch_dim,   rtLaunchDim  , );
rtDeclareVariable(uint2, launch_index, rtLaunchIndex, ); // NOTE: launchIndex = (0, 0) is the bot-left corner (thus, matches OpenGL's texture origin) of the buffer/screen.

// Camera.
rtDeclareVariable(float3, camera_pos    , , );
rtDeclareVariable(float3, camera_right  , , );
rtDeclareVariable(float3, camera_up     , , );
rtDeclareVariable(float3, camera_forward, , );

// Entry point for this ray tracing kernel.
RT_PROGRAM void ray_generation() {

  const float2 pixel = make_float2(launch_index);
  const float2 pixel_center = pixel + make_float2(0.5f);

  // Shoots jittered samples to approximate the true pixel radiance.
  const unsigned int n_samples = 1;
  float3 tot_radiance = make_float3(0.0f);

  for (int i = 0; i < n_samples; i++) {

    // Generate a small offset from the pixel center.
    unsigned int seed = launch_dim.x * launch_index.x + launch_dim.y * launch_index.y + i;
    float2 subpixel_jitter = make_float2(rnd(seed) - 0.5f, rnd(seed) - 0.5f);
    subpixel_jitter *= 0.5f; // Looks a bit better with this when only using a single sample.

    // Derive the ray direction for the sampled subpixel.
    const float2 screen = make_float2(launch_dim);
    const float2 ndc = ((pixel_center + subpixel_jitter) / screen) * 2.0f - 1.0f; // [-1, 1] Normalized Device Coordinates
    const float3 direction = optix::normalize(ndc.x * camera_right + ndc.y * camera_up + camera_forward); // NOTE: the direction must be normalized.

    // Setup ray from origin, towards direction, using ray type 0, and test the interval between 0.0f and RT_DEFAULT_MAX for intersections.
    optix::Ray ray = optix::make_Ray(camera_pos, direction, 0, 0.0f, RT_DEFAULT_MAX);

    // Shoot the ray, storing the result in payload.
    RayPayload payload;
    payload.radiance        = make_float3(0.0f);
    payload.importance      = 1.0f;
    payload.recursion_depth = 0;

    rtTrace(root, ray, payload);

    tot_radiance += payload.radiance;
  }

  // Write result to output buffer.
  output_buffer[launch_index] = make_float4(tot_radiance / n_samples, 1.0f);
}