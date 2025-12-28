/******************************************************************************************************
 * @file            Main_App.h
 * @brief           Header file to support Main_App.c 
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

 #ifndef MAIN_APP_H_
#define MAIN_APP_H_
#include <limits.h>
#ifdef __cplusplus
extern"C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "SoftCore_Audio_SA.h"


// DEFINES
// INIT_FAIL_MODES
#define INIT_FAIL_GPIO                  ((uint16_t)(0x01 << 0))
#define INIT_FAIL_UART                  ((uint16_t)(0x01 << 1))
#define INIT_FAIL_FAT_FS                ((uint16_t)(0x01 << 2))
#define INIT_FAIL_SOFTCORE_HANDLE       ((uint16_t)(0x01 << 3))
// MISC
#define MAX_PRINT_BUFFER                255U

// TYPEDEFS AND ENUMS
typedef enum
{
    MODE_AUDIO = 0,
    MODE_SIGNAL
}Type_Mode;

// typedef struct
// {
//     #if (FF_USE_LFN == 1)
//     char                        Name[FF_MAX_LFN];             // FAT FS supporting long file name
//     char                        PathFileName[FF_MAX_LFN];
//     #else
//     char                        Name[8+1+3];                   // FAT FS 8.3 file name support
//     char                        PathFileName[50];
//     #endif
//     uint16_t                    DirectoryFileCount;
//     uint32_t                    Size;
//     Type_WavHeader              Header;
// }Type_AudioFile;

typedef struct
{
    Type_Mode                   Mode;
    Type_Audio_SA               Audio_SA;
}Type_SoftCore_SA;

// FUNTION PROTOTYPES
void mainApplication(void);


#ifdef __cplusplus
}
#endif
#endif /* MAIN_APP_H_ */