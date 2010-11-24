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
#include    <ade.h>
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
#define             OPE_FINDJOINS       TRUE
#include    <me.h>
#include    <bt.h>
#include    <opxlint.h> /* EXTERNAL DEFINITION */ 
/**
**
**  Name: OPEFINDJ.C - find join clauses and create equivalence classes
**
**  Description:
**      These routines find joining clauses and replace them by equivalence
**      classes.
**
**  History:    
**      18-jun-86 (seputis)    
**          initial creation from jndoms.c
**      16-aug-91 (seputis)
**          add CL include files for PC group
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
[@history_line@]...
**/



/*{
** Name: ope_explicit	- fix up explicit joins to secondaries
**
** Description:
**      This routine is called if the user makes an explicit reference to a 
**      secondary index, which may be explicitly linked to the primary.  This
**      is done if the user wishes to force the use of an index, but it may
**      be incomplete in that all attributes are not joined when they could be
**      so that the transitive property of equivalence classes is not fully
**      utilized.  This routine makes sure all useful secondary attributes are
**      in the proper equivalence classes of the primary
**
** Inputs:
**      subquery                        subquery of relation being analyzed
**      explicit                        range var number of explicitly 
**                                      referenced secondary index
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
**      17-sep-87 (seputis)
**          initial creation
**      23-oct-88 (seputis)
**          change interface to ope_addeqcls and ope_findjoins
**      19-dec-89 (seputis)
**          fix explicit index reference bug
**      29-mar-90 (seputis)
**          fix RDF race condition problem
**      13-oct-90 (seputis)
**          b14544 - make sure explicit secondary indexes have placement
**	    restrictions placed on them
**	26-feb-91 (seputis)
**	    - add improved diagnostics for b35862
**	19-jul-00 (hayke02)
**	    Set OPV_EXPLICIT_IDX in opv_rt[explicit]->opv_mask to indicate
**	    that this index is explicitly defined in the query. This flag
**	    is then used in opn_iarl() to prevent the index being
**	    eliminated  - during the 'index guess' process - because it is
**	    more expensive than just using the base relation. This change
**	    fixes bug 101959.
**      21-Oct-2003 (hanal04) Bug 103856 INGSRV2557
**          Modified ope_jnclaus() to prevent protocol errors in ESQLC
**          front ends when result sets are supplied using a different
**          db_datatype to the expected db_datatype.
**	27-Jul-2004 (schka24)
**	    Make sure we maintain all RDF info that optimizer is interested
**	    in, in the (rare?) case that we have to ask for index info.
**	9-Dec-2009
**		Replaced occurances of 
**		subquery->ops_global->ops_cb->ops_server->opg_ifnull with the new
**		ADI_F32768_IFNULL flag
**	14-Dec-2009 (drewsturgiss)
**		Repaired tests for ifnull, tests are now 
**		nodep->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_fiflags & 
**		nodep->ADI_F32768_IFNULL
[@history_template@]...
*/
static VOID
ope_explicit(
	OPS_SUBQUERY       *subquery,
	OPV_IVARS          explicit)
{   /* a secondary index was referenced explicitly in the query */
    OPZ_IATTS       maxattrs;   /* maximum number of joinop attributes */
    OPE_ET	    *ebase;	/* base of array of ptrs to equivalence class
				** descriptors */
    OPZ_AT	    *abase;	/* base of array of ptrs to joinop attribute
                                ** descriptors */
    OPV_RT	    *vbase;	/* base of array of ptrs to range var
				** descriptors */
    DMT_IDX_ENTRY   *index_ptr; /* ptr to index descriptor for explicitly
				** referenced secondary */
    OPV_VARS        *lvar_ptr;  /* ptr to local joinop range table descriptor
				** of secondary */
    OPV_IVARS	    primary;	/* primary range var number for
				** base relation of explicitly referenced
				** secondary */
    OPZ_BMATTS      *varattrmap; /* bit map of attributes referenced in the
                                ** query belonging to the secondary index  */
    OPZ_IATTS       ex_attr;    /* used to traverse the joinop attributes of
				** the explicitly referenced index */
    vbase = subquery->ops_vars.opv_base;
    ebase = subquery->ops_eclass.ope_base;
    abase = subquery->ops_attrs.opz_base;
    maxattrs = subquery->ops_attrs.opz_av;
    index_ptr = NULL;		/* NULL indicates the index descriptor has not
				** been found */
    lvar_ptr = vbase->opv_rt[explicit]; /*get ptr to joinop range table element
				** being analyzed
				*/
    varattrmap = &lvar_ptr->opv_attrmap; /* bitmap of joinop attributes
				** referenced by the index */

    for (ex_attr = -1; !index_ptr 
		    &&
		    (ex_attr = BTnext((i4)ex_attr, 
				       (char *)varattrmap,
				       (i4)maxattrs)
		      ) >= 0;)
    {
	OPE_IEQCLS	    eqcls; /* used to look for TID equivalence
				** class */

	eqcls = abase->opz_attnums[ex_attr]->opz_equcls;
	if ((eqcls != OPE_NOEQCLS)
	    &&
	    (ebase->ope_eqclist[eqcls]->ope_eqctype == OPE_TID)
	    &&
	    (lvar_ptr->opv_grv->opv_relation->rdr_no_attr
	     == abase->opz_attnums[ex_attr]->opz_attnm.db_att_id)
				/* by convention the tidp attribute
				** is always the last attribute of
				** the index */
	   )
	{   /* TID equivalence class has been found so check
	    ** if a primary relation is referenced in this
	    ** equivalence class, note that the transitive
	    ** property can only be applied if a TID join exists
	    ** and it is the TID of the base relation */
	    OPZ_BMATTS	    *attrmap;
	    OPZ_IATTS       attr;

	    attrmap = &ebase->ope_eqclist[eqcls]->ope_attrmap; /* get
				    ** the attribute map for the
				    ** TID equivalence class */
	    for (attr = -1; !index_ptr
			    &&
			    (attr = BTnext((i4)attr, 
					       (char *)attrmap,
					       (i4)maxattrs)
			      ) >= 0;)
	    {	/* find an implicit TID and check if this is the
		** base relation for the secondary index */
		if (abase->opz_attnums[attr]->opz_attnm.db_att_id == DB_IMTID )
		{   /* implicit TID attribute found */
		    OPV_VARS	    *pvarp; /* ptr to possible primary
				    ** for the secondary index */
		    primary = abase->opz_attnums[attr]->opz_varnm;
		    pvarp = vbase->opv_rt[primary];
		    if (!(pvarp->opv_grv->opv_relation->rdr_rel
			    ->tbl_status_mask & DMT_IDX) /* is this
				    ** a base relation? */
			&&
			    (lvar_ptr->opv_grv->opv_relation->rdr_rel
				->tbl_id.db_tab_base
			    ==
			    pvarp->opv_grv->opv_relation->rdr_rel
				->tbl_id.db_tab_base
			    )	    /* if the db_tab_base is the same
				    ** then this should be the correct
				    ** base relation i.e. the user
				    ** could try to join to the
				    ** wrong base relation for some
				    ** reason ?? */
			)
		    {	/* Try to find the correct index tuple */
			OPV_IINDEX         ituple; /* index into the 
				    ** array of index tuples associated 
				    ** with global range table */
			DB_TAB_NAME	   *index_name; 

			if( !pvarp->opv_grv->opv_relation->rdr_indx )
			{   /* call RDF to get the index descriptor information
			    ** so that the relationship between the attributes
			    ** can be determined */
			    RDF_CB	*rdfcb;	/* ptr to RDF control block 
					** used to obtain index information
					*/
			    i4     status; /* RDF return status */

			    rdfcb = &subquery->ops_global->
				ops_rangetab.opv_rdfcb;
			    STRUCT_ASSIGN_MACRO( 
				subquery->ops_global->ops_qheader->
				  pst_rangetab[pvarp->opv_grv->opv_qrt]->pst_rngvar, 
				rdfcb->rdf_rb.rdr_tabid); /* get table id 
					** from parser range table */
			    /* Ask for all info including index info.  We
			    ** want everything because we're going to
			    ** substitute the (possibly) new RDF info for
			    ** what we had before, and we don't want to
			    ** lose anything if RDF decides to give us a
			    ** new private info block.
			    ** If we have some of this already it doesn't
			    ** cost more to ask.
			    */
			    rdfcb->rdf_rb.rdr_types_mask = RDR_RELATION |
				RDR_ATTRIBUTES | RDR_BLD_PHYS | RDR_INDEXES;
			    rdfcb->rdf_info_blk = pvarp->opv_grv->opv_relation;
			    
			    status = rdf_call( RDF_GETDESC, rdfcb);
			    if ((status != E_RD0000_OK)
				||
				!pvarp->opv_grv->opv_relation->rdr_indx )
				continue; /* if index information not 
					** available then continue 
					** processing */
			    pvarp->opv_grv->opv_relation = rdfcb->rdf_info_blk;
					    /* RDF may change the underlying
					    ** descriptor so may sure PTR is
					    ** to current value */
			}
			index_name = &lvar_ptr->opv_grv->
			    opv_relation->rdr_rel->tbl_name; /* get ptr
				    ** to name of index */
			for( ituple = pvarp->opv_grv->opv_relation->
				rdr_rel->tbl_index_count;
			    ituple-- ;)
			{   /* look thru all index tuples associated
			    ** with the primary relation */
			    if (!MEcmp( 
				(PTR)&pvarp->opv_grv->opv_relation->
				    rdr_indx[ituple]->idx_name,
				(PTR)index_name, 
				sizeof(DB_TAB_NAME))
				)
			    {
				index_ptr = pvarp->opv_grv->
				    opv_relation->rdr_indx[ituple];
				lvar_ptr->opv_index.opv_poffset = primary; /*
				    ** save the reference to the primary */
				subquery->ops_mask |= OPS_CPLACEMENT;
				lvar_ptr->opv_mask |= OPV_CPLACEMENT; /* assume
							** index will be placed
							** with a TID join */
				if (pvarp->opv_mask & OPV_FIRST)   
				{
				    pvarp->opv_mask |= OPV_SECOND;         
				    subquery->ops_mask |= OPS_MULTI_INDEX;   
				}
				else
				    pvarp->opv_mask |= OPV_FIRST;
				break; /* if the names are equal then
				    ** the correct index tuple has been
				    ** found */
			    }
			}
		    }
		}
	    }
	}
    }

    if (index_ptr)
    {	/* if the index descriptor was found then the relationship between
	** the explicitly referenced secondary index and the primary
	** can be determined */
	OPZ_DMFATTR	indexattr;	    /* attribute number of the secondary
					    ** index */

	vbase->opv_rt[primary]->opv_mask |= OPV_EXPLICIT;
	vbase->opv_rt[explicit]->opv_mask |= OPV_EXPLICIT_IDX;
					    /* indicate an explicit
					    ** index is referenced */
	for (indexattr = index_ptr->idx_array_count; indexattr; indexattr--)
	{   /* for each attribute in the secondary index */
	    /* FIXME - some attributes may not be in key, check for these */
	    OPZ_DMFATTR	    baseattr;	    /* attribute number of the base
					    ** relation */
	    OPZ_IATTS	    jindexattr;     /* joinop attribute number for
					    ** the secondary */
	    OPZ_IATTS       jbaseattr;      /* joinop attribute number for
					    ** the primary */
	    baseattr = index_ptr->idx_attr_id[indexattr-1];
	    jbaseattr = opz_findatt( subquery, primary, baseattr);
	    jindexattr = opz_findatt(subquery, explicit, indexattr);
	    if (jbaseattr == OPZ_NOATTR)
	    {
		if (jindexattr == OPZ_NOATTR)
		    continue;		/* attribute is not referenced
					    ** in the query at all */
		else
		{	/* secondary attribute is referenced so create a
		    ** primary attribute and add it to the secondary
		    ** attribute equivalence class */
		    bool		success;

		    jbaseattr = opz_addatts( subquery, primary, 
			(OPZ_DMFATTR)baseattr, 
			&abase->opz_attnums[jindexattr]->opz_dataval ); /*
					    ** create joinop attribute */
		    if (abase->opz_attnums[jindexattr]->opz_equcls 
			== 
			OPE_NOEQCLS)
			(VOID)ope_neweqcls( subquery, jindexattr);
					    /* joinop attribute may
					    ** have only been in the target list
					    ** but not in the qualification
					    ** - still useful if base relation
					    ** can be replaced by index
					    */
		    
		    success = ope_addeqcls( subquery, 
			abase->opz_attnums[jindexattr]->opz_equcls, 
			jbaseattr, TRUE);   /* add the joinop attribute for index 
					    ** to the equivalence class containing
					    ** the corresponding base relation
					    ** attribute, ... do not remove NULLs
					    ** for this join since it is based on
					    ** an index, even though it is explicitly
					    ** referenced, probably a good decision
					    ** to make explicitly and implicitly
					    ** referenced secondaries behave the
					    ** same
					    */
#ifdef    E_OP0385_EQCLS_MERGE
		    if (!success)
			opx_error(E_OP0385_EQCLS_MERGE);
#endif
		}
	    }
	    else
	    {   /* the base attribute is referenced in the query so
		** check if the index attribute is referenced */
		if (jindexattr == OPZ_NOATTR)
		{   /* if the index attribute is not referenced then
		    ** create it and add it to the equivalence class of
		    ** the primary */
		    bool	    success;

		    jindexattr = opz_addatts( subquery, explicit, 
			(OPZ_DMFATTR)indexattr, 
			&abase->opz_attnums[jbaseattr]->opz_dataval ); /*
					    ** create joinop attribute */
		    if (abase->opz_attnums[jbaseattr]->opz_equcls 
			== 
			OPE_NOEQCLS)
			(VOID)ope_neweqcls( subquery, jbaseattr);
					    /* joinop attribute may
					    ** have only been in the target list
					    ** but not in the qualification
					    ** - still useful if base relation
					    ** can be replaced by index
					    */
		    
		    success = ope_addeqcls( subquery, 
			abase->opz_attnums[jbaseattr]->opz_equcls, 
			jindexattr, TRUE);  /* add the joinop attribute for index 
					    ** to the equivalence class containing
					    ** the corresponding base relation
					    ** attribute
					    */
#ifdef    E_OP0385_EQCLS_MERGE
		    if (!success)
			opx_error(E_OP0385_EQCLS_MERGE);
#endif
		}
		else
		    (VOID) ope_mergeatts( subquery, jbaseattr, jindexattr, TRUE, 
			OPL_NOOUTER);	    /* TRUE means do not remove NULLs due
					    ** to this combination of eqcls */
	    }
	}
    }
}

/*{
** Name: ope_rqfunc 	- determines if this is a funcattr with a repeat
**			query parm whose length is unknown
**
** Description:
**		Does recursive descent of parse tree fragment, looking for 
**		function (PST_UOP/BOP) returning unknown length
**
** Inputs:
**	subquery			ptr to subquery being processed
**      nodep   			ptr to qualification parse tree 
**					fragment being analyzed
**
** Outputs:
**	Returns:
**	    TRUE if this qualification involves a func attr with a repaet
**		query parm with unknown length
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	1-dec-98 (inkdo01)
**	    Written (to detect potentially useless func attrs involving 
**	    repeat query parms).
[@history_line@]...
*/
static bool
ope_rqfunc(
	OPS_SUBQUERY        *subquery,
	PST_QNODE           *nodep)

{

    /* Just recurse down right side and iterate down left side. As soon 
    ** as a PST_UOP or PST_BOP is found with ADE_LEN_UNKNOWN, return TRUE
    ** back up the call stack. */

    while (TRUE)	/* exit by return */
    switch (nodep->pst_sym.pst_type) {

       default:		/* everything other than PST_UOP/BOP */
	if (nodep->pst_right && ope_rqfunc(subquery, nodep->pst_right))
	    return(TRUE);	/* recurse on the right */
	if ((nodep = nodep->pst_left) == NULL) return(FALSE);
	continue;		/* iterate on the left */

       case PST_BOP:
	if (nodep->pst_sym.pst_dataval.db_length == ADE_LEN_UNKNOWN)
	    return(TRUE);	/* this is the magic condition */
	if (nodep->pst_right && ope_rqfunc(subquery, nodep->pst_right))
	    return(TRUE);	/* recurse on the right */
	if ((nodep = nodep->pst_left) == NULL) return(FALSE);
	continue;		/* iterate on the left */

       case PST_UOP:
	if (nodep->pst_sym.pst_dataval.db_length == ADE_LEN_UNKNOWN)
	    return(TRUE);	/* this is the magic condition */
	if ((nodep = nodep->pst_left) == NULL) return(FALSE);
	continue;		/* iterate on the left */
    }	/* end of switch and endless loop */

}

/*{
** Name: ope_jnclaus	- determine if this is a joining clause
**
** Description:
**	return true if this is a joining clause, i.e. of the form
**	var1 = var2
**
**      restrict joins to be on atts of same type so that a query will
**      always give the same answer regardless of the query execution plan.
**
**      example:
**
**	r.dat = s.chr		is the joining clause.
**
**	r   dat            s   chr
**      -------------      -------------
**	    1-jan-82           1-jan-82
**	    jan-1-82           jan-1-82
**          82-jan-1           82-jan-81
**
**      suppose r and s are hashed on their atts then
**      if r is the outer, we will have to convert the date field to a char
**      in order to do a hash lookup on s in which case we get only 1 or 3
**      tups in the answer, depending on how the date to character conversion
**      works.  if s is the outer, then we get 9 tups cause all the chars
**      convert to the same internal form of date.
**
**      now this assumes that the converted value is what is used both for
**      hashing and for the equality check that we do on the all the tuples that
**      we scan when running down the primary and overflow pages.  we could
**      however, always convert the date to a char when doing this equality
**      conversion.  this does not work, however, cause the hash may have 
**      brought us to the wrong primary page.
**
**      so we do not use this clause for a join predicate, rather it is just
**      a regular boolean factor which is passed to psetmap which determines
**      what gets converted to what and how the type conversion is done.
**      if this was the only join predicate between these relations then a
**      cartesean product is done.
**
**      another problem occurs if we have r.a = s.a and s.a = q.a where
**      r.a is say a date, s.a is a char, and q.a is, say, a money type.
**      now there are converstions from char to date and from char to money type
**      but none from date to money type.  so if we allowed the above join
**      clauses then we might try to join r and q first, and there would be no
**      way to convert the date and money type so that we could compare them.
**
**      If joining on function or different types, create a new attribute of 
**      the proper type.  This implements joins on functions.
**
** Inputs:
**	subquery			ptr to subquery being processed
**      and_node			ptr to query tree node representing
**                                      the qualification (conjunct)
**      lvarmap                         varmap of left part of conditional
**      rvarmap                         varmap of right part of conditional
**      oldnodep                        NULL if not a null join
**                                      - ptr to "IS NULL" test for null join
**                                      if not null
**
** Outputs:
**	Returns:
**	    TRUE if this qualification is a joining clause i.e. of the form
**          var1 == var2
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	20-apr-86 (seputis)
**          initial creation from jnclaus
**	24-oct-88 (seputis)
**          change ope_mergeatts to support NULL preserving joins
**      13-apr-92 (seputis)
**          fix bug 43569 - pass pointer to original null join
**          qual to function attribute routines.
**	21-may-93 (ed)
**	    fix outer join problem which occurs when function attributes
**	    are calculated using attributes from an outer join
**	15-jan-94 (ed)
**	    keep ON clause boolean factors including equi-joins
**	    available to assist in relation placement
**	15-apr-94 (ed)
**	    b59588 - create iftrue for function attribute if variable can
**	    have nulls introduced by a lower scoped outer join, otherwise
**	    do not introduce function attribute.
**	6-may-94 (ed)
**	    - null joins were not reconstructing the qualifications completely
**	    if the equivalence class insertions failed
**	    - this will be more common for nested outer joins in which
**	    null joins need to be used for relation placement
**	14-apr-97 (inkdo01)
**	    Minor changes to support noncnf nulljoin calls.
**	27-jul-98 (hayke02 from pchang)
**	    fix bug 42451 - do not create function attributes on a join clause 
**	    which consists of a boolean expression that contains a repeated 
**	    query parameter (e.g. r.var1 = s.var2 + :rpt_parm) since such a 
**	    boolean factor cannot be evaluated until run-time we cannot
**	    accomplish a join by substituting its associated function
**	    attributes with equivalence classes.
**	1-dec-98 (inkdo01)
**	    Disallow funcattr exprs involving RQ params. We're not architected
**	    to pass such things around merge join strategies.
**	3-dec-02 (inkdo01)
**	    Change lvarmap, rvarmap to OPV_GBMVARS * for range table expansion.
**	10-sep-03 (hayke02)
**	    Create function attributes in the presence of a collation sequence
**	    and a join between character types. This prevents incorrect results
**	    when we use data from another attribute in the joining equivalence
**	    class. This change fixes bug 110675.
**      21-Oct-2003 (hanal04) Bug 103856 INGSRV2557
**          Create function attributes in the presence of VARs with different
**          nullability and/or length if this is a distributed query using
**          cursors. 
**	 6-sep-04 (hayke02)
**	    Back out changes made in fix for 110675/INGSRV2450 and fix instead
**	    in opz. This change fixes problem INGSRV 2940, bug 112873.
**      27-jan-2005 (huazh01)
**          do not merge equivalence classes for the subselect query that 
**          has been transformed into an outer join. 
**          INGSRV3015, b113296.
**	16-aug-05 (inkdo01)
**	    Remove above change.
**	16-aug-05 (inkdo01)
**	    Init previously uninit'ed local ojid.
**      27-feb-2007 (huazh01)
**          don't create func attr for OJ ifnull() CP-Prod query, which 
**          means there is no join conditions on the 'ON' clause between 
**          relations specified in OJ. An example query is: 
**          select ... from a left join b on b.col_1 >= 123456 and 
**          b.col2_2 <= 654321 where ... AND ifnull(b.col_3, 0) = a.col.
**          This fixes b117793. 
**	14-Dec-2009 (drewsturgiss)
**		Repaired tests for ifnull, tests are now 
**		nodep->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_fiflags & 
**		nodep->ADI_F32768_IFNULL
**      3-jun-2010 (huazh01)
**          Modify the fix to b117793 by using pst_joinid to test if
**          the ifnull() is in the where clause of the query. (b123744) 
**      7-jun-2010 (huazh01)
**          Rewrite the fix to b117793. We now don't create func attr
**          for ifnull() if:
**          1) ifnull() is in the where clause.
**          2) variables on both side of the and_node, i.e., lvarmap
**          and rvarmap, are referenced in an outer join. 
**          (b123419)
[@history_line@]...
*/
static bool
ope_jnclaus(
	OPS_SUBQUERY        *subquery,
	PST_QNODE           *and_node,
	OPV_GBMVARS	    *lvarmap,
	OPV_GBMVARS	    *rvarmap,
	PST_QNODE           *oldnodep,
	PST_QNODE           *inodep,
	bool		    *funcattr)
{
    PST_QNODE   *l;		/* left side of qualification */
    PST_QNODE   *r;		/* right side of qualification */
    OPL_IOUTER	ojid = OPL_NOOUTER; /* ojid of AND node if outer joins are
				** present */
    bool	left_iftrue;	/* TRUE - if iftrue function needs to be
				** placed on left side */
    bool	right_iftrue;	/* TRUE - if iftrue function needs to be
				** placed on right side */

    left_iftrue = right_iftrue = FALSE;
    *funcattr = FALSE;
    /* note that there has been a check to ensure that the left and
    ** right branches of the qualification do not have identical varmaps
    */

    l = and_node->pst_left->pst_left;
    r = and_node->pst_left->pst_right;

    if (subquery->ops_oj.opl_base)
    {   /* test for special case of equality being referenced in
        ** parent outer join */
	OPV_RT	    *vbase;

	vbase = subquery->ops_vars.opv_base;
        ojid = and_node->pst_left->pst_sym.pst_value.pst_s_op.pst_joinid;
        if ((ojid >= 0)
            &&
            !opl_fojeqc(subquery, lvarmap, rvarmap, ojid /*, &savequal */))
            return(FALSE);      /* outer join semantics do not allow
                                ** this equality to be an equivalence class,
                                ** since it would eliminate tuples too early */
	if (oldnodep
	    &&
	    (	(vbase->opv_rt[l->pst_sym.pst_value.pst_s_var.pst_vno]
		    ->opv_ojeqc != OPE_NOEQCLS)
		||
		(vbase->opv_rt[r->pst_sym.pst_value.pst_s_var.pst_vno]
		    ->opv_ojeqc != OPE_NOEQCLS)
	    ))
	{   /* check for special case in which a full join eqcls is subsequently
	    ** used in a null join,... convert this attribute to an iftrue
	    ** function attribute so that the correct NULL value is used in
	    ** the equi-join e.g.
	    **	    select r1, r2, s1, s2, t1, t2 from T,
	    **		( R full join S on r1=s1 )
	    **	    where ( t1=r1 or ( t1 is null and r1 is null ) )
	    */
	    OPL_IOUTER	    ojid1;
	    for (ojid1 = subquery->ops_oj.opl_lv; --ojid1 >= 0;)
	    {
		OPL_OUTER	*ojp;
		if (ojid == ojid1)
		    continue;
		ojp = subquery->ops_oj.opl_base->opl_ojt[ojid1];
		if (ojp->opl_type == OPL_FULLJOIN)
		{
		    if ((ojid < 0)
			||
			!BTtest((i4)ojid, (char *)ojp->opl_idmap))
		    {	/* found FULL join which may contain variables */
			left_iftrue |= BTtest((i4)l->pst_sym.pst_value.
			    pst_s_var.pst_vno, (char *)ojp->opl_bvmap);
			right_iftrue |= BTtest((i4)r->pst_sym.pst_value.
			    pst_s_var.pst_vno, (char *)ojp->opl_bvmap);
		    }
		}   
	    }
	}
    }

    /* assume that if a VAR node is the left AND right part and the types
    ** are equal then there will be no conversion IDs in the parent
    */
    if (!left_iftrue 
	&&
	!right_iftrue
	&&
	(l->pst_sym.pst_type == PST_VAR) 
	&&
	(r->pst_sym.pst_type == PST_VAR) 
	&&
        ( ((subquery->ops_global->ops_cb->ops_smask & OPS_MDISTRIBUTED)
           &&
           (subquery->ops_global->ops_qheader->pst_mode == PSQ_DEFCURS)
           &&
           (l->pst_sym.pst_dataval.db_datatype
           ==
           r->pst_sym.pst_dataval.db_datatype)
           &&
           (l->pst_sym.pst_dataval.db_length
           ==
           r->pst_sym.pst_dataval.db_length))
          ||
          ((((subquery->ops_global->ops_cb->ops_smask & OPS_MDISTRIBUTED) == 0)
           ||
           (subquery->ops_global->ops_qheader->pst_mode != PSQ_DEFCURS))
           &&
           (abs(l->pst_sym.pst_dataval.db_datatype)
           ==
           abs(r->pst_sym.pst_dataval.db_datatype)))
        )
        )
    {
# ifdef E_OP0380_JNCLAUS
	/* consistency check to see if assumption is true; i.e. if we
	** reach this point then the var number are not equal since
	** this was asserted by a previous varmap check, and the joinop
	** datatypes are also equal since all similiar var nodes have the
	** same data type
	*/
	OPZ_AT                 *base; /* base of joinop attributes array */
	OPZ_ATTS               *lap;/* left node's joinop attribute ptr */
	OPZ_ATTS               *rap;/* right node's joinop attribute ptr */

	base = subquery->ops_attrs.opz_base;
	lap = base->opz_attnums
	    [l->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id];
	rap = base->opz_attnums
	    [r->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id];
	if ((lap->opz_varnm == rap->opz_varnm )
            ||
	        (abs(lap->opz_dataval.db_datatype) 
		 != 
	         abs(rap->opz_dataval.db_datatype)
                )
	   )
	    opx_error(E_OP0380_JNCLAUS);
# endif
        return( ope_mergeatts(subquery, 
            (OPZ_IATTS)l->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id,
	    (OPZ_IATTS)r->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id,
            (oldnodep != NULL), ojid)); /* return if  a "normal"
                                        ** equi-join has been found; i.e. that
                                        ** did not need function attributes
                                        ** - FALSE means that the join does
                                        ** remove NULLs since it is explicitly
                                        ** qualified by the user
                                        */
    }
    /* Before saving funcattr for later perusal, make sure it doesn't
    ** contain a potential VLUP (RQ parm whose length cannot yet be 
    ** predicted. These can't be used in merge joins because of the way
    ** they're evaluated. */
    if (ope_rqfunc(subquery, and_node->pst_left))
    {
	return( FALSE );
    }


    /* b117793: 
    ** do not create functional attribute for OJ ifnull() CP-prod query.
    ** creating func attr will cause wrong result problem because opv_ijnbl()
    ** can't handle it. In opv_ijnbl(), check the code with comments  
    ** "...leave SVARs with ref to inner var of OJ. Can't eval 'til join 
    ** itself.". 
    */

    {
       i4 		i; 
       bool 		found = FALSE; 
       OPL_OUTER 	*outerp; 
       OPV_GBMVARS      fullmap; 

       if ( and_node->pst_left->pst_sym.pst_type == PST_BOP )
       {

	  if (((l->pst_sym.pst_type == PST_BOP || l->pst_sym.pst_type == PST_MOP) &&
	       l->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_fiflags
			& ADI_F32768_IFNULL &&
               l->pst_sym.pst_value.pst_s_op.pst_joinid <= OPL_NOOUTER)
              ||
              ((r->pst_sym.pst_type == PST_BOP || r->pst_sym.pst_type == PST_MOP) &&
               r->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_fiflags
                        & ADI_F32768_IFNULL &&
               r->pst_sym.pst_value.pst_s_op.pst_joinid <= OPL_NOOUTER)
             ) 
	  {
              MEcopy(lvarmap, sizeof(OPV_GBMVARS), (PTR)&fullmap); 
              BTor((i4)BITS_IN(rvarmap), (char*)rvarmap, (PTR)&fullmap); 

              for (i = 0; i < subquery->ops_oj.opl_lv; i++)
              {
                  outerp = subquery->ops_oj.opl_base->opl_ojt[i];

                  if ((outerp->opl_type == OPL_LEFTJOIN ||
                       outerp->opl_type == OPL_RIGHTJOIN) &&
                      BTsubset((char *)&fullmap, 
                               (char *)outerp->opl_bvmap, 
                               (i4)BITS_IN(outerp->opl_bvmap)))
                  {
                      /* found an ifnull() in the where clause, and both
                      ** var in left and right is referenced in an oj
                      */
                      found = TRUE; 
                      break;
                  }
              }
            }
            if (subquery->ops_oj.opl_lv && found) return (FALSE);
          }
              

    }

    /* if oldnodep is not null then, null join was being
    ** attempted but data types do not match so
    ** place into function attribute creation list, but save the old
    ** pointer to the OR test in case the boolean factor needs to be
    ** recreated */
    opz_savequal(subquery, and_node, lvarmap, rvarmap, oldnodep, 
		inodep, left_iftrue, right_iftrue);	/* save the
                                        ** qualification since it may be
                                        ** useful for creating function
                                        ** attributes
                                        ** - do not create function attributes
                                        ** immediately since it may prevent
                                        ** a more useful "normal" join from
                                        ** being used.
                                        */
    *funcattr = TRUE;
    return( TRUE );                     /* detach the query after save info
                                        ** on possibly useful the function
                                        ** attribute
                                        ** - the query may be reattached it
                                        ** the function attribute is not
                                        ** useful
                                        */
}

/*{
** Name: ope_noncnf_nj	- check for null join in non-CNFed qual chunk.
**
** Description:
**      Functions ope_findjoins and ope_njoins contain involved logic 
**	to locate and collapse "null join" expressions for old-style
**	Ingres CNF where clauses. Null joins are like:
**		(r.a = s.b) OR (r.a is null and s.b is null)
**	In a non-CNFed where clause, these are somewhat easier to identify
**	because they are all contained under one OR subtree. This function 
**	contains the logic to determine the condition, then to remove the 
**	"is null" tests from the original query.
[@comment_line@]...
**
** Inputs:
**      subquery                        current subquery being
**					analyzed
**	nodep				ptr to anchor of structure to analyze
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
**	11-apr-97 (inkdo01)
**	    Written.
[@history_template@]...
*/
static bool
ope_noncnf_nj(
	OPS_SUBQUERY       *subquery,
	PST_QNODE	   *nodep)
{
    PST_QNODE	*andptr;
    PST_QNODE	*bopptr;
    PST_QNODE	*bv1ptr;
    PST_QNODE	*bv2ptr;
    PST_QNODE	*uv1ptr;
    PST_QNODE	*uv2ptr;

    OPZ_AT	*abase;		/* ptr to base array of joinop attr descrs */
    OPZ_IATTS	b1attr, b2attr, u1attr, u2attr;
				/* joinop attnos of "=", "is null" operands */
    OPE_IEQCLS	b1eqc, b2eqc, u1eqc, u2eqc;
				/* eqclasses of "=", "is null" operands */
    OPV_IVARS	b1varno, b2varno;


    /* First big if just checks for a = b or c is null and d is null AND
    ** a and b are in different base tables (i.e. a = b is a join pred) AND
    ** there are no outer joins, or the "=" and the "is null"s have the
    ** same joinid's. */

    if (!(nodep->pst_left->pst_sym.pst_type == PST_OR 
	&& ((andptr = nodep->pst_left->pst_left)->pst_sym.pst_type == PST_AND
	|| (andptr = nodep->pst_left->pst_right)->pst_sym.pst_type == PST_AND)
	&& ((bopptr = nodep->pst_left->pst_right)->pst_sym.pst_type == PST_BOP
	|| (bopptr = nodep->pst_left->pst_left)->pst_sym.pst_type == PST_BOP)
	&& bopptr->pst_sym.pst_value.pst_s_op.pst_opno ==
			subquery->ops_global->ops_cb->ops_server->opg_eq
	&& (bv1ptr = bopptr->pst_left)->pst_sym.pst_type == PST_VAR
	&& (bv2ptr = bopptr->pst_right)->pst_sym.pst_type == PST_VAR
	&& (b1varno = bv1ptr->pst_sym.pst_value.pst_s_var.pst_vno) !=
			(b2varno = bv2ptr->pst_sym.pst_value.pst_s_var.pst_vno)
	&& andptr->pst_left->pst_sym.pst_type == PST_UOP 
	&& andptr->pst_left->pst_sym.pst_value.pst_s_op.pst_opno ==
			subquery->ops_global->ops_cb->ops_server->opg_isnull
	&& (uv1ptr = andptr->pst_left->pst_left)->pst_sym.pst_type == PST_VAR
	&& andptr->pst_right->pst_sym.pst_type == PST_UOP
	&& andptr->pst_right->pst_sym.pst_value.pst_s_op.pst_opno ==
			subquery->ops_global->ops_cb->ops_server->opg_isnull
	&& (uv2ptr = andptr->pst_right->pst_left)->pst_sym.pst_type == PST_VAR
	&& (subquery->ops_oj.opl_base == NULL 
	|| bopptr->pst_sym.pst_value.pst_s_op.pst_joinid ==
		andptr->pst_left->pst_sym.pst_value.pst_s_op.pst_joinid
	&& bopptr->pst_sym.pst_value.pst_s_op.pst_joinid ==
		andptr->pst_right->pst_sym.pst_value.pst_s_op.pst_joinid)))
		return(FALSE);		/* if any of the tests fail, this
					** ain't our nulljoin */

    /* The next step is to match up the VARs or their corresponding 
    ** eqclasses (r.a = s.b and s.b = t.c or r.a is null and t.c is null
    ** still qualifies). */

    abase = subquery->ops_attrs.opz_base;
    b1attr = bv1ptr->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id;
    b1eqc = abase->opz_attnums[b1attr]->opz_equcls;
    b2attr = bv2ptr->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id;
    b2eqc = abase->opz_attnums[b2attr]->opz_equcls;
    u1attr = uv1ptr->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id;
    u1eqc = abase->opz_attnums[u1attr]->opz_equcls;
    u2attr = uv2ptr->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id;
    u2eqc = abase->opz_attnums[u2attr]->opz_equcls;

    if (!(b1attr == u1attr && b2attr == u2attr 
	|| b1attr == u2attr && b2attr == u1attr
	|| b1eqc != OPE_NOEQCLS &&
	    (b1eqc == u1eqc && b2attr == u2attr
	  || b1eqc == u2eqc && b2attr == u1attr)
	|| b2eqc != OPE_NOEQCLS &&
	    (b2eqc == u1eqc && b1attr == u2attr
	  || b2eqc == u2eqc && b1attr == u1attr)
	|| b1eqc != OPE_NOEQCLS && b2eqc != OPE_NOEQCLS &&
	    (b1eqc == u1eqc && b2eqc == u2eqc
	  || b1eqc == u2eqc && b2eqc == u1eqc)))
		return(FALSE);		/* if any of these tests fail,
					** the columns don't match and 
					** we have no null join */

    /* Once we're here, we have a syntactically correct null join. 
    ** Because of the usual Ingres complexities, it isn't easy to finish
    ** null join processing here. So we simulate a CNF-style null join,
    ** add it to the null join list (back in the caller) and let the
    ** old code finish the job. (I'll admit it - I'm a wimp!) */
    {
	PST_QNODE	*orptr;

	/* Make another OR node, then simulate :
	**  ... (a = b or a is null) and (a = b or b is null) ...
	** to make it like CNF-style where clause. */

	orptr = opv_opnode(subquery->ops_global, PST_OR, (ADI_OP_ID)0, 
		(OPL_IOUTER)andptr->pst_sym.pst_value.pst_s_op.pst_joinid);
	orptr->pst_left = andptr->pst_right;
					/* 2nd OR gets 2nd "is null" */
	orptr->pst_right = bopptr;	/* MUST have "is null" on left,
					** "=" on right */

	nodep->pst_left->pst_left = andptr->pst_left;
					/* 1st OR gets 1st "is null" */
	nodep->pst_left->pst_right = bopptr;

	andptr->pst_right = nodep->pst_right;
	andptr->pst_left = orptr;
	nodep->pst_right = andptr;	/* elevate modified AND */
    }

    return(TRUE);			/* Ta, da! */

}	/* end of ope_noncnf_nj */

/*{
** Name: ope_njoins	- find NULL joins
**
** Description:
**      These type of joins can be generated by distributed cases 
**      in link back situations, and since we have an operator
**      for it we can use it. 
**	The case to consider is
**                  (r.a = s.a) OR(r.a is NULL and s.a is NULL)
**      which implies a regular join where NULLs are equal
**      this is encoded in the equivalence class as ope_nulljoin
**      being set to TRUE.
[@comment_line@]...
**
** Inputs:
**      subquery                        current subquery being
**					analyzed
**	nodep				ptr to list of "and" nodes
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
**      22-dec-88 (seputis)
**          initial creation
**	4-aug-93 (ed)
**	    fix bug 53670 - infinite loop needs an pointer update to exit
**      13-Mar-2000 (hweho01)
**          Avoid memory overlay on 64-bit platforms (such as axp_osf ), 
**          only set the pst_flags to PST_OJJOIN if the node type is  
**          PST_OP_NODE.
[@history_template@]...
*/
static VOID
ope_njoins(
	OPS_SUBQUERY       *subquery,
	PST_QNODE	   *nodep)
{
    PST_QNODE	    **nodepp;	/* ptr to current boolean factor
				** being analyzed */
    OPZ_AT          *abase;     /* base of array of ptrs to joinop attribute
                                ** descriptors */
    abase = subquery->ops_attrs.opz_base;
    for (nodepp = &nodep; *nodepp; )
    {
	PST_QNODE	*cnodep;	/* ptr to current node */
	OPZ_IATTS	lattr;		/* attribute number from
					** var node on left side of "=" */
	OPE_IEQCLS	leqcls;		/* eqcls number from
					** var node on left side of "=" */
	OPZ_IATTS	rattr;		/* attribute number from
					** var node on right side of "=" */
	OPE_IEQCLS	reqcls;		/* eqcls number from
					** var node on right side of "=" */
	OPZ_IATTS	nattr;		/* attribute number from
					** var node on left side of NULL */
	OPE_IEQCLS	neqcls;		/* eqcls number from
					** var node on left side of NULL */
	PST_QNODE	*lvarp;		/* ptr to var node on left side
					** of "=" */
	PST_QNODE	*rvarp;		/* ptr to var node on right side
					** of "=" */
	PST_QNODE	*nvarp;		/* ptr to var node on left side
					** of "=" */
	PST_QNODE	**inodepp;
	bool		found;		/* TRUE if a NULL join was found */
	OPV_IVARS	lvarno;		/* left variable number */
	OPV_IVARS	rvarno;		/* right variable number */
	OPL_IOUTER	ojid;

	found = FALSE;
	cnodep = *nodepp;
	lvarp = cnodep->pst_left->pst_right->pst_left;
	rvarp = cnodep->pst_left->pst_right->pst_right;
	lvarno = lvarp->pst_sym.pst_value.pst_s_var.pst_vno;
	rvarno = rvarp->pst_sym.pst_value.pst_s_var.pst_vno;
	if (lvarno != rvarno)
	{
	    ojid = cnodep->pst_left->pst_left->pst_sym.pst_value.pst_s_op.pst_joinid;
	    nvarp = cnodep->pst_left->pst_left->pst_left;
	    lattr = lvarp->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id;
	    leqcls = abase->opz_attnums[lattr]->opz_equcls;
	    rattr = rvarp->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id;
	    reqcls = abase->opz_attnums[rattr]->opz_equcls;
	    nattr = nvarp->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id;
	    neqcls = abase->opz_attnums[nattr]->opz_equcls;
	    for (inodepp = &cnodep->pst_right; *inodepp; inodepp = &(*inodepp)->pst_right)
	    {   /* look at remaining nodes for a match */
		OPZ_IATTS	ilattr;		/* attribute number from
					    ** var node on left side of "=" */
		OPE_IEQCLS	ileqcls;	/* eqcls number from
					    ** var node on left side of "=" */
		OPZ_IATTS	irattr;		/* attribute number from
					    ** var node on right side of "=" */
		OPE_IEQCLS	ireqcls;	/* eqcls number from
					    ** var node on right side of "=" */
		OPZ_IATTS	inattr;		/* attribute number from
					    ** var node on left side of NULL */
		OPE_IEQCLS	ineqcls;	/* eqcls number from
					    ** var node on left side of NULL */
		PST_QNODE	*inodep;

		inodep = *inodepp;
		ilattr = inodep->pst_left->pst_right->pst_left->pst_sym.
		    pst_value.pst_s_var.pst_atno.db_att_id;
		irattr = inodep->pst_left->pst_right->pst_right->pst_sym.
		    pst_value.pst_s_var.pst_atno.db_att_id;
		ileqcls = abase->opz_attnums[ilattr]->opz_equcls;
		ireqcls = abase->opz_attnums[irattr]->opz_equcls;
		inattr = inodep->pst_left->pst_left->pst_left->pst_sym.
		    pst_value.pst_s_var.pst_atno.db_att_id;
		ineqcls = abase->opz_attnums[inattr]->opz_equcls;
		if (subquery->ops_oj.opl_base
		    &&
		    (ojid != inodep->pst_left->pst_right->pst_sym.pst_value.pst_s_op.pst_joinid))
		    ;			/* outer join ids should match */
		else if (
		    (   (	(   (lattr == ilattr)
				&&
				(rattr == irattr)
			    )
			    ||
			    (   (lattr == irattr)
				&&
				(rattr == ilattr)
			    )
			)
			&&
			(	(   (lattr == nattr)
				&&
				(rattr == inattr)
			    )
			    ||
			    (   (lattr == inattr)
				&&
				(rattr == nattr)
			    )
			)
		    )
		    ||
		    (   (leqcls != OPE_NOEQCLS)
			&&
			(	(   (leqcls == ileqcls)
				&&
				(rattr == irattr)
			     )
			     ||
			     (  (leqcls == ireqcls)
				&&
				(rattr == ilattr)
			     )
			)
			&&
			(	(   (leqcls == neqcls)
				&&
				(rattr == inattr)
			     )
			     ||
			     (  (leqcls == ineqcls)
				&&
				(rattr == nattr)
			     )
			)
		    )
		    ||
		    (   (reqcls != OPE_NOEQCLS)
			&&
			(	(   (reqcls == ireqcls)
				&&
				(lattr == ilattr)
			     )
			     ||
			     (  (reqcls == ileqcls)
				&&
				(lattr == irattr)
			     )
			)
			&&
			(	(   (reqcls == neqcls)
				&&
				(lattr == inattr)
			     )
			     ||
			     (  (reqcls == ineqcls)
				&&
				(lattr == nattr)
			     )
			)
		    )
		    ||
		    (   (reqcls != OPE_NOEQCLS)
			&&
			(leqcls != OPE_NOEQCLS)
			&&
			(	(   (reqcls == ireqcls)
				&&
				(leqcls == ileqcls)
			     )
			     ||
			     (  (reqcls == ileqcls)
				&&
				(leqcls == ireqcls)
			     )
			)
			&&
			(	(   (reqcls == neqcls)
				&&
				(leqcls == ineqcls)
			     )
			     ||
			     (  (reqcls == ineqcls)
				&&
				(leqcls == neqcls)
			     )
			)
		    )
		    )
		{   /* at this point the "=" operators matched and there
		    ** is an IS NULL check so that the boolean factors can
		    ** be eliminated and eqcls merged */
		    OPV_GBMVARS	lvarmap;
		    OPV_GBMVARS	rvarmap;
		    PST_QNODE	*oldnodep;
		    bool	funcattr;

		    MEfill(sizeof(lvarmap), 0, (char *)&lvarmap);
		    MEfill(sizeof(rvarmap), 0, (char *)&rvarmap);
		    oldnodep = cnodep->pst_left;
		    cnodep->pst_left = cnodep->pst_left->pst_right; /* make it
					    ** look like a regular join */
		    BTset((i4)lvarno, (char *)&lvarmap);
		    BTset((i4)rvarno, (char *)&rvarmap);
		    if (ope_jnclaus(subquery, cnodep, &lvarmap, &rvarmap, 
						oldnodep, inodep, &funcattr))
		    {
			if (subquery->ops_oj.opl_base
			    &&
			    (inodep->pst_left->pst_right->pst_sym.pst_value.pst_s_op.pst_joinid
			    != OPL_NOOUTER)
			    &&
			    !funcattr)
			{   /* keep boolean factors to help with outer join placement
			    ** but mark those boolean factors to avoid histogramming */
			    (*inodepp)->pst_left->pst_right->pst_sym.pst_value.
				pst_s_op.pst_flags |= PST_OJJOIN;
                         if( (*nodepp)->pst_left->pst_right->pst_sym.pst_len
                             == sizeof( PST_OP_NODE ))        
			    (*nodepp)->pst_left->pst_right->pst_sym.pst_value.
				pst_s_op.pst_flags |= PST_OJJOIN;
			    cnodep->pst_left = oldnodep; /* restore boolean factor */
			}
			else
			{
			    *inodepp = (*inodepp)->pst_right;
			    *nodepp = (*nodepp)->pst_right;
			    found = TRUE;
			}
			break;
		    }
		    else
			cnodep->pst_left = oldnodep; /* restore boolean factor */
		}
	    }
	}
	if (!found)
	    nodepp = &(*nodepp)->pst_right;
    }
    /* the remainder of the boolean factors cannot be used for the NULL
    ** join so place back into boolean factor list */
    if (nodep)
    {
	*nodepp = subquery->ops_root->pst_right; /* end of list is pointed at
					** by nodepp */
	subquery->ops_root->pst_right = nodep;
    }
}

/*{
** Name: ope_findjoins	- find joining clauses
**
** Description:
**      Traverse the qualification of the subquery and detach the joining
**      clauses.  Determine equivalence classes for all variables in the
**      joining clauses.
**
**	There should not be 2 atts with same eqclass in a relation
**	so for each eqclass, if two atts in it are from the same
**	rel then 
**          1) create equality clause between the two, and
**	    2) create new eqclass for one of the atts.
**
**	this mod has not been done and is put here as a
**	reminder
**
**	On the other hand, maybe we do want 2 atts from a relation to be
**	able to be in the same eqclass,  Have to handle this condition in
**	eqattmap and in jxpreds and places like that.
**
** Inputs:
**	subquery			ptr to subquery being processed
**          .ops_root->pst_right        beginning of qualification
**
** Outputs:
**      subquery
**          .ope_eclass                 updated with equivalence classes
**                                      which replace the joining clauses
**                                      in the qualification
**          .ops_root->pst_right        joining clauses removed and deleted
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
**          initial creation jndoms
**	18-apr-90 (seputis)
**	    fixed b21117 - IS NULL bug
**	3-may-94 (ed)
**	    fix lint error, remove obsolete variable
**	11-apr-97 (inkdo01)
**	    Add support for null joins in non-CNF'd qualification.
**      14-jan-2005 (huazh01)
**          Do not detach qualification if variable referenced under
**          ifnull() is also being referenced under any other OJ. 
**          b113160, INGSRV2984. 
**	17-feb-06 (hayke02)
**	    Limit the fix for bug 113160 (findOJ TRUE) to an OPL_NOOUTER
**	    pst_joinid, as we are only interested in an ifnull() in the
**	    WHERE clause. This change fixes bug 115763.
**      06-jun-2006 (huazh01)
**          remove the fix for 113160 and let the fix to b115165 and 
**          114912 handles such problem. This change fixes 116098. 
**	04-Nov-2007 (kiria01) b119410
**	    Disable the operator ADFI_1347_NUMERIC_EQ from participating
**	    in the usual join optimisation.
**	27-Oct-2009 (kiria01) SIR 121883
**	    Scalar sub-selects - Allow for PST_CARD_01R to be present
**	    to be picked up later.
**	13-Nov-2009 (kiria01) SIR 121883
**	    Distinguish between LHS and RHS card checks for 01.
**	14-Jul-2010 (kschendel) SIR 123104
**	    Use symbolic value for 1347 FI.
*/
VOID
ope_findjoins(
	OPS_SUBQUERY          *subquery)
{
    register PST_QNODE  *and_node;  /* used to traverse qualification list */
    register PST_QNODE  *previous;  /* previous link to current AND node so that
                                    ** the qualification can be removed
                                    */
    PST_QNODE	    *njoinp;	    /* ptr to list of qualifications which could
				    ** from a NULL join */
    PST_QNODE	    **njoinpp;      /* ptr to next insertion point for NULL joins
				    */

    if ( subquery->ops_vars.opv_prv <= 1 )
	return;			    /* return if no possibility of a join */
    njoinp = (PST_QNODE *)NULL;
    njoinpp = &njoinp;
    previous = subquery->ops_root;
 
    for (and_node = previous->pst_right;
	and_node && and_node->pst_sym.pst_type != PST_QLEND;
	and_node = and_node->pst_right)
    {	/* traverse the AND nodes of the query until the end of the 
	** qualification is reached
	*/
	PST_QNODE	    *conditional; /* conditional associated with
                                        ** AND node ... AND node must have
                                        ** a conditional.
                                        */
	OPV_GBMVARS	    lvarmap;	/* bit map of vars used in the left
					** part of the "=" conditional
                                        */
        OPV_GBMVARS         rvarmap;    /* bit map of vars used in the right
                                        ** part of the "=" conditional
                                        */
	OPV_GBMVARS	    tempmap;
	OPV_IVARS           select;     /* get the range variable associated
					** with all the selects */
	bool		    funcattr;
	MEfill(sizeof(lvarmap), 0, (char *)&lvarmap);
	MEfill(sizeof(rvarmap), 0, (char *)&rvarmap);
	conditional = and_node->pst_left;
	select = subquery->ops_joinop.opj_select; /* get the select variable 
					** which contains the subselect 
					** attributes */

	if ((conditional->pst_sym.pst_type != PST_BOP) 
	    ||
		(conditional->pst_sym.pst_value.pst_s_op.pst_opno 
		!= 
		subquery->ops_global->ops_cb->ops_server->opg_eq)
	    ||
	    (conditional->pst_sym.pst_value.pst_s_op.pst_opmeta != PST_NOMETA &&
	     conditional->pst_sym.pst_value.pst_s_op.pst_opmeta != PST_CARD_01R &&
	     conditional->pst_sym.pst_value.pst_s_op.pst_opmeta != PST_CARD_01L)
				/* exit since the qualification cannot be an
                                ** equi-join of the form "var1 == var2" since
                                ** the operator is not a "=", or if it is then
                                ** it is a subselect which requires special
                                ** handling
                                */
	    || conditional->pst_sym.pst_value.pst_s_op.pst_opinst == ADFI_1347_NUMERIC_EQ
				/* FIXME
				** The present support for ADFI_1347_NUMERIC_EQ
				** is limited to that of a simple predicate
				** pending histogram support.
				*/
	    )
	{
	    ADI_OP_ID	leftopno; /* operator ID of node */
	    ADI_OP_ID   rightopno; /* operator ID of node */
	    bool	bool_switch; /* TRUE if disjuncts
				** should be switch to make later
				** processing more straightforward */
	    if (subquery->ops_mask & OPS_NONCNF &&
			ope_noncnf_nj(subquery, and_node)) 
				/* perform more detailed nonCNF nulljoin
				** checks in separate function */
	    {
		*njoinpp = and_node;	/* add to null join list */
		njoinpp = &and_node->pst_right->pst_right;
		previous->pst_right = and_node->pst_right->pst_right;
		and_node = and_node->pst_right;
				/* detach and proceed */
		continue;
	    }
	    if ((conditional->pst_sym.pst_type == PST_OR)
		&&
		(   (   (conditional->pst_left->pst_sym.pst_type == PST_BOP)
			&&
			(conditional->pst_right->pst_sym.pst_type == PST_UOP)
		    )
		    ||
		    (   (conditional->pst_left->pst_sym.pst_type == PST_UOP)
			&&
			(conditional->pst_right->pst_sym.pst_type == PST_BOP)
		    )
		)
		&&
		(leftopno = conditional->pst_left->
			    pst_sym.pst_value.pst_s_op.pst_opno)
		&&
		(rightopno = conditional->pst_right->
			    pst_sym.pst_value.pst_s_op.pst_opno)
		&&
		(   (leftopno 
		    ==
		    subquery->ops_global->ops_cb->ops_server->opg_isnull
		    )
		    ||
		    (rightopno 
		    ==
		    subquery->ops_global->ops_cb->ops_server->opg_isnull
		    )		/* at this point one operator is an
				** "IS NULL" operator */
		)
		&&
		(   (bool_switch = 
			(leftopno 
			== 
			subquery->ops_global->ops_cb->ops_server->opg_eq
		    )	)
		    ||
		    (rightopno 
		    == 
		    subquery->ops_global->ops_cb->ops_server->opg_eq
		    )		/* at this point there is at least one
				** "=" operator and one "IS NULL" operator
				*/
		))
	    {	/* a possible NULL join exists at this point so extract
		** from boolean factor list for later processing */
		if (bool_switch)
		{   /* make sure IS NULL operator is to the left */
		    PST_QNODE	    *temp;
		    temp = conditional->pst_right;
		    conditional->pst_right = conditional->pst_left;
		    conditional->pst_left = temp;
		}
		if ((conditional->pst_right->pst_left->pst_sym.pst_type == PST_VAR)
		    &&
		    (conditional->pst_right->pst_right->pst_sym.pst_type == PST_VAR)
		    &&
		    (conditional->pst_left->pst_left->pst_sym.pst_type == PST_VAR)
		    )
		{
		    previous->pst_right = and_node->pst_right; /* detach from the
                                        ** qualification since it may be
                                        ** represented by an
                                        ** equivalence class
                                        */
		    *njoinpp = and_node;	/* place in NULL join list */
		    njoinpp = &and_node->pst_right;
		    continue;
		}
					/* look for VARs now, FIXME
					** can look for function attribute
					** joins also */

	    }

	    previous = and_node;	/* get next qualification */
	    continue;			/* skip this conjunct since it is 
					** not useful 
					*/
	}
	opv_mapvar(conditional->pst_left, &lvarmap); /* check for vars on the
                                        ** left side */
	if (BTcount((char *)&lvarmap, OPV_MAXVAR) == 0)
	{
	    previous = and_node;	/* get next qualification */
	    continue;			/* skip this conjunct since it is 
					** not useful 
					*/
	}
	opv_mapvar(conditional->pst_right, &rvarmap); /* check for vars on the
                                        ** right side */
	if (BTcount((char *)&rvarmap, OPV_MAXVAR) == 0)
	{
	    previous = and_node;	/* get next qualification */
	    continue;			/* skip this conjunct since it is 
					** not useful 
					*/
	}
	MEcopy((char *)&lvarmap, sizeof(lvarmap), (char *)&tempmap);
	BTand(OPV_MAXVAR, (char *)&rvarmap, (char *)&tempmap);
	if (BTcount((char *)&tempmap, OPV_MAXVAR) != 0)
	{   /* if there is any intersection of vars on the left and right
            ** sides then a function attribute would not be useful since
            ** both sides can only have all equivalence classes available
            ** at the same operator tree node; i.e. a function attribute
            ** is only benificial if it can be evaluated earlier in the
            ** operator tree
            */
	    previous = and_node;	/* get next qualification */
	    continue;			/* skip this conjunct since it is 
					** not useful 
					*/
	}

	if (select != OPV_NOVAR
	    &&
	    (
		BTtest((i4)select, (char *)&lvarmap)
		||
		BTtest((i4)select, (char *)&rvarmap)
	    )
	    )
	{   /* a select node was found in the conjunct so it cannot be used
            ** for join */
	    previous = and_node;	/* get next qualification */
	    continue;			/* skip this conjunct since it is 
					** not useful 
					*/
	}

    	/* Find all the boolean factor clauses that describe joins.
    	** ope_jnclaus will build whatever function attributes are needed
    	** to accomplish the join. If the jnclaus is successful and
    	** the equivalence classes can be merged (all atts in the
    	** eqc must come from different vars), remove the joining
    	** boolfact from the qualification.
    	*/

        if (ope_jnclaus(subquery, and_node, &lvarmap, &rvarmap, (PST_QNODE *)NULL, 
		(PST_QNODE *)NULL, &funcattr))

	/* At this point a join will be useful so create detach the
        ** the qualification from the main query.  Qualifications which
        ** could produce function attributes will be separated and analyzed
        ** in a separate pass.  They may be reconnected if it is determined
        ** that function attributes would not be useful.  This first pass
        ** will extract all simple joining clauses.
        */
	{

	    if ((subquery->ops_oj.opl_base
		&&
		(conditional->pst_sym.pst_value.pst_s_op.pst_joinid != OPL_NOOUTER)
		&&
		!funcattr))
	    {
		conditional->pst_sym.pst_value.pst_s_op.pst_flags |= PST_OJJOIN;
					/* eventually make this a boolean factor
					** to help with placement and filtering
					** of nested outer joins */
		previous = and_node;	/* do not detach, even though an
					** equivalence class is created */
	    }
	    else		
		previous->pst_right = and_node->pst_right; /* detach from the
                                        ** qualification since now it is
                                        ** represented by an
                                        ** equivalence classes
                                        */
	}
	else
	    previous = and_node;	/* skip the node since it was not
                                        ** useful in a join
                                        */
    }

    /* An index may be explicitly referenced in the query so check for this
    ** case and introduce new joinop attributes to join the index to the
    ** primary.  Do this before boolean factors are analyzed so that merging
    ** the equivalence classes is still possible in cases in which both
    ** the index attribute and corresponding primary attribute are both
    ** referenced */
    {
	OPV_IVARS   explicit;	    /* current primary being analyzed */
	OPV_RT	    *vbase;	    /* base of array of ptrs to range var
				    ** descriptors */

	vbase = subquery->ops_vars.opv_base;
	for ( explicit = subquery->ops_vars.opv_prv; /* base relations are 0 to 
				    ** opv_prv-1 */
	      (explicit--) > 0;)    
	{
	    OPV_GRV	*gvar_ptr; /* ptr to global range table element
					** being analyzed
					*/
	    gvar_ptr = vbase->opv_rt[explicit]->opv_grv;   /* get ptr to 
					** corresponding global range table 
					** element */
	    if( gvar_ptr->opv_relation  /* if the ptr to the RDF element is NULL
					** then this is a temporary table which
					** is the result of aggregate processing and
					** would not have any indexes defined
					*/
		&&
	        (gvar_ptr->opv_relation->rdr_rel->tbl_status_mask & DMT_IDX )
	       )
	    {   /* found explicitly referenced index, so check if a tid
		** join has been made to a base relation, in which case
		** make sure all of the indexed attributes
		** are in the proper equivalence classes */
		ope_explicit(subquery, explicit);
	    }
	}
    }
    if (njoinp)
    {	/* there is at least one possible NULL join so call NULL join
	** processing routine */
	*njoinpp = (PST_QNODE *)NULL;
	ope_njoins(subquery, njoinp);    
    }
    /* Function attributes are analyzed after all the "normal joins" have been
    ** found.  This will prevent a multi-variable function attribute from
    ** overriding a "normal join".  This can happen in the case of 
    **     where (r.a = func(s.b)) AND (r.a = s.d) 
    ** since an equivalence class cannot have two attributes from the same
    ** range variable.  Given a choice, the normal join is less expensive
    ** since multi-variable joins could cause cartesean products
    */
    opz_fattr(subquery);		/* analyze qualifications which
                                        ** could be used for joins if
                                        ** function attributes were created
                                        */
#ifdef OPM_FLATAGGON
    if (subquery->ops_flat.opm_flagg)
    {	/* if an aggregate subselect has been flattened initialize the
	** equivalence class maps of function attributes since now the
	** equi-joins have been processed and the equivalence classes
	** have been defined */
	OPZ_IFATTS  fattr; /* function attribute */
	OPZ_FT	    *fbase;

	fbase = subquery->ops_funcs.opz_fbase;
	for (fattr = subquery->ops_funcs.opz_fv; --fattr>=0;)
	{
	    OPZ_FATTS	    *fattrp;	/* ptr to current
				** function attribute descriptor
				** being analyzed */
	    fattrp = fbase->opz_fatts[fattr];

	    if (fbase->opz_fatts[fattr]->opz_mask & OPZ_NOEQCLS)
	    {   /* initialize equivalence class map */
		ope_aseqcls( subquery, (OPE_BMEQCLS *)&fattrp->opz_eqcm,
		    fattrp->opz_function); /* assign equivalence classes to
				** function attributes in expression since
				** these have not been traversed yet,... note
				** at this point only those equivalence classes
				** for which histograms would be useful should be
				** defined, so this should really be done later
				** after the oph_histos high water mark is set */
		/* assign equivalence class to function attribute if this has not
		** already been done */
		if (subquery->ops_attrs.opz_base->opz_attnums[fattrp->opz_attno]
			->opz_equcls < 0)
		    ope_neweqcls(subquery, fattrp->opz_attno);
	    }
	}
    }
#endif
}

