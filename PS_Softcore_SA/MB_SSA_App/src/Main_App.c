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

#include "Main_App.h"
#ifdef RUN_MAIN_APPLICATION
// START OF the Main Applicaiton
#include "xparameters.h"
#include "xtmrctr.h"
#include "xgpio.h"
#include "xintc.h"
#include "xil_exception.h"
#include "xuartlite.h"
#include "xspi.h"
#include "xil_printf.h"
#include "xstatus.h"
#include "xil_cache.h"
#include "ff.h"
#include "u8g2.h"
#include <stdio.h>
#include <math.h>
#include "AXI_Timer_PWM_Support.h"
#include "AXI_UART_Lite_Support.h"
#include "AXI_IRQ_Controller_Support.h"
#include "Terminal_Emulator_Support.h"
#include "AXI_QSPI_Support.h"
#include "IO_Support.h"
#include "Audio_File_API.h"
#include "Audio_SoftCore_SA.h"
#include "MCP23S08_Driver.h"


// STATIC FUNCTIONS
static void main_InitApplication(void);
static void main_WhileLoop(void);
static bool init_SoftCoreHandleCommon(Type_SoftCore_SA *Handle, uint32_t SampleFrequency);
static bool init_SoftCoreHandleAudio(Type_SoftCore_SA *Handle);
static void TimerCallbackSample_ISR(void) __attribute__((fast_interrupt));
static void TimerCallbackMode_ISR(void) __attribute__((fast_interrupt));
static void processUserInput(Type_SoftCore_SA *SoftCore_SA);
static void modeSwitch(Type_SoftCore_SA *SoftCore_SA);
static void selectSwitch(Type_SoftCore_SA *SoftCore_SA);



// AXI SUPPORT:
XGpio __attribute__ ((section (".Hab_Fast_Data"))) AXI_GPIO_Handle;
XTmrCtr __attribute__ ((section (".Hab_Fast_Data"))) AXI_SampleTimerHandle;
XTmrCtr __attribute__ ((section (".Hab_Fast_Data"))) AXI_ModeTimerHandle;
XTmrCtr __attribute__ ((section (".Hab_Fast_Data"))) AXI_PWM_Handle;
XUartLite __attribute__ ((section (".Hab_Fast_Data"))) AXI_UART_Handle;
XSpi __attribute__ ((section (".Hab_Fast_Data"))) AXI_SPI_UI_Handle;
XSpi __attribute__ ((section (".Hab_Fast_Data"))) AXI_SPI_USD_Handle;
XIntc __attribute__ ((section (".Hab_Fast_Data"))) AXI_IRQ_ControllerHandle;

// NON AXI PERIPHERAL:
Type_SoftCore_SA __attribute__ ((section (".Hab_Fast_Data"))) SoftCore_SA;
Type_Display_SSD1309 __attribute__ ((section (".Hab_Fast_Data"))) Display_SSD1309; 
Type_MCP23S08_Driver __attribute__ ((section (".Hab_Fast_Data"))) IOX_1;
Type_MCP23S08_Driver __attribute__ ((section (".Hab_Fast_Data"))) IOX_2;
u8g2_t __attribute__ ((section (".Hab_Fast_Data"))) U8G2; 
FATFS __attribute__ ((section (".Hab_Fast_Data"))) FatFs;

volatile uint8_t __attribute__ ((section (".Hab_Fast_Data"))) DutyCyclePercent = 1;



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
    FRESULT FileResult;
    uint16_t InitFailMode = 0;
    char PrintBuffer[MAX_PRINT_BUFFER] = {0};

    // STEP 1: Enable the instruction and data cache
    Xil_ICacheEnable();
    Xil_DCacheEnable();
    

    // STEP 2: Init AXI peripherals for use
    // Init AXI UART
    Status = init_UART_Lite(&AXI_UART_Handle, XPAR_AXI_UARTLITE_0_BASEADDR, POLLING, NULL, NULL, false);    
    if (Status != true)
        InitFailMode |= INIT_FAIL_UART;

    // Init AXI GPIO
    AXI_Status = XGpio_Initialize(&AXI_GPIO_Handle, XPAR_AXI_GPIO_0_BASEADDR);
    if (AXI_Status != XST_SUCCESS)
        InitFailMode |= INIT_FAIL_GPIO;
    XGpio_SetDataDirection(&AXI_GPIO_Handle, GPIO_INPUT_CHANNEL, 0xFFFF);     // Switches and push buttons as input
    XGpio_SetDataDirection(&AXI_GPIO_Handle, GPIO_OUTPUT_CHANNEL, 0x0000);  

    // Init AXI Timer 1 as periodic
    Status = init_PeriodicTimer(&AXI_SampleTimerHandle, XPAR_AXI_TIMER_1_BASEADDR, XTC_TIMER_0, (u32)(XPAR_CPU_CORE_CLOCK_FREQ_HZ / DEFAULT_AUDIO_FREQUENCY), TimerCallbackSample_ISR);
    if (Status != true)
        InitFailMode |= INIT_FAIL_TIMER_1;

    // Init AXI Timer 2 as periodic
    Status = init_PeriodicTimer(&AXI_ModeTimerHandle, XPAR_AXI_TIMER_2_BASEADDR, XTC_TIMER_0, MODE_TIMER_COUNT, TimerCallbackMode_ISR);
    if (Status != true)
        InitFailMode |= INIT_FAIL_TIMER_2;

    // Init AXI Timer 3 as PWM
    Status = init_PWM(&AXI_PWM_Handle, XPAR_AXI_TIMER_3_BASEADDR);
    if (Status != true)
        InitFailMode |= INIT_FAIL_TIMER_3;

    // Init AXI SPI UI
    Status = init_QSPI_PollingMode(&AXI_SPI_UI_Handle, XPAR_AXI_QUAD_SPI_0_BASEADDR);
    if (Status != true)
        InitFailMode |= INIT_FAIL_SPI_0;

    // Init AXI IRQ Controller (6x Steps)
    // Step 1 of 6 IRQ Controller setup: Init or IRQ Controller
    Status = init_IRQ_Controller(&AXI_IRQ_ControllerHandle, XPAR_AXI_INTC_0_BASEADDR);
    if (Status != true)
        InitFailMode |= INIT_FAIL_IRQ_CONTROLLER;
    // Step 2A of 4 IRQ Controller setup: AXI Audio Timer 
    Status = connectPeripheralFast_IRQ(&AXI_IRQ_ControllerHandle, XPAR_FABRIC_AXI_TIMER_1_INTR, TimerCallbackSample_ISR, &AXI_SampleTimerHandle);
    if (Status != true)
        InitFailMode |= INIT_FAIL_IRQ_CONTROLLER;
    // Step 2B of 4 IRQ Controller setup: AXI Generic Timer 
    Status = connectPeripheralFast_IRQ(&AXI_IRQ_ControllerHandle, XPAR_FABRIC_AXI_TIMER_2_INTR, TimerCallbackMode_ISR, &AXI_ModeTimerHandle);
    if (Status != true)
        InitFailMode |= INIT_FAIL_IRQ_CONTROLLER;
    // Step 3 IRQ Controller setup: Start
    Status = start_IRQ_Controller(&AXI_IRQ_ControllerHandle, XIN_REAL_MODE);
    if (Status != true)
        InitFailMode |= INIT_FAIL_IRQ_CONTROLLER;
    // Step 4 IRQ Controller setup: Enable Peripheral Interrupts
    enableDevice_IRQ_Controller(&AXI_IRQ_ControllerHandle, XPAR_FABRIC_AXI_TIMER_1_INTR);
    enableDevice_IRQ_Controller(&AXI_IRQ_ControllerHandle, XPAR_FABRIC_AXI_TIMER_2_INTR);
    // Step 5 IRQ Controller setup: Enable Exceptions
    enableExceptionHandling(&AXI_IRQ_ControllerHandle); 
    // Start / enable peripherals after IRQ Controller setup 
    startPeriodicTimer(&AXI_SampleTimerHandle, XTC_TIMER_0);
    startPeriodicTimer(&AXI_ModeTimerHandle, XTC_TIMER_0);
    setup_PWM(&AXI_PWM_Handle, 100000, 50.0);


    // STEP 3: Init Drivers
    // IO Expander 1:
    Status = init_MCP23S08(&IOX_1, IOX_Reset, IOX_ChipSelect, displayTrasmitReceive, sleep_ms_Wrapper, 
                           &AXI_SPI_UI_Handle, IOX_1_CS_NUMBER, IOX_1_DEVICE_ADDR, IOX_1_IO_DIRECTION, IOX_1_INPUT_POLARITY, IOX_1_IRQ_ON_CHANGE, 
                           IOX_1_IRQ_DEFAULT_VALUE, IOX_1_IRQ_CONTROL, IOX_1_CONFIGURATION, IOX_1_PULLUP, false, true);
    if (Status != true)
        InitFailMode |= INIT_FAIL_UI_IO;
    // IO Expander 2:
    Status = init_MCP23S08(&IOX_2, IOX_Reset, IOX_ChipSelect, displayTrasmitReceive, sleep_ms_Wrapper, 
                           &AXI_SPI_UI_Handle, IOX_2_CS_NUMBER, IOX_2_DEVICE_ADDR, IOX_2_IO_DIRECTION, IOX_2_INPUT_POLARITY, IOX_2_IRQ_ON_CHANGE, 
                           IOX_2_IRQ_DEFAULT_VALUE, IOX_2_IRQ_CONTROL, IOX_2_CONFIGURATION, IOX_2_PULLUP, false, false);
    if (Status != true)
        InitFailMode |= INIT_FAIL_UI_IO;


    // STEP 4: Init Middleware
    // Init FAT FS
    if (is_MicroSD_Inserted())
    {
        FileResult = f_mount(&FatFs, ROOT_PATH, 1);
        if (FileResult != FR_OK)
            InitFailMode |= INIT_FAIL_FAT_FS;
    }
    

    // Init Display
    Status = init_Display_SSD1309(&Display_SSD1309, &AXI_SPI_UI_Handle, DISPLAY_CS_NUMBER, XPAR_AXI_QUAD_SPI_0_FIFO_SIZE, displayResetOrRun, displayCommandOrData, displayTrasmitReceive, displayChipSelect, sleep_ms_Wrapper, sleep_10us_Wrapper, &U8G2); 
    if (Status != true)
        InitFailMode |= INIT_FAIL_UI_DISPLAY;


    // STEP 5: Init Application
    // Init Application Common
    Status = init_SoftCoreHandleCommon(&SoftCore_SA, DEFAULT_AUDIO_FREQUENCY);
    if (Status != true)
        InitFailMode |= INIT_FAIL_SOFTCORE_SA;
    
    // Init Applicaiton Audio
    Status = init_SoftCoreHandleAudio(&SoftCore_SA);
    if (Status != true)
        InitFailMode |= INIT_FAIL_SOFTCORE_SA;


    // STEP 6: Welcome
    terminal_ClearScreen();
    uint32_t PL_Ver = XGpio_DiscreteRead(&AXI_GPIO_Handle, GPIO_INPUT_CHANNEL);
    PL_Ver = (PL_Ver & HW_PL_VER_MASK) >> HW_PL_VER_OFFSET;
    printGreen("IMR Engineering, LLC\r\n");
    printGreen("  Hab Collector, Principal Engineer\r\n");
    printGreen("  http://www.imrengineering.com\r\n\n");
    xil_printf("Softcore Spectrum Analyzer\r\n");
    xil_printf("PS REV: %02d.%02d.%02d\r\n", FW_MAJOR_REV, FW_MINOR_REV, FW_TEST_REV);
    xil_printf("PL VER: %d\r\n\n", PL_Ver);
    if (InitFailMode)
    {
        printBrightRed("Error on Init:\r\n");
        snprintf(PrintBuffer, sizeof(PrintBuffer), "Init Fail Code(s): 0x%04X\r\n\n",InitFailMode);
        printBrightRed(PrintBuffer);
        fflush(stdout);
        while(1);
    }
    else
    {
        xil_printf("Hello Hab, I am ready...\r\n\n");
        displayWelcomeScreen(&Display_SSD1309, FW_MAJOR_REV, FW_MINOR_REV, FW_TEST_REV, HW_REV);
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

    if (SoftCore_SA.Audio_SA.File.uSD_Present)
    {
        countFilesInDirectory(AUDIO_DIRECTORY, &SoftCore_SA.Audio_SA.File.DirectoryFileCount);
        getNextWavFile(AUDIO_DIRECTORY, SoftCore_SA.Audio_SA.File.Name, SoftCore_SA.Audio_SA.File.PathFileName, &SoftCore_SA.Audio_SA.File.Size, SoftCore_SA.Audio_SA.File.DirectoryFileCount);
        drawStaticHeaderAudio(&Display_SSD1309, DISPLAY_AUDIO_HEADING, SoftCore_SA.Audio_SA.File.Name, DISPLAY_AUDIO_PLAY, 235);
    }
    
    while(1)
    {
        processUserInput(&SoftCore_SA);
    }
}



/********************************************************************************************************
* @brief Init of Soft Core Spectrum Analyzer Handle members specific to the common use
*
* @author original: Hab Collector \n
*
* @note: Must be init before main application can be called
* 
* @param Handle: Pointer to Soft Core SA structure
*
* @return True if init OK
*
* STEP 1: Set common handle members
********************************************************************************************************/
static bool init_SoftCoreHandleCommon(Type_SoftCore_SA *Handle, uint32_t SampleFrequency)
{
    // STEP 1: Set common handle members
    // Set LEDs
    Handle->UI_LED_Status = 0x00;   // All LEDs off
    // Set Mode
    Handle->Mode = MODE_AUDIO_SA;
    // FFT
    Handle->FFT.FrameReady = false;
    Handle->FFT.Size = FFT_SIZE;
    Handle->FFT.RBW = (float)SampleFrequency / FFT_SIZE;
    // Calculate the FFT Hann Window
    for (uint16_t N = 0; N < FFT_SIZE; N++)
    {
        Handle->FFT.HannWindow[N] = 0.5 * (1 - cos((2* M_PI* N)/(FFT_SIZE - 1)));
    }

    return(true);

} // END OF init_SoftCoreHandle



/********************************************************************************************************
* @brief Init of Soft Core Spectrum Analyzer Handle members specific to the Audio function
*
* @author original: Hab Collector \n
*
* @note: Must be init before main application can be called
* @note: Requires prior init of FAT FS
* 
* @param Handle: Pointer to Soft Core SA structure
* @param SampleFrequency: Sample frequency of the FFT - must be (Nyquist) 2x the signal frequency 
*
* @return True if init OK
*
* STEP 1: Set audio handle defaults
********************************************************************************************************/
static bool init_SoftCoreHandleAudio(Type_SoftCore_SA *Handle)
{
    // STEP 1: Set audio handle defaults
    Handle->Audio_SA.Enable = false;
    Handle->Audio_SA.File.uSD_Present = is_MicroSD_Inserted();
    Handle->Audio_SA.File.IsOpen = false;
    memset(Handle->Audio_SA.File.Name, 0x00, sizeof(Handle->Audio_SA.File.Name));
    memset(Handle->Audio_SA.File.PathFileName, 0x00, sizeof(Handle->Audio_SA.File.PathFileName));
    Handle->Audio_SA.File.DirectoryFileCount = 0;
    FRESULT FileResult = countFilesInDirectory(AUDIO_DIRECTORY, &Handle->Audio_SA.File.DirectoryFileCount);
    if ((FileResult != FR_OK) || (Handle->Audio_SA.File.DirectoryFileCount == 0))
        return(false);
    else
        return(true);

} // END OF init_SoftCoreHandle



// END OF PROCESSOR DEFINE FOR RUN_MAIN_APPLICATION
#endif


#define ISR_USE_DIRECT_REGISTER_ACCESS
__attribute__((section(".Hab_Fast_Text")))
static void TimerCallbackSample_ISR(void)
{
    // Mark the start of the ISR with IO toggle for testing only
    uint32_t CurrentOutput_GPIO = Xil_In32(XPAR_AXI_GPIO_0_BASEADDR + XGPIO_DATA2_OFFSET);
    uint32_t Output_GPIO = (CurrentOutput_GPIO ^ TIMER_1_OUTPUT);
    Xil_Out32(XPAR_AXI_GPIO_0_BASEADDR + XGPIO_DATA2_OFFSET, Output_GPIO);

    // STEP 1: Clear the interrupt 2 different methods - both are essentially the same
#ifdef ISR_USE_DIRECT_REGISTER_ACCESS
    uint32_t RegisterValue = Xil_In32(XPAR_AXI_TIMER_1_BASEADDR + XTC_TCSR_OFFSET);
    Xil_Out32(XPAR_AXI_TIMER_1_BASEADDR + XTC_TCSR_OFFSET, RegisterValue);
#else
    uint32_t ControlStatusReg = XTmrCtr_ReadReg(XPAR_AXI_TIMER_1_BASEADDR, 0, XTC_TCSR_OFFSET);
    XTmrCtr_WriteReg(XPAR_AXI_TIMER_1_BASEADDR, 0, XTC_TCSR_OFFSET, ControlStatusReg);
#endif

    // STEP 2: User Logic
    static volatile uint32_t Inc = 0;
    Inc++;
    if (Inc >= 1000)
    {
        DutyCyclePercent += 1;
        if (DutyCyclePercent >= 100)
            DutyCyclePercent = 1;
        Inc = 0;
    }

    // STEP 3: Ack at interrupt Controller
#ifdef ISR_USE_DIRECT_REGISTER_ACCESS
    Xil_Out32(XPAR_AXI_INTC_0_BASEADDR + IAR_OFFSET, (1 << XPAR_FABRIC_AXI_TIMER_1_INTR));
#else
    XIntc_AckIntr(XPAR_AXI_INTC_0_BASEADDR, 1 << XPAR_FABRIC_AXI_TIMER_1_INTR);
#endif

    // Mark the end of the ISR with IO toggle for testing sake only
    Output_GPIO = (Output_GPIO ^ TIMER_1_OUTPUT);
    Xil_Out32(XPAR_AXI_GPIO_0_BASEADDR + XGPIO_DATA2_OFFSET, Output_GPIO);
}

__attribute__((section(".Hab_Fast_Text")))
static void TimerCallbackMode_ISR(void)
{
    // STEP 1: Clear the interrupt
    uint32_t ControlStatusReg = XTmrCtr_ReadReg(XPAR_AXI_TIMER_2_BASEADDR, 0, XTC_TCSR_OFFSET);
    XTmrCtr_WriteReg(XPAR_AXI_TIMER_2_BASEADDR, 0, XTC_TCSR_OFFSET, ControlStatusReg);

    // STEP 2: Toggle the test point
    static volatile bool ToggleTimer_2 = false;
    if (ToggleTimer_2)
        XGpio_DiscreteSet(&AXI_GPIO_Handle, GPIO_OUTPUT_CHANNEL, TIMER_2_OUTPUT);
    else
        XGpio_DiscreteClear(&AXI_GPIO_Handle, GPIO_OUTPUT_CHANNEL, TIMER_2_OUTPUT);
    ToggleTimer_2 = !ToggleTimer_2;
    
    // STEP 3: Update the Mode LED with toggle action
    if (SoftCore_SA.Mode)
    {
        SoftCore_SA.UI_LED_Status &= ~LED_MODE_SIGNAL;
        SoftCore_SA.UI_LED_Status ^= LED_MODE_AUDIO;
    }
    else
    {
        SoftCore_SA.UI_LED_Status &= ~LED_MODE_AUDIO;
        SoftCore_SA.UI_LED_Status ^= LED_MODE_SIGNAL;
    }
    MCP23S08_WriteOutput(&IOX_1, SoftCore_SA.UI_LED_Status);

    // STEP 4: Ack at interrupt Controller
    XIntc_AckIntr(XPAR_AXI_INTC_0_BASEADDR, 1 << XPAR_FABRIC_AXI_TIMER_2_INTR);
}



static void processUserInput(Type_SoftCore_SA *SoftCore_SA)
{
    static uint32_t PreviousUserInput = 0;

    uint32_t PresentSwitchState = XGpio_DiscreteRead(&AXI_GPIO_Handle, GPIO_INPUT_CHANNEL);
    if (!(PresentSwitchState & IOX_2_IRQ))
        return;
    
    uint8_t UI_Input = MCP23S08_ReadClear_IRQ(&IOX_2, RISING_EDGE);
    switch (UI_Input)
    {
        case MODE_SW:
        {
            xil_printf("SW1 Pressed\r\n"); 
            modeSwitch(SoftCore_SA);
        }
        break;

        case SELECT_SW:
        {
            xil_printf("SW2 Pressed\r\n"); 
            selectSwitch(SoftCore_SA);
        }
        break;

        case UI_SW3:
        {
            xil_printf("SW3 Pressed\r\n"); 
            drawSpectrumMock(&Display_SSD1309);
        }
        break;

        case UI_SW4:
        {
            xil_printf("SW4 Pressed\r\n"); 
        }
        break;

        case UI_SW5:
        {
            xil_printf("SW5 Pressed\r\n"); 
        }
        break;

        default:
        break;
    } // END OF CASE
}


static void modeSwitch(Type_SoftCore_SA *SoftCore_SA)
{
    // STEP 1: Swith the present present mode and take action to be at default state of the new mode
    if (SoftCore_SA->Mode == MODE_AUDIO_SA)
    {
        SoftCore_SA->Mode = MODE_SIGNAL_SA;
        drawStaticHeaderSignal(&Display_SSD1309, DISPLAY_SIGNAL_HEADING);
        
    }
    else
    {
        SoftCore_SA->Mode = MODE_AUDIO_SA;
        drawStaticHeaderAudio(&Display_SSD1309, DISPLAY_AUDIO_HEADING, SoftCore_SA->Audio_SA.File.Name, DISPLAY_AUDIO_STOP, 0);
    }

}

static void selectSwitch(Type_SoftCore_SA *SoftCore_SA)
{
    // STEP 1: Based on Mode make a section
    if (SoftCore_SA->Mode == MODE_AUDIO_SA)
    {
        bool Status = false;
        uint16_t FilesChecked = 0;
        do 
        {
            getNextWavFile(AUDIO_DIRECTORY, SoftCore_SA->Audio_SA.File.Name, SoftCore_SA->Audio_SA.File.PathFileName, &SoftCore_SA->Audio_SA.File.Size, SoftCore_SA->Audio_SA.File.DirectoryFileCount);
            Status = getWavFileHeader(SoftCore_SA->Audio_SA.File.PathFileName, SoftCore_SA->Audio_SA.File.Size, &SoftCore_SA->Audio_SA.File.Header);
            FilesChecked++;
        } while((Status == false) && (FilesChecked < SoftCore_SA->Audio_SA.File.DirectoryFileCount));
        stopAudioProcessing();
        drawStaticHeaderAudio(&Display_SSD1309, DISPLAY_AUDIO_HEADING, SoftCore_SA->Audio_SA.File.Name, DISPLAY_AUDIO_STOP, 0);
    }
}