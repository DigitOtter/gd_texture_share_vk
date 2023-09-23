#pragma once

#ifdef USE_OPENGL

#include <texture_share_vk/opengl/texture_share_gl_client.h>

using texture_share_client_t = TextureShareGlClient;
using texture_id_t           = GLuint;
using texture_format_t       = GLuint;

#else

#include <texture_share_vk/texture_share_vk_client.h>

using texture_share_client_t = TextureShareVkClient;
using texture_id_t           = VkImage;
using texture_format_t       = VkFormat;

#endif
