#ifndef __CTEL_OEM_TYPE_H__
#define __CTEL_OEM_TYPE_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <osi_api.h>

#define UNICODE_LEN	45

typedef enum callbacktype{
	TYPE_NO_SIM,				//未检测到SIM卡
	TYPE_CANT_CONNECT_LTE,		//驻网失败
	TYPE_DATA_CALL_FAILED,		//拨号失败
	TYPE_PTT_LOGIN,				//登录成功
	TYPE_PTT_LOGOUT,  			//PTT退出登录，当前对讲不可用

	TYPE_PTT_LOGIN_USERPWD_ERR, //PTT用户名密码错误
	TYPE_PTT_CONNECT_TIMEOUT,	//PTT服务连接超时
	TYPE_PTT_REQUEST_MIC_SUCC,	//PTT请求MIC成功
	TYPE_PTT_REQUEST_MIC_FAILED, //PTT请求MIC失败
	TYPE_PTT_MIC_LOST, 			//PTT 当前请求的MIC权限丢失 原因在 char数组中

	TYPE_PTT_QUERY_GROUP_SUCC, 	 //PTT 查询组信息成功 组和成员信息在groupinfolist 中
	TYPE_PTT_QUERY_MEMBER_SUCC, //PTT查询组成员成功 组和成员信息在userinfolist 中
	TYPE_PTT_ENTER_GROUP,		 //PTT进入组 组的id在PTTGROUP_INFO中
	TYPE_PTT_LEAVE_GROUP,		//PTT退出组 组的id在PTTGROUP_INFO中
	TYPE_PTT_RELEASE_MIC, 		//PTT释放MIC权限

	TYPE_PTT_MEMBER_GET_MIC,	 //成员获得MIC权限  成员信息在 MEMBER_INFO中
	TYPE_PTT_MEMBER_LOST_MIC,	//成员失去MIC权限  成员信息在 MEMBER_INFO中
	TYPE_PTT_TCP_CLOSE,			//PTT连接关闭
	TYPE_PTT_SERVER_CLOSE, 		//PTT服务停止
	TYPE_PTT_TMP_GROUP_CREAT,	//PTT创建临时组 发起临时呼叫 临时成员在MSGSTRUCT_ARRAY中

	TYPE_PTT_TMP_GROUP_JOIN,	 //PTT进入临时组 组信息在TMPGROUP_INFO中
	TYPE_PTT_TMP_GROUP_QUIT,	//PTT退出临时组 组id 在TMPGROUP_INFO中
	TYPE_RECV_MSG,				 //接收到短消息 相关内容在 char 字符串数组中
	TYPE_CONNECT_SERVER_FAILED,	 //服务器连接失败
	TYPE_PTT_INVALID_LOGININFO,	//获取登录信息失败

	TYPE_DNS_SERVER_CHANGED,    //服务器地址更改 相关内容在 SERVERCHANGED_INFO 中

	TYPE_END

}CALLBACK_TYPE;

typedef enum
{
    DISABLE_CLOSE_STOP = 0,		//关闭、停止
    ENABLE_OPEN_START			//使能、打开、开始
} ENABLE_TYPE;			//使能类型

typedef enum
{
    PTT_START,		//开始PTT对讲
    PTT_STOP,		//结束PTT对讲
    ERROR,			//错误
    PLAY_START,		//播放PTT语音
    PLAY_STOP		//停止PTT语音
} TONE_TYPE;		//提示音类型

typedef enum {
	CTEL_INFO = 1, //基本信息级别
	CTEL_DEBUG,	 //调试信息级别
	CTEL_WARN,	 //警告信息级别
	CTEL_ERR,	 //错误信息级别
	CTEL_CLOSE  //关闭日志输出
}LOG_LEVEL; //日志级别


typedef struct {
	unsigned int availnum;   //有效的array数据长度
	unsigned int array[1024]; 	//int行数组buf
}MSGSTRUCT_ARRAY;

typedef struct {
	CALLBACK_TYPE type;		//回调的类型
	char *reason;			//原因描述
	void *msgstruct;		//回调的数据
	unsigned int len;		//数据长度
}CALLBACK_DATA;

typedef struct memberinfo
{
	unsigned int gid;	//组id
	unsigned int uid;	//成员id
	char name[UNICODE_LEN];	//组名称  unicode编码
}MEMBER_INFO;

typedef struct pttuserinfo
{
	char serverinfo[64];	//当前服务器地址信息
  	unsigned int port;		//当前服务器端口
	unsigned int uid;		//当前用户id
	char name[UNICODE_LEN];  //当前用户名称 unicode编码
}PTTUSER_INFO;

typedef struct pttgroupinfo
{
	unsigned int gid;		//组id
	char name[UNICODE_LEN];	//组名称 unicode编码
}PTTGROUP_INFO;

typedef struct messageinfo
{
  unsigned int gid;		//组id
  unsigned int fromuid;	//成员id
  char message[256];
  unsigned int timestamp;
}MESSAGE_INFO;

typedef struct tmpgroupinfo
{
  unsigned int gid;				//临时群组 组id
  char callingname[UNICODE_LEN];	//临时群组创建组名称 unicode编码
}TMPGROUP_INFO;

typedef struct serverchangedinfo
{
  char oldserverinfo[64];	//更改前服务器地址信息
  unsigned int oldport;		//更改前服务器端口
  char newserverinfo[64];	//更改后服务器地址信息
  unsigned int newport;		//更改后服务器端口
}SERVERCHANGED_INFO;		//服务器迁移信息

typedef struct userinfo
{
	unsigned int gid;
	unsigned int uid;
	char name[UNICODE_LEN];
	int online;
	int usr_attri;
	char shortnum[16];
	unsigned int priority;
	unsigned int utype;
	unsigned int did;
}userinfotype;

typedef struct linklist_userinfo
{
	userinfotype data;
	struct linklist_userinfo * next;
}linklistuserinfo;

typedef struct userinfo_list
{
	linklistuserinfo * head;
	linklistuserinfo * tail;
	osiMutex_t *mtx;
}userinfolist;

typedef struct groupinfo
{
	unsigned int groupid;
	int priority;
	char groupname[UNICODE_LEN];
}groupinfotype;

typedef struct linklist_groupinfo
{
	groupinfotype data;
	struct linklist_groupinfo * next;

}linklistgroupinfo;


typedef struct groupinfo_list
{
	linklistgroupinfo * head;
	linklistgroupinfo * tail;
	osiMutex_t *mtx;
}groupinfolist;

typedef int(*pcm_record_cb)(char *pcm, int len); //声音采集回调函数接口类型

#ifdef __cplusplus
}
#endif

#endif // __CTEL_OEM_TYPE_H__

