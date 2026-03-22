/******************************************************************************************************
 * @file            SoftCore_Audio_SA.c
 * @brief           A collection of functions relevant to supporting Audio (WAV only) files on a SA display
 *                  using FFT
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

#include "Audio_SoftCore_SA.h"
#include "Main_App.h"
#include "AXI_Timer_PWM_Support.h"
#include "AXI_IRQ_Controller_Support.h"
#include "Terminal_Emulator_Support.h"
#include "IO_Support.h"
#include "Hab_Types.h"
#include "Application_Display.h"
#include "FFT_Support.h"
#include "xgpio.h"
#include "ff.h"
#include <stdio.h>
#include <stdlib.h>


// STATIC FUNCTIONS
static bool feedStream_PCM16_WAV(Type_Audio_SA *Audio_SA, Type_FFT *FFT); // __attribute__((fast_interrupt));
static void errorCloseAudioFile(Type_Audio_SA *Audio_SA, Type_FFT *FFT, char *ErrorMsg);
static int16_t convert_PCM16_ToMono(int16_t Left_PCM16_Audio, int16_t Right_PCM16_Audion);
static uint16_t convert_PCM16_To_PWM_DutyPercent(int16_t PCM16_Sample, uint16_t PercentBase);
static bool updateDisplayPlaybackTimer(uint32_t ISR_PlayBackTicks, uint32_t PlaybackRate);
static void apply_FFT_Window(Type_Audio_SA *Audio_SA, Type_FFT *FFT);
static uint8_t LED_AudioBarGraphCalculate(int16_t PCM_AudioLevel);


// GLOBAL AUDIO PLAYBACK
uint8_t __attribute__ ((section (".Hab_Fast_Data"))) RawLinearBuffer[MAX_RAW_BUFFER];
float __attribute__ ((section (".Hab_Fast_Data"))) BinMagnitudes[(FFT_SIZE/2) + 1];
uint8_t __attribute__ ((section (".Hab_Fast_Data"))) DisplayMagnitude[FREQUENCY_SLOTS];
// For testing only
uint32_t __attribute__ ((section (".Hab_Fast_Data"))) CB_EmptyIn_ISR;


/********************************************************************************************************
* @brief This is the main function that handles the audio spectrum analyzer function - it is non-blocking so
* other UI inputs can be processed.  It loads the audio playback stream, updates the audio display LED bar graph, 
* applies the FFT window and updates the audio spectrum display.
*
* @author original: Hab Collector \n
*
* @param Audio_SA: Pointer to audio spectrum analyzer control structure
* @param FFT: Pointer to FFT structure
* @param LED_ModeStatus: LED7 and LED8 bit mask
*
* STEP 1: Audio play and spectrum qualifier
* STEP 2: Stream the audio from the uSD - It is played in the audio ISR
* STEP 3: Update the audio LED bar graph
* STEP 4: FFT Update when frame ready (FFT number of samples collected
********************************************************************************************************/
void audioSpectrumAnalyzer(Type_Audio_SA *Audio_SA, Type_FFT *FFT, uint8_t LED_ModeStatus)
{

    bool Status;

    // STEP 1: Audio play and spectrum qualifier
    if ((!Audio_SA->Enable) || (Audio_SA->AudioAction != AUDIO_ACTION_PLAY))
        return;

    // STEP 2: Stream the audio from the uSD - It is played in the audio ISR
// XGpio_DiscreteSet(&AXI_GPIO_Handle, GPIO_OUTPUT_CHANNEL, TEST_IO_0);
    feedStream_PCM16_WAV(Audio_SA, FFT);
// XGpio_DiscreteClear(&AXI_GPIO_Handle, GPIO_OUTPUT_CHANNEL, TEST_IO_0);  
// StackUsedWaterMark = getStackHighWaterMarkBytes();

    // STEP 3: Update the audio LED bar graph
    update_LED_AudioBarGraph(Audio_SA->PresentValue_PCM16, LED_ModeStatus);

    // STEP 4: FFT Update when frame ready (FFT number of samples collected)
    if (FFT->FrameReady)
    {            
        // Process the FFT frame data
        FFT_ProcessAudioFrame(FFT, BinMagnitudes, BIN_COUNT);

        // Update the display playback time
        updateDisplayPlaybackTimer(Audio_SA->PlaybackTickCounter, Audio_SA->File.Header.SampleRate);
        
        // Prep the information from processing to be displayed - a way to present the BIN Amplitude
        buildAudioSpectrumFrame(Audio_SA->File.Header.SampleRate,
                            FFT->Size,
                            BinMagnitudes,
                            BIN_COUNT,
                            SLOT_MAGNITUDE_RMS,
                            LINEAR_COMPRESSION_NONE,
                            AUDIO_MIN_DB_DISPLAY,
                            DisplayMagnitude,
                            FREQUENCY_SLOTS,
                            MAX_VERTICAL_BAR_COUNT,
                            &AudioSpectrum);

        // Present the display audio spectrum
        displayAudioSpectrum(&Display_SSD1309, DisplayMagnitude, FREQUENCY_SLOTS, MAX_VERTICAL_BAR_COUNT, false);
    }  

} // END OF audioSpectrumAnalyzer



/********************************************************************************************************
* @brief Services a 16-bit PCM WAV audio stream by sequentially reading audio data from a WAV file and
* loading decoded samples into a circular buffer for FFT processing and feedback.  This function is intended 
* to be called repeatedly until EOF or the action has been stopped / paused externaly.  Each call advances 
* the stream only as far as circular buffer has room avilable.
*
* @author original: Hab Collector \n
*
* @note: Requires prior initialization of FAT FS and a valid WAV file header
* @note: This function does not perform FFT processing or display updates
* @note: PCM DATA STORAGE – MONO
* For mono WAV files, audio samples are stored in the file as consecutive signed 16-bit
* little-endian values.  Each sample consists of two bytes:
*   - LSB first
*   - MSB second
* @note: PCM DATA STORAGE – STEREO
* For stereo WAV files, audio samples are stored in the file as interleaved signed 16-bit
* little-endian values in the following order:
*   Byte 0,1: Left channel sample (PCM16)
*   Byte 2,3: Right channel sample (PCM16)
* @note: For each stereo frame, both left and right samples are read, converted to signed 16-bit
* values, and down-mixed to mono by averaging the two channels before being written to the
* circular buffer.
* @note: Though this function will play any PCM16 WAV audio file, the buffer size (circular primarly) and
* the RawLinearBuffer impact how playback will sound.  This is because the read uSD speed is slow and 
* depending on the playback speed the CB buffer can run empty.  The CB buffer running empty excessively will
* sound like slow, or slow and muffled audio.  Tested to work good at 16KHz, Mono with the buffer sizes in 
* use.  Future revision of this code and PL must make more LBM available to the heap for allocation and
* the size of the buffers should then be based on audio rate and number of channels and not fixed.
*
* @param Audio_SA: Pointer to audio spectrum analyzer control structure
* @param FFT: Pointer to FFT structure
*
* @return true if operation is successful or no action is required
* @return false if a file or buffer initialization error occurs
*
* STEP 1: Initialization for audio play back
* STEP 2: Termination Check
* STEP 3: High-Water Mark Refill
* STEP 4: Conversion (Fast Pointer Math)
* STEP 5: Start ISR Trigger - the buffer has been pre-loaded start the ISR - playback in handled by the ISR
********************************************************************************************************/
static bool feedStream_PCM16_WAV(Type_Audio_SA *Audio_SA, Type_FFT *FFT)
{
    static uint32_t BytesToReadFromFile = 0;
    bool Status;
    FIL *FileHandle = &Audio_SA->File.FileHandle;
    FRESULT FileStatus = FR_OK;
    uint32_t BytesLastReadFromFile = 0;
    
    // STEP 1: Initialization for audio play back
    if (!Audio_SA->File.IsOpen)
    {
        if (f_open(FileHandle, Audio_SA->File.PathFileName, FA_READ) != FR_OK)
         {
            errorCloseAudioFile(Audio_SA, FFT, "Fail to open WAV file");
            return(false);
        }
        // Upade handle members and data size
        CB_EmptyIn_ISR = 0;
        Audio_SA->File.IsOpen = true;
        Audio_SA->File.Is_EOF = false;
        Audio_SA->IsPreLoadComplete = false;
        Audio_SA->PlaybackTickCounter = 0;
        BytesToReadFromFile = Audio_SA->File.Header.DataSize;
        // Ensure buffers are large enough (10K+ samples recommended at 16000KHz audio sample rate)
        // Hab Future: Allocate memory based on audio sample rate
        Status = init_I16_CB(&Audio_SA->Samples_CB, (1024 * 22)); 
        if (Status == false)
        {
            errorCloseAudioFile(Audio_SA, FFT, "Fail to allocate memory");
            return(false);
        }
        // Go to audio data offset within WAV file
        FileStatus = f_lseek(FileHandle, WAV_DATA_OFFSET);
        if (FileStatus != FR_OK)
        {
            errorCloseAudioFile(Audio_SA, FFT, "Fail file seek");
            return(false);
        }
    }

    // STEP 2: Termination Check
    if (Audio_SA->File.Is_EOF && isEmpty_I16_CB(&Audio_SA->Samples_CB))
    {
        stopAudio_SA(Audio_SA, FFT);
        return(true);
    }

    // STEP 3: High-Water Mark Refill
    // Only trigger a read if space for a significant "chunk" (e.g., 2048 bytes = sizeof(RawLinearBuffer)
    // This reduces SD Card overhead.
    uint32_t FreeSamples = availableWrites_I16_CB(&Audio_SA->Samples_CB);
    uint32_t ChunkInSamples = 512 * 4; // Process 512 samples at a time
    if (FreeSamples >= ChunkInSamples && !Audio_SA->File.Is_EOF)
    {
        uint32_t BytesToRequest;
        bool IsStereo = (Audio_SA->File.Header.ChannelNumber == STEREO);
        
        // Calculate bytes needed: Stereo = 4 bytes/sample, Mono = 2 bytes/sample
        BytesToRequest = ChunkInSamples * (IsStereo ? 4 : 2);
        // Clamp the read size
        if (BytesToRequest > BytesToReadFromFile)
            BytesToRequest = BytesToReadFromFile;
        // Read directly into your linear scratch buffer
        FileStatus = f_read(FileHandle, RawLinearBuffer, BytesToRequest, &BytesLastReadFromFile);
        if (FileStatus != FR_OK)
        {
            errorCloseAudioFile(Audio_SA, FFT, "Fail file read");
            return(false);
        }
        if (BytesLastReadFromFile == 0) 
            Audio_SA->File.Is_EOF = true;

        // STEP 4: Conversion (Fast Pointer Math)
        // Convert the raw bytes in RawLinearBuffer directly to a PCM16 which is of int16_t
        int16_t *PCM16_Ptr = (int16_t *)RawLinearBuffer;
        uint32_t SamplesToRead = BytesLastReadFromFile / (IsStereo ? 4 : 2);
        for (uint32_t Index = 0; Index < SamplesToRead; Index++)
        {
            if (IsStereo)
            {
                // Left is PCM16_Ptr[i*2], Right is PCM16_Ptr[i*2 + 1]
                int16_t MonoMix = convert_PCM16_ToMono(PCM16_Ptr[Index * 2], PCM16_Ptr[(Index * 2) + 1]);
                write_I16_CB(&Audio_SA->Samples_CB, MonoMix);
            }
            else
            {
                write_I16_CB(&Audio_SA->Samples_CB, PCM16_Ptr[Index]);
            }
        }
        // Update the bytes read from file and check if EoF (end of data read)
        BytesToReadFromFile -= BytesLastReadFromFile;
        if (BytesToReadFromFile == 0) 
            Audio_SA->File.Is_EOF = true;
    }

    // STEP 5: Start ISR Trigger - the buffer has been pre-loaded start the ISR - playback in handled by the ISR
    if (!Audio_SA->IsPreLoadComplete && isFull_I16_CB(&Audio_SA->Samples_CB))
    {
        // Enable audio output: PWM and audio amplifier
        enable_PWM(&AXI_PWM_Handle);
        setup_PWM(&AXI_PWM_Handle, AUDIO_PWM_FREQUENCY, AUDIO_PWM_DEFAULT_DUTY);
        audioEnable(true);
        // Update the ISR to the audio playback frequency and enable the audio playback ISR
        update_PeriodicTimerPeriod(&AXI_SampleTimerHandle, XTC_TIMER_0, (uint32_t)(XPAR_CPU_CORE_CLOCK_FREQ_HZ / Audio_SA->File.Header.SampleRate), false);
        resumeSpecificIRQ(&AXI_IRQ_ControllerHandle, XPAR_FABRIC_AXI_TIMER_1_INTR);
        Audio_SA->IsPreLoadComplete = true;
    }

    return(true);

} // END OF feedStream_PCM16_WAV



/********************************************************************************************************
* @brief A serires of steps necessary when closing out the feedStream_PCM16_WAV due to an error condition
*
* @author original: Hab Collector \n
*
* @param Audio_SA: Pointer to Audio Spectrum Analyzer structure
* @param FFT: Pointer to FFT structure
* @param FileHandle: Pointer to the WAV file handle that maybe open
*
* STEP 1: Make preperations to leave feedStream_PCM16_WAV gracefully
********************************************************************************************************/
static void errorCloseAudioFile(Type_Audio_SA *Audio_SA, Type_FFT *FFT, char *ErrorMsg)
{
    char PrintBuffer[100];
    // STEP 1: Make preperations to leave feedStream_PCM16_WAV gracefully
    stopAudio_SA(Audio_SA, FFT);
    snprintf(PrintBuffer, sizeof(PrintBuffer), "ERROR: %s\r\n", ErrorMsg);
    printBrightRed(PrintBuffer);

} // END OF errorCloseAudioFile



/********************************************************************************************************
* @brief Convert PCM 16 setero audio to mono audio.  Conversion is made by averaging the 2 16b signed values
*
* @author original: Hab Collector \n
*
* @note: Intended for single speaker PWM-based audio playback only
* @note: For use with PWM play back
*
* @param Left_PCM16_AudioSample: Signed 16-bit PCM audio sample (-32768 to +32767) - left channel
* @param Right_PCM16_AudioSample: Signed 16-bit PCM audio sample (-32768 to +32767) - right channel
*
* @return The mono audio sample
*
* STEP 1: Convert stero to mono by averaging the left and right PCM16 samples
********************************************************************************************************/
static int16_t convert_PCM16_ToMono(int16_t Left_PCM16_AudioSample, int16_t Right_PCM16_AudioSample)
{
    // STEP 1: Convert stero to mono by averaging the left and right PCM16 samples
    int32_t MonoAudioSample = Left_PCM16_AudioSample + Right_PCM16_AudioSample;
    MonoAudioSample /= 2;
    return ((int16_t)MonoAudioSample);

} // END OF convert_PCM16_ToMono



/********************************************************************************************************
* @brief Convert a signed 16-bit PCM audio sample to a PWM duty-cycle percentage
*
* @author original: Hab Collector \n
*
* @note: Intended for PWM-based audio playback only
* @note: For use with PWM play back
*
* @param PCM16_Sample: Signed 16-bit PCM audio sample (-32768 to +32767)
* @param PercentBase: Calculations are done based on this number 100, 1024, etc - this value would represent 100%
*
* @return PWM duty-cycle percentage (0.0 to 100.0)
*
* STEP 1: Offset signed PCM sample to an unsigned 16-bit range
* STEP 2: Clamp the value to the maximum 16-bit range (possibly redundant and unnecessary as the math in step 1 suggest)
* STEP 3: Convert to a percentage of full-scale PWM 0 - 100%
********************************************************************************************************/
static uint16_t convert_PCM16_To_PWM_DutyPercent(int16_t PCM16_Sample, uint16_t PercentBase)
{
    // STEP 1: Offset signed PCM sample to an unsigned 16-bit range
    uint32_t PWM_Duty = (uint32_t)((int32_t)PCM16_Sample + 32768U);

    // STEP 2: Clamp the value to the maximum 16-bit range (possibly redundant and unnecessary as the math in step 1 suggest)
    // if (PWM_Duty > 65535U)
    //     PWM_Duty = 65535U;

    // STEP 3: Convert to a percentage of full-scale PWM 0 - 100%    
    // float PWM_DutyPercent = 100.0 * ((float)PWM_Duty / 65535.0);
    uint16_t PWM_DutyPercent = (uint16_t)((PercentBase * PWM_Duty) / 65535U);
    return(PWM_DutyPercent);

} // END OF convert_PCM16_To_PWM_DutyPercent



/********************************************************************************************************
* @brief Apply a precomputed Hann window to the FFT input sample buffer
*
* @author original: Hab Collector \n
*
* @note: Input samples in-place prior to executing the FFT
* @note: FFT samples must be signed and zero-centered
*
* @param Audio_SA: Pointer to Audio Spectrum Analyzer structure
* @param FFT: Pointer to the FFT structure
*
* STEP 1: Apply FFT Hanning Window to FFT Samples store results in FFT Samples
********************************************************************************************************/
static void apply_FFT_Window(Type_Audio_SA *Audio_SA, Type_FFT *FFT)
{
    // STEP 1: Apply FFT Hanning Window to FFT Samples store results in FFT Samples
    for (uint16_t Index = 0; Index < FFT->Size; Index++)
    {
        FFT->Samples[Index] *= FFT->HannWindow[Index];
    }

} // END OF apply_FFT_Window



/********************************************************************************************************
* @brief Process the user input SW3. Stop audio playing action - can be called by user or via other funciton.  
* Stop safely by closing file and clearing allocated memory.
*
* @author original: Hab Collector \n
*
* @param Audio_SA: Pointer to Audio Spectrum Analyzer structure
* @param FFT: Pointer to FFT structure
*
* STEP 1: Apply safe stop of audio playback
********************************************************************************************************/
void stopAudio_SA(Type_Audio_SA *Audio_SA, Type_FFT *FFT)
{
    // STEP 1: Apply safe stop of audio playback
    Audio_SA->AudioAction = AUDIO_ACTION_STOP;
    // Stop audio playback and disable audio outpput
    pauseSpecificIRQ(&AXI_IRQ_ControllerHandle, XPAR_FABRIC_AXI_TIMER_1_INTR);
    audioEnable(false);
    disable_PWM(&AXI_PWM_Handle);
    // Free resources
    f_close(&Audio_SA->File.FileHandle);
    Audio_SA->File.IsOpen = false;
    free_I16_CB(&Audio_SA->Samples_CB);
    deinit_FFT(FFT);
    // Update the display
    displayUpdateAudioPlaybackTime(&Display_SSD1309, 0);
    displayUpdateAudioPlaybackAction(&Display_SSD1309, DISPLAY_AUDIO_STOP);
    displaySpectrumMock(&Display_SSD1309, true);

    // For testing only - remove later
    if (CB_EmptyIn_ISR != 0)
    {
        char PrintBuffer[100];
        snprintf(PrintBuffer, sizeof(PrintBuffer), "  Empty CB on playback: %d\r\n", CB_EmptyIn_ISR);
        printBrightRed(PrintBuffer);
    }

} // END OF stopAudio_SA



/********************************************************************************************************
* @brief Process the user input SW5. Start audio playback, but note that audio playback can start from a
* stopped or pause.  If comming from stop take action to ready resouces to a default state to play.  If
* comming from a pause you are just resuming - the ISR does not playback in any mode but play and the buffer
* will background fill
*
* @author original: Hab Collector \n
*
* @param Audio_SA: Pointer to Audio Spectrum Analyzer structure
* @param FFT: Pointer to FFT structure
*
* STEP 1: Siimple check
* STEP 2: Take a reset action if comming from a state other than pause
* STEP 3: Update dispaly
********************************************************************************************************/
void playAudio_SA(Type_Audio_SA *Audio_SA, Type_FFT *FFT)
{
    // STEP 1: Siimple check
    if ((!Audio_SA->Enable) || (Audio_SA->AudioAction == AUDIO_ACTION_PLAY))
        return;
    
    // STEP 2: Take a reset action if comming from a state other than pause
    if (Audio_SA->AudioAction != AUDIO_ACTION_PAUSE)
    {
        // Init FFT
        if (!init_FFT(FFT, Audio_SA->File.Header.SampleRate))
        {
            errorCloseAudioFile(Audio_SA, FFT, "Fail FFT init");
            displayUpdateAudioPlaybackAction(&Display_SSD1309, DISPLAY_AUDIO_ERROR);
            return;
        }
        // Ready struct members
        Audio_SA->IsPreLoadComplete = false;
        f_close(&Audio_SA->File.FileHandle);    // File should not be open - but just in case
        FFT->FrameReady = false;
        FFT->RBW = Audio_SA->File.Header.SampleRate / FFT->Size;
        // Update the display
        displayUpdateAudioPlaybackTime(&Display_SSD1309, 0);
        displayUpdateAudioPlaybackAction(&Display_SSD1309, DISPLAY_AUDIO_PLAY);
        displaySpectrumMock(&Display_SSD1309, true);
    }

    // STEP 3: Update display
    Audio_SA->AudioAction = AUDIO_ACTION_PLAY;
    displayUpdateAudioPlaybackAction(&Display_SSD1309, DISPLAY_AUDIO_PLAY);
 
} // END OF playAudio_SA



/********************************************************************************************************
* @brief Process the user input SW4. Pause audio playback 
*
* @author original: Hab Collector \n
*
* @param Audio_SA: Pointer to Audio Spectrum Analyzer structure
*
* STEP 1: Apply safe start of audio playback
********************************************************************************************************/
void pauseAudio_SA(Type_Audio_SA *Audio_SA)
{
    // STEP 1: Apply safe start of audio playback
    if (!Audio_SA->Enable)
        return;
    
    if (Audio_SA->AudioAction != AUDIO_ACTION_PLAY)
        return;

    Audio_SA->AudioAction = AUDIO_ACTION_PAUSE;

    // Update the display
    displayUpdateAudioPlaybackAction(&Display_SSD1309, DISPLAY_AUDIO_PAUSE);
    displayUpdateBuffer(&Display_SSD1309);
    
    // For testing only - remove later
    if (CB_EmptyIn_ISR != 0)
    {
        char PrintBuffer[100];
        snprintf(PrintBuffer, sizeof(PrintBuffer), "  Empty CB on playback: %d\r\n", CB_EmptyIn_ISR);
        printBrightRed(PrintBuffer);
    }

} // END OF playAudio_SA



/********************************************************************************************************
* @brief Enable the audio amplifier
*
* @author original: Hab Collector \n
*
* @param Enable: Status of amplifier
********************************************************************************************************/
void audioEnable(bool Enable)
{
    if (Enable)
        XGpio_DiscreteSet(&AXI_GPIO_Handle, GPIO_OUTPUT_CHANNEL, AUDIO_EN);
    else
        XGpio_DiscreteClear(&AXI_GPIO_Handle, GPIO_OUTPUT_CHANNEL, AUDIO_EN);
} 



/********************************************************************************************************
* @brief This is the audio periodic timer ISR.  It is called from main periodic timer ISR for the timer 1
* interrupt.  This ISR handles audio playback.  It is fired at the same rate as as the audio sample 11025, 
* 16000, 22050, 44100Hz.  It reads from the CB Samples buffer and feeds the PWM output.  It also keep udpates
* the FFT samples and sets condition for when the FFT is to be calculated.  The increment tick measure play
* back time (based on audio tick interval).
*
* @author original: Hab Collector \n
*
* @note: Timer is fired at the audio playback rate
* @note: If the CB Samples or Raw Linear Buffers are too small you will get a lot of buffer empty exits
*
* @param Audio_SA: Pointer to Audio Spectrum Analyzer structure
* @param FFT: Pointer to the FFT structure
*
* STEP 1: Simple test 
* STEP 2: Check for done playing - at EOF it is the ISR that stops the playback
* STEP 3: Check if buffer empty - CB_EmptyIn_ISR is a test variable 
* STEP 4: Update tick counter, read audio sample from CB, calculate PWM and udpate PWM output
* STEP 5: Set flag when FFT Frame is ready for processing
********************************************************************************************************/
void audioPeriodicTimer_ISR(Type_Audio_SA *Audio_SA, Type_FFT *FFT)
{
    static uint16_t SampleIndex = 0;

    // STEP 1: Simple test 
    if (Audio_SA->AudioAction != AUDIO_ACTION_PLAY)
        return;
    
    // STEP 2: Check for done playing - at EOF it is the ISR that stops the playback
    if (isEmpty_I16_CB(&Audio_SA->Samples_CB) && (Audio_SA->File.Is_EOF == true))
    {
        pauseSpecificIRQ(&AXI_IRQ_ControllerHandle, XPAR_FABRIC_AXI_TIMER_1_INTR);
        audioEnable(false);
        disable_PWM(&AXI_PWM_Handle);
        return;
    }
    
    // STEP 3: Check if buffer empty - CB_EmptyIn_ISR is a test variable 
    if (isEmpty_I16_CB(&Audio_SA->Samples_CB))
    {
        CB_EmptyIn_ISR++;   // Testing var
        return;
    }

    // STEP 4: Update tick counter, read audio sample from CB, calculate PWM and udpate PWM output
    Audio_SA->PlaybackTickCounter++;
    int16_t SampleValue;
    uint16_t PWM_DutyCycle;
    read_I16_CB(&Audio_SA->Samples_CB, &SampleValue, NULL, NULL);
    Audio_SA->PresentValue_PCM16 = SampleValue;
    FFT->Samples[SampleIndex] = SampleValue;
    PWM_DutyCycle = convert_PCM16_To_PWM_DutyPercent(SampleValue, 1024);
    update_PWM_Duty_Fast(&AXI_PWM_Handle, PWM_DutyCycle);
    
    // STEP 5: Set flag when FFT Frame is ready for processing
    SampleIndex++;
    if (SampleIndex == FFT_SIZE)
    {
        FFT->FrameReady = true;
        SampleIndex = 0;
    }

} // END OF audioPeriodicTimer_ISR



/********************************************************************************************************
* @brief Determines which LEDs should be turned on to represent the represent the audio level 
*
* @author original: Hab Collector \n
*
* @note: Function is logarithmic to match human hearing - linear interpolation would not match
* @note: Based on present audio sample in PCM format (int16_t)
*
* @param PCM_AudioLevel: Audio level in int16_t 
*
* @return A bitmask lowest to hightest of 6 LEDs that should be set
*
* STEP 1: Convert signed PCM value to absolute magnitude safely (handle -32768 and 32768 as same)
* STEP 2: Apply log thresholds and return LED bar mask
********************************************************************************************************/
static uint8_t LED_AudioBarGraphCalculate(int16_t PCM_AudioLevel)
{
    // STEP 1: Convert signed PCM value to absolute magnitude safely (handle -32768 and 32768 as same)
    uint32_t AbsoluteMagnitude = abs(PCM_AudioLevel);

    // STEP 2: Apply log thresholds and return LED bar mask
    if (AbsoluteMagnitude == 0U)
        return((uint8_t)0U);

    else if (AbsoluteMagnitude <= LEVEL_30DB)   // 5461U
        return(LED_BAR_1);

    else if (AbsoluteMagnitude <= LEVEL_26DB)   // 10922U
        return(LED_BAR_2);

    else if (AbsoluteMagnitude <= LEVEL_22DB)   // 16384U
        return(LED_BAR_3);

    else if (AbsoluteMagnitude <= LEVEL_18DB)   // 21845U
        return(LED_BAR_4);

    else if (AbsoluteMagnitude <= LEVEL_14DB)   // 27306U
        return(LED_BAR_5);
    else
        return(LED_BAR_6);

} // END OF LED_AudioBarGraphCalculate



/********************************************************************************************************
* @brief Display of the audio LEDs
*
* @author original: Hab Collector \n
*
* @param PCM_AudioLevel: Audio level in int16_t 
* @param LED_ModeStatusBitMask: LED7 and LED8 bit mask
*
* STEP 1: Determine the LED bit mask based on present audio level
* STEP 2: Update the IO Expander that drives the LEDs
********************************************************************************************************/
void update_LED_AudioBarGraph(int16_t PCM16_Value, uint8_t LED_ModeStatusBitMask)
{
    // STEP 1: Determine the LED bit mask based on present audio level
    uint8_t IOX_1_LED_BitMask = LED_ModeStatusBitMask | LED_AudioBarGraphCalculate(PCM16_Value);

    // STEP 2: Update the IO Expander that drives the LEDs
    MCP23S08_WriteOutput(&IOX_1, IOX_1_LED_BitMask);

} // END OF update_LED_AudioBarGraph



/********************************************************************************************************
* @brief Update the audio timer display
*
* @author original: Hab Collector \n
*
* @param ISR_PlayBackTicks: Ticks of the audio playback ISR - Time is counted in ticks
* @param PlaybackRate: The playback rate of the audio - frequency of the audio playback ISR
*
* @return True if OK - false on invalid playback rate of 0Hz
*
* STEP 1: Determine the LED bit mask based on present audio level
* STEP 2: Update the IO Expander that drives the LEDs
********************************************************************************************************/
static bool updateDisplayPlaybackTimer(uint32_t ISR_PlayBackTicks, uint32_t PlaybackRate)
{
    // STEP 1: Simple check 
    if (PlaybackRate == 0)
        return(false);
    
    // STEP 2: Calculate time in seconds and upate the dispaly
    uint32_t TimeInSeconds = (uint32_t)((1.0 / (float)PlaybackRate) * ISR_PlayBackTicks);
    displayUpdateAudioPlaybackTime(&Display_SSD1309, TimeInSeconds);

    return(true);

} // END OF updateDisplayPlaybackTimer


















