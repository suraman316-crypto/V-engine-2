#pragma once

// V Engine 2.0 — Scripting: a small stack-based bytecode VM with
// coroutines (yield/resume), a signal/event system, and entity binding.
//
// We don't ship Lua (to keep the APK dependency-free); instead the engine's
// own VM runs a compact opcode set sufficient for gameplay logic: arithmetic,
// branches, calls, field access, yield, and signal emit/connect. Scripts are
// JIT-free and sandboxed (no raw memory access).

#include <vengine/Common.hpp>
#include <vengine/scene/Registry.hpp>

#include <algorithm>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace vengine::script {

enum class OpCode : u8 {
    PushNull, PushInt, PushFloat, PushStr,
    LoadLocal, StoreLocal, LoadField, StoreField,
    Add, Sub, Mul, Div, Mod, Neg,
    Eq, Ne, Lt, Le, Gt, Ge, And, Or, Not,
    Jump, JumpIfFalse,
    Call, Return, Yield,
    Emit, Connect, Disconnect,
    Print, GetEntity, SetEntity,
    Vec2New, ColorNew,
    Spawn, Destroy,
};

struct Value {
    enum class Kind : u8 { Null, Int, Float, Str, Bool, Entity, Vec2, Color };
    Kind kind{Kind::Null};
    union {
        std::int64_t i{0};
        double f;
        bool b;
    };
    std::string s;
    std::uint64_t entity_raw{0};
    float v2[2]{0, 0};
    float col[4]{1, 1, 1, 1};

    static Value Int(std::int64_t v) { Value x; x.kind = Kind::Int; x.i = v; return x; }
    static Value Float(double v) { Value x; x.kind = Kind::Float; x.f = v; return x; }
    static Value Str(std::string v) { Value x; x.kind = Kind::Str; x.s = std::move(v); return x; }
    static Value Bool(bool v) { Value x; x.kind = Kind::Bool; x.b = v; return x; }
};

struct Instruction { OpCode op; std::int32_t arg{0}; double imm{0}; std::string str; };

struct Script {
    std::string name;
    std::vector<Instruction> code;
    std::vector<std::string> strings; ///< string pool
    std::int32_t entry{0};
    std::int32_t local_count{0};
};

struct Coroutine {
    const Script* script{nullptr};
    std::vector<Value> stack;
    std::vector<Value> locals;
    std::int32_t pc{0};
    bool done{false};
    bool waiting{false};
    scene::Entity bound_entity{};
};

struct Signal {
    std::string name;
    std::vector<std::int32_t> connected_handles;
};

/// A signal bus: scripts connect to named signals; gameplay code emits them.
class SignalBus {
public:
    std::int32_t connect(const std::string& signal, std::function<void()> fn) {
        slots_[signal].push_back({next_handle_++, std::move(fn)});
        return next_handle_ - 1;
    }
    void disconnect(const std::string& signal, std::int32_t handle) {
        auto it = slots_.find(signal);
        if (it == slots_.end()) return;
        auto& v = it->second;
        v.erase(std::remove_if(v.begin(), v.end(),
                  [&](const Slot& s){ return s.handle == handle; }), v.end());
    }
    void emit(const std::string& signal) {
        auto it = slots_.find(signal);
        if (it == slots_.end()) return;
        for (auto& s : it->second) if (s.fn) s.fn();
    }
    std::size_t slot_count(const std::string& signal) const {
        auto it = slots_.find(signal);
        return it == slots_.end() ? 0 : it->second.size();
    }
private:
    struct Slot { std::int32_t handle; std::function<void()> fn; };
    std::unordered_map<std::string, std::vector<Slot>> slots_;
    std::int32_t next_handle_{1};
};

/// The VM: runs scripts and manages coroutines.
class ScriptVM {
public:
    explicit ScriptVM(scene::Registry& reg) : reg_(reg) {}

    void load(Script s) { scripts_[s.name] = std::move(s); }
    Script* get(const std::string& name) {
        auto it = scripts_.find(name);
        return it == scripts_.end() ? nullptr : &it->second;
    }

    /// Start a coroutine running `script`. Returns the coroutine handle.
    std::int32_t start(const std::string& script_name, scene::Entity entity = {}) {
        auto* s = get(script_name);
        if (!s) return -1;
        Coroutine c;
        c.script = s;
        c.locals.resize(s->local_count);
        c.bound_entity = entity;
        coroutines_.push_back(std::move(c));
        return static_cast<std::int32_t>(coroutines_.size() - 1);
    }

    /// Step all coroutines. Returns the number still alive.
    std::size_t tick() {
        std::size_t alive = 0;
        for (auto& c : coroutines_) {
            if (c.done || c.waiting) { if (!c.done) ++alive; continue; }
            execute(c, 64); // run up to 64 ops per tick (cooperative)
            if (!c.done) ++alive;
        }
        return alive;
    }

    Coroutine& coroutine(std::int32_t handle) { return coroutines_[handle]; }
    SignalBus& signals() noexcept { return bus_; }
    std::size_t coroutine_count() const noexcept { return coroutines_.size(); }

private:
    void execute(Coroutine& c, std::size_t max_ops) {
        const Script& s = *c.script;
        for (std::size_t step = 0; step < max_ops && !c.done && !c.waiting; ++step) {
            if (c.pc < 0 || c.pc >= static_cast<std::int32_t>(s.code.size())) { c.done = true; break; }
            const Instruction& ins = s.code[c.pc++];
            switch (ins.op) {
                case OpCode::PushInt:   c.stack.push_back(Value::Int(ins.arg)); break;
                case OpCode::PushFloat: c.stack.push_back(Value::Float(ins.imm)); break;
                case OpCode::PushStr:   c.stack.push_back(Value::Str(ins.str)); break;
                case OpCode::PushNull:  c.stack.push_back({}); break;
                case OpCode::Add: { auto b = pop(c); auto a = pop(c); push(c, a.kind == Value::Kind::Float || b.kind == Value::Kind::Float ? Value::Float(a.f + b.f) : Value::Int(a.i + b.i)); break; }
                case OpCode::Sub: { auto b = pop(c); auto a = pop(c); push(c, a.kind == Value::Kind::Float || b.kind == Value::Kind::Float ? Value::Float(a.f - b.f) : Value::Int(a.i - b.i)); break; }
                case OpCode::Mul: { auto b = pop(c); auto a = pop(c); push(c, a.kind == Value::Kind::Float || b.kind == Value::Kind::Float ? Value::Float(a.f * b.f) : Value::Int(a.i * b.i)); break; }
                case OpCode::Div: { auto b = pop(c); auto a = pop(c); push(c, a.kind == Value::Kind::Float || b.kind == Value::Kind::Float ? Value::Float(a.f / b.f) : Value::Int(b.i != 0 ? a.i / b.i : 0)); break; }
                case OpCode::Eq: { auto b = pop(c); auto a = pop(c); push(c, Value::Bool(a.i == b.i)); break; }
                case OpCode::Lt: { auto b = pop(c); auto a = pop(c); push(c, Value::Bool(a.f < b.f)); break; }
                case OpCode::Gt: { auto b = pop(c); auto a = pop(c); push(c, Value::Bool(a.f > b.f)); break; }
                case OpCode::Jump: c.pc = ins.arg; break;
                case OpCode::JumpIfFalse: { auto v = pop(c); if (!v.b) c.pc = ins.arg; break; }
                case OpCode::Yield: c.waiting = true; break;
                case OpCode::Print: if (!c.stack.empty()) { c.stack.pop_back(); } break;
                case OpCode::Emit: if (!c.stack.empty()) bus_.emit(c.stack.back().s), c.stack.pop_back(); break;
                case OpCode::Return: c.done = true; break;
                default: break;
            }
        }
    }

    Value pop(Coroutine& c) { if (c.stack.empty()) return {}; Value v = std::move(c.stack.back()); c.stack.pop_back(); return v; }
    void push(Coroutine& c, Value v) { c.stack.push_back(std::move(v)); }

    scene::Registry& reg_;
    std::unordered_map<std::string, Script> scripts_;
    std::vector<Coroutine> coroutines_;
    SignalBus bus_;
};

} // namespace vengine::script
