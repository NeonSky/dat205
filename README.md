# DAT205 - Advanced Computer Graphics

## Dependencies

* C++11
* CUDA 9.0
* OptiX SDK 5.1.1 (https://developer.nvidia.com/designworks/optix/downloads/5.1.1/linux64)

```
yay -S glfw-x11 devil assimp cuda-9.0
```

## Build
```
mkdir build
cd build
export CC=/usr/bin/gcc-6
export CXX=/usr/bin/g++-6
export CUDA_PATH=/opt/cuda-9.0
cmake ../src
make
```

## Rebuild
```
cmake ../src && make dat205
```

## Run
```
cd build
./bin/dat205
```
