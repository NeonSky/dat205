#pragma once

// 1 when debugging, 0 for prod.
#define USE_DEBUG_EXCEPTIONS 0

#include <optix.h>
#include <optixu/optixu_aabb_namespace.h>
#include <optixu/optixu_math_namespace.h>
#include <optixu/optixu_matrix_namespace.h>

#include "device_include/random.h"

#define EPSILON 0.001f

// Ensures that helper functions are inline.
#define RT_FUNCTION __forceinline__ __device__

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
  float intensity;
};

struct Particle {
  optix::float3 position;
  optix::float3 velocity;
  float density;
  float pressure;
  unsigned int prev_hash_cell_index;
};

const unsigned int HASH_CELL_SIZE = 101;
typedef unsigned int HashCell[HASH_CELL_SIZE]; // First element represents size