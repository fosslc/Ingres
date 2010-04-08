/*
**Copyright (c) 2004 Ingres Corporation
*/

#include <compat.h>
#include <gl.h>
#include <iicommon.h>
#include <sl.h>
#include <dbdbms.h>
#include <pc.h>
#include <cs.h>
#include <er.h>
#include <tm.h>
#include <di.h>
#include <lo.h>
#include <lk.h>
#include <st.h>
#include <ulf.h>
#include <ulm.h>
#include <sxf.h>
#include <sxfint.h>
#include <sxap.h>
#include <sa.h>
#include "sxapoint.h"

/*
** Name:	sxapodata.c
**
** Description:	Global data for sxapo facility.
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
** data from sxapoc.c
*/

GLOBALDEF SXAPO_CB *Sxapo_cb = NULL;
