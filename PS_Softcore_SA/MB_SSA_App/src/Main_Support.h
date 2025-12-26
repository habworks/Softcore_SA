/******************************************************************************************************
 * @file            Main_Support.h
 * @brief           Header file to support Main_Support.c 
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

#ifndef MAIN_SUPPORT_H_
#define MAIN_SUPPORT_H_
#ifdef __cplusplus
extern"C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <xparameters.h>
#include "xtmrctr.h"
#include "AXI_SPI_Display_SSD1309.h"


// DEFINES
// PRE-PROCESSOR
#define                         RUN_MAIN_APPLICATION    // ****Comment out this line if running testing****
// PS FW REVSION
#define FW_MAJOR_REV            1
#define FW_MINOR_REV            0
#define FW_TEST_REV             1
// USED IN SLEEP FUNCTIONS
#define TICKS_PER_MILLISECOND   (XPAR_CPU_CORE_CLOCK_FREQ_HZ / 1000)
#define TICKS_PER_10_US         (XPAR_CPU_CORE_CLOCK_FREQ_HZ / 100000)
#define GPIO_INPUT_CHANNEL      1          
#define GPIO_OUTPUT_CHANNEL     2    
// UART USE
#define RX_BUFFER_SIZE          10      


// EXTERNS
extern uint32_t volatile ReceivedBytes;
extern uint8_t RxDataBuffer[RX_BUFFER_SIZE];

// FUNCTION PROTOTYPES
void sleep_10us_Wrapper(uint32_t WaitTime);
void sleep_10us(XTmrCtr *AXI_TimerHandle, uint8_t Timer, uint32_t WaitTime);
void sleep_ms_Wrapper(uint32_t WaitTime);
void sleep_ms(XTmrCtr *AXI_TimerHandle, uint8_t Timer, uint32_t WaitTime);
void displayResetOrRun(Type_DisplayResetRun ResetRunAction);
void displayCommandOrData(Type_DisplayCommandData CommandDataAction);
void displayChipSelect(Type_Display_CS Status);
bool displayTrasmitReceive(XSpi *SPI_DisplayHandle, uint8_t ChipSelect_N, uint8_t *TxBuffer, uint8_t *RxBuffer, uint32_t BytesToTransfer);
// ISR CALLBACK FUNCTIONS
void UART_RxCallback_ISR(void *CallBackRef, unsigned int EventData);
void UART_TxCallback_ISR(void *CallBackRef, unsigned int EventData);

#ifdef __cplusplus
}
#endif
#endif /* MAIN_SUPPORT_H_ */