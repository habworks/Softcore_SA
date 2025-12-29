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

#include "Softcore_Audio_SA.h"
#include "Main_Support.h"
#include "Hab_Types.h"
#include "ff.h"

static bool feedStream_PCM16_WAV(Type_Audio_SA *Audio_SA);
static void errorCloseFileAudio_SA(Type_Audio_SA *Audio_SA, FIL *FileHandle);
static int16_t convert_PCM16_ToMono(int16_t Left_PCM16_Audio, int16_t Right_PCM16_Audion);
static float convert_PCM16_To_PWM_DutyPercent(int16_t PCM16_Sample);
static void load_FFT_PWM_ToBuffers(Type_Audio_SA *Audio_SA);
static void apply_FFT_Window(Type_Audio_SA *Audio_SA);



uint8_t RawBuffer[MAX_CHUNK_BUFFER];

void audioSpectrumAnalyzer(Type_Audio_SA *Audio_SA)
{
    if (!Audio_SA->Enable)
        return;
    feedStream_PCM16_WAV(Audio_SA);
    if (Audio_SA->FFT.FrameReady)
    {
        load_FFT_PWM_ToBuffers(Audio_SA);        
        apply_FFT_Window(Audio_SA);
        Audio_SA->FFT.FrameReady = false;
    }

}



/********************************************************************************************************
* @brief Services a 16-bit PCM WAV audio stream by incrementally reading audio data from a file and
* loading decoded samples into a circular buffer for FFT processing.  This function is intended to be
* called repeatedly.  Each call advances the stream only as far as system conditions allow.
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
*
*   Byte 0,1: Left channel sample (PCM16)
*   Byte 2,3: Right channel sample (PCM16)
*
* For each stereo frame, both left and right samples are read, converted to signed 16-bit
* values, and down-mixed to mono by averaging the two channels before being written to the
* circular buffer.
*
* @param Audio_SA: Pointer to audio spectrum analyzer control structure
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
static bool feedStream_PCM16_WAV(Type_Audio_SA *Audio_SA)
{
    static FIL FileHandle;
    static uint32_t RawIndexOffset = 0;
    static uint32_t BytesLastReadFromFile = 0;
    static uint32_t SeekOffset = 0;
    static uint32_t BytesToReadFromFile = 0;
    static uint32_t BytesToReadFromRawBuffer = 0;
    
    // STEP 1: Open file in read-only mode and set conditions to begin loading the CB
    if (!Audio_SA->File.IsOpen)
    {
        if (f_open(&FileHandle, Audio_SA->File.PathFileName, FA_READ) != FR_OK)
        {
            errorCloseFileAudio_SA(Audio_SA, &FileHandle);
            return(false);
        }
        else
        {
            Audio_SA->File.IsOpen = true;
            Audio_SA->IsFirstRead = true;
            Audio_SA->IsRawBufferEmpty = true;
            SeekOffset = WAV_DATA_OFFSET;
            RawIndexOffset = 0;
            BytesToReadFromFile = Audio_SA->File.Size;
            if (!init_CB(&Audio_SA->CircularBuffer, (Audio_SA->FFT.Size * 2)))
            {
                errorCloseFileAudio_SA(Audio_SA, &FileHandle);
                return(false);
            }
            // The first FFT Frame is made ready here - subsequent frames will be driven by the completion of the ISR PWM Buffer being empty
            Audio_SA->FFT.FrameReady = true;
        }
    }

    // STEP 2: Seek to WAV data offset
    if (Audio_SA->IsRawBufferEmpty)
    {
        if (f_lseek(&FileHandle, SeekOffset) != FR_OK)
        {
            f_close(&FileHandle);
            Audio_SA->File.IsOpen = false;
            return(false);
        }
    }

    // STEP 3: Read a chucnk of data into the raw buffer - the raw buffer feeds the CB
    if (Audio_SA->IsRawBufferEmpty)
    {
        if (f_read(&FileHandle, RawBuffer, sizeof(RawBuffer), &BytesLastReadFromFile) != FR_OK)
        {
            errorCloseFileAudio_SA(Audio_SA, &FileHandle);
            return(false);
        }
        else 
        {
            Audio_SA->IsRawBufferEmpty = false;
            BytesToReadFromRawBuffer = sizeof(RawBuffer);
            BytesToReadFromFile -= BytesLastReadFromFile;
        }
    }

    // STEP 4: Load CB Buffer only if there is room to do so
    if (unusedElements(&Audio_SA->CircularBuffer) < Audio_SA->FFT.Size)
        return(true);

    // STEP 5: Load CB Buffer with FFT Size number of samples as there is room - if stero convert to mono        
    uint32_t BytesReadFromRawBuffer = (Audio_SA->File.Header.ChannelNumber == MONO)? (Audio_SA->FFT.Size * 2) : (Audio_SA->FFT.Size * 4);
    uint8_t IndexIncrement = (Audio_SA->File.Header.ChannelNumber == MONO)? 2 : 4;
    Type_Union_PCM_AudioValue PCM_LeftAudioValue;
    Type_Union_PCM_AudioValue PCM_RightAudioValue;
    for(uint32_t Index = RawIndexOffset; Index < BytesReadFromRawBuffer; Index += IndexIncrement)
    {
        // In Mono you make two reads for left channel a single signed 16b value
        if (Audio_SA->File.Header.ChannelNumber == MONO)
        {
            PCM_LeftAudioValue.ByteValue[LSB] = RawBuffer[Index];
            PCM_LeftAudioValue.ByteValue[MSB] = RawBuffer[Index + 1];
            write_CB(&Audio_SA->CircularBuffer, PCM_LeftAudioValue.Signed16Bit_Value);
        }
        // In Stero you make 4 reads for left and right channel signed 16b value
        if (Audio_SA->File.Header.ChannelNumber == STEREO)
        {
            PCM_LeftAudioValue.ByteValue[LSB] = RawBuffer[Index];
            PCM_LeftAudioValue.ByteValue[MSB] = RawBuffer[Index + 1];
            PCM_RightAudioValue.ByteValue[LSB] = RawBuffer[Index + 2];
            PCM_RightAudioValue.ByteValue[MSB] = RawBuffer[Index + 3];
            write_CB(&Audio_SA->CircularBuffer, convert_PCM16_ToMono(PCM_LeftAudioValue.Signed16Bit_Value, PCM_RightAudioValue.Signed16Bit_Value));              
        }
    }

    // STEP 6: Calculate bytes read
    BytesToReadFromRawBuffer -= BytesReadFromRawBuffer;
    if (BytesToReadFromRawBuffer == 0)
    {
        Audio_SA->IsRawBufferEmpty = true;
        SeekOffset += BytesLastReadFromFile;
        RawIndexOffset += BytesReadFromRawBuffer;
    }

    // STEP 7: Check if full read of file is complete
    if ((BytesToReadFromRawBuffer == 0) && (BytesToReadFromFile == 0))
    {
        f_close(&FileHandle);
        Audio_SA->File.IsOpen = false;
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
static void errorCloseFileAudio_SA(Type_Audio_SA *Audio_SA, FIL *FileHandle)
{
    // STEP 1: Make preperations to leave feedStream_PCM16_WAV gracefully
    f_close(FileHandle);
    Audio_SA->File.IsOpen = false;
    free_CB(&Audio_SA->CircularBuffer);

} // END OF errorCloseFileAudio_SA



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
    int32_t MonoAudioSample = Left_PCM16_AudioSample +  Right_PCM16_AudioSample;
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
static float convert_PCM16_To_PWM_DutyPercent(int16_t PCM16_Sample)
{
    // STEP 1: Offset signed PCM sample to an unsigned 16-bit range
    uint32_t PWM_Duty = (uint32_t)((int32_t)PCM16_Sample + 32768);

    // STEP 2: Clamp the value to the maximum 16-bit range
    if (PWM_Duty > 65535)
        PWM_Duty = 65535;

    // STEP 3: Convert to a percentage of full-scale PWM 0 - 100%    
    float PWM_DutyPercent = 100.0 * ((float)PWM_Duty / 65535.0);
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
*
* STEP 1: Retrieve 16b PCM samples from the circular buffer
* STEP 2: Load the 16b PCM sample to the FFT Buffer (FFT math assumes positive and negative values)
* STEP 3: Load Converted PCM samples to PWM duty-cycle percentage for audio playback
********************************************************************************************************/
static void load_FFT_PWM_ToBuffers(Type_Audio_SA *Audio_SA)
{
    int16_t AudioSample;
    bool Half_Full;
    bool Half_Empty;    
    for (uint16_t Index = 0; Index < Audio_SA->FFT.Size; Index++)
    {
        // STEP 1: Retrieve 16b PCM samples from the circular buffer
        read_CB(&Audio_SA->CircularBuffer, &AudioSample, &Half_Empty, &Half_Full);
        
        // STEP 2: Load the 16b PCM sample to the FFT Buffer (FFT math assumes positive and negative values)
        Audio_SA->FFT.Samples[Index] = (float)AudioSample;

        // STEP 3: Load Converted PCM samples to PWM duty-cycle percentage for audio playback
        Audio_SA->PWM.Samples[Index] = convert_PCM16_To_PWM_DutyPercent(AudioSample);
    }

} // END OF load_FFT_PWM_ToBuffers



/********************************************************************************************************
* @brief Apply a precomputed Hann window to the FFT input sample buffer
*
* @author original: Hab Collector \n
*
* @note: Input samples in-place prior to executing the FFT
* @note: FFT samples must be signed and zero-centered
*
* @param Audio_SA: Pointer to Audio Spectrum Analyzer structure
*
* STEP 1: Apply FFT Hanning Window to FFT Samples store results in FFT Samples
********************************************************************************************************/
static void apply_FFT_Window(Type_Audio_SA *Audio_SA)
{
    // STEP 1: Apply FFT Hanning Window to FFT Samples store results in FFT Samples
    for (uint16_t Index = 0; Index < Audio_SA->FFT.Size; Index++)
    {
        Audio_SA->FFT.Samples[Index] *= Audio_SA->FFT.HannWindow[Index];
    }

} // END OF apply_FFT_Window
