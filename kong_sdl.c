// Simple Donkey Kong–style platformer in an SDL2 window.
// - Arrow keys / A,D: move left/right
// - W / Up / Space: jump
// - Esc: quit
//
// Dependencies: SDL2 + SDL2_image (for PNG sprites)
// Build example (on many distros):
//   gcc kong_sdl.c -o kong_sdl $(sdl2-config --cflags --libs) -lSDL2_image
// or (with pkg-config):
//   gcc kong_sdl.c -o kong_sdl `pkg-config --cflags --libs sdl2 SDL2_image`

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <math.h>
#include <stdio.h>

#define WINDOW_W 800
#define WINDOW_H 600

#define MAX_PLATFORMS 16
#define MAX_LADDERS   16
#define MAX_BARRELS   64
#define MAX_COINS     64

// --- Sprite sheet configuration ---------------------------------------------
// Final asset path: ./kong.png (relative to working directory)
//
// Layout: 6 columns x 4 rows grid (no padding).
// We detect the actual tile size from the texture, but
// we render into a fixed "world" tile size to keep the
// game layout consistent even if the art is high-res.
//
// World tile size (in window pixels):
#define WORLD_TILE 32

typedef struct {
    float x, y, w, h;
} RectF;

typedef struct {
    RectF rect;
} Platform;

typedef struct {
    RectF rect;
} Ladder;

typedef struct {
    RectF rect;
    float vx, vy;
    int active;
} Barrel;

typedef struct {
    RectF rect;
    int active;
} Coin;

typedef struct {
    RectF rect;
    float vx, vy;
    int onGround;
    int facing;      // -1 = left, 1 = right
    float runAnim;   // time accumulator for run cycle
} Player;

typedef struct {
    Platform platforms[MAX_PLATFORMS];
    int numPlatforms;
    Ladder ladders[MAX_LADDERS];
    int numLadders;
    Barrel barrels[MAX_BARRELS];
    Player player;
    RectF goal;        // princess area
    float gravity;
    float moveSpeed;
    float jumpSpeed;
    float barrelSpawnTimer;
    float barrelSpawnInterval;
    int running;
    int win;
    int gameOver;
    int score;
    int coinsCollected;
    float time;        // global time accumulator for simple animation
    float princess_t;  // princess idle animation timer

    Coin coins[MAX_COINS];
    int numCoins;
    int paused;
    int level;         // 1..25
    int lives;         // remaining lives
    int nextLifeScore; // score threshold for next extra life
} Game;

typedef struct {
    SDL_Texture *tex;
    int tileW;
    int tileH;
    SDL_Rect playerIdle;
    SDL_Rect playerRun1;
    SDL_Rect playerRun2;
    SDL_Rect playerJump;
    SDL_Rect playerClimb1;
    SDL_Rect playerClimb2;
    SDL_Rect barrel;
    SDL_Rect platform;
    SDL_Rect ladder;
    SDL_Rect goal;
    SDL_Rect floor;
    SDL_Rect ceiling;
    SDL_Rect coinFront;
    SDL_Rect coinSide;
} Sprites;

static SDL_Rect sprite_tile(const Sprites *s, int tx, int ty) {
    SDL_Rect r;
    r.x = tx * s->tileW;
    r.y = ty * s->tileH;
    r.w = s->tileW;
    r.h = s->tileH;
    return r;
}

static int load_sprites(SDL_Renderer *renderer, Sprites *s) {
    SDL_Texture *tex = IMG_LoadTexture(renderer, "kong.png");
    if (!tex) {
        fprintf(stderr, "IMG_LoadTexture(kong.png) failed: %s\n", IMG_GetError());
        return -1;
    }
    s->tex = tex;

    // Derive tile size from actual texture size (6x4 grid)
    int tw = 0, th = 0;
    if (SDL_QueryTexture(tex, NULL, NULL, &tw, &th) != 0) {
        fprintf(stderr, "SDL_QueryTexture failed: %s\n", SDL_GetError());
        return -1;
    }
    s->tileW = tw / 6;
    s->tileH = th / 4;
    if (s->tileW <= 0 || s->tileH <= 0) {
        fprintf(stderr, "Invalid tile size from kong.png: %dx%d\n", s->tileW, s->tileH);
        return -1;
    }
    printf("Loaded kong.png: %dx%d, tile %dx%d\n", tw, th, s->tileW, s->tileH);
    fflush(stdout);

    // Map to your described layout:
    // Hero
    s->playerIdle   = sprite_tile(s, 0, 0); // first run frame as idle
    s->playerRun1   = sprite_tile(s, 1, 0);
    s->playerRun2   = sprite_tile(s, 2, 0);
    s->playerJump   = sprite_tile(s, 4, 0);
    s->playerClimb1 = sprite_tile(s, 5, 0);
    s->playerClimb2 = sprite_tile(s, 5, 0); // reuse climb tile for now

    // Barrels
    s->barrel       = sprite_tile(s, 2, 1); // first roll frame

    // World tiles
    s->ladder       = sprite_tile(s, 0, 2);
    s->platform     = sprite_tile(s, 1, 2);
    s->floor        = sprite_tile(s, 2, 2);
    s->ceiling      = sprite_tile(s, 3, 2);
    s->coinFront    = sprite_tile(s, 4, 2);
    s->coinSide     = sprite_tile(s, 5, 2);

    // Princess / goal – use cheer frame as "win" pose; we'll animate using other frames
    s->goal         = sprite_tile(s, 5, 3);

    return 0;
}

static int rects_intersect(const RectF *a, const RectF *b) {
    return !(a->x + a->w <= b->x ||
             b->x + b->w <= a->x ||
             a->y + a->h <= b->y ||
             b->y + b->h <= a->y);
}

static void init_level(Game *g) {
    g->numPlatforms = 0;
    g->numLadders = 0;

    // Simple DK‑style zigzag platforms
    // Ground
    g->platforms[g->numPlatforms++].rect = (RectF){ 0, WINDOW_H - 40, WINDOW_W, 40 };
    // Row 1
    g->platforms[g->numPlatforms++].rect = (RectF){ 40, WINDOW_H - 140, WINDOW_W - 80, 20 };
    // Row 2
    g->platforms[g->numPlatforms++].rect = (RectF){ 80, WINDOW_H - 240, WINDOW_W - 160, 20 };
    // Row 3
    g->platforms[g->numPlatforms++].rect = (RectF){ 40, WINDOW_H - 340, WINDOW_W - 80, 20 };
    // Row 4 (top)
    g->platforms[g->numPlatforms++].rect = (RectF){ 120, WINDOW_H - 440, WINDOW_W - 240, 20 };

    // Ladders between rows
    g->ladders[g->numLadders++].rect = (RectF){ 80, WINDOW_H - 140, 20, 120 };
    g->ladders[g->numLadders++].rect = (RectF){ WINDOW_W - 120, WINDOW_H - 240, 20, 120 };
    g->ladders[g->numLadders++].rect = (RectF){ 140, WINDOW_H - 340, 20, 120 };
    g->ladders[g->numLadders++].rect = (RectF){ WINDOW_W / 2 - 10, WINDOW_H - 440, 20, 120 };

    // Goal (princess) at top right
    g->goal = (RectF){ WINDOW_W - 160, WINDOW_H - 480, 80, 60 };

    // Clear barrels
    for (int i = 0; i < MAX_BARRELS; i++) {
        g->barrels[i].active = 0;
    }

    // Player start at bottom left
    g->player.rect = (RectF){ 60, WINDOW_H - 80, 28, 36 };
    g->player.vx = 0;
    g->player.vy = 0;
    g->player.onGround = 0;
    g->player.facing = 1;
    g->player.runAnim = 0.0f;

    g->gravity = 1200.0f;
    g->moveSpeed = 220.0f;
    g->jumpSpeed = 520.0f;

    g->barrelSpawnTimer = 0.0f;
    // Barrel spawn interval scales a bit with level (faster at higher levels)
    float baseInterval = 2.5f;
    float minInterval = 0.8f;
    float interval = baseInterval - 0.06f * (g->level - 1);
    if (interval < minInterval) interval = minInterval;
    g->barrelSpawnInterval = interval;

    g->running = 1;
    g->win = 0;
    g->gameOver = 0;
    g->paused = 0;
    if (g->level <= 0) g->level = 1;
    if (g->level > 25) g->level = 25;
    g->coinsCollected = 0;
    g->time = 0.0f;
    g->princess_t = 0.0f;

    g->numCoins = 0;
    for (int i = 0; i < MAX_COINS; i++) {
        g->coins[i].active = 0;
    }
}

static void spawn_barrel(Game *g) {
    for (int i = 0; i < MAX_BARRELS; i++) {
        if (!g->barrels[i].active) {
            g->barrels[i].active = 1;
            g->barrels[i].rect.w = 26;
            g->barrels[i].rect.h = 26;
            // Spawn near top left on top row platform
            g->barrels[i].rect.x = g->platforms[g->numPlatforms - 1].rect.x + 10;
            g->barrels[i].rect.y = g->platforms[g->numPlatforms - 1].rect.y - g->barrels[i].rect.h;
            // Barrel speed also scales slightly with level
            g->barrels[i].vx = 140.0f + 10.0f * (g->level - 1);
            g->barrels[i].vy = 0.0f;
            break;
        }
    }
}

// Place some coins along the platforms using the tile size from sprites
static void place_coins(Game *g, const Sprites *s) {
    g->numCoins = 0;
    for (int i = 0; i < MAX_COINS; i++) {
        g->coins[i].active = 0;
    }

    // Skip ground (0) and maybe top (last) so coins are along the way
    for (int pi = 1; pi < g->numPlatforms - 1 && g->numCoins < MAX_COINS; pi++) {
        RectF *plat = &g->platforms[pi].rect;
        int coinsOnPlat = 3;
        for (int c = 0; c < coinsOnPlat && g->numCoins < MAX_COINS; c++) {
            float frac = (float)(c + 1) / (float)(coinsOnPlat + 1);
            float cx = plat->x + plat->w * frac;
            float cy = plat->y - (float)WORLD_TILE * 0.6f;

            Coin *coin = &g->coins[g->numCoins++];
            coin->active = 1;
            coin->rect.w = (float)WORLD_TILE * 0.8f;
            coin->rect.h = (float)WORLD_TILE * 0.8f;
            coin->rect.x = cx - coin->rect.w * 0.5f;
            coin->rect.y = cy - coin->rect.h * 0.5f;
        }
    }
}

static void handle_player_platform_collisions(Game *g, float dt) {
    Player *p = &g->player;
    p->onGround = 0;

    // Vertical resolution
    p->rect.y += p->vy * dt;
    for (int i = 0; i < g->numPlatforms; i++) {
        RectF *plat = &g->platforms[i].rect;
        if (rects_intersect(&p->rect, plat)) {
            if (p->vy > 0) {
                // Landing on top
                p->rect.y = plat->y - p->rect.h;
                p->vy = 0;
                p->onGround = 1;
            } else if (p->vy < 0) {
                // Hitting head
                p->rect.y = plat->y + plat->h;
                p->vy = 0;
            }
        }
    }

    // Horizontal resolution
    p->rect.x += p->vx * dt;
    for (int i = 0; i < g->numPlatforms; i++) {
        RectF *plat = &g->platforms[i].rect;
        if (rects_intersect(&p->rect, plat)) {
            if (p->vx > 0) {
                p->rect.x = plat->x - p->rect.w;
            } else if (p->vx < 0) {
                p->rect.x = plat->x + plat->w;
            }
            p->vx = 0;
        }
    }

    // Clamp to world bounds
    if (p->rect.x < 0) p->rect.x = 0;
    if (p->rect.x + p->rect.w > WINDOW_W) p->rect.x = WINDOW_W - p->rect.w;
    if (p->rect.y + p->rect.h > WINDOW_H) {
        // Fell off bottom: lose a life
        g->lives--;
        if (g->lives <= 0) {
            g->gameOver = 1;
        } else {
            g->gameOver = 1; // show death screen; restart via space
        }
    }
}

static int player_on_ladder(const Game *g) {
    RectF p = g->player.rect;
    // Check if player's center is within any ladder
    float cx = p.x + p.w * 0.5f;
    float cy = p.y + p.h * 0.5f;
    for (int i = 0; i < g->numLadders; i++) {
        const RectF *lad = &g->ladders[i].rect;
        if (cx >= lad->x && cx <= lad->x + lad->w &&
            cy >= lad->y && cy <= lad->y + lad->h) {
            return 1;
        }
    }
    return 0;
}

static void update_barrels(Game *g, float dt) {
    for (int i = 0; i < MAX_BARRELS; i++) {
        if (!g->barrels[i].active) continue;
        Barrel *b = &g->barrels[i];
        b->vy += g->gravity * dt * 0.7f;

        // Vertical
        b->rect.y += b->vy * dt;
        for (int p = 0; p < g->numPlatforms; p++) {
            RectF *plat = &g->platforms[p].rect;
            if (rects_intersect(&b->rect, plat)) {
                if (b->vy > 0) {
                    b->rect.y = plat->y - b->rect.h;
                    b->vy = 0;
                } else if (b->vy < 0) {
                    b->rect.y = plat->y + plat->h;
                    b->vy = 0;
                }
            }
        }

        // Horizontal
        b->rect.x += b->vx * dt;
        for (int p = 0; p < g->numPlatforms; p++) {
            RectF *plat = &g->platforms[p].rect;
            if (rects_intersect(&b->rect, plat)) {
                if (b->vx > 0) {
                    b->rect.x = plat->x - b->rect.w;
                } else if (b->vx < 0) {
                    b->rect.x = plat->x + plat->w;
                }
                b->vx = -b->vx; // bounce/reverse
            }
        }

        // World bounds
        if (b->rect.y > WINDOW_H + 100 || b->rect.x < -100 || b->rect.x > WINDOW_W + 100) {
            b->active = 0;
        }

        // Player collision -> lose life
        if (!g->gameOver && !g->win && rects_intersect(&b->rect, &g->player.rect)) {
            g->lives--;
            if (g->lives <= 0) {
                g->gameOver = 1;
            } else {
                g->gameOver = 1; // death screen, restart with space
            }
        }
    }
}

static void update_game(Game *g, float dt, int moveLeft, int moveRight, int jump, int climbUp, int climbDown) {
    // Global timers for animation always tick
    g->time += dt;
    g->princess_t += dt;

    if (!g->running || g->gameOver || g->win || g->paused) return;

    Player *p = &g->player;

    // Horizontal input
    p->vx = 0;
    if (moveLeft)  p->vx -= g->moveSpeed;
    if (moveRight) p->vx += g->moveSpeed;

    // Facing and run animation
    if (moveLeft && !moveRight) {
        p->facing = -1;
    } else if (moveRight && !moveLeft) {
        p->facing = 1;
    }

    int onLadder = player_on_ladder(g);

    if (onLadder && (climbUp || climbDown)) {
        // Disable gravity while climbing
        p->vy = 0;
        if (climbUp)   p->rect.y -= 180.0f * dt;
        if (climbDown) p->rect.y += 180.0f * dt;
    } else {
        // Apply gravity
        p->vy += g->gravity * dt;
        if (jump && p->onGround) {
            p->vy = -g->jumpSpeed;
            p->onGround = 0;
        }
    }

    handle_player_platform_collisions(g, dt);

    // Run animation timer (only when moving on ground)
    if (p->onGround && (moveLeft ^ moveRight) && fabsf(p->vx) > 1.0f) {
        p->runAnim += dt;
    } else {
        p->runAnim = 0.0f;
    }

    // Check win
    if (!g->win && rects_intersect(&p->rect, &g->goal)) {
        g->win = 1;
        // Simple win bonus
        g->score += 1000;
    }

    // Barrels
    g->barrelSpawnTimer += dt;
    if (g->barrelSpawnTimer >= g->barrelSpawnInterval) {
        g->barrelSpawnTimer = 0.0f;
        spawn_barrel(g);
    }
    update_barrels(g, dt);

    // Coins collection
    for (int i = 0; i < g->numCoins; i++) {
        Coin *coin = &g->coins[i];
        if (!coin->active) continue;
        if (rects_intersect(&p->rect, &coin->rect)) {
            coin->active = 0;
            g->coinsCollected++;
            g->score += 100;
            // Extra life every 5000 score
            while (g->score >= g->nextLifeScore) {
                g->lives++;
                g->nextLifeScore += 5000;
            }
        }
    }
}

static void render_rect(SDL_Renderer *r, const RectF *rect, Uint8 cr, Uint8 cg, Uint8 cb, Uint8 ca) {
    SDL_Rect sr = { (int)rect->x, (int)rect->y, (int)rect->w, (int)rect->h };
    SDL_SetRenderDrawColor(r, cr, cg, cb, ca);
    SDL_RenderFillRect(r, &sr);
}

static void render_sprite(SDL_Renderer *r, SDL_Texture *tex, const SDL_Rect *src, const RectF *dst, SDL_RendererFlip flip) {
    SDL_Rect d = { (int)dst->x, (int)dst->y, (int)dst->w, (int)dst->h };
    SDL_RenderCopyEx(r, tex, src, &d, 0.0, NULL, flip);
}

static void render_game(SDL_Renderer *renderer, const Game *g, const Sprites *s) {
    SDL_SetRenderDrawColor(renderer, 10, 10, 20, 255);
    SDL_RenderClear(renderer);

    // --- HUD: Level, Score, Lives at top ---
    {
        // Simple 3x5 digit font reused from scoreboard
        static const char *digits[10] = {
            "111101101101111", // 0
            "010110010010111", // 1
            "111001111100111", // 2
            "111001111001111", // 3
            "101101111001001", // 4
            "111100111001111", // 5
            "111100111101111", // 6
            "111001001001001", // 7
            "111101111101111", // 8
            "111101111001111"  // 9
        };

        int hudY = 4;
        int scale = 3;

        // Helper to draw positive integer; returns x after last digit
        int drawNumberAt(int x, int value) {
            if (value < 0) value = 0;
            char buf[16];
            snprintf(buf, sizeof(buf), "%d", value);
            int digitX = x;
            for (const char *p = buf; *p; ++p) {
                if (*p < '0' || *p > '9') continue;
                int d = *p - '0';
                const char *pat = digits[d];
                for (int row = 0; row < 5; row++) {
                    for (int col = 0; col < 3; col++) {
                        if (pat[row * 3 + col] == '1') {
                            SDL_Rect r = {
                                digitX + col * scale,
                                hudY + row * scale,
                                scale, scale
                            };
                            SDL_SetRenderDrawColor(renderer, 220, 220, 220, 255);
                            SDL_RenderFillRect(renderer, &r);
                        }
                    }
                }
                digitX += 3 * scale + scale;
            }
            return digitX;
        }

        int x = 10;
        // Level
        x = drawNumberAt(x, g->level) + 10;
        // Score
        x = drawNumberAt(x, g->score) + 10;
        // Lives (just draw as a number for now)
        drawNumberAt(x, g->lives);
    }

    // Platforms (tiled, not stretched; world tiles are fixed size)
    for (int i = 0; i < g->numPlatforms; i++) {
        const RectF *plat = &g->platforms[i].rect;
        const SDL_Rect *src = (i == 0) ? &s->floor : &s->platform; // ground uses floor tile
        for (int y = (int)plat->y; y < (int)(plat->y + plat->h); y += WORLD_TILE) {
            for (int x = (int)plat->x; x < (int)(plat->x + plat->w); x += WORLD_TILE) {
                RectF dst = { (float)x, (float)y, (float)WORLD_TILE, (float)WORLD_TILE };
                render_sprite(renderer, s->tex, src, &dst, SDL_FLIP_NONE);
            }
        }
    }

    // Ladders (tiled vertically in world tile size)
    for (int i = 0; i < g->numLadders; i++) {
        const RectF *lad = &g->ladders[i].rect;
        for (int y = (int)lad->y; y < (int)(lad->y + lad->h); y += WORLD_TILE) {
            RectF dst = { lad->x, (float)y, (float)WORLD_TILE, (float)WORLD_TILE };
            render_sprite(renderer, s->tex, &s->ladder, &dst, SDL_FLIP_NONE);
        }
    }

    // Goal / princess (animated, not stretched)
    {
        // Base position is center of goal rect
        float gx = g->goal.x + g->goal.w * 0.5f;
        float gy = g->goal.y + g->goal.h - WORLD_TILE; // stand on platform

        // Simple pacing using sin wave within goal area
        float amp = g->goal.w * 0.3f;
        float offset = sinf(g->princess_t * 1.5f) * amp;
        float px = gx + offset - WORLD_TILE * 0.5f;

        const SDL_Rect *princessSrc;
        if (g->win) {
            // Alternate between jump and cheer on win
            int frame = ((int)(g->time * 6.0f)) % 2;
            princessSrc = (frame == 0) ? &s->goal /* cheer */ : &s->playerJump;
        } else {
            // Idle pacing run animation using princess run frames (row 3 col 0-2)
            int idx = ((int)(g->princess_t * 6.0f)) % 3;
            SDL_Rect runFrames[3] = {
                sprite_tile(s, 0, 3),
                sprite_tile(s, 1, 3),
                sprite_tile(s, 2, 3)
            };
            princessSrc = &runFrames[idx];
        }

        RectF dst = { px, gy, (float)WORLD_TILE, (float)WORLD_TILE };
        SDL_RendererFlip flip = (offset < 0.0f) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
        render_sprite(renderer, s->tex, princessSrc, &dst, flip);
    }

    // Coins
    for (int i = 0; i < g->numCoins; i++) {
        const Coin *coin = &g->coins[i];
        if (!coin->active) continue;
        int phase = ((int)(g->time * 8.0f) + i) % 2;
        const SDL_Rect *src = (phase == 0) ? &s->coinFront : &s->coinSide;
        render_sprite(renderer, s->tex, src, &coin->rect, SDL_FLIP_NONE);
    }

    // Barrels
    for (int i = 0; i < MAX_BARRELS; i++) {
        if (!g->barrels[i].active) continue;
        render_sprite(renderer, s->tex, &s->barrel, &g->barrels[i].rect, SDL_FLIP_NONE);
    }

    // Player: choose frame and flip based on movement
    const SDL_Rect *playerSrc = &s->playerIdle;
    // Climbing has priority over jumping
    if (player_on_ladder(g)) {
        int frame = ((int)(g->time * 6.0f)) % 2;
        playerSrc = (frame == 0) ? &s->playerClimb1 : &s->playerClimb2;
    } else if (!g->player.onGround) {
        playerSrc = &s->playerJump;
    } else {
        // Running animation when moving horizontally on ground
        if (fabsf(g->player.vx) > 1.0f) {
            SDL_Rect runFrames[3] = {
                s->playerIdle,
                s->playerRun1,
                s->playerRun2
            };
            int idx = ((int)(g->player.runAnim * 10.0f)) % 3;
            playerSrc = &runFrames[idx];
        } else {
            playerSrc = &s->playerIdle;
        }
    }
    SDL_RendererFlip heroFlip = (g->player.facing < 0) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
    render_sprite(renderer, s->tex, playerSrc, &g->player.rect, heroFlip);

    // Overlay win / game over + score
    if (g->win || g->gameOver) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 160);
        SDL_Rect full = { 0, 0, WINDOW_W, WINDOW_H };
        SDL_RenderFillRect(renderer, &full);

        // Box background color
        RectF box = { WINDOW_W / 2 - 160, WINDOW_H / 2 - 80, 320, 160 };
        if (g->win) {
            render_rect(renderer, &box, 40, 200, 80, 255);
        } else if (g->gameOver) {
            render_rect(renderer, &box, 200, 40, 40, 255);
        }

        // Draw final score inside the box using a tiny bitmap font
        int score = g->score;
        if (score < 0) score = 0;
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", score);

        // Draw "SCORE" label as coins, then digits as blocks
        int startX = (int)box.x + 20;
        int startY = (int)box.y + 20;

        // Simple coin icons to hint "score"
        int coinsToDraw = g->coinsCollected;
        if (coinsToDraw > 5) coinsToDraw = 5;
        for (int i = 0; i < coinsToDraw; i++) {
            RectF cd = { (float)(startX + i * (WORLD_TILE / 2)), (float)startY,
                         (float)(WORLD_TILE / 2), (float)(WORLD_TILE / 2) };
            render_sprite(renderer, s->tex, &s->coinFront, &cd, SDL_FLIP_NONE);
        }

        // Draw numeric score below coins
        int digitX = startX;
        int digitY = startY + WORLD_TILE / 2 + 8;
        int scale = 4; // size of "pixels" in the bitmap font

        // 3x5 digit patterns
        static const char *digits[10] = {
            "111101101101111", // 0
            "010110010010111", // 1
            "111001111100111", // 2
            "111001111001111", // 3
            "101101111001001", // 4
            "111100111001111", // 5
            "111100111101111", // 6
            "111001001001001", // 7
            "111101111101111", // 8
            "111101111001111"  // 9
        };

        for (const char *p = buf; *p; ++p) {
            if (*p < '0' || *p > '9') continue;
            int d = *p - '0';
            const char *pat = digits[d];
            for (int row = 0; row < 5; row++) {
                for (int col = 0; col < 3; col++) {
                    if (pat[row * 3 + col] == '1') {
                        SDL_Rect r = {
                            digitX + col * scale,
                            digitY + row * scale,
                            scale, scale
                        };
                        SDL_SetRenderDrawColor(renderer, 10, 10, 10, 255);
                        SDL_RenderFillRect(renderer, &r);
                    }
                }
            }
            digitX += 3 * scale + scale; // advance to next digit
        }

        // Princess in the scoreboard popup, jumping/cheering
        {
            float px = box.x + box.w - WORLD_TILE * 1.5f;
            float py = box.y + box.h - WORLD_TILE * 1.5f;
            int frame = ((int)(g->time * 6.0f)) % 3;
            // Use crouch (3), jump (4), cheer (5) from row 3
            SDL_Rect cheerFrames[3] = {
                sprite_tile(s, 3, 3),
                sprite_tile(s, 4, 3),
                sprite_tile(s, 5, 3)
            };
            const SDL_Rect *ps = &cheerFrames[frame];
            RectF dst = { px, py, (float)WORLD_TILE, (float)WORLD_TILE };
            render_sprite(renderer, s->tex, ps, &dst, SDL_FLIP_NONE);
        }
    }

    SDL_RenderPresent(renderer);
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    int imgFlags = IMG_INIT_PNG;
    if ((IMG_Init(imgFlags) & imgFlags) != imgFlags) {
        fprintf(stderr, "IMG_Init failed: %s\n", IMG_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "Mini Kong",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_W, WINDOW_H,
        SDL_WINDOW_SHOWN
    );
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    Sprites sprites;
    if (load_sprites(renderer, &sprites) != 0) {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    Game game;
    SDL_memset(&game, 0, sizeof(Game));
    game.level = 1;
    game.score = 0;
    game.lives = 3;
    game.nextLifeScore = 5000;
    init_level(&game);
    // Now that we know tile sizes, place coins relative to platforms
    place_coins(&game, &sprites);

    Uint32 lastTicks = SDL_GetTicks();
    const float targetDelta = 1.0f / 60.0f;

    while (game.running) {
        SDL_Event e;
        int moveLeft = 0, moveRight = 0, jump = 0, climbUp = 0, climbDown = 0;

        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                game.running = 0;
            } else if (e.type == SDL_KEYDOWN) {
                SDL_Keycode key = e.key.keysym.sym;
                if (key == SDLK_ESCAPE || key == SDLK_q) {
                    game.running = 0;
                } else if (key == SDLK_p) {
                    if (!game.gameOver && !game.win) {
                        game.paused = !game.paused;
                    }
                } else if (key == SDLK_r) {
                    // R: restart whole run from level 1
                    game.level = 1;
                    game.score = 0;
                    game.lives = 3;
                    game.nextLifeScore = 5000;
                    game.win = 0;
                    game.gameOver = 0;
                    init_level(&game);
                    place_coins(&game, &sprites);
                } else if (key == SDLK_SPACE) {
                    // Space from win screen: advance level (up to 25, then wrap)
                    if (game.win) {
                        if (game.level < 25) {
                            game.level++;
                        } else {
                            game.level = 1;
                        }
                        game.win = 0;
                        game.gameOver = 0;
                        init_level(&game);
                        place_coins(&game, &sprites);
                    } else if (game.gameOver) {
                        // From death screen: restart current level while lives remain,
                        // otherwise full reset.
                        if (game.lives > 0) {
                            game.gameOver = 0;
                            init_level(&game);
                            place_coins(&game, &sprites);
                        } else {
                            game.level = 1;
                            game.score = 0;
                            game.lives = 3;
                            game.nextLifeScore = 5000;
                            game.gameOver = 0;
                            init_level(&game);
                            place_coins(&game, &sprites);
                        }
                    }
                }
            }
        }

        const Uint8 *keys = SDL_GetKeyboardState(NULL);
        moveLeft  = keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_A];
        moveRight = keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D];
        jump      = keys[SDL_SCANCODE_SPACE] || keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP];
        climbUp   = keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP];
        climbDown = keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN];

        Uint32 now = SDL_GetTicks();
        float dt = (now - lastTicks) / 1000.0f;
        if (dt > 0.05f) dt = 0.05f; // clamp big hitches
        lastTicks = now;

        // Fixed-ish timestep accumulation
        static float acc = 0.0f;
        acc += dt;
        while (acc >= targetDelta) {
            update_game(&game, targetDelta, moveLeft, moveRight, jump, climbUp, climbDown);
            acc -= targetDelta;
        }

        render_game(renderer, &game, &sprites);

        // small delay to avoid 100% CPU when vsync is off
        SDL_Delay(1);
    }

    SDL_DestroyTexture(sprites.tex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    return 0;
}

