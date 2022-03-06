# Vulkan renderer (VR)engine 

Goal of this project is to learn Vulkan and Rendering in general.


TODO:

## Controls

* __WASD + Mouse__ to move horizontaly
* __ctrl/space__ to go up/down.

## Dependencies

* [Vulkan](https://www.lunarg.com/vulkan-sdk/)
* [GLM](https://github.com/g-truc/glm)
* [glfw3](https://www.glfw.org/)
* [Eigen3](https://eigen.tuxfamily.org/index.php?title=Main_Page)
* [spdlog](https://github.com/gabime/spdlog)

## Build

```
mkdir build
cd build
cmake ../
make
```

To run, go to root directory and execute ```./build/vulkan_fem```

Build tested on MacOS 11.6.
