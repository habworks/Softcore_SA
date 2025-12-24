/******************************************************************************************************
 * @file            IO_Support.h
 * @brief           Header file to support IO_Support.c (if part of code base or can be used alone)
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

#ifndef IO_SUPPORT_H_
#define IO_SUPPORT_H_
#ifdef __cplusplus
extern"C" {
#endif

// DEFINES GPIO BIT MASK: INPUTS
#define SW_0                ((uint32_t)(0x01 << 0))
#define SW_1                ((uint32_t)(0x01 << 1))
#define PB_1                ((uint32_t)(0x01 << 2))
#define PB_2                ((uint32_t)(0x01 << 3))
#define PB_3                ((uint32_t)(0x01 << 4))
#define USD_CD              ((uint32_t)(0x01 << 5))
#define DDR_CALIB_COMPLETE  ((uint32_t)(0x01 << 6))
#define HW_CONST_PL_VER     ((uint32_t)(0x0F << 7))

// DEFINES GPIO BIT MASK: OUTPUTS
#define TIMER_0_OUTPUT      ((uint32_t)(0x01 << 0))
#define TIMER_1_OUTPUT      ((uint32_t)(0x01 << 1))
#define DISPLAY_RESET_RUN   ((uint32_t)(0x01 << 2))
#define DISPLAY_CMD_DATA    ((uint32_t)(0x01 << 3))
#define DISPLAY_CS          ((uint32_t)(0x01 << 4))    


#ifdef __cplusplus
}
#endif
#endif /* IO_SUPPORT_H_ */