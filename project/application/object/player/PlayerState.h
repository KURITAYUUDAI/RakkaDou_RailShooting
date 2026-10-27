#pragma once
#include "myMath.h"
#include <memory>
#include "GameSceneCommand.h"
#include "JustAvoidDarken.h"

class Player;

enum class PlayerStateType
{
	Idle,
	Shot,
	Avoid,
	JustAvoid,
	MeleeAttack,
	CounterMelee,
};

class IPlayerState
{
public:
	virtual PlayerStateType GetType() const = 0;

	virtual void Initialize(Player* player) = 0;
	virtual void Update(Player* player, const float& deltaTime) = 0;
	virtual void Draw(Player* player) = 0;
	virtual void Finalize(Player* player) = 0;
};

class PlayerIdleState : public IPlayerState
{
public:
	PlayerStateType GetType() const override
	{
		return PlayerStateType::Idle;
	}

	void Initialize(Player* player) override;
	void Update(Player* player, const float& deltaTime) override;
	void Draw(Player* player) override;
	void Finalize(Player* player) override;

private:

	std::unique_ptr<IPlayerCommand>      moveCommand_;
	std::unique_ptr<LongPressShotCommand> shotCommand_;
	std::unique_ptr<IPlayerCommand>      avoidCommand_;
	std::unique_ptr<IPlayerCommand>      meleeAttackCommand_;
};

class PlayerMeleeAttackState : public IPlayerState
{
private:
	enum class AttackPhase
	{
		Windup,
		Attack,
		FollowUpWindow,
		FollowUpAttack,
		ComboWindow,
	};

public:
	PlayerStateType GetType() const override { return PlayerStateType::MeleeAttack; }
	void Initialize(Player* player) override;
	void Update(Player* player, const float& deltaTime) override;
	void Draw(Player* player) override;
	void Finalize(Player* player) override;

private:
	void BeginAttack(Player* player);
	void UpdateAttackDirection(Player* player);
	Vector3 GetWindupPosition() const;
	Vector3 GetAttackEndPosition() const;

	AttackPhase phase_ = AttackPhase::Windup;
	float timer_ = 0.0f;
	uint32_t comboIndex_ = 0;
	Vector3 attackDirection_{ 0.0f, 0.0f, 1.0f };

	static constexpr float kWindupDuration_ = 0.045f;
	static constexpr float kAttackDuration_ = 0.075f;
	// 近接終了直後の0.10秒だけ射撃追撃を受け付ける。
	static constexpr float kFollowUpInputStart_ = 0.04f;
	static constexpr float kFollowUpDuration_ = 0.24f;
	static constexpr float kFollowUpAttackDuration_ = 0.08f;
	static constexpr float kComboInputDuration_ = 0.60f;
	static constexpr uint32_t kMaxComboCount_ = 3;
	static constexpr Vector3 kRestPosition_ = { 0.8f, 0.0f, 0.5f };
};

class PlayerCounterMeleeState : public IPlayerState
{
public:
	PlayerStateType GetType() const override { return PlayerStateType::CounterMelee; }
	void Initialize(Player* player) override;
	void Update(Player* player, const float& deltaTime) override;
	void Draw(Player* player) override;
	void Finalize(Player* player) override;

private:
	Vector3 GetTargetLocalPosition(Player* player) const;

	float timer_ = 0.0f;
	Vector3 startPosition_{};
	bool isApproachFinished_ = false;

	static constexpr float kApproachDuration_ = 0.22f;
};

class PlayerShotState : public IPlayerState
{
public:
	PlayerStateType GetType() const override
	{
		return PlayerStateType::Shot;
	}

	void Initialize(Player* player) override;
	void Update(Player* player, const float& deltaTime) override;
	void Draw(Player* player) override;
	void Finalize(Player* player) override;

private:
	float timer_ = 0.0f;
	float duration_ = 0.1f;

	std::unique_ptr<IPlayerCommand>      moveCommand_;
};

class PlayerAvoidState : public IPlayerState
{
public:

	PlayerAvoidState(const Vector2& direction) : inputDirection_(direction) {}

	PlayerStateType GetType() const override
	{
		return PlayerStateType::Avoid;
	}
	
	void Initialize(Player* player) override;
	void Update(Player* player, const float& deltaTime) override;
	void Draw(Player* player) override;
	void Finalize(Player* player) override;

private:
	float timer_ = 0.0f;
	float duration_ = 0.5f;
	float justDuration_ = 0.3f;

	Vector3 avoidDirection_ = { 0.0f, 0.0f, 0.0f };
	Vector2 inputDirection_ = { 0.0f, 0.0f };
	float avoidSpeed_ = 25.0f; // 回避の速度

};

class PlayerJustAvoidState : public IPlayerState
{
public:
	PlayerJustAvoidState(
		const Vector3& direction, JustAvoidDarken* justAvoidDarken) 
		: avoidDirection_(direction), justAvoidDarken_(justAvoidDarken) {}

	PlayerStateType GetType() const override
	{
		return PlayerStateType::JustAvoid;
	}

	void Initialize(Player* player) override;
	void Update(Player* player, const float& deltaTime) override;
	void Draw(Player* player) override;
	void Finalize(Player* player) override;

private:
	float timer_ = 0.0f;
	float duration_ = 0.5f;
	float waitDuration_ = 1.0f;

	Vector3 avoidDirection_ = { 0.0f, 0.0f, 0.0f };
	float avoidSpeed_ = 15.0f; // 回避の速度

	bool isCounter_ = false;

	std::unique_ptr<IPlayerCommand> shotCommand_;

	JustAvoidDarken* justAvoidDarken_;

	JustAvoidDarken::Setting setting
	{
		.duration = 1.0f,
		.maxIntensity = 0.55f,
		.attackRate = 0.05f,
		.returnRate = 0.05f,
		.darkColor = { 0.0f, 0.0f, 0.0f, 1.0f }
	};
};
