# CLOCK AND EXTERNAL RESET
# Clock input (100 MHz single-ended clock on E3)
set_property PACKAGE_PIN E3 [get_ports {CLK_100MHZ}]
set_property IOSTANDARD LVCMOS33 [get_ports {CLK_100MHZ}]
# Reset input (PUSH BTN0 on D9)
set_property PACKAGE_PIN D9 [get_ports {RST_PB}]
set_property IOSTANDARD LVCMOS33 [get_ports {RST_PB}]
set_property PULLDOWN true [get_ports {RST_PB}]


# TEST SWITCH x2 AND PUSH BUTTON INPUTS x3
# Switch input (SW0 on A8)
set_property PACKAGE_PIN A8 [get_ports {SW_0}]
set_property IOSTANDARD LVCMOS33 [get_ports {SW_0}]
# Switch input (SW1 on C11)
set_property PACKAGE_PIN C11 [get_ports {SW_1}]
set_property IOSTANDARD LVCMOS33 [get_ports {SW_1}]
# Switch input (SW2 on C10)
#set_property PACKAGE_PIN C10 [get_ports {SW_2[0]}]
#set_property IOSTANDARD LVCMOS33 [get_ports {SW_2[0]}]
# TEST PUSH BUTTON INPUTS
# TEST 1 (PUSH BTN1 on C9)
set_property PACKAGE_PIN C9 [get_ports {PB_1}]
set_property IOSTANDARD LVCMOS33 [get_ports {PB_1}]
# TEST 2 (PUSH BTN2 on B9)
set_property PACKAGE_PIN B9 [get_ports {PB_2}]
set_property IOSTANDARD LVCMOS33 [get_ports {PB_2}]
# TEST 3 (PUSH BTN3 on B8)
set_property PACKAGE_PIN B8 [get_ports {PB_3}]
set_property IOSTANDARD LVCMOS33 [get_ports {PB_3}]
# Remove no_input_delay warnings - these inputs not tied to clock
set_false_path -from [get_ports PB_*]


# TIMER OUTPUTS x2
# TIMER 0 output PIN (TIM0 on JA1 G13)
set_property PACKAGE_PIN G13 [get_ports {gpio2_io_o_0[0]}]
set_property IOSTANDARD LVCMOS33 [get_ports {gpio2_io_o_0[0]}]
# TIMER 1 output PIN (TIM1 on JA2 B11)
set_property PACKAGE_PIN B11 [get_ports {gpio2_io_o_0[1]}]
set_property IOSTANDARD LVCMOS33 [get_ports {gpio2_io_o_0[1]}]


# UART
# UART TX output PIN (JD7 E2) 
set_property PACKAGE_PIN E2 [get_ports {UART_TX}]
set_property IOSTANDARD LVCMOS33 [get_ports {UART_TX}]
# UART RX input PIN (JD8 D2)
set_property PACKAGE_PIN D2 [get_ports {UART_RX}]
set_property IOSTANDARD LVCMOS33 [get_ports {UART_RX}]
# UART IRQ output PIN (UART_IRQ on JD3 F4)
#set_property PACKAGE_PIN F4 [get_ports {UART_IRQ}]
#set_property IOSTANDARD LVCMOS33 [get_ports {UART_IRQ}]


#SSD1309 OLED DISPLAY
#CS OUPTUT PIN (JD1 D4)
set_property PACKAGE_PIN D4 [get_ports {gpio2_io_o_0[4]}]
set_property IOSTANDARD LVCMOS33 [get_ports {gpio2_io_o_0[4]}]
# Command(0) / Data(1) PIN (JD2 D3)
set_property PACKAGE_PIN D3 [get_ports {gpio2_io_o_0[3]}]
set_property IOSTANDARD LVCMOS33 [get_ports {gpio2_io_o_0[3]}]
# RESET (ACTIVE LOW) PIN (JD3 F4)
set_property PACKAGE_PIN F4 [get_ports {gpio2_io_o_0[2]}]
set_property IOSTANDARD LVCMOS33 [get_ports {gpio2_io_o_0[2]}]
#MOSI OUPTUT PIN (JD4 F3)
set_property PACKAGE_PIN F3 [get_ports {DISPLAY_MOSI}]
set_property IOSTANDARD LVCMOS33 [get_ports {DISPLAY_MOSI}]
#SCLK OUTPUT PIN (JD10 G2)
set_property PACKAGE_PIN G2 [get_ports {DISPLAY_SCLK}]
set_property IOSTANDARD LVCMOS33 [get_ports {DISPLAY_SCLK}]
#MISO INPUT PIN (JD9 H2) ***NOT USED***
set_property PACKAGE_PIN H2 [get_ports {DISPLAY_MISO}]
set_property IOSTANDARD LVCMOS33 [get_ports {DISPLAY_MISO}]


# ADC DUAL 7476A
# ADC_SCLK PIN (JC4 on V11)
set_property PACKAGE_PIN V11 [get_ports {ADC_SCLK}]
set_property IOSTANDARD LVCMOS33 [get_ports {ADC_SCLK}]
# ADC_CS_n PIN (JC1 on U12)
set_property PACKAGE_PIN U12 [get_ports {ADC_CS_n}]
set_property IOSTANDARD LVCMOS33 [get_ports {ADC_CS_n}]
# ADC_MISO_A PIN (JC2 on V12)
set_property PACKAGE_PIN V12 [get_ports {ADC_MISO_A}]
set_property IOSTANDARD LVCMOS33 [get_ports {ADC_MISO_A}]
# ADC_MISO_B PIN (JC3 on V10)
set_property PACKAGE_PIN V10 [get_ports {ADC_MISO_B}]
set_property IOSTANDARD LVCMOS33 [get_ports {ADC_MISO_B}]
# ADC_IP_IRQ PIN (JC7 on U14)
set_property PACKAGE_PIN U14 [get_ports {ADC_IP_IRQ}]
set_property IOSTANDARD LVCMOS33 [get_ports {ADC_IP_IRQ}]
# ADC_IRQ_DONE PIN (JC8 on V14)
set_property PACKAGE_PIN V14 [get_ports {ADC_IRQ_DONE}]
set_property IOSTANDARD LVCMOS33 [get_ports {ADC_IRQ_DONE}]


#MICRO-SD
#CS OUPTUT PIN (JB1 E15)
set_property PACKAGE_PIN E15 [get_ports {USD_CSn[0]}]
set_property IOSTANDARD LVCMOS33 [get_ports {USD_CSn[0]}]
#MOSI OUPTUT PIN (JB2 E16)
set_property PACKAGE_PIN E16 [get_ports {USD_MOSI}]
set_property IOSTANDARD LVCMOS33 [get_ports {USD_MOSI}]
#MISO INPUT PIN (JB3 D15)
set_property PACKAGE_PIN D15 [get_ports {USD_MISO}]
set_property IOSTANDARD LVCMOS33 [get_ports {USD_MISO}]
#SCLK OUTPUT PIN (JB4 C15)
set_property PACKAGE_PIN C15 [get_ports {USD_SCLK}]
set_property IOSTANDARD LVCMOS33 [get_ports {USD_SCLK}]
# CARD DETECT (JB9 on K15)
set_property PACKAGE_PIN K15 [get_ports {USD_CD}]
set_property IOSTANDARD LVCMOS33 [get_ports {USD_CD}]


#TEST SIGNALS
# MB RESET (JA3 ON A11)
set_property PACKAGE_PIN A11 [get_ports {MB_RST}]
set_property IOSTANDARD LVCMOS33 [get_ports {MB_RST}]
#DISPLAY_CSn (JA4)
set_property PACKAGE_PIN D12 [get_ports {MB_CLK}]
set_property IOSTANDARD LVCMOS33 [get_ports {MB_CLK}]
#NOT USED(JA7 ON D13)
set_property PACKAGE_PIN D13 [get_ports {gpio2_io_o_0[5]}]
set_property IOSTANDARD LVCMOS33 [get_ports {gpio2_io_o_0[5]}]
#DISPLAY_CSn (JA8 ON B18)
set_property PACKAGE_PIN B18 [get_ports {DISPLAY_CSn[0]}]
set_property IOSTANDARD LVCMOS33 [get_ports {DISPLAY_CSn[0]}]


# TEST LEDS
# DDR3 CALIBRATION COMPLETE ACTIVE HIGH (LED_4 on H5)
set_property PACKAGE_PIN H5 [get_ports {LED_4}]
set_property IOSTANDARD LVCMOS33 [get_ports {LED_4}]
# GENERIC LED USE (LED_5 on J5)
set_property PACKAGE_PIN J5 [get_ports {gpio_io_o_0[0]}]
set_property IOSTANDARD LVCMOS33 [get_ports {gpio_io_o_0[0]}]
# GENERIC LED USE (LED_6 on T9)
set_property PACKAGE_PIN T9 [get_ports {gpio_io_o_0[1]}]
set_property IOSTANDARD LVCMOS33 [get_ports {gpio_io_o_0[1]}]
# GENERIC LED USE (LED_7 on T10)
set_property PACKAGE_PIN T10 [get_ports {gpio_io_o_0[2]}]
set_property IOSTANDARD LVCMOS33 [get_ports {gpio_io_o_0[2]}]