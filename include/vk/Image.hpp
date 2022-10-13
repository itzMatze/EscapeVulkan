#pragma once

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "vk/VulkanCommandContext.hpp"

namespace ve
{
    class Image
    {
    public:
        Image(const VulkanMainContext& vmc, const VulkanCommandContext& vcc, const std::vector<uint32_t>& queue_family_indices, const std::string& filename) : vmc(vmc), name(filename)
        {
            stbi_uc* pixels = stbi_load(std::string("../assets/textures/" + filename).c_str(), &w, &h, &c, STBI_rgb_alpha);
            VE_ASSERT(pixels, "Failed to load image \"" << filename << "\"!\n");
            byte_size = w * h * 4;
            Buffer buffer(vmc, pixels, byte_size, vk::BufferUsageFlagBits::eTransferSrc, {uint32_t(vmc.queues_family_indices.transfer)});
            stbi_image_free(pixels);
            pixels = nullptr;

            vk::ImageCreateInfo ici{};
            ici.sType = vk::StructureType::eImageCreateInfo;
            ici.imageType = vk::ImageType::e2D;
            ici.extent.width = static_cast<uint32_t>(w);
            ici.extent.height = static_cast<uint32_t>(h);
            ici.extent.depth = 1;
            ici.mipLevels = 1;
            ici.arrayLayers = 1;
            ici.format = vk::Format::eR8G8B8A8Srgb;
            ici.tiling = vk::ImageTiling::eOptimal;
            ici.initialLayout = vk::ImageLayout::eUndefined;
            layout = vk::ImageLayout::eUndefined;
            ici.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
            ici.sharingMode = queue_family_indices.size() == 1 ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent;
            ici.queueFamilyIndexCount = queue_family_indices.size();
            ici.pQueueFamilyIndices = queue_family_indices.data();
            ici.samples = vk::SampleCountFlagBits::e1;
            ici.flags = {};
            image = vmc.logical_device.get().createImage(ici);

            vk::MemoryRequirements mem_reqs = vmc.logical_device.get().getImageMemoryRequirements(image);
            vk::MemoryAllocateInfo mai{};
            mai.sType = vk::StructureType::eMemoryAllocateInfo;
            mai.allocationSize = mem_reqs.size;
            mai.memoryTypeIndex = Buffer::find_memory_type(mem_reqs.memoryTypeBits, {vk::MemoryPropertyFlagBits::eDeviceLocal}, vmc.physical_device.get());
            memory = vmc.logical_device.get().allocateMemory(mai);
            vmc.logical_device.get().bindImageMemory(image, memory, 0);

            transition_image_layout(vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eTransferDstOptimal, vcc);
            copy_buffer_to_image(vcc, buffer);
            transition_image_layout(vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eShaderReadOnlyOptimal, vcc);

            buffer.self_destruct();

            vk::ImageViewCreateInfo ivci{};
            ivci.sType = vk::StructureType::eImageViewCreateInfo;
            ivci.image = image;
            ivci.viewType = vk::ImageViewType::e2D;
            ivci.format = vk::Format::eR8G8B8A8Srgb;
            ivci.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
            ivci.subresourceRange.baseMipLevel = 0;
            ivci.subresourceRange.levelCount = 1;
            ivci.subresourceRange.baseArrayLayer = 0;
            ivci.subresourceRange.layerCount = 1;
            view = vmc.logical_device.get().createImageView(ivci);

            vk::SamplerCreateInfo sci{};
            sci.sType = vk::StructureType::eSamplerCreateInfo;
            sci.magFilter = vk::Filter::eLinear;
            sci.minFilter = vk::Filter::eLinear;
            sci.addressModeU = vk::SamplerAddressMode::eRepeat;
            sci.addressModeV = vk::SamplerAddressMode::eRepeat;
            sci.addressModeW = vk::SamplerAddressMode::eRepeat;
            sci.anisotropyEnable = VK_TRUE;
            sci.maxAnisotropy = std::min(8.0f, vmc.physical_device.get().getProperties().limits.maxSamplerAnisotropy);
            sci.borderColor = vk::BorderColor::eIntOpaqueBlack;
            sci.unnormalizedCoordinates = VK_FALSE;
            sci.compareEnable = VK_FALSE;
            sci.compareOp = vk::CompareOp::eAlways;
            sci.mipmapMode = vk::SamplerMipmapMode::eLinear;
            sci.mipLodBias = 0.0f;
            sci.minLod = 0.0f;
            sci.maxLod = 0.0f;
            sampler = vmc.logical_device.get().createSampler(sci);
        }

        void self_destruct()
        {
            vmc.logical_device.get().destroySampler(sampler);
            vmc.logical_device.get().destroyImageView(view);
            vmc.logical_device.get().destroyImage(image);
            vmc.logical_device.get().freeMemory(memory);
        }

        void transition_image_layout(vk::Format format, vk::ImageLayout new_layout, const VulkanCommandContext& vcc)
        {
            const vk::CommandBuffer& cb = vcc.begin(vcc.graphics_cb[0]);

            vk::PipelineStageFlags src_flags, dst_flags;
            vk::ImageMemoryBarrier imb{};
            imb.sType = vk::StructureType::eImageMemoryBarrier;
            imb.oldLayout = layout;
            imb.newLayout = new_layout;
            imb.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imb.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imb.image = image;
            imb.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
            imb.subresourceRange.baseMipLevel = 0;
            imb.subresourceRange.levelCount = 1;
            imb.subresourceRange.baseArrayLayer = 0;
            imb.subresourceRange.layerCount = 1;
            if (layout == vk::ImageLayout::eUndefined && new_layout == vk::ImageLayout::eTransferDstOptimal)
            {
                imb.srcAccessMask = {};
                imb.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
                src_flags = vk::PipelineStageFlagBits::eTopOfPipe;
                dst_flags = vk::PipelineStageFlagBits::eTransfer;
            }
            else if (layout == vk::ImageLayout::eTransferDstOptimal && new_layout == vk::ImageLayout::eShaderReadOnlyOptimal)
            {
                imb.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
                imb.dstAccessMask = vk::AccessFlagBits::eShaderRead;
                src_flags = vk::PipelineStageFlagBits::eTransfer;
                dst_flags = vk::PipelineStageFlagBits::eFragmentShader;
            }
            else
            {
                VE_THROW("Unsupported image layout transition!\n");
            }
            layout = new_layout;

            cb.pipelineBarrier(src_flags, dst_flags, {}, nullptr, nullptr, imb);
            vcc.submit_graphics(cb, true);
        }

        vk::DeviceSize get_byte_size() const
        {
            return byte_size;
        }

        vk::ImageView get_view() const
        {
            return view;
        }

        vk::Sampler get_sampler() const
        {
            return sampler;
        }

    private:
        const VulkanMainContext& vmc;
        const std::string name;
        int w, h, c;
        vk::DeviceSize byte_size;
        vk::ImageLayout layout;
        vk::Image image;
        vk::DeviceMemory memory;
        vk::ImageView view;
        vk::Sampler sampler;

        void copy_buffer_to_image(const VulkanCommandContext& vcc, const Buffer& buffer)
        {
            const vk::CommandBuffer& cb = vcc.begin(vcc.transfer_cb[0]);
            vk::BufferImageCopy copy_region{};
            copy_region.bufferOffset = 0;
            copy_region.bufferRowLength = 0;
            copy_region.bufferImageHeight = 0;
            copy_region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
            copy_region.imageSubresource.mipLevel = 0;
            copy_region.imageSubresource.baseArrayLayer = 0;
            copy_region.imageSubresource.layerCount = 1;
            copy_region.imageOffset = vk::Offset3D{0, 0, 0};
            copy_region.imageExtent = vk::Extent3D{uint32_t(w), uint32_t(h), 1};

            cb.copyBufferToImage(buffer.get(), image, layout, copy_region);
            vcc.submit_transfer(cb, true);
        }
    };
}// namespace ve
