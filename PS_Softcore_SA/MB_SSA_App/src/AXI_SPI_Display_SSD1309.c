/******************************************************************************************************
 * @file            AXI_SPI_Display_SSD1309.c
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
 * @copyright       IMR Engineering, LLC
 ********************************************************************************************************/

#include "AXI_SPI_Display_SSD1309.h"
#include "Hab_Types.h"
#include "u8g2.h"
#include <string.h>

void *U8G2_UserPointer;

static void setUserPointer_U8G2(void *UserHandlePointer);
static void *getUserPointer_U8G2(void);
static uint8_t U8G2_WriteBytes_SPI(u8x8_t *U8X8, uint8_t Msg, uint8_t ArgInt, void *ArgPtr);
static uint8_t U8G2_GPIO_DelayControl(u8x8_t *U8X8, uint8_t Msg, uint8_t ArgInt, void *ArgPtr);
static void displaySegmented_SPI_Transfer(Type_Display_SSD1309 *SSD1309, uint8_t *DataPtr, uint32_t DataLength);


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
 static void *getUserPointer_U8G2(void)
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
static void displaySegmented_SPI_Transfer(Type_Display_SSD1309 *SSD1309, uint8_t *DataPtr, uint32_t DataLength)
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
    


/********************************************************************************************************
* @brief Simple display test - clear the display and show Hello Hab in top left corner
*
* @author original: Hab Collector \n
*
* @note: Display must be init before use
* 
* @param   Display_SSD1309      Pointer to display handle
********************************************************************************************************/
void displaySimpleTest(Type_Display_SSD1309 *Display_SSD1309)
{
    u8g2_ClearBuffer(Display_SSD1309->U8G2_Handle);
    u8g2_SetFont(Display_SSD1309->U8G2_Handle, u8g2_font_5x8_tr);
    u8g2_DrawStr(Display_SSD1309->U8G2_Handle, 0, 10, "Hello Hab!");
    u8g2_SendBuffer(Display_SSD1309->U8G2_Handle);
}



void displayTest_2(void)
{
    Type_Display_SSD1309 *SSD1309 = (Type_Display_SSD1309 *)getUserPointer_U8G2();

    static uint8_t Y = 20;
    u8g2_DrawStr(SSD1309->U8G2_Handle, 10, Y, "Hello Hab Again!");
    u8g2_SendBuffer(SSD1309->U8G2_Handle);
    Y += 10;
}



void drawSpectrumMock(Type_Display_SSD1309 *Display_SSD1309)
{
    // USER-ADJUSTABLE LOCAL CONSTANTS (self-contained)
    uint8_t NumBars           = 16;     // Number of frequency columns
    uint8_t SegmentsPerBar    = 10;     // Vertical resolution
    uint8_t SegmentHeight     = 2;      // Height of each vertical block (pixels)
    uint8_t SegmentVSpace     = 1;      // Space between vertical blocks
    uint8_t BarWidth          = 4;      // Width of each bar (pixels)
    uint8_t BarHSpace         = 2;      // Horizontal spacing between bars
    uint8_t BaselineY         = 60;     // Vertical baseline position (SSD1309 is 64px tall)

    // Pull U8G2 handle
    u8g2_t *U = Display_SSD1309->U8G2_Handle;

    // Clear screen
    u8g2_ClearBuffer(U);

    // Draw each bar
    for (uint8_t BarIndex = 0; BarIndex < NumBars; BarIndex++)
    {
        // Random height: 0–SegmentsPerBar
        uint8_t Value = rand() % (SegmentsPerBar + 1);

        // Compute X position of this bar
        uint8_t X_Position = BarIndex * (BarWidth + BarHSpace);

        // Draw vertical segments bottom → top
        for (uint8_t SegmentIndex = 0; SegmentIndex < Value; SegmentIndex++)
        {
            uint8_t Y_Top = BaselineY
                            - (SegmentIndex * (SegmentHeight + SegmentVSpace))
                            - SegmentHeight;

            u8g2_DrawBox(U,
                         X_Position,
                         Y_Top,
                         BarWidth,
                         SegmentHeight);
        }
    }

    // Push to display
    u8g2_SendBuffer(U);
}


// void drawSpectrumMock(Type_Display_SSD1309 *Display_SSD1309)
// {
//     // LOCAL CONSTANTS (self-contained)
//     uint8_t NumBars           = 16;     // Number of frequency columns
//     uint8_t SegmentsPerBar    = 10;     // Vertical resolution
//     uint8_t SegmentHeight     = 2;      // Height of each segment (pixels)
//     uint8_t SegmentVSpace     = 1;      // Spacing between segments
//     uint8_t BarWidth          = 4;      // Width of each bar
//     uint8_t BarHSpace         = 2;      // Horizontal spacing between bars
//     uint8_t BaselineY         = 60;     // Vertical baseline (SSD1309 = 64px)

//     uint8_t DisplayWidth      = 128;    // SSD1309 width

//     // Centering calculation:
//     uint8_t TotalBarWidth     = NumBars * BarWidth;
//     uint8_t TotalSpacing      = (NumBars - 1) * BarHSpace;
//     uint8_t SpectrumWidth     = TotalBarWidth + TotalSpacing;
//     uint8_t X_Offset          = (DisplayWidth - SpectrumWidth) / 2;  

//     // Get U8G2 handle
//     u8g2_t *U = Display_SSD1309->U8G2_Handle;

//     u8g2_ClearBuffer(U);

//     // Draw all bars
//     for (uint8_t BarIndex = 0; BarIndex < NumBars; BarIndex++)
//     {
//         // Random bar height: 0–SegmentsPerBar
//         uint8_t Value = rand() % (SegmentsPerBar + 1);

//         // Compute X of this bar (now centered)
//         uint8_t X_Position = X_Offset + BarIndex * (BarWidth + BarHSpace);

//         // Draw vertical segments bottom → top
//         for (uint8_t SegmentIndex = 0; SegmentIndex < Value; SegmentIndex++)
//         {
//             uint8_t Y_Top = BaselineY
//                             - (SegmentIndex * (SegmentHeight + SegmentVSpace))
//                             - SegmentHeight;

//             u8g2_DrawBox(U,
//                          X_Position,
//                          Y_Top,
//                          BarWidth,
//                          SegmentHeight);
//         }

//         // ---------------------------
//         // Draw PEAK marker (random for now)
//         // ---------------------------
//         //
//         // Peak value: somewhere above current bar.
//         // 0 = baseline, higher numbers = more height.
//         uint8_t PeakValue = rand() % (SegmentsPerBar + 1);

//         uint8_t PeakY = BaselineY
//                         - (PeakValue * (SegmentHeight + SegmentVSpace))
//                         - (SegmentHeight + SegmentVSpace);

//         if (PeakY > 0)
//         {
//             u8g2_DrawBox(U,
//                          X_Position,
//                          PeakY,
//                          BarWidth,
//                          2);   // Peak marker thickness
//         }
//     }

//     // Push to display
//     u8g2_SendBuffer(U);
// }



/********************************************************************************************************
* @brief Direct SPI diagnostic test for SSD1309 (128x64).  Writes a deterministic “page band” pattern by
* bypassing u8g2 and programming the controller directly.  The display is refreshed in 8 pages (Page 0–7),
* where each page is 8 pixels tall by 128 pixels wide.  This function writes one full 128-byte row per
* page, alternating black and white pages to create 8 horizontal 8-pixel bands across the full 64-pixel
* height. This function writes directly to the display - it does not use the U8G2 library and can be used
* as a test to see if the display is working correctly.  
*
* @details Page and column positioning on SSD1309 is done via COMMAND writes before each page’s DATA
* stream.  SSD1309 column addressing is split into two separate 4-bit commands:
*   - Set lower column nibble:  0x00–0x0F  (sets bits [3:0])
*   - Set upper column nibble:  0x10–0x1F  (sets bits [7:4])
* The effective column start is:
*   ColumnStart = ((UpperNibble & 0x0F) << 4) | (LowerNibble & 0x0F)
* Therefore, to start at column 0 you must send BOTH:
*   - 0x00  (lower nibble = 0)
*   - 0x10  (upper nibble = 0)
* This is not “column 16.”  It is “set upper column bits to zero.”
*
* @author original: Hab Collector \n
*
* @note: Requires the display to be initialized and out of power-save prior to use (for example via
*        u8g2_InitDisplay() and u8g2_SetPowerSave(..., 0)).  Uses the provided CS and C/D GPIO control
*        function pointers and sends bytes using displaySegmented_SPI_Transfer().
*
* @param   SSD1309      Pointer to display handle and hardware interface function pointers.
*
* STEP 1: Assert display chip select (CS) for the duration of the transaction.
* STEP 2: For each page 0–7:
*         - Issue COMMANDS to select the page and reset the column address to 0.
*         - Issue DATA bytes (128) to fill the page with either 0x00 (black) or 0xFF (white).
* STEP 3: Deassert display chip select (CS).
********************************************************************************************************/
void displayDirectTest(Type_Display_SSD1309 *SSD1309)
{
    uint8_t CommandBuffer[3] = {0};
    uint8_t PageFill[128] = {0};

    // STEP 1: Assert chip select for the entire transaction
    SSD1309->display_CS(CS_ENABLE);

    // STEP 2: For each page (8 pages for 64-pixel height), set page+column, then write 128 bytes
    for (uint8_t Page = 0; Page < 8; Page++)
    {
        // STEP 2.1: Set page address (0xB0..0xB7)
        CommandBuffer[0] = (uint8_t)(0xB0u | Page);

        // STEP 2.2: Set column address to 0 (lower nibble then upper nibble)
        CommandBuffer[1] = 0x00u;  // Column low nibble = 0
        CommandBuffer[2] = 0x10u;  // Column high nibble = 0

        // STEP 2.3: Send the page and column command
        SSD1309->displayCommandData(DISPLAY_COMMAND);
        displaySegmented_SPI_Transfer(SSD1309, CommandBuffer, (uint32_t)sizeof(CommandBuffer));

        // STEP 2.4: Fill the page with alternating pattern (even pages white, odd pages black)
        uint8_t FillByte = (uint8_t)((Page & 0x01u) ? 0x00 : 0xFF);
        for (uint16_t Index = 0; Index < 128; Index++)
        {
            PageFill[Index] = FillByte;
        }

        // STEP 2.5: Send the page data
        SSD1309->displayCommandData(DISPLAY_DATA);
        displaySegmented_SPI_Transfer(SSD1309, PageFill, (uint32_t)sizeof(PageFill));
    }

    // STEP 3: Deassert chip select
    SSD1309->display_CS(CS_DISABLE);

} // END OF displayDirectTest



void drawStaticAudioHeader(Type_Display_SSD1309 *Display_SSD1309, char *Heading, char *FileName, char *AudioAction, uint32_t TimeInSeconds)
{
    u8g2_t *U = Display_SSD1309->U8G2_Handle;

    char TimeString[6] = {0};  // "mm:ss"
    uint32_t Minutes = TimeInSeconds / 60;
    uint32_t Seconds = TimeInSeconds % 60;

    // STEP 1: Format time string
    snprintf(TimeString, sizeof(TimeString), "%02lu:%02lu",
             (unsigned long)Minutes,
             (unsigned long)Seconds);

    // STEP 2: Clear buffer
    u8g2_ClearBuffer(U);

    // STEP 3: Draw centered title (6x10)
    u8g2_SetFont(U, u8g2_font_6x10_tr);

    const char *Title = Heading;
    uint16_t TitleWidth = u8g2_GetStrWidth(U, Title);
    uint8_t X_Title = (uint8_t)((128 - TitleWidth) / 2);

    u8g2_DrawStr(U, X_Title, 10, Title);   // Baseline at Y=10

    // STEP 4: Draw left-justified file info (5x8)
    u8g2_SetFont(U, u8g2_font_5x8_tr);

    u8g2_DrawStr(U, 0, 19, FileName);      // Line 1
    u8g2_DrawStr(U, 0, 28, AudioAction);    // Line 2
    u8g2_DrawStr(U, 0, 37, TimeString);    // Line 3

    // STEP 5: Push to display
    u8g2_SendBuffer(U);
}

void drawStaticSignalHeader(Type_Display_SSD1309 *Display_SSD1309, char *Heading)
{
        // STEP 2: Clear buffer
    u8g2_ClearBuffer(Display_SSD1309->U8G2_Handle);

    // STEP 3: Draw centered title (6x10)
    u8g2_SetFont(Display_SSD1309->U8G2_Handle, u8g2_font_6x10_tr);

    const char *Title = Heading;
    uint16_t TitleWidth = u8g2_GetStrWidth(Display_SSD1309->U8G2_Handle, Title);
    uint8_t X_Title = (uint8_t)((128 - TitleWidth) / 2);

    // STEP 5: Push to display
    u8g2_SendBuffer(Display_SSD1309->U8G2_Handle);
}