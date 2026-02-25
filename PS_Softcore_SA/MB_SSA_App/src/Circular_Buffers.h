/******************************************************************************************************
 * @file            Circular_Buffers.h
 * @brief           Header file to support Circular_Buffers.c 
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

#ifndef CIRCULAR_BUFFERS_H_
#define CIRCULAR_BUFFERS_H_
#ifdef __cplusplus
extern"C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// DEFINES


// TYPEDEFS AND ENUMS
typedef struct
{
    uint16_t                    Size;       // MAX NUMBER OF ELEMENTS
    uint16_t                    Start;      // INDEX OF OLDEST ELEMENT
    uint16_t                    End;        // INDEX AT WHICH TO WRITE NEW ELEMENT
    uint8_t                     *Elements;  // VECTOR OF ELEMENTS
} Type_uint8_t_CircularBuffer;

typedef struct
{
    uint16_t                    Size;       // MAX NUMBER OF ELEMENTS
    uint16_t                    Start;      // INDEX OF OLDEST ELEMENT
    uint16_t                    End;        // INDEX AT WHICH TO WRITE NEW ELEMENT
    int16_t                     *Elements;  // VECTOR OF ELEMENTS
} Type_int16_t_CircularBuffer;


// FUNCTION PROTOTYPES
// Type_uint8_t_CircularBuffer
bool init_U8_CB(Type_uint8_t_CircularBuffer *CircularBuffer, uint16_t Size);
void free_U8_CB(Type_uint8_t_CircularBuffer *CircularBuffer);
bool isFull_U8_CB(Type_uint8_t_CircularBuffer *CircularBuffer);
bool isEmpty_U8_CB(Type_uint8_t_CircularBuffer *CircularBuffer);
bool write_U8_CB(Type_uint8_t_CircularBuffer *CircularBuffer, uint8_t Element);
bool read_U8_CB(Type_uint8_t_CircularBuffer *CircularBuffer, uint8_t *Element, bool *CB_AtOrBelowHalfFull, bool *CB_MoreThanHalfFull);
uint32_t availableWrites_U8_CB(Type_uint8_t_CircularBuffer *CircularBuffer);
uint32_t availableReads_U8_CB(Type_uint8_t_CircularBuffer *CircularBuffer);
bool writeBufferTo_U8_CB(Type_uint8_t_CircularBuffer *CircularBuffer, uint8_t *SourceBuffer, uint16_t Length);
// Type_int16_t_CircularBuffer
bool init_I16_CB(Type_int16_t_CircularBuffer *CircularBuffer, uint16_t Size);
void free_I16_CB(Type_int16_t_CircularBuffer *CircularBuffer);
bool isFull_I16_CB(Type_int16_t_CircularBuffer *CircularBuffer);
bool isEmpty_I16_CB(Type_int16_t_CircularBuffer *CircularBuffer);
bool write_I16_CB(Type_int16_t_CircularBuffer *CircularBuffer, int16_t Element);
bool read_I16_CB(Type_int16_t_CircularBuffer *CircularBuffer, int16_t *Element, bool *CB_AtOrBelowHalfFull, bool *CB_MoreThanHalfFull);
uint32_t availableWrites_I16_CB(Type_int16_t_CircularBuffer *CircularBuffer);
uint32_t availableReads_I16_CB(Type_int16_t_CircularBuffer *CircularBuffer);

#ifdef __cplusplus
}
#endif
#endif /* CIRCULAR_BUFFERS_H_ */