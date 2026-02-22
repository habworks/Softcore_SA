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
#include "xgpio.h"
#include "ff.h"
#include <stdio.h>


static bool feedStream_PCM16_WAV(Type_Audio_SA *Audio_SA, Type_FFT *FFT);
static void errorCloseAudioFile(Type_Audio_SA *Audio_SA, char *ErrorMsg);
static int16_t convert_PCM16_ToMono(int16_t Left_PCM16_Audio, int16_t Right_PCM16_Audion);
static uint16_t convert_PCM16_To_PWM_DutyPercent(int16_t PCM16_Sample, uint16_t PercentBase);
// static void load_FFT_PWM_ToBuffers(Type_Audio_SA *Audio_SA, Type_FFT *FFT);
static void apply_FFT_Window(Type_Audio_SA *Audio_SA, Type_FFT *FFT);


uint8_t __attribute__ ((section (".Hab_Fast_Data"))) RawBuffer[MAX_RAW_BUFFER];
bool __attribute__ ((section (".Hab_Fast_Data"))) *FFT_FrameReadyPtr;
Type_int16_t_CircularBuffer __attribute__ ((section (".Hab_Fast_Data"))) *Samples_CB_Ptr;
Type_AudioAction __attribute__ ((section (".Hab_Fast_Data"))) *AudioActionPtr;
float __attribute__ ((section (".Hab_Fast_Data"))) *FFT_SamplesPtr;



void audioSpectrumAnalyzer(Type_Audio_SA *Audio_SA, Type_FFT *FFT)
{
    // STEP 1: Audio play and spectrum qualifier
    if ((!Audio_SA->Enable) || (Audio_SA->AudioAction != AUDIO_ACTION_PLAY))
        return;

    // STEP 2: Stream the audio from the uSD - It is played in the audio ISR
    XGpio_DiscreteSet(&AXI_GPIO_Handle, GPIO_OUTPUT_CHANNEL, TEST_IO_0);
    feedStream_PCM16_WAV(Audio_SA, FFT);
    XGpio_DiscreteClear(&AXI_GPIO_Handle, GPIO_OUTPUT_CHANNEL, TEST_IO_0);  

    // STEP 3: When FFT size number of samples have been captured update the display
    if (FFT->FrameReady)
    {      
        // apply_FFT_Window(Audio_SA, FFT);
        // displayAudioSpectrum(FFT)
        FFT->FrameReady = false;
    }    
}



/********************************************************************************************************
* @brief Services a 16-bit PCM WAV audio stream by sequentially reading audio data from a WAV file and
* loading decoded samples into a circular buffer for FFT processing.  This function is intended to be
* called repeatedly until EOF or the action has been stopped externaly.  Each call advances the stream only 
* as far as circular buffer has room avilable.
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
*
* @param Audio_SA: Pointer to audio spectrum analyzer control structure
* @param FFT: Pointer to FFT structure
*
* @return true if operation is successful or no action is required
* @return false if a file or buffer initialization error occurs
*
* STEP 1: Verify Audio_SA is enabled and open WAV file on first use
* STEP 2: Seek to WAV data offset if raw buffer is empty
* STEP 3: Read a chunk of raw audio data from the WAV file into the raw buffer
* STEP 4: Check circular buffer has sufficient unused elements for FFT_SIZE samples
* STEP 5: Decode PCM16 samples and convert stereo to mono if required, then load circular buffer
* STEP 6: Update raw buffer and file read offsets
* STEP 7: Detect end of file and close WAV file when complete
********************************************************************************************************/
static bool feedStream_PCM16_WAV(Type_Audio_SA *Audio_SA, Type_FFT *FFT)
{
    static uint32_t FileSeekOffset = 0;
    static uint32_t BytesToReadFromFile = 0;
    static uint32_t RawBufferBytesToRead = 0;
    uint32_t BytesLastReadFromFile = 0;
    uint32_t RawBufferFreeSpace = 0;
    FRESULT FileStatus = FR_OK;
    FIL *FileHandle = &Audio_SA->File.FileHandle;
    bool Status;

    
    // STEP 1: Open file in read-only mode, set conditions to begin loading, and init CBs for use
    if (!Audio_SA->File.IsOpen)
    {
        if (f_open(FileHandle, Audio_SA->File.PathFileName, FA_READ) != FR_OK)
        {
            errorCloseAudioFile(Audio_SA, "Fail to open WAV file");
            return(false);
        }
        else
        {
            Audio_SA->File.IsOpen = true;
            Audio_SA->File.Is_EOF = false;
            Audio_SA->IsPreLoadComplete = false;
            FileSeekOffset = WAV_DATA_OFFSET;
            BytesToReadFromFile = Audio_SA->File.Header.DataSize;
            if (!init_I16_CB(&Audio_SA->Samples_CB, (1024 * 4)))
            {
                errorCloseAudioFile(Audio_SA, "Faill to allocate sample CB memory");
                return(false);
            }
            if (!init_U8_CB(&Audio_SA->Raw_CB, MAX_RAW_BUFFER))
            {
                errorCloseAudioFile(Audio_SA, "Fail to allocate raw CB memory");
                return(false);
            }
        }
    }

    // STEP 2: If sample CB below watermark percentage full then look to fill it - if above do nothing
    uint32_t AvailableSampleWrites = availableWrites_I16_CB(&Audio_SA->Samples_CB);
    if (AvailableSampleWrites >= (512) || (Audio_SA->IsPreLoadComplete == false))
    {
        uint32_t SampleBufferBytesToFill = AvailableSampleWrites * sizeof(int16_t);
        uint32_t AvailableRawBufferBytes = availableWrites_U8_CB(&Audio_SA->Raw_CB);
        if (Audio_SA->File.Header.ChannelNumber == MONO)
            AvailableRawBufferBytes = (AvailableRawBufferBytes/2) * 2;
        else
            AvailableRawBufferBytes = (AvailableRawBufferBytes/4) * 4;
        if (SampleBufferBytesToFill >= AvailableRawBufferBytes)
            RawBufferFreeSpace = AvailableRawBufferBytes;
        else
            RawBufferFreeSpace = SampleBufferBytesToFill; 
    }
    else
    {
         return(true);       
    }

    // STEP 3: Seek to the WAV file loaction for next read
    if ((FileStatus = f_lseek(FileHandle, FileSeekOffset)) != FR_OK)
    {
        errorCloseAudioFile(Audio_SA, "Fail file seek");
        return(false);
    }

    // STEP 4: Read from file the amount of bytes that will fill the raw buffers
    if (f_read(FileHandle, RawBuffer, RawBufferFreeSpace, &BytesLastReadFromFile) != FR_OK)
    {
        errorCloseAudioFile(Audio_SA, "Fail file read");
        return(false);
    }
    else 
    {
        // Copy linear buffer to CB, update bytes read from file and the file seek offset
        Status = writeBufferTo_U8_CB(&Audio_SA->Raw_CB, RawBuffer, BytesLastReadFromFile);         
        if (Status == false)
        {
            errorCloseAudioFile(Audio_SA, "Failed to copy raw");
            return(false);
        }
        BytesToReadFromFile -= BytesLastReadFromFile;
        FileSeekOffset += BytesLastReadFromFile;
    }

    // STEP 5: Load the sample CB from the raw CB mono or stero specific
    uint32_t BytesPerFrame = (Audio_SA->File.Header.ChannelNumber == MONO) ? 2U : 4U;
    while (!isFull_I16_CB(&Audio_SA->Samples_CB) && (!isEmpty_U8_CB(&Audio_SA->Raw_CB)) && (availableReads_U8_CB(&Audio_SA->Raw_CB) >= BytesPerFrame))
    {
        // In Mono you make two reads for left channel a single signed 16b value
        if (Audio_SA->File.Header.ChannelNumber == MONO)
        {
            Type_Union_PCM_AudioValue PCM_LeftAudioValue;
            read_U8_CB(&Audio_SA->Raw_CB, &PCM_LeftAudioValue.ByteValue[LSB], NULL, NULL);
            read_U8_CB(&Audio_SA->Raw_CB, &PCM_LeftAudioValue.ByteValue[MSB], NULL, NULL);
            write_I16_CB(&Audio_SA->Samples_CB, PCM_LeftAudioValue.Signed16Bit_Value);
        }
        // In Stero you make 4 reads for left and right channel signed 16b value
        if (Audio_SA->File.Header.ChannelNumber == STEREO)
        {
            Type_Union_PCM_AudioValue PCM_LeftAudioValue;
            Type_Union_PCM_AudioValue PCM_RightAudioValue;
            read_U8_CB(&Audio_SA->Raw_CB, &PCM_LeftAudioValue.ByteValue[LSB], NULL, NULL);
            read_U8_CB(&Audio_SA->Raw_CB, &PCM_LeftAudioValue.ByteValue[MSB], NULL, NULL);
            read_U8_CB(&Audio_SA->Raw_CB, &PCM_RightAudioValue.ByteValue[LSB], NULL, NULL);
            read_U8_CB(&Audio_SA->Raw_CB, &PCM_RightAudioValue.ByteValue[MSB], NULL, NULL);
            int16_t Value_PCM16 = convert_PCM16_ToMono(PCM_LeftAudioValue.Signed16Bit_Value, PCM_RightAudioValue.Signed16Bit_Value);
            write_I16_CB(&Audio_SA->Samples_CB, Value_PCM16);              
        }
    }

    // STEP X: Preload is complete when the sample buffer is filled for the first time - start audio playback
    if (!Audio_SA->IsPreLoadComplete && isFull_I16_CB(&Audio_SA->Samples_CB))
    {
        // Enable the PWM output
        enable_PWM(&AXI_PWM_Handle);
        setup_PWM(&AXI_PWM_Handle, AUDIO_PWM_FREQUENCY, AUDIO_PWM_DEFAULT_DUTY);
        // Enable Audio 
        audioEnable(true);
        // Enable the audio playback ISR
        resumeSpecificIRQ(&AXI_IRQ_ControllerHandle, AUDIO_TIMER_IRQ_ID);
        Audio_SA->IsPreLoadComplete = true;
    }

    // STEP X: Check if all data has been read from file - this would conclude playback
    if ((RawBufferBytesToRead == 0) && (BytesToReadFromFile == 0))
    {
        // Handle close out action
        f_close(FileHandle);
        Audio_SA->File.IsOpen = false;
        free_U8_CB(&Audio_SA->Raw_CB);
        free_I16_CB(&Audio_SA->Samples_CB);
        Audio_SA->AudioAction = AUDIO_ACTION_STOP;
        // Audio turn off occurs in ISR
        Audio_SA->File.Is_EOF = true;        
    }
    
    return(true);

} // END OF feedStream_PCM16_WAV



/********************************************************************************************************
* @brief A serires of steps necessary when closing out the feedStream_PCM16_WAV due to an error condition
*
* @author original: Hab Collector \n
*
* @param Audio_SA: Pointer to Audio Spectrum Analyzer structure
* @param FileHandle: Pointer to the WAV file handle that maybe open
*
* STEP 1: Make preperations to leave feedStream_PCM16_WAV gracefully
********************************************************************************************************/
static void errorCloseAudioFile(Type_Audio_SA *Audio_SA, char *ErrorMsg)
{
    char PrintBuffer[100];
    // STEP 1: Make preperations to leave feedStream_PCM16_WAV gracefully
    stopAudio_SA(Audio_SA);
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
*
* @return PWM duty-cycle percentage (0.0 to 100.0)
*
* STEP 1: Offset signed PCM sample to an unsigned 16-bit range
* STEP 2: Clamp the value to the maximum 16-bit range
* STEP 3: Convert to a percentage of full-scale PWM 0 - 100%
********************************************************************************************************/
static uint16_t convert_PCM16_To_PWM_DutyPercent(int16_t PCM16_Sample, uint16_t PercentBase)
{
    // STEP 1: Offset signed PCM sample to an unsigned 16-bit range
    uint32_t PWM_Duty = (uint32_t)((int32_t)PCM16_Sample + 32768U);

    // STEP 2: Clamp the value to the maximum 16-bit range
    // if (PWM_Duty > 65535U)
    //     PWM_Duty = 65535U;

    // STEP 3: Convert to a percentage of full-scale PWM 0 - 100%    
    // float PWM_DutyPercent = 100.0 * ((float)PWM_Duty / 65535.0);
    uint16_t PWM_DutyPercent = (uint16_t)((PercentBase * PWM_Duty) / 65535U);
    return(PWM_DutyPercent);

} // END OF convert_PCM16_To_PWM_DutyPercent



/********************************************************************************************************
* @brief Load PCM audio samples from the circular buffer into FFT and PWM playback buffers
*
* @author original: Hab Collector \n
*
* @note: This function retrieves signed 16-bit PCM samples from the audio circular buffer
* @note: FFT samples remain signed and zero-centered for correct spectral analysis
* @note: PWM samples are converted to a duty-cycle percentage for audio playback
*
* @param Audio_SA: Pointer to Audio Spectrum Analyzer structure
* @param FFT: Pointer to the FFT structure
*
* STEP 1: Retrieve 16b PCM samples from the circular buffer
* STEP 2: Load the 16b PCM sample to the FFT Buffer (FFT math assumes positive and negative values)
* STEP 3: Load Converted PCM samples to PWM duty-cycle percentage for audio playback
********************************************************************************************************/
// static void load_FFT_PWM_ToBuffers(Type_Audio_SA *Audio_SA, Type_FFT *FFT)
// {
//     int16_t AudioSample;
//     bool Half_Full;
//     bool Half_Empty;    
//     for (uint16_t Index = 0; Index < FFT->Size; Index++)
//     {
//         // STEP 1: Retrieve 16b PCM samples from the circular buffer
//         read_CB(&Audio_SA->Samples_CB, &AudioSample, &Half_Empty, &Half_Full);
        
//         // STEP 2: Load the 16b PCM sample to the FFT Buffer (FFT math assumes positive and negative values)
//         FFT->Samples[Index] = (float)AudioSample;

//         // STEP 3: Load Converted PCM samples to PWM duty-cycle percentage for audio playback
//         Audio_SA->PWM.Samples[Index] = convert_PCM16_To_PWM_DutyPercent(AudioSample);
//     }

// } // END OF load_FFT_PWM_ToBuffers



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



void stopAudio_SA(Type_Audio_SA *Audio_SA)
{
    Audio_SA->AudioAction = AUDIO_ACTION_STOP;
    
    pauseSpecificIRQ(&AXI_IRQ_ControllerHandle, AUDIO_TIMER_IRQ_ID);
    audioEnable(false);
    disable_PWM(&AXI_PWM_Handle);
    
    f_close(&Audio_SA->File.FileHandle);
    Audio_SA->File.IsOpen = false;
    free_I16_CB(&Audio_SA->Samples_CB);
    free_U8_CB(&Audio_SA->Raw_CB);
}

void playAudio_SA(Type_Audio_SA *Audio_SA, Type_FFT *FFT)
{
    if (!Audio_SA->Enable)
        return;

    Audio_SA->AudioAction = AUDIO_ACTION_PLAY;
    Audio_SA->IsPreLoadComplete = false;
    f_close(&Audio_SA->File.FileHandle);    // File should not be open - but just in case
    FFT->FrameReady = false;
    FFT->RBW = Audio_SA->File.Header.SampleRate / FFT->Size;
}

void audioEnable(bool Enable)
{
    if (Enable)
        XGpio_DiscreteSet(&AXI_GPIO_Handle, GPIO_OUTPUT_CHANNEL, AUDIO_EN);
    else
        XGpio_DiscreteClear(&AXI_GPIO_Handle, GPIO_OUTPUT_CHANNEL, AUDIO_EN);
}


void audioPeriodicTimer_ISR(Type_Audio_SA *Audio_SA, Type_FFT *FFT)
{
    static uint16_t SampleIndex = 0;
    static int16_t PreviousSampleValue = 0;

    // STEP 1: Simple test 
    if (Audio_SA->AudioAction != AUDIO_ACTION_PLAY)
        return;
    
    if (isEmpty_I16_CB(&Audio_SA->Samples_CB) && (Audio_SA->File.Is_EOF == true))
    {
        pauseSpecificIRQ(&AXI_IRQ_ControllerHandle, AUDIO_TIMER_IRQ_ID);
        audioEnable(false);
        disable_PWM(&AXI_PWM_Handle);
        return;
    }
    
    if (isEmpty_I16_CB(&Audio_SA->Samples_CB))
        return;

    int16_t SampleValue;
    uint16_t PWM_DutyCycle;
    read_I16_CB(&Audio_SA->Samples_CB, &SampleValue, NULL, NULL);
    // FFT_SamplesPtr[SampleIndex] = SampleValue;
    if (SampleValue != PreviousSampleValue)
    {
        PWM_DutyCycle = convert_PCM16_To_PWM_DutyPercent(SampleValue, 1024);
        update_PWM_Duty_Fast(&AXI_PWM_Handle, PWM_DutyCycle);
        PreviousSampleValue = SampleValue;
    }
    SampleIndex++;
    if (SampleIndex == FFT_SIZE)
    {
        SampleIndex = 0;
        FFT->FrameReady = true;
    }
}