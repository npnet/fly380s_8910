
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

#define LV_POC_IDLE_PAE_MAX_MSG_SIZE (20)

typedef struct{
	lv_poc_idle_page2_display_t state;
	char title[64];
	char second_line_text_1[64];
	char second_line_text_2[64];
	char third_line_text_1[64];
	char third_line_text_2[64];
} lv_poc_idle_page2_msg_content_t;

typedef struct{
	int reader;
	int writer;
	int normal_msg_count;
	lv_poc_idle_page2_msg_content_t msg[LV_POC_IDLE_PAE_MAX_MSG_SIZE];
	lv_poc_idle_page2_msg_content_t msg_normal_info;
	lv_poc_idle_page2_msg_content_t msg_warnning_info;
	lv_poc_idle_page2_msg_content_t msg_login_info;
	lv_poc_idle_page2_msg_content_t msg_audio;
	lv_poc_idle_page2_msg_content_t msg_join_group;
	lv_poc_idle_page2_msg_content_t msg_list_update;
	lv_poc_idle_page2_msg_content_t msg_speak;
	lv_poc_idle_page2_msg_content_t msg_tone;
	lv_poc_idle_page2_msg_content_t msg_tts;
	lv_poc_idle_page2_msg_content_t msg_listen;
} lv_poc_idle_page2_msg_t;

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
static void lv_poc_idle_page_task_cb(lv_task_t * task);


/*************************************************
*
*                  Local variable
*
*************************************************/
lv_poc_activity_t *activity_idle = NULL;
static lv_task_t * idle_page_task = NULL;
static lv_poc_idle_page2_msg_t *idle_page_task_msg = NULL;
static lv_poc_idle_page2_display_t page2_display_state = lv_poc_idle_page2_normal_info;
static lv_obj_t * idle_date_label;
static lv_obj_t * idle_big_clock;
static char idle_current_page = IDLE_PAGE_CURRENT;
static char idle_old_page = IDLE_PAGE_CURRENT;
static char idle_total_page = IDLE_PAGE_NUMBER;

static idle_display_func_t idle_display_funcs[] =
{
    {lv_poc_idle_page_1_show, lv_poc_idle_page_1_hide},
    {lv_poc_idle_page_2_show, lv_poc_idle_page_2_hide},
};

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

char * idle_title_label_text = "";
char * idle_user_label_text  = "";
char * idle_user_name_label_text = "";
char * idle_group_label_text = "";
char * idle_group_name_label_text = "";

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
    return NULL;
}

static void lv_poc_idle_prepare_destory(lv_obj_t *obj)
{
}


static lv_res_t lv_poc_idle_signal_func(struct _lv_obj_t * obj, lv_signal_t sign, void * param)
{
	OSI_LOGI(0, "[poc][signal][idle] sign <- %d\n", sign);
	OSI_LOGI(0, "[poc][signal][idle] param <- 0x%p\n", param);
	switch(sign)
	{
		case LV_SIGNAL_CONTROL:
		{
			if(NULL == param) return LV_RES_OK;
			unsigned int c = *(unsigned int *)param;
			switch(c)
			{
				case LV_GROUP_KEY_ENTER:
				{
					//lv_poc_main_menu_open();
					lv_event_send(activity_idle->control->left_button, LV_EVENT_CLICKED, NULL);
					break;
				}

				case LV_GROUP_KEY_ESC:
				{
					//lv_poc_idle_next_page();
					lv_event_send(activity_idle->control->right_button, LV_EVENT_CLICKED, NULL);
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
					lv_poc_group_list_open(NULL);
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

		case LV_SIGNAL_FOCUS:
		{
			lv_poc_idle_time_task();

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
	return LV_RES_INV;
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
	static bool is_running = false;
	if(is_running == true)
	{
		return;
	}
	is_running = true;
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
        is_running = false;
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
	is_running = false;
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

    lv_obj_set_hidden(idle_title_label, true);
    lv_obj_set_hidden(idle_user_label, true);
    lv_obj_set_hidden(idle_user_name_label, true);
    lv_obj_set_hidden(idle_group_label, true);
    lv_obj_set_hidden(idle_group_name_label, true);

    lv_obj_t *obj_content[5];
    char * obj_content_text[5];
    memset(obj_content, 0, sizeof(lv_obj_t *));
    memset(obj_content_text, 0, sizeof(char *));
    int obj_write = 0;

    if(page2_display_state == lv_poc_idle_page2_login_info)
    {
	    obj_content[obj_write] = idle_user_label;
	    obj_content_text[obj_write++] = idle_user_label_text;
    }
    else if(page2_display_state == lv_poc_idle_page2_audio)
    {
	    obj_content[obj_write] = idle_user_label;
	    obj_content_text[obj_write++] = idle_user_label_text;
	    obj_content[obj_write] = idle_group_label;
	    obj_content_text[obj_write++] = idle_group_label_text;
    }
    else if(page2_display_state == lv_poc_idle_page2_join_group)
    {
	    obj_content[obj_write] = idle_user_label;
	    obj_content_text[obj_write++] = idle_user_label_text;
	    obj_content[obj_write] = idle_group_label;
	    obj_content_text[obj_write++] = idle_group_label_text;
    }
    else if(page2_display_state == lv_poc_idle_page2_list_update)
    {
	    obj_content[obj_write] = idle_user_label;
	    obj_content_text[obj_write++] = idle_user_label_text;
    }
    else if(page2_display_state == lv_poc_idle_page2_speak)
    {
	    obj_content[obj_write] = idle_user_label;
	    obj_content_text[obj_write++] = idle_user_label_text;
	    obj_content[obj_write] = idle_group_label;
	    obj_content_text[obj_write++] = idle_group_label_text;
    }
    else if(page2_display_state == lv_poc_idle_page2_tone)
    {
	    obj_content[obj_write] = idle_user_label;
	    obj_content_text[obj_write++] = idle_user_label_text;
    }
    else if(page2_display_state == lv_poc_idle_page2_tts)
    {
	    obj_content[obj_write] = idle_user_label;
	    obj_content_text[obj_write++] = idle_user_label_text;
    }
    else if(page2_display_state == lv_poc_idle_page2_listen)
    {
	    obj_content[obj_write] = idle_user_label;
	    obj_content_text[obj_write++] = idle_user_label_text;
	    obj_content[obj_write] = idle_group_label;
	    obj_content_text[obj_write++] = idle_group_label_text;
    }
    else if(page2_display_state == lv_poc_idle_page2_normal_info)
    {
	    obj_content[obj_write] = idle_title_label;
	    obj_content_text[obj_write++] = idle_title_label_text;
	    obj_content[obj_write] = idle_user_label;
	    obj_content_text[obj_write++] = idle_user_label_text;
	    obj_content[obj_write] = idle_group_label;
	    obj_content_text[obj_write++] = idle_group_label_text;
	    obj_content[obj_write] = idle_user_name_label;
	    obj_content_text[obj_write++] = idle_user_name_label_text;
	    obj_content[obj_write] = idle_group_name_label;
	    obj_content_text[obj_write++] = idle_group_name_label_text;
    }
    else if(page2_display_state == lv_poc_idle_page2_warnning_info)
    {
	    obj_content[obj_write] = idle_user_label;
	    obj_content_text[obj_write++] = idle_user_label_text;
    }

	if(obj_write == 1)
	{
		lv_obj_set_width(obj_content[0], lv_poc_get_display_width(activity_idle->display)/40*35);
		lv_obj_align(obj_content[0],
			activity_idle->display,
			LV_ALIGN_IN_TOP_LEFT,
			lv_poc_get_display_width(activity_idle->display)/20,
			lv_poc_get_display_height(activity_idle->display)/12*5);
	}
	else if(obj_write == 2)
	{
		lv_obj_set_width(obj_content[0], lv_poc_get_display_width(activity_idle->display)/40*35);
		lv_obj_align(obj_content[0],
			activity_idle->display,
			LV_ALIGN_IN_TOP_LEFT,
			lv_poc_get_display_width(activity_idle->display)/20,
			lv_poc_get_display_height(activity_idle->display)/40*9);

		lv_obj_set_width(obj_content[1], lv_poc_get_display_width(activity_idle->display)/40*35);
		lv_obj_align(obj_content[1],
			obj_content[0],
			LV_ALIGN_OUT_BOTTOM_LEFT,
			0,
			0);
	}
	else if(obj_write == 3)
	{
		lv_obj_set_width(obj_content[0], lv_poc_get_display_width(activity_idle->display)/40*35);
		lv_obj_align(obj_content[0],
			activity_idle->display,
			LV_ALIGN_IN_TOP_LEFT,
			lv_poc_get_display_width(activity_idle->display)/20,
			lv_poc_get_display_height(activity_idle->display)/5);

		lv_obj_set_width(obj_content[1], lv_poc_get_display_width(activity_idle->display)/40*35);
		lv_obj_align(obj_content[1],
			obj_content[0],
			LV_ALIGN_OUT_BOTTOM_LEFT,
			0,
			0);

		lv_obj_set_width(obj_content[2], lv_poc_get_display_width(activity_idle->display)/40*35);
		lv_obj_align(obj_content[2],
			obj_content[1],
			LV_ALIGN_OUT_BOTTOM_LEFT,
			0,
			0);
	}
	else if(obj_write == 4)
	{
		lv_obj_set_width(obj_content[0], lv_poc_get_display_width(activity_idle->display)/40*13);
		lv_obj_align(obj_content[0],
			activity_idle->display,
			LV_ALIGN_IN_TOP_LEFT,
			lv_poc_get_display_width(activity_idle->display)/20,
			lv_poc_get_display_height(activity_idle->display)/5);

		lv_obj_set_width(obj_content[1], lv_poc_get_display_width(activity_idle->display)/40*13);
		lv_obj_align(obj_content[0],
			obj_content[0],
			LV_ALIGN_OUT_BOTTOM_LEFT,
			0,
			0);

		lv_obj_set_width(obj_content[2], lv_poc_get_display_width(activity_idle->display)/40*35 - lv_poc_get_display_width(obj_content[0]));
		lv_obj_align(obj_content[2],
			obj_content[0],
			LV_ALIGN_OUT_RIGHT_MID,
			0,
			0);

		lv_obj_set_width(obj_content[3], lv_poc_get_display_width(activity_idle->display)/40*35 - lv_poc_get_display_width(obj_content[1]));
		lv_obj_align(obj_content[3],
			obj_content[1],
			LV_ALIGN_OUT_RIGHT_MID,
			0,
			0);
	}
    else if(obj_write == 5)
    {
		lv_obj_set_width(obj_content[0], lv_poc_get_display_width(activity_idle->display)/10*9);
		lv_obj_align(obj_content[0],
			activity_idle->display,
			LV_ALIGN_IN_TOP_LEFT,
			lv_poc_get_display_width(activity_idle->display)/20,
			lv_poc_get_display_height(activity_idle->display)/10);

		lv_obj_set_width(obj_content[1], lv_poc_get_display_width(activity_idle->display)/40*13);
		lv_obj_align(obj_content[1],
			obj_content[0],
			LV_ALIGN_OUT_BOTTOM_LEFT,
			0,
			0);

		lv_obj_set_width(obj_content[2], lv_poc_get_display_width(activity_idle->display)/40*13);
		lv_obj_align(obj_content[2],
			obj_content[1],
			LV_ALIGN_OUT_BOTTOM_LEFT,
			0,
			0);

		lv_obj_set_width(obj_content[3], lv_poc_get_display_width(activity_idle->display)/40*35 - lv_poc_get_display_width(obj_content[1]));
		lv_obj_align(obj_content[3],
			obj_content[1],
			LV_ALIGN_OUT_RIGHT_MID,
			0,
			0);

		lv_obj_set_width(obj_content[4], lv_poc_get_display_width(activity_idle->display)/40*35 - lv_poc_get_display_width(obj_content[2]));
		lv_obj_align(obj_content[4],
			obj_content[2],
			LV_ALIGN_OUT_RIGHT_MID,
			0,
			0);
    }

    for(int obj_read = 0; obj_read < obj_write; obj_read++)
    {
	    if(obj_content[obj_read] != NULL)
	    {
		    lv_obj_set_hidden(obj_content[obj_read], false);
		    if(obj_content_text[obj_read] != NULL)
		    {
			    lv_label_set_text(obj_content[obj_read], obj_content_text[obj_read]);
		    }
		    else
		    {
			    lv_label_set_text(obj_content[obj_read], "");
		    }

			#if 0
			if(obj_write > 3)
			{
				if(obj_write == 4)
				{
					if(obj_read > 1)
					{
						lv_label_set_long_mode(obj_content[obj_read], LV_LABEL_LONG_EXPAND);
					}
					else
					{
						lv_label_set_long_mode(obj_content[obj_read], LV_LABEL_LONG_SROLL_CIRC);
					}
				}
				else if(obj_write == 5)
				{
					if(obj_read > 2)
					{
						lv_label_set_long_mode(obj_content[obj_read], LV_LABEL_LONG_EXPAND);
					}
					else
					{
						lv_label_set_long_mode(obj_content[obj_read], LV_LABEL_LONG_SROLL_CIRC);
					}
				}
			}
			else
			{
				lv_label_set_long_mode(obj_content[obj_read], LV_LABEL_LONG_EXPAND);
			}
			#endif
	    }
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
        lv_label_set_text(idle_title_label, idle_title_label_text);
        lv_label_set_text(idle_user_label, idle_user_label_text);
        lv_label_set_text(idle_group_label, idle_user_name_label_text);
        lv_label_set_text(idle_user_name_label, idle_group_label_text);
        lv_label_set_text(idle_group_name_label, idle_group_name_label_text);
        lv_label_set_long_mode(idle_title_label, LV_LABEL_LONG_SROLL_CIRC);
        lv_label_set_long_mode(idle_user_label, LV_LABEL_LONG_SROLL_CIRC);
        lv_label_set_long_mode(idle_group_label, LV_LABEL_LONG_SROLL_CIRC);
        lv_label_set_long_mode(idle_group_name_label, LV_LABEL_LONG_SROLL_CIRC);
        lv_label_set_long_mode(idle_user_name_label, LV_LABEL_LONG_SROLL_CIRC);
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

static void lv_poc_idle_page_task_cb(lv_task_t * task)
{
    static lv_poc_idle_page2_msg_content_t *dest_msg = NULL;
    static bool have_warning = false;

	do{
  		if(task == NULL || task->user_data == NULL) break;

		lv_poc_idle_page2_msg_t *page2_msg = (lv_poc_idle_page2_msg_t *)task->user_data;
		lv_poc_idle_page2_msg_content_t *msg_content = &page2_msg->msg[page2_msg->reader];
		bool is_normal = false;

		if(msg_content->state == lv_poc_idle_page2_none_msg)
		{
			if(page2_display_state != lv_poc_idle_page2_speak
				&& page2_display_state != lv_poc_idle_page2_listen
				&& page2_display_state != lv_poc_idle_page2_normal_info
				&& (have_warning == false || (have_warning == true && page2_display_state != lv_poc_idle_page2_warnning_info)))
			{
				page2_msg->normal_msg_count = (page2_msg->normal_msg_count + 1) % 25;

				if(page2_msg->normal_msg_count > 21)
				{
					if(have_warning == true)
					{
						page2_display_state = lv_poc_idle_page2_warnning_info;
						dest_msg = &page2_msg->msg_warnning_info;
						char *content[] = {dest_msg->second_line_text_1,};
						lv_poc_idle_set_page2(lv_poc_idle_page2_warnning_info, content, 1);
					}
					else
					{
						page2_display_state = lv_poc_idle_page2_normal_info;
						dest_msg = &page2_msg->msg_normal_info;
						char *content[] = {dest_msg->second_line_text_2, dest_msg->third_line_text_2};
						lv_poc_idle_set_page2(lv_poc_idle_page2_normal_info, content, 2);
					}
					page2_msg->normal_msg_count = 0;
					break;
				}
			}
			return;
		}
		page2_msg->normal_msg_count = 0;


		if(msg_content->state == lv_poc_idle_page2_normal_info)
		{
			page2_msg->reader = (page2_msg->reader + 1) % LV_POC_IDLE_PAE_MAX_MSG_SIZE;
			dest_msg = &page2_msg->msg_normal_info;
			memset(dest_msg, 0, sizeof(lv_poc_idle_page2_msg_content_t));
			dest_msg->state = lv_poc_idle_page2_normal_info;
			if(msg_content->title[0] == 0)
			{
				strcpy(dest_msg->title, (const char *)"公网对讲");
			}
			else
			{
				strcpy(dest_msg->title, (const char *)msg_content->title);
			}
			strcpy(dest_msg->second_line_text_1, (const char *)"用户:");
			if(msg_content->second_line_text_2[0] != 0)
			{
				strcpy(dest_msg->second_line_text_2, (const char *)msg_content->second_line_text_2);
			}
			strcpy(dest_msg->third_line_text_1, (const char *)"群组:");
			if(msg_content->third_line_text_2[0] != 0)
			{
				strcpy(dest_msg->third_line_text_2, (const char *)msg_content->third_line_text_2);
			}
			is_normal = true;
		}
		else if(msg_content->state == lv_poc_idle_page2_warnning_info)
		{
			switch((int)page2_display_state)
			{
				case lv_poc_idle_page2_none_msg:
				case lv_poc_idle_page2_normal_info:
				case lv_poc_idle_page2_warnning_info:
				case lv_poc_idle_page2_login_info:
				case lv_poc_idle_page2_audio:
				case lv_poc_idle_page2_join_group:
				case lv_poc_idle_page2_list_update:
				case lv_poc_idle_page2_tone:
				case lv_poc_idle_page2_tts:
				{
					if(msg_content->second_line_text_1[0] == 0)
					{
						dest_msg = &page2_msg->msg_normal_info;
						is_normal = true;
						have_warning = false;
						char *content[] = {dest_msg->second_line_text_2, dest_msg->third_line_text_2};
						lv_poc_idle_set_page2(lv_poc_idle_page2_normal_info, content, 2);
					}
					else
					{
						dest_msg = &page2_msg->msg_warnning_info;
						have_warning = true;
					}
					break;
				}
			}
		}
		else if(msg_content->state == lv_poc_idle_page2_login_info)
		{
			switch((int)page2_display_state)
			{
				case lv_poc_idle_page2_none_msg:
				case lv_poc_idle_page2_normal_info:
				case lv_poc_idle_page2_warnning_info:
				case lv_poc_idle_page2_login_info:
				case lv_poc_idle_page2_audio:
				case lv_poc_idle_page2_join_group:
				case lv_poc_idle_page2_list_update:
				case lv_poc_idle_page2_tone:
				case lv_poc_idle_page2_tts:
				{
					dest_msg = &page2_msg->msg_login_info;
					break;
				}
			}
		}
		else if(msg_content->state == lv_poc_idle_page2_audio)
		{
			switch((int)page2_display_state)
			{
				case lv_poc_idle_page2_none_msg:
				case lv_poc_idle_page2_normal_info:
				case lv_poc_idle_page2_warnning_info:
				case lv_poc_idle_page2_login_info:
				case lv_poc_idle_page2_audio:
				case lv_poc_idle_page2_join_group:
				case lv_poc_idle_page2_list_update:
				case lv_poc_idle_page2_tone:
				case lv_poc_idle_page2_tts:
				{
					dest_msg = &page2_msg->msg_audio;
					break;
				}
			}
		}
		else if(msg_content->state == lv_poc_idle_page2_join_group)
		{
			switch((int)page2_display_state)
			{
				case lv_poc_idle_page2_none_msg:
				case lv_poc_idle_page2_normal_info:
				case lv_poc_idle_page2_warnning_info:
				case lv_poc_idle_page2_login_info:
				case lv_poc_idle_page2_audio:
				case lv_poc_idle_page2_join_group:
				case lv_poc_idle_page2_list_update:
				case lv_poc_idle_page2_tone:
				case lv_poc_idle_page2_tts:
				{
					dest_msg = &page2_msg->msg_join_group;
					break;
				}
			}
		}
		else if(msg_content->state == lv_poc_idle_page2_list_update)
		{
			switch((int)page2_display_state)
			{
				case lv_poc_idle_page2_none_msg:
				case lv_poc_idle_page2_normal_info:
				case lv_poc_idle_page2_warnning_info:
				case lv_poc_idle_page2_login_info:
				case lv_poc_idle_page2_audio:
				case lv_poc_idle_page2_join_group:
				case lv_poc_idle_page2_list_update:
				case lv_poc_idle_page2_tone:
				case lv_poc_idle_page2_tts:
				{
					dest_msg = &page2_msg->msg_list_update;
					break;
				}
			}
		}
		else if(msg_content->state == lv_poc_idle_page2_speak)
		{
			switch((int)page2_display_state)
			{
				case lv_poc_idle_page2_none_msg:
				case lv_poc_idle_page2_normal_info:
				case lv_poc_idle_page2_warnning_info:
				case lv_poc_idle_page2_login_info:
				case lv_poc_idle_page2_audio:
				case lv_poc_idle_page2_join_group:
				case lv_poc_idle_page2_list_update:
				case lv_poc_idle_page2_tone:
				case lv_poc_idle_page2_tts:
				case lv_poc_idle_page2_speak:
				case lv_poc_idle_page2_listen:
				{
					if(msg_content->second_line_text_1[0] == 0
						|| msg_content->third_line_text_1[0] == 0)
					{
						dest_msg = &page2_msg->msg_normal_info;
						is_normal = true;
						char *content[] = {dest_msg->second_line_text_2, dest_msg->third_line_text_2};
						lv_poc_idle_set_page2(lv_poc_idle_page2_normal_info, content, 2);
					}
					else
					{
						dest_msg = &page2_msg->msg_speak;
					}
					break;
				}
			}
		}
		else if(msg_content->state == lv_poc_idle_page2_tone)
		{
			switch((int)page2_display_state)
			{
				case lv_poc_idle_page2_none_msg:
				case lv_poc_idle_page2_normal_info:
				case lv_poc_idle_page2_warnning_info:
				case lv_poc_idle_page2_login_info:
				case lv_poc_idle_page2_audio:
				case lv_poc_idle_page2_join_group:
				case lv_poc_idle_page2_list_update:
				case lv_poc_idle_page2_tone:
				case lv_poc_idle_page2_tts:
				{
					dest_msg = &page2_msg->msg_tone;
					break;
				}
			}
		}
		else if(msg_content->state == lv_poc_idle_page2_tts)
		{
			switch((int)page2_display_state)
			{
				case lv_poc_idle_page2_none_msg:
				case lv_poc_idle_page2_normal_info:
				case lv_poc_idle_page2_warnning_info:
				case lv_poc_idle_page2_login_info:
				case lv_poc_idle_page2_audio:
				case lv_poc_idle_page2_join_group:
				case lv_poc_idle_page2_list_update:
				case lv_poc_idle_page2_tone:
				case lv_poc_idle_page2_tts:
				{
					dest_msg = &page2_msg->msg_tts;
					break;
				}
			}
		}
		else if(msg_content->state == lv_poc_idle_page2_listen)
		{
			switch((int)page2_display_state)
			{
				case lv_poc_idle_page2_none_msg:
				case lv_poc_idle_page2_normal_info:
				case lv_poc_idle_page2_warnning_info:
				case lv_poc_idle_page2_login_info:
				case lv_poc_idle_page2_audio:
				case lv_poc_idle_page2_join_group:
				case lv_poc_idle_page2_list_update:
				case lv_poc_idle_page2_tone:
				case lv_poc_idle_page2_tts:
				case lv_poc_idle_page2_speak:
				case lv_poc_idle_page2_listen:
				{
					if(msg_content->second_line_text_1[0] == 0
						|| msg_content->third_line_text_1[0] == 0)
					{
						dest_msg = &page2_msg->msg_normal_info;
						is_normal = true;
						char *content[] = {dest_msg->second_line_text_2, dest_msg->third_line_text_2};
						lv_poc_idle_set_page2(lv_poc_idle_page2_normal_info, content, 2);
					}
					else
					{
						dest_msg = &page2_msg->msg_speak;
					}
				}
			}
		}
		else
		{
			memset(msg_content, 0, sizeof(lv_poc_idle_page2_msg_content_t));
			page2_msg->reader = (page2_msg->reader + 1) % LV_POC_IDLE_PAE_MAX_MSG_SIZE;
			return;
		}

		dest_msg->state = msg_content->state;
		if(is_normal)
		{
			dest_msg->state = lv_poc_idle_page2_normal_info;
		}

		if(!is_normal)
		{
			strcpy(dest_msg->title, (const char *)msg_content->title);
			strcpy(dest_msg->second_line_text_1, (const char *)msg_content->second_line_text_1);
			strcpy(dest_msg->second_line_text_2, (const char *)msg_content->second_line_text_2);
			strcpy(dest_msg->third_line_text_1, (const char *)msg_content->third_line_text_1);
			strcpy(dest_msg->third_line_text_2, (const char *)msg_content->third_line_text_2);
		}

		page2_display_state = dest_msg->state;
		memset(msg_content, 0, sizeof(lv_poc_idle_page2_msg_content_t));
		page2_msg->reader = (page2_msg->reader + 1) % LV_POC_IDLE_PAE_MAX_MSG_SIZE;
	} while(0);

	if(dest_msg != NULL)
	{
		idle_title_label_text = dest_msg->title;
		idle_user_label_text = dest_msg->second_line_text_1;
		idle_user_name_label_text = dest_msg->second_line_text_2;
		idle_group_label_text = dest_msg->third_line_text_1;
		idle_group_name_label_text = dest_msg->third_line_text_2;
	}

#if 1  //关掉会导致不在idle界面时，信息不刷新
	lv_label_set_text(idle_title_label, idle_title_label_text);
	lv_label_set_text(idle_user_label, idle_user_label_text);
	lv_label_set_text(idle_user_name_label, idle_user_name_label_text);
	lv_label_set_text(idle_group_label, idle_group_label_text);
	lv_label_set_text(idle_group_name_label, idle_group_name_label_text);
#endif

    if(activity_idle != NULL)
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

}

void lv_poc_idle_set_page2(lv_poc_idle_page2_display_t msg_type, char * content[], int csize)
{
	if(content == NULL || idle_page_task_msg == NULL || idle_page_task == NULL || csize < 1)
	{
		return;
	}

	int content_index = 0;
	char * text_content[5];
	int text_content_write = 0;
	memset(text_content, 0, sizeof(char *) * 5);

	lv_poc_idle_page2_msg_t *page2_msg = idle_page_task_msg;
	lv_poc_idle_page2_msg_content_t *msg_content = &page2_msg->msg[page2_msg->writer];

	memset(msg_content, 0, sizeof(lv_poc_idle_page2_msg_content_t));

    if(msg_type == lv_poc_idle_page2_login_info && csize > 0)
    {
	    text_content[text_content_write++] = msg_content->second_line_text_1;
    }
    else if(msg_type == lv_poc_idle_page2_audio && csize > 1)
    {
	    text_content[text_content_write++] = msg_content->second_line_text_1;
	    text_content[text_content_write++] = msg_content->third_line_text_1;
    }
    else if(msg_type == lv_poc_idle_page2_join_group && csize > 1)
    {
	    text_content[text_content_write++] = msg_content->second_line_text_1;
	    text_content[text_content_write++] = msg_content->third_line_text_1;
    }
    else if(msg_type == lv_poc_idle_page2_list_update && csize > 0)
    {
	    text_content[text_content_write++] = msg_content->second_line_text_1;
    }
    else if(msg_type == lv_poc_idle_page2_speak && csize > 1)
    {
	    text_content[text_content_write++] = msg_content->second_line_text_1;
	    text_content[text_content_write++] = msg_content->third_line_text_1;
    }
    else if(msg_type == lv_poc_idle_page2_tone && csize > 0)
    {
	    text_content[text_content_write++] = msg_content->second_line_text_1;
    }
    else if(msg_type == lv_poc_idle_page2_tts && csize > 0)
    {
	    text_content[text_content_write++] = msg_content->second_line_text_1;
    }
    else if(msg_type == lv_poc_idle_page2_listen && csize > 1)
    {
	    text_content[text_content_write++] = msg_content->second_line_text_1;
	    text_content[text_content_write++] = msg_content->third_line_text_1;
    }
    else if(msg_type == lv_poc_idle_page2_normal_info && csize > 1)
    {
	    if(csize > 2)
	    {
		    text_content[text_content_write++] = msg_content->title;
		    text_content[text_content_write++] = msg_content->second_line_text_2;
		    text_content[text_content_write++] = msg_content->third_line_text_2;
	    }
	    else
	    {
		    text_content[text_content_write++] = msg_content->second_line_text_2;
		    text_content[text_content_write++] = msg_content->third_line_text_2;
	    }
    }
    else if(msg_type == lv_poc_idle_page2_warnning_info && csize > 0)
    {
	    text_content[text_content_write++] = msg_content->second_line_text_1;
    }

	for(int text_read = 0; text_read < text_content_write; text_read++)
	{
		if(text_content[text_read] != NULL)
		{
			if(content[content_index] != NULL)
			{
				strcpy(text_content[text_read], content[content_index]);
			}
			content_index++;
		}
	}

	if(text_content_write > 0)
	{
		page2_msg->writer = (page2_msg->writer + 1) % LV_POC_IDLE_PAE_MAX_MSG_SIZE;
		msg_content->state = msg_type;
	}
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
		lv_poc_main_menu_open();
	}
}

static void  lv_poc_idle_control_right_label_event_cb(lv_obj_t * obj, lv_event_t event)
{
	if(LV_EVENT_CLICKED == event)
	{
		lv_poc_idle_next_page();
	}
}

lv_poc_activity_t * lv_poc_create_idle(void)
{
	static lv_poc_activity_ext_t	activity_idle_ext = {ACT_ID_POC_IDLE,
															lv_poc_idle_create,
															lv_poc_idle_prepare_destory};
	lv_poc_control_text_t control = {"主菜单", "", "下一页"}; //{"主菜单","群组","下一页"}
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

	lv_poc_idle_time_task();
	lv_poc_idle_page_2_hide();

	idle_page_task_msg = (lv_poc_idle_page2_msg_t *)lv_mem_alloc(sizeof(lv_poc_idle_page2_msg_t));
	if(idle_page_task_msg != NULL)
	{
		memset(idle_page_task_msg, 0, sizeof(lv_poc_idle_page2_msg_t));
		idle_page_task = lv_task_create(lv_poc_idle_page_task_cb, 100, LV_TASK_PRIO_LOWEST, idle_page_task_msg);
	}

	char * content[] = {"正在检查网络",};
	lv_poc_idle_set_page2(lv_poc_idle_page2_warnning_info, content, 1);

	return activity_idle;
}


