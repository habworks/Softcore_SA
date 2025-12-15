//------------------------------------------------------------------------------
// IMR_ADC_7476A_DEFS.vh
// Declarations for AD7476A 12-bit SPI ADC with AXI-Lite register map
//------------------------------------------------------------------------------
`ifndef IMR_ADC_7476A_DEFS_VH
`define IMR_ADC_7476A_DEFS_VH

`define TRUE                1'b1
`define FALSE               1'b0

`define STATE_IDLE          3'd0  // waiting for start
`define STATE_START         3'd1  // assert CS low, prime divider
`define STATE_SHIFT         3'd2  // clock in 16-bit frame
`define STATE_LATCH         3'd3  // move sample to registers
`define STATE_QUIET         3'd4  // CS high quiet period (tQUIET)
`define STATE_NEXT_CONV     3'd5  // prepare for next conversion in continuous mode

//------------------------- Register Map (byte offsets) ------------------------
`define REG_CTRL_OFFSET     32'h00  // Control
`define REG_STATUS_OFFSET   32'h04  // Status
`define REG_DATA_A_OFFSET   32'h08  // Latest sample
`define REG_DATA_B_OFFSET   32'h0C  // Previous sample
`define REG_IRQ_OFFSET      32'h10  // Interrupt 

//----------------------------- CTRL bitfields ---------------------------------
`define CTRL_EN_BIT         0   // enable engine
`define CTRL_START_BIT      1   // write 1 = single-shot start pulse
`define CTRL_CONT_BIT       2   // 0 = Single, 1 = continuous conversions
`define CTRL_CLKDIV_LSB     4   // SCLK divider field [7:4] - Total 4 bits
`define CTRL_CLKDIV_MSB     7   // SCLK = SYSCLK / (2*(N+1)) - Total 4 bits
`define CTRL_CONT_CNT_LSB   8   // Contineous Conversion Count LSB [19:8] - Total 12bits (4096 Max Conversions)
`define CTRL_CONT_CNT_MSB   19  // Contineous Conversion Count MSB

//----------------------------- IRQ bitfields ---------------------------------
`define IRQ_EN_BIT          0   // enable the IRQ
`define IRQ_CLR_BIT         1   // enable clear the pending irq

//---------------------------- STATUS bitfields --------------------------------
`define STATUS_BUSY_BIT     0   // 1 = converting
`define STATUS_RDY_BIT      1   // 1 = new sample available (sticky)
`define STATUS_ERR_LSB      2   // Error detected 3 bits 
`define STATUS_ERR_MSB      4   // Error detected 3 bits
`define STATUS_STATE_LSB    8   // Present State 3 bits
`define STATUS_STATE_MSB    10  // Present State 3 bits

//---------------------------- STATUS DEFINES AND FIELDS------------------------
`define STATUS_ERR_NONE     3'b000  // No error
`define STATUS_ERR_CONVERT  3'b001  // Invalid conversion
`define STATUS_ERR_FIELD    Status_Register[`STATUS_ERR_MSB:`STATUS_ERR_LSB]
`define STATUS_STATE_FIELD  Status_Register[`STATUS_STATE_MSB:`STATUS_STATE_LSB]
`define STATUS_REV_FIELD    Status_Register[31:28]  // ADC IP Revision 4 bits
`define STATUS_C_CNT_FIELD  Status_Register[27:16]  // Conversion Count value for continuous mode 12 bits
`define STATUS_DEBUG_FIELD  Status_Register[15:12]  // Reserved for debug use 4 bits

//------------------------------ ADC framing -----------------------------------
`define ADC_BITS            12  // AD7476A resolution
`define FRAME_CLKS          16  // 16 SCLKs per conversion frame

// tQUIET >= 50 ns.  With 100 MHz SYSCLK (10 ns), 5 cycles meet min.  Use 6.
`define QUIET_SYS_CLKS      6

`endif