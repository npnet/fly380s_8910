/******************************************
*   将check box封装为单选(radio box)按钮
*   采用 radio box 组的管理方式，一个 radio box 组中，有且只有一个 check box 处于选中(按下)状态
*   可以有多个其他状态
*   rb组不管理cb具体对象的存续，rb组清空删除操作并不会删除实际对象cb, 需要自己使用lv_obj_del()删除cb
******************************************/
#ifndef __INCLUDE_LV_POC_RB__
#define  __INCLUDE_LV_POC_RB__

#ifdef __cplusplus
extern "C" {
#endif

/*************************************************
*
*                  INCLUDE
*
*************************************************/
#include "lv_include/lv_poc_type.h"
#include <stdarg.h>

/*************************************************
*
*                  DEFINE
*
*************************************************/
#define LV_POC_WIN_TITLE_HEIGHT_RATIO  5

/*************************************************
*
*                  ENUM
*
*************************************************/


/*************************************************
*
*                  TYPEDEF
*
*************************************************/
/*Data of list*/
typedef struct _lv_poc_rb_node_t {
    lv_obj_t * cb;
    lv_btn_state_t state;
    struct _lv_poc_rb_node_t * next;
} lv_poc_rb_node_t;


typedef struct _lv_poc_rb_t{
    lv_obj_t * last_cb;
    lv_poc_rb_node_t *cbs;
} lv_poc_rb_t;



/*************************************************
*
*                  STRUCT
*
*************************************************/
/*******************
*     NAME:
*   AUTHOR:   lugj
* DESCRIPT:
*     DATE:   2019-11-25
********************/


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


/*************************************************
*
*                  PUBLIC DEFINE
*
*************************************************/
/*******************
*     NAME:   lv_poc_rb_create
*   AUTHOR:   lugj
* DESCRIPT:   创建interphone radio box对象组
*     DATE:   2019-11-25
********************/
lv_poc_rb_t * lv_poc_rb_create(void);

/*******************
*     NAME:   lv_poc_rb_add
*   AUTHOR:   lugj
* DESCRIPT:   将check box作为radio box添加到radio box对象组中
*     DATE:   2019-11-25
********************/
void lv_poc_rb_add(lv_poc_rb_t * rb, lv_obj_t * cb);

/*******************
*     NAME:   lv_poc_rb_press
*   AUTHOR:   lugj
* DESCRIPT:   选中rb中cb
*     DATE:   2019-11-25
********************/
void lv_poc_rb_press(lv_poc_rb_t * rb, lv_obj_t * cb);

/*******************
*     NAME:   lv_poc_rb_get_pressed
*   AUTHOR:   lugj
* DESCRIPT:   获取rb中当前被选中的cb
*     DATE:   2019-11-25
********************/
lv_obj_t * lv_poc_rb_get_pressed(lv_poc_rb_t * rb);

/*******************
*     NAME:   lv_poc_rb_remove_cb
*   AUTHOR:   lugj
* DESCRIPT:   删除rb中指定的cb
*     DATE:   2019-11-25
********************/
void lv_poc_rb_remove_cb(lv_poc_rb_t * rb, lv_obj_t * cb);

/*******************
*     NAME:   lv_poc_rb_clean
*   AUTHOR:   lugj
* DESCRIPT:   清空rb中所有cb
*     DATE:   2019-11-25
********************/
void lv_poc_rb_clean(lv_poc_rb_t * rb);

/*******************
*     NAME:   lv_poc_rb_del
*   AUTHOR:   lugj
* DESCRIPT:   删除rb并清空所有cb
*     DATE:   2019-11-25
********************/
void lv_poc_rb_del(lv_poc_rb_t * rb);



#ifdef __cplusplus
}
#endif


#endif //__INCLUDE_LV_INTER_PHONE_RB__


