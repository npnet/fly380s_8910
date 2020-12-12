#ifndef __LV_POC_PART_CHECK_H_
#define __LV_POC_PART_CHECK_H_

extern lv_poc_activity_t * poc_cit_part_test_activity;

bool lv_poc_cit_get_key_valid(void);

int lv_poc_cit_get_run_status(void);

void lv_poc_cit_part_test_open(void);

void lv_poc_cit_auto_test_items_register_cb(bool status);

void lv_poc_cit_auto_test_items_perform(lv_poc_cit_test_ui_id id);

#endif

