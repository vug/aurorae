cmake_minimum_required(VERSION 3.15)
project(AuroraeSuperbuild NONE) # NONE indicates no languages for this project itself

include(ExternalProject) # Required for ExternalProject_Add

set(SUPERBUILD_CONFIGS "Debug" "Release") # "RelWithDebInfo")
# Where dependencies will be installed for each configuration
set(DEP_INSTALL_BASE_DIR ${CMAKE_BINARY_DIR}/dependencies-install)
message("DEP_INSTALL_BASE_DIR: ${DEP_INSTALL_BASE_DIR}")

# EXTRA_CMAKE_ARGS: Additional arguments for this specific dependency
function(add_external_dependency NAME SOURCE_DIR EXTRA_CMAKE_ARGS)

  foreach(CONFIG ${SUPERBUILD_CONFIGS})
    set(INSTALL_DIR "${DEP_INSTALL_BASE_DIR}/${CONFIG}")
    # message("${NAME}_${CONFIG} INSTALL_DIR: ${INSTALL_DIR}")

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
      CMAKE_ARGS ${COMMON_EP_ARGS} ${EXTRA_CMAKE_ARGS} -DCMAKE_BUILD_TYPE=${CONFIG}
      BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --config ${CONFIG}
      INSTALL_COMMAND ${CMAKE_COMMAND} --install <BINARY_DIR> --config ${CONFIG} --prefix ${INSTALL_DIR}
      BUILD_ALWAYS OFF # Only rebuild if source changes
    )

    set(${NAME}_INSTALL_DIR_${CONFIG} ${INSTALL_DIR} PARENT_SCOPE)

    if(CONFIG STREQUAL "Debug" OR CONFIG STREQUAL "RelWithDebInfo")
      ExternalProject_Add_Step(${NAME}_${CONFIG} copy_pdb
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_LIST_DIR}/copy_pdb.cmake
          "<BINARY_DIR>/${CONFIG}"
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
add_external_dependency(Volk "volk" "-DVOLK_INSTALL=ON")
add_external_dependency(VKBootstrap "vk-bootstrap" "-DVK_BOOTSTRAP_TEST=OFF;-DVK_BOOTSTRAP_INSTALL=ON")
# git submodule add https://github.com/gabime/spdlog.git
add_external_dependency(SpdLog "spdlog" "-DSPDLOG_MASTER_PROJECT=OFF;-DSPDLOG_INSTALL=ON;-DSPDLOG_USE_STD_FORMAT=ON;-DCMAKE_INSTALL_INCLUDEDIR=include")

# TODO(vug): add GLFW
# TODO(vug): bring OpenImageIO but reduce the features/dependencies so that I can compile (just need texture stuff) 
# TODO(vug): assimp, imgui, glm, stb, xatlas, embree

add_custom_target(build_all_deps)
foreach(CONFIG ${SUPERBUILD_CONFIGS})
    # Add a dependency for each named external project and its config
    add_dependencies(build_all_deps
        # GLFW_${CONFIG}
        Volk_${CONFIG}
        VKBootstrap_${CONFIG}
        SpdLog_${CONFIG}
        # VMA_${CONFIG}
        # ImGui_${CONFIG}
        # STB_${CONFIG}
    )
endforeach()
