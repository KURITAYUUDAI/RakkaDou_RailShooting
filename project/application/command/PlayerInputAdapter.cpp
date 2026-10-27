#include "PlayerInputAdapter.h"

PlayerInputAdapter::PlayerInputAdapter()
{
	handler_ = std::make_unique<MappedInputHandler>();
}

IInputHandler* PlayerInputAdapter::GetHandler()
{
	return handler_.get();
}
