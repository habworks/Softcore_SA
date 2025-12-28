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
 #include "ff.h"


bool read_PCM16_WAV(Type_AudioFile *AudioFile, Type_int16_t_CircularBuffer *CircularBuffer)
{
    FIL FileHandle;
    UINT BytesRead;
    

    // STEP 1: Open file in read-only mode
    if (f_open(&FileHandle, AudioFile->PathFileName, FA_READ) != FR_OK)
        return(false);

    // STEP 2: Seek to WAV data offset
    if (f_lseek(&FileHandle, WAV_DATA_OFFSET) != FR_OK)
    {
        f_close(&FileHandle);
        return(false);
    }

    // STEP 3: Load Buffer
    uint8_t RawBuffer[MAX_CHUNK_BUFFER];
    uint32_t BytesToReadFromFile = AudioFile->Size;
    Type_Union_PCM_AudioValue PCM_LeftAudioValue;
    Type_Union_PCM_AudioValue PCM_RightAudioValue;
    do 
    {
        if (f_read(&FileHandle, RawBuffer, sizeof(RawBuffer), &BytesRead) != FR_OK)
        {
            f_close(&FileHandle);
            return(false);
        }
        BytesToReadFromFile -= BytesRead;
        for (uint16_t Index = 0; Index < MAX_CHUNK_BUFFER; Index++)
        {
            if (AudioFile->Header.ChannelNumber == SINGLE_CHANNEL)
            {
                PCM_LeftAudioValue.ByteValue[LSB] = RawBuffer[Index++];
                PCM_LeftAudioValue.ByteValue[MSB] = RawBuffer[Index];
                write_CB(CircularBuffer, PCM_LeftAudioValue.Signed16Bit_Value);
                Index += 2;
            }
            if (AudioFile->Header.ChannelNumber == STEREO)
            {
                PCM_LeftAudioValue.ByteValue[LSB] = RawBuffer[Index++];
                PCM_LeftAudioValue.ByteValue[MSB] = RawBuffer[Index++];
                PCM_RightAudioValue.ByteValue[LSB] = RawBuffer[Index++];
                PCM_RightAudioValue.ByteValue[MSB] = RawBuffer[Index++];
                write_CB(CircularBuffer, convert_PCM16_ToMono(PCM_LeftAudioValue.Signed16Bit_Value, PCM_RightAudioValue.Signed16Bit_Value));
                Index += 4;                
            }
        }
    }while ((BytesRead == sizeof(RawBuffer)) && (BytesToReadFromFile != 0));

    f_close(&FileHandle);


}


int16_t convert_PCM16_ToMono(int16_t Left_PCM16_Audio, int16_t Right_PCM16_Audion)
{
    int32_t Mono = Left_PCM16_Audio +  Right_PCM16_Audion;
    Mono /= 2;
    return ((int16_t)Mono);
}