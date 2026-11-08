#include <raylib.h>
#include <rlgl.h>

#include "player.h"
#include "world.h"
#include "render.h"
#include "physics/physics.h"


void reset(World &world, Player &player) {
	
	world.initialise();

	player.position = {2, 3, 2};

}

int main() {
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(1280, 720, "Raylib minecraft clone");
	DisableCursor();
	PollInputEvents();

	PhysicsWorld physics;


	// initialise state of the world
	
	// todo: put everything in one struct and build it in one place.

	// todo: add reset functionality for easy testing
	
	World world;
	Player player;
	
	reset(world, player);
	
	Renderer renderer(world, player);
	SetTargetFPS(60);

	Camera3D camera = player.get_camera();


	while (!WindowShouldClose()) {
		// User input
		if (IsKeyPressed(KEY_R)) {
			reset(world, player);
		}
		
		bool should_delete_target_block = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
		bool should_create_new_block = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
		bool jump_button_pressed = IsKeyPressed(KEY_SPACE);

		Vector2 mouse_delta = GetMouseDelta();

		Vector3 h_movement = Vector3(
			// Move forward-backward
			(IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) -
			(IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)),
			0,
			// Move right-left
			(IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) -
			(IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)));

		float modifier = 0.002f;

		player.yaw += mouse_delta.x * modifier;
		player.pitch -= mouse_delta.y * modifier;

		constexpr float pitch_limit = std::numbers::pi_v<float> / 2.0f - 0.01f;

		player.pitch = std::clamp(
				player.pitch,
				-pitch_limit,
				pitch_limit
		);



		float dt = GetFrameTime();
		
		// game logic and mutating game state
		player.find_target(world, camera);
		
		// todo: later use enum or something to distinguish mode
		player.manipulate_blocks(world, should_create_new_block, should_delete_target_block);

		Vector3 previous_position = player.position; 
		player.apply_xz_movement(h_movement, camera, dt);
		player.apply_physics(dt);
		player.apply_collisions(world, previous_position);
		// jump requires grounded to be calculated from apply_collisions
		if (jump_button_pressed) {
			player.jump();		
		}

		
		camera = player.get_camera();


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

