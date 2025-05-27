cmake_minimum_required(VERSION 3.15)
project(Aurorae LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# --- Dependency Location ---
# This variable should point to the root directory where your superbuild
# installed the dependencies. For example: "C:/Users/veliu/repos/aurorae/build/dependencies-install"
# You can set this when configuring CMake:
# cmake -S . -B build -DAURORAE_DEPENDENCIES_DIR:PATH=/path/to/your/dependencies-install
set(AURORAE_DEPENDENCIES_DIR "" CACHE PATH "Root directory of prebuilt Aurorae dependencies")

if(NOT AURORAE_DEPENDENCIES_DIR)
    message(FATAL_ERROR "AURORAE_DEPENDENCIES_DIR is not set. "
                        "Please specify the path to the prebuilt dependencies installation directory "
                        "(e.g., -DAURORAE_DEPENDENCIES_DIR:PATH=/path/to/aurorae/build/dependencies-install).")
endif()

# --- Configure CMAKE_PREFIX_PATH ---
# This helps find_package locate the dependencies.
get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
message(STATUS "Generator is multi-config: ${is_multi_config}")

# The superbuild installs dependencies into configuration-specific subdirectories
# (e.g., ${AURORAE_DEPENDENCIES_DIR}/Debug, ${AURORAE_DEPENDENCIES_DIR}/Release).
# The *Config.cmake files (like VolkConfig.cmake) are located within these subdirectories.
# CMAKE_BUILD_TYPE is set by our CMakePresets.json (and passed on the command line, as seen in your log)
# for both Debug and Release configurations, and is available during this configure step.

if(NOT CMAKE_BUILD_TYPE)
  # This check is a fallback. Presets should ensure CMAKE_BUILD_TYPE is set.
  # If it's not set, we cannot reliably find dependencies installed in config-specific dirs.
  message(FATAL_ERROR "CMAKE_BUILD_TYPE is not set. It is required to locate dependencies "
                      "in configuration-specific subdirectories (e.g., Debug, Release). "
                      "Please ensure your CMake preset or command line sets CMAKE_BUILD_TYPE "
                      "(e.g., -DCMAKE_BUILD_TYPE=Debug).")
endif()

# Add the path to the specific configuration's installation directory.
# For example, if AURORAE_DEPENDENCIES_DIR is ".../dependencies-install" and CMAKE_BUILD_TYPE is "Debug",
# this adds ".../dependencies-install/Debug" to CMAKE_PREFIX_PATH.
# find_package(Volk) will then search in paths like ".../dependencies-install/Debug/lib/cmake/volk".
list(APPEND CMAKE_PREFIX_PATH "${AURORAE_DEPENDENCIES_DIR}/${CMAKE_BUILD_TYPE}")

# --- Find Dependencies ---
# Find Vulkan (usually provided by the SDK)
find_package(Vulkan REQUIRED)
message(STATUS "Found Vulkan: ${Vulkan_LIBRARIES}")

# Find Volk (installed by your superbuild)
# Volk installs VolkConfig.cmake which provides the Volk::volk target.
find_package(Volk REQUIRED)
message(STATUS "Found Volk: ${Volk_LIBRARIES}") # Volk_LIBRARIES might not be set, target is key

# Find VKBootstrap (installed by your superbuild)
# vk-bootstrap installs unofficial-vk-bootstrapConfig.cmake
# which provides the unofficial::vk-bootstrap::vk-bootstrap target.
find_package(vk-bootstrap REQUIRED)
message(STATUS "Found vk-bootstrap: ${vk-bootstrap_LIBRARIES}")


# --- Your Application Target ---
add_executable(Aurorae main.cpp)

# --- Link Dependencies ---
target_link_libraries(Aurorae PRIVATE
    Vulkan::Vulkan                      # From find_package(Vulkan)
    volk::volk                          # From find_package(Volk)
    vk-bootstrap::vk-bootstrap # From find_package(unofficial-vk-bootstrap)
)

# The find_package calls for Volk and unofficial-vk-bootstrap should correctly
# set up include directories via their imported targets, thanks to how you
# installed them with `CMAKE_INSTALL_INCLUDEDIR=include/${SOURCE_DIR}`.
# So, you can directly use:
# #include <volk/volk.h>
# #include <vk-bootstrap/VkBootstrap.h>

# --- Optional: Ensure PDBs are found by MSVC for easier debugging ---
if(MSVC)
    # For Debug and RelWithDebInfo builds, ensure the linker can find PDBs
    # This is often handled automatically if PDBs are next to LIBs,
    # but explicit flags can sometimes help certain setups or older VS versions.
    # Modern VS usually handles this well if CMAKE_PREFIX_PATH is set correctly.
    # target_link_options(Aurorae PRIVATE
    #   $<$<CONFIG:Debug,RelWithDebInfo>:/DEBUG:FULL>
    # )
endif()

message(STATUS "Aurorae configured. CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")

