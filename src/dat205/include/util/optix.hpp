#pragma once

#include "shaders/cuda/common.cuh"

// Contains helpers for loading compiled CUDA files and more.
#include <sutil.h>

#include <functional>

std::string ptxPath(std::string const& cuda_file);

void run_unsafe_optix_code(std::function<void()> f);
void unregister_buffer(optix::Buffer buffer, std::function<void()> f);
