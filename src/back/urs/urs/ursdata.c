/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <lo.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <ursm.h>

/*
** Name:	ursdata.c
**
** Description:	Global data for User Request Services (urs) facility.
**
** History:
**      06-Nov-1997 wonst02
**          Original User Request Services Manager.
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPICELIBDATA
**	    in the Jamfile.
*/

/*
**
LIBRARY = IMPICELIBDATA
**
*/

GLOBALDEF   URS_MGR_CB		*Urs_mgr_cb	= 0;
