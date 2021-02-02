
#ifdef __cplusplus
extern "C" {
#endif
#include "lv_include/lv_poc.h"

/**********************
 *  STATIC VARIABLES
 **********************/

static osiMutex_t *mutex = NULL;

/*******************
*     NAME:   lv_poc_rb_create
*   AUTHOR:   lugj
* DESCRIPT:   创建poc radio box对象组
*     DATE:   2019-11-25
********************/
lv_poc_rb_t * lv_poc_rb_create(void)
{
	if(mutex == NULL)
	{
		mutex = osiMutexCreate();
	}

    lv_poc_rb_t * rb = (lv_poc_rb_t *)lv_mem_alloc(sizeof(lv_poc_rb_t));
    if(rb == NULL)
    {
        return NULL;
    }
    memset(rb, 0, sizeof(lv_poc_rb_t));
    rb->last_cb = NULL;
    rb->cbs = NULL;
    return rb;
}

/*******************
*     NAME:   lv_poc_rb_add
*   AUTHOR:   lugj
* DESCRIPT:   将check box作为radio box添加到radio box对象组中,返回上一个被选中的cb
*     DATE:   2019-11-25
********************/
void lv_poc_rb_add(lv_poc_rb_t * rb, lv_obj_t * cb)
{
	mutex ? osiMutexLock(mutex) : 0;
    lv_btn_state_t cb_state;
    lv_poc_rb_node_t * new_rb_node = NULL;

    if(rb == NULL || cb == NULL)
    {
		mutex ? osiMutexUnlock(mutex) : 0;
        return;
    }

    cb_state = lv_btn_get_state(cb);
    new_rb_node = (lv_poc_rb_node_t *)lv_mem_alloc(sizeof(lv_poc_rb_node_t));
    if(new_rb_node == NULL)
    {
		mutex ? osiMutexUnlock(mutex) : 0;
        return;
    }
    new_rb_node->next = NULL;
    new_rb_node->cb = cb;
    new_rb_node->state = cb_state;

    if(rb->cbs != NULL)
    {
        new_rb_node->next = rb->cbs;
        rb->cbs = new_rb_node;
    }
    else
    {
        rb->cbs = new_rb_node;
    }
#if 0
    lv_poc_rb_node_t * p_rb_node = NULL;
    p_rb_node = rb->cbs;
    if(p_rb_node != NULL)
    {
        if(cb_state == LV_BTN_STATE_PR || cb_state == LV_BTN_STATE_TGL_PR)
        {
            if(p_rb_node->state == LV_BTN_STATE_PR || p_rb_node->state == LV_BTN_STATE_TGL_PR)
            {
                rb->last_cb = p_rb_node->cb;
                p_rb_node->state = LV_BTN_STATE_REL;
                lv_btn_set_state(p_rb_node->cb, LV_BTN_STATE_REL);
            }
            while(p_rb_node->next)
            {
                p_rb_node = p_rb_node->next;
                if(p_rb_node->state == LV_BTN_STATE_PR || p_rb_node->state == LV_BTN_STATE_TGL_PR)
                {
                    rb->last_cb = p_rb_node->cb;
                    p_rb_node->state = LV_BTN_STATE_REL;
                    lv_btn_set_state(p_rb_node->cb, LV_BTN_STATE_REL);
                }
            }
        }
        else
        {
            while(p_rb_node->next)
            {
                p_rb_node = p_rb_node->next;
            }
        }
        p_rb_node->next = new_rb_node;
        return rb->last_cb;

    }
    else
    {
        rb->cbs = new_rb_node;
        //rb->last_cb = cb;
        //new_rb_node->state = LV_BTN_STATE_PR;
        //lv_btn_set_state(cb, LV_BTN_STATE_PR);
        return rb->last_cb;
    }
#endif
	mutex ? osiMutexUnlock(mutex) : 0;
}

/*******************
*     NAME:   lv_poc_rb_press
*   AUTHOR:   lugj
* DESCRIPT:   选中rb中cb
*     DATE:   2019-11-25
********************/
void lv_poc_rb_press(lv_poc_rb_t * rb, lv_obj_t * cb)
{
    lv_poc_rb_node_t * p_rb_node = NULL;
    lv_btn_state_t cb_state;

    if(rb == NULL || rb->cbs == NULL || cb == NULL)
    {
        return;
    }

	mutex ? osiMutexLock(mutex) : 0;
    p_rb_node = rb->cbs;
    while(p_rb_node)
    {
        cb_state = lv_btn_get_state(p_rb_node->cb);
        if(p_rb_node->cb == cb)
        {
            lv_btn_set_state(cb, LV_BTN_STATE_TGL_PR);
        }
        else if(cb_state == LV_BTN_STATE_PR || cb_state == LV_BTN_STATE_TGL_REL || cb_state == LV_BTN_STATE_TGL_PR)
        {
            lv_btn_set_state(p_rb_node->cb, LV_BTN_STATE_REL);
        }
        p_rb_node = p_rb_node->next;
    }

#if 0
    if(rb == NULL || rb->cbs == NULL || rb->last_cb == NULL)
    {
        return NULL;
    }

    if(cb == NULL)
    {
        return rb->last_cb;
    }
    p_rb_node = rb->cbs;
    while(p_rb_node)
    {
        if(p_rb_node->cb == cb)
        {
            if(p_rb_node->state == LV_BTN_STATE_PR || p_rb_node->state == LV_BTN_STATE_TGL_PR)
            {
                return rb->last_cb;
            }
            break;
        }
        p_rb_node = p_rb_node->next;
    }

    if(p_rb_node->cb != cb)
    {
        return rb->last_cb;
    }

    p_rb_node = rb->cbs;
    while(p_rb_node)
    {
        if(p_rb_node->state == LV_BTN_STATE_PR || p_rb_node->state == LV_BTN_STATE_TGL_PR)
        {
            rb->last_cb = p_rb_node->cb;
            p_rb_node->state = LV_BTN_STATE_REL;
            lv_btn_set_state(p_rb_node->cb, p_rb_node->state);
        }

        if(p_rb_node->cb == cb)
        {
            p_rb_node->state = LV_BTN_STATE_PR;
            lv_btn_set_state(p_rb_node->cb, p_rb_node->state);
        }
        p_rb_node = p_rb_node->next;
    }
    return rb->last_cb;
#endif
	mutex ? osiMutexUnlock(mutex) : 0;
}

/*******************
*     NAME:   lv_poc_rb_get_pressed
*   AUTHOR:   lugj
* DESCRIPT:   获取rb中当前被选中的cb
*     DATE:   2019-11-25
********************/
lv_obj_t * lv_poc_rb_get_pressed(lv_poc_rb_t * rb)
{
    lv_poc_rb_node_t * p_rb_node = NULL;
    lv_btn_state_t cb_state;

    if(rb == NULL || rb->cbs == NULL)
    {
        return NULL;
    }

	mutex ? osiMutexLock(mutex) : 0;
    p_rb_node = rb->cbs;
    while(p_rb_node)
    {
        cb_state = lv_btn_get_state(p_rb_node->cb);
        if( cb_state == LV_BTN_STATE_TGL_PR)
        {
            break;
        }
        p_rb_node = p_rb_node->next;
    }
	mutex ? osiMutexUnlock(mutex) : 0;
    return p_rb_node->cb;
}

/*******************
*     NAME:   lv_poc_rb_remove_cb
*   AUTHOR:   lugj
* DESCRIPT:   删除rb中指定的cb
*     DATE:   2019-11-25
********************/
void lv_poc_rb_remove_cb(lv_poc_rb_t * rb, lv_obj_t * cb)
{
    lv_poc_rb_node_t * p_rb_node = NULL;

    if(rb == NULL || rb->cbs == NULL || cb == NULL)
    {
        return;
    }
    if(rb->cbs->cb == cb)
    {
        rb->cbs = rb->cbs->next;
    }
    else
    {
        p_rb_node = rb->cbs;
        while(p_rb_node->next)
        {
            if(p_rb_node->next->cb == cb)
            {
                p_rb_node->next = p_rb_node->next->next;
                return;
            }
            p_rb_node = p_rb_node->next;
        }
    }

#if 0
    if(rb == NULL || rb->cbs == NULL || rb->last_cb == NULL || cb == NULL)
    {
        return;
    }

    if(rb->last_cb == cb)
    {
        rb->last_cb = NULL;
    }

    if(rb->cbs->cb == cb)
    {
        rb->cbs = rb->cbs->next;
    }
    else
    {
        p_rb_node = rb->cbs;
        while(p_rb_node->next)
        {
            if(p_rb_node->next->cb == cb)
            {
                p_rb_node->next = p_rb_node->next->next;
                return;
            }
            p_rb_node = p_rb_node->next;
        }
    }
#endif
}

/*******************
*     NAME:   lv_poc_rb_clean
*   AUTHOR:   lugj
* DESCRIPT:   清空rb中所有cb
*     DATE:   2019-11-25
********************/
void lv_poc_rb_clean(lv_poc_rb_t * rb)
{
    lv_poc_rb_node_t * p_rb_node = NULL;
    lv_poc_rb_node_t * p = NULL;
    p_rb_node = rb->cbs->next;
    lv_mem_free(rb->cbs);
    while(p_rb_node)
    {
        p = p_rb_node;
        p_rb_node = p_rb_node->next;
        lv_mem_free(p);
    }

}

/*******************
*     NAME:   lv_poc_rb_del
*   AUTHOR:   lugj
* DESCRIPT:   删除rb并清空所有cb
*     DATE:   2019-11-25
********************/
void lv_poc_rb_del(lv_poc_rb_t * rb)
{
    lv_poc_rb_clean(rb);
    lv_mem_free(rb);
}

#ifdef __cplusplus
}
#endif

