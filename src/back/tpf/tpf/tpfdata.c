/*
**Copyright (c) 2004 Ingres Corporation
*/


#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <pc.h>
#include    <di.h>
#include    <lo.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <cs.h>
#include    <cv.h>
#include    <me.h>
#include    <nm.h>
#include    <st.h>
#include    <tm.h>
#include    <tr.h>
#include    <scf.h>
#include    <qsf.h>
#include    <adf.h>
#include    <qefrcb.h>
#include    <rqf.h>
#include    <tpf.h>
#include    <tpfcat.h>
#include    <tpfddb.h>
#include    <tpfqry.h>

/*
** Name:	tpfdata.c
**
** Description:	Global data for tpf facility.
**
** History:
**      23-Oct-95 (tutto01)
**          Created.
**	23-sep-96 (canor01)
**	    Updated.
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPTPFLIBDATA
**	    in the Jamfile.
**
*/

/*
**
LIBRARY = IMPTPFLIBDATA
**
*/

/*
**  Data from tpfmain.c
*/

GLOBALDEF   struct _TPD_SV_CB	*Tpf_svcb_p = NULL;
