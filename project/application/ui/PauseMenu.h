#pragma once

#include <memory>

#include "Sprite.h"
#include "Text.h"

class PauseMenu
{
public:
	enum class Action
	{
		None,
		ContinueGame,
		ReturnToTitle,
	};

	void Initialize();
	void Finalize();
	Action Update();
	void Draw();

	void SetVisible(bool visible);
	bool IsVisible() const { return isVisible_; }

private:
	void RefreshSelection();

private:
	std::unique_ptr<Sprite> backdrop_;
	std::unique_ptr<Sprite> panel_;
	std::unique_ptr<Sprite> selection_;
	std::unique_ptr<Text> heading_;
	std::unique_ptr<Text> continueText_;
	std::unique_ptr<Text> titleText_;
	std::unique_ptr<Text> helpText_;

	bool isVisible_ = false;
	uint32_t selectedIndex_ = 0;
};
