

#target_include_directories(HelloVulkan PRIVATE 
#	"${CMAKE_CURRENT_LIST_DIR}/Header"
#	"${CMAKE_CURRENT_LIST_DIR}/Header/Apps"
#	"${CMAKE_CURRENT_LIST_DIR}/Header/Pipelines"
#	"${CMAKE_CURRENT_LIST_DIR}/Header/Raytracing"
#	"${CMAKE_CURRENT_LIST_DIR}/Header/Resources"
#	"${CMAKE_CURRENT_LIST_DIR}/Header/Vulkan"
#)

file(GLOB_RECURSE HeaderFiles CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/Header/*.h")
file(GLOB_RECURSE SourceFiles CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/Source/*.cpp")

add_executable(HelloVUlkan ${HeaderFiles} ${SourceFiles})