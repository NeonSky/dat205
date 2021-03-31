#pragma once

// 1 when debugging, 0 for prod.
#define USE_DEBUG_EXCEPTIONS 1

#include <optix.h>
#include <optixu/optixu_aabb_namespace.h>
#include <optixu/optixu_math_namespace.h>
#include <optixu/optixu_matrix_namespace.h>

struct PerRayData {
  optix::float3 radiance; // Radiance along the current path segment.
};

struct VertexData {
  optix::float3 position;
  optix::float3 tangent;
  optix::float3 normal;
  optix::float3 texcoord;
};