#pragma once

#include <godot_cpp/classes/image.hpp>

#include "rendering_backend.hpp"

inline tsv_image_format_t convert_godot_to_rendering_device_format(godot::Image::Format format)
{
	switch(format)
	{
	    case godot::Image::Format::FORMAT_RGBA8:
		    return ImgFormat::R8G8B8A8;
	    case godot::Image::Format::FORMAT_RGB8:
		    return ImgFormat::R8G8B8;
	    default:
		    return ImgFormat::Undefined;
	}
}

inline godot::Image::Format convert_rendering_device_to_godot_format(tsv_image_format_t format)
{
	switch(format)
	{
	    case ImgFormat::B8G8R8A8:
	    case ImgFormat::R8G8B8A8:
		    return godot::Image::Format::FORMAT_RGBA8;

	    case ImgFormat::B8G8R8:
	    case ImgFormat::R8G8B8:
		    return godot::Image::Format::FORMAT_RGB8;

	    default:
		    return godot::Image::Format::FORMAT_MAX;
	}
}

//inline texture_format_t convert_godot_to_rendering_device_format(godot::Image::Format format)
//{
//#ifdef USE_OPENGL
//	switch(format)
//	{
//		case godot::Image::Format::FORMAT_RGBA8:
//			return GL_RGBA;
//		case godot::Image::Format::FORMAT_RGB8:
//			return GL_RGB;
//		default:
//			return GL_NONE;
//	}
//#else
//	switch(format)
//	{
//		case godot::Image::Format::FORMAT_RGBA8:
//			return VkFormat::VK_FORMAT_R8G8B8A8_UNORM;
//		case godot::Image::Format::FORMAT_RGB8:
//			return VkFormat::VK_FORMAT_R8G8B8_UNORM;
//		default:
//			return VkFormat::VK_FORMAT_UNDEFINED;
//	}
//#endif
//}

//inline godot::Image::Format convert_rendering_device_to_godot_format(texture_format_t format)
//{
//#ifdef USE_OPENGL
//	switch(format)
//	{
//		case GL_BGRA:
//		case GL_RGBA:
//			return godot::Image::Format::FORMAT_RGBA8;

//		default:
//			return godot::Image::Format::FORMAT_MAX;
//	}
//#else
//	switch(format)
//	{
//		case VkFormat::VK_FORMAT_R8G8B8A8_UNORM:
//		case VkFormat::VK_FORMAT_B8G8R8A8_UNORM:
//			return godot::Image::Format::FORMAT_RGBA8;

//		case VkFormat::VK_FORMAT_B8G8R8_UNORM:
//		case VkFormat::VK_FORMAT_R8G8B8_UNORM:
//			return godot::Image::Format::FORMAT_RGB8;

//		default:
//			return godot::Image::Format::FORMAT_MAX;
//	}
//#endif
//}
