#include <vulkan/vulkan.h>
#include "Instance.h"
#include "Window.h"
#include "Renderer.h"
#include "Camera.h"
#include "Scene.h"
#include "Image.h"
#include <iostream>
#include "ObjLoader.h"


Device* device;
SwapChain* swapChain;
Renderer* renderer;
Camera* camera;

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
	Model* collisionSphere = new Model(device, transferCommandPool, vertices, indices);
	collisionSphere->SetTexture(mannequinDiffuseImage);

	ObjLoader::LoadObj("models/mannequin.obj", vertices, indices);
	Model* head = new Model(device, transferCommandPool, vertices, indices);
	head->SetTexture(mannequinDiffuseImage);

	Hair* hair = new Hair(device, transferCommandPool, "models/mannequin_segment.obj");

	Collider headCollider1 = { glm::vec3(0.0, 2.8, 0.0), 0.8 };
	Collider headCollider2 = { glm::vec3(0.0, 2.16, 0.37), 0.68 };
	Collider headCollider3 = { glm::vec3(0.0, -0.53, 0.025), 1.5 };
	Collider testCollider = { glm::vec3(2.0, 0.0, 1.0), 1.0 };

    Scene* scene = new Scene(device);
    scene->AddModel(collisionSphere);
    scene->AddModel(head);
    scene->AddHair(hair);
    scene->AddCollider(headCollider1);
    scene->AddCollider(headCollider2);
    scene->AddCollider(headCollider3);
    scene->AddCollider(testCollider);

	scene->CreateCollidersBuffer(transferCommandPool);

	vkDestroyCommandPool(device->GetVkDevice(), transferCommandPool, nullptr);


    renderer = new Renderer(device, swapChain, scene, camera);

    glfwSetWindowSizeCallback(GetGLFWWindow(), resizeCallback);
    glfwSetMouseButtonCallback(GetGLFWWindow(), mouseDownCallback);
    glfwSetCursorPosCallback(GetGLFWWindow(), mouseMoveCallback);

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
