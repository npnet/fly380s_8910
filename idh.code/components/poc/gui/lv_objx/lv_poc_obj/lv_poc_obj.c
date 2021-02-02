#ifdef __cplusplus
extern "C" {
#endif

#include "lv_include/lv_poc.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "uart3_gps.h"

#ifndef __GNUC__
#define __attribute__(x)
#endif

#define IDT_POC_MODE 1//录音及播放使用POC模式

/*************************************************
*
*                  EXTERN
*
*************************************************/
extern lv_poc_activity_t * poc_group_list_activity;
extern lv_poc_activity_t * poc_member_list_activity;
extern int pub_lv_poc_get_watchdog_status(void);
extern int lv_poc_cit_get_run_status(void);
extern void lv_poc_record_playback_open(void);

/*************************************************
*
*                  GLOBAL
*
*************************************************/
lv_obj_t                        *lv_poc_base_activity_status_obj       = NULL;
#if FT_POC_NOSTATUS_FLAG
lv_obj_t                        *lv_poc_base_activity_nostatus_obj     = NULL;
#endif
lv_poc_status_bar_t      *lv_poc_status_bar                      = NULL;
lv_poc_status_bar_fptr_t  *lv_poc_status_bar_fptr                  = NULL;

lv_poc_display_t         *lv_poc_base_activity_stabar_control_obj      = NULL;
lv_poc_display_t         *lv_poc_base_activity_stabar_nocontrol_obj    = NULL;

#if FT_POC_NOSTATUS_FLAG
lv_poc_display_t         *lv_poc_base_activity_nostabar_control_obj    = NULL;
lv_poc_display_t         *lv_poc_base_activity_nostabar_nocontrol_obj  = NULL;
#endif

lv_group_t                      *poc_indev_group = NULL;

lv_indev_t               *poc_keypad_dev = NULL;

lv_poc_activity_t       * current_activity = NULL;

nv_poc_setting_msg_t * poc_setting_conf = NULL;

lv_style_t theme_white_style_base = {0};
lv_style_t theme_white_style_list_scroll = {0};
lv_style_t theme_white_style_list_page = {0};
lv_style_t theme_white_style_list_btn_rel = {0};
lv_style_t theme_white_style_list_btn_pr = {0};
lv_style_t theme_white_style_list_btn_ina = {0};
lv_style_t theme_white_style_win_header = {0};
lv_style_t theme_white_style_idle_big_clock = {0};
lv_style_t theme_white_style_idle_date_label = {0};
lv_style_t theme_white_style_idle_msg_label = {0};
lv_style_t theme_white_style_switch_bg = {0};
lv_style_t theme_white_style_switch_indic = {0};
lv_style_t theme_white_style_switch_knob_off = {0};
lv_style_t theme_white_style_switch_knob_on = {0};
lv_style_t theme_white_style_rb = {0};
lv_style_t theme_white_style_cb = {0};
lv_style_t theme_white_style_about_label = {0};
lv_style_t theme_white_style_fota_label = {0};
lv_style_t theme_white_style_cit_label = {0};
lv_style_t theme_white_style_status_bar = {0};
lv_style_t theme_white_style_status_bar_time = {0};
lv_style_t theme_white_style_control = {0};

#ifdef CONFIG_POC_GUI_CHOICE_THEME_SUPPORT
lv_style_t theme_black_style_base = {0};
lv_style_t theme_black_style_list_scroll = {0};
lv_style_t theme_black_style_list_page = {0};
lv_style_t theme_black_style_list_btn_rel = {0};
lv_style_t theme_black_style_list_btn_pr = {0};
lv_style_t theme_black_style_list_btn_ina = {0};
lv_style_t theme_black_style_win_header = {0};
lv_style_t theme_black_style_idle_big_clock = {0};
lv_style_t theme_black_style_idle_date_label = {0};
lv_style_t theme_black_style_idle_msg_label = {0};
lv_style_t theme_black_style_switch_bg = {0};
lv_style_t theme_black_style_switch_indic = {0};
lv_style_t theme_black_style_switch_knob_off = {0};
lv_style_t theme_black_style_switch_knob_on = {0};
lv_style_t theme_black_style_rb = {0};
lv_style_t theme_black_style_cb = {0};
lv_style_t theme_black_style_about_label = {0};
lv_style_t theme_black_style_fota_label = {0};
lv_style_t theme_black_style_cit_label = {0};
lv_style_t theme_black_style_status_bar = {0};
lv_style_t theme_black_style_status_bar_time = {0};
lv_style_t theme_black_style_control = {0};
#endif


/*************************************************
*
*                  LOCAL
*
*************************************************/
static char lv_poc_time[LV_POC_STABAR_TIME_LEN_DEFAULT]   =   {0};
static lv_poc_activity_list_t *lv_poc_activity_list = NULL;
static lv_signal_cb_t ancient_signal_func = NULL;
static lv_design_cb_t ancient_design_func = NULL;
static lv_event_cb_t  ancient_event_func  = NULL;
static status_bar_task_t status_bar_task_ext[LV_POC_STABAR_TASK_EXT_LENGTH];
static lv_poc_activity_t * _idle_activity = NULL;
static char lv_poc_refresh_ui_state = 0;
static POC_LONGPRESS_MSG_TYPE_T long_press_msg = 0;
#define LV_POC_STACK_SIZE   100
static struct _lv_poc_stack_t
{
    int size;
    void *activity[LV_POC_STACK_SIZE];
} lv_poc_statck;

static lv_poc_status_t prv_lv_poc_member_list_add(lv_poc_oem_member_list *member_list_obj, const char * name, bool is_online, void * information);
static void prv_lv_poc_member_list_clear(lv_poc_oem_member_list *member_list_obj);
static int prv_lv_poc_member_list_get_information(lv_poc_oem_member_list *member_list_obj, const char * name, void *** information);
static void prv_lv_poc_member_list_refresh(lv_poc_oem_member_list *member_list_obj);
static void prv_lv_poc_member_list_refresh_with_data(lv_poc_oem_member_list *member_list_obj);
static lv_poc_status_t prv_lv_poc_member_list_set_state(lv_poc_oem_member_list *member_list_obj, const char * name, void * information, bool is_online);
static lv_poc_status_t prv_lv_poc_member_list_is_exists(lv_poc_oem_member_list *member_list_obj, const char * name, void * information);
static lv_poc_status_t prv_lv_poc_member_list_get_state(lv_poc_oem_member_list *member_list_obj, const char * name, void * information);
static void prv_lv_poc_group_member_list_activity(char *grpname);

static int prv_lv_poc_group_list_get_information(lv_poc_oem_group_list *group_list_obj, const char * name, void *** information);
static void prv_lv_poc_group_list_refresh(lv_poc_oem_group_list *group_list_obj);
static void prv_lv_poc_group_list_refresh_with_data(lv_poc_oem_group_list *group_list_obj);
static lv_poc_status_t prv_lv_poc_group_list_is_exists(lv_poc_oem_group_list *group_list_obj, const char * name, void * information);
static lv_poc_status_t prv_lv_poc_group_list_lock_group(lv_poc_oem_group_list *group_list_obj, lv_poc_group_oprator_type opt);

static void prv_lv_poc_idle_set_page2_note_func(lv_poc_idle_page2_display_t msg_type, int num, ...);

static bool prv_lvPocLedIdtCom_Msg_func(LVPOCIDTCOM_Led_SignalType_t signal, bool steals);

static void prv_lv_poc_member_call_open(void * information);

static void prv_lv_poc_member_call_close(void);

static void prv_lv_poc_build_tempgrp_get_member_list(void);

static void prv_lv_poc_build_tmpgrp_open(char *grpname);

static void prv_lv_poc_build_tmpgrp_close(int type);

static bool prv_lv_poc_notation_msg(lv_poc_notation_msg_type_t msg_type, const uint8_t *text_1, const uint8_t *text_2);

static __attribute__((unused)) lv_poc_activity_attribute_cb_set_obj prv_lv_poc_activity_attribute_cb_set = {
    .member_list = {
        {
            .activity_id = ACT_ID_POC_MEMBER_LIST,
            .active = false,
            .clear = lv_poc_member_list_clear,
            .get_info = lv_poc_member_list_get_information,
            .refresh = lv_poc_member_list_refresh,
            .refresh_with_data = lv_poc_member_list_refresh_with_data,
            .set_state = lv_poc_member_list_set_state,
            .exists = lv_poc_member_list_is_exists,
            .get_state = lv_poc_member_list_get_state,
            .group_member_act = lv_poc_memberlist_activity_open,
        },

        {
            .activity_id = ACT_ID_POC_MEMBER_CALL,
            .active = false,
            .clear = lv_poc_member_call_clear,
            .get_info = lv_poc_member_call_get_information,
            .refresh = NULL,
            .refresh_with_data = NULL,
            .set_state = lv_poc_member_call_set_state,
            .exists = lv_poc_member_call_is_exists,
            .get_state = lv_poc_member_call_get_state,
        },

        {0},

        {0},

        {0},

        {0},

        {0},

        {0},

        {0},
    },

    .group_list = {
        {
            .activity_id = ACT_ID_POC_GROUP_LIST,
            .active = false,
            .get_info = lv_poc_group_list_get_information,
            .refresh = lv_poc_group_list_refresh,
            .refresh_with_data = lv_poc_group_list_refresh_with_data,
            .exists = lv_poc_group_list_is_exists,
            .lock_group = lv_poc_group_list_monitor_group,
        },

        {0},

        {0},

        {0},

        {0},

        {0},

        {0},

        {0},

        {0},

        {0},
    },

    .note = lv_poc_notation_msg,
};

__attribute__((unused)) lv_poc_activity_attribute_cb_set lv_poc_activity_func_cb_set = {
    .member_list = {
        .add = prv_lv_poc_member_list_add,
        .clear = prv_lv_poc_member_list_clear,
        .get_info = prv_lv_poc_member_list_get_information,
        .refresh = prv_lv_poc_member_list_refresh,
        .refresh_with_data = prv_lv_poc_member_list_refresh_with_data,
        .set_state = prv_lv_poc_member_list_set_state,
        .exists = prv_lv_poc_member_list_is_exists,
        .get_state = prv_lv_poc_member_list_get_state,
    },

    .group_list = {
        .get_info = prv_lv_poc_group_list_get_information,
        .refresh = prv_lv_poc_group_list_refresh,
        .refresh_with_data = prv_lv_poc_group_list_refresh_with_data,
        .exists = prv_lv_poc_group_list_is_exists,
        .lock_group = prv_lv_poc_group_list_lock_group,
    },

    .window_note = prv_lv_poc_notation_msg,
    .idle_note = prv_lv_poc_idle_set_page2_note_func,
    .status_led = prv_lvPocLedIdtCom_Msg_func,
    .member_call_open = prv_lv_poc_member_call_open,
    .member_call_close = prv_lv_poc_member_call_close,
	.build_tmp_get_info = prv_lv_poc_build_tempgrp_get_member_list,
    .build_tmp_open = prv_lv_poc_build_tmpgrp_open,
    .build_tmp_close = prv_lv_poc_build_tmpgrp_close,
	.group_member_activity_open = prv_lv_poc_group_member_list_activity,
};


/*电池图标*/
const lv_img_dsc_t *battery_img_dispaly[9] = { &stat_sys_battery_charge_anim0
												,&stat_sys_battery_charge_anim15
												//,&stat_sys_battery_charge_anim28
												,&stat_sys_battery_charge_anim43
												,&stat_sys_battery_charge_anim57
												,&stat_sys_battery_charge_anim71
												,&stat_sys_battery_charge_anim85
												,&stat_sys_battery_charge_anim100};


static __attribute__((const)) lv_poc_activity_attribute_cb_set_obj * lv_poc_get_activity_attribute_cb_set_obj(void)
{
	return &prv_lv_poc_activity_attribute_cb_set;
}

/*************************************************
*
*                  STATIC DEFINE
*
*************************************************/

/*******************
*     NAME:    lv_poc_theme_init
*   AUTHOR:    lugj
* DESCRIPT:    初始化对讲机主题
*     DATE:    2019-10-24
********************/
static bool lv_poc_theme_init(void);

/*******************
*     NAME:    lv_poc_status_bar_init
*   AUTHOR:    lugj
* DESCRIPT:    初始化对讲机状态栏
*     DATE:    2019-10-24
********************/
static bool lv_poc_status_bar_init(void);

/*******************
*     NAME:    lv_poc_init_stabar_time_label
*   AUTHOR:    lugj
* DESCRIPT:    初始化状态栏时间label
*     DATE:    2019-10-25
********************/
static bool lv_poc_init_stabar_time_label(void);

/*******************
*     NAME:    lv_poc_init_stabar_battery_img
*   AUTHOR:    lugj
* DESCRIPT:    初始化状态栏电池图标
*     DATE:    2019-10-25
********************/
static bool lv_poc_init_stabar_battery_img(void);

///*******************
//*     NAME:    lv_poc_init_stabar_operator_img
//* DESCRIPT:    初始化状态栏运营商图标
//*     DATE:    2020-12-14
//********************/
//static bool lv_poc_init_stabar_operator_img(void);

/*******************
*     NAME:    lv_poc_stabar_sim_clean
*   AUTHOR:    lugj
* DESCRIPT:    清空状态sim图标
*     DATE:    2020-1-20
********************/
static void lv_poc_stabar_sim_clean(lv_poc_status_bar_sim_obj_t * sim_obj);

/*******************
*     NAME:    lv_poc_init_stabar_sim1_img
*   AUTHOR:    lugj
* DESCRIPT:    初始化状态sim1图标
*     DATE:    2019-10-25
********************/
static bool lv_poc_init_stabar_sim1_img(void);

/*******************
*     NAME:    lv_poc_init_stabar_sim2_img
*   AUTHOR:    lugj
* DESCRIPT:    初始化状态sim2图标
*     DATE:    2019-10-25
********************/
static bool lv_poc_init_stabar_sim2_img(void);

/*******************
*     NAME:    lv_poc_init_stabar_signal_img
*   AUTHOR:    lugj
* DESCRIPT:    初始化状态signal图标
*     DATE:    2019-10-25
********************/
static bool lv_poc_init_stabar_signal_img(void);

#if 0
/*******************
*     NAME:    lv_poc_display_init
*   AUTHOR:    lugj
* DESCRIPT:    初始化显示区域
*     DATE:    2019-10-25
********************/
static bool lv_poc_display_init(void);
#endif

/*******************
*     NAME:    lv_poc_control_init
*   AUTHOR:    lugj
* DESCRIPT:    初始化控制区域
*     DATE:    2019-10-25
********************/
static void lv_poc_control_init(lv_poc_activity_t *activity,
                                       const char *left_text,
                                       const char *middle_text,
                                       const char *right_text);

/*******************
*     NAME:    lv_poc_activity_list_init
*   AUTHOR:    lugj
* DESCRIPT:    初始化activity查询表
*     DATE:    2019-10-25
********************/
static void	lv_poc_activity_list_init(void);


/*******************
*     NAME:    lv_poc_activity_list_insert
*   AUTHOR:    lugj
* DESCRIPT:    将new_activity插入activity查询表
*     DATE:    2019-10-30
********************/
static bool	lv_poc_activity_list_insert(lv_poc_activity_t * new_activity);



/*******************
*     NAME:    lv_poc_activity_list_delete
*   AUTHOR:    lugj
* DESCRIPT:    将old_activity从activity查询表中删除
*     DATE:    2019-10-30
********************/
static void	lv_poc_activity_list_delete(lv_poc_activity_t * old_activity);

/*******************
*     NAME:    lv_poc_is_keypad_msg
*   AUTHOR:    lugj
* DESCRIPT:    <C5>ж<CF><CA>?<F1><CA>?<B4><BC><FC><CF><FB>?
*     DATE:    2019-1-20
********************/
static bool    lv_poc_is_keypad_msg(lv_signal_t sign, void * param);

/*******************
*     NAME:    lv_poc_signal_func
*   AUTHOR:    lugj
* DESCRIPT:    对讲机界面signal函数
*     DATE:    2019-10-31
********************/
static lv_res_t lv_poc_signal_cb(lv_obj_t * obj, lv_signal_t sign, void * param);

/*******************
*     NAME:    lv_poc_design_func
*   AUTHOR:    lugj
* DESCRIPT:    对讲机界面design函数
*     DATE:    2019-10-25
********************/
static bool lv_poc_design_cb(lv_obj_t * obj, const lv_area_t * mask_p, lv_design_mode_t mode);

/*******************
*     NAME:    lv_poc_event_cb
*   AUTHOR:    lugj
* DESCRIPT:    对讲机界面event函数
*     DATE:    2020-03-30
********************/
static void lv_poc_event_cb(lv_obj_t * obj, lv_event_t event);

/*******************
*     NAME:    lv_poc_stack_init
*   AUTHOR:    lugj
* DESCRIPT:    初始化界面管理栈
*     DATE:    2019-10-25
********************/
static void lv_poc_stack_init(void);

/*******************
*     NAME:    lv_poc_stack_push
*   AUTHOR:    lugj
* DESCRIPT:    压入界面管理栈
*     DATE:    2019-10-25
********************/
static bool lv_poc_stack_push(lv_poc_activity_t *activity);

/*******************
*     NAME:    lv_poc_stack_pop
*   AUTHOR:    lugj
* DESCRIPT:    弹出界面管理栈并删除
*     DATE:    2019-10-25
********************/
static lv_poc_activity_t * lv_poc_stack_pop(void);

/*******************
*     NAME:    lv_poc_stack_top
*   AUTHOR:    lugj
* DESCRIPT:    查看界面管理栈顶元素
*     DATE:    2019-10-25
********************/
static lv_poc_activity_t * lv_poc_stack_top(void);


/*******************
*     NAME:    lv_poc_stabar_battery_task
*   AUTHOR:    lugj
* DESCRIPT:    状态栏电池图标动态更新任务
*     DATE:    2020-1-22
********************/
static void lv_poc_stabar_battery_task(void);

/*******************
*     NAME:    lv_poc_stabar_signal_task
*   AUTHOR:    lugj
* DESCRIPT:    状态栏SIM图标动态更新任务
*     DATE:    2020-1-22
********************/
static void lv_poc_stabar_signal_task(void);

/*******************
*     NAME:    lv_exec_task
*   AUTHOR:    lugj
* DESCRIPT:    向特定目标发送sign
*     DATE:    2020-1-22
********************/
static void lv_exec_task(lv_task_t * task);

#ifdef CONFIG_POC_GUI_GPS_SUPPORT
/*******************
*     NAME:    lv_poc_init_stabar_gps_img
* DESCRIPT:    初始化状态栏GPS图标
*     DATE:    2020-08-03
********************/
static bool lv_poc_init_stabar_gps_img(lv_obj_t ** align_obj);
#endif

/*************************************************
*
*                  EXTERN
*
*************************************************/


/*************************************************
*
*                  STATIC
*
*************************************************/






/*******************
*     NAME:    lv_poc_setting_init
*   AUTHOR:    lugj
* DESCRIPT:    初始化对讲机设置
*     DATE:    2019-10-24
********************/
bool lv_poc_setting_init(void)
{
    lv_poc_setting_conf_init();
    poc_setting_conf = lv_poc_setting_conf_read();
	if(!pub_lv_poc_get_watchdog_status())//no watchdog
	{
		poc_set_lcd_blacklight(poc_setting_conf->screen_brightness);
	}
	poc_set_lcd_bright_time(poc_setting_conf->screen_bright_time);
#if IDT_POC_MODE
	lv_poc_set_volum(POC_MMI_VOICE_VOICE, poc_setting_conf->voicevolume, false, false);
#else
	lv_poc_set_volum(POC_MMI_VOICE_PLAY, poc_setting_conf->volume, false, false);
#endif
#ifdef CONFIG_POC_GUI_KEYPAD_LIGHT_SUPPORT
	poc_keypad_led_init();
#endif
	lv_poc_ear_ppt_key_init();
	lv_poc_volum_set_reconfig_status(lv_poc_volum_key_init);
	poc_ext_pa_init();
	lv_poc_virt_at_init();
	lv_poc_boot_timeing_task_create();
	lv_poc_set_loopback_recordplay(false);
	lv_poc_opt_refr_status(LVPOCUNREFOPTIDTCOM_SIGNAL_NUMBLE_STATUS);

    return true;
}

/*******************
*     NAME:    lv_poc_theme_init
*   AUTHOR:    lugj
* DESCRIPT:    初始化对讲机主题
*     DATE:    2019-10-24
********************/
static bool lv_poc_theme_init(void)
{
	#define LV_POC_SWITCH_ON_OFF_INDIC_COLOR LV_COLOR_MAKE(0x77, 0x77, 0x77);
// 初始化白色主题
    lv_style_copy(&theme_white_style_base,&lv_style_scr);
    theme_white_style_base.body.main_color = LV_COLOR_WHITE;
    theme_white_style_base.body.grad_color = LV_COLOR_WHITE;
    theme_white_style_base.body.radius = 0;
    theme_white_style_base.body.opa = 255;
    theme_white_style_base.image.color = LV_COLOR_MAKE(0xFF, 0xF5, 0x98);//天蓝色
    theme_white_style_base.image.intense = 60;
	theme_white_style_base.image.opa = 240;

    lv_style_copy(&theme_white_style_list_scroll, &lv_style_scr);
    lv_style_copy(&theme_white_style_list_page, &theme_white_style_list_scroll);

    theme_white_style_list_page.body.main_color = LV_COLOR_MAKE(0xff, 0xff, 0xff);
    theme_white_style_list_page.body.grad_color = LV_COLOR_MAKE(0xff, 0xff, 0xff);
    theme_white_style_list_page.body.padding.top = 0;
    theme_white_style_list_page.body.padding.bottom = 0;
    theme_white_style_list_page.body.padding.left = 0;
    theme_white_style_list_page.body.padding.right = 0;
    theme_white_style_list_page.body.padding.inner = 0;
    theme_white_style_list_page.body.border.width = 0;

    theme_white_style_list_scroll.body.padding.top = 0;
    theme_white_style_list_scroll.body.padding.bottom = 0;
    theme_white_style_list_scroll.body.padding.left = 0;
    theme_white_style_list_scroll.body.padding.right = 0;
    theme_white_style_list_scroll.body.padding.inner = 1;
    theme_white_style_list_scroll.body.border.width = 0;
    theme_white_style_list_scroll.body.main_color = LV_COLOR_MAKE(0xff, 0xff, 0xff);
    theme_white_style_list_scroll.body.grad_color = LV_COLOR_MAKE(0xff, 0xff, 0xff);

    lv_style_copy(&theme_white_style_list_btn_rel, &theme_white_style_list_scroll);
    theme_white_style_list_btn_rel.text.color = LV_COLOR_MAKE(0x11, 0x11, 0x11);
    theme_white_style_list_btn_rel.text.font = (lv_font_t *)poc_setting_conf->font.list_btn_current_font;
    theme_white_style_list_btn_rel.body.padding.inner = 5;
    theme_white_style_list_btn_rel.body.padding.left = 2;
    theme_white_style_list_btn_rel.body.padding.right = 3;
    //theme_white_style_list_btn_rel.body.opa = 0;

    lv_style_copy(&theme_white_style_list_btn_pr, &theme_white_style_list_btn_rel);
    theme_white_style_list_btn_pr.body.main_color = LV_COLOR_BLUE;
    theme_white_style_list_btn_pr.body.grad_color = LV_COLOR_BLUE;
    theme_white_style_list_btn_pr.body.opa = 0xaa;
    theme_white_style_list_btn_pr.text.font = (lv_font_t *)poc_setting_conf->font.list_btn_current_font;

    lv_style_copy(&theme_white_style_list_btn_ina, &theme_white_style_list_btn_rel);
    theme_white_style_list_btn_ina.body.main_color = LV_COLOR_GRAY;
    theme_white_style_list_btn_ina.body.grad_color = LV_COLOR_GRAY;
    theme_white_style_list_btn_ina.text.font = (lv_font_t *)poc_setting_conf->font.list_btn_current_font;


    lv_style_copy(&theme_white_style_win_header, &lv_style_scr);
    theme_white_style_win_header.body.main_color = LV_COLOR_MAKE(0xee,0xb7,0x00);
    theme_white_style_win_header.body.grad_color = LV_COLOR_MAKE(0xee,0xb7,0x00);
    theme_white_style_win_header.body.opa = 255;
    theme_white_style_win_header.text.font = (lv_font_t *)poc_setting_conf->font.win_title_font;
    theme_white_style_win_header.text.color = LV_COLOR_MAKE(0xee,0xee,0xee);

    lv_style_copy(&theme_white_style_idle_big_clock, &lv_style_transp_tight);
    theme_white_style_idle_big_clock.text.color = LV_COLOR_WHITE;
    theme_white_style_idle_big_clock.text.font = (lv_font_t *)poc_setting_conf->font.idle_big_clock_font;

    lv_style_copy(&theme_white_style_idle_date_label, &lv_style_plain);
    theme_white_style_idle_date_label.text.color = LV_COLOR_WHITE;
    theme_white_style_idle_date_label.text.font = (lv_font_t *)poc_setting_conf->font.idle_date_label_font;

    lv_style_copy(&theme_white_style_idle_msg_label, &lv_style_transp_tight);
    theme_white_style_idle_msg_label.text.color = LV_COLOR_WHITE;
    theme_white_style_idle_msg_label.text.font = (lv_font_t *)poc_setting_conf->font.idle_page2_msg_font;

    lv_style_copy(&theme_white_style_switch_bg, &lv_style_pretty);
    theme_white_style_switch_bg.body.radius = LV_RADIUS_CIRCLE;
    theme_white_style_switch_bg.body.main_color = LV_COLOR_MAKE(0x3f, 0x38, 0x3f);
    theme_white_style_switch_bg.body.grad_color = LV_COLOR_MAKE(0x3f, 0x38, 0x3f);
    theme_white_style_switch_bg.body.padding.top = 2;
    theme_white_style_switch_bg.body.padding.bottom = 2;
    theme_white_style_switch_bg.body.padding.left = 2;
    theme_white_style_switch_bg.body.padding.right = 2;
    theme_white_style_switch_bg.body.border.width = 0;

    lv_style_copy(&theme_white_style_switch_indic, &lv_style_pretty_color);
    theme_white_style_switch_indic.body.radius = LV_RADIUS_CIRCLE;
    theme_white_style_switch_indic.body.main_color = LV_COLOR_MAKE(0xef, 0xc8, 0x9f);
    theme_white_style_switch_indic.body.grad_color = LV_COLOR_MAKE(0xef, 0xc8, 0x9f);
    theme_white_style_switch_indic.body.padding.top = 0;
    theme_white_style_switch_indic.body.padding.bottom = 0;
    theme_white_style_switch_indic.body.padding.left = 0;
    theme_white_style_switch_indic.body.padding.right = 0;
    theme_white_style_switch_indic.body.padding.inner= 0;
    theme_white_style_switch_indic.body.border.width = 0;

    lv_style_copy(&theme_white_style_switch_knob_off, &lv_style_pretty);
    theme_white_style_switch_knob_off.body.radius = LV_RADIUS_CIRCLE;
    theme_white_style_switch_knob_off.body.shadow.width = 0;
    theme_white_style_switch_knob_off.body.shadow.type = LV_SHADOW_BOTTOM;
    theme_white_style_switch_knob_off.body.border.width = 0;
    theme_white_style_switch_knob_off.body.main_color = LV_POC_SWITCH_ON_OFF_INDIC_COLOR;
    theme_white_style_switch_knob_off.body.grad_color = LV_POC_SWITCH_ON_OFF_INDIC_COLOR;

    lv_style_copy(&theme_white_style_switch_knob_on, &lv_style_pretty);
    theme_white_style_switch_knob_on.body.radius = LV_RADIUS_CIRCLE;
    theme_white_style_switch_knob_on.body.shadow.width = 0;
    theme_white_style_switch_knob_on.body.shadow.type = LV_SHADOW_BOTTOM;
    theme_white_style_switch_knob_on.body.border.width = 0;
    theme_white_style_switch_knob_on.body.main_color = LV_POC_SWITCH_ON_OFF_INDIC_COLOR;
    theme_white_style_switch_knob_on.body.grad_color = LV_POC_SWITCH_ON_OFF_INDIC_COLOR;

    lv_style_copy(&theme_white_style_rb, &lv_style_pretty_color);
    theme_white_style_rb.body.radius = LV_RADIUS_CIRCLE;
    theme_white_style_rb.body.shadow.width = 0;
    theme_white_style_rb.body.shadow.type = LV_SHADOW_BOTTOM;
    theme_white_style_rb.body.border.width = 0;

    lv_style_copy(&theme_white_style_cb, &lv_style_pretty_color);
    theme_white_style_cb.body.radius = LV_RADIUS_CIRCLE;
    theme_white_style_cb.body.shadow.width = 0;
    theme_white_style_cb.body.shadow.type = LV_SHADOW_BOTTOM;
    theme_white_style_cb.body.border.width = 0;

    lv_style_copy(&theme_white_style_about_label, &theme_white_style_list_scroll);
    theme_white_style_about_label.text.color = LV_COLOR_MAKE(0x11, 0x11, 0x11);
    theme_white_style_about_label.text.font = (lv_font_t *)poc_setting_conf->font.about_label_current_font;

    lv_style_copy(&theme_white_style_fota_label, &theme_white_style_list_scroll);
    theme_white_style_fota_label.text.color = LV_COLOR_MAKE(0x11, 0x11, 0x11);
    theme_white_style_fota_label.text.font = (lv_font_t *)poc_setting_conf->font.fota_label_current_font;

    lv_style_copy(&theme_white_style_cit_label, &theme_white_style_list_scroll);
    theme_white_style_cit_label.text.color = LV_COLOR_MAKE(0x11, 0x11, 0x11);
    theme_white_style_cit_label.text.font = (lv_font_t *)poc_setting_conf->font.cit_label_current_font;

	lv_style_copy(&theme_white_style_status_bar,&lv_style_plain);
    theme_white_style_status_bar.body.main_color = LV_COLOR_MAKE(0x00,0x00,0x00);
    theme_white_style_status_bar.body.grad_color = LV_COLOR_MAKE(0x00,0x00,0x00);
    theme_white_style_status_bar.body.radius = 0;
    theme_white_style_status_bar.body.opa = 240;

    lv_style_copy(&theme_white_style_status_bar_time,&lv_style_plain);
    theme_white_style_status_bar_time.text.color = LV_COLOR_WHITE;
    theme_white_style_status_bar_time.text.font = (lv_font_t *)poc_setting_conf->font.status_bar_time_font;

    lv_style_copy(&theme_white_style_control,&lv_style_plain);
    theme_white_style_control.body.main_color = LV_COLOR_MAKE(0x88,0x33,0x00);
    theme_white_style_control.body.grad_color = LV_COLOR_MAKE(0x88,0x63,0x00);
    theme_white_style_control.text.color = LV_COLOR_WHITE;
    theme_white_style_control.text.font = (lv_font_t *)poc_setting_conf->font.activity_control_font;

#ifdef CONFIG_POC_GUI_CHOICE_THEME_SUPPORT
// 初始化黑色主题
    lv_style_copy(&theme_black_style_base,&lv_style_transp);
    theme_black_style_base.body.main_color = LV_COLOR_MAKE(0x00,0x00,0x00);
    theme_black_style_base.body.grad_color = LV_COLOR_MAKE(0x00,0x00,0x00);
    theme_black_style_base.body.radius = 0;
    theme_black_style_base.body.opa = 255;
    theme_black_style_base.image.color = LV_COLOR_BLUE;
    theme_black_style_base.image.intense = 0x33;

    lv_style_copy(&theme_black_style_list_scroll, &lv_style_scr);
    lv_style_copy(&theme_black_style_list_page, &theme_black_style_list_scroll);

    theme_black_style_list_page.body.main_color = LV_COLOR_MAKE(0x10, 0x10, 0x10);
    theme_black_style_list_page.body.grad_color = LV_COLOR_MAKE(0x10, 0x10, 0x10);
    theme_black_style_list_page.body.padding.top = 0;
    theme_black_style_list_page.body.padding.bottom = 0;
    theme_black_style_list_page.body.padding.left = 0;
    theme_black_style_list_page.body.padding.right = 0;
    theme_black_style_list_page.body.padding.inner = 0;
    theme_black_style_list_page.body.border.width = 0;

    theme_black_style_list_scroll.body.padding.top = 0;
    theme_black_style_list_scroll.body.padding.bottom = 0;
    theme_black_style_list_scroll.body.padding.left = 0;
    theme_black_style_list_scroll.body.padding.right = 0;
    theme_black_style_list_scroll.body.padding.inner = 1;
    theme_black_style_list_scroll.body.border.width = 0;
    theme_black_style_list_scroll.body.main_color = LV_COLOR_MAKE(0x20, 0x20, 0x20);
    theme_black_style_list_scroll.body.grad_color = LV_COLOR_MAKE(0x20, 0x20, 0x20);

    lv_style_copy(&theme_black_style_list_btn_rel, &theme_black_style_list_scroll);
    theme_black_style_list_btn_rel.text.color = LV_COLOR_MAKE(0xee, 0xee, 0xee);
    theme_black_style_list_btn_rel.text.font = (lv_font_t *)poc_setting_conf->font.list_btn_current_font;
    theme_black_style_list_btn_rel.body.padding.inner = 5;
    theme_black_style_list_btn_rel.body.padding.left = 2;
    theme_black_style_list_btn_rel.body.padding.right = 3;
    theme_black_style_list_btn_rel.image.color = LV_COLOR_GRAY;
    theme_black_style_list_btn_rel.image.intense = 230;
    theme_black_style_list_btn_rel.image.opa = 255;
    //theme_black_style_list_btn_rel.body.opa = 0;

    lv_style_copy(&theme_black_style_list_btn_pr, &theme_black_style_list_btn_rel);
    theme_black_style_list_btn_pr.body.main_color = LV_COLOR_MAKE(0xbb,0x00,0x00);
    theme_black_style_list_btn_pr.body.grad_color = LV_COLOR_MAKE(0xbb,0x00,0x00);
    theme_black_style_list_btn_pr.text.font = (lv_font_t *)poc_setting_conf->font.list_btn_current_font;
    theme_black_style_list_btn_pr.image.color = LV_COLOR_GRAY;
    theme_black_style_list_btn_pr.image.intense = 230;
    theme_black_style_list_btn_pr.image.opa = 255;
    theme_black_style_list_btn_pr.body.opa = 0xaa;

    lv_style_copy(&theme_black_style_list_btn_ina, &theme_black_style_list_btn_rel);
    theme_black_style_list_btn_ina.body.main_color = LV_COLOR_MAKE(0x7f,0x7f,0x7f);
    theme_black_style_list_btn_ina.body.grad_color = LV_COLOR_MAKE(0x7f,0x7f,0x7f);
    theme_black_style_list_btn_ina.text.font = (lv_font_t *)poc_setting_conf->font.list_btn_current_font;
    theme_black_style_list_btn_ina.image.color = LV_COLOR_GRAY;
    theme_black_style_list_btn_ina.image.intense = 255;
    theme_black_style_list_btn_ina.image.opa = 255;


    lv_style_copy(&theme_black_style_win_header, &lv_style_scr);
    theme_black_style_win_header.body.main_color = LV_COLOR_MAKE(0x60,0x60,0x60);
    theme_black_style_win_header.body.grad_color = LV_COLOR_MAKE(0x60,0x60,0x60);
    theme_black_style_win_header.body.opa = 255;
    theme_black_style_win_header.text.font = (lv_font_t *)poc_setting_conf->font.win_title_font;
    theme_black_style_win_header.text.color = LV_COLOR_MAKE(0x11,0x11,0x11);

    lv_style_copy(&theme_black_style_idle_big_clock, &lv_style_transp_tight);
    theme_black_style_idle_big_clock.text.color = LV_COLOR_WHITE;
    theme_black_style_idle_big_clock.text.font = (lv_font_t *)poc_setting_conf->font.idle_big_clock_font;

    lv_style_copy(&theme_black_style_idle_date_label, &lv_style_plain);
    theme_black_style_idle_date_label.text.color = LV_COLOR_WHITE;
    theme_black_style_idle_date_label.text.font = (lv_font_t *)poc_setting_conf->font.idle_date_label_font;

    lv_style_copy(&theme_black_style_idle_msg_label, &lv_style_transp_tight);
    theme_black_style_idle_msg_label.text.color = LV_COLOR_WHITE;
    theme_black_style_idle_msg_label.text.font = (lv_font_t *)poc_setting_conf->font.idle_page2_msg_font;

    lv_style_copy(&theme_black_style_switch_bg, &lv_style_pretty);
    theme_black_style_switch_bg.body.radius = LV_RADIUS_CIRCLE;
    theme_black_style_switch_bg.body.main_color = LV_COLOR_MAKE(0xf0, 0xc7, 0xc0);
    theme_black_style_switch_bg.body.grad_color = LV_COLOR_MAKE(0xf0, 0xc7, 0xc0);
    theme_black_style_switch_bg.body.padding.top = 2;
    theme_black_style_switch_bg.body.padding.bottom = 2;
    theme_black_style_switch_bg.body.padding.left = 2;
    theme_black_style_switch_bg.body.padding.right = 2;
    theme_black_style_switch_bg.body.border.width = 0;

    lv_style_copy(&theme_black_style_switch_indic, &lv_style_pretty_color);
    theme_black_style_switch_indic.body.radius = LV_RADIUS_CIRCLE;
    theme_black_style_switch_indic.body.main_color = LV_COLOR_MAKE(0x60, 0x37, 0x10);
    theme_black_style_switch_indic.body.grad_color = LV_COLOR_MAKE(0x60, 0x37, 0x10);
    theme_black_style_switch_indic.body.padding.top = 0;
    theme_black_style_switch_indic.body.padding.bottom = 0;
    theme_black_style_switch_indic.body.padding.left = 0;
    theme_black_style_switch_indic.body.padding.right = 0;
    theme_black_style_switch_indic.body.padding.inner= 0;
    theme_black_style_switch_indic.body.border.width = 0;

    lv_style_copy(&theme_black_style_switch_knob_off, &lv_style_pretty);
    theme_black_style_switch_knob_off.body.radius = LV_RADIUS_CIRCLE;
    theme_black_style_switch_knob_off.body.shadow.width = 0;
    theme_black_style_switch_knob_off.body.shadow.type = LV_SHADOW_BOTTOM;
    theme_black_style_switch_knob_off.body.border.width = 0;
    theme_black_style_switch_knob_off.body.main_color = LV_POC_SWITCH_ON_OFF_INDIC_COLOR;
    theme_black_style_switch_knob_off.body.grad_color = LV_POC_SWITCH_ON_OFF_INDIC_COLOR;

    lv_style_copy(&theme_black_style_switch_knob_on, &lv_style_pretty_color);
    theme_black_style_switch_knob_on.body.radius = LV_RADIUS_CIRCLE;
    theme_black_style_switch_knob_on.body.shadow.width = 0;
    theme_black_style_switch_knob_on.body.shadow.type = LV_SHADOW_BOTTOM;
    theme_black_style_switch_knob_on.body.border.width = 0;
    theme_black_style_switch_knob_on.body.main_color = LV_POC_SWITCH_ON_OFF_INDIC_COLOR;
    theme_black_style_switch_knob_on.body.grad_color = LV_POC_SWITCH_ON_OFF_INDIC_COLOR;

    lv_style_copy(&theme_black_style_rb, &lv_style_pretty_color);
    theme_black_style_rb.body.radius = LV_RADIUS_CIRCLE;
    theme_black_style_rb.body.shadow.width = 0;
    theme_black_style_rb.body.shadow.type = LV_SHADOW_BOTTOM;
    theme_black_style_rb.body.border.width = 0;

    lv_style_copy(&theme_black_style_cb, &lv_style_pretty_color);
    theme_black_style_cb.body.radius = LV_RADIUS_CIRCLE;
    theme_black_style_cb.body.shadow.width = 0;
    theme_black_style_cb.body.shadow.type = LV_SHADOW_BOTTOM;
    theme_black_style_cb.body.border.width = 0;

    lv_style_copy(&theme_black_style_about_label, &theme_black_style_list_scroll);
    theme_black_style_about_label.text.color = LV_COLOR_MAKE(0xee, 0xee, 0xee);
    theme_black_style_about_label.text.font = (lv_font_t *)poc_setting_conf->font.about_label_current_font;

    lv_style_copy(&theme_black_style_fota_label, &theme_black_style_list_scroll);
    theme_black_style_fota_label.text.color = LV_COLOR_MAKE(0xee, 0xee, 0xee);
    theme_black_style_fota_label.text.font = (lv_font_t *)poc_setting_conf->font.fota_label_current_font;

    lv_style_copy(&theme_black_style_cit_label, &theme_black_style_list_scroll);
    theme_black_style_cit_label.text.color = LV_COLOR_MAKE(0xee, 0xee, 0xee);
    theme_black_style_cit_label.text.font = (lv_font_t *)poc_setting_conf->font.cit_label_current_font;

    lv_style_copy(&theme_black_style_status_bar,&lv_style_plain);
    theme_black_style_status_bar.body.main_color = LV_COLOR_MAKE(0x00,0x00,0x00);
    theme_black_style_status_bar.body.grad_color = LV_COLOR_MAKE(0x00,0x00,0x00);
    theme_black_style_status_bar.body.radius = 0;
    theme_black_style_status_bar.body.opa = 240;

    lv_style_copy(&theme_black_style_status_bar_time,&lv_style_plain);
    theme_black_style_status_bar_time.text.color = LV_COLOR_WHITE;
    theme_black_style_status_bar_time.text.font = (lv_font_t *)poc_setting_conf->font.status_bar_time_font;

    lv_style_copy(&theme_black_style_control,&lv_style_plain);
    theme_black_style_control.body.main_color = LV_COLOR_MAKE(0x88,0x33,0x00);
    theme_black_style_control.body.grad_color = LV_COLOR_MAKE(0x88,0x33,0x00);
    theme_black_style_control.text.color = LV_COLOR_WHITE;
    theme_black_style_control.text.font = (lv_font_t *)poc_setting_conf->font.activity_control_font;
#endif
    return true;
}

/*******************
*     NAME:    lv_poc_power_on_delay_refresh_battery_img_task
* DESCRIPT:    延时开始刷电量
*     DATE:    2020-07-20
********************/
static void lv_poc_power_on_delay_refresh_battery_img_task(lv_task_t *task)
{
	lv_poc_status_bar_task_ext_add(lv_poc_stabar_battery_task);
}

/*******************
*     NAME:    lv_poc_status_bar_init
*   AUTHOR:    lugj
* DESCRIPT:    初始化对讲机状态栏
*     DATE:    2019-10-24
********************/
static bool lv_poc_status_bar_init(void)
{
    bool ret_val = true;
    lv_style_t * poc_stabar_style;
    poc_setting_conf = lv_poc_setting_conf_read();
    poc_stabar_style = (lv_style_t *)(poc_setting_conf->theme.current_theme->style_status_bar);

    lv_poc_status_bar = lv_obj_create(lv_scr_act(), NULL);
    lv_obj_set_size(lv_poc_status_bar, LV_POC_STATUS_BAR_HOR_RES, LV_POC_STATUS_BAR_VER_RES);
    lv_obj_set_pos(lv_poc_status_bar, LV_POC_STATUS_BAR_POSITION_X, LV_POC_STATUS_BAR_POSITION_Y);
    lv_obj_set_style(lv_poc_status_bar, poc_stabar_style);

	if(lv_poc_status_bar_fptr == NULL)
	{
    	lv_poc_status_bar_fptr = (lv_poc_status_bar_fptr_t *)lv_mem_alloc(sizeof(lv_poc_status_bar_fptr_t));
	}
	memset(lv_poc_status_bar_fptr, 0, sizeof(lv_poc_status_bar_fptr_t));
    lv_poc_status_bar->user_data = lv_poc_status_bar_fptr;

    lv_poc_init_stabar_time_label();
    lv_poc_init_stabar_battery_img();
    lv_poc_init_stabar_sim1_img();
    lv_poc_init_stabar_sim2_img();
    lv_poc_update_stabar_sim_img();
    lv_poc_init_stabar_signal_img();
	//lv_poc_init_stabar_operator_img();
#ifdef CONFIG_POC_GUI_GPS_SUPPORT
	lv_poc_init_stabar_gps_img(NULL);
#endif
    lv_poc_refr_task_once(lv_poc_power_on_delay_refresh_battery_img_task, LVPOCLISTIDTCOM_LIST_PERIOD_2000, LV_TASK_PRIO_MID);
    memset(status_bar_task_ext, 0, sizeof(status_bar_task_t) * LV_POC_STABAR_TASK_EXT_LENGTH);

    lv_poc_status_bar_task_ext_add(lv_poc_stabar_signal_task);
	lv_task_create(lv_poc_stabar_task, 1000, LV_TASK_PRIO_MID, NULL);
	return ret_val;
}

/*******************
*     NAME:    lv_poc_init_stabar_time_label
*   AUTHOR:    lugj
* DESCRIPT:    初始化状态栏时间label
*     DATE:    2019-10-25
********************/
static bool lv_poc_init_stabar_time_label(void)
{
    bool ret_val = true;
    lv_obj_t *time_label;
    lv_poc_time_t time = {0};
    lv_style_t * label_style;
    poc_setting_conf = lv_poc_setting_conf_read();
    label_style = (lv_style_t *)(poc_setting_conf->theme.current_theme->style_status_bar_time);
    lv_poc_status_bar_fptr->time_formate = LV_POC_STABAR_TIME_FORMATE_DEFAULT;
    lv_poc_status_bar_fptr->time_label  = lv_label_create(lv_poc_status_bar, NULL);
    time_label = lv_poc_status_bar_fptr->time_label;
    lv_label_set_style(time_label, LV_LABEL_STYLE_MAIN,label_style);
    lv_obj_align(time_label, lv_poc_status_bar, LV_ALIGN_IN_LEFT_MID, 2, 2);

    lv_poc_get_time(&time);

    switch(lv_poc_status_bar_fptr->time_formate)
    {
	    case lv_poc_time_format_hhmm:
	    {
	        sprintf(lv_poc_time,"%02d:%02d",time.tm_hour,time.tm_min);
	        break;
	    }

	    case lv_poc_time_format_mmhh:
	    {
	        sprintf(lv_poc_time,"%02d:%02d",time.tm_min,time.tm_hour);
	        break;
	    }

	    case lv_poc_time_format_hhmmss:
	    {
	        sprintf(lv_poc_time,"%02d:%02d:%02d",time.tm_hour,time.tm_min,time.tm_sec);
	        break;
	    }

	    case lv_poc_time_format_sshhmm:
	    {
	        sprintf(lv_poc_time,"%02d:%02d:%02d",time.tm_sec,time.tm_hour,time.tm_min);
	        break;
	    }

	    case lv_poc_time_format_ssmmhh:
	    {
	        sprintf(lv_poc_time,"%02d:%02d:%02d",time.tm_sec,time.tm_min,time.tm_hour);
	        break;
	    }

	    case lv_poc_time_format_mmhhss:
	    {
	        sprintf(lv_poc_time,"%02d:%02d:%02d",time.tm_min,time.tm_hour,time.tm_sec);
	        break;
	    }

	    default:
	    {
	        sprintf(lv_poc_time,"%02d:%02d",time.tm_hour,time.tm_min);
	    }
    }

    lv_label_set_text(time_label, lv_poc_time);
    return ret_val;
}

/*******************
*     NAME:    lv_poc_init_stabar_battery_img
*   AUTHOR:    lugj
* DESCRIPT:    初始化状态栏电池图标
*     DATE:    2019-10-25
********************/
static bool lv_poc_init_stabar_battery_img(void)
{
    bool ret_val = true;
    lv_obj_t *obj = NULL;

    lv_poc_status_bar_fptr->has_battery = lv_poc_get_battery_state();
	lv_poc_status_bar_fptr->battery_img = lv_img_create(lv_poc_status_bar, NULL);
    obj = lv_poc_status_bar_fptr->battery_img;

    lv_img_set_src(obj, lv_poc_get_battery_img());
    lv_obj_set_opa_scale_enable(obj, false);
    lv_obj_align(obj, lv_poc_status_bar, LV_ALIGN_IN_RIGHT_MID, -5, 1);
    return ret_val;
}

///*******************
//*     NAME:    lv_poc_init_stabar_operator_img
//* DESCRIPT:    初始化状态栏运营商图标
//*     DATE:    2020-12-14
//********************/
//static bool lv_poc_init_stabar_operator_img(void)
//{
//    bool ret_val = true;
//	char countrystr[16] = {0};
//
//	if(lv_poc_status_bar_fptr->operator_img == NULL)
//   {
//	   	lv_poc_status_bar_fptr->operator_img = (lv_poc_status_bar_operator_obj_t *)lv_mem_alloc(sizeof(lv_poc_status_bar_operator_obj_t));
//	    memset(lv_poc_status_bar_fptr->operator_img, 0, sizeof(lv_poc_status_bar_operator_obj_t));
//
//		lv_poc_status_bar_fptr->operator_img->align_l_obj = &(lv_poc_status_bar_fptr->time_label);
//		lv_poc_status_bar_fptr->operator_img->align_r_obj = lv_poc_status_bar_fptr->operator_img->align_l_obj;
//
//		lv_poc_status_bar_fptr->operator_img->img = lv_img_create(lv_poc_status_bar, NULL);
//		lv_img_set_auto_size(lv_poc_status_bar_fptr->operator_img->img, false);
//		lv_obj_set_size(lv_poc_status_bar_fptr->operator_img->img, 18, 18);
//	}
//	if(lv_poc_status_bar_fptr->operator_img->img == NULL)
//	{
//		return false;
//	}
//	lv_poc_get_mobile_card_operator(countrystr, true);
//	if(NULL != strstr(countrystr, "CMCC"))
//	{
//		lv_img_set_src(lv_poc_status_bar_fptr->operator_img->img, &optcmcc);
//	}
//	else if(NULL != strstr(countrystr, "CUCC"))
//	{
//		lv_img_set_src(lv_poc_status_bar_fptr->operator_img->img, &optcucc);
//	}
//	else if(NULL != strstr(countrystr, "CTCC"))
//	{
//		lv_img_set_src(lv_poc_status_bar_fptr->operator_img->img, &optctcc);
//	}
//	lv_obj_align(lv_poc_status_bar_fptr->operator_img->img, lv_poc_status_bar_fptr->time_label, LV_ALIGN_OUT_RIGHT_MID, 2, -1);
//    return ret_val;
//}

#ifdef CONFIG_POC_GUI_GPS_SUPPORT
/*******************
*     NAME:    lv_poc_init_stabar_gps_img
* DESCRIPT:    初始化状态栏GPS图标
*     DATE:    2020-08-03
********************/
static bool lv_poc_init_stabar_gps_img(lv_obj_t ** align_obj)
{
    bool ret_val = true;
	nv_poc_setting_msg_t *gps_config = lv_poc_setting_conf_read();

	if(lv_poc_status_bar_fptr->gps_img == NULL)
   {
	   	lv_poc_status_bar_fptr->gps_img    = (lv_poc_status_bar_gps_obj_t *)lv_mem_alloc(sizeof(lv_poc_status_bar_gps_obj_t));
	    memset(lv_poc_status_bar_fptr->gps_img, 0, sizeof(lv_poc_status_bar_gps_obj_t));

		lv_poc_status_bar_fptr->gps_img->align_r_obj = lv_poc_status_bar_fptr->sim1->align_l_obj;
		lv_poc_status_bar_fptr->gps_img->align_l_obj = lv_poc_status_bar_fptr->gps_img->align_r_obj;

		lv_poc_status_bar_fptr->gps_img->gps_location_img = lv_img_create(lv_poc_status_bar, NULL);
		lv_img_set_auto_size(lv_poc_status_bar_fptr->gps_img->gps_location_img, false);
		lv_obj_set_size(lv_poc_status_bar_fptr->gps_img->gps_location_img, 18, 18);

		if(gps_config->GPS_switch == 1){
			lvPocLedCom_Msg(LVPOCGUIIDTCOM_SIGNAL_GPS_RESUME_IND, false);
		}
		else{
			lvPocLedCom_Msg(LVPOCGUIIDTCOM_SIGNAL_GPS_SUSPEND_IND, false);
		}
	}

	if(pubPocIdtGpsLocationStatus())
	{
		lv_poc_show_gps_location_status_img(true);
	}
	else
	{
		if(pubPocIdtGpsTimeoutNoLocationStatus())
		{
			lv_poc_show_gps_location_status_img(false);//time out nolocation
		}
		else
		{
			lv_poc_show_gps_location_status_img(true);
		}
	}

	if(align_obj == NULL)
    {
	   lv_obj_align(lv_poc_status_bar_fptr->gps_img->gps_location_img, *(lv_poc_status_bar_fptr->sim1->align_l_obj), LV_ALIGN_OUT_LEFT_MID, 0, 0);
    }
    else
    {
	   lv_obj_align(lv_poc_status_bar_fptr->gps_img->gps_location_img, *(align_obj), LV_ALIGN_OUT_LEFT_MID, 0, 0);
    }

	if(gps_config->GPS_switch == 1){
		lv_obj_set_hidden(lv_poc_status_bar_fptr->gps_img->gps_location_img, false);
	}else{
		lv_obj_set_hidden(lv_poc_status_bar_fptr->gps_img->gps_location_img, true);
	}
    return ret_val;
}

/*******************
*     NAME:    lv_poc_show_gps_location_status_img
* DESCRIPT:    显示定位及未定位图标
*     DATE:    2020-10-22
********************/
void lv_poc_show_gps_location_status_img(bool status)
{
	if(status == true)
	{
		lv_img_set_src(lv_poc_status_bar_fptr->gps_img->gps_location_img, &ic_gps_on);
	}
	else
	{
		lv_img_set_src(lv_poc_status_bar_fptr->gps_img->gps_location_img, &ic_qs_gps_on_sprd);
	}
}

/*******************
*     NAME:    lv_poc_stabar_show_gps_img
* DESCRIPT:    打开或关闭GPS图标
*     DATE:    2020-08-03
********************/
bool lv_poc_stabar_show_gps_img(bool enable)
{
	if(enable == true)
	{
		lv_obj_set_hidden(lv_poc_status_bar_fptr->gps_img->gps_location_img, false);
	}
	else
	{
		lv_obj_set_hidden(lv_poc_status_bar_fptr->gps_img->gps_location_img, true);
	}
	return true;
}
#endif

/*******************
*     NAME:    lv_poc_stabar_sim_clean
*   AUTHOR:    lugj
* DESCRIPT:    清空状态sim图标
*     DATE:    2020-1-20
********************/
static void lv_poc_stabar_sim_clean(lv_poc_status_bar_sim_obj_t * sim_obj)
{
    if(sim_obj->sim_img != NULL)
    {
        lv_obj_del(sim_obj->sim_img);
        sim_obj->sim_img = NULL;
    }

    if(sim_obj->sim_net_type_img != NULL)
    {
        lv_obj_del(sim_obj->sim_net_type_img);
        sim_obj->sim_net_type_img = NULL;
    }

    if(sim_obj->sim_signal_strength_img != NULL)
    {
        lv_obj_del(sim_obj->sim_signal_strength_img);
        sim_obj->sim_signal_strength_img = NULL;
    }

    if(sim_obj->sim_up_down_img != NULL)
    {
        lv_obj_del(sim_obj->sim_up_down_img);
        sim_obj->sim_up_down_img = NULL;
    }

    //memset(sim_obj, 0, sizeof(lv_poc_status_bar_sim_obj_t));
}

/*******************
*     NAME:    lv_poc_init_stabar_sim1_img
*   AUTHOR:    lugj
* DESCRIPT:    初始化状态sim1图标
*     DATE:    2019-10-25
********************/
static bool lv_poc_init_stabar_sim1_img(void)
{
    bool ret_val = true;
    lv_poc_status_bar_fptr->has_sim1 = lv_poc_get_sim1_state();
    lv_poc_status_bar_fptr->sim1    = (lv_poc_status_bar_sim_obj_t *)lv_mem_alloc(sizeof(lv_poc_status_bar_sim_obj_t));
	memset(lv_poc_status_bar_fptr->sim1, 0, sizeof(lv_poc_status_bar_sim_obj_t));
    lv_poc_status_bar_fptr->sim1->align_r_obj = &(lv_poc_status_bar_fptr->battery_img);
    lv_poc_status_bar_fptr->sim1->align_l_obj = lv_poc_status_bar_fptr->sim1->align_r_obj;

    return ret_val;
}

/*******************
*     NAME:    lv_poc_init_stabar_sim2_img
*   AUTHOR:    lugj
* DESCRIPT:    初始化状态sim2图标
*     DATE:    2019-10-25
********************/
static bool lv_poc_init_stabar_sim2_img(void)
{
    bool ret_val = true;
    lv_poc_status_bar_fptr->has_sim2 = lv_poc_get_sim2_state();
    lv_poc_status_bar_fptr->sim2    = (lv_poc_status_bar_sim_obj_t *)lv_mem_alloc(sizeof(lv_poc_status_bar_sim_obj_t));
	memset(lv_poc_status_bar_fptr->sim2, 0, sizeof(lv_poc_status_bar_sim_obj_t));
    lv_poc_status_bar_fptr->sim2->align_r_obj = lv_poc_status_bar_fptr->sim1->align_l_obj;
    lv_poc_status_bar_fptr->sim2->align_l_obj = lv_poc_status_bar_fptr->sim2->align_r_obj;

    return ret_val;
}

/*******************
*     NAME:    lv_poc_init_stabar_sim2_img
*   AUTHOR:    lugj
* DESCRIPT:    初始化状态signal图标
*     DATE:    2019-10-25
********************/
static bool lv_poc_init_stabar_signal_img(void)
{
    bool ret = true;
    return ret;
}

/*******************
*     NAME:    lv_poc_control_init
*   AUTHOR:    lugj
* DESCRIPT:    初始化控制区域
*     DATE:    2019-10-25
********************/
static void lv_poc_control_init(lv_poc_activity_t *activity,
                                       const char *left_text,
                                       const char *middle_text,
                                       const char *right_text)
{
    lv_poc_control_t *control = NULL;
    lv_style_t * ctrl_background_style;
    poc_setting_conf = lv_poc_setting_conf_read();
    ctrl_background_style = (lv_style_t *)(poc_setting_conf->theme.current_theme->style_control);
    ctrl_background_style->text.font = (lv_font_t *)poc_setting_conf->font.activity_control_font;

	if(control == NULL)
	{
    	control = (lv_poc_control_t *)lv_mem_alloc(sizeof(lv_poc_control_t));
	}

    control->background = lv_obj_create(activity->display, NULL);

    if(activity->has_stabar)
    {
        lv_obj_set_size(control->background, LV_POC_CONTROL_HOR_RES, LV_POC_CONTROL_VER_RES);

        lv_obj_set_pos(control->background, LV_POC_CONTROL_POSITION_X, LV_POC_CONTROL_POSITION_Y);
    }
    else
    {
        lv_obj_set_size(control->background, LV_POC_CONTROL_HOR_RES, LV_POC_CONTROL_VER_RES);

        lv_obj_set_pos(control->background, LV_POC_CONTROL_NOSTABAR_POSITION_X, LV_POC_CONTROL_NOSTABAR_POSITION_Y);
    }
    lv_obj_set_style(control->background, ctrl_background_style);
    //lv_refr_now();
    activity->control = control;

    control->left_button = lv_label_create(control->background,NULL);
    lv_label_set_text(control->left_button,left_text);
    lv_label_set_align(control->left_button, LV_LABEL_ALIGN_CENTER);
    lv_obj_align(control->left_button, lv_obj_get_parent(control->left_button), LV_ALIGN_IN_LEFT_MID, 1, -1);

    control->middle_button = lv_label_create(control->background,NULL);
    lv_label_set_text(control->middle_button, middle_text);
    lv_label_set_align(control->middle_button, LV_LABEL_ALIGN_CENTER);
    lv_obj_align(control->middle_button, lv_obj_get_parent(control->middle_button), LV_ALIGN_CENTER, 0, -1);

    control->right_button = lv_label_create(control->background,NULL);
    lv_label_set_text(control->right_button, right_text);
    lv_label_set_align(control->right_button, LV_LABEL_ALIGN_CENTER);
    lv_obj_align(control->right_button, lv_obj_get_parent(control->right_button), LV_ALIGN_IN_RIGHT_MID, -5, -1);

}


/*******************
*     NAME:    lv_poc_activity_list_init
*   AUTHOR:    lugj
* DESCRIPT:    初始化activity查询表
*     DATE:    2019-10-25
********************/
static void	lv_poc_activity_list_init(void)
{
	if(lv_poc_activity_list == NULL)
	{
    	lv_poc_activity_list = (lv_poc_activity_list_t *)lv_mem_alloc(sizeof(lv_poc_activity_list_t));
	}

	lv_poc_activity_list->size = 0;
    lv_poc_activity_list->head = (lv_poc_activity_list_node_t *)lv_mem_alloc(sizeof(lv_poc_activity_list_node_t));
	lv_poc_activity_list->head->activity = NULL;
    lv_poc_activity_list->head->next = NULL;
}


/*******************
*     NAME:    lv_poc_activity_list_insert
*   AUTHOR:    lugj
* DESCRIPT:    将new_activity插入activity查询表
*     DATE:    2019-10-30
********************/
static bool	lv_poc_activity_list_insert(lv_poc_activity_t * new_activity)
{
    lv_poc_activity_list_node_t * p = lv_poc_activity_list->head;
    lv_poc_activity_list_node_t * nonius = (lv_poc_activity_list_node_t *)lv_mem_alloc(sizeof(lv_poc_activity_list_node_t));
    if(NULL == nonius)
    {
        return false;
    }
    nonius->activity = new_activity;
    nonius->next = NULL;

    if(lv_poc_activity_list->size == 0)
    {
        p->next = nonius;
    }
    else
    {
        p = p->next;
        while(p->next != NULL)
        {
            if(p->activity == new_activity)
            {
                return true;
            }
            p = p->next;
        }
        p->next = nonius;
    }
    lv_poc_activity_list->size = lv_poc_activity_list->size + 1;
    return true;
}



/*******************
*     NAME:    lv_poc_activity_list_delete
*   AUTHOR:    lugj
* DESCRIPT:    将old_activity从activity查询表中删除
*     DATE:    2019-10-30
********************/
static void	lv_poc_activity_list_delete(lv_poc_activity_t * old_activity)
{
    lv_poc_activity_list_node_t * p = lv_poc_activity_list->head;
    lv_poc_activity_list_node_t * t;

    while(p->next && p->next->activity != old_activity)
    {
        p = p->next;
    }

    if(p->next != NULL)
    {
        t = p->next;
        p->next = t->next;
        lv_mem_free(t);
        lv_poc_activity_list->size = lv_poc_activity_list->size - 1;
    }
}

/*******************
*     NAME:    lv_poc_is_keypad_msg
*   AUTHOR:    lugj
* DESCRIPT:
*     DATE:    2019-1-20
********************/
static bool lv_poc_is_keypad_msg(lv_signal_t sign, void * param)
{
	if((LV_SIGNAL_CONTROL == sign
		|| LV_SIGNAL_RELEASED == sign
		|| LV_SIGNAL_LONG_PRESS_REP == sign)//自定义加入需要的消息类型
		&& NULL != param)
	{
		switch(*((uint32_t *)param))
		{
			case LV_GROUP_KEY_VOL_DOWN:
			case LV_GROUP_KEY_VOL_UP:
			case LV_GROUP_KEY_POC:
			case LV_GROUP_KEY_ENTER:
			case LV_GROUP_KEY_UP:
			case LV_GROUP_KEY_ESC:
			case LV_GROUP_KEY_GP:
			case LV_GROUP_KEY_DOWN:
			case LV_GROUP_KEY_MB:
			case 0xf0:
			{
				return true;
			}

			default:
			{
				break;
			}
		}
	}
    return false;
}

/*******************
*     NAME:    lv_poc_signal_cb
*   AUTHOR:    lugj
* DESCRIPT:    对讲机界面signal函数
*     DATE:    2019-10-31
********************/
static lv_res_t lv_poc_signal_cb(lv_obj_t * obj, lv_signal_t sign, void * param)
{
    lv_res_t ret = LV_RES_OK;

    bool is_keypad_msg = lv_poc_is_keypad_msg(sign, param);
    bool lcd_is_wakeup = poc_get_lcd_status();

	if(is_keypad_msg && !lcd_is_wakeup)
	{
		return LV_RES_INV;
	}

    poc_setting_conf = lv_poc_setting_conf_read();
	if(param == NULL) return LV_RES_OK;
	unsigned int cur_key = *(unsigned int *)param;

    if(false == is_keypad_msg)
    {
		//OSI_PRINTFI("[ancient][sigal](%s)(%d):other, key(%d), sign(%d)", __func__, __LINE__, cur_key, sign);
        ret = (*ancient_signal_func)(obj,sign,param);

        if(ret != LV_RES_OK)
        {
	        return ret;
        }
    }

    if(!(sign == LV_SIGNAL_STYLE_CHG
      || sign == LV_SIGNAL_PRESSED
      || sign == LV_SIGNAL_PRESSING
      || sign == LV_SIGNAL_PRESS_LOST
      || sign == LV_SIGNAL_RELEASED
      || sign == LV_SIGNAL_LONG_PRESS
      || sign == LV_SIGNAL_LONG_PRESS_REP
      || sign == LV_SIGNAL_DRAG_BEGIN
      || sign == LV_SIGNAL_DRAG_END
      || sign == LV_SIGNAL_FOCUS
      || sign == LV_SIGNAL_DEFOCUS
      || sign == LV_SIGNAL_CONTROL))
	{
		return ret;
	}

	if(lv_poc_cit_get_run_status() == LV_POC_CIT_OPRATOR_TYPE_KEY)
	{
		return LV_RES_INV;
	}

    lv_poc_activity_t * activity = lv_poc_activity_list_lookup(obj);
    if(NULL == activity)
    {
	    return ret;
    }

	if(activity != current_activity)
    {

    }
	OSI_PRINTFI("[ancient][sigal](%s)(%d):key(%d), sign(%d), is_keypad_msg(%d)", __func__, __LINE__, cur_key, sign, is_keypad_msg);

	uint8_t vol_cur = 0;

#if IDT_POC_MODE

	if(lv_poc_cit_get_run_status() == LV_POC_CIT_OPRATOR_TYPE_VOLUM
		|| lv_poc_cit_get_run_status() == LV_POC_CIT_OPRATOR_TYPE_MIC
		|| lv_poc_cit_get_run_status() == LV_POC_CIT_OPRATOR_TYPE_HEADSET)
	{
		vol_cur = lv_poc_setting_get_current_volume(POC_MMI_VOICE_PLAY);
		if(is_keypad_msg)
		{
			cur_key = *((uint32_t *)param);

			if(cur_key == LV_GROUP_KEY_VOL_DOWN)
			{
				if(vol_cur > 0)
				{
					vol_cur = vol_cur - 1;
					lv_poc_set_volum(POC_MMI_VOICE_PLAY , vol_cur, poc_setting_conf->btn_voice_switch, true);
				}
			}
			else if(cur_key == LV_GROUP_KEY_VOL_UP)
			{
				if(vol_cur < 11)
				{
					vol_cur = vol_cur + 1;
					lv_poc_set_volum(POC_MMI_VOICE_PLAY , vol_cur, poc_setting_conf->btn_voice_switch, true);
				}
			}
			else if(cur_key != LV_GROUP_KEY_POC
					&& (sign ==	LV_SIGNAL_RELEASED))//按键音
			{
				if(lv_poc_get_screenon_status())
				{
					OSI_LOGI(0, "[poc][signal](%d)screen on first, cannel key tone", __LINE__);
				}
				else
				{
					poc_play_btn_voice_one_time(0,
#ifdef CONFIG_POC_TTS_SUPPORT
					   poc_setting_conf->voice_broadcast_switch ||
#endif
					   !(poc_setting_conf->btn_voice_switch));
				}
			}
		}
	}
	else
	{
		vol_cur = lv_poc_setting_get_current_volume(POC_MMI_VOICE_VOICE);
		if(is_keypad_msg)
		{
			cur_key = *((uint32_t *)param);

			if(cur_key == LV_GROUP_KEY_VOL_DOWN)
			{
				if(vol_cur > 0)
				{
					vol_cur = vol_cur - 1;
					lv_poc_set_volum(POC_MMI_VOICE_VOICE , vol_cur, poc_setting_conf->btn_voice_switch, true);
				}
			}
			else if(cur_key == LV_GROUP_KEY_VOL_UP)
			{
				if(vol_cur < 11)
				{
					vol_cur = vol_cur + 1;
					lv_poc_set_volum(POC_MMI_VOICE_VOICE , vol_cur, poc_setting_conf->btn_voice_switch, true);
				}
			}
			else if(cur_key != LV_GROUP_KEY_POC
					&& (sign ==	LV_SIGNAL_RELEASED))//按键音
			{
				if(lv_poc_get_screenon_status())
				{
					OSI_LOGI(0, "[poc][signal](%d)screen on first, cannel key tone", __LINE__);
				}
				else
				{
					poc_play_btn_voice_one_time(0,
#ifdef CONFIG_POC_TTS_SUPPORT
					   poc_setting_conf->voice_broadcast_switch ||
#endif
					   !(poc_setting_conf->btn_voice_switch));
				}
			}
		}
	}

#else
	vol_cur = lv_poc_setting_get_current_volume(POC_MMI_VOICE_PLAY);
	if(is_keypad_msg)
	{
		cur_key = *((uint32_t *)param);

		if(cur_key == LV_GROUP_KEY_VOL_DOWN)
		{
			if(vol_cur > 0)
			{
				vol_cur = vol_cur - 1;
				lv_poc_set_volum(POC_MMI_VOICE_PLAY , vol_cur, poc_setting_conf->btn_voice_switch, true);
			}
		}
		else if(cur_key == LV_GROUP_KEY_VOL_UP)
		{
			if(vol_cur < 11)
			{
				vol_cur = vol_cur + 1;
				lv_poc_set_volum(POC_MMI_VOICE_PLAY , vol_cur, poc_setting_conf->btn_voice_switch, true);
			}
		}
		else if(cur_key != LV_GROUP_KEY_POC
				&& (sign ==	LV_SIGNAL_RELEASED))//按键音
		{
			if(lv_poc_get_screenon_status())
			{
				OSI_LOGI(0, "[poc][signal](%d)screen on first, cannel key tone", __LINE__);
			}
			else
			{
				poc_play_btn_voice_one_time(0,
#ifdef CONFIG_POC_TTS_SUPPORT
				   poc_setting_conf->voice_broadcast_switch ||
#endif
				   !(poc_setting_conf->btn_voice_switch));
			}
		}
	}
#endif
	//longpress key
	static bool longpress_keyup_memberlist = false;

	if(sign == LV_SIGNAL_LONG_PRESS_REP)
	{
		switch(cur_key)
		{
			case LV_GROUP_KEY_UP:
			{
				if(activity_idle == current_activity)
				{
					OSI_LOGI(0, "[poc][longpress](%d):member list open", __LINE__);
					longpress_keyup_memberlist = true;
					lv_poc_member_list_open(NULL, NULL, false);
					return LV_RES_OK;
				}
				break;
			}

			case LV_GROUP_KEY_DOWN:
			{
				if(poc_group_list_activity == current_activity)
				{
					OSI_LOGI(0, "[poc][longpress](%d):delete group", __LINE__);
					sign = LV_SIGNAL_LONG_PRESS_REP;
					*(unsigned int *)param = LV_GROUP_KEY_GP;
					cur_key = *(unsigned int *)param;
				}
				break;
			}
		}
	}

	if(sign ==	LV_SIGNAL_RELEASED)
	{
		switch(cur_key)
		{
			case 0xf0:
			{
				if(lv_poc_get_screenon_status())
				{
					lv_poc_set_screenon_status(false);
					OSI_LOGI(0, "[poc][signal](%d)screen on first, release return", __LINE__);
					return LV_RES_OK;
				}
				OSI_LOGI(0, "[poc][signal](%d)pwer key esc or enter", __LINE__);
				if(activity_idle == current_activity)//main menu
				{
					lv_poc_main_menu_open();
					return LV_RES_OK;
				}
				else//esc key
				{
					sign = LV_SIGNAL_CONTROL;
					*(unsigned int *)param = LV_GROUP_KEY_ESC;
				}
				long_press_msg = POC_LONGPRESS_MSG_TYPE_START;
			}

			case LV_GROUP_KEY_UP:
			{
				if(longpress_keyup_memberlist)
				{
					OSI_LOGI(0, "[poc][longpress](%d):reset key up", __LINE__);
					longpress_keyup_memberlist = false;
					return LV_RES_INV;
				}

				if(activity_idle == current_activity)
				{
					lv_poc_group_list_open(NULL);
					return LV_RES_OK;
				}
				break;
			}

			case LV_GROUP_KEY_DOWN:
			{
				if(activity_idle == current_activity)
				{
					return LV_RES_OK;
				}
				break;
			}
		}

	}

	switch(cur_key)
	{
		case LV_GROUP_KEY_POC:
		{
			if(activity_idle != current_activity)//enter key
			{
				sign = LV_SIGNAL_CONTROL;
				*(unsigned int *)param = LV_GROUP_KEY_ENTER;
			}
			break;
		}
	}

	OSI_PRINTFI("[ancient][sigal](%s)(%d):key(%d), sign(%d)", __func__, __LINE__, cur_key, sign);

    if(NULL != activity->signal_func)
    {
        ret = activity->signal_func(obj, sign, param);
    }
    return ret;
}

/*******************
*     NAME:    lv_poc_design_cb
*   AUTHOR:    lugj
* DESCRIPT:    对讲机界面design函数
*     DATE:    2019-10-25
********************/
static bool lv_poc_design_cb(lv_obj_t * obj, const lv_area_t * mask_p, lv_design_mode_t mode)
{
    bool ret = ancient_design_func(obj,mask_p, mode);
    lv_poc_activity_t * activity = lv_poc_activity_list_lookup(obj);
    if(activity == NULL)
    {
        return ret;
    }
    if(NULL != activity->design_func)
    {
        activity->design_func(obj,mask_p, mode);
    }

    return ret;
}

/*******************
*     NAME:    lv_poc_event_cb
*   AUTHOR:    lugj
* DESCRIPT:    对讲机界面event函数
*     DATE:    2020-03-30
********************/
static void lv_poc_event_cb(lv_obj_t * obj, lv_event_t event)
{
	if(ancient_event_func == NULL)
	{
		return;
	}
    ancient_event_func(obj, event);
    lv_poc_activity_t * activity = lv_poc_activity_list_lookup(obj);
    if(activity == NULL)
    {
        return;
    }
    if(NULL != activity->event_func)
    {
        activity->event_func(obj, event);
    }
}



/*******************
*     NAME:    lv_poc_stack_init
*   AUTHOR:    lugj
* DESCRIPT:    初始化界面管理栈
*     DATE:    2019-10-25
********************/
static void lv_poc_stack_init(void)
{
    lv_poc_statck.size = 0;
    memset(lv_poc_statck.activity, 0, sizeof(void *) * LV_POC_STACK_SIZE);
}

/*******************
*     NAME:    lv_poc_stack_push
*   AUTHOR:    lugj
* DESCRIPT:    压入界面管理栈
*     DATE:    2019-10-25
********************/
static bool lv_poc_stack_push(lv_poc_activity_t *activity)
{
    if(activity == NULL)
    {
        OSI_LOGI(0, "Please set corrent parameter -- activity\n");
        return false;
    }
    if(lv_poc_statck.size < 0 || lv_poc_statck.size >= LV_POC_STACK_SIZE)
    {
        OSI_LOGI(0, "capacity of lv_poc_statck is full and can not add any activity to lv_poc_statck\n");
        return false;
    }
    for(int j = 0; j < lv_poc_statck.size; j++)
    {
        if(lv_poc_statck.activity[j] == (void *)activity)
        {
            OSI_LOGI(0, "The activity is existed in the lv_poc_statck!\n");
            return true;
        }
    }
    lv_poc_statck.activity[lv_poc_statck.size++] = (void *)activity;
    return true;
}

/*******************
*     NAME:    lv_poc_stack_pop
*   AUTHOR:    lugj
* DESCRIPT:    弹出界面管理栈并删除
*     DATE:    2019-10-25
********************/
static lv_poc_activity_t * lv_poc_stack_pop(void)
{
    if(lv_poc_statck.size <= 0)
    {
        OSI_LOGI(0, "lv_poc_statck is null! Please add idle activity\n");
        return NULL;
    }
    if(lv_poc_statck.size == 1)
    {
        OSI_LOGI(0, "lv_poc_statck have one item only, only return idle activity and dont decrease size\n");
        return lv_poc_statck.activity[lv_poc_statck.size - 1];
    }
    lv_poc_statck.size = lv_poc_statck.size - 1;
    return (lv_poc_activity_t *)lv_poc_statck.activity[lv_poc_statck.size];
}

/*******************
*     NAME:    lv_poc_stack_top
*   AUTHOR:    lugj
* DESCRIPT:    查看界面管理栈顶元素
*     DATE:    2019-10-25
********************/
static lv_poc_activity_t * lv_poc_stack_top(void)
{
    if(lv_poc_statck.size <= 0)
    {
        OSI_LOGI(0, "lv_poc_statck is null! Please add idle activity\n");
        return NULL;
    }
    return (lv_poc_activity_t *)lv_poc_statck.activity[lv_poc_statck.size - 1];
}

static void lv_poc_stabar_battery_task(void)
{
    static lv_img_dsc_t * pre_battery_img = NULL;
    lv_img_dsc_t * cur_battery_img = lv_poc_get_battery_img();
    if(cur_battery_img != pre_battery_img && cur_battery_img != NULL)
    {
        pre_battery_img = cur_battery_img;
        lv_img_set_src(lv_poc_status_bar_fptr->battery_img, cur_battery_img);
    }
}

static void lv_poc_stabar_signal_task(void)
{
    lv_poc_update_stabar_sim_img();
}

static void lv_exec_task(lv_task_t * task)
{
	if(task->user_data == NULL)
	{
		return;
	}
    lv_exec_parameter_t * obj = (lv_exec_parameter_t *)task->user_data;
    OSI_LOGI(0, "func:%s 0x%p\n", __func__, obj);
    if(obj->activity->signal_func != NULL && obj->activity->display != NULL)
    {
        OSI_LOGI(0, "func:%s 0x%p - 0x%p - %d\n", __func__, obj->activity->signal_func, obj->activity->display, obj->sign);
        obj->activity->signal_func(obj->activity->display, obj->sign, obj->parameters);

    }
    lv_mem_free(obj);
}

static lv_poc_status_t prv_lv_poc_member_list_add(lv_poc_oem_member_list *member_list_obj, const char * name, bool is_online, void * information)
{
     lv_poc_activity_attribute_cb_set_obj * cb_set_obj = lv_poc_get_activity_attribute_cb_set_obj();
     lv_poc_status_t status = POC_UNKNOWN_FAULT;

     for(int i = 0; i < LV_POC_ACTIVITY_ATTRIBUTE_CB_SET_SIZE; i++)
     {
         if(cb_set_obj->member_list[i].active == true && cb_set_obj->member_list[i].activity_id > 0)
         {
            if(cb_set_obj->member_list[i].add != NULL)
            {
                status = cb_set_obj->member_list[i].add(member_list_obj, name, is_online, information);
            }
         }
     }

     return status;
}

static void prv_lv_poc_member_list_clear(lv_poc_oem_member_list *member_list_obj)
{
     lv_poc_activity_attribute_cb_set_obj * cb_set_obj = lv_poc_get_activity_attribute_cb_set_obj();

     for(int i = 0; i < LV_POC_ACTIVITY_ATTRIBUTE_CB_SET_SIZE; i++)
     {
         if(cb_set_obj->member_list[i].active == true && cb_set_obj->member_list[i].activity_id > 0)
         {
            if(cb_set_obj->member_list[i].clear != NULL)
            {
                cb_set_obj->member_list[i].clear(member_list_obj);
            }
         }
     }
}

static int prv_lv_poc_member_list_get_information(lv_poc_oem_member_list *member_list_obj, const char * name, void *** information)
{
     lv_poc_activity_attribute_cb_set_obj * cb_set_obj = lv_poc_get_activity_attribute_cb_set_obj();
     int ret = 0;

     for(int i = 0; i < LV_POC_ACTIVITY_ATTRIBUTE_CB_SET_SIZE; i++)
     {
         if(cb_set_obj->member_list[i].active == true && cb_set_obj->member_list[i].activity_id > 0)
         {
            if(cb_set_obj->member_list[i].get_info != NULL)
            {
                ret = cb_set_obj->member_list[i].get_info(member_list_obj, name, information);
            }
         }
     }

     return ret;
}

static void prv_lv_poc_member_list_refresh(lv_poc_oem_member_list *member_list_obj)
{
     lv_poc_activity_attribute_cb_set_obj * cb_set_obj = lv_poc_get_activity_attribute_cb_set_obj();

     for(int i = 0; i < LV_POC_ACTIVITY_ATTRIBUTE_CB_SET_SIZE; i++)
     {
         if(cb_set_obj->member_list[i].active == true && cb_set_obj->member_list[i].activity_id > 0)
         {
            if(cb_set_obj->member_list[i].refresh != NULL)
            {
                cb_set_obj->member_list[i].refresh(member_list_obj);
            }
         }
     }
}

static void prv_lv_poc_member_list_refresh_with_data(lv_poc_oem_member_list *member_list_obj)
{
     lv_poc_activity_attribute_cb_set_obj * cb_set_obj = lv_poc_get_activity_attribute_cb_set_obj();

     for(int i = 0; i < LV_POC_ACTIVITY_ATTRIBUTE_CB_SET_SIZE; i++)
     {
         if(cb_set_obj->member_list[i].active == true && cb_set_obj->member_list[i].activity_id > 0)
         {
            if(cb_set_obj->member_list[i].refresh_with_data != NULL)
            {
                cb_set_obj->member_list[i].refresh_with_data(member_list_obj);
            }
         }
     }
}

static lv_poc_status_t prv_lv_poc_member_list_set_state(lv_poc_oem_member_list *member_list_obj, const char * name, void * information, bool is_online)
{
     lv_poc_activity_attribute_cb_set_obj * cb_set_obj = lv_poc_get_activity_attribute_cb_set_obj();
     lv_poc_status_t status = POC_UNKNOWN_FAULT;

     for(int i = 0; i < LV_POC_ACTIVITY_ATTRIBUTE_CB_SET_SIZE; i++)
     {
         if(cb_set_obj->member_list[i].active == true && cb_set_obj->member_list[i].activity_id > 0)
         {
            if(cb_set_obj->member_list[i].set_state != NULL)
            {
                status = cb_set_obj->member_list[i].set_state(member_list_obj, name, information, is_online);
            }
         }
     }

     return status;
}

static lv_poc_status_t prv_lv_poc_member_list_is_exists(lv_poc_oem_member_list *member_list_obj, const char * name, void * information)
{
     lv_poc_activity_attribute_cb_set_obj * cb_set_obj = lv_poc_get_activity_attribute_cb_set_obj();
     lv_poc_status_t status = POC_UNKNOWN_FAULT;

     for(int i = 0; i < LV_POC_ACTIVITY_ATTRIBUTE_CB_SET_SIZE; i++)
     {
         if(cb_set_obj->member_list[i].active == true && cb_set_obj->member_list[i].activity_id > 0)
         {
            if(cb_set_obj->member_list[i].exists != NULL)
            {
                status = cb_set_obj->member_list[i].exists(member_list_obj, name, information);
            }
         }
     }

     return status;
}

static lv_poc_status_t prv_lv_poc_member_list_get_state(lv_poc_oem_member_list *member_list_obj, const char * name, void * information)
{
     lv_poc_activity_attribute_cb_set_obj * cb_set_obj = lv_poc_get_activity_attribute_cb_set_obj();
     lv_poc_status_t status = POC_UNKNOWN_FAULT;

     for(int i = 0; i < LV_POC_ACTIVITY_ATTRIBUTE_CB_SET_SIZE; i++)
     {
         if(cb_set_obj->member_list[i].active == true && cb_set_obj->member_list[i].activity_id > 0)
         {
            if(cb_set_obj->member_list[i].get_state != NULL)
            {
                status = cb_set_obj->member_list[i].get_state(member_list_obj, name, information);
            }
         }
     }

     return status;
}

static void prv_lv_poc_group_member_list_activity(char *grpname)
{
   lv_poc_activity_attribute_cb_set_obj * cb_set_obj = lv_poc_get_activity_attribute_cb_set_obj();

   for(int i = 0; i < LV_POC_ACTIVITY_ATTRIBUTE_CB_SET_SIZE; i++)
   {
      if(cb_set_obj->member_list[i].group_member_act != NULL)
      {
         lv_task_t *onece_task = lv_task_create(cb_set_obj->member_list[i].group_member_act, 30, LV_TASK_PRIO_MID, (void *)grpname);
         lv_task_once(onece_task);
      }
   }
}

static int prv_lv_poc_group_list_get_information(lv_poc_oem_group_list *group_list_obj, const char * name, void *** information)
{
     lv_poc_activity_attribute_cb_set_obj * cb_set_obj = lv_poc_get_activity_attribute_cb_set_obj();
     int ret = 0;

     for(int i = 0; i < LV_POC_ACTIVITY_ATTRIBUTE_CB_SET_SIZE; i++)
     {
         if(cb_set_obj->group_list[i].active == true && cb_set_obj->group_list[i].activity_id > 0)
         {
            if(cb_set_obj->group_list[i].get_info != NULL)
            {
                ret = cb_set_obj->group_list[i].get_info(group_list_obj, name, information);
            }
         }
     }

     return ret;
}

static void prv_lv_poc_group_list_refresh(lv_poc_oem_group_list *group_list_obj)
{
     lv_poc_activity_attribute_cb_set_obj * cb_set_obj = lv_poc_get_activity_attribute_cb_set_obj();

     for(int i = 0; i < LV_POC_ACTIVITY_ATTRIBUTE_CB_SET_SIZE; i++)
     {
         if(cb_set_obj->group_list[i].active == true && cb_set_obj->group_list[i].activity_id > 0)
         {
            if(cb_set_obj->group_list[i].refresh != NULL)
            {
                cb_set_obj->group_list[i].refresh(group_list_obj);
            }
         }
     }
}

static void prv_lv_poc_group_list_refresh_with_data(lv_poc_oem_group_list *group_list_obj)
{
     lv_poc_activity_attribute_cb_set_obj * cb_set_obj = lv_poc_get_activity_attribute_cb_set_obj();

     for(int i = 0; i < LV_POC_ACTIVITY_ATTRIBUTE_CB_SET_SIZE; i++)
     {
         if(cb_set_obj->group_list[i].active == true && cb_set_obj->group_list[i].activity_id > 0)
         {
            if(cb_set_obj->group_list[i].refresh_with_data != NULL)
            {
                cb_set_obj->group_list[i].refresh_with_data(group_list_obj);
            }
         }
     }
}

static lv_poc_status_t prv_lv_poc_group_list_is_exists(lv_poc_oem_group_list *group_list_obj, const char * name, void * information)
{
     lv_poc_activity_attribute_cb_set_obj * cb_set_obj = lv_poc_get_activity_attribute_cb_set_obj();
     lv_poc_status_t status = POC_UNKNOWN_FAULT;

     for(int i = 0; i < LV_POC_ACTIVITY_ATTRIBUTE_CB_SET_SIZE; i++)
     {
         if(cb_set_obj->group_list[i].active == true && cb_set_obj->group_list[i].activity_id > 0)
         {
            if(cb_set_obj->group_list[i].exists != NULL)
            {
                status = cb_set_obj->group_list[i].exists(group_list_obj, name, information);
            }
         }
     }

     return status;
}

static lv_poc_status_t prv_lv_poc_group_list_lock_group(lv_poc_oem_group_list *group_list_obj, lv_poc_group_oprator_type opt)
{
     lv_poc_activity_attribute_cb_set_obj * cb_set_obj = lv_poc_get_activity_attribute_cb_set_obj();
     lv_poc_status_t status = POC_UNKNOWN_FAULT;

     for(int i = 0; i < LV_POC_ACTIVITY_ATTRIBUTE_CB_SET_SIZE; i++)
     {
         if(cb_set_obj->group_list[i].active == true && cb_set_obj->group_list[i].activity_id > 0)
         {
            if(cb_set_obj->group_list[i].lock_group != NULL)
            {
                status = cb_set_obj->group_list[i].lock_group(group_list_obj, opt);
            }
         }
     }

     return status;
}

static void prv_lv_poc_idle_set_page2_note_func(lv_poc_idle_page2_display_t msg_type, int num, ...)
{
	int str_count = num;
	char * content[5];
	int index = 0;
	memset(content, 0 ,sizeof(char *) * 5);
	va_list param_list;
	va_start(param_list, num);
	for(index = 0; index < str_count; index++)
	{
		content[index] = va_arg(param_list, char *);
	}
	va_end(param_list);
	lv_poc_idle_set_page2(msg_type, content, index);
}

static bool prv_lvPocLedIdtCom_Msg_func(LVPOCIDTCOM_Led_SignalType_t signal, bool steals)
{
	return lvPocLedCom_Msg(signal, steals);
}

static void prv_lv_poc_member_call_open_cb(lv_task_t *task)
{
	void *infomation = (void *)task->user_data;
	lv_poc_member_call_open(infomation);
}

static void prv_lv_poc_member_call_open(void * information)
{
	lv_task_t *once_task = lv_task_create(prv_lv_poc_member_call_open_cb, 30, LV_TASK_PRIO_MID, information);
	lv_task_once(once_task);
}

static void prv_lv_poc_member_call_close_cb(lv_task_t *task)
{
	lv_poc_member_call_close();
}

static void prv_lv_poc_member_call_close(void)
{
	lv_task_t *once_task = lv_task_create(prv_lv_poc_member_call_close_cb, 30, LV_TASK_PRIO_MID, NULL);
	lv_task_once(once_task);
}

static void prv_lv_poc_build_tempgrp_get_member_list(void)
{
	lv_poc_build_tempgrp_get_tmpgrp_member_list();
}

static void prv_lv_poc_build_tmpgrp_open(char *grpname)
{
	lv_task_t *once_task = lv_task_create(lv_poc_build_tempgrp_member_list_activity_open, 30, LV_TASK_PRIO_MID, (void *)grpname);
	lv_task_once(once_task);
}

static void prv_lv_poc_build_tmpgrp_close(int type)
{
	lv_task_t *once_task = lv_task_create(lv_poc_build_tempgrp_memberlist_activity_close, 30, LV_TASK_PRIO_MID, (void *)type);
	lv_task_once(once_task);
}

static bool prv_lv_poc_notation_msg(lv_poc_notation_msg_type_t msg_type, const uint8_t *text_1, const uint8_t *text_2)
{
	 lv_poc_activity_attribute_cb_set_obj * cb_set_obj = lv_poc_get_activity_attribute_cb_set_obj();

	 if(cb_set_obj->note != NULL)
	 {
		 return cb_set_obj->note(msg_type, text_1, text_2);
	 }
	 return false;
}

/*************************************************
*
*                  PUBLIC
*
*************************************************/

/*******************
*	  NAME: 	lv_poc_stabar_task
*	AUTHOR:    lugj
* DESCRIPT:    状态栏任务
*	  DATE:    2019-10-25
********************/
void lv_poc_stabar_task(lv_task_t * task)
{
    static lv_poc_time_t time = {0};
    int k;
    lv_poc_get_time(&time);

    switch(lv_poc_status_bar_fptr->time_formate)
    {
	    case lv_poc_time_format_hhmm:
	    {
	        sprintf(lv_poc_time,"%02d:%02d",time.tm_hour,time.tm_min);
	        break;
	    }

	    case lv_poc_time_format_mmhh:
	    {
	        sprintf(lv_poc_time,"%02d:%02d",time.tm_min,time.tm_hour);
	        break;
	    }

	    case lv_poc_time_format_hhmmss:
	    {
	        sprintf(lv_poc_time,"%02d:%02d:%02d",time.tm_hour,time.tm_min,time.tm_sec);
	        break;
	    }

	    case lv_poc_time_format_sshhmm:
	    {
	        sprintf(lv_poc_time,"%02d:%02d:%02d",time.tm_sec,time.tm_hour,time.tm_min);
	        break;
	    }

	    case lv_poc_time_format_ssmmhh:
	    {
	        sprintf(lv_poc_time,"%02d:%02d:%02d",time.tm_sec,time.tm_min,time.tm_hour);
	        break;
	    }

	    case lv_poc_time_format_mmhhss:
	    {
	        sprintf(lv_poc_time,"%02d:%02d:%02d",time.tm_min,time.tm_hour,time.tm_sec);
	        break;
	    }

	    default:
	    {
	        sprintf(lv_poc_time,"%02d:%02d",time.tm_hour,time.tm_min);
	    }
    }
    lv_label_set_text(lv_poc_status_bar_fptr->time_label,lv_poc_time);

    for(k = 0; k < LV_POC_STABAR_TASK_EXT_LENGTH; k++)
    {
        if(status_bar_task_ext[k] != NULL)
        {
            (status_bar_task_ext[k])();
        }
    }
}


/*******************
*     NAME:    lv_poc_get_battery_state
*   AUTHOR:    lugj
* DESCRIPT:    获取电池状态，
*     DATE:    2019-10-25
********************/
UINT8 lv_poc_get_battery_state(void)
{
    static battery_values_t * battery_value = NULL;

	if(battery_value == NULL)
	{
		battery_value = lv_mem_alloc(sizeof(battery_values_t));
	}

	if(battery_value == NULL)
	{
		return 0;
	}

	memset(battery_value, 0, sizeof(battery_values_t));
    poc_battery_get_status(battery_value);
    lv_poc_status_bar_fptr->has_battery = battery_value->battery_status == 1? true:false;

    return battery_value->battery_status;
}

/*******************
*     NAME:    lv_poc_get_sim1_state
*   AUTHOR:    lugj
* DESCRIPT:    获取SIM1是否存在
*     DATE:    2019-10-25
********************/
bool lv_poc_get_sim1_state(void)
{
    return poc_check_sim_prsent(POC_SIM_1);
}

/*******************
*     NAME:    lv_poc_get_sim2_state
*   AUTHOR:    lugj
* DESCRIPT:    获取SIM2是否存在
*     DATE:    2019-10-25
********************/
bool lv_poc_get_sim2_state(void)
{
    return poc_check_sim_prsent(POC_SIM_2);
}

/*******************
*     NAME:    lv_poc_get_battery_cnt
* DESCRIPT:    获取当前电量基值
*     DATE:    2020-07-20
********************/
uint8_t lv_poc_get_battery_cnt(OUT battery_values_t *values)
{
	uint8_t battery_img_cnt = 0;

    if(values->battery_value >= 100)
    {
		battery_img_cnt = 6;
    }
    else if(values->battery_value >= 85)
    {
		battery_img_cnt=5;
    }
    else if(values->battery_value >= 65)
    {
		battery_img_cnt=4;
    }
    else if(values->battery_value >= 42)
    {
		battery_img_cnt=3;
    }
    else if(values->battery_value >= 20)
    {
		battery_img_cnt=2;
    }
    else if(values->battery_value >= 8)
    {
		battery_img_cnt=1;
    }
    else if(values->battery_value >= 0)
    {
		battery_img_cnt=0;
    }
	return battery_img_cnt;
}

/*******************
*     NAME:    lv_poc_get_battery_img
*   AUTHOR:    lugj
* DESCRIPT:    <BB><F1>?<B5><B1>???<B5><E7><B3><D8>?<B1><EA>
*     DATE:    2020-01-19
********************/
lv_img_dsc_t * lv_poc_get_battery_img(void)
{
	battery_values_t battery_t;
	poc_battery_get_status(&battery_t);
    const lv_img_dsc_t * battery_img = NULL;
	static uint8_t battery_img_cur = 100;
	static uint8_t low_battery_check_count = 0;

	static bool charge_status = false;
	static bool charge_complete_status = false;
	static uint8_t battery_pri_cnt = 0;

	if(battery_pri_cnt >= 5)//bat pri
	{
#if 0
		OSI_PRINTFI("[charge][mv](%s)(%d):volt(%d mv), cur(%d ma), val(%d), temp(%d)", __func__, __LINE__, battery_t.battery_val_mV, \
			battery_t.charge_cur_mA, battery_t.battery_value, battery_t.battery_temp);
#endif
		battery_pri_cnt = 0;
	}
	else
	{
		battery_pri_cnt++;
	}

	if(battery_img_cur == 100)//获取一次当前电量图标基础值
	{
		battery_img_cur = lv_poc_get_battery_cnt(&battery_t);
		if(POC_CHG_DISCONNECTED == battery_t.charging)
		{
			charge_status = true;//上次为充电状态
		}
		else
		{
			charge_status = false;//上次为不充电状态
		}
	}

    if(!battery_t.battery_status)
    {
        battery_img = &stat_sys_no_battery;
        return (lv_img_dsc_t *)battery_img;
    }

    if(POC_CHG_DISCONNECTED == battery_t.charging)
    {
		if(charge_status == true)
		{
			charge_status = false;
			charge_complete_status = false;
			lv_poc_set_charge_status(charge_status);
			lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_DISCHARGING_STATUS, true);
			lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_IDLE_STATUS, true);
		}

        if(battery_t.battery_value >= 100)
        {
            battery_img = &stat_sys_battery_100;
        }
        else if(battery_t.battery_value >= 85)
        {
            battery_img = &stat_sys_battery_85;
        }
        else if(battery_t.battery_value >= 70)
        {
            battery_img = &stat_sys_battery_71;
        }
        else if(battery_t.battery_value >= 42)
        {
            battery_img = &stat_sys_battery_57;
        }
        else if(battery_t.battery_value >= 20)
        {
            battery_img = &stat_sys_battery_43;
        }
        else if(battery_t.battery_value >= 4)
        {
            battery_img = &stat_sys_battery_15;
        }
        else if(battery_t.battery_value >= 0)
        {
            battery_img = &stat_sys_battery_0;
            if(low_battery_check_count < 1 && battery_t.battery_value <= 1)
            {
				lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_LOW_BATTERY_STATUS, true);
	            poc_play_voice_one_time(LVPOCAUDIO_Type_Low_Battery, 50, false);
            }
            low_battery_check_count = (low_battery_check_count + 1) % (60*5);//5min提示一次
            return (lv_img_dsc_t *)battery_img;
        }
        low_battery_check_count = 0;
    }
    else
    {
		if(charge_status == false)
		{
			charge_status = true;
			lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_CHARGING_STATUS, false);
		}

        if(battery_t.battery_value >= 100)
        {
			if(charge_complete_status == false)
			{
				charge_complete_status = true;
				lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_CHARGING_COMPLETE_STATUS, false);
			}
			battery_img_cur = 6;
            battery_img = battery_img_dispaly[battery_img_cur];
        }
        else if(battery_t.battery_value >= 85)
        {
			battery_img = battery_img_dispaly[battery_img_cur];
			battery_img_cur++;
			if(battery_img_cur>6)
			battery_img_cur=5;
        }
        else if(battery_t.battery_value >= 70)
        {
			battery_img = battery_img_dispaly[battery_img_cur];

			battery_img_cur++;
			if(battery_img_cur>6)
			battery_img_cur=4;
        }
        else if(battery_t.battery_value >= 42)
        {
            battery_img = battery_img_dispaly[battery_img_cur];
			battery_img_cur++;
			if(battery_img_cur>6)
			battery_img_cur=3;
        }
        else if(battery_t.battery_value >= 20)
        {
            battery_img = battery_img_dispaly[battery_img_cur];
			battery_img_cur++;
			if(battery_img_cur>6)
			battery_img_cur=2;
        }
        else if(battery_t.battery_value >= 4)
        {
            battery_img = battery_img_dispaly[battery_img_cur];
			battery_img_cur++;
			if(battery_img_cur>6)
			battery_img_cur=1;
        }
        else if(battery_t.battery_value >= 0)
        {
            battery_img = battery_img_dispaly[battery_img_cur];
			battery_img_cur++;
			if(battery_img_cur>6)
			battery_img_cur=0;
        }
    }
    return (lv_img_dsc_t *)battery_img;
}

static unsigned short lv_poc_get_sim_state_code(POC_SIM_ID sim_id)
{
    unsigned short sim_state_code = 0;
    POC_MMI_MODEM_PLMN_RAT rat;
    char operator[34];

    if(poc_check_sim_prsent(sim_id))
    {
        sim_state_code = sim_state_code | 0x01;
        sim_state_code = sim_state_code | (((unsigned short)poc_get_signal_bar_strenth(sim_id)) << 1);

        poc_get_operator_network_type_req(POC_SIM_1, (int8_t *)operator, &rat);
        if(MMI_MODEM_PLMN_RAT_LTE == rat)
        {
            sim_state_code = sim_state_code | ( 0x04 << 5);
        }
        else if(MMI_MODEM_PLMN_RAT_UMTS == rat)
        {
            sim_state_code = sim_state_code | ( 0x03 << 5);
        }
        else if(MMI_MODEM_PLMN_RAT_GSM == rat)
        {
            sim_state_code = sim_state_code | ( 0x02 << 5);
        }
		else if(MMI_MODEM_PLMN_RAT_NO_SERVICE == rat)
		{
			sim_state_code = sim_state_code | ( 0x06 << 5);
		}
        else
        {
            sim_state_code = sim_state_code | ( 0x00 << 5);
        }

#if 0
        sim_state_code = sim_state_code | (((unsigned short)MMI_Modem_Get_Signal_UP_DOWN(sim_id)) << 8);
#endif
    }
    return sim_state_code;
}

void lv_poc_update_stabar_sim_img(void)
{
#define MAX_SIM_COUNT 1
#define MAX_SIM_SIGNAL_STRENGTH 4
    unsigned short sim_state_code[MAX_SIM_COUNT] = { 0 };
    lv_poc_status_bar_sim_obj_t * sim_cont[MAX_SIM_COUNT] = {lv_poc_status_bar_fptr->sim1,
#if MAX_SIM_COUNT > 1
														    lv_poc_status_bar_fptr->sim2,
#endif
														    };
    const lv_img_dsc_t * sim_signal_img[MAX_SIM_COUNT * MAX_SIM_SIGNAL_STRENGTH + 1] = {&ic_signal_no_signal,
                                                                                  &ic_signal_1_1, &ic_signal_1_2,
                                                                                  &ic_signal_1_3, &ic_signal_1_4,
#if MAX_SIM_COUNT > 1
                                                                                  &ic_signal_2_1, &ic_signal_2_2,
                                                                                  &ic_signal_2_3, &ic_signal_2_4
#endif
                                                                                 };
    bool * has_sim[MAX_SIM_COUNT] = {&(lv_poc_status_bar_fptr->has_sim1),
#if MAX_SIM_COUNT > 1
								    &(lv_poc_status_bar_fptr->has_sim2),
#endif
								    };
    static unsigned short old_sim_state_code[MAX_SIM_COUNT] = { 0 };
    static bool is_continue = true;
    static int is_net_type = 5;
    int k;
    static lv_obj_t * obj2 = NULL;
    static lv_obj_t * obj3 = NULL;
    static lv_obj_t * obj4 = NULL;

    sim_state_code[0] = lv_poc_get_sim_state_code(POC_SIM_1);
#if MAX_SIM_COUNT > 1
    sim_state_code[1] = lv_poc_get_sim_state_code(POC_SIM_2);
#endif

    for(k = 0; k < MAX_SIM_COUNT; k++)
    {
        if(old_sim_state_code[k] != sim_state_code[k] || true == is_continue)
        {
            lv_poc_stabar_sim_clean(sim_cont[k]);

            if(k > 0)
            {
                sim_cont[k]->align_r_obj = sim_cont[k - 1]->align_l_obj;
            }

            if(k < MAX_SIM_COUNT - 1)
            {
                is_continue = true;
            }

            sim_cont[k]->align_l_obj = sim_cont[k]->align_r_obj;

            if(sim_state_code[k] & 0x01)
            {
                *(has_sim[k]) = true;
#if 0
                //在此处添加数据上传下载的图标


#endif

                obj2 = lv_img_create(lv_poc_status_bar, NULL);
                switch((sim_state_code[k] & 0x1e) >> 1)    //判断信号强度是多少
                {
                case MMI_MODEM_SIGNAL_BAR_0:
                {
                    lv_img_set_src(obj2, sim_signal_img[0]);
                    break;
                }

                case MMI_MODEM_SIGNAL_BAR_1:
                {
                    lv_img_set_src(obj2, sim_signal_img[k * MAX_SIM_SIGNAL_STRENGTH + 1]);
                    break;
                }

                case MMI_MODEM_SIGNAL_BAR_2:
                case MMI_MODEM_SIGNAL_BAR_3:
                {
                    lv_img_set_src(obj2, sim_signal_img[k * MAX_SIM_SIGNAL_STRENGTH + 2]);
                    break;
                }

                case MMI_MODEM_SIGNAL_BAR_4:
                {
                    lv_img_set_src(obj2, sim_signal_img[k * MAX_SIM_SIGNAL_STRENGTH + 3]);
                    break;
                }

                case MMI_MODEM_SIGNAL_BAR_5:
                {
                    lv_img_set_src(obj2, sim_signal_img[k * MAX_SIM_SIGNAL_STRENGTH + 4]);
                    break;
                }

                default:
                {
                    OSI_LOGI(0, "FUNC:%s  get a error net signal strength", __func__);
                    lv_poc_stabar_sim_clean(sim_cont[k]);
                    lv_img_set_src(obj2, &ic_no_sim);
                    lv_obj_align(obj2, *(sim_cont[k]->align_l_obj), LV_ALIGN_OUT_LEFT_MID, -2, 0);
                    sim_cont[k]->sim_img = obj2;
                    sim_cont[k]->align_l_obj = &(sim_cont[k]->sim_img);
                    old_sim_state_code[k] = sim_state_code[k];
                    continue;
                    break;
                }
                }
                lv_obj_align(obj2, *(sim_cont[k]->align_l_obj), LV_ALIGN_OUT_LEFT_MID, -2, 0);
                sim_cont[k]->sim_signal_strength_img = obj2;
                sim_cont[k]->align_l_obj = &(sim_cont[k]->sim_signal_strength_img);

                obj3 = lv_img_create(lv_poc_status_bar, NULL);
                switch((sim_state_code[k] & 0xe0) >> 5)   //判断是什么类型的信号
                {
                case 1:
                case 2://2G网络
                {	//显示G
                	(lvPocGuiBndCom_get_status()) ? lv_poc_set_apply_note(POC_APPLY_NOTE_TYPE_LOGINSUCCESS) : lv_poc_set_apply_note(POC_APPLY_NOTE_TYPE_NOLOGIN);
                    lv_img_set_src(obj3, &stat_sys_data_connected_2g_sprd);
					if(is_net_type != 1)
					{
						is_net_type = 1;
						lvPocLedCom_Msg(LVPOCLEDIDTCOM_SIGNAL_IDLE_STATUS, true);
					}
                    break;
                }

                case 3://3G网络
                {
                	(lvPocGuiBndCom_get_status()) ? lv_poc_set_apply_note(POC_APPLY_NOTE_TYPE_LOGINSUCCESS) : lv_poc_set_apply_note(POC_APPLY_NOTE_TYPE_NOLOGIN);
                    lv_img_set_src(obj3, &stat_sys_data_fully_connected_3g_sprd_reliance);
					if(is_net_type != 1)
					{
						is_net_type = 1;
						lvPocLedCom_Msg(LVPOCLEDIDTCOM_SIGNAL_IDLE_STATUS, true);
					}
                    break;
                }

                case 4:
                case 5:
                {
                	(lvPocGuiBndCom_get_status()) ? lv_poc_set_apply_note(POC_APPLY_NOTE_TYPE_LOGINSUCCESS) : lv_poc_set_apply_note(POC_APPLY_NOTE_TYPE_NOLOGIN);
                    lv_img_set_src(obj3, &stat_sys_data_connected_4g_sprd);
					if(is_net_type != 1)
					{
						is_net_type = 1;
						lvPocLedCom_Msg(LVPOCLEDIDTCOM_SIGNAL_IDLE_STATUS, true);
					}
					break;
                }
				case 6://显示无服务
				{
					lv_poc_set_apply_note(POC_APPLY_NOTE_TYPE_NONETWORK);
					lv_img_set_src(obj3, &ic_signal_no_server);
					if(is_net_type != 2)
					{
						is_net_type = 2;
						lvPocLedCom_Msg(LVPOCLEDIDTCOM_SIGNAL_SCAN_NETWORK_STATUS, true);
					}
					break;

				}
                default:
                {
                    OSI_LOGI(0, "FUNC:%s  get a error net type", __func__);
                    lv_obj_del(obj3);
                    old_sim_state_code[k] = sim_state_code[k];

#ifdef CONFIG_POC_GUI_GPS_SUPPORT
					//refr gps view
                    if(lv_poc_status_bar_fptr->gps_img != NULL)
                    {
                       lv_obj_del(lv_poc_status_bar_fptr->gps_img->gps_location_img);
                       lv_poc_status_bar_fptr->gps_img->gps_location_img = lv_img_create(lv_poc_status_bar, NULL);
                       lv_poc_init_stabar_gps_img(sim_cont[k]->align_l_obj);
                    }
#endif
                    continue;
                }
                }
                lv_obj_align(obj3, *(sim_cont[k]->align_l_obj), LV_ALIGN_OUT_LEFT_MID, -2, 0);
                sim_cont[k]->sim_net_type_img = obj3;
                sim_cont[k]->align_l_obj = &(sim_cont[k]->sim_net_type_img);
            }
            else
            {
                *(has_sim[k]) = false;
                obj4 = lv_img_create(lv_poc_status_bar, NULL);
                lv_img_set_src(obj4, &ic_no_sim);
                lv_obj_align(obj4, *(sim_cont[k]->align_l_obj), LV_ALIGN_OUT_LEFT_MID, -2, 0);
                sim_cont[k]->sim_img = obj4;
                sim_cont[k]->align_l_obj = &(sim_cont[k]->sim_img);
            }
            old_sim_state_code[k] = sim_state_code[k];

#ifdef CONFIG_POC_GUI_GPS_SUPPORT
			//refr gps view
	        if(lv_poc_status_bar_fptr->gps_img != NULL)
	        {
	           lv_obj_del(lv_poc_status_bar_fptr->gps_img->gps_location_img);
	           lv_poc_status_bar_fptr->gps_img->gps_location_img = lv_img_create(lv_poc_status_bar, NULL);
	           lv_poc_init_stabar_gps_img(sim_cont[k]->align_l_obj);
	        }
#endif
        }
    }
    is_continue = false;
}

/*******************
*     NAME:    lv_poc_get_refresh_ui
*   AUTHOR:    lugj
* DESCRIPT:    获取是否刷新UI
*     DATE:    2019-10-31
********************/
int lv_poc_get_refresh_ui(void)
{
	int state = lv_poc_refresh_ui_state;
	lv_poc_refresh_ui_state = 0;
	return state;
}

/*******************
*     NAME:    lv_poc_refresh_ui_next
*   AUTHOR:    lugj
* DESCRIPT:    刷新下一个UI
*     DATE:    2019-10-31
********************/
void lv_poc_refresh_ui_next(void)
{
	lv_poc_refresh_ui_state = 1;
}

/*******************
*     NAME:    lv_poc_create_activity
*   AUTHOR:    lugj
* DESCRIPT:    创建一个有效的activity
*     DATE:    2019-10-25
********************/
lv_poc_activity_t *lv_poc_create_activity(lv_poc_activity_ext_t *activity_ext,
        bool has_stabar,
        bool has_control,
        lv_poc_control_text_t *control_text)
{
    static bool is_lv_poc_atctivity_init = false;
    lv_style_t * poc_display_style = NULL;

    if(!is_lv_poc_atctivity_init)
    {
        is_lv_poc_atctivity_init = true;
        lv_poc_theme_init();
        lv_poc_status_bar_init();
        lv_poc_activity_list_init();
        lv_poc_stack_init();
    }

    poc_setting_conf = lv_poc_setting_conf_read();
    poc_display_style = (lv_style_t *)(poc_setting_conf->theme.current_theme->style_base);
    lv_poc_activity_t *activity = (lv_poc_activity_t *)lv_mem_alloc(sizeof(lv_poc_activity_t));
    memset(activity,0,sizeof(lv_poc_activity_t));
    if(_idle_activity == NULL)
    {
        _idle_activity = activity;
    }
    lv_poc_stack_push(activity);
    lv_poc_activity_list_insert(activity);

    current_activity = activity;

    activity->has_control = has_control;
    activity->has_stabar= has_stabar;
    activity->status_bar = lv_poc_status_bar;
    activity->activity_ext.actId = activity_ext->actId;
    activity->activity_ext.create = activity_ext->create;
    activity->activity_ext.prepare_destory = activity_ext->prepare_destory;

    activity->base = lv_obj_create(lv_scr_act(), NULL);
    lv_obj_set_size(activity->base,LV_POC_SCREEN_SCALE_HOR_RES,LV_POC_SCREEN_SCALE_VER_RES);
    lv_obj_set_pos(activity->base,LV_POC_SCREEN_X,LV_POC_SCREEN_Y);

    lv_obj_set_style(activity->base, poc_display_style);
    activity->display = lv_img_create(activity->base,NULL);
    lv_img_set_auto_size(activity->display, false);
    if(has_stabar)
    {
        lv_obj_set_size(activity->display,LV_POC_DISPLAY_NOCTR_HOR_RES,LV_POC_DISPLAY_NOCTR_VER_RES);
        lv_obj_set_pos(activity->display,LV_POC_DISPLAY_NOCTR_POSITION_X,LV_POC_DISPLAY_NOCTR_POSITION_Y);
        if(current_activity == _idle_activity)
        {
            lv_img_set_src(activity->display,&ic_launcher_bg);
        }
        lv_obj_set_parent(lv_poc_status_bar, activity->base);
    }
    else
    {
        lv_obj_set_size(activity->display,LV_POC_DISPLAY_NOCTR_STABAR_HOR_RES,LV_POC_DISPLAY_NOCTR_STABAR_VER_RES);
        lv_obj_set_pos(activity->display,LV_POC_DISPLAY_NOCTR_STABAR_POSITION_X,LV_POC_DISPLAY_NOCTR_STABAR_POSITION_Y);
        if(current_activity == _idle_activity)
        {
            lv_img_set_src(activity->display,&ic_launcher_bg);
        }
    }
    if(has_control)
    {
        if(control_text != NULL)
        {
            lv_poc_control_init(activity,
                                       control_text->left_text,
                                       control_text->middle_text,
                                       control_text->right_text);
        }
        else
        {
            lv_poc_control_init(activity,
                                       "",
                                       "",
                                       "");
        }
    }

    if(NULL == ancient_signal_func)
    {
        ancient_signal_func = activity->display->signal_cb;
    }

    if(NULL == ancient_design_func)
    {
        ancient_design_func = activity->display->design_cb;
    }

    if(NULL == ancient_event_func)
    {
        ancient_event_func = activity->display->event_cb;
    }

    lv_obj_set_signal_cb(activity->display, lv_poc_signal_cb);
    lv_obj_set_design_cb(activity->display, lv_poc_design_cb);
    lv_obj_set_event_cb(activity->display, lv_poc_event_cb);

    activity->signal_func = NULL;
    activity->design_func = NULL;
    activity->event_func = NULL;

    if(poc_indev_group == NULL)
    {
        poc_indev_group = lv_group_create();
        poc_keypad_dev  = lv_poc_get_keypad_dev();
        lv_group_set_wrap(poc_indev_group,true);
        lv_indev_set_group(poc_keypad_dev,poc_indev_group);
    }

    lv_group_focus_freeze(poc_indev_group,false);
    lv_group_add_obj(poc_indev_group,activity->display);
    lv_group_focus_obj(activity->display);
    lv_group_focus_freeze(poc_indev_group,true);

    if(activity->activity_ext.create != NULL)
    {
        activity->ext_data = (void *)activity->activity_ext.create(activity->display);
    }
    OSI_LOGI(0, "[poc_activity] create a activity\n");

    return activity;
}

/*******************
*     NAME:    lv_poc_create_mini_activity
*   AUTHOR:    lugj
* DESCRIPT:    创建一个有效的mini activity
*     DATE:    2019-10-25
********************/
lv_poc_activity_t *lv_poc_create_mini_activity(lv_poc_activity_ext_t *activity_ext, lv_area_t *act_area)
{
    lv_style_t * poc_display_style = NULL;

    poc_setting_conf = lv_poc_setting_conf_read();
    poc_display_style = (lv_style_t *)(poc_setting_conf->theme.current_theme->style_base);
    lv_poc_activity_t *activity = (lv_poc_activity_t *)lv_mem_alloc(sizeof(lv_poc_activity_t));
    memset(activity,0,sizeof(lv_poc_activity_t));

    lv_poc_stack_push(activity);
    lv_poc_activity_list_insert(activity);

    current_activity = activity;

    activity->has_control = false;
    activity->has_stabar= true;
    activity->status_bar = lv_poc_status_bar;
    activity->activity_ext.actId = activity_ext->actId;
    activity->activity_ext.create = activity_ext->create;
    activity->activity_ext.prepare_destory = activity_ext->prepare_destory;

	//背景虚化功能
	static lv_style_t BackgroundBlur_style;

	lv_style_copy(&BackgroundBlur_style, &lv_style_plain_color);

	BackgroundBlur_style.body.main_color = BackgroundBlur_style.body.grad_color = LV_COLOR_BLACK;
	BackgroundBlur_style.body.opa = LV_OPA_70;//透明度
	BackgroundBlur_style.body.radius = 0;//弧度

	activity->bg_blur_obj = lv_obj_create(lv_scr_act(), NULL);
	lv_obj_set_style(activity->bg_blur_obj, &BackgroundBlur_style);
	if(act_area->y2 <= 105)//默认弹窗大小
	{
		lv_obj_set_size(activity->bg_blur_obj, LV_HOR_RES - 0, LV_VER_RES - act_area->y1 + 15);
	}
	else
	{
		lv_obj_set_size(activity->bg_blur_obj, LV_HOR_RES - 0, LV_VER_RES - act_area->y1 + 8);
	}
	lv_obj_set_pos(activity->bg_blur_obj, 0, 18);
	lv_obj_set_opa_scale_enable(activity->bg_blur_obj, true);
	lv_obj_invalidate(activity->bg_blur_obj);

	/*信息框*/
    activity->base = lv_img_create(activity->bg_blur_obj, NULL);
    lv_img_set_auto_size(activity->base, false);
    lv_obj_set_size(activity->base, act_area->x2 - act_area->x1, act_area->y2 - act_area->y1);
    lv_obj_set_pos(activity->base, act_area->x1, act_area->y1 - 18);
    lv_img_set_src(activity->base,&ic_launcher_bg);
    lv_obj_set_style(activity->base, poc_display_style);

    activity->display = activity->base;

    lv_obj_set_signal_cb(activity->display, lv_poc_signal_cb);
    lv_obj_set_design_cb(activity->display, lv_poc_design_cb);
    lv_obj_set_event_cb(activity->display, lv_poc_event_cb);

    activity->signal_func = NULL;
    activity->design_func = NULL;
    activity->event_func = NULL;

#if 1
    lv_group_focus_freeze(poc_indev_group,false);
    lv_group_add_obj(poc_indev_group,activity->display);
    lv_group_focus_obj(activity->display);
    lv_group_focus_freeze(poc_indev_group,true);
#endif
    if(activity->activity_ext.create != NULL)
    {
        activity->ext_data = (void *)activity->activity_ext.create(activity->display);
    }

	lv_poc_anim_note(activity->bg_blur_obj);

    OSI_LOGI(0, "[poc_activity] create a mini activity\n");

    return activity;
}


/*******************
*     NAME:    lv_poc_get_user_obj
*   AUTHOR:    lugj
* DESCRIPT:    从activity中获取可以显示区域
*     DATE:    2019-10-25
********************/
lv_poc_display_t *lv_poc_get_display(lv_poc_activity_t *activity)
{
    if(activity)
    {
        return activity->display;
    }
    else
    {
        return NULL;
    }
}


/*******************
*     NAME:    lv_poc_del_activity
*   AUTHOR:    lugj
* DESCRIPT:    删除activity
*     DATE:    2019-10-25
********************/
bool lv_poc_del_activity(lv_poc_activity_t *activity)
{
    bool ret_val = true;

    lv_poc_activity_t * pre_activity;
    lv_poc_control_t *ctl = NULL;
    lv_poc_activity_ext_t * ext = &(activity->activity_ext);
    lv_poc_stack_pop();
    pre_activity = lv_poc_stack_top();
    lv_obj_set_parent(lv_poc_status_bar, pre_activity->base);

    current_activity = pre_activity;

    if(activity->has_control)
    {
        ctl = activity->control;
		if(ctl)//返回新建群组空指针
        lv_obj_del(ctl->background);
    }

    lv_poc_activity_list_delete(activity);
    lv_group_focus_freeze(poc_indev_group,false);
    lv_group_focus_prev(poc_indev_group);
    lv_group_focus_freeze(poc_indev_group, true);
    lv_indev_reset(poc_keypad_dev);
    lv_group_remove_obj(activity->display);
    lv_obj_del(activity->base);

	if(activity->bg_blur_obj != NULL)
	{
		lv_obj_del(activity->bg_blur_obj);
	}

    lv_obj_invalidate(pre_activity->base);

    if(ext->prepare_destory != NULL)
    {
        ext->prepare_destory(activity->display);
    }

    lv_mem_free(activity);
    lv_group_focus_obj(pre_activity->display);
    return ret_val;
}

/*******************
*     NAME:    lv_poc_group_remove_obj
*   AUTHOR:    lugj
* DESCRIPT:    将obj添加到keypad输入设备组中
*     DATE:    2019-10-25
********************/
void lv_poc_group_remove_obj(lv_obj_t *obj)
{
    if(poc_indev_group)
    {
        lv_group_add_obj(poc_indev_group,obj);
    }
}

/*******************
*     NAME:    lv_poc_group_add_obj
*   AUTHOR:    lugj
* DESCRIPT:    将obj从keypad输入设备组中移除
*     DATE:    2019-10-25
********************/
void lv_poc_group_add_obj(lv_obj_t *obj)
{
    if(poc_indev_group)
    {
        lv_group_remove_obj(obj);
    }
}

/*******************
*     NAME:    lv_poc_activity_set_signal_cb
*   AUTHOR:    lugj
* DESCRIPT:    设置activity的signal函数
*     NOTE:    signal函数运行机制:
					lv_xxx_create(parent,copy) ==> 调用parent->signal_func函数
*     DATE:    2019-10-30
********************/
void lv_poc_activity_set_signal_cb(lv_poc_activity_t *activity,
        lv_signal_cb_t func)
{
    if(NULL != func)
    {
        activity->signal_func = func;
    }
}

/*******************
*     NAME:    lv_poc_activity_set_design_cb
*   AUTHOR:    lugj
* DESCRIPT:    设置activity的signal函数
*     DATE:    2019-10-31
********************/
void lv_poc_activity_set_design_cb(lv_poc_activity_t *activity,
        lv_design_cb_t func)
{
    if(NULL != func)
    {
        activity->design_func = func;
    }
}

/*******************
*     NAME:    lv_poc_activity_set_background
*   AUTHOR:    lugj
* DESCRIPT:    设置背景图像
*     DATE:    2019-10-30
********************/
void lv_poc_activity_set_background(lv_poc_activity_t *activity,
        lv_img_dsc_t *bg_img)
{
    if(NULL != bg_img)
    {
        lv_img_set_src(activity->display,bg_img);
    }
}

/*******************
*     NAME:    lv_poc_activity_list_lookup
*   AUTHOR:    lugj
* DESCRIPT:    从activity查询表中查询display的activity
*     DATE:    2019-10-30
********************/
lv_poc_activity_t *	lv_poc_activity_list_lookup(lv_poc_display_t * display)
{
    lv_poc_activity_list_node_t * p = lv_poc_activity_list->head;
    if(display == NULL)
    {
        return NULL;
    }
    if(p->next != NULL)
    {
        p = p->next;
        while(p)
        {
            if(p->activity->display == display && p->activity != NULL && p->activity->display != NULL)
            {
                return p->activity;
            }
            p = p->next;
        }
    }
    return NULL;
}

/*******************
*     NAME:    lv_poc_activity_send_sign
*   AUTHOR:    lugj
* DESCRIPT:    从activity查询表中查询activity,并给其发送一个消息
*     DATE:    2019-10-30
********************/
lv_poc_activity_t *	lv_poc_activity_send_sign(lv_poc_Activity_Id_t ID, int sign, void * param)
{
    lv_poc_activity_list_node_t * p = lv_poc_activity_list->head;
    lv_poc_activity_ext_t * q;
    lv_exec_parameter_t * exec_obj;
    lv_task_t * task;
    if(p->next != NULL)
    {
        p = p->next;
        while(p)
        {
            if(p != NULL && p->activity != NULL && p->activity->base != NULL)
            {
                q = (lv_poc_activity_ext_t *)p->activity->base->ext_attr;
                if(q != NULL && q->actId == ID)
                {
                    exec_obj = (lv_exec_parameter_t *)lv_mem_alloc(sizeof(lv_exec_parameter_t));
                    exec_obj->activity = p->activity;
                    exec_obj->sign = sign;
                    exec_obj->parameters = (void *)param;
                    task = lv_task_create(lv_exec_task, 30, LV_TASK_PRIO_MID, (void *)exec_obj);
                    lv_task_once(task);
                    return p->activity;
                }
                p = p->next;
            }
        }
    }
    return NULL;
}

/*******************
*     NAME:    lv_poc_get_display_width
*   AUTHOR:    lugj
* DESCRIPT:    获取显示区域宽
*     DATE:    2019-10-31
********************/
lv_coord_t lv_poc_get_display_width(const lv_poc_display_t * display)
{
    return lv_obj_get_width(display);
}

/*******************
*     NAME:    lv_poc_get_display_height
*   AUTHOR:    lugj
* DESCRIPT:    获取显示区域高
*     DATE:    2019-10-31
********************/
lv_coord_t lv_poc_get_display_height(lv_poc_display_t * display)
{
    lv_coord_t height = 0;
    lv_poc_activity_t * activity;
    activity = lv_poc_activity_list_lookup(display);
    if(activity->has_control)
    {
        height = -LV_POC_CONTROL_VER_RES;
    }
    height = height + lv_obj_get_height(display);
    return height;
}


/*******************
*     NAME:    lv_poc_get_display_area
*   AUTHOR:    lugj
* DESCRIPT:    获取显示区域范围
*     DATE:    2019-10-31
********************/
void lv_poc_get_display_area(lv_poc_display_t * display, lv_area_t * area)
{
    area->x1 = lv_obj_get_x(display);
    area->y1 = lv_obj_get_y(display);
    area->x2 = area->x1 + lv_poc_get_display_width(display);
    area->y2 = area->y1 + lv_poc_get_display_height(display);
}

/*******************
*     NAME:    lv_poc_status_bar_task_ext_add
*   AUTHOR:    lugj
* DESCRIPT:    添加状态栏额外任务
*     DATE:    2019-10-31
********************/
bool lv_poc_status_bar_task_ext_add(status_bar_task_t task)
{
    static int stabar_task_ext_count = 0;
    if(stabar_task_ext_count >= LV_POC_STABAR_TASK_EXT_LENGTH)
    {
        return false;
    }

    status_bar_task_ext[stabar_task_ext_count] = task;
    stabar_task_ext_count = stabar_task_ext_count + 1;
    return true;
}


/*******************
*     NAME:    lv_poc_activity_hidden
*   AUTHOR:    lugj
* DESCRIPT:   隐藏activity
*     DATE:    2019-10-31
********************/
bool lv_poc_activity_hidden(lv_poc_activity_t * activity, bool isHidden)
{
    int j;
    lv_poc_activity_t * act;
    if(activity == NULL)
    {
        return false;
    }
    if(activity == _idle_activity)
    {
        OSI_LOGI(0, "idle activity is not hide\n");
        return false;
    }


    lv_obj_set_hidden(activity->base, isHidden);
    if( activity == current_activity)
    {
        activity->base->hidden = isHidden == false? 0:1;
        act = (lv_poc_activity_t *)lv_poc_statck.activity[lv_poc_statck.size - 2];
        lv_obj_set_hidden(act->base, false);
        if(activity->has_stabar)
        {
            for(j = lv_poc_statck.size - 2; j > 0; j--)
            {
                act = (lv_poc_activity_t *)lv_poc_statck.activity[j];
                if(act->has_stabar == true)
                {
                    break;
                }
            }
            act = (lv_poc_activity_t *)lv_poc_statck.activity[j];
            lv_obj_set_parent(lv_poc_status_bar, act->base);
        }
    }
    return true;


}


/*******************
*     NAME:    lv_poc_activity_set_idle
*   AUTHOR:    lugj
* DESCRIPT:   设置idle activity
*     DATE:    2019-10-31
********************/
bool lv_poc_activity_set_idle(lv_poc_activity_t * idle_activity)
{
    int k;
    _idle_activity = idle_activity;
    for(k=0; k < lv_poc_statck.size; k++)
    {
        if(lv_poc_statck.activity[k] == idle_activity)
        {
            break;
        }
    }
    if(k < lv_poc_statck.size)
    {
        for(int j = k; j > 0; j--)
        {
            lv_poc_statck.activity[j] = lv_poc_statck.activity[j - 1];
        }
    }
    lv_poc_statck.activity[0] = idle_activity;
    return true;
}

/*******************
*     NAME:    lv_poc_activity_enter_idle
*   AUTHOR:    lugj
* DESCRIPT:  回到主界面
*     DATE:    2019-10-31
********************/
void lv_poc_activity_enter_idle(void)
{
    lv_poc_activity_list_node_t * p = lv_poc_activity_list->head;
    lv_poc_activity_list_node_t * q;
    if(_idle_activity == NULL)
    {
        OSI_LOGI(0, "Please set idle activity\n");
        return;
    }
    while(p->next != NULL)
    {
        if(p->next->activity == _idle_activity)
        {
            p = p->next;
        }
        q = p->next;
        p->next =q->next;
        lv_poc_del_activity(q->activity);
        lv_mem_free(q);
    }
    if(lv_poc_activity_list->head->next == NULL)
    {
        OSI_LOGI(0, "Please set idle activity and Enter idle activity!\n");
        return;
    }
    lv_poc_activity_hidden(p->activity, false);
    lv_group_focus_obj(p->activity->display);
    lv_group_focus_freeze(poc_indev_group,true);
}

/*******************
*     NAME:    lv_poc_activity_pre
*   AUTHOR:    lugj
* DESCRIPT:   进入上一个activity
*     DATE:    2019-10-31
********************/
lv_poc_activity_t * lv_poc_activity_pre(bool forcs)
{
    lv_poc_activity_t * activity, * act;
    lv_poc_stack_pop();
    if(forcs == true)
    {
        activity = lv_poc_stack_pop();
    }
    else
    {
        do
        {
            activity = lv_poc_stack_pop();
            if(activity->base->hidden == false || activity == _idle_activity)
                break;
            lv_poc_del_activity(activity);
        }
        while(1);
    }
    lv_poc_stack_push(activity);
    lv_obj_set_hidden(activity->base, false);
    if(activity->has_stabar ==  false && lv_poc_statck.size > 1)
    {
        for(int k = lv_poc_statck.size - 1; k > 0; k--)
        {
            act = (lv_poc_activity_t *)lv_poc_statck.activity[k];
            if(act->has_stabar == true)
                break;
        }
        lv_obj_set_parent(lv_poc_status_bar, act->base);
    }
    else
    {
        lv_obj_set_parent(lv_poc_status_bar, activity->base);
    }
    return activity;
}

void FT_Trace(const char * formate, ...)
{
#if 0
#if FT_COUNT_DEBUG != 0
    static unsigned int print_number = 0;
    static char print_str[10] = {0};
    memset(print_str, 0, 10);
    sprintf(print_str, "FT-----> %d", ++print_number);
    lv_poc_print(print_str);
#endif
#elif 0
    static const int length_max = 100;
    char * p;
    char ch;
    char print_str[length_max + 1];
    char str_1[length_max];

    va_list args;
    memset(print_str, 0,length_max + 1);
    p = (char *)formate;
    va_start(args, formate);
    while(*p != '\0')
    {
        if(*p == '%')
        {
            p = p + 1;
            memset(str_1, 0, 100);
            switch(*p)
            {
            case 'd':
            case 'D':
            {
                int dd = va_arg(args, int);
                sprintf(str_1, "%d", dd);
                strcat(print_str, str_1);
            }
            break;
            case 'f':
            case 'F':
            {
                double dd = va_arg(args, double);
                sprintf(str_1, "%f", dd);
                strcat(print_str, str_1);
            }
            break;
            case 'c':
            case 'C':
            {
                ch = va_arg(args, char);
                sprintf(str_1, "%c", ch);
                strcat(print_str, str_1);
            }
            break;
            case 's':
            case 'S':
            {
                char * cc = va_arg(args, char *);
                strcat(print_str, cc);
            }
            break;
            default:
            {
                ch = *p;
                sprintf(str_1, "%%c", ch);
                strcat(print_str, str_1);
            }
            break;
            }
        }
        else
        {
            memset(str_1, 0, 100);
            ch = *p;
            sprintf(str_1, "%c", ch);
            strcat(print_str, str_1);
        }
        p = p + 1;
    }
    va_end(args);
    OSI_LOGI(0, print_str);
#else
    va_list ap;
    char buffer[128];
    memset(buffer, 0, sizeof(buffer));
    va_start(ap, formate);
    vsnprintf(buffer, sizeof(buffer)-1, formate, ap);
    va_end(ap);

    OSI_LOGI(0, "%s", buffer);
#endif
}

void lv_poc_member_list_cb_set_active(lv_poc_Activity_Id_t activity_id, bool enable)
{
	if(activity_id < 1)
	{
		return;
	}
	lv_poc_activity_attribute_cb_set_obj * cb_set_obj = lv_poc_get_activity_attribute_cb_set_obj();

	for(int i = 0; i < LV_POC_ACTIVITY_ATTRIBUTE_CB_SET_SIZE; i++)
	{
		if(cb_set_obj->member_list[i].activity_id == activity_id)
		{
		 cb_set_obj->member_list[i].active = enable;
		}
	}
}

void lv_poc_group_list_cb_set_active(lv_poc_Activity_Id_t activity_id, bool enable)
{
	if(activity_id < 1)
	{
		return;
	}
	lv_poc_activity_attribute_cb_set_obj * cb_set_obj = lv_poc_get_activity_attribute_cb_set_obj();

	for(int i = 0; i < LV_POC_ACTIVITY_ATTRIBUTE_CB_SET_SIZE; i++)
	{
		if(cb_set_obj->group_list[i].activity_id == activity_id)
		{
		 cb_set_obj->group_list[i].active = enable;
		}
	}
}

/*******************
*	  NAME:    lv_poc_anim_note
* DESCRIPT:    虚拟动画
*	  DATE:    2020-08-03
********************/
void lv_poc_anim_note(lv_obj_t *obj)
{
	//动画
	lv_anim_t lv_anim_obj;

	lv_anim_init(&lv_anim_obj);
	lv_anim_set_time(&lv_anim_obj, 500, 0);
	lv_anim_set_values(&lv_anim_obj, LV_OPA_TRANSP, LV_OPA_COVER);
	lv_anim_set_exec_cb(&lv_anim_obj, obj, (lv_anim_exec_xcb_t)lv_obj_set_opa_scale);

	lv_anim_create(&lv_anim_obj);
}

/*******************
*	  NAME:    lv_poc_get_current_activity
* DESCRIPT:    获取当前窗口
*	  DATE:    2020-10-09
********************/
lv_poc_activity_t *lv_poc_get_current_activity(void)
{
	return current_activity;
}

/*******************
*	  NAME:    lv_poc_is_build_tmpgroup
* DESCRIPT:    tmp grp
*	  DATE:    2020-12-29
********************/
bool lv_poc_is_build_tmpgroup(void)
{
	OSI_PRINTFI("[tmpgrp](%s)(%d):tmp state(%d)", __func__, __LINE__, lv_poc_build_tempgrp_progress(POC_TMPGRP_READ));
	if(lv_poc_build_tempgrp_progress(POC_TMPGRP_READ) <= POC_TMPGRP_OPENMEMLIST//must wait refr'compele
		&& lv_poc_build_tempgrp_progress(POC_TMPGRP_READ) >= POC_TMPGRP_VIEW)
	{
		return true;
	}
	return false;
}

/*******************
*	  NAME:    lv_poc_cbn_key_obj
* DESCRIPT:    组合键
*	  DATE:    2020-12-04
********************/
bool lv_poc_cbn_key_obj(lv_indev_data_t *data)
{
	//1(ptt)1(volum down)1(volum up)1(power)
	#define CBN_KEY_STATE_PRESS     (1<<0)
	#define CBN_KEY_GPS_DEBUG       (0b0011)
	#define CBN_KEY_CIT_CHECK       (0b0101)
	#define CBN_KEY_RECORDERBACK	(0b0110)
	#define CBN_KEY_PTT				(0b1010)

	static int  multi_keyvalue = 0;//multi-key
	//extra
	static bool  mic_status = false;

	switch(data->key)
	{
		case 0xf0:
		{
			int status = data->state == CBN_KEY_STATE_PRESS?true:false;

			do
			{
				status == true ? (multi_keyvalue|=0b0001):(multi_keyvalue&=0b0000);
			}while(0);

			break;
		}

		case LV_GROUP_KEY_UP:
		{
			if(lv_poc_cit_get_run_status() == LV_POC_CIT_OPRATOR_TYPE_KEY)
			{
				lv_poc_type_key_up_cb(true);
				return false;
			}

			int status = data->state == CBN_KEY_STATE_PRESS?true:false;

			do
			{
				status == true ? (multi_keyvalue|=0b0010):(multi_keyvalue&=0b0000);
			}while(0);

			break;
		}

		case LV_GROUP_KEY_DOWN:
		{
			if(lv_poc_cit_get_run_status() == LV_POC_CIT_OPRATOR_TYPE_KEY)
			{
				lv_poc_type_key_down_cb(true);
				return false;
			}

			int status = data->state == CBN_KEY_STATE_PRESS?true:false;

			do
			{
				status == true ? (multi_keyvalue|=0b0100):(multi_keyvalue&=0b0000);
			}while(0);

			break;
		}

		case LV_GROUP_KEY_POC:
		{
			int status = data->state == CBN_KEY_STATE_PRESS?true:false;

			do
			{
				status == true ? (multi_keyvalue|=0b1000):(multi_keyvalue&=0b0000);
			}while(0);

			break;
		}
	}

	switch(multi_keyvalue)
	{
		case CBN_KEY_GPS_DEBUG://debug gps
		{
#ifdef CONFIG_POC_GUI_GPS_SUPPORT
			lv_poc_gps_monitor_open();
#endif
			return true;
		}

		case CBN_KEY_CIT_CHECK://cit test
		{
			lv_poc_cit_open();
			return true;
		}

		case CBN_KEY_RECORDERBACK://record playback
		{
			//lv_poc_record_playback_open();
			return true;
		}

		case CBN_KEY_PTT://ptt
		{
			if(mic_status == false
				&& lv_poc_cit_get_run_status() == LV_POC_CIT_OPRATOR_TYPE_MIC)
			{
				lvPocLedCom_Msg(LVPOCGUICOM_SIGNAL_LOOPBACK_RECORDER_IND, false);
				mic_status = true;
			}
			return true;
		}

		default:
		{
			//extra
			if(mic_status == true
				&& lv_poc_cit_get_run_status() == LV_POC_CIT_OPRATOR_TYPE_MIC)
			{
				mic_status = false;
				lvPocLedCom_Msg(LVPOCGUICOM_SIGNAL_LOOPBACK_PLAYER_IND, false);
			}
			return false;
		}
	}

	return false;
}

/*******************
*	  NAME:    lv_poc_net_ping_task
* DESCRIPT:
*	  DATE:    2020-12-08
********************/
static
void lv_poc_net_ping_task(lv_task_t *task)
{
	lv_poc_virt_at_resp_send(POC_TYPE_VAT_PING);
}

/*******************
*	  NAME:    lv_poc_net_ping_task_create
* DESCRIPT:
*	  DATE:    2020-12-08
********************/
void lv_poc_net_ping_task_create(void)
{
	lv_task_create(lv_poc_net_ping_task, 1000, LV_TASK_PRIO_MID, NULL);
}

#ifdef __cplusplus
}
#endif
