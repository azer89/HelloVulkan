find_package(Vulkan REQUIRED)

add_subdirectory(volk)
add_subdirectory(glfw)
add_subdirectory(tracy)
add_subdirectory(assimp)

#[[
add_library(volk STATIC)
add_library(glfw STATIC)
add_library(tracy STATIC)
add_library(assimp STATIC)
]]

add_library(imgui STATIC)
target_include_directories(imgui PRIVATE imgui)
target_compile_definitions(imgui PRIVATE VK_NO_PROTOTYPES)
target_sources(imgui PRIVATE 
	"${CMAKE_CURRENT_LIST_DIR}/ImGui/imgui.h"
	"${CMAKE_CURRENT_LIST_DIR}/ImGui/backends/imgui_impl_vulkan.h"
	"${CMAKE_CURRENT_LIST_DIR}/ImGui/backends/imgui_impl_glfw.h"
	"${CMAKE_CURRENT_LIST_DIR}/ImGui/imgui.cpp"

	"${CMAKE_CURRENT_LIST_DIR}/ImGui/imgui_demo.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/ImGui/imgui_draw.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/ImGui/imgui_widgets.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/ImGui/imgui_tables.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/ImGui/backends/imgui_impl_vulkan.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/ImGui/backends/imgui_impl_glfw.cpp"
)