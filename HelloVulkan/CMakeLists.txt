file(GLOB HEADER_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/Header/*.h")
file(GLOB HEADER_APPS_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/Header/Apps/*.h")
file(GLOB HEADER_PIPELINES_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/Header/Pipelines/*.h")
file(GLOB HEADER_RAYTRACING_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/Header/Raytracing/*.h")
file(GLOB HEADER_RESOURCES_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/Header/Resources/*.h")
file(GLOB HEADER_VULKAN_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/Header/Vulkan/*.h")

file(GLOB SOURCE_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/Source/*.cpp")
file(GLOB SOURCE_APPS_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/Source/Apps/*.cpp")
file(GLOB SOURCE_PIPELINES_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/Source/Pipelines/*.cpp")
file(GLOB SOURCE_RAYTRACING_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/Source/Raytracing/*.cpph")
file(GLOB SOURCE_RESOURCES_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/Source/Resources/*.cpp")
file(GLOB SOURCE_VULKAN_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/Source/Vulkan/*.cpp")

add_executable(HelloVulkan 
    ${HEADER_FILES}
    ${HEADER_APPS_FILES}
    ${HEADER_PIPELINES_FILES}
    ${HEADER_RAYTRACING_FILES}
    ${HEADER_RESOURCES_FILES}
    ${HEADER_VULKAN_FILES}

    ${SOURCE_FILES}
    ${SOURCE_APPS_FILES}
    ${SOURCE_PIPELINES_FILES}
    ${SOURCE_RAYTRACING_FILES}
    ${SOURCE_RESOURCES_FILES}
    ${SOURCE_VULKAN_FILES}
)

source_group(TREE "${CMAKE_CURRENT_LIST_DIR}/Header/Apps" PREFIX "Header Files/Apps" FILES ${HEADER_APPS_FILES}) 
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}/Header/Pipelines" PREFIX "Header Files/Pipelines" FILES ${HEADER_PIPELINES_FILES}) 
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}/Header/Raytracing" PREFIX "Header Files/Raytracing" FILES ${HEADER_RAYTRACING_FILES}) 
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}/Header/Resources" PREFIX "Header Files/Resources" FILES ${HEADER_RESOURCES_FILES}) 
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}/Header/Vulkan" PREFIX "Header Files/Vulkan" FILES ${HEADER_VULKAN_FILES}) 

source_group(TREE "${CMAKE_CURRENT_LIST_DIR}/Source/Apps" PREFIX "Source Files/Apps" FILES ${SOURCE_APPS_FILES}) 
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}/Source/Pipelines" PREFIX "Source Files/Pipelines" FILES ${SOURCE_PIPELINES_FILES}) 
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}/Source/Raytracing" PREFIX "Source Files/Raytracing" FILES ${SOURCE_RAYTRACING_FILES}) 
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}/Source/Resources" PREFIX "Source Files/Resources" FILES ${SOURCE_RESOURCES_FILES}) 
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}/Source/Vulkan" PREFIX "Source Files/Vulkan" FILES ${SOURCE_VULKAN_FILES})

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