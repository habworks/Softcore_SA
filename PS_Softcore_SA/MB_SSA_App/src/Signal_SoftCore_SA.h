/******************************************************************************************************
 * @file            Signal_Mode_API.h
 * @brief           Header file to support Signal_Mode_API.c
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

#ifndef SIGNAL_SOFTCORE_SA_H_
#define SIGNAL_SOFTCORE_SA_H_
#ifdef __cplusplus
extern"C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "Main_Support.h"
#include "AXI_IMR_ADC_7476A_DUAL.h"


// DEFINES
#define DEFAULT_SIGNAL_SAMPLE_RATE_HZ   100000U


// TYPEDEFS AND ENUMS
typedef enum
{
    SIGNAL_ON_BOARD_OSCILLATOR = 0,
    SIGNAL_OFF_BOARD_BNC
} Type_SignalSelect;

typedef struct
{
    bool                Enable;
    Type_SignalSelect   Source;    
} Type_Signal_SA;


// FUNCTION PROTOTYPES
bool initSpectrumAnalyzer(Type_Signal_SA *Signal_SA, Type_FFT *FFT, Type_AXI_IMR_7476A_Handle *ADC_Handle, uint32_t SignalSampleRate, uint16_t *ADC_BufferDatum_A, uint32_t *ADC_BufferDatum_B);
void signalSpectrumAnalyzer(Type_Signal_SA *Signal_SA, Type_FFT *FFT);
void signalSelect(Type_SignalSelect Signal);
void signalPeriodicTimer_ISR(Type_Signal_SA *Signal_SA, Type_FFT *FFT, uint16_t *ADC_BufferDatum_A, uint16_t *ADC_BufferDatum_B);
void signal_ADC_7476A_ISR(Type_Signal_SA *Signal_SA, Type_FFT *FFT, Type_AXI_IMR_7476A_Handle *ADC_Handle);


#ifdef __cplusplus
}
#endif
#endif /* SIGNAL_SOFTCORE_SA_H_ */
