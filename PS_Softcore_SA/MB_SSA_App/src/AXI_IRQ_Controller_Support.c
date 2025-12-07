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


/********************************************************************************************************
* @brief Init of an AXI IRQ Controller IP Block for use.  On success of the init start the conroller.  When
* using the IRQ Controller all peripherals should be init before calling this function.  This is STEP 1 of
* a 3 STEP process in setting up the IRQ Controller and IRQ ISR Callbacks for perhiperal devices.  This 
* function should be called only once.  
*
* @author original: Hab Collector \n
*
* @note: This is Step 1 of 4
* @note: See peripheral AXI IRQ Controller
* @note: Generally speaking there is only 1 AXI IRQ Controller in the design ID is 0
* @note: See BSP xintc.h for peripheral specifics based version of timer in use (supports v3.19)
* 
* @param IRQ_ControllerHandle: Pointer to the IRQ Controller  handle that will be used 
* @param IRQ_ControllerDevice_ID: If there is only 1, and there should only be 1 IRQ Controller in your design the device ID = 0
*
* @return True if init OK
*
* STEP 1: Initializes a specific AXI INTC instance
* STEP 2: Starts the interrupt controller operation
********************************************************************************************************/
bool init_IRQ_Controller(XIntc *IRQ_ControllerHandle, uint8_t IRQ_ControllerDevice_ID)
{
    int AXI_Status;

    // STEP 1: Initializes a specific AXI INTC instance
    AXI_Status = XIntc_Initialize(IRQ_ControllerHandle, IRQ_ControllerDevice_ID);
    if (AXI_Status != XST_SUCCESS)
        return(false);

    // STEP 2: Starts the interrupt controller operation
    AXI_Status = XIntc_Start(IRQ_ControllerHandle, XIN_REAL_MODE);
    if (AXI_Status != XST_SUCCESS)
        return(false);
    else
        return(true);
    
} // END OF init_IRQ_Controller



/********************************************************************************************************
* @brief Connects a peripheral IRQ to the IRQ Controller.  This is step 2 of a 3 step process.  This function
* should be called for each peripheral based IRQ.
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
* AXI peripheral itself - it is where you enable said AXI peripheral for use IRQ Mode
*
* @author original: Hab Collector \n
*
* @note: This is Step 3 of 4
* @note: Read header notes on step 4 - step 4 not part of this api set
* @note: See peripheral AXI IRQ Controller
* @note: Generally speaking there is only 1 AXI IRQ Controller in the design ID is 0
* @note: See BSP xintc.h for peripheral specifics based version of timer in use (supports v3.19)
* 
* @param IRQ_ControllerHandle: Pointer to the IRQ Controller  handle that will be used 
*
* STEP 1: Initializes the exception handling system
* STEP 2: For the AXI INTC  register the AXI INTC interrupt handler itself as a general exception handler
* STEP 3: Enables exceptions globally in the processor
********************************************************************************************************/
void enableExceptionHandling(XIntc *IRQ_ControllerHandle)
{
    // STEP 1: Initializes the exception handling system
    Xil_ExceptionInit();

    // STEP 2: For the AXI INTC  register the AXI INTC interrupt handler itself as a general exception handler
    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT, (Xil_ExceptionHandler)XIntc_InterruptHandler, IRQ_ControllerHandle);

    // STEP 3: Enables exceptions globally in the processor
    Xil_ExceptionEnable();

} // END OF enableExceptionHandling


