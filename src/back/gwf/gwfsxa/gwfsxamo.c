/*
**Copyright (c) 2004 Ingres Corporation
*/

# include    <compat.h>
# include    <gl.h>
# include    <cs.h>
# include    <di.h>
# include    <pc.h>
# include    <tm.h>
# include    <erglf.h>
# include    <sp.h>
# include    <mo.h>
# include    <sl.h>
# include    <iicommon.h>
# include    <dbdbms.h>
# include    <adf.h>
# include    <ulf.h>
# include    <ulm.h>
# include    <dmf.h>
# include    <dmrcb.h>
# include    <dmtcb.h>
# include    <dmucb.h>
# include    <sxf.h>
# include    <gwf.h>
# include    <gwfint.h>
# include    "gwfsxa.h"

/*
** Name: GWFSXAMO.C		- SXA Managed Objects
**
** Description:
**	This file contains the routines and data structures which define the
**	managed objects for GWFSXA. Managed objects appear in the MIB, 
**	and allow monitoring, analysis, and control of GWFSXA through the 
**	MO interface.
**
** History:
**	20-nov-1992 (robf)_
**	    Created
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	23-sep-1996 (canor01)
**	    Moved class definitions to gwfsxadata.c.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      24-Feb-2004 (kodse01)
**          Removed gwxit.h inclusion which is not required.
*/

GLOBALREF MO_CLASS_DEF SXAmo_stat_classes[];

/*
**	GWSXA_mo_attach() - Attach to the MIB
**
**	Inputs:
**		None
**
**	Returns:
**		E_DB_OK    - Worked
**		E_DB_ERROR - Failure.
*/
DB_STATUS
GWSXAmo_attach(void)
{
    STATUS	status;
    i4		i;


    status = MOclassdef(MAXI2, SXAmo_stat_classes);

    if (status!=OK)
    {
	/*
	**	Error attaching to MIB, so log this went wrong
	*/
	gwsxa_error(NULL,E_GW407A_SXA_BAD_MO_ATTACH,
			SXA_ERR_INTERNAL, 0);
	return E_DB_ERROR;
    }
    else
	 return E_DB_OK;
}
