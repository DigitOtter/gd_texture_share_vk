#include "godot_texture_share.hpp"

#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>

GodotTextureShare::GodotTextureShare()
{
	if(ExternalHandleGl::LoadGlEXT())
		ERR_PRINT_ONCE("Loaded OpenGL Extensions");
	else
		ERR_PRINT("Failed to load OpenGL Extensions");

	this->_create_initial_texture(1, 1, godot::Image::FORMAT_RGBA8);

	GLint drawFboId = 0;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFboId);

	std::array<uint8_t, 4> dat{255, 255, 255, 0};
	this->_receiver.reset(new TextureShareGlClient());
	this->_receiver->InitImage("gd_image", 8, 8, GL_RGBA, true);
	this->_receiver->ClearImage("gd_image", dat.data());
}

GodotTextureShare::~GodotTextureShare()
{
	godot::RenderingServer *const prs = godot::RenderingServer::get_singleton();
	if(this->_texture.is_valid())
	{
		prs->free_rid(this->_texture);
		this->_texture = godot::RID();
	}
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
	this->_update_texture(8, 8, godot::Image::Format::FORMAT_RGB8);
	//	this->_channel_name = shared_name.ascii().ptr();
	//	if(this->_create_receiver(this->_channel_name))
	//		this->_receive_texture();
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

	ClassDB::bind_method(D_METHOD("_receive_texture"), &GodotTextureShare::_receive_texture);
}

bool GodotTextureShare::_create_receiver(const std::string &name)
{
	if(!this->_receiver)
		this->_receiver.reset(new TextureShareGlClient());

	const auto *shared_image_data = this->_receiver->SharedImageHandle(name);
	if(!shared_image_data)
		return false;

	godot::Image::Format format;
	switch(shared_image_data->ImageFormat())
	{
		case GL_BGRA:
		case GL_RGBA:
			format = godot::Image::Format::FORMAT_RGBA8;
			break;

		default:
			format = godot::Image::Format::FORMAT_MAX;
	}

	this->_update_texture(shared_image_data->Width(), shared_image_data->Height(), format);

	return true;
}

void GodotTextureShare::_receive_texture()
{
	if(!this->_receiver)
		return;

	// Receive texture
	const TextureShareGlClient::ImageExtent dim{
		{0,					 0					 },
		{(GLsizei)this->_width, (GLsizei)this->_height},
	};

	GLint drawFboId = 0;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFboId);

	this->_receiver->RecvImageBlit(this->_channel_name, this->_gl_tex_id, GL_TEXTURE_2D, dim, false, drawFboId);
}

void GodotTextureShare::_update_texture(const uint64_t width, const uint64_t height, const godot::Image::Format format)
{
	this->_width  = width;
	this->_height = height;

	godot::Ref<godot::Image> img = godot::Image::create(width, height, false, format);
	img->fill(godot::Color(1.0f, 1.0f, 0.0f));

	godot::RenderingServer *const prs = godot::RenderingServer::get_singleton();
	assert(this->_texture.is_valid());

	// Replace texture (only way to change height and width)
	godot::RID tmp_tex = prs->texture_2d_create(img);
	prs->texture_replace(this->_texture, tmp_tex);
	prs->free_rid(tmp_tex);

	this->_gl_tex_id = prs->texture_get_native_handle(this->_texture, true);

	//	const godot::String rdname = prs->get_rendering_device()->get_device_vendor_name();
	//	ERR_PRINT(rdname);

	GLint drawFboId = 0;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFboId);

	TextureShareGlClient::ImageExtent dat{
		{0, 0},
		{8, 8}
    };
	this->_receiver->RecvImageBlit("gd_image", this->_gl_tex_id, GL_TEXTURE_2D, dat, false, drawFboId);
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
	this->_gl_tex_id                  = prs->texture_get_native_handle(this->_texture);

	prs->texture_set_force_redraw_if_visible(this->_texture, true);
}
