cmake_minimum_required (VERSION 3.8)

project (HelloVulkan)

find_package(Vulkan REQUIRED)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED on)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")

add_subdirectory("${PROJECT_SOURCE_DIR}/External")
add_subdirectory("${PROJECT_SOURCE_DIR}/HelloVulkan")


#[[
target_link_libraries(HelloVulkan PUBLIC
	"$(VULKAN_SDK)/Lib/GenericCodeGen.lib"
	"$(VULKAN_SDK)/Lib/glslang.lib"
	"$(VULKAN_SDK)/Lib/glslang-default-resource-limits.lib"
	"$(VULKAN_SDK)/Lib/MachineIndependent.lib"
	"$(VULKAN_SDK)/Lib/SPIRV.lib"
	"$(VULKAN_SDK)/Lib/SPIRV-Tools.lib"
	"$(VULKAN_SDK)/Lib/SPIRV-Tools-opt.lib"
	"$(VULKAN_SDK)/Lib/OSDependent.lib"
)
]]

#target_link_libraries(HelloVulkan PUBLIC glfw assimp volk tracy)

#[[
SET(GLFW_INSTALL            OFF CACHE BOOL "" FORCE) 
SET(GLFW_BUILD_DOCS         OFF CACHE BOOL "" FORCE) 
SET(GLFW_BUILD_TESTS        OFF CACHE BOOL "" FORCE) 
SET(GLFW_BUILD_EXAMPLES     OFF CACHE BOOL "" FORCE) 
]]

#add_subdirectory("${PROJECT_SOURCE_DIR}/External/glfw")
#add_subdirectory("${PROJECT_SOURCE_DIR}/External/assimp")

#[[
add_library(glfw STATIC)
add_library(assimp STATIC)
add_library(volk STATIC)
add_library(tracy STATIC)
add_library(imgui STATIC)
]]