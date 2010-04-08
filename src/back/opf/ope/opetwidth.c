/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <sl.h>
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
#define             OPE_TWIDTH          TRUE
#include    <bt.h>
#include    <opxlint.h> /* EXTERNAL DEFINITION */ 
/**
**
**  Name: OPETWIDTH.C - get the width of the set of attributes in eqcls map
**
**  Description:
**      Routine to calculate the width of a tuple represented by an
**      equivalence class map
**
**  History:    
**      13-jun-86 (seputis)    
**          initial creation
**      16-aug-91 (seputis)
**          add CL include files for PC group
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**      06-mar-96 (nanpr01)
**          Use the typedef to be consistent with prototype. 
[@history_line@]...
**/


/*{
** Name: ope_twidth	- get width of tuple given equivalence class map
**
** Description:
**	Finds number of bytes in a tuple containing the equivalence classes
**      selected in the map
**
**	This only works for interior nodes, not original nodes,
**	cause it calls ope_type.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      emap                            map of equivalence classes in tuple
**
** Outputs:
**	Returns:
**	    width of tuple containing the equivalence classes in the map
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	13-jun-86 (seputis)
**          initial creation from awidth
**      06-mar-96 (nanpr01)
**          Use the typedef to be consistent with prototype. 
[@history_line@]...
*/
OPS_WIDTH
ope_twidth(
	OPS_SUBQUERY       *subquery,
	OPE_BMEQCLS        *emap)
{
    OPS_WIDTH           width;		/* used to acculmulate width of tuple */
    OPE_IEQCLS          eqcls;		/* equivalence class currently being
                                        ** analyzed */
    OPE_IEQCLS          maxeqcls;	/* number of equivalence classes in
                                        ** subquery */

    width = 0;
    maxeqcls = subquery->ops_eclass.ope_ev; /* number of equivalence classes
                                        ** defined */
    for (eqcls = -1; 
	 (eqcls = BTnext((i4)eqcls, (char *)emap, (i4)maxeqcls)) >= 0;)
    {
        DB_DATA_VALUE          *datatype; /* data type of equivalence class */
	datatype = ope_type(subquery, eqcls);
	width += datatype->db_length;
    }
    return (width);
}
