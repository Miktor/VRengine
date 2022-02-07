cmake_minimum_required(VERSION 3.7 FATAL_ERROR)

set(CMAKE_ENABLE_EXPORTS on)

project(vrengine VERSION 0.1.0)

enable_language(CXX)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

add_compile_options(-std=c++17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_EXTENSIONS off)

IF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    add_definitions(-DWIN32_LEAN_AND_MEAN)
ELSEIF(APPLE)
    add_compile_options ( "-Wno-nullability-extension" "-Wno-missing-field-initializers" "-Wno-nullability-completeness")  ## "-Wall" "-Wextra" "-Wpedantic" "-Werror"
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_MACOS_MVK")
    add_compile_options(-Wno-macro-redefined)
    set(CMAKE_EXE_LINKER_FLAGS "-Wl,-export_dynamic")
ENDIF()

IF(MSVC)
    add_definitions(-DNOMINMAX)
ENDIF()

add_definitions(-DVK_DEBUG)

add_subdirectory(external)

find_package(Vulkan REQUIRED)
find_package(GLM REQUIRED)
find_package(spdlog REQUIRED)

file(GLOB_RECURSE SOURCES
     "src/*.h"
     "src/*.cpp"
)
AUX_SOURCE_DIRECTORY(src SOURCES)

add_library(vrengine_core OBJECT ${SOURCES})

IF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    target_compile_definitions(vrengine_core PUBLIC VK_USE_PLATFORM_WIN32_KHR)
    set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)
ELSE()
    target_compile_definitions(vrengine_core PUBLIC VK_USE_PLATFORM_MACOS_MVK)
    set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_MACOS_MVK)
ENDIF()

target_include_directories(vrengine_core PUBLIC
    src
    Vulkan::Vulkan
    external
    external/glfw/include
    ${GLM_INCLUDE_DIR})

target_link_libraries(vrengine_core PUBLIC Vulkan::Vulkan)
target_link_libraries(vrengine_core PUBLIC spdlog::spdlog)
target_link_libraries(vrengine_core PUBLIC glfw)
target_link_libraries(vrengine_core PUBLIC shaderc)
target_link_libraries(vrengine_core PUBLIC spirv-cross-core)
target_link_libraries(vrengine_core PUBLIC VulkanMemoryAllocator)

add_executable(vrengine $<TARGET_OBJECTS:vrengine_core>)
target_link_libraries(vrengine PUBLIC vrengine_core)

IF(CLANG_TIDY)
    set_target_properties(
        vrengine_core
        PROPERTIES
            CXX_CLANG_TIDY "${CLANG_TIDY}"
    )
ENDIF()