#include "shared_texture.hpp"

#include "format_conversion.hpp"

#include <assert.h>

#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>

SharedTexture::SharedTexture()
{
#ifdef USE_OPENGL
	if(!TextureShareGlClient::initialize_gl_external())
		ERR_PRINT("Failed to load OpenGL Extensions");

	if(!this->_tsv_client.init_with_server_launch())
		ERR_PRINT("Failed to launch/connect to shared texture server");
#else
	// Get Vulkan data from RenderingDevice
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

	TextureShareVkSetup vk_setup;
	vk_setup.import_vulkan(vk_inst, vk_dev, vk_ph_dev, vk_queue, vk_queue_index, true);
	if(!this->_tsv_client.init_with_server_launch(vk_setup.release()))
		ERR_PRINT("Failed to launch/connect to VkServer");

	VkFenceCreateInfo fence_info{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, 0};
	VK_CHECK(vkCreateFence(vk_dev, &fence_info, nullptr, &this->_fence));

#endif

	// this->_create_initial_texture(1, 1, godot::Image::FORMAT_RGBA8);
}

SharedTexture::~SharedTexture()
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

void SharedTexture::_draw(const godot::RID &to_canvas_item, const godot::Vector2 &pos, const godot::Color &modulate,
                          bool transpose) const
{
	// Code taken from godot source ImageTexture class
	if((this->_width || this->_height) == 0)
		return;

	godot::RenderingServer::get_singleton()->canvas_item_add_texture_rect(
		to_canvas_item, godot::Rect2(pos, godot::Size2(this->_width, this->_height)), this->_texture, false, modulate,
		transpose);
}

void SharedTexture::_draw_rect(const godot::RID &to_canvas_item, const godot::Rect2 &rect, bool tile,
                               const godot::Color &modulate, bool transpose) const
{
	// Code taken from godot source ImageTexture class
	if((this->_width | this->_height) == 0)
		return;

	godot::RenderingServer::get_singleton()->canvas_item_add_texture_rect(to_canvas_item, rect, this->_texture, tile,
	                                                                      modulate, transpose);
}

void SharedTexture::_draw_rect_region(const godot::RID &to_canvas_item, const godot::Rect2 &rect,
                                      const godot::Rect2 &src_rect, const godot::Color &modulate, bool transpose,
                                      bool clip_uv) const
{
	// Code taken from godot source ImageTexture class
	if((this->_width | this->_height) == 0)
		return;

	godot::RenderingServer::get_singleton()->canvas_item_add_texture_rect_region(
		to_canvas_item, rect, this->_texture, src_rect, modulate, transpose, clip_uv);
}

godot::String SharedTexture::get_shared_texture_name() const
{
	return godot::String(this->_shared_texture_name.c_str());
}

void SharedTexture::set_shared_texture_name(godot::String shared_name)
{
	this->_shared_texture_name = shared_name.ascii().ptr();
	this->_check_and_update_shared_texture();
}

void SharedTexture::_receive_texture()
{
	return this->receive_texture_internal();
}

void SharedTexture::connect_to_frame_pre_draw()
{
	godot::RenderingServer *const prs = godot::RenderingServer::get_singleton();
	prs->connect("frame_pre_draw", godot::Callable(this, "__receive_texture"));
}

bool SharedTexture::is_connected_to_frame_pre_draw()
{
	godot::RenderingServer *const prs = godot::RenderingServer::get_singleton();
	return prs->is_connected("frame_pre_draw", godot::Callable(this, "__receive_texture"));
}

void SharedTexture::disconnect_to_frame_pre_draw()
{
	godot::RenderingServer *const prs = godot::RenderingServer::get_singleton();
	prs->disconnect("frame_pre_draw", godot::Callable(this, "__receive_texture"));
}

void SharedTexture::_bind_methods()
{
	using godot::ClassDB;
	using godot::D_METHOD;
	using godot::PropertyInfo;

	ClassDB::bind_method(D_METHOD("get_shared_texture_name"), &SharedTexture::get_shared_texture_name);
	ClassDB::bind_method(D_METHOD("set_shared_texture_name"), &SharedTexture::set_shared_texture_name);
	ClassDB::add_property("SharedTexture", PropertyInfo(godot::Variant::STRING, "shared_texture_name"),
	                      "set_shared_texture_name", "get_shared_texture_name");

	ClassDB::bind_method(D_METHOD("connect_to_frame_pre_draw"), &SharedTexture::connect_to_frame_pre_draw);
	ClassDB::bind_method(D_METHOD("is_connected_to_frame_pre_draw"), &SharedTexture::is_connected_to_frame_pre_draw);
	ClassDB::bind_method(D_METHOD("disconnect_to_frame_pre_draw"), &SharedTexture::disconnect_to_frame_pre_draw);

	ClassDB::bind_method(D_METHOD("_receive_texture"), &SharedTexture::_receive_texture);
	ClassDB::bind_method(D_METHOD("__receive_texture"), &SharedTexture::receive_texture_internal);
}

// bool SharedTexture::_create_receiver(const std::string &name)
//{
//	const auto  shared_image_lock = this->_tsv_client.find_image_data(name.c_str(), false);
//	const auto *shared_image_data = shared_image_lock.read();
//	if(!shared_image_data)
//		return false;

//	const godot::Image::Format format = convert_rendering_device_to_godot_format(shared_image_data->format);
//	this->_update_texture(shared_image_data->width, shared_image_data->height, format);

//	return true;
//}

bool SharedTexture::_check_and_update_shared_texture()
{
	bool update_texture = false;

	// Check if local texture was initialized
	if(!this->_texture.is_valid())
		update_texture = true;

	// Check if remote texture was changed
	ImageLookupResult res = this->_tsv_client.find_image(this->_shared_texture_name.c_str(), false);
	if(res == ImageLookupResult::Error || res == ImageLookupResult::NotFound)
		return false; // TODO: Error handling
	else if(res == ImageLookupResult::RequiresUpdate)
	{
		// Update local texture to remote parameters
		res = this->_tsv_client.find_image(this->_shared_texture_name.c_str(), true);
		if(res == ImageLookupResult::Found)
			update_texture = true;
		else if(res == ImageLookupResult::Error || res == ImageLookupResult::NotFound)
			return false;
	}

	if(update_texture)
	{
		// Update local texture to remote parameters
		const auto  data_lock = this->_tsv_client.find_image_data(this->_shared_texture_name.c_str(), false);
		const auto *data      = data_lock.read();
		if(data == nullptr)
			return false;

		if(!this->_texture.is_valid())
			this->_create_initial_texture(data->width, data->height,
			                              convert_rendering_device_to_godot_format(data->format));

		this->_update_texture(data->width, data->height, convert_rendering_device_to_godot_format(data->format));
	}

	return true;
}

void SharedTexture::_update_texture(const uint64_t width, const uint64_t height, const godot::Image::Format format)
{
	this->_width  = width;
	this->_height = height;

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

void SharedTexture::_create_initial_texture(const uint64_t width, const uint64_t height,
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

void SharedTexture::receive_texture_internal()
{
	if(!this->_check_and_update_shared_texture())
		return;


		// Receive texture
#ifdef USE_OPENGL
	const ImageExtent dim{
		{0,					 0					 },
		{(GLsizei)this->_width, (GLsizei)this->_height},
	};

	GLint drawFboId = 0;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFboId);
	this->_tsv_client.recv_image(this->_shared_texture_name.c_str(), this->_texture_id, GL_TEXTURE_2D, false, drawFboId,
	                             &dim);
#else
	// Use VK_IMAGE_LAYOUT_UNDEFINED to discard old data
	this->_tsv_client.recv_image(this->_shared_texture_name.c_str(), this->_texture_id, VK_IMAGE_LAYOUT_UNDEFINED,
	                             VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, this->_fence, nullptr);
#endif
}
