/******************************************************************************************************
 * @file            AXI_IRQ_Controller_Support.h
 * @brief           Header file to support AXI_IRQ_Controller_Support.c
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

#ifndef AXI_IRQ_CONTROLLER_SUPPORT_H_
#define AXI_IRQ_CONTROLLER_SUPPORT_H_
#ifdef __cplusplus
extern"C" {
#endif

#include "xintc.h"
#include <stdint.h>
#include <stdbool.h>

// DEFINES
#define ISR_OFFSET   ((uint32_t)(0x00))  // Interrupt Status Register
#define IPR_OFFSET   ((uint32_t)(0x04))  // Interrupt Pending Register
#define IER_OFFSET   ((uint32_t)(0x08))  // Interrupt Enable Register
#define IAR_OFFSET   ((uint32_t)(0x0C))  // Interrupt Acknowledge Register (write-1-to-clear)
#define SIE_OFFSET   ((uint32_t)(0x10))  // Set Interrupt Enable Register
#define CIE_OFFSET   ((uint32_t)(0x14))  // Clear Interrupt Enable Register
#define IVR_OFFSET   ((uint32_t)(0x18))  // Interrupt Vector Register
#define MER_OFFSET   ((uint32_t)(0x1C))  // Master Enable Register


// FUNCTION PROTOTYPES
bool init_IRQ_Controller(XIntc *IRQ_ControllerHandle, UINTPTR IPB_BaseAddress);
bool connectPeripheral_IRQ(XIntc *IRQ_ControllerHandle, uint8_t ISR_HandlerFabric_ID, XInterruptHandler ISR_Handler, void *ISR_CallbackReference);
bool connectPeripheralFast_IRQ(XIntc *IRQ_ControllerHandle, uint8_t ISR_HandlerFabric_ID, XInterruptHandler ISR_Handler, void *ISR_CallbackReference);
bool start_IRQ_Controller(XIntc *IRQ_ControllerHandle, uint8_t InterruptMode);
void enableDevice_IRQ_Controller(XIntc *IRQ_ControllerHandle, uint8_t DeviceInterrupt_ID);
void enableExceptionHandling(XIntc *IRQ_ControllerHandle);
uint32_t pauseFastIRQs(XIntc *IRQ_ControllerHandle);
void resumeFastIRQs(XIntc *IRQ_ControllerHandle, uint32_t SavedMask);
void pauseSpecificIRQ(XIntc *IRQ_ControllerHandle, uint8_t InterruptId);
void resumeSpecificIRQ(XIntc *IRQ_ControllerHandle, uint8_t InterruptId);

#ifdef __cplusplus
}
#endif
#endif /* AXI_IRQ_CONTROLLER_SUPPORT_H_ */