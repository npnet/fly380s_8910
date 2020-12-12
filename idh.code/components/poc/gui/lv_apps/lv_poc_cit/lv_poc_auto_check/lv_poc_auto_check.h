#ifndef __LV_POC_AUTO_CHECK_H_
#define __LV_POC_AUTO_CHECK_H_

void lvPocCitAutoTestActiOpen(void);

void lvPocCitAutoTestActiClose(void);

bool pubPocCitAutoTestMode(void);

void lvPocCitAutoTestCom_Msg(lv_poc_cit_auto_test_type result);

void lv_poc_cit_result_open(lv_task_t *task);

#endif

