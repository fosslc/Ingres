/*
**Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <gl.h>
# include <pc.h>
# include <sl.h>
# include <iicommon.h>
# include <dbdbms.h>
# include <cs.h>
# include <lk.h>
# include <tm.h>
# include <ulf.h>
# include <sxf.h>
# include <sxfint.h>
# include <sxap.h>

/*
** Name: SXFGLOB.C - allocate SXF global variables
**
** Description:
**	This file allocates all global variables used in the SXF.
**
** History:
**	9-july-1992 (markg)
**	    Initial creation.
**	10-aug-1992 (markg)
**	    Modified to include tm.h.
**	20-oct-1992 (markg)
**	    Include definitions of the SXAP call vector and initialization
**	    table.
**	10-Dec-1992 (markg)
**	    Remove the READONLY definition for Sxap_itab, because it doesn't
**	    work correctly on ANSI compilers that have READONLY defined as
**	    const.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-jan-94 (stephenb)
**          Add sxapo_startup to the Sxap_itab list.
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
** Definition of all global variables owned by this file
*/

GLOBALDEF  SXF_SVCB  *Sxf_svcb  ZERO_FILL;

GLOBALDEF  SXAP_VECTOR  *Sxap_vector ZERO_FILL;

GLOBALDEF  SXAP_ITAB  Sxap_itab =
{
    NULL,
    sxap_startup,
    sxapo_startup
};
