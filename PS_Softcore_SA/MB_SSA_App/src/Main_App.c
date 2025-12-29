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
#include <math.h>
#include "AXI_Timer_PWM_Support.h"
#include "AXI_UART_Lite_Support.h"
#include "AXI_IRQ_Controller_Support.h"
#include "Terminal_Emulator_Support.h"
#include "Audio_File_API.h"
#include "Softcore_Audio_SA.h"
#include "IO_Support.h"

// STATIC FUNCTIONS
static void main_InitApplication(void);
static void main_WhileLoop(void);
static bool init_SoftCoreHandle(Type_SoftCore_SA *Handle);


// GLOBAL DEFINES
Type_SoftCore_SA SoftCore_SA;

// AXI GPIO SUPPORT
XGpio AXI_GPIO_Handle;

// AXI TIMER SUPPORT
XTmrCtr AXI_TimerHandle;

// AXI PWM SUPPORT
XTmrCtr AXI_PWM_Handle;

// AXI UART SUPPORT
XUartLite AXI_UART_Handle;

// AXI IRQ CONTROLLER SUPPORT
XIntc AXI_IRQ_ControllerHandle;

// FAT FS SUPPORT
FATFS FatFs; 


/********************************************************************************************************
* @brief This is the mian application - it is broken up into two parts - the main init and the main never
* ending loop
*
* @author original: Hab Collector \n
********************************************************************************************************/
void mainApplication(void)
{
    main_InitApplication();
    main_WhileLoop();
}
// END OF the Main Application



/********************************************************************************************************
* @brief Init of main application peripherals, drivers, libaries, and handlers - code only runs once
*
* @author original: Hab Collector \n
*
* @note: Must be the first function call of mainApplication
* 
* STEP 1: Init AXI peripherals for use
* STEP 2: Init of libraries
* STEP 3: Init SoftCore SA Handle
* STEP 4: Welcome
********************************************************************************************************/
static void main_InitApplication(void)
{
    int AXI_Status;
    bool Status;
    uint16_t InitFailMode = 0;
    char PrintBuffer[MAX_PRINT_BUFFER] = {0};
    
    // STEP 1: Init AXI peripherals for use
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

    // Init AXI Timer as PWM
    Status = init_PWM(&AXI_PWM_Handle, XPAR_AXI_TIMER_1_BASEADDR);
    if (Status == false)
        InitFailMode |= INIT_FAIL_PWM;


    // STEP 2: Init of libraries
    // Init FAT FS
    if (f_mount(&FatFs, ROOT_PATH, 1) != FR_OK)
        InitFailMode |= INIT_FAIL_FAT_FS;


    // STEP 3: Init SoftCore SA Handle
    Status = init_SoftCoreHandle(&SoftCore_SA);
    if (Status == false)
        InitFailMode |= INIT_FAIL_SOFTCORE_HANDLE;


    // STEP 4: Welcome
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

} // END OF main_InitApplication



/********************************************************************************************************
* @brief The application is bear metal - this is the contineous while loop that runs after main init. This
* loop in normal operatioon is non-existing.
*
* @author original: Hab Collector \n
*
* @note: Must be the sectond function call of mainApplication
* 
* STEP 1: Init peripherals for use
* STEP 2: Init of libraries
* STEP 3: Init SoftCore SA Handle
* STEP 4: Welcome
********************************************************************************************************/
static void main_WhileLoop(void)
{
    bool Status;
    char PrintBuffer[MAX_PRINT_BUFFER] = {0};


    // for (uint8_t FileIndex = 0; FileIndex < SoftCore_SA.Audio_SA.File.DirectoryFileCount; FileIndex++)
    // {
    //     FRESULT FileResult = getNextWavFile(AUDIO_DIRECTORY, SoftCore_SA.Audio_SA.File.Name, SoftCore_SA.Audio_SA.File.PathFileName, &SoftCore_SA.Audio_SA.File.Size, SoftCore_SA.Audio_SA.File.DirectoryFileCount);
    //     if (FileResult != FR_OK)
    //         printBrightRed("Error: getting next file\r\n");

    //     Status = getWavFileHeader(SoftCore_SA.Audio_SA.File.PathFileName, SoftCore_SA.Audio_SA.File.Size, &SoftCore_SA.Audio_SA.File.Header);
    //     if (Status == true)
    //     {
    //         xil_printf("%s: %d: OK\r\n",SoftCore_SA.Audio_SA.File.Name, SoftCore_SA.Audio_SA.File.Size);
    //         fflush(stdout);
    //     }
    //     else
    //     {
    //         snprintf(PrintBuffer, sizeof(PrintBuffer), "%s Not a valid audio file\r\n", SoftCore_SA.Audio_SA.File.Name);
    //         printBrightRed(PrintBuffer);
    //         fflush(stdout);
    //     }
    // }

    for (uint8_t Test = 0; Test < 10; Test++)
    {
        xil_printf("HannWindow[%d]: %1.6f\r\n", Test, SoftCore_SA.Audio_SA.FFT.HannWindow[Test]);
    }


    FRESULT FileResult = getNextWavFile(AUDIO_DIRECTORY, SoftCore_SA.Audio_SA.File.Name, SoftCore_SA.Audio_SA.File.PathFileName, &SoftCore_SA.Audio_SA.File.Size, SoftCore_SA.Audio_SA.File.DirectoryFileCount);
    if (FileResult != FR_OK)
        printBrightRed("Error: getting next file\r\n");

    Status = getWavFileHeader(SoftCore_SA.Audio_SA.File.PathFileName, SoftCore_SA.Audio_SA.File.Size, &SoftCore_SA.Audio_SA.File.Header);
    if (Status == true)
    {
        xil_printf("%s: %d: OK\r\n",SoftCore_SA.Audio_SA.File.Name, SoftCore_SA.Audio_SA.File.Size);
    }

    

    f_closedir(&Directory);
    f_mount(0, ROOT_PATH, 0);

    setup_PWM(&AXI_PWM_Handle, 200000, 50.0);

    while(1);
}



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
    Handle->Audio_SA.Enable = false;
    Handle->Audio_SA.File.IsOpen = false;
    memset(Handle->Audio_SA.File.Name, 0x00, sizeof(Handle->Audio_SA.File.Name));
    memset(Handle->Audio_SA.File.PathFileName, 0x00, sizeof(Handle->Audio_SA.File.PathFileName));
    Handle->Audio_SA.File.DirectoryFileCount = 0;
    FRESULT FileResult = countFilesInDirectory(AUDIO_DIRECTORY, &Handle->Audio_SA.File.DirectoryFileCount);
    if ((FileResult != FR_OK) || (Handle->Audio_SA.File.DirectoryFileCount == 0))
        return(false);
    else
        return(true);
    // Calculate the FFT Hann Window
    for (uint16_t N = 0; N < FFT_SIZE; N++)
    {
        Handle->Audio_SA.FFT.HannWindow[N] = 0.5 * (1 - cos((2* M_PI* N)/(FFT_SIZE - 1)));
    }

} // END OF init_SoftCoreHandle



// END OF PROCESSOR DEFINE FOR RUN_MAIN_APPLICATION
#endif