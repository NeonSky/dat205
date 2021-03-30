#pragma once

// 1 when debugging, 0 for prod.
#define USE_DEBUG_EXCEPTIONS 1

#include <optix.h>
#include <optixu/optixu_aabb_namespace.h>
#include <optixu/optixu_math_namespace.h>

struct PerRayData {
  optix::float3 radiance; // Radiance along the current path segment.
};

struct VertexAttributes {
  optix::float3 vertex;
  optix::float3 tangent;
  optix::float3 normal;
  optix::float3 texcoord;
};