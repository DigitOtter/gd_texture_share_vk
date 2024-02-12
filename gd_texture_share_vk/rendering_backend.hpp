#pragma once

#ifdef USE_OPENGL

#include <texture_share_gl/texture_share_gl_client.hpp>

using texture_share_client_t = TextureShareGlClient;
using texture_id_t           = GLuint;
using texture_format_t       = GLuint;

#else
#include <iostream>
#include <texture_share_vk/texture_share_vk_client.hpp>
#include <texture_share_vk/texture_share_vk_setup.hpp>

using texture_share_client_t = TextureShareVkClient;
using texture_id_t           = VkImage;
using texture_format_t       = VkFormat;

#define VK_CHECK(x)                                                                    \
	do                                                                             \
	{                                                                              \
		VkResult err = x;                                                      \
		if(err)                                                                \
		{                                                                      \
			std::cout << "Detected Vulkan error: " << err << std::endl;    \
			abort();                                                       \
		}                                                                      \
	} while(0)
#endif

using tsv_image_format_t     = ImgFormat;
