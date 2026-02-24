/******************************************************************************************************
 * @file            Circular_Buffers.c
 * @brief           A collection of functions relevant to supporting all types of circular buffers
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

#include "Circular_Buffers.h"


/********************************************************************************************************
* @brief Initializes a circular buffer of uint8_t type
*
* @author original: Hab Collector \n
*
* @note: Allocates memory for circular buffer storage
*
* @param CircularBuffer: Pointer to circular buffer structure
* @param Size: Maximum number of elements to store
*
* @return True if OK
*
* STEP 1: Set start and end of buffer to 0 and assign size
* STEP 2: Allocate memory for circular buffer elements
********************************************************************************************************/
bool init_U8_CB(Type_uint8_t_CircularBuffer *CircularBuffer, uint16_t Size)
{
    // STEP 1: Set start and end of buffer to 0 and assign size
    CircularBuffer->Size  = Size + 1; // INCLUDES AN EMPTY LOCATION
    CircularBuffer->Start = 0;
    CircularBuffer->End   = 0;

    // STEP 2: Allocate memory for circular buffer elements
    CircularBuffer->Elements = (uint8_t *)calloc(CircularBuffer->Size, sizeof(uint8_t));
    if (CircularBuffer->Elements == NULL)
        return(false);
    else
        return(true);

} // END OF init_U8_CB



/********************************************************************************************************
* @brief Frees memory allocated for a circular buffer of uint8_t type
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
void free_U8_CB(Type_uint8_t_CircularBuffer *CircularBuffer)
{
    // STEP 1: Free allocated circular buffer memory
    if (CircularBuffer == NULL)
        return;
    free(CircularBuffer->Elements);
    CircularBuffer->Elements = NULL;

} // END OF free_U8_CB



/********************************************************************************************************
* @brief Checks if the circular buffer is full of uint8_t type
*
* @author original: Hab Collector \n
*
* @param CircularBuffer: Pointer to circular buffer structure
*
* @return True if buffer is full, else false
*
* STEP 1: Determine if buffer is full
********************************************************************************************************/
bool isFull_U8_CB(Type_uint8_t_CircularBuffer *CircularBuffer)
{
    // STEP 1: Determine if buffer is full
    return((CircularBuffer->End + 1) % CircularBuffer->Size == CircularBuffer->Start);
} // END OF isFull_U8_CB



/********************************************************************************************************
* @brief Checks if the circular buffer is empty of uint8_t type
*
* @author original: Hab Collector \n
*
* @param CircularBuffer: Pointer to circular buffer structure
*
* @return True if buffer is empty, else false
*
* STEP 1: Determine if buffer is empty
********************************************************************************************************/
bool isEmpty_U8_CB(Type_uint8_t_CircularBuffer *CircularBuffer)
{
    // STEP 1: Determine if buffer is empty
    return(CircularBuffer->End == CircularBuffer->Start);
} // END OF isEmpty_U8_CB



/********************************************************************************************************
* @brief Writes a single element to the circular buffer of uint8_t type
*
* @author original: Hab Collector \n
*
* @note: Overwrites oldest element if buffer is full
*        Caller may check isFull_CB() to prevent overwrite
*
* @param CircularBuffer: Pointer to circular buffer structure
* @param Element: Element value to store
*
* @return True if write OK, false if buffer is full
*
* STEP 1: Do not overwrite
* STEP 2: Store element at end index
* STEP 3: Advance end index 
********************************************************************************************************/
bool write_U8_CB(Type_uint8_t_CircularBuffer *CircularBuffer, uint8_t Element)
{
    // STEP 1: Do not overwrite
    if (isFull_U8_CB(CircularBuffer))
        return(false);

    // STEP 2: Store element at end index
    CircularBuffer->Elements[CircularBuffer->End] = Element;

    // STEP 3: Advance end index
    CircularBuffer->End = (CircularBuffer->End + 1) % CircularBuffer->Size;

    return(true);

} // END OF write_U8_CB



/********************************************************************************************************
* @brief Reads a single element from the circular buffer and reports buffer fullness state of uint8_t type
*
* @author original: Hab Collector \n
*
* @note: Function will not read from the buffer if it is empty
*        Return value reflects success or failure of read operation
*        Buffer fullness indicators reflect state after the read
* @note: If not used CB_AtOrBelowHalfFull or CB_MoreThanHalfFull it is acceptable to pass NULL to either
*
* @param CircularBuffer: Pointer to circular buffer structure
* @param Element: Pointer to element to receive data
* @param CB_AtOrBelowHalfFull: Pointer is set true true if buffer is at or below half full (Count <= Capacity/2)
* @param CB_MoreThanHalfFull: Pointer is set true if buffer is more than half full (Count >  Capacity/2)
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
bool read_U8_CB(Type_uint8_t_CircularBuffer *CircularBuffer, uint8_t *Element, bool *CB_AtOrBelowHalfFull, bool *CB_MoreThanHalfFull)
{
    uint16_t Count;
    uint16_t Capacity;

    // STEP 1: Verify circular buffer is not empty
    if (isEmpty_U8_CB(CircularBuffer))
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
    if (CB_AtOrBelowHalfFull != NULL)
        *CB_AtOrBelowHalfFull = (Count <= (Capacity / 2));
    if (CB_MoreThanHalfFull != NULL)
        *CB_MoreThanHalfFull  = !(*CB_AtOrBelowHalfFull);

    return(true);

} // END OF read_U8_CB



/********************************************************************************************************
* @brief Returns the number of free elements available for writing in the circular buffer of uint8_t type
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
uint32_t availableWrites_U8_CB(Type_uint8_t_CircularBuffer *CircularBuffer)
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

} // END OF availableWrites_U8_CB



/********************************************************************************************************
* @brief Returns the number of elements available for reading from the CB
*
* @author original: Hab Collector \n
*
* @note: Buffer uses a one-empty-slot design to distinguish full vs empty
*        Returned value reflects how many additional writes can succeed
*
* @param CircularBuffer: Pointer to circular buffer structure
*
* @return Number of elements in the buffer
********************************************************************************************************/
uint32_t availableReads_U8_CB(Type_uint8_t_CircularBuffer *CircularBuffer)
{   
    uint32_t Capacity = CircularBuffer->Size - 1;
    return(Capacity - availableWrites_U8_CB(CircularBuffer));
} // END OF availableReads_U8_CB



/********************************************************************************************************
* @brief Writes the contents of a linear buffer into a circular buffer.  Function takes into account safety
* checks and the fact that the buffer may wrap.
*
* @author original: Hab Collector \n
*
* @note: Buffer uses a one-empty-slot design to distinguish full vs empty
*        Returned value reflects how many additional writes can succeed
*
* @param CircularBuffer: Pointer to circular buffer structure
* @param SourceBuffer: Pointer to linear buffer from which data is copied
* @param Length: Number of bytes to copy
*
* @return True if write is OK
*
* STEP 1: Safety Checks
* STEP 2: Calculate the block lenghts to copy before and after the wrap
* STEP 3: Copy first block - pre wrap
* STEP 4: Copy second block (wrap to 0)
********************************************************************************************************/
bool writeBufferTo_U8_CB(Type_uint8_t_CircularBuffer *CircularBuffer, uint8_t *SourceBuffer, uint16_t Length)
{
    // STEP 1: Safety Checks
    if (Length == 0U)
        return(true);

    if ((CircularBuffer == NULL) || (SourceBuffer == NULL))
        return(false);

    if (availableWrites_U8_CB(CircularBuffer) < (uint32_t)Length)
        return(false);

    // STEP 2: Calculate the block lenghts to copy before and after the wrap
    uint16_t Capacity = (uint16_t)(CircularBuffer->Size - 1U);
    // How many bytes we can write contiguously before wrapping
    uint16_t SpaceToEnd = (CircularBuffer->End >= Capacity) ? 0U : (uint16_t)(Capacity - CircularBuffer->End);
    uint16_t FirstCopyLength = (Length < SpaceToEnd) ? Length : SpaceToEnd;
    uint16_t RemainingLength = (uint16_t)(Length - FirstCopyLength);

    // STEP 3: Copy first block - pre wrap
    if (FirstCopyLength != 0U)
    {
        for (uint16_t Index = 0U; Index < FirstCopyLength; Index++)
            CircularBuffer->Elements[CircularBuffer->End + Index] = SourceBuffer[Index];

        CircularBuffer->End = (uint16_t)((CircularBuffer->End + FirstCopyLength) % CircularBuffer->Size);
    }

    // STEP 4: Copy second block (wrap to 0)
    if (RemainingLength != 0U)
    {
        for (uint16_t Index = 0U; Index < RemainingLength; Index++)
            CircularBuffer->Elements[CircularBuffer->End + Index] = SourceBuffer[FirstCopyLength + Index];

        CircularBuffer->End = (uint16_t)((CircularBuffer->End + RemainingLength) % CircularBuffer->Size);
    }

    return(true);

} // END OF writeBufferTo_U8_CB



/********************************************************************************************************
* @brief Initializes a circular buffer of int16_t type
*
* @author original: Hab Collector \n
*
* @note: Allocates memory for circular buffer storage
*
* @param CircularBuffer: Pointer to circular buffer structure
* @param Size: Maximum number of elements to store
*
* @return True if OK
*
* STEP 1: Set start and end of buffer to 0 and assign size
* STEP 2: Allocate memory for circular buffer elements
********************************************************************************************************/
bool init_I16_CB(Type_int16_t_CircularBuffer *CircularBuffer, uint16_t Size)
{
    // STEP 1: Set start and end of buffer to 0 and assign size
    CircularBuffer->Size  = Size + 1; // INCLUDES AN EMPTY LOCATION
    CircularBuffer->Start = 0;
    CircularBuffer->End   = 0;

    // STEP 2: Allocate memory for circular buffer elements
    CircularBuffer->Elements = (int16_t *)calloc(CircularBuffer->Size, sizeof(int16_t));
    if (CircularBuffer->Elements == NULL)
        return(false);
    else
        return(true);

} // END OF init_I16_CB



/********************************************************************************************************
* @brief Frees memory allocated for a circular buffer of int16_t type
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
void free_I16_CB(Type_int16_t_CircularBuffer *CircularBuffer)
{
    // STEP 1: Free allocated circular buffer memory
    if (CircularBuffer == NULL)
        return;
    free(CircularBuffer->Elements);
    CircularBuffer->Elements = NULL;

} // END OF free_I16_CB



/********************************************************************************************************
* @brief Checks if the circular buffer is full of int16_t type
*
* @author original: Hab Collector \n
*
* @param CircularBuffer: Pointer to circular buffer structure
*
* @return True if buffer is full, else false
*
* STEP 1: Determine if buffer is full
********************************************************************************************************/
bool isFull_I16_CB(Type_int16_t_CircularBuffer *CircularBuffer)
{
    // STEP 1: Determine if buffer is full
    return((CircularBuffer->End + 1) % CircularBuffer->Size == CircularBuffer->Start);
} // END OF isFull_I16_CB



/********************************************************************************************************
* @brief Checks if the circular buffer is empty of int16_t type
*
* @author original: Hab Collector \n
*
* @param CircularBuffer: Pointer to circular buffer structure
*
* @return True if buffer is empty, else false
*
* STEP 1: Determine if buffer is empty
********************************************************************************************************/
bool isEmpty_I16_CB(Type_int16_t_CircularBuffer *CircularBuffer)
{
    // STEP 1: Determine if buffer is empty
    return(CircularBuffer->End == CircularBuffer->Start);
} // END OF isEmpty_I16_CB



/********************************************************************************************************
* @brief Writes a single element to the circular buffer of int16_t type
*
* @author original: Hab Collector \n
*
* @note: Overwrites oldest element if buffer is full
*        Caller may check isFull_CB() to prevent overwrite
*
* @param CircularBuffer: Pointer to circular buffer structure
* @param Element: Element value to store
*
* @return True if write OK, false if buffer is full
*
* STEP 1: Do not overwrite
* STEP 2: Store element at end index
* STEP 3: Advance end index 
********************************************************************************************************/
bool write_I16_CB(Type_int16_t_CircularBuffer *CircularBuffer, int16_t Element)
{
    // STEP 1: Do not overwrite
    if (isFull_I16_CB(CircularBuffer))
        return(false);

    // STEP 2: Store element at end index
    CircularBuffer->Elements[CircularBuffer->End] = Element;

    // STEP 3: Advance end index
    CircularBuffer->End = (CircularBuffer->End + 1) % CircularBuffer->Size;

    return(true);

} // END OF write_I16_CB



/********************************************************************************************************
* @brief Reads a single element from the circular buffer and reports buffer fullness state of int16_t type
*
* @author original: Hab Collector \n
*
* @note: Function will not read from the buffer if it is empty
*        Return value reflects success or failure of read operation
*        Buffer fullness indicators reflect state after the read
* @note: If not used CB_AtOrBelowHalfFull or CB_MoreThanHalfFull it is acceptable to pass NULL to either
*
* @param CircularBuffer: Pointer to circular buffer structure
* @param Element: Pointer to element to receive data
* @param CB_AtOrBelowHalfFull: Pointer is set true true if buffer is at or below half full (Count <= Capacity/2)
* @param CB_MoreThanHalfFull: Pointer is set true if buffer is more than half full (Count >  Capacity/2)
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
bool read_I16_CB(Type_int16_t_CircularBuffer *CircularBuffer, int16_t *Element, bool *CB_AtOrBelowHalfFull, bool *CB_MoreThanHalfFull)
{
    uint16_t Count;
    uint16_t Capacity;

    // STEP 1: Verify circular buffer is not empty
    if (isEmpty_I16_CB(CircularBuffer))
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
    if (CB_AtOrBelowHalfFull != NULL)
        *CB_AtOrBelowHalfFull = (Count <= (Capacity / 2));
    if (CB_MoreThanHalfFull != NULL)
        *CB_MoreThanHalfFull  = !(*CB_AtOrBelowHalfFull);

    return(true);

} // END OF read_I16_CB



/********************************************************************************************************
* @brief Returns the number of free elements available for writing in the circular buffer of int16_t type
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
uint32_t availableWrites_I16_CB(Type_int16_t_CircularBuffer *CircularBuffer)
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

} // END OF availableWrites_I16_CB



/********************************************************************************************************
* @brief Returns the number of elements available for reading from the CB
*
* @author original: Hab Collector \n
*
* @note: Buffer uses a one-empty-slot design to distinguish full vs empty
*        Returned value reflects how many additional writes can succeed
*
* @param CircularBuffer: Pointer to circular buffer structure
*
* @return Number of elements in the buffer
********************************************************************************************************/
uint32_t availableReads_I16_CB(Type_int16_t_CircularBuffer *CircularBuffer)
{   
    uint32_t Capacity = CircularBuffer->Size - 1;
    return(Capacity - availableWrites_I16_CB(CircularBuffer));
} // END OF availableReads_U8_CB



