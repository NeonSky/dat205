#include "common.cuh"

// The current ray payload.
rtDeclareVariable(ShadowRayPayload, payload, rtPayload, );

RT_PROGRAM void any_hit() {
  payload.attenuation = make_float3(0.0f);
  rtTerminateRay();
}
