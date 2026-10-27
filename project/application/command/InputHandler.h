#pragma once

#include "application/command/input/InputMapping.h"

class IInputHandler
{
public:
	virtual ~IInputHandler() = default;
	virtual Vector2 GetDirection() = 0;
	virtual bool IsActionPressed(InputAction action) = 0;
	virtual bool IsActionTriggered(InputAction action) = 0;
};

// InputMappingを既存のPlayerCommandから利用するための安定したアダプター。
// 接続デバイスが変わっても、このオブジェクト自体のアドレスは変わらない。
class MappedInputHandler final : public IInputHandler
{
public:
	explicit MappedInputHandler(
		InputMapping* inputMapping = InputMapping::GetInstance())
		: inputMapping_(inputMapping)
	{
	}

	Vector2 GetDirection() override;
	bool IsActionPressed(InputAction action) override;
	bool IsActionTriggered(InputAction action) override;

private:
	InputMapping* inputMapping_ = nullptr;
};
