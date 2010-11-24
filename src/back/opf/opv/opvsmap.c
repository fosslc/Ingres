/*
**Copyright (c) 2004, 2010 Ingres Corporation
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
#define        OPV_SMAP      TRUE
#include    <me.h>
#include    <bt.h>
#include    <opxlint.h>


/**
**
**  Name: OPVSMAP.C - map query tree associated with subquery
**
**  Description:
**      Contains routine to map query tree associated with subquery
**
**  History:    
**      3-jul-86 (seputis)    
**          initial creation
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
void opv_smap(
	OPS_SUBQUERY *subquery);

/*{
** Name: opv_smap	- map query tree associated with subquery
**
** Description:
**      This routine will map the query tree associated with the subquery.  A
**      subquery is typically associated with a PST_AGHEAD, or PST_ROOT node. 
**      A flag is check to see if the variable map was invalidated by any 
**      previous substitution.  For now this will be a consistency check 
**      and the tree will be mapped anyways.  FIXME later change this to 
**      avoid mapping the tree if there has been no substitutions.
**
** Inputs:
**      subquery                        ptr to subquery which will be mapped
**
** Outputs:
**      subquery->ops_root              this PST_RT_NODE will have the bitmaps
**                                      updated
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	3-jul-86 (seputis)
**          initial creation
**	22-apr-90 (seputis)
**          fix rohm-haas bug (no bug number), aggregate on a select distinct view
**	24-jun-91 (seputis)
**	    turn off consistency check to avoid traversal of parse tree
**	5-dec-02 (inkdo01)
**	    Changes for range table expansion.
**	31-Aug-2006 (kschendel)
**	    Watch for HFAGG as well as RFAGG.
*/
VOID
opv_smap(
	OPS_SUBQUERY       *subquery)
{
    OPV_GBMVARS         map;		/* range var map of query tree fragment
                                        */

    if (subquery->ops_vmflag)		/* TRUE if varmap is up-to-date */
    {
#ifdef E_OP0388_VARBITMAP
	/* check if this var map is valid as claimed */
#ifdef xDEBUG
	MEfill(sizeof(map), 0, (char *)&map);
	opv_mapvar(subquery->ops_root->pst_left, &map);
	if (MEcmp((char *)&map, (char *)&subquery->ops_root->pst_sym.pst_value.
		pst_s_root.pst_lvrm, sizeof(map)) != 0)
	    opx_error( E_OP0388_VARBITMAP); /* bit map
					** inconsistent with left side */
	MEfill(sizeof(map), 0, (char *)&map);
	opv_mapvar(subquery->ops_root->pst_right, &map);
	if (MEcmp((char *)&map, (char *)&subquery->ops_root->pst_sym.pst_value.
		pst_s_root.pst_rvrm, sizeof(map)) != 0)
	    opx_error( E_OP0388_VARBITMAP); /* bit map
					** inconsistent with right side */
#endif
	return;
#endif
    }

    if ((subquery->ops_sqtype == OPS_FAGG)
	||
	(subquery->ops_sqtype == OPS_HFAGG)
	||
	(subquery->ops_sqtype == OPS_RFAGG)
	)
    {
	/* map the bylist for the function aggregate */
	MEfill(sizeof(map), 0, (char *)&map);
	opv_mapvar(subquery->ops_agg.opa_byhead->pst_left, &map); /* map
					** the bylist portion of the function
					** aggregate */
	MEcopy((char *)&map, sizeof(map), (char *)&subquery->ops_agg.opa_blmap);
	if (subquery->ops_root->pst_left == subquery->ops_agg.opa_byhead)
	{
	    OPV_GBMVARS         aopmap;	/* map of AOP operator
					    */
	    MEfill(sizeof(aopmap), 0, (char *)&aopmap);
	    opv_mapvar(subquery->ops_agg.opa_aop, &aopmap); /* map the AOP node
					    ** of the function aggregate */
	    MEcopy((char *)&map, sizeof(map),
		(char *)&subquery->ops_root->pst_sym.pst_value.pst_s_root.pst_lvrm);
	    BTor(OPV_MAXVAR, (char *)&aopmap, 
		(char *)&subquery->ops_root->pst_sym.pst_value.pst_s_root.pst_lvrm);
	}
	else
	{   /* non-printing resdoms exist above the byhead so 
	    ** the entire tree needs to be scanned */
	    MEfill(sizeof(PST_J_MASK), 0, 
		(char *)&subquery->ops_root->pst_sym.pst_value.pst_s_root.pst_lvrm);
	    opv_mapvar(subquery->ops_root->pst_left, 
		&subquery->ops_root->pst_sym.pst_value.pst_s_root.pst_lvrm ); 
					/* map the AOP node
					** of the function aggregate */
	}
    }
    else
    {
	/* map the left side of the tree */
	MEfill(sizeof(map), 0, (char *)&map);
	opv_mapvar(subquery->ops_root->pst_left, &map);
	MEcopy((char *)&map, sizeof(map),
	    (char *)&subquery->ops_root->pst_sym.pst_value.pst_s_root.pst_lvrm);
    }

    /* map the right side of the tree */
    MEfill(sizeof(map), 0, (char *)&map);
    opv_mapvar(subquery->ops_root->pst_right, &map);
    MEcopy((char *)&map, sizeof(map),
	(char *)&subquery->ops_root->pst_sym.pst_value.pst_s_root.pst_rvrm);

    subquery->ops_vmflag = TRUE;	/* bit maps are now valid */
}
