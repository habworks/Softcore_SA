/******************************************************************************************************
 * @file            SSD1309_Driver.c
 * @brief           A collection of functions relevant to the Display SSD1309 128x64 pixels
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
 *                  The display library can be found here: https://github.com/olikraus/u8g2
 *                  Some setup help - very minor mostly for hobbist not so much for custom: https://github.com/olikraus/u8g2/wiki/setup_tutorial
 *                  The supported display setups can be found here: ...\src\U8G2\csrc\  (There are several u8g2_Setup_ssd1309_128x64_noname* - try to you find one that works )
 *                  You must add the U8G2\csrc and all of the assoicated files to your project
 *                  This implementation worked for this particual SSD1309 2.42" OLED display - another SSD1309 maybe different
 *                  I purchased the display from Amazon here: https://www.amazon.com/dp/B0CFF2QW5V?ref_=ppx_hzsearch_conn_dt_b_fed_asin_title_1&th=1
 *
 ****************** VERY IMPORTANT:
 ****************** VERY IMPORTANT:
 ****************** VERY IMPORTANT:
 ****************** VERY IMPORTANT:
 *                  The file SoftCore_SA_Used_Fonts.c was created from u8g2_fonts.c as u8g2_fonts.c contants every font supported by the library
 *                  that complied list of fonts is far too large to fit this applicaiton.  SoftCore_SA_Used_Fonts.c is just a copy and paste of the used fonts
 *                  For similar reasons u8x8_fonts.c was also deleted from the library - none of those fonts are used here
 *                  There are other drivers that can be ommitted from the library and the library is a catch all for everything - but the juice is not worth the squeeze
 *                  That is unless you applicaiton gets really large and you need to reduce its size
 *
 * @copyright       IMR Engineering, LLC
 ********************************************************************************************************/

#include "SSD1309_Driver.h"
#include "Hab_Types.h"
#include "u8g2.h"
#include <string.h>

void *U8G2_UserPointer;

static void setUserPointer_U8G2(void *UserHandlePointer);
static uint8_t U8G2_WriteBytes_SPI(u8x8_t *U8X8, uint8_t Msg, uint8_t ArgInt, void *ArgPtr);
static uint8_t U8G2_GPIO_DelayControl(u8x8_t *U8X8, uint8_t Msg, uint8_t ArgInt, void *ArgPtr);


/********************************************************************************************************
* @brief Init of SSD1309 Display Handle
*
* @author original: Hab Collector \n
*
* @note: Display must be init before use
* 
* @param   Display_SSD1309              Pointer to display handle structure
* @param   QSPI_Handle                  Pointer to initialized XSpi instance
* @param   ChipSelect_N                 Quad SPI Chip Select # - starts at 1 (used when wired as the display chip select)
* @param   FIFO_Depth                   Depth of FIFO - SPI does not transfer glitches well when sending beyond its FIFO Depth 
* @param   displayResetRunFunction      Function pointer for hardware reset/run control (GPIO control reset = 0, run = 1)
* @param   displayCommandDataFunction   Function pointer for Command / Data control (GPIO Command = 0, Data = 1)
* @param   displayTxRxFunction          Function pointer for SPI data transfer only (Half duplex)
* @param   displaySleep_msFunction      Function pointer used in delay sleep intervals in milliseconds
* @param   displaySleep_10usFunction    Function pointer used in delay sleep intervals in 10s of microseconds
* @param   U8G2_Object                  Pointer to the u8g2 display object
*
* @return True if init OK
*
* STEP 1: Basic test
* STEP 2: Load struct members
* STEP 3: Reset the display
* STEP 4: Init the display driver
********************************************************************************************************/
bool init_Display_SSD1309(Type_Display_SSD1309 *Display_SSD1309, XSpi *QSPI_Handle, uint8_t ChipSelect_N, uint16_t FIFO_Depth, displayResetRunFunctionPtr displayResetRunFunction, displayCommandDataFunctionPtr displayCommandDataFunction, displayTxRxFunctionPtr displayTxRxFunction, displayChipSelectFunctionPtr displayChipSelectFunction, displaySleep_msFunctionPtr displaySleep_msFunction, displaySleep_10usFunctionPtr displaySleep_10usFunction, u8g2_t *U8G2_Object)
{
    // STEP 1: Basic test
    if ((Display_SSD1309 == NULL) || (QSPI_Handle == NULL) || (displayResetRunFunction == NULL) || (displayCommandDataFunction == NULL) || (ChipSelect_N == 0))
        return(false);
    
    // STEP 2: Load struct members
    Display_SSD1309->SPI_Handle = QSPI_Handle;
    Display_SSD1309->ChipSelectBitMask = ChipSelect_N;
    Display_SSD1309->FIFO_BufferDepth = FIFO_Depth;
    Display_SSD1309->displayResetRun = displayResetRunFunction;
    Display_SSD1309->displayCommandData = displayCommandDataFunction;
    Display_SSD1309->displayTxRx = displayTxRxFunction;
    Display_SSD1309->display_CS = displayChipSelectFunction;
    Display_SSD1309->displaySleep_ms = displaySleep_msFunction;
    Display_SSD1309->displaySleep_10us = displaySleep_10usFunction;
    Display_SSD1309->U8G2_Handle = U8G2_Object;

    // STEP 3: Reset the display
    Display_SSD1309->displayResetRun(DISPLAY_RESET);
    Display_SSD1309->displaySleep_ms(10);
    Display_SSD1309->displayResetRun(DISPLAY_RUN);

    // STEP 4: Init the display driver
    setUserPointer_U8G2(Display_SSD1309);
    u8g2_Setup_ssd1309_128x64_noname0_f(Display_SSD1309->U8G2_Handle, U8G2_R0, U8G2_WriteBytes_SPI, U8G2_GPIO_DelayControl);
    // Critical for SSD1309:
    u8g2_InitDisplay(Display_SSD1309->U8G2_Handle);
    u8g2_SetPowerSave(Display_SSD1309->U8G2_Handle, 0);
    u8g2_SetContrast(Display_SSD1309->U8G2_Handle, 64);
    u8g2_SetFlipMode(Display_SSD1309->U8G2_Handle, 0);

    return(true);

} // END OF init_Display_SSD1309



/********************************************************************************************************
* @brief Load the user pointer for recall later
*
* @author original: Hab Collector \n
*
* @note: Display must be init before use
* 
* @param   UserHandlePointer    Pointer to the display handle
********************************************************************************************************/
 static void setUserPointer_U8G2(void *UserHandlePointer)
 {  
     U8G2_UserPointer = UserHandlePointer;
 }



/********************************************************************************************************
* @brief Return the user handle
*
* @author original: Hab Collector \n
*
* @note: Display must be init before use
* 
* @return User handle returned as void pointer - calling functiion must type cast to display handle
********************************************************************************************************/
 void *getUserPointer_U8G2(void)
 {
     return(U8G2_UserPointer);
 }



/********************************************************************************************************
* @brief Required by the U8G2 library init function.  Called by the U8G2 various library functions  to facilitate 
* command and data transfers to the display.
*
* @author original: Hab Collector \n
*
* @note: Display must be init before use
* 
* @param   U8X8         U8G2 library object - not used but must be included per the function definition
* @param   Msg          The present action desired by the U8G2 library
* @param   ArgInt       Msg dependent value indicates the status of control or datalenght of ArgPtr
* @param   ArgPtr       Msg dependent used in case U8X8_MSG_BYTE_SEND this is the data buffer to send  
*
* @return Must always return 1
*
* STEP 1: Get the display handle
* STEP 2: Perform Msg action
********************************************************************************************************/
static uint8_t U8G2_WriteBytes_SPI(u8x8_t *U8X8, uint8_t Msg, uint8_t ArgInt, void *ArgPtr)
{
    NOT_USED(U8X8);

    // STEP 1: Get the display handle
    Type_Display_SSD1309 *SSD1309 = (Type_Display_SSD1309 *)getUserPointer_U8G2();

    // STEP 2: Perform Msg action
    switch (Msg)
    {
        case U8X8_MSG_BYTE_INIT:
        {
            return(1);
        }
        break;

        case U8X8_MSG_BYTE_START_TRANSFER:
        {
            SSD1309->display_CS(CS_ENABLE);
            return(1);
        }
        break;

        case U8X8_MSG_BYTE_END_TRANSFER:
        {
            SSD1309->display_CS(CS_DISABLE);
            return(1);
        }
        break;

        case U8X8_MSG_BYTE_SET_DC:
        {
            if (ArgInt)
                SSD1309->displayCommandData(DISPLAY_DATA);     // D/C = 1 → data
            else
                SSD1309->displayCommandData(DISPLAY_COMMAND);  // D/C = 0 → command
            return(1);
        }
        break;

        case U8X8_MSG_BYTE_SEND:
        {
            // SSD1309->displayTxRx(SSD1309->SPI_Handle, SSD1309->ChipSelectBitMask, (uint8_t *)ArgPtr, NULL, (uint32_t)ArgInt);
            displaySegmented_SPI_Transfer(SSD1309, (uint8_t *)ArgPtr, (uint32_t)ArgInt);
            return(1);
        }
        break;

        default:
        {
            return(1);
        }
    }

} // END OF U8G2_WriteBytes_SPI



/********************************************************************************************************
* @brief Required by the U8G2 library init function.  Called by the U8G2 various library functions to facilitate 
* command and sleep functions
*
* @author original: Hab Collector \n
*
* @note: Display must be init before use
* @note: This implementation uses a seperate GPIO control for CS
* 
* @param   U8X8         U8G2 library object - not used but must be included per the function definition
* @param   Msg          The present action desired by the U8G2 library
* @param   ArgInt       Msg dependent value indicates the status of control or datalenght of ArgPtr
* @param   ArgPtr       Msg dependent used in case U8X8_MSG_BYTE_SEND this is the data buffer to send  
*
* @return Must always return 1
*
* STEP 1: Get the display handle
* STEP 2: Perform Msg action
********************************************************************************************************/
static uint8_t U8G2_GPIO_DelayControl(u8x8_t *U8X8, uint8_t Msg, uint8_t ArgInt, void *ArgPtr)
{
    NOT_USED(U8X8);

    // STEP 1: Get the display handle
    Type_Display_SSD1309 *SSD1309 = (Type_Display_SSD1309 *)getUserPointer_U8G2(); //u8x8_GetUserPtr(U8X8);

    // STEP 2: Perform Msg action
    switch (Msg)
    {
        case U8X8_MSG_GPIO_DC:
        {
            if (ArgInt) 
                SSD1309->displayCommandData(DISPLAY_DATA); 
            else 
                SSD1309->displayCommandData(DISPLAY_COMMAND); 
            return(1);
        }
        break;

        case U8X8_MSG_GPIO_RESET:
        {
            if (ArgInt) 
                SSD1309->displayResetRun(DISPLAY_RUN); 
            else 
                SSD1309->displayResetRun(DISPLAY_RESET); 
            return(1);
        }
        break;

        case U8X8_MSG_GPIO_CS:
        {
            // If you ever drive CS via GPIO - even when driving CS by external GPIO (see U8X8_MSG_BYTE_START_TRANSFER) I did not find this to be necessary
            return(1);
        }
        break;

        case U8X8_MSG_DELAY_MILLI:
        {
            SSD1309->displaySleep_ms(ArgInt);
            return(1);
        }
        break;

        case U8X8_MSG_DELAY_10MICRO:
        {
            SSD1309->displaySleep_10us(ArgInt);
            return(1);
        }
        break;

        case U8X8_MSG_DELAY_100NANO:
        default:
        {
            return(1);
        }
    }

} // END OF U8G2_GPIO_DelayControl



/********************************************************************************************************
* @brief The SPI glitches when transmitting data beyond the FIFO depth length.  When transmiting data beyond
* the FIFO buffer depth, break up the data into FIFO depth segments for transmission.  
*
* @author original: Hab Collector \n
*
* @note: Display must be init before use
* @note: This function has only been tested with the CS was enabled contineously - ie CS is not used from the
* QSPI IP, but rather as a seperate GPIO - that is not to say it does not work - just not tested tha way
* 
* @param   SSD1309      Pointer to the  handle
* @param   DataPtr      Pointer to the data to transmit
* @param   DataLength   Lenght of data in bytes to transmit
*
* STEP 1: If data lenght can fit with in the FIFO buffer no need to segment
* STEP 2: Data lenght beyond FIFO buffer - segmented trasmission
********************************************************************************************************/
void displaySegmented_SPI_Transfer(Type_Display_SSD1309 *SSD1309, uint8_t *DataPtr, uint32_t DataLength)
{
    // STEP 1: If data lenght can fit with in the FIFO buffer no need to segment
    if (DataLength <= SSD1309->FIFO_BufferDepth)
    {
        SSD1309->displayTxRx(SSD1309->SPI_Handle, SSD1309->ChipSelectBitMask, DataPtr, NULL, DataLength);
        return;
    }

    // STEP 2: Data lenght beyond FIFO buffer - segmented trasmission
    uint8_t DataBuffer[SSD1309->FIFO_BufferDepth];
    uint8_t DataOffset = 0;
    uint8_t BytesTransmitted = 0;
    uint8_t BytesRemaining;
    uint8_t BytesToTransmit;
    do 
    {
        // Determine Bytes remaining to send
        BytesRemaining = DataLength - BytesTransmitted;
        if (BytesRemaining > SSD1309->FIFO_BufferDepth)
            BytesToTransmit = SSD1309->FIFO_BufferDepth;
        else
            BytesToTransmit = BytesRemaining;
        // Copy upto the FIFP depth bytes to transmit and transmit
        memcpy(DataBuffer, &DataPtr[DataOffset], BytesToTransmit);
        SSD1309->displayTxRx(SSD1309->SPI_Handle, SSD1309->ChipSelectBitMask, DataBuffer, NULL, BytesToTransmit);
        // Update the Buffer poniter, bytes transmitted and test if done
        DataOffset += BytesToTransmit;
        BytesTransmitted += BytesToTransmit;    
    }while (BytesTransmitted < DataLength);

} // END OF displaySegmented_SPI_Transfer
    


