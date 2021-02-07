//#include "osi_log.h"
//#include "osi_api.h"
//#include "ctel_oem_api.h"
//#include "ctel_oem_callback_realize.h"
//#define LOG_TAG_CTEL_PTT  OSI_MAKE_LOG_TAG('C', 'T', 'E', 'L')
//
//static void demo_poc(void *param)
//{
//    OSI_LOGI(0, "enter demo_poc\n");
//
//	int demoflag = 1;
//	while (demoflag)
//    {
//		OSI_LOGI(0, "ctelpttdemo_start**************************");
//        osiThreadSleep(1000);
//		OSI_LOGI(0, "ctelpttdemo_start --------------- end");
//    }
//
//    osiThreadExit();
//}
//
//
//#define demotest
//void  ctelpttdemo_start()
//{
//#ifdef demotest
//	ctel_set_log_level(CTEL_DEBUG); //设置日志的优先级 默认为debug
//    OSI_LOGI(0, "ctelpttapp>>appimg_enter");
//
//	CTEL_REGISTER_CB ctelCb;
//	ctelCb.callbackInform = ui_Inform;
//	ctelCb.callbackGetGprsAttState = ui_GetGprsAttState;
//	ctelCb.callbackGetSimInfo = ui_GetSimInfo;
//	ctelCb.callbackRecord = ui_Record;
//	ctelCb.callbackPlayer = ui_Player;
//	ctelCb.callbackPlayPcm = ui_PlayPcm;
//	ctelCb.callbackTone = ui_Tone;
//	ctelCb.callbackTTS = ui_TTS;
//	ctel_register_callback(&ctelCb); //注册回调
//
//	ctel_set_dev_name("cat1-utf8",9); //设置终端名称，这里设置是为了让服务器给终端的群组、成员信息为UTF8编码
//	ctel_set_sim_cid(1);	//设置sim卡激活pdp绑定的cid
//	ctel_set_pdp_apn("cmnet",5); //设置驻网APN
//	ctel_set_task_priority(OSI_PRIORITY_NORMAL);//设置任务优先级
//	ctel_set_headbeat_time(20);	//设置心跳周期
//	ctel_set_register_info("emp.189diaodu.cn",2072,"poc.189diaodu.cn",2072); //设置主辅服务器地址
//
//	ctel_start_pttservice(); //启动ptt服务
//
//	osiThreadSleep(10000);
//	osiThread_t * demo = osiThreadCreate("demo",demo_poc, NULL, OSI_PRIORITY_NORMAL, 10240, 8);
//	#endif
//}
//
