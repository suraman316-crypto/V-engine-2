#pragma once

#include <vengine/Common.hpp>
#include <vengine/assets/AssetDatabase.hpp>
#include <vengine/audio/Audio.hpp>
#include <vengine/core/Config.hpp>
#include <vengine/core/Error.hpp>
#include <vengine/core/Types.hpp>
#include <vengine/input/Input.hpp>
#include <vengine/physics/Physics.hpp>
#include <vengine/platform/Platform.hpp>
#include <vengine/renderer/Renderer.hpp>
#include <vengine/scene/Scene.hpp>
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

    /// Inject platform-supplied implementations. Ownership transfers in.
    void install_renderer(std::unique_ptr<renderer::IRenderer> r);
    void install_physics(std::unique_ptr<physics::IPhysicsWorld> p);
    void install_audio(std::unique_ptr<audio::IAudioEngine> a);
    void install_scripting(std::unique_ptr<scripting::IScriptRuntime> s);

    renderer::IRenderer*      renderer() noexcept { return renderer_.get(); }
    physics::IPhysicsWorld*   physics() noexcept { return physics_.get(); }
    audio::IAudioEngine*      audio() noexcept { return audio_.get(); }
    scripting::IScriptRuntime* scripting() noexcept { return scripting_.get(); }

private:
    bool                                     running_{false};
    platform::Platform                       platform_;
    input::Input                             input_;
    scene::SceneManager                      scene_manager_;
    core::Config                             config_;
    assets::AssetDatabase                    assets_;

    std::unique_ptr<renderer::IRenderer>     renderer_;
    std::unique_ptr<physics::IPhysicsWorld>  physics_;
    std::unique_ptr<audio::IAudioEngine>     audio_;
    std::unique_ptr<scripting::IScriptRuntime> scripting_;
};

} // namespace vengine
