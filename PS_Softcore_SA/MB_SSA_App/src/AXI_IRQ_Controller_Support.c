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
* a 6 STEP process in setting up the IRQ Controller and IRQ ISR Callbacks for perhiperal devices.  This 
* function should be called only once.  This function is used for both normal and fast interrupts
*
* @author original: Hab Collector \n
*
* @note: This is Step 1 of 6
* @note: See peripheral AXI IRQ Controller
* @note: Generally speaking there is only 1 AXI IRQ Controller in the design ID is 0
* @note: See BSP xintc.h for peripheral specifics based version of timer in use (supports v3.19)
* 
* @param IRQ_ControllerHandle: Pointer to the IRQ Controller handle that will be used 
* @param IPB_BaseAddress: The base address of the IRQ Controller IP block
*
* @return True if init OK
*
* STEP 1: Initializes a specific AXI INTC instance
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
    
} // END OF init_IRQ_Controller



/********************************************************************************************************
* @brief Connects a peripheral IRQ to the IRQ Controller.  This is step 2 of a 6 step process.  This function
* should be called for each peripheral based IRQ.  ***This is for NON-low latency (slow / normal) interrupts.
* For fast interrupts use instead connectPeripheralFast_IRQ().
*
* @author original: Hab Collector \n
*
* @note: This is Step 2 of 6
* @note: Some peripherals use a generic ISR handler (for example AXI Timer).  These peripherals will include a setHandler function in their API
* (for ecample XTmrCtr_SetHandler).  As part of said peripheral init you must call the setHandler api that associates the actual ISR to be called.
* The generic ISR will call the actual ISR - this is how it works. See function init_PeriodicTimer in AXI_Timer_PWM_Support.c for example
* @note: Generally speaking there is only 1 AXI IRQ Controller in the design ID is 0 
* @note: See peripheral AXI IRQ Controller
* @note: See BSP xintc.h for peripheral specifics based version of timer in use (supports v3.19)
* 
* @param IRQ_ControllerHandle: Pointer to the IRQ Controller handle that will be used 
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
* @brief Connects and enables a fast interrupt handler for a specific fabric interrupt source
* using the AXI Interrupt Controller.  This function registers the handler as a fast interrupt
* (low-latency path) and enables the corresponding interrupt ID within the INTC. Only use this function for 
* fast interrupts.  For normal interrupts use instead connectPeripheral_IRQ().
*
* @author original: Hab Collector \n
*
* @note: This is Step 2 of 6
* @note: This function uses the AXI INTC fast interrupt mechanism
*        (XIntc_ConnectFastHandler).  The ISR is expected to be written to fast-interrupt
*        constraints (minimal latency, explicit acknowledge, no blocking operations).
*        The callback reference parameter is intentionally unused for fast handlers.
* @note Callback reference for fast interrupts excepts only void and must be prototyed as such:
* void my_ISR_Function(void) __attribute__((fast_interrupt));
* void my_ISR_Funtion(void)
*
* @param   IRQ_ControllerHandle     Pointer to initialized AXI Interrupt Controller handle
* @param   ISR_HandlerFabric_ID     Fabric interrupt ID associated with the peripheral
* @param   ISR_Handler              Pointer to fast interrupt service routine
* @param   ISR_CallbackReference    Callback reference (unused for fast interrupt handlers)
*
* @return  True if the handler was successfully connected and enabled
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
* @brief Start the interrupt controller.  This is step 3 of 6.  It is noted here that step 5, you will not
* not find here. Used with both normal and fast interrupts.
* 
* @author original: Hab Collector \n
*
* @note: This is Step 3 of 6
* @note: See peripheral AXI IRQ Controller
* @note: Generally speaking there is only 1 AXI IRQ Controller in the design ID is 0
* @note: See BSP xintc.h for peripheral specifics based version of timer in use (supports v3.19)
* 
* @param IRQ_ControllerHandle: Pointer to the IRQ Controller  handle that will be used 
* @param InterruptMode: Mode of operation for the interrupt controller can only be XIN_REAL_MODE or XIN_SIMULATION_MODE - use normally XIN_REAL_MODE
*
* STEP 1: Start the interrupt controller
********************************************************************************************************/
bool start_IRQ_Controller(XIntc *IRQ_ControllerHandle, uint8_t InterruptMode)
{
    // STEP 1: Start the interrupt controller
    int AXI_Status = XIntc_Start(IRQ_ControllerHandle, InterruptMode);
    return(AXI_Status == XST_SUCCESS);

} // END OF start_IRQ_Controller



/********************************************************************************************************
* @brief Enable a specific interrupt assoicated with the interrupt controller.  Used with both normal and
* fast interrupts.  
* 
* @author original: Hab Collector \n
*
* @note: This is Step 4 of 6
* @note: See peripheral AXI IRQ Controller
* @note: Generally speaking there is only 1 AXI IRQ Controller in the design ID is 0
* @note: See BSP xintc.h for peripheral specifics based version of timer in use (supports v3.19)
* 
* @param IRQ_ControllerHandle: Pointer to the IRQ Controller  handle that will be used 
* @param enableDevice_IRQ_Controller: Peripheral device ID interrupt value
*
* STEP 1: Enable specific interrupt
********************************************************************************************************/
void enableDevice_IRQ_Controller(XIntc *IRQ_ControllerHandle, uint8_t DeviceInterrupt_ID)
{
    // STEP 1: Enable specific interrupt
    XIntc_Enable(IRQ_ControllerHandle, DeviceInterrupt_ID);

} // END OF enableDevice_IRQ_Controller



/********************************************************************************************************
* @brief Enable IRQ Exceptions for the MicroBlaze.  Initializes the exception handling system and enables 
* interrupts at the processor level. It should only be called once. This function supports both normal and 
* low latency (fast) interrupts.  Note step 6 is not found here.  In step 6 you start / enable the connected 
* interrupt peripherals.  
* 
* @author original: Hab Collector \n
*
* @note: This is Step 5 of 6
* @note: Step 6 of 6 is actually starting the interrupt periperials - unique to the specific interrupt
* @note: See peripheral AXI IRQ Controller
* @note: Generally speaking there is only 1 AXI IRQ Controller in the design ID is 0
* @note: See BSP xintc.h for peripheral specifics based version of timer in use (supports v3.19)
* 
* @param IRQ_ControllerHandle: Pointer to the IRQ Controller  handle that will be used 
*
* STEP 1: Initializes the exception handling system
* STEP 2: Register the AXI INTC interrupt handler as a general exception handler 
* STEP 3: Enables exceptions globally in the processor
********************************************************************************************************/
void enableExceptionHandling(XIntc *IRQ_ControllerHandle)
{
    // STEP 1: Initializes the exception handling system
    Xil_ExceptionInit();

    // STEP 2: Register the AXI INTC interrupt handler as a general exception handler 
    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT, (Xil_ExceptionHandler)XIntc_InterruptHandler, IRQ_ControllerHandle);

    // STEP 3: Enables exceptions globally in the processor
    Xil_ExceptionEnable();

} // END OF enableExceptionHandling



/********************************************************************************************************
* @brief Temporarily disables all currently-enabled fast interrupt sources at the AXI Interrupt
* Controller and returns the previous interrupt enable mask.  This allows critical sections
* (for example SPI or display transactions) to run without interruption, while preserving
* which interrupts were active beforehand.  Tested with fast, but shouild also work with normal interrupts. 
*
* @author original: Hab Collector \n
*
* @note: This function operates at the AXI INTC level (IER/IPR/IAR registers) and does not
*        globally disable MicroBlaze exceptions.  Only interrupt sources enabled in the
*        AXI INTC are affected.
*
* @param   IRQ_ControllerHandle   Pointer to initialized AXI Interrupt Controller handle
*
* @return  Saved interrupt enable mask representing interrupts that were active prior to disable
*
* STEP 1: Read and save the current Interrupt Enable Register (IER) mask.
* STEP 2: Disable all interrupt sources by clearing the IER.
* STEP 3: Acknowledge any pending interrupts to prevent immediate retrigger on resume.
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
* safely re-enable only those interrupts that were active before a critical section.  Tested with fast, but
* shouild also work with normal interrupts. 
*
* @author original: Hab Collector \n
*
* @note: If an interrupt source is level-sensitive and remains asserted while masked,
*        it may fire immediately upon restore.  This behavior is expected and hardware-dependent.
*
* @param   IRQ_ControllerHandle   Pointer to initialized AXI Interrupt Controller handle
* @param   SavedMask              Interrupt enable mask previously returned by pauseFastIRQs()
*
* STEP 1: Write the saved interrupt enable mask back to the Interrupt Enable Register (IER).
********************************************************************************************************/
void resumeFastIRQs(XIntc *IRQ_ControllerHandle, uint32_t SavedMask)
{
    uint32_t BaseAddress = IRQ_ControllerHandle->CfgPtr->BaseAddress;

    // STEP 1: Restore previously-enabled interrupts
    Xil_Out32(BaseAddress + XIN_IER_OFFSET, SavedMask);

} // END OF resumeFastIRQs



/********************************************************************************************************
* @brief Temporarily disables an individual interrupt source at the AXI Interrupt Controller.  Tested with 
* fast, but shouild also work with normal interrupts. 
*
* @author original: Hab Collector \n
*
* @note: This function operates at the AXI INTC level (IER/IPR/IAR registers) and does not
*        globally disable MicroBlaze exceptions.  Only interrupt sources enabled in the
*        AXI INTC are affected.
*
* @param IRQ_ControllerHandle: Pointer to initialized AXI Interrupt Controller handle
* @param InterruptId: Fabric ID of the interrupt to pause
*
* STEP 1: Disable only the specific bit
* STEP 2: Optional: Clear pending status for just this line
********************************************************************************************************/
void pauseSpecificIRQ(XIntc *IRQ_ControllerHandle, uint8_t InterruptId)
{
    uint32_t BaseAddress = IRQ_ControllerHandle->CfgPtr->BaseAddress;
    uint32_t Mask = (1 << InterruptId);

    // STEP 1: Disable only the specific bit
    uint32_t CurrentIER = Xil_In32(BaseAddress + XIN_IER_OFFSET);
    Xil_Out32(BaseAddress + XIN_IER_OFFSET, CurrentIER & ~Mask);
    
    // STEP 2: Optional: Clear pending status for just this line
    Xil_Out32(BaseAddress + XIN_IAR_OFFSET, Mask);

} // END OF pauseSpecificIRQ 



/********************************************************************************************************
* @brief resumes a paused individual interrupt source at the AXI Interrupt Controller.  Tested with fast,  
* but shouild also work with normal interrupts. 
*
* @author original: Hab Collector \n
*
* @note: This function operates at the AXI INTC level (IER/IPR/IAR registers) and does not
*        globally disable MicroBlaze exceptions.  Only interrupt sources enabled in the
*        AXI INTC are affected.
*
* @param IRQ_ControllerHandle: Pointer to initialized AXI Interrupt Controller handle
* @param InterruptId: Fabric ID of the interrupt to pause
*
* STEP 1: Enable only the specific interrupt bit
********************************************************************************************************/
void resumeSpecificIRQ(XIntc *IRQ_ControllerHandle, uint8_t InterruptId)
{
    uint32_t BaseAddress = IRQ_ControllerHandle->CfgPtr->BaseAddress;
    uint32_t Mask = (1 << InterruptId);

    // STEP 1: Enable only the specific interrupt bit
    uint32_t CurrentIER = Xil_In32(BaseAddress + XIN_IER_OFFSET);
    Xil_Out32(BaseAddress + XIN_IER_OFFSET, CurrentIER | Mask);

} // END OF resumeSpecificIRQ
