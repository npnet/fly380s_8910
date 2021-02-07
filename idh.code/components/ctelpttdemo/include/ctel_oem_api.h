#ifndef __CTEL_OEM_API_H__
#define __CTEL_OEM_API_H__
#ifdef __cplusplus
extern "C" {
#endif
#include "ctel_oem_callback_api.h"

 /*!
  * Description:
  * UI注册回调的统一接口
  *
  * Rarameter:
  * CTEL_REGISTER_CB 类型指针
  *
  * Return:
  * none
  */
 void ctel_register_callback(CTEL_REGISTER_CB *cb);

 /*!
  * Description:
  * UI设置心跳周期
  *
  * Rarameter:
  * headbeat 取值范围（1~120）默认值为20 单位秒
  *
  * Return:
  * none
  */
 void ctel_set_headbeat_time(unsigned int headbeat);

  /*!
  * Description:
  * 设置注册服务器地址信息
  *
  * Rarameter:
  * masterurl 主登录地址  masterport主登录端口 （登录成功后，终端会上报给UI当前使用url，下次开机，UI需将url设为主登录地址）
  * slaveurl 辅登录地址  slaveport辅登录端口
  *
  * Return:
  * none
  */
 void ctel_set_register_info(char *masterurl,unsigned int masterport ,char* slaveurl, unsigned int slaveport);

   /*!
  * Description:
  * 设置注册服务器地址信息
  *
  * Rarameter:
  * url 登录服务器的地址 目前只支持IP地址 不带端口，端口有服务默认为2072
  * username 用户名
  * passwd 密码
  *
  * Return:
  * none
  */
 void ctel_set_login_info(char *url,char* username, char *passwd);

 /*!
  * Description:
  * 设置sim卡激活pdp绑定的cid
  *
  * Rarameter:
  * cid 取值范围 0~18
  *
  * Return:
  * none
  */
 void ctel_set_sim_cid(int cid);

 /*!
  * Description:
  * 设置设备名称
  *
  * Rarameter:
  * name 设备名称（utf8）   len 名称长度
  *
  * Return:
  * none
  */
 void ctel_set_dev_name(char *name,int len);

  /*!
  * Description:
  * 设置ptt服务的基础优先级
  *
  * Rarameter:
  * priority 任务优先级
  *
  * Return:
  * none
  */
 void ctel_set_task_priority(int priority);

 /*!
 * Description:
 * 设置驻网的apn
 *
 * Rarameter:
 * apn  apn名称 “cmnet” “ctnet”
 *len apn 长度
 *
 * Return:
 * none
 */
 void ctel_set_pdp_apn(char *apn,int len);

 /*!
 * Description:
 * 设置ptt服务日志等级
 *
 * Rarameter:
 * level 日志输出级别
 *
 * Return:
 * none
 */
 void ctel_set_log_level(LOG_LEVEL level);

  /*!
 * Description:
 * 获取当前ptt服务的版本
 *
 * Rarameter:
 *
 * Return:
 * 版本号 unsigned long  类型，便于常看版本日期
 */
 unsigned long ctel_get_version();

  /*!
 * Description:
 * 与服务器同步时间
 *
 * Rarameter:
 *
 * Return:
 */
 void ctel_sync_time();

 /**********************************************************************************************************************************/
 /*!
 * Description:
 * 上报当前位置信息
 *
 * Rarameter:
 * longitude 经度 latitude 纬度
 *
 * Return:
 * 小于1 失败 =1 成功
 */

 int ctel_report_location_info(double longitude,double latitude);

 /*!
 * Description:
 * 查询终端群组信息
 *
 * Rarameter:
 * none
 *
 * Return:
 * none
 */
 void ctel_query_group();

/*!
 * Description:
 * 进入群组
 *
 * Rarameter:
 * gid 需要进入的群组id
 *
 * Return:
 * none
 */
 void ctel_enter_group(unsigned int gid);

/*!
 * Description:
 * 查询群组成员
 *
 * Rarameter:
 * gid 群组id
 *
 * Return:
 * none
 */
 void ctel_query_member(unsigned int gid);

 /*!
 * Description:
 * 单呼或者临时群组
 *
 * Rarameter:
 * callnumber 呼叫的用户id数组   size 用户id数目
 *
 * Return:
 *  =1成功 < 1 失败
 */
 int ctel_call_members(unsigned int callnumber[],int size);

 /*!
 * Description:
 * 按下ptt对讲
 *
 * Rarameter:
 *none
 *
 * Return:
 *  =1成功 < 1失败
 */
 int ctel_press_ppt_button();

 /*!
 * Description:
 * 松开ptt对讲
 *
 * Rarameter:
 *none
 *
 * Return:
 * none
 */
 void ctel_release_ppt_button();


 /*!
 * Description:
 * 启动ptt服务
 *
 * Rarameter:
 *none
 *
 * Return:
 * -1  回调注册不全 0 失败 1 成功
 */
int ctel_start_pttservice();

/*!
* Description:
* 停止ptt服务
*
* Rarameter:
*none
*
* Return:
*
*/
void ctel_stop_pttservice();


#ifdef __cplusplus
}
#endif

#endif // __CTEL_OEM_API_H__

