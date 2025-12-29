#include "xtmrctr.h"

XTmrCtr_Config XTmrCtr_ConfigTable[] __attribute__ ((section (".drvcfg_sec"))) = {

	{
		"xlnx,axi-timer-2.0", /* compatible */
		0x41c00000, /* reg */
		0x5f5e100, /* clock-frequency */
		0x2003, /* interrupts */
		0x41200001 /* interrupt-parent */
	},
	{
		"xlnx,axi-timer-2.0", /* compatible */
		0x41c10000, /* reg */
		0x5f5e100, /* clock-frequency */
		0xffff, /* interrupts */
		0xffff /* interrupt-parent */
	},
	 {
		 NULL
	}
};