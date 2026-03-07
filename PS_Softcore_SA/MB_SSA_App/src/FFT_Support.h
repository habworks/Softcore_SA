/******************************************************************************************************
 * @file            FFT_Support.h
 * @brief           Header file to support FFT_Support.c
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

#ifndef FFT_SUPPORT_H_
#define FFT_SUPPORT_H_
#ifdef __cplusplus
extern"C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "Main_Support.h"

// DEFINES
#define FREQUENCY_SLOTS         18U  // Number of frequency slots (bars) to display
#define MAX_VERTICAL_BAR_COUNT  8U   // Max vertical bar height in segments


// TYPEDEFS AND ENUMS
typedef enum
{
    LINEAR_COMPRESSION_NONE = 0,
    LINEAR_COMPRESSION_SQRT,
    LINEAR_COMPRESSION_LOG1P
} Type_LinearCompression;

typedef enum
{
    SLOT_MAGNITUDE_AVERAGE = 0,
    SLOT_MAGNITUDE_RMS
} Type_SlotMagnitudeMethod;

typedef struct
{
    bool                        IsFrequencyLogSpacing;     // true = log-like frequency grouping, false = linear grouping
    bool                        IsMagnitudeLog;            // true = dB-like magnitude scaling, false = linear magnitude
    uint8_t                     FrequencyBarCount;         // Must equal FREQUENCY_SLOTS
    uint8_t                     MagnitudeBarCount;         // Must equal MAX_VERTICAL_BAR_COUNT
    float                      *Frequency;                 // Array length = FREQUENCY_SLOTS.  Center frequency per slot (Hz)
    float                      *Magnitude;                 // Array length = FREQUENCY_SLOTS.  Slot magnitude (linear or dB, per IsMagnitudeLog)
} Type_AudioSpectrum;


// EXTERNS
extern Type_AudioSpectrum AudioSpectrum;


// FUNCTION PROTOTYPES
bool init_FFT(Type_FFT *FFT, uint32_t SampleRate_Hz);
void deinit_FFT(Type_FFT *FFT);
// Audio Related Functions
uint32_t FFT_ProcessAudioFrame(Type_FFT *FFT, float *BinMagnitudes, uint32_t BinCount);
bool buildAudioSpectrumFrame(uint32_t SampleRate_Hz,
                            uint16_t FFT_Size,
                            const float *BinMagnitudes,
                            uint32_t BinCount,
                            Type_SlotMagnitudeMethod SlotMagnitudeMethod,
                            Type_LinearCompression LinearCompression,
                            float MinDB,
                            uint8_t *DisplayMagnitudeBars,
                            uint8_t FrequencyBarCount,
                            uint8_t MagnitudeBarCount,
                            Type_AudioSpectrum *Spectrum);
// Signal Related Functions
bool FFT_ProcessSignalFrame(Type_FFT *FFT, float *BinMagnitudes, uint16_t LowBin, uint16_t HighBin);
bool calculateBinMinMaxFromSpan(float LowSpan, float HighSpan, uint16_t MaxBinCount, float RBW, uint16_t *LowBin, uint16_t *HighBin);



#ifdef __cplusplus
}
#endif
#endif /* FFT_SUPPORT_H_ */