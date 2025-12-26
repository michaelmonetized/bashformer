// Enhanced Breakout / Brick Breaker in ncurses with powerups:
// - Prize drops from bricks:
//   * DOUBLE  : spawn an extra ball
//   * TRIPLE  : up to 3 balls total
//   * BOMB    : clear a local cluster of bricks
//   * PADDLE- : shrink paddle
//   * PADDLE+ : enlarge paddle
//   * NUKE    : clear all bricks
//   * MAGNET  : paddle catches ball for a while
//
// Controls:
//   Mouse move: control paddle X directly
//   Left/Right: nudge paddle
//   SPACE/Up  : launch balls (if stuck to paddle)
//   P         : pause
//   R         : reset
//   Q         : quit

#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <locale.h>
#include <math.h>
#include <stdio.h>

#define MAX_BRICK_ROWS 8
#define MAX_BRICK_COLS 40
#define MAX_BALLS      8
#define MAX_POWERUPS   32

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
    int   active;
    int   stuck;      // 1 = stuck to paddle, waits for launch
} Ball;

typedef enum {
    PU_NONE = 0,
    PU_DOUBLE,
    PU_TRIPLE,
    PU_BOMB,
    PU_PADDLE_SMALL,
    PU_PADDLE_LARGE,
    PU_NUKE,
    PU_MAGNET
} PowerType;

typedef struct {
    float x, y;
    float vy;
    PowerType type;
    int   active;
} Powerup;

typedef struct {
    int score;
    int lives;
    int paused;
    int game_over;
    int magnet_ticks;  // frames of magnet effect remaining
    int level;         // current level (difficulty)
} GameState;

static int bricks[MAX_BRICK_ROWS][MAX_BRICK_COLS];

// --- Arena / bricks ---------------------------------------------------------

static void InitArena(Arena *a) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    a->rows = rows;
    a->cols = cols;

    a->brick_rows = 6;
    a->brick_h = 1;
    a->brick_w = 4;                // each brick 4 columns wide
    a->brick_cols = cols / a->brick_w;
    if (a->brick_cols > MAX_BRICK_COLS) a->brick_cols = MAX_BRICK_COLS;

    a->brick_start_y = 2;
    a->brick_start_x = (cols - a->brick_cols * a->brick_w) / 2;
    if (a->brick_start_y < 1) a->brick_start_y = 1;
    if (a->brick_start_x < 1) a->brick_start_x = 1;

    a->paddle_w = cols / 5;
    if (a->paddle_w < 8) a->paddle_w = 8;
    a->paddle_y = rows - 3;
}

// Apply difficulty scaling based on current level
static void ApplyLevelDifficulty(Arena *a, const GameState *g) {
    int base_rows = 6;
    int rows = base_rows + (g->level - 1);
    if (rows > MAX_BRICK_ROWS) rows = MAX_BRICK_ROWS;
    if (rows < 1) rows = 1;
    a->brick_rows = rows;

    // Paddle starts at cols/5 and shrinks with level (down to 40% of base)
    float base_width = (float)a->cols / 5.0f;
    float factor = 1.0f - 0.1f * (g->level - 1);
    if (factor < 0.4f) factor = 0.4f;
    int pw = (int)(base_width * factor);
    if (pw < 4) pw = 4;
    if (pw > a->cols / 2) pw = a->cols / 2;
    a->paddle_w = pw;
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

// --- Powerups ---------------------------------------------------------------

static Powerup powerups[MAX_POWERUPS];

static void ResetPowerups(void) {
    for (int i = 0; i < MAX_POWERUPS; i++) {
        powerups[i].active = 0;
    }
}

static void SpawnPowerup(const Arena *a, int brickRow, int brickCol) {
    // 20% chance to drop something
    if ((rand() % 100) >= 20) return;

    int slot = -1;
    for (int i = 0; i < MAX_POWERUPS; i++) {
        if (!powerups[i].active) { slot = i; break; }
    }
    if (slot < 0) return;

    Powerup *p = &powerups[slot];
    p->active = 1;
    p->vy = 0.3f;

    p->x = a->brick_start_x + brickCol * a->brick_w + a->brick_w / 2.0f;
    p->y = a->brick_start_y + brickRow * a->brick_h + a->brick_h;

    int r = rand() % 100;
    if (r < 15)      p->type = PU_DOUBLE;
    else if (r < 25) p->type = PU_TRIPLE;
    else if (r < 40) p->type = PU_BOMB;
    else if (r < 55) p->type = PU_PADDLE_SMALL;
    else if (r < 70) p->type = PU_PADDLE_LARGE;
    else if (r < 85) p->type = PU_NUKE;
    else             p->type = PU_MAGNET;
}

static void ApplyPowerup(PowerType type, Arena *a, Ball balls[], GameState *g, int *paddle_x) {
    switch (type) {
        case PU_DOUBLE:
        case PU_TRIPLE: {
            int want = (type == PU_DOUBLE) ? 2 : 3;
            int count = 0;
            for (int i = 0; i < MAX_BALLS; i++)
                if (balls[i].active) count++;
            if (count >= want) break;
            // duplicate first active ball
            int base = -1;
            for (int i = 0; i < MAX_BALLS; i++) {
                if (balls[i].active) { base = i; break; }
            }
            if (base < 0) break;
            for (int i = 0; i < MAX_BALLS && count < want; i++) {
                if (!balls[i].active) {
                    balls[i] = balls[base];
                    balls[i].vx = -balls[i].vx; // send in opposite direction
                    balls[i].active = 1;
                    balls[i].stuck = 0;
                    count++;
                }
            }
        } break;

        case PU_BOMB: {
            // Clear a 3-row band of bricks around middle
            int mid = a->brick_rows / 2;
            for (int r = mid - 1; r <= mid + 1; r++) {
                if (r < 0 || r >= a->brick_rows) continue;
                for (int c = 0; c < a->brick_cols; c++) {
                    if (bricks[r][c]) {
                        bricks[r][c] = 0;
                        g->score += 10;
                    }
                }
            }
        } break;

        case PU_PADDLE_SMALL: {
            a->paddle_w /= 2;
            if (a->paddle_w < 4) a->paddle_w = 4;
            if (*paddle_x + a->paddle_w >= a->cols) *paddle_x = a->cols - a->paddle_w;
        } break;

        case PU_PADDLE_LARGE: {
            a->paddle_w *= 2;
            if (a->paddle_w > a->cols / 2) a->paddle_w = a->cols / 2;
            if (*paddle_x + a->paddle_w >= a->cols) *paddle_x = a->cols - a->paddle_w;
        } break;

        case PU_NUKE: {
            for (int r = 0; r < a->brick_rows; r++)
                for (int c = 0; c < a->brick_cols; c++)
                    if (bricks[r][c]) {
                        bricks[r][c] = 0;
                        g->score += 10;
                    }
        } break;

        case PU_MAGNET: {
            // e.g. 600 frames at 120 FPS ~ 5 seconds
            g->magnet_ticks = 600;
        } break;

        default: break;
    }
}

// Check if any bricks remain
static int BricksRemaining(const Arena *a) {
    for (int r = 0; r < a->brick_rows; r++) {
        for (int c = 0; c < a->brick_cols; c++) {
            if (bricks[r][c]) return 1;
        }
    }
    return 0;
}

// Start next, harder level after all bricks are gone
static void StartNextLevel(Arena *a, Ball balls[], GameState *g, int *paddle_x) {
    g->level++;
    if (g->level < 1) g->level = 1;

    // Re-apply difficulty for new level (more rows / smaller paddle)
    ApplyLevelDifficulty(a, g);

    // Center paddle for new level
    *paddle_x = a->cols / 2 - a->paddle_w / 2;
    if (*paddle_x < 0) *paddle_x = 0;
    if (*paddle_x + a->paddle_w >= a->cols) *paddle_x = a->cols - a->paddle_w;

    // Reset bricks and powerups, clear magnet
    ResetBricks(a);
    ResetPowerups();
    g->magnet_ticks = 0;

    // Single stuck ball on paddle to start the new level
    for (int i = 0; i < MAX_BALLS; i++) {
        balls[i].active = 0;
        balls[i].stuck = 0;
    }
    balls[0].active = 1;
    balls[0].stuck = 1;
    balls[0].x = *paddle_x + a->paddle_w / 2.0f;
    balls[0].y = a->paddle_y - 1;
    balls[0].vx = 0.0f;
    balls[0].vy = 0.0f;
}

// --- Game init --------------------------------------------------------------

static void ResetGame(const Arena *a, Ball balls[], GameState *g, int *paddle_x) {
    *paddle_x = a->cols / 2 - a->paddle_w / 2;
    for (int i = 0; i < MAX_BALLS; i++) {
        balls[i].active = 0;
        balls[i].stuck = 0;
    }
    balls[0].active = 1;
    balls[0].stuck = 1;
    balls[0].x = *paddle_x + a->paddle_w / 2.0f;
    balls[0].y = a->paddle_y - 1;
    balls[0].vx = 0.0f;
    balls[0].vy = 0.0f;

    g->score = 0;
    g->lives = 3;
    g->paused = 0;
    g->game_over = 0;
    g->magnet_ticks = 0;
    g->level = 1;

    ResetBricks(a);
    ResetPowerups();
}

// --- Rendering --------------------------------------------------------------

static void DrawGame(const Arena *a, const Ball balls[], int paddle_x, const GameState *g) {
    erase();

    // HUD
    mvprintw(0, 0,
             "Score: %d  Lives: %d  Level: %d  Q: quit  Mouse: move  SPACE: launch  P: pause",
             g->score, g->lives, g->level);
    if (g->game_over) {
        mvprintw(1, 0, "GAME OVER - press R to restart");
    } else if (g->paused) {
        mvprintw(1, 0, "PAUSED - press P to resume");
    } else if (g->magnet_ticks > 0) {
        mvprintw(1, 0, "MAGNET ACTIVE");
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

    // Powerups
    for (int i = 0; i < MAX_POWERUPS; i++) {
        if (!powerups[i].active) continue;
        int x = (int)(powerups[i].x + 0.5f);
        int y = (int)(powerups[i].y + 0.5f);
        if (x < 0 || x >= a->cols || y <= 1 || y >= a->rows) continue;
        char ch = '?';
        switch (powerups[i].type) {
            case PU_DOUBLE:        ch = '2'; break;
            case PU_TRIPLE:        ch = '3'; break;
            case PU_BOMB:          ch = 'B'; break;
            case PU_PADDLE_SMALL:  ch = 's'; break;
            case PU_PADDLE_LARGE:  ch = 'L'; break;
            case PU_NUKE:          ch = 'N'; break;
            case PU_MAGNET:        ch = 'M'; break;
            default:               ch = '?'; break;
        }
        mvaddch(y, x, ch);
    }

    // Paddle
    for (int x = 0; x < a->paddle_w; x++) {
        int px = paddle_x + x;
        if (px >= 0 && px < a->cols)
            mvaddch(a->paddle_y, px, '=');
    }

    // Balls
    for (int i = 0; i < MAX_BALLS; i++) {
        if (!balls[i].active) continue;
        int bx = (int)(balls[i].x + 0.5f);
        int by = (int)(balls[i].y + 0.5f);
        if (bx >= 0 && bx < a->cols && by >= 1 && by < a->rows)
            mvaddch(by, bx, 'o');
    }

    refresh();
}

// --- Game update ------------------------------------------------------------

static void UpdateGame(Arena *a, Ball balls[], GameState *g,
                       int *paddle_x, int launchPressed, int mouseX, int moveDir) {
    if (g->game_over || g->paused) return;

    // Move paddle: mouse has priority
    if (mouseX >= 0) {
        int newX = mouseX - a->paddle_w / 2;
        if (newX < 0) newX = 0;
        if (newX + a->paddle_w >= a->cols) newX = a->cols - a->paddle_w;
        *paddle_x = newX;
    } else if (moveDir != 0) {
        *paddle_x += moveDir * 4;
        if (*paddle_x < 0) *paddle_x = 0;
        if (*paddle_x + a->paddle_w >= a->cols) *paddle_x = a->cols - a->paddle_w;
    }

    // Magnet timer
    if (g->magnet_ticks > 0) g->magnet_ticks--;

    int activeCount = 0;
    for (int i = 0; i < MAX_BALLS; i++) {
        if (!balls[i].active) continue;
        activeCount++;

        Ball *ball = &balls[i];

        // Stuck to paddle before launch
        if (ball->stuck) {
            ball->x = *paddle_x + a->paddle_w / 2.0f;
            ball->y = a->paddle_y - 1;
            if (launchPressed) {
                ball->stuck = 0;
                ball->vx = (rand() % 2 == 0) ? 0.4f : -0.4f;
                ball->vy = -0.7f;
            }
            continue;
        }

        // Integrate
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

        // Bottom: this ball is lost
        if (by >= a->rows - 1) {
            ball->active = 0;
            activeCount--;
            continue;
        }

        // Paddle collision
        if (by == a->paddle_y - 1 && bx >= *paddle_x && bx < *paddle_x + a->paddle_w && ball->vy > 0) {
            if (g->magnet_ticks > 0) {
                // Catch and stick
                ball->stuck = 1;
                ball->vx = 0;
                ball->vy = 0;
            } else {
                float hitPos = (ball->x - *paddle_x) / (float)a->paddle_w; // 0..1
                ball->vy = -fabsf(ball->vy);
                ball->vx = (hitPos - 0.5f) * 1.0f;
            }
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
                SpawnPowerup(a, brickRow, brickCol);
            }
        }
    }

    // If all balls are gone, lose a life and spawn a new stuck ball (unless out of lives)
    if (activeCount == 0) {
        g->lives--;
        if (g->lives <= 0) {
            g->game_over = 1;
        } else {
            // spawn new stuck ball
            for (int i = 0; i < MAX_BALLS; i++) {
                balls[i].active = 0;
                balls[i].stuck = 0;
            }
            balls[0].active = 1;
            balls[0].stuck = 1;
            balls[0].x = *paddle_x + a->paddle_w / 2.0f;
            balls[0].y = a->paddle_y - 1;
            balls[0].vx = 0;
            balls[0].vy = 0;
        }
    }

    // Update powerups
    for (int i = 0; i < MAX_POWERUPS; i++) {
        Powerup *p = &powerups[i];
        if (!p->active) continue;
        p->y += p->vy;
        int px = (int)(p->x + 0.5f);
        int py = (int)(p->y + 0.5f);
        if (py >= a->rows - 1) {
            p->active = 0;
            continue;
        }
        // Collected by paddle
        if (py == a->paddle_y && px >= *paddle_x && px < *paddle_x + a->paddle_w) {
            ApplyPowerup(p->type, a, balls, g, paddle_x);
            p->active = 0;
        }
    }

    // If there are no bricks left, player wins the level -> go to harder level
    if (!BricksRemaining(a) && !g->game_over) {
        StartNextLevel(a, balls, g, paddle_x);
    }
}

// --- Main -------------------------------------------------------------------

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
    // Request "any event" mouse tracking (so we get move events, not just clicks)
    printf("\033[?1003h");
    fflush(stdout);

    Arena arena;
    InitArena(&arena);

    Ball balls[MAX_BALLS];
    GameState g = {0};
    int paddle_x = 0;

    ResetGame(&arena, balls, &g, &paddle_x);
    // Apply initial difficulty scaling now that level is set
    ApplyLevelDifficulty(&arena, &g);
    ResetBricks(&arena);

    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = (long)(1e9 / 120); // ~120 FPS

    int running = 1;
    while (running) {
        int ch = getch();
        int moveDir = 0;
        int launch = 0;
        int mouseX = -1;
        MEVENT ev;

        if (ch == 'q' || ch == 'Q') {
            running = 0;
        } else if (ch == 'p' || ch == 'P') {
            if (!g.game_over) {
                g.paused = !g.paused;
            }
        } else if (ch == 'r' || ch == 'R') {
            ResetGame(&arena, balls, &g, &paddle_x);
        } else if (ch == KEY_MOUSE) {
            if (getmouse(&ev) == OK) {
                mouseX = ev.x;
            }
        }

        if (!g.game_over && !g.paused) {
            if (ch == KEY_LEFT)  moveDir = -1;
            if (ch == KEY_RIGHT) moveDir = 1;
            if (ch == ' ' || ch == KEY_UP) launch = 1;
        }

        InitArena(&arena); // handle terminal resizes
        ApplyLevelDifficulty(&arena, &g);
        UpdateGame(&arena, balls, &g, &paddle_x, launch, mouseX, moveDir);
        DrawGame(&arena, balls, paddle_x, &g);

        nanosleep(&ts, NULL);
    }

    endwin();
    // Disable mouse tracking mode we enabled earlier
    printf("\033[?1003l");
    fflush(stdout);
    return 0;
}

