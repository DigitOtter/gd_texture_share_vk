#include "godot_texture_share.hpp"

#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>

GodotTextureShare::GodotTextureShare()
{
	this->_receiver.reset(new texture_share_client_t());

#ifdef USE_OPENGL
	if(!ExternalHandleGl::LoadGlEXT())
		ERR_PRINT("Failed to load OpenGL Extensions");

#else
	using godot::RenderingDevice;
	using godot::RID;

	godot::RenderingDevice *const prd = godot::RenderingServer::get_singleton()->get_rendering_device();
	assert(prd);
	VkInstance vk_inst =
		(VkInstance)prd->get_driver_resource(RenderingDevice::DRIVER_RESOURCE_VULKAN_INSTANCE, RID(), 0);
	VkPhysicalDevice vk_ph_dev =
		(VkPhysicalDevice)prd->get_driver_resource(RenderingDevice::DRIVER_RESOURCE_VULKAN_PHYSICAL_DEVICE, RID(), 0);
	VkDevice vk_dev   = (VkDevice)prd->get_driver_resource(RenderingDevice::DRIVER_RESOURCE_VULKAN_DEVICE, RID(), 0);
	VkQueue  vk_queue = (VkQueue)prd->get_driver_resource(RenderingDevice::DRIVER_RESOURCE_VULKAN_QUEUE, RID(), 0);
	uint32_t vk_queue_index =
		(uint32_t)prd->get_driver_resource(RenderingDevice::DRIVER_RESOURCE_VULKAN_QUEUE_FAMILY_INDEX, RID(), 0);

	this->_receiver->InitializeVulkan(vk_inst, vk_dev, vk_ph_dev, vk_queue, vk_queue_index, true);

	VkFenceCreateInfo fence_info{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, 0};
	VK_CHECK(vkCreateFence(vk_dev, &fence_info, nullptr, &this->_fence));

#endif

	this->_create_initial_texture(1, 1, godot::Image::FORMAT_RGBA8);
}

GodotTextureShare::~GodotTextureShare()
{
	godot::RenderingServer *const prs = godot::RenderingServer::get_singleton();
	if(this->_texture.is_valid())
	{
		prs->free_rid(this->_texture);
		this->_texture = godot::RID();
	}

#ifndef USE_OPENGL
	godot::RenderingDevice *const prd = prs->get_rendering_device();
	assert(prd);
	VkDevice vk_dev =
		(VkDevice)prd->get_driver_resource(godot::RenderingDevice::DRIVER_RESOURCE_VULKAN_DEVICE, godot::RID(), 0);

	if(this->_fence != VK_NULL_HANDLE)
	{
		vkDestroyFence(vk_dev, this->_fence, nullptr);
		this->_fence = VK_NULL_HANDLE;
	}
#endif
}

godot::Image::Format GodotTextureShare::convert_rendering_device_to_godot_format(texture_format_t format)
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

texture_format_t GodotTextureShare::convert_godot_to_rendering_device_format(godot::Image::Format format)
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

void GodotTextureShare::_init()
{
	assert(this->_texture == godot::RID());
	this->_width  = 0;
	this->_height = 0;
	this->_flags  = 0;

	this->_receiver.reset(nullptr);
	this->_channel_name = "";

	this->_create_initial_texture(1, 1, godot::Image::FORMAT_RGB8);
}

void GodotTextureShare::_draw(const godot::RID &to_canvas_item, const godot::Vector2 &pos, const godot::Color &modulate,
                              bool transpose) const
{
	// Code taken from godot source ImageTexture class
	if((this->_width | this->_height) == 0)
		return;

	godot::RenderingServer::get_singleton()->canvas_item_add_texture_rect(
		to_canvas_item, godot::Rect2(pos, godot::Size2(this->_width, this->_height)), this->_texture, false, modulate,
		transpose);
}

void GodotTextureShare::_draw_rect(const godot::RID &to_canvas_item, const godot::Rect2 &rect, bool tile,
                                   const godot::Color &modulate, bool transpose) const
{
	// Code taken from godot source ImageTexture class
	if((this->_width | this->_height) == 0)
		return;

	godot::RenderingServer::get_singleton()->canvas_item_add_texture_rect(to_canvas_item, rect, this->_texture, tile,
	                                                                      modulate, transpose);
}

void GodotTextureShare::_draw_rect_region(const godot::RID &to_canvas_item, const godot::Rect2 &rect,
                                          const godot::Rect2 &src_rect, const godot::Color &modulate, bool transpose,
                                          bool clip_uv) const
{
	// Code taken from godot source ImageTexture class
	if((this->_width | this->_height) == 0)
		return;

	godot::RenderingServer::get_singleton()->canvas_item_add_texture_rect_region(
		to_canvas_item, rect, this->_texture, src_rect, modulate, transpose, clip_uv);
}

godot::String GodotTextureShare::get_shared_name() const
{
	return godot::String(this->_channel_name.c_str());
}

void GodotTextureShare::set_shared_name(godot::String shared_name)
{
	this->_channel_name = shared_name.ascii().ptr();
	if(this->_create_receiver(this->_channel_name))
		this->_receive_texture();
}

void GodotTextureShare::connect_to_frame_pre_draw()
{
	godot::RenderingServer *const prs = godot::RenderingServer::get_singleton();
	prs->connect("frame_pre_draw", godot::Callable(this, "__receive_texture"));
}

bool GodotTextureShare::is_connected_to_frame_pre_draw()
{
	godot::RenderingServer *const prs = godot::RenderingServer::get_singleton();
	return prs->is_connected("frame_pre_draw", godot::Callable(this, "__receive_texture"));
}

void GodotTextureShare::disconnect_to_frame_pre_draw()
{
	godot::RenderingServer *const prs = godot::RenderingServer::get_singleton();
	prs->disconnect("frame_pre_draw", godot::Callable(this, "__receive_texture"));
}

void GodotTextureShare::_bind_methods()
{
	using godot::ClassDB;
	using godot::D_METHOD;
	using godot::PropertyInfo;

	ClassDB::bind_method(D_METHOD("get_shared_name"), &GodotTextureShare::get_shared_name);
	ClassDB::bind_method(D_METHOD("set_shared_name"), &GodotTextureShare::set_shared_name);
	ClassDB::add_property("GodotTextureShare", PropertyInfo(godot::Variant::STRING, "shared_name"), "set_shared_name",
	                      "get_shared_name");

	ClassDB::bind_method(D_METHOD("connect_to_frame_pre_draw"), &GodotTextureShare::connect_to_frame_pre_draw);
	ClassDB::bind_method(D_METHOD("is_connected_to_frame_pre_draw"),
	                     &GodotTextureShare::is_connected_to_frame_pre_draw);
	ClassDB::bind_method(D_METHOD("disconnect_to_frame_pre_draw"), &GodotTextureShare::disconnect_to_frame_pre_draw);

	ClassDB::bind_method(D_METHOD("_receive_texture"), &GodotTextureShare::_receive_texture);
	ClassDB::bind_method(D_METHOD("__receive_texture"), &GodotTextureShare::receive_texture_internal);
}

bool GodotTextureShare::_create_receiver(const std::string &name)
{
	if(!this->_receiver)
		this->_receiver.reset(new texture_share_client_t());

	const auto *shared_image_data = this->_receiver->SharedImageHandle(name);
	if(!shared_image_data)
		return false;

	const godot::Image::Format format = convert_rendering_device_to_godot_format(shared_image_data->ImageFormat());
	this->_update_texture(shared_image_data->Width(), shared_image_data->Height(), format);

	return true;
}

void GodotTextureShare::_receive_texture()
{
	return this->receive_texture_internal();
}

bool GodotTextureShare::_check_and_update_shared_texture()
{
	if(!this->_receiver)
		return false;

	if(!this->_texture.is_valid())
		return false;

	const auto *const pimg_handle = this->_receiver->SharedImageHandle(this->_channel_name);
	if(!pimg_handle)
		return false;


	const uint32_t             new_width  = pimg_handle->Width();
	const uint32_t             new_height = pimg_handle->Height();
	const godot::Image::Format new_format = convert_rendering_device_to_godot_format(pimg_handle->ImageFormat());

	if(this->_width != new_width || this->_height != new_height || this->_format != new_format)
		this->_update_texture(new_width, new_height, new_format);

	return true;
}

void GodotTextureShare::_update_texture(const uint64_t width, const uint64_t height, const godot::Image::Format format)
{
	this->_width  = width;
	this->_height = height;
	this->_format = format;

	// Create new texture with correct width, height, and format
	godot::Ref<godot::Image> img = godot::Image::create(width, height, false, format);
	img->fill(godot::Color(1.0f, 0.0f, 0.0f));

	godot::RenderingServer *const prs = godot::RenderingServer::get_singleton();
	assert(this->_texture.is_valid());

	// Replace texture (only way to change height and width)
	godot::RID tmp_tex = prs->texture_2d_create(img);
	prs->texture_replace(this->_texture, tmp_tex);
	prs->free_rid(tmp_tex);

	this->_texture_id = (texture_id_t)prs->texture_get_native_handle(this->_texture, true);
}

void GodotTextureShare::_create_initial_texture(const uint64_t width, const uint64_t height,
                                                const godot::Image::Format format)
{
	this->_width  = width;
	this->_height = height;

	// Create simple texture
	godot::Ref<godot::Image> img = godot::Image::create(width, height, false, format);
	img->fill(godot::Color(1.0f, 0.0f, 0.0f));

	godot::RenderingServer *const prs = godot::RenderingServer::get_singleton();
	this->_texture                    = prs->texture_2d_create(img);
	this->_texture_id                 = (texture_id_t)prs->texture_get_native_handle(this->_texture);

	// Force redraw
	prs->texture_set_force_redraw_if_visible(this->_texture, true);
}

void GodotTextureShare::receive_texture_internal()
{
	if(!this->_receiver)
		return;

	if(!this->_check_and_update_shared_texture())
		return;


		// Receive texture
#ifdef USE_OPENGL
	const texture_share_client_t::ImageExtent dim{
		{0,					 0					 },
		{(GLsizei)this->_width, (GLsizei)this->_height},
	};

	GLint drawFboId = 0;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFboId);
	this->_receiver->RecvImageBlit(this->_channel_name, this->_texture_id, GL_TEXTURE_2D, dim, false, drawFboId);
#else
	// Use VK_IMAGE_LAYOUT_UNDEFINED to discard old data
	this->_receiver->RecvImageBlit(this->_channel_name, this->_texture_id, VK_IMAGE_LAYOUT_UNDEFINED,
	                               VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, this->_fence);
#endif
}
