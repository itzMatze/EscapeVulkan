cmake_minimum_required(VERSION 3.19)
project(EscapeVulkan)
set(CMAKE_CXX_STANDARD 20)

set(SOURCE_FILES src/main.cpp src/Camera.cpp src/EventHandler.cpp src/Window.cpp src/UI.cpp
src/vk/CommandPool.cpp src/vk/DescriptorSetHandler.cpp src/vk/ExtensionsHandler.cpp
src/vk/Instance.cpp src/vk/LogicalDevice.cpp src/vk/PhysicalDevice.cpp
src/vk/Pipeline.cpp src/vk/RenderPass.cpp src/vk/Swapchain.cpp
src/vk/Shader.cpp src/vk/Synchronization.cpp src/vk/Image.cpp
src/vk/RenderObject.cpp src/vk/TunnelObjects.cpp src/vk/Tunnel.cpp src/vk/Fireflies.cpp src/vk/CollisionHandler.cpp
src/vk/Scene.cpp src/vk/Model.cpp src/vk/Mesh.cpp src/vk/Timer.cpp
src/vk/VulkanCommandContext.cpp src/vk/VulkanMainContext.cpp src/WorkContext.cpp src/Storage.cpp
"${PROJECT_SOURCE_DIR}/dependencies/imgui-1.89.2/imgui.cpp" "${PROJECT_SOURCE_DIR}/dependencies/imgui-1.89.2/imgui_draw.cpp" "${PROJECT_SOURCE_DIR}/dependencies/imgui-1.89.2/imgui_widgets.cpp" "${PROJECT_SOURCE_DIR}/dependencies/imgui-1.89.2/imgui_tables.cpp" "${PROJECT_SOURCE_DIR}/dependencies/imgui-1.89.2/backends/imgui_impl_vulkan.cpp" "${PROJECT_SOURCE_DIR}/dependencies/imgui-1.89.2/backends/imgui_impl_sdl.cpp" "${PROJECT_SOURCE_DIR}/dependencies/implot-0.14/implot.cpp" "${PROJECT_SOURCE_DIR}/dependencies/implot-0.14/implot_items.cpp")

set(SHADER_FILES debug.vert debug.frag default.vert default.frag basic.frag emissive.frag
tunnel_skybox.vert tunnel_skybox.frag tunnel.vert tunnel.frag tunnel.comp tunnel_normals.comp
fireflies.vert fireflies.frag fireflies_move.comp fireflies_tunnel_collision.comp
create_noise_textures.comp player_tunnel_collision.comp)
set(SHADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shader")

add_executable(EscapeVulkan ${SOURCE_FILES})
add_custom_target(Shaders)
add_dependencies(EscapeVulkan Shaders)
include_directories(EscapeVulkan PUBLIC "${PROJECT_SOURCE_DIR}/include" "${PROJECT_SOURCE_DIR}/dependencies/VulkanMemoryAllocator-3.0.1/include" "${PROJECT_SOURCE_DIR}/dependencies/tinygltf-2.6.3/" "${PROJECT_SOURCE_DIR}/dependencies/imgui-1.89.2/" "${PROJECT_SOURCE_DIR}/dependencies/implot-0.14/")

find_package(glm REQUIRED)
find_package(SDL2 REQUIRED)
find_package(Vulkan REQUIRED)
find_package(spdlog REQUIRED)
find_package(Boost REQUIRED)

target_link_libraries(EscapeVulkan ${SDL2_LIBRARIES} ${Vulkan_LIBRARIES} spdlog::spdlog)


function(add_shader TARGET SHADER)
    find_program(GLSLC glslc)

    set(current-shader-path "${SHADER_DIR}/${SHADER}")
    set(current-output-path "${SHADER_DIR}/bin/${SHADER}.spv")

    add_custom_command(
           OUTPUT "${current-output-path}"
           COMMENT "Compiling \"${current-shader-path}\" to \"${current-output-path}\""
           DEPENDS "${current-shader-path}"
           COMMAND ${GLSLC} -O -o "${current-output-path}" "${current-shader-path}"
           VERBATIM)

    target_sources(${TARGET} PRIVATE ${current-output-path})
endfunction(add_shader)

foreach(SHADER_FILE ${SHADER_FILES})
    add_shader(Shaders "${SHADER_FILE}")
endforeach()
