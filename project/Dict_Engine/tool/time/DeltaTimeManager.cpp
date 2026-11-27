#include "DeltaTimeManager.h"
#include <algorithm>
#include <cmath>
#include <cassert>
#include "ImGuiManager.h"

std::unique_ptr<DeltaTimeManager> DeltaTimeManager::instance_ = nullptr;

class DeltaTimeManager::DeltaTimeIdlePhase final : public IDeltaTimePhase
{
public:
    const char* GetName() const override { return "Idle"; }
    std::unique_ptr<IDeltaTimePhase> Update(
        GroupState&, float) override;
};

class DeltaTimeManager::DeltaTimeBlendInPhase final : public IDeltaTimePhase
{
public:
    const char* GetName() const override { return "BlendIn"; }
    std::unique_ptr<IDeltaTimePhase> Update(
        GroupState& state, float rawDeltaTime) override;

private:
    float timer_ = 0.0f;
};

class DeltaTimeManager::DeltaTimeHoldPhase final : public IDeltaTimePhase
{
public:
    const char* GetName() const override { return "Hold"; }
    std::unique_ptr<IDeltaTimePhase> Update(
        GroupState& state, float rawDeltaTime) override;

private:
    float timer_ = 0.0f;
};

class DeltaTimeManager::DeltaTimeBlendOutPhase final : public IDeltaTimePhase
{
public:
    const char* GetName() const override { return "BlendOut"; }
    std::unique_ptr<IDeltaTimePhase> Update(
        GroupState& state, float rawDeltaTime) override;

private:
    float timer_ = 0.0f;
};

std::unique_ptr<DeltaTimeManager::IDeltaTimePhase>
DeltaTimeManager::DeltaTimeIdlePhase::Update(GroupState&, float)
{
    return nullptr;
}

std::unique_ptr<DeltaTimeManager::IDeltaTimePhase>
DeltaTimeManager::DeltaTimeBlendInPhase::Update(
    GroupState& state, float rawDeltaTime)
{
    timer_ += rawDeltaTime;
    const float t = state.blendDuration > 0.0f
        ? std::clamp(timer_ / state.blendDuration, 0.0f, 1.0f)
        : 1.0f;
    const float ease = 1.0f - std::pow(1.0f - t, 3.0f);
    state.currentScale =
        state.defaultScale + (state.targetScale - state.defaultScale) * ease;
    if (t < 1.0f)
    {
        return nullptr;
    }

    state.currentScale = state.targetScale;
    return std::make_unique<DeltaTimeHoldPhase>();
}

std::unique_ptr<DeltaTimeManager::IDeltaTimePhase>
DeltaTimeManager::DeltaTimeHoldPhase::Update(
    GroupState& state, float rawDeltaTime)
{
    timer_ += rawDeltaTime;
    if (timer_ < state.holdTime)
    {
        return nullptr;
    }
    return std::make_unique<DeltaTimeBlendOutPhase>();
}

std::unique_ptr<DeltaTimeManager::IDeltaTimePhase>
DeltaTimeManager::DeltaTimeBlendOutPhase::Update(
    GroupState& state, float rawDeltaTime)
{
    timer_ += rawDeltaTime;
    const float t = state.blendOutDuration > 0.0f
        ? std::clamp(timer_ / state.blendOutDuration, 0.0f, 1.0f)
        : 1.0f;
    const float ease = std::pow(t, 2.0f);
    state.currentScale =
        state.targetScale + (state.defaultScale - state.targetScale) * ease;
    if (t < 1.0f)
    {
        return nullptr;
    }

    state.currentScale = state.defaultScale;
    return std::make_unique<DeltaTimeIdlePhase>();
}

DeltaTimeManager* DeltaTimeManager::GetInstance()
{
    if (instance_ == nullptr)
    {
        instance_ = std::make_unique<DeltaTimeManager>(ConstructorKey());
    }
    return instance_.get();
}

void DeltaTimeManager::Finalize()
{
    instance_.reset();
}

void DeltaTimeManager::Initialize()
{
    groups_.clear();
    // 全グループをデフォルト値で登録
    for (auto group : {
        DeltaTimeGroup::Player,
        DeltaTimeGroup::Enemy,
        DeltaTimeGroup::Effect,
        DeltaTimeGroup::World,
        DeltaTimeGroup::UI })
    {
        GroupState& state = groups_.try_emplace(group).first->second;
        state.currentScale = 1.0f;
        state.targetScale = 1.0f;
        state.defaultScale = 1.0f;
        state.phase = std::make_unique<DeltaTimeIdlePhase>();
    }
}

void DeltaTimeManager::Update(float rawDeltaTime)
{
    rawDeltaTime_ = rawDeltaTime;
    for (auto& [group, state] : groups_) 
    {
        UpdateGroupState(state, rawDeltaTime);
    }
}

void DeltaTimeManager::ImGuiDebug()
{
#ifdef _DEBUG

    ImGui::Begin("DeltaTime View");

    const char* groupNames[] = { "Player", "Enemy", "Effect", "World", "UI" };
    ImGui::Text("RawDeltaTime : %.4f", rawDeltaTime_);
    ImGui::Text("RawFPS       : %.1f", 1.0f / rawDeltaTime_);
    ImGui::Separator();

    for (auto& [group, state] : groups_)
    {
        int groupIdx = static_cast<int>(group);
        ImGui::Text("[%s]", groupNames[groupIdx]);
        ImGui::SameLine();
        ImGui::Text(
            "Phase: %s",
            state.phase ? state.phase->GetName() : "Unknown");

        // スケールをプログレスバーで可視化（0.0〜2.0の範囲を想定）
        char label[64];
        sprintf_s(label, "%.3f##%d", state.currentScale, groupIdx);
        ImGui::ProgressBar(state.currentScale / 2.0f, ImVec2(-1, 0), label);

        ImGui::Text("  scaled dt : %.4f", rawDeltaTime_ * state.currentScale);
        ImGui::Spacing();
    }


    ImGui::End();

#endif
}

void DeltaTimeManager::UpdateGroupState(GroupState& state, float rawDt) 
{
    if (!state.phase)
    {
        state.phase = std::make_unique<DeltaTimeIdlePhase>();
        return;
    }

    std::unique_ptr<IDeltaTimePhase> nextPhase =
        state.phase->Update(state, rawDt);
    if (nextPhase)
    {
        state.phase = std::move(nextPhase);
    }
}

void DeltaTimeManager::SetGroupScale(DeltaTimeGroup group, float scale) 
{
    groups_[group].currentScale = scale;
}

void DeltaTimeManager::SetGlobalScale(float scale, bool affectPlayer) 
{
    for (auto& [group, state] : groups_) 
    {
        if (!affectPlayer && group == DeltaTimeGroup::Player) continue;
        state.currentScale = scale;
    }
}

float DeltaTimeManager::GetDeltaTime(DeltaTimeGroup group) const 
{
    auto it = groups_.find(group);
    if (it == groups_.end()) return rawDeltaTime_;
    assert(it != groups_.end() && "未登録のTimeGroup。Initializeを確認してください。");
    return rawDeltaTime_ * it->second.currentScale;
}

void DeltaTimeManager::RequestSlowMotion(DeltaTimeGroup group, float scale,
                                    float blendInTime, float holdTime, float blendOutTime) 
{
    auto& state = groups_[group];
    state.targetScale = scale;
    state.blendDuration = blendInTime;
    state.holdTime = holdTime;
    state.blendOutDuration = blendOutTime;
    state.phase = std::make_unique<DeltaTimeBlendInPhase>();
}

void DeltaTimeManager::RequestOtherSlowMotion(DeltaTimeGroup selfGroup, float scale, float blendInTime, float holdTime, float blendOutTime)
{
    for (auto& [group, state] : groups_)
    {
        if(group == selfGroup) continue;
        if(group == DeltaTimeGroup::UI) continue;

        state.targetScale = scale;
        state.blendDuration = blendInTime;
        state.holdTime = holdTime;
        state.blendOutDuration = blendOutTime;
        state.phase = std::make_unique<DeltaTimeBlendInPhase>();
    }
}

void DeltaTimeManager::ResetAll() 
{
    for (auto& [group, state] : groups_) 
    {
        state.currentScale = 1.0f;
        state.phase = std::make_unique<DeltaTimeIdlePhase>();
    }
}
