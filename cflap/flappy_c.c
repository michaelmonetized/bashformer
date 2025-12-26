 #include <ncurses.h>
 #include <stdlib.h>
 #include <time.h>
 #include <unistd.h>
 
 #define MAX_PIPES 256
 
 typedef struct {
     float x;
     int gap_y;      // center of gap
     int active;
 } Pipe;
 
 typedef struct {
     float y;
     float vy;
     int score;
     int started;
     int game_over;
 } BirdState;
 
 typedef struct {
     int width;
     int height;
     int ground_y;
 } Arena;
 
 // Tunable config
 typedef struct {
     float gravity;
     float flap_vy;
     float max_fall_vy;
     float max_flap_vy;
     float pipe_speed;        // columns per second
     float pipe_spawn_chance; // probability per frame
     int   pipe_gap;
     int   fps;
 } Config;
 
 static void init_arena(Arena *a) {
     int rows, cols;
     getmaxyx(stdscr, rows, cols);
     a->width  = cols;
     a->height = rows - 1; // 1 line for HUD
     if (a->height < 16) a->height = 16;
     a->ground_y = a->height - 2;
 }
 
 static void init_config(Config *c) {
    c->fps = 60;
    // Bird motion (doubled from previous tuning)
    c->gravity      = 0.032f;   // was 0.016
    c->flap_vy      = -0.25f;   // was -0.125
    c->max_fall_vy  = 0.30f;    // was 0.15
    c->max_flap_vy  = -0.36f;   // was -0.18
    // Make pipes 10x faster across the screen (columns per second)
    c->pipe_speed   = 31.0f;    // was 3.1
    // Spawn rate unchanged for now; will feel denser with faster pipes
    c->pipe_spawn_chance = 0.04f;
     c->pipe_gap = 8;
 }
 
 static float frand01(void) {
     return (float)rand() / (float)RAND_MAX;
 }
 
 static void reset_game(BirdState *b, Pipe pipes[], const Arena *a) {
     (void)a;
     b->y = 8.0f;
     b->vy = 0.0f;
     b->score = 0;
     b->started = 0;
     b->game_over = 0;
     for (int i = 0; i < MAX_PIPES; i++) {
         pipes[i].active = 0;
     }
 }
 
 static void spawn_pipe(Pipe pipes[], const Arena *a, const Config *c) {
    // Enforce a minimum horizontal distance between pipes of ~5% terminal width
    float min_dist = a->width * 0.05f;
    float rightmost_x = -1e9f;
    for (int i = 0; i < MAX_PIPES; i++) {
        if (pipes[i].active && pipes[i].x > rightmost_x) {
            rightmost_x = pipes[i].x;
        }
    }
    // If the rightmost existing pipe is still too close to the spawn edge,
    // skip spawning this frame.
    if (rightmost_x > (float)a->width - min_dist) {
        return;
    }

     int idx = -1;
     for (int i = 0; i < MAX_PIPES; i++) {
         if (!pipes[i].active) {
             idx = i;
             break;
         }
     }
     if (idx < 0) return;
 
     int min_top = 2;
     int max_bottom = a->ground_y - 2;
     int gap_half = c->pipe_gap / 2;
     int min_center = min_top + gap_half;
     int max_center = max_bottom - gap_half;
     if (max_center <= min_center) max_center = min_center + 1;
 
     int center = min_center + (int)(frand01() * (max_center - min_center + 1));
 
     pipes[idx].x = (float)a->width;
     pipes[idx].gap_y = center;
     pipes[idx].active = 1;
 }
 
 static void update_game(BirdState *b, Pipe pipes[], const Arena *a, const Config *c, int flap_pressed) {
     if (!b->started || b->game_over) return;
 
     if (flap_pressed) {
         b->vy = c->flap_vy;
     }
 
     // gravity
     b->vy += c->gravity;
     if (b->vy > c->max_fall_vy) b->vy = c->max_fall_vy;
     if (b->vy < c->max_flap_vy) b->vy = c->max_flap_vy;
 
     // integrate
     b->y += b->vy;
     if (b->y < 0.0f) {
         b->y = 0.0f;
         b->vy = 0.0f;
     }
     if (b->y >= (float)a->ground_y) {
         b->y = (float)a->ground_y;
         b->game_over = 1;
     }
 
     int bird_x = a->width / 5;
     int bird_y_int = (int)(b->y + 0.5f);
 
    // move pipes & collisions
     for (int i = 0; i < MAX_PIPES; i++) {
         if (!pipes[i].active) continue;
         pipes[i].x -= c->pipe_speed / c->fps; // convert speed/sec to per-frame
         if (pipes[i].x < -5.0f) {
             pipes[i].active = 0;
             continue;
         }
 
         int pipe_x = (int)(pipes[i].x + 0.5f);
         int pipe_right = pipe_x + 2;
 
         // scoring: pipe just passed bird
         if (!b->game_over && pipe_right == bird_x - 1) {
             b->score += 1;
         }
 
         // collision
         if (pipe_x <= bird_x && pipe_right >= bird_x) {
             int gap_half = c->pipe_gap / 2;
             int top_end = pipes[i].gap_y - gap_half;
             int bottom_start = pipes[i].gap_y + (c->pipe_gap - gap_half);
             if (bird_y_int < top_end || bird_y_int >= bottom_start) {
                 b->game_over = 1;
             }
         }
     }

    // Maybe spawn a new pipe this frame
    if (!b->game_over && frand01() < c->pipe_spawn_chance) {
        spawn_pipe(pipes, a, c);
    }
 }
 
 static void draw_game(const BirdState *b, const Pipe pipes[], const Arena *a, const Config *c) {
     (void)c;
     erase();
 
     // HUD
     move(0, 0);
     if (!b->started) {
         printw("Press SPACE to start  |  Q to quit");
     } else if (b->game_over) {
         printw("GAME OVER  |  Score: %d  |  SPACE: restart  Q: quit", b->score);
     } else {
         printw("Score: %d", b->score);
     }
 
     // ground
     for (int x = 0; x < a->width; x++) {
         mvaddch(a->ground_y, x, '=');
     }
 
     // pipes
     for (int i = 0; i < MAX_PIPES; i++) {
         if (!pipes[i].active) continue;
         int pipe_x = (int)(pipes[i].x + 0.5f);
         if (pipe_x >= a->width || pipe_x + 2 < 0) continue;
 
         int gap_half = c->pipe_gap / 2;
         int top_end = pipes[i].gap_y - gap_half;
         int bottom_start = pipes[i].gap_y + (c->pipe_gap - gap_half);
 
         for (int x = pipe_x; x < pipe_x + 3; x++) {
             if (x < 0 || x >= a->width) continue;
             // top
             for (int y = 1; y < top_end && y < a->height; y++) {
                 mvaddch(y, x, '#');
             }
             // bottom
             for (int y = bottom_start; y < a->ground_y && y < a->height; y++) {
                 mvaddch(y, x, '#');
             }
         }
     }
 
     // bird
     int bird_x = a->width / 5;
     int bird_y_int = (int)(b->y + 0.5f);
     if (bird_y_int >= 1 && bird_y_int < a->ground_y && bird_x >= 0 && bird_x < a->width) {
         mvaddch(bird_y_int, bird_x, '@');
     }
 
     refresh();
 }
 
 int main(void) {
     srand((unsigned)time(NULL));
 
     initscr();
     cbreak();
     noecho();
     curs_set(0);
     nodelay(stdscr, TRUE);
     keypad(stdscr, TRUE);
 
     Arena arena;
     Config cfg;
     init_arena(&arena);
     init_config(&cfg);
 
     BirdState bird;
     Pipe pipes[MAX_PIPES];
     reset_game(&bird, pipes, &arena);
 
     struct timespec ts;
     ts.tv_sec = 0;
     ts.tv_nsec = (long)(1e9 / cfg.fps);
 
     for (;;) {
         int ch = getch();
         int flap_pressed = 0;
         if (ch == 'q' || ch == 'Q') break;
         if (ch == ' ' || ch == KEY_UP) {
             if (!bird.started) {
                 bird.started = 1;
                 bird.game_over = 0;
                 bird.vy = 0.0f;
             } else if (bird.game_over) {
                 reset_game(&bird, pipes, &arena);
                 bird.started = 1;
             } else {
                 flap_pressed = 1;
             }
         }
 
         init_arena(&arena); // adapt to terminal resizes
 
         update_game(&bird, pipes, &arena, &cfg, flap_pressed);
         draw_game(&bird, pipes, &arena, &cfg);
 
         nanosleep(&ts, NULL);
     }
 
     endwin();
     return 0;
 }
