#pragma once

enum class WorldPresentation
{
	EntryRoom,
	GameplayWorld,
};

enum class GameplaySimulation
{
	Frozen,
	Running,
};

enum class PlayerControl
{
	Sequence,
	Gameplay,
};

enum class PlayerVisibility
{
	Hidden,
	Visible,
};

enum class GameplayActorVisibility
{
	Hidden,
	Visible,
};

enum class HudVisibility
{
	Hidden,
	Visible,
};

struct SequencePolicy
{
	WorldPresentation world = WorldPresentation::GameplayWorld;
	GameplaySimulation simulation = GameplaySimulation::Frozen;
	PlayerControl playerControl = PlayerControl::Sequence;
	PlayerVisibility playerVisibility = PlayerVisibility::Visible;
	GameplayActorVisibility gameplayActors = GameplayActorVisibility::Hidden;
	HudVisibility hud = HudVisibility::Hidden;
	bool pauseEnabled = false;
};
