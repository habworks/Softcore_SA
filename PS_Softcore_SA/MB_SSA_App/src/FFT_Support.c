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
 * @note            This module uses the KISSFFT (Keep It Simple, Stupid FFT) open-source library.
 *                  Repository: https://github.com/mborgerding/kissfft
 *                  KISSFFT is a lightweight, portable FFT implementation used here for real-valued
 *                  frequency-domain analysis in an embedded environment.
 *
 * @note            FFT Theory Overview:
 *                  The Fast Fourier Transform (FFT) converts a time-domain signal into its frequency-
 *                  domain representation.  The time-domain signal consists of discrete samples taken
 *                  at a fixed sampling rate (SampleRate_Hz).  The sampling rate defines how many
 *                  samples per second are acquired from the analog signal.  According to the Nyquist
 *                  criterion, the maximum frequency that can be represented without aliasing is
 *                  one-half of the sampling rate (SampleRate_Hz / 2).  This frequency is known as
 *                  the Nyquist frequency.
 *
 *                  The FFT operates on a fixed number of samples defined by FFT_Size (N).  For
 *                  example, if FFT_Size is 1024, then 1024 consecutive time-domain samples are
 *                  transformed into 1024 complex frequency-domain coefficients.  When the input
 *                  signal is real-valued (as is the case for PCM audio), the FFT output exhibits
 *                  conjugate symmetry.  Because of this symmetry, only the first (N/2 + 1) bins
 *                  contain unique frequency information.  These bins represent frequencies from
 *                  0 Hz (DC) up to the Nyquist frequency.
 *
 *                  Each FFT bin represents a specific frequency interval.  The spacing between
 *                  adjacent bins is called the Resolution Bandwidth (RBW) and is computed as:
 *                      RBW = SampleRate_Hz / FFT_Size
 *                  This value determines the frequency resolution of the analysis.  For example,
 *                  with a 16 kHz sampling rate and FFT_Size of 1024, the RBW is 15.625 Hz.  This
 *                  means each bin represents a 15.625 Hz wide frequency band.
 *
 *                  Bin 0 corresponds to DC (0 Hz).  Bin 1 corresponds to RBW Hz.  Bin k corresponds
 *                  to k * RBW Hz.  The highest meaningful bin is N/2, which corresponds to the
 *                  Nyquist frequency.  Bins above N/2 contain redundant information and are not
 *                  used for real-valued signal analysis.
 *
 *                  Increasing FFT_Size improves frequency resolution (smaller RBW) but increases
 *                  computational load and latency.  Decreasing FFT_Size reduces latency and CPU
 *                  requirements but increases RBW, reducing frequency discrimination.  Therefore,
 *                  FFT_Size is a trade-off between resolution, responsiveness, and processing cost.
 *
 *                  In this application, the FFT is used for audio spectrum analysis.  The raw
 *                  frequency bins are grouped into a smaller number of display slots to match
 *                  the available display resolution.  The magnitude of each bin is computed from
 *                  the complex FFT output and optionally converted to dB for perceptually meaningful
 *                  visualization.
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

// STATIC FUNCTIONS
static float clamp_f32(float Value, float Minimum, float Maximum);

// GLOBAL FFT RELATED
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



/********************************************************************************************************
* @brief Init the FFT for use
*
* @author original: Hab Collector \n
*
* @note: Uses KissFFT library
* @note: Sample rate only needs to meet Nquist (2x frequency of signal)
* 
* @param FFT: Pointer to FFT structure
* @param SampleRate_Hz: Sample rate of the signal (can be any signal)
*
* @return: True if OK
*
* STEP 1: Validate input pointer
* STEP 2: Store FFT size and sample rate
* STEP 3: Compute resolution bandwidth (RBW)
* STEP 4: Clear runtime flags
* STEP 5: Allocate KissFFT real FFT configuration
********************************************************************************************************/
bool init_FFT(Type_FFT *FFT, uint32_t SampleRate_Hz)
{
    // STEP 1: Validate input pointer
    if (FFT == NULL)
        return(false);

    // STEP 2: Store FFT size and sample rate
    FFT->Size = FFT_SIZE;
    FFT->SampleRate_Hz = SampleRate_Hz;

    // STEP 3: Compute resolution bandwidth (RBW)
    FFT->RBW = ((float)SampleRate_Hz / (float)FFT_SIZE);

    // STEP 4: Clear runtime flags
    FFT->FrameReady = false;

    // STEP 5: Allocate KissFFT real FFT configuration
    FFT->FFT_Config = kiss_fftr_alloc(FFT_SIZE, 0, NULL, NULL);
    if (FFT->FFT_Config == NULL)
        return(false);

    return(true);

} // END OF init_FFT



/********************************************************************************************************
* @brief DeInit the FFT
*
* @author original: Hab Collector \n
*
* @note: Uses KissFFT library
* 
* @param FFT: Pointer to FFT structure
*
* STEP 1: Free memory that was allocated for the FFT
********************************************************************************************************/
void deinit_FFT(Type_FFT *FFT)
{
    // STEP 1: Free memory that was allocated for the FFT
    if (FFT->FFT_Config != NULL)
    {
        kiss_fftr_free(FFT->FFT_Config);
        FFT->FFT_Config = NULL;
    }

} // END OF deinit_FFT



/********************************************************************************************************
* @brief Process the FFT Frame.  This function is to be called when the FFT number of samples has been collected.
* It will copy the collected samples (FFT->Samples) to its internal buffer allowing the sample buffer to be
* filled by the audio ISR.  It will then apply the FFT window function and call the FFT function.  As a last
* step in the process the Amplitude for each bin is calculated.
*
* @author original: Hab Collector \n
*
* @note: Uses KissFFT library
* @note: Sample rate only needs to meet Nquist (2x frequency of signal)
* @note: Audio PCM data is already centered about the 0 since PCM is int16_t (both positive and negative)
* 
* @param FFT: Pointer to FFT structure
* @param BinMagnitudes: Buffer in which to fill the Bin Amplitudes
* @param BinCount: Number of bins to fill - it does not have to be FFT_MAX_BINS - For example if you only care about low frequency you may only want the first 200
*
* @return: True if OK
*
* STEP 1: Place in fast memory to speed things up
* STEP 2: Validate inputs and confirm a frame is ready
* STEP 3: Snapshot the FFT sample buffer and clear FrameReady
* STEP 4: Apply Hann window to the snapshot samples
* STEP 5: Run KissFFT real FFT on the windowed samples - this takes the longest amount of time
* STEP 6: Compute magnitude for each requested bin and store into BinMagnitudes
********************************************************************************************************/
uint32_t FFT_ProcessAudioFrame(Type_FFT *FFT, float *BinMagnitudes, uint32_t BinCount)
{
    // STEP 1: Place in fast memory to speed things up
    static float __attribute__ ((section (".Hab_Fast_Data"))) WindowedSamples[FFT_SIZE];
    static kiss_fft_cpx __attribute__ ((section (".Hab_Fast_Data"))) OutputBins[FFT_MAX_BINS];

    // STEP 2: Validate inputs and confirm a frame is ready
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

    // STEP 3: Snapshot the FFT sample buffer and clear FrameReady
    memcpy(WindowedSamples, (const void *)FFT->Samples, (size_t)(FFT_SIZE * sizeof(float)));
    FFT->FrameReady = false;

    // STEP 4: Apply Hann window to the snapshot samples
    for (uint16_t Index = 0; Index < FFT_SIZE; Index++)
    {
        WindowedSamples[Index] = (WindowedSamples[Index] * FFT->HannWindow[Index]);
    }

    // STEP 5: Run KissFFT real FFT on the windowed samples - this takes the longest amount of time
    kiss_fftr(FFT->FFT_Config, WindowedSamples, OutputBins);

    // STEP 6: Compute magnitude for each requested bin and store into BinMagnitudes
    for (uint32_t Bin = 0; Bin < BinCount; Bin++)
    {
        float Real = (float)OutputBins[Bin].r;
        float Imag = (float)OutputBins[Bin].i;
        BinMagnitudes[Bin] = sqrtf((Real * Real) + (Imag * Imag));  // Amplidute 
        // BinMagnitudes[Bin] = ((Real * Real) + (Imag * Imag));    // Power
    }

    return(true);

} // END OF FFT_ProcessAudioFrame



/********************************************************************************************************
* @brief Clamp a floating point value between a minimum and maximum bound.  This utility function is
* used to enforce safe numeric limits during signal processing and display scaling operations.
* It ensures that computed values such as normalized magnitudes, dB levels, or bar heights do not
* exceed expected ranges, preventing visual distortion, overflow, or unintended behavior.
*
* The purpose of clamping is not to modify the signal itself, but to constrain derived values so that
* downstream logic (such as display mapping or normalization) operates within defined boundaries.
*
* @author original: Hab Collector \n
*
* @param Value: Input floating point value to constrain
* @param Minimum: Lower allowable bound
* @param Maximum: Upper allowable bound
*
* @return: Clamped floating point value within [Minimum, Maximum]
*
* STEP 1: Clamp value within bounds
********************************************************************************************************/
static float clamp_f32(float Value, float Minimum, float Maximum)
{
    // STEP 1: Clamp value within bounds
    if (Value < Minimum)
        return(Minimum);

    if (Value > Maximum)
        return(Maximum);

    return(Value);

} // END OF clamp_f32



/********************************************************************************************************
* @brief Apply optional linear-domain compression to a magnitude value prior to display scaling.
* This function is used when the spectrum is displayed in linear (non-dB) mode.  It modifies the
* dynamic range of the input value to improve visual readability by compressing large variations
* in magnitude.
*
* The purpose of linear compression is to reduce the dominance of large values while preserving
* smaller values so that low-level content remains visible.  Different compression methods may be
* selected depending on desired visual response characteristics.
*
* @note: Never really tested - so IDK - I got the idea from AI
*
* @author original: Hab Collector \n
*
* @param Value:               Input magnitude value (must be >= 0)
* @param LinearCompression:   Compression method selection
*
* @return: Compressed magnitude value in linear domain
*
* STEP 1: Clamp negative inputs to zero to maintain physical validity
* STEP 2: If no compression selected, return Value unchanged
* STEP 3: If SQRT selected, apply square root compression
* STEP 4: If LOG1P selected, apply log1p compression
* STEP 5: Default case returns Value unchanged
********************************************************************************************************/
static float applyLinearCompression(float Value, Type_LinearCompression LinearCompression)
{
    // STEP 1: Clamp negative inputs to zero to maintain physical validity
    if (Value < 0.0f)
        Value = 0.0f;

    // STEP 2: If no compression selected, return Value unchanged
    if (LinearCompression == LINEAR_COMPRESSION_NONE)
        return(Value);

    // STEP 3: If SQRT selected, apply square root compression
    if (LinearCompression == LINEAR_COMPRESSION_SQRT)
        return(sqrtf(Value));

    // STEP 4: If LOG1P selected, apply log1p compression
    if (LinearCompression == LINEAR_COMPRESSION_LOG1P)
        return(log1pf(Value));

    // STEP 5: Default case returns Value unchanged
    return(Value);

} // END OF applyLinearCompression



/********************************************************************************************************
* @brief Build a display-ready audio spectrum frame from raw FFT bin magnitudes.  This function exists
* to translate the high-resolution FFT output (hundreds of bins) into a small, stable, and readable set
* of display bars (example: 18 bars by 8 vertical segments).  The FFT produces data in the frequency
* domain, but the display needs a small number of representative magnitudes that track perceived energy
* across frequency in a way that looks correct to a human observer.  This function performs that mapping
* by grouping FFT bins into frequency “slots,” computing a magnitude for each slot, optionally converting
* the result into dB, and finally scaling the result into integer bar heights for the OLED.
*
* STEP 1 validates all inputs and operating constraints.  It confirms that the input bin array exists,
* the output display buffer exists, and the Spectrum analysis structure is properly configured with
* valid Frequency[] and Magnitude[] storage.  It also confirms that SampleRate_Hz and FFT_Size are not
* zero, and that BinCount matches the real-FFT rule of (FFT_Size/2 + 1) bins (DC through Nyquist).  It
* enforces that the requested display geometry matches the expected compile-time constants, and it also
* enforces that MinDB is negative when logarithmic magnitude mode is enabled.  These checks prevent
* out-of-range indexing, divide-by-zero conditions, and invalid display scaling.
*
* STEP 2 computes the RBW (resolution bandwidth) for the FFT.  RBW is the frequency spacing per bin and
* is equal to SampleRate_Hz / FFT_Size.  With this RBW, each FFT bin index maps directly to a physical
* frequency in Hz, which is needed for the Spectrum->Frequency[] outputs.  This step also defines the
* valid bin range used for display.  Bin 0 (DC) is excluded by setting FirstBin to 1 because DC offsets
* and bias can create misleading energy that is not useful for an audio spectrum display.  LastBin is set
* to BinCount-1, which corresponds to the Nyquist bin, and the function verifies that a non-empty range
* remains after excluding DC.
*
* STEP 3 divides the available FFT bins into FrequencyBarCount slots.  Each slot represents one vertical
* bar on the display, and each slot corresponds to a contiguous range of FFT bins.  The slot boundaries
* are constructed using StartFraction and EndFraction, which represent how far along the full bin range
* each slot begins and ends.  If logarithmic-like spacing is enabled, the fractions are squared, which
* allocates more bars to the low-frequency region where human perception and musical content are often
* more sensitive.  The fractions are converted to integer bin offsets, which yields StartBin and EndBin
* for each slot.  Several guard rules are applied so that bins do not overlap between slots, and so every
* slot has a valid range even when rounding effects occur.
*
* STEP 4 computes two things per slot: a representative frequency and a representative magnitude.  The
* representative frequency is computed using the center bin of the slot multiplied by RBW, which yields
* a physically meaningful slot center frequency in Hz.  The representative magnitude is computed by
* iterating through all bins assigned to the slot and aggregating their magnitudes.  Two aggregation
* methods are supported.  Average magnitude provides a simple mean value across the slot.  RMS magnitude
* provides an energy-weighted result that emphasizes stronger bins and tends to look more natural for
* spectral displays.  Negative bin magnitudes are clamped to zero for physical validity, and then the
* slot magnitude is stored in Spectrum->Magnitude[] in linear units.
*
* STEP 5 optionally converts the slot magnitudes into dB.  This step is used when Spectrum->IsMagnitudeLog
* is true and you want a dB-style display instead of a linear display.  A fixed reference magnitude is
* selected to define what 0 dB means.  In this implementation, the reference is set to 32768.0f to match
* PCM16 full-scale amplitude, which produces a dBFS-like behavior.  Each slot magnitude is clamped to a
* small epsilon to avoid log10(0), then divided by the reference to form a relative ratio.  The ratio is
* converted to dB using 20*log10() because the quantities are amplitude-like.  The result is clamped to
* the user-provided MinDB floor so that extremely small values do not dominate the display with unstable
* negative values.  The result is also clamped at 0 dB so that values above the reference do not produce
* positive dB values that would exceed the intended display range.
*
* STEP 6 converts the final per-slot magnitudes into integer bar heights for the OLED.  In logarithmic
* mode, the dB value is clamped between MinDB and 0 dB and normalized into a 0.0 to 1.0 range.  That
* normalized value is then scaled by MagnitudeBarCount to produce a bar height of 0..MagnitudeBarCount.
* In linear mode, a separate path is used.  The magnitudes can optionally be compressed (sqrt or log1p)
* to improve visibility of low-level content.  The maximum value across all slots is found, and each
* slot is normalized by that maximum so that the tallest bar reaches full height.  The normalized values
* are then scaled into integer heights in the same manner.  In both modes, clamp logic is used to keep
* the output bounded and stable.
*
* The end result is that Spectrum->Frequency[] and Spectrum->Magnitude[] provide a meaningful analysis
* view of the frame, while DisplayMagnitudeBars[] provides an efficient, fixed-range representation that
* the OLED rendering code can draw quickly and consistently.  This separation also allows the UI to use
* the analysis data for future features, while keeping the draw path simple and deterministic.
*
* @author original: Vector \n
* @author modified by: Hab Collector \n
*
* @note: Designed to work with displayAudioSpectrum
* @note: This function was orignally designed by my AI assistant
*
* @param SampleRate_Hz:        Audio sample rate in Hz (example: 16000)
* @param FFT_Size:             FFT size in samples (example: 1024)
* @param BinMagnitudes:        Pointer to FFT bin magnitudes array (bins 0..FFT_Size/2)
* @param BinCount:             Number of valid bins in BinMagnitudes (must equal FFT_Size/2 + 1)
* @param SlotMagnitudeMethod:  Slot aggregation method (AVERAGE or RMS)
* @param LinearCompression:    Compression method used only when IsMagnitudeLog is false
* @param MinDB:                dB floor used when IsMagnitudeLog is true (example: -60.0f)
* @param DisplayMagnitudeBars: Output buffer of length FrequencyBarCount containing bar heights
* @param FrequencyBarCount:    Number of frequency bars (must equal FREQUENCY_SLOTS)
* @param MagnitudeBarCount:    Number of vertical segments per bar (must equal MAX_VERTICAL_BAR_COUNT)
* @param Spectrum:             Output analysis structure containing Frequency[] and Magnitude[] arrays
*
* @return: True if OK
*
* STEP 1: Validate inputs
* STEP 2: Compute RBW and determine valid bin range (ignore DC)
* STEP 3: Build slot bin ranges using linear or log-like frequency spacing
* STEP 4: Compute slot Frequency[] (center frequency) and slot Magnitude[] (average or RMS)
* STEP 5: Convert slot Magnitude[] to dB if enabled
* STEP 6: Convert slot magnitudes into DisplayMagnitudeBars[] (0..MagnitudeBarCount)
********************************************************************************************************/
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
    // STEP 1: Validate inputs
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

    // STEP 2: Compute RBW and determine valid bin range (ignore DC)
    float RBW_Hz = ((float)SampleRate_Hz / (float)FFT_Size);
    uint32_t FirstBin = 1;
    uint32_t LastBin = (BinCount - 1);
    if (LastBin <= FirstBin)
        return(false);

    // STEP 3: Build slot bin ranges using linear or log-like frequency spacing
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

        // STEP 4: Compute slot Frequency[] (center frequency) and slot Magnitude[] (average or RMS)
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

    // STEP 5: Convert slot Magnitude[] to dB if enabled
    if (Spectrum->IsMagnitudeLog == true)
    {
        // Fixed reference magnitude (tune this).
        // If your FFT input is normalized to +/-1.0, start with 1.0f.
        // If your FFT input is PCM16 scale (~32768), start with 32768.0f.
        float Reference = 32768.0f;
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
            if (Mag_dB > 0.0f)
                Mag_dB = 0.0f;
            Spectrum->Magnitude[Slot] = Mag_dB;
        }
    }

    // STEP 6: Convert slot magnitudes into DisplayMagnitudeBars[] (0..MagnitudeBarCount)
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

} // END OF buildAudioSpectrumFrame











/******************************************************************************************************
 * @brief Processes one completed FFT frame and populates magnitude values for selected bins in Signal Mode
 * this function takes the applied signal, applies the necessary offset (to center about the 0), applies 
 * the window function then computes the FFT magtitude results.
 *
 * @author original: Hab Collector \n
 *
 * @note: This function assumes a real FFT where the usable bins are 0 through (FFT_SIZE/2).  Only the bins 
 * in the requested range are converted to magnitude.
 * @note: WindowedSamples and OutputBins are placed in fast memory to reduce processing time for the FFT operation.
 * @note: Low Bin, High Bin allows the calling function to limit the frequency span of interest and with it limit either
 * the pure DC content (offset in bin 0) and or the total amount of calculations that must be performed
 * @note: The input signal is purely positive - it is NOT centered about 0V, but instead offset so 0V is the lowest
 * peak value.  The signal must be offset such that it is centered about the 0V.  This step is conceptually important.  
 * An FFT of the raw biased data would contain a large DC term at bin 0 and nearby low-frequency contamination.  
 * By recentring the samples first, the code focuses the FFT on the AC content that actually represents the signal 
 * shape and harmonics of interest.
 *
 * @param FFT: Pointer to FFT control structure containing:
 * @param BinMagnitudes: Pointer to an array that stores magnitude results for FFT bins.  The array must be sized to at least (FFT_SIZE / 2) + 1 elements. Each index corresponds directly to the FFT bin number.
 * @param LowBin: First FFT bin to compute (inclusive).
 * @param HighBin: Last FFT bin boundary (exclusive).  Valid bins processed are: LowBin ≤ Bin < HighBin.
 *
 * @return True if OK  
 *
 * STEP 1: Place in fast memory to speed things up
 * STEP 2: Validate inputs and confirm a frame is ready
 * STEP 3: Snapshot the FFT sample buffer and clear FrameReady
 * STEP 4: Convert to ADC signed and bias the signal around its center value
 * STEP 5: Apply Hann window to the snapshot samples
 * STEP 6: Run KissFFT real FFT on the windowed samples
 * STEP 7: Compute magnitude for requested bins
 ******************************************************************************************************/
bool FFT_ProcessSignalFrame(Type_FFT *FFT, float *BinMagnitudes, uint16_t LowBin, uint16_t HighBin)
{
    // STEP 1: Place in fast memory to speed things up
    static float __attribute__ ((section (".Hab_Fast_Data"))) WindowedSamples[FFT_SIZE];
    static kiss_fft_cpx __attribute__ ((section (".Hab_Fast_Data"))) OutputBins[(FFT_SIZE/2) + 1];

    // STEP 2: Validate inputs and confirm a frame is ready
    if (FFT == NULL)
        return(false);

    if (BinMagnitudes == NULL)
        return(false);

    if (LowBin >= HighBin)
        return(false);

    if (HighBin > ((FFT_SIZE/2) + 1))
        return(false);

    if (FFT->FFT_Config == NULL)
        return(false);

    if (FFT->FrameReady == false)
        return(false);

    // STEP 3: Snapshot the FFT sample buffer and clear FrameReady
    memcpy(WindowedSamples, (const void *)FFT->Samples, (size_t)(FFT_SIZE * sizeof(float)));
    FFT->FrameReady = false;

    // STEP 4: Convert to ADC signed and bias the signal around its center value
    // Since the input signal can only be positive centering the signal will remove DC offset that is inherent to how it is being fed
    for (uint16_t Index = 0; Index < FFT_SIZE; Index++)
    {
        WindowedSamples[Index] -= ADC_MID_SCALE_COUNT;
    }

    // STEP 5: Apply Hann window to the snapshot samples
    for (uint16_t Index = 0; Index < FFT_SIZE; Index++)
    {
        WindowedSamples[Index] = (WindowedSamples[Index] * FFT->HannWindow[Index]);
    }

    // STEP 6: Run KissFFT real FFT on the windowed samples
    kiss_fftr(FFT->FFT_Config, WindowedSamples, OutputBins);

    // STEP 7: Compute magnitude for requested bins
    for (uint16_t Bin = LowBin; Bin < HighBin; Bin++)
    {
        float Real = (float)OutputBins[Bin].r;
        float Imag = (float)OutputBins[Bin].i;

        BinMagnitudes[Bin] = sqrtf((Real * Real) + (Imag * Imag));  // Amplitude
        // BinMagnitudes[Bin] = ((Real * Real) + (Imag * Imag));     // Power option
    }

    return(true);

} // END OF FFT_ProcessSignalFrame



/******************************************************************************************************
 * @brief Calculates the FFT bin limits corresponding to a requested frequency span
 *
 * @author original: Hab Collector \n
 *
 * @note: The returned bin indices are clamped to the valid FFT bin range [0, MaxBinCount - 1]
 *
 * @param LowSpan: Lower frequency limit of the requested span in Hz aka Start Frequency
 * @param HighSpan: Upper frequency limit of the requested span in Hz aka Stop Frequency
 * @param MaxBinCount: Total number of valid FFT bins covering the 0 Hz to Nyquist range.  Typically FFT_SIZE / 2 for a real FFT
 * @param RBW: Resolution Bandwidth (Hz per bin) RBW = Fs / FFT_SIZE
 * @param LowBin: Pointer to returned lower FFT bin index corresponding to LowSpan
 * @param HighBin: Pointer to returned upper FFT bin index corresponding to HighSpan
 *
 * STEP 1: Validate
 * STEP 2: Convert span frequencies to bin numbers
 * STEP 3: Clamp bin indices to valid range
 ******************************************************************************************************/
bool calculateBinMinMaxFromSpan(float LowSpan, float HighSpan, uint16_t MaxBinCount, float RBW, uint16_t *LowBin, uint16_t *HighBin)
{
    float LowBinFloat;
    float HighBinFloat;

    // STEP 1: Validate
    if (LowSpan == HighSpan)
        return(false);

    // STEP 2: Convert span frequencies to bin numbers
    *LowBin  = (uint16_t)(LowSpan  / RBW);
    *HighBin = (uint16_t)(HighSpan / RBW);

    // STEP 3: Clamp bin indices to valid range
    if (*LowBin == 0)
        *LowBin = 1U;

    if (*LowBin >= MaxBinCount)
        *LowBin = MaxBinCount - 1U;

    if (*HighBin >= MaxBinCount)
        *HighBin = MaxBinCount - 1U;
    
} // END OF calculateBinMinMaxFromSpan

