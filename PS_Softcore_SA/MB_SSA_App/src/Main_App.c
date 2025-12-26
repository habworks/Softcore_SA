/******************************************************************************************************
 * @file            Main_App.c
 * @brief           This is the main application that runs.  Here are the specificis of what it does and
 *                  how it operates.
 *                  This is PS bear-metal based on the Xilinx (AMD) MicroBlaze softcore
 *                  There are two major components to this application: Audio Specturm FFT and Signal Spectrum FFT
 *                  Audio Spectrum FFT:
 *
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
#ifdef RUN_MAIN_APPLICATION
// START OF the Main Applicaiton
#include "Main_App.h"
#include "xparameters.h"
#include "xtmrctr.h"
#include "xgpio.h"
#include "xintc.h"
#include "xil_exception.h"
#include "xuartlite.h"
#include "xspi.h"
#include "xil_printf.h"
#include "xstatus.h"
#include "ff.h"
#include <stdio.h>
#include "AXI_Timer_PWM_Support.h"
#include "AXI_UART_Lite_Support.h"
#include "AXI_IRQ_Controller_Support.h"
#include "Terminal_Emulator_Support.h"
#include "Audio_File_API.h"
#include "IO_Support.h"

// STATIC FUNCTIONS
static bool init_SoftCoreHandle(Type_SoftCore_SA *Handle);


// GLOBAL DEFINES
Type_SoftCore_SA SoftCore_SA;

// AXI GPIO SUPPORT
XGpio AXI_GPIO_Handle;

// AXI TIMER SUPPORT
XTmrCtr AXI_TimerHandle_0;

// AXI UART SUPPORT
XUartLite AXI_UART_Handle;

// AXI IRQ CONTROLLER SUPPORT
XIntc AXI_IRQ_ControllerHandle;

// FAT FS SUPPORT
FATFS FatFs; 



void mainApplication(void)
{
    int AXI_Status;
    bool Status;
    uint16_t InitFailMode = 0;
    char PrintBuffer[MAX_PRINT_BUFFER] = {0};
    
    // STEP 1: Init peripherals for use
    // Init AXI GPIO
    AXI_Status = XGpio_Initialize(&AXI_GPIO_Handle, XPAR_AXI_GPIO_0_BASEADDR);
    if (AXI_Status != XST_SUCCESS)
        InitFailMode |= INIT_FAIL_GPIO;
    XGpio_SetDataDirection(&AXI_GPIO_Handle, GPIO_INPUT_CHANNEL, 0xFFFF);     // Switches and push buttons as input
    XGpio_SetDataDirection(&AXI_GPIO_Handle, GPIO_OUTPUT_CHANNEL, 0x0000);  

    // Init AXI UART
    Status = init_UART_Lite(&AXI_UART_Handle, XPAR_AXI_UARTLITE_0_BASEADDR, INTERRUPT, UART_TxCallback_ISR, UART_RxCallback_ISR);
    if (Status == false)
        InitFailMode |= INIT_FAIL_GPIO;

    // Init FAT FS
    if (f_mount(&FatFs, ROOT_PATH, 1) != FR_OK)
        InitFailMode |= INIT_FAIL_FAT_FS;

    // Init SoftCore SA Handle
    Status = init_SoftCoreHandle(&SoftCore_SA);
    if (Status == false)
        InitFailMode |= INIT_FAIL_SOFTCORE_HANDLE;

    
    // STEP 2: Welcome
    terminal_ClearScreen();
    uint32_t PL_Ver = XGpio_DiscreteRead(&AXI_GPIO_Handle, GPIO_INPUT_CHANNEL);
    PL_Ver = (PL_Ver & HW_CONST_PL_VER) >> HW_CONST_PL_VER_OFFSET;
    printGreen("IMR Engineering, LLC\r\n");
    printGreen("  Hab Collector, Principal Engineer\r\n");
    printGreen("  http://www.imrengineering.com\r\n\n");
    xil_printf("Softcore Spectrum Analyzer\r\n");
    xil_printf("PS REV: %02d.%02d.%02d\r\n", FW_MAJOR_REV, FW_MINOR_REV, FW_TEST_REV);
    xil_printf("PL VER: %d\r\n\n", PL_Ver);
    if (InitFailMode)
    {
        snprintf(PrintBuffer, sizeof(PrintBuffer), "Init Fail Code(s): 0x%04X\r\n\n",InitFailMode);
        printBrightRed(PrintBuffer);
        fflush(stdout);
        while(1);
    }
    else
    {
        xil_printf("Hello Hab, I am ready...\r\n\n");
    }

for (uint8_t FileIndex = 0; FileIndex < SoftCore_SA.AudioFile.DirectoryFileCount; FileIndex++)
{
    FRESULT FileResult = getNextWavFile(AUDIO_DIRECTORY, SoftCore_SA.AudioFile.Name, SoftCore_SA.AudioFile.PathFileName, &SoftCore_SA.AudioFile.Size, SoftCore_SA.AudioFile.DirectoryFileCount);
    if (FileResult != FR_OK)
        printBrightRed("Error: getting next file\r\n");

    Status = getWavFileHeader(SoftCore_SA.AudioFile.PathFileName, SoftCore_SA.AudioFile.Size, &SoftCore_SA.AudioFile.Header);
    if (Status == true)
        xil_printf("%s: %d: OK\r\n",SoftCore_SA.AudioFile.Name, SoftCore_SA.AudioFile.Size);
    else
        printBrightRed("%s Not a valid audio file\r\n");
}


    f_closedir(AUDIO_DIRECTORY);
    f_mount(0, ROOT_PATH, 0);

    while(1);
}

// END OF the Main Application
#endif





/********************************************************************************************************
* @brief Init of Soft Core Spectrum Analyzer Handle
*
* @author original: Hab Collector \n
*
* @note: Must be init before main application can be called
* @note: Requires prior init of FAT FS
* 
* @param Handle: Pointer to Soft Core SA structure
*
* @return True if init OK
*
* STEP 1: Set default operating mode
* STEP 2: Set defaults for audio File 
********************************************************************************************************/
static bool init_SoftCoreHandle(Type_SoftCore_SA *Handle)
{
    // STEP 1: Set default operating mode
    Handle->Mode = MODE_AUDIO;

    // STEP 2: Set defaults for audio File 
    memset(Handle->AudioFile.Name, 0x00, sizeof(Handle->AudioFile.Name));
    Handle->AudioFile.DirectoryFileCount = 0;
    FRESULT FileResult = countFilesInDirectory(AUDIO_DIRECTORY, &Handle->AudioFile.DirectoryFileCount);
    if ((FileResult != FR_OK) || (Handle->AudioFile.DirectoryFileCount == 0))
        return(false);
    else
        return(true);

} // END OF init_SoftCoreHandle




