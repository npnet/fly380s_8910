#ifndef __CTEL_OEM_CALLBACK_API_H__
#define __CTEL_OEM_CALLBACK_API_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "ctel_oem_type.h"

 /*!
  * Description:
  *UI实现  回调类型   处理ptt服务异步数据
  *
  * Rarameter:
  * result -1 失败 0 成功  data 各种数据
  *
  * Return:
  * none
  */
typedef void (*callback_Inform)(int result,CALLBACK_DATA *data );

 /*!
  * Description:
  *UI实现  回调类型   获取当前驻网状态
  *
  * Rarameter:
  *
  *
  * Return:
  *  1 成功 0 失败
  */
typedef int (*callback_GetGprsAttState)();

 /*!
  * Description:
  *UI实现  回调类型   获取SIM相关信息
  *
  * Rarameter:
  *imsi 存放imsi的buf  lenimsi imsi的长度
  *iccid 存放iccid的buf  leniccid iccid的长度
  *imei 存放imei的buf 	lenimei imei的长度
  *
  * Return:
  * none
  */
typedef void (*callback_GetSimInfo)(char *imsi,unsigned int *lenimsi ,char *iccid,unsigned int *leniccid,char *imei,unsigned int *lenimei);

 /*!
  * Description:
  *UI实现  回调类型 打开或者关闭音频采集
  *
  * Rarameter:
  *enable  0 关闭采集 1 打开采集
  *cb 采集回调接口 只有在打开情况下有效
  *
  * Return:
  *  1 成功 0 失败
  */
typedef int (*callback_Record)(ENABLE_TYPE enable,pcm_record_cb cb );

 /*!
  * Description:
  *UI实现  回调类型 打开或者关闭音频播放
  *
  * Rarameter:
  *enable  0 关闭播放 1 打开播放
  *
  * Return:
  *  1 成功 0 失败
  */
typedef int (*callback_Player)(ENABLE_TYPE enable);

 /*!
  * Description:
  *UI实现  回调类型 播放pcm数据
  *
  * Rarameter:
  *pcm 数据  len pcm数据长度
  *
  * Return:
  *  1 成功 0 失败
  */
typedef int (*callback_PlayPcm)( char *pcm , int len);

 /*!
  * Description:
  *UI实现  回调类型 播放tone音
  *
  * Rarameter:
  *enable  0 关闭播放 1 打开播放
  *type 播放类型
  *
  * Return:
  *  1 成功 0 失败
  */
typedef int (*callback_Tone)(ENABLE_TYPE enable,TONE_TYPE type);


 /*!
  * Description:
  *UI实现  回调类型 播放TTS
  *
  * Rarameter:
  *enable  0 关闭播放 1 打开播放
  *unicodestr 播放unicode字符串 大端
  *len unicode字符串长度
  *
  * Return:
  *  1 成功 0 失败
  */
typedef int (*callback_TTS)(ENABLE_TYPE enable,char *unicodestr , int len);

//注册回调总结构体
typedef struct ctel_register_st
{
    callback_Inform callbackInform;
    callback_GetGprsAttState callbackGetGprsAttState;
	callback_GetSimInfo callbackGetSimInfo;
	callback_Record callbackRecord;
    callback_Player callbackPlayer;
	callback_PlayPcm callbackPlayPcm;
	callback_Tone callbackTone;
	callback_TTS callbackTTS;
}CTEL_REGISTER_CB;

#ifdef __cplusplus
}
#endif

#endif // __CTEL_OEM_CALLBACK_API_H__