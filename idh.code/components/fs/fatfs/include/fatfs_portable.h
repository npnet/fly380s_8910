
#ifndef _FATFS_PORTABLE
#define _FATFS_PORTABLE

#ifdef __cplusplus
extern "C" {
#endif
#include "stdbool.h"
//BYTE fat_disk_set_device(blockDevice_t *dev);

bool fat_disk_remove_device(BYTE idx);

DSTATUS fat_disk_status(BYTE index);

DSTATUS fat_disk_initialize(BYTE index);

DRESULT fat_disk_read(BYTE index, BYTE *buff, DWORD sector, UINT count);

DRESULT fat_disk_write(BYTE index, const BYTE *buff, DWORD sector, UINT count);

DRESULT fat_disk_ioctl(BYTE index, BYTE *buff, BYTE cmd);

#ifdef __cplusplus
}
#endif

#endif
