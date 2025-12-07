/******************************************************************************************************
 * @file            AXI_Timer_PWM_Support.h
 * @brief           Header file to support AXI_Timer_PWM_Support.c
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

#ifndef TIMER_PWM_SUPPORT_H_
#define TIMER_PWM_SUPPORT_H_
#ifdef __cplusplus
extern"C" {
#endif

#include "xtmrctr.h"
#include <stdint.h>
#include <stdbool.h>

// DEFINES
// PRE-PROCESSORS
#define USE_AXI_TIMER_IRQ_CALLBACK_API
//#define USE_SIMPLE_PWM_TIMER_CONFIG


// TYPEDEFS AND ENUMS
typedef void (*Type_TimerFunction_ISR)(void *, u8);


// FUNCTION PROTOTYPES
// PWM FUNCTIONS
bool init_PWM(XTmrCtr *TimerHandle, UINTPTR IPB_BaseAddress);
bool setup_PWM(XTmrCtr *TimerHandle, uint32_t PWM_Frequency, float DutyCyclePercent);
void enable_PWM(XTmrCtr *TimerHandle);
void disable_PWM(XTmrCtr *TimerHandle);
// TIMER FUNCTIONS
bool init_PeriodicTimer(XTmrCtr *TimerHandle, UINTPTR IPB_BaseAddress, u8 TimerNumber, u32 TimerIntervalTicks, Type_TimerFunction_ISR TimerFunction_ISR);
bool startPeriodicTimer(XTmrCtr *TimerHandle, u8 TimerNumber);
bool stopPeriodicTimer(XTmrCtr *TimerHandle, u8 TimerNumber);

#ifdef __cplusplus
}
#endif
#endif /* TIMER_PWM_SUPPORT_H_ */