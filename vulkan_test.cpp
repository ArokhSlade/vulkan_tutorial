#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <cstring> //strcmp
#include <map> //multimap
#include <optional>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

VkResult createDebugUtilsMessengerEXT(
	VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
	const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger) {
		
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	VkResult result;
	if (func != nullptr) {
		result = func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	} else { 
		result = VK_ERROR_EXTENSION_NOT_PRESENT;
	}
	return result;
}

void destroyDebugUtilsMessengerEXT(VkInstance instance, 
	VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator) {
		
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	
	func(instance, debugMessenger, pAllocator);
}

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
	VkDebugUtilsMessengerEXT debugMessenger;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device;
	
	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;
		
		bool isComplete(){
			bool complete = graphicsFamily.has_value();
			return complete;
		}
	};
	
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
		void *pUserData) {
			
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
		
		return VK_FALSE;
	}

	void initWindow() {
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Tutorial", nullptr, nullptr);
	}

    void initVulkan() {
		createInstance();
		setupDebugMessenger();
		pickPhysicalDevice();
		createLogicalDevice();
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
		
		std::vector<VkExtensionProperties> availableExtensions = getAvailableExtensions();
		std::cout << "available extensions:\n";
		for(const auto& extension : availableExtensions) {
			std::cout << '\t' << extension.extensionName << " : " << extension.specVersion << '\n';
		}		
		
		std::vector<const char*> requiredExtensions = getRequiredExtensions();		
		createInfo.enabledExtensionCount += requiredExtensions.size();
		createInfo.ppEnabledExtensionNames = requiredExtensions.data();				
		
		bool extensions_supported = checkRequiredExtensions(createInfo, availableExtensions);
		if (!extensions_supported) {
			throw std::runtime_error("unsupported extension required.");
		}
		
		
		VkDebugUtilsMessengerCreateInfoEXT messengerCreateInfo{};
		if (enableValidationLayers) {
			bool layers_supported = checkValidationLayerSupport();
			if (!layers_supported) {
				throw std::runtime_error("unsupported validation layer requested.");
			}
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
			
			populateDebugUtilsMessengerCreateInfoEXT(messengerCreateInfo);
			createInfo.pNext = &messengerCreateInfo;
		}
		
		result = vkCreateInstance(&createInfo, nullptr, &instance);		
		if (result != VK_SUCCESS) {
			throw std::runtime_error("vkCreateInstance() failed.");
		}
	}
	
	std::vector<VkExtensionProperties> getAvailableExtensions() {
		uint32_t extensionCount = 0;
		VkResult result = vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> extensions(extensionCount);
		result = vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
		return extensions;
	}
	
	std::vector<const char*> getRequiredExtensions() {		
		uint32_t extensionCount;
		const char** requiredExtensionNames = glfwGetRequiredInstanceExtensions(&extensionCount);
		std::vector<const char*> requiredExtensions(requiredExtensionNames, requiredExtensionNames + extensionCount);
		
		if (enableValidationLayers) {
			requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
		
		return requiredExtensions;
	}
	
	bool checkRequiredExtensions(const VkInstanceCreateInfo& createInfo, const std::vector<VkExtensionProperties>& extensions) {
		const char*required;
		bool all_supported = true;
		std::cout << "required extensions:\n";
		for (uint32_t i = 0 ; i < createInfo.enabledExtensionCount ; ++i) {
			required = createInfo.ppEnabledExtensionNames[i];
			bool supported = false;
			for (const auto& available : extensions) {
				supported = !std::strcmp(required, available.extensionName);
				if (supported) break;
			}			
			std::cout << '\t' << required << " : " << (supported ? "was found" : "not found") << '\n';
			all_supported &= supported;			
		}
		
		return all_supported;
	}
	
	bool checkValidationLayerSupport() {		
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
		
		std::cout << "available validation layers:\n";		
		for (const auto& available : availableLayers) {
			std::cout << '\t' << available.layerName << '\n';
		}
		
		bool all_supported = true;
		std::cout << "checking validation layers:\n";
		for (const auto& requested : validationLayers) {
			std::cout << '\t' << requested << '\n';
			bool supported = false;
			for (const auto& available : availableLayers) {
				supported = 0 == std::strcmp(requested, available.layerName);
				if (supported) break;
			}			
			all_supported &= supported;
			
			if (!all_supported) break;
		}
		return all_supported;
	}
	
	void setupDebugMessenger() {
		if (!enableValidationLayers) return;
		
		VkDebugUtilsMessengerCreateInfoEXT createInfo{};
		populateDebugUtilsMessengerCreateInfoEXT(createInfo);
		VkResult result = createDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger);
		if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to setup debug messenger.");
		}
	}
	
	void populateDebugUtilsMessengerCreateInfoEXT(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = /*VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | */VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallback;
		createInfo.pUserData = this;
	}
	
	void pickPhysicalDevice() {
		uint32_t deviceCount;
		VkResult result = vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
		std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
		result = vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data());
		
		std::cout << "Physical Devices found:\n";
		for (const auto& device : physicalDevices) {
			VkPhysicalDeviceProperties properties{};
			vkGetPhysicalDeviceProperties(device, &properties);
			std::cout << '\t' << properties.deviceName << '\n';			
		}
		
		VkPhysicalDevice picked{};
		std::multimap<int32_t, VkPhysicalDevice> scores = rateDevices(physicalDevices);
		if (scores.rbegin()->first > 0) {
			picked = scores.rbegin()->second;
		} else {
			throw std::runtime_error("no suitable GPU found.");
		}
		
		physicalDevice = picked;
		
		return;
	}
	
	std::multimap<int32_t, VkPhysicalDevice> rateDevices(std::vector<VkPhysicalDevice> devices) {
		std::multimap<int32_t, VkPhysicalDevice> scores{};
		for (const auto& device : devices){
			int32_t score = rateDevice(device);
			std::pair<int32_t, VkPhysicalDevice> entry{score, device};
			scores.insert(entry);
		}
		return scores;
	}
	
	int32_t rateDevice(const VkPhysicalDevice& device) {
		int32_t score = 0;
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(device, &properties);
		VkPhysicalDeviceFeatures features;
		vkGetPhysicalDeviceFeatures(device, &features);
		
		if (!features.geometryShader) return 0; //geometryShader is necessary
		QueueFamilyIndices indices = findQueueFamilies(device);
		if (!indices.isComplete()) return 0;
		
		switch(properties.deviceType) {
			break;case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: {
				score += 30;
			} 
			break;case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: {
				score += 100;
			}
			break;case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: {
				score += 5;
			}
			break;case VK_PHYSICAL_DEVICE_TYPE_CPU: {
				score += 10;
			}
			break;default:{}
		}
		
		score += properties.limits.maxImageDimension2D/128;
		
		std::cout << properties.deviceName << " : " << score << '\n';
		
		return score;
	} 
	
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
		QueueFamilyIndices indices;
		uint32_t count;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilies{count};
		vkGetPhysicalDeviceQueueFamilyProperties(device, &count, queueFamilies.data());
				
		int32_t i = 0;
		for (const auto& properties : queueFamilies) {
			if (properties.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				indices.graphicsFamily = i;				
			}
			if (indices.isComplete()) {
				break;
			}
			++i;
		}
		
		
		return indices;
	}
	
	void createLogicalDevice() {
		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
		queueCreateInfo.queueCount = 1;
		
		float queuePriority = 1.0f;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		
		VkPhysicalDeviceFeatures deviceFeatures{}; //need nothing for now
	}
	

    void mainLoop() {
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
		}
    }

    void cleanup() {
		if (enableValidationLayers) {
			destroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
		}
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