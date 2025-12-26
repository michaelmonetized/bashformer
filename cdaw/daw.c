// Full-screen DAW for creating trap beats
// Compile: gcc daw.c -o daw $(sdl2-config --cflags --libs) -lSDL2_image -lSDL2_mixer -lm
// Usage: ./daw

#include <SDL.h>
#include <SDL_mixer.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <unistd.h>

#define SAMPLE_RATE 22050
#define GRID_COLS 512  // 32 bars * 16 sixteenth notes per bar
#define GRID_ROWS 8   // 8 tracks
#define STEPS_PER_BAR 16

#define TRACK_KICK 0
#define TRACK_SNARE 1
#define TRACK_HIHAT 2
#define TRACK_EXTRA 3
#define TRACK_5 4
#define TRACK_6 5
#define TRACK_7 6
#define TRACK_8 7

#define TRACK_NAMES {"Kick", "Snare", "Hi-hat", "Chord 1", "Chord 2", "Chord 3", "Chord 4", "Track 8"}

typedef struct {
    int cells[GRID_ROWS][GRID_COLS];
    float playhead;
    int playing;
    float bpm;
    int current_step;
    float step_duration;
    char pattern_name[64];
    int editing_name;
    int name_cursor_pos;
    int loop_end_bar;  // Last bar with notes
    Mix_Chunk *chunks[GRID_ROWS];
} BeatPattern;

// Audio generation functions (from generate_sfx.c)
static void write_sample(FILE *f, int16_t sample) {
    fwrite(&sample, sizeof(int16_t), 1, f);
}

static void generate_kick_sample(const char *filename, float volume) {
    FILE *f = fopen(filename, "wb");
    if (!f) return;
    
    // WAV header placeholder
    fseek(f, 44, SEEK_SET);
    
    // Deep thump kick - lower frequency, longer decay
    int samples = (int)(SAMPLE_RATE * 0.3f); // Longer decay
    for (int i = 0; i < samples; i++) {
        float t = (float)i / SAMPLE_RATE;
        // Deep thump: start at very low frequency, slow decay
        float freq = 40.0f + 10.0f * expf(-t * 8.0f); // Lower, slower sweep
        float phase = fmodf((float)i * freq / SAMPLE_RATE, 1.0f);
        // Use sine wave for smoother thump
        float value = sinf(2.0f * M_PI * phase);
        // Longer, smoother envelope
        float envelope = expf(-t * 4.0f);
        int16_t sample = (int16_t)(value * envelope * volume * 32767.0f);
        write_sample(f, sample);
    }
    
    long data_end = ftell(f);
    uint32_t data_size = data_end - 44;
    
    // Write WAV header
    fseek(f, 0, SEEK_SET);
    fwrite("RIFF", 4, 1, f);
    uint32_t file_size = data_size + 36;
    fwrite(&file_size, 4, 1, f);
    fwrite("WAVE", 4, 1, f);
    fwrite("fmt ", 4, 1, f);
    uint32_t fmt_size = 16;
    fwrite(&fmt_size, 4, 1, f);
    uint16_t audio_format = 1;
    fwrite(&audio_format, 2, 1, f);
    uint16_t channels = 1;
    fwrite(&channels, 2, 1, f);
    uint32_t sample_rate = SAMPLE_RATE;
    fwrite(&sample_rate, 4, 1, f);
    uint32_t byte_rate = SAMPLE_RATE * 2;
    fwrite(&byte_rate, 4, 1, f);
    uint16_t block_align = 2;
    fwrite(&block_align, 2, 1, f);
    uint16_t bits_per_sample = 16;
    fwrite(&bits_per_sample, 2, 1, f);
    fwrite("data", 4, 1, f);
    fwrite(&data_size, 4, 1, f);
    
    fclose(f);
}

static void generate_snare_sample(const char *filename, float volume) {
    FILE *f = fopen(filename, "wb");
    if (!f) return;
    
    fseek(f, 44, SEEK_SET);
    
    int samples = (int)(SAMPLE_RATE * 0.1f);
    for (int i = 0; i < samples; i++) {
        float t = (float)i / SAMPLE_RATE;
        float noise = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
        float tone = sinf(2.0f * M_PI * 200.0f * t);
        float value = (noise * 0.7f + tone * 0.3f);
        float envelope = expf(-t * 15.0f);
        int16_t sample = (int16_t)(value * envelope * volume * 32767.0f);
        write_sample(f, sample);
    }
    
    long data_end = ftell(f);
    uint32_t data_size = data_end - 44;
    
    fseek(f, 0, SEEK_SET);
    fwrite("RIFF", 4, 1, f);
    uint32_t file_size = data_size + 36;
    fwrite(&file_size, 4, 1, f);
    fwrite("WAVE", 4, 1, f);
    fwrite("fmt ", 4, 1, f);
    uint32_t fmt_size = 16;
    fwrite(&fmt_size, 4, 1, f);
    uint16_t audio_format = 1;
    fwrite(&audio_format, 2, 1, f);
    uint16_t channels = 1;
    fwrite(&channels, 2, 1, f);
    uint32_t sample_rate = SAMPLE_RATE;
    fwrite(&sample_rate, 4, 1, f);
    uint32_t byte_rate = SAMPLE_RATE * 2;
    fwrite(&byte_rate, 4, 1, f);
    uint16_t block_align = 2;
    fwrite(&block_align, 2, 1, f);
    uint16_t bits_per_sample = 16;
    fwrite(&bits_per_sample, 2, 1, f);
    fwrite("data", 4, 1, f);
    fwrite(&data_size, 4, 1, f);
    
    fclose(f);
}

static void generate_hihat_sample(const char *filename, float volume) {
    FILE *f = fopen(filename, "wb");
    if (!f) return;
    
    fseek(f, 44, SEEK_SET);
    
    int samples = (int)(SAMPLE_RATE * 0.05f);
    for (int i = 0; i < samples; i++) {
        float t = (float)i / SAMPLE_RATE;
        float noise = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
        float envelope = expf(-t * 30.0f);
        int16_t sample = (int16_t)(noise * envelope * volume * 32767.0f);
        write_sample(f, sample);
    }
    
    long data_end = ftell(f);
    uint32_t data_size = data_end - 44;
    
    fseek(f, 0, SEEK_SET);
    fwrite("RIFF", 4, 1, f);
    uint32_t file_size = data_size + 36;
    fwrite(&file_size, 4, 1, f);
    fwrite("WAVE", 4, 1, f);
    fwrite("fmt ", 4, 1, f);
    uint32_t fmt_size = 16;
    fwrite(&fmt_size, 4, 1, f);
    uint16_t audio_format = 1;
    fwrite(&audio_format, 2, 1, f);
    uint16_t channels = 1;
    fwrite(&channels, 2, 1, f);
    uint32_t sample_rate = SAMPLE_RATE;
    fwrite(&sample_rate, 4, 1, f);
    uint32_t byte_rate = SAMPLE_RATE * 2;
    fwrite(&byte_rate, 4, 1, f);
    uint16_t block_align = 2;
    fwrite(&block_align, 2, 1, f);
    uint16_t bits_per_sample = 16;
    fwrite(&bits_per_sample, 2, 1, f);
    fwrite("data", 4, 1, f);
    fwrite(&data_size, 4, 1, f);
    
    fclose(f);
}

// Generate epic EDM dance stab chord - punchy, aggressive, with harmonics
static void generate_synth_stab(const char *filename, float *freqs, int num_freqs, float volume) {
    FILE *f = fopen(filename, "wb");
    if (!f) return;
    
    fseek(f, 44, SEEK_SET);
    
    // Epic EDM stab - short, punchy, with harmonics and saturation
    int samples = (int)(SAMPLE_RATE * 0.15f); // Shorter, punchier
    for (int i = 0; i < samples; i++) {
        float t = (float)i / SAMPLE_RATE;
        float value = 0.0f;
        
        // Mix all frequencies in the chord with harmonics
        for (int j = 0; j < num_freqs; j++) {
            float freq = freqs[j];
            // Add fundamental + harmonics for richness
            float fundamental = sinf(2.0f * M_PI * freq * t);
            float harmonic2 = sinf(2.0f * M_PI * freq * 2.0f * t) * 0.3f; // 2nd harmonic
            float harmonic3 = sinf(2.0f * M_PI * freq * 3.0f * t) * 0.15f; // 3rd harmonic
            // Use square wave component for punchiness
            float square = (sinf(2.0f * M_PI * freq * t) > 0.0f) ? 0.5f : -0.5f;
            value += (fundamental + harmonic2 + harmonic3 + square * 0.2f) / num_freqs;
        }
        
        // Add some low-frequency punch (sub-bass layer)
        float sub_freq = freqs[0] * 0.5f; // Octave below root
        float sub = sinf(2.0f * M_PI * sub_freq * t) * 0.3f;
        value = (value + sub) / 1.3f;
        
        // Aggressive envelope: very quick attack, fast decay
        float envelope = 1.0f;
        if (t < 0.005f) {
            envelope = t / 0.005f; // Very quick attack (5ms)
        } else {
            envelope = expf(-(t - 0.005f) * 12.0f); // Fast decay
        }
        
        // Add saturation/distortion for more punch
        float saturated = value * envelope * volume;
        // Soft clipping
        if (saturated > 0.7f) {
            saturated = 0.7f + (saturated - 0.7f) * 0.3f;
        } else if (saturated < -0.7f) {
            saturated = -0.7f + (saturated + 0.7f) * 0.3f;
        }
        
        int16_t sample = (int16_t)(saturated * 32767.0f);
        write_sample(f, sample);
    }
    
    long data_end = ftell(f);
    uint32_t data_size = data_end - 44;
    
    fseek(f, 0, SEEK_SET);
    fwrite("RIFF", 4, 1, f);
    uint32_t file_size = data_size + 36;
    fwrite(&file_size, 4, 1, f);
    fwrite("WAVE", 4, 1, f);
    fwrite("fmt ", 4, 1, f);
    uint32_t fmt_size = 16;
    fwrite(&fmt_size, 4, 1, f);
    uint16_t audio_format = 1;
    fwrite(&audio_format, 2, 1, f);
    uint16_t channels = 1;
    fwrite(&channels, 2, 1, f);
    uint32_t sample_rate = SAMPLE_RATE;
    fwrite(&sample_rate, 4, 1, f);
    uint32_t byte_rate = SAMPLE_RATE * 2;
    fwrite(&byte_rate, 4, 1, f);
    uint16_t block_align = 2;
    fwrite(&block_align, 2, 1, f);
    uint16_t bits_per_sample = 16;
    fwrite(&bits_per_sample, 2, 1, f);
    fwrite("data", 4, 1, f);
    fwrite(&data_size, 4, 1, f);
    
    fclose(f);
}

static void update_loop_end(BeatPattern *beat) {
    // Find the last bar that has any notes
    beat->loop_end_bar = 0;
    for (int bar = 0; bar < GRID_COLS / STEPS_PER_BAR; bar++) {
        int bar_start = bar * STEPS_PER_BAR;
        int has_notes = 0;
        for (int track = 0; track < GRID_ROWS; track++) {
            for (int step = 0; step < STEPS_PER_BAR; step++) {
                if (beat->cells[track][bar_start + step]) {
                    has_notes = 1;
                    break;
                }
            }
            if (has_notes) break;
        }
        if (has_notes) {
            beat->loop_end_bar = bar + 1;
        }
    }
    // Default to at least 1 bar
    if (beat->loop_end_bar == 0) beat->loop_end_bar = 1;
}

static void init_beat_pattern(BeatPattern *beat) {
    memset(beat->cells, 0, sizeof(beat->cells));
    beat->playhead = 0.0f;
    beat->playing = 0;
    beat->bpm = 140.0f;
    beat->current_step = 0;
    beat->step_duration = 60.0f / beat->bpm / 4.0f; // 16th note duration
    strcpy(beat->pattern_name, "Untitled");
    beat->editing_name = 0;
    beat->name_cursor_pos = 0;
    beat->loop_end_bar = 1;
    
    // Initialize chunks
    for (int i = 0; i < GRID_ROWS; i++) {
        beat->chunks[i] = NULL;
    }
    
    // Generate sample files for each track
    generate_kick_sample("/tmp/daw_kick.wav", 0.7f);
    generate_snare_sample("/tmp/daw_snare.wav", 0.6f);
    generate_hihat_sample("/tmp/daw_hihat.wav", 0.4f);
    
    // Generate synth stab chords for tracks 4-7 (different chords)
    // Track 4: C major (C, E, G)
    float chord1[] = {261.63f, 329.63f, 392.00f};
    generate_synth_stab("/tmp/daw_track4.wav", chord1, 3, 0.5f);
    
    // Track 5: D minor (D, F, A)
    float chord2[] = {293.66f, 349.23f, 440.00f};
    generate_synth_stab("/tmp/daw_track5.wav", chord2, 3, 0.5f);
    
    // Track 6: E minor (E, G, B)
    float chord3[] = {329.63f, 392.00f, 493.88f};
    generate_synth_stab("/tmp/daw_track6.wav", chord3, 3, 0.5f);
    
    // Track 7: F major (F, A, C)
    float chord4[] = {349.23f, 440.00f, 523.25f};
    generate_synth_stab("/tmp/daw_track7.wav", chord4, 3, 0.5f);
    
    generate_hihat_sample("/tmp/daw_track8.wav", 0.5f);
    
    // Load samples
    beat->chunks[TRACK_KICK] = Mix_LoadWAV("/tmp/daw_kick.wav");
    beat->chunks[TRACK_SNARE] = Mix_LoadWAV("/tmp/daw_snare.wav");
    beat->chunks[TRACK_HIHAT] = Mix_LoadWAV("/tmp/daw_hihat.wav");
    beat->chunks[TRACK_EXTRA] = Mix_LoadWAV("/tmp/daw_track4.wav");
    beat->chunks[TRACK_5] = Mix_LoadWAV("/tmp/daw_track5.wav");
    beat->chunks[TRACK_6] = Mix_LoadWAV("/tmp/daw_track6.wav");
    beat->chunks[TRACK_7] = Mix_LoadWAV("/tmp/daw_track7.wav");
    beat->chunks[TRACK_8] = Mix_LoadWAV("/tmp/daw_track8.wav");
}

static void cleanup_beat_pattern(BeatPattern *beat) {
    for (int i = 0; i < GRID_ROWS; i++) {
        if (beat->chunks[i]) Mix_FreeChunk(beat->chunks[i]);
    }
    unlink("/tmp/daw_kick.wav");
    unlink("/tmp/daw_snare.wav");
    unlink("/tmp/daw_hihat.wav");
    unlink("/tmp/daw_track4.wav");
    unlink("/tmp/daw_track5.wav");
    unlink("/tmp/daw_track6.wav");
    unlink("/tmp/daw_track7.wav");
    unlink("/tmp/daw_track8.wav");
}

static void update_beat_pattern(BeatPattern *beat, float dt) {
    if (!beat->playing) return;
    
    beat->playhead += dt;
    
    int new_step = (int)(beat->playhead / beat->step_duration) % GRID_COLS;
    
    if (new_step != beat->current_step) {
        beat->current_step = new_step;
        
        // Play sounds for this step
        for (int track = 0; track < GRID_ROWS; track++) {
            if (beat->cells[track][beat->current_step] && beat->chunks[track]) {
                Mix_PlayChannel(-1, beat->chunks[track], 0);
            }
        }
    }
    
    // Loop - only loop bars with notes
    int loop_end_step = beat->loop_end_bar * STEPS_PER_BAR;
    if (beat->playhead >= beat->step_duration * loop_end_step) {
        beat->playhead = 0.0f;
        beat->current_step = -1;
    }
}

// Simple text rendering using bitmap font
static void draw_text(SDL_Renderer *renderer, int x, int y, int scale, const char *text, Uint8 r, Uint8 g, Uint8 b) {
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
    
    static const char *letters[26] = {
        "111101101101111", // A
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
    
    int textX = x;
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
            textX += 3 * scale + scale;
            continue;
        } else if (*p == ':') {
            // Colon - simple vertical dots
            SDL_Rect dot1 = {textX + scale, y + scale, scale, scale};
            SDL_Rect dot2 = {textX + scale, y + 3 * scale, scale, scale};
            SDL_RenderFillRect(renderer, &dot1);
            SDL_RenderFillRect(renderer, &dot2);
            textX += 3 * scale + scale;
            continue;
        } else {
            continue;
        }
        
        if (pat) {
            for (int row = 0; row < 5; row++) {
                for (int col = 0; col < 3; col++) {
                    if (pat[row * 3 + col] == '1') {
                        SDL_Rect rect = {
                            textX + col * scale,
                            y + row * scale,
                            scale, scale
                        };
                        SDL_RenderFillRect(renderer, &rect);
                    }
                }
            }
            textX += 3 * scale + scale;
        }
    }
}

static void render_grid(SDL_Renderer *renderer, BeatPattern *beat, int width, int height) {
    int margin = 20;
    int track_height = 80;
    int track_label_width = 100;
    int grid_x = margin + track_label_width;
    int grid_y = margin + 100;
    int cell_width = (width - grid_x - margin) / GRID_COLS;
    int cell_height = track_height;
    
    const char *track_names[] = TRACK_NAMES;
    
    // Draw background
    SDL_SetRenderDrawColor(renderer, 20, 20, 30, 255);
    SDL_RenderClear(renderer);
    
    // Draw title
    SDL_Rect title_rect = {margin, margin, width - 2*margin, 60};
    SDL_SetRenderDrawColor(renderer, 40, 40, 60, 255);
    SDL_RenderFillRect(renderer, &title_rect);
    
    // Draw grid background
    SDL_Rect grid_bg = {grid_x, grid_y, cell_width * GRID_COLS, cell_height * GRID_ROWS};
    SDL_SetRenderDrawColor(renderer, 30, 30, 40, 255);
    SDL_RenderFillRect(renderer, &grid_bg);
    
    // Draw tracks
    for (int row = 0; row < GRID_ROWS; row++) {
        int track_y = grid_y + row * cell_height;
        
        // Track label background
        SDL_Rect label_bg = {margin, track_y, track_label_width, cell_height};
        SDL_SetRenderDrawColor(renderer, 50, 50, 70, 255);
        SDL_RenderFillRect(renderer, &label_bg);
        
        // Draw cells
        for (int col = 0; col < GRID_COLS; col++) {
            int cell_x = grid_x + col * cell_width;
            SDL_Rect cell_rect = {cell_x, track_y, cell_width - 2, cell_height - 2};
            
            // Color based on track (8 different colors)
            if (beat->cells[row][col]) {
                int colors[8][3] = {
                    {200, 80, 80},   // Kick - red
                    {80, 200, 80},   // Snare - green
                    {80, 80, 200},   // Hi-hat - blue
                    {200, 200, 80},  // Track 4 - yellow
                    {200, 80, 200},  // Track 5 - magenta
                    {80, 200, 200},  // Track 6 - cyan
                    {200, 150, 80},  // Track 7 - orange
                    {150, 80, 200}   // Track 8 - purple
                };
                SDL_SetRenderDrawColor(renderer, colors[row][0], colors[row][1], colors[row][2], 255);
                SDL_RenderFillRect(renderer, &cell_rect);
            } else {
                SDL_SetRenderDrawColor(renderer, 40, 40, 50, 255);
                SDL_RenderFillRect(renderer, &cell_rect);
            }
            
            // Grid lines
            SDL_SetRenderDrawColor(renderer, 60, 60, 70, 255);
            SDL_RenderDrawRect(renderer, &cell_rect);
            
            // Beat markers (every 4th step = 16th notes)
            if (col % 4 == 0) {
                SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
                SDL_RenderDrawLine(renderer, cell_x, track_y, cell_x, track_y + cell_height);
            }
            // Bar markers (every 16th step = bars)
            if (col % STEPS_PER_BAR == 0) {
                SDL_SetRenderDrawColor(renderer, 150, 150, 200, 255);
                SDL_RenderDrawLine(renderer, cell_x, track_y - 5, cell_x, track_y + cell_height + 5);
            }
        }
        
        // Highlight current playhead
        if (beat->playing) {
            int playhead_x = grid_x + beat->current_step * cell_width;
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 180);
            SDL_RenderDrawLine(renderer, playhead_x, grid_y, playhead_x, grid_y + cell_height * GRID_ROWS);
        }
    }
    
    // Draw controls area
    int control_y = grid_y + cell_height * GRID_ROWS + 20;
    SDL_Rect control_bg = {margin, control_y, width - 2*margin, 80};
    SDL_SetRenderDrawColor(renderer, 40, 40, 60, 255);
    SDL_RenderFillRect(renderer, &control_bg);
    
    // Play/Pause button
    SDL_Rect play_rect = {margin + 20, control_y + 15, 100, 50};
    SDL_SetRenderDrawColor(renderer, beat->playing ? 200 : 100, beat->playing ? 150 : 200, 100, 255);
    SDL_RenderFillRect(renderer, &play_rect);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &play_rect);
    
    // Stop button
    SDL_Rect stop_rect = {margin + 140, control_y + 15, 100, 50};
    SDL_SetRenderDrawColor(renderer, 200, 100, 100, 255);
    SDL_RenderFillRect(renderer, &stop_rect);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &stop_rect);
    
    // BPM display
    SDL_Rect bpm_rect = {margin + 260, control_y + 15, 150, 50};
    SDL_SetRenderDrawColor(renderer, 60, 60, 80, 255);
    SDL_RenderFillRect(renderer, &bpm_rect);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &bpm_rect);
    
    // Draw BPM text
    char bpm_text[16];
    snprintf(bpm_text, sizeof(bpm_text), "BPM:%d", (int)beat->bpm);
    draw_text(renderer, margin + 270, control_y + 25, 3, bpm_text, 255, 255, 255);
    
    // Pattern name display (editable)
    SDL_Rect name_rect = {margin + 430, control_y + 15, 300, 50};
    // Highlight if editing
    if (beat->editing_name) {
        SDL_SetRenderDrawColor(renderer, 80, 80, 120, 255);
    } else {
        SDL_SetRenderDrawColor(renderer, 50, 50, 70, 255);
    }
    SDL_RenderFillRect(renderer, &name_rect);
    // Draw border - thicker if editing
    if (beat->editing_name) {
        SDL_SetRenderDrawColor(renderer, 100, 200, 255, 255);
        for (int i = 0; i < 3; i++) {
            SDL_Rect border = {name_rect.x - i, name_rect.y - i, name_rect.w + i*2, name_rect.h + i*2};
            SDL_RenderDrawRect(renderer, &border);
        }
    } else {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &name_rect);
    }
    
    // Draw pattern name text with cursor
    char name_display[80];
    snprintf(name_display, sizeof(name_display), "NAME:%s", beat->pattern_name);
    draw_text(renderer, margin + 440, control_y + 25, 2, name_display, 255, 255, 255);
    
    // Draw cursor if editing
    if (beat->editing_name) {
        int cursor_x = margin + 440 + (5 + beat->name_cursor_pos) * (3 * 2 + 2); // Approximate cursor position
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawLine(renderer, cursor_x, control_y + 20, cursor_x, control_y + 45);
    }
    
    // Info text area
    SDL_Rect info_rect = {margin + 750, control_y + 15, width - margin - 770, 50};
    SDL_SetRenderDrawColor(renderer, 50, 50, 70, 255);
    SDL_RenderFillRect(renderer, &info_rect);
    
    // Draw info text
    draw_text(renderer, margin + 760, control_y + 25, 2, "CTRL+E:EXPORT CTRL+N:NAME", 200, 200, 200);
}

static int handle_click(BeatPattern *beat, int x, int y, int width, int height) {
    int margin = 20;
    int track_label_width = 100;
    int grid_x = margin + track_label_width;
    int grid_y = margin + 100;
    int cell_width = (width - grid_x - margin) / GRID_COLS;
    int cell_height = 80;
    
    // Check if click is in grid
    if (x >= grid_x && y >= grid_y && x < grid_x + cell_width * GRID_COLS && y < grid_y + cell_height * GRID_ROWS) {
        int col = (x - grid_x) / cell_width;
        int row = (y - grid_y) / cell_height;
        
        if (col >= 0 && col < GRID_COLS && row >= 0 && row < GRID_ROWS) {
            beat->cells[row][col] = !beat->cells[row][col];
            update_loop_end(beat);
            return 1;
        }
    }
    
    // Check controls
    int control_y = grid_y + cell_height * GRID_ROWS + 20;
    
    // Play/Pause button
    if (x >= margin + 20 && x <= margin + 120 && y >= control_y + 15 && y <= control_y + 65) {
        beat->playing = !beat->playing;
        if (!beat->playing) {
            beat->playhead = 0.0f;
            beat->current_step = -1;
        }
        return 1;
    }
    
    // Stop button
    if (x >= margin + 140 && x <= margin + 240 && y >= control_y + 15 && y <= control_y + 65) {
        beat->playing = 0;
        beat->playhead = 0.0f;
        beat->current_step = -1;
        return 1;
    }
    
    // Pattern name field
    SDL_Rect name_rect = {margin + 430, control_y + 15, 300, 50};
    if (x >= name_rect.x && x <= name_rect.x + name_rect.w && y >= name_rect.y && y <= name_rect.y + name_rect.h) {
        beat->editing_name = 1;
        beat->name_cursor_pos = strlen(beat->pattern_name);
        SDL_StartTextInput();
        return 1;
    } else {
        beat->editing_name = 0;
        SDL_StopTextInput();
    }
    
    return 0;
}

static void write_wav_header(FILE *f, uint32_t data_size) {
    fseek(f, 0, SEEK_SET);
    fwrite("RIFF", 4, 1, f);
    uint32_t file_size = data_size + 36;
    fwrite(&file_size, 4, 1, f);
    fwrite("WAVE", 4, 1, f);
    fwrite("fmt ", 4, 1, f);
    uint32_t fmt_size = 16;
    fwrite(&fmt_size, 4, 1, f);
    uint16_t audio_format = 1;
    fwrite(&audio_format, 2, 1, f);
    uint16_t channels = 1;
    fwrite(&channels, 2, 1, f);
    uint32_t sample_rate = SAMPLE_RATE;
    fwrite(&sample_rate, 4, 1, f);
    uint32_t byte_rate = SAMPLE_RATE * 2;
    fwrite(&byte_rate, 4, 1, f);
    uint16_t block_align = 2;
    fwrite(&block_align, 2, 1, f);
    uint16_t bits_per_sample = 16;
    fwrite(&bits_per_sample, 2, 1, f);
    fwrite("data", 4, 1, f);
    fwrite(&data_size, 4, 1, f);
}

static void export_wav(BeatPattern *beat) {
    char filename[128];
    snprintf(filename, sizeof(filename), "%s.wav", beat->pattern_name);
    
    FILE *f = fopen(filename, "wb");
    if (!f) {
        printf("Failed to create %s\n", filename);
        return;
    }
    
    // Write placeholder header
    fseek(f, 44, SEEK_SET);
    
    float sixteenth_note = 60.0f / beat->bpm / 4.0f;
    int loop_end_step = beat->loop_end_bar * STEPS_PER_BAR;
    float pattern_duration = sixteenth_note * loop_end_step;
    int total_samples = (int)(SAMPLE_RATE * pattern_duration);
    
    // Generate audio for each sample
    for (int sample_idx = 0; sample_idx < total_samples; sample_idx++) {
        float t = (float)sample_idx / SAMPLE_RATE;
        float mixed = 0.0f;
        
        // Check which tracks should play at this time
        int step = ((int)(t / sixteenth_note)) % loop_end_step;
        float step_t = fmodf(t, sixteenth_note);
        
        // Mix all active tracks
        for (int track = 0; track < GRID_ROWS; track++) {
            if (beat->cells[track][step] && beat->chunks[track]) {
                // Generate audio based on track type
                float track_vol = 0.0f;
                if (step_t < 0.3f) { // Only play at the start of the step
                    if (track == TRACK_KICK) {
                        // Deep thump kick
                        float freq = 40.0f + 10.0f * expf(-step_t * 8.0f);
                        track_vol = sinf(2.0f * M_PI * freq * step_t) * expf(-step_t * 4.0f) * 0.6f;
                    } else if (track == TRACK_SNARE) {
                        // Snare
                        float noise = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
                        float tone = sinf(2.0f * M_PI * 200.0f * step_t);
                        track_vol = (noise * 0.7f + tone * 0.3f) * expf(-step_t * 15.0f) * 0.5f;
                    } else if (track >= TRACK_EXTRA && track <= TRACK_7) {
                        // Epic EDM synth stab chords
                        float chord_vol = 0.0f;
                        if (step_t < 0.15f) {
                            float freqs[3];
                            if (track == TRACK_EXTRA) {
                                freqs[0] = 261.63f; freqs[1] = 329.63f; freqs[2] = 392.00f; // C major
                            } else if (track == TRACK_5) {
                                freqs[0] = 293.66f; freqs[1] = 349.23f; freqs[2] = 440.00f; // D minor
                            } else if (track == TRACK_6) {
                                freqs[0] = 329.63f; freqs[1] = 392.00f; freqs[2] = 493.88f; // E minor
                            } else { // TRACK_7
                                freqs[0] = 349.23f; freqs[1] = 440.00f; freqs[2] = 523.25f; // F major
                            }
                            // Mix with harmonics for punch
                            for (int j = 0; j < 3; j++) {
                                float freq = freqs[j];
                                float fund = sinf(2.0f * M_PI * freq * step_t);
                                float harm2 = sinf(2.0f * M_PI * freq * 2.0f * step_t) * 0.3f;
                                float harm3 = sinf(2.0f * M_PI * freq * 3.0f * step_t) * 0.15f;
                                float square = (sinf(2.0f * M_PI * freq * step_t) > 0.0f) ? 0.5f : -0.5f;
                                chord_vol += (fund + harm2 + harm3 + square * 0.2f) / 3.0f;
                            }
                            // Add sub-bass
                            float sub = sinf(2.0f * M_PI * freqs[0] * 0.5f * step_t) * 0.3f;
                            chord_vol = (chord_vol + sub) / 1.3f;
                            
                            // Aggressive envelope
                            float envelope = 1.0f;
                            if (step_t < 0.005f) {
                                envelope = step_t / 0.005f;
                            } else {
                                envelope = expf(-(step_t - 0.005f) * 12.0f);
                            }
                            
                            // Soft clipping
                            float saturated = chord_vol * envelope * 0.6f;
                            if (saturated > 0.7f) {
                                saturated = 0.7f + (saturated - 0.7f) * 0.3f;
                            } else if (saturated < -0.7f) {
                                saturated = -0.7f + (saturated + 0.7f) * 0.3f;
                            }
                            track_vol = saturated;
                        }
                    } else {
                        // Hi-hat and other tracks
                        float noise = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
                        track_vol = noise * expf(-step_t * 30.0f) * 0.4f;
                    }
                }
                mixed += track_vol;
            }
        }
        
        // Clamp
        if (mixed > 1.0f) mixed = 1.0f;
        if (mixed < -1.0f) mixed = -1.0f;
        
        int16_t sample = (int16_t)(mixed * 32767.0f);
        write_sample(f, sample);
    }
    
    long data_end = ftell(f);
    uint32_t data_size = data_end - 44;
    write_wav_header(f, data_size);
    fclose(f);
    
    printf("Exported pattern to %s\n", filename);
}

static void save_pattern(BeatPattern *beat, const char *filename) {
    FILE *f = fopen(filename, "wb");
    if (!f) return;
    
    fwrite(&beat->bpm, sizeof(float), 1, f);
    int name_len = strlen(beat->pattern_name) + 1;
    fwrite(&name_len, sizeof(int), 1, f);
    fwrite(beat->pattern_name, name_len, 1, f);
    fwrite(beat->cells, sizeof(beat->cells), 1, f);
    fclose(f);
    printf("Saved pattern to %s\n", filename);
}

static void load_pattern(BeatPattern *beat, const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        printf("Failed to open %s\n", filename);
        return;
    }
    
    fread(&beat->bpm, sizeof(float), 1, f);
    int name_len;
    fread(&name_len, sizeof(int), 1, f);
    if (name_len > 0 && name_len < 64) {
        fread(beat->pattern_name, name_len, 1, f);
    }
    fread(beat->cells, sizeof(beat->cells), 1, f);
    beat->step_duration = 60.0f / beat->bpm / 4.0f;
    update_loop_end(beat);
    fclose(f);
    printf("Loaded pattern from %s\n", filename);
}

static void show_file_picker(BeatPattern *beat) {
    printf("\n=== Pattern Files ===\n");
    printf("Enter filename (or 'cancel' to abort): ");
    fflush(stdout);
    
    char filename[256];
    if (fgets(filename, sizeof(filename), stdin)) {
        // Remove newline
        size_t len = strlen(filename);
        if (len > 0 && filename[len-1] == '\n') {
            filename[len-1] = '\0';
        }
        
        if (strcmp(filename, "cancel") != 0) {
            // Add .pattern extension if not present
            char fullpath[512];
            if (strstr(filename, ".pattern") == NULL) {
                snprintf(fullpath, sizeof(fullpath), "%s.pattern", filename);
            } else {
                strncpy(fullpath, filename, sizeof(fullpath));
            }
            load_pattern(beat, fullpath);
        }
    }
}

int main(int argc, char *argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "SDL init failed: %s\n", SDL_GetError());
        return 1;
    }
    
    if (Mix_OpenAudio(SAMPLE_RATE, MIX_DEFAULT_FORMAT, 1, 512) < 0) {
        fprintf(stderr, "Mix_OpenAudio failed: %s\n", Mix_GetError());
        SDL_Quit();
        return 1;
    }
    
    // Get fullscreen dimensions
    SDL_DisplayMode dm;
    SDL_GetDesktopDisplayMode(0, &dm);
    int width = dm.w;
    int height = dm.h;
    
    SDL_Window *window = SDL_CreateWindow("Trap Beat DAW",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height, SDL_WINDOW_FULLSCREEN_DESKTOP);
    
    if (!window) {
        fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
        Mix_CloseAudio();
        SDL_Quit();
        return 1;
    }
    
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        fprintf(stderr, "Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        Mix_CloseAudio();
        SDL_Quit();
        return 1;
    }
    
    BeatPattern beat;
    init_beat_pattern(&beat);
    
    // Load pattern if provided
    if (argc > 1) {
        load_pattern(&beat, argv[1]);
    }
    
    int running = 1;
    Uint32 last_time = SDL_GetTicks();
    
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = 0;
            } else if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_ESCAPE) {
                    running = 0;
                } else if (e.key.keysym.sym == SDLK_SPACE) {
                    beat.playing = !beat.playing;
                    if (!beat.playing) {
                        beat.playhead = 0.0f;
                        beat.current_step = -1;
                    }
                } else if (e.key.keysym.sym == SDLK_s && (e.key.keysym.mod & KMOD_CTRL)) {
                    save_pattern(&beat, "beat.pattern");
                } else if (e.key.keysym.sym == SDLK_o && (e.key.keysym.mod & KMOD_CTRL)) {
                    show_file_picker(&beat);
                } else if (e.key.keysym.sym == SDLK_UP) {
                    beat.bpm += 5.0f;
                    if (beat.bpm > 200.0f) beat.bpm = 200.0f;
                    beat.step_duration = 60.0f / beat.bpm / 4.0f;
                } else if (e.key.keysym.sym == SDLK_DOWN) {
                    beat.bpm -= 5.0f;
                    if (beat.bpm < 60.0f) beat.bpm = 60.0f;
                    beat.step_duration = 60.0f / beat.bpm / 4.0f;
                } else if (e.key.keysym.sym == SDLK_e && (e.key.keysym.mod & KMOD_CTRL)) {
                    export_wav(&beat);
                } else if (e.key.keysym.sym == SDLK_c && (e.key.keysym.mod & KMOD_CTRL)) {
                    // Clear pattern
                    memset(beat.cells, 0, sizeof(beat.cells));
                    beat.loop_end_bar = 1;
                    beat.playhead = 0.0f;
                    beat.current_step = -1;
                    beat.playing = 0;
                } else if (e.key.keysym.sym == SDLK_RETURN && beat.editing_name) {
                    // Finish editing name
                    beat.editing_name = 0;
                    SDL_StopTextInput();
                } else if (e.key.keysym.sym == SDLK_BACKSPACE && beat.editing_name) {
                    // Backspace
                    int len = strlen(beat.pattern_name);
                    if (len > 0 && beat.name_cursor_pos > 0) {
                        memmove(&beat.pattern_name[beat.name_cursor_pos - 1], 
                                &beat.pattern_name[beat.name_cursor_pos],
                                len - beat.name_cursor_pos);
                        beat.pattern_name[len - 1] = '\0';
                        beat.name_cursor_pos--;
                    }
                } else if (e.key.keysym.sym == SDLK_LEFT && beat.editing_name) {
                    if (beat.name_cursor_pos > 0) beat.name_cursor_pos--;
                } else if (e.key.keysym.sym == SDLK_RIGHT && beat.editing_name) {
                    if (beat.name_cursor_pos < (int)strlen(beat.pattern_name)) beat.name_cursor_pos++;
                } else if (e.key.keysym.sym == SDLK_ESCAPE && beat.editing_name) {
                    beat.editing_name = 0;
                    SDL_StopTextInput();
                }
            } else if (e.type == SDL_TEXTINPUT && beat.editing_name) {
                // Handle text input
                int len = strlen(beat.pattern_name);
                if (len < 63) {
                    // Insert character at cursor
                    memmove(&beat.pattern_name[beat.name_cursor_pos + 1],
                            &beat.pattern_name[beat.name_cursor_pos],
                            len - beat.name_cursor_pos + 1);
                    beat.pattern_name[beat.name_cursor_pos] = e.text.text[0];
                    beat.name_cursor_pos++;
                }
            } else if (e.type == SDL_MOUSEBUTTONDOWN) {
                if (e.button.button == SDL_BUTTON_LEFT) {
                    handle_click(&beat, e.button.x, e.button.y, width, height);
                }
            }
        }
        
        Uint32 current_time = SDL_GetTicks();
        float dt = (current_time - last_time) / 1000.0f;
        last_time = current_time;
        
        update_beat_pattern(&beat, dt);
        render_grid(renderer, &beat, width, height);
        SDL_RenderPresent(renderer);
    }
    
    cleanup_beat_pattern(&beat);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    Mix_CloseAudio();
    SDL_Quit();
    
    return 0;
}

