
/*************************************************
*
*                  Include
*
*************************************************/
#include "lv_include/lv_poc.h"
#include "stdlib.h"

#define LV_POC_NOTATIONWINDOW_LABEL_TEXT_MAX_SIZE 50

#define LV_POC_NOTATION_TASK_QUEUE_SIZE (15)

typedef struct
{
	lv_poc_notation_msg_type_t msg_type;
	int8_t label_1_text[LV_POC_NOTATIONWINDOW_LABEL_TEXT_MAX_SIZE];
	int8_t label_2_text[LV_POC_NOTATIONWINDOW_LABEL_TEXT_MAX_SIZE];
} lv_poc_notation_task_msg_t;

/**
 * Represents a point on the screen.
 */

static lv_obj_t * lv_poc_notationwindow_obj = NULL;
static lv_obj_t * lv_poc_notationwindow_label_1 = NULL;
static lv_obj_t * lv_poc_notationwindow_label_2 = NULL;
static lv_obj_t * lv_poc_notationwindow_label_3 = NULL;
static int8_t lv_poc_notationwindow_label_1_text[LV_POC_NOTATIONWINDOW_LABEL_TEXT_MAX_SIZE] = {0};
static int8_t lv_poc_notationwindow_label_2_text[LV_POC_NOTATIONWINDOW_LABEL_TEXT_MAX_SIZE] = {0};
static int8_t lv_poc_notationwindow_label_3_text[LV_POC_NOTATIONWINDOW_LABEL_TEXT_MAX_SIZE] = {0};

static lv_style_t lv_poc_notation_style = {0};

static lv_task_t * lv_poc_notation_delay_close_task = NULL;

static lv_task_t * lv_poc_notation_task = NULL;

static lv_poc_notation_task_msg_t lv_poc_notation_task_queue[LV_POC_NOTATION_TASK_QUEUE_SIZE] = {0};

static int32_t lv_poc_notation_task_queue_reader = 0;

static int32_t lv_poc_notation_task_queue_writer = 0;

static bool lv_poc_notation_delay_close_task_running = false;

static uint32_t lv_poc_txt_utf8_get_length(const char * txt);

static bool lv_poc_notation_listen_speak_status = false;

lv_obj_t * lv_poc_notation_create(void)
{
	if(lv_poc_notationwindow_obj != NULL)
	{
		return lv_poc_notationwindow_obj;
	}
	poc_setting_conf = lv_poc_setting_conf_read();
	memset(&lv_poc_notation_style, 0, sizeof(lv_style_t));
    lv_style_copy(&lv_poc_notation_style, &lv_style_pretty_color);
    lv_poc_notation_style.text.color = LV_COLOR_BLACK;
    lv_poc_notation_style.text.font = (lv_font_t *)(poc_setting_conf->font.idle_popwindows_msg_font);
    lv_poc_notation_style.text.opa = 255;
    lv_poc_notation_style.body.main_color = LV_COLOR_SILVER;
    lv_poc_notation_style.body.grad_color = LV_COLOR_SILVER;
    lv_poc_notation_style.body.opa = 220;/*更改背景透明度*/
	lv_poc_notation_style.text.letter_space = 1;//字间隔
	//lv_poc_notation_style.text.line_space = 2;//行间隔

	lv_poc_notationwindow_obj = lv_obj_create(lv_scr_act(), NULL);
    lv_obj_set_style(lv_poc_notationwindow_obj,&lv_poc_notation_style);
    lv_obj_set_size(lv_poc_notationwindow_obj, LV_HOR_RES * 27/ 40, LV_VER_RES / 2);
    lv_obj_set_pos(lv_poc_notationwindow_obj, LV_HOR_RES * 13/ 80, LV_VER_RES / 4);
	lv_poc_notationwindow_label_1 = lv_label_create(lv_poc_notationwindow_obj, NULL);
	lv_obj_set_size(lv_poc_notationwindow_label_1, lv_obj_get_width(lv_poc_notationwindow_obj) - 4, lv_obj_get_height(lv_poc_notationwindow_obj) / 3);
    lv_label_set_style(lv_poc_notationwindow_label_1, 1,&lv_poc_notation_style);
    lv_label_set_long_mode(lv_poc_notationwindow_label_1, LV_LABEL_LONG_SROLL_CIRC);
    lv_label_set_text(lv_poc_notationwindow_label_1, "                    ");
    lv_label_set_align(lv_poc_notationwindow_label_1, LV_LABEL_ALIGN_CENTER);

	lv_poc_notationwindow_label_2 = lv_label_create(lv_poc_notationwindow_obj, NULL);
	lv_obj_set_size(lv_poc_notationwindow_label_2, lv_obj_get_width(lv_poc_notationwindow_obj) - 4, lv_obj_get_height(lv_poc_notationwindow_obj) / 3);
    lv_label_set_style(lv_poc_notationwindow_label_2,1,&lv_poc_notation_style);
    lv_label_set_long_mode(lv_poc_notationwindow_label_2, LV_LABEL_LONG_SROLL_CIRC);
    lv_label_set_text(lv_poc_notationwindow_label_2, "                    ");
    lv_label_set_align(lv_poc_notationwindow_label_2, LV_LABEL_ALIGN_CENTER);

    return lv_poc_notationwindow_obj;
}


void lv_poc_notation_destory(void)
{
	if(lv_poc_notationwindow_obj == NULL)
	{
		return;
	}

	lv_obj_del(lv_poc_notationwindow_obj);
	//不可以加上，会死机
	//lv_obj_del(lv_poc_notationwindow_label_1);
	//lv_obj_del(lv_poc_notationwindow_label_2);

	lv_poc_notationwindow_obj = NULL;
	lv_poc_notationwindow_label_1 = NULL;
	lv_poc_notationwindow_label_2 = NULL;
	lv_poc_notationwindow_label_3 = NULL;

	memset(lv_poc_notationwindow_label_1_text,
			0,
			LV_POC_NOTATIONWINDOW_LABEL_TEXT_MAX_SIZE * sizeof(int8_t));
	memset(lv_poc_notationwindow_label_2_text,
			0,
			LV_POC_NOTATIONWINDOW_LABEL_TEXT_MAX_SIZE * sizeof(int8_t));
	memset(lv_poc_notationwindow_label_3_text,
			0,
			LV_POC_NOTATIONWINDOW_LABEL_TEXT_MAX_SIZE * sizeof(int8_t));
}

void lv_poc_notation_refresh(void)
{
	uint8_t notationwindows_size = 0;//判断换是否有两行字
	lv_obj_t *label_1 = NULL;
	lv_obj_t *label_2 = NULL;
	uint32_t label_1_length = 0;//字的长度
	uint32_t label_2_length = 0;

	if(lv_poc_notationwindow_obj == NULL)
	{
		return;
	}

	if(lv_poc_notationwindow_obj->hidden == true)
	{
		return;
	}

	lv_obj_set_parent(lv_poc_notationwindow_obj, lv_scr_act());

    if(lv_poc_notationwindow_label_1->hidden == true)
    {
	    if(lv_poc_notationwindow_label_2->hidden == false)
	    {
    		lv_label_set_text(lv_poc_notationwindow_label_2,(char *) lv_poc_notationwindow_label_2_text);
			lv_obj_set_width(lv_poc_notationwindow_label_2,lv_obj_get_width(lv_poc_notationwindow_obj) - 4);
    		lv_obj_set_pos(lv_poc_notationwindow_label_2, 2,
    						lv_obj_get_height(lv_poc_notationwindow_obj) / 3//消息框居中
    							- lv_obj_get_height(lv_poc_notationwindow_label_2) / 3);
		}
		label_1 = lv_poc_notationwindow_label_2;
		notationwindows_size = 1;//1行

		//获取文本长度
		label_2_length = lv_poc_txt_utf8_get_length((char *)lv_poc_notationwindow_label_2_text);
    }
    else
    {
    	if(lv_poc_notationwindow_label_2->hidden == true)
    	{
		    lv_label_set_text(lv_poc_notationwindow_label_1, (char *)lv_poc_notationwindow_label_1_text);
			lv_obj_set_width(lv_poc_notationwindow_label_1,lv_obj_get_width(lv_poc_notationwindow_obj) - 4);
    		lv_obj_set_pos(lv_poc_notationwindow_label_1, 2,
    						lv_obj_get_height(lv_poc_notationwindow_obj) / 3//消息框居中
    							- lv_obj_get_height(lv_poc_notationwindow_label_1) / 3);
			label_1 = lv_poc_notationwindow_label_1;
			notationwindows_size = 1;//1行

			//获取文本长度
			label_1_length = lv_poc_txt_utf8_get_length((char *)lv_poc_notationwindow_label_1_text);
    	}
    	else/*两行都没hide*/
    	{
		    lv_label_set_text(lv_poc_notationwindow_label_1, (char *)lv_poc_notationwindow_label_1_text);
			lv_obj_set_width(lv_poc_notationwindow_label_1,lv_obj_get_width(lv_poc_notationwindow_obj) - 4);
    		lv_obj_set_pos(lv_poc_notationwindow_label_1, 2,
    						lv_obj_get_height(lv_poc_notationwindow_obj) / 2
    							- lv_obj_get_height(lv_poc_notationwindow_label_1) - 4);

			/*加入群组字样不滚动文本框*/
			if(lv_poc_notation_listen_speak_status == true)
			{
				/*设置群组字样框*/
				lv_label_set_text(lv_poc_notationwindow_label_3, (char *)lv_poc_notationwindow_label_3_text);
				lv_obj_set_width(lv_poc_notationwindow_label_3, 48);
				lv_obj_set_pos(lv_poc_notationwindow_label_3, 2,
	    						lv_obj_get_height(lv_poc_notationwindow_obj) / 2 + 4);

	    		lv_label_set_text(lv_poc_notationwindow_label_2, (char *)lv_poc_notationwindow_label_2_text);
				lv_obj_set_width(lv_poc_notationwindow_label_2, 80);
				lv_obj_set_pos(lv_poc_notationwindow_label_2, 2 + 48,
	    						lv_obj_get_height(lv_poc_notationwindow_obj) / 2 + 4);

			}
			else
			{
	    		lv_label_set_text(lv_poc_notationwindow_label_2, (char *)lv_poc_notationwindow_label_2_text);
				//宽度
				lv_obj_set_width(lv_poc_notationwindow_label_2,lv_obj_get_width(lv_poc_notationwindow_obj) - 4);
				//消息提示框大小
				lv_obj_set_pos(lv_poc_notationwindow_label_2, 2,
	    						lv_obj_get_height(lv_poc_notationwindow_obj) / 2 + 4);
			}
			notationwindows_size = 2;//2行
			label_1 = lv_poc_notationwindow_label_1;
			label_2 = lv_poc_notationwindow_label_2;

			//获取文本长度
			label_1_length = lv_poc_txt_utf8_get_length((char *)lv_poc_notationwindow_label_1_text);
			label_2_length = lv_poc_txt_utf8_get_length((char *)lv_poc_notationwindow_label_2_text);
    	}
    }
#if 0
	OSI_LOGI(0,"[song]label_1_length = %d,label_2_length = %d",
	label_1_length,label_2_length);
#endif
	if(label_1_length > 4 || label_2_length > 4)//文本长度 大于4个汉字
	{
		//重新设置消息框原点                                                 13/80
		lv_obj_set_pos(lv_poc_notationwindow_obj, LV_HOR_RES * 3/ 80, LV_VER_RES / 3);
		//重新设置消息框宽度
		lv_obj_set_width(lv_poc_notationwindow_obj,LV_HOR_RES * 37/ 40);//1行消息
                                                                   //27/ 40
		//重新设置文本框原点
		lv_obj_set_pos(lv_poc_notationwindow_label_1, 1,
						lv_obj_get_height(lv_poc_notationwindow_obj) / 2
							- lv_obj_get_height(lv_poc_notationwindow_label_1) - 4);
		//重新设置文本宽度
		lv_obj_set_width(lv_poc_notationwindow_label_1,
							lv_obj_get_width(lv_poc_notationwindow_obj) - 1);
	}
	else//还原消息框
	{
		//重新设置消息框原点
		lv_obj_set_pos(lv_poc_notationwindow_obj, LV_HOR_RES * 13/ 80, LV_VER_RES / 3);
		//重新设置消息框宽度
		lv_obj_set_width(lv_poc_notationwindow_obj,LV_HOR_RES * 27/ 40);

	}

    /*设置消息框大小*/
	if(notationwindows_size == 1)
	{
		//消息框高度
		lv_obj_set_height(lv_poc_notationwindow_obj,LV_VER_RES / 3);//1行消息

		//设置文本框
		lv_obj_align(label_1, lv_poc_notationwindow_obj, LV_ALIGN_CENTER, 0, 0);

	}
	else if(notationwindows_size == 2)//2行消息
	{
		if(label_1_length == 4 && label_2_length == 2)
		{//切换群主选项
			//重新设置消息框原点
			lv_obj_set_pos(lv_poc_notationwindow_obj, LV_HOR_RES * 13/ 80, LV_VER_RES / 3);
			//重新设置消息框宽度
			lv_obj_set_width(lv_poc_notationwindow_obj,LV_HOR_RES * 27/ 40);
		}
		else
		{
			//重新设置消息框原点															//消息框拉高		LV_VER_RES / 3(中间)
			lv_obj_set_pos(lv_poc_notationwindow_obj, LV_HOR_RES * 3/ 80, LV_VER_RES / 4);//（空白的中间）
			//重新设置消息框高度
			lv_obj_set_height(lv_poc_notationwindow_obj,LV_VER_RES / 2);

		}
		//设置文本框
		lv_obj_align(label_1, lv_poc_notationwindow_obj, LV_ALIGN_IN_TOP_MID, 0, 10);

		if(lv_poc_notation_listen_speak_status == true)/*设置群组框*/
		{
			lv_obj_align(lv_poc_notationwindow_label_3, lv_poc_notationwindow_obj, LV_ALIGN_IN_BOTTOM_LEFT, 10, -10);
			lv_obj_align(label_2, lv_poc_notationwindow_obj, LV_ALIGN_IN_BOTTOM_RIGHT, -8, -10);
		}
		else
		{
			lv_obj_align(label_2, lv_poc_notationwindow_obj, LV_ALIGN_IN_BOTTOM_MID, 0, -10);
		}
	}

}

void lv_poc_notation_hide(const bool hide)
{
	if(lv_poc_notationwindow_obj == NULL)
	{
		return;
	}

	lv_obj_set_hidden(lv_poc_notationwindow_obj, hide);
}

lv_obj_t * lv_poc_notation_listenning(const int8_t * text_1, const int8_t * text_2)
{
	if(lv_poc_notationwindow_obj == NULL)
	{
		lv_poc_notation_create();
	}
	if(lv_poc_notationwindow_label_3 == NULL)
	{
		/*添加 (群组:) 固定框*/
		lv_poc_notationwindow_label_3 = lv_label_create(lv_poc_notationwindow_obj, NULL);
		lv_label_set_style(lv_poc_notationwindow_label_3,1,&lv_poc_notation_style);
		lv_label_set_long_mode(lv_poc_notationwindow_label_3, LV_LABEL_LONG_SROLL_CIRC);
		lv_label_set_text(lv_poc_notationwindow_label_3, "                    ");
		lv_label_set_align(lv_poc_notationwindow_label_3, LV_LABEL_ALIGN_CENTER);
	}
	if(text_1 != NULL && text_1[0] != 0)
	{
		lv_obj_set_hidden(lv_poc_notationwindow_label_1, false);
		memset(lv_poc_notationwindow_label_1_text, 0, LV_POC_NOTATIONWINDOW_LABEL_TEXT_MAX_SIZE * sizeof(int8_t));
		//strcpy((char *)lv_poc_notationwindow_label_1_text,(char *)"用户：");
		strcpy((char *)lv_poc_notationwindow_label_1_text, (char *)text_1);
	}
	else
	{
		lv_obj_set_hidden(lv_poc_notationwindow_label_1, true);
	}

	if(text_2 != NULL && text_2[0] != 0)
	{
		/*show text3*/
		lv_obj_set_hidden(lv_poc_notationwindow_label_3, false);
		strcpy((char *)lv_poc_notationwindow_label_3_text, (char *)"群组:");

		lv_obj_set_hidden(lv_poc_notationwindow_label_2, false);
		memset(lv_poc_notationwindow_label_2_text, 0, LV_POC_NOTATIONWINDOW_LABEL_TEXT_MAX_SIZE * sizeof(int8_t));
		strcpy((char *)lv_poc_notationwindow_label_2_text, (char *)text_2);
	}
	else
	{
		lv_obj_set_hidden(lv_poc_notationwindow_label_3, true);
		lv_obj_set_hidden(lv_poc_notationwindow_label_2, true);
	}

	lv_poc_notation_refresh();

	return lv_poc_notationwindow_obj;
}

lv_obj_t * lv_poc_notation_speaking(const int8_t * text_1, const int8_t * text_2)
{
	if(lv_poc_notationwindow_obj == NULL)
	{
		lv_poc_notation_create();
	}
	if(lv_poc_notationwindow_label_3 == NULL)
	{
		/*添加 (群组:) 固定框*/
		lv_poc_notationwindow_label_3 = lv_label_create(lv_poc_notationwindow_obj, NULL);
		lv_label_set_style(lv_poc_notationwindow_label_3,1,&lv_poc_notation_style);
		lv_label_set_long_mode(lv_poc_notationwindow_label_3, LV_LABEL_LONG_SROLL_CIRC);
		lv_label_set_text(lv_poc_notationwindow_label_3, "                    ");
		lv_label_set_align(lv_poc_notationwindow_label_3, LV_LABEL_ALIGN_CENTER);
	}
	if(text_1 != NULL && text_1[0] != 0)
	{
		lv_obj_set_hidden(lv_poc_notationwindow_label_1, false);
		strcpy((char *)lv_poc_notationwindow_label_1_text, (char *)text_1);
	}
	else
	{
		lv_obj_set_hidden(lv_poc_notationwindow_label_1, true);
		memset(lv_poc_notationwindow_label_1_text, 0, LV_POC_NOTATIONWINDOW_LABEL_TEXT_MAX_SIZE * sizeof(int8_t));
	}

	if(text_2 != NULL && text_2[0] != 0)
	{
		/*show text3*/
		lv_obj_set_hidden(lv_poc_notationwindow_label_3, false);
		strcpy((char *)lv_poc_notationwindow_label_3_text, (char *)"群组:");

		lv_obj_set_hidden(lv_poc_notationwindow_label_2, false);
		strcpy((char *)lv_poc_notationwindow_label_2_text, (char *)text_2);
	}
	else
	{
		lv_obj_set_hidden(lv_poc_notationwindow_label_3, true);
		memset(lv_poc_notationwindow_label_3_text, 0, LV_POC_NOTATIONWINDOW_LABEL_TEXT_MAX_SIZE * sizeof(int8_t));

		lv_obj_set_hidden(lv_poc_notationwindow_label_2, true);
		memset(lv_poc_notationwindow_label_2_text, 0, LV_POC_NOTATIONWINDOW_LABEL_TEXT_MAX_SIZE * sizeof(int8_t));
	}

	lv_poc_notation_refresh();

	return lv_poc_notationwindow_obj;
}

lv_obj_t * lv_poc_notation_normal_msg(const int8_t * text_1, const int8_t * text_2)
{
	if(lv_poc_notationwindow_obj == NULL)
	{
		lv_poc_notation_create();
	}
	else//防止界面重叠 若上次的窗口未消失就不弹出新窗口了
	{
		return NULL;
	}

	if(text_1 != NULL && text_1[0] != 0)
	{
		lv_obj_set_hidden(lv_poc_notationwindow_label_1, false);
		strcpy((char *)lv_poc_notationwindow_label_1_text, (char *)text_1);
	}
	else
	{
		lv_obj_set_hidden(lv_poc_notationwindow_label_1, true);
		memset(lv_poc_notationwindow_label_1_text, 0, LV_POC_NOTATIONWINDOW_LABEL_TEXT_MAX_SIZE * sizeof(int8_t));
	}

	if(text_2 != NULL && text_2[0] != 0)
	{
		lv_obj_set_hidden(lv_poc_notationwindow_label_2, false);
		strcpy((char *)lv_poc_notationwindow_label_2_text, (char *)text_2);
	}
	else
	{
		lv_obj_set_hidden(lv_poc_notationwindow_label_2, true);
		memset(lv_poc_notationwindow_label_2_text, 0, LV_POC_NOTATIONWINDOW_LABEL_TEXT_MAX_SIZE * sizeof(int8_t));
	}

	lv_poc_notation_refresh();

	return lv_poc_notationwindow_obj;
}

static void lv_poc_notation_normal_msg_delay_close_task(lv_task_t * task)
{
	OSI_LOGI(0, "[notation] close notation\n");

	lv_poc_notation_delay_close_task_running = true;
	lv_poc_notation_destory();
	lv_poc_notation_delay_close_task = NULL;
	lv_poc_notation_delay_close_task_running = false;
}

static void lv_poc_notation_task_cb(lv_task_t * task)
{

	if(lv_poc_notation_delay_close_task_running == true)
	{
		return;
	}

	lv_poc_notation_task_msg_t * notation_msg = &lv_poc_notation_task_queue[lv_poc_notation_task_queue_reader];
	lv_poc_notation_msg_type_t   msg_type = notation_msg->msg_type;
	if(msg_type == LV_POC_NOTATION_NONE)// && lv_poc_notationwindow_obj != NULL)
	{
		return;
	}
	notation_msg->msg_type = LV_POC_NOTATION_NONE;
	lv_poc_notation_task_queue_reader = (lv_poc_notation_task_queue_reader + 1) % LV_POC_NOTATION_TASK_QUEUE_SIZE;

	switch(msg_type)
	{
		case LV_POC_NOTATION_REFRESH:
		{
			lv_poc_notation_refresh();
			return;
		}

		case LV_POC_NOTATION_LISTENING:
		{
			lv_poc_notation_listen_speak_status = true;
			lv_poc_notation_listenning((const int8_t *)notation_msg->label_1_text,
				(const int8_t *)notation_msg->label_2_text);
			break;
		}

		case LV_POC_NOTATION_SPEAKING:
		{
			lv_poc_notation_listen_speak_status = true;
			lv_poc_notation_speaking((const int8_t *)notation_msg->label_1_text,
				(const int8_t *)notation_msg->label_2_text);

			break;
		}

		case LV_POC_NOTATION_NORMAL_MSG:
		{
			lv_poc_notation_listen_speak_status = false;
			lv_poc_notation_normal_msg((const int8_t *)notation_msg->label_1_text,
				(const int8_t *)notation_msg->label_2_text);

			if(lv_poc_notation_delay_close_task != NULL)
			{
				lv_task_del(lv_poc_notation_delay_close_task);
				lv_poc_notation_delay_close_task = NULL;
			}

			lv_poc_notation_delay_close_task = lv_task_create(lv_poc_notation_normal_msg_delay_close_task,
				2000,
				LV_TASK_PRIO_LOWEST,
				NULL);

			if(lv_poc_notation_delay_close_task != NULL)
			{
				lv_task_once(lv_poc_notation_delay_close_task);
			}
			return;
		}

		case LV_POC_NOTATION_LAUNCH_NOTE_MSG:
		{
			lv_poc_notation_listen_speak_status = false;
			lv_poc_notation_normal_msg((const int8_t *)notation_msg->label_1_text,
				(const int8_t *)notation_msg->label_2_text);

			if(lv_poc_notation_delay_close_task != NULL)
			{
				lv_task_del(lv_poc_notation_delay_close_task);
				lv_poc_notation_delay_close_task = NULL;
			}

			lv_poc_notation_delay_close_task = lv_task_create(lv_poc_notation_normal_msg_delay_close_task,
				4500,
				LV_TASK_PRIO_LOWEST,
				NULL);

			if(lv_poc_notation_delay_close_task != NULL)
			{
				lv_task_once(lv_poc_notation_delay_close_task);
			}
			return;
		}

		case LV_POC_NOTATION_ERROR_MSG:
		{
			lv_poc_notation_listen_speak_status = false;
			lv_poc_notation_normal_msg((const int8_t *)notation_msg->label_1_text,
				(const int8_t *)notation_msg->label_2_text);

			if(lv_poc_notation_delay_close_task != NULL)
			{
				lv_task_del(lv_poc_notation_delay_close_task);
				lv_poc_notation_delay_close_task = NULL;
			}

			lv_poc_notation_delay_close_task = lv_task_create(lv_poc_notation_normal_msg_delay_close_task,
				10000,
				LV_TASK_PRIO_LOWEST,
				NULL);

			if(lv_poc_notation_delay_close_task != NULL)
			{
				lv_task_once(lv_poc_notation_delay_close_task);
			}
			return;
		}

		case LV_POC_NOTATION_DESTORY:
		{
			lv_poc_notation_destory();
			return;
		}

		case LV_POC_NOTATION_HIDEN:
		{
			if(notation_msg->label_1_text[0] == 0)
			{
				lv_poc_notation_hide(false);
			}
			else
			{
				lv_poc_notation_hide(true);
			}
			return;
		}

		default:
			return;
	}

	if(lv_poc_notation_delay_close_task != NULL)
	{
		lv_task_del(lv_poc_notation_delay_close_task);
		lv_poc_notation_delay_close_task = NULL;
	}
}

bool lv_poc_notation_msg(lv_poc_notation_msg_type_t msg_type, const uint8_t *text_1, const uint8_t *text_2)
{
	if(lv_poc_notation_task == NULL)
	{
		lv_poc_notation_task = lv_task_create(lv_poc_notation_task_cb, 50, LV_TASK_PRIO_HIGHEST, NULL);
		if(lv_poc_notation_task == NULL)
		{
			return false;
		}

		memset(lv_poc_notation_task_queue, 0, sizeof(lv_poc_notation_task_msg_t) * LV_POC_NOTATION_TASK_QUEUE_SIZE);
	}

	lv_poc_notation_task_msg_t * dest = &lv_poc_notation_task_queue[lv_poc_notation_task_queue_writer];
	lv_poc_notation_task_queue_writer = (lv_poc_notation_task_queue_writer + 1) % LV_POC_NOTATION_TASK_QUEUE_SIZE;
	dest->msg_type = msg_type;
	dest->label_1_text[0] = 0;
	dest->label_2_text[0] = 0;
	if(text_1 != NULL)
	{
		strcpy((char *)dest->label_1_text, (const char *)text_1);
	}

	if(text_2 != NULL)
	{
		strcpy((char *)dest->label_2_text, (const char *)text_2);
	}

	return true;
}

static uint32_t lv_poc_txt_utf8_get_length(const char * txt)
{
    uint32_t len = 0;
    uint32_t i   = 0;

    while(txt[i] != '\0') {
        lv_txt_encoded_next(txt, &i);
        len++;
    }

    return len;
}

