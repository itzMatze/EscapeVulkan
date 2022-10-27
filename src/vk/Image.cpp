#include "vk/Image.hpp"

#include <stb/stb_image.h>

#include "ve_log.hpp"
#include "vk/VulkanCommandContext.hpp"

namespace ve
{
    Image::Image(const VulkanMainContext& vmc, const std::string& name) : vmc(vmc), name(name)
    {}

    Image::Image(const VulkanMainContext& vmc, const VulkanCommandContext& vcc, const std::vector<uint32_t>& queue_family_indices, const unsigned char* data, uint32_t width, uint32_t height) : vmc(vmc), w(width), h(height), byte_size(width * height * 4)
    {
        create_image_from_data(data, vcc, queue_family_indices);
    }

    Image::Image(const VulkanMainContext& vmc, const VulkanCommandContext& vcc, const std::vector<uint32_t>& queue_family_indices, const std::string& filename) : vmc(vmc), name(filename)
    {
        stbi_uc* pixels = stbi_load(filename.c_str(), &w, &h, &c, STBI_rgb_alpha);
        VE_ASSERT(pixels, "Failed to load image \"" << filename << "\"!\n");
        byte_size = w * h * 4;
        create_image_from_data(pixels, vcc, queue_family_indices);
        stbi_image_free(pixels);
        pixels = nullptr;
    }

    void Image::create_image_from_data(const unsigned char* data, const VulkanCommandContext& vcc, const std::vector<uint32_t>& queue_family_indices)
    {
        Buffer buffer(vmc, data, byte_size, vk::BufferUsageFlagBits::eTransferSrc, {uint32_t(vmc.queues_family_indices.transfer)});

        create_image(queue_family_indices, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::Format::eR8G8B8A8Srgb);

        transition_image_layout(vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eTransferDstOptimal, vcc);
        copy_buffer_to_image(vcc, buffer);
        transition_image_layout(vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eShaderReadOnlyOptimal, vcc);

        buffer.self_destruct();

        create_image_view(vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor);

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

    void Image::create_image(const std::vector<uint32_t>& queue_family_indices, vk::ImageUsageFlags usage, vk::Format format, uint32_t width, uint32_t height)
    {
        w = width;
        h = height;
        create_image(queue_family_indices, usage, format);
    }

    void Image::create_image(const std::vector<uint32_t>& queue_family_indices, vk::ImageUsageFlags usage, vk::Format format)
    {
        vk::ImageCreateInfo ici{};
        ici.sType = vk::StructureType::eImageCreateInfo;
        ici.imageType = vk::ImageType::e2D;
        ici.extent.width = static_cast<uint32_t>(w);
        ici.extent.height = static_cast<uint32_t>(h);
        ici.extent.depth = 1;
        ici.mipLevels = 1;
        ici.arrayLayers = 1;
        ici.format = format;
        ici.tiling = vk::ImageTiling::eOptimal;
        ici.initialLayout = vk::ImageLayout::eUndefined;
        layout = vk::ImageLayout::eUndefined;
        ici.usage = usage;
        ici.sharingMode = queue_family_indices.size() == 1 ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent;
        ici.queueFamilyIndexCount = queue_family_indices.size();
        ici.pQueueFamilyIndices = queue_family_indices.data();
        ici.samples = vk::SampleCountFlagBits::e1;
        ici.flags = {};

        VmaAllocationCreateInfo vaci{};
        vaci.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        VkBuffer local_buffer;
        VmaAllocation local_vmaa;
        vmaCreateImage(vmc.va, (VkImageCreateInfo*) (&ici), &vaci, (VkImage*) (&image), &vmaa, nullptr);
    }

    void Image::create_image_view(vk::Format format, vk::ImageAspectFlags aspects)
    {
        vk::ImageViewCreateInfo ivci{};
        ivci.sType = vk::StructureType::eImageViewCreateInfo;
        ivci.image = image;
        ivci.viewType = vk::ImageViewType::e2D;
        ivci.format = format;
        ivci.subresourceRange.aspectMask = aspects;
        ivci.subresourceRange.baseMipLevel = 0;
        ivci.subresourceRange.levelCount = 1;
        ivci.subresourceRange.baseArrayLayer = 0;
        ivci.subresourceRange.layerCount = 1;
        view = vmc.logical_device.get().createImageView(ivci);
    }

    void Image::self_destruct()
    {
        vmc.logical_device.get().destroySampler(sampler);
        vmc.logical_device.get().destroyImageView(view);
        vmaDestroyImage(vmc.va, VkImage(image), vmaa);
    }

    void Image::transition_image_layout(vk::Format format, vk::ImageLayout new_layout, const VulkanCommandContext& vcc)
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
            VE_THROW("Unsupported image layout transition!");
        }
        layout = new_layout;

        cb.pipelineBarrier(src_flags, dst_flags, {}, nullptr, nullptr, imb);
        vcc.submit_graphics(cb, true);
    }

    vk::DeviceSize Image::get_byte_size() const
    {
        return byte_size;
    }

    vk::ImageView Image::get_view() const
    {
        return view;
    }

    vk::Sampler Image::get_sampler() const
    {
        return sampler;
    }

    void Image::copy_buffer_to_image(const VulkanCommandContext& vcc, const Buffer& buffer)
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
}// namespace ve
