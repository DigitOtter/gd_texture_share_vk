# GdExtension for Texture Sharing between Vulkan and OpenGL instances

GdExtension to exchange textures with other plugins. Can be used to exchange images without performing a CPU roundtrip. 

NOTE: This plugin requires a custom build of Godot with https://github.com/DigitOtter/gd_module_texture_share_vk.git! A custom build is required to activate a Vulkan instances's external texture sharing capabilities on startup.

## Build

### Linux

Install the underlying API:
- Follow the instructions under [texture-share-vk](https://github.com/DigitOtter/texture-share-vk) to install the TextureShareVk API library

Create a custom Godot build:
- Download and compile godot with:
  ```bash
  git clone https://github.com/godotengine/godot.git
  git checkout 4.1
  git submodule update --init --recursive
  git clone https://github.com/DigitOtter/gd_module_texture_share_vk.git ./modules/gd_module_texture_share_vk
  
  # For the editor, run
  scons platform=linuxbsd target=editor -- -j$(nproc)
  
  # For exports, run
  scons platform=linuxbsd target=template_release production=yes -- -j$(nproc)
  ```
- For further information, visit (Godot's page on custom builds)[https://docs.godotengine.org/en/stable/contributing/development/compiling/compiling_for_linuxbsd.html]

In the godot project:
- Clone repository into the project's `addons/gd_texture_share_vk/` directory
- Inside the repository's directory, build gd\_texture\_share\_vk: 
  ```bash
  git submodule update --init --recursive
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -GNinja
  cmake --build build
  sudo cmake --install build
  ```
- Restart the editor
- A new texture should now be visible (`TsvReceiveTexture`), along with a new resource (`TsvSender`)

### Windows

- Currently not supported (I'd recommend using the Spout2 OBS plugin on Windows)

## Usage

The plugin offers an additional texture and resource. The `TsvReceiveTexture` texture can be used to receive an image, and the `TsvSender` resource can be used to receive an image. An example project is available at [https://github.com/DigitOtter/gd\_texture\_share\_vk\_example.git](https://github.com/DigitOtter/gd_texture_share_vk_example.git)

For the `TsvReceiveTexture` texture:
- Add the texture anywhere an `ImageTexture` could be added
- In the texture's properties, set the name under which to look for external images

For the `TsvSender` resource:
- Load the resource in any script
- Set the name under which to share images with `set_shared_texture_name`
- Set the texture to share with `set_texture`
- Connect the sender to Godot's post-frame processing with `connect_to_frame_post_draw`

