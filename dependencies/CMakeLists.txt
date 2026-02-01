cmake_minimum_required(VERSION 3.15)
project(AuroraeDependencies)

# Find Vulkan SDK (required by volk and vk-bootstrap)
find_package(Vulkan REQUIRED COMPONENTS shaderc_combined)
message("Vulkan include dirs: ${Vulkan_INCLUDE_DIRS}")

# Prevent multiple uninstall targets from being created
# Some dependencies create uninstall targets, which conflict when using add_subdirectory
set(CMAKE_SKIP_INSTALL_ALL ON)

# Configure each dependency with appropriate flags
set(VOLK_INSTALL OFF)
set(VK_BOOTSTRAP_TEST OFF)
set(VK_BOOTSTRAP_INSTALL OFF)
set(SPDLOG_MASTER_PROJECT OFF)
set(SPDLOG_USE_STD_FORMAT ON)
set(SPDLOG_INSTALL OFF)
set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_DOCS OFF)
set(GLFW_INSTALL OFF)
set(VMA_ENABLE_INSTALL OFF)
set(VMA_BUILD_SAMPLES OFF)
set(VMA_BUILD_DOCUMENTATION OFF)
set(GLM_BUILD_TESTS OFF)
set(BUILD_SHARED_LIBS OFF)
set(GLM_BUILD_INSTALL OFF)
set(GLM_ENABLE_SIMD_AVX2 ON)
set(ASSIMP_BUILD_TESTS OFF)
set(ASSIMP_WARNINGS_AS_ERRORS OFF)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT ON)
set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT ON)
set(ASSIMP_INSTALL OFF)
set(ASSIMP_INSTALL_PDB OFF)
set(ASSIMP_INJECT_DEBUG_POSTFIX ON)
set(CMAKE_DEBUG_POSTFIX "d")
set(ASSIMP_BUILD_USD_IMPORTER OFF)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF)
set(ASSIMP_BUILD_SAMPLES OFF)
set(glaze_BUILD_EXAMPLES OFF)
set(glaze_ENABLE_FUZZING OFF)
set(BUILD_TESTING OFF)
set(MUUID_NO_TESTS ON)

# Add all dependencies
add_subdirectory(glfw)
add_subdirectory(modern-uuid)
add_subdirectory(spdlog)
add_subdirectory(volk)
add_subdirectory(vk-bootstrap)
add_subdirectory(VulkanMemoryAllocator)
add_subdirectory(glm)
add_subdirectory(assimp)
add_subdirectory(glaze)

# Create static library combining all dependencies
add_library(AuroraeDependencies STATIC dummy.cpp)

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

foreach(lib IN LISTS SPIRV_CROSS_LIBS)
    add_library(${lib} STATIC IMPORTED)
    set_target_properties(${lib} PROPERTIES
            IMPORTED_LOCATION "${SPIRV_CROSS_LIB_DIR}/${lib}d.lib"
    )
endforeach()

target_link_libraries(AuroraeDependencies PUBLIC
    glfw
    modern-uuid-static
    spdlog::spdlog
    volk
    assimp
    glaze::glaze
    Vulkan::Vulkan
    Vulkan::shaderc_combined
    ${SPIRV_CROSS_LIBS}
)

target_include_directories(AuroraeDependencies PUBLIC
    $<TARGET_PROPERTY:glfw,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:modern-uuid-static,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:spdlog::spdlog,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:volk,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:vk-bootstrap,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:VulkanMemoryAllocator,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:glm,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:assimp,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:glaze::glaze,INTERFACE_INCLUDE_DIRECTORIES>
)

# Install the static library
install(TARGETS AuroraeDependencies
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# Install all built dependency libraries
install(TARGETS glfw modern-uuid-static spdlog assimp volk
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# Also copy zlib and vk-bootstrap from their builds
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/assimp/contrib/zlib/Debug/zlibstaticd.lib"
    DESTINATION lib
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/vk-bootstrap/Debug/vk-bootstrapd.lib"
    DESTINATION lib
)

# Collect and install all dependency headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/glfw/include/GLFW DESTINATION include)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/modern-uuid/inc/modern-uuid DESTINATION include)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/spdlog/include/spdlog DESTINATION include)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/glaze/include/ DESTINATION include)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/volk/volk.h DESTINATION include/volk/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/volk/volk.c DESTINATION include/volk/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/vk-bootstrap/src/VkBootstrap.h DESTINATION include/vk-bootstrap)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/vk-bootstrap/src/VkBootstrapDispatch.h DESTINATION include/vk-bootstrap)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/vk-bootstrap/src/VkBootstrapFeatureChain.h DESTINATION include/vk-bootstrap)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/VulkanMemoryAllocator/include/vk_mem_alloc.h DESTINATION include/VulkanMemoryAllocator)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/glm/glm DESTINATION include)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/assimp/include/assimp DESTINATION include)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/assimp/include/assimp/config.h DESTINATION include/assimp)

# Generate and install config files for find_package
include(CMakePackageConfigHelpers)

set(CMAKE_INSTALL_LIBDIR "lib")
set(CMAKE_INSTALL_INCLUDEDIR "include")

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/AuroraeDependenciesConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/AuroraeDependenciesConfig.cmake
    INSTALL_DESTINATION lib/cmake/AuroraeDependencies
    PATH_VARS CMAKE_INSTALL_LIBDIR CMAKE_INSTALL_INCLUDEDIR
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/AuroraeDependenciesTargets.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/AuroraeDependenciesTargets.cmake
    INSTALL_DESTINATION lib/cmake/AuroraeDependencies
    PATH_VARS CMAKE_INSTALL_LIBDIR CMAKE_INSTALL_INCLUDEDIR
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/AuroraeDependenciesConfigVersion.cmake
    VERSION 1.0.0
    COMPATIBILITY ExactVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/AuroraeDependenciesConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/AuroraeDependenciesConfigVersion.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/AuroraeDependenciesTargets.cmake
    DESTINATION lib/cmake/AuroraeDependencies
)