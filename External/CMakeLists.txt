find_package(Vulkan REQUIRED)

add_subdirectory(volk)

add_subdirectory(tracy)

set(GLFW_INSTALL OFF CACHE BOOL "")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "")
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "")
add_subdirectory(glfw)

set(ASSIMP_NO_EXPORT ON CACHE BOOL "")
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "")
set(ASSIMP_INSTALL_PBD OFF CACHE BOOL "")
set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "")
set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE BOOL "")
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "")
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "")
add_subdirectory(assimp)

add_library(imgui STATIC)
target_include_directories(imgui PRIVATE 
    "$ENV{VULKAN_SDK}/Include"
    "${CMAKE_CURRENT_LIST_DIR}/glfw/include"
    "${CMAKE_CURRENT_LIST_DIR}/imgui"
)
target_compile_definitions(imgui PRIVATE VK_NO_PROTOTYPES)
target_sources(imgui PRIVATE 
	"${CMAKE_CURRENT_LIST_DIR}/ImGui/imgui.h"
    "${CMAKE_CURRENT_LIST_DIR}/ImGui/backends/imgui_impl_glfw.h"
	"${CMAKE_CURRENT_LIST_DIR}/ImGui/backends/imgui_impl_vulkan.h"
	
	"${CMAKE_CURRENT_LIST_DIR}/ImGui/imgui.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/ImGui/imgui_demo.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/ImGui/imgui_draw.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/ImGui/imgui_widgets.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/ImGui/imgui_tables.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ImGui/backends/imgui_impl_glfw.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/ImGui/backends/imgui_impl_vulkan.cpp"
)

set_property(TARGET glfw PROPERTY FOLDER "ThirdPartyLibraries")
set_property(TARGET volk PROPERTY FOLDER "ThirdPartyLibraries")
set_property(TARGET assimp PROPERTY FOLDER "ThirdPartyLibraries")
set_property(TARGET imgui PROPERTY FOLDER "ThirdPartyLibraries")
set_property(TARGET zlibstatic PROPERTY FOLDER "ThirdPartyLibraries")
set_property(TARGET TracyClient PROPERTY FOLDER "ThirdPartyLibraries")
set_property(TARGET update_mappings PROPERTY FOLDER "ThirdPartyLibraries") # glfw
set_property(TARGET UpdateAssimpLibsDebugSymbolsAndDLLs PROPERTY FOLDER "ThirdPartyLibraries")