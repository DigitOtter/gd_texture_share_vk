set(PROJECT_NAME "GodotTextureShare")
set(HEADER_DIRECTORY "godot_texture_share")

set(NAMESPACE_NAME "${PROJECT_NAME}")

set(LIBRARY_NAME "${PROJECT_NAME}")
set(EXECUTABLE_NAME "${PROJECT_NAME}Exec")
set(TEST_NAME "${PROJECT_NAME}Tests")

set(LIB_EXPORT_NAME "${LIBRARY_NAME}Targets")
set(LIB_CONFIG_NAME "${LIBRARY_NAME}Config")
set(LIB_VERSION_NAME "${LIB_CONFIG_NAME}Version")

cmake_minimum_required(VERSION 3.18)
project("${PROJECT_NAME}" VERSION 1.0.0)

include(GNUInstallDirs)

option(USE_OPENGL "Build for Godot OpenGl Backend" OFF)

set (CMAKE_CXX_STANDARD 20)

add_subdirectory(godot-cpp)

find_package(TextureShareVulkan REQUIRED)

# List library files here
set(LIB_SRC_FILES
	"godot_texture_share/godot_texture_share.cpp"
	"godot_texture_share/texture_sender.cpp"
	"godot_texture_share/register_types.cpp"
)

##########################################
## Library
add_library("${LIBRARY_NAME}" SHARED ${LIB_SRC_FILES})
add_library("${NAMESPACE_NAME}::${LIBRARY_NAME}" ALIAS ${LIBRARY_NAME})
target_compile_options(${LIBRARY_NAME} PUBLIC $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:-Wall -Wextra>)

if(USE_OPENGL)
	target_compile_definitions(${LIBRARY_NAME} PRIVATE USE_OPENGL)
	set(TSVLibraries TextureShareVulkan::TextureShareOpenGL)
else()
	target_compile_definitions(${LIBRARY_NAME} PRIVATE USE_VULKAN)
	set(TSVLibraries TextureShareVulkan::TextureShareVulkan)
endif()

target_include_directories(${LIBRARY_NAME}
	PUBLIC
	    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
		"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"

	PRIVATE
)

target_link_libraries(${LIBRARY_NAME}
	PUBLIC
	    godot::cpp
		${TSVLibraries}

	PRIVATE
)
