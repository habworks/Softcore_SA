# Copyright (C) 2023-2024 Advanced Micro Devices, Inc.  All rights reserved.
# SPDX-License-Identifier: MIT
cmake_minimum_required(VERSION 3.16)

###    USER SETTINGS  START    ###
# Below settings can be customized
# User needs to edit it manually as per their needs.
###    DO NOT ADD OR REMOVE VARIABLES FROM THIS SECTION    ###
# -----------------------------------------
# Add any compiler definitions, they will be added as extra definitions
# Example : Adding VERBOSE=1 will pass -DVERBOSE=1 to the compiler.
set(USER_COMPILE_DEFINITIONS
)

# Undefine any previously specified compiler definitions, either built in or provided with a -D option
# Example : Adding MY_SYMBOL will pass -UMY_SYMBOL to the compiler.
set(USER_UNDEFINED_SYMBOLS
"__clang__"
)


# Add any directories below, they will be added as extra include directories.
# Example 1: Adding /proj/data/include will pass -I/proj/data/include.
# Example 2: Adding ../../common/include will consider the path as relative to this component directory.
# Example 3: Adding ${CMAKE_SOURCE_DIR}/data/include to add data/include from this project.

set(USER_INCLUDE_DIRECTORIES
"U8G2/csrc"
)
set(USER_COMPILE_SOURCES
"AXI_IMR_ADC_7476A_DUAL.c"
"AXI_IMR_PL_Revision.c"
"AXI_IRQ_Controller_Support.c"
"AXI_SPI_Display_SSD1309.c"
"AXI_Timer_PWM_Support.c"
"AXI_UART_Lite_Support.c"
"main.c"
"U8G2/csrc/mui.c"
"U8G2/csrc/mui_u8g2.c"
"U8G2/csrc/u8g2_arc.c"
"U8G2/csrc/u8g2_bitmap.c"
"U8G2/csrc/u8g2_box.c"
"U8G2/csrc/u8g2_buffer.c"
"U8G2/csrc/u8g2_button.c"
"U8G2/csrc/u8g2_circle.c"
"U8G2/csrc/u8g2_cleardisplay.c"
"U8G2/csrc/u8g2_d_memory.c"
"U8G2/csrc/u8g2_d_setup.c"
"U8G2/csrc/u8g2_font.c"
"U8G2/csrc/u8g2_fonts.c"
"U8G2/csrc/u8g2_hvline.c"
"U8G2/csrc/u8g2_input_value.c"
"U8G2/csrc/u8g2_intersection.c"
"U8G2/csrc/u8g2_kerning.c"
"U8G2/csrc/u8g2_line.c"
"U8G2/csrc/u8g2_ll_hvline.c"
"U8G2/csrc/u8g2_message.c"
"U8G2/csrc/u8g2_polygon.c"
"U8G2/csrc/u8g2_selection_list.c"
"U8G2/csrc/u8g2_setup.c"
"U8G2/csrc/u8log.c"
"U8G2/csrc/u8log_u8g2.c"
"U8G2/csrc/u8log_u8x8.c"
"U8G2/csrc/u8x8_8x8.c"
"U8G2/csrc/u8x8_byte.c"
"U8G2/csrc/u8x8_cad.c"
"U8G2/csrc/u8x8_capture.c"
"U8G2/csrc/u8x8_debounce.c"
"U8G2/csrc/u8x8_display.c"
"U8G2/csrc/u8x8_d_a2printer.c"
"U8G2/csrc/u8x8_d_ch1120.c"
"U8G2/csrc/u8x8_d_gp1247ai.c"
"U8G2/csrc/u8x8_d_gp1287ai.c"
"U8G2/csrc/u8x8_d_gp1294ai.c"
"U8G2/csrc/u8x8_d_gu800.c"
"U8G2/csrc/u8x8_d_hd44102.c"
"U8G2/csrc/u8x8_d_il3820_296x128.c"
"U8G2/csrc/u8x8_d_ist3020.c"
"U8G2/csrc/u8x8_d_ist3088.c"
"U8G2/csrc/u8x8_d_ist7920.c"
"U8G2/csrc/u8x8_d_ks0108.c"
"U8G2/csrc/u8x8_d_lc7981.c"
"U8G2/csrc/u8x8_d_ld7032_60x32.c"
"U8G2/csrc/u8x8_d_ls013b7dh03.c"
"U8G2/csrc/u8x8_d_max7219.c"
"U8G2/csrc/u8x8_d_pcd8544_84x48.c"
"U8G2/csrc/u8x8_d_pcf8812.c"
"U8G2/csrc/u8x8_d_pcf8814_hx1230.c"
"U8G2/csrc/u8x8_d_s1d15300.c"
"U8G2/csrc/u8x8_d_s1d15721.c"
"U8G2/csrc/u8x8_d_s1d15e06.c"
"U8G2/csrc/u8x8_d_sbn1661.c"
"U8G2/csrc/u8x8_d_sed1330.c"
"U8G2/csrc/u8x8_d_sh1106_64x32.c"
"U8G2/csrc/u8x8_d_sh1106_72x40.c"
"U8G2/csrc/u8x8_d_sh1107.c"
"U8G2/csrc/u8x8_d_sh1108.c"
"U8G2/csrc/u8x8_d_sh1122.c"
"U8G2/csrc/u8x8_d_ssd1305.c"
"U8G2/csrc/u8x8_d_ssd1306_128x32.c"
"U8G2/csrc/u8x8_d_ssd1306_128x64_noname.c"
"U8G2/csrc/u8x8_d_ssd1306_2040x16.c"
"U8G2/csrc/u8x8_d_ssd1306_48x64.c"
"U8G2/csrc/u8x8_d_ssd1306_64x32.c"
"U8G2/csrc/u8x8_d_ssd1306_64x48.c"
"U8G2/csrc/u8x8_d_ssd1306_72x40.c"
"U8G2/csrc/u8x8_d_ssd1306_96x16.c"
"U8G2/csrc/u8x8_d_ssd1306_96x40.c"
"U8G2/csrc/u8x8_d_ssd1309.c"
"U8G2/csrc/u8x8_d_ssd1312.c"
"U8G2/csrc/u8x8_d_ssd1315_128x64_noname.c"
"U8G2/csrc/u8x8_d_ssd1316.c"
"U8G2/csrc/u8x8_d_ssd1317.c"
"U8G2/csrc/u8x8_d_ssd1318.c"
"U8G2/csrc/u8x8_d_ssd1320.c"
"U8G2/csrc/u8x8_d_ssd1322.c"
"U8G2/csrc/u8x8_d_ssd1325.c"
"U8G2/csrc/u8x8_d_ssd1326.c"
"U8G2/csrc/u8x8_d_ssd1327.c"
"U8G2/csrc/u8x8_d_ssd1329.c"
"U8G2/csrc/u8x8_d_ssd1362.c"
"U8G2/csrc/u8x8_d_ssd1363.c"
"U8G2/csrc/u8x8_d_ssd1606_172x72.c"
"U8G2/csrc/u8x8_d_ssd1607_200x200.c"
"U8G2/csrc/u8x8_d_st7302.c"
"U8G2/csrc/u8x8_d_st7305.c"
"U8G2/csrc/u8x8_d_st7511.c"
"U8G2/csrc/u8x8_d_st75160.c"
"U8G2/csrc/u8x8_d_st75161.c"
"U8G2/csrc/u8x8_d_st75256.c"
"U8G2/csrc/u8x8_d_st7528.c"
"U8G2/csrc/u8x8_d_st75320.c"
"U8G2/csrc/u8x8_d_st7539.c"
"U8G2/csrc/u8x8_d_st7565.c"
"U8G2/csrc/u8x8_d_st7567.c"
"U8G2/csrc/u8x8_d_st7571.c"
"U8G2/csrc/u8x8_d_st7586s_erc240160.c"
"U8G2/csrc/u8x8_d_st7586s_jlx320160.c"
"U8G2/csrc/u8x8_d_st7586s_jlx384160.c"
"U8G2/csrc/u8x8_d_st7586s_md240128.c"
"U8G2/csrc/u8x8_d_st7586s_s028hn118a.c"
"U8G2/csrc/u8x8_d_st7586s_ymc240160.c"
"U8G2/csrc/u8x8_d_st7588.c"
"U8G2/csrc/u8x8_d_st7920.c"
"U8G2/csrc/u8x8_d_stdio.c"
"U8G2/csrc/u8x8_d_t6963.c"
"U8G2/csrc/u8x8_d_uc1601.c"
"U8G2/csrc/u8x8_d_uc1604.c"
"U8G2/csrc/u8x8_d_uc1608.c"
"U8G2/csrc/u8x8_d_uc1609.c"
"U8G2/csrc/u8x8_d_uc1610.c"
"U8G2/csrc/u8x8_d_uc1611.c"
"U8G2/csrc/u8x8_d_uc1617.c"
"U8G2/csrc/u8x8_d_uc1628.c"
"U8G2/csrc/u8x8_d_uc1638.c"
"U8G2/csrc/u8x8_d_uc1701_dogs102.c"
"U8G2/csrc/u8x8_d_uc1701_mini12864.c"
"U8G2/csrc/u8x8_fonts.c"
"U8G2/csrc/u8x8_gpio.c"
"U8G2/csrc/u8x8_input_value.c"
"U8G2/csrc/u8x8_message.c"
"U8G2/csrc/u8x8_selection_list.c"
"U8G2/csrc/u8x8_setup.c"
"U8G2/csrc/u8x8_string.c"
"U8G2/csrc/u8x8_u16toa.c"
"U8G2/csrc/u8x8_u8toa.c"
)

# -----------------------------------------

# Turn on all optional warnings (-Wall)
set(USER_COMPILE_WARNINGS_ALL "-Wall")

# Enable extra warning flags (-Wextra)
set(USER_COMPILE_WARNINGS_EXTRA "-Wextra")

# Make all warnings into hard errors (-Werror)
set(USER_COMPILE_WARNINGS_AS_ERRORS "")

# Check the code for syntax errors, but don't do anything beyond that (-fsyntax-only)
set(USER_COMPILE_WARNINGS_CHECK_SYNTAX_ONLY "")

# Issue all the mandatory diagnostics listed in the C standard (-pedantic)
set(USER_COMPILE_WARNINGS_PEDANTIC "")

# Issue all the mandatory diagnostics, and make all mandatory diagnostics into errors. (-pedantic-errors)
set(USER_COMPILE_WARNINGS_PEDANTIC_AS_ERRORS "")

# Suppress all warnings (-w)
set(USER_COMPILE_WARNINGS_INHIBIT_ALL "")

# -----------------------------------------

# Optimization level   "-O0" [None], "-O1" [Optimize] , "-O2" [Optimize More], "-O3" [Optimize Most] or "-Os" [Optimize Size]
set(USER_COMPILE_OPTIMIZATION_LEVEL "-O0")

# Other flags related to optimization
set(USER_COMPILE_OPTIMIZATION_OTHER_FLAGS "")

# -----------------------------------------

# Debug level "" [None], "-g1" [Minimum], "g2" [Default], "g3" [Maximum]
set(USER_COMPILE_DEBUG_LEVEL "-g3")

# Other flags related to debugging
set(USER_COMPILE_DEBUG_OTHER_FLAGS "")

# -----------------------------------------

# Enable profiling (-pg) (This feature is not supported currently)
# set(USER_COMPILE_PROFILING_ENABLE )

# -----------------------------------------

# Verbose (-v)
set(USER_COMPILE_VERBOSE "")

# Support ANSI_PROGRAM (-ansi)
set(USER_COMPILE_ANSI "")

# Add any compiler options that are not covered by the above variables, they will be added as extra compiler options
# To enable profiling -pg [ for gprof ]  or -p [ for prof information ]
set(USER_COMPILE_OTHER_FLAGS "")

# -----------------------------------------

# Linker options
# Do not use the standard system startup files when linking.
# The standard system libraries are used normally, unless -nostdlib or -nodefaultlibs is used. (-nostartfiles)
set(USER_LINK_NO_START_FILES "")

# Do not use the standard system libraries when linking. (-nodefaultlibs)
set(USER_LINK_NO_DEFAULT_LIBS "")

# Do not use the standard system startup files or libraries when linking. (-nostdlib)
set(USER_LINK_NO_STDLIB "")

# Omit all symbol information. (-s)
set(USER_LINK_OMIT_ALL_SYMBOL_INFO "")


# -----------------------------------------

# Add any libraries to be linked below, they will be added as extra libraries.
# User needs to update USER_LINK_DIRECTORIES below with these library search paths.
set(USER_LINK_LIBRARIES
)

# Add any directories to look for the libraries to be linked.
# Example 1: Adding /proj/compression/lib will pass -L/proj/compression/lib to the linker.
# Example 2: Adding ../../common/lib will consider the path as relative to this directory and will pass the path to -L option.
set(USER_LINK_DIRECTORIES
)

# -----------------------------------------

set(USER_LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/lscript.ld")

# Add linker options to be passed, they will be added as extra linker options
# Example : Adding -s will pass -s to the linker.
set(USER_LINK_OTHER_FLAGS
)

# -----------------------------------------

###   END OF USER SETTINGS SECTION ###
###   DO NOT EDIT BEYOND THIS LINE ###

set(USER_COMPILE_OPTIONS
    " ${USER_COMPILE_WARNINGS_ALL}"
    " ${USER_COMPILE_WARNINGS_EXTRA}"
    " ${USER_COMPILE_WARNINGS_AS_ERRORS}"
    " ${USER_COMPILE_WARNINGS_CHECK_SYNTAX_ONLY}"
    " ${USER_COMPILE_WARNINGS_PEDANTIC}"
    " ${USER_COMPILE_WARNINGS_PEDANTIC_AS_ERRORS}"
    " ${USER_COMPILE_WARNINGS_INHIBIT_ALL}"
    " ${USER_COMPILE_OPTIMIZATION_LEVEL}"
    " ${USER_COMPILE_OPTIMIZATION_OTHER_FLAGS}"
    " ${USER_COMPILE_DEBUG_LEVEL}"
    " ${USER_COMPILE_DEBUG_OTHER_FLAGS}"
    " ${USER_COMPILE_VERBOSE}"
    " ${USER_COMPILE_ANSI}"
    " ${USER_COMPILE_OTHER_FLAGS}"
)
foreach(entry ${USER_UNDEFINED_SYMBOLS})
    list(APPEND USER_COMPILE_OPTIONS " -U${entry}")
endforeach()

set(USER_LINK_OPTIONS
    " ${USER_LINKER_NO_START_FILES}"
    " ${USER_LINKER_NO_DEFAULT_LIBS}"
    " ${USER_LINKER_NO_STDLIB}"
    " ${USER_LINKER_OMIT_ALL_SYMBOL_INFO}"
    " ${USER_LINK_OTHER_FLAGS}"
)
