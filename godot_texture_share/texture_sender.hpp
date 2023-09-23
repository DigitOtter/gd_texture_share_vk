#pragma once

#include <texture_share_vk/opengl/texture_share_gl_client.h>

#include <gdextension_interface.h>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/texture2d.hpp>

#include "rendering_backend.hpp"

class TextureSender : public godot::Resource
{
	GDCLASS(TextureSender, godot::Resource);

	public:
	TextureSender();
	~TextureSender() override;

	void set_texture(const godot::Ref<godot::Texture2D> &texture, godot::Image::Format texture_format);
	godot::Ref<godot::Texture2D> get_texture();

	void          set_shared_texture_name(const godot::String &shared_texture_name);
	godot::String get_shared_texture_name();

	bool send_texture();

	void connect_to_frame_post_draw();
	bool is_connected_to_frame_post_draw();
	void disconnect_to_frame_post_draw();

	protected:
	static void _bind_methods();

	private:
	godot::Ref<godot::Texture2D> _texture;

	std::unique_ptr<texture_share_client_t> _client = nullptr;

	std::string          _shared_texture_name;
	uint32_t             _width  = 0;
	uint32_t             _height = 0;
	godot::Image::Format _format = godot::Image::FORMAT_MAX;

#ifndef USE_OPENGL
	VkFence _fence;
#endif

	bool update_shared_texture(uint32_t width, uint32_t height, godot::Image::Format format);
	bool check_and_update_shared_texture(godot::Image::Format format);

	bool send_texture_internal();
};
