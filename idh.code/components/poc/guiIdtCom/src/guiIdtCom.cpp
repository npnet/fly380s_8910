/* Copyright (C) 2018 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * This software is supplied "AS IS" without any warranties.
 * RDA assumes no responsibility or liability for the use of the software,
 * conveys no license or title under any patent, copyright, or mask work
 * right to the product. RDA reserves the right to make changes in the
 * software without notification.  RDA also make no representation or
 * warranty that such application will be suitable for the specified use
 * without further testing or modification.
 */

#include "osi_log.h"
#include "osi_api.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "poc_config.h"
#include "guiIdtCom.h"

#ifdef CONFIG_POC_SUPPORT

#define APPTEST_THREAD_PRIORITY (OSI_PRIORITY_NORMAL)
#define APPTEST_STACK_SIZE (8192 * 4)
#define APPTEST_EVENT_QUEUE_SIZE (64)

osiThread_t *gAppTest;

char *GetCauseStr(USHORT usCause);
char *GetOamOptStr(DWORD dwOpt);
char *GetSrvTypeStr(SRV_TYPE_e SrvType);

#endif

