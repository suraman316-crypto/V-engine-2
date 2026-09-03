#pragma once

#include <vengine/Common.hpp>
#include <vengine/core/Error.hpp>
#include <vengine/core/Types.hpp>
#include <vengine/math/Color.hpp>
#include <vengine/math/Rect.hpp>
#include <vengine/math/Transform2D.hpp>
#include <vengine/math/Vec2.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace vengine::renderer {

/// Backend graphics API. The engine targets OpenGL ES 3.2 but is designed so
/// Vulkan can be added without changing game code.
enum class Backend {
    OpenGLES32,
    Vulkan,   ///< Reserved for future; not yet implemented.
};

/// A GPU texture handle. Opaque id; the concrete backend owns the resource.
struct TextureHandle { std::uint32_t id{0}; bool valid() const noexcept { return id != 0; } };
struct ShaderHandle  { std::uint32_t id{0}; bool valid() const noexcept { return id != 0; } };
struct MaterialHandle { std::uint32_t id{0}; bool valid() const noexcept { return id != 0; } };
struct RenderTargetHandle { std::uint32_t id{0}; bool valid() const noexcept { return id != 0; } };

/// One drawable sprite submitted to the renderer each frame. Sorted and
/// batched by the backend to minimize draw calls / texture switches.
struct SpriteDraw {
    math::Transform2D transform;
    TextureHandle     texture;
    math::Rectf       uv{0.0f, 0.0f, 1.0f, 1.0f};
    math::Color       color{1.0f, 1.0f, 1.0f, 1.0f};
    int               layer{0};
    int               order_in_layer{0};
    bool              flip_x{false};
    bool              flip_y{false};
};

/// 2D camera the renderer uses to build the view matrix.
struct Camera {
    math::Vec2f position{0.0f, 0.0f};
    float       zoom{1.0f};
    float       rotation{0.0f};
    math::Vec2f viewport{1280.0f, 720.0f};
    math::Color clear_color{0.1f, 0.1f, 0.12f, 1.0f};
};

/// Abstract renderer interface. Game code never calls this directly; the
/// Scene/Editor call it. Backends (GLES, future Vulkan) implement it.
class IRenderer {
public:
    virtual ~IRenderer() = default;

    virtual Backend backend() const noexcept = 0;

    /// Initialize against a native surface handle (from IWindow).
    virtual Result<void> initialize(void* native_window_handle) = 0;
    /// Called when the surface is recreated (rotation, resume from background).
    virtual Result<void> recreate(void* native_window_handle) = 0;
    virtual void         shutdown() = 0;

    virtual bool         is_valid() const noexcept = 0;

    // ---- Resource creation --------------------------------------------------
    virtual Result<TextureHandle> create_texture(int width, int height,
        const std::uint8_t* pixels) = 0;
    virtual void         destroy_texture(TextureHandle t) = 0;

    virtual Result<ShaderHandle>  compile_shader(std::string_view vertex_src,
        std::string_view fragment_src) = 0;
    virtual void         destroy_shader(ShaderHandle s) = 0;

    // ---- Frame submission ---------------------------------------------------
    virtual void         begin_frame(const Camera& cam) = 0;
    virtual void         submit_sprites(const SpriteDraw* sprites, std::size_t count) = 0;
    virtual void         end_frame() = 0;

    /// Query the last error detail (e.g. shader info log).
    virtual std::string  last_error() const = 0;
};

} // namespace vengine::renderer
