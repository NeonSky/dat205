#include "common.cuh"

rtBuffer<float4, 2> output_buffer; // RGBA32F

rtDeclareVariable(uint2, launch_index, rtLaunchIndex, );

RT_PROGRAM void exception() {

  const unsigned int err_code = rtGetExceptionCode();
  if (RT_EXCEPTION_USER <= err_code) {
    rtPrintf("User exception %d at (%d, %d)\n", err_code - RT_EXCEPTION_USER, launch_index.x, launch_index.y);
  } else {
    rtPrintf("Exception code 0x%X at (%d, %d)\n", err_code, launch_index.x, launch_index.y);
  }

  // RGBA32F super magenta as error color (makes sure this isn't accumulated away in a progressive renderer).
  output_buffer[launch_index] = make_float4(1000000.0f, 0.0f, 1000000.0f, 1.0f);
}