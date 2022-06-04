#include "VulkanRHI.h"
#include "Runtime/Function/Render/WindowSystem.h"

#include <algorithm>
#include <cassert>

#include <iostream>
#include <stdexcept>
#include <set>

namespace ME
{
	VulkanRHI::~VulkanRHI()
	{
		// TODO
	}

	void VulkanRHI::Initialize(RHIInitInfo init_info)
	{
		m_window = init_info.window_system->GetWindow();

		std::array<int, 2> window_size = init_info.window_system->GetWindowSize();

		m_viewport = { 0.0f, 0.0f, (float)window_size[0], (float)window_size[1], 0.0f, 1.0f };
		m_scissor = { {0, 0}, {(uint32_t)window_size[0], (uint32_t)window_size[1]} };

#ifndef NDEBUG
		m_enable_validation_layers = true;
		m_enable_debug_utils_label = true;
#else
		m_enable_validation_layers = false;
		m_enable_debug_utils_label = false;
#endif // !NDEBUG


		CreateInstance();

		InitializeDebugMessenger();

		CreateWindowSurface();

		InitializePhysicalDevice();

		CreateLogicalDevice();
	}

	void VulkanRHI::PrepareContext()
	{

	}

	bool VulkanRHI::CheckValidationLayerSupport()
	{
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const char* layerName : m_validation_layers)
		{
			bool layerFound = false;

			for (const auto& layerProperties : availableLayers)
			{
				if (strcmp(layerName, layerProperties.layerName) == 0)
				{
					layerFound = true;
					break;
				}
			}

			if (!layerFound)
			{
				return false;
			}
		}

		return true;
	}

	std::vector<const char*> VulkanRHI::GetRequiredExtensions()
	{
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
		if (m_enable_validation_layers || m_enable_debug_utils_label)
		{
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}

	// debug callback
	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT,
														VkDebugUtilsMessageTypeFlagsEXT,
														const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
														void*)
	{
		std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;
		return VK_FALSE;
	}

	void VulkanRHI::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
	{
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = DebugCallback;
	}

	void VulkanRHI::CreateInstance()
	{
		// validation layer will be enabled in debug mode
		if (m_enable_validation_layers && !CheckValidationLayerSupport())
		{
			throw std::runtime_error("validation layers requested, but not available!");
		}

		m_vulkan_api_version = VK_API_VERSION_1_0;

		// app info
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "MiniEngine_Renderer";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "MiniEngine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = m_vulkan_api_version;

		// create info
		VkInstanceCreateInfo instance_create_info{};
		instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instance_create_info.pApplicationInfo = &appInfo;

		auto extensions = GetRequiredExtensions();
		instance_create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		instance_create_info.ppEnabledExtensionNames = extensions.data();
		
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		if (m_enable_validation_layers)
		{
			instance_create_info.enabledLayerCount = static_cast<uint32_t>(m_validation_layers.size());
			instance_create_info.ppEnabledExtensionNames = m_validation_layers.data();

			PopulateDebugMessengerCreateInfo(debugCreateInfo);
			instance_create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
		}
		else
		{
			instance_create_info.enabledLayerCount = 0;
			instance_create_info.pNext = nullptr;
		}

		// create m_vulkan_context._instance
		if (vkCreateInstance(&instance_create_info, nullptr, &m_instance) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create instance");
		}

	}

	void VulkanRHI::InitializeDebugMessenger()
	{
		if (m_enable_validation_layers)
		{
			VkDebugUtilsMessengerCreateInfoEXT createInfo;
			PopulateDebugMessengerCreateInfo(createInfo);
			if (CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debug_messenger) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to set up debug messenger!");
			}
		}

		if (m_enable_debug_utils_label)
		{
			m_vk_cmd_begin_debug_utils_label_ext =
				(PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(m_instance, "vkCmdBeginUtilsLabelEXT");
			m_vk_cmd_end_debug_utils_label_ext =
				(PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(m_instance, "vkCmdEndDebugUtilsLabelEXT");
		}
	}

	void VulkanRHI::CreateWindowSurface()
	{
		if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create window surface!");
		}
	}

	void VulkanRHI::InitializePhysicalDevice()
	{
		uint32_t physical_device_count;
		vkEnumeratePhysicalDevices(m_instance, &physical_device_count, nullptr);
		if (physical_device_count == 0)
		{
			throw std::runtime_error("Failed to find GUPs with vulkan support!");
		}
		else
		{
			// find one device that matches our requirement
			// or find which is the best
			std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
			vkEnumeratePhysicalDevices(m_instance, &physical_device_count, physical_devices.data());

			std::vector<std::pair<int, VkPhysicalDevice>> ranked_physical_devices;
			for (const auto& device : physical_devices)
			{
				VkPhysicalDeviceProperties physical_device_properties;
				vkGetPhysicalDeviceProperties(device, &physical_device_properties);
				int score = 0;

				if (physical_device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
				{
					score += 1000;
				}
				else if(physical_device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
				{
					score += 100;
				}

				ranked_physical_devices.push_back({ score, device });
			}

			std::sort(ranked_physical_devices.begin(),
					  ranked_physical_devices.end(),
					  [](const std::pair<int, VkPhysicalDevice>& p1, const std::pair<int, VkPhysicalDevice>& p2)
					  {
						  return p1 > p2;
					  });

			for (const auto& device : ranked_physical_devices)
			{
				if (IsDeviceSuitable(device.second))
				{
					m_physical_device = device.second;
					break;
				}
			}

			if (m_physical_device == VK_NULL_HANDLE)
			{
				throw std::runtime_error("Failed to find suitable physical device");
			}
		}
	}

	// logical device
	void VulkanRHI::CreateLogicalDevice()
	{
		m_queue_indices = FindQueueFamilies(m_physical_device);

		std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
		std::set<uint32_t> queue_families = { m_queue_indices.m_graphics_family.value(), m_queue_indices.m_present_family.value()};

		float queue_priority = 1.0f;
		for (uint32_t queue_family : queue_families) // for every queue family
		{
			// queue create info
			VkDeviceQueueCreateInfo queue_create_info{};
			queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queue_create_info.queueFamilyIndex = queue_family;
			queue_create_info.queueCount = 1;
			queue_create_info.pQueuePriorities = &queue_priority;
			queue_create_infos.push_back(queue_create_info);
		}

		// physical device features
		VkPhysicalDeviceFeatures physical_device_features = {};
		physical_device_features.samplerAnisotropy = VK_TRUE;

		// support inefficient readback storage buffer
		physical_device_features.fragmentStoresAndAtomics = VK_TRUE;

		// support independent blending
		physical_device_features.independentBlend = VK_TRUE;

		// support geometry shader
		physical_device_features.geometryShader = VK_TRUE;

		// device create info
		VkDeviceCreateInfo device_create_info{};
		device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		device_create_info.pQueueCreateInfos = queue_create_infos.data();
		device_create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
		device_create_info.pEnabledFeatures = &physical_device_features;
		device_create_info.enabledExtensionCount = static_cast<uint32_t>(m_device_extensions.size());
		device_create_info.ppEnabledExtensionNames = m_device_extensions.data();
		device_create_info.enabledLayerCount = 0;

		if (vkCreateDevice(m_physical_device, &device_create_info, nullptr, &m_device) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create logical device!");
		}

		// initialize queues of this device
		vkGetDeviceQueue(m_device, m_queue_indices.m_graphics_family.value(), 0, &m_graphics_queue);
		vkGetDeviceQueue(m_device, m_queue_indices.m_present_family.value(), 0, &m_present_queue);

	}

	VkResult VulkanRHI::CreateDebugUtilsMessengerEXT(VkInstance instance,
													const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
													const VkAllocationCallbacks* pAllocator,
													VkDebugUtilsMessengerEXT* pDebugMessenger)
	{
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr)
		{
			return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
		}
		else
		{
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	void VulkanRHI::DestroyDebugUtilsMessengerEXT(VkInstance instance,
												  VkDebugUtilsMessengerEXT debugMessenger,
												  const VkAllocationCallbacks* pAllocator)
	{
		auto func =
			(PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr)
		{
			func(instance, debugMessenger, pAllocator);
		}
	}

	ME::QueueFamilyIndices VulkanRHI::FindQueueFamilies(VkPhysicalDevice physical_device)
	{
		QueueFamilyIndices indices;
		uint32_t queue_family_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);
		std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
		vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families.data());

		int i = 0;
		for (const auto& queue_family : queue_families)
		{
			if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) // if support graphics command queue
			{
				indices.m_graphics_family = i;
			}

			VkBool32 is_present_support = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, m_surface, &is_present_support);

			if (is_present_support)
			{
				indices.m_present_family = i;
			}

			if (indices.IsComplete())
			{
				break;
			}
			i++;
		}

		return indices;
	}

	bool VulkanRHI::CheckDeviceExtensionSupport(VkPhysicalDevice physical_device)
	{
		uint32_t extension_count;
		vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, nullptr);

		std::vector<VkExtensionProperties> available_extensions(extension_count);
		vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, available_extensions.data());

		std::set<std::string> required_extensions(m_device_extensions.begin(), m_device_extensions.end());
		for (const auto& extension : available_extensions)
		{
			required_extensions.erase(extension.extensionName);
		}

		return required_extensions.empty();
	}

	bool VulkanRHI::IsDeviceSuitable(VkPhysicalDevice physical_device)
	{
		auto queue_indices = FindQueueFamilies(physical_device);
		bool is_extensions_supported = CheckDeviceExtensionSupport(physical_device);
		bool is_swapchain_adequate = false;
		if (is_extensions_supported)
		{
			SwapChainSupportDetails swapchain_support_details = QuerySwapchainSupport(physical_device);
			is_swapchain_adequate = !swapchain_support_details.m_formats.empty() && !swapchain_support_details.m_presentModes.empty();
		}

		VkPhysicalDeviceFeatures physical_device_features;
		vkGetPhysicalDeviceFeatures(physical_device, &physical_device_features);

		if (!queue_indices.IsComplete() || !is_swapchain_adequate || !physical_device_features.samplerAnisotropy)
		{
			return false;
		}

		return true;
	}

	ME::SwapChainSupportDetails VulkanRHI::QuerySwapchainSupport(VkPhysicalDevice physical_device)
	{
		SwapChainSupportDetails details_result;

		// capabilities
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, m_surface, &details_result.m_capabilities);

		// formats
		uint32_t format_count;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, m_surface, &format_count, nullptr);
		if (format_count != 0)
		{
			details_result.m_formats.resize(format_count);
			vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, m_surface, &format_count, details_result.m_formats.data());
		}

		// present mode
		uint32_t presentmode_count;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, m_surface, &presentmode_count, nullptr);
		if (presentmode_count != 0)
		{
			details_result.m_presentModes.resize(presentmode_count);
			vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, m_surface, &presentmode_count, details_result.m_presentModes.data());
		}

		return details_result;
		
	}

}