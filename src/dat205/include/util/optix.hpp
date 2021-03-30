#pragma once

#include "shaders/cuda/common.cuh"

// Contains helpers for loading compiled CUDA files and more.
#include <sutil.h>

#include <functional>

std::string ptxPath(std::string const& cuda_file);

void unregister_buffer(optix::Buffer buffer, std::function<void()> f);
