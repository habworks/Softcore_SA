/******************************************************************************************************
 * @file            AXI_UART_Lite_Support.h
 * @brief           Header file to support AXI_UART_Lite_Support.c
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

#ifndef UART_LITE_SUPPORT_H_
#define UART_LITE_SUPPORT_H_
#ifdef __cplusplus
extern"C" {
#endif

#include "xuartlite.h"
#include <stdint.h>
#include <stdbool.h>

// TYPEDEFS AND ENUMS
typedef enum 
{
    POLLING = 0,
    INTERRUPT
}Type_OperatingMode;


// FUNCTION PROTOTYPES
bool init_UART_Lite(XUartLite *UART_Handle, UINTPTR IPB_BaseAddress, Type_OperatingMode OperatingMode, XUartLite_Handler TxCallBack, XUartLite_Handler RxCallBack);
bool transmit_UART(XUartLite *UART_Handle, uint8_t *TxDataBuffer, uint16_t DataBufferSize, uint16_t *BytesSent);
bool receive_UART(XUartLite *UART_Handle, uint8_t *RxDataBuffer, uint16_t BytesToReceive, uint16_t *BytesReceived);

#ifdef __cplusplus
}
#endif
#endif /* UART_LITE_SUPPORT_H_ */