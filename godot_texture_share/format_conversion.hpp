#pragma once

#include "rendering_backend.hpp"

inline texture_format_t convert_godot_to_rendering_device_format(godot::Image::Format format)
{
#ifdef USE_OPENGL
	switch(format)
	{
		case godot::Image::Format::FORMAT_RGBA8:
			return GL_RGBA;
		case godot::Image::Format::FORMAT_RGB8:
			return GL_RGB;
		default:
			return GL_NONE;
	}
#else
	switch(format)
	{
		case godot::Image::Format::FORMAT_RGBA8:
			return VkFormat::VK_FORMAT_R8G8B8A8_UNORM;
		case godot::Image::Format::FORMAT_RGB8:
			return VkFormat::VK_FORMAT_R8G8B8_UNORM;
		default:
			return VkFormat::VK_FORMAT_UNDEFINED;
	}
#endif
}

inline godot::Image::Format convert_rendering_device_to_godot_format(texture_format_t format)
{
#ifdef USE_OPENGL
	switch(format)
	{
		case GL_BGRA:
		case GL_RGBA:
			return godot::Image::Format::FORMAT_RGBA8;

		default:
			return godot::Image::Format::FORMAT_MAX;
	}
#else
	switch(format)
	{
		case VkFormat::VK_FORMAT_R8G8B8A8_UNORM:
		case VkFormat::VK_FORMAT_B8G8R8A8_UNORM:
			return godot::Image::Format::FORMAT_RGBA8;

		case VkFormat::VK_FORMAT_B8G8R8_UNORM:
		case VkFormat::VK_FORMAT_R8G8B8_UNORM:
			return godot::Image::Format::FORMAT_RGB8;

		default:
			return godot::Image::Format::FORMAT_MAX;
	}
#endif
}
