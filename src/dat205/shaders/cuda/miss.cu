#include "common.cuh"

rtDeclareVariable(PerRayData, thePrd, rtPayload, );


RT_PROGRAM void miss_environment_constant() {
  thePrd.radiance = make_float3(0.0f, 0.0f, 1.0f);
}
