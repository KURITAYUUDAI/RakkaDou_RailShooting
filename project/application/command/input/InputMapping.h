#pragma once

#include "InputAction.h"
#include "myMath.h"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class InputManager;

class InputMapping
{
public:
	static InputMapping* GetInstance();
	static void Finalize();

	void Initialize();
	void SetContext(InputContext context) { currentContext_ = context; }

	InputContext GetContext() const { return currentContext_; }
	InputDevice GetActiveDevice() const;

	bool IsPressed(InputAction action) const;
	bool IsTriggered(InputAction action) const;
	Vector2 GetAxis(InputAction action) const;

	std::u32string GetBindingLabel(InputAction action) const;
	std::u32string GetBindingLabel(
		InputContext context, InputAction action, InputDevice device) const;

private:
	struct DigitalBinding
	{
		std::vector<uint8_t> keyboardKeys;
		std::vector<uint16_t> gamepadButtons;
		std::u32string keyboardLabel;
		std::u32string gamepadLabel;
	};

	struct Axis2DBinding
	{
		uint8_t keyboardLeft = 0;
		uint8_t keyboardRight = 0;
		uint8_t keyboardDown = 0;
		uint8_t keyboardUp = 0;
		bool useGamepadLeftStick = false;
		std::u32string keyboardLabel;
		std::u32string gamepadLabel;
	};

	using DigitalActionMap = std::unordered_map<InputAction, DigitalBinding>;
	using AxisActionMap = std::unordered_map<InputAction, Axis2DBinding>;

	const DigitalBinding* FindDigitalBinding(InputAction action) const;
	const Axis2DBinding* FindAxisBinding(InputAction action) const;
	explicit InputMapping(InputManager* inputManager);

private:
	static std::unique_ptr<InputMapping> instance_;
	InputManager* inputManager_ = nullptr;
	InputContext currentContext_ = InputContext::Title;
	std::unordered_map<InputContext, DigitalActionMap> digitalBindings_;
	std::unordered_map<InputContext, AxisActionMap> axisBindings_;
};
