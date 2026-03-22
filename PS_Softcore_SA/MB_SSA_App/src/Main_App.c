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
#include <stdbool.h>
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
#include <string.h>
#include "AXI_Timer_PWM_Support.h"
#include "AXI_UART_Lite_Support.h"
#include "AXI_IRQ_Controller_Support.h"
#include "Terminal_Emulator_Support.h"
#include "AXI_QSPI_Support.h"
#include "AXI_IMR_PL_Revision.h"
#include "AXI_IMR_ADC_7476A_DUAL.h"
#include "IO_Support.h"
#include "Audio_File_API.h"
#include "Audio_SoftCore_SA.h"
#include "Signal_SoftCore_SA.h"
#include "MCP23S08_Driver.h"
#include "Water_Mark.h"
#include "Application_Display.h"


// STATIC FUNCTIONS
static void main_InitApplication(void);
static void main_WhileLoop(void);
static bool init_SoftCoreHandleCommon(Type_SoftCore_SA *Handle, uint32_t SampleFrequency);
static bool init_SoftCoreHandleAudio(Type_SoftCore_SA *Handle);
static bool init_SoftCoreHandleSignal(Type_SoftCore_SA *Handle);
static void processUserInput(Type_SoftCore_SA *SoftCore_SA);
static void modeSwitch(Type_SoftCore_SA *SoftCore_SA);
static void selectSwitch(Type_SoftCore_SA *SoftCore_SA);
static void updateModeStatus_LED(void);
// ISR Callbacks
static void TimerCallbackSampleRate_ISR(void) __attribute__((fast_interrupt));
static void TimerCallbackModeStatus_ISR(void) __attribute__((fast_interrupt));
static void ADC_7476A_Primary_ISR(void) __attribute__((fast_interrupt));


// AXI SUPPORT:
XGpio __attribute__ ((section (".Hab_Fast_Data"))) AXI_GPIO_Handle;
XTmrCtr __attribute__ ((section (".Hab_Fast_Data"))) AXI_SampleTimerHandle;
XTmrCtr __attribute__ ((section (".Hab_Fast_Data"))) AXI_ModeTimerHandle;
XTmrCtr __attribute__ ((section (".Hab_Fast_Data"))) AXI_PWM_Handle;
XUartLite __attribute__ ((section (".Hab_Fast_Data"))) AXI_UART_Handle;
XSpi __attribute__ ((section (".Hab_Fast_Data"))) AXI_SPI_UI_Handle;
XSpi __attribute__ ((section (".Hab_Fast_Data"))) AXI_SPI_USD_Handle;
XIntc __attribute__ ((section (".Hab_Fast_Data"))) AXI_IRQ_ControllerHandle;
Type_AXI_IMR_7476A_Handle __attribute__ ((section (".Hab_Fast_Data"))) AXI_IMR_7476A_Handle;


// NON AXI PERIPHERAL:
Type_SoftCore_SA __attribute__ ((section (".Hab_Fast_Data"))) SoftCore_SA;
Type_Display_SSD1309 __attribute__ ((section (".Hab_Fast_Data"))) Display_SSD1309; 
Type_MCP23S08_Driver __attribute__ ((section (".Hab_Fast_Data"))) IOX_1;
Type_MCP23S08_Driver __attribute__ ((section (".Hab_Fast_Data"))) IOX_2;
u8g2_t __attribute__ ((section (".Hab_Fast_Data"))) U8G2; 
FATFS __attribute__ ((section (".Hab_Fast_Data"))) FatFs;
#define ADC_SAMPLE_SIZE 3
volatile uint16_t __attribute__ ((section (".Hab_Fast_Data"))) AnalogInputSignal[ADC_SAMPLE_SIZE];
volatile uint16_t __attribute__ ((section (".Hab_Fast_Data"))) BatteryVoltage[ADC_SAMPLE_SIZE];
uint32_t StackUsedWaterMark = 0;


extern uint8_t __Hab_Fast_Text_start;
extern uint8_t __Hab_Fast_Text_end;
extern uint8_t __Hab_Fast_Text_load;


/********************************************************************************************************
* @brief This is the mian application - it is broken up into two parts - the main init and the main never
* ending loop.  Before the appliction starts Hab_Fast_Text is loaded from DDR to LBM.  To faciliate the .mcs
* image all loadable sections must be placed within DDR.  The watermark for debug purposes only is also seeded
*
* @author original: Hab Collector \n
*
* STEP 1: Load from DDR to LBM linker section .Hab_Fast_Text
* STEP 2: Seed the watermark for testing of stack overflow only - used in conjunction with getStackHighWaterMarkBytes
* STEP 3: Init the application
* STEP 4: Applicaiton Endless Loop 
********************************************************************************************************/
void mainApplication(void)
{
    // STEP 1: Load from DDR to LBM linker section .Hab_Fast_Text
    memcpy(&__Hab_Fast_Text_start, &__Hab_Fast_Text_load, (size_t)(&__Hab_Fast_Text_end - &__Hab_Fast_Text_start));
    
    // STEP 2: Seed the watermark for testing of stack overflow only - used in conjunction with getStackHighWaterMarkBytes
    seedStackForWaterMark();

    // STEP 3: Init the application
    main_InitApplication();

    // STEP 4: Applicaiton Endless Loop 
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
    StackUsedWaterMark = getStackHighWaterMarkBytes();
    

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
    Status = init_PeriodicTimer(&AXI_SampleTimerHandle, XPAR_AXI_TIMER_1_BASEADDR, XTC_TIMER_0, (u32)(XPAR_CPU_CORE_CLOCK_FREQ_HZ / DEFAULT_AUDIO_FREQUENCY), TimerCallbackSampleRate_ISR);
    if (Status != true)
        InitFailMode |= INIT_FAIL_TIMER_1;

    // Init AXI Timer 2 as periodic
    Status = init_PeriodicTimer(&AXI_ModeTimerHandle, XPAR_AXI_TIMER_2_BASEADDR, XTC_TIMER_0, MODE_TIMER_COUNT, TimerCallbackModeStatus_ISR);
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

    // Init Custom IP ADC7476A
    Status = init_IMR_ADC_7476A_X2(&AXI_IMR_7476A_Handle, XPAR_IMR_ADC_7476A_X2_0_BASEADDR ,IMR_ADC_CLOCK_DIVIDER);
    if (Status != true)
        InitFailMode |= INIT_FAIL_ADC7476A;

    // Init AXI IRQ Controller (6x Steps)
    // Step 1 of 6 IRQ Controller setup: Init or IRQ Controller
    Status = init_IRQ_Controller(&AXI_IRQ_ControllerHandle, XPAR_AXI_INTC_0_BASEADDR);
    if (Status != true)
        InitFailMode |= INIT_FAIL_IRQ_CONTROLLER;
    // Step 2A of 6 IRQ Controller setup: AXI Audio Timer 
    Status = connectPeripheralFast_IRQ(&AXI_IRQ_ControllerHandle, XPAR_FABRIC_AXI_TIMER_1_INTR, TimerCallbackSampleRate_ISR, &AXI_SampleTimerHandle);
    if (Status != true)
        InitFailMode |= INIT_FAIL_IRQ_CONTROLLER;
    // Step 2B of 6 IRQ Controller setup: AXI Generic Timer 
    Status = connectPeripheralFast_IRQ(&AXI_IRQ_ControllerHandle, XPAR_FABRIC_AXI_TIMER_2_INTR, TimerCallbackModeStatus_ISR, &AXI_ModeTimerHandle);
    if (Status != true)
        InitFailMode |= INIT_FAIL_IRQ_CONTROLLER;
    // Step 2C of 6 Custom ADC IP: ADC7476A 2x Channel 
    Status = connectPeripheralFast_IRQ(&AXI_IRQ_ControllerHandle, ADC_7476A_X2_FABRIC_ID, ADC_7476A_Primary_ISR, &AXI_IMR_7476A_Handle);
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
    pauseSpecificIRQ(&AXI_IRQ_ControllerHandle, XPAR_FABRIC_AXI_TIMER_1_INTR);
    StackUsedWaterMark = getStackHighWaterMarkBytes();


    // STEP 3: Init Drivers
    // IO Expander 1:
    Status = init_MCP23S08(&IOX_1, IOX_Reset, IOX_ChipSelect, userInterfaceTrasmitReceive, sleep_ms_Wrapper, 
                           &AXI_SPI_UI_Handle, IOX_1_CS_NUMBER, IOX_1_DEVICE_ADDR, IOX_1_IO_DIRECTION, IOX_1_INPUT_POLARITY, IOX_1_IRQ_ON_CHANGE, 
                           IOX_1_IRQ_DEFAULT_VALUE, IOX_1_IRQ_CONTROL, IOX_1_CONFIGURATION, IOX_1_PULLUP, false, true);
    if (Status != true)
        InitFailMode |= INIT_FAIL_UI_IO;
    // IO Expander 2:
    Status = init_MCP23S08(&IOX_2, IOX_Reset, IOX_ChipSelect, userInterfaceTrasmitReceive, sleep_ms_Wrapper, 
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
    Status = init_Display_SSD1309(&Display_SSD1309, &AXI_SPI_UI_Handle, DISPLAY_CS_NUMBER, XPAR_AXI_QUAD_SPI_0_FIFO_SIZE, displayResetOrRun, displayCommandOrData, userInterfaceTrasmitReceive, displayChipSelect, sleep_ms_Wrapper, sleep_10us_Wrapper, &U8G2); 
    if (Status != true)
        InitFailMode |= INIT_FAIL_UI_DISPLAY;


    // STEP 5: Init Application
    // Init Application Common
    Status = init_SoftCoreHandleCommon(&SoftCore_SA, DEFAULT_AUDIO_FREQUENCY);
    if (Status != true)
        InitFailMode |= INIT_FAIL_SOFTCORE_SA;
    
    // Init Applicaiton Audio SA
    Status = init_SoftCoreHandleAudio(&SoftCore_SA);
    if (Status != true)
        InitFailMode |= INIT_FAIL_SOFTCORE_SA;

    // Init Applicaiton Signal SA
    Status = init_SoftCoreHandleSignal(&SoftCore_SA);
    if (Status != true)
        InitFailMode |= INIT_FAIL_SOFTCORE_SA;

    StackUsedWaterMark = getStackHighWaterMarkBytes();
    

    // STEP 6: Welcome
    terminal_ClearScreen();
    // PL BLK GPIO Revision
    uint32_t PL_Ver = XGpio_DiscreteRead(&AXI_GPIO_Handle, GPIO_INPUT_CHANNEL);
    PL_Ver = (PL_Ver & HW_PL_VER_MASK) >> HW_PL_VER_OFFSET;
    // PL IMR Revision
    Type_PL_Revision PL_Revision = IMR_PL_RevisionGet(XPAR_IMR_PL_REVISION_0_BASEADDR);
    // Display to screen
    printGreen("IMR Engineering, LLC\r\n");
    printGreen("  Hab Collector, Principal Engineer\r\n");
    printGreen("  http://www.imrengineering.com\r\n\n");
    xil_printf("Softcore Spectrum Analyzer\r\n");
    xil_printf("FW REV: %02d.%02d.%02d\r\n", FW_MAJOR_REV, FW_MINOR_REV, FW_TEST_REV);
    xil_printf("PL REV: %02d.%02d.%02d\r\n", PL_Revision.Major, PL_Revision.Minor, PL_Revision.Test);
    xil_printf("PL BLK: %d\r\n", PL_Ver);
    xil_printf("HW REV: %d\r\n\n", HW_REV);
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
        displayWelcomeScreen(&Display_SSD1309, FW_MAJOR_REV, FW_MINOR_REV, FW_TEST_REV, PL_Revision.Major, PL_Revision.Minor, PL_Revision.Test);
        sleep_ms_Wrapper(SPLASH_SCREEN_HOLD_TIME);     
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
    if (SoftCore_SA.Audio_SA.File.uSD_Present)
    {
        countFilesInDirectory(AUDIO_DIRECTORY, &SoftCore_SA.Audio_SA.File.DirectoryFileCount);
        getNextWavFile(AUDIO_DIRECTORY, SoftCore_SA.Audio_SA.File.Name, SoftCore_SA.Audio_SA.File.PathFileName, &SoftCore_SA.Audio_SA.File.Size, SoftCore_SA.Audio_SA.File.DirectoryFileCount);
        displayStaticHeaderAudio(&Display_SSD1309, DISPLAY_AUDIO_HEADING, SoftCore_SA.Audio_SA.File.Name, DISPLAY_AUDIO_STOP, 0, AUDIO_MIN_DB_DISPLAY);
    }
    
    while(1)
    {
        processUserInput(&SoftCore_SA);
        updateModeStatus_LED();

        if (SoftCore_SA.Mode == MODE_AUDIO_SA)
            audioSpectrumAnalyzer(&SoftCore_SA.Audio_SA, &SoftCore_SA.FFT, SoftCore_SA.UI_LED_Status);
        else
            signalSpectrumAnalyzer(&SoftCore_SA.Signal_SA, &SoftCore_SA.FFT);
    }

} // END OF main_WhileLoop



/********************************************************************************************************
* @brief Init of Soft Core Spectrum Analyzer Handle members specific to the common use
*
* @author original: Hab Collector \n
*
* @note: Must be init before main application can be called
* 
* @param Handle: Pointer to Soft Core SA structure
* @param SampleFrequency: Desired sample frequency it must meet Nyquist criteria x2
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
        Handle->FFT.HannWindow[N] = 0.5 * (1 - cos((2* M_PI * N)/(FFT_SIZE - 1)));
    }

    return(true);

} // END OF init_SoftCoreHandle



/********************************************************************************************************
* @brief Init of Soft Core Spectrum Analyzer Handle members specific to the Audio Mode function
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
* STEP 1: Set audio handle defaults
********************************************************************************************************/
static bool init_SoftCoreHandleAudio(Type_SoftCore_SA *Handle)
{
    // STEP 1: Set audio handle defaults
    // File
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
    // Action
    Handle->Audio_SA.AudioAction = AUDIO_ACTION_STOP;

} // END OF init_SoftCoreHandle



/********************************************************************************************************
* @brief Init of Soft Core Spectrum Analyzer Handle members specific to the Signal Mode function
*
* @author original: Hab Collector \n
*
* @note: Must be init before main application can be called
* 
* @param Handle: Pointer to Soft Core SA structure
*
* @return True if init OK
*
* STEP 1: Set audio handle defaults
********************************************************************************************************/
bool init_SoftCoreHandleSignal(Type_SoftCore_SA *Handle)
{
    Handle->Signal_SA.Enable = false;
    Handle->Signal_SA.Source = SIGNAL_ON_BOARD_OSCILLATOR;
    signalSelect(SIGNAL_ON_BOARD_OSCILLATOR);
    return(true);

} // END OF init_SoftCoreHandleSignal




/********************************************************************************************************
* @brief This is the ISR callback for Timer 1.  Timer 1 serves as the periodic timer for both the Audio and
* signal specturm.  The application usings test point TIMER_1_OUTPUT as a means for knowing how long the ISR
* takes to run.  When enabled either sub-ISR responsibe for audio or signal spectrum analyzer duties will run.
* consult audioPeriodicTimer_ISR or signalPeriodicTimer_ISR for respective details
*
* @author original: Hab Collector \n
*
* @note: ISR for low latency (fast) interrupt
* @note: Serves both Audio and Signal Specturm Analyzer duties depending on moode
* @note: This is a low latency (fast) Microblaze ISR and is registered as such and placed in LBM - see attributes
* 
* STEP 1: Mark the start of the ISR with IO toggle for testing only
* STEP 2: Clear the interrupt 2 different methods - both are essentially the same
* STEP 3: Run either the Audio or Signal Spectrum Analyzer sub-ISR routine
* STEP 4: Ack at interrupt Controller
* STEP 5: Mark the end of the ISR with IO toggle for testing sake only
********************************************************************************************************/
#define ISR_USE_DIRECT_REGISTER_ACCESS
__attribute__((section(".Hab_Fast_Text")))
static void TimerCallbackSampleRate_ISR(void)
{
    // STEP 1: Mark the start of the ISR with IO toggle for testing only
    uint32_t CurrentOutput_GPIO = Xil_In32(XPAR_AXI_GPIO_0_BASEADDR + XGPIO_DATA2_OFFSET);
    uint32_t Output_GPIO = (CurrentOutput_GPIO ^ TIMER_1_OUTPUT);
    Xil_Out32(XPAR_AXI_GPIO_0_BASEADDR + XGPIO_DATA2_OFFSET, Output_GPIO);

    // STEP 2: Clear the interrupt 2 different methods - both are essentially the same
#ifdef ISR_USE_DIRECT_REGISTER_ACCESS
    uint32_t RegisterValue = Xil_In32(XPAR_AXI_TIMER_1_BASEADDR + XTC_TCSR_OFFSET);
    Xil_Out32(XPAR_AXI_TIMER_1_BASEADDR + XTC_TCSR_OFFSET, RegisterValue);
#else
    uint32_t ControlStatusReg = XTmrCtr_ReadReg(XPAR_AXI_TIMER_1_BASEADDR, 0, XTC_TCSR_OFFSET);
    XTmrCtr_WriteReg(XPAR_AXI_TIMER_1_BASEADDR, 0, XTC_TCSR_OFFSET, ControlStatusReg);
#endif

    // STEP 3: Run either the Audio or Signal Spectrum Analyzer sub-ISR routine
    if (SoftCore_SA.Mode == MODE_AUDIO_SA)
        audioPeriodicTimer_ISR(&SoftCore_SA.Audio_SA, &SoftCore_SA.FFT);
    else 
        signalPeriodicTimer_ISR(&SoftCore_SA.Signal_SA, &SoftCore_SA.FFT, &AnalogInputSignal, &BatteryVoltage);

    // STEP 4: Ack at interrupt Controller
#ifdef ISR_USE_DIRECT_REGISTER_ACCESS
    Xil_Out32(XPAR_AXI_INTC_0_BASEADDR + IAR_OFFSET, (1 << XPAR_FABRIC_AXI_TIMER_1_INTR));
#else
    XIntc_AckIntr(XPAR_AXI_INTC_0_BASEADDR, 1 << XPAR_FABRIC_AXI_TIMER_1_INTR);
#endif

    // STEP 5: Mark the end of the ISR with IO toggle for testing sake only
    Output_GPIO = (Output_GPIO ^ TIMER_1_OUTPUT);
    Xil_Out32(XPAR_AXI_GPIO_0_BASEADDR + XGPIO_DATA2_OFFSET, Output_GPIO);

} // END OF TimerCallbackSampleRate_ISR



/********************************************************************************************************
* @brief This is the ISR callback for Timer 21.  Timer 1 serves as the periodic timer for status LEDs.  LED7
* (Audio Status) and LED8 (Signal Staus) will blink at this ISR rate depending on the moode the device is in.
*
* @author original: Hab Collector \n
*
* @note: ISR for low latency (fast) interrupt
* @note: Serves both Audio and Signal Specturm Analyzer duties depending on mode
* @note: This is a low latency (fast) Microblaze ISR and is registered as such and placed in LBM - see attributes
* @note: As this IRQ rate and what it needs to do is very slow - no need to time
* @note: For debug testing it is recommend to disable this interrupt
*
* STEP 1: Clear the interrupt
* STEP 2: Toggle the test point
* STEP 3: Update the Mode LED with toggle action
* STEP 4: Ack at interrupt Controller
********************************************************************************************************/
__attribute__((section(".Hab_Fast_Text")))
static void TimerCallbackModeStatus_ISR(void)
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

    // STEP 4: Ack at interrupt Controller
    XIntc_AckIntr(XPAR_AXI_INTC_0_BASEADDR, 1 << XPAR_FABRIC_AXI_TIMER_2_INTR);

} // END OF TimerCallbackModeStatus_ISR



/********************************************************************************************************
* @brief This is the ISR from the for the ADC 7476A x2 PL IRQ.  It signals that a convervsion is complete.
* The ISR is made active on every conversion (single and multi-conversion moodes).  In other words in multi-
* conversion for a 3 conversion set it would be active 3 times.
*
* @author original: Hab Collector \n
*
* @note: ISR for low latency (fast) interrupt
* @note: This is a low latency (fast) Microblaze ISR and is registered as such and placed in LBM - see attributes
* @note: For debug testing it is recommend to disable this interrupt
********************************************************************************************************/
__attribute__((section(".Hab_Fast_Text")))
static void ADC_7476A_Primary_ISR(void)
{
    signal_ADC_7476A_ISR(&SoftCore_SA.Signal_SA, &SoftCore_SA.FFT, &AXI_IMR_7476A_Handle);
}



/********************************************************************************************************
* @brief Process the user input - this function should be called when IO Expander 2 IRQ goes active. IO
* Expander 2 represents all UI inputs
*
* @author original: Hab Collector \n
*
* @note: IO Expander 2 must be init
* @note: Only process on IRQ from IO Expander 2
*
* @param SoftCore_SA: Pointer to the application main handle
*
* STEP 1: Only act upon change in input
* STEP 2: Process the input and perform an action based unique to Audio or Signal Spectrum Mode
********************************************************************************************************/
static void processUserInput(Type_SoftCore_SA *SoftCore_SA)
{
    static uint32_t PreviousUserInput = 0;

    // STEP 1: Only act upon change in input
    uint32_t PresentSwitchState = XGpio_DiscreteRead(&AXI_GPIO_Handle, GPIO_INPUT_CHANNEL);
    if (!(PresentSwitchState & IOX_2_IRQ))
        return;
    
    // STEP 2: Process the input and perform an action based unique to Audio or Signal Spectrum Mode
    uint8_t UI_Input = MCP23S08_ReadClear_IRQ(&IOX_2, RISING_EDGE);
    switch (UI_Input)
    {
        case MODE_SW:
        {
            modeSwitch(SoftCore_SA);
        }
        break;

        case SELECT_SW:
        {
            selectSwitch(SoftCore_SA);
        }
        break;

        case UI_SW3:
        {
            if (SoftCore_SA->Mode == MODE_AUDIO_SA)
            {
                printMagenta("Audio Stop\r\n");
                stopAudio_SA(&SoftCore_SA->Audio_SA, &SoftCore_SA->FFT);
            }
        }
        break;

        case UI_SW4:
        {
            if (SoftCore_SA->Mode == MODE_AUDIO_SA)
            {
                printMagenta("Audio Pause\r\n"); 
                pauseAudio_SA(&SoftCore_SA->Audio_SA);
            }
        }
        break;

        case UI_SW5:
        {
            if (SoftCore_SA->Mode == MODE_AUDIO_SA)
            {
                printMagenta("Audio Play\r\n");
                playAudio_SA(&SoftCore_SA->Audio_SA, &SoftCore_SA->FFT);
            }
        }
        break;

        default:
        break;
    } // END OF CASE

} // END OF processUserInput



/********************************************************************************************************
* @brief Process the user input SW1.  MODE: A toggle function to switch between Audio and Signal specturm
* modes.  Call the appropiate display and set default start conditions for that mode
*
* @author original: Hab Collector \n
*
* @param SoftCore_SA: Pointer to the application main handle
*
* STEP 1: Toggle the present mode and take action to be at default state of the new mode
********************************************************************************************************/
static void modeSwitch(Type_SoftCore_SA *SoftCore_SA)
{
    bool Status = true;
    // STEP 1: Toggle the present mode and take action to be at default state of the new mode
    // Moving from Audio to Signal Mode
    if (SoftCore_SA->Mode == MODE_AUDIO_SA)
    {
        // Stop Audio playback and disable audio outpput
        stopAudio_SA(&SoftCore_SA->Audio_SA, &SoftCore_SA->FFT);
        // Init for Signal SA
        Status = initSignal_SA(&SoftCore_SA->Signal_SA, &SoftCore_SA->FFT, DEFAULT_SIGNAL_SAMPLE_RATE_HZ);
        if (Status == true)
        {
            // Ready Singal SA
            SoftCore_SA->Mode = MODE_SIGNAL_SA;
            SoftCore_SA->Signal_SA.Enable = true;
            // Re-start timer
            resumeSpecificIRQ(&AXI_IRQ_ControllerHandle, XPAR_FABRIC_AXI_TIMER_1_INTR);
            // Update display and debug port
            displayStaticHeaderSignal(&Display_SSD1309, DISPLAY_SIGNAL_HEADING, 0, (DEFAULT_SIGNAL_SAMPLE_RATE_HZ / 4.0), (DEFAULT_SIGNAL_SAMPLE_RATE_HZ / 2.0), SoftCore_SA->Signal_SA.Source);
            printYellow("Signal Mode Active\r\n"); 
            return;
        }
        else
        {
            printBrightRed("Error Starting Signal Mode\r\n");
            return;
        }
    }
    
    // Moving from Signal to Audio Mode
    if (SoftCore_SA->Mode == MODE_SIGNAL_SA)
    {
        if (Status == true)  // TODO: Hab just a place holder until you get around to an init for audio
        {
            // Stop signal mode
            SoftCore_SA->Signal_SA.Enable = false;
            pauseSpecificIRQ(&AXI_IRQ_ControllerHandle, XPAR_FABRIC_AXI_TIMER_1_INTR);
            // Ready Audio Mode
            SoftCore_SA->Mode = MODE_AUDIO_SA;
            SoftCore_SA->Audio_SA.Enable = true;
            stopAudio_SA(&SoftCore_SA->Audio_SA, &SoftCore_SA->FFT);
            // Update display and debug port
            displayStaticHeaderAudio(&Display_SSD1309, DISPLAY_AUDIO_HEADING, SoftCore_SA->Audio_SA.File.Name, DISPLAY_AUDIO_STOP, 0, AUDIO_MIN_DB_DISPLAY);
            printMagenta("Audio Mode Active\r\n"); 
            return;
        }
    }

} // END OF modeSwitch



/********************************************************************************************************
* @brief Process the user input SW2.  SELECT: select action unique to mode.  In Audio mode selects the next
* WAV file to play and sets the default start condition
*
* @author original: Hab Collector \n
*
* @param SoftCore_SA: Pointer to the application main handle
*
* STEP 1: Audio Mode: Get the next valid file - if valid file found enable Audio SA
* STEP 2: Signal Mode: Toggle the analog signal input between the on board osscilator and external BNC connector
********************************************************************************************************/
static void selectSwitch(Type_SoftCore_SA *SoftCore_SA)
{
    char PrintBuffer[MAX_PRINT_BUFFER] = {0};

    // STEP 1: Audio Mode: Get the next valid file - if valid file found enable Audio SA
    if (SoftCore_SA->Mode == MODE_AUDIO_SA)
    {
        stopAudio_SA(&SoftCore_SA->Audio_SA, &SoftCore_SA->FFT);
        bool Status = false;
        uint16_t FilesChecked = 0;
        do 
        {
            getNextWavFile(AUDIO_DIRECTORY, SoftCore_SA->Audio_SA.File.Name, SoftCore_SA->Audio_SA.File.PathFileName, &SoftCore_SA->Audio_SA.File.Size, SoftCore_SA->Audio_SA.File.DirectoryFileCount);
            Status = getWavFileHeader(SoftCore_SA->Audio_SA.File.PathFileName, SoftCore_SA->Audio_SA.File.Size, &SoftCore_SA->Audio_SA.File.Header);
            FilesChecked++;
        } while((Status == false) && (FilesChecked < SoftCore_SA->Audio_SA.File.DirectoryFileCount));
        
        if (Status == true)
        {
            SoftCore_SA->Audio_SA.Enable = true;
            displayStaticHeaderAudio(&Display_SSD1309, DISPLAY_AUDIO_HEADING, SoftCore_SA->Audio_SA.File.Name, DISPLAY_AUDIO_STOP, 0, AUDIO_MIN_DB_DISPLAY);
            snprintf(PrintBuffer, sizeof(PrintBuffer), "Audio File Select: %s\r\n", SoftCore_SA->Audio_SA.File.Name);
        }
        else
        {
            SoftCore_SA->Audio_SA.Enable = false;
            displayStaticHeaderAudio(&Display_SSD1309, DISPLAY_AUDIO_HEADING, DISPLAY_FILE_ERROR, DISPLAY_AUDIO_ERROR, 0, AUDIO_MIN_DB_DISPLAY);
            snprintf(PrintBuffer, sizeof(PrintBuffer), "Audio File Select Error\r\n");
        }
        printMagenta(PrintBuffer); 
        return;
    }

    // STEP 2: Signal Mode: Toggle the analog signal input between the on board osscilator and external BNC connector
    if (SoftCore_SA->Mode == MODE_SIGNAL_SA)
    {
        if (SoftCore_SA->Signal_SA.Source == SIGNAL_ON_BOARD_OSCILLATOR)
        {
            SoftCore_SA->Signal_SA.Source = SIGNAL_OFF_BOARD_BNC;
            signalSelect(SIGNAL_OFF_BOARD_BNC);
            displayUpdateSignalSource(&Display_SSD1309, SIGNAL_OFF_BOARD_BNC);
            printYellow("Signal Source External BNC\r\n"); 
        }
        else
        {
            SoftCore_SA->Signal_SA.Source = SIGNAL_ON_BOARD_OSCILLATOR;
            signalSelect(SIGNAL_ON_BOARD_OSCILLATOR);
            displayUpdateSignalSource(&Display_SSD1309, SIGNAL_ON_BOARD_OSCILLATOR);
            printYellow("Signal Source On Board Oscillator\r\n"); 
        }
        return;
    }

} // END OF selectSwitch


/********************************************************************************************************
* @brief Blinks the status LED associated with the mode - LED 7 or LED 8.  This function is called contineiously
* but only has something to do based on the tick rate of Timer 2 - The mode status LED interval tick.  The 
* ISR toggles the state of LED 7 or LED 8 depending on the present mode setting Audio SA or Signal SA respectively.
*
* @author original: Hab Collector \n
*
* STEP 1: Update IOX LED output if there has been a change 
********************************************************************************************************/
static void updateModeStatus_LED(void)
{
    static uint8_t PreviousModeState = 0;

    // STEP 1: Update IOX LED output if there has been a change 
    uint8_t PresentModeState = (SoftCore_SA.UI_LED_Status | SoftCore_SA.Audio_SA.LED_BarGraph);
    if (PreviousModeState != PresentModeState)
    {
        MCP23S08_WriteOutput(&IOX_1, PresentModeState);
        PreviousModeState = PresentModeState;
    }

} // END OF updateModeStatus_LED




