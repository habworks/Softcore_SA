/******************************************************************************************************
 * @file            Water_Mark.c
 * @brief           Functions used to help debug stack overflow problems
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
/********************************************************************************************************
* @brief Linker-provided stack boundary symbols
*
* @note These symbols are defined in the linker script:
*
*   _stack_end = .;        // LOW address of reserved stack region
*   . += _STACK_SIZE;
*   _stack = .;            // HIGH address of reserved stack region
*
*   Memory Layout (increasing addresses):
*
*       low addr
*           |
*           v
*       _stack_end  ----------------------  _stack
*                    <---- STACK REGION ---->
*                                           ^
*                                           |
*                                       Initial SP
*
* @note MicroBlaze and most (not all) MCU stack grows DOWNWARD:
*       - Stack Pointer (SP) starts at _stack (high address)
*       - Each function call pushes data downward toward _stack_end
*       - Local variables, return addresses, saved registers all consume
*         memory from HIGH to LOW addresses
*
* @note We declare these as uint8_t so pointer arithmetic is performed
*       in BYTES.  These are not variables, they are linker symbols.
********************************************************************************************************/
#include "Water_Mark.h"


/********************************************************************************************************
* @brief For use when debugging potential stack blown issues.  With a bare metal app the stack holds the  
* the following information:
*   Local Vars (not global)
*   static Vars (within functions)
*   Where functions return to after it has finished execution
*   Interrupt ISRs are stored on stack
*
* What is not stored on the stack
*   Global Vars (stored in .data if init or .bss if not init)
*   Memory allocated via mallor or calloc
*   Program code (stored in .text)
*
* @author original: Hab Collector \n
*
* @note: This function is intended for bare metal applications only - Most RTOS will have a watermark or 
* equivlent function
* @note: See value is 0xA5
* @note: Remember in almost all cases a stack is filled from the its high address to its low address
* @note: To use: place this function as the very first thing you execute in main.  Call the sister function
* getStackHighWaterMarkBytes where needed or allow the applciation to run for a while and in different use 
* cases (espically when IRQs are known to have been serviced and where functions call functions call functions.
* Compare this value returned from getStackHighWaterMarkBytes to the size of your stack. 
*
* IMPORTANT NOTES FOR PORTABILITY:
* @note: This function fills the stack memory with a seed value (0xA5) so stack usage
*        can later be measured. Because the function writes through the stack region,
*        care must be taken that variables controlling the loop are not placed on the
*        stack itself. Otherwise the seedStackForWaterMark operation could overwrite them.
*
* @note: The variables Pointer and EndPointer are therefore declared static so they
*        are placed in static storage (.bss) instead of the stack. This prevents the
*        watermark loop from corrupting its own control variables.
*
* @note: The function is placed in the .Hab_Fast_Text section so its code resides in
*        a deterministic memory region (LMB). While not strictly required for
*        correctness, keeping this routine out of the default code section avoids
*        unintended placement changes as the application grows.
*
* @note: Creating a dedicated linker section for routines such as this can help keep
*        memory placement explicit and easier to reason about.
* 
* STEP 1: Assign the pointers
* STEP 2: Fill the stack area with seed value 0xA5
********************************************************************************************************/
__attribute__((section(".Hab_Fast_Text")))
void seedStackForWaterMark(void)
{
    // STEP 1: Assign the pointers
    static volatile uint8_t *Pointer = (volatile uint8_t *)&STACK_LOW_ADDRESS;
    static volatile uint8_t *EndPointer = (volatile uint8_t *)&STACK_HIGH_ADDRESS;

    // STEP 2: Fill the stack area with seed value 0xA5
    while (Pointer < EndPointer)
    {
        *Pointer = (uint8_t)0xA5;
        Pointer++;
    }

} // END OF seedStackForWaterMark



/********************************************************************************************************
* @brief Get how much of the stack is used - The larger the return value the more of the stack has been used.
* Theorically speaking you can use up to the full stack.  However this is unwise and you should in practice
* have stack remaining. Note you are looking at byte address.
*
* @author original: Hab Collector \n
*
* @note: See seedStackForWaterMark
* @note: Remember in almost all cases a stack is filled from the its high address to its low address
*
* @return: How much of the stack has been used - the higher the number the more of the stack has been used
* 
* STEP 1: Assign the pointers
* STEP 2: Scan upward until first non-0xA5
* STEP 3: Total Used stack = (total stack size - untouched region)
********************************************************************************************************/
uint32_t getStackHighWaterMarkBytes(void)
{
    // STEP 1: Assign the pointers
    static volatile uint8_t *Pointer = &STACK_LOW_ADDRESS;
    static volatile uint8_t *EndPointer = &STACK_HIGH_ADDRESS;

    // STEP 2: Scan upward until first non-0xA5
    while ((Pointer < EndPointer) && (*Pointer == 0xA5))
    {
        Pointer++;
    }

    // STEP 3: Total Used stack = (total stack size - untouched region)
    return((uint32_t)(EndPointer - Pointer));

} // END OF getStackHighWaterMarkBytes
