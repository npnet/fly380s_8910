/*****************************************************************************
* Copyright (c) 2020 ABUP.Co.Ltd. All rights reserved.
* File name: abup_fota.c
* Description: fota
* Author: YK
* Version: v1.0
* Date: 20200403
*****************************************************************************/
#include "abup_fota.h"
#include "abup_http.h"
#include "abup_common.h"
#include "abup_file.h"
#include "osi_api.h"
#include "osi_log.h"
#include "lwip/netdb.h"

#define ABUP_STACK_SIZE (1024 * 16)
#define ABUP_EVENT_QUEUE_SIZE (64 * 1)

#define ABUP_DOWNLOAD_UNIT_BYTES    1*1024 //每次下载差分包数据的最大长度
#define ABUP_SOCKET_BUFFER_LEN 2*1024 //socket buff最大长度，需要大于ABUP_DOWNLOAD_UNIT_BYTES

#define ABUP_RECV_BUFFER_LEN 4*1024 //缓存下载差分包数据长度,达到此长度写入文件系统

typedef struct
{
	int socket;
	bool is_quit;
	struct addrinfo *res;
}abup_task_info;

static abup_task_info s_abup_task_data = {0};
static char s_abup_socket_buf[ABUP_SOCKET_BUFFER_LEN] = {0};
static uint32_t abup_ratio = 0xFF;
osiThread_t *Abup_thread_id;
static uint8_t abup_socket_retry = 0;
uint8_t abup_process_state = 0;
uint8_t abup_current_state = ABUP_FOTA_READY;

//缓存下载数据长度
uint32_t abup_recv_buff_len(void)
{
    return ABUP_RECV_BUFFER_LEN;
}

//socket buff长度
uint32_t abup_socket_buff_len(void)
{
    return ABUP_SOCKET_BUFFER_LEN;
}

//下载数据 buff长度
uint32_t abup_download_len(void)
{
    return ABUP_DOWNLOAD_UNIT_BYTES;
}

//fota域名
char *abup_get_host_domain(void)
{
	if (abup_get_fota_status() == ABUP_FOTA_DL)
	{
		return abup_get_fota_dl_server();
	}
	else
	{
		return abup_get_fota_server();
	}
}

//fota端口
char *abup_get_host_port(void)
{
	if (abup_get_fota_status() == ABUP_FOTA_DL)
	{
		return abup_get_fota_dl_port();
	}
	else
	{
		return abup_get_fota_port();
	}
}

//开始下载差分包
void abup_start_download_packet(void)
{
    OSI_LOGI(0, "Abup start download packet.");
}

/* 重启函数 */
void abup_device_reboot(void)
{
	OSI_LOGI(0, "Abup reboot now......");
	extern bool fupdateSetReady(const char *curr_version);
	if(fupdateSetReady(NULL))
	{
		OSI_LOGI(0, "Abup fota ready ok");
	}

	osiThreadSleep(800);
	osiShutdown(OSI_SHUTDOWN_RESET);
}

/*
//退出fota
abup_get_http_dl_state():99:空间不足;1: 下载成功,   8 :下载失败
abup_get_fota_status():1:注册,2:检测版本,3:下载,4:下载结果上报,5:升级结果上报
abup_get_socket_type():fota 下载结果，对应ABUP_ERROR_TYPE_ENUM
*/
void abup_exit_restore_context(void)
{
    OSI_LOGI(0, "abup:Fota exit:http_dl_state:%d", abup_get_http_dl_state());
    OSI_LOGI(0, "abup:http_state:%d,http_socket_type:%d", abup_get_fota_status(), abup_get_socket_type());

    if (abup_get_http_dl_state() == 1)
    {
		abup_current_state = ABUP_FOTA_REBOOT_UPDATE;
        abup_device_reboot();
        return;
    }
    else
    {
        if (abup_get_socket_type() == 5)
        {
            OSI_LOGI(0, "abup:network error");
			abup_current_state = ABUP_FOTA_NO_NETWORK;
        }
        else if (abup_get_socket_type() == 6)
        {
            OSI_LOGI(0, "abup:not found new update");
			abup_current_state = ABUP_FOTA_NO_NEW_VERSION;
        }
        else if (abup_get_socket_type() == 8)
        {
            OSI_LOGI(0, "abup:Not enough space.");
			abup_current_state = ABUP_FOTA_NOT_ENOUGH_SPACE;
        }
        else if (abup_get_socket_type() == 11)
        {
            OSI_LOGI(0, "abup:imei access count max is 100");
			abup_current_state = ABUP_FOTA_NO_ACCESS_TIMES;
        }
		abup_task_exit();
    }
}

//释放dns
void abup_free_dns(void)
{
	if (s_abup_task_data.res)
	{
		freeaddrinfo(s_abup_task_data.res);
		s_abup_task_data.res = NULL;
	}
}

//释放socket
void abup_close_socket(void)
{
	close(s_abup_task_data.socket);
	s_abup_task_data.socket = -1;
	OSI_LOGI(0, "abup_close_socket");
}

//设置fota下载进度
void abup_set_fota_ratio(uint16_t ratio)
{
	if (abup_ratio != ratio)
	{
		abup_ratio = ratio;
		OSI_LOGI(0, "abup:Current progress = %d ", abup_ratio);
	}
}

//fota差分包下载进度
uint32_t abup_get_fota_ratio(void)
{
	return abup_ratio;
}

int abup_socket_read_ex(char *buf, int len)
{
	return (int)recv(s_abup_task_data.socket, buf, len,0);
}

//退出task
void abup_task_exit(void)
{
	s_abup_task_data.is_quit = true;
	Abup_thread_id = NULL;
	osiThreadExit();
}

//初始化fota相关参数
//login.txt 文件不存在，先进行设备信息注册
static void abup_task_cv_init(void)
{
    OSI_LOGI (0,"abup_task_cv_init");
	s_abup_task_data.socket = -1;
	s_abup_task_data.is_quit = false;
	s_abup_task_data.res = NULL;

	if (abup_get_pid_psec() == false)
	{
		OSI_LOGI(0, "abup_get_pid_psec fail, device should register.");
		abup_set_fota_status(ABUP_FOTA_RG);
		abup_current_state = ABUP_FOTA_RMI;
	}
	else
	{
		abup_set_fota_status(ABUP_FOTA_CV);
		abup_current_state = ABUP_FOTA_CVI;
		abup_set_fota_ratio(0);
	}
	OSI_LOGXI(OSI_LOGPAR_S,0,"abup_task_init-->version:%s", abup_get_firmware_version());
}

static void abup_task_ru_init(void)
{
    OSI_LOGI (0,"abup_task_ru_init");
	s_abup_task_data.socket = -1;
	s_abup_task_data.is_quit = false;
	s_abup_task_data.res = NULL;

	if (abup_get_pid_psec() == false)
	{
		OSI_LOGI(0, "abup_get_pid_psec fail, device should register.");
		abup_remove_file(abup_get_version_file_path());
		return;
	}
	abup_set_fota_status(ABUP_FOTA_RU);
	abup_current_state = ABUP_FOTA_RU;
}

//task进程
static void abup_fota_task (void *argument)
{
	const struct addrinfo hints =
	{
		.ai_family = AF_INET,
		.ai_socktype = SOCK_STREAM,
	};
	struct in_addr *addr;
	uint32_t r=0;
	int dns_retry=0;

	abup_current_state = ABUP_FOTA_START;
    for (int i = 0 ; i < 5 ; i++) {
        osiThreadSleep (6000);
        OSI_LOGI (0, "abup: wait 6s\n");
    }
    OSI_LOGI (0,"abup: wait 30s end\n");
    //osiThreadSleep (1000);

	abup_init_file();
	osiThreadSleep (100);
	if(abup_process_state==1)
	{
		abup_task_cv_init();
	}
	else
	{
		abup_task_ru_init();
	}

	while(!s_abup_task_data.is_quit)
	{
		if (abup_get_fota_status() == ABUP_FOTA_IDLE)
		{
			OSI_LOGI(0,"abup: Current fota in idle.");
			abup_fota_exit();
			osiThreadSleep (1000);
			break;
		}

		OSI_LOGXI(OSI_LOGPAR_S,0,"abup-->domain:%s",abup_get_host_domain());
		OSI_LOGXI(OSI_LOGPAR_S,0,"abup-->port:%s",abup_get_host_port());

		if (s_abup_task_data.res == NULL)
		{
			int err = getaddrinfo(abup_get_host_domain(), abup_get_host_port(), &hints, &s_abup_task_data.res);

			if(err != 0 || s_abup_task_data.res == NULL)
			{
				OSI_LOGI(0,"abup: DNS lookup failed err=%d res=%p", err, s_abup_task_data.res);
				osiThreadSleep (1000);
				dns_retry++;
				if(dns_retry>10){
					dns_retry=0;
					abup_fota_exit();
					break;
				}
				else
					continue;
			}

			/* Code to print the resolved IP.
			Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
			addr = &((struct sockaddr_in *)s_abup_task_data.res->ai_addr)->sin_addr;
			OSI_LOGXI(OSI_LOGPAR_S,0,"abup: DNS lookup succeeded. IP=%s", inet_ntoa(*addr));
		}

		if (s_abup_task_data.socket < 0)
		{
			s_abup_task_data.socket = socket(s_abup_task_data.res->ai_family, s_abup_task_data.res->ai_socktype, 0);
			if(s_abup_task_data.socket < 0)
			{
				OSI_LOGI(0,"abup: Failed to allocate socket.");
				osiThreadSleep (1000);
				continue;
			}
			OSI_LOGI(0,"abup:allocated socket");

			if(connect(s_abup_task_data.socket, s_abup_task_data.res->ai_addr, s_abup_task_data.res->ai_addrlen) != 0)
			{
				OSI_LOGI(0,"abup: socket connect failed errno=%d", errno);
				abup_close_socket();
				osiThreadSleep (3000);
				abup_socket_retry++;
				if(abup_socket_retry>5)
				{
					abup_socket_retry=0;
					abup_fota_exit();
					break;
				}
				else
					continue;
			}
			else{
				abup_socket_retry=0;
			}

			OSI_LOGI(0,"abup: socket connected");
		}

		abup_get_http_data(s_abup_socket_buf, abup_socket_buff_len());
		OSI_LOGXI(OSI_LOGPAR_S,0,"abup: send buf:%s",s_abup_socket_buf);
		osiThreadSleep (200);

		abup_BytePrint(s_abup_socket_buf, strlen(s_abup_socket_buf));
		if(send(s_abup_task_data.socket, s_abup_socket_buf, strlen(s_abup_socket_buf),0) <= 0)
		{
			OSI_LOGI(0,"abup: socket send failed");
			abup_close_socket();
			osiThreadSleep (3000);
			abup_socket_retry++;
			if(abup_socket_retry>5)
			{
				abup_free_dns();
				abup_socket_retry=0;
				break;
			}
			else
				continue;
		}
		else{
			abup_socket_retry=0;
		}
		//osiThreadSleep (1000);
		OSI_LOGI(0,"abup: socket send success");

		while(1)
		{
			memset(s_abup_socket_buf,0, sizeof(s_abup_socket_buf));
			r = recv(s_abup_task_data.socket, s_abup_socket_buf, abup_socket_buff_len(),0);

			OSI_LOGI(0,"abup: read socket:%d", r);
			OSI_LOGXI(OSI_LOGPAR_S,0,"abup: recv buf:%s",s_abup_socket_buf);
			osiThreadSleep (100);

			if (r == abup_socket_buff_len())
			{
				if (abup_get_fota_status() != ABUP_FOTA_DL)
				{
					abup_BytePrint(s_abup_socket_buf, r);
				}

				OSI_LOGI(0,"abup: recv 333");
				abup_parse_http_data(s_abup_socket_buf, r);
				abup_set_continue_read();
			}
			else if (r > 0)
			{
				if (abup_get_fota_status() != ABUP_FOTA_DL)
				{
					if (strstr(s_abup_socket_buf, "0\r\n\r\n") == NULL)
					{
						r += recv(s_abup_task_data.socket, s_abup_socket_buf + r, abup_socket_buff_len() - r,0);
					}
					abup_BytePrint(s_abup_socket_buf, r);
				}
				OSI_LOGI(0,"abup: recv 111");
				abup_parse_http_data(s_abup_socket_buf, r);
				OSI_LOGI(0,"abup: recv 222");
				abup_reset_continue_read();
				break;
			}
			else
			{
				OSI_LOGI(0,"abup: warning! read null data.");
				abup_reset_continue_read();
				break;
			}
		}
	}
}

//task进程，只有启动fota才会调用
void abup_create_fota_task(void)
{
	abup_current_state = ABUP_FOTA_READY;
	OSI_LOGI (0,"abup_create_fota_task\n");
    Abup_thread_id = osiThreadCreate ("abup_fota_task", abup_fota_task, NULL, OSI_PRIORITY_NORMAL, ABUP_STACK_SIZE, ABUP_EVENT_QUEUE_SIZE);
}

//检查版本API，检查到新版本，自动下载完重启升级
//客户自行调用，开机不启动
void abup_check_version(void)
{
	abup_process_state=1;
	abup_create_fota_task();
}

//开机启动升级结果上报
//存在标志符，进行升级结果上报；不存在，退出
//如需开机不启动，app_start.c 去掉调用此函数的地方，自行调用
void abup_check_update_result(void)
{
	OSI_LOGI (0,"abup_check_update_result");
	if (abup_check_upgrade() != 0)
	{
		abup_process_state=0;
		abup_create_fota_task();
	}
	else
	{
		abup_check_version();
	}
}

//返回检查版本是否在运行
uint8_t abup_update_status(void)
{
	if(abup_get_fota_status() == ABUP_FOTA_DL)
	{
		return ABUP_FOTA_DLI;
	}

	return abup_current_state;
}

