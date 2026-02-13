/******************************************************************************************************
 * @file            Signal_Mode_API.h
 * @brief           Header file to support Signal_Mode_API.c
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

#ifndef SIGNAL_MODE_API_H_
#define SIGNAL_MODE_API_H_
#ifdef __cplusplus
extern"C" {
#endif

#include <stdint.h>
#include <stdbool.h>


// DEFINES


// TYPEDEFS AND ENUMS
typedef enum
{
    SIGNAL_ON_BOARD_OSCILLATOR = 0,
    SIGNAL_OFF_BOARD_BNC
}Type_SignalSelect;


// FUNCTION PROTOTYPES
void signalSelect(Type_SignalSelect Signal);


#ifdef __cplusplus
}
#endif
#endif /* SIGNAL_MODE_API_H_ */
