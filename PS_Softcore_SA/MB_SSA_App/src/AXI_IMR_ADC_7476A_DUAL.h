/******************************************************************************************************
 * @file            AXI_IMR_ADC_7476A_X2_.h
 * @brief           Header file to support IMR Custom IP Dual ADC7476A ADC
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

#ifndef AXI_IMR_ADC_7476A_X2__H_
#define AXI_IMR_ADC_7476A_X2__H_
#ifdef __cplusplus
extern"C" {
#endif

#include <stdint.h>
#include <stdbool.h>

// DEFINES
// ADDRESS OFFSET
// #define ADC_BASEADDR            XPAR_IMR_ADC_7476A_X2_0_BASEADDR
#define REG_CTRL_OFFSET         0x00        // Register 0: Control 
#define REG_STATUS_OFFSET       0x04        // Register 1: Status Register (read only)
#define REG_DATA_A_OFFSET       0x08        // Register 2: Data A value (read only) 12b
#define REG_DATA_B_OFFSET       0x0C        // Register 3: Data B value (read only) 12b
#define REG_IRQ_OFFSET          0x10        // Register 4: IRQ Register
// MISC
#define ADC_7476A_X2_FABRIC_ID  1           // I manually created this based on the ADC IP IRQ connection to the Concat block (2 means 3rd connection counting from 0 [x:0] where x is last connection)
#define IMR_ADC_CLOCK_DIVIDER   4           // Max ADC Clock 20MHz. SysClk = 100MHz - ClockDivider = 3, ADC_CLK = 16.6667MHz, ClockDivider = 4, ADC_CLK = 12.5MHz, ClockDivider = 5, ADC_CLK = 10.0MHz
//----------------------------- CTRL bitfields ---------------------------------
#define CTRL_EN_BIT             0           // enable engine
#define CTRL_START_BIT          1           // write 1 = single-shot start pulse
#define CTRL_MULTI_BIT          2           // 1 = continuous conversions
#define CTRL_CLKDIV_LSB         4           // SCLK divider field [7:4] - Total 4 bits
#define CTRL_CLKDIV_MSB         7           // SCLK = SYSCLK / (2*(N+1)) - Total 4 bits
#define CTRL_CONT_CNT_LSB       8           // Contineous Conversion Count LSB [19:8] - Total 12bits (4096 Max Conversions)
#define CTRL_CONT_CNT_MSB       19          // Contineous Conversion Count MSB
//----------------------------- IRQ bitfields ---------------------------------
#define IRQ_EN_BIT              0           // enable the IRQ
#define IRQ_CLR_BIT             1           // enable clear the pending irq
//----------------------------- Masks ---------------------------------
#define IRQ_ENABLE_MASK         (uint32_t)(0x01 << IRQ_EN_BIT)
#define IRQ_CLR_MASK            (uint32_t)(0x01 << IRQ_CLR_BIT)
#define CTRL_EN_BIT_MASK        (uint32_t)(0x01 << CTRL_EN_BIT)
#define CTRL_START_BIT_MASK     (uint32_t)(0x01 << CTRL_START_BIT)
#define CTRL_MULTI_BIT_MASK     (uint32_t)(0x01 << CTRL_MULTI_BIT)


// TYPEDEFS AND ENUMS
typedef struct
{
    uint8_t     ClockDivider;
    uint16_t *  ADC_Data_A;
    uint16_t *  ADC_Data_B;
    uint32_t    ADC_BaseAddress;
    uint32_t    ControlRegister;
    uint32_t    TotalConversions;
    uint32_t    ConversionCount;
} Type_AXI_IMR_7476A_Handle;


// FUNCTION PROTOTYPES
bool init_IMR_ADC_7476A_X2(Type_AXI_IMR_7476A_Handle *IP_Handle, uint32_t IP_BaseAddress, uint8_t ClockDivider);
bool IMR_ADC_7476A_X2_SingleConvert(Type_AXI_IMR_7476A_Handle *IP_Handle, uint16_t *BufferData_A, uint16_t *BufferData_B);
bool IMR_ADC_7476A_X2_MultiConvert(Type_AXI_IMR_7476A_Handle *IP_Handle, uint16_t *BufferData_A, uint16_t *BufferData_B, uint32_t TotalConversions);
void IMR_ADC_7476A_X2_ClrIrq(Type_AXI_IMR_7476A_Handle *IP_Handle);
uint32_t IMR_ADC_7476A_X2_GetCtrlReg(Type_AXI_IMR_7476A_Handle *IP_Handle);
uint32_t IMR_ADC_7476A_X2_GetStatusReg(Type_AXI_IMR_7476A_Handle *IP_Handle);
uint32_t IMR_ADC_7476A_X2_GetIrqReg(Type_AXI_IMR_7476A_Handle *IP_Handle);
uint32_t IMR_ADC_7476A_X2_GetDataAReg(Type_AXI_IMR_7476A_Handle *IP_Handle);
uint32_t IMR_ADC_7476A_X2_GetDataBReg(Type_AXI_IMR_7476A_Handle *IP_Handle);


#ifdef __cplusplus
}
#endif
#endif /* AXI_IMR_ADC_7476A_X2__H_ */