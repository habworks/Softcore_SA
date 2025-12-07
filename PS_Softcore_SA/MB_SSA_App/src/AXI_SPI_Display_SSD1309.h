/******************************************************************************************************
 * @file            AXI_SPI_Display_SSD1309.h
 * @brief           Header file to support AXI_SPI_Display_SSD1309.c
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

#ifndef AXI_SPI_DISPLAY_SSD1309_H_
#define AXI_SPI_DISPLAY_SSD1309_H_
#ifdef __cplusplus
extern"C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "xspi.h"
#include "u8g2.h"
#include "u8x8.h"

// DEFINES


// TYPEDEFES AND ENUMS
typedef enum
{
    CS_DISABLE = 0,
    CS_ENABLE
}Type_Display_CS;

typedef enum
{
    DISPLAY_RESET = 0,
    DISPLAY_RUN
}Type_DisplayResetRun;
typedef void (*displayResetRunFunctionPtr)(Type_DisplayResetRun);

typedef enum
{
    DISPLAY_COMMAND = 0,
    DISPLAY_DATA
}Type_DisplayCommandData;
typedef void (*displayCommandDataFunctionPtr)(Type_DisplayCommandData);

typedef bool (*displayTxRxFunctionPtr)(XSpi *, uint8_t, uint8_t *, uint8_t *, uint32_t);
typedef void (*displaySleep_msFunctionPtr)(uint32_t);
typedef void (*displaySleep_10usFunctionPtr)(uint32_t);
typedef void (*displayChipSelectFunctionPtr)(Type_Display_CS);

typedef struct
{
    XSpi                            *SPI_Handle;            // SPI handle used with display
    uint8_t                         ChipSelectBitMask;      // Chip select associated with the display and SPI handle
    uint16_t                        FIFO_BufferDepth;        // Depth of FIFO Buffer
    displayResetRunFunctionPtr      displayResetRun;        // Funtion pointer to set reset or run
    displayCommandDataFunctionPtr   displayCommandData;     // Function pointer to set Command or Data
    displayTxRxFunctionPtr          displayTxRx;            // Function pointer SPI based Transmit and receive
    displayChipSelectFunctionPtr    display_CS;             // Function pointer CS enable or disable
    displaySleep_msFunctionPtr      displaySleep_ms;        // Sleep function ms interval (blocking)
    displaySleep_10usFunctionPtr    displaySleep_10us;      // Sleep function 10us interval (blocking)
    u8g2_t                          *U8G2_Handle;           // Graphics Library Handle
}Type_Display_SSD1309;


// FUNCTION PROTOTYPES
bool init_Display_SSD1309(Type_Display_SSD1309 *Display_SSD1309, XSpi *QSPI_Handle, uint8_t ChipSelect_N, uint16_t FIFO_Depth, displayResetRunFunctionPtr displayResetRunFunction, displayCommandDataFunctionPtr displayCommandDataFunction, displayTxRxFunctionPtr displayTxRxFunction, displayChipSelectFunctionPtr displayChipSelectFunction, displaySleep_msFunctionPtr displaySleep_msFunction, displaySleep_10usFunctionPtr displaySleep_10usFunction, u8g2_t *U8G2_Object);
void displaySimpleTest(Type_Display_SSD1309 *Display_SSD1309);
void displayTest_2(void);
void drawSpectrumMock(Type_Display_SSD1309 *Display_SSD1309);

#ifdef __cplusplus
}
#endif
#endif /* AXI_SPI_DISPLAY_SSD1309_H_ */