#pragma once
#include "InputHandler.h"
#include <memory>

// 既存のPlayerCommandへ、アドレスが変わらない入力ハンドラーを提供する。
class PlayerInputAdapter
{
public:
	PlayerInputAdapter();

	IInputHandler* GetHandler();

private:
	std::unique_ptr<MappedInputHandler> handler_;
};
