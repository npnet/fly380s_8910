#ifdef __cplusplus
extern "C" {
#endif
#include "lv_include/lv_poc.h"
#include "stdlib.h"

#define POCCITPARTTESTLISTITEMMAX 18

typedef void(* lv_poc_cit_part_test_btn_func_t)(lv_obj_t *obj);

static lv_poc_cit_part_test_btn_func_t lv_poc_cit_part_test_btn_func_items[POCCITPARTTESTLISTITEMMAX] = {0};

static lv_obj_t * activity_create(lv_poc_display_t *display);

static void activity_destory(lv_obj_t *obj);

static void * cit_part_test_list_create(lv_obj_t * parent, lv_area_t display_area);

static void cit_part_test_list_config(lv_obj_t * list, lv_area_t list_area);

static lv_res_t signal_func(struct _lv_obj_t * obj, lv_signal_t sign, void * param);

static bool design_func(struct _lv_obj_t * obj, const lv_area_t * mask_p, lv_design_mode_t mode);

static lv_poc_win_t * cit_part_test_win = NULL;

static lv_obj_t * activity_list = NULL;

static osiMutex_t * mutex = NULL;

static lv_poc_cit_test_type_t cit_info = {
	.id = LV_POC_CIT_OPRATOR_TYPE_START,
	.name[0] = 0,

	.cit_calib_attr.valid = false,
	.cit_calib_attr.cb = lv_poc_get_calib_status,

	.cit_rtc_attr.valid = false,
	.cit_rtc_attr.cb = lv_poc_get_time,

	.cit_backlight_attr.valid = false,
	.cit_backlight_attr.cb = poc_set_lcd_status,

	.cit_volum_attr.valid = false,
	.cit_volum_attr.cb = lv_poc_type_volum_cb,

	.cit_mic_attr.valid = false,
	.cit_mic_attr.cb = lv_poc_set_loopback_recordplay,

	.cit_signal_attr.valid = false,
	.cit_signal_attr.cb = poc_get_signal_dBm,

	.cit_key_attr.valid = false,
	.cit_key_attr.keynumber = LV_KEY_NUMBER,
	.cit_key_attr.validnumber = 0,
	.cit_key_attr.key_attr[0] = {.keyindex = 0, .keywidth = LV_KEY_AVERAGE_WIDTH, .keyheight = LV_KEY_AVERAGE_WIDTH, .checkstatus = false, .lv_key = LV_GROUP_KEY_UP, .lv_poc_cit_key_cb = lv_poc_type_key_group_cb},
	.cit_key_attr.key_attr[1] = {.keyindex = 1, .keywidth = LV_KEY_AVERAGE_WIDTH, .keyheight = LV_KEY_AVERAGE_WIDTH, .checkstatus = false, .lv_key = LV_GROUP_KEY_DOWN, .lv_poc_cit_key_cb = lv_poc_type_key_up_cb},
	.cit_key_attr.key_attr[2] = {.keyindex = 2, .keywidth = LV_KEY_AVERAGE_WIDTH, .keyheight = LV_KEY_AVERAGE_WIDTH, .checkstatus = false, .lv_key = LV_GROUP_KEY_VOL_UP, .lv_poc_cit_key_cb = lv_poc_type_key_down_cb},
	.cit_key_attr.key_attr[3] = {.keyindex = 3, .keywidth = LV_KEY_AVERAGE_WIDTH, .keyheight = LV_KEY_AVERAGE_WIDTH, .checkstatus = false, .lv_key = LV_GROUP_KEY_VOL_DOWN, .lv_poc_cit_key_cb = lv_poc_type_key_member_cb},
	.cit_key_attr.key_attr[4] = {.keyindex = 4, .keywidth = LV_KEY_AVERAGE_WIDTH, .keyheight = LV_KEY_AVERAGE_WIDTH, .checkstatus = false, .lv_key = LV_GROUP_KEY_POC, .lv_poc_cit_key_cb = lv_poc_type_key_poc_cb},
	.cit_key_attr.key_attr[5] = {.keyindex = 5, .keywidth = LV_KEY_AVERAGE_WIDTH, .keyheight = LV_KEY_AVERAGE_WIDTH, .checkstatus = false, .lv_key = LV_GROUP_KEY_POWER, .lv_poc_cit_key_cb = lv_poc_type_key_volum_up_cb},
	.cit_key_attr.key_attr[6] = {.keyindex = 6, .keywidth = LV_KEY_AVERAGE_WIDTH, .keyheight = LV_KEY_AVERAGE_WIDTH, .checkstatus = false, .lv_key = LV_GROUP_KEY_VOL_UP, .lv_poc_cit_key_cb = lv_poc_type_key_volum_down_cb},
	.cit_key_attr.key_attr[7] = {.keyindex = 7, .keywidth = LV_KEY_AVERAGE_WIDTH, .keyheight = LV_KEY_AVERAGE_WIDTH, .checkstatus = false, .lv_key = LV_GROUP_KEY_VOL_DOWN, .lv_poc_cit_key_cb = lv_poc_type_key_power_cb},
	.cit_key_attr.key_attr[8] = {.keyindex = 8, .keywidth = LV_KEY_AVERAGE_WIDTH, .keyheight = LV_KEY_AVERAGE_WIDTH, .checkstatus = false, .lv_key = LV_GROUP_KEY_POC, .lv_poc_cit_key_cb = lv_poc_type_key_enter_cb},
	.cit_key_attr.key_attr[9] = {.keyindex = 9, .keywidth = LV_KEY_AVERAGE_WIDTH, .keyheight = LV_KEY_AVERAGE_WIDTH, .checkstatus = false, .lv_key = LV_GROUP_KEY_POWER, .lv_poc_cit_key_cb = lv_poc_type_key_escape_cb},
	.cit_key_attr.cb = lv_poc_key_param_init_cb,

	.cit_charge_attr.valid = false,
	.cit_charge_attr.cb = poc_battery_get_status,

	.cit_gps_attr.valid = false,
	.cit_gps_attr.cb = lv_poc_type_gps_cb,
#ifdef CONFIG_POC_GUI_TOUCH_SUPPORT
	.cit_touch_attr.valid = false,
	.cit_touch_attr.cb = poc_set_torch_status,
#endif
	.cit_rgb_attr.valid = false,
	.cit_rgb_attr.cb = lv_poc_type_rgb_cb,

	.cit_headset_attr.valid = false,
	.cit_headset_attr.cb = lv_poc_set_loopback_recordplay,

	.cit_flash_attr.valid = false,
	.cit_flash_attr.cb = lv_poc_type_flash_cb,
};

lv_poc_activity_t * poc_cit_part_test_activity = NULL;

static void lv_poc_system_version_test_cb(lv_obj_t * obj);
static void lv_poc_calibration_test_cb(lv_obj_t * obj);
static void lv_poc_rtc_test_cb(lv_obj_t * obj);
static void lv_poc_backlight_test_cb(lv_obj_t * obj);
static void lv_poc_lcd_test_cb(lv_obj_t * obj);
static void lv_poc_volum_test_cb(lv_obj_t * obj);
static void lv_poc_micspk_test_cb(lv_obj_t * obj);
static void lv_poc_signal_test_cb(lv_obj_t * obj);
static void lv_poc_key_test_cb(lv_obj_t * obj);
static void lv_poc_charge_test_cb(lv_obj_t * obj);
static void lv_poc_gps_test_cb(lv_obj_t * obj);
static void lv_poc_sim_test_cb(lv_obj_t * obj);
#ifdef CONFIG_POC_GUI_TOUCH_SUPPORT
static void lv_poc_touch_test_cb(lv_obj_t * obj);
#endif
static void lv_poc_rgb_test_cb(lv_obj_t * obj);
static void lv_poc_headset_test_cb(lv_obj_t * obj);
static void lv_poc_flash_test_cb(lv_obj_t * obj);

static lv_obj_t * activity_create(lv_poc_display_t *display)
{
    cit_part_test_win = lv_poc_win_create(display, "部件测试", cit_part_test_list_create);
    return (lv_obj_t *)cit_part_test_win;
}

static void activity_destory(lv_obj_t *obj)
{
	poc_cit_part_test_activity = NULL;
}

static void * cit_part_test_list_create(lv_obj_t * parent, lv_area_t display_area)
{
    activity_list = lv_poc_list_create(parent, NULL, display_area, cit_part_test_list_config);
    lv_poc_notation_refresh();
    return (void *)activity_list;
}

typedef struct
{
    lv_img_dsc_t * ic;
    char *title;
    lv_label_long_mode_t title_long_mode;
    lv_label_align_t title_text_align;
    lv_align_t title_align;
    char *content;
    lv_label_long_mode_t content_long_mode;
    lv_label_align_t content_text_align;
    lv_align_t content_align;
    lv_coord_t content_align_x;
    lv_coord_t content_align_y;
	lv_poc_cit_part_test_btn_func_t func;
} lv_poc_cit_part_test_label_struct_t;

lv_poc_cit_part_test_label_struct_t lv_poc_cit_part_test_label_array[] = {
    {
        NULL,
        "项1"                     , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
        "系统版本测试"                 , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
		lv_poc_system_version_test_cb,
    },

    {
        NULL,
        "项2"                   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
        "校准测试"				   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
        lv_poc_calibration_test_cb,
    },

	{
		NULL,
		"项3"				   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
		"RTC测试"				   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
		lv_poc_rtc_test_cb,
	},

	{
		NULL,
		"项4"				   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
		"背光灯测试"				   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
		lv_poc_backlight_test_cb,
	},

	{
		NULL,
		"项5"				   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
		"LCD测试"				   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
		lv_poc_lcd_test_cb,
	},

	{
		NULL,
		"项6"				   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
		"音量测试"				   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
		lv_poc_volum_test_cb,
	},

	{
		NULL,
		"项7"				   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
		"麦克风测试"				   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
		lv_poc_micspk_test_cb,
	},

	{
		NULL,
		"项8"				   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
		"信号测试"				   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
		lv_poc_signal_test_cb,
	},

	{
		NULL,
		"项9"				   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
		"按键测试"				   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
		lv_poc_key_test_cb,
	},

	{
		NULL,
		"项10"				   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
		"充电测试"				   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
		lv_poc_charge_test_cb,
	},

	{
		NULL,
		"项11"				   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
		"GPS测试"				   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
		lv_poc_gps_test_cb,
	},

	{
		NULL,
		"项12"				   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
		"SIM卡测试"				   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
		lv_poc_sim_test_cb,
	},

#ifdef CONFIG_POC_GUI_TOUCH_SUPPORT
	{
		NULL,
		"项13"				   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
		"手电筒测试"				   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
		lv_poc_touch_test_cb,
	},
#endif

	{
		NULL,
		"项14"				   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
		"三色灯测试"				   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
		lv_poc_rgb_test_cb,
	},

	{
		NULL,
		"项15"				   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
		"耳机测试"				   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
		lv_poc_headset_test_cb,
	},

	{
		NULL,
		"项16"				   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
		"Flash测试"			   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
		lv_poc_flash_test_cb,
	},
};

static void lv_poc_cit_config_init(void)
{
	cit_info.id = LV_POC_CIT_OPRATOR_TYPE_START;
	cit_info.name[0] = 0;
	cit_info.cit_key_attr.keynumber = 0;
	cit_info.cit_key_attr.valid = false;
}

static void lv_poc_system_version_test_cb(lv_obj_t * obj)
{
	lv_poc_terminfo_open();
}

static void lv_poc_calibration_test_cb(lv_obj_t * obj)
{
	lv_poc_cit_config_init();
	cit_info.id = LV_POC_CIT_OPRATOR_TYPE_CALIBATE;
	cit_info.cit_calib_attr.valid = true;
	strcpy(cit_info.name, "校准测试");
	lv_poc_test_ui_open((void *)&cit_info);
}

static void lv_poc_rtc_test_cb(lv_obj_t * obj)
{
	lv_poc_cit_config_init();
	cit_info.id = LV_POC_CIT_OPRATOR_TYPE_RTC;
	cit_info.cit_rtc_attr.valid = true;
	strcpy(cit_info.name, "RTC测试");
	lv_poc_test_ui_open((void *)&cit_info);
}

static void lv_poc_backlight_test_cb(lv_obj_t * obj)
{
	lv_poc_cit_config_init();
	cit_info.id = LV_POC_CIT_OPRATOR_TYPE_BACKLIGHT;
	cit_info.cit_backlight_attr.valid = true;
	strcpy(cit_info.name, "背光灯测试");
	lv_poc_test_ui_open((void *)&cit_info);
}

static void lv_poc_lcd_test_cb(lv_obj_t * obj)
{
	lv_poc_cit_config_init();
	cit_info.id = LV_POC_CIT_OPRATOR_TYPE_LCD;
	strcpy(cit_info.name, "LCD测试");
	lv_poc_test_ui_open((void *)&cit_info);
}

static void lv_poc_volum_test_cb(lv_obj_t * obj)
{
	lv_poc_cit_config_init();
	cit_info.id = LV_POC_CIT_OPRATOR_TYPE_VOLUM;
	cit_info.cit_volum_attr.valid = true;
	strcpy(cit_info.name, "音量测试");
	lv_poc_test_ui_open((void *)&cit_info);
}

static void lv_poc_micspk_test_cb(lv_obj_t * obj)
{
	lv_poc_cit_config_init();
	cit_info.id = LV_POC_CIT_OPRATOR_TYPE_MIC;
	cit_info.cit_mic_attr.valid = true;
	strcpy(cit_info.name, "麦克风测试");
	lv_poc_test_ui_open((void *)&cit_info);
}

static void lv_poc_signal_test_cb(lv_obj_t * obj)
{
	lv_poc_cit_config_init();
	cit_info.id = LV_POC_CIT_OPRATOR_TYPE_SIGNAL;
	cit_info.cit_signal_attr.valid = true;
	strcpy(cit_info.name, "信号测试");
	lv_poc_test_ui_open((void *)&cit_info);
}

static void lv_poc_key_test_cb(lv_obj_t * obj)
{
	lv_poc_cit_config_init();
	cit_info.id = LV_POC_CIT_OPRATOR_TYPE_KEY;
	cit_info.cit_key_attr.valid = true;
	cit_info.cit_key_attr.keynumber = LV_KEY_NUMBER;
	cit_info.cit_key_attr.validnumber = 0;
	for(int i = 0; i < cit_info.cit_key_attr.keynumber; i++)
	{
		cit_info.cit_key_attr.key_attr[i].checkstatus = false;
	}
	strcpy(cit_info.name, "按键测试");
	lv_poc_test_ui_open((void *)&cit_info);
}

static void lv_poc_charge_test_cb(lv_obj_t * obj)
{
	lv_poc_cit_config_init();
	cit_info.id = LV_POC_CIT_OPRATOR_TYPE_CHARGE;
	cit_info.cit_charge_attr.valid = true;
	strcpy(cit_info.name, "充电测试");
	lv_poc_test_ui_open((void *)&cit_info);
}

static void lv_poc_gps_test_cb(lv_obj_t * obj)
{
	lv_poc_cit_config_init();
	cit_info.id = LV_POC_CIT_OPRATOR_TYPE_GPS;
	cit_info.cit_gps_attr.valid = true;
	strcpy(cit_info.name, "GPS测试");
	lv_poc_test_ui_open((void *)&cit_info);
}

static void lv_poc_sim_test_cb(lv_obj_t * obj)
{
	lv_poc_cit_config_init();
	cit_info.id = LV_POC_CIT_OPRATOR_TYPE_SIM;
	strcpy(cit_info.name, "SIM卡测试");
	lv_poc_test_ui_open((void *)&cit_info);
}

#ifdef CONFIG_POC_GUI_TOUCH_SUPPORT
static void lv_poc_touch_test_cb(lv_obj_t * obj)
{
	lv_poc_cit_config_init();
	cit_info.id = LV_POC_CIT_OPRATOR_TYPE_TOUCH;
	cit_info.cit_touch_attr.valid = true;
	strcpy(cit_info.name, "手电筒测试");
	lv_poc_test_ui_open((void *)&cit_info);
}
#endif

static void lv_poc_rgb_test_cb(lv_obj_t * obj)
{
	lv_poc_cit_config_init();
	cit_info.id = LV_POC_CIT_OPRATOR_TYPE_RGB;
	cit_info.cit_rgb_attr.valid = true;
	strcpy(cit_info.name, "三色灯测试");
	lv_poc_test_ui_open((void *)&cit_info);
}

static void lv_poc_headset_test_cb(lv_obj_t * obj)
{
	lv_poc_cit_config_init();
	cit_info.id = LV_POC_CIT_OPRATOR_TYPE_HEADSET;
	cit_info.cit_headset_attr.valid = true;
	strcpy(cit_info.name, "耳机测试");
	lv_poc_test_ui_open((void *)&cit_info);
}

static void lv_poc_flash_test_cb(lv_obj_t * obj)
{
	lv_poc_cit_config_init();
	cit_info.id = LV_POC_CIT_OPRATOR_TYPE_FLASH;
	cit_info.cit_flash_attr.valid = true;
	strcpy(cit_info.name, "Flash测试");
	lv_poc_test_ui_open((void *)&cit_info);
}

static void lv_poc_cit_part_test_pressed_cb(lv_obj_t * obj, lv_event_t event)
{
    if(LV_EVENT_CLICKED == event || LV_EVENT_PRESSED == event)
    {
		int index = lv_list_get_btn_index((lv_obj_t *)cit_part_test_win->display_obj, obj);

		lv_poc_cit_part_test_btn_func_t func = lv_poc_cit_part_test_btn_func_items[index];
		func(obj);
    }
}

static void cit_part_test_list_config(lv_obj_t * list, lv_area_t list_area)
{
    lv_obj_t *btn = NULL;
    lv_obj_t *label = NULL;
    lv_obj_t *btn_label = NULL;
    lv_coord_t btn_height = (list_area.y2 - list_area.y1)/LV_POC_LIST_COLUM_COUNT;
    lv_coord_t btn_width = (list_area.x2 - list_area.x1);
    lv_style_t * style_label = NULL;
    poc_setting_conf = lv_poc_setting_conf_read();
    style_label = ( lv_style_t * )poc_setting_conf->theme.current_theme->style_fota_label;//no use dead(style_cit_label)
    style_label->text.font = (lv_font_t *)poc_setting_conf->font.cit_label_current_font;

    int label_array_size = sizeof(lv_poc_cit_part_test_label_array)/sizeof(lv_poc_cit_part_test_label_struct_t);
    lv_obj_t ** btn_array = (lv_obj_t **)lv_mem_alloc(sizeof(lv_obj_t *) * label_array_size);

    for(int i = 0; i < label_array_size; i++)
    {
        btn = lv_list_add_btn(list, NULL, lv_poc_cit_part_test_label_array[i].title);
        btn_array[i] = btn;
        lv_btn_set_fit(btn, LV_FIT_NONE);
        lv_obj_set_height(btn, btn_height);
        btn_label = lv_list_get_btn_label(btn);
        label = lv_label_create(btn, NULL);
        btn->user_data = (void *)label;

        lv_label_set_text(label, lv_poc_cit_part_test_label_array[i].content);

        lv_label_set_long_mode(btn_label, lv_poc_cit_part_test_label_array[i].content_long_mode);
        lv_label_set_align(btn_label, lv_poc_cit_part_test_label_array[i].content_text_align);
        lv_label_set_long_mode(label, lv_poc_cit_part_test_label_array[i].content_long_mode);
        lv_label_set_align(label, lv_poc_cit_part_test_label_array[i].content_text_align);

        lv_label_set_style(label, LV_LABEL_STYLE_MAIN, style_label);

        lv_obj_set_width(btn_label, btn_width/4);
        lv_obj_set_width(label, btn_width - lv_obj_get_width(btn_label));
        lv_obj_align(btn_label, btn, lv_poc_cit_part_test_label_array[i].title_align, lv_poc_cit_part_test_label_array[i].content_align_x, lv_poc_cit_part_test_label_array[i].content_align_y);
        lv_obj_align(label, btn_label, lv_poc_cit_part_test_label_array[i].content_align, lv_poc_cit_part_test_label_array[i].content_align_x, lv_poc_cit_part_test_label_array[i].content_align_y);
		lv_obj_set_click(btn, true);
		lv_obj_set_event_cb(btn, lv_poc_cit_part_test_pressed_cb);
		lv_poc_cit_part_test_btn_func_items[i] = lv_poc_cit_part_test_label_array[i].func;
	}
    lv_list_set_btn_selected(list, btn_array[0]);
	//free
	if(btn_array != NULL)
	{
		lv_mem_free(btn_array);
		btn_array = NULL;
	}
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
					lv_signal_send(activity_list, LV_SIGNAL_PRESSED, NULL);
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
					lv_poc_del_activity(poc_cit_part_test_activity);
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


void lv_poc_cit_part_test_open(void)
{
    lv_poc_activity_ext_t activity_ext = {ACT_ID_POC_CIT_MAIN,
											activity_create,
											activity_destory};
	lv_poc_control_text_t control = {"确定", "", "取消"};
	if(poc_cit_part_test_activity != NULL)
	{
		return;
	}

	if(mutex == NULL)
	{
		mutex = osiMutexCreate();
	}

	mutex ? osiMutexLock(mutex) : 0;
    poc_cit_part_test_activity = lv_poc_create_activity(&activity_ext, true, true, &control);
    lv_poc_activity_set_signal_cb(poc_cit_part_test_activity, signal_func);
    lv_poc_activity_set_design_cb(poc_cit_part_test_activity, design_func);
	mutex ? osiMutexUnlock(mutex) : 0;
}

bool lv_poc_cit_get_key_valid(void)
{
	return cit_info.cit_key_attr.valid;
}

int lv_poc_cit_get_run_status(void)
{
	return cit_info.id;
}

void lv_poc_cit_auto_test_items_register_cb(bool status)
{
    int label_array_size = sizeof(lv_poc_cit_part_test_label_array)/sizeof(lv_poc_cit_part_test_label_struct_t);
	for(int i = 0; i < label_array_size; i++)
    {
		if(status == true)
		{
			lv_poc_cit_part_test_btn_func_items[i] = lv_poc_cit_part_test_label_array[i].func;
		}
		else
		{
			lv_poc_cit_part_test_btn_func_items[i] = NULL;
		}
	}
}

void lv_poc_cit_auto_test_items_perform(lv_poc_cit_test_ui_id id)
{
	lv_poc_cit_part_test_btn_func_t func = lv_poc_cit_part_test_btn_func_items[id];
	func(NULL);
}

#ifdef __cplusplus
}
#endif

