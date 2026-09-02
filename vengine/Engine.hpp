#pragma once

#include <vengine/Common.hpp>
#include <vengine/animation/Tween.hpp>
#include <vengine/assets/AssetDatabase.hpp>
#include <vengine/audio/Audio.hpp>
#include <vengine/core/Config.hpp>
#include <vengine/core/Error.hpp>
#include <vengine/core/Types.hpp>
#include <vengine/input/Input.hpp>
#include <vengine/physics/Physics.hpp>
#include <vengine/platform/Platform.hpp>
#include <vengine/renderer/Camera2D.hpp>
#include <vengine/renderer/DebugDraw.hpp>
#include <vengine/renderer/RenderCommand.hpp>
#include <vengine/renderer/Renderer.hpp>
#include <vengine/scene/Scene.hpp>
#include <vengine/scripting/NativeScript.hpp>
#include <vengine/scripting/Scripting.hpp>

#include <memory>

namespace vengine {

/// The engine facade. Matches the README public API:
///
///   Engine::Initialize();
///   Engine::Update(deltaTime);
///   Engine::Render();
///   Engine::Shutdown();
///
/// The engine owns the platform, renderer, physics, audio, input, assets and
/// scene manager. Platform layers inject concrete implementations of the
/// abstract interfaces at startup.
class Engine {
public:
    Engine();
    ~Engine();

    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    /// Bring up subsystems. Platform must be installed first.
    Result<void> initialize();
    void         update(float dt);
    void         render();
    void         shutdown();

    bool         is_running() const noexcept { return running_; }

    // ---- Subsystem access (game code & editor) ------------------------------
    platform::Platform&       platform() noexcept { return platform_; }
    input::Input&             input() noexcept { return input_; }
    scene::SceneManager&      scene_manager() noexcept { return scene_manager_; }
    core::Config&             config() noexcept { return config_; }
    assets::AssetDatabase&    assets() noexcept { return assets_; }
    animation::TweenManager&  tweens() noexcept { return tweens_; }
    renderer::Camera2D&       camera() noexcept { return camera_; }
    renderer::RenderCommandBuffer& command_buffer() noexcept { return cmd_buf_; }
    renderer::DebugDraw&     debug_draw() noexcept { return debug_draw_; }
    math::Rng&                rng() noexcept { return rng_; }

    /// Inject platform-supplied implementations. Ownership transfers in.
    void install_renderer(std::unique_ptr<renderer::IRenderer> r);
    void install_physics(std::unique_ptr<physics::IPhysicsWorld> p);
    void install_audio(std::unique_ptr<audio::IAudioEngine> a);
    void install_scripting(std::unique_ptr<scripting::IScriptRuntime> s);

    renderer::IRenderer*      renderer() noexcept { return renderer_.get(); }
    physics::IPhysicsWorld*   physics() noexcept { return physics_.get(); }
    audio::IAudioEngine*      audio() noexcept { return audio_.get(); }
    scripting::IScriptRuntime* scripting() noexcept {
        return scripting_ ? scripting_.get()
                          : static_cast<scripting::IScriptRuntime*>(native_scripts_.get());
    }
    scripting::NativeScriptRuntime* native_scripts() noexcept { return native_scripts_.get(); }

    /// Install the built-in native C++ script runtime (registers it as both
    /// the IScriptRuntime and a directly-accessible behaviour host).
    void install_native_scripting();

private:
    bool                                     running_{false};
    platform::Platform                       platform_;
    input::Input                             input_;
    scene::SceneManager                      scene_manager_;
    core::Config                             config_;
    assets::AssetDatabase                    assets_;
    animation::TweenManager                  tweens_;
    renderer::Camera2D                       camera_;
    renderer::RenderCommandBuffer            cmd_buf_;
    renderer::DebugDraw                      debug_draw_;
    math::Rng                                rng_{0x5EED1234ull};

    std::unique_ptr<renderer::IRenderer>     renderer_;
    std::unique_ptr<physics::IPhysicsWorld>  physics_;
    std::unique_ptr<audio::IAudioEngine>     audio_;
    std::unique_ptr<scripting::IScriptRuntime> scripting_;
    std::unique_ptr<scripting::NativeScriptRuntime> native_scripts_;
};

} // namespace vengine
