#include "diskio.h"
#include "ff.h"

#include "../drivers/sd/sd_spi.h"

DSTATUS disk_initialize(BYTE pdrv)
{
    if (pdrv != 0) {
        return STA_NOINIT;
    }

    if (SD_Init() != HAL_OK) {
        return STA_NOINIT;
    }

    return 0;
}

DSTATUS disk_status(BYTE pdrv)
{
    if (pdrv != 0) {
        return STA_NOINIT;
    }

    if (SD_IsReady() == 0U) {
        return STA_NOINIT;
    }

    return 0;
}

DRESULT disk_read(
    BYTE pdrv,
    BYTE *buff,
    DWORD sector,
    UINT count)
{
    if (pdrv != 0 || buff == NULL || count == 0U) {
        return RES_PARERR;
    }

    for (UINT i = 0; i < count; i++) {
        if (SD_ReadBlock(
                buff + (i * 512U),
                (uint32_t)sector + i) != HAL_OK) {
            return RES_ERROR;
        }
    }

    return RES_OK;
}

#if _FS_READONLY == 0

DRESULT disk_write(
    BYTE pdrv,
    const BYTE *buff,
    DWORD sector,
    UINT count)
{
    if (pdrv != 0 || buff == NULL || count == 0U) {
        return RES_PARERR;
    }

    for (UINT i = 0; i < count; i++) {
        if (SD_WriteBlock(
                buff + (i * 512U),
                (uint32_t)sector + i) != HAL_OK) {
            return RES_ERROR;
        }
    }

    return RES_OK;
}

#endif

DRESULT disk_ioctl(
    BYTE pdrv,
    BYTE cmd,
    void *buff)
{
    if (pdrv != 0) {
        return RES_PARERR;
    }

    switch (cmd) {

    case CTRL_SYNC:
        return RES_OK;

    case GET_SECTOR_COUNT:
        if (buff == NULL) {
            return RES_PARERR;
        }

        *(DWORD *)buff = (DWORD)SD_GetSectorCount();
        return RES_OK;

    case GET_SECTOR_SIZE:
        if (buff == NULL) {
            return RES_PARERR;
        }

        *(WORD *)buff = 512U;
        return RES_OK;

    case GET_BLOCK_SIZE:
        if (buff == NULL) {
            return RES_PARERR;
        }

        *(DWORD *)buff = 1U;
        return RES_OK;

    default:
        return RES_PARERR;
    }
}