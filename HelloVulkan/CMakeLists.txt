# This CMake file has only been tested on Windows 11 and Visual Studio 2022 

file(GLOB_RECURSE SHADER_FILES CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/Shaders/*")
file(GLOB_RECURSE H_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/Header/*.h")
file(GLOB_RECURSE CPP_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/Source/*.cpp")

add_executable(HelloVulkan 
    ${H_FILES}
    ${CPP_FILES}
    ${SHADER_FILES}
)

source_group(TREE "${PROJECT_SOURCE_DIR}/Shaders" PREFIX "Shaders" FILES ${SHADER_FILES})
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}/Header" PREFIX "Header Files" FILES ${H_FILES})
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}/Source" PREFIX "Source Files" FILES ${CPP_FILES})

target_compile_definitions(HelloVulkan PRIVATE
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:>

    _CONSOLE
    _CRT_SECURE_NO_WARNINGS
    TRACY_ENABLE
    GLM_FORCE_DEPTH_ZERO_TO_ONE
)

if(MSVC)
    # Tracy does not like /ZI so set it to /Zi
    target_compile_options(HelloVulkan PRIVATE "/Zi")
endif()  

target_include_directories(HelloVulkan PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/Header"
    "${CMAKE_CURRENT_LIST_DIR}/Header/Apps"
    "${CMAKE_CURRENT_LIST_DIR}/Header/Pipelines"
    "${CMAKE_CURRENT_LIST_DIR}/Header/Raytracing"
    "${CMAKE_CURRENT_LIST_DIR}/Header/Resources"
    "${CMAKE_CURRENT_LIST_DIR}/Header/Vulkan"
)

target_include_directories(HelloVulkan PRIVATE
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

# TODO Build glslang manually
target_include_directories(HelloVulkan PRIVATE 
    "$ENV{VULKAN_SDK}/Include"
    "$ENV{VULKAN_SDK}/Include/glslang/Include"
    "$ENV{VULKAN_SDK}/Include/glslang/MachineIndependent"
    "$ENV{VULKAN_SDK}/Include/glslang/Public"
    "$ENV{VULKAN_SDK}/Include/glslang/SPIRV"
    "$ENV{VULKAN_SDK}/Include/spirv_cross"
    "$ENV{VULKAN_SDK}/Include/spirv-tools"
    "$ENV{VULKAN_SDK}/Include/spirv-headers"
)

target_link_libraries(HelloVulkan PRIVATE volk glfw TracyClient assimp imgui)

# If you are not on Windows you need to somehow modify code below
if(MSVC)
    target_link_libraries(HelloVulkan PRIVATE
        $<$<CONFIG:Debug>:
            "$ENV{VULKAN_SDK}/Lib/GenericCodeGend.lib"
            "$ENV{VULKAN_SDK}/Lib/glslangd.lib"
            "$ENV{VULKAN_SDK}/Lib/glslang-default-resource-limitsd.lib"
            "$ENV{VULKAN_SDK}/Lib/MachineIndependentd.lib"
            "$ENV{VULKAN_SDK}/Lib/SPIRVd.lib"
            "$ENV{VULKAN_SDK}/Lib/SPIRV-Toolsd.lib"
            "$ENV{VULKAN_SDK}/Lib/SPIRV-Tools-optd.lib"
            "$ENV{VULKAN_SDK}/Lib/OSDependentd.lib"
        >
        $<$<CONFIG:Release>:
            "$ENV{VULKAN_SDK}/Lib/GenericCodeGen.lib"
            "$ENV{VULKAN_SDK}/Lib/glslang.lib"
            "$ENV{VULKAN_SDK}/Lib/glslang-default-resource-limits.lib"
            "$ENV{VULKAN_SDK}/Lib/MachineIndependent.lib"
            "$ENV{VULKAN_SDK}/Lib/SPIRV.lib"
            "$ENV{VULKAN_SDK}/Lib/SPIRV-Tools.lib"
            "$ENV{VULKAN_SDK}/Lib/SPIRV-Tools-opt.lib"
            "$ENV{VULKAN_SDK}/Lib/OSDependent.lib"
        >
    )
endif()