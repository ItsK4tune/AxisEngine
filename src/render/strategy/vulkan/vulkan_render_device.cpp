#include <render/strategy/vulkan/vulkan_render_device.h>
#if AXIS_HAS_VULKAN_BACKEND
#include <render/strategy/vulkan/vulkan_utils.h>
#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <vector>
namespace rhi
{
VulkanRenderDevice::VulkanRenderDevice() : m_CommandList(*this)
{
}

VulkanRenderDevice::~VulkanRenderDevice()
{
    Shutdown();
}

bool VulkanRenderDevice::Initialize(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device,
                                    VkQueue graphicsQueue, uint32_t graphicsQueueFamily)
{
    m_Instance = instance;
    m_PhysicalDevice = physicalDevice;
    m_Device = device;
    m_GraphicsQueue = graphicsQueue;
    m_GraphicsQueueFamily = graphicsQueueFamily;

    VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = graphicsQueueFamily;
    if (!Check(vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_CommandPool), "vkCreateCommandPool"))
        return false;

    std::array<VkDescriptorPoolSize, 5> sizes = {{
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1024},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1024},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1024},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1024},
        {VK_DESCRIPTOR_TYPE_SAMPLER, 1024},
    }};
    VkDescriptorPoolCreateInfo descriptorPoolInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    descriptorPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    descriptorPoolInfo.maxSets = 1024;
    descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(sizes.size());
    descriptorPoolInfo.pPoolSizes = sizes.data();
    return Check(vkCreateDescriptorPool(m_Device, &descriptorPoolInfo, nullptr, &m_DescriptorPool),
                 "vkCreateDescriptorPool");
}

void VulkanRenderDevice::Shutdown()
{
    if (!m_Device)
        return;

    WaitIdle();
    for (auto& [_, pipeline] : m_Pipelines)
    {
        if (pipeline.pipeline)
            vkDestroyPipeline(m_Device, pipeline.pipeline, nullptr);
        if (pipeline.layout)
            vkDestroyPipelineLayout(m_Device, pipeline.layout, nullptr);
    }
    for (auto& [_, renderPass] : m_RenderPassCache) vkDestroyRenderPass(m_Device, renderPass, nullptr);
    for (auto& [_, setLayout] : m_DescriptorSetLayouts)
        vkDestroyDescriptorSetLayout(m_Device, setLayout.layout, nullptr);
    for (auto& [_, shader] : m_ShaderModules) vkDestroyShaderModule(m_Device, shader.module, nullptr);
    for (auto& [_, sampler] : m_Samplers) vkDestroySampler(m_Device, sampler.sampler, nullptr);
    for (auto& [_, image] : m_Images)
    {
        if (image.view)
            vkDestroyImageView(m_Device, image.view, nullptr);
        if (image.owned && image.image)
            vkDestroyImage(m_Device, image.image, nullptr);
        if (image.owned && image.memory)
            vkFreeMemory(m_Device, image.memory, nullptr);
    }
    for (auto& [_, buffer] : m_Buffers)
    {
        vkDestroyBuffer(m_Device, buffer.buffer, nullptr);
        vkFreeMemory(m_Device, buffer.memory, nullptr);
    }
    if (m_DescriptorPool)
        vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
    if (m_CommandPool)
        vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);

    m_Pipelines.clear();
    m_RenderPassCache.clear();
    m_DescriptorSets.clear();
    m_DescriptorSetLayouts.clear();
    m_ShaderModules.clear();
    m_Samplers.clear();
    m_Images.clear();
    m_Buffers.clear();
    m_DescriptorPool = VK_NULL_HANDLE;
    m_CommandPool = VK_NULL_HANDLE;
}

BufferHandle VulkanRenderDevice::CreateBuffer(const BufferDesc& desc, const void* initialData)
{
    VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = desc.size;
    bufferInfo.usage = ToVkBufferUsage(desc.usage);
    if (desc.memoryUsage == MemoryUsage::GpuOnly && initialData)
        bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer buffer = VK_NULL_HANDLE;
    if (!Check(vkCreateBuffer(m_Device, &bufferInfo, nullptr, &buffer), "vkCreateBuffer"))
        return {};

    VkMemoryRequirements requirements{};
    vkGetBufferMemoryRequirements(m_Device, buffer, &requirements);

    VkMemoryAllocateInfo allocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocInfo.allocationSize = requirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(requirements.memoryTypeBits, ToVkMemoryProperties(desc.memoryUsage));

    VkDeviceMemory memory = VK_NULL_HANDLE;
    if (!Check(vkAllocateMemory(m_Device, &allocInfo, nullptr, &memory), "vkAllocateMemory(Buffer)"))
    {
        vkDestroyBuffer(m_Device, buffer, nullptr);
        return {};
    }
    vkBindBufferMemory(m_Device, buffer, memory, 0);

    const uint32_t id = AllocateHandle();
    m_Buffers[id] = BufferResource{buffer, memory, desc.size, desc.memoryUsage};
    if (initialData)
        UpdateBuffer(BufferHandle{id}, 0, desc.size, initialData);
    return BufferHandle{id};
}

void VulkanRenderDevice::UpdateBuffer(BufferHandle handle, uint64_t offset, uint64_t size, const void* data)
{
    const auto* buffer = GetBuffer(handle);
    if (!buffer || !data || offset + size > buffer->size)
        return;

    if (buffer->memoryUsage == MemoryUsage::GpuOnly)
    {
        BufferDesc stagingDesc;
        stagingDesc.size = size;
        stagingDesc.usage = BufferUsage::TransferSrc;
        stagingDesc.memoryUsage = MemoryUsage::CpuToGpu;
        stagingDesc.debugName = "VulkanBufferUpload";
        BufferHandle staging = CreateBuffer(stagingDesc, data);
        const auto* stagingBuffer = GetBuffer(staging);
        if (!stagingBuffer)
            return;

        VkCommandBuffer commandBuffer = BeginImmediateCommandBuffer();
        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = offset;
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, stagingBuffer->buffer, buffer->buffer, 1, &copyRegion);

        VkBufferMemoryBarrier barrier{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_INDEX_READ_BIT |
                                VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_READ_BIT;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.buffer = buffer->buffer;
        barrier.offset = offset;
        barrier.size = size;
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0,
                             nullptr, 1, &barrier, 0, nullptr);
        EndImmediateCommandBuffer(commandBuffer);
        DestroyBuffer(staging);
        return;
    }

    void* mapped = nullptr;
    if (vkMapMemory(m_Device, buffer->memory, offset, size, 0, &mapped) == VK_SUCCESS)
    {
        std::memcpy(mapped, data, static_cast<size_t>(size));
        vkUnmapMemory(m_Device, buffer->memory);
    }
}

void VulkanRenderDevice::DestroyBuffer(BufferHandle handle)
{
    auto it = m_Buffers.find(handle.id);
    if (it == m_Buffers.end())
        return;
    vkDestroyBuffer(m_Device, it->second.buffer, nullptr);
    vkFreeMemory(m_Device, it->second.memory, nullptr);
    m_Buffers.erase(it);
}

ImageHandle VulkanRenderDevice::CreateImage(const ImageDesc& desc, const void* initialData)
{
    (void)initialData;
    VkImageCreateInfo imageInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    imageInfo.imageType = desc.dimension == ImageDimension::Image3D ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D;
    imageInfo.extent = {desc.width, desc.height, desc.depth};
    imageInfo.mipLevels = desc.mipLevels;
    imageInfo.arrayLayers = desc.dimension == ImageDimension::Cube ? 6 : desc.arrayLayers;
    imageInfo.format = ToVkFormat(desc.format);
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = ToVkImageUsage(desc.usage);
    if (initialData)
        imageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (desc.dimension == ImageDimension::Cube)
        imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

    VkImage image = VK_NULL_HANDLE;
    if (!Check(vkCreateImage(m_Device, &imageInfo, nullptr, &image), "vkCreateImage"))
        return {};

    VkMemoryRequirements requirements{};
    vkGetImageMemoryRequirements(m_Device, image, &requirements);
    VkMemoryAllocateInfo allocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocInfo.allocationSize = requirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkDeviceMemory memory = VK_NULL_HANDLE;
    if (!Check(vkAllocateMemory(m_Device, &allocInfo, nullptr, &memory), "vkAllocateMemory(Image)"))
    {
        vkDestroyImage(m_Device, image, nullptr);
        return {};
    }
    vkBindImageMemory(m_Device, image, memory, 0);

    VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    viewInfo.image = image;
    viewInfo.viewType =
        desc.dimension == ImageDimension::Cube
            ? VK_IMAGE_VIEW_TYPE_CUBE
            : (desc.dimension == ImageDimension::Image3D ? VK_IMAGE_VIEW_TYPE_3D : VK_IMAGE_VIEW_TYPE_2D);
    viewInfo.format = imageInfo.format;
    viewInfo.subresourceRange.aspectMask = GetAspectMask(desc.format);
    viewInfo.subresourceRange.levelCount = desc.mipLevels;
    viewInfo.subresourceRange.layerCount = imageInfo.arrayLayers;

    VkImageView view = VK_NULL_HANDLE;
    if (!Check(vkCreateImageView(m_Device, &viewInfo, nullptr, &view), "vkCreateImageView"))
    {
        vkDestroyImage(m_Device, image, nullptr);
        vkFreeMemory(m_Device, memory, nullptr);
        return {};
    }

    const uint32_t id = AllocateHandle();
    m_Images[id] = ImageResource{image, view, memory, desc, VK_IMAGE_LAYOUT_UNDEFINED, true};
    if (initialData)
    {
        const uint32_t layers = desc.dimension == ImageDimension::Image3D ? 1 : imageInfo.arrayLayers;
        const uint32_t depth = desc.dimension == ImageDimension::Image3D ? desc.depth : 1;
        const uint64_t uploadSize =
            static_cast<uint64_t>(desc.width) * desc.height * depth * layers * GetFormatByteSize(desc.format);

        BufferDesc stagingDesc;
        stagingDesc.size = uploadSize;
        stagingDesc.usage = BufferUsage::TransferSrc;
        stagingDesc.memoryUsage = MemoryUsage::CpuToGpu;
        stagingDesc.debugName = "VulkanImageUpload";
        BufferHandle staging = CreateBuffer(stagingDesc, initialData);
        const auto* stagingBuffer = GetBuffer(staging);
        if (stagingBuffer)
        {
            VkCommandBuffer commandBuffer = BeginImmediateCommandBuffer();

            VkImageMemoryBarrier toTransfer{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
            toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            toTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            toTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            toTransfer.image = image;
            toTransfer.subresourceRange.aspectMask = GetAspectMask(desc.format);
            toTransfer.subresourceRange.baseMipLevel = 0;
            toTransfer.subresourceRange.levelCount = desc.mipLevels;
            toTransfer.subresourceRange.baseArrayLayer = 0;
            toTransfer.subresourceRange.layerCount = imageInfo.arrayLayers;
            toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
                                 nullptr, 0, nullptr, 1, &toTransfer);

            VkBufferImageCopy copyRegion{};
            copyRegion.imageSubresource.aspectMask = GetAspectMask(desc.format);
            copyRegion.imageSubresource.mipLevel = 0;
            copyRegion.imageSubresource.baseArrayLayer = 0;
            copyRegion.imageSubresource.layerCount = layers;
            copyRegion.imageExtent = {desc.width, desc.height, depth};
            vkCmdCopyBufferToImage(commandBuffer, stagingBuffer->buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                                   &copyRegion);

            VkImageLayout finalLayout = VK_IMAGE_LAYOUT_GENERAL;
            if (HasFlag(desc.usage, ImageUsage::Sampled))
                finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            else if (HasFlag(desc.usage, ImageUsage::ColorAttachment))
                finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            else if (HasFlag(desc.usage, ImageUsage::DepthStencilAttachment))
                finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            VkImageMemoryBarrier toFinal = toTransfer;
            toFinal.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            toFinal.newLayout = finalLayout;
            toFinal.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            toFinal.dstAccessMask = HasFlag(desc.usage, ImageUsage::Sampled)
                                        ? VK_ACCESS_SHADER_READ_BIT
                                        : VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0,
                                 0, nullptr, 0, nullptr, 1, &toFinal);

            EndImmediateCommandBuffer(commandBuffer);
            m_Images[id].layout = finalLayout;
        }
        DestroyBuffer(staging);
    }
    return ImageHandle{id};
}

void VulkanRenderDevice::DestroyImage(ImageHandle handle)
{
    auto it = m_Images.find(handle.id);
    if (it == m_Images.end())
        return;
    if (it->second.view)
        vkDestroyImageView(m_Device, it->second.view, nullptr);
    if (it->second.owned && it->second.image)
        vkDestroyImage(m_Device, it->second.image, nullptr);
    if (it->second.owned && it->second.memory)
        vkFreeMemory(m_Device, it->second.memory, nullptr);
    m_Images.erase(it);
}

SamplerHandle VulkanRenderDevice::CreateSampler(const SamplerDesc& desc)
{
    VkSamplerCreateInfo samplerInfo{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    samplerInfo.magFilter = ToVkFilter(desc.magFilter);
    samplerInfo.minFilter = ToVkFilter(desc.minFilter);
    samplerInfo.mipmapMode = ToVkMipmapMode(desc.mipmapMode);
    samplerInfo.addressModeU = ToVkAddressMode(desc.addressU);
    samplerInfo.addressModeV = ToVkAddressMode(desc.addressV);
    samplerInfo.addressModeW = ToVkAddressMode(desc.addressW);
    samplerInfo.maxAnisotropy = desc.maxAnisotropy;
    samplerInfo.anisotropyEnable = desc.maxAnisotropy > 1.0f ? VK_TRUE : VK_FALSE;
    samplerInfo.maxLod = VK_LOD_CLAMP_NONE;

    VkSampler sampler = VK_NULL_HANDLE;
    if (!Check(vkCreateSampler(m_Device, &samplerInfo, nullptr, &sampler), "vkCreateSampler"))
        return {};

    const uint32_t id = AllocateHandle();
    m_Samplers[id] = SamplerResource{sampler};
    return SamplerHandle{id};
}

void VulkanRenderDevice::DestroySampler(SamplerHandle handle)
{
    auto it = m_Samplers.find(handle.id);
    if (it == m_Samplers.end())
        return;
    vkDestroySampler(m_Device, it->second.sampler, nullptr);
    m_Samplers.erase(it);
}

ShaderModuleHandle VulkanRenderDevice::CreateShaderModule(const ShaderModuleDesc& desc)
{
    if (desc.sourceType != ShaderSourceType::SpirVBinary || !desc.data || desc.size == 0)
    {
        LOGGER_ERROR("VulkanRHI") << "Vulkan shader modules require SPIR-V binary input.";
        return {};
    }

    VkShaderModuleCreateInfo moduleInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    moduleInfo.codeSize = static_cast<size_t>(desc.size);
    moduleInfo.pCode = static_cast<const uint32_t*>(desc.data);

    VkShaderModule module = VK_NULL_HANDLE;
    if (!Check(vkCreateShaderModule(m_Device, &moduleInfo, nullptr, &module), "vkCreateShaderModule"))
        return {};

    const uint32_t id = AllocateHandle();
    m_ShaderModules[id] = ShaderModuleResource{module, desc.stage, desc.entryPoint};
    return ShaderModuleHandle{id};
}

void VulkanRenderDevice::DestroyShaderModule(ShaderModuleHandle handle)
{
    auto it = m_ShaderModules.find(handle.id);
    if (it == m_ShaderModules.end())
        return;
    vkDestroyShaderModule(m_Device, it->second.module, nullptr);
    m_ShaderModules.erase(it);
}

DescriptorSetLayoutHandle VulkanRenderDevice::CreateDescriptorSetLayout(const DescriptorSetLayoutDesc& desc)
{
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    bindings.reserve(desc.bindings.size());
    for (const auto& binding : desc.bindings)
    {
        VkDescriptorSetLayoutBinding vkBinding{};
        vkBinding.binding = binding.binding;
        vkBinding.descriptorType = ToVkDescriptorType(binding.type);
        vkBinding.descriptorCount = binding.count;
        vkBinding.stageFlags = ToVkShaderStages(binding.stages);
        bindings.push_back(vkBinding);
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    if (!Check(vkCreateDescriptorSetLayout(m_Device, &layoutInfo, nullptr, &layout), "vkCreateDescriptorSetLayout"))
        return {};

    const uint32_t id = AllocateHandle();
    m_DescriptorSetLayouts[id] = DescriptorSetLayoutResource{layout, desc};
    return DescriptorSetLayoutHandle{id};
}

void VulkanRenderDevice::DestroyDescriptorSetLayout(DescriptorSetLayoutHandle handle)
{
    auto it = m_DescriptorSetLayouts.find(handle.id);
    if (it == m_DescriptorSetLayouts.end())
        return;
    vkDestroyDescriptorSetLayout(m_Device, it->second.layout, nullptr);
    m_DescriptorSetLayouts.erase(it);
}

DescriptorSetHandle VulkanRenderDevice::CreateDescriptorSet(DescriptorSetLayoutHandle layout)
{
    const auto* layoutResource = GetDescriptorSetLayout(layout);
    if (!layoutResource)
        return {};

    VkDescriptorSetAllocateInfo allocInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocInfo.descriptorPool = m_DescriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layoutResource->layout;

    VkDescriptorSet set = VK_NULL_HANDLE;
    if (!Check(vkAllocateDescriptorSets(m_Device, &allocInfo, &set), "vkAllocateDescriptorSets"))
        return {};

    const uint32_t id = AllocateHandle();
    m_DescriptorSets[id] = DescriptorSetResource{set, layout};
    return DescriptorSetHandle{id};
}

void VulkanRenderDevice::UpdateDescriptorSet(DescriptorSetHandle set, const DescriptorUpdate* updates, uint32_t count)
{
    const auto* setResource = GetDescriptorSet(set);
    if (!setResource || !updates)
        return;

    std::vector<VkDescriptorBufferInfo> bufferInfos;
    std::vector<VkDescriptorImageInfo> imageInfos;
    std::vector<VkWriteDescriptorSet> writes;
    bufferInfos.reserve(count);
    imageInfos.reserve(count);
    writes.reserve(count);

    for (uint32_t i = 0; i < count; ++i)
    {
        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.dstSet = setResource->set;
        write.dstBinding = updates[i].binding;
        write.dstArrayElement = updates[i].arrayElement;
        write.descriptorType = ToVkDescriptorType(updates[i].type);
        write.descriptorCount = 1;

        if (updates[i].type == DescriptorType::UniformBuffer || updates[i].type == DescriptorType::StorageBuffer)
        {
            const auto* buffer = GetBuffer(updates[i].buffer);
            if (!buffer)
                continue;
            bufferInfos.push_back(
                VkDescriptorBufferInfo{buffer->buffer, updates[i].bufferOffset,
                                       updates[i].bufferRange ? updates[i].bufferRange : VK_WHOLE_SIZE});
            write.pBufferInfo = &bufferInfos.back();
        }
        else
        {
            const auto* image = GetImage(updates[i].image);
            const auto* sampler = GetSampler(updates[i].sampler);
            imageInfos.push_back(VkDescriptorImageInfo{sampler ? sampler->sampler : VK_NULL_HANDLE,
                                                       image ? image->view : VK_NULL_HANDLE,
                                                       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
            write.pImageInfo = &imageInfos.back();
        }
        writes.push_back(write);
    }

    if (!writes.empty())
        vkUpdateDescriptorSets(m_Device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}

void VulkanRenderDevice::DestroyDescriptorSet(DescriptorSetHandle handle)
{
    auto it = m_DescriptorSets.find(handle.id);
    if (it == m_DescriptorSets.end())
        return;
    vkFreeDescriptorSets(m_Device, m_DescriptorPool, 1, &it->second.set);
    m_DescriptorSets.erase(it);
}

PipelineHandle VulkanRenderDevice::CreateGraphicsPipeline(const GraphicsPipelineDesc& desc)
{
    const auto* vertexShader = GetShaderModule(desc.vertexShader);
    const auto* fragmentShader = GetShaderModule(desc.fragmentShader);
    if (!vertexShader || !fragmentShader)
        return {};

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    shaderStages.reserve(desc.geometryShader ? 3 : 2);
    auto addStage = [&](const ShaderModuleResource& shader) {
        VkPipelineShaderStageCreateInfo stageInfo{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
        stageInfo.stage = ToVkShaderStageBit(shader.stage);
        stageInfo.module = shader.module;
        stageInfo.pName = shader.entryPoint.c_str();
        shaderStages.push_back(stageInfo);
    };
    addStage(*vertexShader);
    addStage(*fragmentShader);
    if (const auto* geometryShader = GetShaderModule(desc.geometryShader))
        addStage(*geometryShader);

    std::vector<VkVertexInputBindingDescription> bindings;
    bindings.reserve(desc.vertexInput.bindings.size());
    for (const auto& binding : desc.vertexInput.bindings)
    {
        VkVertexInputBindingDescription vkBinding{};
        vkBinding.binding = binding.binding;
        vkBinding.stride = binding.stride;
        vkBinding.inputRate = binding.inputRate == VertexInputRate::PerInstance ? VK_VERTEX_INPUT_RATE_INSTANCE
                                                                                : VK_VERTEX_INPUT_RATE_VERTEX;
        bindings.push_back(vkBinding);
    }

    std::vector<VkVertexInputAttributeDescription> attributes;
    attributes.reserve(desc.vertexInput.attributes.size());
    for (const auto& attribute : desc.vertexInput.attributes)
    {
        VkVertexInputAttributeDescription vkAttribute{};
        vkAttribute.location = attribute.location;
        vkAttribute.binding = attribute.binding;
        vkAttribute.format = ToVkVertexFormat(attribute.format);
        vkAttribute.offset = attribute.offset;
        attributes.push_back(vkAttribute);
    }

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindings.size());
    vertexInputInfo.pVertexBindingDescriptions = bindings.data();
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributes.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    inputAssembly.topology = ToVkTopology(desc.topology);
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = ToVkPolygonMode(desc.rasterizer.polygonMode);
    rasterizer.cullMode = ToVkCullMode(desc.rasterizer.cullMode);
    rasterizer.frontFace = ToVkFrontFace(desc.rasterizer.frontFace);
    rasterizer.lineWidth = desc.rasterizer.lineWidth;

    VkPipelineMultisampleStateCreateInfo multisampling{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    depthStencil.depthTestEnable = desc.depthStencil.depthTestEnable ? VK_TRUE : VK_FALSE;
    depthStencil.depthWriteEnable = desc.depthStencil.depthWriteEnable ? VK_TRUE : VK_FALSE;
    depthStencil.depthCompareOp = ToVkCompareOp(desc.depthStencil.depthCompare);
    depthStencil.stencilTestEnable = desc.depthStencil.stencilTestEnable ? VK_TRUE : VK_FALSE;

    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;
    const size_t colorAttachmentCount = std::max<size_t>(desc.renderTargetLayout.colorFormats.size(), 1);
    colorBlendAttachments.reserve(colorAttachmentCount);
    for (size_t i = 0; i < colorAttachmentCount; ++i)
    {
        BlendAttachmentDesc blend = i < desc.blendAttachments.size() ? desc.blendAttachments[i] : BlendAttachmentDesc{};
        VkPipelineColorBlendAttachmentState attachment{};
        attachment.blendEnable = blend.blendEnable ? VK_TRUE : VK_FALSE;
        attachment.srcColorBlendFactor = ToVkBlendFactor(blend.srcColorBlendFactor);
        attachment.dstColorBlendFactor = ToVkBlendFactor(blend.dstColorBlendFactor);
        attachment.colorBlendOp = ToVkBlendOp(blend.colorBlendOp);
        attachment.srcAlphaBlendFactor = ToVkBlendFactor(blend.srcAlphaBlendFactor);
        attachment.dstAlphaBlendFactor = ToVkBlendFactor(blend.dstAlphaBlendFactor);
        attachment.alphaBlendOp = ToVkBlendOp(blend.alphaBlendOp);
        attachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachments.push_back(attachment);
    }

    VkPipelineColorBlendStateCreateInfo colorBlending{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    colorBlending.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size());
    colorBlending.pAttachments = colorBlendAttachments.data();

    std::array<VkDynamicState, 2> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    std::vector<VkDescriptorSetLayout> setLayouts;
    for (auto layout : desc.descriptorSetLayouts)
    {
        if (const auto* resource = GetDescriptorSetLayout(layout))
            setLayouts.push_back(resource->layout);
    }

    VkPushConstantRange pushRange{};
    VkShaderStageFlags stages = 0;
    if (desc.vertexShader) stages |= VK_SHADER_STAGE_VERTEX_BIT;
    if (desc.fragmentShader) stages |= VK_SHADER_STAGE_FRAGMENT_BIT;
    if (desc.geometryShader) stages |= VK_SHADER_STAGE_GEOMETRY_BIT;
    pushRange.stageFlags = stages;
    pushRange.size = desc.pushConstantSize;

    VkPipelineLayoutCreateInfo layoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
    layoutInfo.pSetLayouts = setLayouts.data();
    layoutInfo.pushConstantRangeCount = desc.pushConstantSize > 0 ? 1u : 0u;
    layoutInfo.pPushConstantRanges = desc.pushConstantSize > 0 ? &pushRange : nullptr;

    VkPipelineLayout layout = VK_NULL_HANDLE;
    if (!Check(vkCreatePipelineLayout(m_Device, &layoutInfo, nullptr, &layout), "vkCreatePipelineLayout(Graphics)"))
        return {};

    VkRenderPass renderPass = GetOrCreateRenderPass(desc.renderTargetLayout);
    if (!renderPass)
    {
        vkDestroyPipelineLayout(m_Device, layout, nullptr);
        return {};
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState =
        desc.renderTargetLayout.depthStencilFormat != Format::Undefined ? &depthStencil : nullptr;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = layout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    VkPipeline pipeline = VK_NULL_HANDLE;
    if (!Check(vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline),
               "vkCreateGraphicsPipelines"))
    {
        vkDestroyPipelineLayout(m_Device, layout, nullptr);
        return {};
    }

    const uint32_t id = AllocateHandle();
    m_Pipelines[id] = PipelineResource{pipeline, layout, false};
    return PipelineHandle{id};
}

PipelineHandle VulkanRenderDevice::CreateComputePipeline(const ComputePipelineDesc& desc)
{
    const auto* shader = GetShaderModule(desc.computeShader);
    if (!shader)
        return {};

    std::vector<VkDescriptorSetLayout> setLayouts;
    for (auto layout : desc.descriptorSetLayouts)
    {
        if (const auto* resource = GetDescriptorSetLayout(layout))
            setLayouts.push_back(resource->layout);
    }

    VkPushConstantRange pushRange{};
    pushRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushRange.size = desc.pushConstantSize;

    VkPipelineLayoutCreateInfo layoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
    layoutInfo.pSetLayouts = setLayouts.data();
    layoutInfo.pushConstantRangeCount = desc.pushConstantSize > 0 ? 1u : 0u;
    layoutInfo.pPushConstantRanges = desc.pushConstantSize > 0 ? &pushRange : nullptr;

    VkPipelineLayout layout = VK_NULL_HANDLE;
    if (!Check(vkCreatePipelineLayout(m_Device, &layoutInfo, nullptr, &layout), "vkCreatePipelineLayout"))
        return {};

    VkComputePipelineCreateInfo pipelineInfo{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
    pipelineInfo.stage = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipelineInfo.stage.module = shader->module;
    pipelineInfo.stage.pName = shader->entryPoint.c_str();
    pipelineInfo.layout = layout;

    VkPipeline pipeline = VK_NULL_HANDLE;
    if (!Check(vkCreateComputePipelines(m_Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline),
               "vkCreateComputePipelines"))
    {
        vkDestroyPipelineLayout(m_Device, layout, nullptr);
        return {};
    }

    const uint32_t id = AllocateHandle();
    m_Pipelines[id] = PipelineResource{pipeline, layout, true};
    return PipelineHandle{id};
}

void VulkanRenderDevice::DestroyPipeline(PipelineHandle handle)
{
    auto it = m_Pipelines.find(handle.id);
    if (it == m_Pipelines.end())
        return;
    vkDestroyPipeline(m_Device, it->second.pipeline, nullptr);
    vkDestroyPipelineLayout(m_Device, it->second.layout, nullptr);
    m_Pipelines.erase(it);
}

ICommandList& VulkanRenderDevice::BeginCommandList(CommandQueueType queue)
{
    (void)queue;
    m_CommandList.Begin();
    return m_CommandList;
}

void VulkanRenderDevice::Submit(ICommandList& commandList)
{
    commandList.End();
    auto* vkCommandList = dynamic_cast<VulkanCommandList*>(&commandList);
    if (!vkCommandList)
        return;

    VkCommandBuffer commandBuffer = vkCommandList->GetNative();
    VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    if (Check(vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE), "vkQueueSubmit"))
        vkQueueWaitIdle(m_GraphicsQueue);
}

void VulkanRenderDevice::WaitIdle()
{
    if (m_Device)
        vkDeviceWaitIdle(m_Device);
}

BackendType VulkanRenderDevice::GetBackendType() const
{
    return BackendType::Vulkan;
}

const char* VulkanRenderDevice::GetName() const
{
    return "Vulkan";
}

uint32_t VulkanRenderDevice::AllocateHandle()
{
    return m_NextHandle++;
}

uint32_t VulkanRenderDevice::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const
{
    VkPhysicalDeviceMemoryProperties memoryProperties{};
    vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memoryProperties);
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
    {
        if ((typeFilter & (1u << i)) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }
    return 0;
}

VkCommandBuffer VulkanRenderDevice::AllocateCommandBuffer()
{
    VkCommandBufferAllocateInfo allocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocInfo.commandPool = m_CommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    Check(vkAllocateCommandBuffers(m_Device, &allocInfo, &commandBuffer), "vkAllocateCommandBuffers");
    return commandBuffer;
}

VkCommandBuffer VulkanRenderDevice::BeginImmediateCommandBuffer()
{
    VkCommandBuffer commandBuffer = AllocateCommandBuffer();
    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    Check(vkBeginCommandBuffer(commandBuffer, &beginInfo), "vkBeginCommandBuffer(Immediate)");
    return commandBuffer;
}

void VulkanRenderDevice::EndImmediateCommandBuffer(VkCommandBuffer commandBuffer)
{
    if (!commandBuffer)
        return;

    Check(vkEndCommandBuffer(commandBuffer), "vkEndCommandBuffer(Immediate)");
    VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    if (Check(vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE), "vkQueueSubmit(Immediate)"))
        vkQueueWaitIdle(m_GraphicsQueue);
    vkFreeCommandBuffers(m_Device, m_CommandPool, 1, &commandBuffer);
}

VkRenderPass VulkanRenderDevice::GetOrCreateRenderPass(const RenderTargetLayoutDesc& layout)
{
    const std::string key = RenderPassKey(layout);
    if (auto it = m_RenderPassCache.find(key); it != m_RenderPassCache.end())
        return it->second;

    std::vector<VkAttachmentDescription> attachments;
    std::vector<VkAttachmentReference> colorRefs;
    attachments.reserve(layout.colorFormats.size() + 1);
    colorRefs.reserve(layout.colorFormats.size());

    for (uint32_t i = 0; i < layout.colorFormats.size(); ++i)
    {
        VkAttachmentDescription attachment{};
        attachment.format = ToVkFormat(layout.colorFormats[i]);
        attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachments.push_back(attachment);

        VkAttachmentReference ref{};
        ref.attachment = i;
        ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorRefs.push_back(ref);
    }

    VkAttachmentReference depthRef{};
    const bool hasDepth = layout.depthStencilFormat != Format::Undefined;
    if (hasDepth)
    {
        VkAttachmentDescription attachment{};
        attachment.format = ToVkFormat(layout.depthStencilFormat);
        attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depthRef.attachment = static_cast<uint32_t>(attachments.size());
        depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachments.push_back(attachment);
    }

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = static_cast<uint32_t>(colorRefs.size());
    subpass.pColorAttachments = colorRefs.data();
    subpass.pDepthStencilAttachment = hasDepth ? &depthRef : nullptr;

    VkRenderPassCreateInfo renderPassInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    VkRenderPass renderPass = VK_NULL_HANDLE;
    if (!Check(vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &renderPass), "vkCreateRenderPass"))
        return VK_NULL_HANDLE;

    m_RenderPassCache[key] = renderPass;
    return renderPass;
}

VkFramebuffer VulkanRenderDevice::CreateFramebuffer(const RenderPassBeginInfo& beginInfo, VkRenderPass renderPass)
{
    std::vector<VkImageView> attachments;
    attachments.reserve(beginInfo.colorAttachments.size() + (beginInfo.hasDepthStencilAttachment ? 1 : 0));
    for (const auto& attachment : beginInfo.colorAttachments)
    {
        const auto* image = GetImage(attachment.image);
        if (!image)
            return VK_NULL_HANDLE;
        attachments.push_back(image->view);
    }

    if (beginInfo.hasDepthStencilAttachment)
    {
        const auto* image = GetImage(beginInfo.depthStencilAttachment.image);
        if (!image)
            return VK_NULL_HANDLE;
        attachments.push_back(image->view);
    }

    VkFramebufferCreateInfo framebufferInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments = attachments.data();
    framebufferInfo.width = beginInfo.renderArea.width;
    framebufferInfo.height = beginInfo.renderArea.height;
    framebufferInfo.layers = 1;

    VkFramebuffer framebuffer = VK_NULL_HANDLE;
    if (!Check(vkCreateFramebuffer(m_Device, &framebufferInfo, nullptr, &framebuffer), "vkCreateFramebuffer"))
        return VK_NULL_HANDLE;
    return framebuffer;
}

void VulkanRenderDevice::TransitionImage(ImageHandle handle, VkImageLayout newLayout, VkImageAspectFlags aspectMask)
{
    if (auto* image = GetImage(handle))
        TransitionImage(*image, newLayout, aspectMask);
}

void VulkanRenderDevice::TransitionImage(ImageResource& image, VkImageLayout newLayout, VkImageAspectFlags aspectMask)
{
    if (image.layout == newLayout || m_CommandList.GetNative() == VK_NULL_HANDLE)
        return;

    VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    barrier.oldLayout = image.layout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image.image;
    barrier.subresourceRange.aspectMask = aspectMask;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = image.desc.mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = image.desc.dimension == ImageDimension::Cube ? 6 : image.desc.arrayLayers;

    VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;

    if (newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    else if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        barrier.dstAccessMask =
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    else if (newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    else if (newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
        barrier.dstAccessMask = 0;

    vkCmdPipelineBarrier(m_CommandList.GetNative(), srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    image.layout = newLayout;
}

VulkanRenderDevice::ImageResource* VulkanRenderDevice::GetImage(ImageHandle handle)
{
    auto it = m_Images.find(handle.id);
    return it == m_Images.end() ? nullptr : &it->second;
}

const VulkanRenderDevice::BufferResource* VulkanRenderDevice::GetBuffer(BufferHandle handle) const
{
    auto it = m_Buffers.find(handle.id);
    return it == m_Buffers.end() ? nullptr : &it->second;
}

const VulkanRenderDevice::ImageResource* VulkanRenderDevice::GetImage(ImageHandle handle) const
{
    auto it = m_Images.find(handle.id);
    return it == m_Images.end() ? nullptr : &it->second;
}

const VulkanRenderDevice::SamplerResource* VulkanRenderDevice::GetSampler(SamplerHandle handle) const
{
    auto it = m_Samplers.find(handle.id);
    return it == m_Samplers.end() ? nullptr : &it->second;
}

const VulkanRenderDevice::ShaderModuleResource* VulkanRenderDevice::GetShaderModule(ShaderModuleHandle handle) const
{
    auto it = m_ShaderModules.find(handle.id);
    return it == m_ShaderModules.end() ? nullptr : &it->second;
}

const VulkanRenderDevice::DescriptorSetLayoutResource* VulkanRenderDevice::GetDescriptorSetLayout(
    DescriptorSetLayoutHandle handle) const
{
    auto it = m_DescriptorSetLayouts.find(handle.id);
    return it == m_DescriptorSetLayouts.end() ? nullptr : &it->second;
}

const VulkanRenderDevice::DescriptorSetResource* VulkanRenderDevice::GetDescriptorSet(DescriptorSetHandle handle) const
{
    auto it = m_DescriptorSets.find(handle.id);
    return it == m_DescriptorSets.end() ? nullptr : &it->second;
}

const VulkanRenderDevice::PipelineResource* VulkanRenderDevice::GetPipeline(PipelineHandle handle) const
{
    auto it = m_Pipelines.find(handle.id);
    return it == m_Pipelines.end() ? nullptr : &it->second;
}

}  // namespace rhi
#endif
