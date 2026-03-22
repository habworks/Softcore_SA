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
#include <string.h>
#include <math.h>


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
void displaySpectrumMock(Type_Display_SSD1309 *Display_SSD1309, bool ClearOnly)
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
    u8g2_DrawBox(Display_SSD1309->U8G2_Handle, 0, Spectrum_Y_Start, SPECTRUM_ERASE_WIDTH, Spectrum_Height);
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

} // END OF displaySpectrumMock



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
void displayWelcomeScreen(Type_Display_SSD1309 *Display_SSD1309, uint8_t FW_Major, uint8_t FW_Minor, uint8_t FW_Test, uint8_t PL_Major, uint8_t PL_Minor, uint8_t PL_Test)
{
    char FW_String[20] = {0};
    char PL_String[20] = {0};

    // STEP 1: Clear display buffer
    u8g2_ClearBuffer(Display_SSD1309->U8G2_Handle);

    // STEP 2: Draw large IMR Engineering
    u8g2_SetFont(Display_SSD1309->U8G2_Handle, u8g2_font_6x12_tr);
    u8g2_DrawStr(Display_SSD1309->U8G2_Handle, 0, 13, "IMR Engineering");
    // Draw Ideas Made Real
    u8g2_SetFont(Display_SSD1309->U8G2_Handle, u8g2_font_6x10_tr);
    u8g2_DrawStr(Display_SSD1309->U8G2_Handle, 0,24, "Ideas Made Real");

    // STEP 3: Format revision strings
    snprintf(FW_String, sizeof(FW_String),"FW REV: %02u.%02u.%02u", FW_Major, FW_Minor, FW_Test);
    snprintf(PL_String, sizeof(PL_String),"PL REV: %02u.%02u.%02u", PL_Major, PL_Minor, PL_Test);

    // STEP 4: Draw lower informational text (5x8)
    u8g2_SetFont(Display_SSD1309->U8G2_Handle, u8g2_font_5x8_tr);
    u8g2_DrawStr(Display_SSD1309->U8G2_Handle, 0,44, "SoftCore Spectrum Analyzer");
    u8g2_DrawStr(Display_SSD1309->U8G2_Handle, 0, 52, FW_String);
    u8g2_DrawStr(Display_SSD1309->U8G2_Handle, 0, 60, PL_String);

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
* @param Heading: Heading text to display
* @param FileName: Audio filename that will be played
* @param AudioAction: Play, Stop, Pause, or Error as text
* @param TimeInSeconds: Play time of current WAV audio file in seconds
* @param MinValue_dB: Min dB value of audio the max will be 0
*
* STEP 1: Format time string
* STEP 2: Clear the display buffer entirely 
* STEP 3: Draw centered title (6x10)
* STEP 4: Draw left-justified file info (5x8)
* STEP 5: Push to display
********************************************************************************************************/
void displayStaticHeaderAudio(Type_Display_SSD1309 *Display_SSD1309, char *Heading, char *FileName, char *AudioAction, uint32_t TimeInSeconds, int8_t MinValue_dB)
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

    // STEP 5: Draw right-justified dB scale labels on the far right side
    char MinDbString[6] = {0};
    snprintf(MinDbString, sizeof(MinDbString), "%d", (int)MinValue_dB);
    uint16_t Width_dB = u8g2_GetStrWidth(Display_SSD1309->U8G2_Handle, "dB");
    uint16_t Width_Zero = u8g2_GetStrWidth(Display_SSD1309->U8G2_Handle, "00");
    uint16_t Width_Min = u8g2_GetStrWidth(Display_SSD1309->U8G2_Handle, MinDbString);
    uint8_t X_dB = (uint8_t)(DISPLAY_WIDTH_PIXEL - Width_dB);
    uint8_t X_Zero = (uint8_t)(DISPLAY_WIDTH_PIXEL - Width_Zero);
    uint8_t X_Min = (uint8_t)(DISPLAY_WIDTH_PIXEL - Width_Min);
    // Place scale labels safely within the spectrum area (Spectrum_Y_Start = 40) using 5x8 font
    u8g2_DrawStr(Display_SSD1309->U8G2_Handle, X_dB, 29U, "dB");
    u8g2_DrawStr(Display_SSD1309->U8G2_Handle, X_Zero, 37U, "00");
    u8g2_DrawStr(Display_SSD1309->U8G2_Handle, X_Min, 63U, MinDbString);

    // STEP 6: Push to display
    u8g2_SendBuffer(Display_SSD1309->U8G2_Handle);

} // END OF displayStaticHeaderAudio



/********************************************************************************************************
* @brief: Erases and updates the audio playback time in the format of 00:00 mm:ss
*
* @author original: Hab Collector \n
*
* @note: Display must be init before use
* @note: This function does not in of itself refresh the screen.  It must be called prior to a function that
* does (like: drawSpectrumMock, drawAudioSpectrum, or displayUpdateBuffer)
* 
* @param Display_SSD1309: Pointer to display handle
* @param TimeInSeconds: Playback audio time in seconds elapsed
*
* STEP 1: Format time string
* STEP 2: Clear only time area (convert baseline to top-left)
* STEP 3: Print the updated time to screen - note: requires another function call that will push to display
********************************************************************************************************/
void displayUpdateAudioPlaybackTime(Type_Display_SSD1309 *Display_SSD1309, uint32_t TimeInSeconds)
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

} // END OF displayUpdateAudioPlaybackTime



/********************************************************************************************************
* @brief: Erases and updates the audio playback action (Play, Pause, Stop, Error)
*
* @author original: Hab Collector \n
*
* @note: Display must be init before use
* @note: This function does not in of itself refresh the screen.  It must be called prior to a function that
* does (like: drawSpectrumMock, drawAudioSpectrum, or displayUpdateBuffer)
* 
* @param Display_SSD1309: Pointer to display handle
* @param PlaybackAction: String that descriptes the playback action: play, pause, stop, error
*
* STEP 1: Clear only the playback action area
* STEP 2: Print the updated play action to screen - note: requires another function call that will push to display
********************************************************************************************************/
void displayUpdateAudioPlaybackAction(Type_Display_SSD1309 *Display_SSD1309, char *PlaybackAction)
{
    // STEP 1: Clear only the playback action area
    uint8_t Y_Top = (uint8_t)(ACTION_BASELINE_Y - ACTION_FONT_H);
    u8g2_SetDrawColor(Display_SSD1309->U8G2_Handle, 0);
    u8g2_DrawBox(Display_SSD1309->U8G2_Handle, ACTION_X, Y_Top, ACTION_BOX_W, ACTION_BOX_H);
    u8g2_SetDrawColor(Display_SSD1309->U8G2_Handle, 1);

    // STEP 2: Print the updated play action to screen - note: requires another function call that will push to display
    u8g2_SetFont(Display_SSD1309->U8G2_Handle, u8g2_font_5x8_tr);
    u8g2_DrawStr(Display_SSD1309->U8G2_Handle, ACTION_X, ACTION_BASELINE_Y, PlaybackAction);

} // END OF displayUpdateAudioPlaybackAction



/********************************************************************************************************
* @brief Displays a the audio specturm.  No calculations are done here.  All caculations are computed by
* the calling function to match the display height and number of spectrum slots.  
*
* @author original: Hab Collector \n
*
* @note: Display must be init before use
* 
* @param Display_SSD1309: Pointer to display handle
* @param DisplayMagnitude: Buffer of relatiive amplitude to match the VerticalBarCount for each slot
* @param FrequencySlots: Number of frequencies to display
* @param VerticalBarCount: The max number of bars - corresponds to the max display magnitude
* @param ClearOnly: Only clears the sprecrturm display area - no specturm is displayed
*
* STEP 1: Clear the spectrum area of the display by drawing a black box
* STEP 2: Draw each spectral bar
* STEP 3: Push to display
********************************************************************************************************/
void displayAudioSpectrum(Type_Display_SSD1309 *Display_SSD1309, uint8_t *DisplayMagnitude, uint8_t FrequencySlots, uint8_t VerticalBarCount, bool ClearOnly)
{
    // USER-ADJUSTABLE LOCAL CONSTANTS (self-contained)
    uint8_t NumBars           = FrequencySlots;     // Number of frequency columns
    uint8_t SegmentsPerBar    = VerticalBarCount;   // Vertical resolution
    uint8_t SegmentHeight     = 2;      // Height of each vertical block (pixels)
    uint8_t SegmentVSpace     = 1;      // Space between vertical blocks
    uint8_t BarWidth          = 4;      // Width of each bar (pixels)
    uint8_t BarHSpace         = 2;      // Horizontal spacing between bars
    uint8_t BaselineY         = 63;     // Vertical baseline position (SSD1309 is 64px tall)

    // STEP 1: Clear the spectrum area of the display by drawing a black box
    uint8_t Spectrum_Y_Start = 40;
    uint8_t Spectrum_Height = (uint8_t)(DISPLAY_HEIGH_PIXEL - Spectrum_Y_Start);
    u8g2_SetDrawColor(Display_SSD1309->U8G2_Handle, 0);
    u8g2_DrawBox(Display_SSD1309->U8G2_Handle, 0, Spectrum_Y_Start, SPECTRUM_ERASE_WIDTH, Spectrum_Height);
    u8g2_SetDrawColor(Display_SSD1309->U8G2_Handle, 1);

    // STEP 2: Draw each spectral bar
    if (!ClearOnly)
    {
        for (uint8_t BarIndex = 0; BarIndex < NumBars; BarIndex++)
        {
            // Random height: 0–SegmentsPerBar
            uint8_t Value = DisplayMagnitude[BarIndex];

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

} // END OF displayAudioSpectrum



/********************************************************************************************************
* @brief Displays the static Signal Header.  This is the base information present when the user is in Audio
* SA mode: Title, Signal Source and Span
*
* @author original: Hab Collector \n
*
* @note: Display must be init before use
* 
* @param Display_SSD1309: Pointer to display handle
* @param Heading: Heading text to be displayed
* @param StartFrequency: Start (far left side) frequency
* @param CenterFrequency: Center (middle of display) frequency
* @param StopFrequency: Stop (far right side) frequency
* @param SignalSource: Signal source either local ossiclator or external BNC
*
* STEP 1: Clear buffer
* STEP 2: Draw centered title (6x10)
* STEP 3: Display Source
* STEP 4: Display Center and Span
* STEP 5: Push to display
********************************************************************************************************/
void displayStaticHeaderSignal(Type_Display_SSD1309 *Display_SSD1309, char *Heading, float StartFrequency, float CenterFrequency, float StopFrequency, Type_SignalSelect SignalSource)
{
    // STEP 1: Clear buffer
    u8g2_ClearBuffer(Display_SSD1309->U8G2_Handle);

    // STEP 2: Draw centered title (6x10)
    u8g2_SetFont(Display_SSD1309->U8G2_Handle, u8g2_font_6x10_tr);
    const char *Title = Heading;
    uint16_t TitleWidth = u8g2_GetStrWidth(Display_SSD1309->U8G2_Handle, Title);
    uint8_t X_Title = (uint8_t)((DISPLAY_WIDTH_PIXEL - TitleWidth) / 2);
    u8g2_DrawStr(Display_SSD1309->U8G2_Handle, X_Title, 10, Title); 

    // STEP 3: Display Source
    u8g2_SetFont(Display_SSD1309->U8G2_Handle, u8g2_font_5x8_tr);
    if (SignalSource == SIGNAL_ON_BOARD_OSCILLATOR)
        u8g2_DrawStr(Display_SSD1309->U8G2_Handle, 0, 19U, SOURCE_LOCAL_OSSCILATOR);
    else
        u8g2_DrawStr(Display_SSD1309->U8G2_Handle, 0, 19U, SOURCE_EXTERNAL_BNC);

    // STEP 4: Display Center and Span
    // Format frequency string
    char StartString[12] = {0};
    char CenterString[12] = {0};
    char StopString[12] = {0};
    snprintf(StartString, sizeof(StartString), "%dKHz", ((int)StartFrequency / 1000));
    snprintf(CenterString, sizeof(CenterString), "%.1fKHz", (CenterFrequency / 1000.0f));
    snprintf(StopString, sizeof(StopString), "%dKHz", ((int)StopFrequency / 1000));
    // Calculate X location
    uint16_t CenterWidth = u8g2_GetStrWidth(Display_SSD1309->U8G2_Handle, CenterString);
    uint16_t StopWidth = u8g2_GetStrWidth(Display_SSD1309->U8G2_Handle, StopString);
    uint8_t X_Center = (uint8_t)(((DISPLAY_WIDTH_PIXEL - CenterWidth)) / 2U); // + strlen(CenterString) + 1U);
    uint8_t X_Stop = (uint8_t)(DISPLAY_WIDTH_PIXEL - StopWidth); // + strlen(StopString) + 1U); 
    // Display
    u8g2_DrawStr(Display_SSD1309->U8G2_Handle, 0, 28U, StartString);
    u8g2_DrawStr(Display_SSD1309->U8G2_Handle, X_Center, 28U, CenterString);
    u8g2_DrawStr(Display_SSD1309->U8G2_Handle, X_Stop, 28U, StopString);

    // STEP 5: Push to display
    u8g2_SendBuffer(Display_SSD1309->U8G2_Handle);

} // END OF displayStaticHeaderSignal



/********************************************************************************************************
* @brief: Erases and updates the signal source (Local Osscilator or External BNC)
*
* @author original: Hab Collector \n
*
* @note: Display must be init before use
* @note: This function does not in of itself refresh the screen.  It must be called prior to a function that
* does (like: drawSpectrumMock, drawSignalSpectrum, or displayUpdateBuffer)
* 
* @param Display_SSD1309: Pointer to display handle
* @param PlaybackAction: String that descriptes the playback action: play, pause, stop, error
*
* STEP 1: Clear only the playback action area
* STEP 2: Print the updated signal source to screen - note: requires another function call that will push to display
********************************************************************************************************/
void displayUpdateSignalSource(Type_Display_SSD1309 *Display_SSD1309, Type_SignalSelect SignalSource)
{
    // STEP 1: Clear only the playback action area
    uint8_t Y_Top = (uint8_t)(SOURCE_BASELINE_Y - SOURCE_FONT_H);
    u8g2_SetDrawColor(Display_SSD1309->U8G2_Handle, 0);
    u8g2_DrawBox(Display_SSD1309->U8G2_Handle, SOURCE_X, Y_Top, SOURCE_BOX_W, SOURCE_BOX_H);
    u8g2_SetDrawColor(Display_SSD1309->U8G2_Handle, 1);

    // STEP 2: Print the updated signal source to screen - note: requires another function call that will push to display
    u8g2_SetFont(Display_SSD1309->U8G2_Handle, u8g2_font_5x8_tr);
    if (SignalSource == SIGNAL_ON_BOARD_OSCILLATOR)
        u8g2_DrawStr(Display_SSD1309->U8G2_Handle, SOURCE_X, SOURCE_BASELINE_Y, SOURCE_LOCAL_OSSCILATOR);
    else
        u8g2_DrawStr(Display_SSD1309->U8G2_Handle, SOURCE_X, SOURCE_BASELINE_Y, SOURCE_EXTERNAL_BNC);        

} // END OF displayUpdateSignalSource



/********************************************************************************************************
* @brief Renders the Signal SA FFT data to the SSD1309 OLED as a continuous spectrum trace.  The function
* uses a selected range of FFT bins and maps that bin range across the horizontal trace width of the signal
* display window.  Since the number of FFT bins is typically larger than the number of available X pixels,
* multiple bins are grouped together for each displayed pixel column.  The function averages the magnitudes
* of all bins assigned to a given pixel column so the displayed trace is more stable and less sensitive to a
* single noisy bin.  The averaged column magnitude is then converted to dB using a fixed magnitude reference
* so the display behaves more like a traditional spectrum analyzer and uses the vertical space more
* effectively than a linear magnitude display.  The resulting dB value is clamped to the requested display
* range so anything below the minimum display range is forced to the bottom of the trace area and anything
* above the maximum display range is forced to the top.  After clamping, the dB value is normalized to a
* 0.0 to 1.0 range and then converted into a vertical pixel height inside the defined spectrum box.  Each
* point is connected to the previous point with a line segment so the result is a continuous trace rather
* than a disconnected set of points.  Bin 0 is intentionally skipped if requested so the DC term does not
* dominate the display.  This routine only handles rendering and assumes the FFT magnitudes were already
* computed elsewhere.  The visual result is therefore a practical display layer that converts processed FFT
* output into a usable on-screen frequency-domain view for the Signal SA mode.
*
* @author original: Vector \n
* @author modified: Hab Collector \n
*
* @note: This function was orignally designed by my AI assistant
* @note: The function expects BinMagnitudes[] to already contain valid FFT magnitude data for the requested bin range
* @note: The function uses a fixed dB reference magnitude.  This is a display calibration choice and not an absolute measurement standard
* @note: Bin 0 is suppressed to avoid displaying the DC component when LowBin is 0
* @note: Multiple FFT bins may map into one display column.  This is required because the display width is limited
* @note: Column averaging reduces sensitivity to isolated noisy bins but does not remove aliasing or front-end analog issues
* @note: The displayed result is only as accurate as the FFT data and the analog signal chain feeding the ADC
* 
* @param Display_SSD1309: Pointer to the SSD1309 display handle structure
* @param BinMagnitudes: Pointer to the FFT magnitude array indexed by FFT bin number
* @param LowBin: Lowest FFT bin requested for display
* @param HighBin: One past the highest FFT bin requested for display
* @param MinDisplay_dB: Minimum dB value mapped to the bottom of the trace region
* @param MaxDisplay_dB: Maximum dB value mapped to the top of the trace region
*
* @return None
*
* STEP 1: Validate input pointers and display range limits
* STEP 2: Determine the actual FFT bin range to display and suppress DC if needed
* STEP 3: Calculate the displayed bin span and trace width in pixels
* STEP 4: Clear the signal trace region and draw the spectrum frame
* STEP 5: Map FFT bins into display columns, average magnitudes, convert to dB, convert to Y pixels, and draw the trace
* STEP 6: Send the completed buffer to the OLED
********************************************************************************************************/
void displaySignalSpectrum(Type_Display_SSD1309 *Display_SSD1309, float *BinMagnitudes, uint16_t LowBin, uint16_t HighBin, float MinDisplay_dB, float MaxDisplay_dB)
{
    #define SIGNAL_BOX_X0                    (0U)
    #define SIGNAL_BOX_Y0                    (28U)
    #define SIGNAL_BOX_WIDTH                 (DISPLAY_WIDTH_PIXEL)
    #define SIGNAL_BOX_HEIGHT                (DISPLAY_HEIGH_PIXEL - SIGNAL_BOX_Y0)
    #define SIGNAL_TRACE_X0                  (1U)
    #define SIGNAL_TRACE_X1                  (DISPLAY_WIDTH_PIXEL - 2U)
    #define SIGNAL_TRACE_Y0                  (SIGNAL_BOX_Y0 + 1U)
    #define SIGNAL_TRACE_Y1                  (DISPLAY_HEIGH_PIXEL - 2U)
    #define SIGNAL_MAGNITUDE_FLOOR           (0.001f)
    #define SIGNAL_DB_REFERENCE_MAGNITUDE    (10000.0f)

    uint16_t StartBin;
    uint16_t BinCount;
    uint16_t TraceWidth;
    uint8_t PreviousY = 0U;
    bool IsFirstPoint = true;

    // STEP 1: Validate input pointers and display range limits
    if (Display_SSD1309 == NULL)
        return;

    if (BinMagnitudes == NULL)
        return;

    if (LowBin >= HighBin)
        return;

    if (MinDisplay_dB >= MaxDisplay_dB)
        return;

    // STEP 2: Determine the actual FFT bin range to display and suppress DC if needed
    StartBin = LowBin;
    StartBin = LowBin;
    if (StartBin == 0U)
        StartBin = 1U;
    if (StartBin >= HighBin)
        return;

    // STEP 3: Calculate the displayed bin span and trace width in pixels
    BinCount = (uint16_t)(HighBin - StartBin);
    TraceWidth = (uint16_t)(SIGNAL_TRACE_X1 - SIGNAL_TRACE_X0 + 1U);

    // STEP 4: Clear the signal trace region and draw the spectrum frame
    u8g2_SetDrawColor(Display_SSD1309->U8G2_Handle, 0U);
    u8g2_DrawBox(Display_SSD1309->U8G2_Handle, SIGNAL_BOX_X0, SIGNAL_BOX_Y0, SIGNAL_BOX_WIDTH, SIGNAL_BOX_HEIGHT);
    u8g2_SetDrawColor(Display_SSD1309->U8G2_Handle, 1U);
    u8g2_DrawFrame(Display_SSD1309->U8G2_Handle, SIGNAL_BOX_X0, SIGNAL_BOX_Y0, SIGNAL_BOX_WIDTH, SIGNAL_BOX_HEIGHT);
    // Draw the vertical center reference line
    u8g2_DrawVLine(Display_SSD1309->U8G2_Handle, (DISPLAY_WIDTH_PIXEL / 2U), SIGNAL_TRACE_Y0, (SIGNAL_TRACE_Y1 - SIGNAL_TRACE_Y0 + 1U));

    // STEP 5: Map FFT bins into display columns, average magnitudes, convert to dB, convert to Y pixels, and draw the trace
    for (uint16_t X = SIGNAL_TRACE_X0; X <= SIGNAL_TRACE_X1; X++)
    {
        uint16_t PixelIndex = (uint16_t)(X - SIGNAL_TRACE_X0);
        uint16_t BinStart = (uint16_t)(StartBin + (((uint32_t)PixelIndex * BinCount) / TraceWidth));
        uint16_t BinStop  = (uint16_t)(StartBin + ((((uint32_t)PixelIndex + 1U) * BinCount) / TraceWidth));
        float ColumnAverageMagnitude = 0.0f;
        float Magnitude_dB;
        float NormalizedMagnitude;
        float TraceHeightFloat;
        uint16_t BinCounter = 0U;
        uint8_t TraceHeight;
        uint8_t CurrentY;

        if (BinStop <= BinStart)
            BinStop = (uint16_t)(BinStart + 1U);

        if (BinStop > HighBin)
            BinStop = HighBin;

        for (uint16_t Bin = BinStart; Bin < BinStop; Bin++)
        {
            ColumnAverageMagnitude += BinMagnitudes[Bin];
            BinCounter++;
        }

        if (BinCounter > 0U)
            ColumnAverageMagnitude /= (float)BinCounter;

        if (ColumnAverageMagnitude < SIGNAL_MAGNITUDE_FLOOR)
            ColumnAverageMagnitude = SIGNAL_MAGNITUDE_FLOOR;

        Magnitude_dB = 20.0f * log10f(ColumnAverageMagnitude / SIGNAL_DB_REFERENCE_MAGNITUDE);

        if (Magnitude_dB < MinDisplay_dB)
            Magnitude_dB = MinDisplay_dB;

        if (Magnitude_dB > MaxDisplay_dB)
            Magnitude_dB = MaxDisplay_dB;

        NormalizedMagnitude = (Magnitude_dB - MinDisplay_dB) / (MaxDisplay_dB - MinDisplay_dB);

        TraceHeightFloat = NormalizedMagnitude * (float)(SIGNAL_TRACE_Y1 - SIGNAL_TRACE_Y0);
        TraceHeight = (uint8_t)(TraceHeightFloat + 0.5f);

        CurrentY = (uint8_t)(SIGNAL_TRACE_Y1 - TraceHeight);

        if (CurrentY < SIGNAL_TRACE_Y0)
            CurrentY = SIGNAL_TRACE_Y0;

        if (CurrentY > SIGNAL_TRACE_Y1)
            CurrentY = SIGNAL_TRACE_Y1;

        if (IsFirstPoint == false)
            u8g2_DrawLine(Display_SSD1309->U8G2_Handle, (X - 1U), PreviousY, X, CurrentY);

        PreviousY = CurrentY;
        IsFirstPoint = false;
    }

    // STEP 6: Send the completed buffer to the OLED
    u8g2_SendBuffer(Display_SSD1309->U8G2_Handle);

} // END OF displaySignalSpectrum