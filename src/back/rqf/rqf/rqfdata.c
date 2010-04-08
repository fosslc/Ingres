/*
**Copyright (c) 2004 Ingres Corporation
*/

/*
**
LIBRARY = IMPRQFLIBDATA
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <sl.h>
#include    <pc.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <lg.h>
#include    <ulm.h>
#include    <gca.h>
#include    <st.h>
#include    <cm.h>
#include    <me.h>
#include    <nm.h>
#include    <tm.h>
#include    <tr.h>
#include    <pc.h>
#include    <lk.h>
#include    <cx.h>
#include    <scf.h>
#include    <adf.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <qefmain.h>
#include    <rqf.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmccb.h>
#include    <qefnode.h>
#include    <psfparse.h>
#include    <qefact.h>
#include    <rqfprvt.h>
#include    <generr.h>
#include    <sqlstate.h>

/*
** Name:	rqfdata.c
**
** Description:	Global data for rqf facility.
**
** History:
**	23-sep-96 (canor01)
**	    Created.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	12-apr-2002 (devjo01)
**	    Add headers needed to get CX_MAX_NODE_NAME_LEN for 'rqfprvt.h'.
**	18-sep-2003 (abbjo03)
**	    Add include of pc.h (required by lg.h).
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPRQFLIBDATA
**	    in the Jamfile.
*/

GLOBALDEF	RQF_CB	*Rqf_facility ZERO_FILL;
GLOBALDEF       i4  ss5000g_dbproc_message = 0;
GLOBALDEF	i4  ss50009_error_raised_in_dbproc = 0;
GLOBALDEF       i4  ss00000_success = 0;       
