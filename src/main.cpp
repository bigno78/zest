#include "raylib.h"

#include <iostream>

int main()
{
    InitWindow(800, 450, "raylib [core] example - basic window");

    while (!WindowShouldClose())
    {
        BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawText("Hello there!", 190, 200, 20, LIGHTGRAY);
        EndDrawing();
    }

    CloseWindow();
}
