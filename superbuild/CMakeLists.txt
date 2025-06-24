cmake_minimum_required(VERSION 3.15)
project(AuroraeSuperbuild NONE) # NONE indicates no languages for this project itself

include(ExternalProject) # Required for ExternalProject_Add

set(SUPERBUILD_CONFIGS "Debug" "DebugAsan" "Release") # "RelWithDebInfo")
# Where dependencies will be installed for each configuration
set(DEP_INSTALL_BASE_DIR ${CMAKE_BINARY_DIR}/dependencies-install)
message("DEP_INSTALL_BASE_DIR: ${DEP_INSTALL_BASE_DIR}")

# NAME: The alias of the dependency (e.g., SpdLog)
# SOURCE_DIR: The subdirectory in dependencies/ (e.g., spdlog)
# LIST_OF_DEFINITIONS: A CMake list (semicolon-separated string) of definitions like "KEY=VALUE" or "KEY"
function(add_external_dependency NAME SOURCE_DIR LIST_OF_DEFINITIONS)
  set(FORMATTED_DEFINITIONS "")
  foreach(DEFINITION_ITEM ${LIST_OF_DEFINITIONS})
    # Prepend -D to each definition item
    list(APPEND FORMATTED_DEFINITIONS "-D${DEFINITION_ITEM}")
  endforeach()

  foreach(CONFIG ${SUPERBUILD_CONFIGS})
    set(DEP_CXX_FLAGS "")
    set(DEP_C_FLAGS "")
    set(INNER_BUILD_CONFIG ${CONFIG}) # Default to the loop's config name

    # If the config is our special Asan one, set flags and use "Debug" for the actual build tool.
    if(CONFIG STREQUAL "DebugAsan")
      set(DEP_CXX_FLAGS "/fsanitize=address")
      set(DEP_C_FLAGS "/fsanitize=address") # Also apply to C files, as some dependencies might use them
      set(INNER_BUILD_CONFIG "Debug")
    endif()
    
    set(INSTALL_DIR "${DEP_INSTALL_BASE_DIR}/${CONFIG}")
    message("${NAME}_${CONFIG} INSTALL_DIR: ${INSTALL_DIR}")

    set(COMMON_EP_ARGS
        -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR} # where the dependency will install its files
        -DCMAKE_INSTALL_LIBDIR=lib
        -DCMAKE_INSTALL_INCLUDEDIR=include/${SOURCE_DIR} # Install headers into a subdirectory named after the source
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON # Important for static libraries
        -DCMAKE_VS_PLATFORM_TOOLSET_HOST_ARCHITECTURE=x64
        -DCMAKE_GENERATOR_PLATFORM=x64
    )
    
    set(BINARY_DIR "${CMAKE_BINARY_DIR}/dep_${NAME}_${CONFIG}")
    ExternalProject_Add(${NAME}_${CONFIG}
      SOURCE_DIR "${CMAKE_SOURCE_DIR}/../dependencies/${SOURCE_DIR}" # when dependency is in the repo (maybe via submodules)
      # use GIT_REPOSITORY to make ExternalProject to clone the repo itself
      INSTALL_DIR "${INSTALL_DIR}"
      CMAKE_ARGS ${COMMON_EP_ARGS} ${FORMATTED_DEFINITIONS} -DCMAKE_CXX_FLAGS=${DEP_CXX_FLAGS} -DCMAKE_C_FLAGS=${DEP_C_FLAGS}
      BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --config ${INNER_BUILD_CONFIG}
      INSTALL_COMMAND ${CMAKE_COMMAND} --install <BINARY_DIR> --config ${INNER_BUILD_CONFIG} --prefix ${INSTALL_DIR}
      BUILD_ALWAYS OFF # Only rebuild if source changes
    )

    set(${NAME}_INSTALL_DIR_${CONFIG} ${INSTALL_DIR} PARENT_SCOPE)

    if(INNER_BUILD_CONFIG STREQUAL "Debug" OR INNER_BUILD_CONFIG STREQUAL "RelWithDebInfo")
      ExternalProject_Add_Step(${NAME}_${CONFIG} copy_pdb
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_LIST_DIR}/copy_pdb.cmake
          "<BINARY_DIR>/${INNER_BUILD_CONFIG}"
          "${SOURCE_DIR}"
          "${INSTALL_DIR}/lib"
        DEPENDEES install
        COMMENT "Copying PDB for ${SOURCE_DIR} (${CONFIG}) to ${INSTALL_DIR}/lib/"
      )
    endif()

    # ExternalProject_Get_Property(${NAME}_${CONFIG} BINARY_DIR)
    # message("${NAME}_${CONFIG} BINARY_DIR: ${BINARY_DIR}")
    # ExternalProject_Get_Property(${NAME}_${CONFIG} INSTALL_DIR)
    # message("${NAME}_${CONFIG} INSTALL_DIR: ${INSTALL_DIR}")
  endforeach()
endfunction()

# Vulkan SDK needs to be installed so that volk and vk-bootstrap can use it
find_package(Vulkan REQUIRED)
message("Vulkan include dirs: ${Vulkan_INCLUDE_DIRS}")

# git submodule add https://github.com/gnuradio/volk.git
set(VOLK_DEFS "VOLK_INSTALL=ON")
add_external_dependency(Volk "volk" "${VOLK_DEFS}")

# git submodule add https://github.com/charles-lunarg/vk-bootstrap.git
set(VKBOOTSTRAP_DEFS
  "VK_BOOTSTRAP_TEST=OFF"
  "VK_BOOTSTRAP_INSTALL=ON")
add_external_dependency(VKBootstrap "vk-bootstrap" "${VKBOOTSTRAP_DEFS}")

# git submodule add https://github.com/gabime/spdlog.git
set(SPDLOG_DEFS
  "SPDLOG_MASTER_PROJECT=OFF"
  "SPDLOG_USE_STD_FORMAT=ON"
  "SPDLOG_INSTALL=ON"
  "CMAKE_INSTALL_INCLUDEDIR=include")
add_external_dependency(SpdLog "spdlog" "${SPDLOG_DEFS}")

# git submodule add https://github.com/glfw/glfw.git
set(GLFW_DEFS
  "GLFW_BUILD_EXAMPLES=OFF"
  "GLFW_BUILD_TESTS=OFF"
  "GLFW_BUILD_DOCS=OFF"
  "GLFW_INSTALL=ON")
add_external_dependency(Glfw "glfw" "${GLFW_DEFS}")

# git submodule add https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
set(VMA_DEFS
  "VMA_ENABLE_INSTALL=ON"
  "VMA_BUILD_SAMPLES=OFF"
  "VMA_BUILD_DOCUMENTATION=OFF")
add_external_dependency(VMA "VulkanMemoryAllocator" "${VMA_DEFS}")

# git submodule add https://github.com/g-truc/glm.git
set(GLM_DEFS
  "GLM_BUILD_TESTS=OFF"
  "BUILD_SHARED_LIBS=OFF"
  "GLM_BUILD_INSTALL=ON"
  "GLM_ENABLE_SIMD_AVX2=ON")
add_external_dependency(GLM "glm" "${GLM_DEFS}")

# TODO(vug): bring OpenImageIO but reduce the features/dependencies so that I can compile (just need texture stuff) 
# TODO(vug): assimp, imgui, stb, xatlas, embree

add_custom_target(build_all_deps)
foreach(CONFIG ${SUPERBUILD_CONFIGS})
    add_dependencies(build_all_deps
        Volk_${CONFIG}
        VKBootstrap_${CONFIG}
        SpdLog_${CONFIG}
        Glfw_${CONFIG}
        VMA_${CONFIG}
        GLM_${CONFIG}
        # Vulkan_${CONFIG}}
    )
endforeach()
