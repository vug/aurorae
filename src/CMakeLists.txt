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
    message(FATAL_ERROR "CMAKE_BUILD_TYPE is not set."
            "Please ensure your CMake preset or command line sets CMAKE_BUILD_TYPE "
            "(e.g., -DCMAKE_BUILD_TYPE=Debug).")
endif ()

if (NOT SUBFOLDER)
    message(FATAL_ERROR "SUBFOLDER is not set. It is required to locate dependencies "
            "in configuration-specific subdirectories (e.g., Debug, DebugAsan, Release). "
            "Please ensure your CMake Preset or CMake CLI command sets SUBFOLDER, e.g. -DSUBFOLDER=DebugAsan")
endif ()

# Add the path to the specific configuration's installation directory.
# For example, if AURORAE_DEPENDENCIES_DIR is ".../dependencies-install" and SUBFOLDER is "Debug",
# this adds ".../dependencies-install/Debug" to CMAKE_PREFIX_PATH.
# find_package(Volk) will then search in paths like ".../dependencies-install/Debug/lib/cmake/volk".
list(APPEND CMAKE_PREFIX_PATH "${AURORAE_DEPENDENCIES_DIR}/${SUBFOLDER}")
message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")

# --- Find Dependencies ---
# Find Vulkan (usually provided by the SDK)
find_package(Vulkan REQUIRED COMPONENTS shaderc_combined)

# The cmake config files for SPIRV-Cross library that comes with VulkanSDK are broken
# Here we'll define a static library for each lib file.
# Note that they all use the same header directory
set(VULKAN_SDK_ROOT "$ENV{VULKAN_SDK}")
set(SPIRV_CROSS_INCLUDE_DIR "${VULKAN_SDK_ROOT}/Include/spirv_cross")
set(SPIRV_CROSS_LIB_DIR "${VULKAN_SDK_ROOT}/Lib/lib")
set(SPIRV_CROSS_LIBS_RELEASE
        spirv-cross-core
        spirv-cross-glsl
        spirv-cross-hlsl
        spirv-cross-msl
        spirv-cross-reflect
        spirv-cross-cpp
        spirv-cross-c
)
set(SPIRV_CROSS_LIBS_DEBUG
        spirv-cross-cored
        spirv-cross-glsld
        spirv-cross-hlsld
        spirv-cross-msld
        spirv-cross-reflectd
        spirv-cross-cppd
        spirv-cross-cd
)

# Select the libs based on build type
if (CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo" OR CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
    set(SPIRV_CROSS_LIBS ${SPIRV_CROSS_LIBS_RELEASE})
elseif (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(SPIRV_CROSS_LIBS ${SPIRV_CROSS_LIBS_DEBUG})
else ()
    message(FATAL_ERROR "Unsupported CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
endif ()
foreach (lib IN LISTS SPIRV_CROSS_LIBS)
    add_library(${lib} STATIC IMPORTED)
    set_target_properties(${lib} PROPERTIES
            IMPORTED_LOCATION "${SPIRV_CROSS_LIB_DIR}/${lib}.lib"
            INTERFACE_INCLUDE_DIRECTORIES "${SPIRV_CROSS_INCLUDE_DIR}"
    )
endforeach ()

#message(STATUS "Vulkan include dirs: ${Vulkan_INCLUDE_DIRS}")
#message(STATUS "Found Vulkan: ${Vulkan_LIBRARIES}")
#get_target_property(VULKAN_LIBRARY Vulkan::Vulkan LOCATION)
#message(STATUS "Vulkan target location: ${VULKAN_LIBRARY}")

find_package(Volk REQUIRED)
get_target_property(VOLK_LIBRARY volk::volk LOCATION)
message(STATUS "Volk target location: ${VOLK_LIBRARY}")
get_target_property(VOLK_INCLUDE_DIR volk::volk INTERFACE_INCLUDE_DIRECTORIES)
message(STATUS "Volk include dirs: ${VOLK_INCLUDE_DIR}")
message(STATUS "Vulkan include dirs: ${Vulkan_INCLUDE_DIRS}")


find_package(vk-bootstrap REQUIRED)
find_package(spdlog REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(assimp REQUIRED)
find_package(glaze REQUIRED)
find_package(modern-uuid REQUIRED)

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
        Pipeline.h Pipeline.cpp
        Resources/VulkanResource.h
        Resources/Allocator.h Resources/Allocator.cpp
        Resources/Buffer.h Resources/Buffer.cpp
        Resources/DescriptorPool.h Resources/DescriptorPool.cpp
        Resources/DescriptorSet.h Resources/DescriptorSet.cpp
        Resources/DescriptorSetLayout.h Resources/DescriptorSetLayout.cpp
        Resources/PipelineLayout.h Resources/PipelineLayout.cpp
        Resources/PipelineCache.h Resources/PipelineCache.cpp
        Resources/ShaderModule.h Resources/ShaderModule.cpp
        Vertex.h Vertex.cpp
        Handle.h Handle.cpp
        asset/AssimpUtils.h asset/AssimpUtils.cpp
        asset/AssetProcessor.h asset/AssetProcessor.cpp
        asset/AssetManager.h asset/AssetManager.cpp
        asset/Shader.h asset/Shader.cpp
        asset/Material.h asset/Material.cpp
        asset/Mesh.h asset/Mesh.cpp
        render/Shader.h render/Shader.cpp
        render/Material.h render/Material.cpp
        render/Mesh.h render/Mesh.cpp
        asset/Common.h
        asset/ShaderStage.cpp
        asset/ShaderStage.h
)

target_compile_features(Aurorae PRIVATE cxx_std_23)
message(STATUS "EXTERNAL: ${PROJECT_SOURCE_DIR}/../superbuild")
target_compile_options(Aurorae PRIVATE /W4 /external:I${PROJECT_SOURCE_DIR}/../superbuild /external:W0)
target_compile_definitions(Aurorae PRIVATE
        SPDLOG_USE_STD_FORMAT SPDLOG_USE_STD_SOURCE_LOCATION
        GLM_FORCE_DEPTH_ZERO_TO_ONE #GLM_FORCE_LEFT_HANDED
)

target_link_libraries(Aurorae PRIVATE
        Vulkan::Vulkan
        volk::volk
        vk-bootstrap::vk-bootstrap
        spdlog::spdlog
        # spdlog$<$<CONFIG:Debug>:d> # alternative
        glfw
        GPUOpen::VulkanMemoryAllocator
        glm::glm
        assimp::assimp
        Vulkan::shaderc_combined
        ${SPIRV_CROSS_LIBS}
        glaze::glaze
        modern-uuid::modern-uuid
)
# shaderc_combined does not come with PDB files, causing warnings at linking in debug mode
target_link_options(Aurorae PRIVATE "/ignore:4099")


if (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    target_compile_definitions(Aurorae PRIVATE REL_WITH_DEBUG_INFO_BUILD)
elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(Aurorae PRIVATE RELEASE_BUILD)
elseif (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(Aurorae PRIVATE DEBUG_BUILD)
else ()
    message(FATAL_ERROR "Unsupported CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}. "
            "Add macro for this build type  .")
endif ()

if (SUBFOLDER STREQUAL "DebugAsan")
    message(STATUS "AddressSanitizer is enabled, configuring post-build step to copy runtime DLL.")

    # Find the directory where the MSVC compiler (cl.exe) is located.
    get_filename_component(MSVC_COMPILER_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
    message(STATUS "Compiler directory: ${MSVC_COMPILER_DIR}")
    # C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64

    # The ASan runtime DLL is in the same directory as the compiler.
    set(ASAN_RUNTIME_DLL "${MSVC_COMPILER_DIR}/clang_rt.asan_dynamic-x86_64.dll")

    # Add a custom command that runs after the 'Aurorae' executable is built.
    # This command copies the ASan DLL to the same directory as the executable.
    add_custom_command(
            TARGET Aurorae
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${ASAN_RUNTIME_DLL}"
            "$<TARGET_FILE_DIR:Aurorae>"
            COMMENT "Copying MSVC AddressSanitizer runtime DLL to output directory..."
    )

endif ()

target_compile_definitions(Aurorae PRIVATE ASSETS_FOLDER="${PROJECT_SOURCE_DIR}/assets")