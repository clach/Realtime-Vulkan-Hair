#include <vulkan/vulkan.h>
#include "Instance.h"
#include "Window.h"
#include "Renderer.h"
#include "Camera.h"
#include "Scene.h"
#include "Image.h"
#include <iostream>
#include "ObjLoader.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_vulkan.h"
#include "imgui/imgui_impl_glfw.h"

Device* device;
SwapChain* swapChain;
Renderer* renderer;
Camera* camera;

#define VK_CHECK(expr_) \
	do { \
		VkResult result_ = expr_; \
		if (result_ != 0) \
			CheckFailed(result_, __FILE__, __LINE__, #expr_); \
	} while (0)

namespace {
    void resizeCallback(GLFWwindow* window, int width, int height) {
        if (width == 0 || height == 0) return;

        vkDeviceWaitIdle(device->GetVkDevice());
        swapChain->Recreate();
        renderer->RecreateFrameResources();
    }

    bool leftMouseDown = false;
    bool rightMouseDown = false;
    double previousX = 0.0;
    double previousY = 0.0;

    void mouseDownCallback(GLFWwindow* window, int button, int action, int mods) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (action == GLFW_PRESS) {
                leftMouseDown = true;
                glfwGetCursorPos(window, &previousX, &previousY);
            }
            else if (action == GLFW_RELEASE) {
                leftMouseDown = false;
            }
        } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            if (action == GLFW_PRESS) {
                rightMouseDown = true;
                glfwGetCursorPos(window, &previousX, &previousY);
            }
            else if (action == GLFW_RELEASE) {
                rightMouseDown = false;
            }
        }
    }

    void mouseMoveCallback(GLFWwindow* window, double xPosition, double yPosition) {
        if (leftMouseDown) {
            double sensitivity = 0.5;
            float deltaX = static_cast<float>((previousX - xPosition) * sensitivity);
            float deltaY = static_cast<float>((previousY - yPosition) * sensitivity);

            camera->UpdateOrbit(deltaX, deltaY, 0.0f);

            previousX = xPosition;
            previousY = yPosition;
        } else if (rightMouseDown) {
            double deltaZ = static_cast<float>((previousY - yPosition) * 0.05);

            camera->UpdateOrbit(0.0f, 0.0f, deltaZ);

            previousY = yPosition;
        }
    }
}

void CheckFailed(VkResult result, const char* file, int line, const char* expr)
{
#ifdef _WIN32
	char buffer[1024];
	_snprintf_s(buffer, sizeof(buffer), "%s:%d %s returned %d\n", file, line, expr, result);
	OutputDebugStringA(buffer);
	__debugbreak();
#else
	fprintf(stderr, "%s:%d %s returned %d\n", file, line, expr, result);
	__builtin_trap();
#endif
}

void InitImGuiFonts(VkDevice& device, ImGui_ImplVulkanH_Window& window)
{
	ImGui_ImplVulkanH_Frame* fd = &window.Frames[window.FrameIndex];

	VK_CHECK(vkResetCommandPool(device, fd->CommandPool, 0));

	VkCommandBufferBeginInfo cbbi = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	cbbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	VK_CHECK(vkBeginCommandBuffer(fd->CommandBuffer, &cbbi));

	ImGui_ImplVulkan_CreateFontsTexture(fd->CommandBuffer);

	VK_CHECK(vkEndCommandBuffer(fd->CommandBuffer));

	VkSubmitInfo si = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	si.commandBufferCount = 1;
	si.pCommandBuffers = &fd->CommandBuffer;
	//VK_CHECK(vkQueueSubmit(vulkan.queue, 1, &si, VK_NULL_HANDLE));

	//VK_CHECK(vkQueueWaitIdle(vulkan.queue));

	ImGui_ImplVulkan_DestroyFontUploadObjects();
}

int main() {
    static constexpr char* applicationName = "Realtime Vulkan Hair";
    InitializeWindow(640, 480, applicationName);

    unsigned int glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    Instance* instance = new Instance(applicationName, glfwExtensionCount, glfwExtensions);

    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(instance->GetVkInstance(), GetGLFWWindow(), nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create window surface");
    }

    instance->PickPhysicalDevice({ VK_KHR_SWAPCHAIN_EXTENSION_NAME }, QueueFlagBit::GraphicsBit | QueueFlagBit::TransferBit | QueueFlagBit::ComputeBit | QueueFlagBit::PresentBit, surface);

    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.tessellationShader = VK_TRUE;
    deviceFeatures.fillModeNonSolid = VK_TRUE;
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    device = instance->CreateDevice(QueueFlagBit::GraphicsBit | QueueFlagBit::TransferBit | QueueFlagBit::ComputeBit | QueueFlagBit::PresentBit, deviceFeatures);

    swapChain = device->CreateSwapChain(surface, 5);

    camera = new Camera(device, 640.f / 480.f);

    VkCommandPoolCreateInfo transferPoolInfo = {};
    transferPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    transferPoolInfo.queueFamilyIndex = device->GetInstance()->GetQueueFamilyIndices()[QueueFlags::Transfer];
    transferPoolInfo.flags = 0;

    VkCommandPool transferCommandPool;
    if (vkCreateCommandPool(device->GetVkDevice(), &transferPoolInfo, nullptr, &transferCommandPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool");
    }

	VkImage mannequinDiffuseImage;
	VkDeviceMemory mannequinDiffuseImageMemory;
	Image::FromFile(device,
		transferCommandPool,
		"images/mannequin_diffuse.png",
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		mannequinDiffuseImage,
		mannequinDiffuseImageMemory
	);

	// TODO: add capacity for multiple textures/normal maps/etc for a Model

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	ObjLoader::LoadObj("models/collisionTest.obj", vertices, indices);
	Model* collisionSphere = new Model(device, transferCommandPool, vertices, indices, glm::scale(glm::vec3(0.98)));
	collisionSphere->SetTexture(mannequinDiffuseImage);

	ObjLoader::LoadObj("models/mannequin.obj", vertices, indices);
	Model* head = new Model(device, transferCommandPool, vertices, indices, glm::scale(glm::vec3(0.98)));
	head->SetTexture(mannequinDiffuseImage);

	Hair* hair = new Hair(device, transferCommandPool, "models/mannequin_segment.obj");

	// trans, rot, scale
	Collider testCollider = Collider(glm::vec3(2.0, 0.0, 1.0), glm::vec3(0.0), glm::vec3(1.0));
	Collider headCollider = Collider(glm::vec3(0.0, 2.64, 0.08), glm::vec3(-38.270, 0.0, 0.0), glm::vec3(0.844, 1.195, 1.05));
	Collider neckCollider = Collider(glm::vec3(0.0, 1.35, -0.288), glm::vec3(18.301, 0.0, 0.0), glm::vec3(0.457, 1.0, 0.538));
	Collider bustCollider = Collider(glm::vec3(0.0, -0.482, -0.061), glm::vec3(-17.260, 0.0, 0.0), glm::vec3(1.078, 1.467, 0.9));
	Collider shoulderRCollider = Collider(glm::vec3(-0.698, 0.087, -0.36), glm::vec3(-20.254, 13.144, 34.5), glm::vec3(0.721, 1.0, 0.724));
	Collider shoulderLCollider = Collider(glm::vec3(0.698, 0.087, -0.36), glm::vec3(-20.254, 13.144, -34.5), glm::vec3(0.721, 1.0, 0.724));

    Scene* scene = new Scene(device);
    scene->AddModel(collisionSphere);
    scene->AddModel(head);
    scene->AddHair(hair);
	scene->AddCollider(testCollider);
    scene->AddCollider(headCollider);
    scene->AddCollider(neckCollider);
    scene->AddCollider(bustCollider);
    scene->AddCollider(shoulderRCollider);
	scene->AddCollider(shoulderLCollider);

	scene->CreateCollidersBuffer(transferCommandPool);

	vkDestroyCommandPool(device->GetVkDevice(), transferCommandPool, nullptr);

    renderer = new Renderer(device, swapChain, scene, camera);

    glfwSetWindowSizeCallback(GetGLFWWindow(), resizeCallback);
    glfwSetMouseButtonCallback(GetGLFWWindow(), mouseDownCallback);
    glfwSetCursorPosCallback(GetGLFWWindow(), mouseMoveCallback);



	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	(void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForVulkan(GetGLFWWindow(), true);

	ImGui_ImplVulkan_InitInfo initInfo = {};
	initInfo.Instance = instance->GetVkInstance();
	initInfo.PhysicalDevice = instance->GetPhysicalDevice();
	initInfo.Device = device->GetVkDevice();
	initInfo.QueueFamily = device->GetInstance()->GetQueueFamilyIndices()[QueueFlags::Graphics];
	initInfo.Queue = device->GetQueue(QueueFlags::Graphics);
	initInfo.PipelineCache = VK_NULL_HANDLE;
	initInfo.DescriptorPool = renderer->GetDescriptorPool();
	initInfo.Allocator = nullptr;
	initInfo.MinImageCount = 2;
	initInfo.ImageCount = swapChain->GetCount();
	initInfo.CheckVkResultFn = VK_NULL_HANDLE;
	//ImGui_ImplVulkan_Init(&initInfo, renderer->GetRenderPass());

    while (!ShouldQuit()) {
        glfwPollEvents();
        scene->UpdateTime();
		double previousTime = glfwGetTime();
        renderer->Frame();
		double currentTime = glfwGetTime();
		//std::cout << currentTime - previousTime << std::endl;
    }

    vkDeviceWaitIdle(device->GetVkDevice());

	vkDestroyImage(device->GetVkDevice(), mannequinDiffuseImage, nullptr);
	vkFreeMemory(device->GetVkDevice(), mannequinDiffuseImageMemory, nullptr);

    delete scene;
	delete collisionSphere;
	delete head;
    delete hair;
    delete camera;
    delete renderer;
    delete swapChain;
    delete device;
    delete instance;
    DestroyWindow();
    return 0;
}
