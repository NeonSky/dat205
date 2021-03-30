#pragma once

#include <optix.h>
#include <optixu/optixpp_namespace.h>
#include <optixu/optixu_math_namespace.h>
#include <optixu/optixu_matrix_namespace.h>

#include <functional>

std::string ptxPath(std::string const& cuda_file);

void unregister_buffer(optix::Buffer buffer, std::function<void()> f);
