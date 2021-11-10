cmake_minimum_required(VERSION 3.7 FATAL_ERROR)
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
    add_compile_options ( "-Wno-nullability-extension" "-Wno-missing-field-initializers")  ## "-Wall" "-Wextra" "-Wpedantic" "-Werror"
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_MACOS_MVK")
ENDIF()


IF(MSVC)
    add_definitions(-DNOMINMAX)
ENDIF()

add_subdirectory(external)

find_package(Vulkan REQUIRED)
find_package(GLM REQUIRED)
find_package(spdlog REQUIRED)

include_directories(${GLM_INCLUDE_DIR})

file(GLOB_RECURSE SOURCES
     "src/*.h"
     "src/*.cpp"
)
AUX_SOURCE_DIRECTORY(src SOURCES)
add_executable(vrengine ${SOURCES})

IF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    target_compile_definitions(vrengine PRIVATE VK_USE_PLATFORM_WIN32_KHR)
    set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)
ELSE()
    target_compile_definitions(vrengine PRIVATE VK_USE_PLATFORM_MACOS_MVK)
    set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_MACOS_MVK)
ENDIF()

target_include_directories(vrengine PRIVATE
    src
    Vulkan::Vulkan
    external
    external/glfw/include)

target_link_libraries(vrengine PRIVATE Vulkan::Vulkan)
target_link_libraries(vrengine PRIVATE spdlog::spdlog)
target_link_libraries(vrengine PRIVATE glfw)
target_link_libraries(vrengine PRIVATE shaderc)

file(GLOB_RECURSE ALL_SOURCE_FILES *.cpp *.h)

add_custom_target(
        clangformat
        COMMAND /usr/local/opt/llvm@11/bin/clang-format 
        -i
        ${ALL_SOURCE_FILES}
)
