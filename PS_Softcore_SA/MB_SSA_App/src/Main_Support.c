/******************************************************************************************************
 * @file            Main_Support.c
 * @brief           A collection of functions relevant to supporting both the main test and main application
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

#include "Main_Support.h"
#include "IO_Support.h"
#include "xgpio.h"


extern XTmrCtr AXI_TimerHandle_0;
extern XGpio AXI_GPIO_Handle;



void sleep_10us_Wrapper(uint32_t WaitTime)
{
    sleep_10us(&AXI_TimerHandle_0, XTC_TIMER_0, WaitTime);
}
void sleep_10us(XTmrCtr *AXI_TimerHandle, uint8_t Timer, uint32_t WaitTime)
{
    uint32_t StartCount = XTmrCtr_GetValue(AXI_TimerHandle, Timer);
    uint32_t DelayCount = (TICKS_PER_10_US * WaitTime);    
    while(StartCount - XTmrCtr_GetValue(AXI_TimerHandle, Timer) < DelayCount);
}



void sleep_ms_Wrapper(uint32_t WaitTime)
{
    sleep_ms(&AXI_TimerHandle_0, XTC_TIMER_0, WaitTime);    
}
void sleep_ms(XTmrCtr *AXI_TimerHandle, uint8_t Timer, uint32_t WaitTime)
{
    uint32_t StartCount = (uint32_t)XTmrCtr_GetValue(AXI_TimerHandle, Timer);
    uint32_t DelayCount = (TICKS_PER_MILLISECOND * WaitTime);
    do 
    {
        // asm volatile("nop");
    } while(StartCount - XTmrCtr_GetValue(AXI_TimerHandle, Timer) < DelayCount);
}



void displayResetOrRun(Type_DisplayResetRun ResetRunAction)
{
    if (ResetRunAction == DISPLAY_RUN)
        XGpio_DiscreteSet(&AXI_GPIO_Handle, GPIO_OUTPUT_CHANNEL, DISPLAY_RESET_RUN);
    else
        XGpio_DiscreteClear(&AXI_GPIO_Handle, GPIO_OUTPUT_CHANNEL, DISPLAY_RESET_RUN);
}

void displayCommandOrData(Type_DisplayCommandData CommandDataAction)
{
    if (CommandDataAction == DISPLAY_DATA)
        XGpio_DiscreteSet(&AXI_GPIO_Handle, GPIO_OUTPUT_CHANNEL, DISPLAY_CMD_DATA);
    else
        XGpio_DiscreteClear(&AXI_GPIO_Handle, GPIO_OUTPUT_CHANNEL, DISPLAY_CMD_DATA);
}

void displayChipSelect(Type_Display_CS Status)
{
    if (Status == CS_ENABLE)
        XGpio_DiscreteClear(&AXI_GPIO_Handle, GPIO_OUTPUT_CHANNEL, DISPLAY_CS);
    else
        XGpio_DiscreteSet(&AXI_GPIO_Handle, GPIO_OUTPUT_CHANNEL, DISPLAY_CS);
}

bool displayTrasmitReceive(XSpi *SPI_DisplayHandle, uint8_t ChipSelect_N, uint8_t *TxBuffer, uint8_t *RxBuffer, uint32_t BytesToTransfer)
{
    // STEP 1: Simple test
    if ((SPI_DisplayHandle == NULL) || (ChipSelect_N == 0))
        return(false);

    // STEP 2: Reset the SPI
    XSpi_Reset(SPI_DisplayHandle);

    // STEP 3: Pre-transfer data: Set the SPI options, select the correct slave device, and start the SPI
    XSpi_SetOptions(SPI_DisplayHandle,XSP_MASTER_OPTION);
    XSpi_SetSlaveSelect(SPI_DisplayHandle, ChipSelect_N);
    XSpi_Start(SPI_DisplayHandle);

    // STEP 4: Transfer the data
    int AXI_Status = XSpi_Transfer(SPI_DisplayHandle, TxBuffer, RxBuffer, BytesToTransfer);    
    if (AXI_Status != XST_SUCCESS)
        return(false);
    else
        return(true);

}