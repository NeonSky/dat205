#include "common.cuh"

rtDeclareVariable(RayPayload, payload, rtPayload, );

RT_PROGRAM void miss_environment_constant() {
  payload.radiance = make_float3(0.0f, 0.0f, 1.0f);
}
