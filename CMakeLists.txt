cmake_minimum_required(VERSION 3.19)
project(Vulkan_Engine)
set(CMAKE_CXX_STANDARD 20)

set(SOURCE_FILES src/main.cpp src/Camera.cpp src/EventHandler.cpp src/Window.cpp src/UI.cpp
src/vk/CommandPool.cpp src/vk/DescriptorSetHandler.cpp src/vk/ExtensionsHandler.cpp
src/vk/Image.cpp src/vk/Instance.cpp src/vk/LogicalDevice.cpp
src/vk/PhysicalDevice.cpp src/vk/Pipeline.cpp src/vk/RenderPass.cpp
src/vk/Shader.cpp src/vk/Swapchain.cpp src/vk/Synchronization.cpp
src/vk/RenderObject.cpp src/vk/Scene.cpp src/vk/Model.cpp src/vk/Mesh.cpp 
src/vk/VulkanCommandContext.cpp src/vk/VulkanMainContext.cpp src/vk/VulkanRenderContext.cpp src/vk/VulkanStorageContext.cpp
"${PROJECT_SOURCE_DIR}/dependencies/imgui-1.89.2/imgui.cpp" "${PROJECT_SOURCE_DIR}/dependencies/imgui-1.89.2/imgui_draw.cpp" "${PROJECT_SOURCE_DIR}/dependencies/imgui-1.89.2/imgui_widgets.cpp" "${PROJECT_SOURCE_DIR}/dependencies/imgui-1.89.2/imgui_tables.cpp" "${PROJECT_SOURCE_DIR}/dependencies/imgui-1.89.2/backends/imgui_impl_vulkan.cpp" "${PROJECT_SOURCE_DIR}/dependencies/imgui-1.89.2/backends/imgui_impl_sdl.cpp")

set(SHADER_FILES default.vert default.frag basic.frag)

add_executable(Vulkan_Engine ${SOURCE_FILES})
include_directories(Vulkan_Engine PUBLIC "${PROJECT_SOURCE_DIR}/include" "${PROJECT_SOURCE_DIR}/dependencies/VulkanMemoryAllocator-3.0.1/include" "${PROJECT_SOURCE_DIR}/dependencies/tinygltf-2.6.3/" "${PROJECT_SOURCE_DIR}/dependencies/imgui-1.89.2/")

find_package(glm REQUIRED)
find_package(SDL2 REQUIRED)
find_package(Vulkan REQUIRED)
find_package(spdlog REQUIRED)

target_link_libraries(Vulkan_Engine ${SDL2_LIBRARIES} ${Vulkan_LIBRARIES} spdlog::spdlog)

set(SHADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shader")

function(add_shader TARGET SHADER)
    find_program(GLSLC glslc)

    set(current-shader-path "${SHADER_DIR}/${SHADER}")
    set(current-output-path "${SHADER_DIR}/bin/${SHADER}.spv")

    add_custom_command(
           OUTPUT "${current-output-path}"
           COMMENT "Compiling \"${current-shader-path}\" to \"${current-output-path}\""
           DEPENDS "${current-shader-path}"
           IMPLICIT_DEPENDS CXX "${current-shader-path}"
           COMMAND ${GLSLC} -O -o "${current-output-path}" "${current-shader-path}"
           VERBATIM)

    target_sources(${TARGET} PRIVATE ${current-output-path})
endfunction(add_shader)

foreach(SHADER_FILE ${SHADER_FILES})
    add_shader(Vulkan_Engine "${SHADER_FILE}")
endforeach()
