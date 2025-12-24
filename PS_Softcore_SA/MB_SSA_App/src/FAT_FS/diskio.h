/* =============================================================================
 *  File: diskio.h
 *  Project: MicroBlaze + AXI Quad SPI (microSD Interface)
 *  Author: IMR Engineering / Hab Collector
 *  ---------------------------------------------------------------------------
 *  Purpose:
 *      This header defines the low-level disk I/O interface expected by FatFs.
 *      It declares the data types, control codes, and function prototypes
 *      required by the FatFs core (ff.c) to communicate with a physical
 *      storage device through user-supplied drivers such as diskio.c.
 *
 *      In this project, diskio.h pairs with diskio.c, which implements
 *      sector-level access to an SD/microSD card over the AXI Quad SPI IP
 *      in standard SPI mode for a MicroBlaze system running Vitis 2024.2.
 *
 *  ---------------------------------------------------------------------------
 *  Functional Overview:
 *      FatFs uses a hardware-agnostic interface layer.  The functions below
 *      form that interface and are called internally by the file-system core.
 *
 *          DSTATUS disk_initialize (BYTE pdrv);
 *          DSTATUS disk_status     (BYTE pdrv);
 *          DRESULT disk_read       (BYTE pdrv, BYTE* buff, LBA_t sector, UINT count);
 *          DRESULT disk_write      (BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count);
 *          DRESULT disk_ioctl      (BYTE pdrv, BYTE cmd, void* buff);
 *
 *      Each of these is implemented in diskio.c and linked into the
 *      application together with FatFs (xilffs) to provide complete
 *      file-system support for the SPI-based SD interface.
 *
 *  ---------------------------------------------------------------------------
 *  Adapting to Other Hardware:
 *      • The API definitions in this header should not be changed.
 *      • Only the corresponding diskio.c needs modification when porting
 *        to a different SPI controller, SD interface, or hardware platform.
 *      • For non-SPI implementations (e.g., SDIO or SD Host), use the
 *        Xilinx-provided “diskio_sdps.c” instead of this pair.
 *
 *  ---------------------------------------------------------------------------
 *  Notes:
 *      - This implementation supports a single drive (pdrv = 0).
 *      - All operations are blocking/polled.  DMA and interrupt modes
 *        are not used in this project.
 *      - get_fattime() provides a fixed timestamp unless a real-time
 *        clock is integrated and used.
 *
 *  ---------------------------------------------------------------------------
 *  References:
 *      - ChaN FatFs API Reference:  https://elm-chan.org/fsw/ff/
 *      - AMD/Xilinx xilffs BSP (Vitis 2024.2)
 *      - diskio.c (corresponding implementation file)
 * =============================================================================
 */

#ifndef _DISKIO_DEFINED
#define _DISKIO_DEFINED

#ifdef __cplusplus
extern "C" {
#endif

#include "ff.h"
#include "xil_types.h"

/* Status of Disk Functions */
typedef BYTE DSTATUS;

/* Results of Disk Functions */
typedef enum
{
    RES_OK = 0,     /* Successful */
    RES_ERROR,      /* R/W Error */
    RES_WRPRT,      /* Write Protected */
    RES_NOTRDY,     /* Not Ready */
    RES_PARERR      /* Invalid Parameter */
} DRESULT;

/* Disk Status Bits (DSTATUS) */
#define STA_NOINIT  0x01U   /* Drive not initialized */
#define STA_NODISK  0x02U   /* No medium in the drive */
#define STA_PROTECT 0x04U   /* Write protected */

/* diskio API required by FatFs */
DSTATUS disk_initialize (BYTE pdrv);
DSTATUS disk_status     (BYTE pdrv);
DRESULT disk_read       (BYTE pdrv, BYTE* buff, LBA_t sector, UINT count);
DRESULT disk_write      (BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count);
DRESULT disk_ioctl      (BYTE pdrv, BYTE cmd, void* buff);

/* Ioctl codes (FatFs uses a subset) */
#define CTRL_SYNC           0U
#define GET_SECTOR_COUNT    1U
#define GET_SECTOR_SIZE     2U
#define GET_BLOCK_SIZE      3U
#define CTRL_TRIM           4U

/* Optional timestamp provider (FatFs calls get_fattime) */
DWORD get_fattime(void);

#ifdef __cplusplus
}
#endif
#endif /* _DISKIO_DEFINED */
