#if !defined _CAUSE_H_
#define _CAUSE_H_

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------------------
//     原因值开始
//--------------------------------------------------------------------------------
//Normal class
#define CAUSE_ZERO  					0       //null
#define CAUSE_UNASSIGNED_NUMBER 		1       //未分配号码
#define CAUSE_NO_ROUTE_TO_DEST  		2       //无目的路由
#define CAUSE_USER_BUSY 				3       //用户忙
#define CAUSE_ALERT_NO_ANSWER   		4       //用户无应答(人不应答)
#define CAUSE_CALL_REJECTED 			5       //呼叫被拒绝
#define CAUSE_ROUTING_ERROR 			6       //路由错误
#define CAUSE_FACILITY_REJECTED 		7       //设备拒绝
#define CAUSE_ERROR_IPADDR              8       //错误IP地址过来的业务请求
#define CAUSE_NORMAL_UNSPECIFIED		9       //通常,未指定
#define CAUSE_TEMPORARY_FAILURE 		10      //临时错误
#define CAUSE_RESOURCE_UNAVAIL  		11      //资源不可用
#define CAUSE_INVALID_CALL_REF  		12      //不正确的呼叫参考号
#define CAUSE_MANDATORY_IE_MISSING  	13      //必选信息单元丢失
#define CAUSE_TIMER_EXPIRY  			14      //定时器超时
#define CAUSE_CALL_REJ_BY_USER          15      //被用户拒绝
#define CAUSE_CALLEE_STOP               16      //被叫停止
#define CAUSE_USER_NO_EXIST             17      //用户不存在
#define CAUSE_MS_UNACCESSAVLE           18      //不可接入
#define CAUSE_MS_POWEROFF               19      //用户关机
#define CAUSE_FORCE_RELEASE             20      //强制拆线
#define CAUSE_HO_RELEASE                21      //切换拆线
#define CAUSE_CALL_CONFLICT             22      //呼叫冲突
#define CAUSE_TEMP_UNAVAIL              23      //暂时无法接通
#define CAUSE_AUTH_ERROR                24      //鉴权错误
#define CAUSE_NEED_AUTH                 25      //需要鉴权
#define CAUSE_SDP_SEL                   26      //SDP选择错误
#define CAUSE_MS_ERROR                  27      //媒体资源错误
#define CAUSE_INNER_ERROR               28      //内部错误
#define CAUSE_PRIO_ERROR                29      //优先级不够
#define CAUSE_SRV_CONFLICT              30      //业务冲突
#define CAUSE_NOTREL_RECALL             31      //由于业务要求,不释放,启动重呼定时器
#define CAUSE_NO_CALL                   32      //呼叫不存在
#define CAUSE_DUP_REG                   33      //重复注册
#define CAUSE_MG_OFFLINE                34      //MG离线
#define CAUSE_DS_REQ_QUITCALL           35      //调度员要求退出呼叫
#define CAUSE_DB_ERROR                  36      //数据库操作错误
#define CAUSE_TOOMANY_USER              37      //太多的用户
#define CAUSE_SAME_USERNUM              38      //相同的用户号码
#define CAUSE_SAME_USERIPADDR           39      //相同的固定IP地址
#define CAUSE_PARAM_ERROR               40      //参数错误
#define CAUSE_SAME_GNUM                 41      //相同的组号码
#define CAUSE_TOOMANY_GROUP             42      //太多的组
#define CAUSE_NO_GROUP                  43      //没有这个组
#define CAUSE_SAME_USERNAME             44      //相同的用户名字
#define CAUSE_OAM_OPT_ERROR             45      //OAM操作错误
#define CAUSE_INVALID_NUM_FORMAT		46      //不正确的地址格式
#define CAUSE_INVALID_DNSIP             47      //DNS或IP地址错误
#define CAUSE_SRV_NOTSUPPORT            48      //不支持的业务
#define CAUSE_MEDIA_NOTDATA             49      //没有媒体数据
#define CAUSE_RECALL                    50      //重新呼叫
#define CAUSE_LINK_DISC                 51      //断链
#define CAUSE_ORG_RIGHT                 52      //组织越权(节点)
#define CAUSE_SAME_ORGNUM               53      //相同的组织号码
#define CAUSE_SAME_ORGNAME              54      //相同的组织名字
#define CAUSE_UNASSIGNED_ORG            55      //未分配的组织号码
#define CAUSE_INOTHER_ORG               56      //在其他组织中
#define CAUSE_HAVE_GCALL                57      //已经有组呼存在
#define CAUSE_HAVE_CONF                 58      //已经有会议存在
#define CAUSE_SEG_FORMAT                59      //错误的号段格式
#define CAUSE_USEG_CONFLICT             60      //用户号码段冲突
#define CAUSE_GSEG_CONFLICT             61      //组号码段冲突
#define CAUSE_NOTIN_SEG                 62      //不在号段内
#define CAUSE_USER_IN_TOOMANY_GROUP     63      //用户所在组太多
#define CAUSE_DSSEG_CONFLICT            64      //调度台号码段冲突
#define CAUSE_OUTNETWORK_NUM            65      //外网用户
#define CAUSE_CFU                       66      //无条件呼叫前转
#define CAUSE_CFB                       67      //遇忙呼叫前转
#define CAUSE_CFNRc                     68      //不可及呼叫前转
#define CAUSE_CFNRy                     69      //无应答呼叫前转
#define CAUSE_MAX_FWDTIME               70      //到达最大前转次数
#define CAUSE_OAM_RIGHT                 71      //OAM操作无权限
#define CAUSE_DGT_ERROR                 72      //号码错误
#define CAUSE_RESOURCE_NOTENOUGH        73      //资源不足
#define CAUSE_ORG_EXPIRE                74      //组织到期
#define CAUSE_USER_EXPIRE               75      //用户到期
#define CAUSE_SAME_ROUTENAME            76      //相同的路由名字
#define CAUSE_UNASSIGNED_ROUTE          77      //未分配的路由
#define CAUSE_OAM_FWD                   78      //OAM消息前转
#define CAUSE_UNCFG_MAINNUM             79      //未配置主号码
#define CAUSE_G_NOUSER                  80      //组中没有用户
#define CAUSE_U_LOCK_G                  81      //用户锁定在其他组
#define CAUSE_U_OFFLINE_G               82      //组中没有在线用户

#define CAUSE_MAX                       0x1fff

#define CAUSE_EXPIRE_IDT                0x0000  //IDT定时器超时
#define CAUSE_EXPIRE_MC                 0x4000  //MC定时器超时
#define CAUSE_EXPIRE_MG                 0x8000  //MG定时器超时

//--------------------------------------------------------------------------------
//     原因值结束
//--------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif //


