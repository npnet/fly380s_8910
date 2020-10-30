/*****************************************************************************
* Copyright (c) 2020 ABUP.Co.Ltd. All rights reserved.
* File name: abup_file.c
* Description: fota
* Author: YK
* Version: v1.0
* Date: 20200403
*****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "vfs.h"
#include "osi_api.h"
#include "osi_log.h"
#include "abup_fota.h"
#include "abup_file.h"

#define ABUP_FOTA_DATA_DIR "/fota"
#define ABUP_VERSION 		"version.txt"
#define ABUP_LOGIN 			"login.txt"
#define ABUP_DETAID_FILE 	"fota.pack"

#define ABUP_DETAID_FILE_NAME ABUP_FOTA_DATA_DIR "/" ABUP_DETAID_FILE
#define ABUP_LOGIN_FILE_NAME ABUP_FOTA_DATA_DIR "/" ABUP_LOGIN
#define ABUP_VERSION_FILE_NAME ABUP_FOTA_DATA_DIR "/" ABUP_VERSION

//fota文件夹
char* abup_get_abup_root_path(void)
{
	return (char *)ABUP_FOTA_DATA_DIR;
}

//差分包文件
char* abup_get_delta_file_path(void)
{
	return (char *)ABUP_DETAID_FILE_NAME;
}

//存放版本号和差分包delta ID
char* abup_get_version_file_path(void)
{
	return (char *)ABUP_VERSION_FILE_NAME;
}

//注册设备信息时存放设备唯一标识码
char* abup_get_login_file_path(void)
{
	return (char *)ABUP_LOGIN_FILE_NAME;
}

//删除文件夹下的对应文件
int32_t abup_remove_file(char *file_path)
{
	int32_t ret=0;

	ret=vfs_unlink(file_path);
	OSI_LOGI(0,"abup Remove_File,ret=%d",ret);
	if(ret == 0)
		return 1;
	else
		return 0;
}

char *abup_recv_buf = NULL;
int32_t total_len=0;
void abup_recv_buf_memory(void)
{
	uint32_t buf_len=abup_recv_buff_len();

	if (abup_recv_buf == NULL)
	{
		abup_recv_buf = (char *)malloc(buf_len);
	}
	if (abup_recv_buf == NULL)
	{
		OSI_LOGI(0,"abup malloc recv buff fail.");
		return;
	}
	else
	{
		total_len=0;
		OSI_LOGI(0,"abup malloc recv buff sucess:0x%x.", abup_recv_buf);
	}
}

void abup_socket_buf_free(void)
{
	if (abup_recv_buf)
	{
		free(abup_recv_buf);
		abup_recv_buf = NULL;
		total_len=0;
		OSI_LOGI(0,"abup free socket buff sucess");
	}
}

//将每组差分包数据copy到动态内存里
//buff:  每包 差分包数据
//write_len:  每包 差分包大小
int8_t abup_copy_detla_buff(char *buff,uint32_t data_len)
{
	int32_t ret=-1;

	memcpy(abup_recv_buf+total_len, buff, data_len);
	total_len+=data_len;

	if(total_len>=abup_recv_buff_len())
	{
		ret=abup_write_detla_to_file();
		if(total_len==ret)
		{
			total_len=0;
			return 1;
		}
		else
			return 0;
	}
	else
		return 1;
}

//下载结束，将 动态buff 的内容写入文件 abup_get_delta_file_path()
int32_t abup_write_detla_to_file(void)
{
	int pfile;
	int32_t ret=-1;

	OSI_LOGI(0,"abup write_detla_buff,total_len=%d",total_len);
	OSI_LOGI(0,"abup-->temp_buf1:%x,%x,%x,%x", abup_recv_buf[0], abup_recv_buf[1], abup_recv_buf[2], abup_recv_buf[3]);
	OSI_LOGI(0,"abup-->temp_buf2:%x,%x,%x,%x", abup_recv_buf[total_len-4], abup_recv_buf[total_len-3], abup_recv_buf[total_len-2], abup_recv_buf[total_len-1]);
	osiThreadSleep (300);

	pfile = vfs_open(abup_get_delta_file_path(), O_RDWR | O_CREAT, 0);
	vfs_lseek(pfile,0,SEEK_END);
	if(pfile >= 0)
	{
		ret = vfs_write(pfile,abup_recv_buf,total_len);
		OSI_LOGI(0,"abup write_detla_to_file,ret=%d",ret);
		osiThreadSleep (100);
		if(ret != total_len)
		{
			OSI_LOGI(0,"abup write_detla_to_file,ret: %ld",ret);
			return -1;
		}
		vfs_close(pfile);
		return ret;
	}
	else
	{
		OSI_LOGI(0," abup write_detla_to_file,vfs_open fail");
		return -1;
	}

}


//关闭文件
void abup_close_file(int file_handle)
{
	int32_t ret = -1;
	ret=vfs_close(file_handle);
	OSI_LOGI(0,"abup close_file file,ret=%d",ret);
}

//获取文件夹大小，差分包不能大于此值，需贵司完成函数内容
uint32_t abup_get_fota_size(void)
{
	return 8*64*1024;
}

//检测之前是否有升级动作
//下载差分包时会存放版本号到ABUP_DETAID 文件里，用于升级结果上报
uint8_t abup_check_upgrade(void)
{
	int32_t ret = -1;
    struct stat st;

	ret=vfs_stat(abup_get_version_file_path(),&st);
	OSI_LOGI(0,"abup check_upgrade-->ret:%d,file_size:%d",ret,st.st_size);
	osiThreadSleep (100);
	if(ret==0&&st.st_size>0)
		return 1;
	else
		return 0;
}

//初始化文件夹
void abup_init_file(void)
{
	int32_t ret = -1;

	#if 0
	ret=vfs_rmdir_recursive(abup_get_abup_root_path());
	OSI_LOGI(0,"abup:vfs_rmdir_recursive,ret:%d",ret);
    osiThreadSleep (100);
	if(ret<0){
		return;
	}
	#endif
	ret=vfs_mkdir(abup_get_abup_root_path(),0);
	if(ret<0)
		OSI_LOGI(0,"abup_init_file,Already exists");
	else
		OSI_LOGI(0,"abup_init_file,success");
}

