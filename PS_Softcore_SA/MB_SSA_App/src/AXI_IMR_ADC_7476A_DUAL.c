/******************************************************************************************************
 * @file            AXI_IMR_ADC_7476A_DUAL.h
 * @brief           Header file to support IMR Custome ADC IP which consist of dual 7476A ADCs
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
 * ADC IP FUNCTIONAL OVERVIEW:
 * The IMR_ADC_7476A IP implements a dual-channel interface to the AD7476A SPI ADC, supporting two modes:
 * 1. Single conversion mode – performs one 16-bit ADC conversion for each channel (A and B)
 * 2. Multi conversion mode – performs a fixed number of conversions and returns the sampled values.
 * In both modes, sampling is synchronized to an internally generated SCLK derived from the AXI clock via a divider.
 * Data is returned MSB-first via shift registers, and results are latched at the end of each frame.  See IP HDL notes for more information
 *
 * The ADC IP includes:
 *  - Control register (start, mode, clock divider)
 *  - Status register (busy, ready, debug)
 *  - Data A register (16-bit results per channel)
 *  - Data B register (16-bit results per channel)
 *  - IRQ register (enable/clear)
 *
 * @copyright       IMR Engineering, LLC
 ********************************************************************************************************/

 #include "AXI_IMR_ADC_7476A_DUAL.h"
 #include "xil_io.h"
 #include "xgpio.h"
 #include <math.h>

 extern XGpio AXI_GPIO_Handle;

/********************************************************************************************************
* @brief Init of the custom IMR ADC Dual ADC7476A IP Block for use.  
*
* @author original: Hab Collector \n
*
* @note: IP must be initialized before use
* @note: See IP HDL notes for more information on the IP operation
* 
* @param IP_Handle: Pointer to the IMR ADC Dual ADC7476A IP handle  
* @param IP_BaseAddress: Base address of the IMR ADC Dual ADC7476A IP
* @param ClockDivider: Clock divider value to generate SCLK for the ADCs
*
* @return True if init OK
*
* STEP 1: Load IP Handle members
* STEP 2: In all modes IRQ must be enabled
* STEP 3: Load the ADC Clock divider will be the same in all modes
* STEP 4: Clear GPIO output signals - This is the poor man's DMA signals
********************************************************************************************************/
 bool init_IMR_ADC_7476A_X2(Type_AXI_IMR_7476A_Handle *IP_Handle, uint32_t IP_BaseAddress, uint8_t ClockDivider)
 {
     if (IP_Handle ==  NULL)
        return(false);

     // STEP 1: Load IP Handle members
     IP_Handle->ADC_BaseAddress = IP_BaseAddress;
     IP_Handle->ControlRegister = 0x00;
     IP_Handle->ClockDivider = (uint32_t)ClockDivider;
     IP_Handle->ADC_Data_A = 0x00;
     IP_Handle->ADC_Data_B = 0x00;

     // STEP 2: In all modes IRQ must be enabled
     Xil_Out32(IP_Handle->ADC_BaseAddress + REG_IRQ_OFFSET, IRQ_ENABLE_MASK);

     // STEP 3: Load the ADC Clock divider will be the same in all modes
     IP_Handle->ControlRegister = IP_Handle->ClockDivider;
     IP_Handle->ControlRegister = IP_Handle->ControlRegister << CTRL_CLKDIV_LSB;
     Xil_Out32(IP_Handle->ADC_BaseAddress + REG_CTRL_OFFSET, IP_Handle->ControlRegister);

     // STEP 4: Clear GPIO output signals - This is the poor man's DMA signals
     XGpio_DiscreteClear(&AXI_GPIO_Handle, 2, 0x20);

     return(true);   

 } // END init_IMR_ADC_7476A_X2



/********************************************************************************************************
* @brief The IP functions in one of two modes: Single conversion or Multi conversion.  This function starts a 
* single conversion.  Note upon completion of the conversion an IRQ will be generated and the ISR associated
* with the "Poor Man's DMA" will be called.
*
* @author original: Hab Collector \n
*
* @note: IP must be initialized before use
* @note: See IP HDL notes for more information on the IP operation
* @note: Buffer memory must be allocated by the caller
* 
* @param IP_Handle: Pointer to the IMR ADC Dual ADC7476A IP handle  
* @param BufferData_A: Base address to store ADC channel A data
* @param BufferData_B: Base address to store ADC channel B data
*
* @return True if init OK
*
* STEP 1: Test for valid handle
* STEP 2: Set data pointers
* STEP 3: Load IP config register for single conversion
********************************************************************************************************/
bool IMR_ADC_7476A_X2_SingleConvert(Type_AXI_IMR_7476A_Handle *IP_Handle, uint16_t *BufferData_A, uint16_t *BufferData_B) 
{
    // STEP 1: Test for valid handle
    if (IP_Handle ==  NULL)
        return(false);

    // STEP 2: Set data pointers
    IP_Handle->ADC_Data_A = BufferData_A;
    IP_Handle->ADC_Data_B = BufferData_B;

    // STEP 3: Load IP config register for single conversion
    IP_Handle->ControlRegister = 0x00;
    uint32_t ClockDividerOffset = IP_Handle->ClockDivider;
    ClockDividerOffset = ClockDividerOffset << CTRL_CLKDIV_LSB;
    IP_Handle->ControlRegister = ClockDividerOffset | CTRL_EN_BIT_MASK | CTRL_START_BIT_MASK;
    Xil_Out32(IP_Handle->ADC_BaseAddress + REG_CTRL_OFFSET, IP_Handle->ControlRegister);

    return(true);

} // END IMR_ADC_7476A_X2_SingleConvert



/********************************************************************************************************
* @brief The IP functions in one of two modes: Single conversion or Multi conversion.  This function starts a 
* multi conversion.  Note upon completion of the conversion an IRQ will be generated and the ISR associated
* with the "Poor Man's DMA" will be called.
*
* @author original: Hab Collector \n
*
* @note: IP must be initialized before use
* @note: See IP HDL notes for more information on the IP operation
* @note: Buffer memory must be allocated by the caller
* 
* @param IP_Handle: Pointer to the IMR ADC Dual ADC7476A IP handle  
* @param BufferData_A: Base address to store ADC channel A data
* @param BufferData_B: Base address to store ADC channel B data
* @param TotalConversions: Total number of conversions to perform
*
* @return True if init OK
*
* STEP 1: Test for valid handle
* STEP 2: Set data pointers and handle members for multi conversion
* STEP 3: Load IP config register for muti conversions: Total Conversions, Clock Divider, Multi Bit, Start Bit and Enable Bit
********************************************************************************************************/
bool IMR_ADC_7476A_X2_MultiConvert(Type_AXI_IMR_7476A_Handle *IP_Handle, uint16_t *BufferData_A, uint16_t *BufferData_B, uint32_t TotalConversions) 
{
    // STEP 1: Test for valid handle
    if (IP_Handle ==  NULL)
        return(false);
    if (TotalConversions == 0)
        return(false);
    if (TotalConversions > pow(2, 1 +  (CTRL_CONT_CNT_MSB - CTRL_CONT_CNT_LSB)) - 1)
        return(false);

    // STEP 2: Set data pointers and handle members for multi conversion
    IP_Handle->ADC_Data_A = BufferData_A;
    IP_Handle->ADC_Data_B = BufferData_B;
    IP_Handle->TotalConversions = TotalConversions;
    IP_Handle->ConversionCount = 0;

    // STEP 3: Load IP config register for muti conversions: Total Conversions, Clock Divider, Multi Bit, Start Bit and Enable Bit
    IP_Handle->ControlRegister = 0x00;
    uint32_t TotalConversionsOffset = IP_Handle->TotalConversions;
    TotalConversionsOffset = TotalConversionsOffset << CTRL_CONT_CNT_LSB;
    uint32_t ClockDividerOffset = IP_Handle->ClockDivider;
    ClockDividerOffset = ClockDividerOffset << CTRL_CLKDIV_LSB;
    IP_Handle->ControlRegister = IP_Handle->ControlRegister | TotalConversionsOffset | ClockDividerOffset | CTRL_MULTI_BIT_MASK | CTRL_START_BIT_MASK | CTRL_EN_BIT_MASK;
    Xil_Out32(IP_Handle->ADC_BaseAddress + REG_CTRL_OFFSET, IP_Handle->ControlRegister);

    return(true);    

} // END IMR_ADC_7476A_X2_MultiConvert



/********************************************************************************************************
* @brief The IP functions in one of two modes: Single conversion or Multi conversion.  This is the IRQ clear
* function called by the ADC IP IRQ ISR.  It handles both modes of conversion (single and multi).  It is NOT 
* the "Poor Man's DMA" ISR.  This clear function triggers the "Poor Man's DMA" ISR (when all conversions are
* completed), which in turns signals to the application to process the ADC data.
*
* @author original: Hab Collector \n
*
* @note: IP must be initialized before use
* @note: See IP HDL notes for more information on the IP operation
* 
* @param IP_Handle: Pointer to the IMR ADC Dual ADC7476A IP handle  
*
* STEP 1: Service Multiple Conversions Interrupt
* STEP 2: Service Single Conversions Interrupt
********************************************************************************************************/
void IMR_ADC_7476A_X2_ClrIrq(Type_AXI_IMR_7476A_Handle *IP_Handle)
{
    // STEP 1: Service Multiple Conversions Interrupt
    if (IP_Handle->ControlRegister & CTRL_MULTI_BIT_MASK)
    {
        IP_Handle->ADC_Data_A[IP_Handle->ConversionCount] = (uint16_t)IMR_ADC_7476A_X2_GetDataAReg(IP_Handle);
        IP_Handle->ADC_Data_B[IP_Handle->ConversionCount] = (uint16_t)IMR_ADC_7476A_X2_GetDataBReg(IP_Handle);

        if (IP_Handle->ConversionCount >= IP_Handle->TotalConversions - 1)
        {
            IP_Handle->ControlRegister = 0x00; 
            Xil_Out32(IP_Handle->ADC_BaseAddress + REG_CTRL_OFFSET, IP_Handle->ControlRegister);
            Xil_Out32(IP_Handle->ADC_BaseAddress + REG_IRQ_OFFSET, IRQ_ENABLE_MASK | IRQ_CLR_MASK);
            Xil_Out32(IP_Handle->ADC_BaseAddress + REG_IRQ_OFFSET, IRQ_ENABLE_MASK);
            XGpio_DiscreteSet(&AXI_GPIO_Handle, 2, 0x20);
        }
        else
        {
            IP_Handle->ConversionCount = IP_Handle->ConversionCount + 1;
            Xil_Out32(IP_Handle->ADC_BaseAddress + REG_IRQ_OFFSET, IRQ_ENABLE_MASK | IRQ_CLR_MASK);
            Xil_Out32(IP_Handle->ADC_BaseAddress + REG_IRQ_OFFSET, IRQ_ENABLE_MASK);
        }
    }
    // STEP 2: Service Single Conversion Interrupt
    else
    {
        IP_Handle->ControlRegister = 0x00; 
        Xil_Out32(IP_Handle->ADC_BaseAddress + REG_CTRL_OFFSET, IP_Handle->ControlRegister);
        Xil_Out32(IP_Handle->ADC_BaseAddress + REG_IRQ_OFFSET, IRQ_ENABLE_MASK | IRQ_CLR_MASK);
        Xil_Out32(IP_Handle->ADC_BaseAddress + REG_IRQ_OFFSET, IRQ_ENABLE_MASK);
        *IP_Handle->ADC_Data_A = (uint16_t)IMR_ADC_7476A_X2_GetDataAReg(IP_Handle);
        *IP_Handle->ADC_Data_B = (uint16_t)IMR_ADC_7476A_X2_GetDataBReg(IP_Handle);
        XGpio_DiscreteSet(&AXI_GPIO_Handle, 2, 0x20);
    }

} // END IMR_ADC_7476A_X2_ClrIrq



// QUICK ACCESS GET FUNCTIONS
uint32_t IMR_ADC_7476A_X2_GetCtrlReg(Type_AXI_IMR_7476A_Handle *IP_Handle)
{
    return(Xil_In32(IP_Handle->ADC_BaseAddress + REG_CTRL_OFFSET));
}

uint32_t IMR_ADC_7476A_X2_GetStatusReg(Type_AXI_IMR_7476A_Handle *IP_Handle)
{
    return(Xil_In32(IP_Handle->ADC_BaseAddress + REG_STATUS_OFFSET));
}

uint32_t IMR_ADC_7476A_X2_GetIrqReg(Type_AXI_IMR_7476A_Handle *IP_Handle)
{
    return(Xil_In32(IP_Handle->ADC_BaseAddress + REG_IRQ_OFFSET));
}

uint32_t IMR_ADC_7476A_X2_GetDataAReg(Type_AXI_IMR_7476A_Handle *IP_Handle)
{
    return(Xil_In32(IP_Handle->ADC_BaseAddress + REG_DATA_A_OFFSET));
}

uint32_t IMR_ADC_7476A_X2_GetDataBReg(Type_AXI_IMR_7476A_Handle *IP_Handle)
{
    return(Xil_In32(IP_Handle->ADC_BaseAddress + REG_DATA_B_OFFSET));
}



