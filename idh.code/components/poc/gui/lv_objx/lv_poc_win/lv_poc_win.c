
#ifdef __cplusplus
extern "C" {
#endif
#include "lv_include/lv_poc.h"

/**********************
 *  STATIC VARIABLES
 **********************/

static void * empty_win(lv_obj_t * parent, lv_area_t display_area)
{
    lv_obj_t * display = lv_obj_create(parent, NULL);
    if(NULL == display)
    {
        return NULL;
    }
    lv_obj_set_pos(display, display_area.x1, display_area.y1);
    lv_obj_set_size(display, display_area.x2 - display_area.x1, display_area.y2 - display_area.y1);
    return (void *)display;
}


/*******************
*     NAME:   lv_poc_win_create
*   AUTHOR:   lugj
* DESCRIPT:   创建poc窗口
              parent只可以是activity中display成员，其他的lv_obj_t在有控制栏时会出现部分内容被控制栏遮挡
*     DATE:   2019-11-25
********************/
lv_poc_win_t * lv_poc_win_create(lv_obj_t * parent, const char * title, lv_poc_win_create_func_t func)
{
    lv_style_t * style_header;
    lv_poc_win_t * new_win = (lv_poc_win_t *)lv_mem_alloc(sizeof(lv_poc_win_t));
    poc_setting_conf = lv_poc_setting_conf_read();
    style_header = (lv_style_t *)(poc_setting_conf->theme.current_theme->style_win_header);
    if(new_win == NULL)
    {
        return NULL;
    }
    memset(new_win, 0, sizeof(lv_poc_win_t));
    new_win->header = lv_obj_create(parent, NULL);
    style_header->text.font = (lv_font_t *)poc_setting_conf->font.win_title_font;
    lv_obj_set_style(new_win->header, style_header);
    lv_poc_activity_t * activity = lv_poc_activity_list_lookup(parent);
    if(NULL != activity)
    {
        lv_obj_set_size(new_win->header, lv_poc_get_display_width(parent), lv_poc_get_display_height(parent)/LV_POC_WIN_TITLE_HEIGHT_RATIO);
    }
    else
    {
        lv_obj_set_size(new_win->header, lv_obj_get_width(parent), lv_obj_get_height(parent)/LV_POC_WIN_TITLE_HEIGHT_RATIO);
    }
    new_win->title = lv_label_create(new_win->header, NULL);
    if(title != NULL)
    {
        lv_label_set_text(new_win->title, title);
    }
    else
    {
        lv_label_set_text(new_win->title, "");
    }
    lv_label_set_long_mode(new_win->title, LV_LABEL_LONG_SROLL_CIRC);
    lv_label_set_align(new_win->title, LV_LABEL_ALIGN_CENTER);
    lv_obj_align(new_win->title, lv_obj_get_parent(new_win->title), LV_ALIGN_CENTER, 0, 0);
    new_win->display_area.x1 = lv_obj_get_x(new_win->header);
    new_win->display_area.x2 = lv_obj_get_x(new_win->header) + lv_obj_get_width(new_win->header);
    new_win->display_area.y1 = lv_obj_get_y(new_win->header) + lv_obj_get_height(new_win->header);
    if(NULL != activity)
    {
        new_win->display_area.y2 = lv_obj_get_y(new_win->header) + lv_poc_get_display_height(parent);
    }
    else
    {
        new_win->display_area.y2 = lv_obj_get_y(new_win->header) + lv_obj_get_height(parent);
    }

    if( func != NULL)
    {
        new_win->display_obj = func(parent, new_win->display_area);
    }
    else
    {
        new_win->display_obj = empty_win(parent, new_win->display_area);
    }

    if(new_win->display_obj == NULL)
    {
        lv_obj_del(new_win->title);
        lv_obj_del(new_win->header);
        lv_mem_free(new_win);
        return NULL;
    }
    return new_win;
}


#ifdef __cplusplus
}
#endif

