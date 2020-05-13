
/*************************************************
*
*                  Include
*
*************************************************/
#include "lv_include/lv_poc.h"
#include "stdlib.h"

#define LV_POC_NOTATIONWINDOW_LABEL_TEXT_MAX_SIZE 50

static lv_obj_t * lv_poc_notationwindow_obj = NULL;
static lv_obj_t * lv_poc_notationwindow_label_1 = NULL;
static lv_obj_t * lv_poc_notationwindow_label_2 = NULL;
static int8_t lv_poc_notationwindow_label_1_text[LV_POC_NOTATIONWINDOW_LABEL_TEXT_MAX_SIZE] = {0};
static int8_t lv_poc_notationwindow_label_2_text[LV_POC_NOTATIONWINDOW_LABEL_TEXT_MAX_SIZE] = {0};

static lv_style_t lv_poc_notation_style = {0};



lv_obj_t * lv_poc_notation_create(void)
{
	if(lv_poc_notationwindow_obj != NULL)
	{
		return lv_poc_notationwindow_obj;
	}
	memset(&lv_poc_notation_style, 0, sizeof(lv_style_t));
    lv_style_copy(&lv_poc_notation_style, &lv_style_pretty_color);
    lv_poc_notation_style.text.color = LV_COLOR_BLACK;
    lv_poc_notation_style.text.font = &lv_font_dejavu_20;
    lv_poc_notation_style.text.opa = 255;
    lv_poc_notation_style.body.main_color = LV_COLOR_BLUE;
    lv_poc_notation_style.body.grad_color = LV_COLOR_BLUE;
    lv_poc_notation_style.body.opa = 200;

	lv_poc_notationwindow_obj = lv_obj_create(lv_scr_act(), NULL);
    lv_obj_set_style(lv_poc_notationwindow_obj,&lv_poc_notation_style);
    lv_obj_set_size(lv_poc_notationwindow_obj, LV_HOR_RES * 27/ 40, LV_VER_RES / 2);
    lv_obj_set_pos(lv_poc_notationwindow_obj, LV_HOR_RES * 13/ 80, LV_VER_RES / 4);

	lv_poc_notationwindow_label_1 = lv_label_create(lv_poc_notationwindow_obj, NULL);
	lv_obj_set_size(lv_poc_notationwindow_label_1, lv_obj_get_width(lv_poc_notationwindow_obj) - 4, lv_obj_get_height(lv_poc_notationwindow_obj) / 3);
    lv_label_set_style(lv_poc_notationwindow_label_1, 1,&lv_poc_notation_style);
    lv_label_set_long_mode(lv_poc_notationwindow_label_1, LV_LABEL_LONG_SROLL);
    lv_label_set_text(lv_poc_notationwindow_label_1, "                    ");
    lv_label_set_align(lv_poc_notationwindow_label_1, LV_LABEL_ALIGN_LEFT);
    //lv_obj_align(lv_poc_notationwindow_label_1, lv_poc_notationwindow_obj, LV_ALIGN_IN_TOP_LEFT, 0, 0);

	lv_poc_notationwindow_label_2 = lv_label_create(lv_poc_notationwindow_obj, NULL);
	lv_obj_set_size(lv_poc_notationwindow_label_2, lv_obj_get_width(lv_poc_notationwindow_obj) - 4, lv_obj_get_height(lv_poc_notationwindow_obj) / 3);
    lv_label_set_style(lv_poc_notationwindow_label_2,1,&lv_poc_notation_style);
    lv_label_set_long_mode(lv_poc_notationwindow_label_2, LV_LABEL_LONG_SROLL);
    lv_label_set_text(lv_poc_notationwindow_label_2, "                    ");
    lv_label_set_align(lv_poc_notationwindow_label_2, LV_LABEL_ALIGN_LEFT);
    //lv_obj_align(lv_poc_notationwindow_label_2, lv_poc_notationwindow_label_1, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);

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
	memset(lv_poc_notationwindow_label_1_text,
			0,
			LV_POC_NOTATIONWINDOW_LABEL_TEXT_MAX_SIZE * sizeof(int8_t));
	memset(lv_poc_notationwindow_label_1_text,
			0,
			LV_POC_NOTATIONWINDOW_LABEL_TEXT_MAX_SIZE * sizeof(int8_t));
}

void lv_poc_notation_refresh(void)
{
	if(lv_poc_notationwindow_obj == NULL)
	{
		return;
	}

	if(lv_poc_notationwindow_obj->hidden == true)
	{
		return;
	}

	lv_obj_set_parent(lv_poc_notationwindow_obj, lv_scr_act());
    lv_obj_set_pos(lv_poc_notationwindow_obj, LV_HOR_RES * 13/ 80, LV_VER_RES / 4);

    if(lv_poc_notationwindow_label_1->hidden == true)
    {
	    if(lv_poc_notationwindow_label_2->hidden == false)
	    {
    		lv_label_set_text(lv_poc_notationwindow_label_2,(char *) lv_poc_notationwindow_label_2_text);
			lv_obj_set_size(lv_poc_notationwindow_label_2,
							lv_obj_get_width(lv_poc_notationwindow_obj) - 4,
							lv_obj_get_height(lv_poc_notationwindow_obj) / 3);
    		lv_obj_set_pos(lv_poc_notationwindow_label_2, 2,
    						lv_obj_get_height(lv_poc_notationwindow_obj) / 2
    							- lv_obj_get_height(lv_poc_notationwindow_label_2) / 2);
		}
    }
    else
    {
    	if(lv_poc_notationwindow_label_2->hidden == true)
    	{
		    lv_label_set_text(lv_poc_notationwindow_label_1, (char *)lv_poc_notationwindow_label_1_text);
			lv_obj_set_size(lv_poc_notationwindow_label_1,
							lv_obj_get_width(lv_poc_notationwindow_obj) - 4,
							lv_obj_get_height(lv_poc_notationwindow_obj) / 3);
    		lv_obj_set_pos(lv_poc_notationwindow_label_1, 2,
    						lv_obj_get_height(lv_poc_notationwindow_obj) / 2
    							- lv_obj_get_height(lv_poc_notationwindow_label_1) / 2);
    	}
    	else
    	{
		    lv_label_set_text(lv_poc_notationwindow_label_1, (char *)lv_poc_notationwindow_label_1_text);
			lv_obj_set_size(lv_poc_notationwindow_label_1,
							lv_obj_get_width(lv_poc_notationwindow_obj) - 4,
							lv_obj_get_height(lv_poc_notationwindow_obj) / 3);
    		lv_obj_set_pos(lv_poc_notationwindow_label_1, 2,
    						lv_obj_get_height(lv_poc_notationwindow_obj) / 2
    							- lv_obj_get_height(lv_poc_notationwindow_label_1) - 4);

    		lv_label_set_text(lv_poc_notationwindow_label_2, (char *)lv_poc_notationwindow_label_2_text);
			lv_obj_set_size(lv_poc_notationwindow_label_2,
							lv_obj_get_width(lv_poc_notationwindow_obj) - 4,
							lv_obj_get_height(lv_poc_notationwindow_obj) / 3);
    		lv_obj_set_pos(lv_poc_notationwindow_label_2, 2,
    						lv_obj_get_height(lv_poc_notationwindow_obj) / 2 + 4);
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

	if(text_1 != NULL)
	{
		lv_obj_set_hidden(lv_poc_notationwindow_label_1, false);
		memset(lv_poc_notationwindow_label_1_text, 0, LV_POC_NOTATIONWINDOW_LABEL_TEXT_MAX_SIZE * sizeof(int8_t));
		strcpy((char *)lv_poc_notationwindow_label_1_text,(char *)"用户：");
		strcat((char *)lv_poc_notationwindow_label_1_text, (char *)text_1);
	}
	else
	{
		lv_obj_set_hidden(lv_poc_notationwindow_label_1, true);
	}

	if(text_2 != NULL)
	{
		lv_obj_set_hidden(lv_poc_notationwindow_label_2, false);
		memset(lv_poc_notationwindow_label_2_text, 0, LV_POC_NOTATIONWINDOW_LABEL_TEXT_MAX_SIZE * sizeof(int8_t));
		strcpy((char *)lv_poc_notationwindow_label_2_text,(char *)"群组：");
		strcat((char *)lv_poc_notationwindow_label_2_text, (char *)text_2);
	}
	else
	{
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

	if(text_1 != NULL)
	{
		lv_obj_set_hidden(lv_poc_notationwindow_label_1, false);
		memset(lv_poc_notationwindow_label_1_text, 0, LV_POC_NOTATIONWINDOW_LABEL_TEXT_MAX_SIZE * sizeof(int8_t));
		strcpy((char *)lv_poc_notationwindow_label_1_text, (char *)text_1);
	}
	else
	{
		lv_obj_set_hidden(lv_poc_notationwindow_label_1, true);
	}

	if(text_2 != NULL)
	{
		lv_obj_set_hidden(lv_poc_notationwindow_label_2, false);
		memset(lv_poc_notationwindow_label_2_text, 0, LV_POC_NOTATIONWINDOW_LABEL_TEXT_MAX_SIZE * sizeof(int8_t));
		strcpy((char *)lv_poc_notationwindow_label_2_text, (char *)text_1);
	}
	else
	{
		lv_obj_set_hidden(lv_poc_notationwindow_label_2, true);
	}
	lv_poc_notation_refresh();

	return lv_poc_notationwindow_obj;
}

bool lv_poc_notation_msg(int msg_type, const uint8_t *text_1, const uint8_t *text_2)
{
	switch(msg_type)
	{
		case 1:
		{
			lv_poc_notation_listenning(text_1, text_2);
			break;
		}

		case 2:
		{
			lv_poc_notation_speaking(text_1, text_2);
			break;
		}

		case 0:
		{
			lv_poc_notation_destory();
			break;
		}

		case 3:
		{
			lv_poc_notation_refresh();
			break;
		}

		default:
			return false;
	}
	return true;
}


