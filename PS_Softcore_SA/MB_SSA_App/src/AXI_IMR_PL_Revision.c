/******************************************************************************************************
 * @file            AXI_IMR_PL_Revision.c
 * @brief           IMR custom IP that returns the revision value of the processor logic
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
 * PL Revision  OVERVIEW:
 * The IMR_PL_Revision custom IP is used to return the revision level of the PL.  The revision level is 
 * set in Vivado and from the generated bitstream the PL Major, Minor and Test revisions can be retrived.
 * All revision levels rages run from 0 to 255.
 *
 * @copyright       IMR Engineering, LLC
 ********************************************************************************************************/

 #include "AXI_IMR_PL_Revision.h"
 #include "xil_io.h"

 Type_PL_Revision IMR_PL_RevisionGet(uint32_t IP_BaseAddress)
 {
     Type_PL_Revision PL_Revision = {0,0,0};

     PL_Revision.Major = (uint8_t)Xil_In32(IP_BaseAddress + PL_REVISION_MAJOR_OFFSET);
     PL_Revision.Minor = (uint8_t)Xil_In32(IP_BaseAddress + PL_REVISION_MINOR_OFFSET);
     PL_Revision.Test = (uint8_t)Xil_In32(IP_BaseAddress + PL_REVISION_TEST_OFFSET);

     return(PL_Revision);

 } // END OF IMR_PL_RevisionGet