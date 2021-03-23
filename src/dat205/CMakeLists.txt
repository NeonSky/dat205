file(GLOB_RECURSE inc_files "include/*.h" "include/*.hpp")
file(GLOB_RECURSE src_files "src/*.c" "src/*.cpp")
file(GLOB_RECURSE cu_files "shaders/*.cu")
file(GLOB_RECURSE glsl_files "shaders/*.frag" "shaders/*.vert")

OPTIX_add_sample_executable(dat205
  src/main.cpp

  ${inc_files}
  ${src_files}
  ${cu_files}
  ${glsl_files}
  )

include_directories(
  "./include"
  )

# Copy GLSL shaders to build.
file(COPY "shaders" DESTINATION ".")
