cmake_minimum_required(VERSION 3.15)
project(Aurorae LANGUAGES CXX)


# --- Dependency Location ---
# This variable should point to the root directory where your superbuild
# installed the dependencies. For example: "C:/Users/veliu/repos/aurorae/build/dependencies-install"
# You can set this when configuring CMake:
# cmake -S . -B build -DAURORAE_DEPENDENCIES_DIR:PATH=/path/to/your/dependencies-install
set(AURORAE_DEPENDENCIES_DIR "" CACHE PATH "Root directory of prebuilt Aurorae dependencies")

if (NOT AURORAE_DEPENDENCIES_DIR)
    message(FATAL_ERROR "AURORAE_DEPENDENCIES_DIR is not set. "
            "Please specify the path to the prebuilt dependencies installation directory "
            "(e.g., -DAURORAE_DEPENDENCIES_DIR:PATH=/path/to/aurorae/build/dependencies-install).")
endif ()

get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
message(STATUS "Generator is multi-config: ${is_multi_config}")

# The superbuild installs dependencies into configuration-specific subdirectories
# (e.g., ${AURORAE_DEPENDENCIES_DIR}/Debug, ${AURORAE_DEPENDENCIES_DIR}/Release).
# The *Config.cmake files (like VolkConfig.cmake) are located within these subdirectories.
# CMAKE_BUILD_TYPE is set by our CMakePresets.json (and passed on the command line, as seen in your log)
# for both Debug and Release configurations, and is available during this configure step.

if (NOT CMAKE_BUILD_TYPE)
    # This check is a fallback. Presets should ensure CMAKE_BUILD_TYPE is set.
    # If it's not set, we cannot reliably find dependencies installed in config-specific dirs.
    message(FATAL_ERROR "CMAKE_BUILD_TYPE is not set. It is required to locate dependencies "
            "in configuration-specific subdirectories (e.g., Debug, Release). "
            "Please ensure your CMake preset or command line sets CMAKE_BUILD_TYPE "
            "(e.g., -DCMAKE_BUILD_TYPE=Debug).")
endif ()

# Add the path to the specific configuration's installation directory.
# For example, if AURORAE_DEPENDENCIES_DIR is ".../dependencies-install" and CMAKE_BUILD_TYPE is "Debug",
# this adds ".../dependencies-install/Debug" to CMAKE_PREFIX_PATH.
# find_package(Volk) will then search in paths like ".../dependencies-install/Debug/lib/cmake/volk".
list(APPEND CMAKE_PREFIX_PATH "${AURORAE_DEPENDENCIES_DIR}/${CMAKE_BUILD_TYPE}")
message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")

# --- Find Dependencies ---
# Find Vulkan (usually provided by the SDK)
#find_package(Vulkan REQUIRED)
#message(STATUS "Found Vulkan: ${Vulkan_LIBRARIES}")
#get_target_property(VULKAN_LIBRARY Vulkan::Vulkan LOCATION)
#message(STATUS "Vulkan target location: ${VULKAN_LIBRARY}")

find_package(Volk REQUIRED)
get_target_property(VOLK_LIBRARY volk::volk LOCATION)
message(STATUS "Volk target location: ${VOLK_LIBRARY}")
get_target_property(VOLK_INCLUDE_DIR volk::volk INTERFACE_INCLUDE_DIRECTORIES)
message(STATUS "Volk include dirs: ${VOLK_INCLUDE_DIR}")

find_package(vk-bootstrap REQUIRED)
get_target_property(VK_BOOTSTRAP_LIBRARY vk-bootstrap::vk-bootstrap LOCATION)
message(STATUS "vk-bootstrap target location: ${VK_BOOTSTRAP_LIBRARY}")
get_target_property(VK_BOOTSTRAP_INCLUDE_DIR vk-bootstrap::vk-bootstrap INTERFACE_INCLUDE_DIRECTORIES)
message(STATUS "vk-bootstrap include dirs: ${VK_BOOTSTRAP_INCLUDE_DIR}")

find_package(spdlog REQUIRED)
get_target_property(SPDLOG_LIBRARY spdlog::spdlog LOCATION)
message(STATUS "spdlog target location: ${SPDLOG_LIBRARY}")
get_target_property(SPDLOG_INCLUDE_DIR spdlog::spdlog INTERFACE_INCLUDE_DIRECTORIES)
message(STATUS "spdlog include dirs: ${SPDLOG_INCLUDE_DIR}")

find_package(glfw3 REQUIRED)
get_target_property(GLFW_LIBRARY glfw LOCATION)
message(STATUS "glfw target location: ${GLFW_LIBRARY}")
get_target_property(GLFW_INCLUDE_DIR glfw INTERFACE_INCLUDE_DIRECTORIES)
message(STATUS "glfw include dirs: ${GLFW_INCLUDE_DIR}")

find_package(glm REQUIRED)
get_target_property(GLM_LIBRARY glm::glm LOCATION)
message(STATUS "glm target location: ${GLM_LIBRARY}")
get_target_property(GLM_INCLUDE_DIR glm::glm INTERFACE_INCLUDE_DIRECTORIES)
message(STATUS "glm include dirs: ${GLM_INCLUDE_DIR}")

find_package(VulkanMemoryAllocator CONFIG REQUIRED)

# TODO(vug): add a function that takes package name and target name.
# target name can be found in the *Config.cmake file.

add_executable(Aurorae
        main.cpp
        Utils.h Utils.cpp
        Logger.h Logger.cpp
        FileIO.h FileIO.cpp
        Application.h Application.cpp
        GlfwUtils.h GlfwUtils.cpp
        Window.h Window.cpp
        VulkanContext.h VulkanContext.cpp
        VulkanWrappers.h VulkanWrappers.cpp
        Swapchain.h Swapchain.cpp
        Renderer.h Renderer.cpp
        AppContext.h AppContext.cpp
        Pipelines.h Pipelines.cpp
        Pipeline.h
        Resources/Allocator.h Resources/Allocator.cpp
        Resources/Buffer.h Resources/Buffer.cpp
        Resources/DescriptorPool.h Resources/DescriptorPool.cpp
        Resources/DescriptorSet.h Resources/DescriptorSet.cpp
        Resources/DescriptorSetLayout.h Resources/DescriptorSetLayout.cpp
        Resources/PipelineLayout.h Resources/PipelineLayout.cpp
)

target_compile_features(Aurorae PRIVATE cxx_std_23)
message(STATUS "EXTERNAL: ${PROJECT_SOURCE_DIR}/../superbuild")
target_compile_options(Aurorae PRIVATE /W4 /external:I${PROJECT_SOURCE_DIR}/../superbuild /external:W0)
target_compile_definitions(Aurorae PRIVATE SPDLOG_USE_STD_FORMAT SPDLOG_USE_STD_SOURCE_LOCATION)

target_link_libraries(Aurorae PRIVATE
        Vulkan::Vulkan
        volk::volk
        vk-bootstrap::vk-bootstrap
        spdlog::spdlog
        # spdlog$<$<CONFIG:Debug>:d> # alternative
        glfw
        GPUOpen::VulkanMemoryAllocator
        glm::glm
)

if (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    target_compile_definitions(Aurorae PRIVATE REL_WITH_DEBUG_INFO_BUILD)
elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(Aurorae PRIVATE RELEASE_BUILD)
elseif (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(Aurorae PRIVATE DEBUG_BUILD)
else ()
    message(FATAL_ERROR "Unsupported CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}. "
            "Add macro for this build type .")
endif ()

target_compile_definitions(Aurorae PRIVATE ASSETS_FOLDER="${PROJECT_SOURCE_DIR}/assets")