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

#include "mal_api.h"
#include "mal_internal.h"
#include "at_response.h"
#include "cfw.h"
#include "cfw_event.h"
#include "cfw_errorcode.h"

extern bool gSimDropInd[CONFIG_NUMBER_OF_SIM];

typedef struct
{
    unsigned errcode;
    unsigned state;
    unsigned remainRetries;
} ateContextReadCPIN_t;

static void prvCpinReadRspCB(malTransaction_t *trans, const osiEvent_t *event)
{
    // EV_CFW_SIM_GET_AUTH_STATUS_RSP
    ateContextReadCPIN_t *ctx = (ateContextReadCPIN_t *)malTransContext(trans);
    const CFW_EVENT *cfw_event = (const CFW_EVENT *)event;

    int auth = -1;
    if (cfw_event->nType == 0)
        auth = cfw_event->nParam1;
    if ((cfw_event->nType == 0xf0) /*&& (gAtCfwCtx.init_status == AT_MODULE_INIT_NO_SIM)*/)
        auth = CFW_STY_AUTH_NOSIM;
    if (auth == CFW_STY_AUTH_PIN1_DISABLE || auth == CFW_STY_AUTH_PIN1_READY)
        auth = CFW_STY_AUTH_READY;

    if (auth >= 0)
    {
        ctx->errcode = 0;
        ctx->state = auth;
        ctx->remainRetries = cfw_event->nParam2;
    }
    else
    {
        ctx->errcode = atCfwToCmeError(cfw_event->nParam1);
    }

    malTransFinished(trans);
}

unsigned malSimGetState(int sim, malSimState_t *state, unsigned *remaintries)
{
    if (state == NULL)
        return ERR_AT_CME_PARAM_INVALID;

    if (remaintries == NULL)
        return ERR_AT_CME_PARAM_INVALID;

    if (gSimDropInd[sim])
        return ERR_AT_CME_SIM_NOT_INSERTED;

    ateContextReadCPIN_t ctx = {};
    malTransaction_t *trans = malCreateTrans();
    if (trans == NULL)
        return ERR_AT_CME_NO_MEMORY;

    malSetTransContext(trans, &ctx, NULL);
    malStartUtiTrans(trans, prvCpinReadRspCB);

    unsigned result = CFW_SimGetAuthenticationStatus(malTransUti(trans), sim);
    if (result != 0)
    {
        malAbortTrans(trans);

        if (ERR_CFW_INVALID_PARAMETER == result)
            MAL_TRANS_RETURN_ERR(trans, ERR_AT_CME_PARAM_INVALID);
        else if (ERR_CME_OPERATION_NOT_ALLOWED == result)
            MAL_TRANS_RETURN_ERR(trans, ERR_AT_CME_OPERATION_NOT_ALLOWED);
        else if (ERR_NO_MORE_MEMORY == result)
            MAL_TRANS_RETURN_ERR(trans, ERR_AT_CME_NO_MEMORY);
        else if ((ERR_CME_SIM_NOT_INSERTED == result) ||
                 (ERR_CFW_SIM_NOT_INITIATE == result))
            MAL_TRANS_RETURN_ERR(trans, ERR_AT_CME_SIM_NOT_INSERTED);
        else
            MAL_TRANS_RETURN_ERR(trans, ERR_AT_CME_EXE_NOT_SURPORT);
    }

    if (!malTransWait(trans, CONFIG_MAL_GET_SIM_AUTH_TIMEOUT))
    {
        malAbortTrans(trans);
        MAL_TRANS_RETURN_ERR(trans, ERR_AT_CME_SEND_TIMEOUT);
    }

    if (ctx.errcode == 0)
    {
        *state = ctx.state;
        *remaintries = ctx.remainRetries;
    }
    MAL_TRANS_RETURN_ERR(trans, ctx.errcode);
}
