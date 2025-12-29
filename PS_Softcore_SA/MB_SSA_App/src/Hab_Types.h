/******************************************************************************************************
 * @file            Hab_Types.h
 * @brief           Various defines and types I like to use
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

#ifndef HAB_TYPES_H_
#define HAB_TYPES_H_
#include <limits.h>
#ifdef __cplusplus
extern"C" {
#endif

// DEFINES
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// MACROS
#define NOT_USED(x)            (void)(x)


#ifdef __cplusplus
}
#endif
#endif /* HAB_TYPES_H_ */
