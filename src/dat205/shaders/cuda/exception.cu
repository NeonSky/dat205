#include "common.cuh"

// The 2D color (RGBA32F) buffer we will render our result to.
rtBuffer<float4, 2> output_buffer;

// Current pixel index.
rtDeclareVariable(uint2, launch_index, rtLaunchIndex, );

RT_PROGRAM void exception() {

  // See common.cuh
  #if USE_DEBUG_EXCEPTIONS

  const unsigned int err_code = rtGetExceptionCode();
  if (RT_EXCEPTION_USER <= err_code) {
    rtPrintf("User exception %d at (%d, %d)\n", err_code - RT_EXCEPTION_USER, launch_index.x, launch_index.y);
  } else {
    rtPrintf("Exception code 0x%X at (%d, %d)\n", err_code, launch_index.x, launch_index.y);
  }

  // Write a very bright magenta as error color (makes sure that the error color is not accumulated away in a progressive renderer).
  output_buffer[launch_index] = make_float4(1000000.0f, 0.0f, 1000000.0f, 1.0f);

  #endif
}