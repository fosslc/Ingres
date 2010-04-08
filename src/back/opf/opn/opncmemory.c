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
#define             OPN_CMEMORY         TRUE
#define             OPO_INITCO          TRUE
#include    <opxlint.h>


/**
**
**  Name: OPNCMEMORY.C - get memory for a new co node
**
**  Description:
**      routine to get memory for a new co node
**
**  History:    
**      14-jun-86 (seputis)    
**          initial creation
**	15-dec-91 (seputis)
**	    make better guess for initial set of relations to enumerate
**      21-may-93 (ed)
**          - changed opo_var to be a union structure so that
**          the outer join ID can be stored in interior CO nodes
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**      06-mar-96 (nanpr01)
**          Initialize the new field in opo_cost structure.
[@history_line@]...
**/

/*{
** Name: opo_initco	- initialize a CO node
**
** Description:
**      Routine will initialize all fields of a CO node to default values
**
** Inputs:
**      cop                             ptr to co node to initialize
**
** Outputs:
**      cop                             co node fields will be initialized
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      24-aug-86 (seputis)
**          initial creation
**      06-mar-96 (nanpr01)
**          Initialize the new field in opo_cost structure.
**	13-Nov-2009 (kiria01) SIR 121883
**	    To distinguish between LHS and RHS card checks for 01 we have
**	    opo_card_* that need initialising.
*/
VOID
opo_initco(subquery, cop)
OPS_SUBQUERY	   *subquery;
OPO_CO             *cop;
{
    cop->opo_coforw = NULL;
    cop->opo_coback = NULL;
    cop->opo_sjpr = 0;
    cop->opo_storage = 0;
    cop->opo_outer = NULL;
    cop->opo_inner = NULL;
    cop->opo_cost.opo_cpu = 0.0;
    cop->opo_cost.opo_dio = 0.0;
    cop->opo_cost.opo_reltotb = 0.0;
    cop->opo_cost.opo_reduction = 0.0;
    cop->opo_cost.opo_cvar.opo_relprim = 0.0;
    cop->opo_cost.opo_pagestouched = 0.0;
    cop->opo_cost.opo_tups = 0.0;
    cop->opo_cost.opo_adfactor = 0.0;
    cop->opo_cost.opo_sortfact = 0.0;
    cop->opo_cost.opo_tpb = 0.0;
    cop->opo_cost.opo_pagesize = 0;
    cop->opo_ordeqc = OPE_NOEQCLS;
    cop->opo_sjeqc = OPE_NOEQCLS;
    cop->opo_union.opo_orig = OPV_NOVAR;
    cop->opo_maps = NULL;
    cop->opo_pointercnt = 0;
    cop->opo_deleteflag = FALSE;
    cop->opo_existence = FALSE;
    cop->opo_idups = OPO_DDEFAULT;
    cop->opo_odups = OPO_DDEFAULT;
    cop->opo_pdups = OPO_DDEFAULT;
    cop->opo_psort = FALSE;
    cop->opo_isort = FALSE;
    cop->opo_osort = FALSE;
    cop->opo_held = FALSE;
    cop->opo_card_own = 0;
    cop->opo_card_outer = 0;
    cop->opo_card_inner = 0;
    cop->opo_is_swapped = 0;
    if (subquery->ops_global->ops_cb->ops_smask & OPS_MDISTRIBUTED)
    {
	OPD_SCOST	*basep;	    /* ptr to array of costs be be initialized */
	OPD_ISITE	site;
	basep = cop->opo_variant.opo_scost;
	for (site = subquery->ops_global->ops_gdist.opd_dv;
	    --site >= 0;)
	    basep->opd_intcosts[site] = OPO_INFINITE;
    }
    else
	cop->opo_variant.opo_scost = NULL;
}

/*{
** Name: opn_cpmemory	- clone of opn_cmemory to get a co node in permanent
**	memory for the new enumeration algorithm
**
** Description:
**      See above. Doesn't link these guys like regular permanent memory.
**
** Inputs:
**      subquery                        ptr to current subquery being analyzed
**	cocount				count of OPO_PERM's to allocate
**
** Outputs:
**	Returns:
**	    ptr to free OPO_PERM structure
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	2-oct-02 (inkd001)
**	    Written (for new enumeration algorithm).
**	30-oct-04 (inkdo01)
**	    Added cocount parm to allow pre-allocation of a bunch.
[@history_line@]...
*/
OPO_PERM *
opn_cpmemory(
	OPS_SUBQUERY       *subquery,
	i4		   cocount)
{
    OPS_STATE           *global;	/* ptr to global state variable */
    OPO_CO              *cop;		/* ptr to newly allocated co node */
    OPO_PERM		*permanent;     /* structure contain a CO struct */
    i4			i;

    global = subquery->ops_global;      /* get global state variable */

    for (i = 0; i < cocount; i++)
    {
	permanent = (OPO_PERM *) opu_memory(global, sizeof(OPO_PERM));
	if (cocount > 1)
	{
	    /* If more than 1, these are for greedy enumeration fragments
	    ** and must be listed from "global". */
	    permanent->opo_next = global->ops_estate.opn_fragcolist;
	    global->ops_estate.opn_fragcolist = permanent;
	}
	cop = &permanent->opo_conode;	/* set ptr to CO struct in non-enum
                                        ** memory */
	cop->opo_variant.opo_scost = NULL;

	opo_initco(subquery, cop);
    }
    return (permanent);
}

/*{
** Name: opn_cmemory	- routine to get memory for a co node
**
** Description:
**      The routine will look in the free list of available co nodes and
**      obtain one.  Otherwise, get memory from stream.
**
** Inputs:
**      subquery                        ptr to current subquery being analyzed
**
** Outputs:
**	Returns:
**	    ptr to free OPO_CO node
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	14-jun-86 (seputis)
**          initial creation
**	6-nov-88 (seputis)
**          add ID counter for query plan tracing
[@history_line@]...
*/
OPO_CO *
opn_cmemory(
	OPS_SUBQUERY       *subquery)
{
    OPS_STATE           *global;	/* ptr to global state variable */
    OPO_CO              *cop;		/* ptr to newly allocated co node */

    global = subquery->ops_global;      /* get global state variable */
#if 0
    subquery->ops_tcurrent++;		/* increment ID counter so query
					** plans can be identified */
#endif
    if (global->ops_estate.opn_cocount < global->ops_estate.opn_corequired)
    {	/* make sure that enough CO nodes exist outside of enumeration memory
        ** before continuing, since an out-of-memory error may occur, which
        ** requires the CO tree to be copied, prior to query compilation 
        ** - use these special permanent CO nodes in normal way and copy CO 
        ** tree after enumeration, using linked list of special CO nodes 
        */
	OPO_PERM	*permanent;     /* structure contain a CO struct */

	permanent = (OPO_PERM *) opu_memory(global, sizeof(OPO_PERM));
	permanent->opo_next = global->ops_estate.opn_colist;
	global->ops_estate.opn_colist = permanent;  /* link structures so that can
					** be found for the CO tree copy */
	global->ops_estate.opn_cocount++; /* indicate another CO node has
                                        ** been allocated */
	cop = &permanent->opo_conode;	/* set ptr to CO struct in non-enum
                                        ** memory */
	cop->opo_variant.opo_scost = NULL;
    }
    else if (cop = global->ops_estate.opn_cofree)
    {
	global->ops_estate.opn_cofree = cop->opo_coforw; /* deallocate from
					** free list */
    }
    else
    {
	if ((4*global->ops_mstate.ops_mlimit) > global->ops_mstate.ops_memleft)
	{   /* check if memory limit has been reached prior to allocating
            ** another CO node from the stream...  in this case try to
            ** fetch unused CO nodes 
            ** - useless CO lists get fairly long so it is a good idea to
            ** try to reclaim these nodes fairly early
            */
	    opn_fmemory(subquery, (PTR *)&global->ops_estate.opn_cofree);
	    if (cop = global->ops_estate.opn_cofree)
	    {   /* at least one CO node has been found */
		global->ops_estate.opn_cofree = cop->opo_coforw; /* deallocate
					** from free list */
            }
	}
	if (!cop)
	{
	    cop = (OPO_CO *) opn_memory( global, sizeof( OPO_CO ) ); /* get new
					** element if none available */
	    cop->opo_variant.opo_scost = NULL;
	}
    }
    if ((global->ops_cb->ops_smask & OPS_MDISTRIBUTED)
	&&
	!cop->opo_variant.opo_scost)
    {   /* allocate site array memory */
	cop->opo_variant.opo_scost = (OPD_SCOST *) opn_memory( global, 
	    sizeof( cop->opo_variant.opo_scost->opd_intcosts[0]) * 
		global->ops_gdist.opd_dv);
    }
    opo_initco(subquery, cop);
    return (cop);
}
