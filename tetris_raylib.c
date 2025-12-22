// Simple Tetris-like game using raylib
// Board is wider than classic (12 columns), with smooth windowed rendering.
// Controls:
//   Left / Right arrows - move
//   Up arrow            - rotate
//   Down arrow          - soft drop
//   Space               - hard drop
//   R                   - restart
//   Esc                 - quit

#include "raylib.h"
#include <stdlib.h>
#include <time.h>

#define BOARD_W 12
#define BOARD_H 22   // includes a couple of hidden rows at top
#define VISIBLE_H 20

typedef struct {
    int cells[4][4];
} Shape;

typedef struct {
    int type;   // 0-6
    int rot;    // 0-3
    int x, y;
} Piece;

static Shape SHAPES[7][4];
static int board[BOARD_H][BOARD_W];

static void InitShapes(void) {
    // I
    int I0[4][4] = {
        {0,0,0,0},
        {1,1,1,1},
        {0,0,0,0},
        {0,0,0,0}
    };
    int I1[4][4] = {
        {0,0,1,0},
        {0,0,1,0},
        {0,0,1,0},
        {0,0,1,0}
    };
    int I2[4][4] = {
        {0,0,0,0},
        {0,0,0,0},
        {1,1,1,1},
        {0,0,0,0}
    };
    int I3[4][4] = {
        {0,1,0,0},
        {0,1,0,0},
        {0,1,0,0},
        {0,1,0,0}
    };

    // O
    int O0[4][4] = {
        {0,1,1,0},
        {0,1,1,0},
        {0,0,0,0},
        {0,0,0,0}
    };

    // T
    int T0[4][4] = {
        {0,1,0,0},
        {1,1,1,0},
        {0,0,0,0},
        {0,0,0,0}
    };
    int T1[4][4] = {
        {0,1,0,0},
        {0,1,1,0},
        {0,1,0,0},
        {0,0,0,0}
    };
    int T2[4][4] = {
        {0,0,0,0},
        {1,1,1,0},
        {0,1,0,0},
        {0,0,0,0}
    };
    int T3[4][4] = {
        {0,1,0,0},
        {1,1,0,0},
        {0,1,0,0},
        {0,0,0,0}
    };

    // L
    int L0[4][4] = {
        {0,0,1,0},
        {1,1,1,0},
        {0,0,0,0},
        {0,0,0,0}
    };
    int L1[4][4] = {
        {0,1,0,0},
        {0,1,0,0},
        {0,1,1,0},
        {0,0,0,0}
    };
    int L2[4][4] = {
        {0,0,0,0},
        {1,1,1,0},
        {1,0,0,0},
        {0,0,0,0}
    };
    int L3[4][4] = {
        {1,1,0,0},
        {0,1,0,0},
        {0,1,0,0},
        {0,0,0,0}
    };

    // J
    int J0[4][4] = {
        {1,0,0,0},
        {1,1,1,0},
        {0,0,0,0},
        {0,0,0,0}
    };
    int J1[4][4] = {
        {0,1,1,0},
        {0,1,0,0},
        {0,1,0,0},
        {0,0,0,0}
    };
    int J2[4][4] = {
        {0,0,0,0},
        {1,1,1,0},
        {0,0,1,0},
        {0,0,0,0}
    };
    int J3[4][4] = {
        {0,1,0,0},
        {0,1,0,0},
        {1,1,0,0},
        {0,0,0,0}
    };

    // S
    int S0[4][4] = {
        {0,1,1,0},
        {1,1,0,0},
        {0,0,0,0},
        {0,0,0,0}
    };
    int S1[4][4] = {
        {0,1,0,0},
        {0,1,1,0},
        {0,0,1,0},
        {0,0,0,0}
    };

    // Z
    int Z0[4][4] = {
        {1,1,0,0},
        {0,1,1,0},
        {0,0,0,0},
        {0,0,0,0}
    };
    int Z1[4][4] = {
        {0,0,1,0},
        {0,1,1,0},
        {0,1,0,0},
        {0,0,0,0}
    };

    // Zero all
    for (int t = 0; t < 7; t++)
        for (int r = 0; r < 4; r++)
            for (int y = 0; y < 4; y++)
                for (int x = 0; x < 4; x++)
                    SHAPES[t][r].cells[y][x] = 0;

    // I
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < 4; x++) {
            SHAPES[0][0].cells[y][x] = I0[y][x];
            SHAPES[0][1].cells[y][x] = I1[y][x];
            SHAPES[0][2].cells[y][x] = I2[y][x];
            SHAPES[0][3].cells[y][x] = I3[y][x];
        }
    // O
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < 4; x++)
            SHAPES[1][0].cells[y][x] =
            SHAPES[1][1].cells[y][x] =
            SHAPES[1][2].cells[y][x] =
            SHAPES[1][3].cells[y][x] = O0[y][x];
    // T
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < 4; x++) {
            SHAPES[2][0].cells[y][x] = T0[y][x];
            SHAPES[2][1].cells[y][x] = T1[y][x];
            SHAPES[2][2].cells[y][x] = T2[y][x];
            SHAPES[2][3].cells[y][x] = T3[y][x];
        }
    // L
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < 4; x++) {
            SHAPES[3][0].cells[y][x] = L0[y][x];
            SHAPES[3][1].cells[y][x] = L1[y][x];
            SHAPES[3][2].cells[y][x] = L2[y][x];
            SHAPES[3][3].cells[y][x] = L3[y][x];
        }
    // J
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < 4; x++) {
            SHAPES[4][0].cells[y][x] = J0[y][x];
            SHAPES[4][1].cells[y][x] = J1[y][x];
            SHAPES[4][2].cells[y][x] = J2[y][x];
            SHAPES[4][3].cells[y][x] = J3[y][x];
        }
    // S
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < 4; x++) {
            SHAPES[5][0].cells[y][x] = S0[y][x];
            SHAPES[5][1].cells[y][x] = S1[y][x];
            SHAPES[5][2].cells[y][x] = S0[y][x];
            SHAPES[5][3].cells[y][x] = S1[y][x];
        }
    // Z
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < 4; x++) {
            SHAPES[6][0].cells[y][x] = Z0[y][x];
            SHAPES[6][1].cells[y][x] = Z1[y][x];
            SHAPES[6][2].cells[y][x] = Z0[y][x];
            SHAPES[6][3].cells[y][x] = Z1[y][x];
        }
}

static void ClearBoard(void) {
    for (int y = 0; y < BOARD_H; y++)
        for (int x = 0; x < BOARD_W; x++)
            board[y][x] = 0;
}

static int Collides(const Piece *p) {
    for (int dy = 0; dy < 4; dy++)
        for (int dx = 0; dx < 4; dx++)
            if (SHAPES[p->type][p->rot].cells[dy][dx]) {
                int bx = p->x + dx;
                int by = p->y + dy;
                if (bx < 0 || bx >= BOARD_W || by < 0 || by >= BOARD_H) return 1;
                if (board[by][bx]) return 1;
            }
    return 0;
}

static void PlacePiece(const Piece *p) {
    for (int dy = 0; dy < 4; dy++)
        for (int dx = 0; dx < 4; dx++)
            if (SHAPES[p->type][p->rot].cells[dy][dx]) {
                int bx = p->x + dx;
                int by = p->y + dy;
                if (bx >= 0 && bx < BOARD_W && by >= 0 && by < BOARD_H)
                    board[by][bx] = p->type + 1;
            }
}

static void ClearLines(int *score, int *linesCleared) {
    for (int y = BOARD_H - 1; y >= 0; y--) {
        int full = 1;
        for (int x = 0; x < BOARD_W; x++)
            if (!board[y][x]) {
                full = 0;
                break;
            }
        if (full) {
            (*linesCleared)++;
            *score += 100;
            for (int yy = y; yy > 0; yy--)
                for (int x = 0; x < BOARD_W; x++)
                    board[yy][x] = board[yy - 1][x];
            for (int x = 0; x < BOARD_W; x++) board[0][x] = 0;
            y++;
        }
    }
}

static void SpawnPiece(Piece *p) {
    p->type = rand() % 7;
    p->rot = 0;
    p->x = BOARD_W / 2 - 2;
    p->y = 0;
}

int main(void) {
    const int cell = 32;
    const int sidePanel = 200;
    const int screenWidth = BOARD_W * cell + sidePanel;
    const int screenHeight = VISIBLE_H * cell;

    InitWindow(screenWidth, screenHeight, "Tetris Variant - Raylib");
    SetTargetFPS(60);
    InitShapes();
    ClearBoard();

    Color colors[8] = {
        (Color){30, 30, 30, 255}, // unused
        SKYBLUE,       // I
        YELLOW,        // O
        PURPLE,        // T
        ORANGE,        // L
        BLUE,          // J
        GREEN,         // S
        RED            // Z
    };

    Piece current;
    SpawnPiece(&current);

    int score = 0;
    int lines = 0;
    int gameOver = 0;

    float dropTimer = 0.0f;
    float dropInterval = 0.7f; // seconds per row at level 1

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        if (IsKeyPressed(KEY_R)) {
            ClearBoard();
            score = lines = 0;
            gameOver = 0;
            SpawnPiece(&current);
            dropInterval = 0.7f;
        }

        Piece trial = current;

        if (!gameOver) {
            // Horizontal
            if (IsKeyPressed(KEY_LEFT)) {
                trial.x--;
                if (!Collides(&trial)) current = trial;
            }
            if (IsKeyPressed(KEY_RIGHT)) {
                trial.x++;
                if (!Collides(&trial)) current = trial;
            }
            // Rotate
            if (IsKeyPressed(KEY_UP)) {
                trial = current;
                trial.rot = (trial.rot + 1) % 4;
                if (!Collides(&trial)) current = trial;
            }
            // Soft drop
            if (IsKeyDown(KEY_DOWN)) {
                trial = current;
                trial.y++;
                if (!Collides(&trial)) {
                    current = trial;
                    score += 1;
                }
            }
            // Hard drop
            if (IsKeyPressed(KEY_SPACE)) {
                trial = current;
                while (!Collides(&trial)) {
                    current = trial;
                    trial.y++;
                }
                PlacePiece(&current);
                ClearLines(&score, &lines);
                SpawnPiece(&current);
                if (Collides(&current)) gameOver = 1;
            }
        }

        // Auto drop
        if (!gameOver) {
            dropTimer += dt;
            if (dropTimer >= dropInterval) {
                dropTimer = 0.0f;
                trial = current;
                trial.y++;
                if (!Collides(&trial)) {
                    current = trial;
                } else {
                    PlacePiece(&current);
                    ClearLines(&score, &lines);
                    SpawnPiece(&current);
                    if (Collides(&current)) gameOver = 1;
                }
            }
        }

        // Shadow computation
        int shadowY = current.y;
        Piece tmp = current;
        while (1) {
            tmp.y++;
            if (Collides(&tmp)) break;
            shadowY = tmp.y;
        }

        BeginDrawing();
        ClearBackground((Color){20, 20, 20, 255});

        // Board background and locked pieces
        int originX = 40;
        int originY = 20;

        // Board border
        DrawRectangleLines(originX - 2, originY - 2, BOARD_W * cell + 4, VISIBLE_H * cell + 4, RAYWHITE);

        for (int y = 2; y < BOARD_H; y++) { // hide top 2 rows
            for (int x = 0; x < BOARD_W; x++) {
                int val = board[y][x];
                int screenY = y - 2;
                if (screenY < 0 || screenY >= VISIBLE_H) continue;
                int sx = originX + x * cell;
                int sy = originY + screenY * cell;
                DrawRectangleLines(sx, sy, cell, cell, (Color){40, 40, 40, 255});
                if (val > 0) {
                    DrawRectangle(sx + 1, sy + 1, cell - 2, cell - 2, colors[val]);
                }
            }
        }

        // Shadow piece
        if (!gameOver) {
            for (int dy = 0; dy < 4; dy++)
                for (int dx = 0; dx < 4; dx++)
                    if (SHAPES[current.type][current.rot].cells[dy][dx]) {
                        int bx = current.x + dx;
                        int by = shadowY + dy;
                        if (by >= 2 && by < BOARD_H && bx >= 0 && bx < BOARD_W) {
                            int screenY = by - 2;
                            int sx = originX + bx * cell;
                            int sy = originY + screenY * cell;
                            DrawRectangleLines(sx + 4, sy + 4, cell - 8, cell - 8, RAYWHITE);
                        }
                    }
        }

        // Current piece
        if (!gameOver) {
            for (int dy = 0; dy < 4; dy++)
                for (int dx = 0; dx < 4; dx++)
                    if (SHAPES[current.type][current.rot].cells[dy][dx]) {
                        int bx = current.x + dx;
                        int by = current.y + dy;
                        if (by >= 2 && by < BOARD_H && bx >= 0 && bx < BOARD_W) {
                            int screenY = by - 2;
                            int sx = originX + bx * cell;
                            int sy = originY + screenY * cell;
                            Color c = colors[current.type + 1];
                            DrawRectangle(sx + 1, sy + 1, cell - 2, cell - 2, c);
                        }
                    }
        }

        // Side panel
        int panelX = originX + BOARD_W * cell + 20;
        DrawText(TextFormat("Score: %d", score), panelX, 40, 20, RAYWHITE);
        DrawText(TextFormat("Lines: %d", lines), panelX, 70, 20, RAYWHITE);
        if (gameOver) {
            DrawText("GAME OVER", panelX, 120, 24, RED);
            DrawText("R: Restart", panelX, 150, 18, RAYWHITE);
        } else {
            DrawText("Arrows: move/rot", panelX, 120, 18, RAYWHITE);
            DrawText("Down: soft drop", panelX, 140, 18, RAYWHITE);
            DrawText("Space: hard drop", panelX, 160, 18, RAYWHITE);
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}

