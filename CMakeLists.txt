cmake_minimum_required(VERSION 3.8)

project(NBody VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE "Debug")

set(GLFW_BUILD_DOCS OFF)
set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
add_subdirectory(dependencies/glfw EXCLUDE_FROM_ALL)

find_package(Vulkan REQUIRED)

include(cmake/glslc.cmake)

add_executable(vkexample 
    src/main.cpp 
    src/app.cpp
    
    src/util/BoundedBuffer.cpp
    src/util/BoundImage.cpp
    src/util/callbacks.cpp
    src/util/general.cpp
    src/util/MVPTransform.cpp
    src/util/query.cpp
    src/util/QueueFamilyIndices.cpp
)
target_link_libraries(vkexample glfw Vulkan::Vulkan)
target_include_directories(vkexample 
    PRIVATE ${GLFW_INCLUDE_DIRS} 
    PRIVATE Vulkan::Vulkan
    PRIVATE dependencies/stb)

add_shader(vkexample src/simple.frag frag.spv)
add_shader(vkexample src/simple.vert vert.spv)
