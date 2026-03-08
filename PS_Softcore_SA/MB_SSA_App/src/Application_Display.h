/******************************************************************************************************
 * @file            Application_Display.h
 * @brief           Header file to support Application_Display.c
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

#ifndef APPLICATION_DISPLAY_H_
#define APPLICATION_DISPLAY_H_
#ifdef __cplusplus
extern"C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "SSD1309_Driver.h"
#include "Signal_SoftCore_SA.h"

// DEFINES
// FOR USE WITH THE AUDIO SPECTRUM TIME
// TIME:
#define TIME_X                  0U
#define TIME_BASELINE_Y         37U
#define TIME_FONT_H             8U          // 5x8 font height
#define TIME_BOX_W              (5U * 6U)   // conservative for 5x8 font width - 6 chars long
#define TIME_BOX_H              9U
// ACTION:
#define ACTION_X                0U
#define ACTION_BASELINE_Y       28U
#define ACTION_FONT_H           8U          // 5x8 font height
#define ACTION_BOX_W            (5U * 8U)   // conservative for 5x8 font width - 8 chars long
#define ACTION_BOX_H            9U          // 1px margin
// SPECTRUM:
#define SPECTRUM_ERASE_WIDTH    111U  
// FOR USE WITH THE SIGNAL SPECTRUM
// SIGNAL SOURCE    
#define SOURCE_X                0U
#define SOURCE_BASELINE_Y       19U
#define SOURCE_FONT_H           8U          // 5x8 font height
#define SOURCE_BOX_W            (5U * 13U)  // conservative for 5x8 font width - 10 chars long
#define SOURCE_BOX_H            9U          // 1px margin


// TYPEDEFS AND ENUMS


// FUNCTION PROTOTYPES
// TEST SCREENS
void displaySimpleTest(Type_Display_SSD1309 *Display_SSD1309);
void displaySpectrumMock(Type_Display_SSD1309 *Display_SSD1309, bool ClearOnly);
void displayDirectTest(Type_Display_SSD1309 *SSD1309);
// WELCOME SPLASH SCREEN
void displayWelcomeScreen(Type_Display_SSD1309 *Display_SSD1309, uint8_t FW_Major, uint8_t FW_Minor, uint8_t FW_Test, uint8_t PL_Major, uint8_t PL_Minor, uint8_t PL_Test);
void displayUpdateBuffer(Type_Display_SSD1309 *Display_SSD1309);
// AUDIO SA RELATED
void displayStaticHeaderAudio(Type_Display_SSD1309 *Display_SSD1309, char *Heading, char *FileName, char *AudioAction, uint32_t TimeInSeconds, int8_t MinValue_dB);
void displayUpdateAudioPlaybackTime(Type_Display_SSD1309 *Display_SSD1309, uint32_t TimeInSeconds);
void displayUpdateAudioPlaybackAction(Type_Display_SSD1309 *Display_SSD1309, char *PlaybackAction);
void displayAudioSpectrum(Type_Display_SSD1309 *Display_SSD1309, uint8_t *DisplayMagnitude, uint8_t FrequencySlots, uint8_t VerticalBarCount, bool ClearOnly);
// SIGNAL SA RELATED
void displayStaticHeaderSignal(Type_Display_SSD1309 *Display_SSD1309, char *Heading, float StartFrequency, float CenterFrequency, float StopFrequency, Type_SignalSelect SignalSource);
void displayUpdateSignalSource(Type_Display_SSD1309 *Display_SSD1309, Type_SignalSelect SignalSource);
// void displaySignalSpectrum(Type_Display_SSD1309 *Display_SSD1309, float *BinMagnitudes, uint16_t LowBin, uint16_t HighBin, float FullScaleMagnitude);
void displaySignalSpectrum(Type_Display_SSD1309 *Display_SSD1309, float *BinMagnitudes, uint16_t LowBin, uint16_t HighBin, float MinDisplay_dB, float MaxDisplay_dB);

#ifdef __cplusplus
}
#endif
#endif /* APPLICATION_DISPLAY_H_ */