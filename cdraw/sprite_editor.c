//
//  sprite_editor.c
//  Simple sprite editor for creating 6x6 grid sprites (32x32 per cell)
//
//  Build: gcc sprite_editor.c -o sprite_editor $(sdl2-config --cflags --libs) -lSDL2_image -lm
//

#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define GRID_COLS 6
#define GRID_ROWS 6
#define CELL_SIZE 32
#define WINDOW_W (GRID_COLS * CELL_SIZE + 200)  // Extra space for UI
#define WINDOW_H (GRID_ROWS * CELL_SIZE + 100)

#define ZOOM_LEVELS 8
static int zoom_level = 1;  // Start at 2x zoom (index 1 = 2x)
static int zoom_sizes[] = {1, 2, 4, 8, 16, 32, 64, 80};  // 100% to 8000% (1x to 80x)

// Color palette (RGB)
static uint32_t palette[] = {
    0x000000,  // Black
    0xFFFFFF,  // White
    0xFF0000,  // Red
    0x00FF00,  // Green
    0x0000FF,  // Blue
    0xFFFF00,  // Yellow
    0xFF00FF,  // Magenta
    0x00FFFF,  // Cyan
    0x808080,  // Gray
    0xFF8080,  // Light Red
    0x80FF80,  // Light Green
    0x8080FF,  // Light Blue
    0xFFA500,  // Orange
    0x800080,  // Purple
    0x8B4513,  // Brown
    0xFFC0CB,  // Pink
};

#define PALETTE_SIZE (sizeof(palette) / sizeof(palette[0]))

// Transparent pixel marker (RGBA: 0x00000000)
#define TRANSPARENT_PIXEL 0x00000000

// Sprite data: 6x6 grid, each cell is 32x32 pixels (RGBA format)
static uint32_t sprite_data[GRID_ROWS][GRID_COLS][CELL_SIZE][CELL_SIZE];
static int current_color = 0;
static int current_cell_x = 0;
static int current_cell_y = 0;
static int drawing = 0;
static int show_grid = 1;
static int eraser_mode = 0;  // 1 = eraser, 0 = draw

// Convert RGBA to SDL color
static SDL_Color rgba_to_color(uint32_t rgba) {
    SDL_Color c;
    c.r = (rgba >> 24) & 0xFF;
    c.g = (rgba >> 16) & 0xFF;
    c.b = (rgba >> 8) & 0xFF;
    c.a = rgba & 0xFF;
    return c;
}

// Check if pixel is transparent
static int is_transparent(uint32_t rgba) {
    return (rgba & 0xFF) == 0;  // Alpha channel is 0
}

// Convert RGB palette color to RGBA (with alpha = 255)
static uint32_t rgb_to_rgba(uint32_t rgb) {
    return (rgb << 8) | 0xFF;  // Add alpha channel
}

// Get pixel at cell position
static uint32_t* get_pixel(int cell_x, int cell_y, int px, int py) {
    if (cell_x < 0 || cell_x >= GRID_COLS || cell_y < 0 || cell_y >= GRID_ROWS) {
        return NULL;
    }
    if (px < 0 || px >= CELL_SIZE || py < 0 || py >= CELL_SIZE) {
        return NULL;
    }
    return &sprite_data[cell_y][cell_x][py][px];
}

// Draw a pixel in a cell
static void draw_pixel(int cell_x, int cell_y, int px, int py, uint32_t color) {
    uint32_t *p = get_pixel(cell_x, cell_y, px, py);
    if (p) {
        *p = color;
    }
}

// Clear a cell (set all pixels to transparent)
static void clear_cell(int cell_x, int cell_y) {
    for (int y = 0; y < CELL_SIZE; y++) {
        for (int x = 0; x < CELL_SIZE; x++) {
            sprite_data[cell_y][cell_x][y][x] = TRANSPARENT_PIXEL;
        }
    }
}

// Clear all cells
static void clear_all(void) {
    for (int cy = 0; cy < GRID_ROWS; cy++) {
        for (int cx = 0; cx < GRID_COLS; cx++) {
            clear_cell(cx, cy);
        }
    }
}

// Save sprite to PNG with transparency
static void save_sprite(const char *filename) {
    int total_w = GRID_COLS * CELL_SIZE;
    int total_h = GRID_ROWS * CELL_SIZE;
    
    // Create SDL surface with RGBA format
    SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0, total_w, total_h, 32, SDL_PIXELFORMAT_RGBA8888);
    if (!surface) {
        printf("Error: Could not create surface: %s\n", SDL_GetError());
        return;
    }
    
    // Lock surface for pixel access
    SDL_LockSurface(surface);
    uint32_t *pixels = (uint32_t *)surface->pixels;
    
    // Copy sprite data to surface (in RGBA8888 format: R=bits 24-31, G=16-23, B=8-15, A=0-7)
    for (int cy = 0; cy < GRID_ROWS; cy++) {
        for (int py = 0; py < CELL_SIZE; py++) {
            for (int cx = 0; cx < GRID_COLS; cx++) {
                for (int px = 0; px < CELL_SIZE; px++) {
                    int x = cx * CELL_SIZE + px;
                    int y = cy * CELL_SIZE + py;
                    pixels[y * total_w + x] = sprite_data[cy][cx][py][px];
                }
            }
        }
    }
    
    SDL_UnlockSurface(surface);
    
    // Save as PNG
    if (IMG_SavePNG(surface, filename) != 0) {
        printf("Error: Could not save PNG: %s\n", IMG_GetError());
    } else {
        printf("Saved sprite to %s (PNG with transparency)\n", filename);
    }
    
    SDL_FreeSurface(surface);
}

// Load sprite from PNG with transparency
static void load_sprite(const char *filename) {
    SDL_Surface *surface = IMG_Load(filename);
    if (!surface) {
        printf("Error: Could not load image: %s\n", IMG_GetError());
        return;
    }
    
    // Convert to RGBA8888 format if needed
    SDL_Surface *converted = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA8888, 0);
    if (!converted) {
        printf("Error: Could not convert surface: %s\n", SDL_GetError());
        SDL_FreeSurface(surface);
        return;
    }
    
    int w = converted->w;
    int h = converted->h;
    
    if (w != GRID_COLS * CELL_SIZE || h != GRID_ROWS * CELL_SIZE) {
        printf("Warning: Image size %dx%d doesn't match expected %dx%d\n", 
               w, h, GRID_COLS * CELL_SIZE, GRID_ROWS * CELL_SIZE);
    }
    
    SDL_LockSurface(converted);
    uint32_t *pixels = (uint32_t *)converted->pixels;
    
    // Copy from surface to sprite data
    for (int cy = 0; cy < GRID_ROWS; cy++) {
        for (int py = 0; py < CELL_SIZE; py++) {
            for (int cx = 0; cx < GRID_COLS; cx++) {
                for (int px = 0; px < CELL_SIZE; px++) {
                    int x = cx * CELL_SIZE + px;
                    int y = cy * CELL_SIZE + py;
                    
                    if (x < w && y < h) {
                        sprite_data[cy][cx][py][px] = pixels[y * w + x];
                    } else {
                        sprite_data[cy][cx][py][px] = TRANSPARENT_PIXEL;
                    }
                }
            }
        }
    }
    
    SDL_UnlockSurface(converted);
    SDL_FreeSurface(converted);
    SDL_FreeSurface(surface);
    
    printf("Loaded sprite from %s\n", filename);
}

// Render the sprite editor
static void render(SDL_Renderer *renderer) {
    SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
    SDL_RenderClear(renderer);
    
    int zoom = zoom_sizes[zoom_level];
    int grid_w = GRID_COLS * CELL_SIZE * zoom;
    int grid_h = GRID_ROWS * CELL_SIZE * zoom;
    int offset_x = 10;
    int offset_y = 10;
    
    // Draw grid background
    SDL_Rect grid_rect = {offset_x, offset_y, grid_w, grid_h};
    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
    SDL_RenderFillRect(renderer, &grid_rect);
    
    // Draw cells
    for (int cy = 0; cy < GRID_ROWS; cy++) {
        for (int cx = 0; cx < GRID_COLS; cx++) {
            int cell_x = offset_x + cx * CELL_SIZE * zoom;
            int cell_y = offset_y + cy * CELL_SIZE * zoom;
            
            // Draw cell pixels with transparency support
            for (int py = 0; py < CELL_SIZE; py++) {
                for (int px = 0; px < CELL_SIZE; px++) {
                    uint32_t rgba = sprite_data[cy][cx][py][px];
                    
                    if (!is_transparent(rgba)) {
                        SDL_Color c = rgba_to_color(rgba);
                        SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
                        
                        SDL_Rect pixel = {
                            cell_x + px * zoom,
                            cell_y + py * zoom,
                            zoom,
                            zoom
                        };
                        SDL_RenderFillRect(renderer, &pixel);
                    } else {
                        // Draw checkerboard pattern for transparent pixels
                        int checker = ((px / 4) + (py / 4)) % 2;
                        SDL_SetRenderDrawColor(renderer, 
                                              checker ? 60 : 40, 
                                              checker ? 60 : 40, 
                                              checker ? 60 : 40, 255);
                        SDL_Rect pixel = {
                            cell_x + px * zoom,
                            cell_y + py * zoom,
                            zoom,
                            zoom
                        };
                        SDL_RenderFillRect(renderer, &pixel);
                    }
                }
            }
            
            // Draw cell border
            if (show_grid) {
                SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
                SDL_Rect border = {cell_x, cell_y, CELL_SIZE * zoom, CELL_SIZE * zoom};
                SDL_RenderDrawRect(renderer, &border);
            }
            
            // Highlight current cell
            if (cx == current_cell_x && cy == current_cell_y) {
                SDL_SetRenderDrawColor(renderer, 255, 255, 0, 200);
                SDL_Rect highlight = {cell_x - 2, cell_y - 2, 
                                      CELL_SIZE * zoom + 4, CELL_SIZE * zoom + 4};
                SDL_RenderDrawRect(renderer, &highlight);
            }
        }
    }
    
    // Draw UI panel on the right
    int ui_x = offset_x + grid_w + 20;
    int ui_y = offset_y;
    
    // Color palette
    SDL_Rect palette_rect = {ui_x, ui_y, 150, 240};
    SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
    SDL_RenderFillRect(renderer, &palette_rect);
    
    // Eraser indicator
    if (eraser_mode) {
        SDL_Rect eraser_indicator = {ui_x + 5, ui_y + 5, 140, 25};
        SDL_SetRenderDrawColor(renderer, 200, 100, 100, 255);
        SDL_RenderFillRect(renderer, &eraser_indicator);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &eraser_indicator);
    }
    
    // Palette swatches (offset down if eraser indicator is shown)
    int palette_start_y = eraser_mode ? ui_y + 35 : ui_y + 10;
    int palette_cols = 4;
    int palette_rows = (PALETTE_SIZE + palette_cols - 1) / palette_cols;
    int swatch_size = 30;
    
    for (int i = 0; i < PALETTE_SIZE; i++) {
        int px = ui_x + 10 + (i % palette_cols) * (swatch_size + 5);
        int py = palette_start_y + (i / palette_cols) * (swatch_size + 5);
        
        SDL_Color c;
        uint32_t rgba = rgb_to_rgba(palette[i]);
        c = rgba_to_color(rgba);
        SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
        SDL_Rect swatch = {px, py, swatch_size, swatch_size};
        SDL_RenderFillRect(renderer, &swatch);
        
        if (i == current_color && !eraser_mode) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDrawRect(renderer, &swatch);
        }
    }
}

// Handle mouse input
static void handle_mouse(SDL_MouseButtonEvent *e, int is_down) {
    int zoom = zoom_sizes[zoom_level];
    int offset_x = 10;
    int offset_y = 10;
    
    int mx = e->x;
    int my = e->y;
    
    // Check if clicking in grid area
    int grid_w = GRID_COLS * CELL_SIZE * zoom;
    int grid_h = GRID_ROWS * CELL_SIZE * zoom;
    
    if (mx >= offset_x && mx < offset_x + grid_w &&
        my >= offset_y && my < offset_y + grid_h) {
        
        // Calculate which cell and pixel
        int rel_x = mx - offset_x;
        int rel_y = my - offset_y;
        
        int cell_x = rel_x / (CELL_SIZE * zoom);
        int cell_y = rel_y / (CELL_SIZE * zoom);
        
        int pixel_x = (rel_x % (CELL_SIZE * zoom)) / zoom;
        int pixel_y = (rel_y % (CELL_SIZE * zoom)) / zoom;
        
        if (cell_x >= 0 && cell_x < GRID_COLS && 
            cell_y >= 0 && cell_y < GRID_ROWS &&
            pixel_x >= 0 && pixel_x < CELL_SIZE &&
            pixel_y >= 0 && pixel_y < CELL_SIZE) {
            
            if (is_down) {
                drawing = 1;
                current_cell_x = cell_x;
                current_cell_y = cell_y;
                uint32_t color = eraser_mode ? TRANSPARENT_PIXEL : rgb_to_rgba(palette[current_color]);
                draw_pixel(cell_x, cell_y, pixel_x, pixel_y, color);
            } else {
                drawing = 0;
            }
        }
    }
    
    // Check if clicking in palette area
    int ui_x = offset_x + grid_w + 20;
    int ui_y = 10;
    int palette_cols = 4;
    int swatch_size = 30;
    int palette_start_y = eraser_mode ? ui_y + 35 : ui_y + 10;
    int palette_rows = (PALETTE_SIZE + palette_cols - 1) / palette_cols;
    
    // Check palette area (adjust for eraser indicator)
    if (mx >= ui_x + 10 && mx < ui_x + 140 && 
        my >= palette_start_y && my < palette_start_y + palette_rows * (swatch_size + 5)) {
        int rel_x = mx - ui_x - 10;
        int rel_y = my - palette_start_y;
        int col = rel_x / (swatch_size + 5);
        int row = rel_y / (swatch_size + 5);
        int idx = row * palette_cols + col;
        
        if (idx >= 0 && idx < PALETTE_SIZE && is_down) {
            current_color = idx;
            eraser_mode = 0;  // Switch to draw mode when selecting color
        }
    }
    
    // Check for eraser button click (below palette)
    int eraser_y = palette_start_y + palette_rows * (swatch_size + 5);
    if (mx >= ui_x + 10 && mx < ui_x + 140 && my >= eraser_y && my < eraser_y + 30) {
        if (is_down) {
            eraser_mode = !eraser_mode;
        }
    }
}

// Handle mouse motion for drawing
static void handle_mouse_motion(SDL_MouseMotionEvent *e) {
    if (!drawing) return;
    
    int zoom = zoom_sizes[zoom_level];
    int offset_x = 10;
    int offset_y = 10;
    
    int mx = e->x;
    int my = e->y;
    
    int grid_w = GRID_COLS * CELL_SIZE * zoom;
    int grid_h = GRID_ROWS * CELL_SIZE * zoom;
    
    if (mx >= offset_x && mx < offset_x + grid_w &&
        my >= offset_y && my < offset_y + grid_h) {
        
        int rel_x = mx - offset_x;
        int rel_y = my - offset_y;
        
        int cell_x = rel_x / (CELL_SIZE * zoom);
        int cell_y = rel_y / (CELL_SIZE * zoom);
        
        int pixel_x = (rel_x % (CELL_SIZE * zoom)) / zoom;
        int pixel_y = (rel_y % (CELL_SIZE * zoom)) / zoom;
        
        if (cell_x >= 0 && cell_x < GRID_COLS && 
            cell_y >= 0 && cell_y < GRID_ROWS &&
            pixel_x >= 0 && pixel_x < CELL_SIZE &&
            pixel_y >= 0 && pixel_y < CELL_SIZE) {
            
            uint32_t color = eraser_mode ? TRANSPARENT_PIXEL : rgb_to_rgba(palette[current_color]);
            draw_pixel(cell_x, cell_y, pixel_x, pixel_y, color);
        }
    }
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    
    SDL_Window *window = SDL_CreateWindow(
        "Sprite Editor - 6x6 Grid (32x32 per cell)",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_W, WINDOW_H,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    // Initialize SDL_image
    int imgFlags = IMG_INIT_PNG;
    if (!(IMG_Init(imgFlags) & imgFlags)) {
        fprintf(stderr, "SDL_image initialization failed: %s\n", IMG_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    // Initialize sprite data to transparent
    clear_all();
    
    printf("Sprite Editor Controls:\n");
    printf("  Mouse: Draw pixels\n");
    printf("  Click palette: Select color\n");
    printf("  E: Toggle eraser tool\n");
    printf("  C: Clear current cell\n");
    printf("  A: Clear all cells\n");
    printf("  G: Toggle grid\n");
    printf("  +/-: Zoom in/out\n");
    printf("  S: Save sprite (sprite.png)\n");
    printf("  O: Load sprite (sprite.png)\n");
    printf("  hjkl: Navigate cells (vim-style: h=left, j=down, k=up, l=right)\n");
    printf("  1-9, 0: Select palette colors\n");
    printf("  Esc: Quit\n");
    
    int running = 1;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = 0;
            } else if (e.type == SDL_KEYDOWN) {
                SDL_Keycode key = e.key.keysym.sym;
                if (key == SDLK_ESCAPE) {
                    running = 0;
                } else if (key == SDLK_c) {
                    clear_cell(current_cell_x, current_cell_y);
                } else if (key == SDLK_a) {
                    clear_all();
                } else if (key == SDLK_g) {
                    show_grid = !show_grid;
                } else if (key == SDLK_PLUS || key == SDLK_EQUALS) {
                    if (zoom_level < ZOOM_LEVELS - 1) zoom_level++;
                } else if (key == SDLK_MINUS) {
                    if (zoom_level > 0) zoom_level--;  // Minimum is 1x (100%)
                } else if (key == SDLK_e) {
                    eraser_mode = !eraser_mode;
                    printf("Eraser mode: %s\n", eraser_mode ? "ON" : "OFF");
                } else if (key == SDLK_s) {
                    save_sprite("sprite.png");
                } else if (key == SDLK_o) {
                    // O for load (Open)
                    load_sprite("sprite.png");
                } else if (key == SDLK_h) {
                    // vim-style: h = left
                    if (current_cell_x > 0) current_cell_x--;
                } else if (key == SDLK_l) {
                    // vim-style: l = right
                    if (current_cell_x < GRID_COLS - 1) current_cell_x++;
                } else if (key == SDLK_j) {
                    // vim-style: j = down
                    if (current_cell_y < GRID_ROWS - 1) current_cell_y++;
                } else if (key == SDLK_k) {
                    // vim-style: k = up
                    if (current_cell_y > 0) current_cell_y--;
                } else if (key >= SDLK_1 && key <= SDLK_9) {
                    current_color = key - SDLK_1;
                    eraser_mode = 0;  // Switch to draw mode when selecting color
                } else if (key == SDLK_0) {
                    current_color = 9;
                    eraser_mode = 0;  // Switch to draw mode when selecting color
                }
            } else if (e.type == SDL_MOUSEBUTTONDOWN) {
                if (e.button.button == SDL_BUTTON_LEFT) {
                    handle_mouse(&e.button, 1);
                }
            } else if (e.type == SDL_MOUSEBUTTONUP) {
                if (e.button.button == SDL_BUTTON_LEFT) {
                    handle_mouse(&e.button, 0);
                }
            } else if (e.type == SDL_MOUSEMOTION) {
                if (drawing) {
                    handle_mouse_motion(&e.motion);
                }
            }
        }
        
        render(renderer);
        SDL_RenderPresent(renderer);
        SDL_Delay(16);  // ~60 FPS
    }
    
    IMG_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}


