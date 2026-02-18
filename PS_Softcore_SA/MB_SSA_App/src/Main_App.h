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
#ifdef __cplusplus
extern"C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "Audio_SoftCore_SA.h"
#include "Main_Support.h"


// DEFINES
// INIT_FAIL_MODES
#define INIT_FAIL_UART                  ((uint16_t)(0x01 << 0))
#define INIT_FAIL_GPIO                  ((uint16_t)(0x01 << 1))
#define INIT_FAIL_TIMER_1               ((uint16_t)(0x01 << 2))
#define INIT_FAIL_TIMER_2               ((uint16_t)(0x01 << 3))
#define INIT_FAIL_TIMER_3               ((uint16_t)(0x01 << 4))
#define INIT_FAIL_SPI_0                 ((uint16_t)(0x01 << 5))
#define INIT_FAIL_SPI_1                 ((uint16_t)(0x01 << 6))
#define INIT_FAIL_IRQ_CONTROLLER        ((uint16_t)(0x01 << 7))
#define INIT_FAIL_UI_IO                 ((uint16_t)(0x01 << 8))
#define INIT_FAIL_FAT_FS                ((uint16_t)(0x01 << 9))
#define INIT_FAIL_UI_DISPLAY            ((uint16_t)(0x01 << 10))
#define INIT_FAIL_SOFTCORE_SA           ((uint16_t)(0x01 << 11))
// MISC
#define MAX_PRINT_BUFFER                255U


// TYPEDEFS AND ENUMS
typedef enum
{
    MODE_AUDIO_SA = 0,
    MODE_SIGNAL_SA
}Type_Mode;

typedef struct
{
    bool                        FrameReady;                 // An FFT size data is ready for processing in the PWM and HannWindow Buffers
    uint16_t                    Size;
    float                       HannWindow[FFT_SIZE];
    float                       Samples[FFT_SIZE];
    float                       RBW;
} Type_FFT;

typedef struct
{
    Type_Mode                   Mode;
    Type_FFT                    FFT;
    Type_Audio_SA               Audio_SA;
}Type_SoftCore_SA;


// FUNTION PROTOTYPES
void mainApplication(void);


#ifdef __cplusplus
}
#endif
#endif /* MAIN_APP_H_ */