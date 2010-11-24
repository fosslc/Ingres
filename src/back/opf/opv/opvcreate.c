/*
**Copyright (c) 2004, 2010 Ingres Corporation
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
#define        OPV_CREATE      TRUE
#include    <me.h>
#include    <bt.h>
#include    <opxlint.h>


/**
**
**  Name: OPVCREATE.C - allocate the joinop range table
**
**  Description:
**      Contains routines to do initialization the joinop range table
**
**  History:    
**      11-jun-86 (seputis)    
**          initial creation from getqry.c
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
void opv_submap(
	OPS_SUBQUERY *subquery,
	OPV_GBMVARS *globalmap);
void opv_create(
	OPS_SUBQUERY *subquery);

/*{
** Name: opv_submap	- subquery map of valid range variables
**
** Description:
**      This routine will calculate the actual from list, eliminating 
**      references to simple aggregate variables, and including variables 
**      are not in the from list but are referenced in the query tree
**      as sometimes happens in quel 
**
** Inputs:
**      subquery                        subquery to calculate from list
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      27-mar-89 (seputis)
**          initial creation
**	3-dec-02 (inkdo01)
**	    Range table expansion - change int ops to BTs.
[@history_template@]...
*/
VOID
opv_submap(
	OPS_SUBQUERY       *subquery,
	OPV_GBMVARS        *globalmap)
{
    opv_smap(subquery);             /* map the subquery if necessary */

    MEcopy((char *)&subquery->ops_aggmap, sizeof(*globalmap), 
				(char *)globalmap);
    BTnot(OPV_MAXVAR, (char *)globalmap);
    BTand(OPV_MAXVAR, (char *)&subquery->ops_root->pst_sym.pst_value.
		pst_s_root.pst_tvrm, (char *)globalmap);
				    /* remove variables in FROM list which
				    ** are used in aggregates since substitution
                                    ** may have occurred
                                    ** - the from list is used here to ensure
                                    ** any variables which are not referenced
                                    ** in the main query, are still included
                                    ** as a cartesean product
				    ** - for SQL the range variables are
                                    ** determined by the from list, since a
                                    ** variable does not need to be referenced
                                    ** directly in the query */
    BTor(OPV_MAXVAR, (char *)&subquery->ops_root->pst_sym.pst_value.
		pst_s_root.pst_lvrm, (char *)globalmap);
    BTor(OPV_MAXVAR, (char *)&subquery->ops_root->pst_sym.pst_value.
		pst_s_root.pst_rvrm, (char *)globalmap);
				    /* map of variables in subquery (with
				    ** respect to global range table) */
    /* need to include left and right maps because aggregate temporaries
    ** will not be in the from list */

}

/*{
** Name: opv_create	- create the joinop range table
**
** Description:
**      The procedure will initialize the joinop range table variables.
**      This routine needs to be called prior to any references to this 
**      range table.
**
** Inputs:
**	subquery				ptr to subquery being analyzed
**
** Outputs:
**      subquery->ops_vars                      initialize vars structure
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
**          initial creation from getqry
**	26-dec-90 (seputis)
**	    fix access violation in OPC
**      14-sep-92 (ed)
**          - fix 44899 - on update/insert/delete target table not referenced
**	    in the query plan, result in enumerations being bypassed when the
**	    tuple estimate is used in locking... force enumeration in these
**	    cases
**      18-sep-92 (ed)
**          - bug 44850 - changed mechanism for global to local mapping
**	3-dec-02 (inkdo01)
**	    Range table expansion - change int ops to BTs.
**      12-jun-2009 (huazh01)
**          Fill in opv_lrvbase[] for variables referenced in the subselect
**          query. This prevents E_OP0491 error. (b120508)
[@history_line@]...
*/
VOID
opv_create(
	OPS_SUBQUERY       *subquery)
{
    OPV_GBMVARS	       globalmap;   /* map of global range table elements
				    ** used in the subquery
				    */
    OPV_GBMVARS        submap;      /* map of subselects referenced in this
                                    ** subquery */
    OPV_IGVARS	       globalrv;    /* index into the global range variable
				    ** table
				    */
    OPS_STATE          *global;     /* ptr to global state variable */
    OPV_IGVARS	       resultrel;  /* global var number of result relation */
    OPV_GRV            *resultvarp; /* ptr to global range var element 
                                    ** representing the result */
    OPV_IVARS		subselect; /* local range variable assigned
					** to subselect evaluation */
    OPV_SUBSELECT      *subsel_qry; 
    OPV_IVARS          local;
    i4                 i, opv_rv; 
    OPV_GRV            *varp;
 

    subselect = OPV_NOVAR;	    /* set to local range var only if
                                    ** a subselect is referenced */
    global = subquery->ops_global;  /* get ptr to global state variable */

    MEfill( sizeof(OPV_RANGEV), (u_char)0, (PTR)(&subquery->ops_vars));

    opv_submap(subquery, &globalmap); /* get map of variables in from list */

    MEcopy((char *)&globalmap, sizeof(globalmap), 
	(char *)&subquery->ops_vars.opv_gbmap); /* init the global range variables
                                    ** which are used in the query so that if an
                                    ** index is explicitly referenced, that it
                                    ** will not conflict with an internal use
                                    ** of the index */
    if (global->ops_mstate.ops_trt)
	subquery->ops_vars.opv_base = global->ops_mstate.ops_trt;	
				    /* use existing array of ptrs if available */
    else
    {
	global->ops_mstate.ops_trt = 
	subquery->ops_vars.opv_base = (OPV_RT *) opu_memory( subquery->ops_global, 
		    (i4) sizeof(OPV_RT) ); /* get memory for array of pointers
                                    ** to range table elements */
    }

    /* reset result relation number, it will be set if the
    ** result relation is referenced in the query */
    resultrel = subquery->ops_result;
    if (resultrel != OPV_NOGVAR)
    {
	resultvarp = global->ops_rangetab.opv_base->opv_grv[resultrel];
	global->ops_rangetab.opv_lrvbase[resultrel] = OPV_NOVAR;
    }
    else
	resultvarp = (OPV_GRV *)NULL;
    MEcopy((char *)&globalmap, sizeof(globalmap), (char *)&submap);
    BTand(OPV_MAXVAR, (char *)&global->ops_rangetab.opv_msubselect,
					(char *)&submap);
    if (global->ops_qheader->pst_subintree
	&&
	BTcount((char *)&submap, OPV_MAXVAR) != 0
       )
    {	/* at least one subselect has been found 
	** - only assign one local range variable for all subselects, and later
        ** assign more, if some subselects do not need to be computed together
        ** - the idea is that it is easier to add local range variables instead
        ** of removing them later, if subselects need to be computed together
        */
	OPV_IGVARS              grv;    /* global range var of subselect */
	OPV_VARS                *lrvp;  /* ptr to local range variable
                                        ** which represents all the subselects
                                        */
	for ( grv = -1;
	      (grv = BTnext( (i4)grv, 
			    (char *)&submap, 
			    (i4)BITS_IN(OPV_GBMVARS)
			   )
	       ) 
               >= 0;
	    )
	{   
	    BTclear((i4)grv, (char *)&globalmap); /* clear global map of
                                        ** all references to subselect
                                        ** nodes */
	    if (subselect == OPV_NOVAR)
	    {	/* first time get a range variable */
		(VOID) opv_relatts( subquery, grv, OPE_NOEQCLS, OPV_NOVAR);
		subselect = global->ops_rangetab.opv_lrvbase[grv];
		lrvp = subquery->ops_vars.opv_base->opv_rt[subselect];
		subquery->ops_joinop.opj_select = subselect; /* save the joinop
                                        ** range variable used to contain the
                                        ** subselect attributes */
	    }
	    else
	    {	/* subsequent time only need to update the map */
		global->ops_rangetab.opv_lrvbase[grv]
		    = subselect;	/* update mapping used to reassign
                                        ** local range variables */
	    }
	    opv_virtual_table(subquery, lrvp, grv); /* initialize the 
					** opv_subselect structure */
	}
    }

    {
	OPV_BMVARS	keep_dups;	/* map of variables which need
                                        ** duplicates kept but does not
                                        ** have TIDs available */
	bool		onevar;		/* TRUE if at least one variable
					** exists */
	if (subquery->ops_keep_dups)
	{
	    MEfill(sizeof(keep_dups), (u_char)0, (PTR)&keep_dups);
	    subquery->ops_remove_dups = (OPV_BMVARS *)opu_memory(global,
		    (i4)sizeof(*subquery->ops_remove_dups));
	    MEfill(sizeof(subquery->ops_remove_dups), (u_char)0, 
		    (PTR)subquery->ops_remove_dups);

	}
	onevar = FALSE;
	for (globalrv = -1;
		(globalrv = BTnext( (i4)globalrv, 
				    (char *)&globalmap, 
				    (i4)BITS_IN(OPV_GBMVARS)
				  )
		) >= 0;
	    )
	{   
	    /* create "local" range variable elements for all global range
	    ** variables referenced in the query
	    */
	    onevar = TRUE;
	    local = opv_relatts( subquery, globalrv, OPE_NOEQCLS, OPV_NOVAR);
	    if (subquery->ops_keep_dups)
	    {
		if (BTtest((i4)globalrv, (char *)subquery->ops_keep_dups))
		    BTset((i4)local, (char *)&keep_dups); /* translate the
					** keep dup global range number to a
                                        ** local range number */
		else if (!BTtest((i4)globalrv, (char *)&subquery->ops_agg.opa_keeptids))
		    BTset((i4)local, (char *)subquery->ops_remove_dups); /*
					** if TIDs are not to be kept then
                                        ** duplicates must be removed for this
                                        ** query */
	    }
	}

        /* b120508 */
        if (subquery->ops_joinop.opj_virtual)
        {
           opv_rv = subquery->ops_vars.opv_rv; 
           for (i = 0; i < opv_rv; i++)
           {
               varp = subquery->ops_vars.opv_base->opv_rt[i]->opv_grv; 
               if (varp->opv_created == OPS_SELECT &&
                   varp->opv_gsubselect)
               {
                  for (subsel_qry = subquery->ops_vars.opv_base->
                          opv_rt[i]->opv_subselect;  
                       subsel_qry; 
                       subsel_qry = subsel_qry->opv_nsubselect)
                  {
                       OPV_SEPARM	*gparmp;
                       OPV_GRV          *grvp;

                       grvp = subquery->ops_global->ops_rangetab.opv_base->
                                 opv_grv[subsel_qry->opv_sgvar];
	   
                       for (gparmp = grvp->opv_gsubselect->opv_parm;
                            gparmp;
                            gparmp = gparmp->opv_senext) 
	               {
                           if (subquery->ops_global->ops_rangetab.opv_lrvbase
		              [gparmp->opv_sevar->pst_sym.pst_value.pst_s_var.pst_vno] 
                                  == OPV_NOVAR)
		               local = opv_relatts(subquery, 
				  gparmp->opv_sevar->pst_sym.pst_value.pst_s_var.pst_vno, 
				  OPE_NOEQCLS, 
				  OPV_NOVAR); 
 	               }
                  }
               }
           }
        }

	if (onevar
	    &&
	    (subquery->ops_sqtype == OPS_MAIN)
	    )
	{
	    OPV_IGVARS	    resultvar;
	    resultvar = global->ops_qheader->pst_restab.pst_resvno;
	    if ((resultvar >= 0)
		&&
		!BTtest((i4)resultvar, (char *)&globalmap)
		)
		subquery->ops_enum = TRUE;	/* fix bug 44899, if at least one
						** table exists in the query but the
						** target relation is not in the
						** query, then go thru enumeration to
						** give an accurate tuple count which
						** is used by OPC to determine page
						** versus table locking */
	}
	if (subquery->ops_keep_dups)
	{
	    MEcopy((PTR)&keep_dups, sizeof(subquery->ops_keep_dups),
		(PTR)subquery->ops_keep_dups);
	}
    }
    if ((resultrel != OPV_NOGVAR) && resultvarp)
	subquery->ops_localres 
	    = global->ops_rangetab.opv_lrvbase[resultrel]; /*
					** set the result relation variable
                                        ** if it is referenced in the query
                                        ** - which should be true for replace
                                        ** and delete */

    subquery->ops_vars.opv_prv = subquery->ops_vars.opv_rv; /* save number of 
                                        ** primary range variables
                                        ** defined */

}
