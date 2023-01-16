#include "vk/Image.hpp"

#include <stb/stb_image.h>

#include "ve_log.hpp"
#include "vk/VulkanCommandContext.hpp"

namespace ve
{
    Image::Image(const VulkanMainContext& vmc, const std::string& name, bool use_mip_maps) : vmc(vmc), name(name), mip_levels(use_mip_maps ? 2 : 1)
    {}

    Image::Image(const VulkanMainContext& vmc, const VulkanCommandContext& vcc, const std::vector<uint32_t>& queue_family_indices, const unsigned char* data, uint32_t width, uint32_t height, bool use_mip_maps) : vmc(vmc), w(width), h(height), byte_size(width * height * 4), mip_levels(use_mip_maps ? 2 : 1)
    {
        create_image_from_data(data, vcc, queue_family_indices);
    }

    Image::Image(const VulkanMainContext& vmc, const VulkanCommandContext& vcc, const std::vector<uint32_t>& queue_family_indices, Image& src_image, uint32_t base_mip_level) : vmc(vmc), mip_levels(2)
    {
        w = std::max(1.0, src_image.w / std::pow(2, base_mip_level));
        h = std::max(1.0, src_image.h / std::pow(2, base_mip_level));
        byte_size = w * h * 4;
        constexpr vk::Format format = vk::Format::eR8G8B8A8Srgb;
        create_image(queue_family_indices, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, format, vk::SampleCountFlagBits::e1);

        src_image.transition_image_layout(vcc, vk::ImageLayout::eTransferSrcOptimal, vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eTransferRead);
        transition_image_layout(vcc, vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {}, vk::AccessFlagBits::eTransferWrite);
        vk::FormatProperties format_properties = vmc.physical_device.get().getFormatProperties(format);
        if (!(format_properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) mip_levels = 1;

        const vk::CommandBuffer& cb = vcc.begin(vcc.graphics_cb[0]);

        vk::ImageBlit blit{};
        blit.srcOffsets[0] = vk::Offset3D(0, 0, 0);
        blit.srcOffsets[1] = vk::Offset3D(w, h, 1);
        blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        blit.srcSubresource.mipLevel = base_mip_level;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = vk::Offset3D(0, 0, 0);
        blit.dstOffsets[1] = vk::Offset3D(w, h, 1);
        blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        blit.dstSubresource.mipLevel = 0;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;
        cb.blitImage(src_image.get_image(), vk::ImageLayout::eTransferSrcOptimal, image, vk::ImageLayout::eTransferDstOptimal, blit, vk::Filter::eLinear);

        vcc.submit_graphics(cb, true);

        mip_levels > 1 ? generate_mipmaps(vcc) : transition_image_layout(vcc, vk::ImageLayout::eShaderReadOnlyOptimal, vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead);
        create_image_view(format, vk::ImageAspectFlagBits::eColor);
        create_sampler();
    }

    Image::Image(const VulkanMainContext& vmc, const VulkanCommandContext& vcc, const std::vector<uint32_t>& queue_family_indices, const std::string& filename, bool use_mip_maps) : vmc(vmc), name(filename), mip_levels(use_mip_maps ? 2 : 1)
    {
        stbi_uc* pixels = stbi_load(filename.c_str(), &w, &h, &c, STBI_rgb_alpha);
        VE_ASSERT(pixels, "Failed to load image \"{}\"!", filename);
        byte_size = w * h * 4;
        create_image_from_data(pixels, vcc, queue_family_indices);
        stbi_image_free(pixels);
        pixels = nullptr;
    }

    void Image::create_image_from_data(const unsigned char* data, const VulkanCommandContext& vcc, const std::vector<uint32_t>& queue_family_indices)
    {
        Buffer buffer(vmc, data, byte_size, vk::BufferUsageFlagBits::eTransferSrc, {uint32_t(vmc.queues_family_indices.transfer)});

        constexpr vk::Format format = vk::Format::eR8G8B8A8Srgb;
        create_image(queue_family_indices, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, format, vk::SampleCountFlagBits::e1);

        transition_image_layout(vcc, vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {}, vk::AccessFlagBits::eTransferWrite);
        copy_buffer_to_image(vcc, buffer);

        vk::FormatProperties format_properties = vmc.physical_device.get().getFormatProperties(format);
        if (!(format_properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) mip_levels = 1;

        mip_levels > 1 ? generate_mipmaps(vcc) : transition_image_layout(vcc, vk::ImageLayout::eShaderReadOnlyOptimal, vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead);

        buffer.self_destruct();

        create_image_view(format, vk::ImageAspectFlagBits::eColor);
        create_sampler();
    }

    void Image::create_image(const std::vector<uint32_t>& queue_family_indices, vk::ImageUsageFlags usage, vk::Format format, uint32_t width, uint32_t height, vk::SampleCountFlagBits sample_count)
    {
        w = width;
        h = height;
        create_image(queue_family_indices, usage, format, sample_count);
    }

    void Image::create_image(const std::vector<uint32_t>& queue_family_indices, vk::ImageUsageFlags usage, vk::Format format, vk::SampleCountFlagBits sample_count)
    {
        mip_levels = mip_levels > 1 ? std::floor(std::log2(std::max(w, h))) + 1 : 1;
        if (mip_levels > 1) usage |= vk::ImageUsageFlagBits::eTransferSrc;
        vk::ImageCreateInfo ici{};
        ici.sType = vk::StructureType::eImageCreateInfo;
        ici.imageType = vk::ImageType::e2D;
        ici.extent.width = static_cast<uint32_t>(w);
        ici.extent.height = static_cast<uint32_t>(h);
        ici.extent.depth = 1;
        ici.mipLevels = mip_levels;
        ici.arrayLayers = 1;
        ici.format = format;
        ici.tiling = vk::ImageTiling::eOptimal;
        ici.initialLayout = vk::ImageLayout::eUndefined;
        layout = vk::ImageLayout::eUndefined;
        ici.usage = usage;
        ici.sharingMode = queue_family_indices.size() == 1 ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent;
        ici.queueFamilyIndexCount = queue_family_indices.size();
        ici.pQueueFamilyIndices = queue_family_indices.data();
        ici.samples = sample_count;
        ici.flags = {};

        VmaAllocationCreateInfo vaci{};
        vaci.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
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
        ivci.subresourceRange.levelCount = mip_levels;
        ivci.subresourceRange.baseArrayLayer = 0;
        ivci.subresourceRange.layerCount = 1;
        view = vmc.logical_device.get().createImageView(ivci);
    }

    void Image::create_sampler()
    {
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
        sci.maxLod = mip_levels;
        sampler = vmc.logical_device.get().createSampler(sci);
    }

    void Image::self_destruct()
    {
        vmc.logical_device.get().destroySampler(sampler);
        vmc.logical_device.get().destroyImageView(view);
        vmaDestroyImage(vmc.va, VkImage(image), vmaa);
    }

    void Image::transition_image_layout(const VulkanCommandContext& vcc, vk::ImageLayout new_layout, vk::PipelineStageFlags src_stage_flags, vk::PipelineStageFlags dst_stage_flags, vk::AccessFlags src_access_flags, vk::AccessFlags dst_access_flags)
    {
        const vk::CommandBuffer& cb = vcc.begin(vcc.graphics_cb[0]);

        vk::ImageMemoryBarrier imb{};
        imb.sType = vk::StructureType::eImageMemoryBarrier;
        imb.oldLayout = layout;
        imb.newLayout = new_layout;
        imb.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imb.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imb.image = image;
        imb.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        imb.subresourceRange.baseMipLevel = 0;
        imb.subresourceRange.levelCount = mip_levels;
        imb.subresourceRange.baseArrayLayer = 0;
        imb.subresourceRange.layerCount = 1;
        imb.srcAccessMask = src_access_flags;
        imb.dstAccessMask = dst_access_flags;
        src_stage_flags = src_stage_flags;
        dst_stage_flags = dst_stage_flags;
        layout = new_layout;

        cb.pipelineBarrier(src_stage_flags, dst_stage_flags, {}, nullptr, nullptr, imb);
        vcc.submit_graphics(cb, true);
    }

    vk::DeviceSize Image::get_byte_size() const
    {
        return byte_size;
    }

    vk::Image Image::get_image() const
    {
        return image;
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

    void Image::generate_mipmaps(const VulkanCommandContext& vcc)
    {
        const vk::CommandBuffer& cb = vcc.begin(vcc.graphics_cb[0]);

        vk::ImageMemoryBarrier imb{};
        imb.sType = vk::StructureType::eImageMemoryBarrier;
        imb.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imb.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imb.image = image;
        imb.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        imb.subresourceRange.levelCount = 1;
        imb.subresourceRange.baseArrayLayer = 0;
        imb.subresourceRange.layerCount = 1;

        uint32_t mip_w = w;
        uint32_t mip_h = h;
        for (uint32_t i = 1; i < mip_levels; ++i)
        {
            imb.subresourceRange.baseMipLevel = i - 1;
            imb.oldLayout = vk::ImageLayout::eTransferDstOptimal;
            imb.newLayout = vk::ImageLayout::eTransferSrcOptimal;
            imb.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            imb.dstAccessMask = vk::AccessFlagBits::eTransferRead;
            cb.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {}, nullptr, nullptr, imb);

            vk::ImageBlit blit{};
            blit.srcOffsets[0] = vk::Offset3D(0, 0, 0);
            blit.srcOffsets[1] = vk::Offset3D(mip_w, mip_h, 1);
            blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = vk::Offset3D(0, 0, 0);
            blit.dstOffsets[1] = vk::Offset3D(mip_w > 1 ? mip_w / 2 : 1, mip_h > 1 ? mip_h / 2 : 1, 1);
            blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;
            cb.blitImage(image, vk::ImageLayout::eTransferSrcOptimal, image, vk::ImageLayout::eTransferDstOptimal, blit, vk::Filter::eLinear);

            imb.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
            imb.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            imb.srcAccessMask = vk::AccessFlagBits::eTransferRead;
            imb.dstAccessMask = vk::AccessFlagBits::eShaderRead;
            cb.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, nullptr, nullptr, imb);

            if (mip_w > 1) mip_w /= 2;
            if (mip_h > 1) mip_h /= 2;
        }

        imb.subresourceRange.baseMipLevel = mip_levels - 1;
        imb.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        imb.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        layout = imb.newLayout;
        imb.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        imb.dstAccessMask = vk::AccessFlagBits::eShaderRead;
        cb.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, nullptr, nullptr, imb);

        vcc.submit_graphics(cb, true);
    }
}// namespace ve
