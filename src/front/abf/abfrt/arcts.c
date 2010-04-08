/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/
#include	<compat.h>
#include	<ex.h>
#include	<me.h>
#include	<si.h>
#include	<st.h>
#include	<ol.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<eqlang.h>
#include	<eqrun.h>
#include	<feconfig.h>
#include	<adf.h>
#include	<ade.h>
#include	<afe.h>
#include	<ooclass.h>
#include	<abfcnsts.h>
#include	<abftrace.h>
#include	<fdesc.h>
#include	<abfrts.h>
#include	<rtsdata.h>

GLOBALREF ABRTSSTK	*IIarRtsStack;

VOID
IIARctsCreateTestStack(name)
char	*name;
{
    static ABRTSSTK	stkele;

    IIarRtsStack = &stkele;
    IIarRtsStack->abrfsname = name;
    IIarRtsStack->abrfsnext = NULL;
    IIarRtsStack->abrfsusage = 0;
    IIarRtsStack->abrfsfrm = NULL;
    IIarRtsStack->abrfsprm = NULL;
    IIarRtsStack->abrfsfdesc = NULL;
    IIarRtsStack->abrfsdml = 0;
}

IIARstrSetTopRtsstk(fdesc)
FDESC	*fdesc;
{
    if (IIarRtsStack != NULL)
	IIarRtsStack->abrfsfdesc = fdesc;
}
