/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
* All rights reserved.
*
* This software is supplied "AS IS" without any warranties.
* RDA assumes no responsibility or liability for the use of the software,
* conveys no license or title under any patent, copyright, or mask work
* right to the product. RDA reserves the right to make changes in the
* software without notification.  RDA also make no representation or
* warranty that such application will be suitable for the specified use
* without further testing or modification.
*/

//#define _INC_TCHAR
//#define _INC_INT_DEFINES

#include "block_device.h"
#include "flash_block_device.h"

#include "ffconf.h"
#include "diskio.h"

static blockDevice_t *s_block_device[FF_VOLUMES] = {NULL};

BYTE fat_disk_set_device(blockDevice_t *dev)
{
    int i;
    for (i = 0; i < FF_VOLUMES; i++)
    {
        if (s_block_device[i] == NULL)
        {
            s_block_device[i] = dev;
            return i;
        }
    }
    return 0xFF;
}

bool fat_disk_remove_device(BYTE idx)
{
    if (s_block_device[idx] != NULL)
    {
        s_block_device[idx] = NULL;
        return true;
    }
    return false;
}

DSTATUS fat_disk_status(BYTE index)
{
    if (index >= FF_VOLUMES)
    {
        return STA_NODISK;
    }
    else if (s_block_device[index] == NULL)
    {
        return STA_NOINIT;
    }
    return 0;
}

DSTATUS fat_disk_initialize(BYTE index)
{
    if (index >= FF_VOLUMES || s_block_device[index] == NULL)
    {
        return STA_NODISK;
    }
    return 0;
}

DRESULT fat_disk_read(BYTE index, BYTE *buff, DWORD sector, UINT count)
{
    int ret;

    if (index >= FF_VOLUMES)
    {

        return STA_NODISK;
    }
    else if (s_block_device[index] == NULL)
    {

        return RES_NOTRDY;
    }

    ret = blockDeviceRead(s_block_device[index], sector, count, buff);
    if (ret != count)
        return RES_ERROR;

    return RES_OK;
}

DRESULT fat_disk_write(BYTE index, const BYTE *buff, DWORD sector, UINT count)
{
    int ret;

    if (index >= FF_VOLUMES)
    {

        return STA_NODISK;
    }
    else if (s_block_device[index] == NULL)
    {

        return RES_NOTRDY;
    }

    ret = blockDeviceWrite(s_block_device[index], sector, count, buff);
    if (ret != count)
        return RES_ERROR;

    return RES_OK;
}

DRESULT fat_disk_ioctl(BYTE index, BYTE *buff, BYTE cmd)
{
    DRESULT res;

    if (index >= FF_VOLUMES)
    {
        return STA_NODISK;
    }
    else if (s_block_device[index] == NULL)
    {
        return RES_NOTRDY;
    }

    res = RES_PARERR;

    switch (cmd)
    {
    case CTRL_SYNC:
        //TODO
        res = RES_OK;
        break;
    case CTRL_TRIM:
    {
        res = RES_OK;
        DWORD start = ((DWORD *)buff)[0];
        DWORD end = ((DWORD *)buff)[1];
        if (end >= start)
        {
            int r = blockDeviceErase(s_block_device[index], start, end - start + 1);
            if (r < 0)
                res = RES_ERROR;
            // COS_Assert(r >= 0, "bdev erase: res is %d", r);
        }
    }
    break;
    case GET_SECTOR_COUNT:
        *((DWORD *)(buff)) = s_block_device[index]->block_count;
        res = RES_OK;
        break;
    case GET_SECTOR_SIZE:
        *((DWORD *)(buff)) = s_block_device[index]->block_size;
        res = RES_OK;
        break;
    case GET_BLOCK_SIZE:
        *((DWORD *)(buff)) = 1;
        res = RES_OK;
        break;
    default:
        break;
    }

    return res;
}

typedef struct
{
    uint16_t uYear;
    uint8_t uMonth;
    uint8_t uDayOfWeek;
    uint8_t uDay;
    uint8_t uHour;
    uint8_t uMinute;
    uint8_t uSecond;
    uint16_t uMilliseconds;
} tm_systime;

DWORD fat_disk_gettime(void)
{
    tm_systime SystemTime = {
        .uYear = 2018,
        .uMonth = 1,
    };

    return ((DWORD)(SystemTime.uYear - 1980) << 25) |
           ((DWORD)SystemTime.uMonth << 21) |
           ((DWORD)SystemTime.uDay << 16) |
           (WORD)(SystemTime.uHour << 11) |
           (WORD)(SystemTime.uMinute << 5) |
           (WORD)(SystemTime.uSecond >> 1);
}

#if 0
extern unsigned char g_Character[12];

WCHAR fat_disk_uni2oem (DWORD uni, WORD cp)
{
    WCHAR c = 0;
    if (uni < 0x80) {
        c = (WCHAR)uni;
    } else {
        struct ML_Table *pTable = ML_FindTable(g_Character);
        if( NULL != pTable )
            pTable->uni2char((UINT16)uni, (UINT8*)&c, 2);
    }
    return c;
}

WCHAR fat_disk_oem2uni (WCHAR oem, WORD cp)
{
    WCHAR c = 0;
    if (oem < 0x80) {
        c = oem;
    } else {
        struct ML_Table *pTable = ML_FindTable(g_Character);
        if( NULL != pTable )
            pTable->char2uni((UINT8 *)&oem, 2, &c);
    }
    return c;
}
#endif
