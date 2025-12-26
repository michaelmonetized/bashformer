#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <locale.h>
#include <math.h>
#include <stdio.h>

#define MAX_BRICK_ROWS 8
#define MAX_BRICK_COLS 40

typedef struct {
    int rows;
    int cols;
    int brick_rows;
    int brick_cols;
    int brick_w;      // brick width in columns
    int brick_h;      // brick height in rows
    int brick_start_y;
    int brick_start_x;
    int paddle_y;
    int paddle_w;
} Arena;

typedef struct {
    float x, y;
    float vx, vy;
} Ball;

typedef struct {
    int score;
    int lives;
    int paused;
    int game_over;
} GameState;

static int bricks[MAX_BRICK_ROWS][MAX_BRICK_COLS];

static void InitArena(Arena *a) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    a->rows = rows;
    a->cols = cols;

    a->brick_rows = 6;
    a->brick_h = 1;
    a->brick_w = 4; // each brick 4 columns wide
    a->brick_cols = cols / a->brick_w;
    if (a->brick_cols > MAX_BRICK_COLS) a->brick_cols = MAX_BRICK_COLS;

    a->brick_start_y = 2;
    a->brick_start_x = (cols - a->brick_cols * a->brick_w) / 2;
    if (a->brick_start_x < 1) a->brick_start_x = 1;

    // Make paddle a bit wider so it's easier to catch fast balls
    a->paddle_w = cols / 5;          // was cols / 6
    if (a->paddle_w < 8) a->paddle_w = 8;
    a->paddle_y = rows - 3;
}

static void ResetBricks(const Arena *a) {
    for (int r = 0; r < MAX_BRICK_ROWS; r++)
        for (int c = 0; c < MAX_BRICK_COLS; c++)
            bricks[r][c] = 0;

    for (int r = 0; r < a->brick_rows && r < MAX_BRICK_ROWS; r++) {
        for (int c = 0; c < a->brick_cols && c < MAX_BRICK_COLS; c++) {
            bricks[r][c] = 1;
        }
    }
}

static void ResetGame(const Arena *a, Ball *ball, int *paddle_x, GameState *g) {
    *paddle_x = a->cols / 2 - a->paddle_w / 2;
    ball->x = a->cols / 2.0f;
    ball->y = a->rows / 2.0f;
    // Start with a slightly slower horizontal speed so the paddle can keep up
    ball->vx = (rand() % 2 == 0) ? 0.4f : -0.4f;
    ball->vy = -0.7f;
    g->score = 0;
    g->lives = 3;
    g->paused = 0;
    g->game_over = 0;
    ResetBricks(a);
}

static void DrawGame(const Arena *a, const Ball *ball, int paddle_x, const GameState *g) {
    erase();

    // HUD
    mvprintw(0, 0, "Score: %d  Lives: %d  Q: quit  Arrows: move  SPACE: launch  P: pause",
             g->score, g->lives);
    if (g->game_over) {
        mvprintw(1, 0, "GAME OVER - press R to restart");
    } else if (g->paused) {
        mvprintw(1, 0, "PAUSED - press P to resume");
    }

    // Bricks
    for (int r = 0; r < a->brick_rows; r++) {
        for (int c = 0; c < a->brick_cols; c++) {
            if (!bricks[r][c]) continue;
            int y = a->brick_start_y + r * a->brick_h;
            int x = a->brick_start_x + c * a->brick_w;
            for (int dy = 0; dy < a->brick_h; dy++) {
                for (int dx = 0; dx < a->brick_w; dx++) {
                    mvaddch(y + dy, x + dx, '#');
                }
            }
        }
    }

    // Paddle
    for (int x = 0; x < a->paddle_w; x++) {
        int px = paddle_x + x;
        if (px >= 0 && px < a->cols)
            mvaddch(a->paddle_y, px, '=');
    }

    // Ball
    int bx = (int)(ball->x + 0.5f);
    int by = (int)(ball->y + 0.5f);
    if (bx >= 0 && bx < a->cols && by >= 1 && by < a->rows)
        mvaddch(by, bx, 'o');

    refresh();
}

static void UpdateGame(const Arena *a, Ball *ball, int *paddle_x, GameState *g, int launchPressed, int moveDir) {
    if (g->game_over || g->paused) return;

    // Move paddle (stronger step so holding arrow can outrun the ball)
    *paddle_x += moveDir * 4;  // was *2
    if (*paddle_x < 0) *paddle_x = 0;
    if (*paddle_x + a->paddle_w >= a->cols) *paddle_x = a->cols - a->paddle_w;

    // If ball is "stuck" on paddle before launch
    if (g->lives == 3 && ball->vy == 0.0f && ball->vx == 0.0f && !g->game_over) {
        ball->x = *paddle_x + a->paddle_w / 2.0f;
        ball->y = a->paddle_y - 1;
        if (launchPressed) {
            ball->vx = 0.4f;
            ball->vy = -0.7f;
        }
        return;
    }

    // Integrate ball
    ball->x += ball->vx;
    ball->y += ball->vy;

    // Wall collisions
    if (ball->x < 0) {
        ball->x = 0;
        ball->vx = -ball->vx;
    }
    if (ball->x >= a->cols - 1) {
        ball->x = a->cols - 2;
        ball->vx = -ball->vx;
    }
    if (ball->y < 1) {
        ball->y = 1;
        ball->vy = -ball->vy;
    }

    int bx = (int)(ball->x + 0.5f);
    int by = (int)(ball->y + 0.5f);

    // Bottom: lose life
    if (by >= a->rows - 1) {
        g->lives--;
        if (g->lives <= 0) {
            g->game_over = 1;
            return;
        }
        // reset ball on paddle
        ball->vx = ball->vy = 0;
        return;
    }

    // Paddle collision
    if (by == a->paddle_y - 1 && bx >= *paddle_x && bx < *paddle_x + a->paddle_w && ball->vy > 0) {
        float hitPos = (ball->x - *paddle_x) / (float)a->paddle_w; // 0..1
        ball->vy = -fabsf(ball->vy);
        // Slightly gentler angle so horizontal speed doesn't spike too high
        ball->vx = (hitPos - 0.5f) * 1.0f; // was *1.6f
    }

    // Brick collision
    int brickRow = (by - a->brick_start_y) / a->brick_h;
    int brickCol = (bx - a->brick_start_x) / a->brick_w;
    if (brickRow >= 0 && brickRow < a->brick_rows &&
        brickCol >= 0 && brickCol < a->brick_cols) {
        if (bricks[brickRow][brickCol]) {
            bricks[brickRow][brickCol] = 0;
            g->score += 10;
            ball->vy = -ball->vy;
        }
    }
}

int main(void) {
    srand((unsigned)time(NULL));
    setlocale(LC_ALL, "");

    initscr();
    cbreak();
    noecho();
    curs_set(0);
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);
    // Enable mouse support so we can control paddle with mouse X
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
    // Request \"any event\" mouse tracking (so we get move events, not just clicks)
    printf("\033[?1003h");
    fflush(stdout);

    Arena arena;
    InitArena(&arena);

    Ball ball = {0};
    GameState g = {0};
    int paddle_x = 0;

    ResetGame(&arena, &ball, &paddle_x, &g);
    // Start with ball stuck to paddle until first launch
    ball.vx = ball.vy = 0.0f;

    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = (long)(1e9 / 120); // ~120 FPS for smoothness

    int running = 1;
    while (running) {
        int ch = getch();
        int moveDir = 0;
        int launch = 0;
        MEVENT ev;

        if (ch == 'q' || ch == 'Q') {
            running = 0;
        } else if (ch == 'p' || ch == 'P') {
            if (!g.game_over) {
                g.paused = !g.paused;
            }
        } else if (ch == 'r' || ch == 'R') {
            ResetGame(&arena, &ball, &paddle_x, &g);
            ball.vx = ball.vy = 0.0f;
        } else if (ch == KEY_MOUSE) {
            // Mouse moves paddle directly based on X position (no click required)
            if (getmouse(&ev) == OK) {
                int newX = ev.x - arena.paddle_w / 2;
                if (newX < 0) newX = 0;
                if (newX + arena.paddle_w >= arena.cols) newX = arena.cols - arena.paddle_w;
                paddle_x = newX;
            }
        }

        if (!g.game_over && !g.paused) {
            if (ch == KEY_LEFT)  moveDir = -1;
            if (ch == KEY_RIGHT) moveDir = 1;
            if (ch == ' ' || ch == KEY_UP) launch = 1;
        }

        InitArena(&arena); // handle terminal resizes
        UpdateGame(&arena, &ball, &paddle_x, &g, launch, moveDir);
        DrawGame(&arena, &ball, paddle_x, &g);

        nanosleep(&ts, NULL);
    }

    endwin();
    // Disable mouse tracking mode we enabled earlier
    printf("\033[?1003l");
    fflush(stdout);
    return 0;
}

