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
#define                         RUN_MAIN_APPLICATION    // ****Comment out this line if running testing app****
// PS FW REVSION
#define FW_MAJOR_REV            1
#define FW_MINOR_REV            0
#define FW_TEST_REV             2
// USED IN IO ACCESS
#define GPIO_INPUT_CHANNEL      1          
#define GPIO_OUTPUT_CHANNEL     2    
// UART USE
#define RX_BUFFER_SIZE          10  
// MICROBLAZE CACHE SUPPORT *** THESE LINES SHOULD BE ADDED WITHIN XPARAMETERS.H BUT THERE IS A BUG IN 2024.2 Vivado / Viits
#define XPAR_MICROBLAZE_USE_ICACHE      1
#define XPAR_MICROBLAZE_USE_DCACHE      1
#define XPAR_MICROBLAZE_USE_MSR_INSTR   1
#define XPAR_MICROBLAZE_ICACHE_BYTE_SIZE 65536  // Value must match MicroBlaze Instruction cache
#define XPAR_MICROBLAZE_DCACHE_BYTE_SIZE 32768  // Value must match MicroBlaze Data cache
// DISPLAY
#define DISPLAY_CS_NUMBER       ((uint8_t)(0x01 << 0))
// IO EXPANDER 1
#define IOX_1_CS_NUMBER         ((uint8_t)(0x01 << 1))
#define IOX_1_DEVICE_ADDR       ((uint8_t)(0x00))
#define IOX_1_IO_DIRECTION      ((uint8_t)(0x00))
#define IOX_1_INPUT_POLARITY    ((uint8_t)(0x00))
#define IOX_1_IRQ_ON_CHANGE     ((uint8_t)(0x00))
#define IOX_1_IRQ_DEFAULT_VALUE ((uint8_t)(0x00))
#define IOX_1_IRQ_CONTROL       ((uint8_t)(0x00))
#define IOX_1_CONFIGURATION     MCP23S08_SEQOP_BIT_MASK
#define IOX_1_PULLUP            ((uint8_t)(0x00))
// IO EXPANDER 2
#define IOX_2_CS_NUMBER         ((uint8_t)(0x01 << 2))
#define IOX_2_DEVICE_ADDR       ((uint8_t)(0x01))
#define IOX_2_IO_DIRECTION      ((uint8_t)(0xFF))
#define IOX_2_INPUT_POLARITY    ((uint8_t)(0x00))
#define IOX_2_IRQ_ON_CHANGE     ((uint8_t)(0x1F))
#define IOX_2_IRQ_DEFAULT_VALUE ((uint8_t)(0x00))
#define IOX_2_IRQ_CONTROL       ((uint8_t)(0x00))
#define IOX_2_CONFIGURATION     (MCP23S08_SEQOP_BIT_MASK | MCP23S08_INTPOL_BIT_MASK)
#define IOX_2_PULLUP            ((uint8_t)(0x00))
// IO EXPANDER 2 INPUTS AND ALIAS
#define IOX_2_INPUT_0_MASK      ((uint8_t)(0x01 << 0))
#define IOX_2_INPUT_1_MASK      ((uint8_t)(0x01 << 1))
#define IOX_2_INPUT_2_MASK      ((uint8_t)(0x01 << 2))
#define IOX_2_INPUT_3_MASK      ((uint8_t)(0x01 << 3))
#define IOX_2_INPUT_4_MASK      ((uint8_t)(0x01 << 4))
#define UI_SW5                  IOX_2_INPUT_0_MASK
#define UI_SW4                  IOX_2_INPUT_1_MASK
#define UI_SW3                  IOX_2_INPUT_2_MASK
#define UI_SW2                  IOX_2_INPUT_3_MASK
#define UI_SW1                  IOX_2_INPUT_4_MASK
// MISC
#define MODE_PERIODIC_IRQ_TIME  4                                                       // Value in sec
#define MODE_TIMER_COUNT        (MODE_PERIODIC_IRQ_TIME * XPAR_CPU_CORE_CLOCK_FREQ_HZ)  // Count that lead to Value in sec delay
#define FFT_BINS                1024U
#define FFT_SIZE                FFT_BINS

// TYPEDEFS AND ENUMS


// EXTERNS
extern volatile uint32_t volatile ReceivedBytes;
extern volatile uint8_t RxDataBuffer[RX_BUFFER_SIZE];


// MACROS
#define NOT_USED(X)             ((void)(X))
#define DO_NOTHING()            asm volatile ("nop")
#define DO_NOTHING_REPEAT(X)    do { for (volatile uint32_t i = 0; i < (uint32_t)(X); i++) asm volatile ("nop"); } while (0)


// FUNCTION PROTOTYPES
void sleep_10us_Wrapper(uint32_t WaitTime);
void sleep_ms_Wrapper(uint32_t WaitTime);
void displayResetOrRun(Type_DisplayResetRun ResetRunAction);
void displayCommandOrData(Type_DisplayCommandData CommandDataAction);
void displayChipSelect(Type_Display_CS DisplaySelect);
bool displayTrasmitReceive(XSpi *SPI_DisplayHandle, uint8_t ChipSelect_N, uint8_t *TxBuffer, uint8_t *RxBuffer, uint32_t BytesToTransfer);
bool is_MicroSD_Inserted(void);
void IOX_Reset(bool Status);
void IOX_ChipSelect(bool ChipSelect);
// ISR CALLBACK FUNCTIONS
// void UART_RxCallback_ISR(void *CallBackRef, unsigned int EventData);


#ifdef __cplusplus
}
#endif
#endif /* MAIN_SUPPORT_H_ */