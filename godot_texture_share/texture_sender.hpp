#pragma once

#include <gdextension_interface.h>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/texture2d.hpp>

#include "rendering_backend.hpp"

/*! \brief Send textures to other processes
 */
class TextureSender : public godot::Resource
{
	GDCLASS(TextureSender, godot::Resource);

	public:
	TextureSender();
	~TextureSender() override;

	/*! \brief Specify the texture to share
	 */
	void set_texture(const godot::Ref<godot::Texture2D> &texture, godot::Image::Format texture_format);

	/*! \brief Get the shared texture
	 */
	godot::Ref<godot::Texture2D> get_texture();

	/*! \brief Set the share channel name
	 */
	void set_shared_texture_name(const godot::String &shared_texture_name);

	/*! \brief Get the share channel name
	 */
	godot::String get_shared_texture_name();

	/*! \brief Explicitly update the shared texture. MUST be called after the frame has been drawn (use after `await
	 * get_tree().process_frame`). It's easier to just connect this SharedTexture to the RenderingDevice's
	 * frame_post_draw with `connect_to_frame_post_draw()`
	 */
	bool send_texture();

	/*! \brief Connect to the RenderingDevice's frame_post_draw signal. This automatically updates the shared texture
	 * after every frame, and also ensures that _texture contains valid data)
	 */
	void connect_to_frame_post_draw();

	/*! \brief Check if connected to RenderingDevice's frame_post_draw signal
	 */
	bool is_connected_to_frame_post_draw();

	/*! \brief Disconnect from the RenderingDevice's frame_post_draw signal
	 */
	void disconnect_to_frame_post_draw();

	protected:
	static void _bind_methods();

	private:
	godot::Ref<godot::Texture2D> _texture;

	std::string          _shared_texture_name;
	uint32_t             _width  = 0;
	uint32_t             _height = 0;
	godot::Image::Format _format = godot::Image::FORMAT_MAX;

	texture_share_client_t _tsv_client;
#ifndef USE_OPENGL
	VkFence _fence;
#endif

	bool update_shared_texture(uint32_t width, uint32_t height, godot::Image::Format format);
	bool check_and_update_shared_texture(godot::Image::Format format);

	bool send_texture_internal();
};
