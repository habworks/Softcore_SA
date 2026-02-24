/******************************************************************************************************
 * @file            AXI_QSPI_Support.h
 * @brief           Header file to support AXI_QSPI_Support.c
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

#ifndef AXI_QSPI_SUPPORT_H_
#define AXI_QSPI_SUPPORT_H_
#ifdef __cplusplus
extern"C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "xspi.h"


// DEFINES AND ENUMS


// FUNCTION PROTOTYPES
bool init_QSPI_PollingMode(XSpi *QSPI_Handle, UINTPTR IPB_BaseAddress);

#ifdef __cplusplus
}
#endif
#endif /* AXI_QSPI_SUPPORT_H_ */