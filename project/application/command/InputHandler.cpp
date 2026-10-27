#include "InputHandler.h"

Vector2 MappedInputHandler::GetDirection()
{
	return inputMapping_->GetAxis(InputAction::Move);
}

bool MappedInputHandler::IsActionPressed(InputAction action)
{
	return inputMapping_->IsPressed(action);
}

bool MappedInputHandler::IsActionTriggered(InputAction action)
{
	return inputMapping_->IsTriggered(action);
}
