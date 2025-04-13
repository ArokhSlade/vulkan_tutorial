#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <cstring> // strcmp
#include <map> // multimap
#include <optional>
#include <set>
#include <limits> // std::numeric_limits
#include <algorithm> // std::clamp

#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

constexpr int NOT_UNDERSTOOD = 0;
constexpr int DEPRECATED_AND_IGNORED = 0;

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation",
};

const std::vector<const char*> deviceExtensions = {
	"VK_KHR_swapchain", //can also use macro: VK_KHR_SWAPCHAIN_EXTENSION_NAME
};


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
	VkSurfaceKHR surface;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device;
	VkQueue graphicsQueue, presentQueue;
	VkSwapchainKHR swapchain;
	
	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;
		
		bool isComplete(){
			bool complete = graphicsFamily.has_value() && presentFamily.has_value();
			return complete;
		}
	};
	
	struct SwapchainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
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
		createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
		createSwapchain();
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
	
	void createSurface() {
		VkResult result = glfwCreateWindowSurface(instance, window, nullptr, &surface);
		if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to create window surface");
		}
		
		return;
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
		//TODO(Gerald, 2025 04 12): add logic to explicitly prefer a physical device that supports drawing and presentation in the same queue
		int32_t score = 0;
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(device, &properties);
		VkPhysicalDeviceFeatures features;
		vkGetPhysicalDeviceFeatures(device, &features);
		
		if (!isDeviceSuitable(device, features)) return false;
		
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
	
	bool isDeviceSuitable(const VkPhysicalDevice& device, const VkPhysicalDeviceFeatures& features) {
		if (!features.geometryShader) return false; // geometry shader is necessary
		QueueFamilyIndices indices = findQueueFamilies(device);
		if (!indices.isComplete()) return false;
		
		if (!deviceSupportsRequiredExtensions(device)) return false; // verifies swap chain support
		
		SwapchainSupportDetails swapchainSupport = querySwapchainSupport(device);
		bool swapchainIsAdequate = !swapchainSupport.formats.empty() && !swapchainSupport.presentModes.empty();
		
		return true;
	}
	
	bool deviceSupportsRequiredExtensions(const VkPhysicalDevice& device) {
		
		uint32_t extensionCount;
		VkResult result = vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> supportedExtensions(extensionCount);
		result = vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, supportedExtensions.data());
		
		for (const auto& requiredExtension : deviceExtensions) {
			if (!deviceSupportsExtension(requiredExtension, supportedExtensions)) return false;
		}
		return true;
	}
	
	bool deviceSupportsExtension(const char* extension, std::vector<VkExtensionProperties>& supportedExtensions) {
		
		for (const auto& supportedExtension : supportedExtensions) {			
			if (0 == strcmp(supportedExtension.extensionName, extension)) {
				std::cout << "success: device supports: " << extension << std::endl;
				return true;
			}
		}
		return false;
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
			VkBool32 supportsSurface{};
			VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR( device, i, surface, &supportsSurface );
			if (result != VK_SUCCESS) {
				throw std::runtime_error("could not query surface support");
			}
			if (supportsSurface) {
				indices.presentFamily = i;
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
		
		std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};		
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};
		queueCreateInfos.reserve(uniqueQueueFamilies.size());
		
		float queuePriority = 1.0f;
		for(const auto& index : uniqueQueueFamilies) {
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = index;			
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			
			queueCreateInfos.push_back(queueCreateInfo);
		}
		
		VkPhysicalDeviceFeatures deviceFeatures{}; //need nothing for now
		
		VkDeviceCreateInfo createInfo {
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.pNext = nullptr,
			.flags = NOT_UNDERSTOOD,
			.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
			.pQueueCreateInfos = queueCreateInfos.data(),
			.enabledLayerCount = enableValidationLayers ? static_cast<uint32_t>(validationLayers.size()) : 0,
			.ppEnabledLayerNames = enableValidationLayers ? validationLayers.data() : 0,
			
			.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
			.ppEnabledExtensionNames = deviceExtensions.data(),
			.pEnabledFeatures = &deviceFeatures,
		};
		
		VkResult result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
		if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to create logical device");
		}
		
		vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
		vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
	}
	
	void createSwapchain() {
		SwapchainSupportDetails swapchainSupport = querySwapchainSupport(physicalDevice);
		
		VkSurfaceFormatKHR format = chooseSwapSurfaceFormat(swapchainSupport.formats);
		VkPresentModeKHR presentMode = chooseSwapPresentMode(swapchainSupport.presentModes);
		VkExtent2D extent = chooseSwapExtent(swapchainSupport.capabilities);
		
		
		/* sticking to minimum means that we may sometimes have to wait on the driver to 
		   complete internal operations before we can acquire another image to render to */
		uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
		if (swapchainSupport.capabilities.maxImageCount != 0 && 
		    imageCount > swapchainSupport.capabilities.maxImageCount) {
			imageCount = swapchainSupport.capabilities.maxImageCount;
		}
		
		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;
		createInfo.minImageCount = imageCount;
        createInfo.imageFormat = format.format;
        createInfo.imageColorSpace = format.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1; //"This is always 1 unless you are developing a stereoscopic 3D application"
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // render directly to images (no post processing)
		
		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
		uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};
		if (indices.graphicsFamily == indices.presentFamily) {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		} else {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT; // avoid managing image ownership among queue families
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
        createInfo.preTransform = swapchainSupport.capabilities.currentTransform; // no special transform
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; //ignore alpha
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;
		
		VkResult result = vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain);
		if (result != VK_SUCCESS) {
			throw std::runtime_error("failed creating swap chain");
		}
	}
	
	SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device) {
		SwapchainSupportDetails details{};
		
		VkSurfaceCapabilitiesKHR capabilities;
		VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
		
		uint32_t formatCount = 0;		
		result = vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
		if (formatCount != 0) {
			details.formats.resize(formatCount);
			result = vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
		}
		
		uint32_t modeCount = 0;
		result = vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &modeCount, nullptr);
		if (modeCount != 0) {
			details.presentModes.resize(modeCount);
			result = vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &modeCount, details.presentModes.data());
		}
		
		return details;
	}
	
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
		constexpr VkFormat preferredFormat = VK_FORMAT_B8G8R8A8_SRGB;
		constexpr VkColorSpaceKHR preferredColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
		for (const auto& availableFormat : availableFormats) {
			if (availableFormat.format == preferredFormat && availableFormat.colorSpace == preferredColorSpace) {
				return availableFormat;
			}
		}
		return availableFormats[0]; // happy with whatever we get
	}
	
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
		constexpr VkPresentModeKHR preferredPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
		for (const auto& availablePresentMode : availablePresentModes) {
			if (availablePresentMode == preferredPresentMode) {
				return availablePresentMode;
			}
		}
		return VK_PRESENT_MODE_FIFO_KHR; // guaranteed to exist
	}
	
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			return capabilities.currentExtent;
		} else {
			int width, height;
			glfwGetFramebufferSize(window, &width, &height);
			
			VkExtent2D actualExtent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height),
			};
			
			actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
			
			return actualExtent;
		}
		
	}
	

    void mainLoop() {
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
		}
    }

    void cleanup() {
		vkDestroySwapchainKHR(device, swapchain, nullptr);
		vkDestroyDevice(device, nullptr);
		if (enableValidationLayers) {
			destroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
		}
		vkDestroySurfaceKHR(instance, surface, nullptr);
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