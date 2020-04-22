
#ifndef __INCLUDE_LV_POC_LIST__
#define  __INCLUDE_LV_POC_LIST__

#ifdef __cplusplus
extern "C" {
#endif

/*************************************************
*
*                  INCLUDE
*
*************************************************/
#include "lvgl.h"
#include "lv_include/lv_poc.h"
//#include "lv_include/lv_poc_conf.h"
#include <stdarg.h>

/*************************************************
*
*                  DEFINE
*
*************************************************/

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

typedef void (* lv_poc_list_config_func_t)(lv_obj_t * list, lv_area_t list_area);

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
*     NAME:   lv_poc_list_create
*   AUTHOR:   lugj
* DESCRIPT:   ´´½¨interphone list
*     DATE:   2019-11-25
********************/
lv_obj_t * lv_poc_list_create(lv_obj_t * parent, lv_obj_t * copy,lv_area_t list_area, lv_poc_list_config_func_t func);


#ifdef __cplusplus
}
#endif


#endif //__INCLUDE_LV_INTER_PHONE_LIST__

