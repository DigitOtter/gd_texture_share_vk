#include "tsv_sender.hpp"

#include "format_conversion.hpp"

#include <assert.h>

#include <godot_cpp/classes/rendering_device.hpp>
#include <godot_cpp/core/class_db.hpp>

TsvSender::TsvSender()
{
	// Load Extensions
#ifdef USE_OPENGL
	if(!TextureShareGlClient::initialize_gl_external())
		ERR_PRINT("Failed to load OpenGL Extensions");

	if(!this->_tsv_client.init_with_server_launch())
		ERR_PRINT("Failed to launch/connect to shared texture server");

#else
	using godot::RenderingDevice;
	using godot::RID;

	godot::RenderingDevice *const prd = godot::RenderingServer::get_singleton()->get_rendering_device();
	if(!prd)
	{
		WARN_PRINT("Unable to load RenderingDevice. Can't use TsvSender without Renderer");
		return;
	}

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
	this->_tsv_client.init_with_server_launch(vk_setup.release());

	VkFenceCreateInfo fence_info{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, 0};
	VK_CHECK(vkCreateFence(vk_dev, &fence_info, nullptr, &this->_fence));

//	this->_client->InitializeVulkan();

//	if(!ExternalHandleVk::LoadVulkanHandleExtensions(vk_inst))
//		ERR_PRINT("Failed to load Vulkan Extensions");
//	if(!ExternalHandleVk::LoadCompatibleSemaphorePropsInfo(vk_ph_dev))
//		ERR_PRINT("Failed to load Vulkan semaphore info");
#endif
}

TsvSender::~TsvSender()
{
#ifndef USE_OPENGL
	godot::RenderingDevice *const prd = godot::RenderingServer::get_singleton()->get_rendering_device();
	if(!prd)
	{
		WARN_PRINT("Unable to load RenderingDevice. Can't use TsvSender without Renderer");
		return;
	}

	VkDevice vk_dev =
		(VkDevice)prd->get_driver_resource(godot::RenderingDevice::DRIVER_RESOURCE_VULKAN_DEVICE, godot::RID(), 0);

	if(this->_fence != VK_NULL_HANDLE)
	{
		vkDestroyFence(vk_dev, this->_fence, nullptr);
		this->_fence = VK_NULL_HANDLE;
	}
#endif
}

void TsvSender::set_texture(const godot::Ref<godot::Texture2D> &texture, godot::Image::Format texture_format)
{
	this->_texture = texture;
	if(!this->_shared_texture_name.empty())
		this->check_and_update_shared_texture(texture_format);
}

godot::Ref<godot::Texture2D> TsvSender::get_texture()
{
	return this->_texture;
}

void TsvSender::set_shared_texture_name(const godot::String &shared_texture_name)
{
	const std::string new_name = shared_texture_name.ascii().ptr();
	if(new_name != this->_shared_texture_name)
	{
		this->_shared_texture_name = new_name;

		if(this->_texture.is_valid())
			this->update_shared_texture(this->_width, this->_height, this->_format);
	}
}

godot::String TsvSender::get_shared_texture_name()
{
	return godot::String(this->_shared_texture_name.c_str());
}

bool TsvSender::send_texture()
{
	return this->send_texture_internal();
}

void TsvSender::connect_to_frame_post_draw()
{
	godot::RenderingServer *const prs = godot::RenderingServer::get_singleton();
	prs->connect("frame_post_draw", godot::Callable(this, "__send_texture"));
}

bool TsvSender::is_connected_to_frame_post_draw()
{
	godot::RenderingServer *const prs = godot::RenderingServer::get_singleton();
	return prs->is_connected("frame_post_draw", godot::Callable(this, "__send_texture"));
}

void TsvSender::disconnect_to_frame_post_draw()
{
	godot::RenderingServer *const prs = godot::RenderingServer::get_singleton();
	prs->disconnect("frame_post_draw", godot::Callable(this, "__send_texture"));
}

void TsvSender::_bind_methods()
{
	using godot::ClassDB;
	using godot::D_METHOD;
	using godot::PropertyInfo;

	ClassDB::bind_method(D_METHOD("get_texture"), &TsvSender::get_texture);
	ClassDB::bind_method(D_METHOD("set_texture", "texture", "format"), &TsvSender::set_texture);
	//	ClassDB::add_property("TextureSender", PropertyInfo(godot::Variant::Type::OBJECT, "texture"), "set_texture",
	//	                      "get_texture");

	ClassDB::bind_method(D_METHOD("get_shared_texture_name"), &TsvSender::get_shared_texture_name);
	ClassDB::bind_method(D_METHOD("set_shared_texture_name"), &TsvSender::set_shared_texture_name);
	ClassDB::add_property("TsvSender", PropertyInfo(godot::Variant::STRING, "shared_texture_name"),
	                      "set_shared_texture_name", "get_shared_texture_name");

	ClassDB::bind_method(D_METHOD("connect_to_frame_post_draw"), &TsvSender::connect_to_frame_post_draw);
	ClassDB::bind_method(D_METHOD("is_connected_to_frame_post_draw"), &TsvSender::is_connected_to_frame_post_draw);
	ClassDB::bind_method(D_METHOD("disconnect_to_frame_post_draw"), &TsvSender::disconnect_to_frame_post_draw);

	ClassDB::bind_method(D_METHOD("send_texture"), &TsvSender::send_texture);

	// Connect this to "frame_post_draw"
	ClassDB::bind_method(D_METHOD("__send_texture"), &TsvSender::send_texture_internal);
}

bool TsvSender::update_shared_texture(uint32_t width, uint32_t height, godot::Image::Format format)
{
	assert(!this->_shared_texture_name.empty());
	if(this->_width == width && this->_height == height && this->_format == format)
		return true;

	const tsv_image_format_t tsv_format = convert_godot_to_rendering_device_format(format);
	if(tsv_format == tsv_image_format_t::Undefined)
		return false;

	this->_width  = width;
	this->_height = height;
	this->_format = format;

	this->_tsv_client.init_image(this->_shared_texture_name.c_str(), width, height, tsv_format, true);

	return true;
}

bool TsvSender::check_and_update_shared_texture(godot::Image::Format format)
{
	if(this->_texture.is_valid() && !this->_shared_texture_name.empty())
	{
		const uint32_t new_width  = this->_texture->get_width();
		const uint32_t new_height = this->_texture->get_height();
		return this->update_shared_texture(new_width, new_height, format);
	}

	return false;
}

bool TsvSender::send_texture_internal()
{
	if(!this->check_and_update_shared_texture(this->_format))
		return false;

	// Send texture
	const texture_id_t texture_id =
		(texture_id_t)godot::RenderingServer::get_singleton()->texture_get_native_handle(this->_texture->get_rid());

#ifdef USE_OPENGL
	const ImageExtent dim{
		{0,					 0					 },
		{(GLsizei)this->_width, (GLsizei)this->_height},
	};

	GLint drawFboId = 0;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFboId);
	this->_tsv_client.send_image(this->_shared_texture_name.c_str(), texture_id, GL_TEXTURE_2D, false, drawFboId, &dim);
#else
	this->_tsv_client.send_image(this->_shared_texture_name.c_str(), texture_id,
	                             VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	                             this->_fence, nullptr);
#endif

	return true;
}
