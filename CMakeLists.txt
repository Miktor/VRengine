cmake_minimum_required(VERSION 3.7 FATAL_ERROR)

set(CMAKE_ENABLE_EXPORTS on)

project(vrengine VERSION 0.1.0)

enable_language(CXX)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(external)

find_package(Vulkan REQUIRED)
find_package(GLM CONFIG REQUIRED)

file(GLOB_RECURSE SOURCES
     "src/*.h"
     "src/*.cpp"
)
AUX_SOURCE_DIRECTORY(src SOURCES)

add_library(vrengine_core OBJECT ${SOURCES})

target_compile_definitions(vrengine_core PRIVATE GLM_FORCE_DEPTH_ZERO_TO_ONE)
target_compile_definitions(vrengine_core PRIVATE VK_DEBUG)

IF (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    target_compile_options (vrengine_core PRIVATE
        -Wall 
        -Wextra 
        -Wpedantic
        -Wno-nullability-extension 
        -Wno-missing-field-initializers 
        -Wno-nullability-completeness 
        -Wno-macro-redefined
        -Wno-nullability-extension 
        -Wno-nullability-completeness
    )
ENDIF()

IF(WIN32)
    target_compile_definitions(vrengine_core PRIVATE
        VOLK_STATIC_DEFINES 
        VK_USE_PLATFORM_WIN32_KHR 
        WIN32_LEAN_AND_MEAN 
        NOMINMAX
    )
ELSEIF(APPLE)
    target_compile_definitions(vrengine_core PRIVATE VK_USE_PLATFORM_MACOS_MVK)
ENDIF()

target_include_directories(vrengine_core PRIVATE
    src
    Vulkan::Vulkan
    external
)

target_link_libraries(vrengine_core PRIVATE Vulkan::Vulkan)
target_link_libraries(vrengine_core PRIVATE spdlog::spdlog)
target_link_libraries(vrengine_core PRIVATE glfw)
target_link_libraries(vrengine_core PRIVATE shaderc)
target_link_libraries(vrengine_core PRIVATE spirv-cross-core)
target_link_libraries(vrengine_core PRIVATE VulkanMemoryAllocator)
target_link_libraries(vrengine_core PRIVATE glm::glm)
target_link_libraries(vrengine_core PRIVATE tinygltf)

add_executable(vrengine $<TARGET_OBJECTS:vrengine_core>)
target_link_libraries(vrengine PRIVATE vrengine_core)

IF(CLANG_TIDY)
    set_target_properties(
        vrengine_core
        PROPERTIES
            CXX_CLANG_TIDY "${CLANG_TIDY}"
    )
ENDIF()
