#include <raylib.h>
#include <rlgl.h>

#include "player.h"
#include "world.h"
#include "render.h"


int main() {
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(1280, 720, "Raylib minecraft clone");
	DisableCursor();
	PollInputEvents();


	// initialise state of the world
	Camera3D camera{
		.position = {-2.0f, 4.0f, -2.0f},
		.target = {2.5f, 0, 2.5f},
		.up = {0.0f, 1.0f, 0.0f},
		.fovy = 45.0f,
		.projection = CAMERA_PERSPECTIVE
	};

	Player player{
		.position = {2, 5, 2}
	};

	World world;

	Renderer renderer(world, player);

	SetTargetFPS(60);



	while (!WindowShouldClose()) {
		// User input
		// todo: separate out polling user input from mutating game state
		UpdateCamera(
				&camera,
				CAMERA_FIRST_PERSON
		);

		bool should_delete_target_block = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
		bool should_create_new_block = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);


		// game logic and mutating game state
		player.find_target(world, camera);
		
		// todo: later use enum or something to distinguish mode
		player.manipulate_blocks(world, should_create_new_block, should_delete_target_block);

		player.apply_gravity(GetFrameTime());


		// rendering
		BeginDrawing();
		ClearBackground(RAYWHITE);

		BeginMode3D(camera);
		
		renderer.draw_world();
		renderer.draw_player();

		EndMode3D();
		
		renderer.draw_ui();
		
		EndDrawing();
	}

	CloseWindow();
	return 0;
}

