#pragma once
#include <unordered_map>

#include "world.h"
#include "raylib.h"
#include "player.h"

struct Renderer {
	Renderer(World &world, Player &player): world(world), player(player){};
	
	void draw_world();
	void draw_player();
	void draw_ui();

private:
	const World &world;
	const Player &player;
	const std::unordered_map<BlockType, Color> block_color =
	{
		{BlockType::Grass, GREEN},
		{BlockType::Dirt, BROWN}
	};

	void draw_player_target();

};
