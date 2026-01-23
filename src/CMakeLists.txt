get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
message(STATUS "Generator is multi-config: ${is_multi_config}")

# --- Find Dependencies ---
# Find Vulkan (usually provided by the SDK)
find_package(Vulkan REQUIRED COMPONENTS shaderc_combined)

# Define SPIRV-Cross libraries from Vulkan SDK
set(VULKAN_SDK_ROOT "$ENV{VULKAN_SDK}")
set(SPIRV_CROSS_LIB_DIR "${VULKAN_SDK_ROOT}/Lib")
set(SPIRV_CROSS_LIBS
        spirv-cross-core
        spirv-cross-glsl
        spirv-cross-hlsl
        spirv-cross-msl
        spirv-cross-reflect
        spirv-cross-cpp
        spirv-cross-c
)

# TODO(vug): do I need to do this spirv library here too? I think we do the same in dependencies too.
foreach(lib IN LISTS SPIRV_CROSS_LIBS)
    if(NOT TARGET ${lib})
        add_library(${lib} STATIC IMPORTED)
        set_target_properties(${lib} PROPERTIES
                IMPORTED_LOCATION "${SPIRV_CROSS_LIB_DIR}/${lib}d.lib"
        )
    endif()
endforeach()

#message(STATUS "Vulkan include dirs: ${Vulkan_INCLUDE_DIRS}")
#message(STATUS "Found Vulkan: ${Vulkan_LIBRARIES}")
#get_target_property(VULKAN_LIBRARY Vulkan::Vulkan LOCATION)
#message(STATUS "Vulkan target location: ${VULKAN_LIBRARY}")

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
        asset/AssetConcepts.h
        asset/AssetIds.h
        asset/AssetTraits.h
        asset/ShaderReflection.h asset/ShaderReflection.cpp
        asset/ShaderStage.h asset/ShaderStage.cpp
        asset/GraphicsProgram.h asset/AssetGraphicsProgram.cpp
        asset/Material.h asset/AssetMaterial.cpp
        asset/Mesh.h asset/AssetMesh.cpp
        asset/AssetRegistry.h asset/AssetRegistry.cpp
        asset/AssimpUtils.h asset/AssimpUtils.cpp
        asset/AssetProcessor.h asset/AssetProcessor.cpp
        asset/AssetManager.h asset/AssetManager.cpp
        render/RenderConcepts.h
        render/GraphicsProgram.h render/RenderGraphicsProgram.cpp
        render/Material.h render/RenderMaterial.cpp
        render/Mesh.h render/RenderMesh.cpp
        asset/AssetIds.cpp
)

target_compile_features(Aurorae PRIVATE cxx_std_23)
target_compile_options(Aurorae PRIVATE
        /W4 # get lots of warnings
        /external:I${PROJECT_SOURCE_DIR}/../install /external:W0 # ignore warnings from dependencies
        /EHsc # something related to exceptions
        /MP # enable parallel builds
        #/Bt+      # Show time for each compilation phase
        #/d1reportTime  # Template instantiation times
        #/d2cgsummary
        /Zc:enumTypes
)
target_link_options(Aurorae PRIVATE /time)
target_precompile_headers(Aurorae PRIVATE
        # STL
        <array>
        <cassert>
        <cstddef>
        <cstdio>
        <filesystem>
        <format>
        <functional>
        <memory>
        <optional>
        <ranges>
        <source_location>
        <string>
        <string_view>
        <tuple>
        <type_traits>
        <unordered_map>
        <utility>
        <vector>
        # # Dependencies
        # ## Assimp and GLFW files are only included in single CPP files
        # <glaze/glaze/glaze.hpp>
        # <modern-uuid/uuid.h>
        # <glm/gtc/type_ptr.hpp>
        # <glm/mat4x4.hpp>
        # <vk-bootstrap/VkBootstrap.h>
        # <volk/volk.h>
        # <VulkanMemoryAllocator/vk_mem_alloc.h>
        # <vulkan/vulkan_core.h>
        # <vulkan/vulkan.h>
)


target_compile_definitions(Aurorae PRIVATE
        VK_NO_PROTOTYPES
        SPDLOG_USE_STD_FORMAT SPDLOG_USE_STD_SOURCE_LOCATION
        GLM_FORCE_DEPTH_ZERO_TO_ONE #GLM_FORCE_LEFT_HANDED
)

target_link_libraries(Aurorae PRIVATE
        AuroraeDependencies
        Vulkan::Vulkan
        Vulkan::shaderc_combined
        spirv-cross-core
        spirv-cross-glsl
        spirv-cross-hlsl
        spirv-cross-msl
        spirv-cross-reflect
        spirv-cross-cpp
        spirv-cross-c
)
# shaderc_combined does not come with PDB files, causing warnings at linking in debug mode
target_link_options(Aurorae PRIVATE "/ignore:4099")


# if (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
#     target_compile_definitions(Aurorae PRIVATE REL_WITH_DEBUG_INFO_BUILD)
# elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
#     target_compile_definitions(Aurorae PRIVATE RELEASE_BUILD)
# elseif (CMAKE_BUILD_TYPE STREQUAL "Debug")
#     target_compile_definitions(Aurorae PRIVATE DEBUG_BUILD)
# else ()
#     message(FATAL_ERROR "Unsupported CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}. "
#             "Add macro for this build type  .")
# endif ()

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