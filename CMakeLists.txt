cmake_minimum_required(VERSION 3.19)
project(Vulkan_Engine)
set(CMAKE_CXX_STANDARD 20)

set(SOURCE_FILES src/main.cpp)

add_executable(Vulkan_Engine ${SOURCE_FILES})
include_directories(Vulkan_Engine PUBLIC "${PROJECT_SOURCE_DIR}/include")

find_package(glm REQUIRED)
find_package(SDL2 REQUIRED)
find_package(Vulkan REQUIRED)

target_link_libraries(Vulkan_Engine ${SDL2_LIBRARIES} ${Vulkan_LIBRARIES})
