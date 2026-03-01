/******************************************************************************************************
 * @file            FFT_Support.c
 * @brief           Various application layer functions that work with the middleware KISSFFT library
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

#include "FFT_Support.h"
#include <kiss_fftr.h>
#include <math.h>
#include <string.h>
#include "Hab_Types.h"

#include "Main_App.h"
#include "IO_Support.h"

static uint32_t clamp_u32(uint32_t Value, uint32_t Min, uint32_t Max);


float SpectrumFrequency[FREQUENCY_SLOTS];
float SpectrumMagnitude[FREQUENCY_SLOTS];
Type_AudioSpectrum  AudioSpectrum =
{
    .IsFrequencyLogSpacing = true,      // Try true first
    .IsMagnitudeLog        = true,      // dB scaling
    .FrequencyBarCount     = FREQUENCY_SLOTS,
    .MagnitudeBarCount     = MAX_VERTICAL_BAR_COUNT,
    .Frequency             = SpectrumFrequency,
    .Magnitude             = SpectrumMagnitude
};


bool init_FFT(Type_FFT *FFT, uint32_t SampleRate_Hz)
{
    // STEP 1: Validate input pointer.
    if (FFT == NULL)
        return(false);

    // STEP 2: Store FFT size and sample rate.
    FFT->Size = FFT_SIZE;
    FFT->SampleRate_Hz = SampleRate_Hz;

    // STEP 3: Compute resolution bandwidth (RBW).
    FFT->RBW = ((float)SampleRate_Hz / (float)FFT_SIZE);

    // STEP 4: Clear runtime flags.
    FFT->FrameReady = false;

    // STEP 5: Generate Hann window coefficients.
    // for (uint16_t N = 0; N < FFT_SIZE; N++)
    // {
    //     FFT->HannWindow[N] =
    //         0.5f * (1.0f - cosf((2.0f * M_PI * N) / (FFT_SIZE - 1)));
    // }

    // STEP 6: Allocate KissFFT real FFT configuration.
    FFT->FFT_Config = kiss_fftr_alloc(FFT_SIZE, 0, NULL, NULL);

    if (FFT->FFT_Config == NULL)
        return(false);

    return(true);
}


// __attribute__((section(".Hab_Fast_Text")))
uint32_t FFT_ProcessFrame(Type_FFT *FFT, float *BinMagnitudes, uint32_t BinCount)
{
    static float __attribute__ ((section (".Hab_Fast_Data"))) WindowedSamples[FFT_SIZE];
    static kiss_fft_cpx __attribute__ ((section (".Hab_Fast_Data"))) OutputBins[FFT_MAX_BINS];

    // STEP 1: Validate inputs and confirm a frame is ready.
    if (FFT == NULL)
        return(false);

    if (BinMagnitudes == NULL)
        return(false);

    if (BinCount == 0)
        return(false);

    if (BinCount > FFT_MAX_BINS)
        return(false);

    if (FFT->FFT_Config == NULL)
        return(false);

    if (FFT->FrameReady == false)
        return(false);

    // STEP 2: Snapshot the FFT sample buffer and clear FrameReady.
    
    memcpy(WindowedSamples, (const void *)FFT->Samples, (size_t)(FFT_SIZE * sizeof(float)));

    FFT->FrameReady = false;

    // STEP 3: Apply Hann window to the snapshot samples.

    for (uint16_t Index = 0; Index < FFT_SIZE; Index++)
    {
        WindowedSamples[Index] = (WindowedSamples[Index] * FFT->HannWindow[Index]);
    }


    // STEP 4: Run KissFFT real FFT on the windowed samples.
XGpio_DiscreteSet(&AXI_GPIO_Handle, GPIO_OUTPUT_CHANNEL, TEST_IO_0); 
    kiss_fftr(FFT->FFT_Config, WindowedSamples, OutputBins);
XGpio_DiscreteClear(&AXI_GPIO_Handle, GPIO_OUTPUT_CHANNEL, TEST_IO_0);  

    // STEP 5: Compute magnitude for each requested bin and store into BinMagnitudes.

    for (uint32_t Bin = 0; Bin < BinCount; Bin++)
    {
        float Real = (float)OutputBins[Bin].r;
        float Imag = (float)OutputBins[Bin].i;

        BinMagnitudes[Bin] = sqrtf((Real * Real) + (Imag * Imag));
        // BinMagnitudes[Bin] = ((Real * Real) + (Imag * Imag));
    }


    return(true);
}


static uint32_t clamp_u32(uint32_t Value, uint32_t Min, uint32_t Max)
{
    if (Value < Min)
        return(Min);

    if (Value > Max)
        return(Max);

    return(Value);
}

bool FFT_MapBinsToBars(Type_FFT *FFT, const float *BinMagnitudes, uint32_t BinCount, float *BarMagnitudes, uint32_t BarCount, bool UseLogSpacing)
{
    // STEP 1: Validate inputs.
    if (FFT == NULL)
        return(false);

    if (BinMagnitudes == NULL)
        return(false);

    if (BarMagnitudes == NULL)
        return(false);

    if (BinCount == 0)
        return(false);

    if (BarCount == 0)
        return(false);

    // STEP 2: Clear BarMagnitudes output.
    for (uint32_t Bar = 0; Bar < BarCount; Bar++)
    {
        BarMagnitudes[Bar] = 0.0f;
    }

    // STEP 3: For each bar, compute start/end bin indices.
    // Note: We typically skip DC (bin 0) for display, so start at 1.
    uint32_t FirstBin = 1;
    uint32_t LastBin = (BinCount - 1);

    if (LastBin <= FirstBin)
        return(false);

    for (uint32_t Bar = 0; Bar < BarCount; Bar++)
    {
        uint32_t StartBin;
        uint32_t EndBin;

        if (UseLogSpacing)
        {
            // Log-like spacing using squared fraction.  KISS and works well visually.
            float StartFraction = ((float)Bar / (float)BarCount);
            float EndFraction = ((float)(Bar + 1) / (float)BarCount);

            StartFraction = (StartFraction * StartFraction);
            EndFraction = (EndFraction * EndFraction);

            StartBin = FirstBin + (uint32_t)((float)(LastBin - FirstBin) * StartFraction);
            EndBin = FirstBin + (uint32_t)((float)(LastBin - FirstBin) * EndFraction);
        }
        else
        {
            // Linear spacing
            StartBin = FirstBin + (uint32_t)(((uint64_t)(LastBin - FirstBin) * (uint64_t)Bar) / (uint64_t)BarCount);
            EndBin = FirstBin + (uint32_t)(((uint64_t)(LastBin - FirstBin) * (uint64_t)(Bar + 1)) / (uint64_t)BarCount);
        }

        StartBin = clamp_u32(StartBin, FirstBin, LastBin);
        EndBin = clamp_u32(EndBin, FirstBin, LastBin);

        if (EndBin < StartBin)
            EndBin = StartBin;

        // STEP 4: Average bin magnitudes within each bar range.
        float Sum = 0.0f;
        uint32_t Count = 0;

        for (uint32_t Bin = StartBin; Bin <= EndBin; Bin++)
        {
            Sum += BinMagnitudes[Bin];
            Count++;
        }

        // STEP 5: Store the averaged value into BarMagnitudes.
        if (Count > 0)
            BarMagnitudes[Bar] = (Sum / (float)Count);
    }

    return(true);
}


bool FFT_ScaleBars(float *BarMagnitudes, uint32_t BarCount, bool UseDBScale, float MinDB)
{
    // STEP 1: Validate inputs.
    if (BarMagnitudes == NULL)
        return(false);

    if (BarCount == 0)
        return(false);

    // STEP 2: Convert magnitude if requested.
    for (uint32_t Bar = 0; Bar < BarCount; Bar++)
    {
        float Value = BarMagnitudes[Bar];

        if (UseDBScale == true)
        {
            if (Value < 1e-12f)
                Value = 1e-12f;

            Value = 20.0f * log10f(Value);

            if (Value < MinDB)
                Value = MinDB;

            // Normalize to 0.0 – 1.0
            Value = (Value - MinDB) / (-MinDB);
        }
        else
        {
            // Linear normalization
            if (Value < 0.0f)
                Value = 0.0f;

            // Optional: simple compression
            Value = sqrtf(Value);
        }

        BarMagnitudes[Bar] = Value;
    }

    return(true);
}








static float clamp_f32(float Value, float Minimum, float Maximum)
{
    if (Value < Minimum)
        return(Minimum);

    if (Value > Maximum)
        return(Maximum);

    return(Value);
}

static float applyLinearCompression(float Value, Type_LinearCompression LinearCompression)
{
    if (Value < 0.0f)
        Value = 0.0f;

    if (LinearCompression == LINEAR_COMPRESSION_NONE)
        return(Value);

    if (LinearCompression == LINEAR_COMPRESSION_SQRT)
        return(sqrtf(Value));

    if (LinearCompression == LINEAR_COMPRESSION_LOG1P)
        return(log1pf(Value));

    return(Value);
}


/**************************************************************************************************
 * Function: buildAudioSpectrumFrame
 *
 * Description:
 *   Builds a display-ready spectrum frame from FFT bin magnitudes.  Produces integer bar heights
 *   (0..MAX_VERTICAL_BAR_COUNT) ordered from low frequency to high frequency, and also fills the
 *   floating-point analysis structure Type_AudioSpectrum.
 *
 * Parameters:
 *   SampleRate_Hz              Audio sample rate in Hz (example: 16000).
 *   FFT_Size                   FFT size (example: 1024).
 *   BinMagnitudes              Pointer to FFT bin magnitudes array (linear magnitude), bins 0..(FFT_Size/2).
 *   BinCount                   Number of valid bins in BinMagnitudes.  Must equal (FFT_Size/2 + 1).
 *   SlotMagnitudeMethod        SLOT_MAGNITUDE_AVERAGE or SLOT_MAGNITUDE_RMS.
 *   LinearCompression          Compression method used when IsMagnitudeLog is false.
 *   MinDB                      dB floor used when IsMagnitudeLog is true (example: -60.0f).
 *   DisplayMagnitudeBars       Output buffer of length FREQUENCY_SLOTS.  Each element is 0..MAX_VERTICAL_BAR_COUNT.
 *   FrequencyBarCount          Must equal FREQUENCY_SLOTS.
 *   MagnitudeBarCount          Must equal MAX_VERTICAL_BAR_COUNT.
 *   Spectrum                   Pointer to Type_AudioSpectrum.  Function fills Frequency[] and Magnitude[] arrays.
 *
 * Return:
 *   true    Success.
 *   false   Error.
 *
 * STEP 1: Validate inputs.
 * STEP 2: Compute RBW and determine valid bin range (ignore DC).
 * STEP 3: Build slot bin ranges using linear or log-like frequency spacing.
 * STEP 4: Compute slot Frequency[] (center frequency) and slot Magnitude[] (average or RMS).
 * STEP 5: Convert slot Magnitude[] to dB if enabled.
 * STEP 6: Convert slot magnitudes into DisplayMagnitudeBars[] (0..MagnitudeBarCount).
 *
 **************************************************************************************************/
bool buildAudioSpectrumFrame(uint32_t SampleRate_Hz,
                            uint16_t FFT_Size,
                            const float *BinMagnitudes,
                            uint32_t BinCount,
                            Type_SlotMagnitudeMethod SlotMagnitudeMethod,
                            Type_LinearCompression LinearCompression,
                            float MinDB,
                            uint8_t *DisplayMagnitudeBars,
                            uint8_t FrequencyBarCount,
                            uint8_t MagnitudeBarCount,
                            Type_AudioSpectrum *Spectrum)
{
    // STEP 1: Validate inputs.
    if (BinMagnitudes == NULL)
        return(false);

    if (DisplayMagnitudeBars == NULL)
        return(false);

    if (Spectrum == NULL)
        return(false);

    if (Spectrum->Frequency == NULL)
        return(false);

    if (Spectrum->Magnitude == NULL)
        return(false);

    if (SampleRate_Hz == 0)
        return(false);

    if (FFT_Size == 0)
        return(false);

    if (BinCount == 0)
        return(false);

    if (BinCount != ((uint32_t)(FFT_Size / 2) + 1U))
        return(false);

    if (FrequencyBarCount != FREQUENCY_SLOTS)
        return(false);

    if (MagnitudeBarCount != MAX_VERTICAL_BAR_COUNT)
        return(false);

    if (MagnitudeBarCount == 0)
        return(false);

    if ((Spectrum->IsMagnitudeLog == true) && (MinDB >= 0.0f))
        return(false);

    Spectrum->FrequencyBarCount = FrequencyBarCount;
    Spectrum->MagnitudeBarCount = MagnitudeBarCount;

    // STEP 2: Compute RBW and determine valid bin range (ignore DC).
    float RBW_Hz = ((float)SampleRate_Hz / (float)FFT_Size);

    uint32_t FirstBin = 1;
    uint32_t LastBin = (BinCount - 1);

    if (LastBin <= FirstBin)
        return(false);

    // STEP 3: Build slot bin ranges using linear or log-like frequency spacing.
    uint32_t TotalBins = (LastBin - FirstBin + 1U);

    uint32_t PreviousEndBin = (FirstBin - 1U);

    for (uint32_t Slot = 0; Slot < (uint32_t)FrequencyBarCount; Slot++)
    {
        float StartFraction = ((float)Slot / (float)FrequencyBarCount);
        float EndFraction = ((float)(Slot + 1U) / (float)FrequencyBarCount);

        if (Spectrum->IsFrequencyLogSpacing == true)
        {
            StartFraction = (StartFraction * StartFraction);
            EndFraction = (EndFraction * EndFraction);
        }

        uint32_t StartOffset = (uint32_t)((float)TotalBins * StartFraction);
        uint32_t EndOffset = (uint32_t)((float)TotalBins * EndFraction);

        uint32_t StartBin = (FirstBin + StartOffset);
        uint32_t EndBin = (FirstBin + EndOffset - 1U);

        if (Slot == 0U)
            StartBin = FirstBin;

        if (Slot == ((uint32_t)FrequencyBarCount - 1U))
            EndBin = LastBin;

        if (StartBin <= PreviousEndBin)
            StartBin = (PreviousEndBin + 1U);

        if (StartBin > LastBin)
            StartBin = LastBin;

        if (EndBin < StartBin)
            EndBin = StartBin;

        if (EndBin > LastBin)
            EndBin = LastBin;

        PreviousEndBin = EndBin;

        // STEP 4: Compute slot Frequency[] (center frequency) and slot Magnitude[] (average or RMS).
        uint32_t BinCountInSlot = (EndBin - StartBin + 1U);

        float Sum = 0.0f;
        float SumSq = 0.0f;

        for (uint32_t Bin = StartBin; Bin <= EndBin; Bin++)
        {
            float Value = BinMagnitudes[Bin];

            if (Value < 0.0f)
                Value = 0.0f;

            Sum += Value;
            SumSq += (Value * Value);
        }

        float SlotMagnitudeLinear;

        if (SlotMagnitudeMethod == SLOT_MAGNITUDE_RMS)
            SlotMagnitudeLinear = sqrtf(SumSq / (float)BinCountInSlot);
        else
            SlotMagnitudeLinear = (Sum / (float)BinCountInSlot);

        float CenterBin = ((float)StartBin + (float)EndBin) * 0.5f;

        Spectrum->Frequency[Slot] = (CenterBin * RBW_Hz);
        Spectrum->Magnitude[Slot] = SlotMagnitudeLinear;
    }

    // STEP 5: Convert slot Magnitude[] to dB if enabled.
    if (Spectrum->IsMagnitudeLog == true)
    {
        float Reference = 0.0f;

        for (uint32_t Slot = 0; Slot < (uint32_t)FrequencyBarCount; Slot++)
        {
            float Value = Spectrum->Magnitude[Slot];

            if (Value > Reference)
                Reference = Value;
        }

        if (Reference < 1e-12f)
            Reference = 1e-12f;

        for (uint32_t Slot = 0; Slot < (uint32_t)FrequencyBarCount; Slot++)
        {
            float Value = Spectrum->Magnitude[Slot];

            if (Value < 1e-12f)
                Value = 1e-12f;

            float Relative = (Value / Reference);
            float Mag_dB = (20.0f * log10f(Relative));

            if (Mag_dB < MinDB)
                Mag_dB = MinDB;

            Spectrum->Magnitude[Slot] = Mag_dB;
        }
    }

    // STEP 6: Convert slot magnitudes into DisplayMagnitudeBars[] (0..MagnitudeBarCount).
    if (Spectrum->IsMagnitudeLog == true)
    {
        for (uint32_t Slot = 0; Slot < (uint32_t)FrequencyBarCount; Slot++)
        {
            float Mag_dB = Spectrum->Magnitude[Slot];

            Mag_dB = clamp_f32(Mag_dB, MinDB, 0.0f);

            float Normalized = ((Mag_dB - MinDB) / (-MinDB));
            Normalized = clamp_f32(Normalized, 0.0f, 1.0f);

            uint32_t Height = (uint32_t)(Normalized * (float)MagnitudeBarCount + 0.5f);

            if (Height > MagnitudeBarCount)
                Height = MagnitudeBarCount;

            DisplayMagnitudeBars[Slot] = (uint8_t)Height;
        }
    }
    else
    {
        float MaxValue = 0.0f;

        for (uint32_t Slot = 0; Slot < (uint32_t)FrequencyBarCount; Slot++)
        {
            float Value = Spectrum->Magnitude[Slot];

            Value = applyLinearCompression(Value, LinearCompression);

            if (Value > MaxValue)
                MaxValue = Value;
        }

        if (MaxValue < 1e-12f)
            MaxValue = 1e-12f;

        for (uint32_t Slot = 0; Slot < (uint32_t)FrequencyBarCount; Slot++)
        {
            float Value = Spectrum->Magnitude[Slot];

            Value = applyLinearCompression(Value, LinearCompression);

            float Normalized = (Value / MaxValue);
            Normalized = clamp_f32(Normalized, 0.0f, 1.0f);

            uint32_t Height = (uint32_t)(Normalized * (float)MagnitudeBarCount + 0.5f);

            if (Height > MagnitudeBarCount)
                Height = MagnitudeBarCount;

            DisplayMagnitudeBars[Slot] = (uint8_t)Height;
        }
    }

    return(true);
}