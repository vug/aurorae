cmake_minimum_required(VERSION 3.15)
project(Aurorae)

# Point to the install folder where dependencies are installed
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/install)

find_package(AuroraeDependencies REQUIRED CONFIG)

# Add your application subdirectory
add_subdirectory(src)
