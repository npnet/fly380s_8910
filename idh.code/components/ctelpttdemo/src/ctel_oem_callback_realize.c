//
//#include "ctel_oem_type.h"
//#include "osi_api.h"
//#include "osi_pipe.h"
//#include "ctel_oem_callback_realize.h"
//#include "osi_log.h"
//
//#define FOR_FLY
//
//#ifdef FOR_FLY
//#include "cfw.h"
//
//#include "audio_player.h"
//#include "audio_recorder.h"
//#include "audio_decoder.h"
//#include "audio_reader.h"
//#include "audio_writer.h"
//#include "audio_tonegen.h"
//
//#define LOG_TAG_CTEL_PTT  OSI_MAKE_LOG_TAG('C', 'T', 'E', 'L')
//
//#define ONE_PCM_DECODE_UNIT  320
//
//typedef struct timer_para
//{
//    unsigned int cycvalue;
//} timer_para_t;
//
//typedef struct
//{
//    osiPipe_t *pipe;
//} pipeForRecord_t;
//
//
//osiMutex_t *mutexForAudio = NULL;
//// for ptt
// osiPipe_t *mctelPipeForPlayer = NULL;
// auPlayer_t *mctelplayer = NULL;
// osiTimer_t *mctelTimerForPlayer = NULL;
// timer_para_t mctelTimerParaForPlayer = {.cycvalue = 0};
//
//osiPipe_t *ctelPipeForRecord = NULL;
//osiWork_t *ctelWordForRecord = NULL;
//auRecorder_t *ctelRecorder = NULL;
//
//pcm_record_cb recordcb = NULL;
//
//#endif
//
//void ui_Inform(int result,CALLBACK_DATA *data)
//{
//	OSI_LOGI(0, "##callback_Inform get result %d data type %d reason %s\n",result,data->type,data->reason);
//}
//
//int ui_GetGprsAttState()
//{
//	int nRet = 0;
//  	 int nAttstate = 0;
//
//	#ifdef FOR_FLY
//  	uint8_t status = 0;
//	CFW_NW_STATUS_INFO nStatusInfo;
//	if (CFW_CfgGetNwStatus(&status, CFW_SIM_0) != 0 ||CFW_NwGetStatus(&nStatusInfo, CFW_SIM_0) != 0)
//	 {
//	 	OSI_LOGI(0,"CFW_CfgGetNwStatus %d or CFW_NwGetStatus %d Pstype %d failed\n", status,nStatusInfo.nStatus,nStatusInfo.PsType);
//	 }
//	else
//	{
//	    nAttstate = 1;
//	}
//	#endif
//	OSI_LOGI(0, "ui_GetGprsAttState ret %d state %d\n",nRet ,nAttstate);
//	return nAttstate;
//}
//
//void ui_GetSimInfo(char *imsi,unsigned int *lenimsi ,char *iccid,unsigned int *leniccid,char *imei,unsigned int *lenimei)
//{
//#ifdef FOR_FLY
//
//		 int ret = getSimImsi(CFW_SIM_0,imsi,lenimsi);
//		 imsi[*lenimsi] = 0;
//		 OSI_LOGI(0,"getSimImsi ret:%d, IMSI len %d :%s",ret,*lenimsi,imsi);
//		  ret = getSimImei(CFW_SIM_0,imei,lenimei);
//		 imei[*lenimei] = 0;
//		 OSI_LOGI(0,"getSimImei ret:%d, IMEI len %d :%s",ret,*lenimei,imei);
//		 ret = getSimIccid(CFW_SIM_0,iccid,leniccid);
//		 iccid[*leniccid] = 0;
//		 OSI_LOGI(0,"getSimIccid ret:%d, CCID len %d :%s",ret,*leniccid,iccid);
//#endif
//
//}
//
//#ifdef FOR_FLY
// static void ctel_pipe_record_work(void *param)
// {
//	/*pipeForRecord_t *d = (pipeForRecord_t *)param;
// 	if(d == NULL)
// 	{
// 		OSI_LOGI(0, "enter ctel_pipe_record_work with err para\n");
//		return;
// 	}*/
//	 char buf[320];
//	 unsigned long pcmlen = 0;
//	 static long index = 0;
//	 static int num = 0;
//	 static long timeus = 0 ;//= get_time_us();
//	 if( timeus == 0)
//	 {
//	 	timeus = get_time_us();
//	 }
//	  if(index %100 == 0)
//	  {
//	  	OSI_LOGI(0, "enter ctel_pipe_record_work with index = %d \n" , index);
//	  }
//	 while(ctelRecorder != NULL && ctelPipeForRecord!= NULL &&recordcb!=NULL)
//	 {
//		 int bytes = osiPipeRead(ctelPipeForRecord, buf, ONE_PCM_DECODE_UNIT);
//		 if (bytes <= 0)
//			 break;
//		 index++;
//		 pcmlen = pcmlen + bytes;
//		 if(index %100 == 99)
//		 {
//			OSI_LOGI(0, "ctel_pipe_record_work %d osiPipeRead %d pcmlen %ld", index,bytes,pcmlen);
//		 }
//		 //add too encode queue
//		 if( bytes <= ONE_PCM_DECODE_UNIT && recordcb!=NULL )
//		 {
//			recordcb(buf,bytes);
//		 }
//		 else
//		 {
//		 	ctel_msleep(20);
//		 }
//	 }
//
//	 if(ctelRecorder == NULL || ctelPipeForRecord == NULL)
//	 {
//	 	index = 0;
//		num = 0;
//		timeus = 0 ;
//	 }
//	if(index %100 == 0)
//	{
//		OSI_LOGI(0, "exit ctel_pipe_record_work %d pcmlen %ld", index,pcmlen);
//	}
//
// }
//
//
// static void ctel_pipe_record_callback(void *param, unsigned event)
//{
//    osiWork_t *work = (osiWork_t *)param;
//    osiWorkEnqueue(work, osiSysWorkQueueLowPriority());
//}
//#endif
//
//int ui_Record(ENABLE_TYPE enable, pcm_record_cb cb)
//{
//	OSI_LOGI(0, "ui_Record enable %d\n",enable);
//
//	#ifdef FOR_FLY
//	if(mutexForAudio == NULL)
//	{
//		ctel_init_mutex(&mutexForAudio);
//	}
//	if(enable ==ENABLE_OPEN_START)
//	{
//		ui_Player(DISABLE_CLOSE_STOP);
//		ui_Record(DISABLE_CLOSE_STOP,cb);
//		recordcb = cb;
//
//		ctel_mutex_lock(mutexForAudio);
//		 ctelPipeForRecord = osiPipeCreate(3200);
//		 if(ctelPipeForRecord== NULL)
//		{
//			OSI_LOGI(0, "##ctel_start_recorde but osiPipeCreate failed\n");
//		}
//		 pipeForRecord_t pipefd = {.pipe = ctelPipeForRecord};
//		 ctelWordForRecord = osiWorkCreate(ctel_pipe_record_work, NULL, &pipefd);
//		  if(ctelWordForRecord== NULL)
//		{
//			OSI_LOGI(0, "##ctel_start_recorde but osiWorkCreate failed\n");
//		}
//		 osiPipeSetReaderCallback(ctelPipeForRecord, OSI_PIPE_EVENT_RX_ARRIVED, ctel_pipe_record_callback, ctelWordForRecord);
//
//		ctelRecorder = auRecorderCreate();
//		if(ctelRecorder== NULL)
//		{
//			OSI_LOGI(0, "##ctel_start_recorde but auRecorderCreate failed\n");
//		}
//		 bool ret = auRecorderStartPipe(ctelRecorder, AUDEV_RECORD_TYPE_MIC, AUSTREAM_FORMAT_PCM, NULL, ctelPipeForRecord);
//		int timer = 0;
//		while(ret == 0 && (timer++ < 5))
//		{
//			OSI_LOGI(0, "##ctel_start_recorde but auRecorderStartPipe failed,restart it timer %d\n",timer);
//			ctel_msleep(200);
//			ret = auRecorderStartPipe(ctelRecorder, AUDEV_RECORD_TYPE_MIC, AUSTREAM_FORMAT_PCM, NULL, ctelPipeForRecord);
//		}
//		 ctel_mutex_unlock(mutexForAudio);
//		 OSI_LOGI(0, "auRecorderStartPipe ret %d\n",ret);
//	}
//	else
//	{
//		recordcb = NULL;
//		ctel_mutex_lock(mutexForAudio);
//		if(ctelRecorder != NULL)
//		{
//			auRecorderStop(ctelRecorder);
//		 	auRecorderDelete(ctelRecorder);
//		}
//		if(ctelWordForRecord != NULL)
//		{
//			 osiWorkDelete(ctelWordForRecord);
//		}
//		if(ctelPipeForRecord != NULL)
//		{
//			 osiPipeDelete(ctelPipeForRecord);
//		}
//		ctelRecorder = NULL;
//		ctelWordForRecord = NULL;
//		ctelPipeForRecord = NULL;
//		ctel_mutex_unlock(mutexForAudio);
//	}
//	#endif
//
//	OSI_LOGI(0, "ui_Record enable %d succ\n",enable);
//	return 1;
//}
//
//int ui_Player(ENABLE_TYPE enable)
//{
//	OSI_LOGI(0, "ui_Player enable %d\n",enable);
//	#ifdef FOR_FLY
//	if(mutexForAudio == NULL)
//	{
//		ctel_init_mutex(&mutexForAudio);
//	}
//	if(enable == ENABLE_OPEN_START)
//	{
//		ui_Player(DISABLE_CLOSE_STOP);
//		ctel_mutex_lock(mutexForAudio);
//		if(mctelPipeForPlayer == NULL)
//		{
//			OSI_LOGI(0, "enter ui_Player osiPipeCreate\n");
//		 	mctelPipeForPlayer = osiPipeCreate(3200);
//		}
//		if(mctelplayer == NULL)
//		{
//			 mctelplayer = auPlayerCreate();
//		}
//		if(mctelPipeForPlayer == NULL || mctelplayer == NULL)
//		{
//			OSI_LOGI(0, "##ui_Player init mctelplayer failed \n");
//			if(mctelPipeForPlayer != NULL)
//			{
//				osiPipeDelete(mctelPipeForPlayer);
//			}
//			if(mctelplayer != NULL)
//			{
//				 auPlayerDelete(mctelplayer);
//			}
//			mctelPipeForPlayer = NULL;
//			mctelplayer = NULL;
//			ctel_mutex_unlock(mutexForAudio);
//			return;
//		}
//		 auFrame_t frame = {.sample_format = AUSAMPLE_FORMAT_S16, .sample_rate = 8000, .channel_count = 1};
//		 auDecoderParamSet_t params[2] = {{AU_DEC_PARAM_FORMAT, &frame}, {0}};
//		 bool ret = auPlayerStartPipe(mctelplayer, AUSTREAM_FORMAT_PCM, params, mctelPipeForPlayer);
//		 int timer = 0;
//		 while(ret == 0 && (timer++ < 5))
//		 {
//			OSI_LOGI(0, "##ui_Player but auPlayerStartPipe failed ,restart timer %d\n",timer);
//			ctel_msleep(200);
//			ret = auPlayerStartPipe(mctelplayer, AUSTREAM_FORMAT_PCM, params, mctelPipeForPlayer);
//		 }
//		 if(ret == 0)
//		 {
//			OSI_LOGI(0, "##ui_Player but auPlayerStartPipe failed ,restart timer %d\n",timer);
//			if(mctelPipeForPlayer != NULL)
//			{
//				osiPipeSetEof(mctelPipeForPlayer);
//			}
//   			if(mctelplayer != NULL)
//			{
//				auPlayerStop(mctelplayer);
//				 auPlayerDelete(mctelplayer);
//			}
//		 	if(mctelPipeForPlayer != NULL)
//			{
//				osiPipeDelete(mctelPipeForPlayer);
//			}
//
//			mctelPipeForPlayer = NULL;
//			mctelplayer = NULL;
//		 }
//		ctel_mutex_unlock(mutexForAudio);
//	}
//	else
//	{
//		ctel_mutex_lock(mutexForAudio);
//
//	   if(mctelPipeForPlayer != NULL)
//		{
//			osiPipeSetEof(mctelPipeForPlayer);
//		}
//
//		if(mctelplayer != NULL)
//		{
//			auPlayerStop(mctelplayer);
//			auPlayerDelete(mctelplayer);
//		}
//		if(mctelPipeForPlayer != NULL)
//		{
//			osiPipeDelete(mctelPipeForPlayer);
//		}
//		mctelPipeForPlayer = NULL;
//		mctelplayer = NULL;
//		ctel_mutex_unlock(mutexForAudio);
//	}
//	#endif
//	OSI_LOGI(0, "ui_Player enable %d succ\n",enable);
//	return 1;
//}
//
//int ui_PlayPcm( char *pcm , int len)
//{
//	//OSI_LOGI(0, "ui_PlayPcm %d\n",len);
//	#ifdef FOR_FLY
//	if(mctelPipeForPlayer != NULL && len >0)
//	{
//		int writesize = osiPipeWriteAll(mctelPipeForPlayer,pcm, len, OSI_WAIT_FOREVER);
//		static int timer = 0;
//		timer++;
//		if(timer % 100 == 0)
//		{
//			OSI_LOGI(0,"###ui_PlayPcm need play %d writesize %d  timer %d\n",len , writesize,timer);
//		}
//	}
//	#endif
//	return 1;
//}
//
//int ui_Tone(ENABLE_TYPE enable, TONE_TYPE type)
//{
//	OSI_LOGI(0, "ui_Tone enable %d type %d\n",enable,type);
//	return 1;
//}
//
//int ui_TTS(ENABLE_TYPE enable, char *unicodestr , int len)
//{
//	OSI_LOGI(0, "ui_TTS enable %d \n",enable);
//	return 1;
//}
//
//
