/*
**Copyright (c) 2004 Ingres Corporation
*/

# include    <compat.h>
# include    <gl.h>
# include    <sl.h>
# include    <iicommon.h>
# include    <dbdbms.h>
# include    <pc.h>
# include    <cs.h>
# include    <lk.h>
# include    <tm.h>
# include    <di.h>
# include    <lo.h>
# include    <me.h>
# include    <lg.h>
# include    <st.h>
# include    <nm.h>
# include    <er.h>
# include    <pm.h>
# include    <cv.h>
# include    <st.h>
# include    <tr.h>
# include    <ulf.h>
# include    <ulm.h>
# include    <sxf.h>
# include    <sxfint.h>
# include    <sxap.h>
# include    "sxapint.h"

/*
** Name:	sxapdata.c
**
** Description:	Global data for sxap facility.
**
** History:
**	23-sep-96 (canor01)
**	    Created.
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPSXFLIBDATA
**	    in the Jamfile.
*/

/*
**
LIBRARY = IMPSXFLIBDATA
**
*/

/*
** data from sxapc.c
*/

GLOBALDEF SXAP_CB  *Sxap_cb = NULL;
