#pragma once

// V Engine 2.0 — Behavior trees: a composable AI decision-making structure.
//
// Nodes return Success/Failure/Running. Composites (Sequence, Selector,
// Parallel) orchestrate child execution. Decorators wrap a single child.
// Leaves are actions (lambdas) or conditions. BTs are more debuggable and
// reusable than state machines for complex AI.

#include <vengine/Common.hpp>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace vengine::ai {

enum class BTStatus : u8 { Success, Failure, Running };

class BTNode {
public:
    virtual ~BTNode() = default;
    virtual BTStatus tick() = 0;
    virtual std::string name() const { return "Node"; }
};

using BTAction = std::function<BTStatus()>;

class ActionNode : public BTNode {
public:
    ActionNode(std::string n, BTAction fn) : name_(std::move(n)), fn_(std::move(fn)) {}
    BTStatus tick() override { return fn_ ? fn_() : BTStatus::Failure; }
    std::string name() const override { return name_; }
private:
    std::string name_;
    BTAction fn_;
};

class ConditionNode : public BTNode {
public:
    ConditionNode(std::function<bool()> cond) : cond_(std::move(cond)) {}
    BTStatus tick() override { return cond_ && cond_() ? BTStatus::Success : BTStatus::Failure; }
private:
    std::function<bool()> cond_;
};

/// Sequence: runs children left-to-right; fails on first failure, returns
/// Running if a child is Running, succeeds only when all succeed.
class SequenceNode : public BTNode {
public:
    explicit SequenceNode(std::string n = "Sequence") : name_(std::move(n)) {}
    SequenceNode& add(std::unique_ptr<BTNode> child) { children_.push_back(std::move(child)); return *this; }
    BTStatus tick() override {
        for (auto& c : children_) {
            BTStatus s = c->tick();
            if (s != BTStatus::Success) return s;
        }
        return BTStatus::Success;
    }
    std::string name() const override { return name_; }
private:
    std::string name_;
    std::vector<std::unique_ptr<BTNode>> children_;
};

/// Selector: runs children until one succeeds; fails only if all fail.
class SelectorNode : public BTNode {
public:
    explicit SelectorNode(std::string n = "Selector") : name_(std::move(n)) {}
    SelectorNode& add(std::unique_ptr<BTNode> child) { children_.push_back(std::move(child)); return *this; }
    BTStatus tick() override {
        for (auto& c : children_) {
            BTStatus s = c->tick();
            if (s != BTStatus::Failure) return s;
        }
        return BTStatus::Failure;
    }
    std::string name() const override { return name_; }
private:
    std::string name_;
    std::vector<std::unique_ptr<BTNode>> children_;
};

/// Parallel: runs all children every tick; succeeds if `req_success` succeed,
/// fails if `req_failure` fail.
class ParallelNode : public BTNode {
public:
    ParallelNode(std::size_t req_success, std::size_t req_failure)
        : req_success_(req_success), req_failure_(req_failure) {}
    ParallelNode& add(std::unique_ptr<BTNode> child) { children_.push_back(std::move(child)); return *this; }
    BTStatus tick() override {
        std::size_t succ = 0, fail = 0;
        for (auto& c : children_) {
            switch (c->tick()) {
                case BTStatus::Success: ++succ; break;
                case BTStatus::Failure: ++fail; break;
                default: break;
            }
        }
        if (succ >= req_success_) return BTStatus::Success;
        if (fail >= req_failure_) return BTStatus::Failure;
        return BTStatus::Running;
    }
private:
    std::size_t req_success_, req_failure_;
    std::vector<std::unique_ptr<BTNode>> children_;
};

/// Inverter decorator: flips Success/Failure.
class InverterNode : public BTNode {
public:
    explicit InverterNode(std::unique_ptr<BTNode> child) : child_(std::move(child)) {}
    BTStatus tick() override {
        switch (child_->tick()) {
            case BTStatus::Success: return BTStatus::Failure;
            case BTStatus::Failure: return BTStatus::Success;
            default: return BTStatus::Running;
        }
    }
private:
    std::unique_ptr<BTNode> child_;
};

/// Repeater: keeps ticking its child forever (or N times).
class RepeaterNode : public BTNode {
public:
    explicit RepeaterNode(std::unique_ptr<BTNode> child, int n = -1) : child_(std::move(child)), count_(n) {}
    BTStatus tick() override {
        if (count_ == 0) return BTStatus::Success;
        child_->tick();
        if (count_ > 0) --count_;
        return BTStatus::Running;
    }
private:
    std::unique_ptr<BTNode> child_;
    int count_;
};

class BehaviorTree {
public:
    explicit BehaviorTree(std::unique_ptr<BTNode> root) : root_(std::move(root)) {}
    BTStatus tick() { return root_ ? root_->tick() : BTStatus::Failure; }
private:
    std::unique_ptr<BTNode> root_;
};

} // namespace vengine::ai
