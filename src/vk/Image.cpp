#include "vk/Image.hpp"

#include "ve_log.hpp"
#include "vk/VulkanCommandContext.hpp"

namespace ve
{
    void blit_image(const vk::CommandBuffer& cb, vk::Image& src, uint32_t src_mip_map_lvl, vk::Offset3D src_offset, vk::Image& dst, uint32_t dst_mip_map_lvl, vk::Offset3D dst_offset, uint32_t layer_count)
    {
        vk::ImageBlit blit{};
        blit.srcOffsets[0] = vk::Offset3D(0, 0, 0);
        blit.srcOffsets[1] = src_offset;
        blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        blit.srcSubresource.mipLevel = src_mip_map_lvl;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = layer_count;
        blit.dstOffsets[0] = vk::Offset3D(0, 0, 0);
        blit.dstOffsets[1] = dst_offset;
        blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        blit.dstSubresource.mipLevel = dst_mip_map_lvl;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = layer_count;
        cb.blitImage(src, vk::ImageLayout::eTransferSrcOptimal, dst, vk::ImageLayout::eTransferDstOptimal, blit, vk::Filter::eLinear);
    }

    std::pair<vk::Image, VmaAllocation> Image::create_image(const std::vector<uint32_t>& queue_family_indices, vk::ImageUsageFlags usage, vk::SampleCountFlagBits sample_count, bool use_mip_levels, vk::Format format, vk::Extent3D extent, uint32_t layer_count, const VmaAllocator& va)
    {
        uint32_t mip_levels = use_mip_levels ? std::floor(std::log2(std::max(extent.width, extent.height))) + 1 : 1;
        if (mip_levels > 1) usage |= vk::ImageUsageFlagBits::eTransferSrc;
        vk::ImageCreateInfo ici{};
        ici.sType = vk::StructureType::eImageCreateInfo;
        ici.imageType = vk::ImageType::e2D;
        ici.extent = extent;
        ici.extent.depth = 1;
        ici.mipLevels = mip_levels;
        ici.arrayLayers = layer_count;
        ici.format = format;
        ici.tiling = vk::ImageTiling::eOptimal;
        ici.initialLayout = vk::ImageLayout::eUndefined;
        ici.usage = usage;
        ici.sharingMode = queue_family_indices.size() == 1 ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent;
        ici.queueFamilyIndexCount = queue_family_indices.size();
        ici.pQueueFamilyIndices = queue_family_indices.data();
        ici.samples = sample_count;
        ici.flags = {};

        std::pair<vk::Image, VmaAllocation> image;
        VmaAllocationCreateInfo vaci{};
        vaci.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        vmaCreateImage(va, (VkImageCreateInfo*) (&ici), &vaci, (VkImage*) (&image.first), &image.second, nullptr);
        return image;
    }

    void perform_image_layout_transition(const vk::CommandBuffer& cb, vk::Image image, vk::ImageLayout old_layout, vk::ImageLayout new_layout, vk::PipelineStageFlags src_stage_flags, vk::PipelineStageFlags dst_stage_flags, vk::AccessFlags src_access_flags, vk::AccessFlags dst_access_flags, uint32_t base_mip_level, uint32_t mip_levels, uint32_t layer_count)
    {
        // perform actual image layout transition independent from this image
        // functionality is needed without changing the state of the class to enable setting a base_mip_map_level
        vk::ImageMemoryBarrier imb{};
        imb.sType = vk::StructureType::eImageMemoryBarrier;
        imb.oldLayout = old_layout;
        imb.newLayout = new_layout;
        imb.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imb.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imb.image = image;
        imb.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        imb.subresourceRange.baseMipLevel = 0;
        imb.subresourceRange.levelCount = mip_levels;
        imb.subresourceRange.baseArrayLayer = 0;
        imb.subresourceRange.layerCount = layer_count;
        imb.srcAccessMask = src_access_flags;
        imb.dstAccessMask = dst_access_flags;
        src_stage_flags = src_stage_flags;
        dst_stage_flags = dst_stage_flags;
        cb.pipelineBarrier(src_stage_flags, dst_stage_flags, {}, nullptr, nullptr, imb);
    }

    void copy_buffer_to_image(VulkanCommandContext& vcc, const Buffer& buffer, vk::Extent3D extent, vk::Image image, uint32_t layer_count, uint32_t pixel_byte_size)
    {
        vk::CommandBuffer& cb = vcc.begin(vcc.transfer_cb[0]);
        std::vector<vk::BufferImageCopy> copy_regions;
        for (uint32_t i = 0; i < layer_count; ++i)
        {
            vk::BufferImageCopy copy_region{};
            copy_region.bufferOffset = i * extent.width * extent.height * pixel_byte_size;
            copy_region.bufferRowLength = 0;
            copy_region.bufferImageHeight = 0;
            copy_region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
            copy_region.imageSubresource.mipLevel = 0;
            copy_region.imageSubresource.baseArrayLayer = i;
            copy_region.imageSubresource.layerCount = 1;
            copy_region.imageOffset = vk::Offset3D{0, 0, 0};
            copy_region.imageExtent = extent;
            copy_regions.push_back(copy_region);
        }

        cb.copyBufferToImage(buffer.get(), image, vk::ImageLayout::eTransferDstOptimal, copy_regions);
        vcc.submit_transfer(cb, true);
    }

    void Image::create_image_from_data(const unsigned char* data, VulkanCommandContext& vcc, const std::vector<uint32_t>& queue_family_indices, uint32_t base_mip_map_lvl)
    {
        Buffer buffer(vmc, vcc, data, byte_size, vk::BufferUsageFlagBits::eTransferSrc, false, vmc.queue_family_indices.transfer);

        vk::FormatProperties format_properties = vmc.physical_device.get().getFormatProperties(format);
        if (!(format_properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear))
        {
            mip_levels = 1;
            base_mip_map_lvl = 0;
        }

        auto move_buffer_to_image = [&](vk::Image image, uint32_t mip_levels) -> void {
            // copy image data to tmp_image
            vk::CommandBuffer& cb = vcc.begin(vcc.graphics_cb[0]);
            perform_image_layout_transition(cb, image, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {}, vk::AccessFlagBits::eTransferWrite, 0, mip_levels, layer_count);
            vcc.submit_graphics(cb, true);
            copy_buffer_to_image(vcc, buffer, vk::Extent3D(w, h, 1), image, layer_count, c);
        };

        // check if image should start at base_mip_map_lvl to save some storage
        // create image with original resolution and copy to actual image with reduced resolution
        if (base_mip_map_lvl > 0)
        {
            auto [tmp_image, tmp_alloc] = create_image({vmc.queue_family_indices.graphics, vmc.queue_family_indices.transfer}, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc, vk::SampleCountFlagBits::e1, false, format, vk::Extent3D(w, h, 1), layer_count, vmc.va);
            move_buffer_to_image(tmp_image, 1);

            vk::Offset3D tmp_image_offset(w, h, 1);
            mip_levels -= base_mip_map_lvl;
            w = std::max(1.0, w / (std::pow(2, base_mip_map_lvl)));
            h = std::max(1.0, h / (std::pow(2, base_mip_map_lvl)));
            byte_size = w * h * 4;

            // create image with reduced resolution by blitting
            vk::CommandBuffer& cb = vcc.begin(vcc.graphics_cb[0]);
            perform_image_layout_transition(cb, tmp_image, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eTransferSrcOptimal, vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eTransferRead, 0, 1, layer_count);
            std::tie(image, vmaa) = create_image(queue_family_indices, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled, vk::SampleCountFlagBits::e1, true, format, vk::Extent3D(w, h, 1), layer_count, vmc.va);
            perform_image_layout_transition(cb, image, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {}, vk::AccessFlagBits::eTransferWrite, 0, mip_levels, layer_count);
            blit_image(cb, tmp_image, 0, tmp_image_offset, image, 0, {w, h, 1}, layer_count);
            vcc.submit_graphics(cb, true);

            vmaDestroyImage(vmc.va, VkImage(tmp_image), tmp_alloc);
        }
        else
        {
            // layout of image is transitioned in move_buffer_to_image
            std::tie(image, vmaa) = create_image(queue_family_indices, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled, vk::SampleCountFlagBits::e1, true, format, vk::Extent3D(w, h, 1), layer_count, vmc.va);
            move_buffer_to_image(image, mip_levels);
        }
        buffer.self_destruct();
        // set current layout of this image
        layout = vk::ImageLayout::eTransferDstOptimal;
        mip_levels > 1 ? generate_mipmaps(vcc) : transition_image_layout(vcc, vk::ImageLayout::eShaderReadOnlyOptimal, vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead);
        create_image_view(vk::ImageAspectFlagBits::eColor);
        create_sampler();
    }

    void Image::create_image_view(vk::ImageAspectFlags aspects)
    {
        vk::ImageViewCreateInfo ivci{};
        ivci.sType = vk::StructureType::eImageViewCreateInfo;
        ivci.image = image;
        ivci.viewType = layer_count > 1 ? vk::ImageViewType::e2DArray : vk::ImageViewType::e2D;
        ivci.format = format;
        ivci.subresourceRange.aspectMask = aspects;
        ivci.subresourceRange.baseMipLevel = 0;
        ivci.subresourceRange.levelCount = mip_levels;
        ivci.subresourceRange.baseArrayLayer = 0;
        ivci.subresourceRange.layerCount = layer_count;
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

    void Image::transition_image_layout(VulkanCommandContext& vcc, vk::ImageLayout new_layout, vk::PipelineStageFlags src_stage_flags, vk::PipelineStageFlags dst_stage_flags, vk::AccessFlags src_access_flags, vk::AccessFlags dst_access_flags)
    {
        // transition the image layout of this image
        vk::CommandBuffer& cb = vcc.begin(vcc.graphics_cb[0]);
        perform_image_layout_transition(cb, image, layout, new_layout, src_stage_flags, dst_stage_flags, src_access_flags, dst_access_flags, 0, mip_levels, layer_count);
        vcc.submit_graphics(cb, true);
        layout = new_layout;
    }

    vk::DeviceSize Image::get_byte_size() const
    {
        return byte_size;
    }

    uint32_t Image::get_layer_count() const
    {
        return layer_count;
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

    void Image::generate_mipmaps(VulkanCommandContext& vcc)
    {
        vk::CommandBuffer& cb = vcc.begin(vcc.graphics_cb[0]);
        vk::ImageMemoryBarrier imb{};
        imb.sType = vk::StructureType::eImageMemoryBarrier;
        imb.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imb.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imb.image = image;
        imb.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        imb.subresourceRange.levelCount = 1;
        imb.subresourceRange.baseArrayLayer = 0;
        imb.subresourceRange.layerCount = layer_count;

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

            blit_image(cb, image, i - 1, {int32_t(mip_w), int32_t(mip_h), 1}, image, i, {int32_t(mip_w > 1 ? mip_w / 2 : 1), int32_t(mip_h > 1 ? mip_h / 2 : 1), 1}, layer_count);

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
} // namespace ve
