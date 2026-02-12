/******************************************************************************************************
 * @file            AXI_QSPI_Support.c
 * @brief           A collection of functions relevant to the use of the AXI Quad SPI peripheral 
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
 *
 * @copyright       IMR Engineering, LLC
 ********************************************************************************************************/

#include "AXI_QSPI_Support.h"


/********************************************************************************************************
* @brief Init of an AXI QSPI IP Block for use in a polling mode.  
*
* @author original: Hab Collector \n
*
* @note: See peripheral AXI QSPI
* @note: This is the setup PWM for axi_timer_#
* @note: See BSP xspi.h for peripheral specifics based version of timer in use (supports v4.13)
* 
* @param QSPI_Handle: Pointer to the SPI handle that will be used 
* @param IPB_BaseAddress: Base address of the AXI SPI block - see xparameters.h XPAR_AXI_QUAD_SPI_#_BASEADDR)
*
* STEP 1: Init SPI and reset to a known start postiion for configuration
* STEP 2: Master mode + manual CS
* STEP 3: Disable interrupts
* STEP 4: Start the SPI engine
* STEP 5: Clear all slave selects
********************************************************************************************************/
bool init_QSPI_PollingMode(XSpi *QSPI_Handle, UINTPTR IPB_BaseAddress)
{
    int AXI_Status; 

    // STEP 1: Init SPI and reset to a known start postiion for configuration
    AXI_Status = XSpi_Initialize(QSPI_Handle,IPB_BaseAddress);
    if (AXI_Status != XST_SUCCESS)
        return(false);
    XSpi_Reset(QSPI_Handle);

    // STEP 2: Master mode + manual CS
    AXI_Status |= XSpi_SetOptions(QSPI_Handle,XSP_MASTER_OPTION | XSP_MANUAL_SSELECT_OPTION);
    
    // STEP 3: Disable interrupts
    XSpi_IntrGlobalDisable(QSPI_Handle);
    
    // STEP 4: Start the SPI engine
    AXI_Status |= XSpi_Start(QSPI_Handle);
    
    // STEP 5: Clear all slave selects
    XSpi_SetSlaveSelect(QSPI_Handle, 0); 
    
    return(AXI_Status == XST_SUCCESS);

} // END OF init_QSPI_PollingMode

