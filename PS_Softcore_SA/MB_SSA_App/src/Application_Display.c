/******************************************************************************************************
 * @file            Application_Display.c
 * @brief           Various display modes and screens used by the appliction - relies on SSD1309_Driver.h
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

#include "Application_Display.h"
#include <stdlib.h>
#include <stdio.h>


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

} // END OF displaySimpleTest



/********************************************************************************************************
* @brief Displays a mock audio specturm.  This will form the structure for the real thing and will suffice
* for testing until I am at that point.
*
* @author original: Hab Collector \n
*
* @note: Display must be init before use
* 
* @param Display_SSD1309: Pointer to display handle
* @param ClearOnly: Only clears the sprecrturm display area - no specturm is displayed
*
* STEP 1: Clear the spectrum area of the display by drawing a black box
* STEP 2: Draw each spectral bar
* STEP 3: Push to display
********************************************************************************************************/
void drawSpectrumMock(Type_Display_SSD1309 *Display_SSD1309, bool ClearOnly)
{
    // USER-ADJUSTABLE LOCAL CONSTANTS (self-contained)
    uint8_t NumBars           = 16;     // Number of frequency columns
    uint8_t SegmentsPerBar    = 8;      // Vertical resolution
    uint8_t SegmentHeight     = 2;      // Height of each vertical block (pixels)
    uint8_t SegmentVSpace     = 1;      // Space between vertical blocks
    uint8_t BarWidth          = 4;      // Width of each bar (pixels)
    uint8_t BarHSpace         = 2;      // Horizontal spacing between bars
    uint8_t BaselineY         = 63;     // Vertical baseline position (SSD1309 is 64px tall)

    // STEP 1: Clear the spectrum area of the display by drawing a black box
    uint8_t Spectrum_Y_Start = 40;
    uint8_t Spectrum_Height = (uint8_t)(DISPLAY_HEIGH_PIXEL - Spectrum_Y_Start);
    u8g2_SetDrawColor(Display_SSD1309->U8G2_Handle, 0);
    u8g2_DrawBox(Display_SSD1309->U8G2_Handle, 0, Spectrum_Y_Start, DISPLAY_WIDTH_PIXEL, Spectrum_Height);
    u8g2_SetDrawColor(Display_SSD1309->U8G2_Handle, 1);

    // STEP 2: Draw each spectral bar
    if (!ClearOnly)
    {
        for (uint8_t BarIndex = 0; BarIndex < NumBars; BarIndex++)
        {
            // Random height: 0–SegmentsPerBar
            uint8_t Value = rand() % (SegmentsPerBar + 1);

            // Compute X position of this bar
            uint8_t X_Position = BarIndex * (BarWidth + BarHSpace);

            // Draw vertical segments bottom → top
            for (uint8_t SegmentIndex = 0; SegmentIndex < Value; SegmentIndex++)
            {
                uint8_t Y_Top = BaselineY - (SegmentIndex * (SegmentHeight + SegmentVSpace)) - SegmentHeight;

                u8g2_DrawBox(Display_SSD1309->U8G2_Handle, X_Position, Y_Top, BarWidth, SegmentHeight);
            }
        }
    }

    // STEP 3: Push to display
    u8g2_SendBuffer(Display_SSD1309->U8G2_Handle);

} // END OF drawSpectrumMock



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
* @param SSD1309: Pointer to display handle and hardware interface function pointers.
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
        for (uint16_t Index = 0; Index < DISPLAY_WIDTH_PIXEL; Index++)
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



/********************************************************************************************************
* @brief Displays the welcome splash screen
*
* @author original: Hab Collector \n
*
* @note: Display must be init before use
* 
* @param Display_SSD1309: Pointer to display handle
* @param FW_Major: Firmware Major Revision
* @param FW_Minor: Firmware Minor Revision
* @param FW_Test: Firmware Test Revision
* @param HW_Revision: Hardware Revision
*
* STEP 1: Clear display buffer
* STEP 2: Draw large IMR Engineering and Ideas Made Real
* STEP 3: Format revision strings
* STEP 4: Draw lower informational text (5x8)
* STEP 5: Push to display
********************************************************************************************************/
void displayWelcomeScreen(Type_Display_SSD1309 *Display_SSD1309, uint8_t FW_Major, uint8_t FW_Minor, uint8_t FW_Test, uint8_t HW_Rev)
{
    char FW_String[20] = {0};
    char HW_String[16] = {0};

    // STEP 1: Clear display buffer
    u8g2_ClearBuffer(Display_SSD1309->U8G2_Handle);

    // STEP 2: Draw large IMR Engineering
    u8g2_SetFont(Display_SSD1309->U8G2_Handle, u8g2_font_6x12_tr);
    // const char *Title = "IMR Engineering";
    // uint16_t TitleWidth = u8g2_GetStrWidth(Display_SSD1309->U8G2_Handle, Title);
    // uint8_t X_Title = (uint8_t)((DISPLAY_WIDTH_PIXEL - TitleWidth) / 2);
    u8g2_DrawStr(Display_SSD1309->U8G2_Handle, 0, 13, "IMR Engineering");
    // Draw Ideas Made Real
    u8g2_SetFont(Display_SSD1309->U8G2_Handle, u8g2_font_6x10_tr);
    u8g2_DrawStr(Display_SSD1309->U8G2_Handle, 0,24, "Ideas Made Real");

    // STEP 3: Format revision strings
    snprintf(FW_String, sizeof(FW_String),"FW REV: %02u.%02u.%02u", FW_Major, FW_Minor, FW_Test);
    snprintf(HW_String, sizeof(HW_String), "HW REV: %02u", HW_Rev);

    // STEP 4: Draw lower informational text (5x8)
    u8g2_SetFont(Display_SSD1309->U8G2_Handle, u8g2_font_5x8_tr);
    u8g2_DrawStr(Display_SSD1309->U8G2_Handle, 0,44, "SoftCore Spectrum Analyzer");
    u8g2_DrawStr(Display_SSD1309->U8G2_Handle, 0, 52, FW_String);
    u8g2_DrawStr(Display_SSD1309->U8G2_Handle, 0, 60, HW_String);

    // STEP 5: Push to display
    u8g2_SendBuffer(Display_SSD1309->U8G2_Handle);

} // END OF displayWelcomeScreen



/********************************************************************************************************
* @brief Push the display buffer contents to the display
*
* @author original: Hab Collector \n
*
* @note: Display must be init before use
* 
* @param Display_SSD1309: Pointer to display handle
********************************************************************************************************/
void displayUpdateBuffer(Type_Display_SSD1309 *Display_SSD1309)
{
    u8g2_SendBuffer(Display_SSD1309->U8G2_Handle);
}



/********************************************************************************************************
* @brief Displays the static Audio Header.  This is the base information present when the user is in Audio
* SA mode: Title, WAV file name, Stop (play action) and time (should be 00:00)
*
* @author original: Hab Collector \n
*
* @note: Display must be init before use
* 
* @param Display_SSD1309: Pointer to display handle
*
* STEP 1: Format time string
* STEP 2: Clear the display buffer entirely 
* STEP 3: Draw centered title (6x10)
* STEP 4: Draw left-justified file info (5x8)
* STEP 5: Push to display
********************************************************************************************************/
void drawStaticHeaderAudio(Type_Display_SSD1309 *Display_SSD1309, char *Heading, char *FileName, char *AudioAction, uint32_t TimeInSeconds)
{
    // STEP 1: Format time string
    char TimeString[6] = {0};  // "mm:ss"
    uint8_t Minutes = (uint8_t)(TimeInSeconds / 60);
    uint8_t  Seconds = (uint8_t)(TimeInSeconds % 60);
    snprintf(TimeString, sizeof(TimeString), "%02u:%02u", (uint8_t)Minutes, (uint8_t)Seconds);

    // STEP 2: Clear the display buffer entirely 
    u8g2_ClearBuffer(Display_SSD1309->U8G2_Handle);

    // STEP 3: Draw centered title (6x10)
    u8g2_SetFont(Display_SSD1309->U8G2_Handle, u8g2_font_6x10_tr);
    const char *Title = Heading;
    uint16_t TitleWidth = u8g2_GetStrWidth(Display_SSD1309->U8G2_Handle, Title);
    uint8_t X_Title = (uint8_t)((DISPLAY_WIDTH_PIXEL - TitleWidth) / 2);
    u8g2_DrawStr(Display_SSD1309->U8G2_Handle, X_Title, 10, Title);   // Baseline at Y=10

    // STEP 4: Draw left-justified file info (5x8)
    u8g2_SetFont(Display_SSD1309->U8G2_Handle, u8g2_font_5x8_tr);
    u8g2_DrawStr(Display_SSD1309->U8G2_Handle, 0, 19, FileName);      // Line 1
    u8g2_DrawStr(Display_SSD1309->U8G2_Handle, 0, 28, AudioAction);    // Line 2
    u8g2_DrawStr(Display_SSD1309->U8G2_Handle, 0, 37, TimeString);    // Line 3

    // STEP 5: Push to display
    u8g2_SendBuffer(Display_SSD1309->U8G2_Handle);

} // END OF drawStaticHeaderAudio



/********************************************************************************************************
* @brief: Erases and updates the audio playback time in the format of 00:00 mm:ss
*
* @author original: Hab Collector \n
*
* @note: Display must be init before use
* @note: This function does not in of itself refresh the screen.  It must be called prior to a function that
* does (like: drawSpectrumMock, drawSpectrum, or displayUpdateBuffer)
* 
* @param Display_SSD1309: Pointer to display handle
* @param TimeInSeconds: Playback audio time in seconds elapsed
*
* STEP 1: Format time string
* STEP 2: Clear only time area (convert baseline to top-left)
* STEP 3: Print the updated time to screen - note: requires another function call that will push to display
********************************************************************************************************/
void updateAudioDisplayPlaybackTime(Type_Display_SSD1309 *Display_SSD1309, uint32_t TimeInSeconds)
{
    // STEP 1: Format time string 
    char TimeString[6] = {0};
    uint8_t Minutes = (uint8_t)(TimeInSeconds / 60);
    uint8_t  Seconds = (uint8_t)(TimeInSeconds % 60);
    snprintf(TimeString, sizeof(TimeString), "%02u:%02u", (uint8_t)Minutes, (uint8_t)Seconds);

    // STEP 2: Clear only time area (convert baseline to top-left)
    uint8_t Y_Top = (uint8_t)(TIME_BASELINE_Y - TIME_FONT_H);
    u8g2_SetDrawColor(Display_SSD1309->U8G2_Handle, 0);
    u8g2_DrawBox(Display_SSD1309->U8G2_Handle, TIME_X, Y_Top, TIME_BOX_W, TIME_BOX_H);
    
    // STEP 3: Print the updated time to screen - note: requires another function call that will push to display
    u8g2_SetDrawColor(Display_SSD1309->U8G2_Handle, 1);
    u8g2_SetFont(Display_SSD1309->U8G2_Handle, u8g2_font_5x8_tr);
    u8g2_DrawStr(Display_SSD1309->U8G2_Handle, TIME_X, TIME_BASELINE_Y, TimeString);

} // END OF updateAudioDisplayPlaybackTime



/********************************************************************************************************
* @brief: Erases and updates the audio playback action (Play, Pause, Stop, Error)
*
* @author original: Hab Collector \n
*
* @note: Display must be init before use
* @note: This function does not in of itself refresh the screen.  It must be called prior to a function that
* does (like: drawSpectrumMock, drawSpectrum, or displayUpdateBuffer)
* 
* @param Display_SSD1309: Pointer to display handle
* @param PlaybackAction: String that descriptes the playback action: play, pause, stop, error
*
* STEP 1: Clear only the playback action area
* STEP 2: Print the updated play action to screen - note: requires another function call that will push to display
********************************************************************************************************/
void updateAudioDisplayPlaybackAction(Type_Display_SSD1309 *Display_SSD1309, char *PlaybackAction)
{
    // STEP 1: Clear only the playback action area
    uint8_t Y_Top = (uint8_t)(ACTION_BASELINE_Y - ACTION_FONT_H);
    u8g2_SetDrawColor(Display_SSD1309->U8G2_Handle, 0);
    u8g2_DrawBox(Display_SSD1309->U8G2_Handle, ACTION_X, Y_Top, ACTION_BOX_W, ACTION_BOX_H);
    u8g2_SetDrawColor(Display_SSD1309->U8G2_Handle, 1);

    // STEP 2: Print the updated play action to screen - note: requires another function call that will push to display
    u8g2_SetFont(Display_SSD1309->U8G2_Handle, u8g2_font_5x8_tr);
    u8g2_DrawStr(Display_SSD1309->U8G2_Handle, ACTION_X, ACTION_BASELINE_Y, PlaybackAction);

} // END OF updateAudioDisplayPlaybackAction



/********************************************************************************************************
* @brief Displays the static Signal Header.  This is the base information present when the user is in Audio
* SA mode: Title, ...
*
* @author original: Hab Collector \n
*
* @note: Display must be init before use
* 
* @param Display_SSD1309: Pointer to display handle
*
* STEP 1: Clear buffer
* STEP 2: Draw centered title (6x10)
* STEP 3: ...
* STEP 4: Push to display
********************************************************************************************************/
void drawStaticHeaderSignal(Type_Display_SSD1309 *Display_SSD1309, char *Heading)
{
    // STEP 1: Clear buffer
    u8g2_ClearBuffer(Display_SSD1309->U8G2_Handle);

    // STEP 2: Draw centered title (6x10)
    u8g2_SetFont(Display_SSD1309->U8G2_Handle, u8g2_font_6x10_tr);
    const char *Title = Heading;
    uint16_t TitleWidth = u8g2_GetStrWidth(Display_SSD1309->U8G2_Handle, Title);
    uint8_t X_Title = (uint8_t)((DISPLAY_WIDTH_PIXEL - TitleWidth) / 2);
    u8g2_DrawStr(Display_SSD1309->U8G2_Handle, X_Title, 10, Title); 

    // STEP 3: ...

    // STEP 4: Push to display
    u8g2_SendBuffer(Display_SSD1309->U8G2_Handle);

} // END OF drawStaticHeaderSignal