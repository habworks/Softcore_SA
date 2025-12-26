/******************************************************************************************************
 * @file            Audio_File_API.c
 * @brief           A collection of functions relevant to supporting Audio (WAV only) files
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
#include "ffconf.h"


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
    static DIR Directory;
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
                strncpy(NextWavFileName, FileName, strlen(FileName));
                NextWavFileName[strlen(FileName)] = 0;
                *NextWavFileSize = FileInfo.fsize;
                buildPathFileName(NextWavPathFileName, DirectoryPath, NextWavFileName);
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
* @param WavFileName: Path and name of WAV file relative to root
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
bool getWavFileHeader(char *WavFileName, uint32_t WavFileSize, Type_WavHeader *WavHeader)
{
    FIL FileHandle;
    UINT BytesRead;

    // STEP 1: Verify minimum file size
    if (WavFileSize < sizeof(Type_WavHeader))
        return(false);

    // STEP 2: Open WAV file
    if (f_open(&FileHandle, WavFileName, FA_READ) != FR_OK)
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
* @brief Creates a DirPath\FileName from Directory and FileName
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
void buildPathFileName(char*PathFileName, const char *DirectoryPath, char *FileName)
{
    // STEP 1: Combine to DirectoryPath\FileName
    strcpy(PathFileName, DirectoryPath);
    strcat(PathFileName, "/");
    strcat(PathFileName, FileName);

} // END OF buildPathFileName
