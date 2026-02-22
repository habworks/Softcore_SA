/******************************************************************************************************
 * @file            Water_Mark.h
 * @brief           Header file to support Water_Mark.c
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

#ifndef WATER_MARK_H_
#define WATER_MARK_H_
#ifdef __cplusplus
extern"C" {
#endif

#include <stdint.h>
#include <stdbool.h>


// DEFINES
#define STACK_HIGH_ADDRESS  _stack
#define STACK_LOW_ADDRESS   _stack_end


// TYPEDEFS AND ENUMS


// EXTERNS
// ***User must extern the stack start and end address - Modify externs to fit application linker
extern uint8_t _stack_end;
extern uint8_t _stack;


// FUNCTION PROTOTYPES
void seedStackForWaterMark(void);
uint32_t getStackHighWaterMarkBytes(void);

#ifdef __cplusplus
}
#endif
#endif /* WATER_MARK_H_ */