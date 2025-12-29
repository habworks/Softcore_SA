/******************************************************************************************************
 * @file            Audio_File_API.h
 * @brief           Header file to support Audio_File.c 
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

#ifndef AUDIO_FILE_API_H_
#define AUDIO_FILE_API_H_
#ifdef __cplusplus
extern"C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "ff.h"
#include "ffconf.h"

// DEFINES
// DIRECTORY
#define AUDIO_DIRECTORY     "AUDIO"
#define ROOT_PATH           "0:/"
// WAVE AUDIO OFFSETS - COMMENTS ARE FOR 16BIT PCM WAV AUDIO
#define RIFF_CHUNCK_OFFSET      0  // EXPECTED "RIFF"
#define RIFF_TYPE_OFFSET        8  // EXPECTED "WAVE"
#define FORMAT_CHUNCK_OFFSET   12  // EXPECTED "fmt "
#define FORMAT_SIZE_OFFSET     16  // EXPECTED 16 FOR PCM
#define COMPRESSION_OFFSET     20  // EXPECTED 1 FOR PCM (uncompressed)
#define CHANNEL_NUMBER_OFFSET  22  // 1 OR 2
#define SAMPLE_RATE_OFFSET     24  // 8000, 44100, etc.
#define BYTE_RATE_OFFSET       28  // SampleRate * NumChannels * BitsPerSample/8
#define BLOCK_ALIGN_OFFSET     32  // NumChannels * BitsPerSample/8
#define BIT_PER_SAMPLE_OFFSET  34  // 8 bits = 8, 16 bits = 16
#define DATA_CHUNCK_OFFSET     36  // EXPECTED "data"
#define DATA_SIZE_OFFSET       40  // NumSamples * NumChannels * BitsPerSample/8
#define WAV_DATA_OFFSET        44  // LEFT IS THE FIST CHANNEL READ
// WAVE AUDIO CHUNK NAMES
#define RIFF_FILE_TYPE          "RIFF"
#define WAVE_RIFF_TYPE          "WAVE"
/*
For standard PCM WAV files, FormatChunkSize is always 16 bytes.
Those 16 bytes are composed of the following fields:

Field                         Size (bytes)
----------------------------  ------------
AudioFormat (Compression)     2
NumChannels                   2
SampleRate                    4
ByteRate                      4
BlockAlign                    2
BitsPerSample                 2
----------------------------  ------------
Total                         16 bytes
*/
#define WAVE_CHUNK_SIZE         16
// FILE SIZE
#if defined FF_MAX_LFN == 1
#define MAX_FILE_NAME_LENGTH    255U
#define MAX_PATH_FILE_LENGTH    255U
#else
#define MAX_FILE_NAME_LENGTH    (8+1+3)U
#define MAX_PATH_FILE_LENGTH    100U
#endif


// TYPEDEFS AND ENAUMS
typedef struct
{
    uint8_t                     RiffChunkID[4];          // Offset 0 "RIFF"
    uint32_t                    RiffChunkSize;           // Offset 4 FileSize - 8
    uint8_t                     RiffType[4];             // Offset 8 "WAVE"
    uint8_t                     FormatChunkID[4];        // Offset 12 "fmt "
    uint32_t                    FormatChunkSize;         // Offset 16 16 for PCM
    uint16_t                    Compression;             // Offset 20 1 = PCM
    uint16_t                    ChannelNumber;           // Offset 22 1 = Mono, 2 = Stereo
    uint32_t                    SampleRate;              // Offset 24 8000, 44100, etc
    uint32_t                    ByteRate;                // Offset 28 SampleRate * Channels * BitsPerSample / 8
    uint16_t                    BlockAlign;              // Offset 32 Channels * BitsPerSample / 8
    uint16_t                    BitsPerSample;           // Offset 34 8 or 16
    uint8_t                     DataChunkID[4];          // Offset 36 "data"
    uint32_t                    DataSize;                // Offset 40 NumSamples * Channels * BitsPerSample / 8
    //                          DataOffset               // Offset 44
} Type_WavHeader;

typedef enum
{
    COMPRESSION_NONE = 1,
    COMPRESSION_IEEE_FLOAT = 3,
    COMPRESSION_A_LAW = 6,
    COMPRESSION_U_LAW = 7
} Type_Compression;

typedef enum
{
    PCM_8_BIT_UNSIGNED = 8,
    PCM_16_BIT_SIGNED = 16,
    PCM_24_BIT_SIGNED = 24
} Type_PCM_BitsPerSample;

typedef struct
{
    uint16_t                    Size;       // MAX NUMBER OF ELEMENTS
    uint16_t                    Start;      // INDEX OF OLDEST ELEMENT
    uint16_t                    End;        // INDEX AT WHICH TO WRITE NEW ELEMENT
    int16_t                     *Elements;  // VECTOR OF ELEMENTS
} Type_int16_t_CircularBuffer;

typedef enum
{
    LSB = 0,
    MSB
} Type_16Bit_ByteOrder;

typedef union
{
    int16_t Signed16Bit_Value;
    uint8_t ByteValue[sizeof(int16_t)];
} Type_Union_PCM_AudioValue;

typedef enum 
{
    NONE = 0,
    MONO,
    STEREO
} Type_AudioChannel;

typedef struct
{
    bool                        IsOpen;
    char                        Name[MAX_FILE_NAME_LENGTH];             // FAT FS supporting long file name
    char                        PathFileName[MAX_PATH_FILE_LENGTH];
    uint16_t                    DirectoryFileCount;
    uint32_t                    Size;
    Type_WavHeader              Header;
} Type_AudioFile;


// EXTERNS
extern DIR Directory;


// FUNCTION PROTOTYPES
FRESULT getNextWavFile(const char *DirectoryPath, char *NextWavFileName, char *NextWavPathFileName, uint32_t *NextWavFileSize, uint16_t FileCount);
FRESULT countFilesInDirectory(const char *DirectoryPath, uint16_t *FileCount);
bool isWavFile(const char *FileName);
bool getWavFileHeader(char *WavPathFileName, uint32_t WavFileSize, Type_WavHeader *WavHeader);
void buildPathFileName(char *PathFileName, const char *DirectoryPath, char *FileName);
bool init_CB(Type_int16_t_CircularBuffer *CircularBuffer, uint16_t Size);
void free_CB(Type_int16_t_CircularBuffer *CircularBuffer);
bool isFull_CB(Type_int16_t_CircularBuffer *CircularBuffer);
bool isEmpty_CB(Type_int16_t_CircularBuffer *CircularBuffer);
bool write_CB(Type_int16_t_CircularBuffer *CircularBuffer, int16_t *Element);
bool read_CB(Type_int16_t_CircularBuffer *CircularBuffer, int16_t *Element, bool *CB_Half_Empty, bool *CB_Half_Full);
uint32_t unusedElements(Type_int16_t_CircularBuffer *CircularBuffer);

#ifdef __cplusplus
}
#endif
#endif /* AUDIO_FILE_API_H_ */