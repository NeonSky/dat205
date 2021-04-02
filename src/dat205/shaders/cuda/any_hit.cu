#include "common.cuh"

// The current ray payload.
rtDeclareVariable(RayPayload, payload, rtPayload, );

RT_PROGRAM void any_hit() {
  payload.radiance = make_float3(0.0f);
  rtTerminateRay();
}
