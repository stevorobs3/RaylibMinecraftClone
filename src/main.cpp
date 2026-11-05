#include <raylib.h>

int main() {
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(1280, 720, "Raylib minecraft clone");
	DisableCursor();
	PollInputEvents();

	Camera3D camera{
		.position = {4.0f, 3.0f, 4.0f},
		.target = {0.0f, 0.0f, 0.0f},
		.up = {0.0f, 1.0f, 0.0f},
		.fovy = 45.0f,
		.projection = CAMERA_PERSPECTIVE
	};
		

	SetTargetFPS(60);
	
	while (!WindowShouldClose()) {
		// User input
		UpdateCamera(
				&camera,
				CAMERA_FIRST_PERSON
		);

		BeginDrawing();
		ClearBackground(RAYWHITE);

		BeginMode3D(camera);

		// Draw 3D objects here
		DrawGrid(10,1);
		DrawCube(Vector3(0.5,0.5,0.5), 1, 1, 1, GREEN);
		DrawCube(Vector3(1.5,0.5,0.5), 1, 1, 1, BLUE);


		EndMode3D();

		EndDrawing();
	}

	CloseWindow();
	return 0;
}
