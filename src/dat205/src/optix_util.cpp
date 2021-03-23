#include "optix_util.hpp"

#include <sutil.h>

#include <iostream>

std::string ptxPath(std::string const& cuda_file) {
  return std::string(sutil::samplesPTXDir()) + std::string("/") + "dat205" + std::string("_generated_") + cuda_file + std::string(".ptx");
}

void unregister_buffer(optix::Buffer buffer, std::function<void()> f) {
  try {
    buffer->unregisterGLBuffer();
    f();
    buffer->registerGLBuffer();
  }
  catch (optix::Exception& e) {
    std::cerr << e.getErrorString() << std::endl;
  }
}