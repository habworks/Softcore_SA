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


/********************************************************************************************************
* @brief Init of MCP23S08 IC for use.  This IC can be used in one of two ways.  You can address it via a 
* dedicated CS signal or you can address it via a shared address chip signal.  The function also allows for
* a shared reset lines.  You must consider both if you are to init multiple devices.  If using a shared chip
* select then on the first init Set_HAEN should be true while subseqent init calls should be false.  If the 
* reset line is also shared the first InitReset should be true all subsequent calls should be false.   This 
* function is writen to be agnostic to the embedded system in which it resides the necessary calling function 
* are passed via function pointers.  
*
* @author original: Hab Collector \n
*
* @note: Use as shared select was not tested - tested only with seperate chip select - see brief
* @note: If you are using the dedicated chip select of your quad SPI driver, or no cip select at all - create a dummy do nothing function
* @note: See datasheet for configuration setup
* @note: Function created via use of a SPI bus, should not be too difficult to change to an I2C bus
* 
* @param MCP23S08_Handle: Pointer to the MCP23S08 handle
* @param chipResetFunction: Function pointer to a chip reset function
* @param chipSelectFunction: Function pointer to a chip reset function ***see notes***
* @param transmitReceive: Function pointer to a transmit receive function
* @param delayFunction: Function pointer to a blocking delay in ms function
* @param SPI_Handle: Pointer to the bus handle 
* @param CS_Number: When a shared SPI controller is used this is the chip select number associated with the MCP23S08 device
* @param DeviceAddress: Hardware address of the MCP23S08 A0 and A1
* @param IO_Direction: Pin direction 1 = input, 0 = output
* @param InputPolarity: Pin Inverted input polarity 1 = inverted, 0 = not inverted
* @param IRQ_OnChange: Pin input to set IRQ on change
* @param IRQ_Default: Pin default values for IRQs
* @param IRQ_Control: Pin set to 1 for IRQ on compare vs default, 0 for IRQ on change
* @param Configuration: Configuration register value
* @param PullUp: Pin pullup 1 = 100K pullup 0 = no pullup
*
* @return True if init OK
*
* STEP 1: Simple test
* STEP 2: Assign struct member values
* STEP 3: Peform reset
* STEP 4: Set condtions for Address decoding option
* STEP 5: Configure the expander for use
* STEP 6: Make expander ready for use
********************************************************************************************************/
bool init_MCP23S08(Type_MCP23S08_Driver *MCP23S08_Handle, chipResetFunctionPointer chipResetFunction, chipSelectFunctionPointer chipSelectFunction, TxRxFunctionPointer TxRxFunction, delayFunctionPointer delayFunction,
                   XSpi *SPI_Handle, uint8_t CS_Number, uint8_t DeviceAddress, uint8_t IO_Direction, uint8_t InputPolarity, uint8_t IRQ_OnChange, uint8_t IRQ_Default, 
                   uint8_t IRQ_Control, uint8_t Configuration, uint8_t PullUp, bool Set_HAEN, bool InitReset)
{
    // STEP 1: Simple test
    MCP23S08_Handle->Ready = false;
    if ((chipResetFunction == NULL) || (chipSelectFunction == NULL) || (TxRxFunction == NULL) || (delayFunction == NULL) || (SPI_Handle == NULL))
        return(false);
    
    // STEP 2: Assign struct member values
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

    // STEP 3: Peform reset
    if (InitReset)
    {
        MCP23S08_Handle->chipReset(true);
        MCP23S08_Handle->delay_ms(MCP23S08_RESET_TIME_MS);
        MCP23S08_Handle->chipReset(false);
    }

    // STEP 4: Set condtions for Address decoding option
    if (Set_HAEN)
    {    
        Status = MCP23S08_HAEN(MCP23S08_Handle);
        if (Status == false)
            return(false);
    }

    // STEP 5: Configure the expander for use
    Status = MCP23S08_WriteRegister(MCP23S08_Handle, MCP23S08_REG_IODIR, IO_Direction)
             & MCP23S08_WriteRegister(MCP23S08_Handle, MCP23S08_REG_IPOL, InputPolarity)
             & MCP23S08_WriteRegister(MCP23S08_Handle, MCP23S08_REG_GPINTEN, IRQ_OnChange)
             & MCP23S08_WriteRegister(MCP23S08_Handle, MCP23S08_REG_DEFVAL, IRQ_Default)
             & MCP23S08_WriteRegister(MCP23S08_Handle, MCP23S08_REG_INTCON, IRQ_Control)
             & MCP23S08_WriteRegister(MCP23S08_Handle, MCP23S08_REG_IOCON, Configuration)
             & MCP23S08_WriteRegister(MCP23S08_Handle, MCP23S08_REG_GPPU, PullUp);
    
    // STEP 6: Make expander ready for use
    MCP23S08_Handle->Ready = Status;
    return(Status);

} // END OF init_MCP23S08



/********************************************************************************************************
* @brief MCP23S08 write a value to a register.  This function will also read that register to confirm the
* value was written successfully.  
*
* @author original: Hab Collector \n
* 
* @param MCP23S08_Handle: Pointer to the MCP23S08 handle
* @param Register: Register to write to
* @param Data: Datum to write to said register
*
* @return True if write OK
*
* STEP 1: Simple test
* STEP 2: Build MCP23S08 write command (0100 A1 A0 0)
* STEP 3: Select register address
* STEP 4: Provide register data to write
* STEP 5: Assert chip select for this device
* STEP 6: Perform SPI transaction (3 bytes)
* STEP 7: Deassert chip select
* STEP 8: Read register and verify the write
********************************************************************************************************/
bool MCP23S08_WriteRegister(Type_MCP23S08_Driver *MCP23S08_Handle, uint8_t Register, uint8_t Data)
{
    uint8_t TxBuffer[3];
    uint8_t RxBuffer[3];

    // STEP 1: Simple test
    if (MCP23S08_Handle == NULL)
        return(0x00);

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

    // STEP 8: Read register and verify the write
    uint8_t RegisterValue;
    MCP23S08_ReadRegister(MCP23S08_Handle, Register, &RegisterValue);
    return(Data == RegisterValue);

} // END OF MCP23S08_WriteRegister



/********************************************************************************************************
* @brief MCP23S08 read register value.  
*
* @author original: Hab Collector \n
* 
* @param MCP23S08_Handle: Pointer to the MCP23S08 handle
* @param Register: Register to write to
* @param RegisterValue: Read register value returned by reference
*
* @return True if read OK
*
* STEP 1: Simple test
* STEP 2: Build MCP23S08 write command (0100 A1 A0 0)
* STEP 3: Register address
* STEP 4: Dummy byte to clock in data
* STEP 5: Assert chip select
* STEP 6: Perform SPI transaction (3 bytes)
* STEP 7: Deassert chip select
* STEP 8: Captured register value is third byte received
********************************************************************************************************/
bool MCP23S08_ReadRegister(Type_MCP23S08_Driver *MCP23S08_Handle, uint8_t Register, uint8_t *RegisterValue)
{
    uint8_t TxBuffer[3];
    uint8_t RxBuffer[3];

    // STEP 1: Simple test
    if (MCP23S08_Handle == NULL)
        return(0x00);

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

} // END OF MCP23S08_ReadRegister



/********************************************************************************************************
* @brief MCP23S08 placed in hardware address mode - Hardware Address Enabled
*
* @author original: Hab Collector \n
* 
* @param MCP23S08_Handle: Pointer to the MCP23S08 handle
* @param Register: Register to write to
* @param RegisterValue: Read register value returned by reference
*
* @return True HAEN OK
*
* STEP 1: Simple test
* STEP 2: Build the FIRST-CONTACT write opcode
* STEP 3: Assert chip select
* STEP 4: SPI transaction (3 bytes)
* STEP 5: Deassert chip select
********************************************************************************************************/
bool MCP23S08_HAEN(Type_MCP23S08_Driver *MCP23S08_Handle)
{
    uint8_t TxBuffer[3];
    uint8_t RxBuffer[3];

    // STEP 1: Simple test
    if (MCP23S08_Handle == NULL) 
        return(0x00);

    // STEP 2: Build the FIRST-CONTACT write opcode
    // HAEN=0 at POR, so address bits must be 00 -> use MCP23S08_DEFAULT_ADDR only.
    TxBuffer[0] = MCP23S08_DEFAULT_ADDR;     // 0x40 (write, address = 00)
    TxBuffer[1] = MCP23S08_REG_IOCON;        // IOCON register
    TxBuffer[2] = MCP23S08_HAEN_BIT_MASK;    // Enable HAEN (bit 3)

    // STEP 3: Assert chip select
    MCP23S08_Handle->chipSelect(true);

    // STEP 4: SPI transaction (3 bytes)
    bool Status = MCP23S08_Handle->transmitReceive(MCP23S08_Handle->SPI_Handle, MCP23S08_Handle->CS_Number, TxBuffer, RxBuffer, (uint32_t)sizeof(TxBuffer));

    // STEP 5: Deassert chip select
    MCP23S08_Handle->chipSelect(false);
    return(Status);

} // END OF MCP23S08_HAEN



/********************************************************************************************************
* @brief MCP23S08 write to output pins.  Note input pins will be ignored.
*
* @author original: Hab Collector \n
* 
* @param MCP23S08_Handle: Pointer to the MCP23S08 handle
* @param OutputValue: Pin output value
*
* @return True output pin write OK
*
* STEP 1: Simple test
* STEP 2: Write output latch (updates output pins configured as outputs)
********************************************************************************************************/
bool MCP23S08_WriteOutput(Type_MCP23S08_Driver *MCP23S08_Handle, uint8_t OutputValue)
{
    // STEP 1: Simple test
    if ((MCP23S08_Handle == NULL) || (!MCP23S08_Handle->Ready))
        return(0x00);

    // STEP 2: Write output latch (updates output pins configured as outputs)
    if (!MCP23S08_WriteRegister(MCP23S08_Handle, MCP23S08_REG_OLAT, OutputValue))
        return(false);

    return(true);

} // END OF MCP23S08_WriteOutput


/********************************************************************************************************
* @brief MCP23S08 on interrupt read and clear the interrupt.  This function to be called to service the 
* device interrupt pin going active.  This function will capture both rising and falling edges.  The input
* paramter EdgeCapture determines which type the calling function is concerned with.
*
* @author original: Hab Collector \n
*
* @note:  For this function to work as expected.  The following registers (per pin) should be as such:
* MCP23S08_REG_GPINTEN = 1
* MCP23S08_REG_DEFVAL = 0
* MCP23S08_REG_INTCON = 0
*
* @param MCP23S08_Handle: Pointer to the MCP23S08 handle
* @param EdgeCapture: The desired interrupt type expected 
*
* @return The interupt input set high for rising or falling types - 0 error (no IRQs)
*
* STEP 1: Simple test
* STEP 2: Read INTF to determine which pins triggered the interrupt
* STEP 3: Read INTCAP to clear the interrupt condition
* STEP 4: Return interrupt flags (bit high = interrupt occurred) based on desired edge capture
********************************************************************************************************/
uint8_t MCP23S08_ReadClear_IRQ(Type_MCP23S08_Driver *MCP23S08_Handle, Type_InterruptCapture EdgeCapture)
{
    uint8_t InterruptFlags = 0;
    uint8_t InterruptInputState;

    // STEP 1: Simple test
    if ((MCP23S08_Handle == NULL) || (!MCP23S08_Handle->Ready))
        return(0x00);

    // STEP 2: Read INTF to determine which pins triggered the interrupt
    if (!MCP23S08_ReadRegister(MCP23S08_Handle, MCP23S08_REG_INTF, &InterruptFlags))
        return(0x00);

    // STEP 3: Read INTCAP to clear the interrupt condition
    if (!MCP23S08_ReadRegister(MCP23S08_Handle, MCP23S08_REG_INTCAP, &InterruptInputState))
        return(0x00);

    // STEP 4: Return interrupt flags (bit high = interrupt occurred) based on desired edge capture
    if (EdgeCapture == RISING_EDGE)
        return(InterruptFlags & InterruptInputState);
    else
        return(InterruptFlags & ~InterruptInputState);

} // END OF MCP23S08_ReadClear_IRQ


