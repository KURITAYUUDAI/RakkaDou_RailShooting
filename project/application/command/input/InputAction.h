#pragma once

enum class InputAction
{
	Move,
	Shot,
	Avoid,
	LockOn,
	Melee,
	Confirm,
	MenuUp,
	MenuDown,
	Pause,
};

enum class InputContext
{
	Title,
	Gameplay,
	Pause,
	Result,
};

enum class InputDevice
{
	Keyboard,
	Gamepad,
};
