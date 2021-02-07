#ifndef __CTELPTT_H__
#define __CTELPTT_H__
#ifdef __cplusplus
extern "C" {
#endif
#include "sockets.h"
#include <osi_api.h>

#define SIM_INFO_LEN 33
#define UNICODE_LEN		45

typedef enum {
	CTEL_INFO = 1,
	CTEL_DEBUG,
	CTEL_WARN,
	CTEL_ERR,
	CTEL_CLOSE
}LOG_LEVEL;

typedef struct 
{
	unsigned char sim_imsi[SIM_INFO_LEN];
	unsigned char sim_iccid[SIM_INFO_LEN];
	unsigned char sim_phonenumber[SIM_INFO_LEN];
	unsigned char sim_imei[SIM_INFO_LEN];
	unsigned char sim_ipaddr[SIM_INFO_LEN];
}CTEL_SIM_INFO;  

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
	osiMutex_t * mtx;
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
	osiMutex_t * mtx;
}groupinfolist;

typedef enum callbacktype{
	TYPE_NO_SIM,
	TYPE_CANT_CONNECT_LTE,
	TYPE_DATA_CALL_FAILED,
	TYPE_PTT_LOGIN,
	TYPE_PTT_LOGOUT,  
	
	TYPE_PTT_LOGIN_USERPWD_ERR,
	TYPE_PTT_CONNECT_TIMEOUT,
	TYPE_PTT_REQUEST_MIC_SUCC,
	TYPE_PTT_REQUEST_MIC_FAILED,
	TYPE_PTT_MIC_LOST,
	
	TYPE_PTT_QUERY_GROUP_SUCC, 
	TYPE_PTT_QUERY_MEMBER_SUCC,
	TYPE_PTT_ENTER_GROUP,
	TYPE_PTT_LEAVE_GROUP,
	TYPE_PTT_RELEASE_MIC, 
	
	TYPE_PTT_MEMBER_GET_MIC,
	TYPE_PTT_MEMBER_LOST_MIC,
	TYPE_PTT_TCP_CLOSE,
	TYPE_PTT_SERVER_CLOSE, 
	TYPE_PTT_TMP_GROUP_CREAT,

	TYPE_PTT_TMP_GROUP_JOIN,
	TYPE_PTT_TMP_GROUP_QUIT,
	TYPE_RECV_MSG,
	TYPE_CONNECT_SERVER_FAILED,
	TYPE_PTT_INVALID_LOGININFO,

	TYPE_DNS_SERVER_CHANGED,

	TYPE_END
	
}CALLBACK_TYPE;

typedef struct {
	unsigned int availnum;
	unsigned int array[1024]; 
}MSGSTRUCT_ARRAY; 

typedef struct {
	CALLBACK_TYPE type;
	char *reason;
	void *msgstruct;
	unsigned int len;
}CALLBACK_DATA; 

typedef struct memberinfo
{
	unsigned int gid;
	unsigned int uid;
	char name[UNICODE_LEN];
}MEMBER_INFO; 	

typedef struct messageinfo
{
  unsigned int gid;
  unsigned int fromuid;
  char message[256];
  unsigned int timestamp;
}MESSAGE_INFO;

typedef struct pttuserinfo
{
	char serverinfo[64];	//当前服务器地址信息
  	unsigned int port;		//当前服务器端口
	unsigned int uid;		//当前用户id
	char name[UNICODE_LEN];  //当前用户名称 unicode编码
}PTTUSER_INFO; 

typedef struct serverchangedinfo
{
  char oldserverinfo[64];	//更改前服务器地址信息
  unsigned int oldport;		//更改前服务器端口
  char newserverinfo[64];	//更改后服务器地址信息
  unsigned int newport;		//更改后服务器端口
}SERVERCHANGED_INFO;		//服务器迁移信息

typedef struct tmpgroupinfo
{
  unsigned int gid;
  char callingname[UNICODE_LEN];
}TMPGROUP_INFO;

typedef void (*callback_factory)(int result,CALLBACK_DATA data ); 
typedef int (*HANDLE_CALLBACK)(const char * msg,int msglength);

struct ptt_info
{
    int socket; //socket of tcp cmd
	struct sockaddr_in udpserverAddr; //server addr
	int datasocket;  //socket of data
	unsigned int uid;
	int intercompriority;
	unsigned int ptt_callid;
	unsigned int defaultgroupid;
	char pttusername[UNICODE_LEN];
	unsigned char brequestingmic;
	unsigned char bquerylist;
	int role; //0 nothing 1 caller 2 callee
	int ownpriority;//�û�������Ȩ��
	int evrcsocket;
	struct sockaddr_in evrcpeer;
	long reconnecttime;
	groupinfolist intercomgrps;
	userinfolist currentgrpusers;
	unsigned int currentgrp;
	char currentgrpname[UNICODE_LEN];
	int bstart;   // 0 failed 1 ok 2 name/pwd err
	int intercomstatus; // 0 ���� 1 ���жԽ� 2 ���жԽ�
	char accountname[UNICODE_LEN];
	char passwd[64];
	char serverip[64];
	int servertcpport;
	int serverudpport;
	int operatecount;
	unsigned char ttsentergroup;
	HANDLE_CALLBACK login_ack;
	HANDLE_CALLBACK querygroup_ack;
	HANDLE_CALLBACK groupmember_userinfo;
	HANDLE_CALLBACK entergroup_ack;
	HANDLE_CALLBACK leavegroup_ack;
	HANDLE_CALLBACK requestmic_ack;
	HANDLE_CALLBACK releasemic_ack;
	HANDLE_CALLBACK logout_ack;
	HANDLE_CALLBACK reportLocation_ack;
	HANDLE_CALLBACK call_ack;
	HANDLE_CALLBACK lost_mic;
	HANDLE_CALLBACK kick_out;
	HANDLE_CALLBACK membergetmic;
	HANDLE_CALLBACK memberlostmic;
	HANDLE_CALLBACK tcp_disconnect;
	HANDLE_CALLBACK push_tmpcallstatus;
	HANDLE_CALLBACK push_tmpcallarrived;
	HANDLE_CALLBACK groupmsg_ack;
	HANDLE_CALLBACK grouplist_changed;
	HANDLE_CALLBACK reconfigured;
};

extern unsigned long ctel_get_version();
extern void ctel_set_log_level(int level);
extern void ctel_set_register_info(char *masterurl,unsigned int masterport ,char* slaveurl, unsigned int slaveport);
extern void ctel_stop_pttservice();
extern void ctel_start_pttservice();	
extern void ctel_init_callback_factory(callback_factory cb);
extern void ctel_set_headbeat_time(unsigned int tcpheadbeat ,unsigned int udpheadbeat );
extern int ctel_press_ppt_button();
extern void ctel_release_ppt_button();
extern int ctel_call_members(int callnumber[],int size);
extern void ctel_query_group();
extern void ctel_enter_group(int gid);
extern void ctel_query_member(int gid);
extern struct ptt_info * ctel_just_get_ptt_info();
extern void ctel_default_tone_play(int number);
extern int ctel_report_location_info(double longitude,double latitude);
extern CTEL_SIM_INFO * ctel_get_cur_sim_info();
extern void ctel_set_use_audio_type(int userv2);

#ifdef __cplusplus
}
#endif

#endif // __CTELPTT_H__
