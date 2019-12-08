#pragma once

#include "Device.h"
#include "SwapChain.h"
#include "Scene.h"
#include "Camera.h"

class Renderer {
public:
    Renderer() = delete;
    Renderer(Device* device, SwapChain* swapChain, Scene* scene, Camera* camera, Camera* shadowCamera);
    ~Renderer();

	Scene* scene;

    void CreateCommandPools();

    void CreateRenderPass();
	void CreateShadowMapRenderPass();
	void CreateOpacityMapRenderPass();

    void CreateCameraDescriptorSetLayout();
    void CreateModelMatrixDescriptorSetLayout();
    void CreateTextureDescriptorSetLayout();
	void CreateHairDescriptorSetLayout();
	void CreateOpacityMapDescriptorSetLayout();
    void CreateTimeDescriptorSetLayout();
    void CreateCollidersDescriptorSetLayout();
	void CreateGridDescriptorSetLayout();
    void CreateComputeDescriptorSetLayout();

    void CreateDescriptorPool();

    void CreateCameraDescriptorSet();
    void CreateModelDescriptorSet();
    void CreateTextureDescriptorSets();
    void CreateShadowCameraDescriptorSet();
    void CreateHairDescriptorSets();
    void CreateOpacityMapDescriptorSets();
    void CreateOpacityMapHairDescriptorSets();
    void CreateTimeDescriptorSet();
    void CreateCollidersDescriptorSets();
	void CreateGridDescriptorSets();
    void CreateComputeDescriptorSets();

	void CreateShadowMapPipeline();
	void CreateOpacityMapPipeline();
    void CreateGraphicsPipeline();
    void CreateHairPipeline();
    void CreateComputePipeline();

	void CreateShadowMapFrameResources();
	void CreateOpacityMapFrameResources();
    void CreateFrameResources();
	void DestroyShadowMapFrameResources();
	void DestroyOpacityMapFrameResources();
    void DestroyFrameResources();
    void RecreateFrameResources();

    void RecordCommandBuffers();
    void RecordComputeCommandBuffer();

	//void UpdateSphere();

    void Frame();

private:
    Device* device;
    VkDevice logicalDevice;
    SwapChain* swapChain;
    Camera* camera;
    Camera* shadowCamera;

    VkCommandPool graphicsCommandPool;
    VkCommandPool computeCommandPool;

    VkRenderPass renderPass;
    VkRenderPass shadowMapRenderPass;
    VkRenderPass opacityMapRenderPass;

    VkDescriptorSetLayout cameraDescriptorSetLayout;
    VkDescriptorSetLayout modelMatrixDescriptorSetLayout;
    VkDescriptorSetLayout textureDescriptorSetLayout;
    VkDescriptorSetLayout hairDescriptorSetLayout;
    VkDescriptorSetLayout opacityMapDescriptorSetLayout;
	VkDescriptorSetLayout timeDescriptorSetLayout;
	VkDescriptorSetLayout collidersDescriptorSetLayout;
	VkDescriptorSetLayout gridDescriptorSetLayout;
	VkDescriptorSetLayout computeDescriptorSetLayout;
    
    VkDescriptorPool descriptorPool;

    VkDescriptorSet cameraDescriptorSet;
    std::vector<VkDescriptorSet> textureDescriptorSets;
    VkDescriptorSet shadowCameraDescriptorSet;
	std::vector<VkDescriptorSet> hairDescriptorSets;
	VkDescriptorSet modelDescriptorSet;
	std::vector<VkDescriptorSet> opacityMapDescriptorSets;
	std::vector<VkDescriptorSet> opacityMapHairDescriptorSets;
    VkDescriptorSet timeDescriptorSet;
	VkDescriptorSet collidersDescriptorSets;
	VkDescriptorSet gridDescriptorSets;
	std::vector<VkDescriptorSet> computeDescriptorSets;

    VkPipelineLayout graphicsPipelineLayout;
    VkPipelineLayout shadowMapPipelineLayout;
    VkPipelineLayout opacityMapPipelineLayout;
    VkPipelineLayout hairPipelineLayout;
    VkPipelineLayout computePipelineLayout;

    VkPipeline graphicsPipeline;
    VkPipeline shadowMapPipeline;
    VkPipeline opacityMapPipeline;
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

	VkImage opacityMapImage;
	VkDeviceMemory opacityMapImageMemory;
	VkImageView opacityMapImageView;
	VkFramebuffer opacityMapFramebuffer;
	VkSampler opacityMapSampler;

    std::vector<VkCommandBuffer> commandBuffers;
    VkCommandBuffer computeCommandBuffer;
};
