/*
** Copyright (c) 2004 Ingres Corporation
*/

#include <compat.h>
#include <gl.h>
#include <gc.h>
#include <mu.h>

#include <iicommon.h>
#include <gca.h>
#include <gcaint.h>
#include <gcxdebug.h>
#include <gcs.h>
#include "gcsint.h"

/*
** Name: gcsdata.c
**
** Description:
**	Global data for GCS.
**
** History:
**	21-May-97 (gordy)
**	    Created.
**	 5-Aug-97 (gordy)
**	    Global GCS data is now passed to mechanisms during
**	    initialization.  Mechanism names now defined in 
**	    mechanisms info.
**	20-Aug-97 (gordy)
**	    Added encryption objects GCS_E_INIT, GCS_E_CONFIRM.
**	 4-Dec-97 (rajus01)
**	    Added GCS_PARM_IP_FUNC for tracing.
**	25-Feb-98 (gordy)
**	    Added GCS_E_DATA object for encrypted data.
**	15-May-98 (gordy)
**	    Made the GCS control block global for access by utility
**	    functions (should not be accessed directly by mechanisms).
**	 4-Sep-98 (gordy)
**	    Added delegation object and release operation.
**	 9-Jul-04 (gordy)
**	    GCS_OP_IP_AUTH is now distinct operation.
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPGCFLIBDATA
**	    in the Jamfile.
*/

/*
**
LIBRARY = IMPGCFLIBDATA
**
*/

/*
** GCS globals.
*/

GLOBALDEF GCS_GLOBAL	*IIgcs_global = NULL;

/*
** Tracing info.
*/

GLOBALDEF GCXLIST gcs_tr_ops[] =
{
    { GCS_OP_INIT, "GCS_OP_INIT" },
    { GCS_OP_TERM, "GCS_OP_TERM" },
    { GCS_OP_INFO, "GCS_OP_INFO" },
    { GCS_OP_SET, "GCS_OP_SET" },
    { GCS_OP_VALIDATE, "GCS_OP_VALIDATE" },
    { GCS_OP_USR_AUTH, "GCS_OP_USR_AUTH" },
    { GCS_OP_PWD_AUTH, "GCS_OP_PWD_AUTH" },
    { GCS_OP_SRV_KEY, "GCS_OP_SRV_KEY" },
    { GCS_OP_SRV_AUTH, "GCS_OP_SRV_AUTH" },
    { GCS_OP_REM_AUTH, "GCS_OP_REM_AUTH" },
    { GCS_OP_RELEASE, "GCS_OP_RELEASE" },
    { GCS_OP_IP_AUTH, "GCS_OP_IP_AUTH" },
    { GCS_OP_E_INIT, "GCS_OP_E_INIT" },
    { GCS_OP_E_CONFIRM, "GCS_OP_E_CONFIRM" },
    { GCS_OP_E_ENCODE, "GCS_OP_E_ENCODE" },
    { GCS_OP_E_DECODE, "GCS_OP_E_DECODE" },
    { GCS_OP_E_TERM, "GCS_OP_E_TERM" },
    { 0, NULL }
};

GLOBALDEF GCXLIST gcs_tr_objs[] =
{
    { GCS_USR_AUTH, "GCS_USR_AUTH" },
    { GCS_PWD_AUTH, "GCS_PWD_AUTH" },
    { GCS_SRV_KEY, "GCS_SRV_KEY" },
    { GCS_SRV_AUTH, "GCS_SRV_AUTH" },
    { GCS_REM_AUTH, "GCS_REM_AUTH" },
    { GCS_E_INIT, "GCS_E_INIT" },
    { GCS_E_CONFIRM, "GCS_E_CONFIRM" },
    { GCS_E_DATA, "GCS_E_DATA" },
    { GCS_DELEGATE, "GCS_DELEGATE" },
    { 0, NULL }
};

GLOBALDEF GCXLIST gcs_tr_parms[] =
{
    { GCS_PARM_GET_KEY_FUNC, "GET_KEY_FUNC" },
    { GCS_PARM_USR_PWD_FUNC, "USR_PWD_FUNC" },
    { GCS_PARM_IP_FUNC, "IP_FUNC" },
    { 0, NULL }
};

