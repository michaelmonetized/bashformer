// Simple MIDI file generator for 8-bit style background music
// Compile: gcc generate_midi.c -o generate_midi
// Usage: ./generate_midi output.mid

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// MIDI file format helpers
static void write_uint16_be(FILE *f, uint16_t value) {
    fputc((value >> 8) & 0xFF, f);
    fputc(value & 0xFF, f);
}

static void write_uint32_be(FILE *f, uint32_t value) {
    fputc((value >> 24) & 0xFF, f);
    fputc((value >> 16) & 0xFF, f);
    fputc((value >> 8) & 0xFF, f);
    fputc(value & 0xFF, f);
}

static void write_var_length(FILE *f, uint32_t value) {
    if (value < 0x80) {
        fputc(value, f);
    } else if (value < 0x4000) {
        fputc((value >> 7) | 0x80, f);
        fputc(value & 0x7F, f);
    } else if (value < 0x200000) {
        fputc((value >> 14) | 0x80, f);
        fputc((value >> 7) | 0x80, f);
        fputc(value & 0x7F, f);
    } else {
        fputc((value >> 21) | 0x80, f);
        fputc((value >> 14) | 0x80, f);
        fputc((value >> 7) | 0x80, f);
        fputc(value & 0x7F, f);
    }
}

static void write_midi_event(FILE *f, uint32_t delta_time, uint8_t status, uint8_t data1, uint8_t data2) {
    write_var_length(f, delta_time);
    fputc(status, f);
    fputc(data1, f);
    fputc(data2, f);
}

// Generate a simple 8-bit style looping MIDI file
int main(int argc, char **argv) {
    const char *filename = (argc > 1) ? argv[1] : "bgmusic.mid";
    
    FILE *f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "Error: Could not create %s\n", filename);
        return 1;
    }
    
    // MIDI Header Chunk
    fwrite("MThd", 1, 4, f);           // Chunk type
    write_uint32_be(f, 6);              // Chunk length
    write_uint16_be(f, 1);              // Format (0=single track, 1=multi-track, 2=multi-song)
    write_uint16_be(f, 1);              // Number of tracks
    write_uint16_be(f, 480);            // Ticks per quarter note (PPQ)
    
    // Track Chunk
    // First, write placeholder for chunk size (we'll come back to fix it)
    long track_start = ftell(f);
    fwrite("MTrk", 1, 4, f);           // Chunk type
    long track_size_pos = ftell(f);
    write_uint32_be(f, 0);              // Placeholder for chunk size
    
    // Track events
    long track_data_start = ftell(f);
    
    // Set tempo (120 BPM)
    write_var_length(f, 0);
    fputc(0xFF, f);                      // Meta event
    fputc(0x51, f);                      // Set tempo
    fputc(3, f);                         // Data length
    fputc(0x07, f);                      // 500000 microseconds per quarter note (120 BPM)
    fputc(0xA1, f);
    fputc(0x20, f);
    
    // Set time signature (4/4)
    write_var_length(f, 0);
    fputc(0xFF, f);                      // Meta event
    fputc(0x58, f);                      // Time signature
    fputc(4, f);                         // Data length
    fputc(4, f);                         // Numerator
    fputc(2, f);                         // Denominator (2^2 = 4)
    fputc(24, f);                        // MIDI clocks per metronome click
    fputc(8, f);                         // 32nd notes per quarter note
    
    // Simple 8-bit style melody - two channels for harmony
    // Channel 0: Main melody
    // Channel 1: Bass/harmony
    
    // Program change - set instrument (square wave / chiptune style)
    write_var_length(f, 0);
    fputc(0xC0, f);                      // Program change on channel 0
    fputc(80, f);                        // Lead 7 (square) - chiptune style
    
    write_var_length(f, 0);
    fputc(0xC1, f);                      // Program change on channel 1
    fputc(33, f);                        // Electric Bass
    
    // Simple looping melody pattern (C major scale, 8-bit style)
    // Pattern repeats: C E G C | E G C E | G C E G | C (restart)
    
    // Notes: C4=60, E4=64, G4=67, C5=72
    int notes[] = {60, 64, 67, 72, 64, 67, 72, 76, 67, 72, 76, 79, 72}; // 13 notes
    int note_count = 13;
    int ticks_per_note = 480; // 1 quarter note per note
    
    for (int repeat = 0; repeat < 4; repeat++) { // Repeat pattern 4 times
        for (int i = 0; i < note_count; i++) {
            // Note on
            write_var_length(f, (i == 0 && repeat == 0) ? 0 : ticks_per_note);
            write_midi_event(f, 0, 0x90, notes[i], 80); // Note on, channel 0, velocity 80
            
            // Add harmony (lower octave)
            if (i % 2 == 0) {
                write_var_length(f, 0);
                write_midi_event(f, 0, 0x91, notes[i] - 12, 60); // Harmony channel 1
            }
        }
        
        // Turn off last notes
        write_var_length(f, ticks_per_note);
        write_midi_event(f, 0, 0x80, notes[note_count - 1], 0); // Note off
        write_var_length(f, 0);
        write_midi_event(f, 0, 0x81, notes[note_count - 1] - 12, 0); // Harmony off
    }
    
    // End of track
    write_var_length(f, 0);
    fputc(0xFF, f);                      // Meta event
    fputc(0x2F, f);                      // End of track
    fputc(0, f);                         // Data length
    
    // Fix track chunk size
    long track_end = ftell(f);
    long track_size = track_end - track_data_start;
    fseek(f, track_size_pos, SEEK_SET);
    write_uint32_be(f, track_size);
    fseek(f, track_end, SEEK_SET);
    
    fclose(f);
    
    printf("Generated MIDI file: %s\n", filename);
    printf("Simple 8-bit style looping music ready!\n");
    printf("Note: For more complex music, consider using:\n");
    printf("  - OpenMusic.ai (free AI MIDI generator)\n");
    printf("  - MIDI Muse (AI-powered)\n");
    printf("  - Manual composition tools like MuseScore or LMMS\n");
    
    return 0;
}

