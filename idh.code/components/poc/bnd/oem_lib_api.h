#ifndef __OEM_LIB__
#define __OEM_LIB__

#ifdef __cplusplus
extern "C" {
#endif
#include "lwip/sockets.h"
#include "osi_api.h"

bool AIR_Log_Debug(const char *format, ...); //日志打印

/* 锁和信号量相关接口*/
//typedef int类型需要补充
typedef int AIR_OSI_Mutex_t;      //锁标识
typedef int AIR_OSI_Semaphore_t;  //信号量标识
AIR_OSI_Mutex_t* AIR_OSI_MutexCreate(void); //创建锁
bool AIR_OSI_MutexDelete(AIR_OSI_Mutex_t* mutex);//删除锁
bool AIR_OSI_MutexTryLock(AIR_OSI_Mutex_t* mutex, unsigned int ms_timeout); //上锁
bool AIR_OSI_MutexUnlock(AIR_OSI_Mutex_t* mutex); //释放锁

AIR_OSI_Semaphore_t* AIR_OSI_SemaphoreCreate(int a, int b); //创建信号量
bool AIR_SemaphoreDelete(AIR_OSI_Semaphore_t* cond); //删除信号量
bool AIR_OSI_SemaphoreRelease(AIR_OSI_Semaphore_t* cond);//释放信号量
bool AIR_OSI_SemaphoreAcquire(AIR_OSI_Semaphore_t* cond);//等待信号量
bool AIR_OSI_SemaphoreTryAcquire(AIR_OSI_Semaphore_t* cond, unsigned int ms_timeout); //等待信号量到超时

/*文件操作*/
typedef enum { //文件操作标志
	AIR_FS_O_RDWR,
	AIR_FS_O_RDONLY,
	AIR_FS_O_WRONLY,
	AIR_FS_O_APPEND,
}FILE_FLAG;
typedef enum{
	AIR_FS_SEEK_SET,
}AIR_FILE_SEEK_TYPE;  //文件流程动作标识
int AIR_FS_Open(char* path, int flags, int a); //打开文件
bool AIR_FS_Close(int file); //关闭文件
int AIR_FS_Read(int file, char* data, int size);//读文件
int AIR_FS_Write(int file, char* data, int size); //写文件
bool AIR_FS_Seek(int file, int flag, int type); //操作文件流指针

/*网络接口*/
typedef struct AIR_SOCK_IP_ADDR_t{
	uint32_t addr;
}AIR_SOCK_IP_ADDR_t; //网络地址结构体

int AIR_SOCK_Error(); //获取网络错误标识
int AIR_SOCK_Gethostbyname( char* pHostname, AIR_SOCK_IP_ADDR_t* pAddr); //域名解析
char* AIR_SOCK_IPAddr_ntoa(AIR_SOCK_IP_ADDR_t* pIpAddr); //网络地址转字符IP
uint32_t AIR_SOCK_inet_addr(char * pIp); //字符串IP转int型

/*管道相关*/
typedef int AIR_OSI_Pipe_t;
AIR_OSI_Pipe_t* AIR_OSI_PipeCreate(); //创建管道
bool AIR_OSI_PipeDelete(AIR_OSI_Pipe_t* pipe); //删除管道
int AIR_OSI_PipeWrite(AIR_OSI_Pipe_t* pipe,void*buf,int nbyte);//向管道写数据
int AIR_OSI_PipeRead(AIR_OSI_Pipe_t* pipe,void*buf,int nbyte); //从管道读数据

/*线程相关*/
typedef enum _AIR_OSI_ThreadPriority //线程级别
{
    AIR_OSI_PRIORITY_IDLE,                        /*!< priority idle **/
    AIR_OSI_PRIORITY_LOW,                         /*!< low priority **/
    AIR_OSI_PRIORITY_BELOW_NORMAL,               /*!< below normal **/
    AIR_OSI_PRIORITY_NORMAL,                     /*!< normal **/
    AIR_OSI_PRIORITY_ABOVE_NORMAL,               /*!< above normal **/
    AIR_OSI_PRIORITY_HIGH,                       /*!< high **/
    AIR_OSI_PRIORITY_REALTIME,                   /*!< realtime **/
    AIR_OSI_PRIORITY_HISR,                       /*!<hisr **/
} AIR_OSI_ThreadPriority_t;
typedef unsigned int AIR_OSI_Thread_t;
typedef void (*AIR_OSI_Callback_t)(void *ctx);
bool AIR_OSI_ThreadSleep(int ms); //延时函数
AIR_OSI_Thread_t AIR_OSI_ThreadCurrent(); //获取当前线程标识
AIR_OSI_Thread_t AIR_OSI_ThreadCreate(const char *name,      //创建线程
                                     AIR_OSI_Callback_t func,
                                     void *argument,
                                     uint32_t        priority,
                                     uint32_t        stack_size,
                                     uint32_t        event_count);
bool AIR_OSI_ThreadExit();  //标识线程退出，释放资源
bool AIR_OSI_ThreadSuspend(AIR_OSI_Thread_t tid); //暂停线程，暂未使用可空实现
bool AIR_OSI_ThreadResume(AIR_OSI_Thread_t tid);  //恢复线程，暂未使用可空实现
unsigned int AIR_OSI_UpTime(); //获取开机后的ms
unsigned int AIR_OSI_EpochSecond(); //获取时间戳


int lib_oem_send_uart(const char* data, int size);
extern int lib_oem_uart_cb(char* data, int size);

/*oem lib extern functions*/
extern void lib_oem_tts_play_end_cb();
extern int lib_oem_record_cb(unsigned char* data, unsigned int length);


/*tts functions*/
/*
* Function: lib_oem_tts_play
*
* Description:
* push the tts data
*
* Rarameter:
* text:unicode data
*
* Return:
* none
*/

int lib_oem_tts_play(char *text, int len);

/*
* Function: lib_oem_tts_stop
*
* Description:
* stop the tts
*
* Rarameter:
* none
*
* Return:
* none
*/

int lib_oem_tts_stop(void);

/*
* Function: lib_oem_tts_status
*
* Description:
* get the tts status
*
* Rarameter:
* none
*
* Return:
* 0-not work
* 1-working
*/

int lib_oem_tts_status(void);

/*
* Function: lib_oem_start_record
*
* Description:
* start record for ptt or open device for record
*
* Rarameter:
* none
*
* Return:
* none
*/

int lib_oem_start_record(void);

/*
* Function: lib_oem_stop_record
*
* Description:
* stop record or close device for record
*
* Rarameter:
* none
*
* Return:
* none
*/

int lib_oem_stop_record(void);

/*
* Function: lib_oem_start_play
*
* Description:
* start play pcm, or open device for play
*
* Rarameter:
* none
*
* Return:
* none
*/

int lib_oem_start_play(void);

/*
* Function: lib_oem_stop_play
*
* Description:
* stop play pcm, or close device for play
*
* Rarameter:
* none
*
* Return:
* none
*/

int lib_oem_stop_play(void);

/*
* Function: lib_oem_play
*
* Description:
* push  pcm data
*
* Rarameter:
* data:pcm data
* length: the length of data
*
* Return:
* receive data length
*/

int lib_oem_play(const char* data, int length);

/*
* Function: lib_oem_play_tone
*
* Description:
* Play tone
*
* Rarameter:
* type:
* [0]--start ptt
* [1]--stop ptt
* [2]--error
* [3]--start play
* [4]--stop play
*
* Return:
* none
*/
int lib_oem_play_tone(int type);

/* network functions*/

/*
* Function: lib_oem_socket_set_apn
*
* Description:
* set the new apn
*
* Rarameter:
* apn
*
* Return:
* none
*/

void lib_oem_socket_set_apn(char *apn);

/*
* Function: lib_oem_socket_net_open
*
* Description:
* Open the network data
*
* Rarameter:
* none
*
* Return:
* 0-FAIL
* 1-SUCCESS
*/

int  lib_oem_socket_net_open(void);

/*
* Function: lib_oem_socket_get_net_status
*
* Description:
* Get network data status
*
* Parameter:
* none
*
* Return:
* 0-FAIL
* 1-SUCCESS
*/

int lib_oem_socket_get_net_status(void);

/*
* Function: lib_oem_socket_get_network
*
* Description:
* Get network status
*
* Rarameter:
* none
*
* Return:
* -1-FAIL
* 0-
* 1-SUCCESS
*/

int  lib_oem_socket_get_network(void);

/*
* Function: lib_oem_get_rssi
*
* Description:
* Get network signal strength
*
* Rarameter:
* none
*
* Return:
* rssi
*
*/

int  lib_oem_get_rssi(void);

/*
* Function: lib_oem_net_close
*
* Description:
* Close the network
*
* Rarameter:
* none
*
* Return:
* 0-SUCCESS
* 1-FAILURE
*
*/

int  lib_oem_net_close(void);

/*
* Function: lib_oem_play_open_pcm
*
* Description:
* Initialize pcm for TTS
*
* Input:
* none
*
* Return:
* 0-FAILURE
* 1-SUCCESS
*
*/

int lib_oem_play_open_pcm();

/*sim*/

/*
* Function: lib_oem_is_sim_present
*
* Description:
* Check the status of the SIM card
*
* Input:
* none
*
* Return:
* 0-SIMcard is absent
* 1-SIM card is present
*
*/
int lib_oem_is_sim_present(void);

/*========================================================================================
FUNCTION:
lib_oem_get_meid

DESCRIPTION:
get meid

PARAMETERS:
data:meid

RETURN:
[0]SUCCESS
[-1]FAILURE

=========================================================================================*/

int lib_oem_get_meid(char* data);

/*========================================================================================
FUNCTION:
lib_oem_get_imei

DESCRIPTION:
get imei

PARAMETERS:
data:imei

RETURN:
[0]SUCCESS
[-1]FAILURE

=========================================================================================*/

int lib_oem_get_imei(char* data);

/*========================================================================================
FUNCTION:
lib_oem_get_imsi

DESCRIPTION:
get imsi

PARAMETERS:
data:imsi

RETURN:
[0]SUCCESS
[-1]FAILURE

=========================================================================================*/

int lib_oem_get_imsi(char* data);

/*========================================================================================
FUNCTION:
lib_oem_get_iccid

DESCRIPTION:
get iccid

PARAMETERS:
data:iccid

RETURN:
[0]SUCCESS
[-1]FAILURE

=========================================================================================*/

int lib_oem_get_iccid(char* data);


/*========================================================================================
FUNCTION:
lib_oem_get_system_version

DESCRIPTION:
get system version

RETURN:
version
=========================================================================================*/
char* lib_oem_get_system_version(void);

/*========================================================================================
FUNCTION:
lib_oem_get_other_version

DESCRIPTION:
get lua version

RETURN:
version
=========================================================================================*/
char* lib_oem_get_other_version(void);

/**远程升级初始化
*@return 0:  表示成功
*   <0：  表示失败
**/
int lib_ota_init(void);

/**远程升级下载
*@param  data:    下载固件包数据
*@param  len:    下载固件包长度
*@param  total:    固件包总大小
*@return 0:  表示成功
*   <0：  表示失败
**/
int lib_ota_process(char* data, unsigned int len, unsigned int total);

/**远程升级结束且校验
*@return 0:  表示成功, 成功后模块自动重启升级
*   <0：  表示失败
**/
int lib_ota_done(void);
#ifdef __cplusplus
}
#endif

#endif

