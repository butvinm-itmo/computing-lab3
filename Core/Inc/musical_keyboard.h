/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    musical_keyboard.h
  * @brief   Musical keyboard module for Lab 3
  *          Implements note playback via buzzer using TIM1 PWM
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __MUSICAL_KEYBOARD_H__
#define __MUSICAL_KEYBOARD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

/* Note definitions */
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

/* Duration range in milliseconds */
#define MIN_DURATION_MS  100
#define MAX_DURATION_MS  5000
#define DEFAULT_DURATION_MS 1000
#define DURATION_STEP_MS 100

/* Note names for UART output */
extern const char* note_names[7];

/**
 * @brief Initialize the musical keyboard module
 */
void musical_keyboard_init(void);

/**
 * @brief Play a single note
 * @param note Note index (0-6: Do, Re, Mi, Fa, Sol, La, Si)
 * @param octave Octave number (0-8)
 * @param duration_ms Duration in milliseconds
 */
void play_note(uint8_t note, uint8_t octave, uint16_t duration_ms);

/**
 * @brief Play a complete scale (all 7 notes sequentially)
 * @param octave Octave number (0-8)
 * @param duration_ms Duration per note in milliseconds
 */
void play_scale(uint8_t octave, uint16_t duration_ms);

/**
 * @brief Stop current note playback
 */
void stop_note(void);

/**
 * @brief Update playback state (call from main loop)
 * Non-blocking function to manage note timing
 */
void musical_keyboard_update(void);

/**
 * @brief Timer callback (call from TIM6 interrupt)
 * Updates internal timing for note duration
 */
void musical_keyboard_timer_callback(void);

#ifdef __cplusplus
}
#endif

#endif /* __MUSICAL_KEYBOARD_H__ */
