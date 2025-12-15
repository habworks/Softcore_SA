//------------------------------------------------------------------------------
// ADC 7476A Dual-Channel Controller
//------------------------------------------------------------------------------
// Description:
//   This module implements a controller for dual ADC7476A analog-to-digital converters,
//   supporting both single-shot and continuous conversion modes with interrupt handling.
//
// Operation Modes:
//   1. Single Conversion: One complete conversion cycle on both channels
//   2. Continuous Conversion: Multiple conversions up to specified count
//
// Signal Interface:
//   Clock/Reset:
//   - SysClk: System clock input (100MHz)
//   - RST_n: Active-low reset
//
//   Control Interface:
//   - Ctrl_Register: Control settings and operation mode
//     * ChipEnable: Global enable for ADC operation
//     * SingleConversion: Single conversion mode
//     * ContinuousConversion: Continuous conversion mode
//     * ClockDivider_N: Sets ADC clock frequency (SCLK = SysClk/(2*(N+1)))
//     * TotalConversions: Number of conversions in continuous mode
//
//   - IRQ_Register: Interrupt control
//     * IRQ_Enable: Enable interrupt generation
//     * IRQ_Clear: Clear active interrupt
//
//   Status/Data Interface:
//   - Status_Register: Operation status and debugging
//     * StatusBusy: Active conversion in progress
//     * StatusReady: Data ready for reading
//     * StatusError: Error conditions
//     * StatusDebug: Debug information
//   - ADC_Data_A_Register: Channel A conversion result (12-bit)
//   - ADC_Data_B_Register: Channel B conversion result (12-bit)
//
// ADC Interface:
//   - MISO_A/B: Serial data input from both ADCs
//   - SCLK: Serial clock output to ADCs
//   - CS_n: Active-low chip select
//
// FSM States and Flow:
//   1. IDLE: Wait for conversion trigger
//      → Enters START when ChipEnable and conversion mode active
//
//   2. START: Prepare for conversion
//      → Waits for !IP_IRQ
//      → Asserts CS_n
//      → Moves to SHIFT when ready
//
//   3. SHIFT: Data acquisition (16 clock cycles)
//      → Generate SCLK
//      → Sample MISO on rising edge
//      → Shift data into registers
//      → Move to LATCH after 16 bits
//
//   4. LATCH: Complete conversion
//      → Store converted data
//      → Set StatusReady
//      → Move to QUIET
//
//   5. QUIET: Inter-conversion delay
//      → Wait QUIET_SYS_CLKS cycles
//      → Deassert CS_n
//      → Move to NEXT_CONV or IDLE
//
//   6. NEXT_CONV: Setup next conversion (continuous mode)
//      → Reset bit counters
//      → Move to START for next conversion
//
// Timing:
//   - ADC Clock: Configurable frequency via ClockDivider_N (Cannot exceed 20MHz)
//     * Example frequencies:
//       N=3: 16.67MHz
//       N=4: 12.50MHz
//       N=5: 10.00MHz
//
// Interrupt Handling:
//   - IP_IRQ asserted when:
//     * StatusReady is set (conversion complete)
//     * IRQ_Enable is active
//   - Cleared by:
//     * IRQ_Clear signal
//     * Reset
//
// Debug Features:
//   - StatusDebug register tracks state transitions and conditions
//   - Conversion counter for continuous mode
//   - Error detection and reporting
//
// Notes:
//   - CS_n timing critical for proper ADC operation
//   - Interrupt must be cleared before starting new conversion
//   - Supports up to 4096 continuous conversions
//------------------------------------------------------------------------------
`timescale 1ns/1ps
`include "IMR_ADC_7476A_X2_Def.vh"

module IMR_ADC_7476A_X2_Core
(
  // Clock / Reset
  input  logic        SysClk,
  input  logic        RST_n,

  // Control + Interrupt enable from AXI regs (e.g., slv_reg0, slv_reg4)
  input  logic [31:0] Ctrl_Register,
  input  logic [31:0] IRQ_Register,

  // Readback to AXI (STATUS / DATA_A / DATA_B)
  output logic [31:0] Status_Register,
  output logic [31:0] ADC_Data_A_Register,
  output logic [31:0] ADC_Data_B_Register,

  // External ADC pins
  input  logic        MISO_A,
  input  logic        MISO_B,
  output logic        SCLK,
  output logic        CS_n,

  // Interrupt to system
  output logic        IP_IRQ
);

  // --------------------------
  // Control Register extraction
  // --------------------------
  logic ChipEnable;
  logic SingleConversion;
  logic ContinuousConversion;
  logic [(`CTRL_CLKDIV_MSB - `CTRL_CLKDIV_LSB):0] ClockDivider_N; 
  logic [(`CTRL_CONT_CNT_MSB - `CTRL_CONT_CNT_LSB):0] TotalConversions; 
  assign ChipEnable           = Ctrl_Register[`CTRL_EN_BIT];
  assign ClockDivider_N       = Ctrl_Register[`CTRL_CLKDIV_MSB:`CTRL_CLKDIV_LSB];
  assign SingleConversion     = Ctrl_Register[`CTRL_START_BIT] & ~Ctrl_Register[`CTRL_CONT_BIT];
  assign ContinuousConversion = Ctrl_Register[`CTRL_START_BIT] & Ctrl_Register[`CTRL_CONT_BIT];
  assign TotalConversions     = Ctrl_Register[`CTRL_CONT_CNT_MSB:`CTRL_CONT_CNT_LSB];

  // --------------------------
  // Status Register extraction
  // --------------------------
  logic [2:0] StatusError;         // 3 bits any value other than 0 indicates error
  logic [3:0] StatusDebug;

  // --------------------------
  // Interrupt Register extraction
  // --------------------------
  logic IRQ_Enable;
  logic IRQ_Clear;
  assign IRQ_Enable = IRQ_Register[`IRQ_EN_BIT];
  assign IRQ_Clear = IRQ_Register[`IRQ_CLR_BIT];

  // --------------------------
  // State / Counters / Clock
  // --------------------------
  logic [2:0] State;              // FSM State
  logic [4:0] ShiftBitCount;      // 0..15 (16 SCLK edges where we sample)
  logic [(`CTRL_CLKDIV_MSB - `CTRL_CLKDIV_LSB):0] ClockDividerCount;  // 0..ClockDivider_N for half-period ticks
  logic [11:0]ConversionCount;    // 0..4096 Total Conversions possible
  logic ADC_CLK;                  // Clock used to drive ADC
  logic [7:0] QuietCnt;           // SYSCLK cycles after CS_n high

  // --------------------------
  // Shift Registers and Flags
  // --------------------------
  // Shift registers (capture 16-bit frame, MSB-first)
  logic [15:0] ADC_Shift_A; 
  logic [15:0] ADC_Shift_B;
  // Status flags
  logic StatusBusy;               // A conversion is in progress
  logic StatusReady;              // A conversion is ready

  // --------------------------
  // Output assigns (Combinational statments)
  // --------------------------
  assign SCLK = ADC_CLK;
  assign CS_n = (State == `STATE_START || State == `STATE_SHIFT || State == `STATE_LATCH) ? 1'b0 : 1'b1;   // ADC Chip Select (active low)
  assign IP_IRQ = (IRQ_Enable & StatusReady);   // Assert Interrupt (data ready) when Status Ready and Interrupt enabled


  // --------------------------
  // Always combinational - not clock dependent
  // --------------------------
  always_comb
  begin
    // First clear all bits, then set specific fields
    Status_Register = '0;
    Status_Register[`STATUS_BUSY_BIT] = StatusBusy;
    Status_Register[`STATUS_RDY_BIT] = StatusReady;
    `STATUS_ERR_FIELD = StatusError;
    `STATUS_STATE_FIELD = State;
    `STATUS_C_CNT_FIELD = ConversionCount;
    `STATUS_DEBUG_FIELD = StatusDebug;
    `STATUS_REV_FIELD = 4'd2; 
    // { ... , ... } is the concatenation operator 20 bits of 0's and 12bits from Shift_X (32b total)
    // Loads channel data with 20 MSBs = 0s + 12 LSBs from ADC shift register
    ADC_Data_A_Register = {20'd0, ADC_Shift_A[11:0]};
    ADC_Data_B_Register = {20'd0, ADC_Shift_B[11:0]};
  end


  // Rising-edge detect on Single bit (treats write-1 as a pulse)
  logic Single_Delay_1_Clk;
  always_ff @(posedge SysClk or negedge RST_n)
  begin
    if (!RST_n) 
        Single_Delay_1_Clk <= 1'b0;
    else        
        Single_Delay_1_Clk <= SingleConversion;
  end
  wire SingleRisingEdge = (SingleConversion & ~Single_Delay_1_Clk);
  
  
  // Detect rising edge of internal SCLK (sampling moment)
  logic ADC_CLK_Delay_1_Clk;
  always_ff @(posedge SysClk or negedge RST_n)
  begin
    if (!RST_n) 
        ADC_CLK_Delay_1_Clk <= 1'b0;
    else        
        ADC_CLK_Delay_1_Clk <= ADC_CLK;
  end
  wire ADC_CLK_RisingEdge = (ADC_CLK & ~ADC_CLK_Delay_1_Clk);


 // Rising-edge detect on Continuous bit (treats write-1 as a pulse)
  logic Continuous_Delay_1_Clk;
  always_ff @(posedge SysClk or negedge RST_n)
  begin
    if (!RST_n) 
        Continuous_Delay_1_Clk <= 1'b0;
    else        
        Continuous_Delay_1_Clk <= ContinuousConversion;
  end
  wire ContinuousRisingEdge = (ContinuousConversion & ~Continuous_Delay_1_Clk);

  
  // Handle Status Ready flag - Interrupt generation and clearing
  always_ff @(posedge SysClk or negedge RST_n)
  begin
    if (!RST_n)
    begin
      StatusReady <= `FALSE; 
    end

    // Clear the Interrupt
    else if (IRQ_Clear)
    begin
      StatusReady <= `FALSE; 
    end

    // Set the Status Ready flag when in LATCH state - this will trigger the interrupt
    else if (State == `STATE_LATCH)
    begin
      StatusReady <= `TRUE;
    end
  end



  // --------------------------
  // Clock divider (generates ADC_CLK toggles in SHIFT)
  // --------------------------
  // Half-period tick at (ClockDividerCount == ClockDivider_N); ADC_CLK toggles then.
  // SysClk = 100MHz
  // You count ClockDividerCount SysClk cycles to toggle ADC_CLK, ADC_CLK must toogle twice for one period
  // If ClockDividerCount = 3, ADC_CLK = 16.6667MHz, ClockDividerCount = 4, ADC_CLK = 12.5MHz, ClockDividerCount = 5, ADC_CLK = 10.0MHz
  always_ff @(posedge SysClk or negedge RST_n)
  begin
    if (!RST_n)
    begin
      ClockDividerCount  <= '0;
      ADC_CLK <= 1'b0;
    end

    else if ((State == `STATE_SHIFT) || (State == `STATE_LATCH))
    begin
      if (ClockDividerCount == (ClockDivider_N - 1))
      begin
        ClockDividerCount  <= '0;
        ADC_CLK <= ~ADC_CLK;
      end
      else
      begin
        ClockDividerCount <= ClockDividerCount + 1'b1;
      end
    end

    else if ((State == `STATE_QUIET) || (State == `STATE_NEXT_CONV))  
    begin
      ClockDividerCount <= '0;
      ADC_CLK <= 1'b0; // idle low when not shifting
    end
  end


  // --------------------------
  // Main FSM (Finite State Machine)
  // Flow:
  // IDLE -> START -> SHIFT -> LATCH -> QUIET -> (NEXT_CONV -> START ...) or IDLE
  // 
  // Idle State: Waiting for ChipEnable and Single/Continuous start command
  // Start State: Assert CS_n low, prepare for shifting - cannot start if IRQ is asserted
  // Shift State: Shift in 16 bits from MISO on each rising edge of ADC_CLK for 16 bits on both ADC channels 
  // Latch State: Move shifted data to registers, set Status Ready flag
  // Quiet State: Wait tQUIET cycles with CS_n high before next conversion
  // Next Conversion State: Prepare for next conversion (only for continuous mode)
  // --------------------------
  always_ff @(posedge SysClk or negedge RST_n)
  begin
    if (!RST_n)
    begin
      State <= `STATE_IDLE;
      StatusBusy <= `FALSE;
      ShiftBitCount <= '0;
      QuietCnt <= '0;
      ConversionCount <= '0;
      ADC_Shift_A <= '0;
      ADC_Shift_B <= '0;
      StatusDebug <= 4'b0000;
    end

    else
    begin
      unique case (State)

        `STATE_IDLE:
        begin
          StatusBusy <= `FALSE;
          if (ChipEnable && (SingleRisingEdge || ContinuousRisingEdge))
          begin
            ShiftBitCount <= '0;
            ConversionCount <= '0;
            State <= `STATE_START;
          end
        end

        `STATE_START:
        begin
          if (!IP_IRQ)
          begin
            StatusDebug <= 4'b0000;
            StatusBusy <= `TRUE;
            State <= `STATE_SHIFT;
          end
        end

        `STATE_SHIFT:
        begin
          // On each rising edge of SCLK, sample MISO and increment bit count
          if (ADC_CLK_RisingEdge)
          begin
            ShiftBitCount <= ShiftBitCount + 5'd1;
            // MSB-first shift-in
            ADC_Shift_A <= {ADC_Shift_A[14:0], MISO_A};
            ADC_Shift_B <= {ADC_Shift_B[14:0], MISO_B};
            if (ShiftBitCount == (`FRAME_CLKS - 1))
            begin
              // Completed 16 captures. Latch done; Status Ready set data can be read.
              State <= `STATE_LATCH;
            end
          end
        end

        `STATE_LATCH:
        begin
            QuietCnt <= '0;
            State <= `STATE_QUIET;
        end

        `STATE_QUIET:
        begin
          if (QuietCnt >= (`QUIET_SYS_CLKS - 1))
          begin
            // Handle case of contineous conversion
            if (ContinuousConversion)
            begin
              if (ConversionCount == TotalConversions - 12'd1)
              begin
                StatusError <= `STATUS_ERR_NONE;
                StatusBusy <= `FALSE;
                State <= `STATE_IDLE;
                StatusDebug <= StatusDebug | 4'b0010;
              end
              else
              begin
                ConversionCount <= ConversionCount + 12'd1;
                State <= `STATE_NEXT_CONV;
                StatusDebug <= StatusDebug | 4'b0001;
              end
            end

            // Handle case of Single Conversion
            if (SingleConversion)
            begin
              StatusDebug <= StatusDebug | 4'b1000;
              StatusError <= `STATUS_ERR_NONE;
              StatusBusy <= `FALSE;
              State <= `STATE_IDLE;
            end
          end

          // Increment quiet counter
          else
          begin
            QuietCnt <= QuietCnt + 8'd1;
          end
        end

        `STATE_NEXT_CONV:
        begin
          ShiftBitCount <= '0;
          StatusDebug <= StatusDebug | 4'b0100;
          State <= `STATE_START;
        end

        default: State <= `STATE_IDLE;
      endcase
    end
  end

endmodule
