/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    musical_keyboard.c
  * @brief   Musical keyboard implementation for Lab 3
  ******************************************************************************
  */
/* USER CODE END Header */

#include "musical_keyboard.h"
#include "tim.h"

/* Note names for display */
const char* note_names[7] = {"Do", "Re", "Mi", "Fa", "Sol", "La", "Si"};

/* Playback state machine */
typedef enum {
    STATE_IDLE,
    STATE_PLAYING_SINGLE,
    STATE_PLAYING_SCALE
} PlaybackState;

/* Module state variables */
static PlaybackState current_state = STATE_IDLE;
static uint32_t play_start_time = 0;
static uint16_t play_duration_ms = 0;
static uint8_t scale_octave = 0;
static uint8_t scale_current_note = 0;
static uint16_t scale_note_duration = 0;

/* ARR values for TIM1 @ PSC=899 (120MHz timer clock)
 * Formula: ARR = 120,000,000 / (PSC+1) / freq - 1
 * PSC = 899, so ARR = 133,333 / freq - 1
 *
 * Frequency table: [octave][note]
 * Octaves: 0=subcontra, 1=contra, 2=great, 3=small, 4=1st, 5=2nd, 6=3rd, 7=4th, 8=5th
 * Base frequencies (octave 4): Do=261, Re=293, Mi=329, Fa=349, Sol=392, La=440, Si=493 Hz
 */
static const uint16_t note_arr_table[9][7] = {
    /* Octave 0 (subcontra): freq/16 relative to octave 4 */
    {8164, 7273, 6477, 6101, 5433, 4840, 4314},  /* Do=16.3Hz, Re=18.3Hz, Mi=20.6Hz, Fa=21.8Hz, Sol=24.5Hz, La=27.5Hz, Si=30.8Hz */

    /* Octave 1 (contra): freq/8 relative to octave 4 */
    {4082, 3636, 3238, 3050, 2716, 2420, 2157},  /* Do=32.7Hz, Re=36.7Hz, Mi=41.2Hz, Fa=43.7Hz, Sol=49.0Hz, La=55.0Hz, Si=61.7Hz */

    /* Octave 2 (great): freq/4 relative to octave 4 */
    {2040, 1817, 1618, 1524, 1357, 1209, 1077},  /* Do=65.4Hz, Re=73.4Hz, Mi=82.4Hz, Fa=87.3Hz, Sol=98.0Hz, La=110Hz, Si=123Hz */

    /* Octave 3 (small): freq/2 relative to octave 4 */
    {1020, 908, 808, 762, 678, 604, 538},  /* Do=131Hz, Re=147Hz, Mi=165Hz, Fa=175Hz, Sol=196Hz, La=220Hz, Si=247Hz */

    /* Octave 4 (1st octave - base frequencies) */
    {510, 454, 404, 381, 339, 302, 270},  /* Do=261Hz, Re=293Hz, Mi=329Hz, Fa=349Hz, Sol=392Hz, La=440Hz, Si=491Hz */

    /* Octave 5 (2nd octave): freq*2 relative to octave 4 */
    {254, 226, 201, 190, 169, 150, 134},  /* Do=523Hz, Re=587Hz, Mi=659Hz, Fa=699Hz, Sol=784Hz, La=881Hz, Si=987Hz */

    /* Octave 6 (3rd octave): freq*4 relative to octave 4 */
    {126, 112, 100, 94, 84, 75, 67},  /* Do=1047Hz, Re=1175Hz, Mi=1319Hz, Fa=1397Hz, Sol=1568Hz, La=1760Hz, Si=1976Hz */

    /* Octave 7 (4th octave): freq*8 relative to octave 4 */
    {62, 56, 49, 46, 41, 37, 33},  /* Do=2093Hz, Re=2349Hz, Mi=2637Hz, Fa=2794Hz, Sol=3136Hz, La=3520Hz, Si=3951Hz */

    /* Octave 8 (5th octave): freq*16 relative to octave 4 */
    {30, 27, 24, 23, 20, 18, 16}  /* Do=4186Hz, Re=4699Hz, Mi=5274Hz, Fa=5588Hz, Sol=6272Hz, La=7040Hz, Si=7902Hz */
};

/**
 * @brief Set buzzer frequency by updating TIM1 ARR register
 * @param arr_value Auto-reload register value (from lookup table)
 */
static void set_buzzer_frequency(uint16_t arr_value) {
    htim1.Instance->ARR = arr_value;
    htim1.Instance->CCR1 = arr_value / 2;  /* 50% duty cycle */
}

/**
 * @brief Turn buzzer on (start PWM)
 */
static void buzzer_on(void) {
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
}

/**
 * @brief Turn buzzer off (stop PWM)
 */
static void buzzer_off(void) {
    HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
}

/* Public functions */

void musical_keyboard_init(void) {
    current_state = STATE_IDLE;
    buzzer_off();
}

void play_note(uint8_t note, uint8_t octave, uint16_t duration_ms) {
    /* Validate parameters */
    if (note > 6 || octave > 8) {
        return;
    }

    /* Stop any current playback */
    stop_note();

    /* Set frequency and start playing */
    set_buzzer_frequency(note_arr_table[octave][note]);
    buzzer_on();

    /* Update state */
    current_state = STATE_PLAYING_SINGLE;
    play_start_time = HAL_GetTick();
    play_duration_ms = duration_ms;
}

void play_scale(uint8_t octave, uint16_t duration_ms) {
    /* Validate parameters */
    if (octave > 8) {
        return;
    }

    /* Stop any current playback */
    stop_note();

    /* Start scale playback */
    scale_octave = octave;
    scale_current_note = 0;
    scale_note_duration = duration_ms;

    /* Play first note */
    set_buzzer_frequency(note_arr_table[octave][0]);
    buzzer_on();

    current_state = STATE_PLAYING_SCALE;
    play_start_time = HAL_GetTick();
}

void stop_note(void) {
    buzzer_off();
    current_state = STATE_IDLE;
}

void musical_keyboard_update(void) {
    uint32_t current_time = HAL_GetTick();

    switch (current_state) {
        case STATE_PLAYING_SINGLE:
            /* Check if note duration has elapsed */
            if (current_time - play_start_time >= play_duration_ms) {
                stop_note();
            }
            break;

        case STATE_PLAYING_SCALE:
            /* Check if current note duration has elapsed */
            if (current_time - play_start_time >= scale_note_duration) {
                /* Move to next note */
                scale_current_note++;

                if (scale_current_note >= 7) {
                    /* Scale complete */
                    stop_note();
                } else {
                    /* Play next note */
                    set_buzzer_frequency(note_arr_table[scale_octave][scale_current_note]);
                    buzzer_on();
                    play_start_time = current_time;
                }
            }
            break;

        case STATE_IDLE:
        default:
            /* Nothing to do */
            break;
    }
}

void musical_keyboard_timer_callback(void) {
    /* Timer callback - currently unused but available for future enhancements */
    /* Can be used for more precise timing if needed */
}
