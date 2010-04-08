/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** History:
**      12-Aug-98 (fanra01)
**          Add dependent includes.
*/

#ifndef WTS_INCLUDED
#define WTS_INCLUDED
#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
#include	<ddfcom.h>
#include    <dbdbms.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <gca.h>
#include    <scf.h>
#include    <dmf.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <qsf.h>
#include    <adf.h>
#include    <adp.h>
#include    <ddb.h>
#include    <qefrcb.h>
#include    <qefqeu.h>
#include    <dudbms.h>
#include    <sc.h>
#include    <scc.h>
#include    <scs.h>
#include    <scd.h>
#include    <sc0m.h>
#include    <scfcontrol.h>
#include	<wsf.h>

DB_STATUS
WTSExecute(SCD_SCB *scb);

#endif
