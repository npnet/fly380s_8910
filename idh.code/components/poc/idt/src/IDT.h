#ifndef _IDT_H_
#define _IDT_H_

#include <string.h>
#include <stdio.h>
#ifdef  __cplusplus
extern  "C"
{
#endif

#define IDT

#ifdef WINDOWS
#ifndef IDT_API
#ifdef IDT_EXPORTS
#define IDT_API __declspec(dllexport)
#else
#define IDT_API __declspec(dllimport)
#endif
#endif
#endif

//IDT版本1.0
#define IDT_VerMajor        1
#define TDT_VerMinor        0

#include "pub_typedef.h"
#include "pubsrv_def.h"
#include "pubmed_def.h"

//用户数据
typedef struct _UData_s
{
    UCHAR       ucNum[32];          //号码
    UCHAR       ucName[64];         //名字
    UCHAR       ucPwd[64];          //密码
    UCHAR       ucType;             //类型
    UCHAR       ucAttr;             //属性
    UCHAR       ucStatus;           //状态
    UCHAR       ucPriority;         //优先级
    int         iConCurrent;        //并发用户数
    UCHAR       ucIP[32];           //IP地址,字符串形式
    int         iPort;              //端口号
    UCHAR       ucAddr[128];        //用户地址
    UCHAR       ucContact[128];     //联系方式
    UCHAR       ucDesc[128];        //描述
    time_t      CTime;              //创建时间
    time_t      VTime;              //有效时间
    
    UCHAR       ucCamType;          //类型                  UT_TYPE_HK
    UC_32       gwCamNum;           //摄像头网关号码
    UC_64       ucCamIp;            //摄像头地址,可能是IP地址,或者域名
    USHORT      usCamPort;          //端口号
    UC_64       CamName;            //用户名
    UC_64       CamPwd;             //密码
    UCHAR       ucChanNum;          //摄像头通道个数
    UCHAR       ucCamCliType;       //CLITYPE
    UC_32       CamGUID_V;          //GUID
    UC_256      ucWorkInfo;         //WorkInfo
    USERGINFO_s stFGInfo;           //父组信息
    UC_256      ucUserProxy;        //用户代理信息      格式:0,无效
                                    //                  功能块号,代理号码,密码,服务器IP地址:端口号,是否代理注册
                                    //                  例如:
                                    //                  SIP*8000*8000*222.42.245.76:5080*1          代理SIP注册到222.42.245.76:5080
                                    //                  TAP*8000*8000*124.160.11.21:10000*1         代理TAP注册到222.42.245.76:5080
                                    //                  USER*8000*8000*8001*0                       对方是用户接入到自己,代理号码是8000,呼出时,自己号码是8001
                                    //                  自己作为28181上级时,USER*对方接入号码*密码*自己号码*编码格式(0:UTF8,1:GBK)
                                    
    int         iDataRole;          //数据权限
    int         iMenuRole;          //菜单权限
    
    UC_128      ucDeptNum;          //部门号码
    UC_128      ucID;               //身份证
    UC_128      ucWorkID;           //工作证
    UC_128      ucWorkUnit;         //工作单位
    UC_128      ucTitle;            //职务
    UC_128      ucCarID;            //车牌
    UC_128      ucTel;              //电话号码
    UC_256      ucOther;            //其他
    
}UData_s;

//组数据
typedef struct _GData_s
{
    UCHAR           ucNum[32];                  //组号码
    UCHAR           ucName[64];                 //组名字
    UCHAR           ucAGNum[32];                //关联组,摄像头组
    UCHAR           ucPriority;                 //优先级
    DWORD           dwNum;                      //用户个数
    GROUP_MEMBER_s  member[GROUP_MAX_MEMBER];   //组成员
    QUERY_EXT_s     query;                      //查询条件
}GData_s;

//--------------------------------------------------------------------------------
//      用户状态指示
//  输入:
//      status:         当前用户状态
//      usCause:        原因值
//  返回:
//      无
//  注意:
//      由IDT.dll调用,告诉用户状态发生变化
//--------------------------------------------------------------------------------
typedef void (*IDT_StatusInd)(int status, unsigned short usCause);
//--------------------------------------------------------------------------------
//      组信息指示
//  输入:
//      pGInfo:         组信息
//  返回:
//      无
//  注意:
//      由IDT.dll调用,告诉用户状态发生变化
//--------------------------------------------------------------------------------
typedef void (*IDT_GInfoInd)(USERGINFO_s *pGInfo);
//--------------------------------------------------------------------------------
//      启动呼出
//  输入:
//      cPeerNum:       对方号码
//      SrvType:        业务类型
//      pAttr:          媒体属性
//      pUsrCtx:        用户上下文
//      pcPwd:          密码,创建或进入会场的密码
//      ucCallOut:      服务器是否直接呼出,0不呼出,1直接呼出
//      ucDelG:         会议结束后,是否删除组,0不删除,1删除
//      ucAutoMic:      是否自动话权,0不是,1是自动话权
//      pcUserMark:     用户标志,字符串格式,最大256字节
//  返回:
//      -1:             失败
//      else:           呼叫标识
//  注意:
//      如果是组呼(SRV_TYPE_CONF, 语音发送为1,语音接收为0, 视频未定义,或者与语音相同)
//      1.pcPeerNum为组号码
//      2.pAttr中,ucAudioSend为1,其余为0
//      如果是会议:
//      1.发起会议(SRV_TYPE_CONF, 语音发送为1,语音接收为1)
//          a)被叫号码可以为空,或者用户号码/组号码
//          b)pcPwd为会议密码
//          c)在CallPeerAnswer时,带回会议的内部号码,为交换机产生的呼叫标识
//      2.加入会议(SRV_TYPE_CONF_JOIN,ucAudioRecv=1,ucVideoRecv=1)
//          a)pcPeerNum为1中的c
//          b)pcPwd为1中的b
//--------------------------------------------------------------------------------
IDT_API int IDT_CallMakeOut(char* pcPeerNum, SRV_TYPE_e SrvType, MEDIAATTR_s *pAttr, void *pUsrCtx,
    char *pcPwd = NULL, UCHAR ucCallOut = 1, UCHAR ucDelG = 0, UCHAR ucAutoMic = 0, char *pcUserMark = NULL);
//IDT_API int IDT_CallMakeOut(char* pcPeerNum, SRV_TYPE_e SrvType, MEDIAATTR_s *pAttr, void *pUsrCtx,
//    UCHAR ucCallOut = 1, UCHAR ucDelG = 0);
//--------------------------------------------------------------------------------
//      呼出应答
//  输入:
//      pUsrCtx:        用户上下文
//      pcPeerNum:      对方应答的号码,有可能与被叫号码不同
//      pcPeerName:     对方应答的用户名
//      SrvType:        业务类型,实际的业务类型,可能与MakeOut不同
//      pcUserMark:     用户标识
//      pcUserCallRef:  用户使用的呼叫参考号,主叫是自己,被叫是主叫,查看是对端
//  返回:
//      0:              成功
//      -1:             失败
//  注意:
//      由IDT.dll调用,告诉用户对方应答
//--------------------------------------------------------------------------------
typedef int (*IDT_CallPeerAnswer)(void *pUsrCtx, char *pcPeerNum, char *pcPeerName, SRV_TYPE_e SrvType, char *pcUserMark, char *pcUserCallRef);
//--------------------------------------------------------------------------------
//      收到呼入
//  输入:
//      ID:             IDT的呼叫ID
//      pcMyNum:        自己号码
//      pcPeerNum:      对方号码
//      pcPeerName:     对方名字
//      SrvType:        业务类型
//      pAttr:          媒体属性
//      pExtInfo:       附加信息
//      pcUserMark:     用户标识
//      pcUserCallRef:  用户使用的呼叫参考号,主叫是自己,被叫是主叫,查看是对端
//  返回:
//      0:              成功
//      -1:             失败
//  注意:
//      由IDT.dll调用,告诉用户有呼叫进入
//--------------------------------------------------------------------------------
typedef int (*IDT_CallIn)(int ID, char *pcMyNum, char *pcPeerNum, char *pcPeerName, SRV_TYPE_e SrvType, MEDIAATTR_s *pAttr, void *pExtInfo, char *pcUserMark, char *pcUserCallRef);
//--------------------------------------------------------------------------------
//      呼入应答
//  输入:
//      ID:             IDT的呼叫ID
//      pAttr:          媒体属性
//      pUsrCtx:        用户上下文
//      pSdp:           SDP,如果有SDP,一定是全局变量,不能修改!!!!!!!!IDT没有拷贝内容,直接传指针到CC的处理
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
IDT_API int IDT_CallAnswer(int ID, MEDIAATTR_s *pAttr, void *pUsrCtx, void *pSdp = NULL);
//--------------------------------------------------------------------------------
//      呼叫释放
//  输入:
//      ID:             IDT的呼叫ID
//      pUsrCtx:        用户上下文,通常不用这个,但启动主叫后,没有收到主叫应答
//      uiCause:        释放原因值
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
IDT_API int IDT_CallRel(int ID, void *pUsrCtx, UINT uiCause);
//--------------------------------------------------------------------------------
//      对方或IDT内部释放呼叫
//  输入:
//      ID:             IDT的呼叫ID,通常不使用这个,但可能启动被叫后,用户还没有应答,就释放了
//      pUsrCtx:        用户上下文
//      uiCause:        释放原因值
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
typedef int (*IDT_CallRelInd)(int ID, void *pUsrCtx, UINT uiCause);
//--------------------------------------------------------------------------------
//      通话状态下发送号码
//  输入:
//      ID:             IDT的呼叫ID
//      cNum:           发送的号码,ASC字符串形式,有效值为'0'~'9','*','#','A'~'D',16(FLASH)
//      dwStreamId:     流号
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
IDT_API int IDT_CallSendNum(int ID, char* cNum, DWORD dwStreamId = 0);
//--------------------------------------------------------------------------------
//      通话状态下收到对方发送的号码
//  输入:
//      pUsrCtx:        用户上下文
//      dwStreamId:     流号
//      cNum:           收到的号码,ASC字符形式,有效值为'0'~'9','*','#','A'~'D',16(FLASH)
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
typedef int (*IDT_CallRecvNum)(void *pUsrCtx, DWORD dwStreamId, char cNum);

//--------------------------------------------------------------------------------
//      话权控制
//  输入:
//      ID:             IDT的呼叫ID
//      bWant:          是否期望话权
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
IDT_API int IDT_CallMicCtrl(int ID, bool bWant);
//话权请求
#define IDT_CallMicWant(_ID) IDT_CallMicCtrl(_ID, true)
//话权释放
#define IDT_CallMicRel(_ID) IDT_CallMicCtrl(_ID, false)

//--------------------------------------------------------------------------------
//      话权指示
//  输入:
//      pUsrCtx:        用户上下文
//      uiInd:          指示值:0话权被释放,1获得话权,与媒体属性相同
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
typedef int (*IDT_CallMicInd)(void *pUsrCtx, UINT uiInd);

//--------------------------------------------------------------------------------
//      讲话方提示
//  输入:
//      pUsrCtx:        用户上下文
//      pcNum:          讲话方号码
//      pcName:         讲话方名字
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
typedef int (*IDT_CallTalkingIDInd)(void *pUsrCtx, char *pcNum, char *pcName);

//--------------------------------------------------------------------------------
//      媒体修改
//  输入:
//      ID:             IDT的呼叫ID
//      pAttr:          媒体属性
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
IDT_API int IDT_CallModify(int ID, MEDIAATTR_s *pAttr);
//--------------------------------------------------------------------------------
//      呼叫中添加/删除用户
//  输入:
//      pcCallRef:      呼叫参考号
//      pcNum:          用户号码
//      ucOp:           0删除,1添加
//      pAttr:          媒体属性
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
IDT_API int IDT_CallUserCtrl(char *pcCallRef, char *pcNum, UCHAR ucOp, MEDIAATTR_s *pAttr);
//--------------------------------------------------------------------------------
//      媒体流控制
//  输入:
//      ID:             IDT的呼叫ID
//      pcNum:          对端号码
//      ucType:         1语音,2视频,同SDP_MEDIA_AUDIO定义
//      ucOp:           0关闭,1打开
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
IDT_API int IDT_CallStreamCtrl(int ID, char *pcNum, UCHAR ucType, UCHAR ucOp);
//--------------------------------------------------------------------------------
//      查询会场状态
//  输入:
//      pcCallRef:      呼叫参考号
//      dwSn:           操作序号
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
IDT_API int IDT_CallConfStatusReq(char *pcCallRef, DWORD dwSn);
//--------------------------------------------------------------------------------
//      查询会场状态响应
//  输入:
//      dwSn:           操作序号
//      ucAutoMic:      自由发言状态,0不自由发言,1自由发言
//      pData:          会场中用户
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
typedef int (*IDT_CallConfStatusRsp)(DWORD dwSn, UCHAR ucAutoMic, GData_s *pData);

//--------------------------------------------------------------------------------
//      设置音频编码器
//  输入:
//      pUsrCtx:        用户上下文
//      dwStreamId:     流号
//      pcNum:          号码
//      pAInfo:         音频信息
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
#ifdef ANDROID
typedef int (*IDT_CallSetAudioCodec)(void *pUsrCtx, int ID, DWORD dwStreamId, char *pcNum, SDP_AINFO_s* pAInfo);
#else
typedef int (*IDT_CallSetAudioCodec)(void *pUsrCtx, DWORD dwStreamId, char *pcNum, SDP_AINFO_s* pAInfo);
#endif
//--------------------------------------------------------------------------------
//      发送语音数据
//  输入:
//      ID:             IDT的呼叫ID
//      dwStreamId:     流号
//      ucCodec:        CODEC
//      pucBuf:         数据
//      iLen:           数据长度
//      dwTsOfs:        时戳空洞长度
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
IDT_API int IDT_CallSendAuidoData(int ID, DWORD dwStreamId, UCHAR ucCodec, UCHAR *pucBuf, int iLen, DWORD dwTsOfs = 0);
//--------------------------------------------------------------------------------
//      收到语音数据
//  输入:
//      pUsrCtx:        用户上下文
//      dwStreamId:     StreamId
//      ucCodec:        CODEC
//      pucBuf:         数据
//      iLen:           数据长度
//      dwTsOfs:        时戳空洞
//      dwTsLen:        数据块时戳长度
//      dwTs:           起始时戳
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
typedef int (*IDT_CallRecvAudioData)(void *pUsrCtx, DWORD dwStreamId, UCHAR ucCodec, UCHAR *pucBuf, int iLen, DWORD dwTsOfs, DWORD dwTsLen, DWORD dwTs);

//--------------------------------------------------------------------------------
//      设置视频编码器
//  输入:
//      pUsrCtx:        用户上下文
//      dwStreamId:     流号
//      pcNum:          号码
//      pVInfo:         视频信息
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
#ifdef ANDROID
typedef int (*IDT_CallSetVideoCodec)(void *pUsrCtx, int ID, DWORD dwStreamId, char *pcNum, SDP_VINFO_s *pVInfo);
#else
typedef int (*IDT_CallSetVideoCodec)(void *pUsrCtx, DWORD dwStreamId, char *pcNum, SDP_VINFO_s *pVInfo);
#endif
//--------------------------------------------------------------------------------
//      发送视频数据
//  输入:
//      ID:             IDT的呼叫ID
//      dwStreamId:     流号
//      ucCodec:        CODEC,SDP_PT_MY_YUV,或者SDP_PT_MY_H264
//      pucBuf:         数据
//      iLen:           数据长度
//      IFrame:         I帧标识
//      iRotate:        旋转角度,顺时针方向,0:不旋转,90,180,270
//      dwTsOfs:        时戳空洞长度
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
IDT_API int IDT_CallSendVideoData(int ID, DWORD dwStreamId, UCHAR ucCodec, UCHAR *pucBuf, int iLen, int iWidth, int iHeight, UCHAR IFrame, int iRotate = 0, DWORD dwTsOfs = 0);
//--------------------------------------------------------------------------------
//      收到视频数据
//  输入:
//      pUsrCtx:        用户上下文
//      dwStreamId:     StreamId
//      ucCodec:        CODEC
//      pucBuf:         数据
//      iLen:           数据长度
//      dwTsOfs:        时戳空洞
//      IFrame:         I帧标识
//      ucCom:          是否完整
//      dwTs:           起始时戳
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
#ifdef WINDOWS
typedef int (*IDT_CallRecvVideoData)(void *pUsrCtx, DWORD dwStreamId, UCHAR ucCodec, UCHAR *pucBuf, int iLen, DWORD dwTsOfs, UCHAR IFrame, UCHAR ucCom, DWORD dwTs);
#endif
#ifdef ANDROID
typedef int (*IDT_CallRecvVideoData)(void *pUsrCtx, DWORD dwStreamId, UCHAR ucCodec, UCHAR *pucBuf, int iLen, DWORD dwTsOfs, UCHAR IFrame, UCHAR ucCom, int iWidth, int iHeight, DWORD dwTs);
#endif
#ifdef FREERTOS
typedef int (*IDT_CallRecvVideoData)(void *pUsrCtx, DWORD dwStreamId, UCHAR ucCodec, UCHAR *pucBuf, int iLen, DWORD dwTsOfs, UCHAR IFrame, UCHAR ucCom, int iWidth, int iHeight, DWORD dwTs);
#endif


//--------------------------------------------------------------------------------
//      媒体流统计数据
//  输入:
//      pUsrCtx:        用户上下文
//      ucType:         语音还是视频,1语音,2视频,SDP_MEDIA_AUDIO
//      uiRxBytes:      当前统计段接收的所有字节数
//      uiRxUsrBytes:   当前统计段用户接收的字节数
//      uiRxCount:      当前统计段收到的报文个数
//      uiRxUserCount:  当前统计段用户收到的报文个数
//      uiRxBytes:      当前统计段发送的所有字节数
//      uiRxUsrBytes:   当前统计段用户发送的字节数
//      uiRxCount:      当前统计段发送的报文个数
//      uiTxUserCount:  当前统计段用户发送的报文个数
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
typedef void (*IDT_CallMediaStats)(void *pUsrCtx, UCHAR ucType,
    UINT uiRxBytes, UINT uiRxUsrBytes, UINT uiRxCount, UINT uiRxUserCount,
    UINT uiTxBytes, UINT uiTxUsrBytes, UINT uiTxCount, UINT uiTxUserCount);

//--------------------------------------------------------------------------------
//      会议控制
//  输入:
//      ID:             IDT的呼叫ID
//      pcNum:          号码
//      ucCtrl:         控制信息,SRV_INFO_MICGIVE/SRV_INFO_MICTAKE
//      ucQueue:        队列号,0台上队列,1台下队列
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
IDT_API int IDT_CallConfCtrlReq(int ID, char *pcNum, UCHAR ucCtrl, UCHAR ucQueue);
//--------------------------------------------------------------------------------
//      会议控制指示
//  输入:
//      pUsrCtx:        用户上下文
//      pcNum:          号码
//      ucCtrl:         控制信息,SRV_INFO_MICREQ/SRV_INFO_MICREL
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
typedef int (*IDT_CallConfCtrlInd)(void *pUsrCtx, char *pcNum, UCHAR ucCtrl);
//--------------------------------------------------------------------------------
//      呼叫透传信息
//  输入:
//      ID:             IDT的呼叫ID
//      dwInfo:         信息码
//      pucInfo:        信息内容
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
IDT_API int IDT_CallSendInfo(int ID, DWORD dwInfo, UCHAR *pucInfo);
//--------------------------------------------------------------------------------
//      呼叫透传信息指示
//  输入:
//      pUsrCtx:        用户上下文
//      dwInfo:         信息码
//      pucInfo:        信息内容
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
typedef int (*IDT_CallRecvInfo)(void *pUsrCtx, DWORD dwInfo, UCHAR *pucInfo);

//--------------------------------------------------------------------------------
//      发送透传消息
//  输入:
//      dwSn:           消息事务号
//      dwType:         及时消息类型
//      pcFrom:         源号码
//      pcTo:           对端号码,可能为空
//      pucBuf:         透传消息内容
//      iLen:           透传消息长度
//      pcIp:           消息目的IP地址,NULL/0表示和启动时一样地址
//      iPort:          消息目的端口
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
IDT_API int IDT_SendPassThrouth(DWORD dwSn, DWORD dwType, char *pcTo, UCHAR *pucBuf, int iLen, char *pcIp = NULL, int iPort = 0);
//--------------------------------------------------------------------------------
//      收到透传消息
//  输入:
//      pcIp:           消息源IP地址
//      iPort:          消息源端口
//      pucSn:          消息事务号
//      dwType:         及时消息类型
//      pcFrom:         源号码
//      pcTo:           目的号码
//      pucBuf:         透传消息内容
//      iLen:           透传消息长度
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
typedef int (*IDT_RecvPassTrouth)(char *pcIp, int iPort, UCHAR *pucSn, DWORD dwType, char* pcFrom, char* pcTo, UCHAR* pucBuf, int iLen);
//--------------------------------------------------------------------------------
//      获取IM文件名
//  输入:
//      dwSn:           消息事务号
//      pcTo:           目的号码
//      dwType:         及时消息类型
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
IDT_API int IDT_IMGetFileName(DWORD dwSn, char *pcTo, DWORD dwType);
//--------------------------------------------------------------------------------
//      获取IM文件名响应
//  输入:
//      dwSn:           消息事务号
//      pcFileName:     文件名
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
typedef int (*IDT_IMGetFileNameRsp)(DWORD dwSn, char *pcFileName);
//--------------------------------------------------------------------------------
//      发送IM消息
//  输入:
//      dwSn:           消息事务号
//      dwType:         及时消息类型
//      pcTo:           目的号码
//      pcTxt:          文本内容
//      pcFileName:     文件名
//      pcSourceFileName:源文件名
//      pcIp:           消息目的IP地址,NULL/0表示和启动时一样地址
//      iPort:          消息目的端口
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
IDT_API int IDT_IMSend(DWORD dwSn, DWORD dwType, char *pcTo, char *pcTxt, char *pcFileName, char *pcSourceFileName, char *pcIp = NULL, int iPort = 0);
//--------------------------------------------------------------------------------
//      收到IM消息
//  输入:
//      dwSn:           消息事务号
//      dwType:         及时消息类型
//      pcFrom:         源号码
//      pcTo:           目的号码,#+号码:表示是组号码
//      pcOriTo:        原始目的号码
//      pcTxt:          文本内容
//      pcFileName:     文件名
//      pcSourceFileName:源文件名
//      pcTime:         发送的时间
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
typedef int (*IDT_IMRecv)(UCHAR *pucSn, DWORD dwType, char* pcFrom, char* pcTo, char* pcOriTo, char *pcTxt, char *pcFileName, char *pcSourceFileName, char *pcTime);
//--------------------------------------------------------------------------------
//      IM状态指示
//  输入:
//      dwSn:           消息事务号
//      pucSn:          系统的事务号
//      dwType:         及时消息类型
//      ucStatus:       状态
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
typedef int (*IDT_IMStatusInd)(DWORD dwSn, UCHAR *pucSn, DWORD dwType, UCHAR ucStatus);
//--------------------------------------------------------------------------------
//      阅读IM消息
//  输入:
//      dwSn:           消息事务号
//      pucSn:          系统的事务号
//      dwType:         及时消息类型
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
IDT_API int IDT_IMRead(DWORD dwSn, UCHAR *pucSn, DWORD dwType, char *pcTo);
//--------------------------------------------------------------------------------
//      订阅状态
//  输入:
//      pucNum:         用户号码或组号码,"##0"表示取消之前所有订阅,"0"表示所有用户,"###"表示自己所属的所有组
//      ucLevel:        订阅级别,GU_STATUSSUBS_e
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
IDT_API int IDT_StatusSubs(char *pcNum, UCHAR ucLevel);
//--------------------------------------------------------------------------------
//      组/用户状态指示
//  输入:
//      pStatus:        状态
//  返回:
//      无
//  注意:
//      由IDT.dll调用,告诉用户状态发生变化
//--------------------------------------------------------------------------------
typedef void (*IDT_GUStatusInd)(GU_STATUSGINFO_s *pStatus);
//--------------------------------------------------------------------------------
//      上报自己的GPS信息
//  输入:
//      pGps:           GPS信息
//      pucNum:         希望上报的号码,NULL表示与登录的号码相同
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
IDT_API int IDT_GpsReport(GPS_DATA_s *pGps, UCHAR *pucNum);
//--------------------------------------------------------------------------------
//      GPS数据指示,获得其他用户的GPS记录
//  输入:
//      pGpsRec:        GPS记录信息
//  返回:
//      无
//  注意:
//      由IDT.dll调用,告诉用户GPS发生变化
//--------------------------------------------------------------------------------
typedef void (*IDT_GpsRecInd)(GPS_REC_s *pGpsRec);
//--------------------------------------------------------------------------------
//      订阅GPS
//  输入:
//      pucNum:         用户号码,"##0"表示取消之前所有订阅
//      ucSubs:         是否订阅,0取消订阅,1订阅
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
IDT_API int IDT_GpsSubs(char *pcNum, UCHAR ucSubs);


//--------------------------------------------------------------------------------
//      强拆
//  输入:
//      pcPeerNum:      强拆的号码
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
IDT_API int IDT_ForceRel(char *pcPeerNum);
//--------------------------------------------------------------------------------
//      查询GPS历史数据
//  输入:
//      dwSn:           操作序号
//      pucNum:         用户号码
//      pcStartTime:    起始时间,格式为"%04d-%02d-%02d %02d:%02d:%02d"
//      pcEndTime:      结束时间,格式为"%04d-%02d-%02d %02d:%02d:%02d"
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
IDT_API int IDT_GpsHisQuery(DWORD dwSn, char *pcNum, char *pcStartTime, char *pcEndTime);
//--------------------------------------------------------------------------------
//      GPS历史数据指示
//  输入:
//      dwSn:           操作序号
//      pGpsRec:        GPS记录信息,ucStatus无效
//  返回:
//      无
//  注意:
//      由IDT.dll调用,告诉用户查询到的GPS历史数据
//--------------------------------------------------------------------------------
typedef void (*IDT_GpsHisQueryInd)(DWORD dwSn, GPS_REC_s *pGpsRec);
//--------------------------------------------------------------------------------
//      查询存储数据
//  输入:
//      dwSn:           操作序号
//      pQuery:         查询条件
//      pucNum:         存储服务器号码
//      pcNsIp:         存储服务器IP地址
//      iNsPort:        存储服务器端口
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
IDT_API int IDT_NsQueryReq(DWORD dwSn, NS_QUERY_s *pQuery, char *pcNsNum, char* pcNsIp, int iNsPort);
//--------------------------------------------------------------------------------
//      存储数据查询响应
//  输入:
//      dwSn:           操作序号
//      pGpsRec:        GPS记录信息,ucStatus无效
//  返回:
//      无
//  注意:
//      由IDT.dll调用,告诉用户查询到的GPS历史数据
//--------------------------------------------------------------------------------
typedef void (*IDT_NsQueryInd)(DWORD dwSn, NS_QUERYRSP_s *pQueryRsp);

//--------------------------------------------------------------------------------
//      添加用户
//  输入:
//      dwSn:           操作序号
//      pUser:          用户信息
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
IDT_API int IDT_UAdd(DWORD dwSn, UData_s* pUser);
//--------------------------------------------------------------------------------
//      删除用户
//  输入:
//      dwSn:           操作序号
//      pucNum:         用户号码
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
IDT_API int IDT_UDel(DWORD dwSn, UCHAR *pucNum);
//--------------------------------------------------------------------------------
//      修改用户
//  输入:
//      dwSn:           操作序号
//      pUser:          用户信息
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
IDT_API int IDT_UModify(DWORD dwSn, UData_s* pUser);
//--------------------------------------------------------------------------------
//      查询用户
//  输入:
//      dwSn:           操作序号
//      pucNum:         用户号码
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
IDT_API int IDT_UQuery(DWORD dwSn, UCHAR *pucNum);
//--------------------------------------------------------------------------------
//      用户操作响应
//  输入:
//      dwOptCode:      操作码      OPT_USER_ADD
//      dwSn:           操作序号
//      wRes:           结果        CAUSE_ZERO
//      pUser:          用户信息
//  返回:
//      无
//  注意:
//      由IDT.dll调用,告诉用户操作结果
//--------------------------------------------------------------------------------
typedef void (*IDT_UOptRsp)(DWORD dwOptCode, DWORD dwSn, WORD wRes, UData_s* pUser);


//--------------------------------------------------------------------------------
//      添加组
//  输入:
//      dwSn:           操作序号
//      pGroup:         组信息
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
IDT_API int IDT_GAdd(DWORD dwSn, GData_s* pGroup);
//--------------------------------------------------------------------------------
//      删除组
//  输入:
//      dwSn:           操作序号
//      pucNum:         用户号码
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
IDT_API int IDT_GDel(DWORD dwSn, UCHAR *pucNum);
//--------------------------------------------------------------------------------
//      修改组
//  输入:
//      dwSn:           操作序号
//      pGroup:         组信息
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
IDT_API int IDT_GModify(DWORD dwSn, GData_s* pGroup);
//--------------------------------------------------------------------------------
//      查询组
//  输入:
//      dwSn:           操作序号
//      pucNum:         组号码
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
IDT_API int IDT_GQuery(DWORD dwSn, UCHAR *pucNum);
//--------------------------------------------------------------------------------
//      查询组内用户信息
//  输入:
//      dwSn:           操作序号
//      pucNum:         组号码
//      pExt:           查询参数
//  返回:
//      0:              成功
//      -1:             失败
//  注意:
//      pucNum="0"并且pExt->dwCount!=0,表示查询所有用户,此时pExt->dwPage有效
//--------------------------------------------------------------------------------
IDT_API int IDT_GQueryU(DWORD dwSn, UCHAR *pucNum, QUERY_EXT_s *pExt);
//--------------------------------------------------------------------------------
//      查询用户所在组信息
//  输入:
//      dwSn:           操作序号
//      pucNum:         用户号码
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
IDT_API int IDT_UQueryG(DWORD dwSn, UCHAR *pucNum);
//--------------------------------------------------------------------------------
//      组添加用户
//  输入:
//      dwSn:           操作序号
//      pucNum:         组号码
//      pMember:        用户数据
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
IDT_API int IDT_GAddU(DWORD dwSn, UCHAR *pucNum, GROUP_MEMBER_s *pMember);
//--------------------------------------------------------------------------------
//      组删除用户
//  输入:
//      dwSn:           操作序号
//      pucNum:         组号码
//      pucUNum:        用户号码
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
IDT_API int IDT_GDelU(DWORD dwSn, UCHAR *pucNum, UCHAR *pucUNum);
//--------------------------------------------------------------------------------
//      修改用户
//  输入:
//      dwSn:           操作序号
//      pucNum:         组号码
//      pMember:        用户数据
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
IDT_API int IDT_GModifyU(DWORD dwSn, UCHAR *pucNum, GROUP_MEMBER_s *pMember);
//--------------------------------------------------------------------------------
//      组操作响应
//  输入:
//      dwOptCode:      操作码
//      dwSn:           操作序号
//      wRes:           结果
//      pGroup:         组信息
//  返回:
//      无
//  注意:
//      由IDT.dll调用,告诉用户操作结果
//--------------------------------------------------------------------------------
typedef void (*IDT_GOptRsp)(DWORD dwOptCode, DWORD dwSn, WORD wRes,  GData_s *pGroup);


//--------------------------------------------------------------------------------
//      添加组织
//  输入:
//      dwSn:           操作序号
//      pOrg:           组织信息
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
IDT_API int IDT_OAdd(DWORD dwSn, ORG_s* pOrg);
//--------------------------------------------------------------------------------
//      删除组织
//  输入:
//      dwSn:           操作序号
//      pucNum:         组织号码
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
IDT_API int IDT_ODel(DWORD dwSn, UCHAR *pucNum);
//--------------------------------------------------------------------------------
//      修改组织
//  输入:
//      dwSn:           操作序号
//      pOrg:           组织信息
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
IDT_API int IDT_OModify(DWORD dwSn, ORG_s* pOrg);
//--------------------------------------------------------------------------------
//      查询组织
//  输入:
//      dwSn:           操作序号
//      pucNum:         组织号码.
//                      如果是admin,查询"all",返回系统中所有组织列表
//                      如果是admin,查询确定号码的组织,单个组织
//                      如果不是admin,查询"all",返回自己所在的组织信息
//                      如果不是admin,查询确定号码的组织,如果自己在这个组织内,单个组织;如果不在这个组织内,wRes返回组织越权操作
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
IDT_API int IDT_OQuery(DWORD dwSn, UCHAR *pucNum);
//--------------------------------------------------------------------------------
//      设置组成员附加信息
//  输入:
//      dwSn:           操作序号
//      pucUNum:        用户号码
//      pucGNum:        组号码
//      pucInfo:        附加信息,最长512字节
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
IDT_API int IDT_SetGMemberExtInfo(DWORD dwSn, UCHAR *pucUNum, UCHAR *pucGNum, UCHAR *pucInfo);
//--------------------------------------------------------------------------------
//      组织操作响应
//  输入:
//      dwOptCode:      操作码      OPT_ORG_ADD
//      dwSn:           操作序号
//      wRes:           结果        CAUSE_ZERO
//      pOrgList:       组织信息
//      pucCauseStr:    原因值字符串
//  返回:
//      无
//  注意:
//      由IDT.dll调用,告诉用户操作结果
//--------------------------------------------------------------------------------
typedef void (*IDT_OOptRsp)(DWORD dwOptCode, DWORD dwSn, WORD wRes, ORG_LIST_s* pOrgList, UCHAR *pucCauseStr);
//--------------------------------------------------------------------------------
//      OAM操作提示
//  输入:
//      dwOptCode:      操作码
//                      OPT_USER_ADD        pucUNum,pucUName,ucUAttr有效
//                      OPT_USER_DEL        pucUNum有效
//                      OPT_USER_MODIFY     pucUNum,pucUName,ucUAttr有效
//                      OPT_G_ADD           pucGNum,pucGName有效
//                      OPT_G_DEL           pucGNum有效
//                      OPT_G_MODIFY        pucGNum,pucGName有效
//                      OPT_G_ADDUSER       pucGNum,pucUNum,pucUName,ucUAttr有效
//                      OPT_G_DELUSER       pucGNum,pucUNum有效
//                      OPT_G_MODIFYUSER    pucGNum,pucUNum,pucUName,ucUAttr有效
//      pucGNum:        组号码
//      pucGName:       组名字
//      pucUNum:        用户号码
//      pucUName:       用户名字
//      ucUAttr:        用户属性
//  返回:
//      无
//  注意:
//      由IDT.dll调用,告诉用户组删除/添加用户等操作
//--------------------------------------------------------------------------------
typedef void (*IDT_OamNotify)(DWORD dwOptCode, UCHAR *pucGNum, UCHAR *pucGName, UCHAR *pucUNum, UCHAR *pucUName, UCHAR ucUAttr);
//--------------------------------------------------------------------------------
//      组成员扩展信息指示
//  输入:
//      pucGNum:        组号码,如果为空,表示是订阅所有用户返回的
//      pInfo:          扩展信息
//  返回:
//      0:              成功
//      -1:             失败
//  注意:
//      由IDT.dll调用,告诉用户扩展信息指示
//--------------------------------------------------------------------------------
typedef int (*IDT_GMemberExtInfoInd)(UCHAR *pucGNum, GMEMBER_EXTINFO_s *pInfo);
//--------------------------------------------------------------------------------
//      代理注册
//  输入:
//      cIp:            服务器IP地址
//      iPort:          服务器端口号
//      pcNum:          号码
//      cPwd:           密码
//      RType:          注册类型
//  返回:
//      0:              成功
//      -1:             失败
//  注意:
//      IDT发送代理注册消息,带上pcNum,pcPwd，到pcIp:iPort地址去
//--------------------------------------------------------------------------------
IDT_API int IDT_ProxyReg(char *pcIp, int iPort, char* pcNum, char* pcPwd, REG_TYPE_e RType);
//--------------------------------------------------------------------------------
//      记录日志
//  输入:
//      pcFileName:     文件名
//      pcLog:          日志内容
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
IDT_API int IDT_Log(char *pcFileName, char *pcLog);

//--------------------------------------------------------------------------------
//      读取配置文件
//  输入:
//      pcFileName:     文件名
//      pcSection:      段
//      pcKey:          关键字
//      pcVal:          值,256字节.返回
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
IDT_API int IDT_ReadIni(char *pcFileName, char *pcSection, char *pcKey, char *pcVal);
//--------------------------------------------------------------------------------
//      写入配置文件
//  输入:
//      pcFileName:     文件名
//      pcSection:      段
//      pcKey:          关键字
//      pcVal:          值
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
IDT_API int IDT_WriteIni(char *pcFileName, char *pcSection, char *pcKey, char *pcVal);
//--------------------------------------------------------------------------------
//      读取Json数据
//  输入:
//      pcJson:         Json字符串
//      pcKey:          关键字
//      iType:          类型,1字符串,2布尔
//      iStrSize:       字符串大小
//  返回:
//      0:              成功
//          pcStr:      字符串
//          bVal:       布尔值
//      -1:             失败
//--------------------------------------------------------------------------------
IDT_API int IDT_ReadJson(char *pcJson, char *pcKey, int iType, char *pcStr, int iStrSize, bool &bVal);
//--------------------------------------------------------------------------------
//      写入Json数据
//  输入:
//      pcJson:         Json字符串
//      iSize:          Json字符串缓冲区大小
//      pcKey:          关键字
//      iType:          类型,1字符串,2布尔
//      pcStr:          字符串
//      bVal:           布尔值
//  返回:
//      0:              成功
//          pcJson:     Json字符串
//      -1:             失败
//--------------------------------------------------------------------------------
IDT_API int IDT_WriteJson(char *pcJson, int iSize, char *pcKey, int iType, char *pcStr, bool bVal);

//--------------------------------------------------------------------------------
//      获取IDT状态
//  输入:
//      无
//  返回:
//      0:              成功
//          pcJson:     Json字符串
//      -1:             失败
//--------------------------------------------------------------------------------
IDT_API int IDT_GetStatus(char *pcJson, int iSize);

//--------------------------------------------------------------------------------
//      用户调试函数
//  输入:
//      pcTxt:          调试字符串
//  返回:
//      0:              成功
//      -1:             失败
//  注意:
//      FREERTOS调试比较麻烦,加入这个,便于远程调试
//--------------------------------------------------------------------------------
typedef int (*IDT_Dbg)(char *pcTxt);

//回调函数
typedef struct _IDT_CALLBACK_s
{
    IDT_StatusInd               pfStatusInd;
    IDT_GInfoInd                pfGInfoInd;
    IDT_CallPeerAnswer          pfCallPeerAnswer;
    IDT_CallIn                  pfCallIn;
    IDT_CallRelInd              pfCallRelInd;
    IDT_CallRecvNum             pfCallRecvNum;
    IDT_CallRecvAudioData       pfCallRecvAudioData;
    IDT_CallRecvVideoData       pfCallRecvVideoData;
    IDT_CallSetAudioCodec       pfCallSetAudioCodec;
    IDT_CallSetVideoCodec       pfCallSetVideoCodec;
    IDT_CallMediaStats          pfCallMediaStats;
    IDT_CallMicInd              pfCallMicInd;
    IDT_CallRecvInfo            pfCallRecvInfo;
    IDT_CallTalkingIDInd        pfCallTalkingIDInd;
    IDT_CallConfCtrlInd         pfCallConfCtrlInd;
    IDT_CallConfStatusRsp       pfCallConfStatusRsp;
	IDT_RecvPassTrouth			pfRecvPassThrouth;
    IDT_IMGetFileNameRsp        pfIMGetFileNameRsp;
    IDT_IMRecv                  pfIMRecv;
    IDT_IMStatusInd             pfIMStatusInd;
    IDT_UOptRsp                 pfUOptRsp;
    IDT_GOptRsp                 pfGOptRsp;
    IDT_OamNotify               pfOamNotify;
    IDT_GUStatusInd             pfGUStatusInd;
    IDT_GpsRecInd               pfGpsRecInd;
    IDT_GpsHisQueryInd          pfGpsHisQueryInd;
    IDT_NsQueryInd              pfNsQueryInd;
    IDT_OOptRsp                 pfOOptRsp;
    IDT_GMemberExtInfoInd       pfGMemberExtInfoInd;
    IDT_Dbg                     pfDbg;
}IDT_CALLBACK_s;

//--------------------------------------------------------------------------------
//      启动
//  输入:
//      pcIniFile:      INI文件名
//      iMaxCallNum:    最大并发呼叫数
//      cIp:            服务器IP地址
//      iPort:          服务器端口号
//      cGpsIp:         GPS服务器IP地址
//      iGpsPort:       GPS服务器端口号
//      cNumber:        号码
//      cPwd:           密码
//      iRegFlag:       是否要注册,0不需要,1需要注册
//      CallBack:       回调函数
//      iSigPort:       信令端口
//      iMedRtpPort:    RTP媒体端口
//      iMedTcpPort:    TCP媒体监听端口
//      iMediaCtrl:     媒体控制,0不需要IDT控制媒体,1需要IDT控制媒体
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
#ifdef WINDOWS
IDT_API int IDT_Start(char* pcIniFile, int iMaxCallNum, char* pcIp, int iPort, char* pcGpsIp, int iGpsPort, char* pcNum, char* pcPwd,
    int iRegFlag, IDT_CALLBACK_s *CallBack, int iSigPort = 10100, int iMedRtpPort = 11000, int iMedTcpPort = 0, int iMediaCtrl = 1);
#else
IDT_API int IDT_Start(char* pcIniFile, int iMaxCallNum, char* pcIp, int iPort, char* pcGpsIp, int iGpsPort, char* pcNum, char* pcPwd,
    int iRegFlag, IDT_CALLBACK_s *CallBack, int iSigPort = 10100, int iMedRtpPort = 11000, int iMedTcpPort = 0);
#endif
//--------------------------------------------------------------------------------
//      退出
//  输入:
//      无
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
IDT_API int IDT_Exit();


#ifdef WINDOWS
//--------------------------------------------------------------------------------
//      启动VideoIn
//  输入:
//      ID:             呼叫ID
//      hUser:          用户句柄
//      iDeviceID:      第几个摄像头
//      hWnd:           本地预览窗口
//      ucCodec:        编码器
//      iWidth:         宽
//      iHeight:        高
//      iFrameRate:     帧率
//      iBitRate:       码率
//      iGop:           I帧间隔
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
IDT_API int IDT_VideoInStart(int ID, HANDLE hUser, int iDeviceID, HWND hWnd, UCHAR ucCodec, int iWidth, int iHeight, int iFrameRate, int iBitRate, int iGop);
//--------------------------------------------------------------------------------
//      启动VideoOut
//  输入:
//      ID:             呼叫ID
//      hUser:          用户句柄
//      hWnd:           视频显示窗口
//      ucCodec:        编码器
//      iWidth:         宽
//      iHeight:        高
//      iFrameRate:     帧率
//      iBitRate:       码率
//      iGop:           I帧间隔
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
IDT_API int IDT_VideoOutStart(int ID, HANDLE hUser, HWND hWnd, UCHAR ucCodec, int iWidth, int iHeight, int iFrameRate, int iBitRate, int iGop);
#endif



//--------------------------------------------------------------------------------
//      PTT呼出
//  输入:
//      cPeerNum:       对方号码
//      pUsrCtx:        用户上下文
//  返回:
//      -1:             失败
//      else:           呼叫标识
//--------------------------------------------------------------------------------
IDT_API int IDT_PTTCallMakeOut(char* pcPeerNum, void *pUsrCtx);

//--------------------------------------------------------------------------------
//      PTT呼入
//  输入:
//      ID:             IDT的呼叫ID
//      pcPeerNum:      对方号码,组号码
//      pcPeerName:     对方名字,组名字
//  返回:
//      0:              成功
//      -1:             失败
//  注意:
//      由IDT.dll调用,告诉用户有呼叫进入
//--------------------------------------------------------------------------------
typedef int (*IDT_PTTCallIn)(int ID, char *pcPeerNum, char *pcPeerName);

//--------------------------------------------------------------------------------
//      PTT呼入应答
//  输入:
//      ID:             IDT的呼叫ID
//      pUsrCtx:        用户上下文
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
IDT_API int IDT_PTTCallAnswer(int ID, void *pUsrCtx);

//回调函数
typedef struct _IDT_PTTCALLBACK_s
{
    IDT_StatusInd               pfStatusInd;
    IDT_GInfoInd                pfGInfoInd;
    IDT_PTTCallIn               pfCallIn;
    IDT_CallRelInd              pfCallRelInd;
    IDT_CallRecvAudioData       pfCallRecvAudioData;
    IDT_CallMicInd              pfCallMicInd;
    IDT_CallTalkingIDInd        pfCallTalkingIDInd;
    IDT_UOptRsp                 pfUOptRsp;
    IDT_GOptRsp                 pfGOptRsp;
    IDT_OamNotify               pfOamNotify;
}IDT_PTTCALLBACK_s;

//--------------------------------------------------------------------------------
//      PTT启动
//  输入:
//      pcIniFile:      INI文件名
//      pcSrvIp:        服务器IP地址,192.168.0.225:10000这样的格式
//      pcGpsIp:        GPS服务器IP地址,192.168.0.225:10001这样的格式
//      pcNumber:       号码
//      pcPwd:          密码
//      CallBack:       回调函数
//  返回:
//      0:              成功
//      -1:             失败
//  注意:
//      PTT模式启动,只支持PTT呼叫.只支持一路.
//      默认本地信令端口,UDP10100
//      默认本地媒体端口,UDP11000~11003
//--------------------------------------------------------------------------------
IDT_API int IDT_PTTStart(char* pcIniFile, char* pcSrvIp, char* pcGpsIp, char* pcNum, char* pcPwd, IDT_PTTCALLBACK_s *CallBack);



#ifdef  __cplusplus
}
#endif


#endif



