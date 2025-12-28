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


// TYPEDEFS AND ENUMS
typedef struct
{
    Type_AudioFile              File;
} Type_Audio_SA;

// FUNCTION PROTOTYPES
int16_t convert_PCM16_ToMono(int16_t Left_PCM16_Audio, int16_t Right_PCM16_Audion);

#ifdef __cplusplus
}
#endif
#endif /* SOFTCORE_AUDIO_SA_H_ */