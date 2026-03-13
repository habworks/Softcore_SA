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


/********************************************************************************************************
* @brief Ready the Signal Mode for FFT analysis 
*
* @author original: Hab Collector \n
* 
* @param Signal_SA: Pointer to Signal Spectrum Analyzer structure
* @param FFT: Pointer to the FFT structure
* @param SignalSampleRate: Sample rate of the ADC - sample rate of the signal
*
* @return True if OK
*
* STEP 1: Set member init conditions
* STEP 2: Calculate bins min and max from the given span 
* STEP 3: Init FFT
* STEP 4: Ready Timer 1 - sample rate
********************************************************************************************************/
bool initSignal_SA(Type_Signal_SA *Signal_SA, Type_FFT *FFT, uint32_t SignalSampleRate)
{
    // STEP 1: Set member init conditions
    // FFT
    FFT->Size = FFT_SIZE;
    FFT->SampleRate_Hz = SignalSampleRate;
    FFT->FrameReady = false;
    FFT->RBW = (float)SignalSampleRate / FFT_SIZE;
    // Signal SA
    Signal_SA->StartFrequency = 0;
    Signal_SA->CenterFrequency = (float)(SignalSampleRate / 4.0);
    Signal_SA->StopFrequency = (float)(SignalSampleRate / 2.0);
    
    // STEP 2: Calculate bins min and max from the given span 
    calculateBinMinMaxFromSpan(Signal_SA->StartFrequency, Signal_SA->StopFrequency, (FFT_SIZE / 2), FFT->RBW, &Signal_SA->LowBin, &Signal_SA->HighBin);

    // STEP 3: Init FFT
    deinit_FFT(FFT);
    bool FFT_InitStatus = init_FFT(FFT, DEFAULT_SIGNAL_SAMPLE_RATE_HZ);

    // STEP 4: Ready Timer 1 - sample rate
    pauseSpecificIRQ(&AXI_IRQ_ControllerHandle, XPAR_FABRIC_AXI_TIMER_1_INTR);
    bool FrequencyStatus = update_PeriodicTimerPeriod(&AXI_SampleTimerHandle, XTC_TIMER_0, (uint32_t)(XPAR_CPU_CORE_CLOCK_FREQ_HZ / SignalSampleRate), false);    

    return(FrequencyStatus && FFT_InitStatus);

} // END OF initSignal_SA



/********************************************************************************************************
* @brief This is the core working function when in Signal SA mode.  It is call repeadly by main and active
* after FFT_SIZE number of samples have been set - via the associated IRQs
*
* @author original: Hab Collector \n
*
* @note: See ISR functions: signalPeriodicTimer_ISR, and signal_ADC_7476A_ISR
* @note: function FFT_ProcessSignalFrame sets FrameReady false
*
* @param Signal_SA: Pointer to Signal Spectrum Analyzer structure
* @param FFT: Pointer to the FFT structure
*
* STEP 1: Pause ADC acquistion while this frame is processed
* STEP 2: Process the current FFT Sample Window
* STEP 3: Display the results
* STEP 4: Resume acquistion
********************************************************************************************************/
void signalSpectrumAnalyzer(Type_Signal_SA *Signal_SA, Type_FFT *FFT)
{

    if (FFT->FrameReady)
    {
        // STEP 1: Pause ADC acquistion while this frame is processed
        pauseSpecificIRQ(&AXI_IRQ_ControllerHandle, XPAR_FABRIC_AXI_TIMER_1_INTR);

        // STEP 2: Process the current FFT Sample Window
        FFT_ProcessSignalFrame(FFT, BinAmplitudes, Signal_SA->LowBin, Signal_SA->HighBin);

        // STEP 3: Display the results
        displaySignalSpectrum(&Display_SSD1309, BinAmplitudes, Signal_SA->LowBin, Signal_SA->HighBin, -60.0f, 0.0f);
        
        // STEP 4: Resume acquistion
        resumeSpecificIRQ(&AXI_IRQ_ControllerHandle, XPAR_FABRIC_AXI_TIMER_1_INTR);
    }

} // END OF signalSpectrumAnalyzer



/********************************************************************************************************
* @brief Sets the signal input relay to the desire source - on board PCB local osscilator (default on POR)
* or the external BNC connection.
*
* @author original: Hab Collector \n
*
* @param SignalSource: Desired signal source
********************************************************************************************************/
void signalSelect(Type_SignalSelect SignalSource)
{
    if (SignalSource == SIGNAL_OFF_BOARD_BNC)
        XGpio_DiscreteSet(&AXI_GPIO_Handle, GPIO_OUTPUT_CHANNEL, SIG_SEL);
    else
        XGpio_DiscreteClear(&AXI_GPIO_Handle, GPIO_OUTPUT_CHANNEL, SIG_SEL);
}



/********************************************************************************************************
* @brief This function is called by the Timer 1 ISR when in Signal SA Mode - its sole job is to init a single
* conversion.  As the timer is set in initSignal_SA() to the desired sample rate - the ADC conversions are
* processed at said rate - the sampling rate.
*
* @author original: Hab Collector \n
*
* @note: See ISR functions: signalPeriodicTimer_ISR, and signal_ADC_7476A_ISR
* @note: function FFT_ProcessSignalFrame sets FrameReady false
* @note: Unlike the Audio SA this does not require streaming - as such the Timer 1 is suspended during FFT processing
*
* @param Signal_SA: Pointer to Signal Spectrum Analyzer structure
* @param FFT: Pointer to the FFT structure
* @param ADC_BufferDatum_A: Data buffer for ADC A (ADC CH 1)
* @param ADC_BufferDatum_B: Data buffer for ADC B (ADC CH 2)
*
* STEP 1: Start another conversion
********************************************************************************************************/
void signalPeriodicTimer_ISR(Type_Signal_SA *Signal_SA, Type_FFT *FFT, uint16_t *ADC_BufferDatum_A, uint16_t *ADC_BufferDatum_B)
{
    if (!Signal_SA->Enable)
        return;

    // STEP 1: Start another conversion
    IMR_ADC_7476A_X2_SingleConvert(&AXI_IMR_7476A_Handle, ADC_BufferDatum_A, ADC_BufferDatum_B);

} // END OF signalPeriodicTimer_ISR



/********************************************************************************************************
* @brief This function is the ISR for custom ADC IP that interfaces the dual 7476A x2 IRQ.  It signals the
* conversion is completed (single or multi-step).  This applicaiton only uses single conversions.  As the 
* ADC conversion is complete store to the FFT sample buffer and increment the sample count.  When the FFT 
* frame is ready (sample count = FFT_SIZE) then process the FFT by setting the Frame ready.  
*
* @author original: Hab Collector \n
*
* @note: See ISR functions: signalPeriodicTimer_ISR, and signal_ADC_7476A_ISR
* @note: This function must complete before the signalPeriodicTimer_ISR runs again
* @note: Unlike the Audio SA this does not require streaming - as such the Timer 1 is suspended during FFT processing
*
* @param Signal_SA: Pointer to Signal Spectrum Analyzer structure
* @param FFT: Pointer to the FFT structure
* @param ADC_Handle: Pointer to the custom ADC IP handle
*
* STEP 1: Clear the IRQ
* STEP 2: Capture the signal sample and update the Frame ready status
* STEP 3: ACK the IRQ
********************************************************************************************************/
inline void signal_ADC_7476A_ISR(Type_Signal_SA *Signal_SA, Type_FFT *FFT, Type_AXI_IMR_7476A_Handle *ADC_Handle)
{
    static uint32_t SampleIndex = 0;

// USED ONLY FOR TESTING - CAN BE REMOVED - SETS GPIO TEST POINT
// XGpio_DiscreteSet(&AXI_GPIO_Handle, GPIO_OUTPUT_CHANNEL, TEST_IO_0); 
uint32_t CurrentOutput_GPIO = Xil_In32(XPAR_AXI_GPIO_0_BASEADDR + XGPIO_DATA2_OFFSET);
uint32_t Output_GPIO = (CurrentOutput_GPIO ^ TEST_IO_0);
Xil_Out32(XPAR_AXI_GPIO_0_BASEADDR + XGPIO_DATA2_OFFSET, Output_GPIO);

    // STEP 1: Clear the IRQ
    IMR_ADC_7476A_X2_ClrIrq(ADC_Handle);
    
    // STEP 2: Capture the signal sample and update the Frame ready status
    FFT->Samples[SampleIndex] = AXI_IMR_7476A_Handle.ADC_Data_A[0];
    SampleIndex++;
    if (SampleIndex >= FFT->Size)
    {
        FFT->FrameReady = true;
        SampleIndex = 0;
    }

    // STEP 3: ACK the IRQ
    XIntc_AckIntr(XPAR_AXI_INTC_0_BASEADDR, 1 << ADC_7476A_X2_FABRIC_ID);

// USED ONLY FOR TESTING - CAN BE REMOVED - CLEARS GPIO TEST POINT
// XGpio_DiscreteClear(&AXI_GPIO_Handle, GPIO_OUTPUT_CHANNEL, TEST_IO_0); 
Output_GPIO = (Output_GPIO ^ TEST_IO_0);
Xil_Out32(XPAR_AXI_GPIO_0_BASEADDR + XGPIO_DATA2_OFFSET, Output_GPIO);    

} // END OF signal_ADC_7476A_ISR