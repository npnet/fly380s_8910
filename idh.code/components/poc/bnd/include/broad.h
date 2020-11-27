#ifndef __BROAD_OPEN_H__
#define __BROAD_OPEN_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "type.h"

typedef enum AUDIO_STATE{
    AUDIO_IDLE,
    BND_SPEAK_START,
	BND_SPEAK_START_ACK,
    BND_SPEAKING,
    BND_SPEAK_STOP,
    BND_LISTEN_START,
    BND_LISTENING,
    BND_LISTEN_STOP,
    BND_TTS_START,
    BND_TTS_STOP,
    BND_TONE_START,
    BND_TONE_STOP,
    BND_REC_PLAY_START,
    BND_REC_PLAY_STOP,
}AUDIO_STATE;

typedef enum VOL_TYPE{
    BND_VOICE,
    BND_TTS,
    BND_TONE
}VOL_TYPE;
typedef void(*ui_notify_cb)(void* notify);
void broad_register_ui_notify_cb(ui_notify_cb cb);


/*
    FUN:login_state_cb 登录状态回调
    PARAM:
        online:USER_OFFLINE/USER_ONLINE
*/
typedef void(*login_state_cb)(int online);

/*
    FUN:join_group_cb 进组回调,触发源包括主动进组和被动进组
    PARAM:
        groupname:当前群组的名字
        gid：当前群组gid
*/
typedef void(*join_group_cb)(const char* groupname, bnd_gid_t gid);

/*
    FUN:audio_cb 音频的回调
    PARAM:
        state:当前语音状态:收听/讲话/TTS/TONE
        uid：当前操作用户uid，讲话/TTS/TONE时为0
        name:当前操作用户名字，讲话/TTS/TONE时为NULL
        flag:如果state为BND_LISTEN_START,flag==1表示本机可以打断对方讲话,flag==0表示本机不能打断对方讲话
*/
typedef void(*audio_cb)(AUDIO_STATE state, bnd_uid_t uid, const char* name, int flag);

/*
    FUN:callmember_cb 单呼的回调
    PARAM:
        ret:1->成功，0->失败，2->超时解散
*/
typedef void(*callmember_cb)(int ret);

/*
    FUN:listupdate_cb 数据变化的回调
    PARAM:
        flag:1：群组列表变化， 2：成员列表变化
*/
typedef void(*listupdate_cb)(int flag);

/*
    FUN:member_change_cb 数据变化的回调
    PARAM:
        flag:1 离组
        gid:变化的gid
        num:个数
        uids:uid集合
*/
typedef void(*member_change_cb)(int flag, bnd_gid_t gid, int nun, bnd_uid_t* uids);

/*
    FUN:error_cb 异常错误信息的回调
    PARAM:
        info: 异常信息
*/
typedef void(*error_cb)(const char* info);
/*
    FUN:poc_at_cb AT透传回调，用于接收POC处理后的返回AT数据
    PARAM:
        at: POC处理后的返回AT数据
*/
typedef void(*poc_at_cb)(char* at);
/*
    FUN:location_cb GPS定位信息变化通知
    PARAM:
        on: 定位开关, 1是开启,0是关闭
        interval: 定位上报间隔,单位秒
*/
typedef void(*location_cb)(unsigned char on, int interval);
typedef void(*upgrade_cb)(int ret);
typedef void(*reminder_cb)(char* reminder, int day);

int broad_set_thread_priority(int priority);
void broad_set_notify_mode(int flags);
void broad_init(void); 
void broad_free(void);
void broad_log(boolean enable);
int broad_login(login_state_cb cb);
int broad_logout(void);
int broad_joingroup(bnd_gid_t gid);
int broad_leavegroup(void);
//1:start 0:stop
int broad_speak(boolean start);
int broad_calluser(const bnd_uid_t uid ,callmember_cb cb);
int broad_callusers( const bnd_uid_t* uids, int num, callmember_cb cb );

//按索引查询群组信息
int broad_group_getbyindex(int index,bnd_group_t* dst);

//如果gid传入0,可查询当前群组信息
int broad_group_getbyid(bnd_gid_t gid,bnd_group_t* dst);

//如果uid传入0,可查询当前登录用户信息
int broad_member_getbyid(bnd_uid_t uid,bnd_member_t* dst);
int broad_get_loginstate(void);
int broad_get_groupcount(void);

/*
    FUN：broad_get_grouplist
    PARAM：
        dst和dst_size为调用者的数组空间及数组大小
        index_begin为查询的索引起始值
        count为计划查询的个数
    RETURN：
        实际查询到的个数
*/
int broad_get_grouplist(bnd_group_t* dst, int dst_size, int index_begin, int count);

const int broad_get_membercount(bnd_gid_t gid );
/*
    FUN：broad_get_memberlist
    PARAM：
        gid为群组id
        dst和dst_size为调用者的数组空间及数组大小
        index_begin为查询的索引起始值
        count为计划查询的个数
    RETURN：
        实际查询到的个数
*/
const int broad_get_memberlist(bnd_gid_t gid, bnd_member_t* dst, int dst_size, int index_begin, int count);

AUDIO_STATE broad_get_audiostate(void);

//boolean interrupt : true--interrupt current tts playing 
int broad_play_tts(const char* tts, boolean interrupt);

int broad_send_ping(void);

//lat：纬度， lon： 经度， time： 定位时间
int broad_send_gpsinfo(double lon,double lat,bnd_time_t time);
void broad_get_version(char* ver);
void broad_get_upgrade_version(char* ver);
int broad_current_zone_time(bnd_time_t* now, int zone);

//success return 0, failed return -1
int broad_set_tts_enable(boolean enable);
int broad_set_vol(VOL_TYPE type, int vol);
int broad_get_vol(VOL_TYPE type);
//获取帐号可用天数
int broad_get_account_day(void);

//mode:1->open; 0->close; 2->clear all history
int broad_set_rec_audio_mode(unsigned char mode);

int broad_get_rec_audio_count(void);

//index:录音列表索引, name:讲话的用户名utf8格式, delay:语音时长(单位秒), time:录制时间
int broad_get_rec_audio_info(int index, char* name, unsigned char* delay, bnd_time_t* time);

//index:录音列表索引
int broad_play_rec_audio(int index);

int broad_stop_play_rec_audio(void);

//notify
void broad_register_audio_cb(audio_cb cb);
void broad_register_join_group_cb(join_group_cb cb);
void broad_register_listupdate_cb(listupdate_cb cb);
void broad_register_member_change_cb(member_change_cb cb);
void broad_register_error_cb(error_cb cb);
void broad_register_location_cb(location_cb cb);
void broad_register_upgrade_cb(upgrade_cb cb);
void broad_register_reminder_cb(reminder_cb cb);
int broad_read_custom(BND_CUSTOM_TYPE type, const char* value);
int broad_write_custom(BND_CUSTOM_TYPE type, const char* value);

void broad_set_auto_end_temp_call_time(unsigned int t);
void broad_set_is_destroy_temp_call(boolean flag);
int broad_set_poc_uart_device(int open); //默认uart设备是打开的，如需关闭请设置为0
//AT 透传接口
int broad_send_at(char* at); //发送
void broad_register_poc_at_cb(poc_at_cb cb);//注册接收回调
int broad_set_solution(char *solution);
int broad_set_solution_version(char *version);
int broad_set_productInfo(char *productInfo);
int broad_set_manufacturer(char *manufacturer);
int broad_request_upgrade();
int broad_get_init_status();
#ifdef __cplusplus
}
#endif

#endif

