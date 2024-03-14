cmake_minimum_required (VERSION 3.8)

project (HelloVulkan)

find_package(Vulkan REQUIRED)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED on)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")

add_subdirectory("${PROJECT_SOURCE_DIR}/HelloVulkan")

#[[
target_compile_definitions(HelloVulkan PUBLIC
    _CONSOLE
    _CRT_SECURE_NO_WARNINGS
    TRACY_ENABLE
    GLM_FORCE_DEPTH_ZERO_TO_ONE
)
]]

#[[
target_include_directories(HelloVulkan PUBLIC 
    "${PROJECT_SOURCE_DIR}/External/stb"
    "${PROJECT_SOURCE_DIR}/External/assimp/include"
    "${PROJECT_SOURCE_DIR}/External/glfw/include"
    "${PROJECT_SOURCE_DIR}/External/glm"
    "${PROJECT_SOURCE_DIR}/External/gli"
    "${PROJECT_SOURCE_DIR}/External/ImGui"
    "${PROJECT_SOURCE_DIR}/External/ImGui/backends"
    "${PROJECT_SOURCE_DIR}/External/volk"
    "${PROJECT_SOURCE_DIR}/External/VulkanMemoryAllocator/include"
    "${PROJECT_SOURCE_DIR}/External/tracy/public/tracy"
    "${PROJECT_SOURCE_DIR}/External/Binaries/assimp/include"

    "$(VULKAN_SDK)/Include"
    "$(VULKAN_SDK)/Include/glslang/Include"
    "$(VULKAN_SDK)/Include/glslang/MachineIndependent"
    "$(VULKAN_SDK)/Include/glslang/Public"
    "$(VULKAN_SDK)/Include/glslang/SPIRV"
    "$(VULKAN_SDK)/Include/spirv_cross"
    "$(VULKAN_SDK)/Include/spirv-tools"
    "$(VULKAN_SDK)/Include/spirv-headers"
)
]]

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

target_compile_definitions(imgui PRIVATE VK_NO_PROTOTYPES)

target_sources(imgui PRIVATE 
	"External/ImGui/imgui.h"
	"External/ImGui/backends/ImGui/imgui_impl_vulkan.h"
	"External/ImGui/backends/ImGui/imgui_impl_glfw.h"
	"External/ImGui/imgui.cpp"

	"External/ImGui/imgui_demo.cpp"
	"External/ImGui/imgui_draw.cpp"
	"External/ImGui/imgui_widgets.cpp"
	"External/ImGui/imgui_tables.cpp"
	"External/ImGui/backends/imgui_impl_vulkan.cpp"
	"External/ImGui/backends/imgui_impl_glfw.cpp"
)
]]