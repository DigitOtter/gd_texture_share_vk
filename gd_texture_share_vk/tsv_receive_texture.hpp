#pragma once

#include <gdextension_interface.h>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/texture2d.hpp>

#include "rendering_backend.hpp"

/*! \brief Receive a shared texture from other processes
 */
class TsvReceiveTexture : public godot::Texture2D
{
	GDCLASS(TsvReceiveTexture, Texture2D);

	public:
	TsvReceiveTexture();
	~TsvReceiveTexture() override;

	int32_t _get_width() const override { return this->_width; }

	int32_t _get_height() const override { return this->_height; }

	bool _has_alpha() const override { return true; }

	// virtual godot::RID _get_rid() override { return this->_texture; }

	virtual void set_flags(const int64_t flags) { _flags = flags; }

	virtual int64_t get_flags() const { return _flags; }

	// Functions copied from ImageTexture2D
	void _draw(const godot::RID &to_canvas_item, const godot::Vector2 &pos, const godot::Color &modulate,
	           bool transpose) const override;
	void _draw_rect(const godot::RID &to_canvas_item, const godot::Rect2 &rect, bool tile, const godot::Color &modulate,
	                bool transpose) const override;
	void _draw_rect_region(const godot::RID &to_canvas_item, const godot::Rect2 &rect, const godot::Rect2 &src_rect,
	                       const godot::Color &modulate, bool transpose, bool clip_uv) const override;

	/*! \brief Get the share channel name
	 */
	godot::String get_shared_texture_name() const;

	/*! \brief Set the share channel name
	 */
	void set_shared_texture_name(godot::String shared_name);

	/*! \brief Manually receive texture. SHOULD be called after frame has been prepared (e.g. after `await
	 * get_tree().process_frame`). It's easier to just connect this SharedTexture to the RenderingDevice's
	 * frame_pre_draw with `connect_to_frame_pre_draw`
	 */
	void _receive_texture();

	/*! \brief Connect to the RenderingDevice's frame_pre_draw signal. This automatically updates the shared texture
	 * before every frame draw
	 */
	void connect_to_frame_pre_draw();

	/*! \brief Check if connected to RenderingDevice's frame_pre_draw signal
	 */
	bool is_connected_to_frame_pre_draw();

	/*! \brief Disconnect from the RenderingDevice's frame_post_draw signal
	 */
	void disconnect_to_frame_pre_draw();

	protected:
	static void _bind_methods();

	/*! \brief Setup client to receive textures for name's shared texture
	 */
	// bool _create_receiver(const std::string &name);

	bool _check_and_update_shared_texture();
	void _update_texture(const uint64_t width, const uint64_t height, const godot::Image::Format format);

	private:
	// Texture
	godot::RID   _texture    = godot::RID();
	texture_id_t _texture_id = 0;

	int32_t _width  = 0;
	int32_t _height = 0;

	bool _image_found = false;

	uint32_t _flags;

	// TextureShareReceiver
	texture_share_client_t _tsv_client;
	std::string            _shared_texture_name;

#ifndef USE_OPENGL
	VkFence _fence;
#endif

	void _create_initial_texture(const uint64_t width, const uint64_t height, const godot::Image::Format format);
	void receive_texture_internal();
};
