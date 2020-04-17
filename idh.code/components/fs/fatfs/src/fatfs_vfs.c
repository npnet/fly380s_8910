/* Copyright (C) 2018 RDA Technologies Limited and/or its affiliates("RDA").
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
#include "vfs.h"
#include "vfs_ops.h"

#define DIR FDIR
#include "ff.h"
#undef DIR

#include "fatfs_vfs.h"
#include "sys/errno.h"
#include "block_device.h"
#include "osi_api.h"
#include "stdio.h"
#include "string.h"

int errno __attribute__((weak));
#define SET_ERRNO(err) \
    do                 \
    {                  \
        errno = err;   \
    } while (0)

#define FS_ERR_RETURN(fr, failed)        \
    do                                   \
    {                                    \
        SET_ERRNO(fat_res_to_errno(fr)); \
        return (failed);                 \
    } while (0)

#define ERR_RETURN(err, failed) \
    do                          \
    {                           \
        SET_ERRNO(err);         \
        return (failed);        \
    } while (0)

typedef long off_t;

extern size_t strlen(const char *s);
extern char *strcpy(char *dest, const char *src);
extern char *strncpy(char *dest, const char *src, size_t n);
extern char *strchr(const char *s, int c);
extern char *strrchr(const char *s, int c);
extern char *strdup(const char *s);
extern void *memcpy(void *dest, const void *src, size_t n);
extern int memcmp(const void *s1, const void *s2, size_t n);
extern void *malloc(size_t size);
extern void free(void *ptr);
extern void *calloc(size_t nmemb, size_t size);
extern void *realloc(void *ptr, size_t size);
extern int strcmp(const char *s1, const char *s2);

#define FAT_OPEN_MAX (32)

struct fat_fs_ctx
{
    FATFS fs;
    char vol_path[32];
    BYTE fs_dev_no;
    unsigned int open_file_map;
    FIL file_array[FAT_OPEN_MAX];
    osiSemaphore_t *sema;
};

static struct fat_fs_ctx *s_fat_ctx_table[FF_VOLUMES] = {NULL};

static int fat_res_to_errno(FRESULT fr)
{
    switch (fr)
    {
    case FR_OK:
        return 0;
    case FR_DISK_ERR:
    case FR_INVALID_DRIVE:
        return ENODEV;
    case FR_INT_ERR:
        return EINTR;
    case FR_NOT_READY:
        return EBUSY;
    case FR_NO_FILE:
    case FR_NO_PATH:
    case FR_INVALID_NAME:
        return ENOENT;
    case FR_TOO_MANY_OPEN_FILES:
        return ENFILE;
    case FR_NO_FILESYSTEM:
    case FR_INVALID_PARAMETER:
        return EINVAL;
    case FR_DENIED:
        return EACCES;
    case FR_EXIST:
        return EEXIST;
    case FR_WRITE_PROTECTED:
        return EROFS;
    case FR_NOT_ENOUGH_CORE:
    case FR_NOT_ENABLED:
        return ENOMEM;
    case FR_LOCKED:
        return EDEADLK;
    case FR_TIMEOUT:
        return ETIME;
    case FR_MKFS_ABORTED:
    case FR_INVALID_OBJECT:
        return EPERM;
    default:
        return EIO;
    }
    return ENOTSUP;
}

static inline void fat_lock_init(void *ctx)
{
    //COS_SemaInit(&((struct fat_fs_ctx *)ctx)->sema, 1);
    ((struct fat_fs_ctx *)ctx)->sema = osiSemaphoreCreate(1, 1);
}

static inline void fat_lock_term(void *ctx)
{
    //COS_SemaDestroy(&((struct fat_fs_ctx *)ctx)->sema);
    osiSemaphoreDelete(((struct fat_fs_ctx *)ctx)->sema);
}

static inline void fat_lock(void *ctx)
{
    //COS_SemaTake(&((struct fat_fs_ctx *)ctx)->sema);
    osiSemaphoreAcquire(((struct fat_fs_ctx *)ctx)->sema);
}

static inline void fat_unlock(void *ctx)
{
    //COS_SemaRelease(&((struct fat_fs_ctx *)ctx)->sema);
    osiSemaphoreRelease(((struct fat_fs_ctx *)ctx)->sema);
}

static struct fat_fs_ctx *fat_mp_to_ctx(const char *mp)
{
    int i = 0;
    for (struct fat_fs_ctx *c = s_fat_ctx_table[i]; c != NULL && i < FF_VOLUMES; i++)
    {
        if (strcmp(mp, c->vol_path) == 0)
            return c;
    }
    return NULL;
}

static int fat_mode_conv(int mode)
{
    int res = 0;
    int acc_mode = mode & O_ACCMODE;

    if (acc_mode == O_RDONLY)
        res |= FA_READ;
    else if (acc_mode == O_WRONLY)
        res |= FA_WRITE;
    else if (acc_mode == O_RDWR)
        res |= FA_READ | FA_WRITE;

    if (mode & O_TRUNC)
        res |= FA_CREATE_ALWAYS;
    else if (mode & O_APPEND)
        res |= FA_OPEN_APPEND;
    else if (mode & O_EXCL)
        res |= FA_CREATE_NEW;
    else if (mode & O_CREAT)
        res |= FA_OPEN_ALWAYS;

    return res;
}

static int get_free_fil(struct fat_fs_ctx *ctx)
{
    int ret = 0;
    unsigned int _bitmap = ctx->open_file_map;

    for (ret = 0; ret < FAT_OPEN_MAX; ret++)
    {
        if ((_bitmap & 0x1) == 0)
        {
            return ret;
        }
        _bitmap >>= 1;
    }
    return -1;
}

static int set_fil_bmp(struct fat_fs_ctx *ctx, int i)
{
    if (i < 0 || i >= FAT_OPEN_MAX)
        return -1;

    ctx->open_file_map |= (0x1 << i);

    return 0;
}

static int free_fil_bmp(struct fat_fs_ctx *ctx, int i)
{
    if (i < 0 || i >= FAT_OPEN_MAX)
        return -1;

    ctx->open_file_map &= ~(0x1 << i);

    return 0;
}

static bool is_fil_map_set(struct fat_fs_ctx *ctx, int i)
{
    if (i < 0 || i >= FAT_OPEN_MAX)
        return false;

    return ctx->open_file_map & (0x1 << i);
}

static int fat_vfs_open(void *ctx, const char *path, int flags, int mode)
{
    FRESULT fr;
    int m_mode;
    int i_fil;
    struct fat_fs_ctx *_ctx = (struct fat_fs_ctx *)ctx;

    if (path[0] == '\0')
        ERR_RETURN(ENOENT, -1);

    fat_lock(_ctx);

    m_mode = fat_mode_conv(flags);
    i_fil = get_free_fil(_ctx);
    if (i_fil < 0)
    {
        fat_unlock(_ctx);
        ERR_RETURN(ENFILE, -1);
    }
    fr = f_open(&_ctx->file_array[i_fil], path, m_mode);
    if (fr != FR_OK)
    {
        fat_unlock(_ctx);
        FS_ERR_RETURN(fr, -1);
    }

    set_fil_bmp(_ctx, i_fil);
    fat_unlock(ctx);
    return i_fil;
}

static int fat_vfs_close(void *ctx, int fd)
{
    int ret = 0;
    FRESULT fr;
    struct fat_fs_ctx *_ctx = (struct fat_fs_ctx *)ctx;

    fat_lock(_ctx);
    if (is_fil_map_set(_ctx, fd))
    {
        fr = f_close(&_ctx->file_array[fd]);
        if (fr != FR_OK)
        {
            fat_unlock(_ctx);
            FS_ERR_RETURN(fr, -1);
        }
    }
    else
    {
        fat_unlock(_ctx);
        ERR_RETURN(EINVAL, -1);
    }

    free_fil_bmp(_ctx, fd);
    fat_unlock(_ctx);
    return ret;
}

static ssize_t fat_vfs_read(void *ctx, int fd, void *dst, size_t size)
{
    struct fat_fs_ctx *_ctx = (struct fat_fs_ctx *)ctx;
    ssize_t read_len = -1;

    fat_lock(_ctx);
    if (is_fil_map_set(_ctx, fd))
    {
        FRESULT fr = f_read(&_ctx->file_array[fd], dst, size, (UINT *)&read_len);
        if (fr != FR_OK)
        {
            fat_unlock(_ctx);
            FS_ERR_RETURN(fr, -1);
        }
    }
    else
    {
        SET_ERRNO(EINVAL);
    }

    fat_unlock(_ctx);
    return read_len;
}

static ssize_t fat_vfs_write(void *ctx, int fd, const void *data, size_t size)
{
    struct fat_fs_ctx *_ctx = (struct fat_fs_ctx *)ctx;

    fat_lock(_ctx);
    if (!is_fil_map_set(_ctx, fd))
    {
        fat_unlock(_ctx);
        ERR_RETURN(EINVAL, -1);
    }

    FATFS *fs;
    DWORD fre_clust;
    ssize_t write_len = -1;
    FRESULT fr = f_getfree(_ctx->vol_path, &fre_clust, &fs);
    if (fr == FR_OK)
    {
        if (fre_clust * fs->csize * 512 < size)
            SET_ERRNO(ENOSPC);
        else
            fr = f_write(&_ctx->file_array[fd], data, size, (UINT *)&write_len);
    }

    if (fr != FR_OK)
    {
        SET_ERRNO(fat_res_to_errno(fr));
    }
    fat_unlock(_ctx);
    return write_len;
}

static long fat_vfs_lseek(void *ctx, int fd, long offset, int mode)
{
    FRESULT fr;
    FSIZE_t off = 0L;
    struct fat_fs_ctx *_ctx = (struct fat_fs_ctx *)ctx;

    fat_lock(_ctx);
    if (!is_fil_map_set(_ctx, fd))
    {
        fat_unlock(_ctx);
        ERR_RETURN(EINVAL, -1);
    }

    switch (mode)
    {
    case SEEK_SET:
        off = offset;
        break;
    case SEEK_CUR:
        if (offset == 0L)
            off = f_tell(&_ctx->file_array[fd]);
        else if (offset > 0L)
            off = ((f_tell(&_ctx->file_array[fd]) + offset > f_size(&_ctx->file_array[fd])) ? f_size(&_ctx->file_array[fd]) : (f_tell(&_ctx->file_array[fd]) + offset));
        else
            off = ((f_tell(&_ctx->file_array[fd]) > -offset) ? (f_tell(&_ctx->file_array[fd]) + offset) : 0);
        break;
    case SEEK_END:
        off = ((offset > 0L) ? f_size(&_ctx->file_array[fd]) : (f_size(&_ctx->file_array[fd]) + offset));
        break;
    default:
        fat_unlock(_ctx);
        ERR_RETURN(EINVAL, -1);
    }
    fr = f_lseek(&_ctx->file_array[fd], off);

    if (fr != FR_OK)
    {
        fat_unlock(_ctx);
        FS_ERR_RETURN(fr, -1);
    }

    long ret = f_tell(&_ctx->file_array[fd]);

    fat_unlock(_ctx);
    return ret;
}

static int fat_vfs_fstat(void *ctx, int fd, struct stat *st)
{
    struct fat_fs_ctx *_ctx = (struct fat_fs_ctx *)ctx;

    fat_lock(_ctx);
    if (!is_fil_map_set(_ctx, fd))
    {
        fat_unlock(_ctx);
        ERR_RETURN(EINVAL, -1);
    }

    st->st_size = f_size(&_ctx->file_array[fd]);
    st->st_mode = S_IRWXU | S_IRWXG | S_IRWXO;
    if (_ctx->file_array[fd].obj.attr & AM_RDO)
    {
        st->st_mode &= ~(S_IWUSR | S_IWGRP | S_IWOTH);
    }
    st->st_mode |= (_ctx->file_array[fd].obj.attr & AM_DIR) ? S_IFDIR : S_IFREG;
    st->st_mtime = 0;
    st->st_atime = 0;
    st->st_ctime = 0;

    //TODO other st members ....
    fat_unlock(_ctx);
    return 0;
}

static int fat_vfs_stat(void *ctx, const char *path, struct stat *st)
{
    FRESULT fr;
    FILINFO fno;
    struct fat_fs_ctx *_ctx = (struct fat_fs_ctx *)ctx;

    if (st == NULL)
    {
        ERR_RETURN(ENOENT, -1);
    }
    if (strcmp(path, _ctx->vol_path) == 0)
    {
        st->st_size = 0;
        st->st_mtime = 0;
        st->st_atime = 0;
        st->st_ctime = 0;
        st->st_mode = S_IRWXU | S_IRWXG | S_IRWXO | S_IFDIR;
        return 0;
    }

    fat_lock(_ctx);
    fr = f_stat(path, &fno);
    if (fr != FR_OK)
    {
        fat_unlock(_ctx);
        FS_ERR_RETURN(fr, -1);
    }
    st->st_size = fno.fsize;
    st->st_mtime = fno.ftime;
    st->st_atime = fno.ftime;
    st->st_ctime = fno.ftime;

    st->st_mode = S_IRWXU | S_IRWXG | S_IRWXO;
    if (fno.fattrib & AM_RDO)
    {
        st->st_mode &= ~(S_IWUSR | S_IWGRP | S_IWOTH);
    }
    st->st_mode |= (fno.fattrib & AM_DIR) ? S_IFDIR : S_IFREG;

    //TODO other st members ....
    fat_unlock(_ctx);
    return 0;
}

static int fat_vfs_fsync(void *ctx, int fd)
{
    FRESULT fr;
    struct fat_fs_ctx *_ctx = (struct fat_fs_ctx *)ctx;

    fat_lock(_ctx);
    if (!is_fil_map_set(_ctx, fd))
    {
        fat_unlock(_ctx);
        ERR_RETURN(EINVAL, -1);
    }
    fr = f_sync(&_ctx->file_array[fd]);
    fat_unlock(_ctx);
    if (fr != FR_OK)
        FS_ERR_RETURN(fr, -1);
    return 0;
}

static int fat_vfs_ftruncate(void *ctx, int fd, long length)
{
    FRESULT fr;
    struct fat_fs_ctx *_ctx = (struct fat_fs_ctx *)ctx;

    fat_lock(_ctx);
    if (!is_fil_map_set(_ctx, fd))
    {
        fat_unlock(_ctx);
        ERR_RETURN(ENOENT, -1);
    }

    long size = _ctx->file_array[fd].obj.objsize;

    if (length == size)
    {
        fat_unlock(_ctx);
        return 0;
    }
    else if (length < size)
    {
        fr = f_lseek(&_ctx->file_array[fd], length);

        if (fr == FR_OK)
            fr = f_truncate(&_ctx->file_array[fd]);
        fat_unlock(_ctx);
        if (fr != FR_OK)
            FS_ERR_RETURN(fr, -1);

        return 0;
    }
    else // (length > size)
    {
        //fr = f_expand(&_ctx->file_array[fd], length, 1);
        char buf[32] = {};

        fr = f_lseek(&_ctx->file_array[fd], size);
        if (fr != FR_OK)
        {
            fat_unlock(_ctx);
            FS_ERR_RETURN(fr, -1);
        }
        while (size < length)
        {
            int wr_len;
            int trans = (length - size > 32) ? 32 : (length - size);
            if (f_write(&_ctx->file_array[fd], buf, trans, (UINT *)&wr_len) != FR_OK || trans != wr_len)
            {
                fat_unlock(_ctx);
                FS_ERR_RETURN(fr, -1);
            }
            size += trans;
        }
        fat_unlock(_ctx);
        return 0;
    }

    fat_unlock(_ctx);
    return 0;
}

static int fat_vfs_truncate(void *ctx, const char *path, long length)
{
    FILINFO info;

    if (f_stat(path, &info) != FR_OK)
    {
        ERR_RETURN(ENOENT, -1);
    }
    int fd = fat_vfs_open(ctx, path, O_WRONLY, 0);
    if (fd < 0)
    {
        return -1;
    }
    int ret = fat_vfs_ftruncate(ctx, fd, length);

    fat_vfs_close(ctx, fd);

    return ret;
}

static int fat_vfs_unlink(void *ctx, const char *path)
{
    FRESULT fr;

    fat_lock(ctx);
    fr = f_unlink(path);
    fat_unlock(ctx);

    if (fr != FR_OK)
        FS_ERR_RETURN(fr, -1);

    return 0;
}

static int fat_vfs_rename(void *ctx, const char *src, const char *dst)
{
    FRESULT fr;

    fat_lock(ctx);
    fr = f_rename(src, dst);
    fat_unlock(ctx);

    if (fr != FR_OK)
        FS_ERR_RETURN(fr, -1);

    return 0;
}

typedef struct
{
    DIR v_dir;
    FDIR fat_dir;
    struct dirent e;
} MY_DIR;

static DIR *fat_vfs_opendir(void *ctx, const char *name)
{
    MY_DIR *my_dir;
    FRESULT fr;

    my_dir = (MY_DIR *)calloc(1, sizeof(MY_DIR));

    if (my_dir == NULL)
    {
        ERR_RETURN(ENOMEM, NULL);
    }
    fat_lock(ctx);
    fr = f_opendir(&my_dir->fat_dir, name);

    if (fr != FR_OK)
    {
        free(my_dir);
        fat_unlock(ctx);
        FS_ERR_RETURN(fr, NULL);
    }

    f_readdir(&my_dir->fat_dir, NULL);

    fat_unlock(ctx);
    return (DIR *)(&my_dir->v_dir);
}

static struct dirent *fat_vfs_readdir(void *ctx, DIR *pdir)
{
    FRESULT fr;
    MY_DIR *my_dir;
    FILINFO fno;

    my_dir = (MY_DIR *)pdir;

    fat_lock(ctx);
    fr = f_readdir(&my_dir->fat_dir, &fno);
    if (fr != FR_OK || fno.fname[0] == 0)
    {
        fat_unlock(ctx);
        FS_ERR_RETURN(fr, NULL);
    }
    my_dir->e.d_ino = 0;
    my_dir->e.d_type = (fno.fattrib & AM_DIR) ? DT_DIR : DT_REG;
    strcpy((char *)&my_dir->e.d_name, (const char *)&fno.fname);

    fat_unlock(ctx);
    return &my_dir->e;
}

static long fat_vfs_telldir(void *ctx, DIR *pdir)
{
    long i;
    FRESULT fr;
    FILINFO fno;

    MY_DIR *my_dir = (MY_DIR *)pdir;

    fat_lock(ctx);
    // set to the head
    f_readdir(&my_dir->fat_dir, NULL);

    i = 0;
    while ((fr = f_readdir(&my_dir->fat_dir, &fno)) == FR_OK && fno.fname[0] != 0)
    {
        i++;
    }

    f_readdir(&my_dir->fat_dir, NULL);

    fat_unlock(ctx);
    return i;
}

static void fat_vfs_seekdir(void *ctx, DIR *pdir, long loc)
{
    long i;
    FRESULT fr;
    FILINFO fno;

    MY_DIR *my_dir = (MY_DIR *)pdir;

    fat_lock(ctx);
    // set to the head
    f_readdir(&my_dir->fat_dir, NULL);

    for (i = 0; i < loc; i++)
    {
        fr = f_readdir(&my_dir->fat_dir, &fno);
        if (fr != FR_OK || fno.fname[0] == 0)
        {
            break;
        }
    }
    fat_unlock(ctx);
    return;
}

static int fat_vfs_closedir(void *ctx, DIR *pdir)
{
    MY_DIR *my_dir;
    FRESULT fr;

    my_dir = (MY_DIR *)pdir;

    fat_lock(ctx);
    f_readdir(&my_dir->fat_dir, NULL);

    fr = f_closedir(&my_dir->fat_dir);
    free(my_dir);

    if (fr != FR_OK)
    {
        fat_unlock(ctx);
        FS_ERR_RETURN(fr, -1);
    }
    fat_unlock(ctx);
    return 0;
}

static int fat_vfs_mkdir(void *ctx, const char *name, int mode)
{
    FRESULT fr;

    fat_lock(ctx);
    fr = f_mkdir(name);
    fat_unlock(ctx);

    if (fr != FR_OK)
        FS_ERR_RETURN(fr, -1);

    return 0;
}

static int fat_vfs_rmdir(void *ctx, const char *name)
{
    FRESULT fr;
    FILINFO fno;

    fat_lock(ctx);
    fr = f_stat(name, &fno);

    if (fr == FR_OK)
    {
        if (fno.fattrib & AM_DIR)
        {
            fr = f_unlink(name);
            if (fr == FR_DENIED)
            {
                fat_unlock(ctx);
                ERR_RETURN(ENOTEMPTY, -1);
            }
        }
        else
        {
            fat_unlock(ctx);
            ERR_RETURN(ENOTDIR, -1);
        }
    }

    fat_unlock(ctx);
    if (fr != FR_OK)
        FS_ERR_RETURN(fr, -1);
    return 0;
}

static int fat_vfs_statvfs(void *ctx, struct statvfs *buf)
{
    FRESULT fr;
    FATFS *fs;
    DWORD fre_clust;

    if (buf == NULL)
        ERR_RETURN(EINVAL, -1);

    struct fat_fs_ctx *_ctx = (struct fat_fs_ctx *)ctx;

    fat_lock(ctx);
    fr = f_getfree(_ctx->vol_path, &fre_clust, &fs);
    if (fr != FR_OK)
    {
        fat_unlock(ctx);
        FS_ERR_RETURN(fr, -1);
    }
    memset((void *)buf, 0, sizeof(*buf));
    buf->f_bsize = 512;
    buf->f_blocks = fs->n_fatent * fs->csize;
    buf->f_bfree = fre_clust * fs->csize;
    buf->f_bavail = fre_clust * fs->csize;
    buf->f_namemax = FF_MAX_LFN;

    fat_unlock(ctx);
    return 0;
}

static int fat_vfs_umount(void *ctx);

static vfs_ops_t g_fat_vfs_ops = {
    .flags = VFS_NEED_REAL_PATH,
    .umount = fat_vfs_umount,
    .open = fat_vfs_open,
    .close = fat_vfs_close,
    .read = fat_vfs_read,
    .write = fat_vfs_write,
    .lseek = fat_vfs_lseek,
    .fstat = fat_vfs_fstat,
    .stat = fat_vfs_stat,
    .truncate = fat_vfs_truncate,
    .ftruncate = fat_vfs_ftruncate,
    .unlink = fat_vfs_unlink,
    .rename = fat_vfs_rename,
    .opendir = fat_vfs_opendir,
    .readdir = fat_vfs_readdir,
    .readdir_r = NULL,
    .telldir = fat_vfs_telldir,
    .seekdir = fat_vfs_seekdir,
    .closedir = fat_vfs_closedir,
    .mkdir = fat_vfs_mkdir,
    .rmdir = fat_vfs_rmdir,
    .fsync = fat_vfs_fsync,
    .statvfs = fat_vfs_statvfs,
    .sfile_init = NULL,
    .sfile_write = NULL,
    .sfile_read = NULL,
    .sfile_size = NULL,
};

extern bool fat_disk_remove_device(BYTE idx);
extern BYTE fat_disk_set_device(blockDevice_t *dev);

static int fat_vfs_umount(void *ctx)
{
    struct fat_fs_ctx *_ctx = (struct fat_fs_ctx *)ctx;

    fat_lock(_ctx);
    f_unmount(_ctx->vol_path);
    fat_disk_remove_device(_ctx->fs_dev_no);
    s_fat_ctx_table[_ctx->fs_dev_no] = NULL;
    fat_unlock(_ctx);

    fat_lock_term(_ctx);
    free(_ctx);

    return 0;
}

int fatfs_vfs_mount(const char *base_path, blockDevice_t *bdev)
{
    FRESULT res;
    BYTE idx;

    if (bdev == NULL)
        ERR_RETURN(EINVAL, -1);

    idx = fat_disk_set_device(bdev);
    if (idx == 0xFF)
        ERR_RETURN(EXDEV, -1);

    struct fat_fs_ctx *_ctx = (struct fat_fs_ctx *)calloc(1, sizeof(struct fat_fs_ctx));
    if (_ctx == NULL)
        ERR_RETURN(ENOMEM, -1);

    fat_lock_init(_ctx);
    fat_lock(_ctx);

    _ctx->open_file_map = 0;
    _ctx->fs_dev_no = idx;
    strncpy(_ctx->vol_path, base_path, 32);

    s_fat_ctx_table[_ctx->fs_dev_no] = _ctx;

    res = f_mount(&_ctx->fs, _ctx->vol_path, 1);
    if (res != 0)
    {
        SET_ERRNO(fat_res_to_errno(res));

        goto failed;
    }

    if (vfs_register(base_path, &g_fat_vfs_ops, _ctx) < 0)
    {
        f_unmount(_ctx->vol_path);

        goto failed;
    }
    fat_unlock(_ctx);
    return 0;

failed:
    fat_disk_remove_device(_ctx->fs_dev_no);
    s_fat_ctx_table[_ctx->fs_dev_no] = NULL;
    fat_unlock(_ctx);
    fat_lock_term(_ctx);

    free(_ctx);
    return -1;
}

int fatfs_vfs_mkfs(blockDevice_t *bdev)
{
    BYTE work[1024];
    FRESULT res;
    BYTE idx;

    if (bdev == NULL)
        ERR_RETURN(EINVAL, -1);

    idx = fat_disk_set_device(bdev);
    if (idx == 0xFF)
        ERR_RETURN(EXDEV, -1);

    struct fat_fs_ctx _ctx;

    _ctx.open_file_map = 0;
    _ctx.fs_dev_no = idx;
    strcpy(_ctx.vol_path, "/mtmp");

    s_fat_ctx_table[_ctx.fs_dev_no] = &_ctx;
    //res = f_mkfs(_ctx.vol_path, FM_FAT | FM_SFD, 0, work, sizeof(work));

    res = f_mkfs(_ctx.vol_path, FM_FAT, 0, work, sizeof(work));
    fat_disk_remove_device(_ctx.fs_dev_no);
    s_fat_ctx_table[_ctx.fs_dev_no] = NULL;

    if (res != FR_OK)
        FS_ERR_RETURN(res, -1);

    return 0;
}

int fatfs_vfs_set_volume(const char *mount_path, const char *vol_label)
{
    FRESULT fr;
    char volbuff[64];

    if (mount_path == NULL || vol_label == NULL)
        ERR_RETURN(EINVAL, -1);

    sprintf(volbuff, "%s%s", mount_path, vol_label);
    struct fat_fs_ctx *ctx = fat_mp_to_ctx(mount_path);
    if (ctx == NULL)
        ERR_RETURN(EINVAL, -1);

    fat_lock(ctx);
    fr = f_setlabel(volbuff);
    fat_unlock(ctx);
    if (fr != FR_OK)
        FS_ERR_RETURN(fr, -1);

    return 0;
}

int fatfs_vfs_get_volume(const char *mount_path, char *vol_label)
{
    FRESULT fr;

    if (mount_path == NULL || vol_label == NULL)
        ERR_RETURN(EINVAL, -1);

    struct fat_fs_ctx *ctx = fat_mp_to_ctx(mount_path);
    if (ctx == NULL)
        ERR_RETURN(EINVAL, -1);

    fat_lock(ctx);
    fr = f_getlabel(mount_path, vol_label, 0);
    fat_unlock(ctx);
    if (fr != FR_OK)
        FS_ERR_RETURN(fr, -1);

    return 0;
}

int export_get_ldnumber(const TCHAR **path)
{
    UINT i;
    int vol = -1;
    int mounted = 0;

    if (*path != 0)
    { /* If the pointer is not a null */
        for (i = 0; i < FF_VOLUMES; i++)
        {
            if (s_fat_ctx_table[i] != NULL)
            {
                int len = strlen(s_fat_ctx_table[i]->vol_path);
                if (strncmp(s_fat_ctx_table[i]->vol_path, *path, len) == 0)
                {
                    *path += len;
                    if (**path == '/')
                        *path += 1;
                    vol = s_fat_ctx_table[i]->fs_dev_no;
                    break;
                }
                mounted++;
            }
        }
        if (mounted == 0)
        {
            vol = 0;
        }
    }
    return vol;
}
