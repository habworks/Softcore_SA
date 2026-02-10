/******************************************************************************************************
 * @file            AXI_IRQ_Controller_Support.c
 * @brief           A collection of functions relevant to the AXI IRQ peripherals
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

#include "AXI_IRQ_Controller_Support.h"
#include "xil_io.h"
#include "xintc_l.h"  
#include "Hab_Types.h"



/********************************************************************************************************
* @brief Init of an AXI IRQ Controller IP Block for use.  On success of the init start the conroller.  When
* using the IRQ Controller all peripherals should be init before calling this function.  This is STEP 1 of
* a 4 STEP process in setting up the IRQ Controller and IRQ ISR Callbacks for perhiperal devices.  This 
* function should be called only once.  This function is used for both normal and fast interrupts
*
* @author original: Hab Collector \n
*
* @note: This is Step 1 of 4
* @note: See peripheral AXI IRQ Controller
* @note: Generally speaking there is only 1 AXI IRQ Controller in the design ID is 0
* @note: See BSP xintc.h for peripheral specifics based version of timer in use (supports v3.19)
* 
* @param IRQ_ControllerHandle: Pointer to the IRQ Controller  handle that will be used 
* @param IPB_BaseAddress: The base address of the IRQ Controller IP block
*
* @return True if init OK
*
* STEP 1: Initializes a specific AXI INTC instance
* STEP 2: Starts the interrupt controller operation
********************************************************************************************************/
bool init_IRQ_Controller(XIntc *IRQ_ControllerHandle, UINTPTR IPB_BaseAddress)
{
    int AXI_Status;

    // STEP 1: Initializes a specific AXI INTC instance
    AXI_Status = XIntc_Initialize(IRQ_ControllerHandle, IPB_BaseAddress);
    if (AXI_Status != XST_SUCCESS)
        return(false);
    else
        return(true);

    // STEP 2: Starts the interrupt controller operation
    // AXI_Status = XIntc_Start(IRQ_ControllerHandle, XIN_REAL_MODE);
    // if (AXI_Status != XST_SUCCESS)
    //     return(false);
    // else
    //     return(true);
    
} // END OF init_IRQ_Controller



/********************************************************************************************************
* @brief Connects a peripheral IRQ to the IRQ Controller.  This is step 2 of a 3 step process.  This function
* should be called for each peripheral based IRQ.  This is for NON-low latency (slow / normal) interrupts
*
* @author original: Hab Collector \n
*
* @note: This is Step 2 of 4
* @note: Some peripherals use a generic ISR handler (for example AXI Timer).  These peripherals will include a setHandler function in their API
* (for ecample XTmrCtr_SetHandler).  As part of said peripheral init you must call the setHandler api that associates the actual ISR to be called.
* The generic ISR will call the actual ISR - this is how it works. See function init_PeriodicTimer in AXI_Timer_PWM_Support.c for example
* @note: Generally speaking there is only 1 AXI IRQ Controller in the design ID is 0 
* @note: See peripheral AXI IRQ Controller
* @note: See BSP xintc.h for peripheral specifics based version of timer in use (supports v3.19)
* 
* @param IRQ_ControllerHandle: Pointer to the IRQ Controller  handle that will be used 
* @param ISR_HandlerFabric_ID: The interrupt ID for the peripheral. This ID is defined in xparameters.h. If using concat block and the ID is not called out in parameters.h it is the Inx[0:0] value
* @param ISR_Handler: A function pointer to the custom interrupt handler function for that peripheral - ***SEE NOTES***
* @param ISR_CallbackReference: A reference to data that will be passed to the interrupt handler function - usually the peripheral's handle
*
* @return True if connection successful 
*
* STEP 1: Registers a specific interrupt handler function for a given interrupt source in the AXI INTC
* STEP 2: Enables the specific interrupt source within the AXI INTC
********************************************************************************************************/
bool connectPeripheral_IRQ(XIntc *IRQ_ControllerHandle, uint8_t ISR_HandlerFabric_ID, XInterruptHandler ISR_Handler, void *ISR_CallbackReference)
{
    int AXI_Status;

    // STEP 1: Registers a specific interrupt handler function for a given interrupt source in the AXI INTC
    AXI_Status = XIntc_Connect(IRQ_ControllerHandle, ISR_HandlerFabric_ID, (XInterruptHandler)ISR_Handler, ISR_CallbackReference);
    if (AXI_Status != XST_SUCCESS)
        return false;

    // STEP 2: Enables the specific interrupt source within the AXI INTC
    XIntc_Enable(IRQ_ControllerHandle, ISR_HandlerFabric_ID);
    return(true);

} // END OF connectPeripheral_IRQ



/********************************************************************************************************
* @brief Enable IRQ Exceptions for the MicroBlaze.  Initializes the exception handling system and enables 
* interrupts at the processor level.  This is the last thing to be called in the IRQ Handler process.  It
* should only be called once.  Note the last step (step 4), is not part of this api.  It is unique to the 
* AXI peripheral itself - it is where you enable said AXI peripheral for use IRQ Mode.  This function supports
* both normal and low latency (fast) interrupts
*
* @author original: Hab Collector \n
*
* @note: This is Step 3 of 4
* @note: Read header notes on step 4 - step 4 not part of this api set
* @note: See peripheral AXI IRQ Controller
* @note: Generally speaking there is only 1 AXI IRQ Controller in the design ID is 0
* @note: See BSP xintc.h for peripheral specifics based version of timer in use (supports v3.19)
* @note: REMOVE STEP 2 IF USING FAST INTERRUPT.  Do NOT register XIntc_InterruptHandler here 
*        if you are using Fast Interrupts for the Timer. 
*        The MicroBlaze hardware handles the jump directly to your 
*        FastHandler and your normal handlers via the INTC's own vector table.
* 
* @param IRQ_ControllerHandle: Pointer to the IRQ Controller  handle that will be used 
* @param UseFastInterrupts: If true use fast interrupts - if false use normal interrupts
*
* STEP 1: Initializes the exception handling system
* STEP 2: Register, or not the AXI INTC interrupt handler as a general exception handler - see notes
* STEP 3: Enables exceptions globally in the processor
********************************************************************************************************/
void enableExceptionHandling(XIntc *IRQ_ControllerHandle, bool UseFastInterrupts)
{
    // STEP 1: Initializes the exception handling system
    Xil_ExceptionInit();

    // STEP 2: Register, or not the AXI INTC interrupt handler as a general exception handler - see notes
    if (UseFastInterrupts)
        Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT, (Xil_ExceptionHandler)XIntc_InterruptHandler, IRQ_ControllerHandle);

    // STEP 3: Enables exceptions globally in the processor
    Xil_ExceptionEnable();

} // END OF enableExceptionHandling


/********************************************************************************************************
* @brief Connects and enables a fast interrupt handler for a specific fabric interrupt source
* using the AXI Interrupt Controller.  This function registers the handler as a fast interrupt
* (low-latency path) and enables the corresponding interrupt ID within the INTC.
*
* @author original: Hab Collector \n
*
* @note: This function uses the AXI INTC fast interrupt mechanism
*        (XIntc_ConnectFastHandler).  The ISR is expected to be written to fast-interrupt
*        constraints (minimal latency, explicit acknowledge, no blocking operations).
*        The callback reference parameter is intentionally unused for fast handlers.
*
* @param   IRQ_ControllerHandle     Pointer to initialized AXI Interrupt Controller handle
* @param   ISR_HandlerFabric_ID     Fabric interrupt ID associated with the peripheral
* @param   ISR_Handler              Pointer to fast interrupt service routine
* @param   ISR_CallbackReference    Callback reference (unused for fast interrupt handlers)
*
* @return  true if the handler was successfully connected and enabled
*          false if handler registration failed
*
* STEP 1: Register the fast interrupt handler for the specified fabric interrupt ID.
* STEP 2: Enable the corresponding interrupt source in the AXI INTC.
********************************************************************************************************/
bool connectPeripheralFast_IRQ(XIntc *IRQ_ControllerHandle, uint8_t ISR_HandlerFabric_ID, XInterruptHandler ISR_Handler, void *ISR_CallbackReference)
{
    NOT_USED(ISR_CallbackReference);
    int AXI_Status;

    // STEP 1: Registers a specific interrupt handler function for a given interrupt source in the AXI INTC
    AXI_Status = XIntc_ConnectFastHandler(IRQ_ControllerHandle, ISR_HandlerFabric_ID, (XFastInterruptHandler)ISR_Handler);
    if (AXI_Status != XST_SUCCESS)
        return false;

    // STEP 2: Enables the specific interrupt source within the AXI INTC
    XIntc_Enable(IRQ_ControllerHandle, ISR_HandlerFabric_ID);
    return(true);

} // END OF connectPeripheralFast_IRQ



/********************************************************************************************************
* @brief Temporarily disables all currently-enabled fast interrupt sources at the AXI Interrupt
* Controller and returns the previous interrupt enable mask.  This allows critical sections
* (for example SPI or display transactions) to run without interruption, while preserving
* which interrupts were active beforehand.
*
* @author original: Hab Collector \n
*
* @note: This function operates at the AXI INTC level (IER/IPR/IAR registers) and does not
*        globally disable MicroBlaze exceptions.  Only interrupt sources enabled in the
*        AXI INTC are affected.
*
* STEP 1: Read and save the current Interrupt Enable Register (IER) mask.
* STEP 2: Disable all interrupt sources by clearing the IER.
* STEP 3: Acknowledge any pending interrupts to prevent immediate retrigger on resume.
*
* @param   IRQ_ControllerHandle   Pointer to initialized AXI Interrupt Controller handle
*
* @return  Saved interrupt enable mask representing interrupts that were active prior to disable
********************************************************************************************************/
uint32_t pauseFastIRQs(XIntc *IRQ_ControllerHandle)
{
    uint32_t BaseAddress = IRQ_ControllerHandle->CfgPtr->BaseAddress;

    // STEP 1: Capture currently-enabled interrupts
    uint32_t SavedMask = Xil_In32(BaseAddress + XIN_IER_OFFSET);

    // STEP 2: Disable all interrupt sources at the INTC
    Xil_Out32(BaseAddress + XIN_IER_OFFSET, 0x00000000u);

    // STEP 3: Ack any pending (optional but recommended)
    uint32_t Pending = Xil_In32(BaseAddress + XIN_IPR_OFFSET);
    Xil_Out32(BaseAddress + XIN_IAR_OFFSET, Pending);

    return(SavedMask);

} // END OF pauseFastIRQs



/********************************************************************************************************
* @brief Restores previously-enabled fast interrupt sources at the AXI Interrupt Controller
* using a saved interrupt enable mask.  Intended to be paired with pauseFastIRQs() to
* safely re-enable only those interrupts that were active before a critical section.
*
* @author original: Hab Collector \n
*
* @note: If an interrupt source is level-sensitive and remains asserted while masked,
*        it may fire immediately upon restore.  This behavior is expected and hardware-dependent.
*
* STEP 1: Write the saved interrupt enable mask back to the Interrupt Enable Register (IER).
*
* @param   IRQ_ControllerHandle   Pointer to initialized AXI Interrupt Controller handle
* @param   SavedMask              Interrupt enable mask previously returned by pauseFastIRQs()
********************************************************************************************************/
void resumeFastIRQs(XIntc *IRQ_ControllerHandle, uint32_t SavedMask)
{
    uint32_t BaseAddress = IRQ_ControllerHandle->CfgPtr->BaseAddress;

    // STEP 1: Restore previously-enabled interrupts
    Xil_Out32(BaseAddress + XIN_IER_OFFSET, SavedMask);

} // END OF resumeFastIRQs
