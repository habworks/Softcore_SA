/******************************************************************************************************
 * @file            Main_Test.c
 * @brief           Program to demonstrate the use of the various components of the Softcore SS Project
 *
 * Description: 
 * GPIO ACTION:
 * There are a varity of GPIO input and outputs. for various forms of control or status updates.  No GPIOs are used 
 * serve as input interupts in this case
 *
 * PERIODIC TIMER ACTION:
 * There are 1 AXI Timer IP Block within this design.  axi_timer_0 is configured for Timer.
 * axi_timer_0 is configured for periodic timer and both timers are used. 
 * Timer number 0 is set for an ISR IRQ every 300ms.  Timer number 1 is set for an ISR IRQ every 120ms.
 * With a periodic timer, both timer numbers share the same ISR.  There are two switches in use SW0 and SW1.
 *
 * UART LITE ACTION: 
 * The UART is configure in IRQ mode, but even in IRQ mode it can be used in polling via
 * the function xil_printf - xil_printf is tied to this UART via the Platform settings >> Board Support Package >> standalone >> standalone stdin and stdout
 * The two callback functions for the UART in IRQ mode are UART_ReceiveCallback_ISR and UART_TransmitCallback_ISR. 
 * When UART receives input the RX ISR will echo the input to the UART Tx - So if using console what you
 * type you will see. 
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
 * Several PL actions generate interruts.  The Timer, UART, etc. The interrupt controller feeds the various IRQs to the MicroBlaze
 *
 * UI INPUTS
 * SW0 on: Periodic Timer Number 0 is enabled \ disable
 * SW0 off: Periodic Timer Number 0 is enable \ disable
 * PB_0: Board level reset
 * PB_1:
 * PB_2:
 * PB_3:
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
#include "AXI_IMR_PL_Revision.h"
#include "IO_Support.h"

extern void uSD_IntrGlobalDisable(void);

#define XPAR_MICROBLAZE_USE_ICACHE 1
#define XPAR_MICROBLAZE_USE_DCACHE 1
#define XPAR_MICROBLAZE_USE_MSR_INSTR 1
#define XPAR_MICROBLAZE_ICACHE_BYTE_SIZE 65536  
#define XPAR_MICROBLAZE_DCACHE_BYTE_SIZE 32768


// DISPLAY SUPPORT
// #include "AXI_SPI_Display_SSD1309.h"
#include "u8g2.h"
#define DISPLAY_CSN     0x01
XSpi AXI_SPI_DisplayHandle;
u8g2_t U8G2;
Type_Display_SSD1309  Display_SSD1309;
uint32_t __attribute__ ((section (".Hab_Fast_Data"))) DisplayTxError = 0;
uint32_t __attribute__ ((section (".Hab_Fast_Data"))) DisplayTxCount = 0;


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

// TIMER PWM SUPPORT
XTmrCtr __attribute__ ((section (".Hab_Fast_Data"))) AXI_TimerHandle_0;

// TIMER SUPPORT - AUDIO PLAYBACK
#define AUDIO_IRQ_COUNT 1000
XTmrCtr __attribute__ ((section (".Hab_Fast_Data"))) AXI_TimerHandle_1;

// TIMER SUPPORT - GENERIC
XTmrCtr __attribute__ ((section (".Hab_Fast_Data"))) AXI_TimerHandle_2;

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
    uint32_t CurrentOutput_GPIO = Xil_In32(XPAR_AXI_GPIO_0_BASEADDR + XGPIO_DATA2_OFFSET);
    uint32_t Output_GPIO = (CurrentOutput_GPIO ^ TIMER_1_OUTPUT);
    Xil_Out32(XPAR_AXI_GPIO_0_BASEADDR + XGPIO_DATA2_OFFSET, Output_GPIO);

    Output_GPIO = (Output_GPIO ^ TIMER_1_OUTPUT);
    // XGpio_WriteReg(XPAR_AXI_GPIO_0_BASEADDR, XGPIO_DATA2_OFFSET, Output_GPIO);
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
    // uint32_t CurrentOutput_GPIO = XGpio_ReadReg(XPAR_AXI_GPIO_0_BASEADDR, XGPIO_DATA2_OFFSET);
    // uint32_t Output_GPIO = (CurrentOutput_GPIO ^ TIMER_1_OUTPUT);
    // XGpio_WriteReg(XPAR_AXI_GPIO_0_BASEADDR, XGPIO_DATA2_OFFSET, Output_GPIO);

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

Xil_ICacheEnable();
Xil_DCacheEnable();

    // Init AXI UART
    // Status = init_UART_Lite(&AXI_UART_Handle, XPAR_AXI_UARTLITE_0_BASEADDR, INTERRUPT, UART_TransmitCallback_ISR, UART_ReceiveCallback_ISR);
    Status = init_UART_Lite(&AXI_UART_Handle, XPAR_AXI_UARTLITE_0_BASEADDR, POLLING, NULL, NULL);
    if (Status == false)
        while(1);

    // Init AXI GPIO
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

    // Init AXI Timer 1: Audio Playblack
    Status = init_PeriodicTimer(&AXI_TimerHandle_1, XPAR_AXI_TIMER_1_BASEADDR, XTC_TIMER_0, AUDIO_IRQ_COUNT, TimerCallbackAudio_ISR);
    if (Status == false)
        while(1);

    // Init AXI Timer 2: Generic Timer
    Status = init_PeriodicTimer(&AXI_TimerHandle_2, XPAR_AXI_TIMER_2_BASEADDR, XTC_TIMER_0, 400e6, TimerCallbackGeneric_ISR);
    if (Status == false)
        while(1);

    // Init AXI Timer 0: Audio PWM
    Status = init_PWM(&AXI_TimerHandle_0, XPAR_AXI_TIMER_0_BASEADDR);
    if (Status == false)
        while(1);

    // Init AXI SPI: Display Interface
    // Init AXI QSPI for use - will be used with display
    DisplayTxError = 0;
    DisplayTxCount = 0;
    AXI_Status = XSpi_Initialize(&AXI_SPI_DisplayHandle,XPAR_AXI_QUAD_SPI_0_BASEADDR);
    if (AXI_Status != XST_SUCCESS)
        while(1);
    XSpi_Reset(&AXI_SPI_DisplayHandle);
    // Master mode + manual CS
    AXI_Status |= XSpi_SetOptions(&AXI_SPI_DisplayHandle,XSP_MASTER_OPTION | XSP_MANUAL_SSELECT_OPTION);
    // Disable interrupts
    XSpi_IntrGlobalDisable(&AXI_SPI_DisplayHandle);
    // Start the SPI engine
    AXI_Status |= XSpi_Start(&AXI_SPI_DisplayHandle);
    // Is it all good
    XSpi_SetSlaveSelect(&AXI_SPI_DisplayHandle, 0); 
    if (AXI_Status != XST_SUCCESS)
        while(1);

    // Init AXI IRQ Controller (4x Steps)
    // Step 1 of 4 IRQ Controller setup: Init or IRQ Controller
    Status = init_IRQ_Controller(&AXI_IRQ_ControllerHandle, XPAR_AXI_INTC_0_BASEADDR);
    if (Status == false)
        while(1);
    // // Step 2A of 4 IRQ Controller setup: AXI Audio Timer 
    Status = connectPeripheralFast_IRQ(&AXI_IRQ_ControllerHandle, XPAR_FABRIC_AXI_TIMER_1_INTR, TimerCallbackAudio_ISR, &AXI_TimerHandle_1);
    if (Status == false)
        while(1);
    // // Step 2B of 4 IRQ Controller setup: AXI Generic Timer 
    Status = connectPeripheralFast_IRQ(&AXI_IRQ_ControllerHandle, XPAR_FABRIC_AXI_TIMER_2_INTR, TimerCallbackGeneric_ISR, &AXI_TimerHandle_2);
    if (Status == false)
        while(1);
    Status = XIntc_Start(&AXI_IRQ_ControllerHandle, XIN_REAL_MODE);
    if (Status != XST_SUCCESS)
        while(1);
    XIntc_Enable(&AXI_IRQ_ControllerHandle, XPAR_FABRIC_AXI_TIMER_1_INTR);
    XIntc_Enable(&AXI_IRQ_ControllerHandle, XPAR_FABRIC_AXI_TIMER_2_INTR);
    // // Step 3 of 4 IRQ Controller setup: Enable IRQs
    enableExceptionHandling(&AXI_IRQ_ControllerHandle, true); // true = use fast interrupts
    
// XSpi_IntrGlobalDisable(&AXI_SPI_DisplayHandle);
uSD_IntrGlobalDisable();

    // Step 4 of 4 Start the IRQ funtions - not part of the AXI IRQ Controller - unique to the AXI peripheral
    startPeriodicTimer(&AXI_TimerHandle_1, XTC_TIMER_0);
    startPeriodicTimer(&AXI_TimerHandle_2, XTC_TIMER_0);
    setup_PWM(&AXI_TimerHandle_0, 100000, 50.0);




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
            setup_PWM(&AXI_TimerHandle_0, 100000, DutyCyclePercent);
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
                // startPeriodicTimer(&AXI_TimerHandle_1, XTC_TIMER_0);
                // xil_printf("Timer 0 started\r\n");
            }
            else
            {
                // stopPeriodicTimer(&AXI_TimerHandle_1, XTC_TIMER_0);
                // xil_printf("Timer 0 stopped\r\n");
            }
            // SWITCH 2
            if (SwitchState & SW_1)
            {
                // startPeriodicTimer(&AXI_TimerHandle_1, XTC_TIMER_1);
                // xil_printf("Timer 1 started\r\n");
            }
            else
            {
                // stopPeriodicTimer(&AXI_TimerHandle_1, XTC_TIMER_1);
                // xil_printf("Timer 1 stopped\r\n");
            }
            // Push Button 1
            if (SwitchState & PB_1)
            {
                // static uint8_t TestVar = 0x00;
                // uint8_t TxBuffer[2] = {0x00, 0xF0};
                // uint8_t RxBuffer[10]; 
                // TxBuffer[0] = ++TestVar;
                // displayTrasmitReceive(&AXI_SPI_DisplayHandle, 1, TxBuffer, RxBuffer, sizeof(TxBuffer));
                // xil_printf("SPI Test\r\n");
                // XIntc_Disable(&AXI_IRQ_ControllerHandle, XPAR_FABRIC_AXI_TIMER_2_INTR);
                // stopPeriodicTimer(&AXI_TimerHandle_1, XTC_TIMER_0);
                // stopPeriodicTimer(&AXI_TimerHandle_2, XTC_TIMER_0);
                // DisplayTxError = 0;
                // DisplayTxCount = 0;
                // displaySimpleTest(&Display_SSD1309);
                displayDirectTest(&Display_SSD1309);
                // startPeriodicTimer(&AXI_TimerHandle_1, XTC_TIMER_0);
                // startPeriodicTimer(&AXI_TimerHandle_2, XTC_TIMER_0);
            }
            // Push Button 2
            if (SwitchState & PB_2)
            {
                // uint8_t TxByte = 0x00;
                // uint8_t RxByte = 0x00;
                // displayResetOrRun(DISPLAY_RUN);
                // // Commands
                // displayChipSelect(CS_ENABLE);
                // displayCommandOrData(DISPLAY_COMMAND);
                // TxByte = 0xE3; // NOP
                // displayTrasmitReceive(&AXI_SPI_DisplayHandle, DISPLAY_CSN, &TxByte, NULL, 1);
                // TxByte = 0xAE; // Display Off
                // displayTrasmitReceive(&AXI_SPI_DisplayHandle, DISPLAY_CSN, &TxByte, NULL, 1);
                // sleep_ms(1000);
                // TxByte = 0xAF; // Display On
                // displayTrasmitReceive(&AXI_SPI_DisplayHandle, DISPLAY_CSN, &TxByte, NULL, 1);
                // // Data
                // displayCommandOrData(DISPLAY_DATA);
                // for(uint8_t Count = 0; Count < 20; Count++)
                // {
                //     TxByte = 0xFF; // 8 pixels on
                //     displayTrasmitReceive(&AXI_SPI_DisplayHandle, DISPLAY_CSN, &TxByte, NULL, 1);
                //     sleep_ms(100);
                //     TxByte = 0x00; // 8 pixels on
                //     displayTrasmitReceive(&AXI_SPI_DisplayHandle, DISPLAY_CSN, &TxByte, NULL, 1);
                // }
                // displayChipSelect(CS_DISABLE);
                stopPeriodicTimer(&AXI_TimerHandle_1, XTC_TIMER_0);
                drawSpectrumMock(&Display_SSD1309);
                xil_printf("End display test\r\n");
                startPeriodicTimer(&AXI_TimerHandle_1, XTC_TIMER_0);
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
                // stopPeriodicTimer(&AXI_TimerHandle_1, XTC_TIMER_0);
                // stopPeriodicTimer(&AXI_TimerHandle_2, XTC_TIMER_0);
                uint32_t Saved_IRQ_Mask = pauseFastIRQs(&AXI_IRQ_ControllerHandle);
                writeFileTest(ReadWriteFileName);
                // sleep_ms(&AXI_TimerHandle_2, XTC_TIMER_0, 1000);
                readFileTest(ReadWriteFileName);
                // sleep_ms(&AXI_TimerHandle_2, XTC_TIMER_0, 1000);
                readFileTest(ReadOnlyFileName); 
                resumeFastIRQs(&AXI_IRQ_ControllerHandle, Saved_IRQ_Mask);
                // startPeriodicTimer(&AXI_TimerHandle_1, XTC_TIMER_0);
                // startPeriodicTimer(&AXI_TimerHandle_2, XTC_TIMER_0);

                // XGpio_DiscreteSet(&AXI_GPIO_Handle, GPIO_OUTPUT_CHANNEL, TEST_IO_0);
                // sleep_ms_Wrapper(1);
                // XGpio_DiscreteClear(&AXI_GPIO_Handle, GPIO_OUTPUT_CHANNEL, TEST_IO_0);                
                // sleep_ms_Wrapper(5);

                // XGpio_DiscreteSet(&AXI_GPIO_Handle, GPIO_OUTPUT_CHANNEL, TEST_IO_0);
                // sleep_ms_Wrapper(10);
                // XGpio_DiscreteClear(&AXI_GPIO_Handle, GPIO_OUTPUT_CHANNEL, TEST_IO_0);
                // sleep_ms_Wrapper(5);

                // XGpio_DiscreteSet(&AXI_GPIO_Handle, GPIO_OUTPUT_CHANNEL, TEST_IO_0);
                // sleep_10us_Wrapper(1);
                // XGpio_DiscreteClear(&AXI_GPIO_Handle, GPIO_OUTPUT_CHANNEL, TEST_IO_0);
                // sleep_ms_Wrapper(5);
                
                // XGpio_DiscreteSet(&AXI_GPIO_Handle, GPIO_OUTPUT_CHANNEL, TEST_IO_0);
                // sleep_10us_Wrapper(10);
                // XGpio_DiscreteClear(&AXI_GPIO_Handle, GPIO_OUTPUT_CHANNEL, TEST_IO_0);



            }
            // Update switch state for chage
            PreviousSwitchState = SwitchState;
        }
    }

    // Unreachable code
    return(0);
}










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