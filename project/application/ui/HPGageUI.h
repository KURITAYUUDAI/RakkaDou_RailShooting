#pragma once
#include <memory>
#include <functional>
#include "UIBase.h"
#include "GamePlayUIUtility.h"
#include "Sprite.h"
#include "Text.h"

enum class HPGageDirection
{
	LeftToRight,
	RightToLeft,
	TopToBottom,
	BottomToTop
};

struct HPGageUIConfig
{
	Vector2 backPos = { 32.0f, 32.0f };
	Vector2 fillPos = {32.0f, 32.0f};
	Vector2 backSize =  {300.0f, 32.0f };
	Vector2 fillSize = { 300.0f, 32.0f };

	Vector4 backColor = { 0.8f, 0.8f, 0.8f, 1.0f };
	Vector4 fillColor = { 0.0f, 1.0f, 1.0f, 1.0f };
	Vector4 damageColor = { 1.0f, 0.55f, 0.0f, 0.95f };
	Vector4 hitFlashColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	float damageDelay = 0.18f;
	float damageDrainSpeed = 0.8f;
	float hitFlashDuration = 0.10f;

	std::string backTexturePath = "white1x1.png";
	std::string fillTexturePath = "white1x1.png";

	bool showText = false;
	std::u32string label;
	Vector2 labelPos = { 32.0f, 4.0f };
	Vector2 valuePos = { 240.0f, 38.0f };
	Vector4 textColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	uint32_t labelFontSize = 22;
	uint32_t valueFontSize = 20;

	HPGageDirection direction = HPGageDirection::LeftToRight;
	UILayer layer = UILayer::Screen;
};

class HPGageUI : public UIBase<GameUIViewModel>
{
public:

	using HPGetter = std::function<const HPViewModel& (const GameUIViewModel&)>;
	using PositionGetter = std::function<Vector2(const GameUIViewModel&)>;

	void Initialize() override;
	void Update(const GameUIViewModel& viewModel, const float& deltaTime) override;
	void Draw() override;

public: // 外部入出力

	void SetConfig(const HPGageUIConfig& config);
	void SetHPGetter(HPGetter getter);
	void SetPositionGetter(PositionGetter getter);

	UILayer GetLayer() const override { return config_.layer; }

private:
	void ApplySprites();
	void ApplyDirectionAnchor();
	void UpdateFillSprite();
	void UpdateDamageEffect(const HPViewModel& hp, float deltaTime);
	void UpdateDamageSprite();
	void UpdateText(const HPViewModel& hp);

private:
	HPGageUIConfig config_{};
	HPGetter hpGetter_;
	PositionGetter positionGetter_;

	float hpRate_ = 1.0f;
	float damageRate_ = 1.0f;
	float damageDelayTimer_ = 0.0f;
	float hitFlashTimer_ = 0.0f;
	int lastObservedCurrentHP_ = -1;
	bool hpStateInitialized_ = false;

	std::unique_ptr<Sprite> backSprite_;
	std::unique_ptr<Sprite> damageSprite_;
	std::unique_ptr<Sprite> fillSprite_;
	std::unique_ptr<Text> labelText_;
	std::unique_ptr<Text> valueText_;
	int displayedCurrentHP_ = -1;
	int displayedMaxHP_ = -1;
};

