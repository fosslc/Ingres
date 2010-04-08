/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <me.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <scf.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <ulm.h>
#include    <qsfint.h>

/*
** Name:	qsrdata.c
**
** Description:	Global data for qsr facility.
**
** History:
**
**	23-sep-96 (canor01)
**	    Created.
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPQSFLIBDATA
**	    in the Jamfile.
*/

/*
**
LIBRARY = IMPQSFLIBDATA
**
*/

GLOBALDEF QSR_CB              *Qsr_scb = NULL;	    /* Ptr to QSF's server CB */
