#include "vk/Lighting.hpp"
#include "Storage.hpp"
#include "vk/Swapchain.hpp"
#include "vk/TunnelConstants.hpp"
#include "vk/VulkanMainContext.hpp"

namespace ve
{
Lighting::Lighting(const VulkanMainContext& vmc, Storage& storage) : vmc(vmc), storage(storage), lighting_pipeline_0(vmc), lighting_pipeline_1(vmc), lighting_dsh(vmc)
{
}

void Lighting::construct(uint32_t light_count, const Swapchain& swapchain)
{
    create_lighting_pipeline(light_count, swapchain);
}

void Lighting::self_destruct()
{
    lighting_pipeline_0.self_destruct();
    lighting_pipeline_1.self_destruct();
    lighting_dsh.self_destruct();
    for (uint32_t i : restir_reservoir_buffers) storage.destroy_buffer(i);
    restir_reservoir_buffers.clear();
}

void Lighting::pre_pass(vk::CommandBuffer& cb, GameState& gs)
{
    cb.bindPipeline(vk::PipelineBindPoint::eGraphics, lighting_pipeline_0.get());
    cb.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, lighting_pipeline_0.get_layout(), 0, lighting_dsh.get_sets()[gs.game_data.current_frame * frames_in_flight], {});
    if (!gs.settings.disable_rendering)
    {
        cb.draw(3, 1, 0, 0);
    }
}

void Lighting::main_pass(vk::CommandBuffer& cb, GameState& gs)
{
    cb.bindPipeline(vk::PipelineBindPoint::eGraphics, lighting_pipeline_1.get());
    cb.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, lighting_pipeline_1.get_layout(), 0, lighting_dsh.get_sets()[gs.game_data.current_frame * frames_in_flight + 1], {});
    if (!gs.settings.disable_rendering)
    {
        cb.draw(3, 1, 0, 0);
    }
}

void Lighting::create_lighting_pipeline(uint32_t light_count, const Swapchain& swapchain)
{
    create_lighting_descriptor_sets(swapchain.get_extent());
    std::vector<ShaderInfo> shader_infos(2);
    std::array<vk::SpecializationMapEntry, 7> fragment_entries;
    fragment_entries[0] = vk::SpecializationMapEntry(0, 0, sizeof(uint32_t));
    fragment_entries[1] = vk::SpecializationMapEntry(1, sizeof(uint32_t), sizeof(uint32_t));
    fragment_entries[2] = vk::SpecializationMapEntry(2, sizeof(uint32_t) * 2, sizeof(uint32_t));
    fragment_entries[3] = vk::SpecializationMapEntry(3, sizeof(uint32_t) * 3, sizeof(uint32_t));
    fragment_entries[4] = vk::SpecializationMapEntry(4, sizeof(uint32_t) * 4, sizeof(uint32_t));
    fragment_entries[5] = vk::SpecializationMapEntry(5, sizeof(uint32_t) * 5, sizeof(uint32_t));
    fragment_entries[6] = vk::SpecializationMapEntry(6, sizeof(uint32_t) * 6, sizeof(uint32_t));
    std::array<uint32_t, 7> fragment_entries_data{light_count, segment_count, fireflies_per_segment, reservoir_count, swapchain.get_extent().width, swapchain.get_extent().height, 1};
    vk::SpecializationInfo fragment_spec_info(fragment_entries.size(), fragment_entries.data(), sizeof(uint32_t) * fragment_entries_data.size(), fragment_entries_data.data());

    shader_infos[0] = ShaderInfo{"lighting.vert", vk::ShaderStageFlagBits::eVertex};
    shader_infos[1] = ShaderInfo{"lighting.frag", vk::ShaderStageFlagBits::eFragment, fragment_spec_info};
    lighting_pipeline_0.construct(swapchain.get_render_pass(), lighting_dsh.get_layouts()[0], shader_infos, vk::PolygonMode::eFill, std::vector<vk::VertexInputBindingDescription>(), std::vector<vk::VertexInputAttributeDescription>(), vk::PrimitiveTopology::eTriangleList, {});

    fragment_entries_data[6] = 0;
    shader_infos[1] = ShaderInfo{"lighting.frag", vk::ShaderStageFlagBits::eFragment, fragment_spec_info};
    lighting_pipeline_1.construct(swapchain.get_render_pass(), lighting_dsh.get_layouts()[0], shader_infos, vk::PolygonMode::eFill, std::vector<vk::VertexInputBindingDescription>(), std::vector<vk::VertexInputAttributeDescription>(), vk::PrimitiveTopology::eTriangleList, {});
}

void Lighting::create_lighting_descriptor_sets(vk::Extent2D swapchain_extent)
{
    std::vector<Reservoir> reservoirs(swapchain_extent.width * swapchain_extent.height * reservoir_count);
    for(uint32_t i = 0; i < frames_in_flight; ++i)
    {
        restir_reservoir_buffers.push_back(storage.add_buffer(reservoirs, vk::BufferUsageFlagBits::eStorageBuffer, true, vmc.queue_family_indices.graphics));
    }
    lighting_dsh.add_binding(1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
    lighting_dsh.add_binding(2, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment);
    lighting_dsh.add_binding(3, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment);
    lighting_dsh.add_binding(4, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eFragment);
    lighting_dsh.add_binding(5, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment);
    lighting_dsh.add_binding(6, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment);
    lighting_dsh.add_binding(10, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment);
    lighting_dsh.add_binding(11, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment);
    lighting_dsh.add_binding(12, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment);
    lighting_dsh.add_binding(13, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment);
    lighting_dsh.add_binding(90, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eFragment);
    lighting_dsh.add_binding(99, vk::DescriptorType::eAccelerationStructureKHR, vk::ShaderStageFlagBits::eFragment);
    lighting_dsh.add_binding(100, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment);
    lighting_dsh.add_binding(101, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment);
    lighting_dsh.add_binding(102, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment);
    lighting_dsh.add_binding(103, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment);
    lighting_dsh.add_binding(104, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment);
    lighting_dsh.add_binding(200, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment);
    lighting_dsh.add_binding(201, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment);

    for (uint32_t j = 0; j < frames_in_flight; ++j)
    {
        for (uint32_t i = 0; i < frames_in_flight; ++i)
        {
            lighting_dsh.new_set();
            lighting_dsh.add_descriptor(1, storage.get_buffer_by_name("mesh_render_data"));
            lighting_dsh.add_descriptor(2, storage.get_image_by_name("textures"));
            lighting_dsh.add_descriptor(3, storage.get_buffer_by_name("materials"));
            lighting_dsh.add_descriptor(4, storage.get_buffer_by_name("spaceship_lights_" + std::to_string(j)));
            lighting_dsh.add_descriptor(5, storage.get_buffer_by_name("firefly_vertices_" + std::to_string(j)));
            lighting_dsh.add_descriptor(6, storage.get_image_by_name("noise_textures"));
            lighting_dsh.add_descriptor(10, storage.get_buffer_by_name("tunnel_indices"));
            lighting_dsh.add_descriptor(11, storage.get_buffer_by_name("tunnel_vertices"));
            lighting_dsh.add_descriptor(12, storage.get_buffer_by_name("indices"));
            lighting_dsh.add_descriptor(13, storage.get_buffer_by_name("vertices"));
            lighting_dsh.add_descriptor(90, storage.get_buffer_by_name("frame_data_" + std::to_string(j)));
            lighting_dsh.add_descriptor(99, storage.get_buffer_by_name("tlas_" + std::to_string(j)));
            lighting_dsh.add_descriptor(100, storage.get_image_by_name("deferred_position"));
            lighting_dsh.add_descriptor(101, storage.get_image_by_name("deferred_normal"));
            lighting_dsh.add_descriptor(102, storage.get_image_by_name("deferred_color"));
            lighting_dsh.add_descriptor(103, storage.get_image_by_name("deferred_segment_uid"));
            lighting_dsh.add_descriptor(104, storage.get_image_by_name("deferred_motion"));
            lighting_dsh.add_descriptor(200, storage.get_buffer(restir_reservoir_buffers[1 - i]));
            lighting_dsh.add_descriptor(201, storage.get_buffer(restir_reservoir_buffers[i]));
        }
    }
    lighting_dsh.construct();
}
} // namespace ve

