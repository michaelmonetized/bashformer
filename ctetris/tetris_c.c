#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <locale.h>
 
#define BOARD_W 14
 #define BOARD_H 20
 
 typedef struct {
     int cells[4][4]; // 4x4 matrix
 } Shape;
 
 typedef struct {
     int type;      // 0-6
     int rot;       // 0-3
     int x, y;      // position of piece origin on board
 } Piece;
 
 typedef struct {
     int width;
     int height;
 } TermSize;
 
 typedef struct {
     int score;
     int lines;
     int level;
     int game_over;
    int paused;
 } GameInfo;
 
 static Shape SHAPES[7][4]; // 7 tetrominoes, 4 rotations each
 
 static void init_shapes(void) {
     // I piece
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
 
     // O piece
     int O0[4][4] = {
         {0,1,1,0},
         {0,1,1,0},
         {0,0,0,0},
         {0,0,0,0}
     };
 
     // T piece
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
 
     // L piece
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
 
     // J piece
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
 
     // S piece
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
 
     // Z piece
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
 
     // Initialize all shapes to zero first
     for (int t = 0; t < 7; t++) {
         for (int r = 0; r < 4; r++) {
             for (int y = 0; y < 4; y++)
                 for (int x = 0; x < 4; x++)
                     SHAPES[t][r].cells[y][x] = 0;
         }
     }
 
     // I
     for (int y = 0; y < 4; y++)
         for (int x = 0; x < 4; x++) {
             SHAPES[0][0].cells[y][x] = I0[y][x];
             SHAPES[0][1].cells[y][x] = I1[y][x];
             SHAPES[0][2].cells[y][x] = I2[y][x];
             SHAPES[0][3].cells[y][x] = I3[y][x];
         }
     // O (same for all rotations)
     for (int y = 0; y < 4; y++)
         for (int x = 0; x < 4; x++)
             SHAPES[1][0].cells[y][x] = SHAPES[1][1].cells[y][x] =
                 SHAPES[1][2].cells[y][x] = SHAPES[1][3].cells[y][x] = O0[y][x];
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
 
 static TermSize get_term_size(void) {
     TermSize ts;
     int rows, cols;
     getmaxyx(stdscr, rows, cols);
     ts.width = cols;
     ts.height = rows;
     return ts;
 }
 
 static int board[BOARD_H][BOARD_W];
 
 static void clear_board(void) {
     for (int y = 0; y < BOARD_H; y++)
         for (int x = 0; x < BOARD_W; x++)
             board[y][x] = 0;
 }
 
 static int collides(const Piece *p) {
     for (int dy = 0; dy < 4; dy++) {
         for (int dx = 0; dx < 4; dx++) {
             if (!SHAPES[p->type][p->rot].cells[dy][dx]) continue;
             int bx = p->x + dx;
             int by = p->y + dy;
             if (bx < 0 || bx >= BOARD_W || by < 0 || by >= BOARD_H) return 1;
             if (board[by][bx]) return 1;
         }
     }
     return 0;
 }
 
 static void place_piece(const Piece *p) {
     for (int dy = 0; dy < 4; dy++) {
         for (int dx = 0; dx < 4; dx++) {
             if (!SHAPES[p->type][p->rot].cells[dy][dx]) continue;
             int bx = p->x + dx;
             int by = p->y + dy;
             if (bx >= 0 && bx < BOARD_W && by >= 0 && by < BOARD_H) {
                 board[by][bx] = p->type + 1;
             }
         }
     }
 }
 
 static void clear_lines(GameInfo *info) {
     for (int y = BOARD_H - 1; y >= 0; y--) {
         int full = 1;
         for (int x = 0; x < BOARD_W; x++) {
             if (!board[y][x]) {
                 full = 0;
                 break;
             }
         }
         if (full) {
             info->lines++;
             info->score += 100;
             // shift down
             for (int yy = y; yy > 0; yy--) {
                 for (int x = 0; x < BOARD_W; x++) {
                     board[yy][x] = board[yy-1][x];
                 }
             }
             for (int x = 0; x < BOARD_W; x++) board[0][x] = 0;
             y++; // re-check this row
         }
     }
 }
 
 static void spawn_piece(Piece *p) {
     p->type = rand() % 7;
     p->rot = 0;
     p->x = BOARD_W / 2 - 2;
     p->y = 0;
 }
 
 static void draw_game(const TermSize *ts, const GameInfo *info, const Piece *current, int shadow_y) {
     erase();
 
     int offset_x = (ts->width - BOARD_W * 2) / 2;
     if (offset_x < 0) offset_x = 0;
     int offset_y = (ts->height - BOARD_H) / 2;
     if (offset_y < 1) offset_y = 1;
 
    // HUD
    mvprintw(0, 0, "Score: %d  Lines: %d  Q: quit  Arrows: move  Space: drop  Up: rotate  P: pause", info->score, info->lines);
    if (info->game_over) {
        mvprintw(1, 0, "GAME OVER - press R to restart");
    } else if (info->paused) {
        mvprintw(1, 0, "PAUSED - press P to resume");
    }
 
    // Draw board background blocks (locked pieces)
    for (int y = 0; y < BOARD_H; y++) {
        for (int x = 0; x < BOARD_W; x++) {
            int val = board[y][x];         // 0 = empty, 1..7 = tetromino type
            int sy = offset_y + y;
            int sx = offset_x + x * 2;
            if (val > 0) {
                int color_pair = val;      // 1..7
                attron(COLOR_PAIR(color_pair));
                mvaddstr(sy, sx, "██");
                attroff(COLOR_PAIR(color_pair));
            } else {
                mvaddstr(sy, sx, "  ");
            }
        }
    }

    // Draw border around playfield
    int left   = offset_x - 1;
    int right  = offset_x + BOARD_W * 2;
    int top    = offset_y - 1;
    int bottom = offset_y + BOARD_H;
    if (left < 0) left = 0;
    if (top  < 1) top  = 1;
    // horizontal
    for (int x = left; x <= right; x++) {
        mvaddch(top, x, '-');
        mvaddch(bottom, x, '-');
    }
    // vertical
    for (int y = top; y <= bottom; y++) {
        mvaddch(y, left, '|');
        mvaddch(y, right, '|');
    }
 
     // Shadow
     if (!info->game_over) {
         Piece shadow = *current;
         shadow.y = shadow_y;
         for (int dy = 0; dy < 4; dy++)
             for (int dx = 0; dx < 4; dx++)
                 if (SHAPES[shadow.type][shadow.rot].cells[dy][dx]) {
                     int bx = shadow.x + dx;
                     int by = shadow.y + dy;
                     if (bx >= 0 && bx < BOARD_W && by >= 0 && by < BOARD_H) {
                         mvaddch(offset_y + by, offset_x + bx * 2, '.');
                         mvaddch(offset_y + by, offset_x + bx * 2 + 1, '.');
                     }
                 }
     }
 
     // Current piece
     if (!info->game_over) {
         for (int dy = 0; dy < 4; dy++)
             for (int dx = 0; dx < 4; dx++)
                 if (SHAPES[current->type][current->rot].cells[dy][dx]) {
                     int bx = current->x + dx;
                     int by = current->y + dy;
                     if (bx >= 0 && bx < BOARD_W && by >= 0 && by < BOARD_H) {
                         mvaddch(offset_y + by, offset_x + bx * 2, '@');
                         mvaddch(offset_y + by, offset_x + bx * 2 + 1, '@');
                     }
                 }
     }
 
     refresh();
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

    if (has_colors()) {
        start_color();
        use_default_colors();
        // 7 classic tetromino colors
        init_pair(1, COLOR_CYAN,   -1); // I
        init_pair(2, COLOR_YELLOW, -1); // O
        init_pair(3, COLOR_MAGENTA,-1); // T
        init_pair(4, COLOR_WHITE,  -1); // L
        init_pair(5, COLOR_BLUE,   -1); // J
        init_pair(6, COLOR_GREEN,  -1); // S
        init_pair(7, COLOR_RED,    -1); // Z
        init_pair(8, COLOR_WHITE,  -1); // shadow
    }
 
     init_shapes();
     clear_board();
 
     GameInfo info = {0};
     TermSize ts = get_term_size();
     Piece current;
     spawn_piece(&current);
 
     int drop_counter = 0;
     int drop_delay = 30; // frames per drop at level 1
 
     struct timespec tsleep;
     tsleep.tv_sec = 0;
     tsleep.tv_nsec = (long)(1e9 / 60); // ~60Hz render/update
 
     int running = 1;
     while (running) {
         int ch = getch();
         if (ch == 'q' || ch == 'Q') {
             running = 0;
         } else if (ch == 'r' || ch == 'R') {
             clear_board();
             info.score = info.lines = 0;
             info.game_over = 0;
            info.paused = 0;
             spawn_piece(&current);
        } else if (ch == 'p' || ch == 'P') {
            if (!info.game_over) {
                info.paused = !info.paused;
            }
         }
 
         Piece trial = current;
 
        if (!info.game_over && !info.paused) {
             if (ch == KEY_LEFT) {
                 trial.x--;
                 if (!collides(&trial)) current = trial;
             } else if (ch == KEY_RIGHT) {
                 trial.x++;
                 if (!collides(&trial)) current = trial;
             } else if (ch == KEY_UP) { // rotate
                 trial.rot = (trial.rot + 1) % 4;
                 if (!collides(&trial)) current = trial;
             } else if (ch == KEY_DOWN) { // soft drop
                 trial.y++;
                 if (!collides(&trial)) {
                     current = trial;
                     info.score += 1;
                 }
             } else if (ch == ' ') { // hard drop
                 while (!collides(&trial)) {
                     current = trial;
                     trial.y++;
                 }
                 place_piece(&current);
                 clear_lines(&info);
                 spawn_piece(&current);
                 if (collides(&current)) {
                     info.game_over = 1;
                 }
             }
         }
 
         // auto-drop
        if (!info.game_over && !info.paused) {
             drop_counter++;
             if (drop_counter >= drop_delay) {
                 drop_counter = 0;
                 trial = current;
                 trial.y++;
                 if (!collides(&trial)) {
                     current = trial;
                 } else {
                     place_piece(&current);
                     clear_lines(&info);
                     spawn_piece(&current);
                     if (collides(&current)) {
                         info.game_over = 1;
                     }
                 }
             }
         }
 
         // compute shadow y
         int shadow_y = current.y;
         Piece tmp = current;
         while (1) {
             tmp.y++;
             if (collides(&tmp)) break;
             shadow_y = tmp.y;
         }
 
         ts = get_term_size();
         draw_game(&ts, &info, &current, shadow_y);
 
         nanosleep(&tsleep, NULL);
     }
 
     endwin();
     return 0;
 }
