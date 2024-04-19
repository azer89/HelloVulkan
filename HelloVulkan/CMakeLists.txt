# This CMake file has only been tested on Windows 11 and Ubuntu 22.04 LTS

file(GLOB_RECURSE SHADER_FILES CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/Shaders/*")
file(GLOB_RECURSE H_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/Header/*.h")
file(GLOB_RECURSE CPP_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/Source/*.cpp")

add_executable(HelloVulkan 
    ${H_FILES}
    ${CPP_FILES}
    ${SHADER_FILES}
)

source_group(TREE "${CMAKE_CURRENT_LIST_DIR}/Shaders" PREFIX "Shaders" FILES ${SHADER_FILES})
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}/Header" PREFIX "Header Files" FILES ${H_FILES})
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}/Source" PREFIX "Source Files" FILES ${CPP_FILES})

target_compile_definitions(HelloVulkan PRIVATE
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:>

    _CONSOLE
    _CRT_SECURE_NO_WARNINGS
    TRACY_ENABLE # Disable this if tracy is not needed
    GLM_FORCE_DEPTH_ZERO_TO_ONE # This is needed for clustered forward demo
)

if(MSVC)
    # Tracy does not like /ZI so set it to /Zi
    target_compile_options(HelloVulkan PRIVATE "/Zi")
endif()  

# Include directories
set(HEADER_FOLDER "${CMAKE_CURRENT_LIST_DIR}/Header")
target_include_directories(HelloVulkan PRIVATE "${HEADER_FOLDER}")
file(GLOB ITEMS LIST_DIRECTORIES true "${HEADER_FOLDER}/*")
foreach(item ${ITEMS})
    if(IS_DIRECTORY ${item})
        target_include_directories(HelloVulkan PRIVATE ${item})
    endif()
endforeach()

# Include directories from dependencies
target_include_directories(HelloVulkan PRIVATE
    "${PROJECT_SOURCE_DIR}/External/stb"
    "${PROJECT_SOURCE_DIR}/External/assimp/include"
    "${PROJECT_SOURCE_DIR}/External/glfw/include"
    "${PROJECT_SOURCE_DIR}/External/glm"
    "${PROJECT_SOURCE_DIR}/External/ImGui"
    "${PROJECT_SOURCE_DIR}/External/ImGui/backends"
    "${PROJECT_SOURCE_DIR}/External/ImGuizmo"
    "${PROJECT_SOURCE_DIR}/External/volk"
    "${PROJECT_SOURCE_DIR}/External/VulkanMemoryAllocator/include"
    "${PROJECT_SOURCE_DIR}/External/tracy/public/tracy"
    "${PROJECT_SOURCE_DIR}/External/Binaries/assimp/include"
)

# Set dependencies
target_link_libraries(HelloVulkan PRIVATE assimp imgui glfw TracyClient volk)

if(glslang_FOUND)
    target_link_libraries(HelloVulkan PRIVATE
            glslang::glslang
            glslang::SPIRV
            glslang::glslang-default-resource-limits
    )
else()
    if(WIN32)
        get_filename_component(VK_SDK_PATH ${Vulkan_LIBRARY} DIRECTORY) # Some CMake Generators don't play nice with $ENV{VULKAN_SDK}
        get_filename_component(VK_SDK_PATH ${VK_SDK_PATH} DIRECTORY)
        target_link_libraries(HelloVulkan PRIVATE
                $<$<CONFIG:Debug>:
                "${VK_SDK_PATH}/Lib/GenericCodeGend.lib"
                "${VK_SDK_PATH}/Lib/glslangd.lib"
                "${VK_SDK_PATH}/Lib/glslang-default-resource-limitsd.lib"
                "${VK_SDK_PATH}/Lib/MachineIndependentd.lib"
                "${VK_SDK_PATH}/Lib/SPIRVd.lib"
                "${VK_SDK_PATH}/Lib/SPIRV-Toolsd.lib"
                "${VK_SDK_PATH}/Lib/SPIRV-Tools-optd.lib"
                "${VK_SDK_PATH}/Lib/OSDependentd.lib"
                >
                $<$<CONFIG:Release>:
                "${VK_SDK_PATH}/Lib/GenericCodeGen.lib"
                "${VK_SDK_PATH}/Lib/glslang.lib"
                "${VK_SDK_PATH}/Lib/glslang-default-resource-limits.lib"
                "${VK_SDK_PATH}/Lib/MachineIndependent.lib"
                "${VK_SDK_PATH}/Lib/SPIRV.lib"
                "${VK_SDK_PATH}/Lib/SPIRV-Tools.lib"
                "${VK_SDK_PATH}/Lib/SPIRV-Tools-opt.lib"
                "${VK_SDK_PATH}/Lib/OSDependent.lib"
                >
        )
    endif()
endif()