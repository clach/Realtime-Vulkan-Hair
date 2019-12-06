#pragma once

#include "Device.h"
#include "SwapChain.h"
#include "Scene.h"
#include "Camera.h"

class Renderer {
public:
    Renderer() = delete;
    Renderer(Device* device, SwapChain* swapChain, Scene* scene, Camera* camera);
    ~Renderer();

	Scene* scene;


    void CreateCommandPools();

	//void CreateDepthImage();

    void CreateRenderPass();
	void CreateShadowMapRenderPass();


    void CreateCameraDescriptorSetLayout();
    void CreateModelMatrixDescriptorSetLayout();
    void CreateTextureDescriptorSetLayout();
	void CreateHairDescriptorSetLayout();
    void CreateTimeDescriptorSetLayout();
    void CreateCollidersDescriptorSetLayout();
	void CreateGridDescriptorSetLayout();
    void CreateComputeDescriptorSetLayout();

    void CreateDescriptorPool();

    void CreateCameraDescriptorSet();
    void CreateModelDescriptorSet();
    void CreateTextureDescriptorSets();
    void CreateHairDescriptorSets();
    void CreateTimeDescriptorSet();
    void CreateCollidersDescriptorSets();
	void CreateGridDescriptorSets();
    void CreateComputeDescriptorSets();

    void CreateGraphicsPipeline();
	void CreateShadowMapPipeline();
    void CreateHairPipeline();
    void CreateComputePipeline();

    void CreateFrameResources();
    void DestroyFrameResources();
    void RecreateFrameResources();

	void CreateShadowMapFrameResources();
	void DestroyShadowMapFrameResources();

    void RecordCommandBuffers();
    void RecordComputeCommandBuffer();

	//void UpdateShere();

    void Frame();

private:
    Device* device;
    VkDevice logicalDevice;
    SwapChain* swapChain;
    Camera* camera;

    VkCommandPool graphicsCommandPool;
    VkCommandPool computeCommandPool;

    VkRenderPass renderPass;
    VkRenderPass shadowMapRenderPass;

    VkDescriptorSetLayout cameraDescriptorSetLayout;
    VkDescriptorSetLayout modelMatrixDescriptorSetLayout;
    VkDescriptorSetLayout textureDescriptorSetLayout;
    VkDescriptorSetLayout hairDescriptorSetLayout;
	VkDescriptorSetLayout timeDescriptorSetLayout;
	VkDescriptorSetLayout collidersDescriptorSetLayout;
	VkDescriptorSetLayout gridDescriptorSetLayout;
	VkDescriptorSetLayout computeDescriptorSetLayout;
    
    VkDescriptorPool descriptorPool;

    VkDescriptorSet cameraDescriptorSet;
    std::vector<VkDescriptorSet> textureDescriptorSets;
	std::vector<VkDescriptorSet> hairDescriptorSets;

	VkDescriptorSet modelDescriptorSet;
    VkDescriptorSet timeDescriptorSet;
	VkDescriptorSet collidersDescriptorSets;
	VkDescriptorSet gridDescriptorSets;
	std::vector<VkDescriptorSet> computeDescriptorSets;

    VkPipelineLayout graphicsPipelineLayout;
    VkPipelineLayout shadowMapPipelineLayout;
    VkPipelineLayout hairPipelineLayout;
    VkPipelineLayout computePipelineLayout;

    VkPipeline graphicsPipeline;
    VkPipeline shadowMapPipeline;
    VkPipeline hairPipeline;
    VkPipeline computePipeline;

    std::vector<VkImageView> imageViews;
    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;
    std::vector<VkFramebuffer> framebuffers;

	VkImage shadowMapImage;
	VkDeviceMemory shadowMapImageMemory;
	VkImageView shadowMapImageView;
	VkFramebuffer shadowMapFramebuffer;
	VkSampler shadowMapSampler;

    std::vector<VkCommandBuffer> commandBuffers;
    VkCommandBuffer computeCommandBuffer;
};
