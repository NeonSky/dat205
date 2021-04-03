#pragma once

// 1 when debugging, 0 for prod.
#define USE_DEBUG_EXCEPTIONS 1

#include <optix.h>
#include <optixu/optixu_aabb_namespace.h>
#include <optixu/optixu_math_namespace.h>
#include <optixu/optixu_matrix_namespace.h>

#include "device_include/random.h"

#define EPSILON 1.0e-4f
#define SHADOW_EPSILON 0.1f

struct RayPayload {
  optix::float3 radiance;
  float importance;
  int recursion_depth;
};

struct ShadowRayPayload {
  optix::float3 attenuation;
};

struct VertexData {
  optix::float3 position;
  optix::float3 tangent;
  optix::float3 normal;
  optix::float3 uv;
};

struct PointLight {
  optix::float3 position;
  optix::float3 color;
};