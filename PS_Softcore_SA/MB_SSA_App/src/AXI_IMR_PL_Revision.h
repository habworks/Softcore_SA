/******************************************************************************************************
 * @file            AXI_IMR_PL_Revision.h
 * @brief           Header file to support IMR Custom IP PL Revision
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

#ifndef AXI_IMR_PL_REVISION_H_
#define AXI_IMR_PL_REVISION_H_
#ifdef __cplusplus
extern"C" {
#endif

#include <stdint.h>
#include <stdbool.h>

// DEFINES
#define PL_REVISION_MAJOR_OFFSET    0x00
#define PL_REVISION_MINOR_OFFSET    0X04
#define PL_REVISION_TEST_OFFSET     0X08


// TYPEDEFS AND ENUMBS
typedef struct
{
    uint8_t     Major;
    uint8_t     Minor;
    uint8_t     Test;
}Type_PL_Revision;


// PROTOTYPE FUNCTIONS
Type_PL_Revision IMR_PL_RevisionGet(uint32_t IP_BaseAddress);


#ifdef __cplusplus
}
#endif
#endif /* AXI_IMR_PL_REVISION_H_ */