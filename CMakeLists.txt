cmake_minimum_required (VERSION 3.8)

project (HelloVulkan)

find_package(Vulkan REQUIRED)
find_package(glslang QUIET)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED on)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")

# All external libraries from git modules
add_subdirectory("${PROJECT_SOURCE_DIR}/External")

# The actual source code
add_subdirectory("${PROJECT_SOURCE_DIR}/HelloVulkan")

# Set uninstall target inside CMakePredefinedTargets
set_property(TARGET uninstall PROPERTY FOLDER "CMakePredefinedTargets")