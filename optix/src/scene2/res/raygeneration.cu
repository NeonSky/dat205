#include <optix.h>
#include <optixu/optixu_math_namespace.h>

rtBuffer<float4, 2> sysOutputBuffer; // RGBA32F

rtDeclareVariable(uint2, theLaunchIndex, rtLaunchIndex, );

rtDeclareVariable(float3, sysColorBackground, , );

// Entry point for simple color filling kernel.
RT_PROGRAM void raygeneration()
{
  sysOutputBuffer[theLaunchIndex] = make_float4(sysColorBackground, 1.0f);
}