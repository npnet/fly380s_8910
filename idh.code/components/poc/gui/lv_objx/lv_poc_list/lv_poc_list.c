
#ifdef __cplusplus
extern "C" {
#endif
#include "lv_include/lv_poc.h"

/**********************
 *  STATIC VARIABLES
 **********************/

/*******************
*     NAME:   lv_poc_list_create
*   AUTHOR:   lugj
* DESCRIPT:   创建poc list
*     DATE:   2019-11-25
********************/
lv_obj_t * lv_poc_list_create(lv_obj_t * parent, lv_obj_t * copy,lv_area_t list_area, lv_poc_list_config_func_t func)
{
    poc_setting_conf = lv_poc_setting_conf_read();
    lv_style_t * style_list_scroll = ( lv_style_t * )poc_setting_conf->theme.current_theme->style_list_scroll;
    lv_style_t * style_list_page = ( lv_style_t * )poc_setting_conf->theme.current_theme->style_list_page;
    lv_style_t * style_list_btn_rel = ( lv_style_t * )poc_setting_conf->theme.current_theme->style_list_btn_rel;
    lv_style_t * style_list_btn_pr = ( lv_style_t * )poc_setting_conf->theme.current_theme->style_list_btn_pr;
    lv_style_t * style_list_btn_ina = ( lv_style_t * )poc_setting_conf->theme.current_theme->style_list_btn_ina;
    lv_obj_t * new_list = lv_list_create(parent, copy);
    lv_obj_t * scroll = lv_page_get_scrl(new_list);

    lv_list_set_style(new_list, LV_LIST_STYLE_SCRL, style_list_scroll);
    lv_list_set_style(new_list, LV_LIST_STYLE_BG, style_list_page);
    style_list_btn_rel->text.font = (lv_font_t *)poc_setting_conf->font.list_btn_current_font;
    lv_list_set_style(new_list, LV_LIST_STYLE_BTN_REL, style_list_btn_rel);
    lv_list_set_style(new_list, LV_LIST_STYLE_BTN_TGL_REL, style_list_btn_rel);
    style_list_btn_pr->text.font = (lv_font_t *)poc_setting_conf->font.list_btn_current_font;
    lv_list_set_style(new_list, LV_LIST_STYLE_BTN_PR, style_list_btn_pr);
    lv_list_set_style(new_list, LV_LIST_STYLE_BTN_TGL_PR, style_list_btn_pr);
    style_list_btn_ina->text.font = (lv_font_t *)poc_setting_conf->font.list_btn_current_font;
    lv_list_set_style(new_list, LV_LIST_STYLE_BTN_INA, style_list_btn_ina);

    //lv_obj_set_pos(new_list, list_area.x1, list_area.y1);
    lv_obj_align(new_list, parent, LV_ALIGN_IN_TOP_LEFT, 0, list_area.y1);
    lv_obj_set_size(new_list, list_area.x2 - list_area.x1, list_area.y2 - list_area.y1);
    lv_obj_set_size(scroll, list_area.x2 - list_area.x1, list_area.y2 - list_area.y1);
    lv_list_set_sb_mode(new_list, LV_SB_MODE_OFF);
    //lv_list_set_single_mode(new_list, true);

    if(NULL != func)
    {
        func(new_list, list_area);
    }

    return new_list;
}



#ifdef __cplusplus
}
#endif

