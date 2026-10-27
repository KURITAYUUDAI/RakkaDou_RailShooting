#include "PauseMenu.h"

#include "WindowsAPI.h"

namespace
{
	constexpr uint32_t kContinueIndex = 0;
	constexpr uint32_t kTitleIndex = 1;
	constexpr uint32_t kMenuItemCount = 2;
	constexpr const char* kPauseFontPath = "fonts/x8y12pxTheStrongGamer.ttf";
}

void PauseMenu::Initialize()
{
	backdrop_ = std::make_unique<Sprite>();
	backdrop_->Initialize("white1x1.png");
	backdrop_->SetSize({
		static_cast<float>(WindowsAPI::kClientWidth),
		static_cast<float>(WindowsAPI::kClientHeight) });
	backdrop_->SetColor({ 0.0f, 0.0f, 0.0f, 0.68f });

	panel_ = std::make_unique<Sprite>();
	panel_->Initialize("white1x1.png");
	panel_->SetAnchorPoint({ 0.5f, 0.5f });
	panel_->SetPosition({
		static_cast<float>(WindowsAPI::kClientWidth) * 0.5f,
		static_cast<float>(WindowsAPI::kClientHeight) * 0.5f });
	panel_->SetSize({ 520.0f, 320.0f });
	panel_->SetColor({ 0.015f, 0.015f, 0.015f, 0.94f });

	selection_ = std::make_unique<Sprite>();
	selection_->Initialize("white1x1.png");
	selection_->SetAnchorPoint({ 0.5f, 0.5f });
	selection_->SetPosition({ 640.0f, 344.0f });
	selection_->SetSize({ 430.0f, 58.0f });
	selection_->SetColor({ 0.75f, 0.75f, 0.75f, 0.28f });

	heading_ = std::make_unique<Text>();
	heading_->Initialize(kPauseFontPath, 44, U"PAUSE");
	heading_->SetPosition({ 560.0f, 230.0f });
	heading_->SetColor({ 0.9f, 0.0f, 0.0f, 1.0f });

	continueText_ = std::make_unique<Text>();
	continueText_->Initialize(kPauseFontPath, 32, U"CONTINUE");
	continueText_->SetPosition({ 550.0f, 326.0f });

	titleText_ = std::make_unique<Text>();
	titleText_->Initialize(kPauseFontPath, 32, U"TITLE");
	titleText_->SetPosition({ 584.0f, 401.0f });

	keyboardHelpText_ = std::make_unique<Text>();
	keyboardHelpText_->Initialize(
		kPauseFontPath, 18,
		inputMapping_->GetBindingLabel(
			InputContext::Pause, InputAction::MenuUp, InputDevice::Keyboard) +
		U" / " + inputMapping_->GetBindingLabel(
			InputContext::Pause, InputAction::MenuDown, InputDevice::Keyboard) +
		U" : SELECT\n" + inputMapping_->GetBindingLabel(
			InputContext::Pause, InputAction::Confirm, InputDevice::Keyboard) +
		U" : ENTER");
	keyboardHelpText_->SetPosition({ 470.0f, 475.0f });
	keyboardHelpText_->SetColor({ 0.65f, 0.72f, 0.8f, 1.0f });

	controllerHelpText_ = std::make_unique<Text>();
	controllerHelpText_->Initialize(
		kPauseFontPath, 18,
		inputMapping_->GetBindingLabel(
			InputContext::Pause, InputAction::MenuUp, InputDevice::Gamepad) +
		U" / " + inputMapping_->GetBindingLabel(
			InputContext::Pause, InputAction::MenuDown, InputDevice::Gamepad) +
		U" : SELECT\n" + inputMapping_->GetBindingLabel(
			InputContext::Pause, InputAction::Confirm, InputDevice::Gamepad) +
		U" : ENTER");
	controllerHelpText_->SetPosition({ 470.0f, 475.0f });
	controllerHelpText_->SetColor({ 0.65f, 0.72f, 0.8f, 1.0f });

	selectedIndex_ = kContinueIndex;
	isVisible_ = false;
	RefreshSelection();
}

void PauseMenu::Finalize()
{
	if (controllerHelpText_)
	{
		controllerHelpText_->Finalize();
		controllerHelpText_.reset();
	}
	if (keyboardHelpText_)
	{
		keyboardHelpText_->Finalize();
		keyboardHelpText_.reset();
	}
	if (titleText_)
	{
		titleText_->Finalize();
		titleText_.reset();
	}
	if (continueText_)
	{
		continueText_->Finalize();
		continueText_.reset();
	}
	if (heading_)
	{
		heading_->Finalize();
		heading_.reset();
	}
	if (selection_)
	{
		selection_->Finalize();
		selection_.reset();
	}
	if (panel_)
	{
		panel_->Finalize();
		panel_.reset();
	}
	if (backdrop_)
	{
		backdrop_->Finalize();
		backdrop_.reset();
	}

	isVisible_ = false;
}

PauseMenu::Action PauseMenu::Update()
{
	if (!isVisible_)
	{
		return Action::None;
	}

	if (inputMapping_->IsTriggered(InputAction::MenuUp))
	{
		selectedIndex_ = (selectedIndex_ + kMenuItemCount - 1) % kMenuItemCount;
		RefreshSelection();
	}
	else if (inputMapping_->IsTriggered(InputAction::MenuDown))
	{
		selectedIndex_ = (selectedIndex_ + 1) % kMenuItemCount;
		RefreshSelection();
	}

	backdrop_->Update();
	panel_->Update();
	selection_->Update();
	heading_->Update();
	continueText_->Update();
	titleText_->Update();
	keyboardHelpText_->Update();
	controllerHelpText_->Update();

	if (inputMapping_->IsTriggered(InputAction::Confirm))
	{
		return selectedIndex_ == kContinueIndex
			? Action::ContinueGame
			: Action::ReturnToTitle;
	}

	return Action::None;
}

void PauseMenu::Draw()
{
	if (!isVisible_)
	{
		return;
	}

	backdrop_->Draw();
	panel_->Draw();
	selection_->Draw();
	heading_->Draw();
	continueText_->Draw();
	titleText_->Draw();
	if (inputMapping_->GetActiveDevice() == InputDevice::Gamepad)
	{
		controllerHelpText_->Draw();
	} else
	{
		keyboardHelpText_->Draw();
	}
}

void PauseMenu::SetVisible(bool visible)
{
	if (visible && !isVisible_)
	{
		selectedIndex_ = kContinueIndex;
		RefreshSelection();
	}

	isVisible_ = visible;
}

void PauseMenu::RefreshSelection()
{
	const bool continueSelected = selectedIndex_ == kContinueIndex;
	selection_->SetPosition({ 640.0f, continueSelected ? 344.0f : 419.0f });
	continueText_->SetColor(continueSelected
		? Vector4{ 0.9f, 0.0f, 0.0f, 1.0f }
		: Vector4{ 0.85f, 0.85f, 0.85f, 1.0f });
	titleText_->SetColor(!continueSelected
		? Vector4{ 0.9f, 0.0f, 0.0f, 1.0f }
		: Vector4{ 0.85f, 0.85f, 0.85f, 1.0f });
}

