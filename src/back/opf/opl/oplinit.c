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
#define             OPLINIT         TRUE
#include    <bt.h>
#include    <me.h>
#include    <opxlint.h>

/**
**
**  Name: OPLINIT.C - initialization routines for outer join support
**
**  Description:
**      Routines used to init outer join structures 
**
{@func_list@}...
**
**
**  History:
**      14-sep-89 (seputis)
**          initial creation
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
[@history_template@]...
**/

/*{
** Name: opl_ioutjoin	- init outer join IDs for this subquery
**
** Description:
**      This routine will initialize an outer join descriptor for 
**      this subquery.  Note that a PSF var may be referenced in several
**	subqueries due to link backs in aggregates, etc. but only
**	one of these subqueries would contain the outer join ID.  So
**	that OPF delays inserting the join ID into the
**	subquery until a parse tree node is discovered which references
**	the join ID.
**
** Inputs:
**      subquery                        subquery which contains outer
**					join
**      varp                            ptr to range table entry
**					containing the outer join var
**
** Outputs:
**
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-sep-89 (seputis)
**          initial creation
**      15-feb-93 (ed)
**	    fix outer join placement bug 49317
**	19-jan-94 (ed)
**	    remove obsolete structures
**	15-sep-00 (inkdo01)
**	    Init opl_translate to NOINIT to distinguish from legitimate
**	    parent of OPL_NOOUTER (to fix bug 87175).
[@history_line@]...
[@history_template@]...
*/
OPL_IOUTER
opl_ioutjoin(
	OPS_SUBQUERY       *subquery,
	PST_J_ID	   joinid)
{
    OPS_STATE	    *global;
    OPL_OUTER	    *outjoinp;

    global = subquery->ops_global;
    {   /* this ID has not been added to the current set */
	i4		    ojsize; /* size of structures needed to support
				    ** the outer join descriptor */

	ojsize = sizeof(*outjoinp) +
		sizeof(*outjoinp->opl_innereqc) +
		sizeof(*outjoinp->opl_ojtotal) +
		sizeof(*outjoinp->opl_onclause) +
		sizeof(*outjoinp->opl_ovmap) +
		sizeof(*outjoinp->opl_ivmap) +
		sizeof(*outjoinp->opl_bvmap) +
		sizeof(*outjoinp->opl_ojbvmap) +
		sizeof(*outjoinp->opl_idmap) +
		sizeof(*outjoinp->opl_bfmap) +
		sizeof(*outjoinp->opl_minojmap) +
		sizeof(*outjoinp->opl_maxojmap) +
		sizeof(*outjoinp->opl_ojattr) +
		sizeof(*outjoinp->opl_reqinner);
	outjoinp = (OPL_OUTER *)opu_memory(global, ojsize);
	MEfill(ojsize, (u_char)0, (PTR)outjoinp);
	outjoinp->opl_innereqc = (OPE_BMEQCLS *)&outjoinp[1]; /* ptr to set of equivalence classes
				    ** which represent all the inner relations
				    ** to this outer join */
	outjoinp->opl_id = subquery->ops_oj.opl_lv++; /* query tree ID associated with this
				    ** outer join, which should be found in
				    ** the op node of any qualification */
	outjoinp->opl_gid = joinid; /* global ID used in query tree produced
				    ** by parser, this is different from opl_id
				    ** since opl_id is local to the subquery
				    ** and the op nodes were renamed to 
				    ** be opl_id */
	outjoinp->opl_ojtotal= (OPV_BMVARS *)&outjoinp->opl_innereqc[1]; /* map of 
				    ** all variables which  are considered 
				    ** "inner" to this join ID, this map is used
				    ** for legal placement of variables, this
				    ** includes all secondaries etc. */
	outjoinp->opl_onclause = (OPV_BMVARS *)&outjoinp->opl_ojtotal[1]; ;
	outjoinp->opl_ovmap = (OPV_BMVARS *)&outjoinp->opl_onclause[1]; 
	outjoinp->opl_ivmap = (OPV_BMVARS *)&outjoinp->opl_ovmap[1]; 
	outjoinp->opl_bvmap = (OPV_BMVARS *)&outjoinp->opl_ivmap[1]; 
	outjoinp->opl_ojbvmap = (OPV_BMVARS *)&outjoinp->opl_bvmap[1]; 
	outjoinp->opl_idmap = (OPL_BMOJ *)&outjoinp->opl_ojbvmap[1]; 
	outjoinp->opl_bfmap = (OPB_BMBF *)&outjoinp->opl_idmap[1];
	outjoinp->opl_minojmap = (OPV_BMVARS *)&outjoinp->opl_bfmap[1];
	outjoinp->opl_maxojmap = (OPV_BMVARS *)&outjoinp->opl_minojmap[1];
	outjoinp->opl_ojattr = (OPZ_BMATTS *)&outjoinp->opl_maxojmap[1];
	outjoinp->opl_reqinner = (OPL_BMOJ *)&outjoinp->opl_ojattr[1];
	outjoinp->opl_type = OPL_UNKNOWN; /* type of outer join will
				    ** be determined later */
	outjoinp->opl_mask = 0;	    /* mask of various booleans */
	outjoinp->opl_translate = OPL_NOINIT;

	if ((subquery->ops_oj.opl_lv >= OPL_MAXOUTER)
	    ||
	    (joinid > global->ops_goj.opl_glv))
	    opx_error(E_OP038E_MAXOUTERJOIN);
	subquery->ops_oj.opl_base->opl_ojt[outjoinp->opl_id] = outjoinp;
	if (joinid == PST_NOJOIN)
	    return (outjoinp->opl_id); /* in the case of TID joins this routine
				    ** would be called to create a new joinid
				    ** descriptor for a left join */
	if (joinid > global->ops_goj.opl_glv)
	    opx_error(E_OP0395_PSF_JOINID);	    /* join id is out of range */
	if (global->ops_goj.opl_gbase->opl_gojt[joinid] != OPL_NOOUTER)
	    opx_error(E_OP038F_OUTERJOINSCOPE); /* not expecting another translation
				    ** for the same outer join */
	BTset( (i4)joinid, (char *)&subquery->ops_oj.opl_jmap); /* mark
				    ** outer join ID as being processed */
	global->ops_goj.opl_gbase->opl_gojt[joinid] = outjoinp->opl_id;
    }
    {	/* traverse range table and check for any variables which reference
	** this join id */
	OPV_IVARS	    varno;
	OPV_RT              *vbase;	    /* ptr to base of array of ptrs
					** to joinop variables */
	OPL_PARSER	    *pouter;
	OPL_PARSER	    *pinner;

	vbase = subquery->ops_vars.opv_base; /* init ptr to base of array of
					** ptrs to joinop variables */
	pouter = subquery->ops_oj.opl_pouter;
	pinner = subquery->ops_oj.opl_pinner;
	for (varno = subquery->ops_vars.opv_rv; --varno>=0;)
	{
	    OPV_VARS		*varp;
	    varp = vbase->opv_rt[varno];
	    if (varp->opv_grv 
/*
		&&
		(varp->opv_grv->opv_gmask & OPV_GOJVAR)
OPV_GOJVAR is not reliably set 
*/
		)
	    {
		OPV_IGVARS	    gvar;
		gvar = varp->opv_gvar;	    /* outer join semantics are defined with
					    ** the primary */
		if (BTtest((i4)joinid, (char *)&pouter->opl_parser[gvar]))
		{   /* found variable which is an outer to this join id */
		    if (!varp->opv_ojmap)
		    {   /* allocate a bit map for the outer join */
			varp->opv_ojmap = (OPL_BMOJ *)opu_memory(global,
			    (i4)sizeof(*varp->opv_ojmap));
			MEfill(sizeof(*varp->opv_ojmap), (u_char)0,
			    (PTR)varp->opv_ojmap);
		    }
		    BTset((i4)outjoinp->opl_id, (char *)varp->opv_ojmap);
		}
		if (BTtest((i4)joinid, (char *)&pinner->opl_parser[gvar]))
		{   /* found variable which is an inner to this join id */
		    if (!varp->opv_ijmap)
		    {   /* allocate a bit map for the outer join */
			varp->opv_ijmap = (OPL_BMOJ *)opu_memory(global,
			    (i4)sizeof(*varp->opv_ijmap));
			MEfill(sizeof(*varp->opv_ijmap), (u_char)0,
			    (PTR)varp->opv_ijmap);
		    }
		    BTset((i4)outjoinp->opl_id, (char *)varp->opv_ijmap);
		    BTset((i4)varno, (char *)outjoinp->opl_ojtotal);
		}
	    }
	}
    }
    return (outjoinp->opl_id);
}

/*{
** Name: opl_ojinit	- initialize some data structures for outer joins
**
** Description:
**      This routine needs to be called prior to the information gathering 
**      pass of opz_ctree, since opz_ctree will set bitmaps for outer join 
**      processing 
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**
** Outputs:
**
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      7-nov-89 (seputis)
**          initial creation
**      22-apr-2004 (huazh01)
**          Union query shares the same opl_goit[]. Do 
**          initialize it for more than once. 
**          b111536, INGSRV2651. 
**	4-apr-05 (inkdo01)
**	    Removed above change because it causes OP038F errors as 
**	    reported in issue 14052151;1. Will continue to investigate
**	    the original bug.
[@history_template@]...
*/
VOID
opl_ojinit(subquery)
OPS_SUBQUERY       *subquery;
{
    OPL_BMOJ	    ojmap;	/* map of outer join ID's referenced in
				** this subquery */
    OPV_RT	    *vbase;
    OPV_IVARS	    varno;
    OPL_IOUTER	    ojid;
    OPS_STATE	    *global;

    global = subquery->ops_global;
    subquery->ops_oj.opl_where = (OPV_BMVARS *)
	opu_memory(global, (i4)sizeof(*subquery->ops_oj.opl_where));
    MEfill((i4)sizeof(subquery->ops_oj.opl_jmap),
	(u_char)0, (PTR)&subquery->ops_oj.opl_jmap);
    MEfill((i4)sizeof(*subquery->ops_oj.opl_where),
	(u_char)0, (PTR)subquery->ops_oj.opl_where);
    MEfill((i4)sizeof(ojmap), (u_char)0, (char *)&ojmap);
    subquery->ops_oj.opl_agscope = (OPV_BMVARS *)NULL;
    vbase = subquery->ops_vars.opv_base;
    for (varno = subquery->ops_vars.opv_prv; --varno>=0;)
    {
	OPV_IGVARS	gvar;
	gvar = vbase->opv_rt[varno]->opv_gvar;
	if (gvar >= 0)
	    BTor((i4)BITS_IN(ojmap), (char *)&subquery->ops_oj.opl_pinner->opl_parser[gvar],
		(char *)&ojmap);	/* get a map of all outer joins ids referenced in
				** this range table */
    }
    for (ojid = -1; (ojid = BTnext((i4)ojid, (char *)&ojmap, (i4)BITS_IN(ojmap))) >= 0;)
    {
	if (!subquery->ops_oj.opl_base)
	{	/* first time an outer join has been allocated  for this subquery */
	    PST_J_ID	    numjoins;
	    subquery->ops_oj.opl_base = (OPL_OJT *)opu_memory(global,
		(i4)(sizeof(*subquery->ops_oj.opl_base) +
		      sizeof(*subquery->ops_oj.opl_subvar)
		      ) );
	    /* the opl_where map should already have been allocated */
	    subquery->ops_oj.opl_subvar = (OPV_BMVARS *)&subquery->ops_oj.opl_base[1];
	    numjoins = global->ops_qheader->pst_numjoins;
	    if (numjoins < global->ops_goj.opl_glv)
		numjoins = global->ops_goj.opl_glv;
	    if (!global->ops_goj.opl_gbase)
	    {
		i4		    size;

		size = sizeof(OPL_IOUTER) * (numjoins+1); /* PSF outer join IDs
					    ** start at 1 */
		global->ops_goj.opl_gbase = (OPL_GOJT *)opu_memory(global, size); 
					    /* allocate enough
					    ** for PSF translation table, which starts
					    ** at "1" rather and "0" index */

		/* global->ops_goj.opl_mask = 0; already initialized in opsinit */
		global->ops_goj.opl_zero = opv_i1const(global, 0); /* generate
					** an integer constant for boolean factor
					** modifications */
		global->ops_goj.opl_one = opv_i1const(global, 1); /* generate
					** an integer constant for boolean factor
					** modifications */
	    }
	    {
		do
		{
		    global->ops_goj.opl_gbase->opl_gojt[numjoins] = OPL_NOOUTER;
					    /* init all translation elements
					    ** to zero */
		}
		while    (--numjoins >= 0);
	    }
	}
	(VOID)opl_ioutjoin(subquery, ojid); /* assign a local joinid to this
				** global joinid */
    }
}
