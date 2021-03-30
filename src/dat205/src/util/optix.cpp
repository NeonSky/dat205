#include "util/optix.hpp"

#include <iostream>

std::string ptxPath(std::string const& cuda_file) {
  return std::string(sutil::samplesPTXDir()) + std::string("/") + "dat205" + std::string("_generated_") + cuda_file + std::string(".ptx");
}

void run_unsafe_optix_code(std::function<void()> f) {
  try {
    f();
  } catch (optix::Exception& e) {
    std::cerr << e.getErrorString() << std::endl;
  }
}

void unregister_buffer(optix::Buffer buffer, std::function<void()> f) {
  run_unsafe_optix_code([&]() {
    buffer->unregisterGLBuffer();
    f();
    buffer->registerGLBuffer();
  });
}