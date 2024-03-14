file(GLOB_RECURSE H_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/Header/*.h")
file(GLOB_RECURSE CPP_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/Source/*.cpp")

add_executable(HelloVulkan 
    ${H_FILES}
    ${CPP_FILES}
)

source_group(TREE "${CMAKE_CURRENT_LIST_DIR}/Header" PREFIX "Header Files" FILES ${H_FILES})
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}/Source" PREFIX "Source Files" FILES ${CPP_FILES}) 

target_compile_definitions(HelloVulkan PUBLIC
    _CONSOLE
    _CRT_SECURE_NO_WARNINGS
    TRACY_ENABLE
    GLM_FORCE_DEPTH_ZERO_TO_ONE
)

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
)

target_include_directories(HelloVulkan PUBLIC 
    "$ENV{VULKAN_SDK}/Include"
    "$ENV{VULKAN_SDK}/Include/glslang/Include"
    "$ENV{VULKAN_SDK}/Include/glslang/MachineIndependent"
    "$ENV{VULKAN_SDK}/Include/glslang/Public"
    "$ENV{VULKAN_SDK}/Include/glslang/SPIRV"
    "$ENV{VULKAN_SDK}/Include/spirv_cross"
    "$ENV{VULKAN_SDK}/Include/spirv-tools"
    "$ENV{VULKAN_SDK}/Include/spirv-headers"
)