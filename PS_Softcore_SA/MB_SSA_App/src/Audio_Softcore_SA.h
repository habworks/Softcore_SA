/******************************************************************************************************
 * @file            SoftCore_Audio_SA.h
 * @brief           Header file to support SoftCore_Audio_SA.c 
 * ****************************************************************************************************
 * @author          Hab Collector (habco)\n
 *
 * @version         See Main_Support.h: FW_MAJOR_REV, FW_MINOR_REV, FW_TEST_REV
 *
 * @param Development_Environment \n
 * Hardware:        <Xilinx Artix A7> \n
 * IDE:             Vitis 2024.2 \n
 * Compiler:        GCC \n
 * Editor Settings: 1 Tab = 4 Spaces, Recommended Courier New 11
 *
 * @note            The associated header file provides MACRO functions for IO control
 *
 *                  This is an embedded application
 *                  It will be necessary to consult the reference documents to fully understand the code
 *                  It is suggested that the documents be reviewed in the order shown.
 *                    Schematic: 
 *                    IMR Engineering
 *                    IMR Engineering
 *
 * @copyright       IMR Engineering, LLC
 ********************************************************************************************************/

#ifndef AUDIO_SOFTCORE_SA_H_
#define AUDIO_SOFTCORE_SA_H_
#ifdef __cplusplus
extern"C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "Audio_File_API.h"
#include "Main_Support.h"
#include "Circular_Buffers.h"


// DEFINES
#define DEFAULT_AUDIO_FREQUENCY 44100       // Value in Hz
#define AUDIO_PWM_FREQUENCY     100000      // Value in Hz
#define AUDIO_PWM_DEFAULT_DUTY  50          
#define CHUNK_MULTIPLIER        6
// #if ((CHUNK_MULTIPLIER / 2) * 2) != CHUNK_MULTIPLIER
//     #error "MULTIPLER must be even"
// #endif
// #if CHUNK_MULTIPLIER < 4
//     #error "CHUNK_MULTIPLIER must be >= 4 and be an even value
// #endif
#define MAX_RAW_BUFFER          (1024 * CHUNK_MULTIPLIER)
#define AUDIO_MIN_DB_DISPLAY    -60         // Value represents dB
// LCD DISPLAY RELATED
#define DISPLAY_AUDIO_HEADING   "AUDIO_SA"
#define DISPLAY_AUDIO_PLAY      "PLAY"
#define DISPLAY_AUDIO_STOP      "STOP"
#define DISPLAY_AUDIO_PAUSE     "PAUSE"
#define DISPLAY_AUDIO_ERROR     "ERROR"
#define DISPLAY_SIGNAL_HEADING  "SIGNAL_SA"     // ***Hab move to more approipate file
#define DISPLAY_FILE_ERROR      "No WAV File Found"
// LED DISPLAY RELATED
// LED BAR MASKS
#define LED_1_MASK  ((uint8_t)(1U << 0U))
#define LED_2_MASK  ((uint8_t)(1U << 1U))
#define LED_3_MASK  ((uint8_t)(1U << 2U))
#define LED_4_MASK  ((uint8_t)(1U << 3U))
#define LED_5_MASK  ((uint8_t)(1U << 4U))
#define LED_6_MASK  ((uint8_t)(1U << 5U))
#define LED_BAR_1   (LED_1_MASK)
#define LED_BAR_2   ((uint8_t)(LED_1_MASK | LED_2_MASK))
#define LED_BAR_3   ((uint8_t)(LED_1_MASK | LED_2_MASK | LED_3_MASK))
#define LED_BAR_4   ((uint8_t)(LED_1_MASK | LED_2_MASK | LED_3_MASK | LED_4_MASK))
#define LED_BAR_5   ((uint8_t)(LED_1_MASK | LED_2_MASK | LED_3_MASK | LED_4_MASK | LED_5_MASK))
#define LED_BAR_6   ((uint8_t)(LED_1_MASK | LED_2_MASK | LED_3_MASK | LED_4_MASK | LED_5_MASK | LED_6_MASK))
// LED BAR GRAPH LEVELS IN DB
#define LEVEL_30DB   1036u          // BASED ON 32768 FULL SCALE -30dB
#define LEVEL_26DB   1642u          // BASED ON 32768 FULL SCALE -26dB
#define LEVEL_22DB   2603u          // BASED ON 32768 FULL SCALE -22dB
#define LEVEL_18DB   4125u          // BASED ON 32768 FULL SCALE -18dB
#define LEVEL_14DB   6537u          // BASED ON 32768 FULL SCALE -14dB
#define LEVEL_10DB   10361u         // BASED ON 32768 FULL SCALE -10dB


// TYPEDEFS AND ENUMS
typedef struct
{
    float                       Samples[FFT_SIZE];
} Type_PWM;

typedef enum
{
    AUDIO_ACTION_STOP = 0,
    AUDIO_ACTION_PLAY,
    AUDIO_ACTION_PAUSE
} Type_AudioAction;

typedef struct
{
    bool                        Enable;
    bool                        IsPreLoadComplete;
    uint8_t                     LED_BarGraph;
    int16_t                     PresentValue_PCM16;
    uint32_t                    PlaybackTickCounter;
    Type_AudioAction            AudioAction;
    Type_AudioFile              File;
    volatile 
    Type_int16_t_CircularBuffer Samples_CB;
    Type_PWM                    PWM;
} Type_Audio_SA;


// FUNCTION PROTOTYPES
void audioSpectrumAnalyzer(Type_Audio_SA *Audio_SA, Type_FFT *FFT, uint8_t LED_ModeStatus);
void stopAudio_SA(Type_Audio_SA *Audio_SA, Type_FFT *FFT);
void playAudio_SA(Type_Audio_SA *Audio_SA, Type_FFT *FFT);
void pauseAudio_SA(Type_Audio_SA *Audio_SA);
void audioEnable(bool Enable);
void audioPeriodicTimer_ISR(Type_Audio_SA *Audio_SA, Type_FFT *FFT); 
void update_LED_AudioBarGraph(int16_t PCM16_Value, uint8_t LED_ModeStatusBitMask);

#ifdef __cplusplus
}
#endif
#endif /* AUDIO_SOFTCORE_SA_H_ */