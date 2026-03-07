/******************************************************************************************************
 * @file            Signal_Mode_API.c
 * @brief           A collection of functions relevant to supporting incoming Signals (WAV only) files
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

#include "Signal_SoftCore_SA.h"
#include "Main_App.h"
#include "AXI_IRQ_Controller_Support.h"
#include "AXI_Timer_PWM_Support.h"
#include "Application_Display.h"
#include "FFT_Support.h"
#include "IO_Support.h"


// GLOBAL SIGNAL SPECTRUM
float __attribute__ ((section (".Hab_Fast_Data"))) BinAmplitudes[(FFT_SIZE / 2) + 1];
float CenterFrequency[5] = {5e3, 10e3, 12.5e3, 15e3, 20e3};


bool initSpectrumAnalyzer(Type_Signal_SA *Signal_SA, Type_FFT *FFT, uint32_t SignalSampleRate, uint16_t *ADC_BufferDatum_A, uint32_t *ADC_BufferDatum_B)
{
    // STEP 1: Set member init conditions
    FFT->Size = FFT_SIZE;
    FFT->SampleRate_Hz = SignalSampleRate;
    FFT->FrameReady = false;
    FFT->RBW = (float)SignalSampleRate / FFT_SIZE;

    // STEP 2: Ready Timer 1 - sample rate
    pauseSpecificIRQ(&AXI_IRQ_ControllerHandle, XPAR_FABRIC_AXI_TIMER_1_INTR);
    bool FrequencyStatus = update_PeriodicTimerPeriod(&AXI_SampleTimerHandle, XTC_TIMER_0, (uint32_t)(XPAR_CPU_CORE_CLOCK_FREQ_HZ / SignalSampleRate), false);    

    return(FrequencyStatus);

}


void signalSpectrumAnalyzer(Type_Signal_SA *Signal_SA, Type_FFT *FFT)
{

    if (FFT->FrameReady)
    {
        pauseSpecificIRQ(&AXI_IRQ_ControllerHandle, XPAR_FABRIC_AXI_TIMER_1_INTR);
        FFT_ProcessSignalFrame(FFT, BinAmplitudes, Signal_SA->LowBin, Signal_SA->HighBin);
        displaySignalSpectrum(&Display_SSD1309, BinAmplitudes, Signal_SA->LowBin, Signal_SA->HighBin, 1000.0f);
        resumeSpecificIRQ(&AXI_IRQ_ControllerHandle, XPAR_FABRIC_AXI_TIMER_1_INTR);
    }
}


void signalSelect(Type_SignalSelect SignalSource)
{
    if (SignalSource == SIGNAL_OFF_BOARD_BNC)
        XGpio_DiscreteSet(&AXI_GPIO_Handle, GPIO_OUTPUT_CHANNEL, SIG_SEL);
    else
        XGpio_DiscreteClear(&AXI_GPIO_Handle, GPIO_OUTPUT_CHANNEL, SIG_SEL);
}


void signalPeriodicTimer_ISR(Type_Signal_SA *Signal_SA, Type_FFT *FFT, uint16_t *ADC_BufferDatum_A, uint16_t *ADC_BufferDatum_B)
{
    if (!Signal_SA->Enable)
        return;

    // STEP 1: Start another conversion
    IMR_ADC_7476A_X2_SingleConvert(&AXI_IMR_7476A_Handle, ADC_BufferDatum_A, ADC_BufferDatum_B);

}



inline void signal_ADC_7476A_ISR(Type_Signal_SA *Signal_SA, Type_FFT *FFT, Type_AXI_IMR_7476A_Handle *ADC_Handle)
{
    static uint32_t SampleIndex = 0;


// XGpio_DiscreteSet(&AXI_GPIO_Handle, GPIO_OUTPUT_CHANNEL, TEST_IO_0); 
uint32_t CurrentOutput_GPIO = Xil_In32(XPAR_AXI_GPIO_0_BASEADDR + XGPIO_DATA2_OFFSET);
uint32_t Output_GPIO = (CurrentOutput_GPIO ^ TEST_IO_0);
Xil_Out32(XPAR_AXI_GPIO_0_BASEADDR + XGPIO_DATA2_OFFSET, Output_GPIO);

    IMR_ADC_7476A_X2_ClrIrq(ADC_Handle);
    
    FFT->Samples[SampleIndex] = AXI_IMR_7476A_Handle.ADC_Data_B[0];
    SampleIndex++;
    if (SampleIndex >= FFT->Size)
    {
        FFT->FrameReady = true;
        SampleIndex = 0;
    }

    XIntc_AckIntr(XPAR_AXI_INTC_0_BASEADDR, 1 << ADC_7476A_X2_FABRIC_ID);

// XGpio_DiscreteClear(&AXI_GPIO_Handle, GPIO_OUTPUT_CHANNEL, TEST_IO_0); 
Output_GPIO = (Output_GPIO ^ TEST_IO_0);
Xil_Out32(XPAR_AXI_GPIO_0_BASEADDR + XGPIO_DATA2_OFFSET, Output_GPIO);    

}