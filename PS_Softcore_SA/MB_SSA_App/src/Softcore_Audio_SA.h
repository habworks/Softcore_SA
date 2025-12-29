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

#ifndef SOFTCORE_AUDIO_SA_H_
#define SOFTCORE_AUDIO_SA_H_
#ifdef __cplusplus
extern"C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "Audio_File_API.h"


// DEFINES
#define FFT_SIZE                1024U
#define CHUNK_MULTIPLIER        8
#if ((CHUNK_MULTIPLIER / 2) * 2) != CHUNK_MULTIPLIER
    #error "MULTIPLER must be even"
#endif
#if CHUNK_MULTIPLIER < 4
    #error "CHUNK_MULTIPLIER must be >= 4 and be an even value
#endif
#define MAX_CHUNK_BUFFER          (FFT_SIZE * CHUNK_MULTIPLIER)

typedef struct
{
    bool                        FrameReady;
    uint16_t                    Size;
    float                       HannWindow[FFT_SIZE];
    float                       Samples[FFT_SIZE];
} Type_FFT;

typedef struct
{
    float                       Samples[FFT_SIZE];
} Type_PWM;

// TYPEDEFS AND ENUMS
typedef struct
{
    bool                        Enable;
    bool                        IsFirstRead;
    bool                        IsRawBufferEmpty;
    Type_AudioFile              File;
    Type_int16_t_CircularBuffer CircularBuffer;
    Type_FFT                    FFT;
    Type_PWM                    PWM
} Type_Audio_SA;


// FUNCTION PROTOTYPES
void audioSpectrumAnalyzer(Type_Audio_SA *Audio_SA);


#ifdef __cplusplus
}
#endif
#endif /* SOFTCORE_AUDIO_SA_H_ */