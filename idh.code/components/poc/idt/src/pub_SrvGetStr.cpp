#ifndef MC_CC_TM_DEFINE
enum
{
    CCTM_HB_CSA_P               = 1,
    CCTM_HB_CSA_MG              = 2,
    CCTM_HB_BCSM                = 3,

    //CSA消息
    CCTM_WAIT_MGBINGRSP         = 4,
    CCTM_WAIT_MGIVRRSP          = 5,
    CCTM_WAIT_MGCONNRSP         = 6,
    CCTM_WAIT_MGDISCRSP         = 7,
    CCTM_WAIT_MMACCRSP          = 8,
    CCTM_WAIT_PSETUPACK         = 9,
    CCTM_WAIT_PCONNACK          = 10,
    CCTM_WAIT_MY_MGMODMYRSP     = 11,
    CCTM_WAIT_MY_PMODRSP        = 12,
    CCTM_WAIT_MY_MGMODPEERRSP   = 13,
    CCTM_WAIT_P_MGMODRSP        = 14,

    //BCSM消息
    //消息定时器
    CCTM_WAIT_SETUPACK          = 15,
    CCTM_WAIT_CONNACK           = 16,
    //收号定时器
    CCTM_RECVNUM_START          = 17,
    CCTM_RECVNUM_INTER          = 18,
    CCTM_RECVNUM_TOTAL          = 19,
    //O端
    CCTM_O_PIC_AuthorizeOriginationAttempt = 20,        //等待MM的接入响应,CSA
    CCTM_O_PIC_CollectInfomation= 21,
    CCTM_O_PIC_AnalyzeInfomation= 22,
    CCTM_O_PIC_SelectRoute      = 23,                   //等待MM的路由响应 BCSM
    CCTM_O_PIC_AuthorizeCallSetup= 24,
    CCTM_O_PIC_SendCall         = 25,
    CCTM_O_PIC_Alerting         = 26,
    CCTM_O_PIC_Active           = 27,
    CCTM_O_PIC_Suspended        = 28,
    CCTM_O_PIC_Exception        = 29,
    //T端
    CCTM_T_PIC_AuthorizeTerminationAttempt = 30,
    CCTM_T_PIC_SelectFacility   = 31,
    CCTM_T_PIC_PresentCall      = 32,
    CCTM_T_PIC_Alerting         = 33,
    CCTM_T_PIC_Active           = 34,
    CCTM_T_PIC_Suspended        = 35,
    CCTM_T_PIC_Exception        = 36,

    CCTM_RE_CALL                = 37,
    CCTM_CS_MICGET              = 38,
    CCTM_CS_MICREL              = 39,

    CCTM_RECV_NUM               = 40,
    CCTM_RECV_NUM_TOTAL         = 41,

    CCTM_SCAN_DEV               = 42,                   //扫描设备
    CCTM_SCAN_TER               = 43,                   //扫描终端

    CCTM_WAIT_SETUP             = 44,


    MSG_TM_MAX,
};
#endif
char* GetMcTmStr(WORD wTm)
{
    static char cBuf[64];

    switch (wTm)
    {
    case CCTM_HB_CSA_P:
        return (char*)"CCTM_HB_CSA_P";
    case CCTM_HB_CSA_MG:
        return (char*)"CCTM_HB_CSA_MG";
    case CCTM_HB_BCSM:
        return (char*)"CCTM_HB_BCSM";

    //CSA消息
    case CCTM_WAIT_MGBINGRSP:
        return (char*)"CCTM_WAIT_MGBINGRSP";
    case CCTM_WAIT_MGIVRRSP:
        return (char*)"CCTM_WAIT_MGIVRRSP";
    case CCTM_WAIT_MGCONNRSP:
        return (char*)"CCTM_WAIT_MGCONNRSP";
    case CCTM_WAIT_MGDISCRSP:
        return (char*)"CCTM_WAIT_MGDISCRSP";
    case CCTM_WAIT_MMACCRSP:
        return (char*)"CCTM_WAIT_MMACCRSP";
    case CCTM_WAIT_PSETUPACK:
        return (char*)"CCTM_WAIT_PSETUPACK";
    case CCTM_WAIT_PCONNACK:
        return (char*)"CCTM_WAIT_PCONNACK";
    case CCTM_WAIT_MY_MGMODMYRSP:
        return (char*)"CCTM_WAIT_MY_MGMODMYRSP";
    case CCTM_WAIT_MY_PMODRSP:
        return (char*)"CCTM_WAIT_MY_PMODRSP";
    case CCTM_WAIT_MY_MGMODPEERRSP:
        return (char*)"CCTM_WAIT_MY_MGMODPEERRSP";
    case CCTM_WAIT_P_MGMODRSP:
        return (char*)"CCTM_WAIT_P_MGMODRSP";

    //BCSM消息
    //消息定时器
    case CCTM_WAIT_SETUPACK:
        return (char*)"CCTM_WAIT_SETUPACK";
    case CCTM_WAIT_CONNACK:
        return (char*)"CCTM_WAIT_CONNACK";
    //收号定时器
    case CCTM_RECVNUM_START:
        return (char*)"CCTM_RECVNUM_START";
    case CCTM_RECVNUM_INTER:
        return (char*)"CCTM_RECVNUM_INTER";
    case CCTM_RECVNUM_TOTAL:
        return (char*)"CCTM_RECVNUM_TOTAL";
    //O端
    case CCTM_O_PIC_AuthorizeOriginationAttempt:
        return (char*)"CCTM_O_PIC_AuthorizeOriginationAttempt";
    case CCTM_O_PIC_CollectInfomation:
        return (char*)"CCTM_O_PIC_CollectInfomation";
    case CCTM_O_PIC_AnalyzeInfomation:
        return (char*)"CCTM_O_PIC_AnalyzeInfomation";
    case CCTM_O_PIC_SelectRoute:
        return (char*)"CCTM_O_PIC_SelectRoute";
    case CCTM_O_PIC_AuthorizeCallSetup:
        return (char*)"CCTM_O_PIC_AuthorizeCallSetup";
    case CCTM_O_PIC_SendCall:
        return (char*)"CCTM_O_PIC_SendCall";
    case CCTM_O_PIC_Alerting:
        return (char*)"CCTM_O_PIC_Alerting";
    case CCTM_O_PIC_Active:
        return (char*)"CCTM_O_PIC_Active";
    case CCTM_O_PIC_Suspended:
        return (char*)"CCTM_O_PIC_Suspended";
    case CCTM_O_PIC_Exception:
        return (char*)"CCTM_O_PIC_Exception";
    //T端
    case CCTM_T_PIC_AuthorizeTerminationAttempt:
        return (char*)"CCTM_T_PIC_AuthorizeTerminationAttempt";
    case CCTM_T_PIC_SelectFacility:
        return (char*)"CCTM_T_PIC_SelectFacility";
    case CCTM_T_PIC_PresentCall:
        return (char*)"CCTM_T_PIC_PresentCall";
    case CCTM_T_PIC_Alerting:
        return (char*)"CCTM_T_PIC_Alerting";
    case CCTM_T_PIC_Active:
        return (char*)"CCTM_T_PIC_Active";
    case CCTM_T_PIC_Suspended:
        return (char*)"CCTM_T_PIC_Suspended";
    case CCTM_T_PIC_Exception:
        return (char*)"CCTM_T_PIC_Exception";

    case CCTM_RE_CALL:
        return (char*)"CCTM_RE_CALL";
    case CCTM_CS_MICGET:
        return (char*)"CCTM_CS_MICGET";
    case CCTM_CS_MICREL:
        return (char*)"CCTM_CS_MICREL";

    case CCTM_RECV_NUM:
        return (char*)"CCTM_RECV_NUM";
    case CCTM_RECV_NUM_TOTAL:
        return (char*)"CCTM_RECV_NUM_TOTAL";

    case CCTM_SCAN_DEV:
        return (char*)"CCTM_SCAN_DEV";
    case CCTM_SCAN_TER:
        return (char*)"CCTM_SCAN_TER";

    case CCTM_WAIT_SETUP:
        return (char*)"CCTM_WAIT_SETUP";

    default:
        snprintf(cBuf, sizeof(cBuf), "Tm=%d", wTm);
        return cBuf;
    }
}

#ifndef IDT_CC_TM_DEFINE
#define     CPTM_MM_REG         0x10        // 注册请求
#define     CPTM_MM_OFFLINE     0x11        // 离线扫描
#define     CPTM_MM_PERIOD      0x12        // 周期注册
#define     CPTM_MM_MODIFY      0x13        // 修改用户属性
#define     CPTM_MM_NAT         0x14        // NAT

#define     CPTM_CC_SETUPACK    0x20        // 发送SETUP,等待SETUP_ACK
#define     CPTM_CC_CONN        0x21        // 发送SETUP,等待CONN
#define     CPTM_CC_ANSWER      0x22        // 发送CallIn给用户,等用户应答
#define     CPTM_CC_CONNACK     0x23        // 发送CONN给对端,等待CONNACK
#define     CPTM_CC_HB          0x24        // 通话后的心跳定时器
#define     CPTM_CC_PRESS       0x25        // 按键定时器
#endif
char* GetIdtTmStr(WORD wTm)
{
    static char cBuf[32];

    switch (wTm)
    {
    case CPTM_MM_REG:
        return (char*)"CPTM_MM_REG";
    case CPTM_MM_OFFLINE:
        return (char*)"CPTM_MM_OFFLINE";
    case CPTM_MM_PERIOD:
        return (char*)"CPTM_MM_PERIOD";
    case CPTM_MM_MODIFY:
        return (char*)"CPTM_MM_MODIFY";
    case CPTM_MM_NAT:
        return (char*)"CPTM_MM_NAT";
    case CPTM_CC_SETUPACK:
        return (char*)"CPTM_CC_SETUPACK";
    case CPTM_CC_CONN:
        return (char*)"CPTM_CC_CONN";
    case CPTM_CC_ANSWER:
        return (char*)"CPTM_CC_ANSWER";
    case CPTM_CC_CONNACK:
        return (char*)"CPTM_CC_CONNACK";
    case CPTM_CC_HB:
        return (char*)"CPTM_CC_HB";
    case CPTM_CC_PRESS:
        return (char*)"CPTM_CC_PRESS";
    default:
        snprintf(cBuf, sizeof(cBuf), "Tm=%d", wTm);
        return cBuf;
    }
}

#ifndef MG_CC_TM_DEFINE
#define     CPTM_SCAN        0x01   //TCP扫描定时器
#define     CPTM_HB          0x02   //状态机与主控状态机的心跳定时器
#define     CPTM_NUM         0x03   //发送号码定时器
#endif

char* GetMgTmStr(WORD wTm)
{
    static char cBuf[32];

    switch (wTm)
    {
    case CPTM_SCAN:
        return (char*)"CPTM_SCAN";
    case CPTM_HB:
        return (char*)"CPTM_HB";
    case CPTM_NUM:
        return (char*)"CPTM_NUM";
    default:
        snprintf(cBuf, sizeof(cBuf), "Tm=%d", wTm);
        return cBuf;
    }
}

char *GetCauseStr(USHORT usCause)
{
    static char cBuf[96];
    if (CAUSE_TIMER_EXPIRY == (usCause & 0xff))
    {
        WORD ucSrc = (usCause & 0xc000);
        UCHAR ucH = (usCause & 0x3f00) >> 8;
        switch (ucSrc)
        {
        case CAUSE_EXPIRE_IDT://IDT定时器超时
            snprintf(cBuf, sizeof(cBuf), "定时器超时:IDT-%s", GetIdtTmStr(ucH));
            break;
        case CAUSE_EXPIRE_MC://MC定时器超时
            snprintf(cBuf, sizeof(cBuf), "定时器超时:MC-%s", GetMcTmStr(ucH));
            break;
        case CAUSE_EXPIRE_MG://MG定时器超时
            snprintf(cBuf, sizeof(cBuf), "定时器超时:MG-%s", GetMgTmStr(ucH));
            break;
        default:
            snprintf(cBuf, sizeof(cBuf), "定时器超时:%d-%d", ucSrc, ucH);
            break;
        }
        return cBuf;
    }

    switch (usCause)
    {
    case CAUSE_ZERO:
        return (char*)"错误0";
    case CAUSE_UNASSIGNED_NUMBER:
        return (char*)"未分配号码";
    case CAUSE_NO_ROUTE_TO_DEST:
        return (char*)"无目的路由";
    case CAUSE_USER_BUSY:
        return (char*)"用户忙";
    case CAUSE_ALERT_NO_ANSWER:
        return (char*)"用户无应答(人不应答)";
    case CAUSE_CALL_REJECTED:
        return (char*)"呼叫被拒绝";
    case CAUSE_ROUTING_ERROR:
        return (char*)"路由错误";
    case CAUSE_FACILITY_REJECTED:
        return (char*)"设备拒绝";
    case CAUSE_NORMAL_UNSPECIFIED:
        return (char*)"通常,未指定";
    case CAUSE_TEMPORARY_FAILURE:
        return (char*)"临时错误";
    case CAUSE_RESOURCE_UNAVAIL:
        return (char*)"资源不可用";
    case CAUSE_INVALID_CALL_REF:
        return (char*)"不正确的呼叫参考号";
    case CAUSE_MANDATORY_IE_MISSING:
        return (char*)"必选信息单元丢失";
    case CAUSE_TIMER_EXPIRY:
        return (char*)"定时器超时";
    case CAUSE_CALL_REJ_BY_USER:
        return (char*)"被用户拒绝";
    case CAUSE_CALLEE_STOP:
        return (char*)"被叫停止";
    case CAUSE_USER_NO_EXIST:
        return (char*)"用户不存在";
    case CAUSE_MS_UNACCESSAVLE:
        return (char*)"不可接入";
    case CAUSE_MS_POWEROFF:
        return (char*)"用户关机";
    case CAUSE_FORCE_RELEASE:
        return (char*)"强制拆线";
    case CAUSE_HO_RELEASE:
        return (char*)"切换拆线";
    case CAUSE_CALL_CONFLICT:
        return (char*)"呼叫冲突";
    case CAUSE_TEMP_UNAVAIL:
        return (char*)"暂时无法接通";
    case CAUSE_AUTH_ERROR:
        return (char*)"鉴权错误";
    case CAUSE_NEED_AUTH:
        return (char*)"需要鉴权";
    case CAUSE_SDP_SEL:
        return (char*)"SDP选择错误";
    case CAUSE_MS_ERROR:
        return (char*)"媒体资源错误";
    case CAUSE_INNER_ERROR:
        return (char*)"内部错误";
    case CAUSE_PRIO_ERROR:
        return (char*)"优先级不够";
    case CAUSE_SRV_CONFLICT:
        return (char*)"业务冲突";
    case CAUSE_NOTREL_RECALL:
        return (char*)"由于业务要求,不释放,启动重呼定时器";
    case CAUSE_NO_CALL:
        return (char*)"呼叫不存在";
    case CAUSE_ERROR_IPADDR:
        return (char*)"错误IP地址过来的业务请求";
    case CAUSE_DUP_REG:
        return (char*)"重复注册";
    case CAUSE_MG_OFFLINE:
        return (char*)"MG离线";
    case CAUSE_DS_REQ_QUITCALL:
        return (char*)"调度员要求退出呼叫";
    case CAUSE_DB_ERROR:
        return (char*)"数据库操作错误";
    case CAUSE_TOOMANY_USER:
        return (char*)"用户数太多";
    case CAUSE_SAME_USERNUM:
        return (char*)"相同的用户号码";
    case CAUSE_SAME_USERIPADDR:
        return (char*)"相同的固定IP地址";
    case CAUSE_PARAM_ERROR:
        return (char*)"参数错误";
    case CAUSE_SAME_GNUM:
        return (char*)"相同的组号码";
    case CAUSE_TOOMANY_GROUP:
        return (char*)"太多的组";
    case CAUSE_NO_GROUP:
        return (char*)"没有这个组";
    case CAUSE_SAME_USERNAME:
        return (char*)"相同的用户名字";
    case CAUSE_OAM_OPT_ERROR:
        return (char*)"OAM操作错误";
    case CAUSE_INVALID_NUM_FORMAT:
        return (char*)"不正确的地址格式";
    case CAUSE_INVALID_DNSIP:
        return (char*)"DNS或IP地址错误";
    case CAUSE_SRV_NOTSUPPORT:
        return (char*)"不支持的业务";
    case CAUSE_MEDIA_NOTDATA:
        return (char*)"没有媒体数据";
    case CAUSE_RECALL:
        return (char*)"重新呼叫";
    case CAUSE_LINK_DISC:
        return (char*)"断链";
    case CAUSE_ORG_RIGHT:
        return (char*)"组织越权";
    case CAUSE_SAME_ORGNUM:
        return (char*)"相同的组织号码";
    case CAUSE_SAME_ORGNAME:
        return (char*)"相同的组织名字";
    case CAUSE_UNASSIGNED_ORG:
        return (char*)"未分配的组织号码";
    case CAUSE_INOTHER_ORG:
        return (char*)"在其他组织中";
    case CAUSE_HAVE_GCALL:
        return (char*)"已经有组呼存在";
    case CAUSE_HAVE_CONF:
        return (char*)"已经有会议存在";
    case CAUSE_SEG_FORMAT:
        return (char*)"错误的号段格式";
    case CAUSE_USEG_CONFLICT:
        return (char*)"用户号码段冲突";
    case CAUSE_GSEG_CONFLICT:
        return (char*)"组号码段冲突";
    case CAUSE_NOTIN_SEG:
        return (char*)"不在号段内";
    case CAUSE_USER_IN_TOOMANY_GROUP:
        return (char*)"用户所在组太多";
    case CAUSE_DSSEG_CONFLICT:
        return (char*)"调度台号段冲突";
    case CAUSE_OUTNETWORK_NUM:
        return (char*)"外网用户";
    case CAUSE_CFU:
        return (char*)"无条件呼叫前转";
    case CAUSE_CFB:
        return (char*)"遇忙呼叫前转";
    case CAUSE_CFNRc:
        return (char*)"不可及呼叫前转";
    case CAUSE_CFNRy:
        return (char*)"无应答呼叫前转";
    case CAUSE_MAX_FWDTIME:
        return (char*)"最大前转次数";
    case CAUSE_OAM_RIGHT:
        return (char*)"OAM操作无权限";
    case CAUSE_DGT_ERROR:
        return (char*)"号码错误";
    case CAUSE_RESOURCE_NOTENOUGH:
        return (char*)"资源不足";
    case CAUSE_ORG_EXPIRE:
        return (char*)"组织到期";
    case CAUSE_USER_EXPIRE:
        return (char*)"用户到期";
    case CAUSE_SAME_ROUTENAME:
        return (char*)"相同的路由名字";
    case CAUSE_UNASSIGNED_ROUTE:
        return (char*)"未分配的路由";
    case CAUSE_OAM_FWD:
        return (char*)"OAM消息前转";
    case CAUSE_UNCFG_MAINNUM:
        return (char*)"未配置主号码";
    case CAUSE_G_NOUSER:
        return (char*)"组中没有用户";
    case CAUSE_U_LOCK_G:
        return (char*)"用户锁定在其他组";
    case CAUSE_U_OFFLINE_G:
        return (char*)"组中没有在线用户";
    case CAUSE_REG_FWD:
        return (char*)"注册前转";
    case CAUSE_PROTOCOL:
        return (char*)"协议错误";
    case CAUSE_SRV_RIGHT:
        return (char*)"无业务权限";
    default:
        snprintf(cBuf, sizeof(cBuf), "CAUSE=%d", usCause);
        return cBuf;
    }
}

//消息码定义
#ifndef MSG_DEFINE
enum
{
    MSG_TRANS = 1,                  //透传消息

    // 心跳消息
    MSG_HB,                         //心跳消息

    // 移动性管理
    MSG_MM_REGREQ           = 0x10, //登记请求
    MSG_MM_REGRSP,                  //登记响应
    MSG_MM_ACCREQ,                  //接入请求
    MSG_MM_ACCRSP,                  //接入响应
    MSG_MM_ROUTEREQ,                //路由请求
    MSG_MM_ROUTERSP,                //路由响应
    MSG_MM_PROFREQ,                 //获取业务信息
    MSG_MM_PROFRSP,                 //获取业务信息响应
    MSG_MM_QUIT,                    //业务退出
    MSG_MM_MODREQ,                  //修改用户信息
    MSG_MM_MODRSP,                  //修改用户信息响应
    MSG_MM_PASSTHROUGH,             //透传通道
    MSG_MM_PROXYREGREQ,             //代理注册
    MSG_MM_PROXYREGRSP,             //代理注册响应
    MSG_MM_NAT,                     //NAT

    //MG消息
    MSG_MG_BINDREQ          = 0x30, //绑定请求
    MSG_MG_BINDRSP,                 //绑定响应
    MSG_MG_IVRREQ,                  //IVR请求
    MSG_MG_IVRRSP,                  //IVR响应
    MSG_MG_CONNREQ,                 //连接请求
    MSG_MG_CONNRSP,                 //连接响应
    MSG_MG_MODIFYREQ,               //修改请求
    MSG_MG_MODIFYRSP,               //修改响应
    MSG_MG_DISCREQ,                 //断开请求
    MSG_MG_DISCRSP,                 //断开响应
    MSG_MG_BINDCLR,                 //绑定清除
    MSG_MG_EVENT,                   //事件,主要是收号
    MSG_MG_STATEIND,                //状态指示
    MSG_MG_INFO,                    //INFO
    MSG_MG_DTMF,                    //DTMF

    //呼叫消息
    MSG_CC_SETUP            = 0x50, //建立
    MSG_CC_SETUPACK,                //建立应答
    MSG_CC_ALERT,                   //振铃
    MSG_CC_CONN,                    //连接
    MSG_CC_CONNACK,                 //连接应答
    MSG_CC_INFO,                    //信息
    MSG_CC_INFOACK,                 //信息应答
    MSG_CC_MODIFY,                  //媒体修改
    MSG_CC_MODIFYACK,               //媒体修改应答
    MSG_CC_REL,                     //释放
    MSG_CC_RLC,                     //释放完成
    MSG_CC_USERCTRL,                //用户控制,加入呼叫/踢出呼叫
    MSG_CC_STREAMCTRL,              //流控制
    MSG_CC_CONFSTATUSREQ,           //会场状态请求
    MSG_CC_CONFSTATUSRSP,           //会场状态响应

    //操作维护
    MSG_OAM_REQ             = 0x70, //操作请求
    MSG_OAM_RSP,                    //操作响应
    MSG_OAM_NOTIFY,                 //OAM操作提示
    MSG_OAM_SETID,                  //设置OAM ID号
    MSG_OAM_CTRL,                   //给OAM模块的控制命令
    MSG_OAM_CROSS_G2U,              //跨节点组到用户消息
    MSG_OAM_CROSS_U2G,              //跨节点用户到组消息
    MSG_OAM_MC2MCC,                 //分控服务器到总控服务器
    MSG_OAM_MCC2MC,                 //总控服务器到分控服务器

    //移动性管理杂项
    MSG_MM_STATUSSUBS       = 0x90, //订阅状态/GPS订阅
    MSG_MM_STATUSNOTIFY,            //状态提示
    MSG_MM_GPSREPORT,               //GPS上报
    MSG_MM_GPSRECIND,               //GPS数据指示
    MSG_MM_GPSHISQUERYREQ,          //GPS历史数据查询请求
    MSG_MM_GPSHISQUERYRSP,          //GPS历史数据查询响应
    MSG_MM_GMEMBEREXTINFO,          //组用户扩展信息
    MSG_MM_NSQUERYREQ,              //存储数据查询请求
    MSG_MM_NSQUERYRSP,              //存储数据查询响应

    //媒体助手等杂项
    MSG_MA_OPENREQ          = 0xb0, //打开请求
    MSG_MA_OPENRSP,                 //打开响应
    MSG_MA_REL,                     //释放

    MSG_MAX
};
#endif


char *GetSrvMsgStr(WORD wMsgId)
{
    static char cBuf[32];

    switch (wMsgId)
    {
    case MSG_TRANS:
        return (char*)"TRANS";

    // 心跳消息
    case MSG_HB:
        return (char*)"HB";

    // 移动性管理
    case MSG_MM_REGREQ:
        return (char*)"MM_REGREQ";
    case MSG_MM_REGRSP:
        return (char*)"MM_REGRSP";
    case MSG_MM_ACCREQ:
        return (char*)"MM_ACCREQ";
    case MSG_MM_ACCRSP:
        return (char*)"MM_ACCRSP";
    case MSG_MM_ROUTEREQ:
        return (char*)"MM_ROUTEREQ";
    case MSG_MM_ROUTERSP:
        return (char*)"MM_ROUTERSP";
    case MSG_MM_PROFREQ:
        return (char*)"MM_PROFREQ";
    case MSG_MM_PROFRSP:
        return (char*)"MM_PROFRSP";
    case MSG_MM_QUIT:
        return (char*)"MM_QUIT";
    case MSG_MM_MODREQ:
        return (char*)"MM_MODREQ";
    case MSG_MM_MODRSP:
        return (char*)"MM_MODRSP";
    case MSG_MM_PASSTHROUGH:
        return (char*)"MM_PASSTHROUGH";
    case MSG_MM_PROXYREGREQ:
        return (char*)"MM_PROXYREGREQ";
    case MSG_MM_PROXYREGRSP:
        return (char*)"MM_PROXYREGRSP";

    //MG消息
    case MSG_MG_BINDREQ:
        return (char*)"MG_BINDREQ";
    case MSG_MG_BINDRSP:
        return (char*)"MG_BINDRSP";
    case MSG_MG_IVRREQ:
        return (char*)"MG_IVRREQ";
    case MSG_MG_IVRRSP:
        return (char*)"MG_IVRRSP";
    case MSG_MG_CONNREQ:
        return (char*)"MG_CONNREQ";
    case MSG_MG_CONNRSP:
        return (char*)"MG_CONNRSP";
    case MSG_MG_MODIFYREQ:
        return (char*)"MG_MODIFYREQ";
    case MSG_MG_MODIFYRSP:
        return (char*)"MG_MODIFYRSP";
    case MSG_MG_DISCREQ:
        return (char*)"MG_DISCREQ";
    case MSG_MG_DISCRSP:
        return (char*)"MG_DISCRSP";
    case MSG_MG_BINDCLR:
        return (char*)"MG_BINDCLR";
    case MSG_MG_EVENT:
        return (char*)"MG_EVENT";
    case MSG_MG_STATEIND:
        return (char*)"MG_STATEIND";
    case MSG_MG_INFO:
        return (char*)"MSG_MG_INFO";
    case MSG_MG_DTMF:
        return (char*)"MSG_MG_DTMF";

    //呼叫消息
    case MSG_CC_SETUP:
        return (char*)"CC_SETUP";
    case MSG_CC_SETUPACK:
        return (char*)"CC_SETUPACK";
    case MSG_CC_ALERT:
        return (char*)"CC_ALERT";
    case MSG_CC_CONN:
        return (char*)"CC_CONN";
    case MSG_CC_CONNACK:
        return (char*)"CC_CONNACK";
    case MSG_CC_INFO:
        return (char*)"CC_INFO";
    case MSG_CC_INFOACK:
        return (char*)"CC_INFOACK";
    case MSG_CC_MODIFY:
        return (char*)"CC_MODIFY";
    case MSG_CC_MODIFYACK:
        return (char*)"CC_MODIFYACK";
    case MSG_CC_REL:
        return (char*)"CC_REL";
    case MSG_CC_RLC:
        return (char*)"CC_RLC";
    case MSG_CC_USERCTRL:
        return (char*)"CC_USERCTRL";
    case MSG_CC_STREAMCTRL:
        return (char*)"CC_STREAMCTRL";
    case MSG_CC_CONFSTATUSREQ:
        return (char*)"CC_CONFSTATUSREQ";
    case MSG_CC_CONFSTATUSRSP:
        return (char*)"CC_CONFSTATUSRSP";

    //操作维护
    case MSG_OAM_REQ:
        return (char*)"OAM_REQ";
    case MSG_OAM_RSP:
        return (char*)"OAM_RSP";
    case MSG_OAM_NOTIFY:
        return (char*)"OAM_NOTIFY";
    case MSG_OAM_SETID:
        return (char*)"OAM_SETID";
    case MSG_OAM_CTRL:
        return (char*)"OAM_CTRL";

    case MSG_MM_STATUSSUBS:
        return (char*)"MM_STATUSSUBS";
    case MSG_MM_STATUSNOTIFY:
        return (char*)"MM_STATUSNOTIFY";
    case MSG_MM_GPSREPORT:
        return (char*)"MM_GPSREPORT";
    case MSG_MM_GPSRECIND:
        return (char*)"MM_GPSRECIND";
    case MSG_MM_GPSHISQUERYREQ:
        return (char*)"MM_GPSHISQUERYREQ";
    case MSG_MM_GPSHISQUERYRSP:
        return (char*)"MM_GPSHISQUERYRSP";
    case MSG_MM_GMEMBEREXTINFO:
        return (char*)"MSG_MM_GMEMBEREXTINFO";
    case MSG_MM_NSQUERYREQ:
        return (char*)"MSG_MM_NSQUERYREQ";
    case MSG_MM_NSQUERYRSP:
        return (char*)"MSG_MM_NSQUERYRSP";

    case MSG_MA_OPENREQ:
        return (char*)"MSG_MA_OPENREQ";
    case MSG_MA_OPENRSP:
        return (char*)"MSG_MA_OPENRSP";
    case MSG_MA_REL:
        return (char*)"MSG_MA_REL";

    default:
        snprintf(cBuf, sizeof(cBuf), "MSGID=%d", wMsgId);
        return cBuf;
    }
}

#ifdef WINDOWS
WORD GetSrvMsgCode(std::string &msg)
{
    if (0 == msg.compare("TRANS"))
    {
        return MSG_TRANS;
    }
    else if (0 == msg.compare("HB"))
    {
        return MSG_HB;
    }
    // 移动性管理
    else if (0 == msg.compare("MM_REGREQ"))
    {
        return MSG_MM_REGREQ;
    }
    else if (0 == msg.compare("MM_REGRSP"))
    {
        return MSG_MM_REGRSP;
    }
    else if (0 == msg.compare("MM_ACCREQ"))
    {
        return MSG_MM_ACCREQ;
    }
    else if (0 == msg.compare("MM_ACCRSP"))
    {
        return MSG_MM_ACCRSP;
    }
    else if (0 == msg.compare("MM_ROUTEREQ"))
    {
        return MSG_MM_ROUTEREQ;
    }
    else if (0 == msg.compare("MM_ROUTERSP"))
    {
        return MSG_MM_ROUTERSP;
    }
    else if (0 == msg.compare("MM_PROFREQ"))
    {
        return MSG_MM_PROFREQ;
    }
    else if (0 == msg.compare("MM_PROFRSP"))
    {
        return MSG_MM_PROFRSP;
    }
    else if (0 == msg.compare("MM_QUIT"))
    {
        return MSG_MM_QUIT;
    }
    else if (0 == msg.compare("MM_MODREQ"))
    {
        return MSG_MM_MODREQ;
    }
    else if (0 == msg.compare("MM_MODRSP"))
    {
        return MSG_MM_MODRSP;
    }
    else if (0 == msg.compare("MM_PASSTHROUGH"))
    {
        return MSG_MM_PASSTHROUGH;
    }
    else if (0 == msg.compare("MM_PROXYREGREQ"))
    {
        return MSG_MM_PROXYREGREQ;
    }
    else if (0 == msg.compare("MM_PROXYREGRSP"))
    {
        return MSG_MM_PROXYREGRSP;
    }

    //MG消息
    else if (0 == msg.compare("MG_BINDREQ"))
    {
        return MSG_MG_BINDREQ;
    }
    else if (0 == msg.compare("MG_BINDRSP"))
    {
        return MSG_MG_BINDRSP;
    }
    else if (0 == msg.compare("MG_IVRREQ"))
    {
        return MSG_MG_IVRREQ;
    }
    else if (0 == msg.compare("MG_IVRRSP"))
    {
        return MSG_MG_IVRRSP;
    }
    else if (0 == msg.compare("MG_CONNREQ"))
    {
        return MSG_MG_CONNREQ;
    }
    else if (0 == msg.compare("MG_CONNRSP"))
    {
        return MSG_MG_CONNRSP;
    }
    else if (0 == msg.compare("MG_MODIFYREQ"))
    {
        return MSG_MG_MODIFYREQ;
    }
    else if (0 == msg.compare("MG_MODIFYRSP"))
    {
        return MSG_MG_MODIFYRSP;
    }
    else if (0 == msg.compare("MG_DISCREQ"))
    {
        return MSG_MG_DISCREQ;
    }
    else if (0 == msg.compare("MG_DISCRSP"))
    {
        return MSG_MG_DISCRSP;
    }
    else if (0 == msg.compare("MG_BINDCLR"))
    {
        return MSG_MG_BINDCLR;
    }
    else if (0 == msg.compare("MG_EVENT"))
    {
        return MSG_MG_EVENT;
    }
    else if (0 == msg.compare("MG_STATEIND"))
    {
        return MSG_MG_STATEIND;
    }

    //呼叫消息
    else if (0 == msg.compare("CC_SETUP"))
    {
        return MSG_CC_SETUP;
    }
    else if (0 == msg.compare("CC_SETUPACK"))
    {
        return MSG_CC_SETUPACK;
    }
    else if (0 == msg.compare("CC_ALERT"))
    {
        return MSG_CC_ALERT;
    }
    else if (0 == msg.compare("CC_CONN"))
    {
        return MSG_CC_CONN;
    }
    else if (0 == msg.compare("CC_CONNACK"))
    {
        return MSG_CC_CONNACK;
    }
    else if (0 == msg.compare("CC_INFO"))
    {
        return MSG_CC_INFO;
    }
    else if (0 == msg.compare("CC_INFOACK"))
    {
        return MSG_CC_INFOACK;
    }
    else if (0 == msg.compare("CC_MODIFY"))
    {
        return MSG_CC_MODIFY;
    }
    else if (0 == msg.compare("CC_MODIFYACK"))
    {
        return MSG_CC_MODIFYACK;
    }
    else if (0 == msg.compare("CC_REL"))
    {
        return MSG_CC_REL;
    }
    else if (0 == msg.compare("CC_RLC"))
    {
        return MSG_CC_RLC;
    }
    else if (0 == msg.compare("CC_USERCTRL"))
    {
        return MSG_CC_USERCTRL;
    }
    else if (0 == msg.compare("CC_STREAMCTRL"))
    {
        return MSG_CC_STREAMCTRL;
    }
    else if (0 == msg.compare("CC_CONFSTATUSREQ"))
    {
        return MSG_CC_CONFSTATUSREQ;
    }
    else if (0 == msg.compare("CC_CONFSTATUSRSP"))
    {
        return MSG_CC_CONFSTATUSRSP;
    }

    //操作维护
    else if (0 == msg.compare("OAM_REQ"))
    {
        return MSG_OAM_REQ;
    }
    else if (0 == msg.compare("OAM_RSP"))
    {
        return MSG_OAM_RSP;
    }
    else if (0 == msg.compare("OAM_NOTIFY"))
    {
        return MSG_OAM_NOTIFY;
    }
    else if (0 == msg.compare("OAM_SETID"))
    {
        return MSG_OAM_SETID;
    }
    else if (0 == msg.compare("OAM_CTRL"))
    {
        return MSG_OAM_CTRL;
    }

    else if (0 == msg.compare("MM_STATUSSUBS"))
    {
        return MSG_MM_STATUSSUBS;
    }
    else if (0 == msg.compare("MM_STATUSNOTIFY"))
    {
        return MSG_MM_STATUSNOTIFY;
    }
    else if (0 == msg.compare("MM_GPSREPORT"))
    {
        return MSG_MM_GPSREPORT;
    }
    else if (0 == msg.compare("MM_GPSRECIND"))
    {
        return MSG_MM_GPSRECIND;
    }
    else if (0 == msg.compare("MM_GPSHISQUERYREQ"))
    {
        return MSG_MM_GPSHISQUERYREQ;
    }
    else if (0 == msg.compare("MM_GPSHISQUERYRSP"))
    {
        return MSG_MM_GPSHISQUERYRSP;
    }
    else if (0 == msg.compare("MSG_MM_GMEMBEREXTINFO"))
    {
        return MSG_MM_GMEMBEREXTINFO;
    }
    else if (0 == msg.compare("MSG_MM_NSQUERYREQ"))
    {
        return MSG_MM_NSQUERYREQ;
    }
    else if (0 == msg.compare("MSG_MM_NSQUERYRSP"))
    {
        return MSG_MM_NSQUERYRSP;
    }
    else if (0 == msg.compare("MSG_MA_OPENREQ"))
    {
        return MSG_MA_OPENREQ;
    }
    else if (0 == msg.compare("MSG_MA_OPENRSP"))
    {
        return MSG_MA_OPENRSP;
    }
    else if (0 == msg.compare("MSG_MA_REL"))
    {
        return MSG_MA_REL;
    }

    return MSG_MAX;
}
#endif

//信息单元码定义
#ifndef MSG_IE_DEFINE
enum
{
    // 01
    MSG_IE_SYSTIME = 1,             //系统时间
    MSG_IE_CAUSE,                   //原因值
    MSG_IE_PLAYINFO,                //播放信息
    MSG_IE_APTYPE,                  //接入协议类型
    MSG_IE_APPOS,                   //接入协议地址
    MSG_IE_APVER,                   //接入协议版本号
    MSG_IE_AUTHALG,                 //鉴权算法
    MSG_IE_AUTHNAME,                //鉴权使用的用户名
    MSG_IE_AUTHNONCE,               //鉴权使用的随机数
    MSG_IE_AUTHREALM,               //鉴权使用的域名

    // 11
    MSG_IE_AUTHRSP,                 //鉴权结果
    MSG_IE_AUTHURI,                 //鉴权使用的URI
    MSG_IE_MYNUM,                   //本端号码
    MSG_IE_PEERNUM,                 //对端号码
    MSG_IE_CONF,                    //会议资源
    MSG_IE_DNSADDR,                 //DNS地址,contact的DNS地址
    MSG_IE_DSTLEGID,                //目的Leg ID
    MSG_IE_EXPIRED,                 //生存时间
    MSG_IE_FROMADDR,                //源地址,contact的IPV4地址
    MSG_IE_IDLIST,                  //ID列表

    // 21
    MSG_IE_INFO,                    //信息
    MSG_IE_LEGID,                   //Leg ID
    MSG_IE_MGFSMID,                 //MG状态机ID
    MSG_IE_MSGCODE,                 //消息码
    MSG_IE_MYSDP,                   //本端SDP
    MSG_IE_PEERSDP,                 //对端SDP
    MSG_IE_OPCODE,                  //操作码
    MSG_IE_RECVNUMTYPE,             //收号方式
    MSG_IE_REGTYPE,                 //注册类型
    MSG_IE_RESULT,                  //结果

    // 31
    MSG_IE_RSPCODE,                 //SIP协议响应码
    MSG_IE_RSPTYPE,                 //响应类型
    MSG_IE_SN,                      //序列号
    MSG_IE_SRCLEGID,                //源Leg ID
    MSG_IE_SRVTYPE,                 //业务类型
    MSG_IE_TOADDR,                  //终发地址
    MSG_IE_TRANSMSGID,              //透传消息ID
    MSG_IE_TRANSMSG,                //透传消息内容
    MSG_IE_USRNUM,                  //用户电话号码
    MSG_IE_USRNAME,                 //用户名

    // 41
    MSG_IE_USRPOS,                  //用户位置信息
    MSG_IE_PWD,                     //密码
    MSG_IE_FWDTIME,                 //前传次数
    MSG_IE_FWDTYPE,                 //前传类型
    MSG_IE_USRGINFO,                //用户所在组信息
    MSG_IE_CAMINFO,                 //摄像头信息
    MSG_IE_ORIGSRVCS,               //始发的CS号码
    MSG_IE_PRIO,                    //优先级
    MSG_IE_IDS,                     //是否是IDS
    MSG_IE_NOTMODSDP,               //是否不要修改修改SDP

    // 51
    MSG_IE_WATCHLEG,                //监控腿
    MSG_IE_CONFNUM,                 //会场号码
    MSG_IE_OPSN,                    //操作序号
    MSG_IE_USERTYPE,                //用户类型
    MSG_IE_USERATTR,                //用户属性
    MSG_IE_USERSTATUS,              //用户状态
    MSG_IE_CONCURRENT,              //并发数
    MSG_IE_USERIPADDR,              //用户IP地址
    MSG_IE_USERADDR,                //用户地址
    MSG_IE_USERCONTACT,             //用户联系方式

    // 61
    MSG_IE_USERDESC,                //用户描述
    MSG_IE_CTIME,                   //创建时间
    MSG_IE_VTIME,                   //到期时间
    MSG_IE_GNUM,                    //组号码
    MSG_IE_GNAME,                   //组名字
    MSG_IE_GNUMU,                   //组内用户个数
    MSG_IE_GMEMBER,                 //组内成员
    MSG_IE_TRANSID,                 //TRANS ID
    MSG_IE_TRUENUM,                 //真实号码,原始主叫号码
    MSG_IE_TRUENAME,                //真实名字,原始主叫名字

    // 71
    MSG_IE_ORIGCALLEDNUM,           //原始被叫号码
    MSG_IE_ORIGCALLEDNAME,          //原始被叫名字
	MSG_IE_IMINFO,
    MSG_IE_QUERYEXT,                //查询使用的附加参数
    MSG_IE_STATUSSUBS,              //状态订阅
    MSG_IE_STATUSNOTIFY,            //状态指示
    MSG_IE_GMEMBERSTATUS,           //组/用户状态
    MSG_IE_GPSREC,                  //GPS记录
    MSG_IE_GPSQUERYEXT,             //GPS历史数据查询条件
    MSG_IE_CALLEXT,                 //呼叫附加信息

    // 81
    MSG_IE_BCSMID,                  //BCSM状态机ID
    MSG_IE_BCSMSTATE,               //BCSM状态
    MSG_IE_CALLCONF,                //呼叫会议信息
    MSG_IE_CALLUSERCTRL,            //呼叫用户控制
    MSG_IE_CALLSTREAMCTRL,          //呼叫流控制
    MSG_IE_PEERNUM_EX,              //扩展的被叫号码                如果是视频网关,这里是4001-1-hkcam-192.168.2.38-8000-admin-admin123这样的信息
    MSG_IE_REFSDP,                  //参考SDP
    MSG_IE_AGNUM,                   //附加的组号码,摄像头组
    MSG_IE_FTPSERVERINFO,           //FTP服务器信息
    MSG_IE_GPSSERVERINFO,           //GPS服务器信息

    // 91
    MSG_IE_WORKINFO,                //WORKINFO字符串
    MSG_IE_ORGLIST,                 //组织列表
    MSG_IE_INVOKENUM,               //发起请求的号码
    MSG_IE_TASKSERVERINFO,          //任务服务器信息
    MSG_IE_USERPROXY,               //用户代理信息
    MSG_IE_CAUSESTR,                //原因字符串
    MSG_IE_ORGLIST_MGR,             //组织列表,管理时用的
    MSG_IE_DATAROLE,                //数据权限
    MSG_IE_MENUROLE,                //菜单权限
    MSG_IE_DEPTNUM,                 //部门号码

    // 101
    MSG_IE_ID,                      //身份证
    MSG_IE_WORKID,                  //工作证
    MSG_IE_WORKUNIT,                //单位
    MSG_IE_TITLE,                   //职务
    MSG_IE_CARID,                   //车牌
    MSG_IE_TEL,                     //电话号码
    MSG_IE_OTHER,                   //其他
    MSG_IE_SOSNUM,                  //SOS号码
    MSG_IE_GPSRECSTR,               //GPS记录,STR格式
    MSG_IE_GTYPE,                   //组类型

    // 111
    MSG_IE_GPSSERVERINFOWSS,        //GPS服务器信息,WSS
    MSG_IE_FWDNUM,                  //前转
    MSG_IE_GMEMBEREXTINFO,          //组用户扩展信息
    MSG_IE_RESREPORT,               //资源报告
    MSG_IE_FSMPAIR,                 //状态机对
    MSG_IE_CALLREFNUM,              //呼叫参考号
    MSG_IE_LEGEXT,                  //腿扩展信息
    MSG_IE_USERMARK,                //用户标志
    MSG_IE_USERCALLREF,             //用户使用的呼叫参考号,主叫是自己,被叫是主叫,查看是对端
    MSG_IE_ROUTECFG,                //路由配置信息

    // 121
    MSG_IE_COMMQUERY,               //通用查询条件
    MSG_IE_MYUG,                    //自己是用户还是组
    MSG_IE_LICENSE_COUNT,           //授权数
    MSG_IE_RUN_COUNT,               //运行数
    MSG_IE_ASSIGN_TYPE,             //MC分配业务组织方式
    MSG_IE_SUBNUM,                  //分机号
    MSG_IE_SLOT,                    //时隙号
    MSG_IE_LIC,                     //授权信息
    MSG_IE_NSQUERYEXT,              //NS查询条件

    MSG_IE_MAX
};
#endif

#if 1
char *GetSrvIEStr(WORD wIEId)
{
    static char cBuf[32];

    switch (wIEId)
    {
    // 01
    case MSG_IE_SYSTIME:
        return (char*)"SysTime";
    case MSG_IE_CAUSE:
        return (char*)"Cause";
    case MSG_IE_PLAYINFO:
        return (char*)"PlayInfo";
    case MSG_IE_APTYPE:
        return (char*)"ApType";
    case MSG_IE_APPOS:
        return (char*)"ApPos";
    case MSG_IE_APVER:
        return (char*)"ApVer";
    case MSG_IE_AUTHALG:
        return (char*)"AuthAlg";
    case MSG_IE_AUTHNAME:
        return (char*)"AuthName";
    case MSG_IE_AUTHNONCE:
        return (char*)"AuthNonce";
    case MSG_IE_AUTHREALM:
        return (char*)"AuthRealm";

    // 11
    case MSG_IE_AUTHRSP:
        return (char*)"AuthRsp";
    case MSG_IE_AUTHURI:
        return (char*)"AuthUri";
    case MSG_IE_MYNUM:
        return (char*)"MyNum";
    case MSG_IE_PEERNUM:
        return (char*)"PeerNum";
    case MSG_IE_CONF:
        return (char*)"Conf";
    case MSG_IE_DNSADDR:
        return (char*)"DnsAddr";
    case MSG_IE_DSTLEGID:
        return (char*)"DstLegID";
    case MSG_IE_EXPIRED:
        return (char*)"Expired";
    case MSG_IE_FROMADDR:
        return (char*)"FromAddr";
    case MSG_IE_IDLIST:
        return (char*)"IdList";

    // 21
    case MSG_IE_INFO:
        return (char*)"Info";
    case MSG_IE_LEGID:
        return (char*)"LegId";
    case MSG_IE_MGFSMID:
        return (char*)"MgFsmID";
    case MSG_IE_MSGCODE:
        return (char*)"MsgCode";
    case MSG_IE_MYSDP:
        return (char*)"MySdp";
    case MSG_IE_PEERSDP:
        return (char*)"PeerSdp";
    case MSG_IE_OPCODE:
        return (char*)"OpCode";
    case MSG_IE_RECVNUMTYPE:
        return (char*)"RecvNumType";
    case MSG_IE_REGTYPE:
        return (char*)"RegType";
    case MSG_IE_RESULT:
        return (char*)"Result";

    // 31
    case MSG_IE_RSPCODE:
        return (char*)"RspCode";
    case MSG_IE_RSPTYPE:
        return (char*)"RspType";
    case MSG_IE_SN:
        return (char*)"SN";
    case MSG_IE_SRCLEGID:
        return (char*)"SrcLegID";
    case MSG_IE_SRVTYPE:
        return (char*)"SrvType";
    case MSG_IE_TOADDR:
        return (char*)"ToAddr";
    case MSG_IE_TRANSMSGID:
        return (char*)"TransMsgID";
    case MSG_IE_TRANSMSG:
        return (char*)"TransMsg";
    case MSG_IE_USRNUM:
        return (char*)"UsrNum";
    case MSG_IE_USRNAME:
        return (char*)"UsrName";

    // 41
    case MSG_IE_USRPOS:
        return (char*)"UsrPos";
    case MSG_IE_PWD:
        return (char*)"Pwd";
    case MSG_IE_FWDTIME:
        return (char*)"FwdTime";
    case MSG_IE_FWDTYPE:
        return (char*)"FwdType";
    case MSG_IE_USRGINFO:
        return (char*)"UsrGInfo";
    case MSG_IE_CAMINFO:
        return (char*)"CamInfo";
    case MSG_IE_ORIGSRVCS:
        return (char*)"OrigSrvCS";
    case MSG_IE_PRIO:
        return (char*)"Prio";
    case MSG_IE_IDS:
        return (char*)"IDS";
    case MSG_IE_NOTMODSDP:
        return (char*)"NotModSdp";

    // 51
    case MSG_IE_WATCHLEG:
        return (char*)"WatchLeg";
    case MSG_IE_CONFNUM:
        return (char*)"ConfNum";
    case MSG_IE_OPSN:
        return (char*)"OpSN";
    case MSG_IE_USERTYPE:
        return (char*)"UserType";
    case MSG_IE_USERATTR:
        return (char*)"UserAttr";
    case MSG_IE_USERSTATUS:
        return (char*)"UserStatus";
    case MSG_IE_CONCURRENT:
        return (char*)"ConCurrent";
    case MSG_IE_USERIPADDR:
        return (char*)"UserIPAddr";
    case MSG_IE_USERADDR:
        return (char*)"UserAddr";
    case MSG_IE_USERCONTACT:
        return (char*)"UserContact";

    // 61
    case MSG_IE_USERDESC:
        return (char*)"UserDesc";
    case MSG_IE_CTIME:
        return (char*)"CTime";
    case MSG_IE_VTIME:
        return (char*)"VTime";
    case MSG_IE_GNUM:
        return (char*)"GNum";
    case MSG_IE_GNAME:
        return (char*)"GName";
    case MSG_IE_GNUMU:
        return (char*)"GNumU";
    case MSG_IE_GMEMBER:
        return (char*)"GMember";
    case MSG_IE_TRANSID:
        return (char*)"TransID";
    case MSG_IE_TRUENUM:
        return (char*)"TrueNum";
    case MSG_IE_TRUENAME:
        return (char*)"TrueName";

    // 71
    case MSG_IE_ORIGCALLEDNUM:
        return (char*)"OrigCalledNum";
    case MSG_IE_ORIGCALLEDNAME:
        return (char*)"OrigCalledName";
    case MSG_IE_IMINFO:
        return (char*)"ImInfo";
    case MSG_IE_QUERYEXT:
        return (char*)"QueryExt";
    case MSG_IE_STATUSSUBS:
        return (char*)"StatusSubs";
    case MSG_IE_STATUSNOTIFY:
        return (char*)"StatusNotify";
    case MSG_IE_GMEMBERSTATUS:
        return (char*)"GMemberStatus";
    case MSG_IE_GPSREC:
        return (char*)"GpsRec";
    case MSG_IE_GPSQUERYEXT:
        return (char*)"GpsQueryExt";
    case MSG_IE_CALLEXT:
        return (char*)"CallExt";

    // 81
    case MSG_IE_BCSMID:
        return (char*)"BcsmID";
    case MSG_IE_BCSMSTATE:
        return (char*)"BcsmState";
    case MSG_IE_CALLCONF:
        return (char*)"CallConf";
    case MSG_IE_CALLUSERCTRL:
        return (char*)"CallUserCtrl";
    case MSG_IE_CALLSTREAMCTRL:
        return (char*)"CallStreamCtrl";
    case MSG_IE_PEERNUM_EX:
        return (char*)"PeerNumEx";
    case MSG_IE_REFSDP:
        return (char*)"RefSdp";
    case MSG_IE_AGNUM:
        return (char*)"AGNum";
    case MSG_IE_FTPSERVERINFO:
        return (char*)"FtpServerInfo";
    case MSG_IE_GPSSERVERINFO:
        return (char*)"GpsServerInfo";

    // 91
    case MSG_IE_WORKINFO:
        return (char*)"WorkInfo";
    case MSG_IE_ORGLIST:
        return (char*)"OrgList";
    case MSG_IE_INVOKENUM:
        return (char*)"InvokeNum";
    case MSG_IE_TASKSERVERINFO:
        return (char*)"TaskServerInfo";
    case MSG_IE_USERPROXY:
        return (char*)"UserProxy";
    case MSG_IE_CAUSESTR:
        return (char*)"CauseStr";
    case MSG_IE_ORGLIST_MGR:
        return (char*)"OrgListMgr";
    case MSG_IE_DATAROLE:
        return (char*)"DataRole";
    case MSG_IE_MENUROLE:
        return (char*)"MenuRole";
    case MSG_IE_DEPTNUM:
        return (char*)"DeptNum";

    // 101
    case MSG_IE_ID:
        return (char*)"ID";
    case MSG_IE_WORKID:
        return (char*)"WorkID";
    case MSG_IE_WORKUNIT:
        return (char*)"WorkUnit";
    case MSG_IE_TITLE:
        return (char*)"Title";
    case MSG_IE_CARID:
        return (char*)"CarID";
    case MSG_IE_TEL:
        return (char*)"Tel";
    case MSG_IE_OTHER:
        return (char*)"Other";
    case MSG_IE_SOSNUM:
        return (char*)"SosNum";
    case MSG_IE_GPSRECSTR:
        return (char*)"GpsRecStr";
    case MSG_IE_GTYPE:
        return (char*)"GType";

    // 111
    case MSG_IE_GPSSERVERINFOWSS:
        return (char*)"GpsServerInfoWss";
    case MSG_IE_FWDNUM:
        return (char*)"FdwNum";
    case MSG_IE_GMEMBEREXTINFO:
        return (char*)"GMemberExtInfo";
    case MSG_IE_RESREPORT://资源报告
        return (char*)"ResReport";
    case MSG_IE_FSMPAIR://状态机对
        return (char*)"FsmPair";
    case MSG_IE_CALLREFNUM://呼叫参考号
        return (char*)"CallRefNum";
    case MSG_IE_LEGEXT://腿扩展信息
        return (char*)"LegExt";
    case MSG_IE_USERMARK://用户标志
        return (char*)"UserMark";
    case MSG_IE_USERCALLREF://用户使用的呼叫参考号,主叫是自己,被叫是主叫,查看是对端
        return (char*)"UserCallRef";
    case MSG_IE_ROUTECFG://路由配置信息
        return (char*)"RouteCfg";
    case MSG_IE_COMMQUERY://通用查询
        return (char*)"CommQuery";
    case MSG_IE_MYUG://自己是用户还是组
        return (char*)"MyUG";
    case MSG_IE_LICENSE_COUNT://授权数
        return (char*)"LicCount";
    case MSG_IE_RUN_COUNT://运行数
        return (char*)"RunCount";
    case MSG_IE_ASSIGN_TYPE://MC分配业务组织方式
        return (char*)"AssignType";
    case MSG_IE_SUBNUM://分机号
        return (char*)"SubNum";
    case MSG_IE_SLOT://时隙号
        return (char*)"Slot";
    case MSG_IE_LIC://授权信息
        return (char*)"Lic";
    case MSG_IE_NSQUERYEXT://NS查询条件
        return (char*)"NsQueryExt";

    default:
        snprintf(cBuf, sizeof(cBuf), "IEId=%d", wIEId);
        return cBuf;
    }
}
#endif
#if 0
char *GetSrvIEStr(WORD wIEId)
{
    static char cBuf[32];

    switch (wIEId)
    {
    // 01
    case MSG_IE_SYSTIME:
        return (char*)"SYSTIME";
    case MSG_IE_CAUSE:
        return (char*)"CAUSE";
    case MSG_IE_PLAYINFO:
        return (char*)"PLAYINFO";
    case MSG_IE_APTYPE:
        return (char*)"APTYPE";
    case MSG_IE_APPOS:
        return (char*)"APPOS";
    case MSG_IE_APVER:
        return (char*)"APVER";
    case MSG_IE_AUTHALG:
        return (char*)"AUTHALG";
    case MSG_IE_AUTHNAME:
        return (char*)"AUTHNAME";
    case MSG_IE_AUTHNONCE:
        return (char*)"AUTHNONCE";
    case MSG_IE_AUTHREALM:
        return (char*)"AUTHREALM";

    // 11
    case MSG_IE_AUTHRSP:
        return (char*)"AUTHRSP";
    case MSG_IE_AUTHURI:
        return (char*)"AUTHURI";
    case MSG_IE_MYNUM:
        return (char*)"MYNUM";
    case MSG_IE_PEERNUM:
        return (char*)"PEERNUM";
    case MSG_IE_CONF:
        return (char*)"CONF";
    case MSG_IE_DNSADDR:
        return (char*)"DNSADDR";
    case MSG_IE_DSTLEGID:
        return (char*)"DSTLEGID";
    case MSG_IE_EXPIRED:
        return (char*)"EXPIRED";
    case MSG_IE_FROMADDR:
        return (char*)"FROMADDR";
    case MSG_IE_IDLIST:
        return (char*)"IDLIST";

    // 21
    case MSG_IE_INFO:
        return (char*)"INFO";
    case MSG_IE_LEGID:
        return (char*)"LEGID";
    case MSG_IE_MGFSMID:
        return (char*)"MGFSMID";
    case MSG_IE_MSGCODE:
        return (char*)"MSGCODE";
    case MSG_IE_MYSDP:
        return (char*)"MYSDP";
    case MSG_IE_PEERSDP:
        return (char*)"PEERSDP";
    case MSG_IE_OPCODE:
        return (char*)"OPCODE";
    case MSG_IE_RECVNUMTYPE:
        return (char*)"RECVNUMTYPE";
    case MSG_IE_REGTYPE:
        return (char*)"REGTYPE";
    case MSG_IE_RESULT:
        return (char*)"RESULT";

    // 31
    case MSG_IE_RSPCODE:
        return (char*)"RSPCODE";
    case MSG_IE_RSPTYPE:
        return (char*)"RSPTYPE";
    case MSG_IE_SN:
        return (char*)"SN";
    case MSG_IE_SRCLEGID:
        return (char*)"SRCLEGID";
    case MSG_IE_SRVTYPE:
        return (char*)"SRVTYPE";
    case MSG_IE_TOADDR:
        return (char*)"TOADDR";
    case MSG_IE_TRANSMSGID:
        return (char*)"TRANSMSGID";
    case MSG_IE_TRANSMSG:
        return (char*)"TRANSMSG";
    case MSG_IE_USRNUM:
        return (char*)"USRNUM";
    case MSG_IE_USRNAME:
        return (char*)"USRNAME";

    // 41
    case MSG_IE_USRPOS:
        return (char*)"USRPOS";
    case MSG_IE_PWD:
        return (char*)"PWD";
    case MSG_IE_FWDTIME:
        return (char*)"FWDTIME";
    case MSG_IE_FWDTYPE:
        return (char*)"FWDTYPE";
    case MSG_IE_USRGINFO:
        return (char*)"USRGINFO";
    case MSG_IE_CAMINFO:
        return (char*)"CAMINFO";
    case MSG_IE_ORIGSRVCS:
        return (char*)"ORIGSRVCS";
    case MSG_IE_PRIO:
        return (char*)"PRIO";
    case MSG_IE_IDS:
        return (char*)"IDS";
    case MSG_IE_NOTMODSDP:
        return (char*)"NOTMODSDP";

    // 51
    case MSG_IE_WATCHLEG:
        return (char*)"WATCHLEG";
    case MSG_IE_CONFNUM:
        return (char*)"CONFNUM";
    case MSG_IE_OPSN:
        return (char*)"OPSN";
    case MSG_IE_USERTYPE:
        return (char*)"USERTYPE";
    case MSG_IE_USERATTR:
        return (char*)"USERATTR";
    case MSG_IE_USERSTATUS:
        return (char*)"USERSTATUS";
    case MSG_IE_CONCURRENT:
        return (char*)"CONCURRENT";
    case MSG_IE_USERIPADDR:
        return (char*)"USERIPADDR";
    case MSG_IE_USERADDR:
        return (char*)"USERADDR";
    case MSG_IE_USERCONTACT:
        return (char*)"USERCONTACT";

    // 61
    case MSG_IE_USERDESC:
        return (char*)"USERDESC";
    case MSG_IE_CTIME:
        return (char*)"CTIME";
    case MSG_IE_VTIME:
        return (char*)"VTIME";
    case MSG_IE_GNUM:
        return (char*)"GNUM";
    case MSG_IE_GNAME:
        return (char*)"GNAME";
    case MSG_IE_GNUMU:
        return (char*)"GNUMU";
    case MSG_IE_GMEMBER:
        return (char*)"GMEMBER";
    case MSG_IE_TRANSID:
        return (char*)"TRANSID";
    case MSG_IE_TRUENUM:
        return (char*)"TRUENUM";
    case MSG_IE_TRUENAME:
        return (char*)"TRUENAME";

    // 71
    case MSG_IE_ORIGCALLEDNUM:
        return (char*)"ORIGCALLEDNUM";
    case MSG_IE_ORIGCALLEDNAME:
        return (char*)"ORIGCALLEDNAME";
    case MSG_IE_IMINFO:
        return (char*)"IMINFO";
    case MSG_IE_QUERYEXT:
        return (char*)"QUERYEXT";
    case MSG_IE_STATUSSUBS:
        return (char*)"STATUSSUBS";
    case MSG_IE_STATUSNOTIFY:
        return (char*)"STATUSNOTIFY";
    case MSG_IE_GMEMBERSTATUS:
        return (char*)"GMEMBERSTATUS";
    case MSG_IE_GPSREC:
        return (char*)"GPSREC";
    case MSG_IE_GPSQUERYEXT:
        return (char*)"GPSQUERYEXT";
    case MSG_IE_CALLEXT:
        return (char*)"CALLEXT";

    // 81
    case MSG_IE_BCSMID:
        return (char*)"BCSMID";
    case MSG_IE_BCSMSTATE:
        return (char*)"BCSMSTATE";
    case MSG_IE_CALLCONF:
        return (char*)"CALLCONF";
    case MSG_IE_CALLUSERCTRL:
        return (char*)"CALLUSERCTRL";
    case MSG_IE_CALLSTREAMCTRL:
        return (char*)"CALLSTREAMCTRL";
    case MSG_IE_PEERNUM_EX:
        return (char*)"PEERNUM_EX";
    case MSG_IE_REFSDP:
        return (char*)"REFSDP";
    case MSG_IE_AGNUM:
        return (char*)"AGNUM";
    case MSG_IE_FTPSERVERINFO:
        return (char*)"FTPSERVERINFO";
    case MSG_IE_GPSSERVERINFO:
        return (char*)"GPSSERVERINFO";

    // 91
    case MSG_IE_WORKINFO:
        return (char*)"WORKINFO";
    case MSG_IE_ORGLIST:
        return (char*)"ORGLIST";
    case MSG_IE_INVOKENUM:
        return (char*)"INVOKENUM";
    case MSG_IE_TASKSERVERINFO:
        return (char*)"TASKSERVERINFO";
    case MSG_IE_USERPROXY:
        return (char*)"USERPROXY";
    case MSG_IE_CAUSESTR:
        return (char*)"CAUSESTR";
    case MSG_IE_ORGLIST_MGR:
        return (char*)"ORGLIST_MGR";
    case MSG_IE_DATAROLE:
        return (char*)"DATAROLE";
    case MSG_IE_MENUROLE:
        return (char*)"MENUROLE";
    case MSG_IE_DEPTNUM:
        return (char*)"DEPTNUM";

    // 101
    case MSG_IE_ID:
        return (char*)"ID";
    case MSG_IE_WORKID:
        return (char*)"WORKID";
    case MSG_IE_WORKUNIT:
        return (char*)"WORKUNIT";
    case MSG_IE_TITLE:
        return (char*)"TITLE";
    case MSG_IE_CARID:
        return (char*)"CARID";
    case MSG_IE_TEL:
        return (char*)"TEL";
    case MSG_IE_OTHER:
        return (char*)"OTHER";

    default:
        snprintf(cBuf, sizeof(cBuf), "IEId=%d", wIEId);
        return cBuf;
    }
}
#endif


char *GetOamOptStr(DWORD dwOpt)
{
    static char cBuf[32];
    switch (dwOpt)
    {
    case OPT_USER_ADD:
        return (char*)"OPT_USER_ADD";
    case OPT_USER_DEL:
        return (char*)"OPT_USER_DEL";
    case OPT_USER_MODIFY:
        return (char*)"OPT_USER_MODIFY";
    case OPT_USER_QUERY:
        return (char*)"OPT_USER_QUERY";
    case OPT_G_ADD:
        return (char*)"OPT_G_ADD";
    case OPT_G_DEL:
        return (char*)"OPT_G_DEL";
    case OPT_G_MODIFY:
        return (char*)"OPT_G_MODIFY";
    case OPT_G_QUERY:
        return (char*)"OPT_G_QUERY";
    case OPT_G_ADDUSER:
        return (char*)"OPT_G_ADDUSER";
    case OPT_G_DELUSER:
        return (char*)"OPT_G_DELUSER";
    case OPT_G_MODIFYUSER:
        return (char*)"OPT_G_MODIFYUSER";
    case OPT_G_QUERYUSER:
        return (char*)"OPT_G_QUERYUSER";
    case OPT_U_QUERYGROUP:
        return (char*)"OPT_U_QUERYGROUP";
    case OPT_O_ADD:
        return (char*)"OPT_O_ADD";
    case OPT_O_DEL:
        return (char*)"OPT_O_DEL";
    case OPT_O_MODIFY:
        return (char*)"OPT_O_MODIFY";
    case OPT_O_QUERY:
        return (char*)"OPT_O_QUERY";
    case OPT_GMEMBER_EXTINFO:
        return (char*)"OPT_GMEMBER_EXTINFO";
    case OPT_R_ADD:
        return (char*)"OPT_R_ADD";
    case OPT_R_DEL:
        return (char*)"OPT_R_DEL";
    case OPT_R_MODIFY:
        return (char*)"OPT_R_MODIFY";
    case OPT_R_QUERY:
        return (char*)"OPT_R_QUERY";
    case OPT_CROSS_G2U:
        return (char*)"OPT_CROSS_G2U";
    case OPT_CROSS_U2G:
        return (char*)"OPT_CROSS_U2G";
    case OPT_CROSS_STATUS_U_2LOW:
        return (char*)"OPT_CROSS_STATUS_U_2LOW";
    case OPT_CROSS_STATUS_G_2LOW:
        return (char*)"OPT_CROSS_STATUS_G_2LOW";
    case OPT_CROSS_STATUS_U_2HIGH:
        return (char*)"OPT_CROSS_STATUS_U_2HIGH";
    default:
        snprintf(cBuf, sizeof(cBuf), "OPT=%d", (int)dwOpt);
        return cBuf;
    }
}


char *GetSrvTypeStr(SRV_TYPE_e SrvType)
{
    static char cBuf[32];
    switch (SrvType)
    {
    case SRV_TYPE_NONE:
        return (char*)"SRV_TYPE_NONE";
    case SRV_TYPE_BASIC_CALL:
        return (char*)"SRV_TYPE_BASIC_CALL";
    case SRV_TYPE_CONF:
        return (char*)"SRV_TYPE_CONF";
    case SRV_TYPE_CONF_JOIN:
        return (char*)"SRV_TYPE_CONF_JOIN";
    case SRV_TYPE_FORCE_INJ:
        return (char*)"SRV_TYPE_FORCE_INJ";
    case SRV_TYPE_FORCE_REL:
        return (char*)"SRV_TYPE_FORCE_REL";
    case SRV_TYPE_WATCH_DOWN:
        return (char*)"SRV_TYPE_WATCH_DOWN";
    case SRV_TYPE_WATCH_UP:
        return (char*)"SRV_TYPE_WATCH_UP";
    case SRV_TYPE_NS_CALL:
        return (char*)"SRV_TYPE_NS_CALL";
    case SRV_TYPE_SIMP_CALL:
        return (char*)"SRV_TYPE_SIMP_CALL";
    case SRV_TYPE_IM:
        return (char*)"SRV_TYPE_IM";
    case SRV_TYPE_MULTICALL:
        return (char*)"SRV_TYPE_MULTICALL";
    case SRV_TYPE_OAM:
        return (char*)"SRV_TYPE_OAM";
    case SRV_TYPE_GPS:
        return (char*)"SRV_TYPE_GPS";
    case SRV_TYPE_MAX:
        return (char*)"SRV_TYPE_MAX";
    default:
        snprintf(cBuf, sizeof(cBuf), "SRV_TYPE=%d", SrvType);
        return cBuf;
    }
}

char *GetImCodeStr(UCHAR ucCode)
{
    static char cBuf[32];
    switch (ucCode)
    {
    case PTE_CODE_TXREQ://发送请求
        return (char*)"发送请求";
    case PTE_CODE_TXCFM:
        return (char*)"传输确认";
    case PTE_CODE_USRREAD:
        return (char*)"用户阅读";
    case PTE_CODE_USRREADCFM:
        return (char*)"用户阅读消息的确认";
    case PTE_CODE_FILENAMEREQ:
        return (char*)"文件名请求";
    case PTE_CODE_FILENAMERSP:
        return (char*)"文件名响应";
    default:
        snprintf(cBuf, sizeof(cBuf), "IMCode=%d", ucCode);
        return cBuf;
    }
}

char *GetImTypeStr(DWORD dwType)
{
    static char cBuf[32];
    switch (dwType)
    {
    case IM_TYPE_NONE:
        return (char*)"无";
//01~7f需要存储转发,用户确认收到的
    case IM_TYPE_TXT:
        return (char*)"文本";
    case IM_TYPE_GPS:
        return (char*)"GPS";
    case IM_TYPE_IMAGE:
        return (char*)"图像";
    case IM_TYPE_AUDIO:
        return (char*)"语音文件";
    case IM_TYPE_VIDEO:
        return (char*)"视频文件";
//0x80~0xff不需要存储转发,用户确认接收的
    case IM_TYPE_NSSUBS:
        return (char*)"存储订阅";
    case IM_TYPE_NSQUERYREQ:
        return (char*)"存储查询";
    case IM_TYPE_NSQUERYRSP:
        return (char*)"存储查询响应";
    case IM_TYPE_USER:
        return (char*)"用户自定义";
    default:
        snprintf(cBuf, sizeof(cBuf), "IMType=%d", (int)dwType);
        return cBuf;
    }
}

char *GetGUTypeStr(UCHAR ucType)
{
    static char cBuf[32];
    switch (ucType)
    {
    case GROUP_MEMBERTYPE_USER:
        return (char*)"用户";
    case GROUP_MEMBERTYPE_GROUP:
        return (char*)"组";
    case GROUP_MEMBERTYPE_USER_OUT:
        return (char*)"外网用户";
    case GROUP_MEMBERTYPE_USER_SLOT2:
        return (char*)"时隙2用户";

    default:
        snprintf(cBuf, sizeof(cBuf), "GUType=%d", ucType);
        return cBuf;
    }
}

char *GetStatusStr(UCHAR ucStatus)
{
    static char cBuf[32];
    switch (ucStatus)
    {
    case UT_STATUS_OFFLINE:
        return (char*)"离线";
    case UT_STATUS_ONLINE:
        return (char*)"在线";
    default:
        snprintf(cBuf, sizeof(cBuf), "Status=%d", ucStatus);
        return cBuf;
    }
}

char *GetCallOTStr(UCHAR ucOT)
{
    static char cBuf[32];
    switch (ucOT)
    {
    case CC_O:
        return (char*)"主叫";
    case CC_T:
        return (char*)"被叫";
    default:
        snprintf(cBuf, sizeof(cBuf), "OT=%d", ucOT);
        return cBuf;
    }
}

char *GetCallTypeStr(UCHAR ucCallType)
{
    static char cBuf[32];
    switch (ucCallType)
    {
    case SRV_TYPE_BASIC_CALL:
        return (char*)"基本呼叫";
    case SRV_TYPE_CONF:
        return (char*)"会议";
    case SRV_TYPE_CONF_JOIN:
        return (char*)"会议";
    case SRV_TYPE_FORCE_INJ:
        return (char*)"强插";
    case SRV_TYPE_FORCE_REL:
        return (char*)"强拆";
    case SRV_TYPE_WATCH_DOWN:
        return (char*)"监控下载";
    case SRV_TYPE_WATCH_UP:
        return (char*)"监控上传";
    case SRV_TYPE_NS_CALL:
        return (char*)"存储呼叫";
    case SRV_TYPE_SIMP_CALL:
        return (char*)"单工呼叫";
    default:
        snprintf(cBuf, sizeof(cBuf), "CallType=%d", ucCallType);
        return cBuf;
    }
}

char *GetCallStatusStr(UCHAR ucCallStatus)
{
    static char cBuf[32];
    switch (ucCallStatus)
    {
    case GU_STATUSCALL_IDLE:
        return (char*)"空闲";
    case GU_STATUSCALL_OALERT:
        return (char*)"主叫回铃";
    case GU_STATUSCALL_TALERT:
        return (char*)"被叫振铃";
    case GU_STATUSCALL_TALKING:
        return (char*)"通话";
    case GU_STATUSCALL_G_TALKING:
        return (char*)"会议讲话";
    case GU_STATUSCALL_G_LISTEN:
        return (char*)"会议听话";
    default:
        snprintf(cBuf, sizeof(cBuf), "CallStatus=%d", ucCallStatus);
        return cBuf;
    }
}


char *GetWatchDir(UCHAR ucDir)
{
    static char cBuf[32];
    switch (ucDir)
    {
    case WATCH_DIR_NONE:
        return (char*)"没有";
    case WATCH_DIR_UP:
        return (char*)"上行";
    case WATCH_DIR_DOWN:
        return (char*)"下行";
    case WATCH_DIR_BOTH:
        return (char*)"双向";
    default:
        snprintf(cBuf, sizeof(cBuf), "方向=%d", ucDir);
        return cBuf;
    }
}

char *GetInfoStr(DWORD dwInfo)
{
    UCHAR ucCode    = dwInfo & 0xff;
    UCHAR ucQueue   = (dwInfo & 0xff00) >> 8;

    static char cBuf[32];
    switch (ucCode)
    {
    case SRV_INFO_MICREQ:
        snprintf(cBuf, sizeof(cBuf), "话权申请:队列%d", ucQueue);
        break;
    case SRV_INFO_MICREL:
        snprintf(cBuf, sizeof(cBuf), "话权释放:队列%d", ucQueue);
        break;
    case SRV_INFO_MICGIVE:
        snprintf(cBuf, sizeof(cBuf), "给与话权:队列%d", ucQueue);
        break;
    case SRV_INFO_MICTAKE:
        snprintf(cBuf, sizeof(cBuf), "收回话权:队列%d", ucQueue);
        break;
    case SRV_INFO_MICINFO:
        snprintf(cBuf, sizeof(cBuf), "话权提示:队列%d", ucQueue);
        break;
    case SRV_INFO_AUTOMICON:
        snprintf(cBuf, sizeof(cBuf), "打开自由发言:队列%d", ucQueue);
        break;
    case SRV_INFO_AUTOMICOFF:
        snprintf(cBuf, sizeof(cBuf), "关闭自由发言:队列%d", ucQueue);
        break;
    case SRV_INFO_CAMHISPLAY:
        snprintf(cBuf, sizeof(cBuf), "播放历史视频");
        break;
    case SRV_INFO_TALKON:
        snprintf(cBuf, sizeof(cBuf), "打开对讲");
        break;
    case SRV_INFO_TALKOFF:
        snprintf(cBuf, sizeof(cBuf), "关闭对讲");
        break;
    case SRV_INFO_TALKONRSP:
        snprintf(cBuf, sizeof(cBuf), "打开对讲响应");
        break;
    case SRV_INFO_TALKOFFRSP:
        snprintf(cBuf, sizeof(cBuf), "关闭对讲响应");
        break;
    case SRV_INFO_NSREC:
        snprintf(cBuf, sizeof(cBuf), "NS存储记录");
        break;
    case SRV_INFO_ICECAND:
        snprintf(cBuf, sizeof(cBuf), "IceCandidate");
        break;
    case SRV_INFO_ICESTATE:
        snprintf(cBuf, sizeof(cBuf), "IceStateChange");
        break;
    case SRV_INFO_SDP:
        snprintf(cBuf, sizeof(cBuf), "本端SDP传送");
        break;
    case SRV_INFO_CAMCTRL:
        snprintf(cBuf, sizeof(cBuf), "摄像头控制");
        break;
    case SRV_INFO_NUM:
        snprintf(cBuf, sizeof(cBuf), "号码");
        break;
    case SRV_INFO_USERMARK:
        snprintf(cBuf, sizeof(cBuf), "用户标识");
        break;
    case SRV_INFO_CALLREF:
        snprintf(cBuf, sizeof(cBuf), "呼叫流水号");
        break;
    case SRV_INFO_MICMODIFY:
        snprintf(cBuf, sizeof(cBuf), "话筒修改");
        break;
    case SRV_INFO_NUM_DTMF:
        snprintf(cBuf, sizeof(cBuf), "DTMF");
        break;
    default:
        snprintf(cBuf, sizeof(cBuf), "Info=%d:%d", ucCode, ucQueue);
        break;
    }
    return cBuf;
}


