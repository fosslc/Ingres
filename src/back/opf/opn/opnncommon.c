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
#include    <dmccb.h>
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
#define        OPN_NCOMMON      TRUE
#include    <me.h>
#include    <mh.h>
#include    <bt.h>
#include    <opxlint.h>


/**
**
**  Name: OPNNCOMMON.C - common routines between leaf and non-leaf nodecost
**
**  Description:
**      Routine in common between leaf and non-leaf portions of opn_nodecost
**
**  History:    
**      27-may-86 (seputis)    
**          initial creation
**      16-aug-91 (seputis)
**          add CL include files for PC group
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**      12-oct-2000 (stial01)
**          opn_pagesize() Fixed number of rows per page calculation
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**      28-apr-2004 (stial01)
**          Optimizer changes for 256K rows, rows spanning pages
[@history_line@]...
**/

/*{
** Name: opn_ncommon	- common routine between leaf and non-leaf (in nodecost)
**
** Description:
**      Routine in common between the leaf and non-leaf portions of 
**      opn_nodecost.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      nodep                           ptr to joinop node being processed
**      rlmp                            ptr to "temporary relation" descriptor
**      eqr                             bitmap of required equivalence classes
**                                      for this node
**      search                          indicates result of search of saved
**                                      work array i.e. does eqclp or subtp
**                                      or both need to be allocated
**      nleaves                         number of leaves in subtree represented
**                                      by nodep
**      selectivity                     selectivity of original relation prior
**                                      to applying boolean factors
**      iom                             interesting ordering bitmap required
**                                      by this node
**
** Outputs:
**      eqclp                           return ptr to OPN_EQS structure if not
**                                      allocated
**      subtp				return ptr to OPN_SUBTREE structure if
**                                      not allocated
**      blocks                          estimated number of blocks in relation
**      tuples                          estimated number of tuples in relation
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	27-may-86 (seputis)
**          initial creation
**	11-apr-89 (seputis)
**          fix 0 tuple width problem
**      29-mar-90 (seputis)
**          fix byte alignment problems
**	27-jun-90 (seputis)
**	    fix b30785 - fix unix problem since MEcopy did not have the
**	    correct element size
**	06-mar-96 (nanpr01)
**	    Variable pagesize project. Calculate the tuples per block
**	    based on page size.
**	 5-mar-01 (inkdo01 & hayke02)
**	    Changes to support new RLS/EQS/SUBTREE structures. This change
**	    fixes bug 103858.
**	11-Mar-2008 (kschendel) b122118
**	    Kill unused filtermap arg.
[@history_line@]...
*/
VOID
opn_ncommon(
	OPS_SUBQUERY       *subquery,
	OPN_JTREE          *nodep,
	OPN_RLS            **rlmp,
	OPN_EQS            **eqclp,
	OPN_SUBTREE        **subtp,
	OPE_BMEQCLS        *eqr,
	OPN_SRESULT        search,
	OPN_LEAVES         nleaves,
	OPO_BLOCKS         *blocks,
	bool               selapplied,
	OPN_PERCENT        selectivity,
	OPO_TUPLES         *tuples)
{
    OPN_RLS             *trl;	    /* ptr to temp relation descriptor */
    bool                rootflag;   /* TRUE - if at root */
    i4		pagesize;

    rootflag = subquery->ops_global->ops_estate.opn_rootflg
	&&
	(subquery->ops_global->ops_estate.opn_sroot->opn_nleaves != 1);

    trl = *rlmp;
    if ((search <= OPN_EQRLS) && (!rootflag))
	MEcopy((PTR)&nodep->opn_rlmap, sizeof(trl->opn_relmap), 
	    (PTR)&trl->opn_relmap);

    {
	OPO_TUPLES         tups;        /* estimated number of tuples in rel*/

	tups = trl->opn_reltups;
	if (!selapplied)
	    tups *= selectivity;	/* if we are at the root then
					** trl->opn_reltups is that of
					** original relation so
					** multiply by selectivity
					** before calculating number of
					** blocks in new relation */
	if (tups < 1.0)
	    tups = 1.0;			/* minimum of one tuple */
	*tuples = tups;
    }

    if (search <= OPN_RLSONLY)
    {   /* RLSONLY means only the OPN_RLS structure was found */
	OPN_EQS             *eqsp;	/* allocate and init this structure */

	eqsp = *eqclp = opn_ememory(subquery); /* allocate memory */
        eqsp->opn_eqnext = NULL;
	eqsp->opn_subtree = NULL;
	MEcopy((PTR)eqr, sizeof(eqsp->opn_maps.opo_eqcmap), (PTR)&eqsp->opn_maps.opo_eqcmap);
	eqsp->opn_relwid = ope_twidth (subquery, eqr);/* calculate tuple width*/
	if (eqsp->opn_relwid < 1)
	    eqsp->opn_relwid = 1;	/* for cartesean products it is possible
					** to get a zero tuple width here */

	opn_reformat(subquery, *tuples, eqsp->opn_relwid, &eqsp->opn_dio, 
	    &eqsp->opn_cpu);		/* determine the cost of sorting the 
					** tuples at this node and place
					** in OPN_EQS structure 
					** - for now ignore reformats to
					** isam, btree, hash */

    }

    {   /* this point would not have been reached if we did not require at
        ** least an allocation of the subtree structure */
	OPN_SUBTREE            *subtreep; /* allocate and init this structure */

	subtreep = *subtp = opn_smemory(subquery); /* allocate memory */
	subtreep->opn_coforw = subtreep->opn_coback = (OPO_CO *) subtreep; /*
					** no CO tree in list yet */
        subtreep->opn_stnext = NULL;
	subtreep->opn_histogram = (OPH_HISTOGRAM *) NULL;
	subtreep->opn_reltups = -1;
	subtreep->opn_eqsp = *eqclp;
	subtreep->opn_aggnext = subtreep;
	if (!rootflag)
	{
            OPN_CTDSC          *sourp;	    /* ptr to tree structure element */
	    OPN_CTDSC          *destp;      /* destination of element */

	    MEcopy ((PTR)nodep->opn_rlasg, nleaves * sizeof(subtreep->opn_relasgn[0]),
		(PTR)subtreep->opn_relasgn );
            sourp = nodep->opn_sbstruct;    /* copy tree structure */
	    destp = subtreep->opn_structure;
	    while ( *destp++ = *sourp++ );  /* structure is 0 terminated */
	}
    }
    {
        /* Calculate optimal page size */
        OPV_IVARS       varno;
 
        varno = BTnext((i4)-1, (char *)&trl->opn_relmap,
            (i4)BITS_IN(trl->opn_relmap));
        if (varno != -1)
	  if (subquery->ops_vars.opv_base->opv_rt[varno]->
				  opv_grv->opv_relation)
	    pagesize = subquery->ops_vars.opv_base->opv_rt[varno]->
			opv_grv->opv_relation->rdr_rel->tbl_pgsize; 
	  else
	    pagesize = opn_pagesize(subquery->ops_global->ops_cb->ops_server,
				    (*eqclp)->opn_relwid);
	else
	  pagesize = opn_pagesize(subquery->ops_global->ops_cb->ops_server,
				    (*eqclp)->opn_relwid);
    }
    *blocks = (f4) MHceil ( (f8)((*tuples) / 
			   opn_tpblk(subquery->ops_global->ops_cb->ops_server,
				     pagesize, (*eqclp)->opn_relwid)) );
				    /* blocks = (number of tuples)/
				    **   (number of tuples per block) */

#   ifdef xNTR1
    if (tTf(38, 6))
	TRdisplay("nodecommon blocks:%.2f tups:%.2f\n", *blocks, tups);
#   endif

}

/*{
** Name: opn_pagesize - Determine the page size of table based on
**                      row width.
**
** Description:
**      This routine takes tuple width as input, looks at the available
**      page_sizes and picks the appropriate one. If none available,
**      returns an error.
**
** Inputs:
**      width                           takes tuple width.
**
** Outputs:
**      pagesize                        Page Size is returned.
**
**      Returns:
**         None.
**      Exceptions:
**         None.
**
** Side Effects:
**          none
**
** History:
**      06-mar-1996 (nanpr01)
**          Created for multiple page size support and increased tuple size.
**	    For STAR - Just get the best fit for larger tuples. This is
**	    required for star if the sites have different capabilities.
[@history_template@]...
*/
i4
opn_pagesize(
	OPG_CB		*opg_cb,
	OPS_WIDTH       width)
{
	DB_STATUS       status;
	i4		page_sz, pagesize;
	i4             i;
	i4             noofrow, prevnorow;
	i4		page_type;
	i4		db_maxpagesize = DB_MINPAGESIZE;
 
	/*
	** Now find best fit for the row
	** We will try to fit atleast 20 row in the buffer
	*/
	noofrow = prevnorow = 0;
	pagesize = 0;
	for (i = 0, page_sz = DB_MINPAGESIZE; i < DB_NO_OF_POOL; 
		i++, page_sz *= 2)
	{
	  /* Continue if cache not configured */
	  if (opg_cb->opg_tupsize[i] == 0)
            continue;
 
	  db_maxpagesize = page_sz;
	  if (page_sz == DB_COMPAT_PGSIZE)
	    page_type = DB_PG_V1;
	  else
	    page_type = DB_PG_V2;

	  noofrow = ((i4)(page_sz - DB_VPT_PAGE_OVERHEAD_MACRO(page_type))/
	    (width + DB_VPT_SIZEOF_LINEID(page_type) + 
	    DB_VPT_SIZEOF_TUPLE_HDR(page_type)));

          /*
          ** 20 row is arbitrary as discussed and proposed by Dick Williamson
	  ** 1/8/1996 - Changed for STAR to 1 ie best fit so that multiple
	  ** sites with diff capabilities may also work.
          */
          if (noofrow >= 1)
          {
            pagesize = page_sz;
            break;
          }
          else
          {
            if (noofrow > prevnorow)
            {
                prevnorow = noofrow;
                pagesize = page_sz;
            }
          }
	}
	if (noofrow == 0)
	{
          /*
          ** FIXME : cannot fit in buffer return error.
          ** Though this error should be caught before.
          */
	  /*
	  opx_error(E_OP0009_TUPLE_SIZE);
	  */
	  /* Must be rows spanning page */
	  pagesize = db_maxpagesize;
	}
	return(pagesize);
}

