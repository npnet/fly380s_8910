#ifndef _PUBSRV_DEF_H_
#define _PUBSRV_DEF_H_

//全局宏定义
#define USR_SHORT_NUMMAXLEN     8   //组内用户最大短号长度
//字符串长度:
// 1.32字节，号码，参考号(当前时间+序号,包括呼叫参考号等唯一标识，在一个机器内唯一，不同机器不唯一)
typedef UCHAR UC_32[32];
// 2.64字节，用户名，密码，鉴权参数
typedef UCHAR UC_64[64];
// 3.128字节，用户描述，其他描述，DNS
typedef UCHAR UC_128[128];
typedef UCHAR UC_256[256];
typedef UCHAR UC_512[512];
typedef UCHAR UC_1024[1024];
typedef UCHAR UC_1280[1280];

typedef UCHAR UC_TIMESTR[20];   //%04d-%02d-%02d %02d:%02d:%02d         2015-10-01 12:15:35
typedef UCHAR UC_IPSTR[16];     //xxx.xxx.xxx.xxx
typedef UCHAR UC_IPPORTSTR[22]; //xxx.xxx.xxx.xxx:xxxxx

//组织类型
#define O_TYPE_TRIAL        0       //试用
#define O_TYPE_FOMAL        1       //正式
#define O_TYPE_AGENT        2       //代理商

//组/用户内网网类型,最高位表示
#define GU_TYPE_LOCAL       0x00
#define GU_TYPE_CROSS       0x80

//组类型
#define G_TYPE_FOMAL        1       //正式组
#define G_TYPE_TEMPORARY    2       //临时组
#define G_TYPE_OUTGROUP     3       //外网组
#define G_TYPE_TRUNK        4       //呼叫使用的中继组,不作为OAM使用

//终端类型
//最高位,GU_TYPE_LOCAL,表示本节点用户;GU_TYPE_CROSS,表示其他节点用户
#define UT_TYPE_NONE        0       //无效
// 1~0x3f:       标准终端
#define UT_TYPE_SIP         0x01    //标准SIP终端                   1
#define UT_TYPE_SIPEX       0x02    //支持扩展SIP接口的终端         2
#define UT_TYPE_SIPEX_JNZJT 0x03    //玖诺中继台(中心台)            3
#define UT_TYPE_SUB         0x04    //网关下属分机,玖诺中继台下属终端 4

//0x40~0x5f:    使用TAP协议的终端
#define UT_TYPE_TAP         0x40    //通用TAP终端                   64
#define UT_TYPE_DISPATCH    0x41    //调度台                        65
#define UT_TYPE_VGW         0x42    //视频网关                      66
#define UT_TYPE_NS          0x43    //存储                          67
#define UT_TYPE_ORGADMIN    0x44    //组织管理员                    68
#define UT_TYPE_GPSONLY     0x45    //只有GPS功能的终端             69
#define UT_TYPE_28181       0x46    //使用28181协议的平台或终端     70
#define UT_TYPE_OUT         0x47    //外网用户                      71
#define UT_TYPE_MANAGER     0x48    //代理商管理员                  72

//0x60~0x6f:    其他厂家终端
//ucCamType
#define UT_TYPE_AHCS        0x60    //安徽创世摄像头                96
#define UT_TYPE_HK          0x61    //海康摄像头                    97
#define UT_TYPE_FSAN        0x62    //富视安摄像头                  98
#define UT_TYPE_HKPLAT      0x63    //海康平台                      99
#define UT_TYPE_DHPLAT      0x64    //大华平台                      100
#define UT_TYPE_BTX         0x65    //BTX布控球                     101
#define UT_TYPE_TDWY        0x66    //天地伟业                      102
#define UT_TYPE_DH          0x67    //大华                          103

//内部使用的终端类型，不在接口体现,0x70~0x7f
#define UT_TYPE_WEBRTC      0x70    //标准WEBRTC接口                240

//终端属性,使用场合,显示用
#define UT_ATTR_NONE        0       //无效
#define UT_ATTR_PC          1       //PC机
#define UT_ATTR_FIXED       2       //固定台
#define UT_ATTR_HS          3       //手持机
#define UT_ATTR_VEHICLE     4       //车载台
#define UT_ATTR_GW          5       //网关
#define UT_ATTR_ADMIN       0x10    //管理员                        16
#define UT_ATTR_DS          0x11    //调度台                        17
#define UT_ATTR_MS          0x12    //手持机                        18
#define UT_ATTR_IPPHONE     0x13    //IP话机                        19
#define UT_ATTR_GATEWAY     0x14    //网关                          20
#define UT_ATTR_FIXCAM      0x15    //固定摄像头                    21
#define UT_ATTR_DISPATCH_SIMPLE 0x16//只看见自己组内用户的调度员    22
#define UT_ATTR_P_VEHICLE   0x30    //警车                          48
#define UT_ATTR_F_VEHICLE   0x31    //消防车                        49
#define UT_ATTR_E_VEHICLE   0x32    //工程车                        50
#define UT_ATTR_TAXI        0x33    //出租车                        51
#define UT_ATTR_TRUCK       0x34    //卡车                          52
#define UT_ATTR_P_MOTO      0x35    //警用摩托车                    53

//0x60~0x7f:    其他厂家终端,与UT_TYPE_一致
#define UT_ATTR_HKPLAT      0x63    //海康平台                      99
#define UT_ATTR_DHPLAT      0x64    //大华平台                      100
#define UT_ATTR_BTX         0x65    //BTX布控球                     101
#define UT_ATTR_TDWY        0x66    //天地伟业                      102
#define UT_ATTR_DH          0x67    //大华                          103

#define UT_ATTR_GROUP       0x80    //是组,在OAM操作中使用          128

#define UT_ATTR_PC_2        200     //PC
#define UT_ATTR_IHS         201     //智能手机
#define UT_ATTR_HS_NOSCR    202	    //无屏机
#define UT_ATTR_HS_MINI     203	    //小屏机
#define UT_ATTR_IPPHONE_2   204	    //IP话机
#define UT_ATTR_IPPHONE_V   205	    //IP可视话机
#define UT_ATTR_BALL_CTRL   206     //布控球
#define UT_ATTR_CAM_PTZ     207     //云台监控头
#define UT_ATTR_CAM         208     //监控头
#define UT_ATTR_GPS         209     //GPS定位器
#define UT_ATTR_CAR_S       210     //普通车载台
#define UT_ATTR_CAR_VS      211     //视频车载台
#define UT_ATTR_AGW         213     //语音网关
#define UT_ATTR_RADAR       214     //雷达
#define UT_ATTR_CAM_LE      215     //光电摄像头
#define UT_ATTR_BS          216     //基站
#define UT_ATTR_OTHER       220     //其它

//用户状态
#define UT_STATUS_OFFLINE   0       //离线
#define UT_STATUS_ONLINE    1       //在线
#define UT_STATUS_MAX       0xff    //无意义
//以下,同SRV_TYPE_e

//注册类型
enum REG_TYPE_e
{
    REG_TYPE_START = 1,             //启动登记
    REG_TYPE_HB,                    //心跳
    REG_TYPE_EXIT,                  //退出登记    
    REG_TYPE_EXITALL,               //注销所有地址业务

    REG_TYPE_SIPQUEUE,              //SIP查询

    REG_TYPE_DEACTIVE,              //周期去活
    REG_TYPE_MAX
};

//HTTP查询的节点类型
#define HTTP_QUERYTYPE_CAM  0
#define HTTP_QUERYTYPE_CAR  1
#define HTTP_QUERYTYPE_NODE 2

//媒体属性
typedef struct _MEDIAATTR_s
{
    UCHAR       ucAudioRecv;        // 是否接收语音
    UCHAR       ucAudioSend;        // 是否发送语音
    UCHAR       ucVideoRecv;        // 是否接收视频
    UCHAR       ucVideoSend;        // 是否发送视频
}MEDIAATTR_s;

//业务类型,最大0xff,网络用1字节传
enum SRV_TYPE_e
{
    SRV_TYPE_NONE       = 0,                    //无业务

    //呼叫业务
    SRV_TYPE_BASIC_CALL = 16,                   //基本两方呼叫
    SRV_TYPE_CONF       = 17,                   //会议,也是会议主席
    SRV_TYPE_CONF_JOIN  = 18,                   //会议参与方
    SRV_TYPE_FORCE_INJ  = 19,                   //强插
    SRV_TYPE_FORCE_REL  = 20,                   //强拆
    SRV_TYPE_WATCH      = 21,                   //监控
    SRV_TYPE_WATCH_DOWN = 21,                   //监控下载
    SRV_TYPE_WATCH_UP   = 22,                   //监控上传
    SRV_TYPE_NS_CALL    = 23,                   //给存储服务器的呼叫    避免递归存储
    SRV_TYPE_SIMP_CALL  = 24,                   //单工呼叫
    SRV_TYPE_CALL_END   = 0x4f,                 //呼叫业务结束
    
    SRV_TYPE_IM         = 0x50,                 //及时消息

    SRV_TYPE_MULTICALL  = 0x80,                 //多种业务,只用在用户查询,给调度台显示用
    SRV_TYPE_OAM        = 0x81,                 //OAM操作
    SRV_TYPE_GPS        = 0x82,                 //GPS

    SRV_TYPE_MAX,
};

//业务号码  #业务码#号码
#define SRV_TYPE_FLAG_CHAR          '#'
#define SRV_TYPE_FLAG_STR           "#"
#define SRV_TYPE_BASIC_CALL_STR     "#16"
#define SRV_TYPE_CONF_STR           "#17"
#define SRV_TYPE_CONF_JOIN_STR      "#18"
#define SRV_TYPE_FORCE_INJ_STR      "#19"
#define SRV_TYPE_FORCE_REL_STR      "#20"
#define SRV_TYPE_WATCH_DOWN_STR     "#21"
#define SRV_TYPE_WATCH_UP_STR       "#22"
#define SRV_TYPE_NS_CALL_STR        "#23"

//分机号 摄像头通道号 号码-分机号
#define SRV_TYPE_NUMEXT_CHAR        '*'
#define SRV_TYPE_NUMEXT_STR         "*"

//呼叫参考号 -type-fsmid-扩展ID(CS号)-序号-时间     type: csa,1 cs,2 bcsm,3
#define SRV_TYPE_CALLREF_CHAR       '-'       //呼叫参考号
#define SRV_TYPE_CALLREF_STR        "-"       //呼叫参考号

//监测方向，1-上行，2-下行，3-双向
#define WATCH_DIR_NONE              0
#define WATCH_DIR_UP                1
#define WATCH_DIR_DOWN              2
#define WATCH_DIR_BOTH              3

#define SRV_TYPE_PRIO_HIGH          0
#define SRV_TYPE_PRIO_LOW           7
#define SRV_TYPE_PRIO_DEF           SRV_TYPE_PRIO_LOW
#define SRV_TYPE_PRIO_ERROR         (SRV_TYPE_PRIO_LOW + 1)

//收号时的结束符号
#define RECVNUM_END_CHAR            '#'

enum CALL_OT_e
{
    CC_OT_ERROR = 0,
    CC_O,                           //O端
    CC_T,                           //T端
    CC_OT_MAX
};

//号码段,支持10XX~20XX这样的格式
typedef struct _NUM_SEG_s
{
    UC_32           ucStartNum;     //起始号码
    UC_32           ucEndNum;       //结束号码
}NUM_SEG_s;

//"0"表示订阅所有用户,"###"表示订阅自己所属的所有组
#define GU_STATUSSUBS_STR_CLEARALL      "##0"
#define GU_STATUSSUBS_STR_GROUP         "###"
#define GU_STATUSSUBS_STR_ALL           "0"
//状态订阅级别枚举值
enum GU_STATUSSUBS_e
{
    GU_STATUSSUBS_NONE = 0,         //取消订阅
    GU_STATUSSUBS_OAM,              //相关组和用户的操作维护
    GU_STATUSSUBS_BASIC,            //基本状态,在线离线
    GU_STATUSSUBS_DETAIL,           //详细状态,呼叫状态
    GU_STATUSSUBS_DETAIL1,          //最详细状态,呼叫号码
    GU_STATUSSUBS_QUERY_ONETIME,    //只单次查询,不订阅
    
    GU_STATUSQUERY_MAX
};

//OAM操作定义
#define OPT_USER_ADD                1           //添加用户
#define OPT_USER_DEL                2           //删除用户
#define OPT_USER_MODIFY             3           //修改用户
#define OPT_USER_QUERY              4           //查询用户
#define OPT_G_ADD                   5           //添加组
#define OPT_G_DEL                   6           //删除组
#define OPT_G_MODIFY                7           //修改组
#define OPT_G_QUERY                 8           //查询组
#define OPT_G_ADDUSER               9           //组中添加成员
#define OPT_G_DELUSER               10          //组中删除成员
#define OPT_G_MODIFYUSER            11          //组中修改成员
#define OPT_G_QUERYUSER             12          //查询组成员
#define OPT_U_QUERYGROUP            13          //查询用户归属组
#define OPT_O_ADD                   14          //添加组织
#define OPT_O_DEL                   15          //删除组织
#define OPT_O_MODIFY                16          //修改组织
#define OPT_O_QUERY                 17          //组织查询
#define OPT_GMEMBER_EXTINFO         18          //组成员扩展信息
#define OPT_R_ADD                   19          //添加路由
#define OPT_R_DEL                   20          //删除路由
#define OPT_R_MODIFY                21          //修改路由
#define OPT_R_QUERY                 22          //查询路由
#define OPT_CROSS_G2U               23          //跨节点组到用户消息
#define OPT_CROSS_U2G               24          //跨节点用户到组消息
#define OPT_CROSS_STATUS_U_2LOW     25          //跨节点用户状态发送到下级
#define OPT_CROSS_STATUS_G_2LOW     26          //跨节点组状态发送到下级
#define OPT_CROSS_STATUS_U_2HIGH    27          //跨节点用户状态发送到上级
#define OPT_LIC_QUERY               28          //授权信息查询

//最高位,GU_TYPE_LOCAL,表示本节点用户;GU_TYPE_CROSS,表示其他节点用户
#define GROUP_MEMBERTYPE_NONE       0
#define GROUP_MEMBERTYPE_USER       1           //用户(玖诺中继站,时隙1)
#define GROUP_MEMBERTYPE_GROUP      2           //组
#define GROUP_MEMBERTYPE_USER_OUT   3           //外网用户
#define GROUP_MEMBERTYPE_USER_SLOT2 4           //用户,时隙2,玖诺中继站

#define GROUP_MAX_MEMBER            1024        //一个组中最多多少个用户
#define GROUP_MAX_MEMBER_MINI       100         //编解码使用的
#define USER_MAX_GROUP              32          //单个用户最多在多少个组中
#define GROUP_MAX_LAYER             8           //最大组层级

//组成员信息
#define FGNUM_MAX_LEN               1024    //(32 * USER_MAX_GROUP)
typedef struct _GROUP_MEMBER_s
{
    UCHAR           ucPrio;             //优先级
    UCHAR           ucType;             //是用户还是组,GROUP_MEMBERTYPE_USER,GU_TYPE_LOCAL,GU_TYPE_CROSS有效

    UCHAR           ucUTType;           //UTType,UT_TYPE_NONE
    UCHAR           ucAttr;             //终端属性,UT_ATTR_HS

    UC_32           ucNum;              //号码
    UC_64           ucName;             //名字
    UC_32           ucAGNum;            //关联组信息
    UCHAR           ucGType;            //组类型,G_TYPE_FOMAL
    UCHAR           ucChanNum;          //摄像头通道个数
    UCHAR           ucStatus;           //主状态 UT_STATUS_OFFLINE之类

    UCHAR           ucFGCount;          //父组个数

#ifndef T_TINY_MODE
#ifdef IDT
    UCHAR           ucFGNum[FGNUM_MAX_LEN];//父组号码 每个组都是32个数字
#else
    UCHAR           ucFGNum[FGNUM_MAX_LEN / 2];//父组号码 每个组都是32个数字,占用内存太大????????改为内存表,这里可以放开
#endif
#endif
}GROUP_MEMBER_s;

//一个网络报文,最多发送多少个成员
typedef struct _USERGINFO_s
{
    unsigned short  usNum;
    GROUP_MEMBER_s  stGInfo[USER_MAX_GROUP];
}USERGINFO_s;

//状态定义
//单个组和用户状态提醒
#define GU_STATUSSUBS_MAXNUM        16  //一个用户最多可以订阅
//超时订阅失效
#define GU_STATUSSUBS_TIMEMAX       70  //70秒没有继续订阅,失效.终端30秒发送一次

// 状态定义,分级别:
// 1.基本状态,在线/不在线.以后,开通与否,在这里定义.GPS上报状态
// 2.呼叫状态(呼叫类型,呼叫ID)
// 3.详细呼叫状态,对端号码/名字
#define GU_STATUSCALL_IDLE          0
#define GU_STATUSCALL_OALERT        1   //主叫听回铃,主叫起呼就置,有可能被叫还没振铃,还处于接续中
#define GU_STATUSCALL_TALERT        2   //被叫振铃
#define GU_STATUSCALL_TALKING       3   //通话
#define GU_STATUSCALL_G_TALKING     4   //组呼讲话
#define GU_STATUSCALL_G_LISTEN      5   //组呼听话
typedef struct _GU_STATUS_CALL_s
{
    UCHAR           ucCallType;         //呼叫类型          SRV_TYPE_BASIC_CALL
    UCHAR           ucCallStatus;       //呼叫状态          GU_STATUSCALL_OALERT
    DWORD           dwCallId;           //呼叫ID            CSA的ID
    UC_32           ucPeerNum;          //对端号码
    UC_64           ucPeerName;         //对端名字
}GU_STATUS_CALL_s;
#define GU_STATUS_CALL_MAXNUM       4
typedef struct _GU_STATUE_s
{
    UCHAR           ucStatus;           //主状态 UT_STATUS_OFFLINE之类
    UCHAR           ucGpsReport;        //是否正在上报GPS,0未上报,1正在上报
    UCHAR           ucCallNum;          //呼叫个数
    GU_STATUS_CALL_s CallStatus[GU_STATUS_CALL_MAXNUM];//呼叫状态
}GU_STATUE_s;
//组成员状态
typedef struct _GROUP_MEMBERSTATUS_s
{
    UCHAR           ucType;             //是用户还是组,1用户,2组

    UC_32           ucNum;              //号码
    GU_STATUE_s     Status;             //状态
}GROUP_MEMBERSTATUS_s;

#define GU_STATUSGINFO_MAXNUM       8
typedef struct _GU_STATUSGINFO_s
{
    USHORT                  usNum;
    GROUP_MEMBERSTATUS_s    stStatus[GU_STATUSGINFO_MAXNUM];
}GU_STATUSGINFO_s;

//组用户扩展信息,由用户定义
typedef struct _GMEMBER_EXTINFO_ENT_s
{
    UC_32           ucNum;              //号码
    UC_512          ucInfo;             //信息,json格式
}GMEMBER_EXTINFO_ENT_s;
typedef struct _GMEMBER_EXTINFO_s
{
    WORD                    wNum;
    GMEMBER_EXTINFO_ENT_s   stExtInfo[GROUP_MAX_MEMBER];
}GMEMBER_EXTINFO_s;

//组织信息
#define ORG_RUN_IDLE        0       //空闲状态
#define ORG_RUN_ACTIVE      1       //激活状态,可以发送消息
#define ORG_RUN_INACTIVE    2       //非激活状态,已开户,但不可发送消息
typedef struct _ORG_s
{
    UC_32           ucNum;              //号码
    UC_64           ucName;             //名字
    UCHAR           ucType;             //类型      O_TYPE_TRIAL
    UC_128          ucDesc;             //描述
    DWORD           dwUserNum;          //总用户个数
    DWORD           dwDsNum;            //调度台个数
    DWORD           dwGroupNum;         //组个数
    DWORD           dwEndUserNum;       //终端用户个数
    UC_32           ucDs0Num;           //0号调度台号码
    UC_32           ucDs0Pwd;           //0号调度台密码
    UC_64           ucDsName;           //调度台显示的名字
    UC_128          ucDsIcon;           //调度台采用的图标
    UC_64           ucAppName;          //APP显示的名字
    UC_128          ucAppIcon;          //APP采用的图标
    NUM_SEG_s       USeg;               //用户号段
    NUM_SEG_s       GSeg;               //组号段
    NUM_SEG_s       DSSeg;              //调度台号段
    UC_32           ucStartTime;        //开始时间
    UC_32           ucEndTime;          //结束时间
    UC_64           ucMcAddr;           //MC地址
    DWORD           dwMcStaus;          //MC状态,ORG_RUN_IDLE
    UC_32           ucModifyTime;       //最后一次修改时间
    UC_32           ucPrefix;           //前缀
}ORG_s;


#ifdef T_TINY_MODE
#define ORGLIST_MAX_NUM     1           //最多组织个数
#else
#define ORGLIST_MAX_NUM     128         //最多组织个数
#endif
//组织列表信息
typedef struct _ORG_LIST_s
{
    WORD                    wNum;
    ORG_s                   stOrg[ORGLIST_MAX_NUM];
}ORG_LIST_s;

//路由信息
typedef struct _ROUTECFG_s
{
    UC_32           ucName;             //名字
    UCHAR           ucStatus;           //状态,0离线,1在线

    UC_32           ucPeerZone;         //对端区号                  ""表示默认路由,不属于本局的所有号码,在路由表中查不到其他信息,都走到这里
    UC_32           ucPeerOrg;          //对端组织号码
    NUM_SEG_s       CalledSeg;          //被叫号码段
    UC_32           ucMyZone;           //本端区号
    UC_32           ucMyOrg;            //本端组织号码              空表示所有组织都能看到这个路由,有值表示特定组织的路由
    NUM_SEG_s       CallingSeg;         //主叫号码段
    UC_32           ucSwitchBoard;      //本端总机号码              对端如果拨打这个号码,就进入"请拨打分机号"               "x" "X" "." 表示呼入任何号码,都进入"请拨分机号"
    UCHAR           ucCall;             //是否支持呼叫
    UCHAR           ucIm;               //是否支持即时消息
    UCHAR           ucOam;              //是否支持OAM
    UCHAR           ucGps;              //是否支持GPS
    UCHAR           ucMetr;             //优先权,越小优先权越高
    UC_32           ucFid;              //接入协议功能块号,字符串格式
    UC_32           ucIpAddr;           //对端IP地址,如果无效,使用注册号码的IP地址
    UC_32           ucRegNum;           //注册号码,如果这个号码或号码段注册了,更新接口属性的模块号和IP地址.现在只做号码
    UC_32           ucFileSrvAddr;      //文件服务器地址
    UC_32           ucUserName;         //文件服务器用户名
    UC_32           ucPwd;              //文件服务器密码
}ROUTECFG_s;

#define ROUTECFG_QUERYLIST_MAX_COUNT    4     //最多一次路由查询有多少条记录
//路由列表信息
typedef struct _ROUTECFG_QUERYLIST_s
{
    WORD                    wCount;
    ROUTECFG_s              stRouteCfg[ROUTECFG_QUERYLIST_MAX_COUNT];
}ROUTECFG_QUERYLIST_s;

//会场信息状态
#define CONF_MEMBERSTATUS_IDLE      0       //空闲
#define CONF_MEMBERSTATUS_LISTEN    1       //听话
#define CONF_MEMBERSTATUS_TALK1     2       //台下讲话
#define CONF_MEMBERSTATUS_TALK0     3       //台上讲话

//GPS信息
#ifdef T_TINY_MODE
#define GPS_SUBS_MAXNUM     8     //一个用户最多可以订阅多少个GPS信息
#else
#define GPS_SUBS_MAXNUM     4096 //一个用户最多可以订阅多少个GPS信息
#endif

//GPS订阅失效时间
#define GPS_SUBS_TIMEMAX    70  //70秒没有继续订阅,失效.终端30秒发送一次
//GPS离线时间
#define GPS_OFFLINE_TIME    90  //90秒没有收到GPS数据,认为离线
//GPS基本数据
typedef struct _GPS_DATA_s
{
    float           longitude;          //经度
    float           latitude;           //纬度
    float           speed;              //速度
    float           direction;          //方向
    //时间
    USHORT          year;               //年
    UCHAR           month;              //月
    UCHAR           day;                //日
    UCHAR           hour;               //时
    UCHAR           minute;             //分
    UCHAR           second;             //秒
}GPS_DATA_s;
//GPS记录
#define GPS_REC_MAX   24
typedef struct _GPS_REC_s
{
    UC_32           ucNum;              //号码
    UCHAR           ucStatus;           //是否在线,UT_STATUS_ONLINE
    UCHAR           ucCount;            //有效个数
    GPS_DATA_s      gps[GPS_REC_MAX];   //内容
}GPS_REC_s;

#define ucQueryExtUser_All      100
//用户数据查询使用的附加参数
typedef struct _QUERY_EXT_s
{
    UCHAR           ucAll;                      //是否所有用户,0不是,1是所有用户(此时不查询下属组,是调度台查询所有用户)
    UCHAR           ucGroup;                    //是否需要查询下属组
    UCHAR           ucUser;                     //是否需要查询下属用户
    UCHAR           ucOrder;                    //排序方式,0按号码排序,1按名字排序
    DWORD           dwPage;                     //第几页
    DWORD           dwCount;                    //每页有多少数据
    DWORD           dwTotalCount;               //总数
}QUERY_EXT_s;

//存储查询使用的参数
typedef struct _NS_QUERY_s
{
    UC_32           ucNum;                      //本端号码
    UC_32           ucPeerNum;                  //对端号码
    UC_32           ucStartTime;                //开始时间
    UC_32           ucEndTime;                  //结束时间
    UCHAR           ucOT;                       //主叫还是被叫
    UCHAR           ucSrvType;                  //业务类型
    UC_64           ucRef;                      //参考号
    DWORD           dwStartCount;               //数据库查询起始序号,0表示启动查询
}NS_QUERY_s;


//存储查询响应
typedef struct _NS_QUERYREC_s
{
    UC_64           ucNum;                      //本端号码
    UC_64           ucPeerNum;                  //对端号码
    UC_32           ucStartTime;                //开始时间
    UC_32           ucEndTime;                  //结束时间
    UCHAR           ucOT;                       //主叫还是被叫
    UCHAR           ucSrvType;                  //业务类型
    UCHAR           ucMediaType;                //媒体类型
    UC_64           ucRef;                      //参考号
    UC_128          ucFileName;                 //文件名
    UC_1024         ucTxt;                      //消息内容
    UC_256          ucUserMark;                 //用户标志
}NS_QUERYREC_s;
#define NS_QUERYREC_MAX     50
typedef struct _NS_QUERYRSP_s
{
    UCHAR           ucCount;                    //有效记录个数
    NS_QUERYREC_s   stRec[NS_QUERYREC_MAX];     //查询记录
}NS_QUERYRSP_s;

typedef struct _SDP_AINFO_s
{
    unsigned char   ucSend;         //是否发送语音
    unsigned char   ucRecv;         //是否接收语音
    unsigned char   ucCodec;        //编码器
    int             iBitRate;       //码率
    unsigned char   ucTime;         //打包时长

    int Init()
    {
        ucSend = 0;
        ucRecv = 0;
        ucCodec = 0xff;
        iBitRate = -1;
        ucTime = 0;
        return 0;
    }
}SDP_AINFO_s;
//视频信息
typedef struct _SDP_VINFO_s
{
    unsigned char   ucSend;         //是否发送视频
    unsigned char   ucRecv;         //是否接收视频
    unsigned char   ucCodec;        //编码器ID
    int             iWidth;         //宽
    int             iHeight;        //高
    int             iFrameRate;     //帧率
    int             iBitRate;       //码率
    int             iGop;           //I帧间隔

    int Init()
    {
        ucSend  = 0;
        ucRecv  = 0;
        ucCodec = 0xff;
        iWidth  = 0;
        iHeight = 0;
        iFrameRate = 0;
        iBitRate = -1;
        iGop = 0;
        return 0;
    }
}SDP_VINFO_s;

//INFO消息码
#define SRV_INFO_MICNONE    0           //未定义
#define SRV_INFO_MICREQ     1           //话权申请
#define SRV_INFO_MICREL     2           //话权释放
#define SRV_INFO_MICGIVE    3           //给与话权
#define SRV_INFO_MICTAKE    4           //收回话权
#define SRV_INFO_MICINFO    5           //话权提示
#define SRV_INFO_AUTOMICON  10          //打开自由发言
#define SRV_INFO_AUTOMICOFF 11          //关闭自由发言
#define SRV_INFO_CAMHISPLAY 12          //播放历史视频
#define SRV_INFO_TALKON     13          //打开对讲
#define SRV_INFO_TALKOFF    14          //关闭对讲
#define SRV_INFO_TALKONRSP  15          //打开对讲响应
#define SRV_INFO_TALKOFFRSP 16          //关闭对讲响应
#define SRV_INFO_NSREC      17          //NS存储记录
#define SRV_INFO_ICECAND    18          //IceCandidate
#define SRV_INFO_ICESTATE   19          //IceStateChange
#define SRV_INFO_SDP        20          //本端SDP传送
#define SRV_INFO_CAMCTRL    21          //摄像头控制
#define SRV_INFO_NUM        22          //号码          DTMF-RELAY
#define SRV_INFO_USERMARK   23          //用户标识
#define SRV_INFO_CALLREF    24          //呼叫流水号,主叫是自己,被叫是主叫,查看是对端
#define SRV_INFO_MICMODIFY  25          //话筒修改
#define SRV_INFO_NUM_DTMF   26          //号码          DTMF

//摄像头上下左右聚焦等控制
#define SRV_CAMCTRL_UP          1       //上            速度(0~0x3f)            标准
#define SRV_CAMCTRL_DOWN        2       //下            速度(0~0x3f)            标准
#define SRV_CAMCTRL_LEFT        3       //左            速度(0~0x3f)            标准
#define SRV_CAMCTRL_RIGHT       4       //右            速度(0~0x3f)            标准
#define SRV_CAMCTRL_ZOOMWIDE    5       //缩小          速度(0~0x3f)            标准
#define SRV_CAMCTRL_ZOOMTELE    6       //放大          速度(0~0x3f)            标准
#define SRV_CAMCTRL_FOCUSNEAR   7       //近焦          速度(0~0x3f)            标准
#define SRV_CAMCTRL_FOCUSFAR    8       //远焦          速度(0~0x3f)            标准
#define SRV_CAMCTRL_IRISOPEN    9       //光圈放大      速度(0~0x3f)            ?
#define SRV_CAMCTRL_IRISCLOSE   10      //光圈缩小      速度(0~0x3f)            ?
#define SRV_CAMCTRL_AUTOSCAN    11      //自动扫描      开启/停止(0/1)          ?
#define SRV_CAMCTRL_CRUISE      12      //巡航          开启/停止(0/1)          ?
#define SRV_CAMCTRL_INFRARED    13      //红外          开启/停止(0/1)          标准
#define SRV_CAMCTRL_RAINSTRIP   14      //雨刷          开启/停止               ?
#define SRV_CAMCTRL_PRESET      15      //预置位        预置位位置              ?
#define SRV_CAMCTRL_WARNOUTPUT  16      //告警输出                              ?
#define SRV_CAMCTRL_SETPARAM    17      //设置参数                              ?
#define SRV_CAMCTRL_REBOOT      18      //重启设备                              ?
#define SRV_CAMCTRL_STOP        19      //停止之前云台操作

//即时通信消息码,也是即时通信状态指示码
#define PTE_CODE_TXREQ          1       //发送请求
#define PTE_CODE_TXCFM          2       //传输确认
#define PTE_CODE_USRREAD        3       //用户阅读
#define PTE_CODE_USRREADCFM     4       //用户阅读消息的确认
#define PTE_CODE_FILENAMEREQ    5       //文件名请求
#define PTE_CODE_FILENAMERSP    6       //文件名响应


//及时消息类型
#define IM_TYPE_NONE            0x00    //无
//01~7f需要存储转发,用户确认收到的,通过IDT_IMSend/IDT_IMRecv传递
#define IM_TYPE_TXT             0x01    //只有文本                  不需要文件          from + to + orito + text
#define IM_TYPE_GPS             0x02    //GPS位置信息               不需要文件          from + to + orito + text(字符串:经度,纬度)
#define IM_TYPE_IMAGE           0x03    //图像                      需要文件            from + to + orito + text + filename + sourcefilename
#define IM_TYPE_AUDIO           0x04    //语音文件,微信             需要文件            from + to + orito + text + filename + sourcefilename
#define IM_TYPE_VIDEO           0x05    //视频录像文件              需要文件            from + to + orito + text + filename + sourcefilename
#define IM_TYPE_FILE            0x06    //任意文件                  需要文件            from + to + orito + text + filename + sourcefilename
#define IM_TYPE_BCALL           0x07    //单呼语音呼叫              APP本地存储使用,不传递
#define IM_TYPE_VCALL           0x08    //单呼视频呼叫              APP本地存储使用,不传递
#define IM_TYPE_GCALL           0x09    //组呼                      APP本地存储使用,不传递
#define IM_TYPE_CONF            0x11    //会议格式短信              不需要文件          from + to + orito + text
                                        //IDT_IMSend/IDT_IMRecv的pcTxt字段,是会议格式短信内容.采用Json格式,内容为:
                                        //  {
                                        //  public int type;       //0 会议预约信息通知 1 成员会议回复结果消息 2 会议开始通知
                                        //  public String number;   //会议组号码信息
                                        //  public String meetId;   //会场号码
                                        //会议预约信息通知 填写
                                        //  public String title; // 会议标题
                                        //  public String desc;  // 会议描述
                                        //  public String time;  // 会议时间
                                        //成员会议回复结果消息 填写
                                        //  public boolean accept;  // 是否接受
                                        //  public String reason;    // 拒绝原因
                                        //  }
                                        //  会议主持人群发短信时,from=主持人号码,to=组号码
                                        //  会议成员发是否接受会议时,from=会议成员号码,to=组号码
#define IM_TYPE_GCALLAUDIO      0x12    //组呼语音消息              需要文件            from + to + orito + text + filename + sourcefilename
#define IM_TYPE_CONFREC         0x13    //会议录音                  需要文件            from + to + orito + text + filename + sourcefilename

//0x80~0xff不需要存储转发,用户如果不在线,就直接丢弃,通过IDT_SendPassThrouth/IDT_RecvPassTrouth传递
#define IM_TYPE_NSSUBS          0x80    //存储订阅                  不需要文件          起始号码~结束号码;起始号码~结束号码;起始号码~结束号码;
#define IM_TYPE_NSQUERYREQ      0x81    //存储查询                  不需要文件          NS_QUERY_s
#define IM_TYPE_NSQUERYRSP      0x82    //存储查询响应              不需要文件          NS_QUERYRSP_s
#define IM_TYPE_VSQUERYCHAN_REQ 0x83    //视频网关查询通道请求      不需要文件          4001*97*192.168.2.38*8000*admin*admin123
#define IM_TYPE_VSQUERYCHAN_RSP 0x84    //视频网关查询通道响应      不需要文件          号码#通道号-名称;通道号-名称;通道号-名称;通道号-名称;通道号-名称;通道号-名称
#define IM_TYPE_CAM_PLAYBACK    0x85    //摄像头回放                不需要文件          开始时间~结束时间
#define IM_TYPE_ECHO_REQ        0x86    //EchoReq                   不需要文件          事物号*秒*纳秒*填充  TimeMs的值  (%d*%d*%d)
#define IM_TYPE_ECHO_RSP        0x87    //EchoRsp                   不需要文件          事物号*秒*纳秒*填充  TimeMs的值  (%d*%d*%d)
#define IM_TYPE_WRITE_PAR_REQ   0x88    //写参数请求                不需要文件          事物号*section*key*value    (%d*%s*%s*%s)
#define IM_TYPE_WRITE_PAR_RSP   0x89    //写参数响应                不需要文件          事物号*section*key*value    (%d*%s*%s*%s)
#define IM_TYPE_READ_PAR_REQ    0x8A    //读参数请求                不需要文件          事物号*section*key          (%d*%s*%s)
#define IM_TYPE_READ_PAR_RSP    0x8B    //读参数响应                不需要文件          事物号*section*key*value    (%d*%s*%s*%s)
#define IM_TYPE_DISP_CMD_REQ    0x8C    //调度命令请求              不需要文件          事物号*命令号*目标号码      (%d*%d*%s)
                                        //  0x01    版本查看
                                        //  0x02    视频上传
                                        //  0x03    视频下载
                                        //  0x04    通话
                                        //  0x05    发送位置信息
#define IM_TYPE_DISP_CMD_RSP    0x8D    //调度命令响应              不需要文件          事物号*命令号*目标号码      (%d*%d*%s)
#define IM_TYPE_PHONE_SOS       0x91    //手机往群组发送sos消息
#define IM_TYPE_UPVIDEO_NOTIFY  0x92    //手机通知别的用户上传视频

#define IM_TYPE_AUS_AUTH_REQ    0xa0    //授权请求                  不需要文件          终端->AUS   {"MyId":"MyId1", "OrgNum":"201910251439090", "AudioId":"AudioId1", "Imei":"Imei1", "HardType":"HardType1", "OsVer":"OsVer1", "IdsIp":"192.168.0.225:10022", "IdsNum":"1132"}
#define IM_TYPE_AUS_AUTH_RSP    0xa1    //授权响应                  不需要文件          AUS->终端   {"MyId":"xxxxxxx", "Cause":0}
#define IM_TYPE_AUS_AUTH_IND    0xa2    //授权指示                  不需要文件          终端->AUS   {"MyId":"MyId1", "OrgNum":"201910251439090", "AudioId":"AudioId11", "Imei":"Imei1", "HardType":"HardType1", "OsVer":"OsVer1", "IdsIp":"192.168.0.225:10022", "IdsNum":"1132"}
#define IM_TYPE_AUS_AUTH_IND_RSP 0xa3   //授权指示响应              不需要文件          AUS->终端   {"MyId":"xxxxxxx", "Cause":0}
#define IM_TYPE_AUS_CREATE_REQ  0xa4    //创建请求                  不需要文件          管理台->AUS {"Name":"xxxxxxx", "Count":100}                                     
#define IM_TYPE_AUS_CREATE_RSP  0xa5    //创建响应                  不需要文件          AUS->管理台 {"Name":"xxxxxxx", "Count":100, "Num":"xxxxxxx"}
#define IM_TYPE_AUS_LOAD_REQ    0xa6    //加载请求                  不需要文件          管理台->AUS {"Name":"xxxxxxx"}
#define IM_TYPE_AUS_LOAD_RSP    0xa7    //加载响应                  不需要文件          AUS->管理台 {"Name":"xxxxxxx", "Count":100, "Num":"xxxxxxx", "CTime":""}
#define IM_TYPE_AUS_SYNDXSK_REQ 0xa8    //同步大象声科数据请求      不需要文件          AUS->管理台 {}
#define IM_TYPE_AUS_SYNDXSK_RSP 0xa9    //同步大象声科数据响应      不需要文件          AUS->管理台 {""}

#define IM_TYPE_JNZJT_GPS_QUERY 0xb0    //玖诺中继台GPS查询         不需要文件
#define IM_TYPE_DEBUG           0xb1    //调试命令

#define IM_TYPE_USER            0xff    //用户自定义                不需要文件

#include "cause.h"

//日志
//本端号码                  对端号码                开始时间                结束时间
//终端类型(UT_TYPE_TAP)     终端接入协议(FID_TAP)   IP地址                    
//日志类型(LOGDB_TYPE_REG)  子类型(OPT_USER_ADD)    操作内容(开机登记)      备注(操作的详细信息)
#define LOGDB_TYPE_REG          0x01        //登录系统      子类型为REG_TYPE_START等
#define LOGDB_TYPE_OAM          0x03        //OAM操作       子类型为OPT_USER_ADD等
#define LOGDB_TYPE_CALL         0x04        //发起呼叫      子类型为SRV_TYPE_BASIC_CALL等
//终端接入协议
//FID_TAP       4
//FID_SIPUA     8
//FID_WSSRV     16

#endif



