/******************************************************************************************************
 * @file            Audio_File_API.c
 * @brief           A collection of functions relevant to supporting Audio (WAV only) files
 *                  This audio file uses the Elm Chan FAT FS R0.16 specifically for drive and file access
 *                  other versions of FAT FS may work, but it was designed based on R0.16
 *                  https://elm-chan.org/fsw/ff/
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

#include "Audio_File_API.h"
#include <string.h>
#include <stdlib.h>
#include "ffconf.h"

DIR Directory;

/********************************************************************************************************
* @brief Returns the next file name and size (by reference) of the specified directory.  Next means first
* if this is the first time the function is called.  If the last file is reached the next call will rewind
* back to the first entry.
*
* @author original: Hab Collector \n
*
* @note: Requires prior init of FAT FS
* 
* @param DirectoryPath: Path in which to look for files
* @param NextWavFileName: Next wav file name (NULL terminated) - returned by reference does not include path
* @param NextWavFileSize: Next wav file size - returned by reference
* @param FileCount: Total number of files in directory must be known for this function to work
*
* @return FR_OK if successful or a file specific error if not
*
* STEP 1: Check for errors and open directory
* STEP 2: Determine if extension is wav file
********************************************************************************************************/
FRESULT getNextWavFile(const char *DirectoryPath, char *NextWavFileName, char *NextWavPathFileName, uint32_t *NextWavFileSize, uint16_t FileCount)
{
    static FILINFO FileInfo;
    static bool IsDirectoryOpen = false;
    FRESULT FileResult;
    uint16_t TotalFilesScaned = 0;

    // STEP 1: Check for errors and open directory
    if (FileCount == 0)
        return(FR_NO_FILE);
    // Open directory if it is present - if not return the file error
    if (!IsDirectoryOpen)
    {
        FileResult = f_opendir(&Directory, DirectoryPath);
        if (FileResult != FR_OK)
        {
            return(FileResult);
        }
        IsDirectoryOpen = true;
    }

    // STEP 2: Find Files - if none return file error - if end of diectory start search from begininig - if found check if .wav
    do
    {
        FileResult = f_readdir(&Directory, &FileInfo);
        // If no file info then directory is empty - return file error
        if (FileResult != FR_OK)
            return(FileResult);

        // If end of directory → rewind and continue
        if (FileInfo.fname[0] == 0)
        {
            f_rewinddir(&Directory);
            continue;
        }

        // Extract the file name
        const char *FileName = FileInfo.fname;

        // Only files not directories if valid wav file return null terminated name and file size
        if (!(FileInfo.fattrib & AM_DIR))
        {
            TotalFilesScaned++;
            if (isWavFile(FileName))
            {
                memset(NextWavFileName, 0x00, MAX_FILE_NAME_LENGTH);
                strcpy(NextWavFileName, FileName);
                buildPathFileName(NextWavPathFileName, DirectoryPath, NextWavFileName);
                *NextWavFileSize = FileInfo.fsize;
                return(FR_OK);
            }
        }
    } while(TotalFilesScaned < FileCount);

    // No valid WAV file found
    return(FR_NO_FILE);

} // END OF getNextWavFile



/********************************************************************************************************
* @brief Count the number of files only (not directories) - files can be any extension
*
* @author original: Hab Collector \n
*
* @note: This function closes the directory upon exit
* @note: Requires prior init of FAT FS
* 
* @param DirectoryPath: Path of directory to count - relative from root - returned by reference
* @param FileCount: Number of total files returned by reference 
*
* @return FR_OK if successful or a file specific error if not
*
* STEP 1: Open the directory
* STEP 2: Count the number of files
********************************************************************************************************/
FRESULT countFilesInDirectory(const char *DirectoryPath, uint16_t *FileCount)
{
    DIR Directory;
    FILINFO FileInfo;
    FRESULT FileResult;

    *FileCount = 0;

    // STEP 1: Open the directory
    FileResult = f_opendir(&Directory, DirectoryPath);
    if (FileResult != FR_OK)
        return(FileResult);

    // STEP 2: Count the number of files
    while (1)
    {
        FileResult = f_readdir(&Directory, &FileInfo);
        if ((FileResult != FR_OK) || (FileInfo.fname[0] == 0))
        {
            break;   // error or end of directory
        }
        // If not a directory count as file
        if (!(FileInfo.fattrib & AM_DIR))
        {
            *FileCount = *FileCount + 1;
        }
    }

    // STEP 3: Close the directory and return as OK
    f_closedir(&Directory);
    return(FR_OK);

} // END OF countFilesInDirectory



/********************************************************************************************************
* @brief Verifies, by file extsion only if the file is a WAV audio file (.wav, .WAV, .Wav are acceptable)
*
* @author original: Hab Collector \n
*
* @note: Requires prior init of FAT FS
* 
* @param FileName: Path of directory to count - relative from root
* @param FileCount: Number of total files returned by reference 
*
* @return True if file is WAV file
*
* STEP 1: Determine if file name has an extionsion
* STEP 2: Determine if extension is wav file
********************************************************************************************************/
bool isWavFile(const char *FileName)
{
    // STEP 1: Determine if file name has an extionsion
    const char *Ext = strrchr(FileName, '.');
    if (Ext == NULL)
        return(false);

    // STEP 2: Determine if extension is wav file
    if ((strstr(FileName, ".wav") != NULL) || (strstr(FileName, ".WAV") != NULL) || (strstr(FileName, ".Wav") != NULL))
        return(true);
    else
        return(false);

} // END OF isWavFile



/********************************************************************************************************
* @brief Reads and validates a PCM WAV file header
*
* @author original: Hab Collector \n
*
* @note: Requires prior init of FAT FS
*        Assumes standard PCM WAV header (44 bytes)
*
* @param WavPathFileName: Path and name of WAV file relative to root
* @param WavFileSize: Size of WAV file in bytes
* @param WavHeader: Pointer to WAV header structure filled by reference
*
* @return True if WAV header is valid and meets required conditions
*
* STEP 1: Verify file size is large enough to contain a WAV header
* STEP 2: Open file and extract WAV header information
* STEP 3: Extract header information
* STEP 4: Validate required WAV header fields
********************************************************************************************************/
bool getWavFileHeader(char *WavPathFileName, uint32_t WavFileSize, Type_WavHeader *WavHeader)
{
    FIL FileHandle;
    UINT BytesRead;

    // STEP 1: Verify minimum file size
    if (WavFileSize < sizeof(Type_WavHeader))
        return(false);

    // STEP 2: Open WAV file
    if (f_open(&FileHandle, WavPathFileName, FA_READ) != FR_OK)
        return(false);

    // STEP 3: Extract header information
    if (f_read(&FileHandle, WavHeader, sizeof(Type_WavHeader), &BytesRead) != FR_OK)
    {
        f_close(&FileHandle);
        return(false);
    }
    f_close(&FileHandle);

    // STEP 4: Validate required WAV header fields
    // Number of bytes in file
    if (BytesRead != sizeof(Type_WavHeader))
        return(false);
    // Must be RIFF format
    if (memcmp(WavHeader->RiffChunkID, RIFF_FILE_TYPE, 4) != 0)
        return(false);
    // Must be WAV Audio
    if (memcmp(WavHeader->RiffType, WAVE_RIFF_TYPE, 4) != 0)
        return(false);
    // Expected for PCM audio
    if (WavHeader->FormatChunkSize != WAVE_CHUNK_SIZE)
        return(false);
    // Must be no compression
    if (WavHeader->Compression != COMPRESSION_NONE)
        return(false);
    // Only supporting 16 bit samples at this time
    if (WavHeader->BitsPerSample != PCM_16_BIT_SIGNED)
        return(false);

    return(true);
}



/********************************************************************************************************
* @brief Creates a DirPath\FileName from Directory and FileName relative to root.  Example: "0:/AUDIO/FileName.Wave
*
* @author original: Hab Collector \n
*
* @note: PathFileName must be previously allocateed and large enough to hold the entire Path\FileName
*
* @param PathFileName: Path and File Name - returned by reference
* @param DirectoryPath: Directory path
* @param FileName: File name
*
* STEP 1: Combine to DirectoryPath\FileName
********************************************************************************************************/
void buildPathFileName(char *PathFileName, const char *DirectoryPath, char *FileName)
{
    // STEP 1: Combine to DirectoryPath\FileName
    memset(PathFileName, 0x00, MAX_PATH_FILE_LENGTH);
    strcpy(PathFileName, ROOT_PATH);
    strcat(PathFileName, DirectoryPath);
    strcat(PathFileName, "/");
    strncat(PathFileName, FileName, strlen(FileName));

} // END OF buildPathFileName



/********************************************************************************************************
* @brief Converts a 16-bit signed PCM audio sample into a PWM duty-cycle percentage
*
* @author original: Hab Collector \n
*
* @note: Assumes 16-bit signed PCM audio (range -32768 to +32767)
*        Output is a normalized PWM percentage independent of timer resolution
*
* @param PcmSample: Signed 16-bit PCM audio sample
*
* @return PWM duty cycle as a percentage (0.0 to 100.0)
*         0.0  = signal fully low for entire period
*         50.0 = silence (midpoint)
*         100.0 = signal fully high for entire period
*
* STEP 1: Shift signed PCM sample to unsigned domain
* STEP 2: Normalize to a 0.0 to 1.0 range as a percentage (0 to 100%)
********************************************************************************************************/
float pcm16ToPwmPercent(int16_t PcmSample)
{
    float NormalizedValue;
    float PWM_Percent;

    // STEP 1: Shift signed PCM (-32768..32767) to unsigned range (0..65535)
    NormalizedValue = (float)(PcmSample + 32768) / 65535.0f;

    // STEP 2: Normalize to a 0.0 to 1.0 range as a percentage (0 to 100%)
    PWM_Percent = NormalizedValue * 100.0f;
    return(PWM_Percent);
}



/********************************************************************************************************
* @brief Initializes a circular buffer
*
* @author original: Hab Collector \n
*
* @note: Allocates memory for circular buffer storage
*
* @param CircularBuffer: Pointer to circular buffer structure
* @param Size: Maximum number of elements to store
*
* @return None
*
* STEP 1: Set start and end of buffer to 0 and assign size
* STEP 2: Allocate memory for circular buffer elements
********************************************************************************************************/
bool init_CB(Type_int16_t_CircularBuffer *CircularBuffer, uint16_t Size)
{
    // STEP 1: Set start and end of buffer to 0 and assign size
    CircularBuffer->Size  = Size + 1; // INCLUDES AN EMPTY LOCATION
    CircularBuffer->Start = 0;
    CircularBuffer->End   = 0;

    // STEP 2: Allocate memory for circular buffer elements
    CircularBuffer->Elements = (int16_t *)calloc(CircularBuffer->Size, sizeof(int16_t));
    if (CircularBuffer->Elements != NULL)
        return(true);
    else
        return(false);

} // END OF init_CB



/********************************************************************************************************
* @brief Frees memory allocated for a circular buffer
*
* @author original: Hab Collector \n
*
* @note: Safe to call with NULL pointer
*
* @param CircularBuffer: Pointer to circular buffer structure
*
* @return None
*
* STEP 1: Free allocated circular buffer memory
********************************************************************************************************/
void free_CB(Type_int16_t_CircularBuffer *CircularBuffer)
{
    // STEP 1: Free allocated circular buffer memory
    // OK IF NULL
    free(CircularBuffer->Elements);

} // END OF free_CB



/********************************************************************************************************
* @brief Checks if the circular buffer is full
*
* @author original: Hab Collector \n
*
* @param CircularBuffer: Pointer to circular buffer structure
*
* @return True if buffer is full, else false
*
* STEP 1: Determine if buffer is full
********************************************************************************************************/
bool isFull_CB(Type_int16_t_CircularBuffer *CircularBuffer)
{
    // STEP 1: Determine if buffer is full
    return((CircularBuffer->End + 1) % CircularBuffer->Size == CircularBuffer->Start);
}



/********************************************************************************************************
* @brief Checks if the circular buffer is empty
*
* @author original: Hab Collector \n
*
* @param CircularBuffer: Pointer to circular buffer structure
*
* @return True if buffer is empty, else false
*
* STEP 1: Determine if buffer is empty
********************************************************************************************************/
bool isEmpty_CB(Type_int16_t_CircularBuffer *CircularBuffer)
{
    // STEP 1: Determine if buffer is empty
    return(CircularBuffer->End == CircularBuffer->Start);
} // END OF isEmpty_CB



/********************************************************************************************************
* @brief Writes a single element to the circular buffer
*
* @author original: Hab Collector \n
*
* @note: Overwrites oldest element if buffer is full
*        Caller may check isFull_CB() to prevent overwrite
*
* @param CircularBuffer: Pointer to circular buffer structure
* @param Element: Pointer to element to store
*
* @return True if write OK, false if buffer is full
*
* STEP 1: Do not overwrite
* STEP 2: Store element at end index
* STEP 3: Advance end index 
********************************************************************************************************/
bool write_CB(Type_int16_t_CircularBuffer *CircularBuffer, int16_t *Element)
{
    // STEP 1: Do not overwrite
    if (isFull_CB(CircularBuffer))
        return(false);

    // STEP 2: Store element at end index
    CircularBuffer->Elements[CircularBuffer->End] = *Element;

    // STEP 3: Advance end index
    CircularBuffer->End = (CircularBuffer->End + 1) % CircularBuffer->Size;

    return(true);

} // END OF write_CB



/********************************************************************************************************
* @brief Reads a single element from the circular buffer and reports buffer fullness state
*
* @author original: Hab Collector \n
*
* @note: Function will not read from the buffer if it is empty
*        Return value reflects success or failure of read operation
*        Buffer fullness indicators reflect state after the read
*
* @param CircularBuffer: Pointer to circular buffer structure
* @param Element: Pointer to element to receive data
* @param CB_Half_Empty: Pointer set true if buffer is half or more empty
* @param CB_Half_Full: Pointer set true if buffer is more than half full
*
* @return True if element was read successfully
*         False if buffer was empty and no read occurred
*
* STEP 1: Verify circular buffer is not empty
* STEP 2: Read element from start index
* STEP 3: Advance start index
* STEP 4: Determine if buffer is half or more empty
* STEP 5: Set half-empty and half-full indicators
********************************************************************************************************/
bool read_CB(Type_int16_t_CircularBuffer *CircularBuffer, int16_t *Element, bool *CB_Half_Empty, bool *CB_Half_Full)
{
    uint16_t Count;
    uint16_t Capacity;

    // STEP 1: Verify circular buffer is not empty
    if (isEmpty_CB(CircularBuffer))
        return(false);

    // STEP 2: Read element from start index
    *Element = CircularBuffer->Elements[CircularBuffer->Start];

    // STEP 3: Advance start index
    CircularBuffer->Start = (CircularBuffer->Start + 1) % CircularBuffer->Size;

    // STEP 4: Determine if buffer is half or more empty
    Capacity = CircularBuffer->Size - 1;

    if (CircularBuffer->End >= CircularBuffer->Start)
        Count = CircularBuffer->End - CircularBuffer->Start;
    else
        Count = CircularBuffer->Size - (CircularBuffer->Start - CircularBuffer->End);

    // STEP 5: Set half-empty and half-full indicators
    *CB_Half_Empty = (Count <= (Capacity / 2));
    *CB_Half_Full  = !(*CB_Half_Empty);

    return(true);

} // END OF read_CB



/********************************************************************************************************
* @brief Returns the number of free elements available for writing in the circular buffer
*
* @author original: Hab Collector \n
*
* @note: Buffer uses a one-empty-slot design to distinguish full vs empty
*        Returned value reflects how many additional writes can succeed
*
* @param CircularBuffer: Pointer to circular buffer structure
*
* @return Number of elements that can be written before buffer becomes full
*
* STEP 1: Determine usable buffer capacity
* STEP 2: Determine number of elements currently stored
* STEP 3: Calculate number of free elements available
********************************************************************************************************/
uint32_t unusedElements(Type_int16_t_CircularBuffer *CircularBuffer)
{
    uint16_t Used;
    uint16_t Capacity;

    // STEP 1: Determine usable buffer capacity
    Capacity = CircularBuffer->Size - 1;

    // STEP 2: Determine number of elements currently stored
    if (CircularBuffer->End >= CircularBuffer->Start)
        Used = CircularBuffer->End - CircularBuffer->Start;
    else
        Used = CircularBuffer->Size - (CircularBuffer->Start - CircularBuffer->End);

    // STEP 3: Calculate number of free elements available
    return(Capacity - Used);

} // END OF unusedElements






