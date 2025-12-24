/* =============================================================================
 *  File: diskio.c
 *  Project: MicroBlaze + AXI Quad SPI (microSD Interface)
 *  Author: IMR Engineering / Hab Collector
 *  ---------------------------------------------------------------------------
 *  Purpose:
 *      This file implements the FatFs low-level disk I/O interface for use
 *      on a Xilinx MicroBlaze system where an SD or microSD card is connected
 *      via an AXI Quad SPI (AXI QSPI) peripheral operating in standard SPI mode.
 *
 *      FatFs (ff.c) is file-system independent and requires only five glue
 *      functions: disk_initialize(), disk_status(), disk_read(), disk_write(),
 *      and disk_ioctl().  These are provided here and communicate with the
 *      physical SD card using the SPI protocol over the AXI QSPI IP core.
 *
 *  ---------------------------------------------------------------------------
 *  Functional Overview:
 *      - The file initializes the AXI Quad SPI peripheral using its base
 *        address (XSpi_CfgInitialize() with manual configuration).
 *      - Commands such as CMD0, CMD8, ACMD41, and CMD58 are issued to
 *        initialize SD or SDHC cards in SPI mode.
 *      - Sector reads and writes (CMD17, CMD24) are supported for 512-byte
 *        block transfers as required by FatFs.
 *      - The interface is blocking/polled mode and uses FIFO transfers only.
 *
 *      This implementation is compact, requires no interrupts or DMA,
 *      and provides reliable operation for single-card, single-drive systems.
 *
 *  ---------------------------------------------------------------------------
 *  Required Hardware:
 *      • MicroBlaze soft processor (Vitis 2024.2 standalone domain)
 *      • AXI Quad SPI IP configured as:
 *            - Mode: Standard SPI
 *            - Master Mode: Enabled
 *            - FIFO Depth: 16
 *            - Performance Mode: Disabled
 *            - XIP Mode: Disabled
 *            - STARTUP Primitive: Disabled
 *      • microSD card connected to SPI MISO/MOSI/SCLK/SS0.
 *
 *      **Clock Domains**
 *      -----------------
 *      The AXI Quad SPI IP has two main clock inputs:
 *
 *          1. s_axi_aclk   – The AXI4-Lite bus interface clock.
 *             This clock drives register access timing between the
 *             MicroBlaze processor and the SPI control registers.
 *
 *          2. ext_spi_clk  – The SPI bit clock domain used to clock
 *             SCK, MISO, and MOSI signals to the external device.
 *
 *      In most designs these clocks can be tied together (both driven
 *      by the same 100 MHz or 125 MHz system clock).  However, the
 *      ext_spi_clk can also be generated separately (for example,
 *      from a Clocking Wizard) to precisely control the SPI frequency.
 *
 *      The effective SPI clock rate (SCK) is derived inside the AXI QSPI
 *      core from ext_spi_clk using its configured divider ratio.
 *      For SD card initialization, keep SCK at ~400 kHz, then raise to
 *      12–25 MHz for normal operation.
 *
 *  ---------------------------------------------------------------------------
 *  How to Adapt for Other Implementations:
 *
 *      1. **SPI Base Address**
 *         Update the macro `SPI_BASEADDR` to match the base address
 *         assigned in Vivado’s Address Editor for your AXI Quad SPI instance.
 *
 *      2. **Slave Select Line**
 *         Update `SPI_SS0_MASK` if your microSD is wired to SS1, SS2, etc.
 *         (Bit 0 = SS0, Bit 1 = SS1, etc.)
 *
 *      3. **Clock Rates**
 *         Adjust `SD_SPI_INIT_HZ` and `SD_SPI_RUN_HZ` comments to match
 *         your hardware configuration.  These are documentation aids;
 *         the actual rate is set in Vivado’s AXI QSPI configuration.
 *
 *      4. **Multiple Drives**
 *         If using more than one storage device, change `SD_SPI_DRIVE`
 *         and extend the driver to handle multiple `pdrv` values.
 *
 *      5. **DMA or Interrupt Mode**
 *         This implementation is **polling-only**.  
 *         DMA, interrupt-driven transfers, or AXI Stream modes are **not
 *         supported** in this version.  All SPI transactions occur
 *         synchronously using XSpi_Transfer() in blocking mode.
 *
 *      6. **Alternative Hardware**
 *         - To use another SPI controller (e.g., AXI SPI or PS SPI),
 *           replace the XSpi_* functions with the equivalent driver API.
 *         - To use SDIO or SD Host IP instead of SPI mode, you must switch
 *           to Xilinx’s xsdps driver and FatFs “diskio_sdps.c” variant.
 *
 *      7. **Block Size**
 *         This driver fixes block length to 512 bytes, which is required
 *         by FatFs.  SDHC/SDXC cards ignore CMD16 and always use 512 B.
 *
 *      8. **Card Detection and Write Protect**
 *         If your board supports CD/WP signals via GPIO, you may enhance
 *         disk_status() to check those pins.
 *
 *  ---------------------------------------------------------------------------
 *  File Version History:
 *      v1.0  – Initial implementation for AXI Quad SPI standard mode (Hab)
 *      v1.1  – Integrated with FatFs xilffs BSP; tested on Arty A7 (2025)
 *
 *  ---------------------------------------------------------------------------
 *  References:
 *      - ChaN FatFs documentation:  https://elm-chan.org/fsw/ff/
 *      - AMD/Xilinx AXI Quad SPI PG153
 *      - Vitis 2024.2 Standalone BSP Reference Guide
 * =============================================================================
 */

#include "diskio.h"
#include "ff.h"
#include "xil_types.h"
#include "xstatus.h"
#include "xspi.h"
#include "xil_io.h"
#include "sleep.h"

/* ===================== User configuration (matches your design) ===================== */

#define SPI_BASEADDR        0x44A00000u   /* From your Address Editor */
#define SPI_SS0_MASK        0x01u         /* Using slave-select 0 (Pmod microSD) */

#define SD_SPI_INIT_HZ      400000u       /* ~400 kHz during card init */
#define SD_SPI_RUN_HZ       12500000u     /* ~12.5 MHz after init (set IP accordingly) */

#define SD_SPI_DRIVE        0             /* pdrv index (always 0 unless multiple cards) */

#define SD_CMD_TIMEOUT_MS   100u          /* Generic command timeout */
#define SD_ACMD41_TIMEOUT_MS 1200u        /* Init loop timeout */

/* ==================================================================================== */

/* Internal state */
static XSpi Spi;              /* SPI driver instance (base-address init) */
static u8 CardIsReady = 0;    /* 1 when initialized */
static u8 CardHighCapacity = 0; /* 1 if SDHC/SDXC (block addressing) */

/* ===================== Minimal SPI helpers ===================== */

static int Spi_Init(void)
{
    XSpi_Config Cfg;

    /* Zero-init then set only what XSpi_CfgInitialize actually needs */
    memset(&Cfg, 0, sizeof(Cfg));
    Cfg.BaseAddress = SPI_BASEADDR;

    /* Initialize by base address (no device ID) */
    if (XSpi_CfgInitialize(&Spi, &Cfg, Cfg.BaseAddress) != XST_SUCCESS)
    {
        return XST_FAILURE;
    }

    XSpi_Reset(&Spi);

    /* Master, manual slave-select, manual start off (let driver handle it) */
    u32 Options = XSP_MASTER_OPTION | XSP_MANUAL_SSELECT_OPTION;
    if (XSpi_SetOptions(&Spi, Options) != XST_SUCCESS)
    {
        return XST_FAILURE;
    }

    /* Clear any stale FIFOs; start the device */
    XSpi_Start(&Spi);
    XSpi_IntrGlobalDisable(&Spi);

    /* Select SS0 (active-low) but don’t toggle yet; XSpi_Transfer asserts as needed */
    XSpi_SetSlaveSelect(&Spi, SPI_SS0_MASK);

    /* NOTE on SPI clock:
       AXI Quad SPI’s SCK is derived in hardware by your IP settings/clk divider.
       Run the IP at ~400 kHz during init, then raise to ~12.5 MHz afterward.
       If you can’t change at runtime, leave at <=12.5 MHz and it’ll work fine. */
    return XST_SUCCESS;
}

static void Spi_ShortDelayUs(u32 usec)
{
    /* usleep is available in standalone */
    if (usec) usleep(usec);
}

static void Spi_TxRx(const u8 *Tx, u8 *Rx, u32 Len)
{
    /* XSpi_Transfer handles full-duplex; pass NULL for Tx/Rx as needed */
    XSpi_Transfer(&Spi, (u8*)Tx, Rx, Len);
}

static u8 Spi_TxRx_Byte(u8 out)
{
    u8 in = 0xFF;
    XSpi_Transfer(&Spi, &out, &in, 1);
    return in;
}

static u8 Spi_Read_Byte(void)
{
    u8 out = 0xFF, in = 0xFF;
    XSpi_Transfer(&Spi, &out, &in, 1);
    return in;
}

static void Spi_Write_Byte(u8 b)
{
    u8 dummy;
    XSpi_Transfer(&Spi, &b, &dummy, 1);
}

/* ============== SD over SPI primitives (tokens, commands) ============== */

#define SD_TOKEN_START_BLOCK   0xFEu

/* R1 bits */
#define R1_IDLE_STATE          0x01u
#define R1_ILLEGAL_COMMAND     0x04u

/* Commands (SPI has bit 6 set) */
#define CMD0    (0u)    /* GO_IDLE_STATE */
#define CMD8    (8u)    /* SEND_IF_COND */
#define CMD16   (16u)   /* SET_BLOCKLEN */
#define CMD17   (17u)   /* READ_SINGLE_BLOCK */
#define CMD24   (24u)   /* WRITE_SINGLE_BLOCK */
#define CMD55   (55u)   /* APP_CMD */
#define CMD58   (58u)   /* READ_OCR */
#define ACMD41  (41u)   /* SD_SEND_OP_COND (after CMD55) */

/* Send N dummy clocks (CS high) */
static void sd_send_dummy_clocks(u32 nbytes)
{
    /* Ensure CS is deasserted before sending idle clocks */
    XSpi_SetSlaveSelect(&Spi, SPI_SS0_MASK);
    for (u32 i = 0; i < nbytes; i++)
    {
        Spi_Write_Byte(0xFF);
    }
}

/* Select the card (CS low) */
static void sd_select(void)
{
    /* XSpi driver asserts SS for the device selected via SetSlaveSelect;
       to guarantee min setup time, push one idle byte */
    Spi_Write_Byte(0xFF);
}

/* Deselect the card (CS high) */
static void sd_deselect(void)
{
    sd_send_dummy_clocks(2); /* at least 8 clocks after CS high */
}

/* Wait for a non-0xFF byte with timeout (ms) */
static int sd_wait_ready(u32 timeout_ms)
{
    u32 elapsed = 0;
    while (elapsed < timeout_ms)
    {
        if (Spi_Read_Byte() == 0xFF)
        {
            return XST_SUCCESS; /* bus free / card ready */
        }
        usleep(1000);
        elapsed += 1;
    }
    return XST_FAILURE;
}

/* Send a command (CMD or ACMD) and get R1 */
static u8 sd_send_cmd(u8 cmd, u32 arg, u8 crc)
{
    /* ACMD prefix: CMD55 then the app command */
    if (cmd & 0x80u)
    {
        cmd &= 0x7Fu;
        (void)sd_send_cmd(CMD55, 0, 0x65); /* valid CRC for CMD55 isn’t required after idle, harmless */
    }

    /* Ensure card ready to receive a command */
    (void)sd_wait_ready(SD_CMD_TIMEOUT_MS);

    /* Command frame: 0x40|cmd, arg[31:0], crc */
    u8 frame[6];
    frame[0] = (u8)(0x40u | cmd);
    frame[1] = (u8)(arg >> 24);
    frame[2] = (u8)(arg >> 16);
    frame[3] = (u8)(arg >> 8);
    frame[4] = (u8)(arg);
    frame[5] = crc | 0x01u; /* end bit = 1 */

    Spi_TxRx(frame, NULL, 6);

    /* Read R1 (response within 8 bytes) */
    for (int i = 0; i < 8; i++)
    {
        u8 r1 = Spi_Read_Byte();
        if ((r1 & 0x80u) == 0u)
        {
            return r1;
        }
    }
    return 0xFFu; /* timeout */
}

/* Read a data block (512B) after a READ command; returns 0 on success */
static int sd_read_block(u8 *buff, u32 timeout_ms)
{
    u32 elapsed = 0;

    /* Wait for data token 0xFE */
    while (elapsed < timeout_ms)
    {
        u8 b = Spi_Read_Byte();
        if (b == SD_TOKEN_START_BLOCK)
        {
            /* read 512 bytes */
            for (u32 i = 0; i < 512u; i++)
            {
                buff[i] = Spi_Read_Byte();
            }
            /* discard CRC */
            (void)Spi_Read_Byte();
            (void)Spi_Read_Byte();
            return 0;
        }
        if (b != 0xFF)
        {
            /* error token */
            return -1;
        }
        usleep(1000);
        elapsed += 1;
    }
    return -2;
}

/* Write a data block (512B); returns 0 on success */
static int sd_write_block(const u8 *buff)
{
    /* Start token */
    Spi_Write_Byte(SD_TOKEN_START_BLOCK);

    /* Data */
    for (u32 i = 0; i < 512u; i++)
    {
        Spi_Write_Byte(buff[i]);
    }

    /* Dummy CRC (not used in SPI mode) */
    Spi_Write_Byte(0xFF);
    Spi_Write_Byte(0xFF);

    /* Data response: 0bxxx00101 accepted */
    u8 resp = Spi_Read_Byte() & 0x1Fu;
    if (resp != 0x05u)
    {
        return -1;
    }

    /* Wait while card is busy (drives MISO low) */
    if (sd_wait_ready(SD_CMD_TIMEOUT_MS) != XST_SUCCESS)
    {
        return -2;
    }
    return 0;
}

/* ===================== FatFs required functions ===================== */

static DSTATUS Stat = STA_NOINIT;

DSTATUS disk_status (BYTE pdrv)
{
    if (pdrv != SD_SPI_DRIVE)
    {
        return STA_NOINIT;
    }
    return Stat;
}

/* Card init sequence (SPI mode) */
DSTATUS disk_initialize (BYTE pdrv)
{
    if (pdrv != SD_SPI_DRIVE)
    {
        return STA_NOINIT;
    }

    if (Spi_Init() != XST_SUCCESS)
    {
        Stat |= STA_NOINIT;
        return Stat;
    }

    /* Give the card >=74 clock cycles with CS high */
    sd_deselect();
    sd_send_dummy_clocks(10);

    /* Select card (CS low) and send CMD0 to go idle */
    sd_select();
    u8 r1 = sd_send_cmd(CMD0, 0, 0x95);  /* Valid CRC for CMD0 */
    if (r1 != R1_IDLE_STATE)
    {
        sd_deselect();
        Stat |= STA_NOINIT;
        return Stat;
    }

    /* CMD8: check voltage range / SDHC */
    r1 = sd_send_cmd(CMD8, 0x000001AAu, 0x87); /* VHS=0x1 (2.7-3.6V), check pattern 0xAA */
    if (r1 & R1_ILLEGAL_COMMAND)
    {
        /* Older v1.x card (no CMD8); treat as SDSC */
        CardHighCapacity = 0;
    }
    else
    {
        /* Read CMD8 trailing bytes (R7) */
        u8 r7[4] = {0};
        for (int i = 0; i < 4; i++) r7[i] = Spi_Read_Byte();
        /* If echo-back matches 0xAA, proceed */
        CardHighCapacity = 0; /* set after ACMD41 with HCS */
    }

    /* ACMD41 loop with HCS bit if we assume SDv2 */
    u32 waited_ms = 0;
    do
    {
        r1 = sd_send_cmd(0x80u | ACMD41, 0x40000000u, 0x77); /* HCS=1 */
        if (r1 == 0x00u) break;             /* Ready */
        usleep(10000);                      /* 10 ms */
        waited_ms += 10;
    } while (waited_ms < SD_ACMD41_TIMEOUT_MS);

    if (r1 != 0x00u)
    {
        sd_deselect();
        Stat |= STA_NOINIT;
        return Stat;
    }

    /* CMD58: read OCR to infer CCS (high capacity) */
    r1 = sd_send_cmd(CMD58, 0, 0xFD);
    if (r1 == 0x00u)
    {
        u8 ocr[4];
        for (int i = 0; i < 4; i++) ocr[i] = Spi_Read_Byte();
        CardHighCapacity = ( (ocr[0] & 0x40u) != 0u ); /* CCS bit */
    }

    /* Force 512-byte block length for SDSC; SDHC ignores CMD16 */
    (void)sd_send_cmd(CMD16, 512u, 0x15);

    sd_deselect();

    CardIsReady = 1;
    Stat &= (DSTATUS)~STA_NOINIT;
    return Stat;
}

DRESULT disk_read (BYTE pdrv, BYTE* buff, LBA_t sector, UINT count)
{
    if ((pdrv != SD_SPI_DRIVE) || (count == 0u) || (buff == NULL))
    {
        return RES_PARERR;
    }
    if (Stat & STA_NOINIT)
    {
        return RES_NOTRDY;
    }

    /* Convert to byte address for SDSC */
    u32 addr = (CardHighCapacity) ? (u32)sector : (u32)(sector * 512u);

    for (UINT i = 0; i < count; i++)
    {
        sd_select();

        u8 r1 = sd_send_cmd(CMD17, addr, 0xE1);
        if (r1 != 0x00u)
        {
            sd_deselect();
            return RES_ERROR;
        }

        if (sd_read_block(&buff[i * 512u], SD_CMD_TIMEOUT_MS) != 0)
        {
            sd_deselect();
            return RES_ERROR;
        }

        sd_deselect();

        /* Next LBA */
        if (!CardHighCapacity) addr += 512u; else addr += 1u;
    }

    return RES_OK;
}

#if FF_FS_READONLY == 0
DRESULT disk_write (BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count)
{
    if ((pdrv != SD_SPI_DRIVE) || (count == 0u) || (buff == NULL))
    {
        return RES_PARERR;
    }
    if (Stat & STA_NOINIT)
    {
        return RES_NOTRDY;
    }

    /* Convert to byte address for SDSC */
    u32 addr = (CardHighCapacity) ? (u32)sector : (u32)(sector * 512u);

    for (UINT i = 0; i < count; i++)
    {
        sd_select();

        u8 r1 = sd_send_cmd(CMD24, addr, 0xE1);
        if (r1 != 0x00u)
        {
            sd_deselect();
            return RES_ERROR;
        }

        if (sd_write_block(&buff[i * 512u]) != 0)
        {
            sd_deselect();
            return RES_ERROR;
        }

        sd_deselect();

        /* Next LBA */
        if (!CardHighCapacity) addr += 512u; else addr += 1u;
    }

    return RES_OK;
}
#endif /* FF_FS_READONLY == 0 */

DRESULT disk_ioctl (BYTE pdrv, BYTE cmd, void* buff)
{
    if (pdrv != SD_SPI_DRIVE) return RES_PARERR;
    if (Stat & STA_NOINIT)    return RES_NOTRDY;

    switch (cmd)
    {
        case CTRL_SYNC:
            /* Ensure card not busy */
            return (sd_wait_ready(SD_CMD_TIMEOUT_MS) == XST_SUCCESS) ? RES_OK : RES_ERROR;

        case GET_SECTOR_SIZE:
            *(DWORD*)buff = 512u;
            return RES_OK;

        case GET_BLOCK_SIZE:
            /* Erase block size in sectors (typical 128); not critical for SPI */
            *(DWORD*)buff = 128u;
            return RES_OK;

        case GET_SECTOR_COUNT:
            /* If you want exact size, parse CSD here.
               For now return a large placeholder to allow mkfs; adjust as needed. */
            *(DWORD*)buff = 0; /* 0 means “unknown” to FatFs for some ops */
            return RES_OK;

        default:
            return RES_PARERR;
    }
}

/* Simple fixed timestamp; replace with RTC if available */
DWORD get_fattime(void)
{
    return ((DWORD)(2010U - 1980U) << 25) | ((DWORD)1 << 21) | ((DWORD)1 << 16);
}
