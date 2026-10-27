#define NOMINMAX
#include "InputMapping.h"

#include "InputManager.h"

#include <algorithm>
#include <cmath>

namespace
{
	constexpr float kGamepadDeadzone = 0.2f;
}

std::unique_ptr<InputMapping> InputMapping::instance_ = nullptr;

InputMapping* InputMapping::GetInstance()
{
	if (!instance_)
	{
		instance_.reset(new InputMapping(InputManager::GetInstance()));
	}
	return instance_.get();
}

void InputMapping::Finalize()
{
	instance_.reset();
}

InputMapping::InputMapping(InputManager* inputManager)
	: inputManager_(inputManager)
{
}

void InputMapping::Initialize()
{
	digitalBindings_.clear();
	axisBindings_.clear();

	digitalBindings_[InputContext::Title][InputAction::Confirm] = {
		{ DIK_SPACE },
		{ XINPUT_GAMEPAD_A },
		U"SPACE",
		U"A",
	};

	auto& gameplayDigital = digitalBindings_[InputContext::Gameplay];
	gameplayDigital[InputAction::Shot] = {
		{ DIK_SPACE }, { XINPUT_GAMEPAD_RIGHT_SHOULDER }, U"SPACE", U"RB" };
	gameplayDigital[InputAction::Avoid] = {
		{ DIK_E }, { XINPUT_GAMEPAD_X }, U"E", U"X" };
	gameplayDigital[InputAction::LockOn] = {
		{ DIK_LSHIFT }, { XINPUT_GAMEPAD_LEFT_SHOULDER }, U"LSHIFT", U"LB" };
	gameplayDigital[InputAction::Melee] = {
		{ DIK_B }, { XINPUT_GAMEPAD_B }, U"B", U"B" };
	gameplayDigital[InputAction::Pause] = {
		{ DIK_TAB }, { XINPUT_GAMEPAD_START }, U"TAB", U"START" };

	axisBindings_[InputContext::Gameplay][InputAction::Move] = {
		DIK_A,
		DIK_D,
		DIK_S,
		DIK_W,
		true,
		U"WASD",
		U"L STICK",
	};

	auto& pauseDigital = digitalBindings_[InputContext::Pause];
	pauseDigital[InputAction::MenuUp] = {
		{ DIK_W, DIK_UP }, { XINPUT_GAMEPAD_DPAD_UP }, U"W / UP", U"D-PAD UP" };
	pauseDigital[InputAction::MenuDown] = {
		{ DIK_S, DIK_DOWN }, { XINPUT_GAMEPAD_DPAD_DOWN }, U"S / DOWN", U"D-PAD DOWN" };
	pauseDigital[InputAction::Confirm] = {
		{ DIK_SPACE }, { XINPUT_GAMEPAD_A }, U"SPACE", U"A" };
	pauseDigital[InputAction::Pause] = {
		{ DIK_TAB }, { XINPUT_GAMEPAD_START }, U"TAB", U"START" };

	digitalBindings_[InputContext::Result][InputAction::Confirm] = {
		{ DIK_RETURN },
		{ XINPUT_GAMEPAD_A },
		U"ENTER",
		U"A",
	};
}

InputDevice InputMapping::GetActiveDevice() const
{
	return inputManager_->IsControllerConnected()
		? InputDevice::Gamepad
		: InputDevice::Keyboard;
}

bool InputMapping::IsPressed(InputAction action) const
{
	const DigitalBinding* binding = FindDigitalBinding(action);
	if (!binding)
	{
		return false;
	}

	if (GetActiveDevice() == InputDevice::Gamepad)
	{
		return std::ranges::any_of(
			binding->gamepadButtons,
			[this](uint16_t button)
			{
				return inputManager_->PushButton(button);
			});
	}

	return std::ranges::any_of(
		binding->keyboardKeys,
		[this](uint8_t key)
		{
			return inputManager_->PushKey(key);
		});
}

bool InputMapping::IsTriggered(InputAction action) const
{
	const DigitalBinding* binding = FindDigitalBinding(action);
	if (!binding)
	{
		return false;
	}

	if (GetActiveDevice() == InputDevice::Gamepad)
	{
		return std::ranges::any_of(
			binding->gamepadButtons,
			[this](uint16_t button)
			{
				return inputManager_->TriggerButton(button);
			});
	}

	return std::ranges::any_of(
		binding->keyboardKeys,
		[this](uint8_t key)
		{
			return inputManager_->TriggerKey(key);
		});
}

Vector2 InputMapping::GetAxis(InputAction action) const
{
	const Axis2DBinding* binding = FindAxisBinding(action);
	if (!binding)
	{
		return {};
	}

	if (GetActiveDevice() == InputDevice::Gamepad)
	{
		if (!binding->useGamepadLeftStick)
		{
			return {};
		}

		const float x = inputManager_->GetLeftStickX();
		const float y = inputManager_->GetLeftStickY();
		const float magnitude = std::sqrt(x * x + y * y);
		if (magnitude < kGamepadDeadzone)
		{
			return {};
		}

		const float scale =
			(std::min(magnitude, 1.0f) - kGamepadDeadzone) /
			(1.0f - kGamepadDeadzone);
		return { (x / magnitude) * scale, (y / magnitude) * scale };
	}

	float x = 0.0f;
	float y = 0.0f;
	if (inputManager_->PushKey(binding->keyboardLeft))
	{
		x -= 1.0f;
	}
	if (inputManager_->PushKey(binding->keyboardRight))
	{
		x += 1.0f;
	}
	if (inputManager_->PushKey(binding->keyboardDown))
	{
		y -= 1.0f;
	}
	if (inputManager_->PushKey(binding->keyboardUp))
	{
		y += 1.0f;
	}

	const float magnitude = std::sqrt(x * x + y * y);
	if (magnitude > 1.0f)
	{
		x /= magnitude;
		y /= magnitude;
	}
	return { x, y };
}

std::u32string InputMapping::GetBindingLabel(InputAction action) const
{
	return GetBindingLabel(currentContext_, action, GetActiveDevice());
}

std::u32string InputMapping::GetBindingLabel(
	InputContext context, InputAction action, InputDevice device) const
{
	const auto digitalContextIt = digitalBindings_.find(context);
	if (digitalContextIt != digitalBindings_.end())
	{
		const auto actionIt = digitalContextIt->second.find(action);
		if (actionIt != digitalContextIt->second.end())
		{
			return device == InputDevice::Gamepad
				? actionIt->second.gamepadLabel
				: actionIt->second.keyboardLabel;
		}
	}

	const auto axisContextIt = axisBindings_.find(context);
	if (axisContextIt != axisBindings_.end())
	{
		const auto actionIt = axisContextIt->second.find(action);
		if (actionIt != axisContextIt->second.end())
		{
			return device == InputDevice::Gamepad
				? actionIt->second.gamepadLabel
				: actionIt->second.keyboardLabel;
		}
	}

	return {};
}

const InputMapping::DigitalBinding* InputMapping::FindDigitalBinding(
	InputAction action) const
{
	const auto contextIt = digitalBindings_.find(currentContext_);
	if (contextIt == digitalBindings_.end())
	{
		return nullptr;
	}

	const auto actionIt = contextIt->second.find(action);
	return actionIt == contextIt->second.end() ? nullptr : &actionIt->second;
}

const InputMapping::Axis2DBinding* InputMapping::FindAxisBinding(
	InputAction action) const
{
	const auto contextIt = axisBindings_.find(currentContext_);
	if (contextIt == axisBindings_.end())
	{
		return nullptr;
	}

	const auto actionIt = contextIt->second.find(action);
	return actionIt == contextIt->second.end() ? nullptr : &actionIt->second;
}
