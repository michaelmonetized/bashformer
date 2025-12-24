// Simple Donkey Kong–style platformer in an SDL2 window.
// Controls (Vim-style):
// - h / Left: move left
// - l / Right / D: move right
// - j / Up / Space / W: jump
// - k / Up / W: climb up
// - g: attack (break barrels with sword)
// - Esc: quit
//
// Dependencies: SDL2 + SDL2_image (for PNG sprites)
// Build example (on many distros):
//   gcc kong_sdl.c -o kong_sdl $(sdl2-config --cflags --libs) -lSDL2_image
// or (with pkg-config):
//   gcc kong_sdl.c -o kong_sdl `pkg-config --cflags --libs sdl2 SDL2_image`

#include <SDL.h>
#include <SDL_image.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define WINDOW_W 800
#define WINDOW_H 600

#define MAX_PLATFORMS 16
#define MAX_LADDERS   16
#define MAX_BARRELS   64
#define MAX_COINS     64
#define MAX_POWERUPS  16
#define MAX_BADDIES   32

// Power-up types
#define POWER_SWORD    0  // power.png 0:0
#define POWER_FLAME    1  // power.png 1:0
#define POWER_LIGHTNING 2 // power.png 0:1 (super beast mode)
#define POWER_HEART    3  // power.png 1:1 (gives life)

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
    int broken;  // 1 if broken, 0 if normal
    float brokenTime;  // Time since broken (for animation/deletion)
} Barrel;

typedef struct {
    RectF rect;
    int active;
} Coin;

typedef struct {
    RectF rect;
    int active;
    int type;  // POWER_SWORD, POWER_FLAME, POWER_LIGHTNING, POWER_HEART
    float animTime; // for animation
} PowerUp;

typedef struct {
    RectF rect;
    float vx, vy;
    int active;
    int dying;  // 1 if dying (death animation), 0 if alive
    float deathTime;  // Time since death started
    int type;   // Which row in baddies.jpg (0-5): slimes, golems, imps, knights, goblins, bats
    int facing; // -1 = left, 1 = right
    float animTime; // Time accumulator for walk animation
    int onGround;
} Baddie;

typedef struct {
    RectF rect;
    float vx, vy;
    int onGround;
    int facing;      // -1 = left, 1 = right
    float runAnim;   // time accumulator for run cycle
    int hasSword;    // can break barrels
    int hasFlame;    // uses hero row 3
    int hasSuperBeast; // uses hero row 1, or row 2 if combined with sword
    float swordTimer;     // time remaining for sword power (0 = inactive)
    float flameTimer;     // time remaining for flame power (0 = inactive)
    float superBeastTimer; // time remaining for super beast power (0 = inactive)
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
    PowerUp powerUps[MAX_POWERUPS];
    int numPowerUps;
    Baddie baddies[MAX_BADDIES];
    int numBaddies;
    float baddieSpawnTimer;
    float baddieSpawnInterval;
    int paused;
    int level;         // 1..25
    int lives;         // remaining lives
    int nextLifeScore; // score threshold for next extra life
} Game;

typedef struct {
    SDL_Texture *tex;        // kong.png
    SDL_Texture *heroTex;   // hero.png
    SDL_Texture *powerTex;   // power.png
    SDL_Texture *baddiesTex; // baddies.jpg
    SDL_Texture *bgTex;      // kong-bgs.jpg - single texture with 4 backgrounds
    int bgTileW;             // width of each background
    int bgTileH;             // height of each background
    int tileW;
    int tileH;
    int heroTileW;
    int heroTileH;
    int powerTileW;
    int powerTileH;
    int baddieTileW;
    int baddieTileH;
    SDL_Rect playerIdle;
    SDL_Rect playerRun1;
    SDL_Rect playerRun2;
    SDL_Rect playerJump;
    SDL_Rect playerClimb1;
    SDL_Rect playerClimb2;
    SDL_Rect barrel;
    SDL_Rect barrelBroken;   // kong.png 1:5
    SDL_Rect platform;
    SDL_Rect ladder;
    SDL_Rect goal;
    SDL_Rect floor;
    SDL_Rect ceiling;
    SDL_Rect coinFront;
    SDL_Rect coinSide;
    // Power-up sprites
    SDL_Rect powerSword;     // power.png 0:0
    SDL_Rect powerFlame;     // power.png 1:0
    SDL_Rect powerLightning; // power.png 0:1
    SDL_Rect powerHeart;     // power.png 1:1
} Sprites;

static SDL_Rect sprite_tile(const Sprites *s, int tx, int ty) {
    SDL_Rect r;
    r.x = tx * s->tileW;
    r.y = ty * s->tileH;
    r.w = s->tileW;
    r.h = s->tileH;
    return r;
}

static SDL_Rect sprite_tile_custom(int tileW, int tileH, int tx, int ty) {
    SDL_Rect r;
    r.x = tx * tileW;
    r.y = ty * tileH;
    r.w = tileW;
    r.h = tileH;
    return r;
}

static int load_sprites(SDL_Renderer *renderer, Sprites *s) {
    // Load kong.png
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
    // Hero (from kong.png - will be replaced by hero.png when powered up)
    s->playerIdle   = sprite_tile(s, 0, 0); // first run frame as idle
    s->playerRun1   = sprite_tile(s, 1, 0);
    s->playerRun2   = sprite_tile(s, 2, 0);
    s->playerJump   = sprite_tile(s, 4, 0);
    s->playerClimb1 = sprite_tile(s, 5, 0);
    s->playerClimb2 = sprite_tile(s, 5, 0); // reuse climb tile for now

    // Barrels
    s->barrel       = sprite_tile(s, 2, 1); // first roll frame
    s->barrelBroken = sprite_tile(s, 5, 1); // broken barrel (kong.png column 5, row 1)

    // World tiles
    s->ladder       = sprite_tile(s, 0, 2);
    s->platform     = sprite_tile(s, 1, 2);
    s->floor        = sprite_tile(s, 2, 2);
    s->ceiling      = sprite_tile(s, 3, 2);
    s->coinFront    = sprite_tile(s, 4, 2);
    s->coinSide     = sprite_tile(s, 5, 2);

    // Princess / goal – use cheer frame as "win" pose; we'll animate using other frames
    s->goal         = sprite_tile(s, 5, 3);

    // Load hero.png
    s->heroTex = IMG_LoadTexture(renderer, "hero.png");
    if (!s->heroTex) {
        fprintf(stderr, "IMG_LoadTexture(hero.png) failed: %s\n", IMG_GetError());
        return -1;
    }
    int htw = 0, hth = 0;
    if (SDL_QueryTexture(s->heroTex, NULL, NULL, &htw, &hth) != 0) {
        fprintf(stderr, "SDL_QueryTexture hero.png failed: %s\n", SDL_GetError());
        return -1;
    }
    // Assuming hero.png is 6 columns x 4 rows (normal, super beast, sword+super, flame)
    s->heroTileW = htw / 6;
    s->heroTileH = hth / 4;
    printf("Loaded hero.png: %dx%d, tile %dx%d\n", htw, hth, s->heroTileW, s->heroTileH);
    fflush(stdout);

    // Load power.png
    s->powerTex = IMG_LoadTexture(renderer, "power.png");
    if (!s->powerTex) {
        fprintf(stderr, "IMG_LoadTexture(power.png) failed: %s\n", IMG_GetError());
        return -1;
    }
    int ptw = 0, pth = 0;
    if (SDL_QueryTexture(s->powerTex, NULL, NULL, &ptw, &pth) != 0) {
        fprintf(stderr, "SDL_QueryTexture power.png failed: %s\n", SDL_GetError());
        return -1;
    }
    // Assuming power.png is 2 columns x 2 rows
    s->powerTileW = ptw / 2;
    s->powerTileH = pth / 2;
    printf("Loaded power.png: %dx%d, tile %dx%d\n", ptw, pth, s->powerTileW, s->powerTileH);
    fflush(stdout);

    // Power-up sprites
    // power.png layout: 0:0 sword, 0:1 lightning, 1:0 fire, 1:1 heart
    s->powerSword = sprite_tile_custom(s->powerTileW, s->powerTileH, 0, 0);
    s->powerFlame = sprite_tile_custom(s->powerTileW, s->powerTileH, 1, 0);
    s->powerLightning = sprite_tile_custom(s->powerTileW, s->powerTileH, 0, 1);
    s->powerHeart = sprite_tile_custom(s->powerTileW, s->powerTileH, 1, 1);

    // Load baddies.jpg with green screen color key (#35b522 = RGB(53, 181, 34))
    SDL_Surface *baddiesSurface = IMG_Load("baddies.jpg");
    if (!baddiesSurface) {
        fprintf(stderr, "IMG_Load(baddies.jpg) failed: %s\n", IMG_GetError());
        return -1;
    }
    // Set color key for green screen transparency
    Uint32 colorKey = SDL_MapRGB(baddiesSurface->format, 0x35, 0xb5, 0x22);
    SDL_SetColorKey(baddiesSurface, SDL_TRUE, colorKey);
    s->baddiesTex = SDL_CreateTextureFromSurface(renderer, baddiesSurface);
    SDL_FreeSurface(baddiesSurface);
    if (!s->baddiesTex) {
        fprintf(stderr, "SDL_CreateTextureFromSurface(baddies.jpg) failed: %s\n", SDL_GetError());
        return -1;
    }
    int btw = 0, bth = 0;
    if (SDL_QueryTexture(s->baddiesTex, NULL, NULL, &btw, &bth) != 0) {
        fprintf(stderr, "SDL_QueryTexture baddies.jpg failed: %s\n", SDL_GetError());
        return -1;
    }
    // Assuming baddies.jpg is 8 columns x 6 rows (based on description: slimes, golems, imps, knights, goblins, bats)
    s->baddieTileW = btw / 8;
    s->baddieTileH = bth / 6;
    printf("Loaded baddies.jpg: %dx%d, tile %dx%d\n", btw, bth, s->baddieTileW, s->baddieTileH);
    fflush(stdout);

    // Load backgrounds (single file kong-bgs.jpg with 4 backgrounds in 2x2 grid)
    s->bgTex = IMG_LoadTexture(renderer, "kong-bgs.jpg");
    if (!s->bgTex) {
        fprintf(stderr, "Warning: Could not load kong-bgs.jpg: %s\n", IMG_GetError());
        s->bgTex = NULL; // Background is optional
    } else {
        int bgw = 0, bgh = 0;
        if (SDL_QueryTexture(s->bgTex, NULL, NULL, &bgw, &bgh) != 0) {
            fprintf(stderr, "SDL_QueryTexture kong-bgs.jpg failed: %s\n", IMG_GetError());
            SDL_DestroyTexture(s->bgTex);
            s->bgTex = NULL;
        } else {
            // Assume 2x2 grid layout (4 backgrounds)
            s->bgTileW = bgw / 2;
            s->bgTileH = bgh / 2;
            printf("Loaded kong-bgs.jpg: %dx%d, each bg %dx%d\n", bgw, bgh, s->bgTileW, s->bgTileH);
            fflush(stdout);
        }
    }

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

    // Vary level layout based on level number (1-25)
    int levelPattern = (g->level - 1) % 5; // Cycle through 5 different patterns
    
    // Ground (always the same)
    g->platforms[g->numPlatforms++].rect = (RectF){ 0, WINDOW_H - 40, WINDOW_W, 40 };
    
    // Different platform patterns
    switch (levelPattern) {
        case 0: // Original zigzag
            g->platforms[g->numPlatforms++].rect = (RectF){ 40, WINDOW_H - 140, WINDOW_W - 80, 20 };
            g->platforms[g->numPlatforms++].rect = (RectF){ 80, WINDOW_H - 240, WINDOW_W - 160, 20 };
            g->platforms[g->numPlatforms++].rect = (RectF){ 40, WINDOW_H - 340, WINDOW_W - 80, 20 };
            g->platforms[g->numPlatforms++].rect = (RectF){ 120, WINDOW_H - 440, WINDOW_W - 240, 20 };
            g->ladders[g->numLadders++].rect = (RectF){ 80, WINDOW_H - 140, 20, 120 };
            g->ladders[g->numLadders++].rect = (RectF){ WINDOW_W - 120, WINDOW_H - 240, 20, 120 };
            g->ladders[g->numLadders++].rect = (RectF){ 140, WINDOW_H - 340, 20, 120 };
            g->ladders[g->numLadders++].rect = (RectF){ WINDOW_W / 2 - 10, WINDOW_H - 440, 20, 120 };
            break;
        case 1: // Left-heavy
            g->platforms[g->numPlatforms++].rect = (RectF){ 0, WINDOW_H - 140, WINDOW_W * 0.6f, 20 };
            g->platforms[g->numPlatforms++].rect = (RectF){ WINDOW_W * 0.4f, WINDOW_H - 240, WINDOW_W * 0.5f, 20 };
            g->platforms[g->numPlatforms++].rect = (RectF){ 0, WINDOW_H - 340, WINDOW_W * 0.55f, 20 };
            g->platforms[g->numPlatforms++].rect = (RectF){ WINDOW_W * 0.45f, WINDOW_H - 440, WINDOW_W * 0.4f, 20 };
            g->ladders[g->numLadders++].rect = (RectF){ WINDOW_W * 0.25f, WINDOW_H - 140, 20, 120 };
            g->ladders[g->numLadders++].rect = (RectF){ WINDOW_W * 0.65f, WINDOW_H - 240, 20, 120 };
            g->ladders[g->numLadders++].rect = (RectF){ WINDOW_W * 0.3f, WINDOW_H - 340, 20, 120 };
            g->ladders[g->numLadders++].rect = (RectF){ WINDOW_W * 0.65f, WINDOW_H - 440, 20, 120 };
            break;
        case 2: // Right-heavy
            g->platforms[g->numPlatforms++].rect = (RectF){ WINDOW_W * 0.4f, WINDOW_H - 140, WINDOW_W * 0.6f, 20 };
            g->platforms[g->numPlatforms++].rect = (RectF){ WINDOW_W * 0.1f, WINDOW_H - 240, WINDOW_W * 0.5f, 20 };
            g->platforms[g->numPlatforms++].rect = (RectF){ WINDOW_W * 0.45f, WINDOW_H - 340, WINDOW_W * 0.55f, 20 };
            g->platforms[g->numPlatforms++].rect = (RectF){ WINDOW_W * 0.15f, WINDOW_H - 440, WINDOW_W * 0.4f, 20 };
            g->ladders[g->numLadders++].rect = (RectF){ WINDOW_W * 0.7f, WINDOW_H - 140, 20, 120 };
            g->ladders[g->numLadders++].rect = (RectF){ WINDOW_W * 0.35f, WINDOW_H - 240, 20, 120 };
            g->ladders[g->numLadders++].rect = (RectF){ WINDOW_W * 0.7f, WINDOW_H - 340, 20, 120 };
            g->ladders[g->numLadders++].rect = (RectF){ WINDOW_W * 0.35f, WINDOW_H - 440, 20, 120 };
            break;
        case 3: // Center-focused
            g->platforms[g->numPlatforms++].rect = (RectF){ WINDOW_W * 0.2f, WINDOW_H - 140, WINDOW_W * 0.6f, 20 };
            g->platforms[g->numPlatforms++].rect = (RectF){ WINDOW_W * 0.15f, WINDOW_H - 240, WINDOW_W * 0.7f, 20 };
            g->platforms[g->numPlatforms++].rect = (RectF){ WINDOW_W * 0.25f, WINDOW_H - 340, WINDOW_W * 0.5f, 20 };
            g->platforms[g->numPlatforms++].rect = (RectF){ WINDOW_W * 0.3f, WINDOW_H - 440, WINDOW_W * 0.4f, 20 };
            g->ladders[g->numLadders++].rect = (RectF){ WINDOW_W * 0.3f, WINDOW_H - 140, 20, 120 };
            g->ladders[g->numLadders++].rect = (RectF){ WINDOW_W * 0.7f, WINDOW_H - 240, 20, 120 };
            g->ladders[g->numLadders++].rect = (RectF){ WINDOW_W * 0.4f, WINDOW_H - 340, 20, 120 };
            g->ladders[g->numLadders++].rect = (RectF){ WINDOW_W * 0.6f, WINDOW_H - 440, 20, 120 };
            break;
        case 4: // Alternating sides
            g->platforms[g->numPlatforms++].rect = (RectF){ 0, WINDOW_H - 140, WINDOW_W * 0.45f, 20 };
            g->platforms[g->numPlatforms++].rect = (RectF){ WINDOW_W * 0.55f, WINDOW_H - 240, WINDOW_W * 0.45f, 20 };
            g->platforms[g->numPlatforms++].rect = (RectF){ 0, WINDOW_H - 340, WINDOW_W * 0.5f, 20 };
            g->platforms[g->numPlatforms++].rect = (RectF){ WINDOW_W * 0.5f, WINDOW_H - 440, WINDOW_W * 0.5f, 20 };
            g->ladders[g->numLadders++].rect = (RectF){ WINDOW_W * 0.2f, WINDOW_H - 140, 20, 120 };
            g->ladders[g->numLadders++].rect = (RectF){ WINDOW_W * 0.8f, WINDOW_H - 240, 20, 120 };
            g->ladders[g->numLadders++].rect = (RectF){ WINDOW_W * 0.25f, WINDOW_H - 340, 20, 120 };
            g->ladders[g->numLadders++].rect = (RectF){ WINDOW_W * 0.75f, WINDOW_H - 440, 20, 120 };
            break;
    }

    // Goal (princess) at top right of top platform
    if (g->numPlatforms > 1) {
        const RectF *topPlat = &g->platforms[g->numPlatforms - 1].rect;
        g->goal = (RectF){ topPlat->x + topPlat->w - 80, topPlat->y - 60, 80, 60 };
    } else {
        g->goal = (RectF){ WINDOW_W - 160, WINDOW_H - 480, 80, 60 };
    }

    // Clear barrels
    for (int i = 0; i < MAX_BARRELS; i++) {
        g->barrels[i].active = 0;
        g->barrels[i].broken = 0;
        g->barrels[i].brokenTime = 0.0f;
    }

    // Clear power-ups
    g->numPowerUps = 0;
    for (int i = 0; i < MAX_POWERUPS; i++) {
        g->powerUps[i].active = 0;
    }

    // Reset player power states
    g->player.hasSword = 0;
    g->player.hasFlame = 0;
    g->player.hasSuperBeast = 0;
    g->player.swordTimer = 0.0f;
    g->player.flameTimer = 0.0f;
    g->player.superBeastTimer = 0.0f;

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

    // Clear baddies
    g->numBaddies = 0;
    for (int i = 0; i < MAX_BADDIES; i++) {
        g->baddies[i].active = 0;
        g->baddies[i].dying = 0;
        g->baddies[i].deathTime = 0.0f;
    }
    g->baddieSpawnTimer = 0.0f;
    // Baddie spawn interval - spawn more frequently as level increases
    float baddieBaseInterval = 4.0f;
    float baddieMinInterval = 1.5f;
    float baddieInterval = baddieBaseInterval - 0.1f * (g->level - 1);
    if (baddieInterval < baddieMinInterval) baddieInterval = baddieMinInterval;
    g->baddieSpawnInterval = baddieInterval;
}

static void spawn_baddie(Game *g) {
    // Find top platform (highest y, smallest value)
    if (g->numPlatforms < 2) return; // Need at least ground + one platform
    
    int topPlatformIdx = g->numPlatforms - 1;
    for (int i = 0; i < g->numPlatforms; i++) {
        if (g->platforms[i].rect.y < g->platforms[topPlatformIdx].rect.y) {
            topPlatformIdx = i;
        }
    }
    
    for (int i = 0; i < MAX_BADDIES; i++) {
        if (!g->baddies[i].active) {
            g->baddies[i].active = 1;
            g->baddies[i].dying = 0;
            g->baddies[i].deathTime = 0.0f;
            g->baddies[i].rect.w = (float)WORLD_TILE;
            g->baddies[i].rect.h = (float)WORLD_TILE;
            
            // Spawn on top platform, random position
            const RectF *topPlat = &g->platforms[topPlatformIdx].rect;
            float spawnX = topPlat->x + (float)((i * 7 + g->level * 3) % (int)(topPlat->w - WORLD_TILE));
            g->baddies[i].rect.x = spawnX;
            g->baddies[i].rect.y = topPlat->y - g->baddies[i].rect.h;
            
            // Random baddie type (0-5): slimes, golems, imps, knights, goblins, bats
            g->baddies[i].type = (i * 11 + g->level * 5) % 6;
            g->baddies[i].facing = ((i + g->level) % 2 == 0) ? 1 : -1;
            g->baddies[i].vx = (g->baddies[i].facing == 1) ? 80.0f : -80.0f;
            g->baddies[i].vy = 0.0f;
            g->baddies[i].onGround = 0;
            g->baddies[i].animTime = 0.0f;
            
            if (g->numBaddies < MAX_BADDIES) g->numBaddies++;
            break;
        }
    }
}

static void spawn_barrel(Game *g) {
    for (int i = 0; i < MAX_BARRELS; i++) {
        if (!g->barrels[i].active) {
            g->barrels[i].active = 1;
            g->barrels[i].broken = 0;
            g->barrels[i].brokenTime = 0.0f;
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

// Place power-ups randomly on platforms
static void place_powerups(Game *g, const Sprites *s) {
    g->numPowerUps = 0;
    for (int i = 0; i < MAX_POWERUPS; i++) {
        g->powerUps[i].active = 0;
    }

    // Place 2-3 power-ups randomly on platforms (skip ground)
    int powerUpsToPlace = 2 + (g->level % 2); // 2 or 3 power-ups
    for (int p = 0; p < powerUpsToPlace && g->numPowerUps < MAX_POWERUPS; p++) {
        // Pick a random platform (skip ground)
        if (g->numPlatforms < 2) break;
        int platIdx = 1 + (p * 7 + g->level * 3) % (g->numPlatforms - 1);
        if (platIdx >= g->numPlatforms) platIdx = g->numPlatforms - 1;
        
        RectF *plat = &g->platforms[platIdx].rect;
        
        // Random position along platform
        float frac = 0.3f + (float)((p * 13 + g->level * 7) % 40) / 100.0f;
        float px = plat->x + plat->w * frac;
        float py = plat->y - (float)WORLD_TILE * 0.8f;
        
        // Random power-up type
        int types[] = { POWER_SWORD, POWER_FLAME, POWER_LIGHTNING, POWER_HEART };
        int typeIdx = (p * 11 + g->level * 5) % 4;
        int type = types[typeIdx];
        
        PowerUp *pu = &g->powerUps[g->numPowerUps++];
        pu->rect = (RectF){ px - (float)WORLD_TILE * 0.5f, py, (float)WORLD_TILE, (float)WORLD_TILE };
        pu->active = 1;
        pu->type = type;
        pu->animTime = 0.0f;
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
        
        // Update broken state
        if (b->broken) {
            b->brokenTime += dt;
            // Remove broken barrel after a short time
            if (b->brokenTime > 0.5f) {
                b->active = 0;
            }
            continue; // Don't update broken barrels
        }
        
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
        
        // Bounce off window edges
        if (b->rect.x < 0) {
            b->rect.x = 0;
            b->vx = -b->vx;
        } else if (b->rect.x + b->rect.w > WINDOW_W) {
            b->rect.x = WINDOW_W - b->rect.w;
            b->vx = -b->vx;
        }

        // World bounds (remove if out of bounds)
        if (b->rect.y > WINDOW_H + 100 || b->rect.x < -100 || b->rect.x > WINDOW_W + 100) {
            b->active = 0;
        }

        // Player collision -> lose life OR break barrel if hero has flame/lightning (auto-break)
        // OR if hero has sword and attacks (handled in update_game with attack parameter)
        if (!g->gameOver && !g->win && rects_intersect(&b->rect, &g->player.rect)) {
            if (g->player.hasFlame || g->player.hasSuperBeast) {
                // Auto-break barrels with flame or super beast mode
                b->broken = 1;
                b->brokenTime = 0.0f;
                b->vx = 0;
                b->vy = 0;
                g->score += 200; // bonus for breaking barrel
                // Check for extra life after score increase
                while (g->score >= g->nextLifeScore) {
                    g->lives++;
                    g->nextLifeScore += 5000;
                }
            } else if (!g->player.hasSword) {
                // Normal collision - lose life (only if not sword, sword breaking handled separately)
                g->lives--;
                if (g->lives <= 0) {
                    g->gameOver = 1;
                } else {
                    g->gameOver = 1; // death screen, restart with space
                }
            }
            // Note: Sword breaking is handled in update_game when attack key is pressed
        }
    }
}

static void update_baddies(Game *g, float dt) {
    for (int i = 0; i < MAX_BADDIES; i++) {
        if (!g->baddies[i].active) continue;
        Baddie *bad = &g->baddies[i];
        
        // Update dying state
        if (bad->dying) {
            bad->deathTime += dt;
            // Remove after death animation completes
            if (bad->deathTime > 0.5f) {
                bad->active = 0;
                if (g->numBaddies > 0) g->numBaddies--;
            }
            continue; // Don't update dying baddies
        }
        
        // Apply gravity
        bad->vy += g->gravity * dt;
        
        // Vertical movement and platform collision
        bad->rect.y += bad->vy * dt;
        bad->onGround = 0;
        for (int p = 0; p < g->numPlatforms; p++) {
            RectF *plat = &g->platforms[p].rect;
            if (rects_intersect(&bad->rect, plat)) {
                if (bad->vy > 0) {
                    // Landing on platform
                    bad->rect.y = plat->y - bad->rect.h;
                    bad->vy = 0;
                    bad->onGround = 1;
                } else if (bad->vy < 0) {
                    // Hitting head
                    bad->rect.y = plat->y + plat->h;
                    bad->vy = 0;
                }
            }
        }
        
        // Simple AI: move toward player direction (or random if far away)
        // But prioritize moving down - they can't climb up
        float playerCenterX = g->player.rect.x + g->player.rect.w * 0.5f;
        float baddieCenterX = bad->rect.x + bad->rect.w * 0.5f;
        float dx = playerCenterX - baddieCenterX;
        
        // Move toward player horizontally, but can reverse if hitting wall
        if (fabsf(dx) > 20.0f) {
            if (dx > 0) {
                bad->facing = 1;
            } else {
                bad->facing = -1;
            }
        }
        
        float moveSpeed = 80.0f;
        bad->vx = bad->facing * moveSpeed;
        
        // Horizontal movement
        bad->rect.x += bad->vx * dt;
        
        // Check for platform edge or wall, reverse direction
        int hitWall = 0;
        if (bad->rect.x < 0) {
            bad->rect.x = 0;
            hitWall = 1;
        } else if (bad->rect.x + bad->rect.w > WINDOW_W) {
            bad->rect.x = WINDOW_W - bad->rect.w;
            hitWall = 1;
        }
        
        // Check if at platform edge
        if (bad->onGround && !hitWall) {
            float checkX = (bad->facing > 0) ? bad->rect.x + bad->rect.w : bad->rect.x;
            float checkY = bad->rect.y + bad->rect.h + 1.0f;
            int onPlatform = 0;
            for (int p = 0; p < g->numPlatforms; p++) {
                RectF *plat = &g->platforms[p].rect;
                if (checkX >= plat->x && checkX <= plat->x + plat->w &&
                    checkY >= plat->y && checkY <= plat->y + plat->h) {
                    onPlatform = 1;
                    break;
                }
            }
            if (!onPlatform) {
                // At edge - try to jump down (can only jump half hero height)
                if (bad->vy >= 0 && bad->onGround) {
                    bad->vy = -g->jumpSpeed * 0.5f; // Half hero jump height
                    bad->onGround = 0;
                } else {
                    // Or reverse direction
                    bad->facing = -bad->facing;
                }
            }
        }
        
        if (hitWall) {
            bad->facing = -bad->facing;
        }
        
        // Update walk animation
        if (bad->onGround && fabsf(bad->vx) > 1.0f) {
            bad->animTime += dt;
        }
        
        // Remove if fell off bottom
        if (bad->rect.y > WINDOW_H + 100) {
            bad->active = 0;
            if (g->numBaddies > 0) g->numBaddies--;
        }
        
        // Player collision - check for kill conditions
        if (!g->gameOver && !g->win && rects_intersect(&bad->rect, &g->player.rect)) {
            int killed = 0;
            
            // Check if player is jumping on top of baddie
            float playerBottom = g->player.rect.y + g->player.rect.h;
            float baddieTop = bad->rect.y;
            if (g->player.vy >= 0 && playerBottom <= baddieTop + 10.0f && g->player.rect.y < bad->rect.y) {
                // Player is above and falling/landing on top - jump on top kills
                killed = 1;
                // Give player a small bounce
                g->player.vy = -g->jumpSpeed * 0.3f;
            }
            
            // Check if player has flame or super beast (auto-kill on contact)
            if (!killed && (g->player.hasFlame || g->player.hasSuperBeast)) {
                killed = 1;
            }
            
            // Sword attack kills (handled in update_game, but we check here for collision)
            // This will be handled separately in update_game when attack is pressed
            
            if (killed) {
                bad->dying = 1;
                bad->deathTime = 0.0f;
                bad->vx = 0;
                bad->vy = 0;
                g->score += 100; // Bonus for killing baddie
                // Check for extra life after score increase
                while (g->score >= g->nextLifeScore) {
                    g->lives++;
                    g->nextLifeScore += 5000;
                }
            } else {
                // Baddie hit player - lose life
                g->lives--;
                if (g->lives <= 0) {
                    g->gameOver = 1;
                } else {
                    g->gameOver = 1; // death screen, restart with space
                }
            }
        }
    }
}

static void update_game(Game *g, float dt, int moveLeft, int moveRight, int jump, int climbUp, int climbDown, int attack) {
    // Global timers for animation always tick
    g->time += dt;
    g->princess_t += dt;

    if (!g->running || g->gameOver || g->win || g->paused) return;

    Player *p = &g->player;

    // Update power-up timers and expire them
    if (p->swordTimer > 0.0f) {
        p->swordTimer -= dt;
        if (p->swordTimer <= 0.0f) {
            p->hasSword = 0;
            p->swordTimer = 0.0f;
        }
    }
    if (p->flameTimer > 0.0f) {
        p->flameTimer -= dt;
        if (p->flameTimer <= 0.0f) {
            p->hasFlame = 0;
            p->flameTimer = 0.0f;
        }
    }
    if (p->superBeastTimer > 0.0f) {
        p->superBeastTimer -= dt;
        if (p->superBeastTimer <= 0.0f) {
            p->hasSuperBeast = 0;
            p->superBeastTimer = 0.0f;
        }
    }

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

    // Sword attack - break nearby barrels and kill baddies when 'G' is pressed
    if (attack && p->hasSword) {
        float attackRange = 40.0f; // attack range
        // Check all barrels and break ones close to player
        for (int i = 0; i < MAX_BARRELS; i++) {
            Barrel *b = &g->barrels[i];
            if (!b->active || b->broken) continue;
            
            // Check if barrel is in attack range (horizontal and vertical proximity)
            float dx = (b->rect.x + b->rect.w * 0.5f) - (p->rect.x + p->rect.w * 0.5f);
            float dy = (b->rect.y + b->rect.h * 0.5f) - (p->rect.y + p->rect.h * 0.5f);
            float distSq = dx * dx + dy * dy;
            
            if (distSq < attackRange * attackRange) {
                b->broken = 1;
                b->brokenTime = 0.0f;
                b->vx = 0;
                b->vy = 0;
                g->score += 200; // bonus for breaking barrel
                // Check for extra life after score increase
                while (g->score >= g->nextLifeScore) {
                    g->lives++;
                    g->nextLifeScore += 5000;
                }
            }
        }
        
        // Check all baddies and kill ones close to player
        for (int i = 0; i < MAX_BADDIES; i++) {
            Baddie *bad = &g->baddies[i];
            if (!bad->active || bad->dying) continue;
            
            // Check if baddie is in attack range
            float dx = (bad->rect.x + bad->rect.w * 0.5f) - (p->rect.x + p->rect.w * 0.5f);
            float dy = (bad->rect.y + bad->rect.h * 0.5f) - (p->rect.y + p->rect.h * 0.5f);
            float distSq = dx * dx + dy * dy;
            
            if (distSq < attackRange * attackRange) {
                bad->dying = 1;
                bad->deathTime = 0.0f;
                bad->vx = 0;
                bad->vy = 0;
                g->score += 100; // bonus for killing baddie
                // Check for extra life after score increase
                while (g->score >= g->nextLifeScore) {
                    g->lives++;
                    g->nextLifeScore += 5000;
                }
            }
        }
    }

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
        // Check for extra life after score increase
        while (g->score >= g->nextLifeScore) {
            g->lives++;
            g->nextLifeScore += 5000;
        }
    }

    // Barrels
    g->barrelSpawnTimer += dt;
    if (g->barrelSpawnTimer >= g->barrelSpawnInterval) {
        g->barrelSpawnTimer = 0.0f;
        spawn_barrel(g);
    }
    update_barrels(g, dt);

    // Baddies
    g->baddieSpawnTimer += dt;
    if (g->baddieSpawnTimer >= g->baddieSpawnInterval) {
        g->baddieSpawnTimer = 0.0f;
        spawn_baddie(g);
    }
    update_baddies(g, dt);

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

    // Power-up collection
    for (int i = 0; i < g->numPowerUps; i++) {
        PowerUp *pu = &g->powerUps[i];
        if (!pu->active) continue;
        if (rects_intersect(&p->rect, &pu->rect)) {
            pu->active = 0;
            
            switch (pu->type) {
                case POWER_SWORD:
                    g->player.hasSword = 1;
                    g->player.swordTimer = 30.0f; // 30 seconds
                    break;
                case POWER_FLAME:
                    g->player.hasFlame = 1;
                    g->player.flameTimer = 30.0f; // 30 seconds
                    break;
                case POWER_LIGHTNING:
                    g->player.hasSuperBeast = 1;
                    g->player.superBeastTimer = 30.0f; // 30 seconds
                    break;
                case POWER_HEART:
                    g->lives++;
                    break;
            }
        }
        // Animate power-ups
        pu->animTime += dt;
    }
}

static void render_rect(SDL_Renderer *r, const RectF *rect, Uint8 cr, Uint8 cg, Uint8 cb, Uint8 ca) {
    SDL_Rect sr = { (int)rect->x, (int)rect->y, (int)rect->w, (int)rect->h };
    SDL_SetRenderDrawColor(r, cr, cg, cb, ca);
    SDL_RenderFillRect(r, &sr);
}

// Draw text string using bitmap font, returns x position after text
static int draw_text(SDL_Renderer *renderer, int x, int y, int scale, const char *text, Uint8 r, Uint8 g, Uint8 b) {
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
    
    // Simple letter patterns (3x5 grid like digits)
    static const char *letters[26] = {
        "111101101101111", // A (approximate)
        "111101111100111", // B
        "111100100100111", // C
        "111101101101110", // D
        "111100111100111", // E
        "111100111100100", // F
        "111100101101111", // G
        "101101111101101", // H
        "111010010010111", // I
        "111001001001111", // J
        "101101110101101", // K
        "100100100100111", // L
        "101111111101101", // M
        "101111111111101", // N
        "111101101101111", // O
        "111101111100100", // P
        "111101101111111", // Q
        "111101111110101", // R
        "111100111001111", // S
        "111010010010010", // T
        "101101101101111", // U
        "101101101010010", // V
        "101101111111101", // W
        "101101010101101", // X
        "101101010010010", // Y
        "111001010100111"  // Z
    };
    
    int digitX = x;
    SDL_SetRenderDrawColor(renderer, r, g, b, 255);
    
    for (const char *p = text; *p; ++p) {
        const char *pat = NULL;
        if (*p >= '0' && *p <= '9') {
            pat = digits[*p - '0'];
        } else if (*p >= 'A' && *p <= 'Z') {
            pat = letters[*p - 'A'];
        } else if (*p >= 'a' && *p <= 'z') {
            pat = letters[*p - 'a'];
        } else if (*p == ' ') {
            digitX += 3 * scale + scale; // space
            continue;
        } else {
            continue; // skip unknown chars
        }
        
        if (pat) {
            for (int row = 0; row < 5; row++) {
                for (int col = 0; col < 3; col++) {
                    if (pat[row * 3 + col] == '1') {
                        SDL_Rect rect = {
                            digitX + col * scale,
                            y + row * scale,
                            scale, scale
                        };
                        SDL_RenderFillRect(renderer, &rect);
                    }
                }
            }
            digitX += 3 * scale + scale; // advance to next char
        }
    }
    return digitX;
}

static void render_sprite(SDL_Renderer *r, SDL_Texture *tex, const SDL_Rect *src, const RectF *dst, SDL_RendererFlip flip) {
    SDL_Rect d = { (int)dst->x, (int)dst->y, (int)dst->w, (int)dst->h };
    SDL_RenderCopyEx(r, tex, src, &d, 0.0, NULL, flip);
}

// Helper to draw positive integer; returns x after last digit
static int drawNumberAt(SDL_Renderer *renderer, int x, int value, int hudY, int scale, const char *digits[10]) {
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

static void render_game(SDL_Renderer *renderer, const Game *g, const Sprites *s) {
    // Render background based on level pattern
    int levelPattern = (g->level - 1) % 5;
    int bgIndex = levelPattern % 4; // Use first 4 patterns for backgrounds (2x2 grid)
    if (s->bgTex) {
        // Extract the correct background from the 2x2 grid
        // bgIndex: 0 = top-left, 1 = top-right, 2 = bottom-left, 3 = bottom-right
        int bgCol = bgIndex % 2;
        int bgRow = bgIndex / 2;
        SDL_Rect srcRect = {
            bgCol * s->bgTileW,
            bgRow * s->bgTileH,
            s->bgTileW,
            s->bgTileH
        };
        SDL_Rect dstRect = { 0, 0, WINDOW_W, WINDOW_H };
        SDL_RenderCopy(renderer, s->bgTex, &srcRect, &dstRect);
    } else {
        // Fallback to solid color if background not loaded
        SDL_SetRenderDrawColor(renderer, 10, 10, 20, 255);
        SDL_RenderClear(renderer);
    }

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

        int x = 10;
        // Level
        x = drawNumberAt(renderer, x, g->level, hudY, scale, digits) + 10;
        // Score
        x = drawNumberAt(renderer, x, g->score, hudY, scale, digits) + 10;
        // Lives (just draw as a number for now)
        drawNumberAt(renderer, x, g->lives, hudY, scale, digits);
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

    // Power-ups
    for (int i = 0; i < g->numPowerUps; i++) {
        const PowerUp *pu = &g->powerUps[i];
        if (!pu->active) continue;
        
        const SDL_Rect *src = NULL;
        switch (pu->type) {
            case POWER_SWORD:
                src = &s->powerSword;
                break;
            case POWER_FLAME:
                src = &s->powerFlame;
                break;
            case POWER_LIGHTNING:
                src = &s->powerLightning;
                break;
            case POWER_HEART:
                src = &s->powerHeart;
                break;
        }
        
        if (src) {
            // Animate with floating effect
            RectF animRect = pu->rect;
            animRect.y += sinf(pu->animTime * 3.0f) * 3.0f;
            // Ensure heart sprite is rendered at proper aspect ratio (not squished)
            if (pu->type == POWER_HEART) {
                // Heart should maintain square aspect ratio
                float size = animRect.w > animRect.h ? animRect.w : animRect.h;
                animRect.w = size;
                animRect.h = size;
            }
            render_sprite(renderer, s->powerTex, src, &animRect, SDL_FLIP_NONE);
        }
    }

    // Barrels
    for (int i = 0; i < MAX_BARRELS; i++) {
        if (!g->barrels[i].active) continue;
        const SDL_Rect *barrelSrc = g->barrels[i].broken ? &s->barrelBroken : &s->barrel;
        render_sprite(renderer, s->tex, barrelSrc, &g->barrels[i].rect, SDL_FLIP_NONE);
    }

    // Baddies
    for (int i = 0; i < MAX_BADDIES; i++) {
        const Baddie *bad = &g->baddies[i];
        if (!bad->active) continue;
        
        // Get sprite from baddies sheet based on type (row) and animation frame
        SDL_Rect baddieSrc;
        if (bad->dying) {
            // Death animation: use first frame, we'll rotate it
            baddieSrc = sprite_tile_custom(s->baddieTileW, s->baddieTileH, 0, bad->type);
        } else {
            // Walk animation: use first 5 frames (columns 0-4)
            int walkFrame = ((int)(bad->animTime * 8.0f)) % 5;
            baddieSrc = sprite_tile_custom(s->baddieTileW, s->baddieTileH, walkFrame, bad->type);
        }
        
        RectF renderRect = bad->rect;
        double angle = 0.0;
        
        if (bad->dying) {
            // Death animation: rotate 90 degrees clockwise and squish width to 0
            float deathProgress = bad->deathTime / 0.5f; // 0 to 1 over 0.5 seconds
            if (deathProgress > 1.0f) deathProgress = 1.0f;
            
            angle = 90.0 * deathProgress; // Rotate 90 degrees clockwise
            renderRect.w *= (1.0f - deathProgress); // Squish width to 0
        }
        
        SDL_RendererFlip flip = (bad->facing < 0) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
        SDL_Rect d = { (int)renderRect.x, (int)renderRect.y, (int)renderRect.w, (int)renderRect.h };
        SDL_RenderCopyEx(renderer, s->baddiesTex, &baddieSrc, &d, angle, NULL, flip);
    }

    // Player: choose frame and flip based on movement and power state
    SDL_Texture *heroTexture = s->tex; // Default to kong.png
    int heroRow = 0; // Default row
    int heroCol = 0; // Column based on animation
    
    // Determine which sprite sheet and row to use based on power state
    if (g->player.hasSword && g->player.hasSuperBeast) {
        // Sword + Super Beast = hero row 2
        heroTexture = s->heroTex;
        heroRow = 2;
    } else if (g->player.hasSuperBeast) {
        // Super Beast only = hero row 1
        heroTexture = s->heroTex;
        heroRow = 1;
    } else if (g->player.hasFlame) {
        // Flame = hero row 3
        heroTexture = s->heroTex;
        heroRow = 3;
    } else if (g->player.hasSword) {
        // Sword only = hero row 0
        heroTexture = s->heroTex;
        heroRow = 0;
    }
    
    // Determine animation frame
    if (player_on_ladder(g)) {
        heroCol = 5; // Climbing
    } else if (!g->player.onGround) {
        heroCol = 4; // Jumping
    } else {
        // Running animation when moving horizontally on ground
        if (fabsf(g->player.vx) > 1.0f) {
            int idx = ((int)(g->player.runAnim * 10.0f)) % 3;
            heroCol = idx; // 0, 1, or 2 for run frames
        } else {
            heroCol = 0; // Idle
        }
    }
    
    // Get sprite from appropriate texture
    SDL_Rect playerSrc;
    if (heroTexture == s->heroTex) {
        // Use hero.png
        playerSrc = sprite_tile_custom(s->heroTileW, s->heroTileH, heroCol, heroRow);
    } else {
        // Use kong.png (original sprites)
        playerSrc = sprite_tile(s, heroCol, heroRow);
    }
    
    SDL_RendererFlip heroFlip = (g->player.facing < 0) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
    // Adjust player visual position to align with floor sprite's walkable surface
    RectF playerRenderRect = g->player.rect;
    if (g->player.onGround) {
        // Check if player is on ground floor (platforms[0]) or regular platform
        // Ground floor uses floor sprite with more offset, platforms use less
        int onGroundFloor = 0;
        if (g->numPlatforms > 0) {
            const RectF *groundPlat = &g->platforms[0].rect;
            float playerBottom = g->player.rect.y + g->player.rect.h;
            // Check if player is standing on the ground platform
            if (fabsf(playerBottom - groundPlat->y) < 2.0f) {
                onGroundFloor = 1;
            }
        }
        float offset = onGroundFloor ? 24.0f : 8.0f;
        playerRenderRect.y += offset;
    }
    render_sprite(renderer, heroTexture, &playerSrc, &playerRenderRect, heroFlip);

    // Overlay win / game over + score
    if (g->win || g->gameOver) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 160);
        SDL_Rect full = { 0, 0, WINDOW_W, WINDOW_H };
        SDL_RenderFillRect(renderer, &full);

        // Box background color
        RectF box = { WINDOW_W / 2 - 160, WINDOW_H / 2 - 100, 320, 200 };
        if (g->win) {
            render_rect(renderer, &box, 40, 200, 80, 255);
        } else if (g->gameOver) {
            render_rect(renderer, &box, 200, 40, 40, 255);
        }

        // Draw title text at top of box
        int titleX = (int)box.x + (int)(box.w / 2);
        int titleY = (int)box.y + 25;
        int titleScale = 5;
        if (g->win) {
            if (g->level >= 25) {
                // Center "BEAT THE GAME" text
                const char *text = "BEAT THE GAME";
                int textWidth = (int)strlen(text) * (3 * titleScale + titleScale);
                titleX = (int)box.x + (int)((box.w - textWidth) / 2);
                draw_text(renderer, titleX, titleY, titleScale, text, 255, 255, 0);
            } else {
                // Center "YOU WIN" text
                const char *text = "YOU WIN";
                int textWidth = (int)strlen(text) * (3 * titleScale + titleScale);
                titleX = (int)box.x + (int)((box.w - textWidth) / 2);
                draw_text(renderer, titleX, titleY, titleScale, text, 255, 255, 255);
            }
        } else if (g->gameOver) {
            // Center "GAME OVER" text
            const char *text = "GAME OVER";
            int textWidth = (int)strlen(text) * (3 * titleScale + titleScale);
            titleX = (int)box.x + (int)((box.w - textWidth) / 2);
            draw_text(renderer, titleX, titleY, titleScale, text, 255, 255, 255);
        }

        // Draw final score inside the box using a tiny bitmap font
        int score = g->score;
        if (score < 0) score = 0;
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", score);

        // Draw score label and value
        int startX = (int)box.x + 20;
        int startY = (int)box.y + 70;

        // Draw "SCORE:" label
        draw_text(renderer, startX, startY, 3, "SCORE:", 220, 220, 220);
        
        // Draw numeric score
        int digitX = startX;
        int digitY = startY + 25;
        int scale = 4; // size of "pixels" in the bitmap font
        SDL_SetRenderDrawColor(renderer, 10, 10, 10, 255);
        for (const char *p = buf; *p; ++p) {
            if (*p < '0' || *p > '9') continue;
            int d = *p - '0';
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
            const char *pat = digits[d];
            for (int row = 0; row < 5; row++) {
                for (int col = 0; col < 3; col++) {
                    if (pat[row * 3 + col] == '1') {
                        SDL_Rect r = {
                            digitX + col * scale,
                            digitY + row * scale,
                            scale, scale
                        };
                        SDL_RenderFillRect(renderer, &r);
                    }
                }
            }
            digitX += 3 * scale + scale; // advance to next digit
        }
        
        // Draw "SPACE TO CONTINUE" or similar message
        int msgY = (int)box.y + (int)box.h - 30;
        const char *msg = g->gameOver ? (g->lives > 0 ? "SPACE TO RETRY" : "SPACE TO RESTART") : "SPACE TO CONTINUE";
        int msgWidth = (int)strlen(msg) * (3 * 3 + 3);
        int msgX = (int)box.x + (int)((box.w - msgWidth) / 2);
        draw_text(renderer, msgX, msgY, 3, msg, 200, 200, 200);

        // Princess in the scoreboard popup, jumping/cheering (only on win)
        if (g->win) {
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

    int imgFlags = IMG_INIT_PNG | IMG_INIT_JPG;
    int initialized = IMG_Init(imgFlags);
    if ((initialized & IMG_INIT_PNG) == 0) {
        fprintf(stderr, "IMG_Init PNG failed: %s\n", IMG_GetError());
        SDL_Quit();
        return 1;
    }
    if ((initialized & IMG_INIT_JPG) == 0) {
        fprintf(stderr, "Warning: IMG_Init JPG failed: %s\n", IMG_GetError());
        // Continue anyway, PNG might still work
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
    // Now that we know tile sizes, place coins and power-ups relative to platforms
    place_coins(&game, &sprites);
    place_powerups(&game, &sprites);

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
                    place_powerups(&game, &sprites);
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
                        place_powerups(&game, &sprites);
                    } else if (game.gameOver) {
                        // From death screen: restart current level while lives remain,
                        // otherwise full reset.
                        if (game.lives > 0) {
                            game.gameOver = 0;
                            init_level(&game);
                            place_coins(&game, &sprites);
                            place_powerups(&game, &sprites);
                        } else {
                            game.level = 1;
                            game.score = 0;
                            game.lives = 3;
                            game.nextLifeScore = 5000;
                            game.gameOver = 0;
                            init_level(&game);
                            place_coins(&game, &sprites);
                            place_powerups(&game, &sprites);
                        }
                    }
                }
            }
        }

        const Uint8 *keys = SDL_GetKeyboardState(NULL);
        // Vim-style controls: h=left, j=jump, k=climb, l=right, g=attack
        // Also keep arrow keys and space for compatibility
        moveLeft  = keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_H];
        moveRight = keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_L];
        jump      = keys[SDL_SCANCODE_SPACE] || keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP] || keys[SDL_SCANCODE_J];
        climbUp   = keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP] || keys[SDL_SCANCODE_K];
        climbDown = keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN];

        Uint32 now = SDL_GetTicks();
        float dt = (now - lastTicks) / 1000.0f;
        if (dt > 0.05f) dt = 0.05f; // clamp big hitches
        lastTicks = now;

        // Fixed-ish timestep accumulation
        static float acc = 0.0f;
        acc += dt;
        while (acc >= targetDelta) {
            int attack = keys[SDL_SCANCODE_G] || (keys[SDL_SCANCODE_A] && !moveLeft && !moveRight); // G key for attack (vim-style)
            update_game(&game, targetDelta, moveLeft, moveRight, jump, climbUp, climbDown, attack);
            acc -= targetDelta;
        }

        render_game(renderer, &game, &sprites);

        // small delay to avoid 100% CPU when vsync is off
        SDL_Delay(1);
    }

    SDL_DestroyTexture(sprites.tex);
    if (sprites.heroTex) SDL_DestroyTexture(sprites.heroTex);
    if (sprites.powerTex) SDL_DestroyTexture(sprites.powerTex);
    if (sprites.baddiesTex) SDL_DestroyTexture(sprites.baddiesTex);
    if (sprites.bgTex) SDL_DestroyTexture(sprites.bgTex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    return 0;
}


