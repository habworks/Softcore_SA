/******************************************************************************************************
 * @file            MCP23S08_Driver.h
 * @brief           Header file to support MCP23S08_Driver.c
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

#ifndef MCP23S08_DRIVER_H_
#define MCP23S08_DRIVER_H_
#ifdef __cplusplus
extern"C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "xspi.h"

// DEFINES
// ADDRESS
#define MCP23S08_DEFAULT_ADDR       ((uint8_t)0x40) 
// BIT MASKS FOR CONTROL REGISTER
#define MCP23S08_INTPOL_BIT_MASK    ((uint8_t)(0x01 << 1))  // Set polarity of interrupt output pin 1 = high 0 = low
#define MCP23S08_ODR_BIT_MASK       ((uint8_t)(0x01 << 2))  // Set polarity of interrupt output pin 1 = high 0 = low
#define MCP23S08_HAEN_BIT_MASK      ((uint8_t)(0x01 << 3))  // Set interrupt pin as OD 1 = Open Drain, 0 = Active Driver
#define MCP23S08_DISSLW_BIT_MASK    ((uint8_t)(0x01 << 4))  // Set slew rate control for SDA 1 = Slew rate enable, 0 = Slew rate disable
#define MCP23S08_SEQOP_BIT_MASK     ((uint8_t)(0x01 << 5))  // Sequential address pointer 1 = Address pointer does not advance, 0 = Address pointer does advance
// REGISTERS
#define MCP23S08_REG_IODIR          ((uint8_t)0x00)     // IO Direction 1 = Input, 0 = Output
#define MCP23S08_REG_IPOL           ((uint8_t)0x01)     // Input polarity inversion  1 = Inverted
#define MCP23S08_REG_GPINTEN        ((uint8_t)0x02)     // Interrupt On change 1 = Interrupt on change for use with input IRQ
#define MCP23S08_REG_DEFVAL         ((uint8_t)0x03)     // Interrupt Default Value compare for interrupt logic 
#define MCP23S08_REG_INTCON         ((uint8_t)0x04)     // Interrupt Control 1 = Compare against DEFVAL 0 = Interrupt on change
#define MCP23S08_REG_IOCON          ((uint8_t)0x05)     // Configuration Register: HAEN, ODR, INTPOL, SEQOP
#define MCP23S08_REG_GPPU           ((uint8_t)0x06)     // Internal Pull Up 1 = pullup (about 100K), 0 = no pullup
#define MCP23S08_REG_INTF           ((uint8_t)0x07)     // ***READ ONLY Interrupt Flag indicates which pin caused interrupt 
#define MCP23S08_REG_INTCAP         ((uint8_t)0x08)     // ***READ ONLY GPIO Values at time of interrupt -Reading clears interrupt
#define MCP23S08_REG_GPIO           ((uint8_t)0x09)     // Pin state value in real time
#define MCP23S08_REG_OLAT           ((uint8_t)0x0A)     // Output latch Write to update outputs / Read to see last latched output value
// MISC
#define MCP23S08_RESET_TIME_MS      5U                  // Value in ms

// TYPEDEFS AND ENUMS
typedef void (*chipResetFunctionPointer)(bool);
typedef void (*chipSelectFunctionPointer)(bool);
typedef bool (*TxRxFunctionPointer)(XSpi *, uint8_t, uint8_t *, uint8_t *, uint32_t);
typedef void (*delayFunctionPointer)(uint32_t);
typedef struct
{
    chipResetFunctionPointer    chipReset;
    chipSelectFunctionPointer   chipSelect;
    TxRxFunctionPointer         transmitReceive;
    delayFunctionPointer        delay_ms;
    XSpi                        *SPI_Handle;
    uint8_t                     CS_Number;
    uint8_t                     DeviceAddress;
    uint8_t                     IO_Direction;
    uint8_t                     InputPolarity;
    uint8_t                     IRQ_OnChange;
    uint8_t                     IRQ_Default;
    uint8_t                     IRQ_Control;
    uint8_t                     Configuration;
    uint8_t                     PullUp;
    bool                        Ready;
} Type_MCP23S08_Driver;

typedef enum
{
    RISING_EDGE = 0,
    FALLING_EDGE
} Type_InterruptCapture;


// FUNCTION PROTOTYPES
bool init_MCP23S08(Type_MCP23S08_Driver *MCP23S08_Handle, chipResetFunctionPointer chipResetFunction, chipSelectFunctionPointer chipSelectFunction, TxRxFunctionPointer TxRxFunction, delayFunctionPointer delayFunction,
                   XSpi *SPI_Handle, uint8_t CS_Number, uint8_t DeviceAddress, uint8_t IO_Direction, uint8_t InputPolarity, uint8_t IRQ_OnChange, uint8_t IRQ_Default, 
                   uint8_t IRQ_Control, uint8_t Configuration, uint8_t PullUp, bool Set_HAEN, bool InitReset);
bool MCP23S08_WriteRegister(Type_MCP23S08_Driver *MCP23S08_Handle, uint8_t Register, uint8_t Data);
bool MCP23S08_ReadRegister(Type_MCP23S08_Driver *MCP23S08_Handle, uint8_t Register, uint8_t *RegisterValue);
bool MCP23S08_HAEN(Type_MCP23S08_Driver *MCP23S08_Handle);
bool MCP23S08_WriteOutput(Type_MCP23S08_Driver *MCP23S08_Handle, uint8_t OutputValue);
uint8_t MCP23S08_ReadClear_IRQ(Type_MCP23S08_Driver *MCP23S08_Handle, Type_InterruptCapture EdgeCapture);

#ifdef __cplusplus
}
#endif
#endif /* MCP23S08_DRIVER_H_ */