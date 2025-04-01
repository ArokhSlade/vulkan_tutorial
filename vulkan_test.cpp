#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <cstring> //strcmp

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

class HelloTriangleApplication {
public:
    void run() {
		initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:

	GLFWwindow *window;
	VkInstance instance;

	void initWindow() {
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Tutorial", nullptr, nullptr);
	}

    void initVulkan() {
		createInstance();
		
    }
	
	void createInstance() {
		VkResult result;
		
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;
		
		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		
		//check available extensions
		uint32_t extensionCount = 0;
		result = vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> extensions(extensionCount);
		result = vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
		std::cout << "available extensions:\n";
		for(const auto& extension : extensions) {
			std::cout << '\t' << extension.extensionName << " : " << extension.specVersion << '\n';
		}
		
		createInfo.ppEnabledExtensionNames = glfwGetRequiredInstanceExtensions(&createInfo.enabledExtensionCount);
		
		checkRequiredExtensions(createInfo, extensions);
		
		createInfo.enabledLayerCount = 0;
		
		result = vkCreateInstance(&createInfo, nullptr, &instance);		
		if (result != VK_SUCCESS) {
			throw std::runtime_error("vkCreateInstance() failed.");
		}
	}
	
	void checkRequiredExtensions(const VkInstanceCreateInfo& createInfo, const std::vector<VkExtensionProperties>& extensions) {
		const char*required;
		bool all_supported = true;
		std::cout << "required extensions:\n";
		for (int i = 0 ; i < createInfo.enabledExtensionCount ; ++i) {
			required = createInfo.ppEnabledExtensionNames[i];
			bool supported = false;
			for (const auto& available : extensions) {
				supported = !std::strcmp(required, available.extensionName);
				if (supported) break;
			}			
			std::cout << '\t' << required << " : " << (supported ? "was found" : "not found") << '\n';
			all_supported &= supported;			
		}
		
		if (!all_supported) {
			throw std::runtime_error("unsupported extension required by glfw.");
		}
	}

    void mainLoop() {
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
		}
    }

    void cleanup() {
		vkDestroyInstance(instance, nullptr);
		glfwDestroyWindow(window);		
		glfwTerminate();
    }
	
	
};

int main() {
    HelloTriangleApplication app;
	std::cout << "hello\n";

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}