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
#include    <adfops.h>

/* external routine declarations definitions */
#define        OPZFATTR      TRUE
#include    <me.h>
#include    <bt.h>
#include    <opxlint.h>

/**
**
**  Name: OPZFATTR.C - this file contains function attribute routines
**
**  Description:
**      This file contains routines which process function attributes
**
**  History:
**      25-apr-86 (seputis)    
**          initial creation
**	17-apr-89 (jrb)
**	    Properly initialized prec field(s).
**      16-aug-91 (seputis)
**          add CL include files for PC group
**      21-may-93 (ed)
**          - fix outer join problem, when function attributes use the
**          result of an outer join
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	29-nov-95 (wilri01)
**	    Don't allow function attr when subquery has a union view.
**	20-may-2001 (somsa01)
**	    Changed 'nat' to 'i4' from cross-integration.
[@history_line@]...
**/

/*{
** Name: opz_rdatatype	- calculate result type and length of a conversion
**
** Description:
**      This routine will calculate the result type and length of a conversion
**
** Inputs:
**      subquery                        ptr to subquery being processed
**      convid                          ADF function instance conversion ID
**      indatatype                      datatype to convert from
**
** Outputs:
**      outdatatype                     datatype of result after conversion
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	22-apr-86 (seputis)
**          initial creation
**	22-Feb-1999 (shero03)
**	    Support 4 operands in adi_0calclen
**	04-Nov-2007 (kiria01) b119410
**	    Also allow ANY to be handled.
[@history_line@]...
*/
static VOID
opz_rdatatype (
	OPS_SUBQUERY       *subquery,
	DB_DATA_VALUE      *indatatype,
	DB_DT_ID	   outdtid,
	DB_DATA_VALUE      *resultdatatype)
{
    ADI_FI_DESC        *adi_fdesc;  /* ptr to function instance
				    ** description of conversion
				    */
    DB_STATUS	       status;	    /* status of adt operation */
    ADF_CB             *adf_scb;    /* ADF session control block */
    ADI_FI_ID	       convid;	    /* conversion ID for coercion */

    if (abs(outdtid) == abs(indatatype->db_datatype) ||
	abs(outdtid) == DB_ALL_TYPE)
    {   /* ADF will not affect the result length if calclen is called
        ** so a special case check is needed here, ignoring nullability
        ** since ADF considers types equal for ficoerce even if nullability
        ** is different */
	resultdatatype->db_length = indatatype->db_length;
	resultdatatype->db_prec = indatatype->db_prec;
	resultdatatype->db_datatype = indatatype->db_datatype; /* do not
                                    ** change the nullability of the
                                    ** target datatype if types are equal
                                    ** since this would affect the length
                                    ** and would cause unnecessary work
                                    ** in creating the nullable type */
	return;
    }
    else
	resultdatatype->db_datatype = outdtid;
    adf_scb = subquery->ops_global->ops_adfcb;
    status = adi_ficoerce(adf_scb, indatatype->db_datatype, outdtid, &convid);
# ifdef E_OP0783_ADI_FICOERCE
    if ( status != E_DB_OK )
	opx_verror(status, E_OP0783_ADI_FICOERCE, 
            adf_scb->adf_errcb.ad_errcode); /* exit if unexpected error */
# endif
    status = adi_fidesc(adf_scb, convid, &adi_fdesc);
# ifdef E_OP0781_ADI_FIDESC
    if ( status != E_DB_OK )
	opx_verror(status, E_OP0781_ADI_FIDESC, adf_scb->adf_errcb.ad_errcode); 
				    /* exit if unexpected error */
# endif
    /* FIXME: Ed: the second input datatype must be NULL if you only have
    ** one input datatype. This is because of some changes that Gene made
    ** for NULL support. This was your original call:
    ** status = adi_calclen(adf_scb, &adi_fdesc->adi_lenspec, 
    **	    indatatype, indatatype, &resultdatatype->db_length);
    */
    status = adi_0calclen(adf_scb, &adi_fdesc->adi_lenspec, 1,
	&indatatype, resultdatatype);
# ifdef E_OP0782_ADI_CALCLEN
    if (status != E_DB_OK)
	opx_verror(status, E_OP0782_ADI_CALCLEN, 
	    adf_scb->adf_errcb.ad_errcode); /* exit if unexpected error */
# endif
}    

/*{
** Name: opz_findop	- locate a particular opno in a parse tree fragment
**
** Description:
**      The routine descends a parse tree fragment looking for operator nodes
**	whose operator matches the supplied code. This was written to look
**	for ifnull functions in ON clauses, but can be used to check for
**	any ol' operator.
**
** Inputs:
**      function                        ptr to query tree representing the
**                                      function
**      opno				the operator being checked for
**
** Outputs:
**	Returns:
**	    TRUE - if the operator was found in the expression tree
**	    FALSE - if the operator wasn't found
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	23-mar-05 (inkdo01)
**	    Written.
[@history_line@]...
*/
static bool 
opz_findop(
	PST_QNODE	    *function,
	ADI_OP_ID	    opno)

{
    PST_QNODE		*nodep = function;


    /* Simply search the subtree for operator nodes, then compare 
    ** to opno. Recurse down the right side and iterate down the left. */

    while (nodep) {
    	switch (nodep->pst_sym.pst_type) {

	  case PST_AND:
	  case PST_OR:
	  case PST_UOP:
	  case PST_BOP:
	  case PST_AOP:
	  case PST_COP:
	  case PST_MOP:
	    if (nodep->pst_sym.pst_value.pst_s_op.pst_opno == opno)
		return(TRUE);

	  /* Drop through to recurse on pst_right & iterate on pst_left. */
	  default:	/* all the rest */
	    if (nodep->pst_right && opz_findop(nodep->pst_right, opno))
		return(TRUE);
	    nodep = nodep->pst_left;
	    break;
	}
    }

    return(FALSE);

}

/*{
** Name: opz_ftoa	- convert function to attribute
**
** Description:
**      The routine will create a function attribute which will be used 
**      in an equivalence class.  This routine will be called only if
**      there is guaranteed to be no conflicts with vars (of attrs) in existing
**      equivalence classes.  Thus, there is no need to link the function
**      attribute back to the main qualification since it will be contained
**      in a joinop attribute which will be in an equivalence class.
**
** Inputs:
**      subquery                        ptr to subquery being processed
**      fatt                            ptr to allocated function attribute
**                                      element to be used for this function
**      fvarno                          local range variable to be associated
**                                      with this function attribute
**      function                        ptr to query tree representing the
**                                      function
**      datatype                        datatype and length required for the
**                                      result of the function
**      class                           type of function attribute i.e.
**                                      OPZ_TCONV, OPZ_SVAR, OPZ_MVAR
**
** Outputs:
**	Returns:
**	    joinop attribute number for the newly created function attribute
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	22-apr-86 (seputis)
**          initial creation
**	8-jul-97 (inkdo01)
**	    Flag SVAR fattrs which ref inner table of oj so func isn't
**	    materialized 'til oj is executed.
**	24-aug-00 (hayke02)
**	    Modification to above change - we now only flag the SVAR fattrs
**	    as OPZ_OJSVAR if they are not inner to the OJ. This change
**	    fixes bug 102283.
**	20-sep-01 (inkdo01)
**	    Removed above change - it incorrectly undid the "jul-97" fix above.
**	23-mar-05 (inkdo01)
**	    Modify OJSVAR change so they're only flagged when the function 
**	    attr contains a "ifnull" and is therefore sensitive to OJ
**	    null semantics.
**	23-sep-05 (hayke02)
**	    Initialise opz_ijmap to NULL and set it to varp->opv_ijmap
**	    if this is a WHERE clause OJSVAR func att (OPL_NOOUTER opz_ojid)
**	    This change fixes bug 114912.
*/
OPZ_IATTS
opz_ftoa(
	OPS_SUBQUERY       *subquery,
	OPZ_FATTS          *fatt,
	OPV_IVARS	   fvarno,
	PST_QNODE          *function,
	DB_DATA_VALUE      *datatype,
	OPZ_FACLASS	   class)
{
    OPZ_IATTS		newattr;    /* new joinop attribute number assigned
				    ** to the function attribute
				    */
    PST_QNODE           *resdom;    /* result domain node required for
                                    ** query compilation */
    OPV_VARS		*fvarp;
    bool		ifnull_present;
    fvarp = subquery->ops_vars.opv_base->opv_rt[fvarno];
    opz_rdatatype(subquery, 
	&function->pst_sym.pst_dataval,
	datatype->db_datatype,
	&fatt->opz_dataval);	    /* calculate the proper length for the
                                    ** function attribute and return it
                                    ** into fatt->opz_dataval */
    resdom = opv_resdom(subquery->ops_global, function, &fatt->opz_dataval); /*
                                    ** a resdom node which will be used to 
				    ** evaluate the  function */
    newattr = opz_addatts(subquery, fvarno, (OPZ_DMFATTR) OPZ_FANUM, 
	&fatt->opz_dataval);

    subquery->ops_attrs.opz_base->opz_attnums[newattr]->opz_func_att 
	= fatt->opz_fnum;	    /* assign the
				    ** function attribute number to the joinop
				    ** attribute element
				    */

    fatt->opz_function = resdom;
    fatt->opz_attno = newattr;
    fatt->opz_computed = FALSE;
    fatt->opz_type = class;		/* assign the type, should be
					** OPZ_TCONV, OPZ_SVAR, OPZ_MVAR
					*/
    fatt->opz_ijmap = NULL;
    ifnull_present = opz_findop(function, (ADI_OP_ID) ADI_IFNUL_OP);
    if (ifnull_present && class == OPZ_SVAR && fvarp->opv_ojeqc != OPE_NOEQCLS)
    {
	fatt->opz_mask |= OPZ_OJSVAR;
				    /* flag SVAR fattrs which must wait 'til 
				    ** oj to be evaluated */
	fatt->opz_ojid = function->pst_sym.pst_value.pst_s_op.pst_joinid;
				    /* and save OJ id for later */
	if (fatt->opz_ojid == OPL_NOOUTER)
	    fatt->opz_ijmap = fvarp->opv_ijmap;
    }
    else fatt->opz_ojid = OPL_NOOUTER;

    /* assign equiv classes to the function's vars
    ** because this join claus will be dropped from
    ** the query tree in ope_findjoins.
    */
    MEfill( sizeof(fatt->opz_eqcm), (u_char) 0, (PTR)&fatt->opz_eqcm);
#ifdef OPM_FLATAGGON
    if (!(fatt->opz_mask & OPZ_NOEQCLS))
	/* do not create for flattened aggregates at this point since joins
	** have not been processed */
#endif
	ope_aseqcls( subquery, (OPE_BMEQCLS *)&fatt->opz_eqcm, function);
    return(newattr);
}

/*{
** Name: opz_fatest	- attempt to create function attributes
**
** Description:
**      The routine will attempt to create function attributes associated
**      with the qualification.  If the creation would succeed then it is done
**      otherwise no change is made to the equivalence class array or function
**      attribute array.
**
** Inputs:
**      subquery                        ptr to subquery being processed
**      fatt                            ptr to element containing qualification
**      lvar                            range variable which is to be used
**                                      for the left function attribute
**      rvar                            range variable which is to be used
**                                      for the right function attribute
**
** Outputs:
**	Returns:
**	    joinop range varno which would not create a conflict
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	22-apr-86 (seputis)
**          initial creation
**      29-mar-90 (seputis)
**          fix byte alignment problems
**	29-nov-95 (wilri01)
**	    Bug 71040 - Don't allow function attr when subquery has union view. 
**	    Note: This may be allowed in the future, but the design intent of
**	 	  equivalence classes from remote action nodes and the assignment
**		  of a var to a function attribute when it is derived from multiple
**		  vars is not understood enough at this point in time.
**	5-dec-02 (inkdo01)
**	    Changes for range table expansion.
**      09-jul-2008 (huazh01)
**          back out the fix to bug 71040. It causes columns joined in oj which 
**          is nested with union view oj qry to be placed into 2 different eqcls.
**          for example: select .. from t1 left join t2 on t1.col1 = t2.col1
**          left join ... left join union_view on t1.col1 = union_view.col1
**          "t1.col1 = t2.col1" has been placed into 2 different eqcls and makes
**          t1 not joinable with t2, causing OPF picks a less optimal plan. 
**          Bug 119028.  
[@history_line@]...
*/
static OPV_IVARS
opz_fatest(
	OPS_SUBQUERY       *subquery,
	OPZ_IATTS	   attno,
	OPV_GBMVARS	   *varmap)
{
    OPE_IEQCLS		equcls;	    /* equivalence class associated with attno*/
    OPZ_ATTS            *attrptr;   /* ptr to joinop attribute element */

    attrptr = subquery->ops_attrs.opz_base->opz_attnums[attno];
    equcls = attrptr->opz_equcls;
    if ( equcls != OPE_NOEQCLS )
	/* If the equivalence class has not been assigned to attribute then
	** any variable from the varmap will not cause a conflict
	*/
    {	/* equivalence class exists so find a var which does not cause a
        ** a conflict with attno
        */
	OPZ_IATTS	    attr;	/* joinop attribute contained
                                        ** in equivalence class
                                        */
        OPZ_BMATTS          *attrmap;   /* map of attributes in equivalence
                                        ** class */
	OPZ_AT              *abase;     /* ptr to base of array of ptrs to
                                        ** joinop attributes */

        abase = subquery->ops_attrs.opz_base;
	attrmap = &subquery->ops_eclass.ope_base->ope_eqclist[equcls]
	    ->ope_attrmap;		/* get ptr to attribute map of
                                        ** of equivalence class
                                        */
	for( attr = -1; 
	     (attr = BTnext((i4)attr, (char *)attrmap, (i4) BITS_IN(*attrmap)))
		>= 0;)
	    /* test if equivalence class variable is not in the varmap, 
	    ** otherwise the creation of a new function attribute will fail
	    */
	    BTclear( (i4)abase->opz_attnums[attr]->opz_varnm, (char *)varmap );
    }
    if (BTcount((char *)&varmap, OPV_MAXVAR))
	return( BThigh( (char *) varmap, (i4) BITS_IN(OPV_GBMVARS) )); /* at 
					** least one var will not cause a
					** cause a conflict */
    else
	return( OPV_NOVAR );		/* all attributes would cause a conflict
                                        ** so return qualification to the main
                                        ** query
                                        */
}
#if 0

/*{
** Name: opz_cfattr	- copy function attribute into qualification
**
** Description:
**      Outer join semantics require the qualfication to be evaluated
**	to get the correct "NULL" handling with equivalence classes 
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      qnodep                          AND node to be copied and placed
**					into conjunct list
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
**      1-feb-90 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opz_cfattr(
	OPS_SUBQUERY       *subquery,
	PST_QNODE          *qnodep)
{
    PST_QNODE               *root;  /* root of query tree */

    qnodep->pst_right = (PST_QNODE *)NULL;
    opv_copytree(subquery->ops_global, &qnodep); /* make a copy since
				    ** function attribute processing may
				    ** change parse trees */
    root = subquery->ops_root;
    qnodep->pst_right = root->pst_right;
    root->pst_right = qnodep;
}
#endif

/*{
** Name: opl_nestedoj	- add iftrue to function attribute if necessary
**
** Description:
**      if this is a nested outer join and the function attribute
**	expression can handle nulls either by IS NULL null joins or
**	NULL converting functions, then add the iftrue conversions
**	early so that the function attribute will be instantiated at
**	the proper time.
**
** Inputs:
**      subquery                        ptr to subquery being converted
**      ojid                            outer join id of equi join
**      varno                           variable number of var node
**					to be checked for iftrue handling
**
** Outputs:
**
**	Returns:
**	    TRUE - if iftrue needs to be added on top of function attribute
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-apr-94 (ed)
**          initial creation for b59588
[@history_template@]...
*/
static bool
opl_nestedoj(
	OPS_SUBQUERY	    *subquery, 
	OPL_IOUTER	    ojid, 
	OPV_IVARS	    varno)
{
    OPL_OUTER	    *ojp;
    OPL_OJT	    *lbase;
    OPL_IOUTER	    ojid1;
    OPL_IOUTER	    maxoj;

    maxoj = subquery->ops_oj.opl_lv;
    lbase = subquery->ops_oj.opl_base;
    ojp = lbase->opl_ojt[ojid];
    for (ojid1 = -1; (ojid1=BTnext((i4)ojid1, (char *)ojp->opl_idmap,
	maxoj))>=0; )
    {
	OPL_OUTER	*ojp1;
	ojp1 = lbase->opl_ojt[ojid1];
	if ((ojp1->opl_type == OPL_FULLJOIN)
	    ||
	    (ojp1->opl_type == OPL_LEFTJOIN))
	{
	    if (BTtest((i4)varno, (char *)ojp1->opl_ivmap))
		return(TRUE);	    /* an outer join found for which this
				    ** relation is an inner */
	}
    }
    return(FALSE);
}

/*{
** Name: opl_faiftrue	- traverse function attribute query tree
**
** Description:
**      Traverse the function attribute query tree and place the iftrue 
**      function over var nodes when appropriate. 
**
** Inputs:
**      subquery                        ptr to subquery to analyze
**      rootpp                          ptr to current parse tree node
**      ojid                            outer join id from which function
**					attribute equi-join originated
**
** Outputs:
**
**	Returns:
**	    TRUE - if at least one iftrue function substitution occurred
**	Exceptions:
**
** Side Effects:
**
** History:
**      12-may-93 (ed)
**          initial creation to correct outer join handling of function attributes
[@history_template@]...
*/
static bool
opl_faiftrue(
    OPS_SUBQUERY	*subquery, 
    PST_QNODE		**rootpp, 
    OPL_IOUTER		ojid)
{   /* there is an outer join in this subquery, so there may
    ** need to be an "IFTRUE" function placed around this
    ** var node to indicate whether a NULL value should be
    ** used */
    bool	found;
    found = FALSE;
    for (;*rootpp; rootpp = &(*rootpp)->pst_left)
    {
	if ((*rootpp)->pst_sym.pst_type != PST_VAR)
	{
	    if ((*rootpp)->pst_right)
		found |= opl_faiftrue(subquery, &(*rootpp)->pst_right, ojid);
	}
	else
	{
	    OPE_IEQCLS      special_eqc;
	    OPV_VARS	    *varp;
	    varp = subquery->ops_vars.opv_base->opv_rt[
		(*rootpp)->pst_sym.pst_value.pst_s_var.pst_vno];
	    special_eqc = varp->opv_ojeqc;
	    if ((special_eqc != OPE_NOEQCLS)
		&&
		(
		    (	(ojid < 0)
			||
			(
			    (varp->opv_ojattr 
			    != 
			    (*rootpp)->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id)
					/* a special eqc test may already have been added
					** so do not place another special eqc on top of this */
			    &&
			    opl_nestedoj(subquery, ojid, (OPV_IVARS)(*rootpp)->
				pst_sym.pst_value.pst_s_var.pst_vno)
			)
		    )
		))
	    {
		opl_iftrue(subquery, rootpp, ojid); /* attach an iftrue
					** function to the parse tree */
		return(TRUE);
	    }
	}
    }
    return(found);
}

/*{
** Name: opz_iftrue	- insert iftrue functions if necessary
**
** Description:
**      If the function attribute contains var nodes which are the output
**	of an outer join then there is a possibility that the iftrue function
**	will be needed to distinguish NULL values. 
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      function                        ptr to query tree expression fragment
**      ojid                            outer join id
**      fattrp                          function attribute descriptor
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
**      13-may-93 (ed)
**          initial creation for outer join processing
[@history_template@]...
*/
static VOID
opz_iftrue(
	OPS_SUBQUERY	    *subquery, 
	PST_QNODE	    **functionpp, 
	OPL_IOUTER	    ojid, 
	OPZ_FATTS	    *fattrp,
	OPZ_FACLASS	    *fclassp)
{
    /* convert varnode to iftrue */
    if (opl_faiftrue(subquery, functionpp, ojid))
    {	/* re-resolve since non-nullable changes possible */
	PST_QNODE	*faqnodep;
	if ((*fclassp == OPZ_SVAR) || (*fclassp == OPZ_TCONV))
	    *fclassp = OPZ_MVAR; /* since at least one iftrue was
			    ** inserted, this function attribute must
			    ** be computed at an interior node since
			    ** a special eqc is now needed */
	opl_resolve(subquery, *functionpp);
	if (((*functionpp)->pst_sym.pst_dataval.db_datatype < 0)
	    &&
	    (fattrp->opz_dataval.db_datatype > 0))
	{
	    fattrp->opz_dataval.db_datatype = 
		-fattrp->opz_dataval.db_datatype;
	    fattrp->opz_dataval.db_length++;
	}
    }
}

/*{
** Name: opz_copyqual	- place equi-join outer join qual back in list
**
** Description:
**      Place a copy of the equi-join which is about to be made into a
**	function attribute, back into the list in order to help with
**	relation placement.  Also, mark the qualification so that later
**	boolean factor processing will  not perform histogramming since
**	this will already be done for equi-join. 
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      fatt                            function attribute descriptor
**
** Outputs:
**
**	Returns:
**	    
**	Exceptions:
**
** Side Effects:
**
** History:
**      5-aug-93 (ed)
**          initial creation for outer joins
**	9-oct-01 (inkdo01)
**	    Remove setting of PST_OJJOIN flag.
[@history_template@]...
*/
static VOID
opz_copyqual(
	OPS_SUBQUERY	*subquery, 
	OPZ_FATTS	*fatt)
{   /* create copy of qualification and place into boolean
    ** factor list to help with relation placement */
    PST_QNODE	    *qnodep;
    PST_QNODE	    *savep;
    qnodep = fatt->opz_function;
    if (fatt->opz_left.opz_nulljoin)
    {
	savep = qnodep->pst_left;
	qnodep->pst_left = fatt->opz_left.opz_nulljoin;
    }
    opv_copytree(subquery->ops_global, &qnodep);
    /* The following flag set seems to cause no useful actions. In fact,
    ** it seems to inhibit the inclusion of func. att. comparisons in 
    ** the ON clause ADF code of outer joins - the only place they can
    ** be properly executed for key joins. */
    /* qnodep->pst_left->pst_sym.pst_value.pst_s_op.pst_flags
	|= PST_OJJOIN;	/* mark boolean factor as being
			** an equi join so as to
			** not participate in histogramming */
    if (fatt->opz_left.opz_nulljoin)
    {
	fatt->opz_function->pst_left = savep; /* restore IS NULL OR part of
				    ** qualification */
	fatt->opz_right.opz_nulljoin->pst_right = subquery->ops_root->pst_right;
	subquery->ops_root->pst_right = fatt->opz_right.opz_nulljoin; /*
				    ** place second OR part of null join back
				    ** into the query */
	subquery->ops_root->pst_right->pst_left->pst_sym.
	    pst_value.pst_s_op.pst_flags|= PST_OJJOIN; /* mark boolean 
				    ** factor as being an equi join so as to
				    ** not participate in histogramming */
    }
    qnodep->pst_right = subquery->ops_root->pst_right;
    subquery->ops_root->pst_right = qnodep; /* restore boolean factor */
}


/*{
** Name: opz_fvar	- qualification has a function and a var
**
** Description:
**      This routine is used when a qualification has a function and a var
**      node.  It will check whether the creation of the function attribute
**      will conflict with the var node.  If it does not then the function
**      attribute will be created.  Otherwise, the qualification will be
**      replaced in the query.
**
** Inputs:
**      subquery                        ptr to subquery being processed
**      fatt                            function attribute element
**      varattrno                       attribute number of the var node
**      function                        query tree node at head of function
**      fvarmap                         variable map of the function
**      class                           type of function attribute i.e.
**                                      OPZ_TCONV, OPZ_SVAR, OPZ_MVAR
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
**	22-apr-86 (seputis)
**          initial creation
**	22-oct-88 (seputis)
**          modified mergeatts to handle NULL joins
**      13-apr-92 (seputis)
**          fix bug 43569 - restore null join qual if equi-join cannot
**          be made.
**      22-apr-94 (ed)
**	    b59588 - add iftrue to null joins involving outer joins
**	    to obtain correct answer, even though it restricts flexibility
**	    of associativity
**	6-may-94 (ed)
**	    - PST_NOJOIN is not valid at this point, and joinid's >= 0 need
**          to be returned to the boolean factor list to help with outer join
**	    relation placement
**	8-sep-00 (inkdo01)
**	    Removed "#if 0" which avoided iftrue logic, but caused failure of
**	    join containing MVAR func attr in its ON clause in which parms
**	    originated in opposite sides of a nested outer join (partially 
**	    reverses change 409457).
[@history_line@]...
*/
static VOID
opz_fvar(
	OPS_SUBQUERY       *subquery,
	OPZ_FATTS          *fatt,
	OPZ_IATTS	   varattrno,
	PST_QNODE          *function,
	OPZ_FOPERAND	   *operandp)
{
    OPV_IVARS		fvarno;	    /* variable number which can be assigned
				    ** to function attribute which will not
				    ** cause a conflict
				    */
    fvarno = opz_fatest(subquery, varattrno, &operandp->opz_varmap); /* check if there is
			    ** a variable in the function varmap which
                            ** will not cause a conflict
			    */
    if (fvarno != OPV_NOVAR)
    {	/* a variable exists which will not cause a conflict so create the
        ** function attribute
	*/
	OPZ_IATTS              newattr;	/* new joinop attribute created for
					** the function attribute
                                        */
	OPL_IOUTER	       ojid;

	ojid = fatt->opz_function->pst_left->pst_sym.pst_value.
	    pst_s_op.pst_joinid;
	if (subquery->ops_oj.opl_base
	    &&
	    (ojid >= 0))
	    opz_copyqual(subquery, fatt);
#if 0
/* not needed since strength reduction solves problem */
	if (fatt->opz_mask & OPZ_SOJQUAL)
	    /* make a copy of the qualification and place back into
	    ** list for outer join semantics */
	    opz_cfattr(subquery, fatt->opz_function);
#endif

/* try to add iftrue functions after query plan has been decided
** to give more flexibility since they may not be needed */
	if((operandp->opz_opmask & OPZ_OPIFTRUE)
#if 1
	    ||
	    (	(operandp->opz_class == OPZ_MVAR)
		&&
		subquery->ops_oj.opl_base
	    )
#endif
	    )
	    opz_iftrue(subquery, &function, ojid, fatt, &operandp->opz_class);

	newattr = opz_ftoa(subquery, fatt, fvarno, function, 
			   &fatt->opz_dataval, operandp->opz_class);
	if (!ope_mergeatts(subquery, newattr, varattrno,  
		(fatt->opz_left.opz_nulljoin != NULL), ojid))
# ifdef E_OP0382_FATTR_CREATE
	    opx_error(E_OP0382_FATTR_CREATE) /* consistency check error since 
                                        ** the merging of the function 
                                        ** attribute should not fail in this 
					** case */
# endif
	   ;
    }
    else
    {	/* there was a conflict so return the qualification
	** back to the main query
	*/
	PST_QNODE		*root;	/* root of query tree */

	root = subquery->ops_root;
	fatt->opz_function->pst_right = root->pst_right;
        root->pst_right = fatt->opz_function; /* restore boolean factor */
        if (fatt->opz_left.opz_nulljoin)
	{
            fatt->opz_function->pst_left = fatt->opz_left.opz_nulljoin; /* if a
                                        ** null join was used then
                                        ** restore the original null join
                                        ** value */
	    fatt->opz_right.opz_nulljoin->pst_right = root->pst_right;
	    root->pst_right = fatt->opz_right.opz_nulljoin; /* restore
					** second part of IS NULL OR */
	}
	fatt->opz_type = OPZ_NULL;	/* mark function attribute node
					** as being unused
                                        */
    }
}

/*{
** Name: opz_fboth	- qualification requires two function attributes
**
** Description:
**      This routine is called when both sides of a qualification requires 
**      function attributes to be created.  Since function attributes 
**      are created for both, there can be no failure during merging of
**      the attributes into the same equivalence class, as long as different 
**      vars are assigned to each of the function attributes.  This can be
**      guaranteed since this qualification would not be considered if the
**      left and right varmaps had any intersection at all.
**
**	This routine will create two function attributes and place them
**	into the same equivalence class.
** Inputs:
**      subquery                        ptr to subquery being processed
**      fatt                            ptr to function attribute element
**                                      containing the qualification
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
**	25-apr-86 (seputis)
**          initial creation
**	25-oct-88 (seputis)
**          modified mergeatts to handle NULL joins
**	26-mar-94 (ed)
**	    correct uninitialized variable problem
**      22-apr-94 (ed)
**	    b59588 - add iftrue to null joins involving outer joins
**	    to obtain correct answer, even though it restricts flexibility
**	    of associativity
**	6-may-94 (ed)
**	    - PST_NOJOIN is not valid at this point, and joinid's >= 0 need
**          to be returned to the boolean factor list to help with outer join
**	    relation placement
**	8-sep-00 (inkdo01)
**	    Removed "#if 0" which avoided iftrue logic, but caused failure of
**	    join containing MVAR func attr in its ON clause in which parms
**	    originated in opposite sides of a nested outer join (partially 
**	    reverses change 409457).
**	9-oct-01 (inkdo01)
**	    Do quick return when ON clause is a "both", so join factor doesn't
**	    end up in equijoin map when outer join logic requires it to be 
**	    executed later in the process.
**	30-mar-04 (hayke02)
**	    Back out previous change - it causes problem INGSRV 1957, bug
**	    108930.
[@history_line@]...
*/
static VOID
opz_fboth(
	OPS_SUBQUERY       *subquery,
	OPZ_FATTS          *fatt)
{
    PST_QNODE		*qualification;	/* BOP "=" node of qualification */
    OPZ_IATTS		rattr;		/* joinop function attribute number
					** assigned to right part of
					** qualification
					*/
    OPZ_IATTS		lattr;		/* joinop function attribute number
					** assigned to left part of
					** qualification
					*/
    OPL_IOUTER		ojid;

    qualification = fatt->opz_function->pst_left;   /* get ptr to "=" node */
    ojid = qualification->pst_sym.pst_value.pst_s_op.pst_joinid;
    if (subquery->ops_oj.opl_base
	&&
	(ojid >= 0))
	opz_copyqual(subquery, fatt);
#if 0
not needed since strength reduction solves the problem
    if (fatt->opz_mask & OPZ_SOJQUAL)
        /* make a copy of the qualification and place back into
	** list for outer join semantics */
	opz_cfattr(subquery, fatt->opz_function);
#endif
    {	/* create function attribute for right side of qualification */
	OPV_IVARS	    rvarno;	/* range variable to use for creation
					** of the function attribute
					*/
	OPZ_FATTS           *tempfatt;	/* ptr to function attribute element
					** used to represent right part of
					** qualification
					*/
        rvarno = BThigh((char *) &fatt->opz_right.opz_varmap, 
			(i4) BITS_IN(OPV_GBMVARS));
	/* create a new function attribute element since fatt is still
	** being used ... by finding an empty function attribute element
	*/
	tempfatt = opz_fmake(subquery, qualification->pst_right, OPZ_QUAL);

/* try to add iftrue functions after query plan has been decided
** to give more flexibility since they may not be needed */
	if((fatt->opz_right.opz_opmask & OPZ_OPIFTRUE)
#if 1
	    ||
	    (	(fatt->opz_right.opz_class == OPZ_MVAR)
		&&
		subquery->ops_oj.opl_base
	    )
#endif
	    )
	    opz_iftrue(subquery, &qualification->pst_right, ojid, fatt, 
		&fatt->opz_right.opz_class);
	rattr = opz_ftoa(subquery, tempfatt, rvarno, qualification->pst_right,
	    &fatt->opz_dataval, fatt->opz_right.opz_class);	/* convert 
					** function to attribute */
    }

    {	/* create function attribute for left side of qualification */
	OPV_IVARS	    lvarno;	/* range variable to use for creation
					** of the function attribute
					*/
        lvarno = BThigh((char *) &fatt->opz_left.opz_varmap, 
			(i4) BITS_IN(OPV_GBMVARS));
	if((fatt->opz_left.opz_opmask & OPZ_OPIFTRUE)
#if 1
	    ||
	    (	(fatt->opz_left.opz_class == OPZ_MVAR)
		&&
		subquery->ops_oj.opl_base
	    )
#endif
	    )
	    opz_iftrue(subquery, &qualification->pst_left, ojid, fatt,
		&fatt->opz_left.opz_class);

	lattr = opz_ftoa(subquery, fatt, lvarno, qualification->pst_left,
	    &fatt->opz_dataval, fatt->opz_left.opz_class);	/* convert 
					** function to attribute */
    }
    if (!ope_mergeatts(subquery, lattr, rattr, FALSE, ojid))
# ifdef E_OP0382_FATTR_CREATE
	    opx_error(E_OP0382_FATTR_CREATE) /* consistency check error since 
                                        ** the merging of the function 
                                        ** attribute should not fail in this 
					** case */
# endif
	   ;
}

/*{
** Name: opz_fclass	- process a class of function attributes
**
** Description:
**      This routine will process a particular class of function attributes.
**      If the left and right parts of the qualification are within the
**      class then the function attribute will be processed.
**
** Inputs:
**      subquery                        ptr to subquery being processed
**      class                           class of function attributes
**                                      which should be processed
**                                      - if either the left or right function
**                                      attribute have a class of lower
**                                      priority then this qualification will
**                                      not be processed
**
** Outputs:
**	Returns:
**	    TRUE if any qualifications remain unprocessed
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	25-apr-86 (seputis)
**          initial creation
**	10-sep-03 (hayke02)
**	    Create function attributes in the presence of collation sequence
**	    and a join between character types. This prevents incorrect results
**	    when we use the data from another attribute in a joining
**	    equivalence class. This change fixes bug 110675.
**      21-Oct-2003 (hanal04) Bug 103856 INGSRV2557
**          Create function attributes in the presence of VARs with different
**          nullability and/or length if this is a distributed query using
**          cursors. This prevents protocol errors which occur when the
**          received datatype is sent in as an equivalent db_datatype.
**	 6-sep-04 (hayke02)
**	    Back out changes made in fix for 110675/INGSRV2450 and fix instead
**	    elsewhere in opz. This change fixes problem INGSRV 2940, bug
**	    112873.
**          
[@history_line@]...
*/
static bool
opz_fclass(
	OPS_SUBQUERY        *subquery,
	OPZ_FACLASS         class)
{
    OPZ_IFATTS		max_fa;	    /* index of last free slot in function
                                    ** attribute attribute array which contains
                                    ** a qualification
                                    */
    OPZ_FT              *fbase;     /* base of function attribute array */
    bool		unprocessed;/* TRUE if any qualifications are
				    ** unprocessed
				    */
    OPZ_AT              *abase;     /* base of joinop attribute array */

    unprocessed = FALSE;	    /* assume all function attributes are
				    ** processed
				    */
    if (!(max_fa = subquery->ops_funcs.opz_fv))
	return(unprocessed);	    /*return if no function attributes defined*/
    fbase = subquery->ops_funcs.opz_fbase;
    abase = subquery->ops_attrs.opz_base;

    {	/* analyze all elements which are still qualifications and convert
	** any in which both branches of the qualification are within the
	** given class
	*/
	OPZ_IFATTS		index;	/* index into function attribute array*/

	for ( index = 0; index < max_fa ; index++)
	{
	    OPZ_FATTS            *fatt;	/* ptr to function attribute element */
	    fatt = fbase->opz_fatts[index]; /* get ptr to function attribute
                                        ** descriptor */
	    if (fatt->opz_type != OPZ_QUAL)
		continue;		/* continue if already processed */
	    if ((fatt->opz_right.opz_class > class) ||
		(fatt->opz_left.opz_class > class))
		unprocessed = TRUE;	/* at least one unprocessed qual. */
	    else
	    {
		PST_QNODE	    *eqnode;    /* BOP "=" node of qualification
                                                */
		DB_DT_ID            result;	/* target type for function 
						** attr */

		eqnode = fatt->opz_function->pst_left;
		result = fatt->opz_dataval.db_datatype;
		if (fatt->opz_left.opz_class == OPZ_VAR)
		{   /* process case in which left branch is a VAR */
		    OPZ_IATTS	    leftattr;	/* joinop attribute of var node
						*/
		    leftattr = (OPZ_IATTS)eqnode->pst_left->
			    pst_sym.pst_value.pst_s_var.pst_atno.db_att_id;

                    if ( (((subquery->ops_global->ops_cb->ops_smask &
                                                      OPS_MDISTRIBUTED)
                        &&
                        (subquery->ops_global->ops_qheader->pst_mode
                                                        == PSQ_DEFCURS)
                        &&
                        (result
                        ==
                        abase->opz_attnums[leftattr]->opz_dataval.db_datatype)
                        &&
                        (fatt->opz_dataval.db_length
                        ==
                        abase->opz_attnums[leftattr]->opz_dataval.db_length))
                        ||
                        ((((subquery->ops_global->ops_cb->ops_smask
                                                    & OPS_MDISTRIBUTED) == 0)
                        ||
                        (subquery->ops_global->ops_qheader->pst_mode
                                                    != PSQ_DEFCURS))
                        &&
                        (abs(result)
                        ==
                        abs(abase->opz_attnums[leftattr]->opz_dataval.
                                                    db_datatype)))
                        )
		       )
		    {	/* if the var node does not need to be converted
                        ** then only the right side needs to be a function
                        ** attribute */
			opz_fvar( subquery, fatt, leftattr,
			    eqnode->pst_right, &fatt->opz_right);
			continue;
		    }
		}
		if (fatt->opz_right.opz_class == OPZ_VAR)
		{   /* process case in which right branch is a VAR */
		    OPZ_IATTS	    rightattr;	/* joinop attribute of varnode
						*/
		    rightattr = (OPZ_IATTS)eqnode->pst_right->
			    pst_sym.pst_value.pst_s_var.pst_atno.db_att_id;

                    if ( (((subquery->ops_global->ops_cb->ops_smask &
                                                      OPS_MDISTRIBUTED)
                        &&
                        (subquery->ops_global->ops_qheader->pst_mode
                                                        == PSQ_DEFCURS)
                        &&
                        (result
                        ==
                        abase->opz_attnums[rightattr]->opz_dataval.db_datatype)
                        &&
                        (fatt->opz_dataval.db_length
                        ==
                        abase->opz_attnums[rightattr]->opz_dataval.db_length))
                        ||
                        ((((subquery->ops_global->ops_cb->ops_smask
                                                    & OPS_MDISTRIBUTED) == 0)
                        ||
                        (subquery->ops_global->ops_qheader->pst_mode
                                                    != PSQ_DEFCURS))
                        &&
                        (abs(result)
                        ==
                        abs(abase->opz_attnums[rightattr]->opz_dataval.
                                                    db_datatype)))
                        )
		       )
		    {	/* if the var node does not need to be converted
                        ** then only the right side needs to be a function
                        ** attribute */
			opz_fvar( subquery, fatt, rightattr,
			    eqnode->pst_left,
			    &fatt->opz_left);
			continue;
		    }
		}
		/* handle case in which both sides of qualification needs
                ** to be converted to function attributes... there will not
                ** be any conflicts (i.e. 2 attributes of same var cannot
	        ** be placed in the same equivalence class) 
                ** since a new equivalence class will be
                ** created for both the function attributes.
                */
		opz_fboth( subquery, fatt );
	    }
	}
    }
    return(unprocessed);	/* TRUE is at least one unprocessed qual */
}

/*{
** Name: opz_fattr	- analyze function attributes
**
** Description:
**      Analyzes all qualifications which could be used for joins if function
**      attributes were created.  If a useful join could be created then
**      the function attributes will be created for the qualification.
**      Otherwise the clause will be placed back in the qualification list
**      for the subquery.  This approach is an improvement over 4.0 in that
**      if the "wrong variable" was assigned to the function attribute
**      then a possibly useful join would not be made.
**
**      There is a preferred order for creation of function attributes since
**      the creation of one function attribute may prevent the creation of
**      other function attributes (due to var conflicts in the attribute maps
**      of the equivalence classes).  OPZ_MULTIVAR functions do not improve
**      the connectivity so these function attributes should be created last.
**      It is preferrable to create OPZ_VAR attributes prior to OPZ_TCONV,
**      prior to OPZ_SVAR prior to OPZ_MVAR.  
**
**      FIXME - one criterion for creation of function attributes was that
**      no vars would intersect on either side of the equi-join.  This is
**      not entirely optimal since equivalence classes are used to join
**      rather than vars.  An improvement would be to detect whether or not
**      the vars in common between the left and right side of the equi-join
**      are from equivalence classes which contain more than one attribute
**      and thus reference more than one var.  Thus, if each of the vars
**      in the intersection belonged to some equivalence class which with
**      more than one member, then it is conceivable the more connectedness
**      could be provided between the vars not in the intersection.  This
**      is because the function attribute can be evaluated prior to the node
**      that used it for a join; i.e. function attributes are worthless unless
**      they can be evaluated prior to the join node which uses it for a join.
**
**      Future enhancement may be to detect identical function attributes so
**      that f(a) = f(b) and f(b) = f(c) implies f(a) = f(c).
**      It may be worthwhile checking if the attributes used in the functions
**      are subsets of some other equivalence class, so that the creation
**      of the function attribute is not worthwhile (i.e. is should be
**      a boolean factor in this case), since it does not provide any extra
**      "joinability" or "connectedness".
**
**      FIXME - check for multi-variable function attributes which can
**      be made into single-variable function attributes by applying
**      transitive property of equivalence classes
**      FIXME - the current enumeration scheme has a bug related to
**      function attributes - this is only a problem with multi-variable
**      function attributes presently, 
**      Given the query
**          retrieve (r.a) where r.a = s.a AND r.b=s.b 
**                           AND sin(r.a+s.b) = t.a
**      the function attribute sin(r.a+s.b) will be evaluated as early
**      as possilble in the CO tree, but it is based on equivalence classes
**      rather than actual attributes.  For a join tree such as
**                       o
**                     /   \
**                    o     t
**                   / \
**                  s   r
**     The effect will be to have a clause  sin(r.a+r.b) = sin(s.a+s.b) applied
**     at the join of s and r,  this is because the function attributes
**     are evaluated on equivalence classes rather than attributes.  The 
**     optimizer will assume that this clause is restrictive which it is not.
**     This would not affect the results, but may affect performance.
**
** Inputs:
**      subquery                        ptr to subquery being processed
**          .ops_funcs                  describes possible qualifications
**                                      which would be useful for function attrs
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    Function attributes could be created in the joinop attributes array
**
** History:
**	22-apr-86 (seputis)
**          initial creation
**      24-jul-88 (seputis)
**          init opz_fmask, partial fix for b6570
[@history_line@]...
*/
VOID
opz_fattr(
	OPS_SUBQUERY       *subquery)
{
    if (opz_fclass(subquery, OPZ_TCONV)   /* process all qualifications
				    ** which require only a conversion
				    */
	&&
	opz_fclass(subquery, OPZ_SVAR) )   /* process all qualifications
				    ** which have functions containing only
				    ** only variable
				    */
    {
	if (!opz_fclass(subquery, OPZ_MVAR) )   /* process remaining qualifications
					*/
	    subquery->ops_funcs.opz_fmask |= OPZ_MAFOUND;
	else
# ifdef E_OP0382_FATTR_CREATE
	    opx_error(E_OP0382_FATTR_CREATE) /* there should
				    ** be no unprocessed qualifications after
				    ** the multi-variable function attribute
				    ** case is considered
				    */
# endif
	   ;
    }
}

/*{
** Name: opz_faeqcmap	- create function attribute eqc map
**
** Description:
**      This routine will determine which function attributes can be computed
**	given the available equivalence classes. 
**
** Inputs:
**      subquery                        ptr to subquery to process
**      targetp                         ptr to eqcmap available
**      varmap                          ptr to varmap available
**
** Outputs:
**
**	Returns:
**	    
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      20-nov-92 (ed)
**          initial creation
[@history_template@]...
*/
VOID
opz_faeqcmap(
	OPS_SUBQUERY	*subquery,
	OPE_BMEQCLS	*targetp,
	OPV_BMVARS	*varmap)
{
    OPZ_IFATTS	    maxfattr;	    /* maximum function attribute
				    ** defined */
    maxfattr = subquery->ops_funcs.opz_fv; /* maximum number of
				    ** function attributes defined */
    if (maxfattr > 0)
    {	
	OPZ_IFATTS            fattr; /* function attribute currently
				    ** being analyzed */
	OPZ_FT                *fbase; /* ptr to base of an array
				    ** of ptrs to function attributes*/
	OPZ_AT                *abase; /* ptr to base of an array
				    ** of ptrs to attributes */
	OPE_IEQCLS	      maxeqcls;

	maxeqcls = subquery->ops_eclass.ope_ev;
	abase = subquery->ops_attrs.opz_base; /* ptr to base of array
				    ** of ptrs to joinop attributes
				    */
	fbase = subquery->ops_funcs.opz_fbase; /* ptr to base of array
				    ** of ptrs to function attribute
				    ** elements */
	for (fattr = 0; fattr < maxfattr; fattr++)
	{
	    OPZ_FATTS		*fattrp; /* ptr to current function
				    ** attribute being analyzed */

	    fattrp = fbase->opz_fatts[fattr]; /* get ptr to current
				    ** function attr being analyzed*/
	    switch (fattrp->opz_type)
	    {
	    case OPZ_MVAR:
	    case OPZ_TCONV:
	    case OPZ_SVAR:
	    case OPZ_FUNCAGG:
	    {
		if (!(fattrp->opz_mask & OPZ_OJFA)
		    /* check that outer join equivalence class is available
		    ** at this node by checking that all variables which are
		    ** inner, exist in this partition, and that at least one
		    ** outer variable exists */
		    &&
		    BTsubset ((char *)&fattrp->opz_eqcm, 
			      (char *) targetp,
			      (i4) maxeqcls)	/* check that equivalence classes
					    ** are available for function
					    ** attribute */
#ifdef OPM_FLATAGGON
		    &&
		    (	(fattrp->opz_type != OPZ_FUNCAGG)
			||
			(   varmap
			    &&
			    BTsubset((char *)&fattrp->opz_fagg->opm_ivmap,
				(char *)varmap,
				(i4)subquery->ops_vars.opv_rv)    /* check 
					    ** that all relations defined
					    ** in the original subselect are available
					    */
			)
		    )
#endif
		  )
		{   /* set the equivalence class if the attribute can be evaluated
		    ** at this point */
		    BTset((i4)abase->opz_attnums[fattrp->opz_attno]->opz_equcls,
			(char *)targetp);
		}
		break;
	    }
	    default:
		;
	    }
	}
    }
}
