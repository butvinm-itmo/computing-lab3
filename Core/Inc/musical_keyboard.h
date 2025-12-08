#ifndef __MUSICAL_KEYBOARD_H__
#define __MUSICAL_KEYBOARD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

#define NOTE_DO   0
#define NOTE_RE   1
#define NOTE_MI   2
#define NOTE_FA   3
#define NOTE_SOL  4
#define NOTE_LA   5
#define NOTE_SI   6

/* Octave range: 0 (subcontra) to 8 (5th octave) */
#define MIN_OCTAVE 0
#define MAX_OCTAVE 8
#define DEFAULT_OCTAVE 4  /* 1st octave */

#define MIN_DURATION_MS  100
#define MAX_DURATION_MS  5000
#define DEFAULT_DURATION_MS 1000
#define DURATION_STEP_MS 100

extern const char* note_names[7];

void musical_keyboard_init(void);
void play_note(uint8_t note, uint8_t octave, uint16_t duration_ms);
void play_scale(uint8_t octave, uint16_t duration_ms);
void stop_note(void);
void musical_keyboard_update(void);

#ifdef __cplusplus
}
#endif

#endif /* __MUSICAL_KEYBOARD_H__ */
