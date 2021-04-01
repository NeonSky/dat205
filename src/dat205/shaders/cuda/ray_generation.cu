#include "common.cuh"

rtBuffer<float4,  2> output_buffer; // RGBA32F

rtDeclareVariable(rtObject, root, , );

rtDeclareVariable(uint2, launch_dim,   rtLaunchDim, );
rtDeclareVariable(uint2, launch_index, rtLaunchIndex, );

rtDeclareVariable(float3, camera_pos, , );
rtDeclareVariable(float3, camera_right, , );
rtDeclareVariable(float3, camera_up, , );
rtDeclareVariable(float3, camera_forward, , );

// Entry point for simple color filling kernel.
RT_PROGRAM void ray_generation() {

  RayPayload payload;

  payload.radiance = make_float3(0.0f);
  
  // The launch index is the pixel coordinate.
  // Note that launchIndex = (0, 0) is the bottom left corner of the image,
  // which matches the origin in the OpenGL texture used to display the result.
  const float2 pixel = make_float2(launch_index);
  // Sample the ray in the center of the pixel.
  const float2 fragment = pixel + make_float2(0.5f);
  // The launch dimension (set with rtContextLaunch) is the full client window in this demo's setup.
  const float2 screen = make_float2(launch_dim);
  // Normalized device coordinates in range [-1, 1].
  const float2 ndc = (fragment / screen) * 2.0f - 1.0f;

  const float3 origin    = camera_pos;
  const float3 direction = optix::normalize(ndc.x * camera_right + ndc.y * camera_up + camera_forward);

  // Shoot a ray from origin into direction (must always be normalized!) for ray type 0 and test the interval between 0.0f and RT_DEFAULT_MAX for intersections.
  optix::Ray ray = optix::make_Ray(origin, direction, 0, 0.0f, RT_DEFAULT_MAX);

  // Start the ray traversal at the scene's root node.
  // The ray becomes the variable with rtCurrentRay semantic in the other program domains.
  // The PerRayData becomes the variable with the semantic rtPayload in the other program domains,
  // which allows to exchange arbitrary data between the program domains.
  rtTrace(root, ray, payload);

  output_buffer[launch_index] = make_float4(payload.radiance, 1.0f);
}