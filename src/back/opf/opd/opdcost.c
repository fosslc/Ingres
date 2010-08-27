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
#define        OPD_COST	    TRUE
#include    <me.h>
#include    <bt.h>
#include    <opxlint.h>
/**
**
**  Name: OPDORIG.C - initialize an ORIG node for distributed
**
**  Description:
{@comment_line@}...
**
{@func_list@}...
**
**
**  History:
**	17-sep-90 (fpang)
**	   Casted dd_l2_node_name to DD_NAME *.	
**	25-feb-92 (barbara)
**	   Fixed bug 39403 in opd_machineinfo.
**	06-apr-92 (bryanp)
**	    Add support for PSQ_DGTT_AS_SELECT for temporary tables.
**      8-sep-92 (ed)
**          removed OPF_DIST as part of sybil project
**      21-may-93 (ed)
**          - changed opo_var to be a union structure so that
**          the outer join ID can be stored in interior CO nodes
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**      19-jan-94 (ed)
**          use site specific power factor for CPU and DIO
**	6-june-97 (inkdo01 from hayke02)
**	    In the function opd_msite(), we now call opn_cleanup() for as many
**	    sites that have a cost ordering node (OPO_COP) when calculating the
**	    costs of creating and retrieving into a temporary table. This
**	    change fixes bug 79697.
**	6-june-97 (inkdo01 from hayke02)
**	    In the function opd_msite(), we now check for the existence of
**	    the site query plan structure, subquery->ops_dist.opd_bestco,
**	    before calling opn_cleanup() for as many sites that have a cost
**	    ordering node. This change fixes bug 81003.
**	17-aug-99 (sarjo01)
**	    Bug 98108: Undo 09-apr-97 (inkdo01) change in opd_msite() to
**	    now allow better site selection for multisite UPDATE...FROM.
**	03-feb-00 (i4jo01)
**	    Bug 100335: A bad comparison of a float and a float sum was
**	    allowing a comparison to pass where it shouldn't have. Put
**	    in a temp floating point to get around this.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**
[@history_line@]...
[@history_template@]...
**/

/*{
** Name: opd_adjustcost	- adjust cost if site cost descriptor available
**
** Description:
**      If the site cost descriptor is available then adjust the cost
**	estimate 
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      nodecostp                       ptr to site cost descriptor
**      cpu                             cpu of operation
**      dio                             dio of operation
**
** Outputs:
**
**	Returns:
**	    adjusted cost of operation
**	Exceptions:
**
** Side Effects:
**
** History:
**      19-jan-94 (ed)
**          use node cost info if available
[@history_template@]...
*/
static OPO_COST
opd_adjustcost(
    OPS_SUBQUERY	*subquery, 
    DD_COSTS		*nodecostp, 
    OPO_CPU		cpu, 
    OPO_BLOCKS		dio)
{
    return(opn_cost(subquery, dio/nodecostp->dio_cost, cpu/nodecostp->cpu_power));
}

/*{
** Name: opd_msite	- mark best site from cost array 
**
** Description:
**      When a CO tree is copied, the site cost arrays are lost so 
**      a site is determined by this routine, and placed directly into 
**      the CO structure.  Also look at ops_copiedco which knows about 
**      copied CO trees, and the fact that the cost array no longer exists.
**      cop->opo_variant now will have a site rather than a ptr to an array
**      of site costs.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      cop                             ptr to CO node to determine site
**      target_site			site upon which result should be placed
**      root_flag                       TRUE if this is the root of the tree
**                                      being resolved, which means that
**                                      an intermediate does not need to be
**                                      created for the result
**      marker                          non-NULL means that there is a
**                                      possibility of the same node being
**                                      resolved twice, use this ptr to
**                                      avoid this situtation
**	remove				TRUE - if this is the final copy
**					of the best query plan out of enumeration
**					memory
**
** Outputs:
**	cop->opo_cost.opo_cvar.opo_network	will contain the total network cost
**					for non-ORIG nodes, this overloading
**					is done since the field is not used
**					by interior non-ORIG CO nodes, and
**					memory used by CO nodes should be kept
**					to a minimum
**	Returns:
**	    total networking cost required for temp table movement
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-may-88 (seputis)
**          initial creation
**      26-mar-91 (fpang)
**          Renamed opdistributed.h to opdstrib.h
**      17-jun-91 (seputis)
**          fix for b35205, make sure tuple width calculation matches
**	    that done in opnncommon.c
**	17-jul-91 (seputis)
**	    added the remove flag to fix problem cost resolution problem
**	    with star 
**	06-mar-96 (nanpr01)
**	    Consider the ldbcapabilities to make sure that the width can
**	    be handled in the target site.
**	9-apr-97 (inkdo01)
**	    Change DB_SJ to force join_site to be OPD_TSITE if top join and
**	    executing UPDATE/DELETE ('til we can get UPDATE from join on
**	    different site to work).
**	6-jun-97 (inkdo01 from hayke02)
**	    We now call opn_cleanup() for as many sites that have a cost
**	    ordering node (OPO_COP) when calculating the dio and cpu costs
**	    of creating and retrieving into a temporary table. This calculation
**	    now matches the one performed in opo_copyco(). The cost difference
**	    (costdiff) is now calculated correctly, preventing E_OP0A8C further
**	    on in the function. This change fixes bug 79697.
**	6-june-97 (inkdo01 from hayke02)
**	    We now check for the existence of the site query plan structure,
**	    subquery->ops_dist.opd_bestco, before calling opn_cleanup() for
**	    as many sites that have a cost ordering node. This change fixes
**	    bug 81003.
**      17-aug-99 (sarjo01)
**          Bug 98108: Undo 09-apr-97 (inkdo01) change in opd_msite() to
**          now allow better site selection for multisite UPDATE...FROM.
**      26-jun-01 (wanfr01)
**          Bug 105143, INGSTR 41
**          Only fail with E_OP0A8C if join_site == OPD_NOSITE.
[@history_line@]...
[@history_template@]...
*/
OPO_COST
opd_msite(
	OPS_SUBQUERY       *subquery,
	OPO_CO             *cop,
	OPD_ISITE	   target_site,
	bool		   root_flag,
	OPO_CO		   *marker,
	bool		   remove)
{
    OPD_SCOST   *costp;		/* ptr to array of costs for
				** materializing the current cop */
    OPO_COST	target_cost;	/* cost of having result on target site
                                */
    OPO_COST	network;	/* cost of moving result to new location 
				** if necessary */
    OPD_ISITE	source_site;	/* site on which the join temp
				** was created */
    OPO_COST	createcost;
    bool	final_target;	/* TRUE if temporary does not need to be
				** created */
    OPS_WIDTH	width;		/* width of tuple in bytes */
    OPS_STATE	*global;

    global = subquery->ops_global;

    if (marker && (marker == cop->opo_coback))
    	return(cop->opo_cost.opo_cvar.opo_network);   /* this node has already been resolved
				** so return the network cost */
    final_target = 
	root_flag		/* we are at the root node so that
				** an intermediate may not need to be created */
	&&
	(subquery->ops_sqtype == OPS_MAIN);	/* this is the main query so that
				** intermediate results may not be needed */

    if (final_target		/* if we are not at the top of the
				** plan then add the cost of the temporary
				** creation */
	&& 
	(target_site == OPD_TSITE) /* may avoid cost of creation
				** of the temporary in this case */
	&&
	(global->ops_gdist.opd_gmask & OPD_TOUSER) /* no site constraint for the
				** final target relation */
	)
	createcost = 0.0;
    else
	createcost = OPD_CREATE_COST; /* add cost to create a temporary
				** relation on the remote site */
    network = 0.0;		/* assume no network costs unless
				** a movement is required */
    if (!cop)
	opx_error(E_OP0491_NO_QEP); /* internal consistency check */
    switch (cop->opo_sjpr)
    {
    case DB_RSORT:
    case DB_PR:
    {
	if (final_target 
	    && 
	    (target_site == OPD_TSITE)
	    && 
	    (global->ops_gdist.opd_gmask & OPD_TOUSER))
	{   /* special case of node returning result to the user */
	    OPD_ISITE	    best_site;
	    OPO_COST	    best_cost;

	    best_cost = OPO_INFINITE;
	    source_site = OPD_NOSITE;
	    costp = cop->opo_variant.opo_scost;
	    width =  ope_twidth(subquery, &cop->opo_maps->opo_eqcmap);
	    if (width < 1)
		width = 1;		/* match width calculation in
					** opnncommon.c */
	    for (best_site = global->ops_gdist.opd_dv;
		--best_site >= 0;)
	    {   /* consider each site as a source for the join intermediate */
		OPO_COST	    temp_cost; /* cost of producing the temp */

		if ((global->ops_gdist.opd_base->opd_dtable[best_site]->
			    opd_dbcap) &&
		    (width > global->ops_gdist.opd_base->
			     opd_dtable[best_site]->opd_dbcap->dd_p7_maxtup))
		  continue;
		else
		  if (!global->ops_gdist.opd_base->opd_dtable[best_site]->
			    opd_dbcap) 
		    continue;
		temp_cost = costp->opd_intcosts[best_site];
		if (temp_cost > OPO_CINFINITE)
		    continue;	    /* site has already been removed from
				    ** consideration */
		if ((source_site == OPD_NOSITE)
		    ||
		    (best_cost > temp_cost)
		   )
		{
		    best_cost = temp_cost;
		    source_site = best_site;
		}
	    }
	    if (source_site == OPD_NOSITE)
		opx_error(E_OP0495_CO_TYPE);	/* FIXME - create a better error message */
	    cop->opo_variant.opo_site = target_site; /* project restrict reformat
				    ** are always done on the 
				    ** site required by the join */
	    network = opd_msite(subquery, cop->opo_outer, source_site, FALSE, 
		marker, remove);
	}
	else
	{
	    cop->opo_variant.opo_site = target_site; /* project restrict reformat
				    ** are always done on the 
				    ** site required by the join */
	    network = opd_msite(subquery, cop->opo_outer, target_site, FALSE, 
		marker, remove);
	    source_site = cop->opo_outer->opo_variant.opo_site;
	}	
	if (target_site != source_site)
	{   /* this project-restrict is required to be moved to a
	    ** new target site so calculate the cost of the move */
	    width =  ope_twidth(subquery, &cop->opo_maps->opo_eqcmap);
	    if (width < 1)
		width = 1;		/* match width calculation in
					** opnncommon.c */
	    network += createcost /* add cost to create a temporary
				** relation on the remote site */
		+ subquery->ops_global->ops_gdist.opd_base->
		    opd_dtable[source_site]->opd_factor->
		    opd_intcosts[target_site]
		* width
		* cop->opo_cost.opo_tups; /* add cost of moving data */
	}
	cop->opo_cost.opo_cvar.opo_network = network; /* save networking cost for
					** QEP printing routines here */
	break;
    }
    case DB_ORIG:
    {
	OPV_VARS		*varp;

	varp = subquery->ops_vars.opv_base->opv_rt[cop->opo_union.opo_orig];
	if (varp->opv_grv 
	    && 
	    varp->opv_grv->opv_gquery)
	{   /* this is a function aggregate or subselect so use
            ** the plan from the array of plans which is best
	    ** suited for this parent, FIXME - if multiple link backs
            ** of this subquery is made then it is only optimized for the
            ** first use */
	    OPS_SUBQUERY	*child_subquery;
	    child_subquery = varp->opv_grv->opv_gquery;
	    if (child_subquery->ops_dist.opd_target == OPD_NOSITE)
	    {
		/* first time using this subquery so mark the most
		** optimial site for evaluation of this temp */
		child_subquery->ops_dist.opd_target = target_site;
		child_subquery->ops_bestco = 
		    child_subquery->ops_dist.opd_bestco->opd_colist[target_site];
		if (child_subquery->ops_projection != OPV_NOGVAR)
		{   /* this is a projection aggregate so that the target
		    ** site should be the same as where the aggregate
		    ** is evaluated */
		    OPS_SUBQUERY	*proj_subquery;
		    proj_subquery = global->ops_rangetab.opv_base->opv_grv
			[child_subquery->ops_projection]->opv_gquery;
		    if (!proj_subquery
			||
			(proj_subquery->ops_sqtype != OPS_PROJECTION)
			)
			opx_error(E_OP0A8E_SUB_TYPE);
		    proj_subquery->ops_dist.opd_target = target_site;
		    proj_subquery->ops_bestco = proj_subquery->ops_dist.
			opd_bestco->opd_colist[target_site];
		}
		if (child_subquery->ops_union)
		{   /* this is a union subquery so make sure all parts
		    ** of the union are sent to the same site */
		    OPS_SUBQUERY	*unionp;
		    for (unionp=child_subquery; unionp=unionp->ops_union;)
		    {
			if ((unionp->ops_sqtype != OPS_UNION)
			    &&
			    (unionp->ops_sqtype != OPS_VIEW)
			    )
			    opx_error(E_OP0A8E_SUB_TYPE);
			unionp->ops_dist.opd_target = target_site;
			unionp->ops_bestco = unionp->ops_dist.
			    opd_bestco->opd_colist[target_site];
		    }
		}
	    }
	    cop->opo_variant.opo_site = child_subquery->ops_dist.opd_target ;
	}
	else
	{
	    if (varp->opv_grv->opv_siteid == OPD_NOSITE)
		opx_error(E_OP0A80_UNSUPPORTED); /* all types of ORIG nodes
					** are not supported yet */
	    cop->opo_variant.opo_site = varp->opv_grv->opv_siteid; /* the orig node is
					** the original site of the relation
					** but the DB_PR parent may be on
					** another site */
	}
	break;
    }
    case DB_SJ:
    {	/* find site upon which to perform the join */
	OPD_SCOST           *icostp;    /* ptr to array of costs for
					** materializing the inner cop */
	OPD_SCOST           *ocostp;    /* ptr to array of costs for
					** materializing the outer cop */
	OPO_CO		    *ocop;	/* outer CO node */
	OPO_CO		    *icop;	/* inner CO node */
	OPO_CPU		    jcpu;	/* amount of CPU required to produce
                                        ** the join */
	OPO_BLOCKS	    jdio;	/* number of disk I/O required to
					** produce the join */
	OPO_COST	    joincost;	/* cost to perform join, not including
					** cost of intermediates */
	OPO_COST	    icost;	/* cost of producing inner for this
					** site */
	OPO_COST	    ocost;	/* cost of producing outer for this
					** site */
	OPD_ISITE	    join_site;	/* site upon which to perform the join
					** which produces this result */
	OPO_COST	    costdiff;   /* this is the difference between
					** recomputed cost and the cost for
					** the target site, there may be
					** roundoff so that if the cost
					** difference is within the error
					** limits of the the machine then
					** the join site has been found */
	OPO_COST	    precision;  /* with large numbers the costdiff
					** may be lost within the precision
					** of the float */
	OPD_DT		    *sitepp;    /* ptr to array of site descriptors */
	DD_COSTS	    *nodecostp;
	OPD_ISITE	    siteno;

	ocop = cop->opo_outer;
	icop = cop->opo_inner;
	icostp = icop->opo_variant.opo_scost;
	ocostp = ocop->opo_variant.opo_scost;
	costp = cop->opo_variant.opo_scost;
	target_cost = costp->opd_intcosts[target_site];
	precision = target_cost/OPD_PRECISION;
	if (precision < OPD_FMIN)
	    precision = OPD_FMIN;
	jdio = 0.0;
	jcpu = 0.0;
	if (root_flag && remove)
	{
	    if (subquery->ops_dist.opd_bestco)
	    {
		for (siteno = global->ops_gdist.opd_dv; --siteno >= 0;)
		{
		    OPO_CO	     *site_cop;

		    site_cop = subquery->ops_dist.
					opd_bestco->opd_colist[siteno];
		    if (site_cop)
			opn_cleanup(subquery, cop, &jdio, &jcpu);
						/* find the cost
				 		** of creating the temporary
						*/
		}
	    }
	    else
		opn_cleanup(subquery, cop, &jdio, &jcpu);
	}
	
	jcpu = cop->opo_cost.opo_cpu - ocop->opo_cost.opo_cpu 
	    - icop->opo_cost.opo_cpu - jcpu;
	jdio = cop->opo_cost.opo_dio - ocop->opo_cost.opo_dio 
	    - icop->opo_cost.opo_dio - jdio;
	sitepp = global->ops_gdist.opd_base;
	nodecostp = sitepp->opd_dtable[target_site]->opd_nodecosts;
	if (nodecostp)
	    /* adjust costs if node descriptor is available */
	    joincost = opd_adjustcost(subquery, nodecostp, jcpu, jdio);
	else
	    joincost = opn_cost(subquery, jdio, jcpu);
	if (((icost = icostp->opd_intcosts[target_site]) < OPO_CINFINITE)
	    &&
	    ((ocost = ocostp->opd_intcosts[target_site]) < OPO_CINFINITE)
	    &&
	    ((costdiff = joincost + icost + ocost - target_cost) <= precision) 
	    &&
	    (costdiff >= -precision)
			    /* NOTE - the order of additions
			    ** is probably important so that any
			    ** roundoff error will be the same, if roundoff
                            ** occurs, hopefully it will be less than FMIN */
			    /* do join on target site if processing for 
			    ** UPDATE/DELETE */
	    )
	{   /* the join was performed on the target site without
            ** any data movement, so save the site information and
            ** propagate to the children */
	    join_site = target_site;
	}
	else
	{
	    /* need to find other site with produced the join and moved
	    ** the result to this site */
	    /* look at the cost of doing the join on a remote site and moving it
	    ** the target site */
	    OPO_COST	    sitecost;
	    OPS_WIDTH	    width;	/* width of tuple in bytes */

	    width = ope_twidth(subquery, &cop->opo_maps->opo_eqcmap);
	    if (width < 1)
		width = 1;		/* match width calculation in
					** opnncommon.c */
	    join_site = OPD_NOSITE;
	    for (source_site = global->ops_gdist.opd_dv;
		--source_site >= 0;)
	    {   /* consider each site as a source for the join intermediate */
		OPO_COST	    source_cost; /* cost of producing the join */
		OPD_SCOST	    *factorp; /* array of factors for moving the
					** join to the intermediate site */

		if (width > global->ops_gdist.opd_base->
			    opd_dtable[source_site]->opd_dbcap->dd_p7_maxtup)
		  continue;
		if ((source_cost = costp->opd_intcosts[source_site])
		    > OPO_CINFINITE)
		    continue;	    /* site has already been removed from
					** consideration */
		factorp = global->ops_gdist.opd_base->opd_dtable[source_site]
		    ->opd_factor;
		{   /* look at each target site to see if the join can be
		    ** more cheaply created and moved here from elsewhere */
		    network = createcost    /* add cost to create a temporary
					    ** relation on the remote site */
			+ factorp->opd_intcosts[target_site]
			* width
			* cop->opo_cost.opo_tups; /* add cost of moving data */

		    sitecost = (source_cost + network) - target_cost;
		    if (sitecost < - precision)
			opx_error(E_OP0A8C_COST); /* this should not happen or else this would be
					    ** the cheapest plan */
		    if (sitecost < precision)
		    {	/* if the difference is between -OPD_FMIN and OPD_FMIN then the
			** plan is correct or close enough */
			join_site = source_site;
			break;
		    }
		}
	    }
	    if (join_site == OPD_NOSITE)
		opx_error(E_OP0A8C_COST); /* no site was found which fell into the
					** re-computed cost of this plan, probably
					** some round-off computation problem */
	}
	cop->opo_variant.opo_site = join_site; 
	cop->opo_cost.opo_cvar.opo_network = 
	network = network		/* store the network cost 
					** in opo_cvar.opo_network which
					** is not used for interior nodes */
	+ opd_msite(subquery, ocop, join_site, FALSE, marker, remove)
	+ opd_msite(subquery, icop, join_site, FALSE, marker, remove); /* save the 
					** total networking cost here */
	break;
    }
    default:
	opx_error(E_OP0495_CO_TYPE);
    }
    return(network);
}

/*{
** Name: opd_jcost	- cost of a distributed join
**
** Description:
**      This routine will look at the cost arrays for the inner and outer 
**      and produce a new cost array for the result of the join, i.e. it 
**      will represent the cost of having the result of the intermediate 
**      on each of the sites.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**	main_rlsp			ptr to relation descriptor
**	main_subtp			ptr to list of compatible
**					co trees
**	newcop				cop representing this join
**	width				width of output tuple
**	sort_cpu			if sort node needed, CPU cost
**	sort_dio			if sort node needed, DIO cost
**
** Outputs:
**	newcop				sitecost array created if this
**					cop is useful
**	Returns:
**	    TRUE if this cop is useful, otherwise caller should delete it
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      8-may-88 (seputis)
**          initial creation
**      15-jan-94 (ed)
**          remove some parameters to simplify interface, and
**	    added parameters to support site specific characteristics
**	06-mar-96 (nanpr01)
**	    Consider the ldbcapabilities to make sure that the width can
**	    be handled in the target site.
[@history_template@]...
*/
bool
opd_jcost(
	OPS_SUBQUERY       *subquery,
	OPN_RLS		   *main_rlsp,
	OPN_SUBTREE        *main_subtp,
	OPO_CO             *newcop,
	OPS_WIDTH	   width,
	OPO_CPU		   sort_cpu,
	OPO_BLOCKS	   sort_dio)
{
    OPD_ISITE		sitecount;
    OPD_SCOST           *maincostp; /* the array of costs of having the
				    ** result on each of n sites,
				    ** NULL for main query or simple aggregate 
				    */
    OPD_SCOST           *icostp;    /* ptr to array of costs for
				    ** materializing the inner cop */
    OPD_SCOST           *ocostp;    /* ptr to array of costs for
				    ** materializing the outer cop */
    OPD_SCOST           *newbasep;  /* created only if newcop is
                                    ** defined, this contains the new cost
				    ** array for the intermediate CO */
    i4			numsites;   /* number of sites being consider */
    OPD_ISITE		site;	    /* used to traverse arrays of site info */
    OPD_SCOST		*subtreecostp; /* array of best costs for subtree */
    OPD_BESTCO		*subtreebestco; /* array of best COs for each site */
    OPS_STATE		*global;
    OPD_DT		*sitepp;    /* ptr to array of site descriptors */
    bool		bestco_found;
    OPO_COST		joincost;   /* cost of performing the join at this
				    ** generic node */
    OPO_COST		sortcost;   /* cost of sort node on top of this node
				    */
    OPO_CPU		join_cpu;   /* cpu cost of generic join operation */
    OPO_BLOCKS		join_dio;   /* dio cost of generic join operation */

    bestco_found = FALSE;
    global = subquery->ops_global;
    maincostp = subquery->ops_dist.opd_cost; /* get ptr to an array of best cost
				    ** values for each site for this subquery*/
    numsites = global->ops_gdist.opd_dv;
    sitepp = global->ops_gdist.opd_base;

    {	/* first find the array of best plans for this subtree which have been
	** saved, to determine whether the current CO is worthwhile to
	** create, also mark existing COs as deleteable if the current
        ** best complete plans have improved to the point that the saved
        ** subtrees are no longer worthwhile */

	OPN_SUBTREE	*subtp;	    /* current cost ordering set
				    ** or "subtree" being analyzed*/
	if (!(subtreecostp = global->ops_gdist.opd_tcost)
	    ||
	    !(subtreebestco = global->ops_gdist.opd_tbestco)
	    )
	{   /* allocate temp cost array for sites, once for enumeration
	    ** may be deallocated during garbage collection so this needs to
	    ** be checked dynamically */
	    subtreecostp = global->ops_gdist.opd_tcost = (OPD_SCOST *)opn_memory(
		subquery->ops_global, (i4)(numsites * sizeof(subtreecostp->opd_intcosts[0])));
	    subtreebestco = global->ops_gdist.opd_tbestco = (OPD_BESTCO *)opn_memory(
		subquery->ops_global, (i4)(numsites * sizeof(subtreebestco->opd_colist[0])));
	}
	for (site = numsites; --site >= 0;)
	{
	    subtreebestco->opd_colist[site] = NULL;
	    subtreecostp->opd_intcosts[site] = OPO_INFINITE;
	}
	for (subtp = main_subtp; subtp; subtp = subtp->opn_stnext)   /* for the 
					    ** co's in all the subtrees */
	{   /* for distributed, precalculate the array of best plans, and best
	    ** costs for all the sites to determine if the new plan for this
	    ** subtree is worthwhile creating */
	    OPO_CO             *cop;	/* current cost ordering subtree
					** being analyzed */
	    for (cop = subtp->opn_coforw; 
		 cop != (OPO_CO *) subtp; 
		 cop = cop->opo_coforw)
	    {
		if ((cop->opo_storage == newcop->opo_storage)
		    && 
		    (cop->opo_ordeqc == newcop->opo_ordeqc) /* FIXME - ordering may
					   ** be a subset of new ordering */
		    && 
		    (!cop->opo_deleteflag))
		{   /* found a cost ordering with the same storage structure
		    ** ordered on the same equivalence class
		    */
		    bool		delete_plan;/* TRUE if this plan is useful for at
					    ** least one site, otherwise mark
					    ** plan for garbage collection */
		    OPO_CO		*candidate; /* ptr to CO tree which may be
					    ** marked for deletion */
		    OPD_ISITE		site_of_candidate; /* site of CO node which
					    ** may be deleted */
		    OPD_SCOST		*cur_costp; /* ptr to the cost array
					    ** for this CO node being considered */
		    delete_plan = TRUE;
		    candidate = NULL;	    /* mark NULL until 2 different candidates are
					    ** found to avoid unnecessary traversals of
					    ** the site array */
		    cur_costp = cop->opo_variant.opo_scost;
		    for (site = numsites; --site >= 0;)
		    {	/* look at each of the sites for this CO and determine
			** if it is useful for any site */
			bool	skip;
			if (subtreecostp->opd_intcosts[site] 
			    <= cur_costp->opd_intcosts[site])
			    skip = TRUE;    /* skip if this is worse than the 
					    ** last CO looked at */
			else if (maincostp)
			{   /* check for subqueries in which it would be useful
			    ** to have the result on each site */
			    skip = maincostp->opd_intcosts[site] 
				<= cur_costp->opd_intcosts[site];
					    /* is this better than the best 
					    ** plan for this site */
			}
			else 
			    skip = subquery->ops_cost  /* best cost overall */
				 <= cur_costp->opd_intcosts[site];
		
			if (!skip)
			{   /* new CO node found for this site, so mark the previous best
                            ** CO node for deletion */
			    if (candidate)
			    {   /* if 2 candidates to mark for deletion are selected then 
				** process the first one here */
				if (candidate != subtreebestco->opd_colist[site])
				{   /* delay processing of candidate until end of
				    ** loop to avoid unnecessary traversals */
				    OPD_ISITE	site1;
				    bool		delete1_plan;
				    delete1_plan = TRUE;
				    for (site1 = numsites; --site1 >= 0;)
				    {   /* check if replaced plan can be deleted */
					if (subtreebestco->opd_colist[site1] == candidate)
					{   /* the CO plan can be used for another site */
					    delete1_plan = FALSE;
					    break;
					}
				    }
				    if (delete1_plan)
					candidate->opo_deleteflag = TRUE; /* this plan
					 ** is not useful on any site */
				    site_of_candidate = site;	/* save new site */
				    candidate = subtreebestco->opd_colist[site];
				}
			    }
			    else
			    {	/* delay delete of plan until end if possible */
				site_of_candidate = site;
				candidate = subtreebestco->opd_colist[site];
			    }
			    subtreecostp->opd_intcosts[site] = cur_costp->opd_intcosts[site];
			    subtreebestco->opd_colist[site] = cop;
			    delete_plan = FALSE;	    /* do not delete plan since one
							** site was found where this plan
							** could be useful */
			}
			else if (candidate && (candidate == subtreebestco->opd_colist[site]))
			    candidate = NULL; /* there is at least one site where
					    ** this candidate for deletion is
					    ** useful */
		    }
		    if (candidate)
		    {
			OPD_ISITE	    site2;
			bool	    delete2_plan;
			delete2_plan = TRUE;
			for (site2 = site_of_candidate; ++site2 < numsites; )
			{   /* check if this CO is not useful on any site; note that
			    ** sites below the site_of_candidate have already been
			    ** checked */
			    if (subtreebestco->opd_colist[site2] == candidate)
			    {   /* the CO plan can be used for another site */
				delete2_plan = FALSE;
				break;
			    }
			}
			if (delete2_plan)
			    candidate->opo_deleteflag = TRUE; /* this plan
					** is not useful on any site */
		    }
		    if (delete_plan)
			cop->opo_deleteflag = TRUE;	/* there was not one site were this current
							** CO was useful so mark it for deletion */
		}
	    }
	}
    }

    icostp = newcop->opo_inner->opo_variant.opo_scost;
    ocostp = newcop->opo_outer->opo_variant.opo_scost;
    /* 1) if this is the main query then the best cost plans needs the 
    **    relation on the result site
    ** 2) if this is a subquery then the best cost of having the result
    **    on each of the sites (and a plan for each of the sites) is determined
    **
    **    this sub-plan is saved if it is the best plan for any site
    */
    join_cpu = newcop->opo_cost.opo_cpu - newcop->opo_outer->opo_cost.opo_cpu 
	- newcop->opo_inner->opo_cost.opo_cpu;
    join_dio = newcop->opo_cost.opo_dio - newcop->opo_outer->opo_cost.opo_dio
	- newcop->opo_inner->opo_cost.opo_dio;
    joincost = opn_cost(subquery, join_dio, join_cpu);
    sortcost = opn_cost(subquery, sort_dio, sort_cpu);

    for (sitecount = global->ops_gdist.opd_dv;--sitecount >= 0;)
    {
	OPO_COST	newcost;    /* cost of having the join performed on the
                                    ** current site being considered */
	OPO_COST	icost;      /* cost of having the inner on the site
                                    ** being considered for the join */
	OPO_COST	ocost;	    /* cost of having the outer on the site
                                    ** being considered for the join */
	OPO_COST	precision;  /* avoid round-off errors */
	OPO_COST	nosortcost; /* cost of root without sort node */
	DD_COSTS	*nodecostp;
	OPO_COST	local_join; /* possible adjustment to cost for
				    ** cpu power */
	OPO_COST	local_sortcost; /* possible adjustment to cost for
				    ** sort cost */

	if ((icost = icostp->opd_intcosts[sitecount]) > OPO_CINFINITE)
	    continue;		    /* inner cost for site removes plan from
				    ** consideration for this site */
	if ((ocost = ocostp->opd_intcosts[sitecount]) > OPO_CINFINITE)
	    continue;		    /* outer cost for site removes plan from
				    ** consideration for this site */
	nodecostp = sitepp->opd_dtable[sitecount]->opd_nodecosts;
	if (nodecostp)
	{   /* adjust costs if node descriptor is available */
	    local_join = opd_adjustcost(subquery, nodecostp, join_cpu, join_dio);
	    local_sortcost = opd_adjustcost(subquery, nodecostp, sort_cpu, sort_dio);
	}
	else
	{
	    local_join = joincost;
	    local_sortcost = sortcost;
	}

	nosortcost = local_join + icost + ocost; /* add this separately since sortcost
				    ** can be very large */
	if (nosortcost >= OPO_CINFINITE)
	    nosortcost = OPO_CINFINITE/2.0;
	newcost = nosortcost + local_sortcost; /* this is the cost of having this
				    ** intermediate result on this site */
	precision = newcost/OPD_PRECISION;
	if (precision < OPD_FMIN)
	    precision = OPD_FMIN;

	if ((newcost+precision) >= subtreecostp->opd_intcosts[sitecount])
	    continue;		    /* look at best cost for placing this 
				    ** intermediate result on this site */
	if (maincostp)
	{			    /* only entered if a previous set of best plan
				    ** for the entire subquery has already been 
				    ** found */
	    if ((newcost+precision) >= maincostp->opd_intcosts[sitecount])
		continue;		/* cost of producing this join on this
					** site is more expensive than the best
					** plan for this site */
	}
	else if ((newcost+precision) >= subquery->ops_cost)
	    continue;		    /* must be better than best plan for all sites
				    ** - since maincostp is not defined, this must
				    ** be the main query or a simple aggregate */

	if (sitepp->opd_dtable[sitecount]->opd_capability
	    & OPD_CTARGET)
	    continue;		    /* do not consider this plan if the capability
				    ** for performing a join on this site does not
				    ** exist */
	if (width > sitepp->opd_dtable[sitecount]->opd_dbcap->dd_p7_maxtup)
	    continue;		/* do not consider this plan if the capability
				** for performing a join on this site does not
				** exist */

	if (!bestco_found)
	{   /* this CO tree is still possibly the cheapest on at least one
	    ** site */
	    OPD_ISITE		site3;

	    bestco_found=TRUE;	    /* get a new CO node to
				    ** represent this CO tree */
	    newbasep = newcop->opo_variant.opo_scost;
	    for (site3 = global->ops_gdist.opd_dv;
		--site3 >= 0;)
		newbasep->opd_intcosts[site3] = OPO_INFINITE;
	    /* initialize all the site costs to infinite unless it possibly
	    ** can be the cheapest plan for that site */
	}
	newbasep->opd_intcosts[sitecount] = nosortcost; /* sort cost will
				    ** be added later when the top sort node
				    ** is added */
    }

    if (bestco_found)
    {	/* if no plan is cheaper then this plan could not be useful, 
	** so discard it by returning NULL, note that if the sub-plan is
	** corelated then OPC currently does not have the capability to
        ** reexecuted a QP, so do not consider moving from a remote site */
	OPD_ISITE	    source_site; /* site on which the join temp
				    ** was created */
	bool		    final_target; /* TRUE if retrieve is being made to
				    ** the final target */

	/* look at the cost of doing the join on a remote site and moving it
	** the target site */

	final_target = 
	    global->ops_estate.opn_rootflg /* we are not at the root node so that
			    ** an intermediate needs to be created */
	    &&
	    (subquery->ops_sqtype == OPS_MAIN);	/* this is not the main query so that
			    ** intermediate results need to be linked back to the
			    ** main query */

	for (source_site = global->ops_gdist.opd_dv;
	    --source_site >= 0;)
	{   /* consider each site as a source for the join intermediate */
	    OPO_COST	    source_cost; /* cost of producing the join */
	    OPD_SCOST	    *factorp; /* array of factors for moving the
				    ** join to the intermediate site */
	    OPD_ISITE	    target_site; /* current target site being
				    ** considered */

	    if ((source_cost = newbasep->opd_intcosts[source_site])
		> OPO_CINFINITE)
		continue;	    /* site has already been removed from
				    ** consideration */
	    if ((source_site == OPD_TSITE) /* if we are considering moving
				    ** data from the target site then
				    ** there may be certain site constraints
				    */
		&&
		final_target
		&&
		(global->ops_gdist.opd_gmask & OPD_NOUTID) /* this is an
				    ** non-retrieve query and TIDs are not
				    ** available for the target relation */
		)
	    {	/* at this point a movement is only allowed if the source site
		** temporary does not include the target table to be updated,
		** otherwise TIDs would be needed to perform the link back,
		** and TIDs are not available for this site */
		/* FIXME - one thought would be to simulate the "same plan" 
		** without TIDs
		** by linking back the temporary on attributes only
		** but SORTING to remove duplicates when transferring
		** the temporary so that an ambiguous replace problem
		** does not occur, all boolean factors will need to be
		** re-evaluated that involve the target relation, this
		** would be tricky since equivalence classes may have been
		** eliminated so they are not available for boolean factors,
		** - a better way would be to define a new range variable
		** for the target relation, and equate all equivalence classes
		** but not TIDs and treat it like an index, i.e. evaluate with
		** and without this new range variable */
		OPV_BMVARS	*bmvarp; /* Bitmap of vars used to produce
					** this intermediate */
		OPV_IVARS	maxvar;	/* max number of variables defined
					** for this subquery */
		bool		foundvar; /* TRUE - if variable to be replaced
					** or deleted is being moved */
		OPV_IVARS	varno;  /* var to test for result */

		foundvar = FALSE;
		maxvar = subquery->ops_vars.opv_rv;
		bmvarp = &main_rlsp->opn_relmap;
		for (varno = OPV_NOVAR; 
		    (varno = BTnext((i4)varno, (char *)bmvarp, (i4)maxvar))>=0;)
		    if (varno == subquery->ops_localres)
		    {
			foundvar = TRUE;
			break;
		    }
		if (foundvar)
		    continue;		/* the table to be replaced or deleted is
					** being moved as an intermediate so avoid
					** this plan */
	    }
	    factorp = sitepp->opd_dtable[source_site]->opd_factor;
	    for (target_site = global->ops_gdist.opd_dv;
		--target_site >= 0;)
	    {	/* look at each target site to see if the join can be
		** more cheaply created and moved here from elsewhere */

		OPO_COST	    newcost1;
		OPO_COST	    *targetp;
		OPO_COST	    mprecision;  /* avoid round-off errors */


		if (target_site == source_site)
		    continue;
		if (	((sitepp->opd_dtable[target_site]->opd_capability
			 & 
			 OPD_CTARGET) != 0)
			 &&
			 (!final_target)
		    )
		    continue;		    /* do not consider this plan if the capability
					    ** for creating a temp on this site does not
					    ** exist */
		newcost1 = source_cost   /* cost of creation of join on
					** remote site + */
		    + factorp->opd_intcosts[target_site]
                    * width * newcop->opo_cost.opo_tups; /* add cost of moving data */
		if (final_target	/* if we are not at the top of the
					** plan then add the cost of the temporary
					** creation */
		    && 
		    (target_site == OPD_TSITE) /* may avoid cost of creation
					** of the temporary in this case */
		    &&
		    (global->ops_gdist.opd_gmask & OPD_TOUSER) /* no site 
					** constraint for the
					** final target relation */
		    )
		{
		    ;			/* do not add cost of creating temporary
					** since target set is really a tuple
					** stream to the user */
		}
		else
		    newcost1 += OPD_CREATE_COST;   /* add cost to create a temporary
					** relation on the remote site */

		mprecision = newcost1/OPD_PRECISION;
		if (mprecision < OPD_FMIN)
		    mprecision = OPD_FMIN;
		if (maincostp)
		{
		    if (maincostp->opd_intcosts[target_site] <= (newcost1+mprecision))
			continue;	/* new cost for site removes plan from
					** consideration for this site */
		}
		else if (subquery->ops_cost <= (newcost1+mprecision))
		    continue;

		targetp = &newbasep->opd_intcosts[target_site];
		if (*targetp > newcost1)
		    *targetp = newcost1; /* smaller cost to evaluating the
					** join on the source site and moving
					** it to the target site */
		
	    }
	}
    }
    return(bestco_found);
}

/*{
** Name: opd_idistributed	- initialize distributed structures
**
** Description:
**      This routine will initialize the arrays which will help compute 
**      the costs of movement of data between sites.  Each site will have
**      an array of "factors" , one element for each site, which is the
**      cost per byte, to move data from the site to the target site.  Thus,
**	effectively there is a matrix of movement factors. 
**         In order to model the communication cost between sites, a
**      general scheme of representing relationships between sites is
**      proposed.  It also attempts to address a pet peeve of mine in which
**      the "data dictionary" for ingres is uncontrolled, and the relationships
**      are not defined in a consistent manner.  Since distributed is about
**      to define an entirely new set of uncontrolled relations, it seems
**      like a good time to try to design the distributed catalogs with
**      long term data dictionary goals in mind.  I will describe the
**      design for storing communications costs in catalogs and then
**      show how this could be extended for data dictionary purposes.
**
**
**	There is a "cost per byte" factor in order to move data from
**      site A to site B, which could vary depending on the link.  It seems
**      reasonable to model this as a hierarchy initially and then perhaps
**      understand directed graphs in the future.
**
**            iidobjects(oid, oidtype, oidname)
**                oid - is the unique object ID
**                oidtype - represents a type of object
**                   1) DB_OSITE - site object which represents a single
**                      site
**                   2) DB_OSGROUP- a group of sites or other groups
**		  oidname - represents a unique name for the object
**            iidgroups(dependent_oid, independent_oid)
**                   - this defines a hierarchy of objects, in the case
**                   of sites, it can define groups of sites, thus if
**                   sites ws10d,ws11d, and ws12d belonged to the group
**                   name "rabbit" then the OID for rabbit would be the
**                   independent object, and there would be 3 tuples
**                   with different dependent_oid values.  This could be
**                   used to model several layers, e.g. several databases
**                   on a remote site but on the same installation, several
**                   databases on a remote LAN, or several databases on
**                   remote LANs connected by long haul.
**            iidfactors(source_oid, target_oid, factor)
**                   - this describes the cost of moving data between
**                   sites represented by the oid, i.e. all objects 
**                   in the group represented by the oid have a cost
**                   of "factor" units per byte, to move data between
**                   sites, if factor=0.001 for "rabbit" then the
**                   cost of moving data between ws10d and ws12d would
**		     be 0.001 units per byte.  The cost of movement is
**                   cumulative for each link.  If the source and target
**                   OIDs are the same then this means moving data from
**                   one element in this group to another element is given
**                   by the factor.  In initial configuration, the data
**                   will always pass to the coordinator, so that the
**                   path chosen will be to add link costs until the
**                   coordinator is reached and then add link costs
**                   until the destination is reached.  It is assumed that
**                   a "tree structure" will be defined in the initial
**                   configuration.
**
**            Notice that the iidobjects and iidgroups defines a relationship
**	      between objects which could be expanded to contain new
**            object IDs and new relationships.  These relationships could
**            be a directory structure, or network relation etc.
**            This could be used to define user groups by the new object type
**                   DB_USERNAME - each user is an object which can form
**                               groups to which permissions can be assigned
**                   DB_GUSERNAME - each user group is an object which can form
**                               groups to which permissions can be assigned
**            For distributed relationships, ingres currently allows a
**            "weak 1 level hierarchy", in which 5.0 can define multiple
**            databases on one installation.  It seems reasonable for
**            transparency that distributed should be able to define a
**            hierarchy of databases (local and distributed) so that it
**            looks more or less like a directory structure.  Thus,
**            a distributed database object could be defined.  
**                   DB_DATABASE - name of a database
**                   DB_DB_GROUP - name of a distributed database (i.e.
**                               a group of databases)
**
**            The iidgroups is much like iidepends except that it becomes
**            more valuable when objectID's are used to define objects.
**            One problem in iidepends is that "table IDs" and "timestamps"
**            are being coerced into the same column in order to define
**            dependencies.  These types of hacks are not manageable
**            in the long term.  Dependencies can be stored in the
**            iidgroups table and the iidepends table can be removed
**            if a "table object" or a "link" object is added.  Instead
**            of an "object type" there is a "hack" which uses the query
**            mode type of the parser to defined the object type i.e.
**            PSQ_DEFINE_PERMIT would be used to indicate a permit in
**	      iidepends.
**                   DB_OTABLE - for view dependencies is an underlying
**                      table is deleted
**                   DB_OPROCEDURE - for deletion of a procedure object
**                      if an underlying table is deleted
**                   DB_OPERMIT - several entries if an underlying user or 
**                      user group or and underlying table is deleted.
**                   DB_OLINK - name of a link
**                   
[@comment_line@]...
**
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
[@PARAM_DESCR@]...
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      7-may-88 (seputis)
**          initial creation
[@history_line@]...
[@history_template@]...
*/
VOID
opd_idistributed(
	OPS_STATE	*global)
{
    OPD_ISITE		maxsite;
    OPD_ISITE		source_site;
    i4			factorsize;
#if 0
    OPO_COST		samesite;   /* cost per byte of movement between
				** two LDBs on the same site */
    OPO_COST		difsite; /* cost per byte of movement between
				** two LDBs on different sites */

    samesite = 1.0/8196.0;	/* arbitrary number for site movement
				** between LDB's on the same node */
    {
	i4		first;
	i4		second;
	if (global->ops_cb->ops_check
	    &&
	    opt_svtrace( global->ops_cb, OPT_F126_SITE_FACTOR, &first, &second)) /*
					    ** will time out based on subquery
					    ** plan id and number of plans being
					    ** evaluated */
	{
	    if (second > 0)
		samesite = 1.0/second;	    /* this is equivalent to a set CPU
					    ** FACTOR for network costs, it is a
					    ** fraction of the cost of movement
					    ** one byte between LDB's on the same 
					    ** node */
	    difsite = samesite * first / 10.0;
	}
	else
	    difsite = 1.0/1024.0;	    /* default site movement cost for
					    ** different nodes */
    }
#endif

    if (global->ops_gdist.opd_dv == 0)
	opd_addsite(global, (OPV_GRV *)NULL); /* place the target and coordinator
					    ** sites into the array for this
					    ** constant only query */
    maxsite = global->ops_gdist.opd_dv;
    factorsize = maxsite * sizeof(OPO_COST); /* number of bytes needed for
					    ** the factor array for each site */
    global->ops_gdist.opd_copied = NULL;
/* (bool *)opn_memory(global, (i4)(sizeof(bool)*maxsite)); */
    for(source_site = maxsite; --source_site >= 0;)
    {
	OPD_SCOST		*factorp;
	OPD_ISITE		target_site;
/*	char			*sourcenodep; */
	OPD_SITE		*sourcesitep;

	sourcesitep = global->ops_gdist.opd_base->opd_dtable[source_site];
	factorp = sourcesitep->opd_factor = (OPD_SCOST *)opu_memory(global, 
		factorsize);	    /* allocate memory for cost array */
/* 	sourcenodep = &sourcesitep->opd_ldbdesc->dd_l2_node_name[0]; */
	for (target_site = maxsite; --target_site >= 0;)
	{
#if 0
	    char		*targetnodep;
	    targetnodep = &global->ops_gdist.opd_base->opd_dtable[target_site]
		->opd_ldbdesc->dd_l2_node_name[0];
	    char	source_char;
	    char	dest_char;
	    dest_char = global->ops_gdist.opd_base->opd_dtable[target_site]
		->opd_ldbdesc->dd_l2_node_name[0];
	    if ((dest_char < 'a') || (dest_char > 'z'))
		dest_char = 'a';
	    source_char = global->ops_gdist.opd_base->opd_dtable[source_site]
		->opd_ldbdesc->dd_l2_node_name[0];
	    if ((source_char < 'a') || (source_char > 'z'))
		source_char = 'a';
	    factorp->opd_intcosts[target_site] = ((OPO_COST)(dest_char - source_char))
		/1024.0 ; 	    /* the cost of moving one byte is
				    ** equal to the diff in the first char
				    ** of the file name.... where in the
				    ** best case it is 1 DIO for the write
				    ** to the temp relation. */
	    if (factorp->opd_intcosts[target_site] < 0)
		factorp->opd_intcosts[target_site] = 
		- factorp->opd_intcosts[target_site];
	    if ((*targetnodep == *sourcenodep) &&
		!MEcmp((PTR)targetnodep, (PTR)sourcenodep, sizeof(DD_NODE_NAME)))
		/* LDBs are on the same sites so use smaller network cost */
		factorp->opd_intcosts[target_site] = samesite;
	    else
	    	/* not on same site so assume more expensive network cost */
		factorp->opd_intcosts[target_site] = difsite;
#endif
	    factorp->opd_intcosts[target_site] = sourcesitep->opd_fromsite 
		+ global->ops_gdist.opd_base->opd_dtable[target_site]->opd_tosite; 
				    /* the model
				    ** is that all transfers occur from the
				    ** target site to star and then to the 
				    ** destination site */
	}
    }
}

/*{
** Name: opd_isub_dist - initialize subquery for distributed optimization
**
** Description:
**      This module will initialize all distributed fields required for
**      optimization of a single subquery 
**
** Inputs:
**      subquery                        ptr to subquery to init
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
**      10-oct-88 (seputis)
**          initial creation
**	31-Aug-2006 (kschendel)
**	    Watch for HFAGG as well as RFAGG.
*/
VOID
opd_isub_dist(
	OPS_SUBQUERY       *subquery)
{
    OPS_STATE	    *global;
    OPD_ISITE	    maxsite;

    global = subquery->ops_global;
    maxsite = global->ops_gdist.opd_dv;
    global->ops_gdist.opd_scopied = FALSE;/* This will be set TRUE when the bestco
					  ** is copied out of enumeration memory and the
                                          ** site cost arrays in opo_variant have been
					  ** replaced by the site */
    global->ops_gdist.opd_tcost = NULL;	  /* init for every subquery since memory is obtained
					  ** from enumeration memory stream */
    global->ops_gdist.opd_tbestco = NULL;

    switch (subquery->ops_sqtype)
    {
    case OPS_MAIN:
	subquery->ops_dist.opd_target = OPD_TSITE;  /* initialize target site of main query
                                          ** which is always defined to be OPD_TSITE */
    case OPS_SAGG:
    {
	break;
    }
    case OPS_RFAGG:
    case OPS_HFAGG:
    case OPS_FAGG:
    case OPS_PROJECTION:
    case OPS_SELECT:
    case OPS_RSAGG:
    case OPS_UNION:
    case OPS_VIEW:
    {
	OPD_ISITE	site;
	/* initialize the storage for subquery array of plans */
	subquery->ops_dist.opd_cost = (OPD_SCOST *)opu_memory(global, 
	    (i4)(sizeof(subquery->ops_dist.opd_cost->opd_intcosts[0])*maxsite));
	subquery->ops_dist.opd_bestco = (OPD_BESTCO *)opu_memory(global, 
	    (i4)(sizeof(subquery->ops_dist.opd_bestco->opd_colist[0])*maxsite));
	if (!global->ops_gdist.opd_copied)
	{
	    global->ops_gdist.opd_copied = (bool *)opu_memory(global, 
		(i4)(sizeof(bool)*maxsite));
	}
	for (site = 0; site < maxsite; site++)
	{
	    global->ops_gdist.opd_copied[site]=FALSE;
	    if (subquery->ops_dist.opd_bestco)
	    {	/* init the values for the array of best plans */
		subquery->ops_dist.opd_cost->opd_intcosts[site] = OPO_INFINITE;
		subquery->ops_dist.opd_bestco->opd_colist[site] = (OPO_CO *)NULL;
	    }
	}
	break;
    }
#ifdef E_OP048D_QUERYTYPE
    default:
	opx_error(E_OP048D_QUERYTYPE);
#endif
    }
}

/*{
** Name: opd_prleaf	- distribute project-restrict
**
** Description:
**      Calculate distributed cost array for a project restrict 
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      rlsp                            relation descriptor of proj-rest
**      cop                             ptr to proj-rest cost ordering node
**      tuple_width                     width of output tuple for PR
**      sort_cpu                        cpu cost of generic sort
**      sort_dio                        dio cost of generic sort
**
** Outputs:
**	Returns:
**	    TRUE if this is a useful plan, i.e. it is cheaper on
**	    some site
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      7-may-88 (seputis)
**          initial creation
**      26-apr-89 (seputis)
**          fix uninitialized variable problem
*/
bool
opd_prleaf(
	OPS_SUBQUERY       *subquery,
	OPN_RLS		   *rlsp,
	OPO_CO             *cop,
	OPS_WIDTH	   tuple_width,
	OPO_CPU		   sort_cpu,
	OPO_BLOCKS	   sort_dio)
{
    OPV_VARS		*varp;
    OPD_ISITE		sitecount;
    OPD_SCOST		*cbase;
    OPS_STATE		*global;
    OPD_DT		*sitepp;    /* ptr to array of site descriptors */
    bool		final_target; /* TRUE if Project restrict could be
				    ** the final result */
    bool		useful;	    /* set to TRUE if useful */
    OPO_COST		single_cost; /* if a best overall CO tree is required
				    ** then this value is the best cost so far */
    OPD_SCOST		*maincostp; /* if an array of plans is required then
				    ** this is a pointer to the array of best
				    ** costs for each site */
    DD_COSTS		*nodecostp;	    /* ptr to site cost descriptor currently
				    ** being analyzed */
    OPO_COST		temp_prcost;	/* adjusted PR cost of orig node */
    OPO_COST		temp_sort_prcost; /* adjusted PR cost of orig node
					** plus adjusted sort cost */
    OPO_COST		temp_sort_cost; /* adjusted cost of sorting at a site */

    OPO_COST		generic_prcost;	/* unadjusted PR cost of orig node */
    OPO_COST		generic_sort_cost; /* generic sort cost without
					** any adjustments */
    useful = FALSE;
    global = subquery->ops_global;
    maincostp = subquery->ops_dist.opd_cost; /* ptr to array of current best
				    ** costs for this subquery, or NULL if
				    ** only the overall best cost is desired */
    single_cost = OPO_INFINITE;
    generic_prcost = opn_cost(subquery, cop->opo_cost.opo_dio, cop->opo_cost.opo_cpu);
    if (generic_prcost >= OPO_CINFINITE)
	generic_prcost = OPO_CINFINITE/2.0;
    generic_sort_cost = opn_cost(subquery, sort_dio, sort_cpu);	

    if (!maincostp)
    {
	if (subquery->ops_bestco)
	    single_cost = subquery->ops_cost; /* get the cost of the current
				    ** best plan */
	else
	    useful = TRUE;	    /* if there has not been a plan found then
				    ** this PR is useful */
    }

    final_target = 
	global->ops_estate.opn_rootflg /* we are at the root node so that
			** an intermediate may not need to be created */
	&&
	(subquery->ops_sqtype == OPS_MAIN);	/* this is the main query so that
			** intermediate results may not be needed */
#if 0
not needed right now
    bool		corelated;
    OPO_COST		corel_cost; /* cost of reading entire relation
				    ** to new site */

    corelated = rlsp->opn_corelated;	/* TRUE - if a corelated parameter
			** was used to evaluate this project restrict, which
			** needs to be handled specially */
    if (corelated)
    {	/* FIXME - assume no keying can be done for correlated subselects
	** if the data is to be moved to another site, need to modify
	** opn_prcost and calculate actual restriction which is passed
	** and actual keying which can be done */
	corel_cost = opn_cost(subquery, cop->opo_outer->opo_cost.opo_reltotb,
	    cop->opo_outer->opo_cost.opo_tups); /* Kludge for corelated PR which are
				    ** to be moved */
    }
#endif
    sitepp = global->ops_gdist.opd_base;
    varp = subquery->ops_vars.opv_base->opv_rt[cop->opo_outer->opo_union.opo_orig];
    sitecount = global->ops_gdist.opd_dv;
    cbase = cop->opo_variant.opo_scost;
    if (varp->opv_grv->opv_relation || (varp->opv_grv->opv_siteid >= 0))
    {	/* a real relation exists so generate the keying estimate, or
	** a virtual relation may have been bound to a particular site
	** by a previous parent */
	OPD_SCOST		*scostp;

	if (varp->opv_grv->opv_siteid == OPD_NOSITE)
	    opx_error(E_OP0A80_UNSUPPORTED); /* FIXME - this should not occur
					** all types of ORIG nodes
					** are not supported yet */
	scostp = global->ops_gdist.opd_base
	    ->opd_dtable[varp->opv_grv->opv_siteid]->opd_factor; /* ptr to an
					** array of movement factors, from this
					** site to all remote sites */
	nodecostp = sitepp->opd_dtable[varp->opv_grv->opv_siteid]
	    ->opd_nodecosts;
	if (nodecostp)
	{
	    /* adjust costs if node descriptor is available */
	    temp_prcost = opd_adjustcost(subquery, nodecostp, 
		cop->opo_cost.opo_cpu, cop->opo_cost.opo_dio);
	    temp_sort_cost = opd_adjustcost(subquery, 
		nodecostp, sort_cpu, sort_dio);
	    temp_sort_prcost = temp_prcost + temp_sort_cost;
	}
	else
	{
	    temp_prcost = generic_prcost;
	    temp_sort_prcost = generic_prcost + opn_cost(subquery, sort_dio, sort_cpu);
	    temp_sort_cost = generic_sort_cost;
	}
	while(--sitecount >= 0)
	{
	    if (sitecount == varp->opv_grv->opv_siteid)
	    {

		cbase->opd_intcosts[sitecount] = temp_prcost; /* zero cost for
					** an ORIG node used for keying */
		if (maincostp)
		{
		    if ((maincostp->opd_intcosts[sitecount] > OPO_CINFINITE)
			||
			(maincostp->opd_intcosts[sitecount] > temp_sort_prcost))
			useful = TRUE;
		}
		else
		{
		    if (!useful && (single_cost > temp_sort_prcost))
			useful = TRUE;
		}
	    }
	    else if (   (sitepp->opd_dtable[sitecount]->opd_capability & OPD_CTARGET)
			&&
			!final_target
		    )
	    {
		cbase->opd_intcosts[sitecount] = OPO_INFINITE; /* do not 
					** consider this plan if the capability
					** for creating a temp on this site does not
					** exist */
	    }
	    else
	    {
		OPO_COST	    createcost;
		OPO_COST	    tempcost;

		if (final_target	/* if we are not at the top of the
					** plan then add the cost of the temporary
					** creation */
		    && 
		    (sitecount == OPD_TSITE) /* may avoid cost of creation
					** of the temporary in this case */
		    &&
		    (global->ops_gdist.opd_gmask & OPD_TOUSER) /* no site 
					** constraint for the
					** final target relation */
		    )
		    createcost = 0.0;
		else
		{
		    createcost = OPD_CREATE_COST; /* add cost to create a temporary
					** relation on the remote site */
#if 0
    Do not need to support correlated queries since they are flattened (for now)
		    if (corelated)
		    {   /* this needs to be handled specially since the correlated
			** parameter cannot be used for keying and then moving the
			** result to another site, so costs need to be calculated so
			** that any boolean factor which is corelated is applied
			** after the result has been moved to another site rather
			** than normally which is before, this allows plans to be
			** created which do not require an intermediate relation to
			** be created more than once, note that it is important
			** for a corelated result to be available on every site
			** so that enumeration will be able to come up with at
			** least one plan */
			cbase->opd_intcosts[sitecount] = corel_cost
			    + createcost
			    + scostp->opd_intcosts[sitecount] 
				* cop->opo_outer->opo_cost.opo_tups * tuple_width; /* add cost of 
					    ** movement of entire relation to
					    ** the new site */
			continue;	    /* special case calculation */
		    }
#endif
		}
		tempcost = temp_prcost
			+ createcost
			+ scostp->opd_intcosts[sitecount] 
			    * cop->opo_cost.opo_tups * tuple_width;    /* add cost of 
					    ** movement of temporary to
					    ** the new site */
		if (tempcost >= OPO_CINFINITE)
		    tempcost = OPO_CINFINITE/2.0;   /* do not allow large costs
					    ** to be saved */
		cbase->opd_intcosts[sitecount] = tempcost;
		if (maincostp)
		{
		    OPO_COST temp_cost = tempcost+temp_sort_cost;

		    if ((maincostp->opd_intcosts[sitecount] > OPO_CINFINITE)
			||
			(maincostp->opd_intcosts[sitecount] > temp_cost))
			useful = TRUE;
		}
		else
		{
		    if (!useful && (single_cost > (tempcost + temp_sort_cost)))
			useful = TRUE;
		}
	    }
	}
    }
    else
    {	/* an aggregate or subselect temp requires special handling since the
	** result may be on multiple sites, or it may have been referenced
	** by an earlier parent which will place it on a particular site, in
	** that case it would be handled like a "real relation" as above
        */
	OPD_SCOST		*obase;
	OPD_ISITE		best_site;
	OPO_COST		best_cost;  /* cost at best_site */
	OPO_COST		best_sort_cost; /* cost of sorting at best_site */

	best_cost = OPO_INFINITE;
	best_site = OPD_NOSITE;
	obase = cop->opo_outer->opo_variant.opo_scost; /* get base of array
					** of costs to produce the outer */
	while(--sitecount >= 0)
	{
	    if (   (sitepp->opd_dtable[sitecount]->opd_capability & OPD_CTARGET)
			&&
			!final_target
		    )
	    {
		cbase->opd_intcosts[sitecount] = OPO_INFINITE; /* do not 
					** consider this plan if the capability
					** for creating a temp on this site does not
					** exist */
	    }
	    else
	    {
		OPO_COST	    orig_cost;

		nodecostp = sitepp->opd_dtable[sitecount]->opd_nodecosts;
		if (nodecostp)
		{
		    /* adjust costs if node descriptor is available */
		    temp_prcost = opd_adjustcost(subquery, nodecostp, 
			cop->opo_cost.opo_cpu, cop->opo_cost.opo_dio);
		    temp_sort_cost = opd_adjustcost(subquery, 
			nodecostp, sort_cpu, sort_dio);
		}
		else
		{
		    temp_prcost = generic_prcost;
		    temp_sort_cost = generic_sort_cost;
		}
		orig_cost = obase->opd_intcosts[sitecount];
		if (orig_cost > OPO_CINFINITE)
		    cbase->opd_intcosts[sitecount] = OPO_INFINITE;
		else
		{
		    OPO_COST	    new_cost;
		    new_cost = orig_cost + temp_prcost;
		    if (new_cost >= OPO_CINFINITE)
			new_cost = OPO_CINFINITE/2.0; /* do not allow large costs
					 ** to pass thru */
		    cbase->opd_intcosts[sitecount] = new_cost;
		    if (maincostp)
		    {
			if ((maincostp->opd_intcosts[sitecount] > OPO_CINFINITE)
			    ||
			    (maincostp->opd_intcosts[sitecount] > (temp_sort_cost+new_cost)))
			    useful = TRUE;
		    }
		    else
		    {
			if (single_cost > (temp_sort_cost+new_cost))
			    useful = TRUE;
		    }
		    if ((best_site != OPD_NOSITE)
			||
			(new_cost < best_cost))
		    {
			best_site = sitecount;
			best_cost = new_cost;
			best_sort_cost = temp_sort_cost;
		    }
		}
	    }
	}
	if (final_target && (cbase->opd_intcosts[OPD_TSITE] > OPO_CINFINITE))
	{   /* calculate cost of moving best plan to user */
	    OPO_COST	    target_cost;
	    if (best_site == OPD_NOSITE)
		opx_error(E_OP0A80_UNSUPPORTED); /* FIXME - this should not occur
					** all types of ORIG nodes
					** are not supported yet */
	    target_cost = best_cost
		+ cop->opo_cost.opo_tups * tuple_width * global->ops_gdist.opd_base
		    ->opd_dtable[OPD_TSITE]->opd_factor->opd_intcosts[best_site];
	    if (target_cost >= OPO_CINFINITE)
		target_cost = OPO_INFINITE/2.0;	/* do not allow large costs to
					** pass thru */
	    cbase->opd_intcosts[OPD_TSITE] = target_cost;
	    if (maincostp)
	    {
		if ((maincostp->opd_intcosts[OPD_TSITE] > OPO_CINFINITE)
		    ||
		    (maincostp->opd_intcosts[OPD_TSITE] > (best_sort_cost+target_cost)))
		    useful = TRUE;
	    }
	    else
	    {
		if (single_cost > (best_sort_cost+target_cost))
		    useful = TRUE;
	    }
	}
    }
    return(useful);
}

/*{
** Name: opd_orig	- distributed cost of an ORIG node
**
** Description:
**      Calculate distributed costs of an ORIG node
**
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
[@PARAM_DESCR@]...
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      7-may-88 (seputis)
**          initial creation
[@history_template@]...
*/
VOID
opd_orig(
	OPS_SUBQUERY       *subquery,
	OPO_CO             *cop)
{
    OPV_VARS		*varp;
    OPD_ISITE		sitecount;
    OPD_SCOST		*cbase;
    OPS_STATE		*global;
    OPD_ISITE		tsiteid;    /* source site of ORIG node */
    OPO_COST            tcost;      /* cost to create ORIG node */

    global = subquery->ops_global;
    varp = subquery->ops_vars.opv_base->opv_rt[cop->opo_union.opo_orig];
    sitecount = global->ops_gdist.opd_dv;
    cbase = cop->opo_variant.opo_scost;
    if (varp->opv_grv
	&&
	varp->opv_grv->opv_gquery
	)
    {	/* the cost of producing this result should have already been
	** calculated for all of the sites */
	if (varp->opv_grv->opv_gquery->ops_dist.opd_target == OPD_NOSITE)
	{   /* first time subquery is used so all sites are valid */
	    OPS_SUBQUERY	    *unionp;
	    MEcopy ( (PTR)varp->opv_grv->opv_gquery->ops_dist.opd_cost,
		(sitecount * sizeof(cop->opo_variant.opo_scost->opd_intcosts[0])),
		(PTR)cop->opo_variant.opo_scost);
	    if (unionp = varp->opv_grv->opv_gquery->ops_union)
	    {	/* if this is a union subquery then add up all the costs */
		for (;unionp; unionp = unionp->ops_union)
		{
		    OPD_ISITE	    siteno;
		    for (siteno = sitecount; --siteno >= 0; )
		    {
			OPO_COST	union_cost;

			union_cost = unionp->ops_dist.opd_cost->opd_intcosts[siteno];
			if ((union_cost < OPO_CINFINITE)
			    &&
			    (cop->opo_variant.opo_scost->opd_intcosts[siteno] < OPO_CINFINITE)
			    )
			{
			     union_cost += cop->opo_variant.opo_scost->opd_intcosts[siteno];
					    /* if any site is infinite then the final
					    ** cost is also infinite */
			    if (union_cost >= OPO_CINFINITE)
				union_cost = OPO_CINFINITE/2.0;
			    cop->opo_variant.opo_scost->opd_intcosts[siteno] = union_cost;
			}
		    }
		}
	    }
	    return;
	}
	/* a particular plan was previously chosen for this subquery */
	tsiteid = varp->opv_grv->opv_gquery->ops_dist.opd_target ;
	tcost = varp->opv_grv->opv_gquery->ops_dist.opd_cost->
	    opd_intcosts[tsiteid];
    }
    else
    {
	tsiteid = varp->opv_grv->opv_siteid;
	tcost = 0.0;
    }
    {	/* this "real relation" exists only on one site */
	while(--sitecount >= 0)
	{
	    if (sitecount == tsiteid)
		cbase->opd_intcosts[sitecount] = tcost; /* zero cost for
					** an ORIG node used for keying */
	    else
		cbase->opd_intcosts[sitecount] = OPO_INFINITE; /* keying
					** can only be done on the original site
					*/
	}
    }
}

/*{
** Name: opd_bflag	- routine to determine which DDB association to use
**
** Description:
**      Routine will determine which association to use based on table name prefix 
**      and the query mode 
**
** Inputs:
**      global                          ptr to state variable
**
** Outputs:
**	Returns:
**	    TRUE if the priveleged association should be used for this query
**	Exceptions:
**
** Side Effects:
**
** History:
**      6-apr-89 (seputis)
**          initial creation
**	03-apr-1995 (rudtr01)
**	    BUGS 49905 & 52601 fixed (and, I hope, other Star self-deadlock
**	    bugs.  Identified more query types that need to use the 
**	    privileged association.
[@history_template@]...
*/
bool
opd_bflag(
	OPS_STATE          *global)
{
    return
    (
	( (global->ops_qheader->pst_mode == PSQ_RETRIEVE) /* read-only */
	    ||
	  (global->ops_qheader->pst_mode == PSQ_RETINTO)  /* almost read-only */
	    ||
	  (global->ops_qheader->pst_mode == PSQ_DEFCURS)  /* almost read-only */
	    ||
	  (global->ops_qheader->pst_mask1 & PST_CATUPD_OK) /* cat upd priv */
	)
	&& 
	(global->ops_qheader->pst_mask1 & PST_CDB_IIDD_TABLES) 
    );			    /* 
			    ** this means that query contains only references
			    ** to iidd_* style tables , this will cause the
			    ** privileged association to be used so that
			    ** deadlocks will be avoided during queries
			    ** such as that produced by the HELP command 
			    */
}

/*{
** Name: opd_cluster	- check if name is in the set of node names in the cluster
**
** Description:
**      Check if name is in the set of names associated with the cluster 
**
** Inputs:
**      global                          ptr to global state variable
**      namep                           ptr to node name
**
** Outputs:
**	Returns:
**	    TRUE - is node name is in the ingres cluster
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      19-apr-89 (seputis)
**          initial creation
[@history_template@]...
*/
static bool
opd_cluster(
	OPS_STATE          *global,
	char               *namep)
{
    RDD_CLUSTER_INFO	    *clusterp;

    for (clusterp = global->ops_cb->ops_server->opg_cluster; 
	clusterp; clusterp = clusterp->rdd_1_nextnode)
    {
	if (   !MEcmp((PTR)namep, (PTR)&clusterp->rdd_2_node[0], 
		    sizeof(DD_NODE_NAME))
	    )
	    /* found the node name in the cluster */
	    return(TRUE);
    }
    return(FALSE);
}

/*{
** Name: opd_machineinfo	- get machine cost
**
** Description:
**      This routine will retrieve the machine and network costs for a particular site
[@comment_line@]...
**
** Inputs:
**      global                          ptr to global state variable
**      sitep                           ptr to site descriptor to initialize
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
**      17-apr-89 (seputis)
**          initial creation
**	25-feb-92 (barbara)
**	   Bug 39403.  Star's evaluation of the network costs of moving data 
**	   between sites was incorrect for non-clustered installations.  If 
**	   the Star server site was not on a cluster (no cluster.cnf file),
**	   the row in the iiddb_netcost table containing the server site as 
**	   a net_src/net_dest column entry never got considered.  Fixed this 
**	   by comparing the server site name directly with the net_src/
**	   net_dest column from iiddb_netcost.
**	   
[@history_template@]...
*/
static VOID
opd_machineinfo(
	OPS_STATE          *global,
	OPD_SITE           *sitep,
	bool		   target)
{
    DD_NODE_NAME	    *nodenamep;
    bool	    server_cpu;		/* TRUE if this is a node in the
					** ingres cluster upon which titan
					** will run */
    nodenamep = (DD_NODE_NAME *)sitep->opd_ldbdesc->dd_l2_node_name;
    server_cpu = opd_cluster(global, (char *)nodenamep);
    {	/* find the CPU costs */
	DD_NODELIST        *nodecostp; /* ptr to list of node descriptors */

	sitep->opd_nodecosts = NULL;
	for (nodecostp = global->ops_cb->ops_server->opg_nodelist; 
	    nodecostp; nodecostp = nodecostp->node_next)
	{
	    if (   !MEcmp((PTR)nodenamep, (PTR)&nodecostp->nodecosts.cpu_name[0], 
			sizeof(nodecostp->nodecosts.cpu_name))
		)
	    {
		/* found the cpu descriptor */
		sitep->opd_nodecosts = &nodecostp->nodecosts;
		break;
	    }
	    if (server_cpu
		&&
		opd_cluster(global, &nodecostp->nodecosts.cpu_name[0]) /* if this
				    ** is on the server site then we have
				    ** cluster information */
		)
	    {
		sitep->opd_nodecosts = &nodecostp->nodecosts; /* use info from
				    ** one CPU on the cluster, but continue
				    ** looking for the real one */
	    }
	}
	if (sitep->opd_nodecosts)
	{
	    if (sitep->opd_nodecosts->cpu_power <= 0.0)
		sitep->opd_nodecosts->cpu_power = 1.0; /* ensure valid value exists
					** FIXME - check in RDF */
	    if (sitep->opd_nodecosts->dio_cost <= 0.0)
		sitep->opd_nodecosts->dio_cost = 1.0;
	}
    }
    {	/* find the network costs */
	DD_NETLIST	*netcostp; /* ptr to list of net descriptors */

	sitep->opd_tosite = 0.0;
	sitep->opd_fromsite = 0.0;
	for (netcostp = global->ops_cb->ops_server->opg_netlist; 
	    netcostp; netcostp = netcostp->net_next)
	{
	    bool	server_dest;	/* TRUE if destination site is on server*/
	    bool	server_src;	/* TRUE if source site is on server*/

	    server_dest = opd_cluster(global, &netcostp->netcost.net_dest[0]);
	    server_src = opd_cluster(global, &netcostp->netcost.net_source[0]);
	    if ((   !MEcmp((PTR)nodenamep, (PTR)&netcostp->netcost.net_source[0], 
			sizeof(netcostp->netcost.net_source))
		    ||
		    (server_cpu && server_src) /* both current node and source
					** site of this tuple are equivalent
					** since they are on same cluster */
		)
		&&
		(   
		    !MEcmp((PTR)&netcostp->netcost.net_dest[0],
			(PTR)&global->ops_cb->ops_server->opg_ddbsite.dd_l2_node_name[0],
			sizeof(netcostp->netcost.net_dest))
		    ||
		    server_dest
		))
		/* found the a network cost descriptor */
		sitep->opd_fromsite = netcostp->netcost.net_cost;
	    if ((   !MEcmp((PTR)nodenamep, (PTR)&netcostp->netcost.net_dest[0], 
			sizeof(netcostp->netcost.net_dest))
		    ||
		    (server_cpu && server_dest) /* both current node and destination
					** site of this tuple are equivalent
					** since they are on same cluster */
		)
		&&
		(   
		    !MEcmp((PTR)&netcostp->netcost.net_source[0],
			(PTR)&global->ops_cb->ops_server->opg_ddbsite.dd_l2_node_name[0],
			sizeof(netcostp->netcost.net_source))
		    ||
		    server_src
		))
		/* found the a network cost descriptor */
		sitep->opd_tosite = netcostp->netcost.net_cost;
	}
	if (sitep->opd_tosite == 0.0)
	    sitep->opd_tosite = sitep->opd_fromsite;
	else if (sitep->opd_fromsite == 0.0)
	    sitep->opd_fromsite = sitep->opd_tosite; /* use existing costs
					** in case only one direction is given */
	if ((sitep->opd_tosite <= FMIN) || (sitep->opd_tosite >= 1.0))
	{   /* use defaults since the tuple was not available or
	    ** an unreasonable number was given */
	    if (target && (global->ops_gdist.opd_gmask & OPD_TOUSER))
		sitep->opd_tosite = 1.0/4096.0; /* since the tuples are just returned
					** to the star client, there is smaller
					** "to site" cost */
	    else
		sitep->opd_tosite = 1.0/1024.0;
	}
	if ((sitep->opd_fromsite <= FMIN) || (sitep->opd_fromsite >= 1.0))
	{   /* use defaults since the tuple was not available or
	    ** an unreasonable number was given */
	    if (target && (global->ops_gdist.opd_gmask & OPD_TOUSER))
		sitep->opd_fromsite = 1.0/4096.0; /* since the tuples are just returned
					** to the star client, there is smaller
					** "to site" cost */
	    else
		sitep->opd_fromsite = 1.0/1024.0;
	}
    }
}

/*{
** Name: opd_addsite	- add a site to the optimization
**
** Description:
**      This routine will add a site to the optimization if it is 
**      not already added.
**
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
[@PARAM_DESCR@]...
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**      19-nov-88 (seputis)
**          initial creation
**	06-apr-92 (bryanp)
**	    Add support for PSQ_DGTT_AS_SELECT for temporary tables.
**	11-nov-92 (barbara)
**	    Add support for delete cursor - follow same path as update cursor.
**       3-Mar-2010 (hanal04) Bug 123347
**          When node name and ldb name match the table is on the same site.
**          Set opd_dtable[OPD_CSITE] to the target_site to avoid 
**          uninitialised pointer references in opc_qtxt().
**	    
[@history_line@]...
[@history_template@]...
*/
VOID
opd_addsite(
	OPS_STATE	    *global,
	OPV_GRV		   *gvarp)
{
    OPD_ISITE	    maxsite;

    if (!global->ops_gdist.opd_base)
    {	/* initialize the site array for the target site and coordinator */
	char	    *temp;
	OPD_SITE    *target_site;
	DD_LDB_DESC *ldbdescp;
	bool	    non_retrieve;

	global->ops_gdist.opd_base = (OPD_DT *)opu_memory(global,
	    (i4)(sizeof(OPD_DT) + 2 * sizeof(OPD_SITE) ));
	temp = (char *)global->ops_gdist.opd_base;
	temp = &temp[sizeof(OPD_DT)];
	global->ops_gdist.opd_dv = 2;
	{   
            /* initialize the target site */
	    global->ops_gdist.opd_base->opd_dtable[OPD_TSITE] = 
	    target_site = (OPD_SITE *)temp;
	    target_site->opd_capability = 0; /* assume no restrictions for this
				    ** LDB */
            target_site->opd_dbcap = (DD_0LDB_PLUS *)NULL;
	    temp = &temp[sizeof(OPD_SITE)];
	    switch (global->ops_qheader->pst_mode)
	    {
	    case PSQ_RETINTO:
	    case PSQ_DGTT_AS_SELECT:
	    {
		ldbdescp =
		target_site->opd_ldbdesc = &global->ops_qheader->pst_distr->pst_ddl_info->
		    qed_d6_tab_info_p->dd_t9_ldb_p->dd_i1_ldb_desc;
		target_site->opd_dbcap = &global->ops_qheader->pst_distr->pst_ddl_info->
		    qed_d6_tab_info_p->dd_t9_ldb_p->dd_i2_ldb_plus;
		non_retrieve = TRUE;
		break;
	    }
	    case PSQ_DEFCURS:
	    case PSQ_RETRIEVE:
	    {
		target_site->opd_ldbdesc = &global->ops_cb->ops_server->opg_ddbsite;
		non_retrieve = (global->ops_gdist.opd_gmask & OPD_TOUSER) == 0;
		if (!non_retrieve)
		{
		    target_site->opd_capability = OPD_CTARGET;  /* current QEF
					** cannot process any data */
		    break;
		}
		/* FALL THRU - since this is an updatable cursor */
	    }
	    case PSQ_REPCURS:
	    case PSQ_REPLACE:
	    case PSQ_DELETE:
	    case PSQ_DELCURS:
	    {
		/* this point should only be reached if a TID is required
		** for an replace/delete, or an updateable cursor */
		if ((	(global->ops_cb->ops_server->opg_qeftype == OPF_GQEF) /* 
					** tids may not be available for gateways */
			&&
			(global->ops_rangetab.opv_base->opv_grv[global->ops_qheader->pst_restab.pst_resvno]
			    ->opv_relation->rdr_rel->tbl_status_mask 
			    & DMT_GATEWAY
			)		    /* is this a gateway table */
			&&
			(global->ops_cb->ops_server->opg_smask & OPF_TIDUPDATES) /* TRUE
					** is update by TID on this gateway table is
					** not supported */
		    )
		    ||
		    (	(gvarp->opv_relation->rdr_obj_desc->dd_o9_tab_info.dd_t9_ldb_p->
			    dd_i2_ldb_plus.dd_p3_ldb_caps.dd_c1_ldb_caps
			& DD_5CAP_TIDS)
			== 0
		    )
		    )
		    global->ops_gdist.opd_gmask |= OPD_NOUTID;
				    /* the target site does not have TIDs to
				    ** perform a link back operation so a
				    ** semi-join operation cannot be considered
				    ** for the target table to be updated */
		/* FALL THRU */
	    }
	    case PSQ_APPEND:
	    {	/* the first variable to be created should be the target variable
		** but check to make sure */
		OPV_IGVARS	grvno;
		grvno = global->ops_qheader->pst_restab.pst_resvno;
	        if ((grvno < 0)
		    ||
		    (grvno >= OPV_MAXVAR)
		    ||
		    !global->ops_rangetab.opv_base
		    ||
		    (global->ops_rangetab.opv_base->opv_grv[grvno] != gvarp))
		    opx_error(E_OP0A82_RESULT_VAR); /* unexpected result variable
				    ** value */
		ldbdescp = 
		target_site->opd_ldbdesc = &gvarp->opv_relation->rdr_obj_desc
		    ->dd_o9_tab_info.dd_t9_ldb_p->dd_i1_ldb_desc;
		target_site->opd_dbcap = &gvarp->opv_relation->rdr_obj_desc
		    ->dd_o9_tab_info.dd_t9_ldb_p->dd_i2_ldb_plus;
		non_retrieve = TRUE;
		break;
	    }
	    default:
		opx_error(E_OP048D_QUERYTYPE);
	    }
	}
	opd_machineinfo(global, target_site, TRUE);  /* test for cluster installation
				    ** and mark the site, all marked sites will
				    ** have lower CPU costs, also get the
				    ** site network cost and machine info */
	{
	    DD_LDB_DESC	    *coordinatorp;  /* FIXME - currently copy the SCF version
				    ** since we need to change ingres_b, but we should
				    ** remove ingres_b from the LDB structure since it
				    ** does not belong there, the LDB descriptor should
				    ** only have static components of the LDB */
	    bool	    ingres_b; /* create new LDB descriptor if ingres_b 
				    ** flag is not set correctly */

	    ingres_b = opd_bflag(global); /* get correct value of ingres flag */
	    if (non_retrieve
		&&
		!MEcmp((PTR)&ldbdescp->dd_l2_node_name[0],
			(PTR)&global->ops_cb->ops_coordinator->dd_i1_ldb_desc.dd_l2_node_name[0], 
			sizeof(ldbdescp->dd_l2_node_name))
		&&
		!MEcmp((PTR)&ldbdescp->dd_l3_ldb_name[0],
			(PTR)&global->ops_cb->ops_coordinator->dd_i1_ldb_desc.dd_l3_ldb_name[0], 
			sizeof(ldbdescp->dd_l3_ldb_name))
		)
	    {   /* both node name and ldb name match so table is on same site */
                global->ops_gdist.opd_base->opd_dtable[OPD_CSITE] = target_site;
		global->ops_gdist.opd_coordinator = OPD_TSITE;
		global->ops_gdist.opd_dv = 1;   /* coordinator and target of the retrieve into
					** are on the same site */
		if (ingres_b != target_site->opd_ldbdesc->dd_l1_ingres_b)
		{	/* the target site and ingres_b do not agree so create
		    ** a new descriptor for this update */
		    coordinatorp = (DD_LDB_DESC *)opu_memory(global, sizeof(*coordinatorp));
		    MEcopy((PTR)target_site->opd_ldbdesc,
			sizeof(*coordinatorp), (PTR)coordinatorp);
		    coordinatorp->dd_l1_ingres_b = ingres_b;
		    target_site->opd_ldbdesc = coordinatorp;
		}
	    }
	    else
	    {   /* initialize the coordinator site */

		global->ops_gdist.opd_base->opd_dtable[OPD_CSITE] = 
		target_site = (OPD_SITE *)temp;
		temp = &temp[sizeof(OPD_SITE)];
		target_site->opd_capability = 0;  /* assume no restrictions on the
					** coordinator DB */
		coordinatorp = (DD_LDB_DESC *)opu_memory(global, sizeof(*coordinatorp));
		MEcopy((PTR)&global->ops_cb->ops_coordinator->dd_i1_ldb_desc,
		    sizeof(*coordinatorp), (PTR)coordinatorp);
		
		coordinatorp->dd_l1_ingres_b = opd_bflag(global);
		/* FIXME - find a DD_LDB_DESC which
		** does not need to be copied because this field is
		** already FALSE 
		*/
		target_site->opd_ldbdesc = coordinatorp;
		target_site->opd_dbcap = &global->ops_cb->ops_coordinator->dd_i2_ldb_plus;
		opd_machineinfo(global, target_site, FALSE);  /* test for cluster installation
					** and mark the site, all marked sites will
					** have lower CPU costs, also get the
					** site network cost and machine info */
		global->ops_gdist.opd_coordinator = OPD_CSITE; /* currently the coordinator
					** will always default to a different site from the target
					** but if a distributed thread gets implemented
					** it could eventually be the same site, it is easier
					** to model the capabilities this way since the
					** coordinator site can execute queries, but the
					** target site cannot do this yet */
	    }
	}
    }
    maxsite = global->ops_gdist.opd_dv;
    if (gvarp)			    /* may be NULL for constant query */
    {	/* lookup site and initialize the  site info */
	OPD_ISITE   site;
	PTR	    nodep;	/* ptr to node name */
	PTR	    ldbp;	/* ptr to ldb name */
	nodep = (PTR)&gvarp->opv_relation->rdr_obj_desc->dd_o9_tab_info.
	    dd_t9_ldb_p->dd_i1_ldb_desc.dd_l2_node_name[0];
	ldbp = (PTR)&gvarp->opv_relation->rdr_obj_desc->dd_o9_tab_info.
	    dd_t9_ldb_p->dd_i1_ldb_desc.dd_l3_ldb_name[0];
	for (site = 0; site < maxsite; site++)
	{
	    if (!MEcmp((PTR)&global->ops_gdist.opd_base->opd_dtable[site]->opd_ldbdesc->dd_l2_node_name[0],
			(PTR)nodep, 
			sizeof(global->ops_gdist.opd_base->opd_dtable[site]->opd_ldbdesc->dd_l2_node_name))
		&&
		!MEcmp((PTR)&global->ops_gdist.opd_base->opd_dtable[site]->opd_ldbdesc->dd_l3_ldb_name[0],
			(PTR)ldbp, 
			sizeof(global->ops_gdist.opd_base->opd_dtable[site]->opd_ldbdesc->dd_l3_ldb_name))
		)
	    {	/* both node name and ldb name match so table is on same site */
		gvarp->opv_siteid = site;
		return;
	    }
	}
	/* site not found so add it to the set being considered */
	{
	    OPD_SITE	    *sitep;
	    site = global->ops_gdist.opd_dv++;	/* allocate new site
				    ** for this variable */
	    if (site >= OPD_MAXSITE)
		opx_error(E_OP0A00_MAXSITE);
	    global->ops_gdist.opd_base->opd_dtable[site] =
	    sitep = (OPD_SITE *) opu_memory(global, (i4)sizeof(OPD_SITE));
	    sitep->opd_capability = 0;	/* no restrictions on the site */
	    gvarp->opv_siteid = site;
	    sitep->opd_ldbdesc = &gvarp->opv_relation->rdr_obj_desc->
		dd_o9_tab_info.dd_t9_ldb_p->dd_i1_ldb_desc;
		/* save ptr to LDB descriptor for the site */
	    sitep->opd_dbcap =  &gvarp->opv_relation->rdr_obj_desc->
		dd_o9_tab_info.dd_t9_ldb_p->dd_i2_ldb_plus;
	    opd_machineinfo(global, sitep, FALSE);  /* test for cluster installation
				    ** and mark the site, all marked sites will
				    ** have lower CPU costs, also get the
				    ** site network cost and machine info */
	}
    }
}


/*{
** Name: opd_bestplan	- see if new CO produces a best plan for subquery
**
** Description:
**      This routine should only be called if a CO node which is the root 
**      has been processed.  The routine will determine whether this new plan
**      is the best for the root, or possibly one site on the root.
[@comment_line@]...
**
** Inputs:
**      subquery                        ptr to subquery state variable
**      cop                             ptr to possible best CO
**	sortrequired                    TRUE if a top sort node is required
**      sort_cpu                        cpu cost of the sort if sortrequired
**      sort_dio                        dio cost of the sort if sortrequired
**
** Outputs:
**      sortcopp                        ptr to CO ptr which will contain a
**                                      TOP sort node if sortrequired is
**                                      TRUE
**	Returns:
**	    TRUE - if plan was found to be useless, and was deallocated
**	Exceptions:
**
** Side Effects:
**	    updates subquery->ops_bestco or the array of best plans
**
** History:
**      22-sep-88 (seputis)
**          initial creation
**	12-nov-90 (seputis)
**	    fixed bull bug, floating point comparison problem
**	20-dec-90 (seputis)
**	    fix access violation
**      12-jun-91 (seputis)
**          fix b37907, add necessary CO nodes for multi-site best
**	    plans
**      14-apr-94 (peters)
**	    Added update of 'subquery->ops_cost' if current cost was lower than
**	    best previous cost, to make sure the timeout value reflects the
**	    current best plan.  Fixes bug 60500, where timeout was never reached
**	    because the initialization value for timeout was not updated.
**	31-Aug-2006 (kschendel)
**	    Watch for HFAGG as well as RFAGG.
**	13-may-10 (smeke01) b123727
**	    Increment best plan fragment for distributed query plans. Trace 
**	    discarded plan fragment for op215.
*/
bool
opd_bestplan(
	OPS_SUBQUERY       *subquery,
	OPO_CO             *newcop,
	bool		   sortrequired,
	OPO_CPU		   sort_cpu,
	OPO_BLOCKS	   sort_dio,
	OPO_CO		   **sortcopp)
{
    OPD_DT	    *sitepp;	    /* ptr to array of site descriptors */
    OPD_SCOST	    *costp;
    OPS_STATE	    *global;
    DD_COSTS	    *nodecostp;	    /* ptr to site cost descriptor currently
				    ** being analyzed */
    bool	    addnodes;	    /* TRUE - if co nodes need to be added
				    ** in case of copying CO nodes out of
				    ** enumeration memory */
    OPO_COST	    sort_cost;	    /* cost of adding a generic sort node */
    bool	    op215 = FALSE;

    global = subquery->ops_global;
    if (global->ops_cb->ops_check)
    {
	if (opt_strace( global->ops_cb, OPT_F087_ALLFRAGS))
	    op215 = TRUE;
    }

    sitepp = global->ops_gdist.opd_base;
    costp = newcop->opo_variant.opo_scost;
    addnodes = FALSE;
    if (sortrequired)
	sort_cost = opn_cost(subquery, sort_dio, sort_cpu);

    switch (subquery->ops_sqtype)
    {

    case OPS_MAIN:  /* consider only the target site */
    {
	OPO_COST	mcost;	    /* cost of new co node + sort if necessary */
	mcost = costp->opd_intcosts[OPD_TSITE];
	if ((mcost < OPO_CINFINITE) && sortrequired)
	{
	    nodecostp = sitepp->opd_dtable[OPD_TSITE]->opd_nodecosts;
	    if (nodecostp)
	        /* adjust costs if node descriptor is available */
		mcost += opd_adjustcost(subquery, nodecostp, sort_cpu, sort_dio);
	    else
		mcost += sort_cost;
	}
	if (subquery->ops_cost <= mcost)
	{
	    if (op215)
	    {
		global->ops_trace.opt_subquery = subquery;
		global->ops_trace.opt_conode = newcop;
		opt_cotree(newcop);
		global->ops_trace.opt_conode = NULL; 
	    }
	    opn_dcmemory(subquery, newcop); /* recover the memory for the newcop */
	    return(TRUE);	    /* this may occur if a join can occur on another
				    ** site more cheaply than the target site, FIXME
				    ** this assumes that for the prleaf call that
				    ** no query plan has been calculated, since one
				    ** does not want to deallocate for a DB_PR node */
	}
	if (sortrequired)
	{   /* FIXME - can detect user specified orderings here and avoid a sort node */
	    newcop =
	    *sortcopp = opn_oldplan(subquery, &subquery->ops_bestco); /* allocate prior
				    ** to deallocating plan in case of running out of
				    ** memory */
	}
	if (global->ops_gdist.opd_scopied) /* plan is copied out of enumeration
			    ** memory */
	    subquery->ops_global->ops_copiedco = 
		subquery->ops_bestco; /* this value
				** is used to mark a CO tree moved out of
				** enumeration memory */
	opn_dcmemory(subquery, subquery->ops_bestco); /* deallocate the current best plan */
	subquery->ops_dist.opd_target = OPD_TSITE;
	subquery->ops_bestco = newcop;
	global->ops_gdist.opd_scopied = FALSE;	/* mark query as not being copied out
				    ** of enumeration memory */
	subquery->ops_cost = mcost;
	break;
    }
    case OPS_SAGG:  /* simple aggregate requires the best plan
		    ** overall */
    {	/* see if the aggregate can be evaluated on any site more cheaply, it
	** is assumed that getting the scalar result of the simple aggregate
	** from any site is cheap */
	OPD_ISITE	site2;
	OPD_ISITE	dist_site;
	OPO_COST	jcost;

	jcost = subquery->ops_cost;
	dist_site = OPD_NOSITE;
	for (site2 = global->ops_gdist.opd_dv; --site2 >= 0;)
	{
	    OPO_COST	    new_1cost;	/* cost of producing the
				** simple aggregate result on this
				** site */
	    new_1cost = costp->opd_intcosts[site2];
	    if (new_1cost > OPO_CINFINITE)
		continue;
	    if (sortrequired)
	    {
		nodecostp = sitepp->opd_dtable[site2]->opd_nodecosts;
		if (nodecostp)
		    /* adjust costs if node descriptor is available */
		    new_1cost += opd_adjustcost(subquery, nodecostp, sort_cpu, sort_dio);
		else
		    new_1cost += sort_cost;
	    }
	    if (jcost > new_1cost)
	    {	/* new best site found */
		jcost = new_1cost;   /* new best cost is found */
		if (dist_site == OPD_NOSITE)
		{   /* first time need to deallocate previous plan */
		    if (sortrequired)
		    {   /* FIXME - can detect user specified orderings here and avoid a sort node */
			newcop =
			*sortcopp = opn_oldplan(subquery, &subquery->ops_bestco); /* allocate prior
						** to deallocating plan in case of running out of
						** memory */
		    }
		    if (global->ops_gdist.opd_scopied) /* plan is copied out of enumeration
					** memory */
			subquery->ops_global->ops_copiedco = 
			    subquery->ops_bestco; /* this value
					    ** is used to mark a CO tree moved out of
					    ** enumeration memory */
		    opn_dcmemory(subquery, subquery->ops_bestco); /* deallocate the current best plan */
		}
		dist_site = site2;   /* new site to evaluate distributed
				    ** simple aggregate */
		subquery->ops_bestco = newcop;
		global->ops_gdist.opd_scopied = FALSE;	/* mark query as not being copied out
					    ** of enumeration memory */
	    }
	}
	if (dist_site == OPD_NOSITE)
	    opx_error(E_OP0A01_USELESS_PLAN); /* plan should not have
				    ** been returned by opd_jcost if it
				    ** cannot be used */
	subquery->ops_dist.opd_target = dist_site;  /* save site of the
				    ** final join */
	subquery->ops_cost = jcost;
	break;
    }
    case OPS_FAGG:
    case OPS_RFAGG:
    case OPS_HFAGG:
    case OPS_RSAGG:
    case OPS_VIEW:
    case OPS_UNION:
    case OPS_SELECT:
    case OPS_PROJECTION:
    {
	OPD_ISITE	site1;
	bool		first_time;
	OPD_SCOST	*newcostp;
	OPD_SCOST	*curcostp;
	OPO_CO		*tempsortp;

	tempsortp = NULL;
	first_time = FALSE;
	newcostp = newcop->opo_variant.opo_scost;
	curcostp = subquery->ops_dist.opd_cost;
	for (site1 = global->ops_gdist.opd_dv; --site1 >= 0;)
	{
	    OPO_COST	    new_2cost;	/* cost of producing the
				** aggregate result on this
				** site */
	    new_2cost = newcostp->opd_intcosts[site1];
	    if (new_2cost > OPO_CINFINITE)
		continue;   /* cannot evaluate on this site for
			    ** some reason */
	    if (sortrequired)
	    {
		nodecostp = sitepp->opd_dtable[site1]->opd_nodecosts;
		if (nodecostp)
		    /* adjust costs if node descriptor is available */
		    new_2cost += opd_adjustcost(subquery, nodecostp, sort_cpu, sort_dio);
		else
		    new_2cost += sort_cost;
	    }
	    if (new_2cost >= OPO_CINFINITE)
		new_2cost = OPO_CINFINITE/2.0;
	    if ((curcostp->opd_intcosts[site1] < OPO_CINFINITE)
		&&
		(curcostp->opd_intcosts[site1] <= new_2cost)
		)
		continue;   /* plan is not better for this site */
	    if (subquery->ops_dist.opd_bestco->opd_colist[site1] /* a previous
					** plan exists and */
		)
	    {	/* try to recover the CO nodes which have been copied out of
		** enumeration memory, so that can be used in future processing
		** of the query */
		OPD_ISITE	site2;
		OPO_CO		*deletep;
		i4		delete_count;

		delete_count = 0;
		deletep = subquery->ops_dist.opd_bestco->opd_colist[site1];
		for (site2 = global->ops_gdist.opd_dv; --site2 >= 0;)
		{   /* check that this plan is not used more than once prior
		    ** to deallocating it */
		    if(subquery->ops_dist.opd_bestco->opd_colist[site1] == deletep)
			delete_count++;
		}
		if (delete_count == 1)
		{
		    if (global->ops_gdist.opd_copied[site1]) /* plan is copied out of enumeration
					** memory */
		    {
			subquery->ops_global->ops_copiedco = 
			    subquery->ops_dist.opd_bestco->opd_colist[site1]; /* this value
					    ** is used to mark a CO tree moved out of
					    ** enumeration memory */
			opn_dcmemory(subquery, subquery->ops_dist.opd_bestco->opd_colist[site1]); 
					    /* release the space for
					    ** previous best case CO tree and
					    ** replace it with this new one */
		    }
		    else
		    {	/* plan is still in enumeration memory so just deallocate
			** the top node if necessary */
			if (sortrequired)
			{   
			    OPO_CO	*tempcop;
			    tempcop = opn_oldplan(subquery, &subquery->ops_dist.opd_bestco->opd_colist[site1]); 
					/* allocate prior
					** to deallocating plan in case of running out of
					** memory */
			    if (tempsortp)
				opn_dcmemory(subquery, tempcop); /* already have a sort node */
			    else
				newcop = tempsortp = *sortcopp = opn_cmemory(subquery);	
							/* get sort node which may
							** be used in multiple places */
			}
			opn_dcmemory(subquery, subquery->ops_dist.opd_bestco->opd_colist[site1]); /* deallocate 
					** the current best plan */
		    }
		}
	    }
	    if (sortrequired && !tempsortp)
		tempsortp = newcop = *sortcopp 
		    = opn_cmemory(subquery); /* sort node was not retrieved
					** from deallocated plan */
	    global->ops_gdist.opd_copied[site1] = FALSE; /* plan is not copied out
					** out enumeration memory yet */
	    subquery->ops_dist.opd_bestco->opd_colist[site1] = newcop;
	    curcostp->opd_intcosts[site1] = new_2cost; /* may have sort cost
				    ** added so do not use newcostp */

	    /* If the cost of this plan is less than the current best
	    ** value, make sure subquery->ops_cost gets updated to maintain
	    ** the proper timeout limit.
	    */
	    if (new_2cost < subquery->ops_cost)
	        subquery->ops_cost = new_2cost;

	    first_time = TRUE;
	}
	if (!first_time)
	    opx_error(E_OP0A01_USELESS_PLAN); /* plan should not have
				    ** been returned by opd_jcost if it
				    ** cannot be used */
	addnodes = TRUE;
	break;
    }
    default:	    /* need a best CO tree for all sites */
	break;
    }
    if (sortrequired)
    {	/* copy the site costs from the child and add the sort costs
	** FIXME - eventually want to avoid sorting already sorted
	** attributes */
	OPD_ISITE	site4;
	OPD_SCOST	*stcostp; /* ptr to cost array for sort node */
	stcostp = (*sortcopp)->opo_variant.opo_scost;
	for (site4 = global->ops_gdist.opd_dv; --site4 >= 0;)
	{   
	    OPO_COST	    tempcost;

	    nodecostp = sitepp->opd_dtable[site4]->opd_nodecosts;
	    if (nodecostp)
		/* adjust costs if node descriptor is available */
		tempcost = opd_adjustcost(subquery, nodecostp, sort_cpu, sort_dio);
	    else
		tempcost = sort_cost;
	    tempcost = costp->opd_intcosts[site4] + tempcost;
	    if (tempcost < OPO_CINFINITE)
		stcostp->opd_intcosts[site4] = tempcost;
	    else
		stcostp->opd_intcosts[site4] = OPO_INFINITE;
	}
    }
    subquery->ops_tplan = subquery->ops_tcurrent;
    /* save the best fragment so far for trace point op188/op215/op216 */
    subquery->ops_bestfragment = subquery->ops_currfragment;  
    if (addnodes)
    while (global->ops_estate.opn_cocount < global->ops_estate.opn_corequired)
    {
	/* need to allocate CO nodes, part of fix to 37907, so that
	** if CO nodes are copied out of enumeration memory, there will 
	** be enough space */
	OPO_CO	    *tempcop;
	tempcop = opn_cmemory(subquery);
	opn_dcmemory(subquery, tempcop);
    }
    return(FALSE);
}

/*{
** Name: opd_recover	- reset variables lost on out of memory recover
**
** Description:
**      If the number of variables in the optimization is large then 
**      typically the optimizer will run out of memory.  This routine will 
**      reset those variables so that subsequent execution will not 
**      use memory which has not been allocated. 
**
** Inputs:
**      global                          optimizer state variable
**
** Outputs:
[@PARAM_DESCR@]...
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    reset global variables
**
** History:
**      14-mar-89 (seputis)
**          initial creation
[@history_template@]...
*/
VOID
opd_recover(
	OPS_STATE          *global)
{
    global->ops_gdist.opd_tcost = NULL;
    global->ops_gdist.opd_tbestco = NULL;
}
