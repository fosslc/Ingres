/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <ulm.h>
#include    <ulf.h>
#include    <adf.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <scf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <qefnode.h>
#include    <qefact.h>
#include    <qefqp.h>
/* beginning of optimizer header files */
#include    <opglobal.h>
#include    <opdstrib.h>
#include    <opfcb.h>
#include    <opgcb.h>
#include    <opscb.h>
#include    <ophisto.h>
#include    <opboolfact.h>
#include    <oplouter.h>
#include    <opeqclass.h>
#include    <opcotree.h>
#include    <opvariable.h>
#include    <opattr.h>
#include    <openum.h>
#include    <opagg.h>
#include    <opmflat.h>
#include    <opcsubqry.h>
#include    <opsubquery.h>
#include    <opcstate.h>
#include    <opstate.h>

/* external routine declarations definitions */
#define             OPE_CREATE          TRUE
#include    <me.h>
#include    <opxlint.h> /* EXTERNAL DEFINITION */ 
/**
**
**  Name: OPECREATE.C - create and initialize equivalence class table
**
**  Description:
**      Routines to create and initialize equivalence class table
**
**  History:    
**      11-jun-86 (seputis)    
**          initial creation
**      16-aug-91 (seputis)
**          add CL include files for PC group
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
[@history_line@]...
**/


/*{
** Name: ope_create	- create the equivalence class table
**
** Description:
**	This routine will initialize the equivalence class table.  It needs 
**	to be called prior to any reference to this table.
**
** Inputs:
**      subquery                        ptr to current subquery being analyzed
**
** Outputs:
**      subquery->ope_base              ptr to ptr of array of equivalence class
**                                      elements allocated
**      subquery->ope_ev                number of equivalence class elements
**                                      defined
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	20-apr-86 (seputis)
**          initial creation
[@history_line@]...
*/
VOID
ope_create(
	OPS_SUBQUERY       *subquery)
{
    OPS_STATE	    *global;

    MEfill( sizeof(OPE_EQUIVALENCE), 
	    (u_char)0, 
	    (PTR)&subquery->ops_eclass);    /* init ope_ev, ope_seqm, ope_jteqc
					    ** ope_subselect
                                            */
    global = subquery->ops_global;
    if (global->ops_mstate.ops_tet)
    {
	subquery->ops_eclass.ope_base = global->ops_mstate.ops_tet; /* use full size 
                                            ** OPZ_ET structure if available */
    }
    else
    {
	global->ops_mstate.ops_tet =
	subquery->ops_eclass.ope_base = (OPE_ET *) opu_memory( 
	    subquery->ops_global, (i4) sizeof(OPE_ET) ); /* get memory for 
					    ** array of pointers to equivalence
					    ** classes
					    */
    }
}
