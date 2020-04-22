
/*************************************************
*
*                  Include
*
*************************************************/
#include "lv_include/lv_poc.h"

#include <stdio.h>

/*************************************************
*
*                  Define
*
*************************************************/
#define IDLE_DATE_LABEL_STR_LEN 40
#define IDLE_PAGE_NUMBER           lv_poc_idle_page_num

//设置idle界面当前页
#define IDLE_PAGE_CURRENT          lv_poc_idle_page_1

#define IDLE_USER_NAME_LENGTH      100
#define IDLE_GROUP_NAME_LENGTH     100

typedef struct _idle_display_func_t {
    void (*show)(void);
    void (*hide)(void);
} idle_display_func_t;

typedef enum {
	lv_poc_idle_page2_normal_info,
	lv_poc_idle_page2_warnning_info,
	lv_poc_idle_page2_login_info,
	lv_poc_idle_page2_audio,
	lv_poc_idle_page2_join_group,
	lv_poc_idle_page2_list_update,
	lv_poc_idle_page2_speak,
	lv_poc_idle_page2_tone,
	lv_poc_idle_page2_tts,
	lv_poc_idle_page2_listen
} lv_poc_idle_page2_display_t;

typedef enum {
	lv_poc_idle_page_1,
	lv_poc_idle_page_2,
	lv_poc_idle_page_num
} lv_poc_idle_page_e;

/*************************************************
*
*                  Local function protocol
*
*************************************************/
static lv_obj_t * lv_poc_idle_create(lv_obj_t *obj);
static void lv_poc_idle_prepare_destory(lv_obj_t *obj);
static lv_res_t lv_poc_idle_signal_func(struct _lv_obj_t * obj, lv_signal_t sign, void * param);
static bool lv_poc_idle_design_func(struct _lv_obj_t * obj, const lv_area_t * mask_p, lv_design_mode_t mode);
static char * lv_poc_idle_get_date(lv_poc_time_t *time);
static char * lv_poc_idle_week_map(const int wday);
static void lv_poc_idle_time_task(void);
static char lv_poc_idle_next_page(void);
static void lv_poc_idle_page_1_show(void);
static void lv_poc_idle_page_1_hide(void);
static void lv_poc_idle_page_2_show(void);
static void lv_poc_idle_page_2_hide(void);
static void lv_poc_idle_page_2_init(void);
static void lv_poc_idle_page_task(void);
static void lv_poc_idle_page_2_normal_display_task(lv_task_t * task);


/*************************************************
*
*                  Local variable
*
*************************************************/
lv_poc_activity_t *activity_idle = NULL;
static lv_task_t * idle_page2_normal_info_timer_task = NULL;
static lv_obj_t * idle_date_label;
static lv_obj_t * idle_big_clock;
static char idle_current_page = IDLE_PAGE_CURRENT;
static char idle_old_page = IDLE_PAGE_CURRENT;
static char idle_total_page = IDLE_PAGE_NUMBER;

static char get_self_info_success = 0;

static idle_display_func_t idle_display_funcs[] = 
{
    {lv_poc_idle_page_1_show, lv_poc_idle_page_1_hide},
    {lv_poc_idle_page_2_show, lv_poc_idle_page_2_hide},
};

static lv_poc_idle_page2_display_t page2_display_state = 0;

/*************************************************
*
*                  Global variable
*
*************************************************/

lv_obj_t * idle_title_label;
lv_obj_t * idle_user_label;
lv_obj_t * idle_user_name_label;
lv_obj_t * idle_group_label;
lv_obj_t * idle_group_name_label;

char * idle_title_label_text;
char * idle_user_label_text;
char * idle_user_name_label_text;
char * idle_group_label_text;
char * idle_group_name_label_text;

/*************************************************
*
*                  Static function decalre
*
*************************************************/
static lv_obj_t * lv_poc_idle_create(lv_poc_display_t *display)
{
    idle_total_page = sizeof(idle_display_funcs)/sizeof(idle_display_funcs[0]);
    idle_old_page = idle_current_page;
	lv_poc_status_bar_task_ext_add(lv_poc_idle_time_task);
    lv_poc_status_bar_task_ext_add(lv_poc_idle_page_task);
    return NULL;
}

static void lv_poc_idle_prepare_destory(lv_obj_t *obj)
{
}


static lv_res_t lv_poc_idle_signal_func(struct _lv_obj_t * obj, lv_signal_t sign, void * param)
{
	OSI_LOGI(0, "%s [%d]\n", __func__, sign);
	switch(sign)
	{
		case LV_SIGNAL_PRESSED:
		{
			unsigned int c = *(unsigned int *)param;
			switch(c)
			{
				case LV_GROUP_KEY_ENTER:
				{
					lv_poc_main_menu_open();
					break;
				}
				
				case LV_GROUP_KEY_ESC:
				{
					lv_poc_idle_next_page();
					break;
				}
				
				case LV_GROUP_KEY_DOWN:
				{
					break;
				}
				
				case LV_GROUP_KEY_UP:
				{
					break;
				}
				
				case LV_GROUP_KEY_GP:
				{
					lv_poc_group_list_open();
					break;
				}
				
				case LV_GROUP_KEY_MB:
				{
					lv_poc_member_list_open(NULL, NULL, false);
					break;
				}
				
				case LV_GROUP_KEY_VOL_DOWN:
				{
					break;
				}
				
				case LV_GROUP_KEY_VOL_UP:
				{
					break;
				}
				
				case LV_GROUP_KEY_POC:
				{
					break;
				}
			}
			break;
		}
			
		case LV_SIGNAL_LONG_PRESS:
		{
			unsigned int c = *(unsigned int *)param;
			switch(c)
			{
				case LV_GROUP_KEY_ENTER:
				{
					break;
				}
				
				case LV_GROUP_KEY_ESC:
				{
					break;
				}
				
				case LV_GROUP_KEY_DOWN:
				{
					break;
				}
				
				case LV_GROUP_KEY_UP:
				{
					break;
				}
				
				case LV_GROUP_KEY_GP:
				{
					break;
				}
				
				case LV_GROUP_KEY_MB:
				{
					break;
				}
				
				case LV_GROUP_KEY_VOL_DOWN:
				{
					break;
				}
				
				case LV_GROUP_KEY_VOL_UP:
				{
					break;
				}
				
				case LV_GROUP_KEY_POC:
				{
					break;
				}
			}
			break;
		}
			
		case LV_SIGNAL_CONTROL:
		{
			unsigned int c = *(unsigned int *)param;
			switch(c)
			{
				case LV_GROUP_KEY_ENTER:
				{
					break;
				}
				
				case LV_GROUP_KEY_ESC:
				{
					break;
				}
				
				case LV_GROUP_KEY_DOWN:
				{
					break;
				}
				
				case LV_GROUP_KEY_UP:
				{
					break;
				}
				
				case LV_GROUP_KEY_GP:
				{
					break;
				}
				
				case LV_GROUP_KEY_MB:
				{
					break;
				}
				
				case LV_GROUP_KEY_VOL_DOWN:
				{
					break;
				}
				
				case LV_GROUP_KEY_VOL_UP:
				{
					break;
				}
				
				case LV_GROUP_KEY_POC:
				{
					break;
				}
			}
			break;
		}

		case LV_SIGNAL_FOCUS:
		{
			
		    if(idle_page2_normal_info_timer_task != 0)
		    {
		    	lv_task_del(idle_page2_normal_info_timer_task);
		    	idle_page2_normal_info_timer_task = 0;
		    }
		    
			if(get_self_info_success == 1
					&& page2_display_state != lv_poc_idle_page2_normal_info
					&& page2_display_state != lv_poc_idle_page2_warnning_info)
			{
				idle_page2_normal_info_timer_task = lv_task_create(lv_poc_idle_page_2_normal_display_task,
																		2000, LV_TASK_PRIO_LOWEST, NULL);
			}
			
			if(current_activity == activity_idle)
			{
		        for(int k = 0; k < idle_total_page; k++)
		        {
		            if( k == idle_current_page)
		            {
		                idle_display_funcs[k].show();
		            }
		            else
		            {
		                idle_display_funcs[k].hide();
		            }
		        }
	        }
			break;
		}

		case LV_SIGNAL_DEFOCUS:
		{
			break;
		}
			
		default:
		{
			break;
		}
	}
	return LV_RES_OK;
}

static bool lv_poc_idle_design_func(struct _lv_obj_t * obj, const lv_area_t * mask_p, lv_design_mode_t mode)
{
	return true;
}

static char * lv_poc_idle_get_date(lv_poc_time_t * time)
{
	static char data_str[IDLE_DATE_LABEL_STR_LEN];
	memset(data_str, 0, sizeof(data_str)/sizeof(data_str[0]));
	sprintf(data_str,"%d月%d日 星期%s",time->tm_mon, time->tm_mday, lv_poc_idle_week_map(time->tm_wday));
	return data_str;
}

static char * lv_poc_idle_week_map(const int wday)
{
	switch(wday)
	{
		case 0:
			return "日";
		case 1:
			return "一";
		case 2:
			return "二";
		case 3:
			return "三";
		case 4:
			return "四";
		case 5:
			return "五";
		case 6:
			return "六";
		default:
			return "";
	}
}

static void lv_poc_idle_time_task(void)
{
	static lv_poc_time_t time = {0};
	static char big_clock_str[10] = {0};
	static const char * str = NULL;
    static char old_big_clock_str[10] = {0};
    static char old_str[IDLE_DATE_LABEL_STR_LEN] = {0};
    static bool isFirst = true;
	static bool isInit = false;
    static lv_style_t * idle_clock_style;   
	static lv_style_t * style; 
	//static lv_coord_t screen_w = 0;
	static lv_coord_t screen_h = 0;
	static bool isCreatedPocTask = false;
	POC_MMI_MODEM_PLMN_RAT network_type = MMI_MODEM_PLMN_RAT_UNKNOW;
	int8_t net_type_name[64];
	memset(big_clock_str, 0 , 10);
	if(isInit == false)
	{
        isInit = true;
	    //screen_w = lv_poc_get_display_width(activity_idle->display);
	    screen_h = lv_poc_get_display_height(activity_idle->display);
	    idle_clock_style = (lv_style_t *)(poc_setting_conf->theme.current_theme->style_idle_big_clock);
        idle_big_clock = lv_label_create(activity_idle->display, NULL);
        lv_obj_set_style(idle_big_clock, idle_clock_style);        
    	idle_date_label = lv_label_create(activity_idle->display,NULL);
    	style = (lv_style_t *)(poc_setting_conf->theme.current_theme->style_idle_date_label);
    	lv_label_set_align(idle_date_label, LV_LABEL_ALIGN_CENTER);
    	lv_label_set_style(idle_date_label, LV_LABEL_STYLE_MAIN, style);
	}
    
    if(current_activity->has_stabar == false)
    {
        lv_obj_set_hidden(lv_poc_status_bar, true);
    }
    else
    {
        lv_obj_set_hidden(lv_poc_status_bar, false);
    }
	
	if(isCreatedPocTask == false)
	{
		poc_get_operator_req(poc_setting_conf->main_SIM, net_type_name, &network_type);
		if(network_type == MMI_MODEM_PLMN_RAT_LTE || network_type == MMI_MODEM_PLMN_RAT_GSM || network_type == MMI_MODEM_PLMN_RAT_UMTS)
		{
			isCreatedPocTask = true;
		}
	}
	
    if( current_activity != activity_idle)
    {
        //lv_obj_set_hidden(idle_big_clock, true);
        //lv_obj_set_hidden(idle_date_label, true);
        return;
    }
    //else if(current_activity->)
    //{
        //lv_obj_set_hidden(idle_big_clock, false);
        //lv_obj_set_hidden(idle_date_label, false);
    //}
	lv_poc_get_time(&time);
	sprintf(big_clock_str,"%02d:%02d",time.tm_hour,time.tm_min);
	str = lv_poc_idle_get_date(&time);

    if(isFirst == true)
    {
        isFirst = false;
    	lv_label_set_text(idle_big_clock,big_clock_str);
    	lv_label_set_text(idle_date_label,str);
    }
    else
    {
        if( 0 != strcmp(old_big_clock_str, big_clock_str))
        {
            lv_label_set_text(idle_big_clock,big_clock_str);
        }
        if(0 != strcmp(old_str, str))
        {
            lv_label_set_text(idle_date_label,str);
        }
    }
    if(activity_idle->has_control == true)
    {
        lv_obj_align(idle_big_clock, activity_idle->display, LV_ALIGN_CENTER, 0, -screen_h * 4 / 24);
    }
    else
    {
        lv_obj_align(idle_big_clock, activity_idle->display, LV_ALIGN_CENTER, 0, -screen_h/12);
    }
    lv_obj_align(idle_date_label, idle_big_clock, LV_ALIGN_OUT_BOTTOM_MID, 0, -10);
    strcpy(old_big_clock_str, big_clock_str);
    strcpy(old_str,str);
}


static char lv_poc_idle_next_page(void)
{
    idle_current_page = (idle_current_page + 1) % idle_total_page;
    idle_old_page = idle_current_page;
    for(int k = 0; k < idle_total_page; k++)
    {
        if( k == idle_current_page)
        {
            idle_display_funcs[k].show();
        }
        else
        {
            idle_display_funcs[k].hide();
        }
    }
    return idle_current_page;
}

static void lv_poc_idle_page_1_show(void)
{
    lv_obj_set_hidden(idle_date_label, false);
    lv_obj_set_hidden(idle_big_clock, false);
}
static void lv_poc_idle_page_1_hide(void)
{
    lv_obj_set_hidden(idle_date_label, true);
    lv_obj_set_hidden(idle_big_clock, true);
}
static void lv_poc_idle_page_2_show(void)
{
    lv_poc_idle_page_2_init();
    if(page2_display_state == lv_poc_idle_page2_login_info)
    {
	    lv_obj_set_hidden(idle_title_label, false);
	    lv_label_set_text(idle_title_label, idle_title_label_text);
	    lv_obj_set_hidden(idle_user_label, true);
	    lv_obj_set_hidden(idle_user_name_label, true);
	    lv_obj_set_hidden(idle_group_label, true);
	    lv_obj_set_hidden(idle_group_name_label, true);
    }
    else if(page2_display_state == lv_poc_idle_page2_audio)
    {
	    lv_obj_set_hidden(idle_title_label, false);
	    lv_obj_set_hidden(idle_user_label, true);
	    lv_obj_set_hidden(idle_user_name_label, true);
	    lv_obj_set_hidden(idle_group_label, true);
	    lv_obj_set_hidden(idle_group_name_label, true);
    }
    else if(page2_display_state == lv_poc_idle_page2_join_group)
    {
	    lv_obj_set_hidden(idle_title_label, false);
	    lv_label_set_text(idle_title_label, idle_title_label_text);
	    lv_obj_set_hidden(idle_user_label, true);
	    lv_obj_set_hidden(idle_user_name_label, true);
	    lv_obj_set_hidden(idle_group_label, true);
	    lv_obj_set_hidden(idle_group_name_label, true);
    }
    else if(page2_display_state == lv_poc_idle_page2_list_update)
    {
	    lv_obj_set_hidden(idle_title_label, false);
	    lv_label_set_text(idle_title_label, idle_title_label_text);
	    lv_obj_set_hidden(idle_user_label, true);
	    lv_obj_set_hidden(idle_user_name_label, true);
	    lv_obj_set_hidden(idle_group_label, true);
	    lv_obj_set_hidden(idle_group_name_label, true);
    }
    else if(page2_display_state == lv_poc_idle_page2_speak)
    {
	    lv_obj_set_hidden(idle_title_label, false);
	    lv_label_set_text(idle_title_label, idle_title_label_text);
	    lv_obj_set_hidden(idle_user_label, true);
	    lv_obj_set_hidden(idle_user_name_label, true);
	    lv_obj_set_hidden(idle_group_label, true);
	    lv_obj_set_hidden(idle_group_name_label, true);
    }
    else if(page2_display_state == lv_poc_idle_page2_tone)
    {
	    lv_obj_set_hidden(idle_title_label, false);
	    lv_label_set_text(idle_title_label, idle_title_label_text);
	    lv_obj_set_hidden(idle_user_label, true);
	    lv_obj_set_hidden(idle_user_name_label, true);
	    lv_obj_set_hidden(idle_group_label, true);
	    lv_obj_set_hidden(idle_group_name_label, true);
    }
    else if(page2_display_state == lv_poc_idle_page2_tts)
    {
	    lv_obj_set_hidden(idle_title_label, false);
	    lv_label_set_text(idle_title_label, idle_title_label_text);
	    lv_obj_set_hidden(idle_user_label, true);
	    lv_obj_set_hidden(idle_user_name_label, true);
	    lv_obj_set_hidden(idle_group_label, true);
	    lv_obj_set_hidden(idle_group_name_label, true);
    }
    else if(page2_display_state == lv_poc_idle_page2_listen)
    {
	    lv_obj_set_hidden(idle_title_label, false);
	    lv_label_set_text(idle_title_label, idle_title_label_text);
	    lv_obj_set_hidden(idle_user_label, false);
	    lv_label_set_text(idle_user_label, idle_user_label_text);
	    lv_obj_set_hidden(idle_user_name_label, false);
	    lv_label_set_text(idle_user_name_label, idle_user_name_label_text);
	    lv_obj_set_hidden(idle_group_label, true);
	    lv_obj_set_hidden(idle_group_name_label, true);
    }
    else if(page2_display_state == lv_poc_idle_page2_normal_info)
    {
	    lv_obj_set_hidden(idle_title_label, false);
	    lv_label_set_text(idle_title_label, idle_title_label_text);
	    lv_obj_set_hidden(idle_user_label, false);
	    lv_label_set_text(idle_user_label, idle_user_label_text);
	    lv_obj_set_hidden(idle_user_name_label, true);
	    lv_obj_set_hidden(idle_group_label, false);
	    lv_label_set_text(idle_group_label, idle_group_label_text);
	    lv_obj_set_hidden(idle_group_name_label, true);
    }
    else if(page2_display_state == lv_poc_idle_page2_warnning_info)
    {
	    lv_obj_set_hidden(idle_title_label, false);
	    lv_label_set_text(idle_title_label, idle_title_label_text);
	    lv_obj_set_hidden(idle_user_label, true);
	    lv_obj_set_hidden(idle_user_name_label, true);
	    lv_obj_set_hidden(idle_group_label, true);
	    lv_obj_set_hidden(idle_group_name_label, true);
    }
}
static void lv_poc_idle_page_2_hide(void)
{
    lv_poc_idle_page_2_init();
    lv_obj_set_hidden(idle_title_label, true);
    lv_obj_set_hidden(idle_user_label, true);
    lv_obj_set_hidden(idle_user_name_label, true);
    lv_obj_set_hidden(idle_group_label, true);
    lv_obj_set_hidden(idle_group_name_label, true);
}

static void lv_poc_idle_page_2_init(void)
{
    static lv_style_t * style;
    static bool isInit_page_2 = false;
    if(isInit_page_2 == false)
    {
        isInit_page_2 = true;
        page2_display_state = lv_poc_idle_page2_login_info;
        style = (lv_style_t *)(poc_setting_conf->theme.current_theme->style_idle_msg_label);
        
        idle_title_label = lv_label_create(activity_idle->display, NULL);
        lv_label_set_style(idle_title_label, LV_LABEL_STYLE_MAIN, style);
        idle_user_label = lv_label_create(activity_idle->display, NULL);
        idle_user_name_label = lv_label_create(activity_idle->display, NULL);
        idle_group_label = lv_label_create(activity_idle->display, NULL);
        idle_group_name_label = lv_label_create(activity_idle->display, NULL);
        lv_label_set_style(idle_user_label, LV_LABEL_STYLE_MAIN, style);
        lv_label_set_style(idle_user_name_label, LV_LABEL_STYLE_MAIN, style);
        lv_label_set_style(idle_group_label, LV_LABEL_STYLE_MAIN, style);
        lv_label_set_style(idle_group_name_label, LV_LABEL_STYLE_MAIN, style);
        lv_label_set_text(idle_title_label, "检查网络");
        lv_label_set_text(idle_user_label, "");
        lv_label_set_text(idle_group_label, "");
        lv_label_set_text(idle_user_name_label, "");
        lv_label_set_text(idle_group_name_label, "");
        lv_label_set_long_mode(idle_user_name_label, LV_LABEL_LONG_SROLL);
        lv_label_set_long_mode(idle_group_name_label, LV_LABEL_LONG_SROLL);
        if(activity_idle->has_control == true)
        {
            lv_obj_align(idle_title_label, activity_idle->display, LV_ALIGN_IN_TOP_LEFT, 
                lv_poc_get_display_width(activity_idle->display)/20,
                lv_poc_get_display_height(activity_idle->display)/15);
        }
        else
        {
            lv_obj_align(idle_title_label, activity_idle->display, LV_ALIGN_IN_TOP_LEFT, 
                lv_poc_get_display_width(activity_idle->display)/20,
                lv_poc_get_display_height(activity_idle->display)/5);
        }
        lv_obj_align(idle_user_label, idle_title_label, LV_ALIGN_OUT_BOTTOM_LEFT,
            0,lv_obj_get_height(idle_title_label)/10);
        lv_obj_align(idle_group_label, idle_user_label, LV_ALIGN_OUT_BOTTOM_LEFT,
            0,lv_obj_get_height(idle_user_label)/10);
        lv_obj_align(idle_user_name_label, idle_user_label, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
        lv_obj_align(idle_group_name_label, idle_group_label, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
        
    }
}

static void lv_poc_idle_page_task(void)
{
    static lv_poc_activity_t * old_activity = NULL;
    static bool isFirst = true;
    if(true == isFirst)
    {
        old_activity = activity_idle;
    }
    if(current_activity != activity_idle )
    {
        if(old_activity != current_activity)
        {
            old_activity = current_activity;
            for(int k = 0; k < idle_total_page; k++)
            {
                idle_display_funcs[k].hide();
            }
        }
        return;
    }
    
    if(idle_old_page != idle_current_page || true == isFirst)
    {
        idle_old_page = idle_current_page;
        isFirst = false;
        for(int k = 0; k < idle_total_page; k++)
        {
            if( k == idle_current_page)
            {
                idle_display_funcs[k].show();
            }
            else
            {
                idle_display_funcs[k].hide();
            }
        }
    }
    
}

static void lv_poc_idle_page_2_normal_display_task(lv_task_t * task)
{
	idle_page2_normal_info_timer_task = 0;
	activity_idle ->signal_func(activity_idle->display, LV_SIGNAL_FOCUS, NULL);
}

/*************************************************
*
*                  Public function declare
*
*************************************************/
void lv_poc_idle_set_user_name(const char * name_str)
{
    static char idle_user_name[IDLE_USER_NAME_LENGTH] = {0};
    int length = strlen(name_str);
    if(length > IDLE_USER_NAME_LENGTH)
    {
        length = IDLE_USER_NAME_LENGTH;
    }
    if(length <= 0)
    {
        strcpy(idle_user_name, "");
    }
    else
    {
        memcpy(idle_user_name, name_str, length);
    }
    lv_label_set_text(idle_user_name_label, idle_user_name);
}

void lv_poc_idle_set_group_name(const char * name_str)
{
    static char idle_user_name[IDLE_USER_NAME_LENGTH] = {0};
    int length = strlen(name_str);
    if(length > IDLE_USER_NAME_LENGTH)
    {
        length = IDLE_USER_NAME_LENGTH;
    }
    if(length <= 0)
    {
        strcpy(idle_user_name, "");
    }
    else
    {
        memcpy(idle_user_name, name_str, length);
    }
    lv_label_set_text(idle_group_name_label, idle_user_name);
}

char * lv_poc_idle_get_user_name(void)
{
    return lv_label_get_text(idle_user_name_label);
}

char * lv_poc_idle_get_group_name(void)
{
    return lv_label_get_text(idle_group_name_label);
}

static void  lv_poc_idle_control_left_label_event_cb(lv_obj_t * obj, lv_event_t event)
{
	if(LV_EVENT_CLICKED == event)
	{
		//lv_poc_main_menu_open();
	}
}

static void  lv_poc_idle_control_right_label_event_cb(lv_obj_t * obj, lv_event_t event)
{
	if(LV_EVENT_CLICKED == event)
	{
		//lv_poc_idle_next_page();
	}
}

lv_poc_activity_t * lv_poc_create_idle(void)
{
	static lv_poc_activity_ext_t	activity_idle_ext = {ACT_ID_POC_IDLE,
															lv_poc_idle_create,
															lv_poc_idle_prepare_destory};
	lv_poc_control_text_t control = {"主菜单", "群组", "下一页"}; //{"主菜单","群组","下一页"}
	if(activity_idle != NULL)
	{
		return NULL;
	}
	poc_setting_conf = lv_poc_setting_conf_read();
	activity_idle = lv_poc_create_activity(&activity_idle_ext,true,true,&control);
	lv_poc_activity_set_signal_cb(activity_idle, lv_poc_idle_signal_func);
	lv_poc_activity_set_design_cb(activity_idle, lv_poc_idle_design_func);
	lv_obj_set_click(activity_idle->control->left_button, true);
	lv_obj_set_event_cb(activity_idle->control->left_button, lv_poc_idle_control_left_label_event_cb);
	lv_obj_set_click(activity_idle->control->right_button, true);
	lv_obj_set_event_cb(activity_idle->control->right_button, lv_poc_idle_control_right_label_event_cb);

	return activity_idle;
}


