set(PROJECT_NAME "GdTextureShareVk")
set(NAMESPACE_NAME "${PROJECT_NAME}")

set(GODOT_LIB_NAME "${PROJECT_NAME}")
set(GODOT_DIR_NAME "gd_texture_share_vk")

set(CMAKE_INSTALL_PREFIX   "${CMAKE_CURRENT_SOURCE_DIR}/bin" CACHE PATH   "Library install location")
set(INSTALL_FILENAME "${GODOT_LIB_NAME}"                     CACHE STRING "Library name")
set(GD_RES_PATH      "res://addons/${GODOT_DIR_NAME}/bin"    CACHE STRING "Godot resource path")

cmake_minimum_required(VERSION 3.18)
project("${PROJECT_NAME}" VERSION 1.0.0)

option(USE_OPENGL "Build for Godot OpenGl Backend" OFF)

set (CMAKE_CXX_STANDARD 20)

add_subdirectory(godot-cpp)

find_package(TextureShareVk REQUIRED)

set(LIB_SRC_FILES
	"gd_texture_share_vk/tsv_receive_texture.cpp"
	"gd_texture_share_vk/tsv_sender.cpp"
	"gd_texture_share_vk/register_types.cpp"
)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/gd_library_data.gdextension.in" "${CMAKE_CURRENT_BINARY_DIR}/${GODOT_DIR_NAME}.gdextension.in" @ONLY)

##########################################
## Library
add_library("${GODOT_LIB_NAME}" SHARED ${LIB_SRC_FILES})
add_library("${NAMESPACE_NAME}::${GODOT_LIB_NAME}" ALIAS ${GODOT_LIB_NAME})
target_compile_options(${GODOT_LIB_NAME} PUBLIC $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:-Wall -Wextra>)
set_target_properties(${GODOT_LIB_NAME} PROPERTIES
	LIBRARY_OUTPUT_NAME "${INSTALL_FILENAME}"
	PREFIX "")

if(USE_OPENGL)
	target_compile_definitions(${GODOT_LIB_NAME} PRIVATE USE_OPENGL)
	set(TSVLibraries TextureShareVk::TextureShareGlClientCpp)
else()
	target_compile_definitions(${GODOT_LIB_NAME} PRIVATE USE_VULKAN)
	set(TSVLibraries TextureShareVk::TextureShareVkClientCpp)
endif()

target_include_directories(${GODOT_LIB_NAME}
	PUBLIC
	    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
		"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"

	PRIVATE
)

target_link_libraries(${GODOT_LIB_NAME}
	PUBLIC
	    godot::cpp
		${TSVLibraries}

	PRIVATE
)

##########################################
## Install
install(TARGETS ${GODOT_LIB_NAME}
	LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}"
	RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}")

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${GODOT_DIR_NAME}.gdextension.in"
	DESTINATION "${CMAKE_INSTALL_PREFIX}"
	RENAME "${GODOT_DIR_NAME}.gdextension")
