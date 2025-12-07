/******************************************************************************************************
 * @file            AXI_Timer_PWM_Support.c
 * @brief           A collection of functions relevant to the AXI Timere peripherals
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

#include "AXI_Timer_PWM_Support.h"


/********************************************************************************************************
* @brief Init of an AXI Timer IP Block for use as PWM.  When using an AXI IP Block as PWM, both Timer 0 and 
* Timer 1 must be enabled and the output is taken from pwm0 not generateout0.  Additionally the "Active state 
* of Generate Out Signal" must be set to Active High.
*
* @author original: Hab Collector \n
*
* @note: See peripheral AXI Timer
* @note: This is the setup PWM for axi_timer_#
* @note: See BSP xtmrcntr.h for peripheral specifics based version of timer in use (supports v4.11)
* 
* @param TimerHandle: Pointer to the timer handle that will be used 
* @param IPB_BaseAddress: Base address of the AXI Timer block - see xparameters.h (XPAR_AXI_TIMER_#_BASEADDR)
*
* STEP 1: Load the config structure
* STEP 2: Call the timer init function for use with PWM
********************************************************************************************************/
bool init_PWM(XTmrCtr *TimerHandle, UINTPTR IPB_BaseAddress)
{
    // STEP 1: Load the config structure
    XTmrCtr_Config *TimerConfig;
    TimerConfig = XTmrCtr_LookupConfig(IPB_BaseAddress);
    if (TimerConfig == NULL)
        return(false);

    // STEP 2: Call the timer init function for use with PWM
    XTmrCtr_CfgInitialize(TimerHandle, TimerConfig, TimerConfig->BaseAddress);
    return(true);

} // END OF init_PWM



/********************************************************************************************************
* @brief Setup and configure and run of a PWM.  If the PWM is already running it will be disabled and reconfigured.
* When using an AXI IP Block as PWM, both Timer 0 and Timer 1 must be enabled and the output is taken from
* pwm0 not generateout0.  Additionally the "Active state of Generate Out Signal" must be set to Active High.
*
* @author original: Hab Collector \n
*
* @note: See peripheral AXI Timer
* @note: This is the setup PWM for axi_timer_#
* @note: See BSP xtmrcntr.h for peripheral specifics based version of timer in use (supports v4.11)
* @note: PWM must already be init - see function init_PWM
* @note: While in PWM mode output is taken from IPB pwm0, generateout0 and generateout1 contain artifact switching of the PWM process and should not be usedd
* @note: It takes the entire AXI_Timer IPB to create a PWM - it can be used for nothing else
* 
* @param TimerHandle: Pointer to the timer handle that will be used 
* @param PWM_Frequency: PWM Frequency in Hz
* @param DutyCyclePercent: Duty Cycle as a percent 
*
* @return True if setup OK
*
* STEP 1: Check for errors in dutycycle and Calculate PWM period and High time in nanoseconds
* STEP 2: Disable PWM in case it is running
* STEP 3: Configure the PWM based on nanoseconds of period and high tiime
* STEP 4: Start PWM
********************************************************************************************************/
bool setup_PWM(XTmrCtr *TimerHandle, uint32_t PWM_Frequency, float DutyCyclePercent)
{
    // STEP 1: Check for errors in dutycycle and Calculate PWM period and High time in nanoseconds
    if (DutyCyclePercent > 100)
        return(false);
    double PWM_T_Seconds = 1.0 / (double)PWM_Frequency;
    uint32_t PWM_T_NanoSeconds = (uint32_t)(PWM_T_Seconds / 1.0E-9);
    uint32_t PWM_HighTimeNanoSeconds = (uint32_t)(((double)PWM_T_NanoSeconds * DutyCyclePercent) / 100.0);

    // STEP 2: Disable PWM in case it is running
    disable_PWM(TimerHandle);

    // STEP 3: Configure the PWM based on nanoseconds of period and high tiime
    XTmrCtr_PwmConfigure(TimerHandle, PWM_T_NanoSeconds, PWM_HighTimeNanoSeconds);

    // STEP 4: Start PWM
    enable_PWM(TimerHandle);

    return(true);
    
} // END OF setup_PWM


/********************************************************************************************************
* @brief Starts the PWM.  PWM must already be init and configured (setup).  This function is meant to be 
* used if PWM has been disabled and previously setup and needs to be restarted again
*
* @author original: Hab Collector \n
*
* @note: See function header notes for init_PWM and setup_PWM
* 
* @param TimerHandle: Pointer to the timer handle that will be used 
*
* STEP 1: Start PWM
********************************************************************************************************/
void enable_PWM(XTmrCtr *TimerHandle)
{
    // STEP 1: Start PWM
    XTmrCtr_PwmEnable(TimerHandle);

} // END OF enable_PWM



/********************************************************************************************************
* @brief Stops the PWM.  PWM must already be init and configured (setup).  This functionc can be called before
* the PWM has been setup (but does nothing)
*
* @author original: Hab Collector \n
*
* @note: See function header notes for init_PWM and setup_PWM
* 
* @param TimerHandle: Pointer to the timer handle that will be used 
*
* STEP 1: Stop PWM
********************************************************************************************************/
void disable_PWM(XTmrCtr *TimerHandle)
{
    // STEP 1: Stop PWM
    XTmrCtr_PwmDisable(TimerHandle);
    
} // END OF disable_PWM



/********************************************************************************************************
* @brief Init of an AXI Timer IP Block for use as a periodic timer.  When using an AXI IP Block as a periodic
* timer both Timer 0 and Timer 1 are available for use.  However it should be noted they call the same ISR.  
* The ISR must determine (base on Timer 0 or 1) the actual cause for IRQ.  Note the timer counts at the rate 
* of AXI IP Block applied clock (fclk) and it counts down.  So time to IRQ = (1/fclk) * TimerIntervalTicks
*
* @author original: Hab Collector \n
*
* @note: See peripheral AXI Timer
* @note: See BSP xtmrcntr.h for peripheral specifics based version of timer in use (supports v4.11)
* @note: There is only a single ISR handler for both Timers of an AXI Timer IP Block
* @note: This function only inits the periodic timer - to start it call startPeriodicTimer
* 
* @param TimerHandle: Pointer to the timer handle that will be used 
* @param IPB_BaseAddress: Base address of the AXI Timer block - see xparameters.h (XPAR_AXI_TIMER_#_BASEADDR)
* @param TimerNumber: Timer number in use XTC_TIMER_0 (0) or XTC_TIMER_1 (1) (only)
* @param TimerIntervalTicks: Timer Ticks to countdown
*
* STEP 1: Simple parameter check
* STEP 2: Load the config structure and configure the timer
* STEP 3: Set timer reset value, options to allow periodic timer with ISR, and clear any pending IRQs
* STEP 4: Set the interrupt handler for Timer
********************************************************************************************************/
bool init_PeriodicTimer(XTmrCtr *TimerHandle, UINTPTR IPB_BaseAddress, u8 TimerNumber, u32 TimerIntervalTicks, Type_TimerFunction_ISR TimerFunction_ISR)
{
    // STEP 1: Simple parameter check
    if ((TimerNumber != XTC_TIMER_0) && (TimerNumber != XTC_TIMER_1))
        return(false);
    if (TimerIntervalTicks == 0)
        return(false);

    // STEP 2: Load the config structure
    #ifdef USE_SIMPLE_PWM_TIMER_CONFIG
    XTmrCtr_Initialize(TimerHandle, IPB_BaseAddress);
    #else
    XTmrCtr_Config *TimerConfig;
    TimerConfig = XTmrCtr_LookupConfig(IPB_BaseAddress);
    if (TimerConfig == NULL)
        return(false);
    XTmrCtr_CfgInitialize(TimerHandle, TimerConfig, TimerConfig->BaseAddress);
    #endif

    // STEP 3: Set timer reset value, options to allow periodic timer with ISR, and clear any pending IRQs
    XTmrCtr_SetResetValue(TimerHandle, TimerNumber, TimerIntervalTicks);
    XTmrCtr_SetOptions(TimerHandle, TimerNumber, XTC_INT_MODE_OPTION | XTC_AUTO_RELOAD_OPTION | XTC_DOWN_COUNT_OPTION); 
    XTmrCtr_ClearStats(TimerHandle);    

    // STEP 4: Set the interrupt handler for Timer
    #ifdef USE_AXI_TIMER_IRQ_CALLBACK_API 
    XTmrCtr_SetHandler(TimerHandle, TimerFunction_ISR, TimerHandle);
    #endif

    return(true);

} // END OF init_PeriodicTimer



/********************************************************************************************************
* @brief Starts the periodic timer.  Note timer must be init before it can be started.  See function init_PeriodicTimer
*
* @author original: Hab Collector \n
*
* @note: See BSP xtmrcntr.h for peripheral specifics based version of timer in use (supports v4.11)
* 
* @param TimerHandle: Pointer to the timer handle that will be used 
* @param TimerNumber: Timer number in use XTC_TIMER_0 (0) or XTC_TIMER_1 (1) (only)
*
* STEP 1: Simple parameter check
* STEP 2: Verify timer has been configured
* STEP 3: Start the requested timer
********************************************************************************************************/
bool startPeriodicTimer(XTmrCtr *TimerHandle, u8 TimerNumber)
{
     // STEP 1: Simple parameter check
    if ((TimerNumber != XTC_TIMER_0) && (TimerNumber != XTC_TIMER_1))
        return(false);

    // STEP 2: Verify timer has been configured
    if (!TimerHandle->IsReady)
        return(false);

    // STEP 3: Start the requested timer
    if (TimerNumber == XTC_TIMER_0)
    {
        if (!TimerHandle->IsStartedTmrCtr0)
            XTmrCtr_Start(TimerHandle, XTC_TIMER_0);
    }
    else
    {
        if (!TimerHandle->IsStartedTmrCtr1)
            XTmrCtr_Start(TimerHandle, XTC_TIMER_1);
    }
    return(true);

} // END OF startPeriodicTimer



/********************************************************************************************************
* @brief Stops the periodic timer.  Note timer must be init before it can be stopped.  See function init_PeriodicTimer
*
* @author original: Hab Collector \n
*
* @note: See BSP xtmrcntr.h for peripheral specifics based version of timer in use (supports v4.11)
* 
* @param TimerHandle: Pointer to the timer handle that will be used 
* @param TimerNumber: Timer number in use XTC_TIMER_0 (0) or XTC_TIMER_1 (1) (only)
*
* STEP 1: Simple parameter check
* STEP 2: Verify timer has been configured
* STEP 3: Stop the requested timer
********************************************************************************************************/
bool stopPeriodicTimer(XTmrCtr *TimerHandle, u8 TimerNumber)
{
     // STEP 1: Simple parameter check
    if ((TimerNumber != XTC_TIMER_0) && (TimerNumber != XTC_TIMER_1))
        return(false);

    // STEP 2: Verify timer has been configured
    if (!TimerHandle->IsReady)
        return(false);

    // STEP 3: Stop the requested timer
    if (TimerNumber == XTC_TIMER_0)
    {
        if (TimerHandle->IsStartedTmrCtr0)
            XTmrCtr_Stop(TimerHandle, XTC_TIMER_0);
    }
    else
    {
        if (TimerHandle->IsStartedTmrCtr1)
            XTmrCtr_Stop(TimerHandle, XTC_TIMER_1);
    }
    return(true);

} // END OF stopPeriodicTimer