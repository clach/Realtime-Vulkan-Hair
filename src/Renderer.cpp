#include "Renderer.h"
#include "Instance.h"
#include "ShaderModule.h"
#include "Vertex.h"
#include "Strand.h"
#include "Camera.h"
#include "Image.h"

#define SHADOWMAP_WIDTH 1080
#define SHADOWMAP_HEIGHT 720
//#define DEPTH_FORMAT VK_FORMAT_D16_UNORM
#define DEPTH_FORMAT VK_FORMAT_D32_SFLOAT
#define OPACITY_MAP_FORMAT VK_FORMAT_R8G8B8A8_UNORM
#define SHADOWMAP_FILTER VK_FILTER_LINEAR

static constexpr unsigned int WORKGROUP_SIZE = 32;

Renderer::Renderer(Device* device, SwapChain* swapChain, Scene* scene, Camera* camera, Camera* shadowCamera)
  : device(device),
    logicalDevice(device->GetVkDevice()),
    swapChain(swapChain),
    scene(scene),
    camera(camera),
	shadowCamera(shadowCamera) {

    CreateCommandPools();

    CreateRenderPass();
	CreateShadowMapRenderPass();
	CreateOpacityMapRenderPass();

    CreateCameraDescriptorSetLayout();
    CreateModelMatrixDescriptorSetLayout();
    CreateTextureDescriptorSetLayout();
	CreateHairDescriptorSetLayout();
	CreateOpacityMapDescriptorSetLayout();
    CreateTimeDescriptorSetLayout();
	CreateCollidersDescriptorSetLayout();
	CreateGridDescriptorSetLayout();
    CreateComputeDescriptorSetLayout();

    CreateDescriptorPool();

	CreateFrameResources();
	CreateShadowMapFrameResources();
	CreateOpacityMapFrameResources();

    CreateCameraDescriptorSet();
    CreateModelDescriptorSet();
    CreateTextureDescriptorSets();
    CreateShadowCameraDescriptorSet();
    CreateHairDescriptorSets();
	CreateOpacityMapHairDescriptorSets();
	CreateOpacityMapDescriptorSets();
    CreateTimeDescriptorSet();
	CreateCollidersDescriptorSets();
	CreateGridDescriptorSets();
    CreateComputeDescriptorSets();

	CreateShadowMapPipeline();
	CreateOpacityMapPipeline();
    CreateGraphicsPipeline();
    CreateHairPipeline();
    CreateComputePipeline();

    RecordCommandBuffers();
    RecordComputeCommandBuffer();
}

void Renderer::CreateCommandPools() {
    VkCommandPoolCreateInfo graphicsPoolInfo = {};
    graphicsPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    graphicsPoolInfo.queueFamilyIndex = device->GetInstance()->GetQueueFamilyIndices()[QueueFlags::Graphics];
    graphicsPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(logicalDevice, &graphicsPoolInfo, nullptr, &graphicsCommandPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool");
    }

    VkCommandPoolCreateInfo computePoolInfo = {};
    computePoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    computePoolInfo.queueFamilyIndex = device->GetInstance()->GetQueueFamilyIndices()[QueueFlags::Compute];
    computePoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(logicalDevice, &computePoolInfo, nullptr, &computeCommandPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool");
    }
}

void Renderer::CreateRenderPass() {
    // Color buffer attachment represented by one of the images from the swap chain
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = swapChain->GetVkImageFormat();
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // Create a color attachment reference to be used with subpass
    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Depth buffer attachment
    VkFormat depthFormat = device->GetInstance()->GetSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format = depthFormat;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Create a depth attachment reference
    VkAttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Create subpass description
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

    // Specify subpass dependency
    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    // Create render pass
    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(logicalDevice, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create render pass");
    }
}

void Renderer::CreateShadowMapRenderPass() {
	VkAttachmentDescription attachmentDescription = {};

	// Depth attachment (shadow map)
	attachmentDescription.format = DEPTH_FORMAT;
	attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

	// Attachment references from subpasses
	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 0;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	//subpass.inputAttachmentCount = 0;
	//subpass.pInputAttachments = NULL;
	subpass.colorAttachmentCount = 0;
	//subpass.pColorAttachments = NULL;
	//subpass.pResolveAttachments = NULL;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;
	//subpass.preserveAttachmentCount = 0;
	//subpass.pPreserveAttachments = NULL;

	std::array<VkSubpassDependency, 2> dependencies;
	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	// Create render pass
	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.pNext = NULL;
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments = &attachmentDescription;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpass;
	renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
	renderPassCreateInfo.pDependencies = dependencies.data();

	if (vkCreateRenderPass(logicalDevice, &renderPassCreateInfo, NULL, &shadowMapRenderPass) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create render pass");
	}
}


void Renderer::CreateOpacityMapRenderPass() {
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = swapChain->GetVkImageFormat();
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL ???

	// Create a color attachment reference to be used with subpass
	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	//subpass.inputAttachmentCount = 0;
	//subpass.pInputAttachments = NULL;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	//subpass.pResolveAttachments = NULL;
	//subpass.pDepthStencilAttachment = &colorAttachmentRef;
	//subpass.preserveAttachmentCount = 0;
	//subpass.pPreserveAttachments = NULL;

	std::array<VkSubpassDependency, 2> dependencies;
	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	// Create render pass
	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.pNext = NULL;
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments = &colorAttachment;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpass;
	renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
	renderPassCreateInfo.pDependencies = dependencies.data();

	if (vkCreateRenderPass(logicalDevice, &renderPassCreateInfo, NULL, &opacityMapRenderPass) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create render pass");
	}
}

void Renderer::CreateCameraDescriptorSetLayout() {
    // Describe the binding of the descriptor set layout
    VkDescriptorSetLayoutBinding uboLayoutBinding = {};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_ALL;
    uboLayoutBinding.pImmutableSamplers = nullptr;

    std::vector<VkDescriptorSetLayoutBinding> bindings = { uboLayoutBinding };

    // Create the descriptor set layout
    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(logicalDevice, &layoutInfo, nullptr, &cameraDescriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor set layout");
    }
}

void Renderer::CreateModelMatrixDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding uboLayoutBinding = {};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr;

    std::vector<VkDescriptorSetLayoutBinding> bindings = { uboLayoutBinding };

    // Create the descriptor set layout
    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(logicalDevice, &layoutInfo, nullptr, &modelMatrixDescriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor set layout");
    }
}

void Renderer::CreateTextureDescriptorSetLayout() {
	VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
	samplerLayoutBinding.binding = 0;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerLayoutBinding.pImmutableSamplers = nullptr;

	std::vector<VkDescriptorSetLayoutBinding> bindings = { samplerLayoutBinding };

	// Create the descriptor set layout
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(logicalDevice, &layoutInfo, nullptr, &textureDescriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor set layout");
	}

}

void Renderer::CreateHairDescriptorSetLayout() {
	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerLayoutBinding.pImmutableSamplers = nullptr;

	std::vector<VkDescriptorSetLayoutBinding> bindings = { uboLayoutBinding, samplerLayoutBinding };

	// Create the descriptor set layout
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(logicalDevice, &layoutInfo, nullptr, &hairDescriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor set layout");
	}
}

void Renderer::CreateOpacityMapDescriptorSetLayout() {
	VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
	samplerLayoutBinding.binding = 0;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerLayoutBinding.pImmutableSamplers = nullptr;

	std::vector<VkDescriptorSetLayoutBinding> bindings = { samplerLayoutBinding };

	// Create the descriptor set layout
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(logicalDevice, &layoutInfo, nullptr, &opacityMapDescriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor set layout");
	}
}

void Renderer::CreateTimeDescriptorSetLayout() {
    // Describe the binding of the descriptor set layout
    VkDescriptorSetLayoutBinding uboLayoutBinding = {};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr;

    std::vector<VkDescriptorSetLayoutBinding> bindings = { uboLayoutBinding };

    // Create the descriptor set layout
    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(logicalDevice, &layoutInfo, nullptr, &timeDescriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor set layout");
    }
}

void Renderer::CreateCollidersDescriptorSetLayout() {
	// Describe the binding of the descriptor set layout
	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr;

	std::vector<VkDescriptorSetLayoutBinding> bindings = { uboLayoutBinding };

	// Create the descriptor set layout
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(logicalDevice, &layoutInfo, nullptr, &collidersDescriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor set layout");
	}
}

void Renderer::CreateGridDescriptorSetLayout() {
	VkDescriptorSetLayoutBinding gridValuesLayoutBinding = {};
	gridValuesLayoutBinding.binding = 0;										// binding value in compute shader
	gridValuesLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	gridValuesLayoutBinding.descriptorCount = 1;
	gridValuesLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	gridValuesLayoutBinding.pImmutableSamplers = nullptr;

	std::vector<VkDescriptorSetLayoutBinding> bindings = { gridValuesLayoutBinding };

	// Create the descriptor set layout
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(logicalDevice, &layoutInfo, nullptr, &gridDescriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor set layout");
	}
}

void Renderer::CreateComputeDescriptorSetLayout() {
    // NOTE: Remember this is like a class definition stating what types of information
    // will be stored at each binding

	VkDescriptorSetLayoutBinding strandsPosLayoutBinding = {};
	strandsPosLayoutBinding.binding = 0;										// binding value in compute shader
	strandsPosLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	strandsPosLayoutBinding.descriptorCount = 1;
	strandsPosLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	strandsPosLayoutBinding.pImmutableSamplers = nullptr;

	// TODO: Add more bindings if needed (ex. culled strands, num strands)
	VkDescriptorSetLayoutBinding numStrandsLayoutBinding = {};
	numStrandsLayoutBinding.binding = 1;
	numStrandsLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	numStrandsLayoutBinding.descriptorCount = 1;
	numStrandsLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	numStrandsLayoutBinding.pImmutableSamplers = nullptr;

	std::vector<VkDescriptorSetLayoutBinding> bindings = { strandsPosLayoutBinding, numStrandsLayoutBinding };

	// Create the descriptor set layout
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(logicalDevice, &layoutInfo, nullptr, &computeDescriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor set layout");
	}
}

void Renderer::CreateDescriptorPool() {
    // Describe which descriptor types that the descriptor sets will contain
    std::vector<VkDescriptorPoolSize> poolSizes = {
        // Camera
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER , 2},

        // Models + Strands
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , static_cast<uint32_t>(1 + 2 * scene->GetModels().size() + scene->GetHair().size()) },

        // Models + Strands
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER , static_cast<uint32_t>(2 * scene->GetModels().size() + scene->GetHair().size()) },

        // Time (compute)
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER , 1 },

		// Hair (compute)
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER , static_cast<uint32_t>(scene->GetHair().size()) },

		// Collision objects (compute)
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER , 1 },

		// Grid (compute)
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER , 2 },

		// Model Matrices dynamic
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1 },
    };

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = 12; // TODO: idk what determines this number

    if (vkCreateDescriptorPool(logicalDevice, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool");
    }
}

void Renderer::CreateCameraDescriptorSet() {
    // Describe the desciptor set
    VkDescriptorSetLayout layouts[] = { cameraDescriptorSetLayout };
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = layouts;

    // Allocate descriptor sets
    if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, &cameraDescriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor set");
    }

    // Configure the descriptors to refer to buffers
    VkDescriptorBufferInfo cameraBufferInfo = {};
    cameraBufferInfo.buffer = camera->GetBuffer();
    cameraBufferInfo.offset = 0;
    cameraBufferInfo.range = sizeof(CameraBufferObject);

    std::array<VkWriteDescriptorSet, 1> descriptorWrites = {};
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = cameraDescriptorSet;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &cameraBufferInfo;
    descriptorWrites[0].pImageInfo = nullptr;
    descriptorWrites[0].pTexelBufferView = nullptr;

    // Update descriptor sets
    vkUpdateDescriptorSets(logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void Renderer::CreateShadowCameraDescriptorSet() {
	// Describe the desciptor set
	VkDescriptorSetLayout layouts[] = { cameraDescriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	// Allocate descriptor sets
	if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, &shadowCameraDescriptorSet) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate descriptor set");
	}

	// Configure the descriptors to refer to buffers
	VkDescriptorBufferInfo cameraBufferInfo = {};
	cameraBufferInfo.buffer = shadowCamera->GetBuffer();
	cameraBufferInfo.offset = 0;
	cameraBufferInfo.range = sizeof(CameraBufferObject);

	std::array<VkWriteDescriptorSet, 1> descriptorWrites = {};
	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = shadowCameraDescriptorSet;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pBufferInfo = &cameraBufferInfo;
	descriptorWrites[0].pImageInfo = nullptr;
	descriptorWrites[0].pTexelBufferView = nullptr;

	// Update descriptor sets
	vkUpdateDescriptorSets(logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void Renderer::CreateModelDescriptorSet() {
    // Describe the desciptor set
	//std::vector<VkDescriptorSetLayout> layouts(scene->GetModels().size(), modelDescriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(1);
	allocInfo.pSetLayouts = &modelMatrixDescriptorSetLayout;

    // Allocate descriptor sets
    if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, &modelDescriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor set");
    }

    std::vector<VkWriteDescriptorSet> descriptorWrites(1);

	descriptorWrites[0];

    VkDescriptorBufferInfo modelBufferInfo = {};
    modelBufferInfo.buffer = scene->GetModelBuffer();
    modelBufferInfo.offset = 0;
    modelBufferInfo.range = sizeof(ModelBufferObject);

    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = modelDescriptorSet;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &modelBufferInfo;
    descriptorWrites[0].pImageInfo = nullptr;
    descriptorWrites[0].pTexelBufferView = nullptr;

    // Update descriptor sets
    vkUpdateDescriptorSets(logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}



void Renderer::CreateTextureDescriptorSets() {
	textureDescriptorSets.resize(scene->GetModels().size());

	// Describe the desciptor set
	std::vector<VkDescriptorSetLayout> layouts(scene->GetModels().size(), textureDescriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(textureDescriptorSets.size());
	allocInfo.pSetLayouts = layouts.data();

	// Allocate descriptor sets
	if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, textureDescriptorSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate descriptor set");
	}

	std::vector<VkWriteDescriptorSet> descriptorWrites(textureDescriptorSets.size());

	for (uint32_t i = 0; i < scene->GetModels().size(); ++i) {
		// Bind image and sampler resources to the descriptor
		VkDescriptorImageInfo imageInfo = {};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = scene->GetModels()[i]->GetTextureView();
		imageInfo.sampler = scene->GetModels()[i]->GetTextureSampler();

		descriptorWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[i].dstSet = textureDescriptorSets[i];
		descriptorWrites[i].dstBinding = 0;
		descriptorWrites[i].dstArrayElement = 0;
		descriptorWrites[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[i].descriptorCount = 1;
		descriptorWrites[i].pImageInfo = &imageInfo;
	}

	// Update descriptor sets
	vkUpdateDescriptorSets(logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}


void Renderer::CreateHairDescriptorSets() {
	hairDescriptorSets.resize(scene->GetHair().size());

	// Describe the desciptor set
	VkDescriptorSetLayout layouts[] = { hairDescriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(hairDescriptorSets.size());
	allocInfo.pSetLayouts = layouts;

	// Allocate descriptor sets
	if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, hairDescriptorSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate descriptor set");
	}

	std::vector<VkWriteDescriptorSet> descriptorWrites(2 * hairDescriptorSets.size());

	for (uint32_t i = 0; i < scene->GetHair().size(); ++i) {
		VkDescriptorBufferInfo hairBufferInfo = {};
		hairBufferInfo.buffer = scene->GetHair()[i]->GetModelBuffer();
		hairBufferInfo.offset = 0;
		hairBufferInfo.range = sizeof(ModelBufferObject);

		// Bind image and sampler resources to the descriptor
		VkDescriptorImageInfo imageInfo = {};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		imageInfo.imageView = shadowMapImageView;
		imageInfo.sampler = shadowMapSampler;

		descriptorWrites[2 * i + 0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[2 * i + 0].dstSet = hairDescriptorSets[i];
		descriptorWrites[2 * i + 0].dstBinding = 0;
		descriptorWrites[2 * i + 0].dstArrayElement = 0;
		descriptorWrites[2 * i + 0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[2 * i + 0].descriptorCount = 1;
		descriptorWrites[2 * i + 0].pBufferInfo = &hairBufferInfo;
		descriptorWrites[2 * i + 0].pImageInfo = nullptr;
		descriptorWrites[2 * i + 0].pTexelBufferView = nullptr;

		descriptorWrites[2 * i + 1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[2 * i + 1].dstSet = hairDescriptorSets[i];
		descriptorWrites[2 * i + 1].dstBinding = 1;
		descriptorWrites[2 * i + 1].dstArrayElement = 0;
		descriptorWrites[2 * i + 1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[2 * i + 1].descriptorCount = 1;
		descriptorWrites[2 * i + 1].pImageInfo = &imageInfo;
	}

	// Update descriptor sets
	vkUpdateDescriptorSets(logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void Renderer::CreateOpacityMapDescriptorSets() {
	opacityMapDescriptorSets.resize(scene->GetHair().size());

	// Describe the desciptor set
	VkDescriptorSetLayout layouts[] = { opacityMapDescriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(opacityMapDescriptorSets.size());
	allocInfo.pSetLayouts = layouts;

	// Allocate descriptor sets
	if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, opacityMapDescriptorSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate descriptor set");
	}

	std::vector<VkWriteDescriptorSet> descriptorWrites(opacityMapDescriptorSets.size());

	for (uint32_t i = 0; i < scene->GetHair().size(); ++i) {
		// Bind image and sampler resources to the descriptor
		VkDescriptorImageInfo imageInfo = {};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		imageInfo.imageView = opacityMapImageView;
		imageInfo.sampler = opacityMapSampler;

		descriptorWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[i].dstSet = opacityMapDescriptorSets[i];
		descriptorWrites[i].dstBinding = 0;
		descriptorWrites[i].dstArrayElement = 0;
		descriptorWrites[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[i].descriptorCount = 1;
		descriptorWrites[i].pImageInfo = &imageInfo;
	}

	// Update descriptor sets
	vkUpdateDescriptorSets(logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void Renderer::CreateOpacityMapHairDescriptorSets() {
	// TODO: Create Descriptor sets for the hair.
	// This should involve creating descriptor sets which point to the model matrix of each group of hair
	opacityMapHairDescriptorSets.resize(scene->GetHair().size());

	// Describe the desciptor set
	VkDescriptorSetLayout layouts[] = { hairDescriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(opacityMapHairDescriptorSets.size());
	allocInfo.pSetLayouts = layouts;

	// Allocate descriptor sets
	if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, opacityMapHairDescriptorSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate descriptor set");
	}

	std::vector<VkWriteDescriptorSet> descriptorWrites(2 * opacityMapHairDescriptorSets.size());

	for (uint32_t i = 0; i < scene->GetHair().size(); ++i) {
		VkDescriptorBufferInfo hairBufferInfo = {};
		hairBufferInfo.buffer = scene->GetHair()[i]->GetModelBuffer();
		hairBufferInfo.offset = 0;
		hairBufferInfo.range = sizeof(ModelBufferObject);

		// Bind image and sampler resources to the descriptor
		VkDescriptorImageInfo imageInfo = {};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		imageInfo.imageView = shadowMapImageView;
		imageInfo.sampler = shadowMapSampler;

		descriptorWrites[2 * i + 0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[2 * i + 0].dstSet = opacityMapHairDescriptorSets[i];
		descriptorWrites[2 * i + 0].dstBinding = 0;
		descriptorWrites[2 * i + 0].dstArrayElement = 0;
		descriptorWrites[2 * i + 0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[2 * i + 0].descriptorCount = 1;
		descriptorWrites[2 * i + 0].pBufferInfo = &hairBufferInfo;
		descriptorWrites[2 * i + 0].pImageInfo = nullptr;
		descriptorWrites[2 * i + 0].pTexelBufferView = nullptr;

		descriptorWrites[2 * i + 1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[2 * i + 1].dstSet = opacityMapHairDescriptorSets[i];
		descriptorWrites[2 * i + 1].dstBinding = 1;
		descriptorWrites[2 * i + 1].dstArrayElement = 0;
		descriptorWrites[2 * i + 1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[2 * i + 1].descriptorCount = 1;
		descriptorWrites[2 * i + 1].pImageInfo = &imageInfo;
	}

	// Update descriptor sets
	vkUpdateDescriptorSets(logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void Renderer::CreateTimeDescriptorSet() {
    // Describe the desciptor set
    VkDescriptorSetLayout layouts[] = { timeDescriptorSetLayout };
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = layouts;

    // Allocate descriptor sets
    if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, &timeDescriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor set");
    }

    // Configure the descriptors to refer to buffers
    VkDescriptorBufferInfo timeBufferInfo = {};
    timeBufferInfo.buffer = scene->GetTimeBuffer();
    timeBufferInfo.offset = 0;
    timeBufferInfo.range = sizeof(Time);

    std::array<VkWriteDescriptorSet, 1> descriptorWrites = {};
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = timeDescriptorSet;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &timeBufferInfo;
    descriptorWrites[0].pImageInfo = nullptr;
    descriptorWrites[0].pTexelBufferView = nullptr;

    // Update descriptor sets
    vkUpdateDescriptorSets(logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void Renderer::CreateCollidersDescriptorSets() {
	// Describe the desciptor set
	VkDescriptorSetLayout layouts[] = { collidersDescriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	// Allocate descriptor sets
	if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, &collidersDescriptorSets) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate descriptor set");
	}

	// Configure the descriptors to refer to buffers
	VkDescriptorBufferInfo colliderBufferInfo = {};
	colliderBufferInfo.buffer = scene->GetCollidersBuffer();
	colliderBufferInfo.offset = 0;
	colliderBufferInfo.range = scene->GetColliders().size() * sizeof(Collider);

	std::array<VkWriteDescriptorSet, 1> descriptorWrites = {};
	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = collidersDescriptorSets;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pBufferInfo = &colliderBufferInfo;
	descriptorWrites[0].pImageInfo = nullptr;
	descriptorWrites[0].pTexelBufferView = nullptr;

	// Update descriptor sets
	vkUpdateDescriptorSets(logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void Renderer::CreateGridDescriptorSets() {
	// Describe the desciptor set
	VkDescriptorSetLayout layouts[] = { gridDescriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	// Allocate descriptor sets
	if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, &gridDescriptorSets) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate descriptor set");
	}

	// Configure the descriptors to refer to buffers
	VkDescriptorBufferInfo gridBufferInfo = {};
	gridBufferInfo.buffer = scene->GetGridBuffer();
	gridBufferInfo.offset = 0;
	gridBufferInfo.range = scene->GetGrid().size() * sizeof(GridCell);

	std::array<VkWriteDescriptorSet, 1> descriptorWrites = {};
	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = gridDescriptorSets;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pBufferInfo = &gridBufferInfo;
	descriptorWrites[0].pImageInfo = nullptr;
	descriptorWrites[0].pTexelBufferView = nullptr;

	// Update descriptor sets
	vkUpdateDescriptorSets(logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void Renderer::CreateComputeDescriptorSets() {   
	computeDescriptorSets.resize(scene->GetHair().size());

	// Describe the desciptor set
	VkDescriptorSetLayout layouts[] = { computeDescriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(computeDescriptorSets.size());
	allocInfo.pSetLayouts = layouts;

	// Allocate descriptor sets
	if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, computeDescriptorSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate descriptor set");
	}

	int numBuffers = 2; // TODO: set to number of buffers to compute (input, num)
	std::vector<VkWriteDescriptorSet> descriptorWrites(numBuffers * computeDescriptorSets.size()); 

	for (uint32_t i = 0; i < scene->GetHair().size(); ++i) {
		VkDescriptorBufferInfo hairBufferInfo = {};
		hairBufferInfo.buffer = scene->GetHair()[i]->GetStrandsBuffer();
		hairBufferInfo.offset = 0;
		hairBufferInfo.range = scene->GetHair()[i]->GetNumStrands() * sizeof(Strand);

		VkDescriptorBufferInfo numStrandsBufferInfo = {};
		numStrandsBufferInfo.buffer = scene->GetHair()[i]->GetNumStrandsBuffer();
		numStrandsBufferInfo.offset = 0;
		numStrandsBufferInfo.range = sizeof(StrandDrawIndirect);

		descriptorWrites[numBuffers * i + 0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[numBuffers * i + 0].dstSet = computeDescriptorSets[i];
		descriptorWrites[numBuffers * i + 0].dstBinding = 0;
		descriptorWrites[numBuffers * i + 0].dstArrayElement = 0;
		descriptorWrites[numBuffers * i + 0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[numBuffers * i + 0].descriptorCount = 1;
		descriptorWrites[numBuffers * i + 0].pBufferInfo = &hairBufferInfo;
		descriptorWrites[numBuffers * i + 0].pImageInfo = nullptr;
		descriptorWrites[numBuffers * i + 0].pTexelBufferView = nullptr;

		descriptorWrites[numBuffers * i + 1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[numBuffers * i + 1].dstSet = computeDescriptorSets[i];
		descriptorWrites[numBuffers * i + 1].dstBinding = 1;
		descriptorWrites[numBuffers * i + 1].dstArrayElement = 0;
		descriptorWrites[numBuffers * i + 1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[numBuffers * i + 1].descriptorCount = 1;
		descriptorWrites[numBuffers * i + 1].pBufferInfo = &numStrandsBufferInfo;
		descriptorWrites[numBuffers * i + 1].pImageInfo = nullptr;
		descriptorWrites[numBuffers * i + 1].pTexelBufferView = nullptr;
	}

	// Update descriptor sets
	vkUpdateDescriptorSets(logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void Renderer::CreateGraphicsPipeline() {
    VkShaderModule vertShaderModule = ShaderModule::Create("shaders/graphics.vert.spv", logicalDevice);
    VkShaderModule fragShaderModule = ShaderModule::Create("shaders/graphics.frag.spv", logicalDevice);

    // Assign each shader module to the appropriate stage in the pipeline
    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    // --- Set up fixed-function stages ---

    // Vertex input
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // Viewports and Scissors (rectangles that define in which regions pixels are stored)
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapChain->GetVkExtent().width);
    viewport.height = static_cast<float>(swapChain->GetVkExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = { 0, 0 };
    scissor.extent = swapChain->GetVkExtent();

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;

    // Multisampling (turned off here)
    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    // Depth testing
    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f;
    depthStencil.maxDepthBounds = 1.0f;
    depthStencil.stencilTestEnable = VK_FALSE;

    // Color blending (turned off here, but showing options for learning)
    // --> Configuration per attached framebuffer
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    // --> Global color blending settings
    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts = { cameraDescriptorSetLayout, textureDescriptorSetLayout, modelMatrixDescriptorSetLayout };

    // Pipeline layout: used to specify uniform values
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = 0;

    if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, nullptr, &graphicsPipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout");
    }

    // --- Create graphics pipeline ---
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = nullptr;
    pipelineInfo.layout = graphicsPipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    if (vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create graphics pipeline");
    }

    vkDestroyShaderModule(logicalDevice, vertShaderModule, nullptr);
    vkDestroyShaderModule(logicalDevice, fragShaderModule, nullptr);
}

void Renderer::CreateShadowMapPipeline() {
	// --- Set up programmable shaders ---
	VkShaderModule vertShaderModule = ShaderModule::Create("shaders/hair.vert.spv", logicalDevice);
	VkShaderModule tescShaderModule = ShaderModule::Create("shaders/hair.tesc.spv", logicalDevice);
	VkShaderModule teseShaderModule = ShaderModule::Create("shaders/hair.tese.spv", logicalDevice);
	VkShaderModule geomShaderModule = ShaderModule::Create("shaders/hair.geom.spv", logicalDevice);

	// Assign each shader module to the appropriate stage in the pipeline
	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo tescShaderStageInfo = {};
	tescShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	tescShaderStageInfo.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
	tescShaderStageInfo.module = tescShaderModule;
	tescShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo teseShaderStageInfo = {};
	teseShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	teseShaderStageInfo.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
	teseShaderStageInfo.module = teseShaderModule;
	teseShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo geomShaderStageInfo = {};
	geomShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	geomShaderStageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
	geomShaderStageInfo.module = geomShaderModule;
	geomShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, tescShaderStageInfo, teseShaderStageInfo, geomShaderStageInfo };

	// --- Set up fixed-function stages ---

	// Vertex input
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	auto bindingDescription = Strand::getBindingDescription();
	auto attributeDescriptions = Strand::getAttributeDescriptions();

	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	// Input Assembly
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	// Viewports and Scissors (rectangles that define in which regions pixels are stored)
	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(swapChain->GetVkExtent().width);
	viewport.height = static_cast<float>(swapChain->GetVkExtent().height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = swapChain->GetVkExtent();

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	// Rasterizer
	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_NONE;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_TRUE;
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasSlopeFactor = 0.0f;

	// Multisampling (turned off here)
	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;

	// Depth testing
	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f;
	depthStencil.maxDepthBounds = 1.0f;
	depthStencil.stencilTestEnable = VK_FALSE;

	// Color blending (turned off here, but showing options for learning)
	// --> Configuration per attached framebuffer
	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	// --> Global color blending settings
	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 0; // no color attachments for shadow mapping
	colorBlending.pAttachments = nullptr;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	// not sure we need this???
	std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_DEPTH_BIAS };
	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = dynamicStateEnables.size();
	dynamicState.pDynamicStates = dynamicStateEnables.data();
	dynamicState.pNext = 0;
	dynamicState.flags = 0;

	std::vector<VkDescriptorSetLayout> descriptorSetLayouts = { cameraDescriptorSetLayout, hairDescriptorSetLayout, cameraDescriptorSetLayout };

	// Pipeline layout: used to specify uniform values
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = 0;

	if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, nullptr, &shadowMapPipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create pipeline layout");
	}

	// Tessellation state
	VkPipelineTessellationStateCreateInfo tessellationInfo = {};
	tessellationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
	tessellationInfo.pNext = NULL;
	tessellationInfo.flags = 0;
	tessellationInfo.patchControlPoints = 1;

	// --- Create graphics pipeline ---
	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 4;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pTessellationState = &tessellationInfo;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = shadowMapPipelineLayout;
	pipelineInfo.renderPass = shadowMapRenderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	if (vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &shadowMapPipeline) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create shadow map pipeline");
	}

	vkDestroyShaderModule(logicalDevice, vertShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice, tescShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice, teseShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice, geomShaderModule, nullptr);
}

void Renderer::CreateOpacityMapPipeline() {
	// --- Set up programmable shaders ---
	VkShaderModule vertShaderModule = ShaderModule::Create("shaders/hair.vert.spv", logicalDevice);
	VkShaderModule tescShaderModule = ShaderModule::Create("shaders/hair.tesc.spv", logicalDevice);
	VkShaderModule teseShaderModule = ShaderModule::Create("shaders/hair.tese.spv", logicalDevice);
	VkShaderModule geomShaderModule = ShaderModule::Create("shaders/hair.geom.spv", logicalDevice);
	VkShaderModule fragShaderModule = ShaderModule::Create("shaders/hairOpacity.frag.spv", logicalDevice);

	// Assign each shader module to the appropriate stage in the pipeline
	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo tescShaderStageInfo = {};
	tescShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	tescShaderStageInfo.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
	tescShaderStageInfo.module = tescShaderModule;
	tescShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo teseShaderStageInfo = {};
	teseShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	teseShaderStageInfo.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
	teseShaderStageInfo.module = teseShaderModule;
	teseShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo geomShaderStageInfo = {};
	geomShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	geomShaderStageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
	geomShaderStageInfo.module = geomShaderModule;
	geomShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, tescShaderStageInfo, teseShaderStageInfo, geomShaderStageInfo, fragShaderStageInfo };

	// --- Set up fixed-function stages ---

	// Vertex input
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	auto bindingDescription = Strand::getBindingDescription();
	auto attributeDescriptions = Strand::getAttributeDescriptions();

	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	// Input Assembly
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	// Viewports and Scissors (rectangles that define in which regions pixels are stored)
	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(swapChain->GetVkExtent().width);
	viewport.height = static_cast<float>(swapChain->GetVkExtent().height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = swapChain->GetVkExtent();

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	// Rasterizer
	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_NONE;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_TRUE;
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasSlopeFactor = 0.0f;

	// Multisampling (turned off here)
	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;

	// Depth testing
	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f;
	depthStencil.maxDepthBounds = 1.0f;
	depthStencil.stencilTestEnable = VK_FALSE;

	// TODO turn color blending on?
	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	// --> Global color blending settings
	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_DEPTH_BIAS };
	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = dynamicStateEnables.size();
	dynamicState.pDynamicStates = dynamicStateEnables.data();
	dynamicState.pNext = 0;
	dynamicState.flags = 0;

	std::vector<VkDescriptorSetLayout> descriptorSetLayouts = { cameraDescriptorSetLayout, hairDescriptorSetLayout, cameraDescriptorSetLayout };

	// Pipeline layout: used to specify uniform values
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = 0;

	if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, nullptr, &opacityMapPipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create pipeline layout");
	}

	// Tessellation state
	VkPipelineTessellationStateCreateInfo tessellationInfo = {};
	tessellationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
	tessellationInfo.pNext = NULL;
	tessellationInfo.flags = 0;
	tessellationInfo.patchControlPoints = 1;

	// --- Create graphics pipeline ---
	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 5;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pTessellationState = &tessellationInfo;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = opacityMapPipelineLayout;
	pipelineInfo.renderPass = opacityMapRenderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	if (vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &opacityMapPipeline) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create shadow map pipeline");
	}

	vkDestroyShaderModule(logicalDevice, vertShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice, tescShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice, teseShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice, geomShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice, fragShaderModule, nullptr);
}

void Renderer::CreateHairPipeline() {
    // --- Set up programmable shaders ---
    VkShaderModule vertShaderModule = ShaderModule::Create("shaders/hair.vert.spv", logicalDevice);
    VkShaderModule tescShaderModule = ShaderModule::Create("shaders/hair.tesc.spv", logicalDevice);
    VkShaderModule teseShaderModule = ShaderModule::Create("shaders/hair.tese.spv", logicalDevice);
    VkShaderModule geomShaderModule = ShaderModule::Create("shaders/hair.geom.spv", logicalDevice);
    VkShaderModule fragShaderModule = ShaderModule::Create("shaders/hair.frag.spv", logicalDevice);

    // Assign each shader module to the appropriate stage in the pipeline
    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo tescShaderStageInfo = {};
    tescShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    tescShaderStageInfo.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    tescShaderStageInfo.module = tescShaderModule;
    tescShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo teseShaderStageInfo = {};
    teseShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    teseShaderStageInfo.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    teseShaderStageInfo.module = teseShaderModule;
    teseShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo geomShaderStageInfo = {};
	geomShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	geomShaderStageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
	geomShaderStageInfo.module = geomShaderModule;
	geomShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, tescShaderStageInfo, teseShaderStageInfo, geomShaderStageInfo, fragShaderStageInfo };

    // --- Set up fixed-function stages ---

    // Vertex input
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    auto bindingDescription = Strand::getBindingDescription();
    auto attributeDescriptions = Strand::getAttributeDescriptions();

    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    // Input Assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // Viewports and Scissors (rectangles that define in which regions pixels are stored)
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapChain->GetVkExtent().width);
    viewport.height = static_cast<float>(swapChain->GetVkExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = { 0, 0 };
    scissor.extent = swapChain->GetVkExtent();

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;
	
    // Multisampling (turned off here)
    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    // Depth testing
    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f;
    depthStencil.maxDepthBounds = 1.0f;
    depthStencil.stencilTestEnable = VK_FALSE;

    // Color blending (turned off here, but showing options for learning)
    // --> Configuration per attached framebuffer
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    // --> Global color blending settings
    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts = { cameraDescriptorSetLayout, hairDescriptorSetLayout, cameraDescriptorSetLayout, opacityMapDescriptorSetLayout };

    // Pipeline layout: used to specify uniform values
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = 0;

    if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, nullptr, &hairPipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout");
    }

    // Tessellation state
    VkPipelineTessellationStateCreateInfo tessellationInfo = {};
    tessellationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
    tessellationInfo.pNext = NULL;
    tessellationInfo.flags = 0;
	tessellationInfo.patchControlPoints = 1;

    // --- Create graphics pipeline ---
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 5;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pTessellationState = &tessellationInfo;
    pipelineInfo.pDynamicState = nullptr;
    pipelineInfo.layout = hairPipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    if (vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &hairPipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create graphics pipeline");
    }

    // No need for the shader modules anymore
    vkDestroyShaderModule(logicalDevice, vertShaderModule, nullptr);
    vkDestroyShaderModule(logicalDevice, tescShaderModule, nullptr);
    vkDestroyShaderModule(logicalDevice, teseShaderModule, nullptr);
    vkDestroyShaderModule(logicalDevice, geomShaderModule, nullptr);
    vkDestroyShaderModule(logicalDevice, fragShaderModule, nullptr);
}

void Renderer::CreateComputePipeline() {
    // Set up programmable shaders
    VkShaderModule computeShaderModule = ShaderModule::Create("shaders/compute.comp.spv", logicalDevice);

    VkPipelineShaderStageCreateInfo computeShaderStageInfo = {};
    computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    computeShaderStageInfo.module = computeShaderModule;
    computeShaderStageInfo.pName = "main";

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts = { cameraDescriptorSetLayout, timeDescriptorSetLayout, collidersDescriptorSetLayout, gridDescriptorSetLayout, computeDescriptorSetLayout };

    // Create pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = 0;

    if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, nullptr, &computePipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout");
    }

    // Create compute pipeline
    VkComputePipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage = computeShaderStageInfo;
    pipelineInfo.layout = computePipelineLayout;
    pipelineInfo.pNext = nullptr;
    pipelineInfo.flags = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    if (vkCreateComputePipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &computePipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create compute pipeline");
    }

    // No need for shader modules anymore
    vkDestroyShaderModule(logicalDevice, computeShaderModule, nullptr);
}

void Renderer::CreateFrameResources() {
    imageViews.resize(swapChain->GetCount());

    for (uint32_t i = 0; i < swapChain->GetCount(); i++) {
        // --- Create an image view for each swap chain image ---
        VkImageViewCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapChain->GetVkImage(i);

        // Specify how the image data should be interpreted
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swapChain->GetVkImageFormat();

        // Specify color channel mappings (can be used for swizzling)
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        // Describe the image's purpose and which part of the image should be accessed
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        // Create the image view
        if (vkCreateImageView(logicalDevice, &createInfo, nullptr, &imageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create image views");
        }
    }

    VkFormat depthFormat = device->GetInstance()->GetSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    // CREATE DEPTH IMAGE
    Image::Create(device,
        swapChain->GetVkExtent().width,
        swapChain->GetVkExtent().height,
        depthFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        depthImage,
        depthImageMemory
    );

    depthImageView = Image::CreateView(device, depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
    
    // Transition the image for use as depth-stencil
    Image::TransitionLayout(device, graphicsCommandPool, depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    
    // CREATE FRAMEBUFFERS
    framebuffers.resize(swapChain->GetCount());
    for (size_t i = 0; i < swapChain->GetCount(); i++) {
        std::vector<VkImageView> attachments = {
            imageViews[i],
            depthImageView
        };

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapChain->GetVkExtent().width;
        framebufferInfo.height = swapChain->GetVkExtent().height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(logicalDevice, &framebufferInfo, nullptr, &framebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create framebuffer");
        }

    }
}

void Renderer::DestroyFrameResources() {
    for (size_t i = 0; i < imageViews.size(); i++) {
        vkDestroyImageView(logicalDevice, imageViews[i], nullptr);
    }

    vkDestroyImageView(logicalDevice, depthImageView, nullptr);
    vkFreeMemory(logicalDevice, depthImageMemory, nullptr);
    vkDestroyImage(logicalDevice, depthImage, nullptr);

    for (size_t i = 0; i < framebuffers.size(); i++) {
        vkDestroyFramebuffer(logicalDevice, framebuffers[i], nullptr);
    }
}

void Renderer::CreateShadowMapFrameResources() {
	Image::Create(device,
		SHADOWMAP_WIDTH,
		SHADOWMAP_HEIGHT,
		DEPTH_FORMAT,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		shadowMapImage,
		shadowMapImageMemory
	);

	shadowMapImageView = Image::CreateView(device, shadowMapImage, DEPTH_FORMAT, VK_IMAGE_ASPECT_DEPTH_BIT);

	// create sampler to sample from
	VkSamplerCreateInfo sampler = {};
	sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler.magFilter = SHADOWMAP_FILTER;
	sampler.minFilter = SHADOWMAP_FILTER;
	sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler.mipLodBias = 0.f;
	sampler.maxAnisotropy = 1.f;
	sampler.minLod = 0.f;
	sampler.maxLod = 1.f;
	sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

	if (vkCreateSampler(logicalDevice, &sampler, nullptr, &shadowMapSampler) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create sampler");
	}

	// create framebuffer
	VkFramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.pNext = NULL;
	framebufferInfo.renderPass = shadowMapRenderPass;
	framebufferInfo.attachmentCount = 1;
	framebufferInfo.pAttachments = &shadowMapImageView;
	framebufferInfo.width = SHADOWMAP_WIDTH;
	framebufferInfo.height = SHADOWMAP_HEIGHT;
	framebufferInfo.layers = 1;

	if (vkCreateFramebuffer(logicalDevice, &framebufferInfo, nullptr, &shadowMapFramebuffer) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create framebuffer");
	}
}

void Renderer::DestroyShadowMapFrameResources() {
	vkDestroyImageView(logicalDevice, shadowMapImageView, nullptr);
	vkFreeMemory(logicalDevice, shadowMapImageMemory, nullptr);
	vkDestroyImage(logicalDevice, shadowMapImage, nullptr);
	vkDestroyFramebuffer(logicalDevice, shadowMapFramebuffer, nullptr);
	vkDestroySampler(logicalDevice, shadowMapSampler, nullptr);
}

void Renderer::CreateOpacityMapFrameResources() {
	Image::Create(device,
		SHADOWMAP_WIDTH,
		SHADOWMAP_HEIGHT,
		swapChain->GetVkImageFormat(),
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		opacityMapImage,
		opacityMapImageMemory
	);

	opacityMapImageView = Image::CreateView(device, opacityMapImage, swapChain->GetVkImageFormat(), VK_IMAGE_ASPECT_COLOR_BIT);

	// create sampler to sample from
	VkSamplerCreateInfo sampler = {};
	sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler.magFilter = SHADOWMAP_FILTER;
	sampler.minFilter = SHADOWMAP_FILTER;
	sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler.mipLodBias = 0.f;
	sampler.maxAnisotropy = 1.f;
	sampler.minLod = 0.f;
	sampler.maxLod = 1.f;
	sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

	if (vkCreateSampler(logicalDevice, &sampler, nullptr, &opacityMapSampler) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create sampler");
	}

	// create framebuffer
	VkFramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.pNext = NULL;
	framebufferInfo.renderPass = opacityMapRenderPass;
	framebufferInfo.attachmentCount = 1;
	framebufferInfo.pAttachments = &opacityMapImageView;
	framebufferInfo.width = SHADOWMAP_WIDTH;
	framebufferInfo.height = SHADOWMAP_HEIGHT;
	framebufferInfo.layers = 1;

	if (vkCreateFramebuffer(logicalDevice, &framebufferInfo, nullptr, &opacityMapFramebuffer) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create framebuffer");
	}
}

void Renderer::DestroyOpacityMapFrameResources() {
	vkDestroyImageView(logicalDevice, opacityMapImageView, nullptr);
	vkFreeMemory(logicalDevice, opacityMapImageMemory, nullptr);
	vkDestroyImage(logicalDevice, opacityMapImage, nullptr);
	vkDestroyFramebuffer(logicalDevice, opacityMapFramebuffer, nullptr);
	vkDestroySampler(logicalDevice, opacityMapSampler, nullptr);
}

void Renderer::RecreateFrameResources() {
	vkDestroyPipeline(logicalDevice, shadowMapPipeline, nullptr);
	vkDestroyPipeline(logicalDevice, opacityMapPipeline, nullptr);
	vkDestroyPipeline(logicalDevice, graphicsPipeline, nullptr);
	vkDestroyPipeline(logicalDevice, hairPipeline, nullptr);
	vkDestroyPipelineLayout(logicalDevice, shadowMapPipelineLayout, nullptr);
	vkDestroyPipelineLayout(logicalDevice, opacityMapPipelineLayout, nullptr);
	vkDestroyPipelineLayout(logicalDevice, graphicsPipelineLayout, nullptr);
	vkDestroyPipelineLayout(logicalDevice, hairPipelineLayout, nullptr);
	vkFreeCommandBuffers(logicalDevice, graphicsCommandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

	DestroyFrameResources();
	DestroyShadowMapFrameResources();
	DestroyOpacityMapFrameResources();
	CreateFrameResources();
	CreateShadowMapFrameResources();
	CreateOpacityMapFrameResources();
	CreateShadowMapPipeline();
	CreateOpacityMapPipeline();
	CreateGraphicsPipeline();
	CreateHairPipeline();
	RecordCommandBuffers();
}


void Renderer::RecordComputeCommandBuffer() {
    // Specify the command pool and number of buffers to allocate
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = computeCommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(logicalDevice, &allocInfo, &computeCommandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffers");
    }

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    beginInfo.pInheritanceInfo = nullptr;

    // ~ Start recording ~
    if (vkBeginCommandBuffer(computeCommandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin recording compute command buffer");
    }

	/*std::vector<uint32_t> data = std::vector<uint32_t>();
	data.resize(scene->GetGrid().size() * 4, uint32_t(0));*/



    // Bind to the compute pipeline
    vkCmdBindPipeline(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);

    // Bind camera descriptor set
    vkCmdBindDescriptorSets(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 0, 1, &cameraDescriptorSet, 0, nullptr);

    // Bind descriptor set for time uniforms
    vkCmdBindDescriptorSets(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 1, 1, &timeDescriptorSet, 0, nullptr);

	// Bind descriptor set for collider uniforms
    vkCmdBindDescriptorSets(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 2, 1, &collidersDescriptorSets, 0, nullptr);

	// Bind descriptor set for grid uniforms
	vkCmdBindDescriptorSets(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 3, 1, &gridDescriptorSets, 0, nullptr);
	vkCmdFillBuffer(computeCommandBuffer, scene->GetGridBuffer(), 0, scene->GetGrid().size() * sizeof(GridCell), 0);


	// TODO: for each group of strands, bind its descriptor set and dispatch
	// Check these function inputs.  Uncomment dispatch when ready to run compute shader.
	for (int i = 0; i < scene->GetHair().size(); ++i) {
		vkCmdBindDescriptorSets(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 4 + i, 1, &computeDescriptorSets[i], 0, nullptr);
		vkCmdDispatch(computeCommandBuffer, (int)ceil((scene->GetHair()[i]->GetNumStrands() + WORKGROUP_SIZE - 1) / WORKGROUP_SIZE), 1, 1);
	}

    // ~ End recording ~
    if (vkEndCommandBuffer(computeCommandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to record compute command buffer");
    }
}

void Renderer::RecordCommandBuffers() {
    commandBuffers.resize(swapChain->GetCount());

    // Specify the command pool and number of buffers to allocate
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = graphicsCommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    if (vkAllocateCommandBuffers(logicalDevice, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffers");
    }

    // Start command buffer recording
    for (size_t i = 0; i < commandBuffers.size(); i++) {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        beginInfo.pInheritanceInfo = nullptr;

        // ~ Start recording ~
        if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("Failed to begin recording command buffer");
        }

		// First pass: generate shadow map by rendering the hair from the light's POV -----------------------
		VkClearValue shadowMapClearValues[1];
		shadowMapClearValues[0].depthStencil.depth = 1.0f;
		shadowMapClearValues[0].depthStencil.stencil = 0;

		VkRenderPassBeginInfo shadowMapRenderPassBeginInfo;
		shadowMapRenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		shadowMapRenderPassBeginInfo.pNext = NULL;
		shadowMapRenderPassBeginInfo.renderPass = shadowMapRenderPass;
		shadowMapRenderPassBeginInfo.framebuffer = shadowMapFramebuffer;
		shadowMapRenderPassBeginInfo.renderArea.offset.x = 0;
		shadowMapRenderPassBeginInfo.renderArea.offset.y = 0;
		shadowMapRenderPassBeginInfo.renderArea.extent.width = SHADOWMAP_WIDTH;
		shadowMapRenderPassBeginInfo.renderArea.extent.height = SHADOWMAP_HEIGHT;
		shadowMapRenderPassBeginInfo.clearValueCount = 1;
		shadowMapRenderPassBeginInfo.pClearValues = shadowMapClearValues;

		vkCmdBeginRenderPass(commandBuffers[i], &shadowMapRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport opacityMapViewport;
		opacityMapViewport.height = SHADOWMAP_HEIGHT;
		opacityMapViewport.width = SHADOWMAP_WIDTH;
		opacityMapViewport.minDepth = 0.0f;
		opacityMapViewport.maxDepth = 1.0f;
		opacityMapViewport.x = 0;
		opacityMapViewport.y = 0;

		vkCmdSetViewport(commandBuffers[i], 0, 1, &opacityMapViewport);

		VkRect2D opacityMapScissor;
		opacityMapScissor.extent.width = SHADOWMAP_WIDTH;
		opacityMapScissor.extent.height = SHADOWMAP_HEIGHT;
		opacityMapScissor.offset.x = 0;
		opacityMapScissor.offset.y = 0;

		vkCmdSetScissor(commandBuffers[i], 0, 1, &opacityMapScissor);

		const float depthBiasConstant = 1.25f;
		const float depthBiasSlope = 1.75f;

		// Set depth bias, required to avoid shadow mapping artifacts
		vkCmdSetDepthBias(commandBuffers[i], depthBiasConstant, 0.0f, depthBiasSlope);

		vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, shadowMapPipelineLayout, 0, 1, &shadowCameraDescriptorSet, 0, nullptr);

		vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, shadowMapPipeline);

		for (uint32_t j = 0; j < scene->GetHair().size(); ++j) {
			VkBuffer vertexBuffers[] = { scene->GetHair()[j]->GetStrandsBuffer() };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);

			vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, shadowMapPipelineLayout, 1, 1, &opacityMapHairDescriptorSets[j], 0, nullptr);
			vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, shadowMapPipelineLayout, 2, 1, &shadowCameraDescriptorSet, 0, nullptr);

			vkCmdDrawIndirect(commandBuffers[i], scene->GetHair()[j]->GetNumStrandsBuffer(), 1, 1, sizeof(StrandDrawIndirect));
		}

		vkCmdEndRenderPass(commandBuffers[i]);

		// apparently don't need any synchronization between render passes, done implicity via subpass dependencies

		// second pass: render deep opacity map ----------------------------------------
		VkClearValue opacityMapClearValues[1];
		shadowMapClearValues[0].depthStencil.depth = 1.0f;
		shadowMapClearValues[0].depthStencil.stencil = 0;

		VkRenderPassBeginInfo opacityMapRenderPassBeginInfo;
		opacityMapRenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		opacityMapRenderPassBeginInfo.pNext = NULL;
		opacityMapRenderPassBeginInfo.renderPass = opacityMapRenderPass;
		opacityMapRenderPassBeginInfo.framebuffer = opacityMapFramebuffer;
		opacityMapRenderPassBeginInfo.renderArea.offset.x = 0;
		opacityMapRenderPassBeginInfo.renderArea.offset.y = 0;
		opacityMapRenderPassBeginInfo.renderArea.extent.width = SHADOWMAP_WIDTH;
		opacityMapRenderPassBeginInfo.renderArea.extent.height = SHADOWMAP_HEIGHT;
		opacityMapRenderPassBeginInfo.clearValueCount = 1;
		opacityMapRenderPassBeginInfo.pClearValues = opacityMapClearValues;

		vkCmdBeginRenderPass(commandBuffers[i], &opacityMapRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport shadowMapViewport;
		shadowMapViewport.height = SHADOWMAP_HEIGHT;
		shadowMapViewport.width = SHADOWMAP_WIDTH;
		shadowMapViewport.minDepth = 0.0f;
		shadowMapViewport.maxDepth = 1.0f;
		shadowMapViewport.x = 0;
		shadowMapViewport.y = 0;

		vkCmdSetViewport(commandBuffers[i], 0, 1, &shadowMapViewport);

		VkRect2D shadowMapScissor;
		shadowMapScissor.extent.width = SHADOWMAP_WIDTH;
		shadowMapScissor.extent.height = SHADOWMAP_HEIGHT;
		shadowMapScissor.offset.x = 0;
		shadowMapScissor.offset.y = 0;

		vkCmdSetScissor(commandBuffers[i], 0, 1, &shadowMapScissor);

		// Set depth bias, required to avoid shadow mapping artifacts
		vkCmdSetDepthBias(commandBuffers[i], depthBiasConstant, 0.0f, depthBiasSlope);

		vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, opacityMapPipelineLayout, 0, 1, &shadowCameraDescriptorSet, 0, nullptr);

		vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, opacityMapPipeline);

		for (uint32_t j = 0; j < scene->GetHair().size(); ++j) {
			VkBuffer vertexBuffers[] = { scene->GetHair()[j]->GetStrandsBuffer() };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);

			vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, shadowMapPipelineLayout, 1, 1, &opacityMapHairDescriptorSets[j], 0, nullptr);
			vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, shadowMapPipelineLayout, 2, 1, &shadowCameraDescriptorSet, 0, nullptr);

			vkCmdDrawIndirect(commandBuffers[i], scene->GetHair()[j]->GetNumStrandsBuffer(), 1, 1, sizeof(StrandDrawIndirect));
		}

		vkCmdEndRenderPass(commandBuffers[i]);


        // third pass: normal graphics and hair ----------------------------------------
        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = framebuffers[i];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = swapChain->GetVkExtent();

        std::array<VkClearValue, 2> clearValues = {};
        clearValues[0].color = { 101.f / 255.f, 129.f / 255.f, 140.f / 255.f, 1.0f };
        clearValues[1].depthStencil = { 1.0f, 0 };
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

		std::vector<VkBufferMemoryBarrier> barriers(scene->GetHair().size());
        for (uint32_t j = 0; j < barriers.size(); ++j) {
            barriers[j].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            barriers[j].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            barriers[j].dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
            barriers[j].srcQueueFamilyIndex = device->GetQueueIndex(QueueFlags::Compute);
            barriers[j].dstQueueFamilyIndex = device->GetQueueIndex(QueueFlags::Graphics);
            barriers[j].buffer = scene->GetHair()[j]->GetNumStrandsBuffer();
            barriers[j].offset = 0;
            barriers[j].size = sizeof(StrandDrawIndirect);
        } 


		// TODO: move barrier before first graphics pass?
		vkCmdPipelineBarrier(commandBuffers[i], VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, 0, 0, nullptr, barriers.size(), barriers.data(), 0, nullptr);

        // Bind the camera descriptor set. This is set 0 in all pipelines so it will be inherited
        vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelineLayout, 0, 1, &cameraDescriptorSet, 0, nullptr);

        vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        // Bind the graphics pipeline
        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        for (uint32_t j = 0; j < scene->GetModels().size(); ++j) {
            // Bind the vertex and index buffers
            VkBuffer vertexBuffers[] = { scene->GetModels()[j]->getVertexBuffer() };
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);

            vkCmdBindIndexBuffer(commandBuffers[i], scene->GetModels()[j]->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

            // Bind the descriptor set for each model
            vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelineLayout, 1, 1, &textureDescriptorSets[j], 0, nullptr);

			uint32_t dynamicOffset = j * this->scene->dynamicAlignment;
			vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelineLayout, 2, 1, &modelDescriptorSet, 1, &dynamicOffset);

            std::vector<uint32_t> indices = scene->GetModels()[j]->getIndices();
            vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
        }

        // Bind the hair pipeline
        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, hairPipeline);

        for (uint32_t j = 0; j < scene->GetHair().size(); ++j) {
            VkBuffer vertexBuffers[] = { scene->GetHair()[j]->GetStrandsBuffer() }; 
            VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);

			vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, hairPipelineLayout, 1, 1, &hairDescriptorSets[j], 0, nullptr);
			vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, hairPipelineLayout, 2, 1, &shadowCameraDescriptorSet, 0, nullptr);
			vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, hairPipelineLayout, 3, 1, &opacityMapDescriptorSets[j], 0, nullptr);

			vkCmdDrawIndirect(commandBuffers[i], scene->GetHair()[j]->GetNumStrandsBuffer(), 0, 1, sizeof(StrandDrawIndirect));
        }

        // End render pass
        vkCmdEndRenderPass(commandBuffers[i]);

        // ~ End recording ~
        if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to record command buffer");
        }
    }
}

void Renderer::Frame() {

    VkSubmitInfo computeSubmitInfo = {};
    computeSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    computeSubmitInfo.commandBufferCount = 1;
    computeSubmitInfo.pCommandBuffers = &computeCommandBuffer;

    if (vkQueueSubmit(device->GetQueue(QueueFlags::Compute), 1, &computeSubmitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit draw command buffer");
    }

    if (!swapChain->Acquire()) {
        RecreateFrameResources();
        return;
    }

    // Submit the command buffer
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { swapChain->GetImageAvailableVkSemaphore() };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[swapChain->GetIndex()];

    VkSemaphore signalSemaphores[] = { swapChain->GetRenderFinishedVkSemaphore() };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(device->GetQueue(QueueFlags::Graphics), 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit draw command buffer");
    }

    if (!swapChain->Present()) {
        RecreateFrameResources();
    }
}


//void Renderer::UpdateSphere() {
//	VkCommandBufferBeginInfo beginInfo = {};
//	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
//	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
//	beginInfo.pInheritanceInfo = nullptr;
//
//	// ~ Start recording ~
//	if (vkBeginCommandBuffer(computeCommandBuffer, &beginInfo) != VK_SUCCESS) {
//		throw std::runtime_error("Failed to begin recording compute command buffer");
//	}	
//	
//	vkCmdUpdateBuffer(computeCommandBuffer, this->scene->GetCollidersBuffer(), 0, sizeof(Collider) * this->scene->GetColliders().size(), this->scene->GetColliders().data());
//
//	if (vkEndCommandBuffer(computeCommandBuffer) != VK_SUCCESS) {
//		throw std::runtime_error("Failed to record compute command buffer");
//	}
//}

Renderer::~Renderer() {
    vkDeviceWaitIdle(logicalDevice);

    vkFreeCommandBuffers(logicalDevice, graphicsCommandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
    vkFreeCommandBuffers(logicalDevice, computeCommandPool, 1, &computeCommandBuffer);
    
	vkDestroyPipeline(logicalDevice, shadowMapPipeline, nullptr);
	vkDestroyPipeline(logicalDevice, opacityMapPipeline, nullptr);
    vkDestroyPipeline(logicalDevice, graphicsPipeline, nullptr);
    vkDestroyPipeline(logicalDevice, hairPipeline, nullptr);
    vkDestroyPipeline(logicalDevice, computePipeline, nullptr);

    vkDestroyPipelineLayout(logicalDevice, shadowMapPipelineLayout, nullptr);
    vkDestroyPipelineLayout(logicalDevice, opacityMapPipelineLayout, nullptr);
    vkDestroyPipelineLayout(logicalDevice, graphicsPipelineLayout, nullptr);
    vkDestroyPipelineLayout(logicalDevice, hairPipelineLayout, nullptr);
    vkDestroyPipelineLayout(logicalDevice, computePipelineLayout, nullptr);

    vkDestroyDescriptorSetLayout(logicalDevice, cameraDescriptorSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(logicalDevice, modelMatrixDescriptorSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(logicalDevice, textureDescriptorSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(logicalDevice, timeDescriptorSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(logicalDevice, collidersDescriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(logicalDevice, gridDescriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(logicalDevice, hairDescriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(logicalDevice, computeDescriptorSetLayout, nullptr);

    vkDestroyDescriptorPool(logicalDevice, descriptorPool, nullptr);

	vkDestroyRenderPass(logicalDevice, shadowMapRenderPass, nullptr);
	vkDestroyRenderPass(logicalDevice, opacityMapRenderPass, nullptr);
    vkDestroyRenderPass(logicalDevice, renderPass, nullptr);
    DestroyFrameResources();
    DestroyShadowMapFrameResources();
    vkDestroyCommandPool(logicalDevice, computeCommandPool, nullptr);
    vkDestroyCommandPool(logicalDevice, graphicsCommandPool, nullptr);
}
