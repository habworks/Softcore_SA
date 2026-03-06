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
#include "Signal_SoftCore_SA.h"
#include "Main_Support.h"
#include "MCP23S08_Driver.h"
#include "SSD1309_Driver.h"
#include "AXI_IMR_ADC_7476A_DUAL.h"
#include "xtmrctr.h"
#include "xgpio.h"
#include "xintc.h"

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
#define INIT_FAIL_ADC7476A              ((uint16_t)(0x01 << 12))
// MISC
#define MAX_PRINT_BUFFER                255U
#define SPLASH_SCREEN_HOLD_TIME         3000U   // Value in ms


// TYPEDEFS AND ENUMS
typedef struct
{
    uint8_t                     UI_LED_Status;
    Type_Mode                   Mode;
    Type_FFT                    FFT;
    Type_Audio_SA               Audio_SA;
    Type_Signal_SA              Signal_SA;
}Type_SoftCore_SA;


// EXTERNS
extern XGpio AXI_GPIO_Handle;
extern XTmrCtr AXI_SampleTimerHandle;
extern XTmrCtr AXI_ModeTimerHandle;
extern XTmrCtr AXI_PWM_Handle;
extern XIntc AXI_IRQ_ControllerHandle;
extern Type_Display_SSD1309 Display_SSD1309;
extern Type_MCP23S08_Driver IOX_1;
extern Type_AXI_IMR_7476A_Handle AXI_IMR_7476A_Handle;
extern uint32_t StackUsedWaterMark;


// FUNTION PROTOTYPES
void mainApplication(void);


#ifdef __cplusplus
}
#endif
#endif /* MAIN_APP_H_ */