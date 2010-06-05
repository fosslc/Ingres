/*
**Copyright (c) 2004 Ingres Corporation
*/

/*
**
LIBRARY = IMPDMFLIBDATA
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <cm.h>
#include    <cv.h>
#include    <me.h>
#include    <pc.h>
#include    <er.h>
#include    <nm.h>
#include    <tm.h>
#include    <tr.h>
#include    <st.h>
#include    <lo.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lk.h>
#include    <lg.h>
#include    <pm.h>
#include    <tmtz.h>
#include    <ulf.h>
#include    <scf.h>
#include    <adf.h>
#include    <add.h>
#include    <adp.h>
#include    <dmf.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmxcb.h>
#include    <dmtcb.h>
#include    <dmucb.h>
#include    <dmmcb.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dmm.h>
#include    <dmpp.h>
#include    <dm0m.h>
#include    <dm0p.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <sxf.h>
#include    <dma.h>
#include    <dm0llctx.h>
#include    <dm0s.h>
#include    <dm2t.h>
#include    <dml.h>
#include    <dmfgw.h>
#include    <dmve.h>
#include    <dmfrcp.h>
#include    <dmfinit.h>
#include    <dmftrace.h>
#include    <dmpecpn.h>
#include    <dmftm.h>
#include    <dm2rep.h>
#include    <dmfcrypt.h>


/*
** Name:	dmldata.c
**
** Description:	Global data for dml facility.
**
** History:
**
**	23-sep-96 (canor01)
**	    Created.
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPDMFLIBDATA
**	    in the Jamfile.
**	13-Apr-2010 (toumi01) SIR 122403
**	    Added Dmc_crypt for data encryption at rest.
*/

/* dmcstart.c */
GLOBALDEF	DMC_REP		*Dmc_rep;
GLOBALDEF	DMC_CRYPT	*Dmc_crypt;
