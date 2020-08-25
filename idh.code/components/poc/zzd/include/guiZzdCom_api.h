#ifndef _INCLUDE_GUIZZDCOM_API_H_
#define _INCLUDE_GUIZZDCOM_API_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

OSI_EXTERN_C_BEGIN
;

void guiZzdComInit(void);

int OEM_SendUart(char *uf,int len);

void OEM_TTS_Stop();

int OEM_TTS_Spk(char* atxt);

OSI_EXTERN_C_END

#endif
