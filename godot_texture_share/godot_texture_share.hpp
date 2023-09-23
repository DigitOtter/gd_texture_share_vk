#pragma once

#include <gdextension_interface.h>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/texture2d.hpp>

#include <texture_share_vk/opengl/texture_share_gl_client.h>

#include "rendering_backend.hpp"

class GodotTextureShare : public godot::Texture2D
{
	GDCLASS(GodotTextureShare, Texture2D);

	public:
	GodotTextureShare();
	~GodotTextureShare() override;

	static texture_format_t     convert_godot_to_rendering_device_format(godot::Image::Format format);
	static godot::Image::Format convert_rendering_device_to_godot_format(texture_format_t format);

	void        _init();
	static void _register_methods();

	int32_t _get_width() const override { return this->_width; }

	int32_t _get_height() const override { return this->_height; }

	bool _has_alpha() const override { return true; }

	// virtual godot::RID _get_rid() override { return this->_texture; }

	virtual void set_flags(const int64_t flags) { _flags = flags; }

	virtual int64_t get_flags() const { return _flags; }

	void _draw(const godot::RID &to_canvas_item, const godot::Vector2 &pos, const godot::Color &modulate,
	           bool transpose) const override;
	void _draw_rect(const godot::RID &to_canvas_item, const godot::Rect2 &rect, bool tile, const godot::Color &modulate,
	                bool transpose) const override;
	void _draw_rect_region(const godot::RID &to_canvas_item, const godot::Rect2 &rect, const godot::Rect2 &src_rect,
	                       const godot::Color &modulate, bool transpose, bool clip_uv) const override;

	godot::String get_shared_name() const;
	void          set_shared_name(godot::String shared_name);

	void connect_to_frame_pre_draw();
	bool is_connected_to_frame_pre_draw();
	void disconnect_to_frame_pre_draw();

	protected:
	static void _bind_methods();

	bool _create_receiver(const std::string &name);
	void _receive_texture();

	bool _check_and_update_shared_texture();
	void _update_texture(const uint64_t width, const uint64_t height, const godot::Image::Format format);

	private:
	// Texture
	godot::RID   _texture    = godot::RID();
	texture_id_t _texture_id = 0;

	godot::Image::Format _format = godot::Image::FORMAT_MAX;
	uint32_t             _width  = 0;
	uint32_t             _height = 0;

	uint32_t _flags;

	// TextureShareReceiver
	std::unique_ptr<texture_share_client_t> _receiver = nullptr;
	std::string                             _channel_name;

#ifndef USE_OPENGL
	VkFence _fence;
#endif

	void _create_initial_texture(const uint64_t width, const uint64_t height, const godot::Image::Format format);
	void receive_texture_internal();
};
