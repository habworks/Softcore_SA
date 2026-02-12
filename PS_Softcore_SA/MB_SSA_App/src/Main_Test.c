/******************************************************************************************************
 * @file            Main_Test.c
 * @brief           This is test app used for both function development and testing this is not the SoftCore Application
 *
 * Description: 
 * GPIO ACTION:
 * There are a varity of GPIO input and outputs. for various forms of control or status updates.  No GPIOs are used 
 * serve as input interupts in this case
 *
 * PERIODIC TIMER ACTION:
 * There are 3 AXI Timer IP Block within this design.  
 * axi_timer_0 is configured for use with the Board Support Package (BSP) >> xiltimer and sleep based functions
 * axi_timer 1 is configured as a periodic fast (low latency interrupt) - intended for sampling and audio playback use
 * axi_timer_2 is configured as a periodic fast (low latency interrupt) - for TBD 
 * axi_timer_3 is configured as PWM intended for PWM audio playback 
 * 
 * UART LITE ACTION: 
 * The UART is configure in IRQ mode, but even in IRQ mode it can be used in polling via
 * the function xil_printf - xil_printf is tied to this UART via the Platform settings >> BSP >> standalone >> standalone stdin and stdout
 *
 * QUAD SPI 0 ACTION:
 * This Quad SPI is in the interface of the graphics display.  It is a monochrome display 128x64 piexels.
 * The display uses the U8G2 library and is intervaced via SPI - The chip select of the display is driven by GPIO and not AXI QSPI
 * The display forms the primary UI output 
 *
 * QUAD SPI 1 ACTION:
 * This Quad SPI is used in the interface of a uSD FLASH.  Elm Chan FAT FS is used as the library for file system managment
 * The uSD main purpuse to is for file retrival but both file reads and writes are tested here
 *
 * INTERRUPT CONTROLLER ACTION:
 * Several PL actions generate interruts.  Timer 1, and Timer 2 
 *
 * UI INPUTS
 * SW0 on / off: TBD By testing needs
 * SW1 on / off: TBD By testing needs
 * PB_0: Board level reset
 * PB_1: TBD By testing needs
 * PB_2: TBD By testing needs
 * PB_3: TBD By testing needs
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
#include <xintc_l.h>
#ifndef RUN_MAIN_APPLICATION
// START OF the Main Test (used only for testing peripherals for operation)

#include "xparameters.h"
#include "xtmrctr.h"
#include "xtmrctr_l.h"
#include "xgpio.h"
#include "xintc.h"
#include "xil_exception.h"
#include "xuartlite.h"
#include "xil_printf.h"
#include "xspi.h"
#include "xiltimer.h"
#include "xil_cache.h"
#include <stdint.h>
#include <stdbool.h>
#include <xstatus.h>
#include <unistd.h>
#include "ff.h"
#include "AXI_Timer_PWM_Support.h"
#include "AXI_UART_Lite_Support.h"
#include "AXI_IRQ_Controller_Support.h"
#include "AXI_IMR_ADC_7476A_DUAL.h"
#include "AXI_SPI_Display_SSD1309.h"
#include "AXI_QSPI_Support.h"
#include "AXI_IMR_PL_Revision.h"
#include "IO_Support.h"


// DISPLAY SUPPORT
// #include "AXI_SPI_Display_SSD1309.h"
#include "u8g2.h"
#define DISPLAY_CSN     0x01
XSpi AXI_SPI_DisplayHandle;     // Did not seem to make a difference if placed in fast memory
u8g2_t U8G2; // Did not seem to make a difference if placed in fast memory
Type_Display_SSD1309  Display_SSD1309; // Did not seem to make a difference if placed in fast memory


// DDR 3 SUPPORT
#define DDR3_BASE_ADDRESS       ((uint32_t)(0x80000000))
#define DDR3_TEST_VALUE         ((uint32_t)(0xA5A5A5A5))
// DDR 3 GLOBLAS
// A single byte variable
uint8_t Test_u8_var __attribute__ ((section (".Hab_Fast_Data"))) = 100;
// A 16-bit unsigned integer
uint16_t Test_u16_var __attribute__ ((section (".Hab_Fast_Data"))) = 1000;
// A 32-bit unsigned integer
uint32_t Test_u32_var __attribute__ ((section (".Hab_Fast_Data"))) = 100000;

// GPIO SUPPORT
XGpio AXI_GPIO_Handle;

// UART SUPPORT
XUartLite __attribute__ ((section (".Hab_Fast_Data"))) AXI_UART_Handle;

// TIMER SUPPORT - AUDIO PLAYBACK
#define AUDIO_IRQ_COUNT 1000
XTmrCtr __attribute__ ((section (".Hab_Fast_Data"))) AXI_TimerHandle_1;

// TIMER SUPPORT - GENERIC
XTmrCtr __attribute__ ((section (".Hab_Fast_Data"))) AXI_TimerHandle_2;

// TIMER PWM SUPPORT
XTmrCtr __attribute__ ((section (".Hab_Fast_Data"))) AXI_TimerHandle_3;

// IRQ CONTROLLER SUPPORT
XIntc __attribute__ ((section (".Hab_Fast_Data"))) AXI_IRQ_ControllerHandle;

// FAT FS SUPPORT
static void readFileTest(const char *FileName);
static void writeFileTest(const char *FileName);
FATFS FatFs;    
const char *ReadOnlyFileName = "HelloHab.txt";
const char *ReadWriteFileName = "Test_RW.txt";




// ISR FUNCTIONS:
// ISR Callback function for Timer Number 0 and 1
volatile uint8_t __attribute__ ((section (".Hab_Fast_Data"))) DutyCyclePercent = 1;
void TimerCallbackAudio_ISR(void) __attribute__((fast_interrupt));
void TimerCallbackGeneric_ISR(void) __attribute__((fast_interrupt));


#define ISR_USE_DIRECT_REGISTER_ACCESS
#define IAR_OFFSET 0x0C
__attribute__((section(".Hab_Fast_Text")))
void TimerCallbackAudio_ISR(void)
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
void TimerCallbackGeneric_ISR(void)
{
    // STEP 1: Clear the interrupt
    uint32_t ControlStatusReg = XTmrCtr_ReadReg(XPAR_AXI_TIMER_2_BASEADDR, 0, XTC_TCSR_OFFSET);
    XTmrCtr_WriteReg(XPAR_AXI_TIMER_2_BASEADDR, 0, XTC_TCSR_OFFSET, ControlStatusReg);

    // STEP 2: User does something
    static volatile bool ToggleTimer_0 = false;
    // if (TmrCtrNumber == XTC_TIMER_0)
    {
        if (ToggleTimer_0)
            XGpio_DiscreteSet(&AXI_GPIO_Handle, GPIO_OUTPUT_CHANNEL, TIMER_0_OUTPUT);
        else
            XGpio_DiscreteClear(&AXI_GPIO_Handle, GPIO_OUTPUT_CHANNEL, TIMER_0_OUTPUT);
        ToggleTimer_0 = !ToggleTimer_0;
    }

    // STEP 3: Ack at interrupt Controller
    XIntc_AckIntr(XPAR_AXI_INTC_0_BASEADDR, 1 << XPAR_FABRIC_AXI_TIMER_2_INTR);
}














void mainTest(void)
{
    int AXI_Status;
    bool Status;

    // Enable the instruction and data cache
    Xil_ICacheEnable();
    Xil_DCacheEnable();

    // Init AXI UART
    // Status = init_UART_Lite(&AXI_UART_Handle, XPAR_AXI_UARTLITE_0_BASEADDR, INTERRUPT, UART_TransmitCallback_ISR, UART_ReceiveCallback_ISR);
    Status = init_UART_Lite(&AXI_UART_Handle, XPAR_AXI_UARTLITE_0_BASEADDR, POLLING, NULL, NULL, false);
    if (Status == false)
        while(1);

    // Init AXI GPIO and set default GPIO states
    AXI_Status = XGpio_Initialize(&AXI_GPIO_Handle, XPAR_AXI_GPIO_0_BASEADDR);
    if (AXI_Status != XST_SUCCESS)
        while(1);
    XGpio_SetDataDirection(&AXI_GPIO_Handle, GPIO_INPUT_CHANNEL, 0xFFFFFFFF);     // Switches and push buttons as input
    XGpio_SetDataDirection(&AXI_GPIO_Handle, GPIO_OUTPUT_CHANNEL, 0x0000); 
    displayChipSelect(CS_DISABLE);   

    // Wait for DDR3 to be ready
    u32 GPIO_InputState;
    do 
    {
        GPIO_InputState = XGpio_DiscreteRead(&AXI_GPIO_Handle, GPIO_INPUT_CHANNEL);
    }while (!(GPIO_InputState & DDR_CALIB_COMPLETE));

    // Init AXI Timer 1: Audio Playblack Timer
    Status = init_PeriodicTimer(&AXI_TimerHandle_1, XPAR_AXI_TIMER_1_BASEADDR, XTC_TIMER_0, AUDIO_IRQ_COUNT, TimerCallbackAudio_ISR);
    if (Status == false)
        while(1);

    // Init AXI Timer 2: Generic Timer
    Status = init_PeriodicTimer(&AXI_TimerHandle_2, XPAR_AXI_TIMER_2_BASEADDR, XTC_TIMER_0, 400e6, TimerCallbackGeneric_ISR);
    if (Status == false)
        while(1);

    // Init AXI Timer 3: Audio PWM
    Status = init_PWM(&AXI_TimerHandle_3, XPAR_AXI_TIMER_3_BASEADDR);
    if (Status == false)
        while(1);

    // Init AXI SPI: Display UI Interface
    Status = init_QSPI_PollingMode(&AXI_SPI_DisplayHandle, XPAR_AXI_QUAD_SPI_0_BASEADDR);
    if (Status == false)
        while(1);

    // Init AXI IRQ Controller (6x Steps)
    // Step 1 of 6 IRQ Controller setup: Init or IRQ Controller
    Status = init_IRQ_Controller(&AXI_IRQ_ControllerHandle, XPAR_AXI_INTC_0_BASEADDR);
    if (Status == false)
        while(1);
    // Step 2A of 4 IRQ Controller setup: AXI Audio Timer 
    Status = connectPeripheralFast_IRQ(&AXI_IRQ_ControllerHandle, XPAR_FABRIC_AXI_TIMER_1_INTR, TimerCallbackAudio_ISR, &AXI_TimerHandle_1);
    if (Status == false)
        while(1);
    // Step 2B of 4 IRQ Controller setup: AXI Generic Timer 
    Status = connectPeripheralFast_IRQ(&AXI_IRQ_ControllerHandle, XPAR_FABRIC_AXI_TIMER_2_INTR, TimerCallbackGeneric_ISR, &AXI_TimerHandle_2);
    if (Status == false)
        while(1);
    // Step 3 IRQ Controller setup: Start
    Status = start_IRQ_Controller(&AXI_IRQ_ControllerHandle, XIN_REAL_MODE);
    if (Status == false)
        while(1);
    // Step 4 IRQ Controller setup: Enable Peripheral Interrupts
    enableDevice_IRQ_Controller(&AXI_IRQ_ControllerHandle, XPAR_FABRIC_AXI_TIMER_1_INTR);
    enableDevice_IRQ_Controller(&AXI_IRQ_ControllerHandle, XPAR_FABRIC_AXI_TIMER_2_INTR);
    // Step 5 IRQ Controller setup: Enable Exceptions
    enableExceptionHandling(&AXI_IRQ_ControllerHandle); 

    // Start / enable peripherals after IRQ Controller setup 
    startPeriodicTimer(&AXI_TimerHandle_1, XTC_TIMER_0);
    startPeriodicTimer(&AXI_TimerHandle_2, XTC_TIMER_0);
    setup_PWM(&AXI_TimerHandle_3, 100000, 50.0);

    // Init FAT FS
    xil_printf("Mounting file system...\r\n");
    if (f_mount(&FatFs, "0:/", 1) != FR_OK)
    {
        xil_printf("Mount failed: %d\r\n");
        while(1);
    }
    else 
    {
        xil_printf("Drive mounted OK\r\n");
    }

    // DDR3 Self Test
    // Wait for DDR3 to be ready
    // u32 GPIO_InputState;
    do 
    {
        GPIO_InputState = XGpio_DiscreteRead(&AXI_GPIO_Handle, GPIO_INPUT_CHANNEL);
    }while (!(GPIO_InputState & DDR_CALIB_COMPLETE));
    // Test DDR3 memory
    uint32_t TestValue;
    uint32_t ReadValue;
    TestValue = Test_u8_var + Test_u16_var + Test_u32_var;
    // Use function call to save to DDR3 base memory location
    Xil_Out32(DDR3_BASE_ADDRESS, TestValue);
    ReadValue = Xil_In32(DDR3_BASE_ADDRESS);
    if (ReadValue == TestValue)
        xil_printf("Memory Test 1 OK\r\n");
    else
        xil_printf("Memory Test 1 ERROR\r\n"); 
    // Use pointers to test
    Xil_Out32(DDR3_BASE_ADDRESS, 0);
    ReadValue = 0;
    *((uint32_t*)DDR3_BASE_ADDRESS) = TestValue;
    ReadValue = *((uint32_t*)DDR3_BASE_ADDRESS);
    if (ReadValue == TestValue)
        xil_printf("Memory Test 2 OK\r\n");
    else
        xil_printf("Memory Test 2 ERROR\r\n"); 

    // Init the display
    Status = init_Display_SSD1309(&Display_SSD1309, &AXI_SPI_DisplayHandle, DISPLAY_CSN, XPAR_AXI_QUAD_SPI_0_FIFO_SIZE, displayResetOrRun, displayCommandOrData, displayTrasmitReceive, displayChipSelect, sleep_ms_Wrapper, sleep_10us_Wrapper, &U8G2); 
    if (Status == false)
        while(1);
    displaySimpleTest(&Display_SSD1309);

    // Setup complete - Read to start processing
    // Type_PL_Revision PL_Revision = IMR_PL_RevisionGet(XPAR_IMR_PL_REVISION_0_BASEADDR);
    uint32_t PL_Ver = XGpio_DiscreteRead(&AXI_GPIO_Handle, GPIO_INPUT_CHANNEL);
    PL_Ver = (PL_Ver & HW_CONST_PL_VER) >> 7;
    xil_printf("\r\n\n\nHello Hab I am ready\r\n");
    xil_printf("PS REV: %02d.%02d.%02d\r\n", FW_MAJOR_REV, FW_MINOR_REV, FW_TEST_REV);
    // xil_printf("PL REV: %02d.%02d.%02d\r\n", PL_Revision.Major, PL_Revision.Minor, PL_Revision.Test);
    xil_printf("PL Ver %d\r\n\n", PL_Ver);
    

    u32 SwitchState;
    u32 PreviousSwitchState = 0xFFFFFFFF;
    bool SwitchStateChange = false;
    uint16_t BytesTransmitted = 0; 
    float PreviousDutyCyclePercent = 0;
    while (1)
    {
        // Simple Echo of the input UART
        if (ReceivedBytes)
        {
            transmit_UART(&AXI_UART_Handle, RxDataBuffer, ReceivedBytes, &BytesTransmitted);
            ReceivedBytes -= BytesTransmitted;
        }

        if (DutyCyclePercent != PreviousDutyCyclePercent)
        {
            setup_PWM(&AXI_TimerHandle_3, 100000, DutyCyclePercent);
            PreviousDutyCyclePercent = DutyCyclePercent;
        }
        
        // Read and check the input stats for change
        SwitchState = XGpio_DiscreteRead(&AXI_GPIO_Handle, GPIO_INPUT_CHANNEL);
        if (SwitchState ^ PreviousSwitchState)
            SwitchStateChange = true;
        else
            SwitchStateChange = false;

        if (SwitchStateChange)
        {
            // SWITCH 1
            if (SwitchState & SW_0)
            {
                xil_printf("Switch 1 on\r\n");
            }
            else
            {
                xil_printf("Switch 1 off\r\n");
            }
            // SWITCH 2
            if (SwitchState & SW_1)
            {
                xil_printf("Switch 2 on\r\n");
            }
            else
            {
                xil_printf("Switch 2 off\r\n");
            }
            // Push Button 1
            if (SwitchState & PB_1)
            {
                displayDirectTest(&Display_SSD1309);
                sleep_ms_Wrapper(250);
                displaySimpleTest(&Display_SSD1309);
                xil_printf("Display General Test\r\n");

            }
            // Push Button 2
            if (SwitchState & PB_2)
            {
                // stopPeriodicTimer(&AXI_TimerHandle_1, XTC_TIMER_0);
                drawSpectrumMock(&Display_SSD1309);
                xil_printf("Display Specturm Test\r\n");
                // startPeriodicTimer(&AXI_TimerHandle_1, XTC_TIMER_0);
            }
            // Push Button 3
            if (SwitchState & PB_3)
            {
                // xil_printf("***Single ADC conversion Trigger\r\n");
                // IMR_ADC_7476A_X2_SingleConvert(&AXI_IMR_7476A_Handle, ADC_BufferDataA, ADC_BufferDataB);
                // // Wait for IRQ to occur - Data A and B are loaded by the ISR
                // usleep(200);
                // // Print Data A and B and registers
                // xil_printf("Data A: %d\r\n", AXI_IMR_7476A_Handle.ADC_Data_A[0]);
                // xil_printf("Data B: %d\r\n", AXI_IMR_7476A_Handle.ADC_Data_B[0]);
                // xil_printf("Control Register: 0x%08lx\r\n", IMR_ADC_7476A_X2_GetCtrlReg(&AXI_IMR_7476A_Handle));
                // xil_printf("Status Register: 0x%08lx\r\n", IMR_ADC_7476A_X2_GetStatusReg(&AXI_IMR_7476A_Handle));
                // xil_printf("Interrupt Register: 0x%08lx\r\n\n", IMR_ADC_7476A_X2_GetIrqReg(&AXI_IMR_7476A_Handle));

                // xil_printf("***Multi ADC conversion Triggers\r\n");
                // IMR_ADC_7476A_X2_MultiConvert(&AXI_IMR_7476A_Handle, ADC_BufferDataA, ADC_BufferDataB, ADC_SAMPLE_SIZE);
                // // Step 3: Wait for IRQ to occur - Data A and B are loaded by the ISR
                // msleep(10);
                // xil_printf("Control Register: 0x%08lx\r\n", IMR_ADC_7476A_X2_GetCtrlReg(&AXI_IMR_7476A_Handle));
                // xil_printf("Status Register: 0x%08lx\r\n", IMR_ADC_7476A_X2_GetStatusReg(&AXI_IMR_7476A_Handle));
                // xil_printf("Interrupt Register: 0x%08lx\r\n", IMR_ADC_7476A_X2_GetIrqReg(&AXI_IMR_7476A_Handle));
                // for (uint8_t Count = 0; Count < AXI_IMR_7476A_Handle.TotalConversions; Count++)
                // {
                //     xil_printf("Interrupt DataA[%d]: %d\r\n", Count, AXI_IMR_7476A_Handle.ADC_Data_A[Count]);
                //     xil_printf("Interrupt DataB[%d]: %d\r\n", Count, AXI_IMR_7476A_Handle.ADC_Data_B[Count]);
                // }
                // xil_printf("\r\n");

                xil_printf("MicroSD FAT FS Testing...");
                // disable_PWM(&AXI_TimerHandle_0);                
                // stopPeriodicTimer(&AXI_TimerHandle_1, XTC_TIMER_0);
                // uint32_t Saved_IRQ_Mask = pauseFastIRQs(&AXI_IRQ_ControllerHandle);
                writeFileTest(ReadWriteFileName);
                readFileTest(ReadWriteFileName);
                readFileTest(ReadOnlyFileName); 
                // resumeFastIRQs(&AXI_IRQ_ControllerHandle, Saved_IRQ_Mask);
                // startPeriodicTimer(&AXI_TimerHandle_1, XTC_TIMER_0);
                // enable_PWM(&AXI_TimerHandle_0);    

                xil_printf("Sleep Delay Testing\r\n");                 
                XGpio_DiscreteSet(&AXI_GPIO_Handle, GPIO_OUTPUT_CHANNEL, TEST_IO_0);
                sleep_ms_Wrapper(1);
                XGpio_DiscreteClear(&AXI_GPIO_Handle, GPIO_OUTPUT_CHANNEL, TEST_IO_0);                
                sleep_ms_Wrapper(5);

                XGpio_DiscreteSet(&AXI_GPIO_Handle, GPIO_OUTPUT_CHANNEL, TEST_IO_0);
                sleep_ms_Wrapper(10);
                XGpio_DiscreteClear(&AXI_GPIO_Handle, GPIO_OUTPUT_CHANNEL, TEST_IO_0);
                sleep_ms_Wrapper(5);

                XGpio_DiscreteSet(&AXI_GPIO_Handle, GPIO_OUTPUT_CHANNEL, TEST_IO_0);
                sleep_10us_Wrapper(1);
                XGpio_DiscreteClear(&AXI_GPIO_Handle, GPIO_OUTPUT_CHANNEL, TEST_IO_0);
                sleep_ms_Wrapper(5);
                
                XGpio_DiscreteSet(&AXI_GPIO_Handle, GPIO_OUTPUT_CHANNEL, TEST_IO_0);
                sleep_10us_Wrapper(10);
                XGpio_DiscreteClear(&AXI_GPIO_Handle, GPIO_OUTPUT_CHANNEL, TEST_IO_0);
            }
            // Update switch state for chage
            PreviousSwitchState = SwitchState;
        }
    }

} // END OF mainTest










void readFileTest(const char *FileName)
{
    FIL   FileHandle;       /* File object */
    FRESULT FS_Status;      /* FatFs return code */
    UINT BytesRead;         /* Bytes read */
    char ReadBuffer[128];   /* Line buffer */

    xil_printf("\r\nOpening file for reading...\r\n");
    FS_Status = f_open(&FileHandle, FileName, FA_READ);
    if (FS_Status != FR_OK)
    {
        xil_printf("Open failed: %d\r\n", FS_Status);
        f_mount(0, "", 0);   /* Unmount */
        return;
    }

    /* Read in chunks and print */
    xil_printf("Reading file contents:\r\n---------------------------------\r\n");
    do
    {
        FS_Status = f_read(&FileHandle, ReadBuffer, sizeof(ReadBuffer) - 1, &BytesRead);
        if (FS_Status != FR_OK)
        {
            xil_printf("Read error: %d\r\n", FS_Status);
            break;
        }
        ReadBuffer[BytesRead] = '\0';   /* Null-terminate for printing */
        xil_printf("%s", ReadBuffer);

    } while (BytesRead == sizeof(ReadBuffer) - 1);

    xil_printf("\r\n---------------------------------\r\n");

    f_close(&FileHandle);
    // f_mount(0, "", 0);   /* Unmount */
    xil_printf("Done Reading.\r\n");
}



void writeFileTest(const char *FileName)
{
    FRESULT FS_Status;
    FIL     FileHandle;
    UINT    BytesWritten;

    // const char *FileName  = "0:/HabTestingWrite.txt";
    const char *WriteData = "Hab Test of writing to a file example\r\n"
                            "Test 1234";

    /* Step 2: Open or create the file for write */
    FS_Status = f_open(&FileHandle, FileName, FA_CREATE_ALWAYS | FA_WRITE);
    if (FS_Status != FR_OK)
    {
        xil_printf("Failed to open/create file: %d\r\n", FS_Status);
        f_mount(NULL, "0:/", 0);
        return;
    }
    xil_printf("\r\nFile opened for writing: %s\r\n", FileName);

    /* Step 3: Write data to the file */
    FS_Status = f_write(&FileHandle, WriteData, strlen(WriteData), &BytesWritten);
    if ((FS_Status != FR_OK) || (BytesWritten == 0))
    {
        xil_printf("Write failed: %d\r\n", FS_Status);
        f_close(&FileHandle);
        // f_mount(NULL, "0:/", 0);
        return;
    }
    xil_printf("Wrote %d bytes to file.\r\n", BytesWritten);

    /* Step 4: Close the file */
    f_close(&FileHandle);
    xil_printf("File closed successfully.\r\n\n");

    /* Step 5: Unmount the file system */
    // f_mount(NULL, "0:/", 0);
    // xil_printf("File system unmounted.\r\n");
}


// END OF Main Test
#endif