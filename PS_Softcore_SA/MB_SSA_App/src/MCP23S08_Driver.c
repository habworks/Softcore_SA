/******************************************************************************************************
 * @file            MCP23S08_Driver.c
 * @brief           A collection of functions relevant to supporting the IO Expander MCP23S08
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

#include "MCP23S08_Driver.h"

bool init_MCP23S08(Type_MCP23S08_Driver *MCP23S08_Handle, chipResetFunctionPointer chipResetFunction, chipSelectFunctionPointer chipSelectFunction, TxRxFunctionPointer TxRxFunction, delayFunctionPointer delayFunction,
                   XSpi *SPI_Handle, uint8_t CS_Number, uint8_t DeviceAddress, uint8_t IO_Direction, uint8_t InputPolarity, uint8_t IRQ_OnChange, uint8_t IRQ_Default, 
                   uint8_t IRQ_Control, uint8_t Configuration, uint8_t PullUp, bool Set_HAEN)
{
    MCP23S08_Handle->Ready = false;
    if ((chipResetFunction == NULL) || (chipSelectFunction == NULL) || (TxRxFunction == NULL) || (SPI_Handle == NULL))
        return(false);
    
    bool Status;

    MCP23S08_Handle->chipReset = chipResetFunction;
    MCP23S08_Handle->chipSelect = chipSelectFunction;
    MCP23S08_Handle->transmitReceive = TxRxFunction;
    MCP23S08_Handle->delay_ms = delayFunction;
    MCP23S08_Handle->SPI_Handle = SPI_Handle;
    MCP23S08_Handle->CS_Number = CS_Number;
    MCP23S08_Handle->DeviceAddress = DeviceAddress;
    MCP23S08_Handle->IO_Direction = IO_Direction;
    MCP23S08_Handle->InputPolarity = InputPolarity;
    MCP23S08_Handle->IRQ_OnChange = IRQ_OnChange;
    MCP23S08_Handle->IRQ_Default = IRQ_Default;
    MCP23S08_Handle->IRQ_Control = IRQ_Control;
    MCP23S08_Handle->Configuration = Configuration;
    MCP23S08_Handle->PullUp = PullUp;

    if (MCP23S08_Handle->delay_ms != NULL)
    {
        MCP23S08_Handle->chipReset(true);
        MCP23S08_Handle->delay_ms(MCP23S08_RESET_TIME_MS);
        MCP23S08_Handle->chipReset(false);
    }

    if (Set_HAEN)
    {    
        Status = MCP23S08_HAEN(MCP23S08_Handle);
        if (Status == false)
            return(false);
    }

    Status = MCP23S08_WriteRegister(MCP23S08_Handle, MCP23S08_REG_IODIR, IO_Direction);
    Status = MCP23S08_WriteRegister(MCP23S08_Handle, MCP23S08_REG_IPOL, InputPolarity);
    Status = MCP23S08_WriteRegister(MCP23S08_Handle, MCP23S08_REG_GPINTEN, IRQ_OnChange);
    Status = MCP23S08_WriteRegister(MCP23S08_Handle, MCP23S08_REG_DEFVAL, IRQ_Default);
    Status = MCP23S08_WriteRegister(MCP23S08_Handle, MCP23S08_REG_INTCON, IRQ_Control);
    Status = MCP23S08_WriteRegister(MCP23S08_Handle, MCP23S08_REG_IOCON, Configuration);
    Status = MCP23S08_WriteRegister(MCP23S08_Handle, MCP23S08_REG_GPPU, PullUp);
    
    MCP23S08_Handle->Ready = Status;
    return(Status);

}


bool MCP23S08_WriteRegister(Type_MCP23S08_Driver *MCP23S08_Handle, uint8_t Register, uint8_t Data)
{
    uint8_t TxBuffer[3];
    uint8_t RxBuffer[3];

    // STEP 1: Simple test
    if ((MCP23S08_Handle == NULL) || (MCP23S08_Handle->chipSelect == NULL) || (MCP23S08_Handle->transmitReceive == NULL))
        return(false);

    // STEP 2: Build MCP23S08 write command (0100 A1 A0 0)
    TxBuffer[0] = (uint8_t)(MCP23S08_DEFAULT_ADDR | ((MCP23S08_Handle->DeviceAddress & 0x03) << 1) | 0x00);

    // STEP 3: Select register address
    TxBuffer[1] = Register;

    // STEP 4: Provide register data to write
    TxBuffer[2] = Data;

    // STEP 5: Assert chip select for this device
    MCP23S08_Handle->chipSelect(true);

    // STEP 6: Perform SPI transaction (3 bytes)
    bool Status = MCP23S08_Handle->transmitReceive(MCP23S08_Handle->SPI_Handle, MCP23S08_Handle->CS_Number, TxBuffer, RxBuffer, (uint32_t)sizeof(TxBuffer));
    if (Status == false)
        return(false);

    // STEP 7: Deassert chip select
    MCP23S08_Handle->chipSelect(false);

    uint8_t RegisterValue;
    MCP23S08_ReadRegister(MCP23S08_Handle, Register, &RegisterValue);
    return(Data == RegisterValue);
}

bool MCP23S08_ReadRegister(Type_MCP23S08_Driver *MCP23S08_Handle, uint8_t Register, uint8_t *RegisterValue)
{
    uint8_t TxBuffer[3];
    uint8_t RxBuffer[3];

    // STEP 1: Simple test
    if ((MCP23S08_Handle == NULL) || (MCP23S08_Handle->chipSelect == NULL) || (MCP23S08_Handle->transmitReceive == NULL))
        return(false);

    // STEP 2: Build MCP23S08 read opcode (0100 A1 A0 1)
    TxBuffer[0] = (uint8_t)(MCP23S08_DEFAULT_ADDR | ((MCP23S08_Handle->DeviceAddress & 0x03) << 1) | 0x01);
    
    // STEP 3: Register address
    TxBuffer[1] = Register;
    
    // STEP 4: Dummy byte to clock in data
    TxBuffer[2] = 0x00;

    // STEP 5: Assert chip select
    MCP23S08_Handle->chipSelect(true);

    // STEP 6: Perform SPI transaction (3 bytes)
    bool Status = MCP23S08_Handle->transmitReceive(MCP23S08_Handle->SPI_Handle, MCP23S08_Handle->CS_Number, TxBuffer, RxBuffer, (uint32_t)sizeof(TxBuffer));

    // STEP 7: Deassert chip select
    MCP23S08_Handle->chipSelect(false);

    // STEP 8: Captured register value is third byte received
    *RegisterValue = RxBuffer[2];
    return(Status);
}

bool MCP23S08_HAEN(Type_MCP23S08_Driver *MCP23S08_Handle)
{
    uint8_t TxBuffer[3];
    uint8_t RxBuffer[3];

    // STEP 1: Simple test
    if ((MCP23S08_Handle == NULL) || (MCP23S08_Handle->chipSelect == NULL) || (MCP23S08_Handle->transmitReceive == NULL))
        return(false);

    // STEP 2: Build the FIRST-CONTACT write opcode.
    // HAEN=0 at POR, so address bits must be 00 -> use MCP23S08_DEFAULT_ADDR only.
    TxBuffer[0] = MCP23S08_DEFAULT_ADDR;     // 0x40 (write, address = 00)
    TxBuffer[1] = MCP23S08_REG_IOCON;        // IOCON register
    TxBuffer[2] = MCP23S08_HAEN_BIT_MASK;    // Enable HAEN (bit 3)

    // STEP 2: Assert chip select
    MCP23S08_Handle->chipSelect(true);

    // STEP 3: SPI transaction (3 bytes)
    bool Status = MCP23S08_Handle->transmitReceive(MCP23S08_Handle->SPI_Handle, MCP23S08_Handle->CS_Number, TxBuffer, RxBuffer, (uint32_t)sizeof(TxBuffer));

    // STEP 4: Deassert chip select
    MCP23S08_Handle->chipSelect(false);
    return(Status);
}

bool MCP23S08_WriteOutput(Type_MCP23S08_Driver *MCP23S08_Handle, uint8_t OutputValue)
{
    if (MCP23S08_Handle == NULL)
        return(false);

    // STEP 1: Write output latch (updates output pins configured as outputs)
    if (!MCP23S08_WriteRegister(MCP23S08_Handle, MCP23S08_REG_OLAT, OutputValue))
        return(false);

    return(true);
}

