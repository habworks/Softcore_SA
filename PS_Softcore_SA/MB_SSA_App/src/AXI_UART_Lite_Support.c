/******************************************************************************************************
 * @file            AXI_UART_Lite_Support.c
 * @brief           A collection of functions relevant to the AXI UART Lite peripherals
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

#include "AXI_UART_Lite_Support.h"


/********************************************************************************************************
* @brief Init of an AXI UART Lite IP Block for use.  The UART can be started in either polling or IRQ mode.
* Polling mode is the easire to implement and does not require ISR callback functions.  When using in IRQ
* mode see function init_IRQ_Controller function header notes on how to add the UART to the MicroBlaze 
* mechanism.  UART Lite does not support DMA.  UART Lite the bps rate must be set in Vivado - it cannot
* be changed in firmware once it is set.  
*
* @author original: Hab Collector \n
*
* @note: See peripheral notes on AXI IRQ Controller
* @note: Generally speaking there is only 1 AXI IRQ Controller in the design ID is 0
* @note: See BSP xuartlite.h for peripheral specifics based version of timer in use (supports v3.9)
* 
* @param UART_Handle: Pointer to the UART Lite handle that will be used 
* @param IPB_BaseAddress: IPB Base address of the handle can be found in xparameters.h
* @param OperatingMode: must be POLLING or INTERRUPT
* @param TxCallBack: Valid in INTERRUPT mode only - the ISR transmit callback function (see example) - if in polling mode pass NULL
* @param RxCallBack: Valid in INTERRUPT mode only - the ISR receive callback function (see example) - if in polling mode pass NULL
*
* @return True if init OK
*
* STEP 1: Initializes a specific AXI INTC instance
* STEP 2: UART self test
* STEP 3: Clear Tx and Rx FIFOs
* STEP 4: If in IRQ mode assign the Tx and Rx ISR callbacks
********************************************************************************************************/
bool init_UART_Lite(XUartLite *UART_Handle, UINTPTR IPB_BaseAddress, Type_OperatingMode OperatingMode, XUartLite_Handler TxCallBack, XUartLite_Handler RxCallBack)
{
    int AXI_Status;

    // STEP 1: Initializes a specific AXI INTC instance
    AXI_Status = XUartLite_Initialize(UART_Handle, IPB_BaseAddress);
    if (AXI_Status != XST_SUCCESS)
        return(false);

    // STEP 2: UART self test
    AXI_Status = XUartLite_SelfTest(UART_Handle);
    if (AXI_Status != XST_SUCCESS)
        return(false);

    // STEP 3: Clear Tx and Rx FIFOs
    XUartLite_ResetFifos(UART_Handle);

    // STEP 4: If in IRQ mode assign the Tx and Rx ISR callbacks
    if (OperatingMode == POLLING)
        return(true);
    XUartLite_SetSendHandler(UART_Handle, TxCallBack, UART_Handle);
    XUartLite_SetRecvHandler(UART_Handle, RxCallBack, UART_Handle);

    return(true);

} // END OF init_UART_Lite



/********************************************************************************************************
* @brief Transmit of UART data.  This function works in polling or IRQ mode.  In polling mode the use of
* xil_printf function is suggested for printed outpupts to terminal.  See notes on function init_UART_Lite.
* When used in IRQ mode it will be necessary to build a print message queue, otherwise it will be very possible
* that messages will be lost as this function does not buffer data - if busy, new data will be lost.  In polling
* mode it does not block.
*
* @author original: Hab Collector \n
*
* @note: See peripheral notes on AXI IRQ Controller when using in IRQ mode
* @note: Works in polling or IRQ mode - in polling mode it does not block
* @note: See BSP xuartlite.h for peripheral specifics based version of timer in use (supports v3.9)
* @note: When used in IRQ mode, within the ISR Tx callback function - step 1 is necessary
* 
* @param UART_Handle: Pointer to the UART Lite handle that will be used 
* @param TxDataBuffer: Buffer of data to be transmitted
* @param DataBufferSize: Length in bytes of data to be transmitted
* @param BytesSent: Return by reference the number of bytes transmitted
*
* @return True if init OK
*
* STEP 1: Do not transmit unless present buffer is empty
* STEP 2: Transmit the data
********************************************************************************************************/
bool transmit_UART(XUartLite *UART_Handle, uint8_t *TxDataBuffer, uint16_t DataBufferSize, uint16_t *BytesSent)
{
    // STEP 1: Do not transmit unless present buffer is empty
    if ((DataBufferSize == 0) || (XUartLite_IsSending(UART_Handle)))
    {
        *BytesSent = 0;
        return(false);
    }
        
    // STEP 2: Transmit the data
    *BytesSent = XUartLite_Send(UART_Handle, TxDataBuffer, DataBufferSize);
    if (*BytesSent == DataBufferSize)
        return(true);
    else
        return(false);

} // END OF transmit_UART



/********************************************************************************************************
* @brief Receive of UART data.  This function works in polling or IRQ mode.  See notes on function init_UART_Lite.
* When used in IRQ mode it is suggested to use of a Rx message buffer.  In polling mode it does not block.
*
* @author original: Hab Collector \n
*
* @note: See peripheral notes on AXI IRQ Controller when using in IRQ mode
* @note: Works in polling or IRQ mode - in polling mode it does not block
* @note: See BSP xuartlite.h for peripheral specifics based version of timer in use (supports v3.9)
* @note: In IRQ mode the ISR callback function must call this function until the number of BytesReceived = 0, otherwise the ISR will not clear 
* @note: Parameter BytesReceived is only non-zero the first time it is called
* 
* @param UART_Handle: Pointer to the UART Lite handle that will be used 
* @param RxDataBuffer: Buffer for the data that will be recieved
* @param BytesToReceive: Number of bytes that can be recieved - make this larger than the actual number of bytes you expect to receive
* @param BytesReceived: Returned by reference the actual number of bytes received - ***This value is only non-zero the first time it is called***
*
* @return True if init OK
*
* STEP 1: Do not transmit unless present buffer is empty
* STEP 2: Transmit the data
********************************************************************************************************/
bool receive_UART(XUartLite *UART_Handle, uint8_t *RxDataBuffer, uint16_t BytesToReceive, uint16_t *BytesReceived)
{
    // STEP 1: Receive data
    *BytesReceived = XUartLite_Recv(UART_Handle, RxDataBuffer, BytesToReceive);
    return(true);

} // END OF receive_UART