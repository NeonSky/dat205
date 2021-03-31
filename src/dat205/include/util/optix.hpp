#pragma once

#include "shaders/cuda/common.cuh"

// Contains helpers for loading compiled CUDA files and more.
#include <sutil.h>

#include <functional>

// Our default acceleration structure.
#define ACC_TYPE "Trbvh"

std::string ptxPath(std::string const& cuda_file);

void run_unsafe_optix_code(std::function<void()> f);
void set_acceleration_properties(optix::Acceleration acceleration);
void unregister_buffer(optix::Buffer buffer, std::function<void()> f);


class OptixScene {
public:
  OptixScene(optix::Context& ctx);

  optix::Context& context();

  optix::Geometry create_cuboid(float width, float height, float depth);
  optix::Geometry create_plane(const int tessU, const int tessV);
  optix::Geometry create_sphere(const int tessU, const int tessV, const float radius, const float maxTheta);
  optix::Geometry create_geometry(std::vector<VertexData> const& attributes, std::vector<unsigned int> const& indices);

private:
  optix::Context& m_ctx;
  optix::Program m_boundingbox_triangle_indexed;
  optix::Program m_intersection_triangle_indexed;
};