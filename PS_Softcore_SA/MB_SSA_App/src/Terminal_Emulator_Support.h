/** ****************************************************************************************************
 * @file 			TerminalEmulatorSupport.h
 * @brief			This is the Header file used to support TerminalEmulatorSupport.c
 * ****************************************************************************************************
 * @author original	Hab Collector (habco) \n
 *
 * @version       	See Main_Support.h: FIRMWARE_REV_MAJOR, FIRMWARE_REV_MINOR
 *
 * @param Development_Environment \n
 * Hardware:		STM32L476 \n
 * IDE:             STMCubeIDE VER 1.9.0  \n
 * Compiler:        GCC \n
 * Editor Settings: 1 Tab = 4 Spaces, Recommended Courier New 11
 *
 * @note            See source file for notes
 *
 * 					This is an embedded application
 *		            It will be necessary to consult the reference documents to fully understand the code
 *                  It is suggested that the documents be reviewed in the order shown.
 *			          Schematic 002-5671-00
 *                    Pittsburgh_Bootloader
 *				      Design Document
 *
 * @copyright		Applied Concepts, Inc
 ****************************************************************************************************** */

#ifndef INC_TERMINALEMULATORSUPPORT_H_
#define INC_TERMINALEMULATORSUPPORT_H_
#ifdef __cplusplus
extern"C" {
#endif

#include <stdint.h>

// DEFINES
// TEXT FORECOLORS
#define MAX_PRINT_MESSAGE_SIZE			256U
#define ESCAPE_CHARACTER				27U
#define ERASE_DISPLAY					"\033[2J\033[H"
#define CURSOR_BACK_SPACE				"\b"
#define DEFAULT_FOREGROUND_COLOR		"\033[0;39m"
#define SET_RED_FOREGROUND				"\033[0;31m"
#define SET_BRIGHT_RED_FOREGROUND		"\033[1;31m"
#define SET_GREEN_FOREGROUND			"\033[0;32m"
#define SET_BRIGHT_GREEN_FOREGROUND		"\033[1;32m"
#define SET_YELLOW_FOREGROUND			"\033[0;33m"
#define SET_BRIGHT_YELLOW_FOREGROUND	"\033[1;33m"
#define SET_BLUE_FOREGROUND				"\033[0;34m"
#define SET_BRIGHT_BLUE_FOREGROUND		"\033[1;34m"
#define SET_MAGENTA_FOREGROUND          "\033[0;95m"
#define SET_BRIGHT_MAGENTA_FOREGROUND   "\033[1;95m"
// BACKGROUND COLORS
#define SET_GREEN_BACKGROUND			"\033[0;42m"
#define SET_BRIGHT_GREEN_BACKGROUND		"\033[1;42m"
// RESET ALL COLORS TO DEFAULT
#define RESET_COLORS_TO_DEFAULT			"\033[39;49m"
#define USE_EXTENDED_TERMINAL_FUNCTIONS


// TYPEDEFS AND ENUMS
typedef enum
{
	COLOR_DEFAULT = 999,
	COLOR_RED = 9,
	COLOR_BRIGHT_RED = 196,
	COLOR_GREEN = 2,
	COLOR_BRIGHT_GREEN = 10,
	COLOR_YELLOW = 11,
	COLOR_BRIGHT_YELLOW = 228,
	COLOR_BLUE = 12,
	COLOR_BRIGHT_BLUE = 21,
	COLOR_WHITE = 15,
	COLOR_BLACK = 0,
	COLOR_DARK_GRAY = 8,
	COLOR_LIGHT_GRAY = 7,
	COLOR_PURPLE = 5,
	COLOR_MAGENTA = 13
}Type_PrintColor;


// FUNCTION PROTOTYPES
// Terminal Control
void printColor(Type_PrintColor ForeGroundColor, Type_PrintColor BackGroundColor, const char *String, ... );
void terminal_ClearScreen(void);
void terminal_CursorBackSpace(void);
void terminal_SetDefaultColorPalette(void);
#ifdef USE_EXTENDED_TERMINAL_FUNCTIONS
// Foreground Colors
void terminal_SetDefaultForegroundColor(void);
void terminal_SetDefaultForegroundColor(void);
void terminal_SetRedForeground(void);
void terminal_SetBrightRedForeground(void);
void terminal_SetGreenForeground(void);
void terminal_SetBrightGreenForeground(void);
void terminal_SetYellowForeground(void);
void terminal_SetBrightYellowForeground(void);
void terminal_SetBlueForeground(void);
void terminal_SetBrightBlueForeground(void);
void terminal_SetMagentaForeground(void);
void terminal_SetBrightMagentaForeground(void);
// Background Colors: Color results may vary consider printColor()
void terminal_SetGreenBrackground(void);
void terminal_SetGreenBrackground(void);
// Print Direct Color: Color results may vary consider printColor()
void printRed(const char *String);
void printBrightRed(const char *String);
void printGreen(const char *String);
void printBrightGreen(const char *String);
void printYellow(const char *String);
void printBrightYellow(const char *String);
void printBlue(const char *String);
void printBrightBlue(const char *String);
void printMagenta(const char *String);
void printBrightMagenta(const char *String);
#endif

#ifdef __cplusplus
}
#endif
#endif /* INC_TERMINALEMULATORSUPPORT_H_ */
