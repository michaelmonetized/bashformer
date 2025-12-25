// Simple WAV sound effect generator for 8-bit style game sounds
// Compile: gcc generate_sfx.c -o generate_sfx -lm
// Usage: ./generate_sfx

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

#define SAMPLE_RATE 22050
#define BITS_PER_SAMPLE 16
#define CHANNELS 1

// WAV file header structure
typedef struct {
    char riff[4];           // "RIFF"
    uint32_t file_size;     // File size - 8
    char wave[4];           // "WAVE"
    char fmt[4];            // "fmt "
    uint32_t fmt_size;      // 16 for PCM
    uint16_t audio_format;  // 1 for PCM
    uint16_t num_channels;  // 1 for mono
    uint32_t sample_rate;   // 22050
    uint32_t byte_rate;     // sample_rate * num_channels * bits_per_sample / 8
    uint16_t block_align;   // num_channels * bits_per_sample / 8
    uint16_t bits_per_sample; // 16
    char data[4];           // "data"
    uint32_t data_size;     // Size of audio data
} WAVHeader;

static void write_wav_header(FILE *f, uint32_t data_size) {
    WAVHeader header;
    memcpy(header.riff, "RIFF", 4);
    header.file_size = 36 + data_size;
    memcpy(header.wave, "WAVE", 4);
    memcpy(header.fmt, "fmt ", 4);
    header.fmt_size = 16;
    header.audio_format = 1; // PCM
    header.num_channels = CHANNELS;
    header.sample_rate = SAMPLE_RATE;
    header.byte_rate = SAMPLE_RATE * CHANNELS * BITS_PER_SAMPLE / 8;
    header.block_align = CHANNELS * BITS_PER_SAMPLE / 8;
    header.bits_per_sample = BITS_PER_SAMPLE;
    memcpy(header.data, "data", 4);
    header.data_size = data_size;
    
    fwrite(&header, sizeof(WAVHeader), 1, f);
}

static void write_sample(FILE *f, int16_t sample) {
    fwrite(&sample, sizeof(int16_t), 1, f);
}

// Generate a square wave tone
static void generate_tone(FILE *f, float freq, float duration, float volume) {
    int samples = (int)(SAMPLE_RATE * duration);
    for (int i = 0; i < samples; i++) {
        float t = (float)i / SAMPLE_RATE;
        float value = (sinf(2.0f * M_PI * freq * t) > 0.0f) ? 1.0f : -1.0f;
        int16_t sample = (int16_t)(value * volume * 32767.0f);
        write_sample(f, sample);
    }
}

// Generate a triangle wave tone
static void generate_triangle(FILE *f, float freq, float duration, float volume) {
    int samples = (int)(SAMPLE_RATE * duration);
    for (int i = 0; i < samples; i++) {
        float t = (float)i / SAMPLE_RATE;
        float phase = fmodf(t * freq, 1.0f);
        float value = (phase < 0.5f) ? (4.0f * phase - 1.0f) : (3.0f - 4.0f * phase);
        int16_t sample = (int16_t)(value * volume * 32767.0f);
        write_sample(f, sample);
    }
}

// Generate white noise
static void generate_noise(FILE *f, float duration, float volume) {
    int samples = (int)(SAMPLE_RATE * duration);
    for (int i = 0; i < samples; i++) {
        float value = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
        int16_t sample = (int16_t)(value * volume * 32767.0f);
        write_sample(f, sample);
    }
}

// Generate a frequency sweep (slide up or down)
static void generate_sweep(FILE *f, float start_freq, float end_freq, float duration, float volume) {
    int samples = (int)(SAMPLE_RATE * duration);
    for (int i = 0; i < samples; i++) {
        float t = (float)i / samples;
        float freq = start_freq + (end_freq - start_freq) * t;
        float phase = fmodf((float)i * freq / SAMPLE_RATE, 1.0f);
        float value = (phase < 0.5f) ? 1.0f : -1.0f; // Square wave
        int16_t sample = (int16_t)(value * volume * 32767.0f);
        write_sample(f, sample);
    }
}

// Generate a chord (multiple frequencies)
static void generate_chord(FILE *f, float *freqs, int num_freqs, float duration, float volume) {
    int samples = (int)(SAMPLE_RATE * duration);
    for (int i = 0; i < samples; i++) {
        float t = (float)i / SAMPLE_RATE;
        float value = 0.0f;
        for (int j = 0; j < num_freqs; j++) {
            value += sinf(2.0f * M_PI * freqs[j] * t) / num_freqs;
        }
        int16_t sample = (int16_t)(value * volume * 32767.0f);
        write_sample(f, sample);
    }
}

// Generate kick drum - low frequency thump with attack
static void generate_kick(FILE *f, float volume) {
    int samples = (int)(SAMPLE_RATE * 0.15f); // 150ms kick
    for (int i = 0; i < samples; i++) {
        float t = (float)i / SAMPLE_RATE;
        // Start at high freq, sweep down to low (808 style)
        float freq = 60.0f + 40.0f * expf(-t * 30.0f); // Exponential decay
        float phase = fmodf((float)i * freq / SAMPLE_RATE, 1.0f);
        float value = (phase < 0.5f) ? 1.0f : -1.0f; // Square wave
        // Add envelope (quick attack, decay)
        float envelope = expf(-t * 8.0f);
        int16_t sample = (int16_t)(value * envelope * volume * 32767.0f);
        write_sample(f, sample);
    }
}

// Generate snare/clap - sharp noise burst
static void generate_snare(FILE *f, float volume) {
    int samples = (int)(SAMPLE_RATE * 0.1f); // 100ms snare
    for (int i = 0; i < samples; i++) {
        float t = (float)i / SAMPLE_RATE;
        // Mix of noise and tone
        float noise = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
        float tone = sinf(2.0f * M_PI * 200.0f * t); // 200Hz tone
        float value = (noise * 0.7f + tone * 0.3f);
        // Quick decay envelope
        float envelope = expf(-t * 15.0f);
        int16_t sample = (int16_t)(value * envelope * volume * 32767.0f);
        write_sample(f, sample);
    }
}

// Generate hi-hat - high frequency tick
static void generate_hihat(FILE *f, float volume) {
    int samples = (int)(SAMPLE_RATE * 0.05f); // 50ms hi-hat
    for (int i = 0; i < samples; i++) {
        float t = (float)i / SAMPLE_RATE;
        // High frequency noise with quick decay
        float noise = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
        float envelope = expf(-t * 30.0f); // Very quick decay
        int16_t sample = (int16_t)(noise * envelope * volume * 32767.0f);
        write_sample(f, sample);
    }
}

// Generate silence
static void generate_silence(FILE *f, float duration) {
    int samples = (int)(SAMPLE_RATE * duration);
    for (int i = 0; i < samples; i++) {
        write_sample(f, 0);
    }
}

// Generate kill baddie sound - quick hit with explosion
static void generate_kill_baddie(const char *filename) {
    FILE *f = fopen(filename, "wb");
    if (!f) return;
    
    long data_start = ftell(f) + sizeof(WAVHeader);
    fseek(f, sizeof(WAVHeader), SEEK_SET);
    
    // Quick hit sound
    generate_tone(f, 400.0f, 0.05f, 0.8f);
    generate_tone(f, 300.0f, 0.05f, 0.6f);
    // Explosion noise
    generate_noise(f, 0.15f, 0.4f);
    
    long data_end = ftell(f);
    uint32_t data_size = data_end - data_start;
    fseek(f, 0, SEEK_SET);
    write_wav_header(f, data_size);
    fclose(f);
    printf("Generated: %s\n", filename);
}

// Generate break barrel sound - wood crack
static void generate_break_barrel(const char *filename) {
    FILE *f = fopen(filename, "wb");
    if (!f) return;
    
    long data_start = ftell(f) + sizeof(WAVHeader);
    fseek(f, sizeof(WAVHeader), SEEK_SET);
    
    // Wood crack sound - quick noise burst
    generate_noise(f, 0.1f, 0.6f);
    generate_tone(f, 200.0f, 0.05f, 0.4f);
    generate_tone(f, 150.0f, 0.05f, 0.3f);
    
    long data_end = ftell(f);
    uint32_t data_size = data_end - data_start;
    fseek(f, 0, SEEK_SET);
    write_wav_header(f, data_size);
    fclose(f);
    printf("Generated: %s\n", filename);
}

// Generate get coin sound - bright chime
static void generate_get_coin(const char *filename) {
    FILE *f = fopen(filename, "wb");
    if (!f) return;
    
    long data_start = ftell(f) + sizeof(WAVHeader);
    fseek(f, sizeof(WAVHeader), SEEK_SET);
    
    // Bright chime - ascending notes
    generate_tone(f, 523.25f, 0.05f, 0.7f); // C5
    generate_tone(f, 659.25f, 0.05f, 0.7f); // E5
    generate_tone(f, 783.99f, 0.1f, 0.7f);  // G5
    
    long data_end = ftell(f);
    uint32_t data_size = data_end - data_start;
    fseek(f, 0, SEEK_SET);
    write_wav_header(f, data_size);
    fclose(f);
    printf("Generated: %s\n", filename);
}

// Generate extra life sound - fanfare
static void generate_extra_life(const char *filename) {
    FILE *f = fopen(filename, "wb");
    if (!f) return;
    
    long data_start = ftell(f) + sizeof(WAVHeader);
    fseek(f, sizeof(WAVHeader), SEEK_SET);
    
    // Fanfare - ascending chord
    float chord1[] = {523.25f, 659.25f, 783.99f}; // C major
    float chord2[] = {659.25f, 783.99f, 987.77f}; // E major
    generate_chord(f, chord1, 3, 0.15f, 0.6f);
    generate_chord(f, chord2, 3, 0.15f, 0.6f);
    generate_tone(f, 1046.50f, 0.2f, 0.7f); // C6
    
    long data_end = ftell(f);
    uint32_t data_size = data_end - data_start;
    fseek(f, 0, SEEK_SET);
    write_wav_header(f, data_size);
    fclose(f);
    printf("Generated: %s\n", filename);
}

// Generate get flame sound - fire whoosh
static void generate_get_flame(const char *filename) {
    FILE *f = fopen(filename, "wb");
    if (!f) return;
    
    long data_start = ftell(f) + sizeof(WAVHeader);
    fseek(f, sizeof(WAVHeader), SEEK_SET);
    
    // Fire whoosh - upward sweep with noise
    generate_sweep(f, 200.0f, 600.0f, 0.2f, 0.5f);
    generate_noise(f, 0.1f, 0.3f);
    
    long data_end = ftell(f);
    uint32_t data_size = data_end - data_start;
    fseek(f, 0, SEEK_SET);
    write_wav_header(f, data_size);
    fclose(f);
    printf("Generated: %s\n", filename);
}

// Generate get super beast sound - power up
static void generate_get_superbeast(const char *filename) {
    FILE *f = fopen(filename, "wb");
    if (!f) return;
    
    long data_start = ftell(f) + sizeof(WAVHeader);
    fseek(f, sizeof(WAVHeader), SEEK_SET);
    
    // Power up - ascending sweep with chord
    generate_sweep(f, 150.0f, 800.0f, 0.3f, 0.6f);
    float power_chord[] = {392.00f, 493.88f, 587.33f}; // G major
    generate_chord(f, power_chord, 3, 0.2f, 0.7f);
    
    long data_end = ftell(f);
    uint32_t data_size = data_end - data_start;
    fseek(f, 0, SEEK_SET);
    write_wav_header(f, data_size);
    fclose(f);
    printf("Generated: %s\n", filename);
}

// Generate win level sound - victory fanfare
static void generate_win_level(const char *filename) {
    FILE *f = fopen(filename, "wb");
    if (!f) return;
    
    long data_start = ftell(f) + sizeof(WAVHeader);
    fseek(f, sizeof(WAVHeader), SEEK_SET);
    
    // Victory fanfare - classic pattern
    generate_tone(f, 523.25f, 0.1f, 0.7f); // C
    generate_tone(f, 659.25f, 0.1f, 0.7f); // E
    generate_tone(f, 783.99f, 0.1f, 0.7f); // G
    generate_tone(f, 1046.50f, 0.3f, 0.8f); // C (high)
    
    long data_end = ftell(f);
    uint32_t data_size = data_end - data_start;
    fseek(f, 0, SEEK_SET);
    write_wav_header(f, data_size);
    fclose(f);
    printf("Generated: %s\n", filename);
}

// Generate win game sound - grand finale
static void generate_win_game(const char *filename) {
    FILE *f = fopen(filename, "wb");
    if (!f) return;
    
    long data_start = ftell(f) + sizeof(WAVHeader);
    fseek(f, sizeof(WAVHeader), SEEK_SET);
    
    // Grand finale - longer fanfare
    float finale1[] = {523.25f, 659.25f, 783.99f}; // C major
    float finale2[] = {659.25f, 783.99f, 987.77f}; // E major  
    float finale3[] = {783.99f, 987.77f, 1174.66f}; // G major
    generate_chord(f, finale1, 3, 0.2f, 0.7f);
    generate_chord(f, finale2, 3, 0.2f, 0.7f);
    generate_chord(f, finale3, 3, 0.2f, 0.7f);
    generate_tone(f, 1046.50f, 0.4f, 0.8f); // High C
    
    long data_end = ftell(f);
    uint32_t data_size = data_end - data_start;
    fseek(f, 0, SEEK_SET);
    write_wav_header(f, data_size);
    fclose(f);
    printf("Generated: %s\n", filename);
}

// Generate die sound - descending sad tone
static void generate_die(const char *filename) {
    FILE *f = fopen(filename, "wb");
    if (!f) return;
    
    long data_start = ftell(f) + sizeof(WAVHeader);
    fseek(f, sizeof(WAVHeader), SEEK_SET);
    
    // Sad descending tone
    generate_tone(f, 400.0f, 0.1f, 0.6f);
    generate_sweep(f, 400.0f, 200.0f, 0.3f, 0.5f);
    generate_tone(f, 200.0f, 0.1f, 0.4f);
    
    long data_end = ftell(f);
    uint32_t data_size = data_end - data_start;
    fseek(f, 0, SEEK_SET);
    write_wav_header(f, data_size);
    fclose(f);
    printf("Generated: %s\n", filename);
}

// Generate lose game sound - game over
static void generate_lose_game(const char *filename) {
    FILE *f = fopen(filename, "wb");
    if (!f) return;
    
    long data_start = ftell(f) + sizeof(WAVHeader);
    fseek(f, sizeof(WAVHeader), SEEK_SET);
    
    // Game over - dramatic low tone
    generate_tone(f, 150.0f, 0.2f, 0.7f);
    generate_tone(f, 100.0f, 0.2f, 0.6f);
    generate_tone(f, 80.0f, 0.3f, 0.5f);
    
    long data_end = ftell(f);
    uint32_t data_size = data_end - data_start;
    fseek(f, 0, SEEK_SET);
    write_wav_header(f, data_size);
    fclose(f);
    printf("Generated: %s\n", filename);
}

// Generate background music - looping 8-bit style track with EDM trap backbeat
static void generate_bgmusic(const char *filename) {
    FILE *f = fopen(filename, "wb");
    if (!f) return;
    
    long data_start = ftell(f) + sizeof(WAVHeader);
    fseek(f, sizeof(WAVHeader), SEEK_SET);
    
    // Trap beat pattern - 16th note grid (140 BPM = ~0.107s per 16th note)
    // K = Kick, S = Snare, H = Hi-hat, . = silence
    // Classic trap pattern: Kick on 1, 5, 9, 13, Snare on 5, 13, Hi-hats on off-beats
    const char *beat_pattern = "K...K...S...K...K...K...S...K...";
    int pattern_length = 32; // 32 16th notes = 2 bars
    float sixteenth_note = 60.0f / 140.0f / 4.0f; // ~0.107s per 16th note at 140 BPM
    float total_duration = pattern_length * sixteenth_note; // ~3.4 seconds per pattern
    
    // Melody notes (C major scale)
    int notes[] = {60, 64, 67, 72, 64, 67, 72, 76, 67, 72, 76, 79, 72, 76, 79, 84};
    int note_count = 16;
    float note_duration = sixteenth_note * 4; // Quarter note = 4 sixteenths
    
    // Generate 2 full patterns (~6.8 seconds total, will loop)
    for (int repeat = 0; repeat < 2; repeat++) {
        int total_samples = (int)(SAMPLE_RATE * total_duration);
        
        for (int sample_idx = 0; sample_idx < total_samples; sample_idx++) {
            float t = (float)sample_idx / SAMPLE_RATE;
            float melody = 0.0f;
            float beat = 0.0f;
            
            // Generate melody (quarter note timing)
            int note_idx = ((int)(t / note_duration)) % note_count;
            float note_t = fmodf(t, note_duration);
            float freq = 440.0f * powf(2.0f, (notes[note_idx] - 69) / 12.0f);
            float harmony_freq = freq * 0.5f;
            
            float value1 = sinf(2.0f * M_PI * freq * t);
            float value2 = sinf(2.0f * M_PI * harmony_freq * t);
            melody = (value1 * 0.6f + value2 * 0.4f) * 0.3f; // Lower volume for background
            
            // Generate trap beat
            int beat_pos = ((int)(t / sixteenth_note)) % pattern_length;
            char beat_char = beat_pattern[beat_pos];
            float beat_t = fmodf(t, sixteenth_note);
            
            if (beat_char == 'K') {
                // Kick drum - 808 style with pitch sweep
                if (beat_t < 0.15f) {
                    float kick_freq = 60.0f + 40.0f * expf(-beat_t * 30.0f);
                    float kick_phase = fmodf(t * kick_freq, 1.0f);
                    float kick_val = (kick_phase < 0.5f) ? 1.0f : -1.0f;
                    float kick_env = expf(-beat_t * 8.0f);
                    beat += kick_val * kick_env * 0.7f;
                }
            } else if (beat_char == 'S') {
                // Snare - sharp attack
                if (beat_t < 0.1f) {
                    float snare_noise = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
                    float snare_tone = sinf(2.0f * M_PI * 200.0f * t);
                    float snare_val = (snare_noise * 0.7f + snare_tone * 0.3f);
                    float snare_env = expf(-beat_t * 15.0f);
                    beat += snare_val * snare_env * 0.6f;
                }
            } else if (beat_char == 'H') {
                // Hi-hat - quick tick
                if (beat_t < 0.05f) {
                    float hat_noise = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
                    float hat_env = expf(-beat_t * 30.0f);
                    beat += hat_noise * hat_env * 0.4f;
                }
            }
            
            // Add continuous hi-hat pattern on off-beats (trap style)
            if (beat_pos % 2 == 1 && beat_t < 0.03f) {
                float hat_noise = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
                beat += hat_noise * 0.2f;
            }
            
            // Mix melody and beat
            float mixed = melody + beat;
            // Clamp to prevent clipping
            if (mixed > 1.0f) mixed = 1.0f;
            if (mixed < -1.0f) mixed = -1.0f;
            
            int16_t sample = (int16_t)(mixed * 32767.0f);
            write_sample(f, sample);
        }
    }
    
    long data_end = ftell(f);
    uint32_t data_size = data_end - data_start;
    fseek(f, 0, SEEK_SET);
    write_wav_header(f, data_size);
    fclose(f);
    printf("Generated: %s\n", filename);
}

int main(int argc, char **argv) {
    printf("Generating 8-bit style sound effects...\n\n");
    
    generate_kill_baddie("sfx_kill_baddie.wav");
    generate_break_barrel("sfx_break_barrel.wav");
    generate_get_coin("sfx_get_coin.wav");
    generate_extra_life("sfx_extra_life.wav");
    generate_get_flame("sfx_get_flame.wav");
    generate_get_superbeast("sfx_get_superbeast.wav");
    generate_win_level("sfx_win_level.wav");
    generate_win_game("sfx_win_game.wav");
    generate_die("sfx_die.wav");
    generate_lose_game("sfx_lose_game.wav");
    
    // Generate background music (WAV format for compatibility)
    generate_bgmusic("bgmusic.wav");
    
    printf("\nAll sound effects and background music generated!\n");
    return 0;
}

