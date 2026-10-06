#include "HPGageUI.h"
#include "ImGuiManager.h"

#include <algorithm>
#include <string>

namespace
{
	std::u32string ToU32String(const std::string& text)
	{
		return std::u32string(text.begin(), text.end());
	}
}

void HPGageUI::Initialize()
{
	/*backSprite_ = std::make_unique<Sprite>();
	backSprite_->Initialize("white1x1.png");
	backSprite_->SetPosition({32, 32});
	backSprite_->SetSize({300, 32});
	backSprite_->SetColor({0.8f, 0.8f, 0.8f, 1.0f});;
	fillSprite_ = std::make_unique<Sprite>();
	fillSprite_->Initialize("white1x1.png");
	fillSprite_->SetPosition({ 32, 32 });
	fillSprite_->SetSize({ 300.0f, 32.0f });
	fillSprite_->SetColor({ 0.0f, 1.0f, 1.0f, 1.0f });*/

	backSprite_ = std::make_unique<Sprite>();
	backSprite_->Initialize(config_.backTexturePath);

	damageSprite_ = std::make_unique<Sprite>();
	damageSprite_->Initialize(config_.fillTexturePath);

	fillSprite_ = std::make_unique<Sprite>();
	fillSprite_->Initialize(config_.fillTexturePath);

	ApplySprites();

	if (config_.showText)
	{
		labelText_ = std::make_unique<Text>();
		labelText_->Initialize(
			"fonts/x8y12pxTheStrongGamer.ttf",
			config_.labelFontSize,
			config_.label);
		labelText_->SetPosition(config_.labelPos);
		labelText_->SetColor(config_.textColor);

		valueText_ = std::make_unique<Text>();
		valueText_->Initialize(
			"fonts/x8y12pxTheStrongGamer.ttf",
			config_.valueFontSize,
			U"0 / 0");
		valueText_->SetPosition(config_.valuePos);
		valueText_->SetColor(config_.textColor);
	}

	hpRate_ = 1.0f;
	damageRate_ = 1.0f;
}

void HPGageUI::Update(const GameUIViewModel & viewModel, const float& deltaTime)
{
	if (!hpGetter_)
	{
		return;
	}

#ifdef _DEBUG
	//ImGui::Begin("HPGageUI Setting");

	//int playerHP = viewModel.playerHitPoint.currentHitPoint;
	//ImGui::InputInt("playerHp", &playerHP, 0, 0, ImGuiInputTextFlags_ReadOnly);
	//ImGui::InputFloat("hpRate", &hpRate_);

	//ImGui::End();
#endif

	const HPViewModel& hp = hpGetter_(viewModel);

	if (hp.maxHitPoint <= 0)
	{
		hpRate_ = 0.0f;
	} else
	{
		hpRate_ =
			static_cast<float>(hp.currentHitPoint) /
			static_cast<float>(hp.maxHitPoint);
	}
	if (positionGetter_)
	{
		Vector2 position = positionGetter_(viewModel);
		config_.backPos = position;
		config_.fillPos = position;
		backSprite_->SetPosition(position);
		damageSprite_->SetPosition(position);
		fillSprite_->SetPosition(position);
	#ifdef _DEBUG

		ImGui::Begin("enemyHPGage");
		ImGui::InputFloat2("hpGagePosition", &position.x);
		ImGui::End();
	#endif
	}


	hpRate_ = std::clamp(hpRate_, 0.0f, 1.0f);

	UpdateDamageEffect(hp, deltaTime);
	UpdateFillSprite();
	UpdateDamageSprite();
	UpdateText(hp);
	

	backSprite_->Update();
	damageSprite_->Update();
	fillSprite_->Update();
	if (labelText_)
	{
		labelText_->Update();
	}
	if (valueText_)
	{
		valueText_->Update();
	}
}

void HPGageUI::Draw()
{
	if (!isVisible_)
	{
		return;
	}

	backSprite_->Draw();
	damageSprite_->Draw();
	fillSprite_->Draw();
	if (labelText_)
	{
		labelText_->Draw();
	}
	if (valueText_)
	{
		valueText_->Draw();
	}
}

void HPGageUI::SetConfig(const HPGageUIConfig& config)
{
	config_ = config;

	if (backSprite_ && damageSprite_ && fillSprite_)
	{
		ApplySprites();
	}
}

void HPGageUI::SetHPGetter(HPGetter getter)
{
	hpGetter_ = std::move(getter);
}

void HPGageUI::SetPositionGetter(PositionGetter getter)
{
	positionGetter_ = std::move(getter);
}

void HPGageUI::ApplySprites()
{
	/*backSprite_->SetTexture(config_.backTexturePath);*/
	backSprite_->SetPosition(config_.backPos);
	backSprite_->SetSize(config_.backSize);
	backSprite_->SetColor(config_.backColor);

	damageSprite_->SetColor(config_.damageColor);

	/*fillSprite_->SetTexture(config_.fillTexturePath);*/
	fillSprite_->SetColor(config_.fillColor);

	ApplyDirectionAnchor();
	UpdateFillSprite();
	UpdateDamageSprite();

	backSprite_->Update();
	damageSprite_->Update();
	fillSprite_->Update();
}

void HPGageUI::ApplyDirectionAnchor()
{
	Vector2 anchor = { 0.0f, 0.0f };
	Vector2 position = config_.fillPos;

	switch (config_.direction)
	{
	case HPGageDirection::LeftToRight:
		anchor = { 0.0f, 0.0f };
		position = config_.fillPos;
		break;

	case HPGageDirection::RightToLeft:
		anchor = { 1.0f, 0.0f };
		position = {
			config_.fillPos.x,// + config_.fillSize.x,
			config_.fillPos.y
		};
		break;

	case HPGageDirection::TopToBottom:
		anchor = { 0.0f, 0.0f };
		position = config_.fillPos;
		break;

	case HPGageDirection::BottomToTop:
		anchor = { 0.0f, 1.0f };
		position = {
			config_.fillPos.x,
			config_.fillPos.y, //+ config_.fillSize.y
		};
		break;
	}

	backSprite_->SetAnchorPoint(anchor);
	damageSprite_->SetAnchorPoint(anchor);
	fillSprite_->SetAnchorPoint(anchor);
	damageSprite_->SetPosition(position);
	fillSprite_->SetPosition(position);
}

void HPGageUI::UpdateFillSprite()
{
	Vector2 size = config_.fillSize;

	switch (config_.direction)
	{
	case HPGageDirection::LeftToRight:
	case HPGageDirection::RightToLeft:
		size.x = config_.fillSize.x * hpRate_;
		break;

	case HPGageDirection::TopToBottom:
	case HPGageDirection::BottomToTop:
		size.y = config_.fillSize.y * hpRate_;
		break;
	}

	fillSprite_->SetSize(size);
}

void HPGageUI::UpdateDamageEffect(const HPViewModel& hp, float deltaTime)
{
	const float safeDeltaTime = (std::max)(deltaTime, 0.0f);

	if (!hpStateInitialized_)
	{
		damageRate_ = hpRate_;
		lastObservedCurrentHP_ = hp.currentHitPoint;
		hpStateInitialized_ = true;
		fillSprite_->SetColor(config_.fillColor);
		return;
	}

	if (hp.currentHitPoint < lastObservedCurrentHP_)
	{
		float previousRate = damageRate_;
		if (hp.maxHitPoint > 0)
		{
			previousRate = static_cast<float>(hp.previousHitPoint) /
				static_cast<float>(hp.maxHitPoint);
		}

		damageRate_ = (std::max)(
			damageRate_, std::clamp(previousRate, 0.0f, 1.0f));
		damageDelayTimer_ = (std::max)(config_.damageDelay, 0.0f);
		hitFlashTimer_ = (std::max)(config_.hitFlashDuration, 0.0f);
	}
	else if (hp.currentHitPoint > lastObservedCurrentHP_)
	{
		// 回復時は残像を残さず、現在HPへ即座に追従する。
		damageRate_ = hpRate_;
		damageDelayTimer_ = 0.0f;
	}

	lastObservedCurrentHP_ = hp.currentHitPoint;

	if (damageDelayTimer_ > 0.0f)
	{
		damageDelayTimer_ = (std::max)(damageDelayTimer_ - safeDeltaTime, 0.0f);
	}
	else
	{
		const float drainAmount =
			(std::max)(config_.damageDrainSpeed, 0.0f) * safeDeltaTime;
		damageRate_ = (std::max)(hpRate_, damageRate_ - drainAmount);
	}

	if (hitFlashTimer_ > 0.0f)
	{
		hitFlashTimer_ = (std::max)(hitFlashTimer_ - safeDeltaTime, 0.0f);
		fillSprite_->SetColor(config_.hitFlashColor);
	}
	else
	{
		fillSprite_->SetColor(config_.fillColor);
	}
}

void HPGageUI::UpdateDamageSprite()
{
	Vector2 size = config_.fillSize;

	switch (config_.direction)
	{
	case HPGageDirection::LeftToRight:
	case HPGageDirection::RightToLeft:
		size.x = config_.fillSize.x * damageRate_;
		break;

	case HPGageDirection::TopToBottom:
	case HPGageDirection::BottomToTop:
		size.y = config_.fillSize.y * damageRate_;
		break;
	}

	damageSprite_->SetSize(size);
}

void HPGageUI::UpdateText(const HPViewModel& hp)
{
	if (!config_.showText || !valueText_)
	{
		return;
	}

	if (displayedCurrentHP_ == hp.currentHitPoint &&
		displayedMaxHP_ == hp.maxHitPoint)
	{
		return;
	}

	displayedCurrentHP_ = hp.currentHitPoint;
	displayedMaxHP_ = hp.maxHitPoint;

	const std::string value =
		std::to_string((std::max)(hp.currentHitPoint, 0)) + " / " +
		std::to_string((std::max)(hp.maxHitPoint, 0));
	valueText_->SetText(ToU32String(value));
}
