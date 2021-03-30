#include <optix.h>
#include <optixu/optixu_math_namespace.h>

#include "common.cuh"

rtBuffer<float4,  2> sysOutputBuffer; // RGBA32F

rtDeclareVariable(rtObject, sysTopObject, , );

rtDeclareVariable(uint2, theLaunchDim,   rtLaunchDim, );
rtDeclareVariable(uint2, theLaunchIndex, rtLaunchIndex, );

rtDeclareVariable(float3, sysCameraPosition, , );
rtDeclareVariable(float3, sysCameraU, , );
rtDeclareVariable(float3, sysCameraV, , );
rtDeclareVariable(float3, sysCameraW, , );

// Entry point for simple color filling kernel.
RT_PROGRAM void raygeneration() {

  PerRayData prd;

  prd.radiance = make_float3(0.0f);
  
  // The launch index is the pixel coordinate.
  // Note that launchIndex = (0, 0) is the bottom left corner of the image,
  // which matches the origin in the OpenGL texture used to display the result.
  const float2 pixel = make_float2(theLaunchIndex);
  // Sample the ray in the center of the pixel.
  const float2 fragment = pixel + make_float2(0.5f);
  // The launch dimension (set with rtContextLaunch) is the full client window in this demo's setup.
  const float2 screen = make_float2(theLaunchDim);
  // Normalized device coordinates in range [-1, 1].
  const float2 ndc = (fragment / screen) * 2.0f - 1.0f;

  const float3 origin    = sysCameraPosition;
  const float3 direction = optix::normalize(ndc.x * sysCameraU + ndc.y * sysCameraV + sysCameraW);

  // Shoot a ray from origin into direction (must always be normalized!) for ray type 0 and test the interval between 0.0f and RT_DEFAULT_MAX for intersections.
  optix::Ray ray = optix::make_Ray(origin, direction, 0, 0.0f, RT_DEFAULT_MAX);

  // Start the ray traversal at the scene's root node.
  // The ray becomes the variable with rtCurrentRay semantic in the other program domains.
  // The PerRayData becomes the variable with the semantic rtPayload in the other program domains,
  // which allows to exchange arbitrary data between the program domains.
  rtTrace(sysTopObject, ray, prd);

  sysOutputBuffer[theLaunchIndex] = make_float4(prd.radiance, 1.0f);
}