#ifdef __cplusplus
extern "C" {
#endif
#include "lv_include/lv_poc.h"
#include "stdlib.h"

static lv_obj_t * activity_create(lv_poc_display_t *display);

static void activity_destory(lv_obj_t *obj);

static void lv_poc_list_config(lv_obj_t * list, lv_area_t list_area);

static lv_res_t signal_func(struct _lv_obj_t * obj, lv_signal_t sign, void * param);

static bool design_func(struct _lv_obj_t * obj, const lv_area_t * mask_p, lv_design_mode_t mode);

static lv_area_t display_area = {0};

static lv_obj_t * activity_list = NULL;

static osiMutex_t * mutex = NULL;

static lv_style_t lv_poc_terminal_style = {0};

static lv_style_t lv_poc_terminal_header_style = {0};

static lv_style_t lv_poc_terminal_btn_rel_style = {0};

static lv_style_t lv_poc_terminal_sb_style = {0};

lv_poc_activity_t * poc_cit_term_info_activity = NULL;

static lv_obj_t * activity_create(lv_poc_display_t *display)
{
    display_area.x1 = 0;
    display_area.x2 = lv_poc_get_display_width(display);
    display_area.y1 = 0;
    display_area.y2 = lv_poc_get_display_height(display);

    activity_list = lv_poc_list_create(display, NULL, display_area, lv_poc_list_config);
    lv_poc_notation_refresh();
    return (lv_obj_t *)activity_list;
}

static void activity_destory(lv_obj_t *obj)
{
	activity_list = NULL;
	poc_cit_term_info_activity = NULL;
}

static void lv_poc_list_config(lv_obj_t * list, lv_area_t list_area)
{
	lv_obj_t *win = lv_win_create(list, NULL);
	lv_win_set_title(win, "poc info");

	lv_obj_t *close_btn = lv_win_add_btn(win, LV_SYMBOL_CLOSE);
	lv_obj_set_event_cb(close_btn, lv_win_close_event_cb);
	lv_win_add_btn(win, LV_SYMBOL_SETTINGS);
	lv_win_set_btn_size(win, 24);
	lv_win_set_content_size(win, 160, 128);
	//backg
	memset(&lv_poc_terminal_style, 0, sizeof(lv_style_t));
	lv_style_copy(&lv_poc_terminal_style, &lv_style_pretty_color);
	lv_poc_terminal_style.text.color = LV_COLOR_BLACK;
	lv_poc_terminal_style.text.font = (lv_font_t *)(poc_setting_conf->font.activity_control_font);
	lv_poc_terminal_style.text.opa = 255;
	lv_poc_terminal_style.body.main_color = LV_COLOR_WHITE;
	lv_poc_terminal_style.body.grad_color = LV_COLOR_WHITE;
	lv_poc_terminal_style.body.opa = 255;
	lv_poc_terminal_style.body.radius = 0;
	lv_poc_terminal_style.body.border.part = LV_BORDER_NONE;
	lv_win_set_style(win, LV_WIN_STYLE_BG, &lv_poc_terminal_style);
	//head
	memset(&lv_poc_terminal_header_style, 0, sizeof(lv_style_t));
	lv_style_copy(&lv_poc_terminal_header_style, &lv_style_pretty_color);
	lv_poc_terminal_style.text.color = LV_COLOR_BLACK;
	lv_poc_terminal_style.text.font = (lv_font_t *)(poc_setting_conf->font.status_bar_time_font);;
	lv_poc_terminal_style.text.opa = 255;
	lv_poc_terminal_header_style.body.main_color = LV_COLOR_BLUE;
	lv_poc_terminal_header_style.body.grad_color = LV_COLOR_BLUE;
	lv_poc_terminal_header_style.body.opa = 255;
	lv_poc_terminal_header_style.body.radius = 0;
	lv_poc_terminal_header_style.body.border.part = LV_BORDER_NONE;
	lv_win_set_style(win, LV_WIN_STYLE_HEADER, &lv_poc_terminal_header_style);

	//btn rel
	memset(&lv_poc_terminal_btn_rel_style, 0, sizeof(lv_style_t));
	lv_style_copy(&lv_poc_terminal_btn_rel_style, &lv_style_pretty_color);
	lv_poc_terminal_btn_rel_style.body.main_color = LV_COLOR_MAKE(0x80, 0x00, 0x00);
	lv_poc_terminal_btn_rel_style.body.grad_color = LV_COLOR_MAKE(0x80, 0x00, 0x00);
	lv_poc_terminal_btn_rel_style.body.opa = 255;
	lv_poc_terminal_btn_rel_style.body.radius = 0;
	lv_poc_terminal_btn_rel_style.body.border.part = LV_BORDER_NONE;
	lv_win_set_style(win, LV_WIN_STYLE_BTN_REL, &lv_poc_terminal_btn_rel_style);

	//sb
	memset(&lv_poc_terminal_sb_style, 0, sizeof(lv_style_t));
	lv_style_copy(&lv_poc_terminal_sb_style, &lv_style_pretty_color);
	lv_poc_terminal_sb_style.body.main_color = LV_COLOR_BLUE;
	lv_poc_terminal_sb_style.body.grad_color = LV_COLOR_BLUE;
	lv_poc_terminal_sb_style.body.opa = 255;
	lv_poc_terminal_sb_style.body.radius = 0;
	lv_poc_terminal_sb_style.body.border.part = LV_BORDER_NONE;
	lv_win_set_style(win, LV_WIN_STYLE_SB, &lv_poc_terminal_sb_style);

	lv_obj_t *txt = lv_label_create(win, NULL);
	lv_label_set_text(txt, "RTOS : 10.0.1\n"
						   "Modem : 8910_MOD\n"
						   "ULE_V1_3_W20.35.2\n"
						   "SofVer : v1.0.2-02.02");
}

static lv_res_t signal_func(struct _lv_obj_t * obj, lv_signal_t sign, void * param)
{
	switch(sign)
	{
		case LV_SIGNAL_CONTROL:
		{
			if(param == NULL) return LV_RES_OK;
			unsigned int c = *(unsigned int *)param;
			switch(c)
			{
				case LV_GROUP_KEY_ENTER:
				{
					lv_poc_del_activity(poc_cit_term_info_activity);
					lvPocCitAutoTestCom_Msg(LV_POC_CIT_AUTO_TEST_TYPE_SUCCESS);
					break;
				}

				case LV_GROUP_KEY_DOWN:

				case LV_GROUP_KEY_UP:
				{
					lv_signal_send(activity_list, LV_SIGNAL_CONTROL, param);
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

				case LV_KEY_ESC:
				{
					lv_poc_del_activity(poc_cit_term_info_activity);
					lvPocCitAutoTestCom_Msg(LV_POC_CIT_AUTO_TEST_TYPE_FAILED);
					break;
				}
			}
			break;
		}

		case LV_SIGNAL_FOCUS:
		{
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

static bool design_func(struct _lv_obj_t * obj, const lv_area_t * mask_p, lv_design_mode_t mode)
{
	return true;
}


void lv_poc_terminfo_open(void)
{
    lv_poc_activity_ext_t activity_ext = {ACT_ID_POC_CIT_TERM_INFO,
											activity_create,
											activity_destory};
	if(poc_cit_term_info_activity != NULL)
	{
		return;
	}

	if(mutex == NULL)
	{
		mutex = osiMutexCreate();
	}

	mutex ? osiMutexLock(mutex) : 0;
    poc_cit_term_info_activity = lv_poc_create_activity(&activity_ext, true, false, NULL);
    lv_poc_activity_set_signal_cb(poc_cit_term_info_activity, signal_func);
    lv_poc_activity_set_design_cb(poc_cit_term_info_activity, design_func);
	mutex ? osiMutexUnlock(mutex) : 0;
}

#ifdef __cplusplus
}
#endif

