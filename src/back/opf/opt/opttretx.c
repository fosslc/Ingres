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
#include    <uld.h>
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
#include    <opcd.h>
/* external routine declarations definitions */
#define        OPT_TREETEXT      TRUE
#include    <me.h>
#include    <bt.h>
#include    <cv.h>
#include    <st.h>
#include    <tr.h>
#include    <opxlint.h>

/*
** Name: opttretx.c -
**
** Description:
**
** History:
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
*/

/*
** Forward Structure Definitions:
*/
typedef struct _OPT_TTCB OPT_TTCB;

/* TABLE OF CONTENTS */
static void opt_etoa(
	OPT_TTCB *textcb,
	OPE_IEQCLS eqcls);
static void opt_dstring(
	OPS_SUBQUERY *subquery,
	char *header_string,
	ULD_TSTRING *tstring,
	i4 maxline);
static bool opt_mapfunc(
	OPS_SUBQUERY *subquery,
	OPO_CO *cop,
	OPE_IEQCLS eqcls,
	OPE_BMEQCLS *func_eqcmap,
	OPE_BMEQCLS *temp_eqcmap);
static void opt_init(
	OPT_TTCB *handle,
	OPS_SUBQUERY *subquery,
	OPO_CO *cop,
	ULM_RCB *ulmrcb);
static ULD_TSTRING *opt_funcattr(
	OPT_TTCB *textcbp,
	OPZ_IATTS attno,
	OPO_CO *cop,
	ULM_SMARK *markp);
static void opt_varnode(
	OPT_TTCB *textcb,
	i4 varno,
	i4 atno,
	char **namepp);
static bool opt_conjunct(
	OPT_TTCB *handle,
	PST_QNODE **qtreepp);
static bool opt_jconjunct(
	OPT_TTCB *handle,
	PST_QNODE **qtreepp);
static bool opt_tablename(
	OPT_TTCB *global,
	ULD_TSTRING **namepp,
	char **aliaspp);
static PST_QNODE *opt_resdom(
	OPT_TTCB *handle,
	PST_QNODE *resdomp,
	char **namep);
void opt_treetext(
	OPS_SUBQUERY *subquery,
	OPO_CO *cop);

#define             OPT_MAXTEXT         72
/* max number of characters for a single line of text, in the query tree to
** text conversion */



/*}
** Name: OPT_TTCB - query tree to text conversion state
**
** Description:
**      This control block contains the state of the query tree
**      to text conversion.
**
** History:
**      4-apr-88 (seputis)
**          initial creation
**      16-aug-91 (seputis)
**          add CL include files for PC group
**      21-may-93 (ed)
**          - changed opo_var to be a union structure so that
**          the outer join ID can be stored in interior CO nodes
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	30-jun-93 (rickh)
**	    Added TR.H.
**	27-jul-93 (rblumer)
**	    Fixed merge problem where tr.h was included twice.
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	11-Aug-1997 (jenjo02)
**	    Changed ulm_streamid from (PTR) to (PTR*) so that ulm
**	    can destroy those handles when the memory is freed.
*/
struct _OPT_TTCB
{
    OPS_SUBQUERY    *subquery;		/* ptr to subquery state variable */
    ULM_RCB	    *opt_ulmrcb;
    OPO_CO	    *cop;		/* current COP node being analyzed */
    OPB_IBF	    bfno;		/* current boolean factor number
					** being evaluated */
    i4		    joinstate;		/* used when processing joining
					** conjunct to determine which attribute
                                        ** in the equivalence class to use */
#define             OPT_JOUTER          3
/* assume attribute comes from outer so use outer variable bitmap */
#define             OPT_JINNER          2
/* assume attribute comes from inner so use inner variable bitmap */
#define             OPT_JNOTUSED	1
/* attribute used in qualification so use current bitmap */
#define             OPT_JBAD		0
/* this state should not be reached */
    OPE_IEQCLS	    jeqcls;		/* equivalence class of last joining
                                        ** attribute processed or OPE_NOEQCLS
                                        ** if all attributes are processed */
    OPE_BMEQCLS     bmjoin;             /* bitmap of equivalence classes
                                        ** which are joined but not part of
                                        ** the sorted inner list; i.e. they
                                        ** will act like normal conjuncts */
    OPE_IEQCLS	    *jeqclsp;           /* pointer to a list of attributes
					** which will be involved in the join
					*/
    OPE_IEQCLS	    reqcls;             /* next resdom equilvalence class
                                        ** to be returned */
    OPV_IVARS	    opt_vno;            /* last range variable to be considered
                                        ** for from list or range table */
    ULD_TSTRING	    opt_tdesc;          /* temporary descriptor for the table
                                        ** name to be supplied to the tree to
                                        ** text routine */
    OPT_NAME	    opt_tname;          /* temporary location for table name
                                        ** associated with opt_tdesc */
    struct
    {
	PST_QNODE	    eqnode;	/* used as temp equals node for joins */
	PST_QNODE	    leftvar;	/* used as temp left node for joins */
	PST_QNODE	    rightvar;	/* used as temp right node for joins */

	PST_QNODE	    resnode;	/*node used to create temporary resdom*/
	PST_QNODE           resexpr;    /* var node used for expression */
    }   qnodes;
};


/*{
** Name: opt_etoa	- convert equivalence class to attributes
**
** Description:
**      Routine accepts an equilvalence class and initializes PST_VAR nodes
**      such that a join exists
**
** Inputs:
**      textcb                          text creation state variable
**	eqcls				equivalence class used to initialize
**                                      range variable nodes
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
**      11-apr-88 (seputis)
**          initial creation
**	09-jul-93 (andre)
**	    recast first arg to pst_resolve() to (PSF_SESSCB_PTR) to agree
**	    with the prototype declaration
[@history_line@]...
[@history_template@]...
*/
static VOID
opt_etoa(
	OPT_TTCB	    *textcb,
	OPE_IEQCLS	    eqcls)
{
    OPS_SUBQUERY    *subquery;
    OPZ_BMATTS	    *attrmap;	    /* bitmap of attributes in equivalence
				    ** class */
    OPZ_AT	    *abase;
    OPZ_IATTS	    new_attno;
    bool	    inner_found;    /* an attribute from the inner was found */
    bool	    outer_found;    /* an attribute from the outer was found */

    textcb->joinstate = OPT_JOUTER; /* use outer bitmap first */
    subquery = textcb->subquery;
    BTclear ((i4) eqcls, (char *)&textcb->bmjoin); /* this eqcls has been processed */
    abase = subquery->ops_attrs.opz_base;
    attrmap = &subquery->ops_eclass.ope_base->ope_eqclist[eqcls]->ope_attrmap;
    inner_found = FALSE;
    outer_found = FALSE;
    for (new_attno = -1;
	 (new_attno = BTnext((i4)new_attno, (char *)attrmap, (i4)OPZ_MAXATT)) >= 0;)
    {   /* look thru the list of joinop attribute associated with this
	** equivalence class and determine which ones exist in the
	** subtree associated with this CO node */
	OPZ_ATTS		*attrp;
	attrp = abase->opz_attnums[new_attno];
	if (!outer_found 
	    &&
	    BTtest((i4)attrp->opz_varnm, 
	    (char *)textcb->cop->opo_outer->opo_variant.opo_local->opo_bmvars))
	{   /* range variable for this attribute is in subtree */
	    outer_found = TRUE;
	    STRUCT_ASSIGN_MACRO(attrp->opz_dataval, textcb->qnodes.leftvar.pst_sym.pst_dataval);
	    textcb->qnodes.leftvar.pst_sym.pst_value.pst_s_var.pst_vno = attrp->opz_varnm;
	    textcb->qnodes.leftvar.pst_sym.pst_value.pst_s_var.pst_atno.db_att_id = new_attno;
	    if (inner_found)
		break;			    /* both var nodes are initialized */
	}
	else if(!inner_found 
	    &&
	    BTtest((i4)attrp->opz_varnm, 
	    (char *)textcb->cop->opo_inner->opo_variant.opo_local->opo_bmvars))
	{
	    inner_found = TRUE;
	    STRUCT_ASSIGN_MACRO(attrp->opz_dataval, textcb->qnodes.rightvar.pst_sym.pst_dataval);
	    textcb->qnodes.rightvar.pst_sym.pst_value.pst_s_var.pst_vno 
		= attrp->opz_varnm;
	    textcb->qnodes.rightvar.pst_sym.pst_value.pst_s_var.pst_atno.db_att_id = new_attno;
	    if (outer_found)
		break;			    /* both var nodes are initialized */
	}
    }
    if (inner_found && outer_found)
    {
	/* need to resolve the types for the new BOP equals node */

	DB_STATUS	    res_status;	/* return status from resolution
				** routine */
	DB_ERROR	    error; /* error code from resolution routine
				*/
	res_status = pst_resolve((PSF_SESSCB_PTR) NULL,
				/* ptr to "parser session
				** control block, which is needed for
				** tracing information */
	    subquery->ops_global->ops_adfcb, 
	    &textcb->qnodes.eqnode, 
	    subquery->ops_root->pst_sym.pst_value.pst_s_root.pst_qlang,
	    &error);
	if (DB_FAILURE_MACRO(res_status))
	    opx_verror( res_status, E_OP0685_RESOLVE, error.err_code);
    }
    else
	opx_error( E_OP0498_NO_EQCLS_FOUND);	/* both equivalence classes
					**must be found */
}

/*{
** Name: opt_dstring	- display string list to user
**
** Description:
**      This routine will print a ULD_TSTRING list to the user
**
** Inputs:
**      subquery                        subquery state variable
**      tstring                         linked list of strings to be printed
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
**      11-apr-88 (seputis)
**          initial creation
**	06-apr-92 (rganski)
**	    Fixed ansi compiler warning concerning ">" operator. Removed
**	    unnecessary parentheses also.
[@history_template@]...
*/
static VOID
opt_dstring(
	OPS_SUBQUERY	    *subquery,
	char		    *header_string,
	ULD_TSTRING	    *tstring,
	i4		    maxline)
{
    OPS_STATE	    *global;
    
    global = subquery->ops_global;
    if (tstring)
    {   /* an non-empty boolean expression was created */
	i4	    current;
	current = 0;

	TRformat(opt_scc, (i4 *)NULL,
	    (char *)&global->ops_trace.opt_trformat[0],
	    (i4)sizeof(global->ops_trace.opt_trformat),
	     header_string);
	for (;tstring; tstring = tstring->uld_next)
	{
	    if ( (current > 0) && ((i4) (tstring->uld_length + current) > maxline) )
	    {   /* start a new line since maxline is exceeded */
		TRformat(opt_scc, (i4 *)NULL,
		    (char *)&global->ops_trace.opt_trformat[0],
		    (i4)sizeof(global->ops_trace.opt_trformat),
		     "\n        %s", &tstring->uld_text[0]);
		current = tstring->uld_length;
	    }
	    else
	    {   /* concatenate lines as long as < maxline */
		TRformat(opt_scc, (i4 *)NULL,
		    (char *)&global->ops_trace.opt_trformat[0],
		    (i4)sizeof(global->ops_trace.opt_trformat),
		     "%s", &tstring->uld_text[0]);
		current += tstring->uld_length;
	    }
	}
    }
}

/*{
** Name: opt_mapfunc	- determine if function attribute can be evaluated in CO tree
**
** Description:
**	This routine will determine if the function attribute can be evaluated
**      in the CO subtree.
**
** Inputs:
**	textcb                          state variable for tree to text 
**                                      conversion
** Outputs:
**	Returns:
**	    TRUE if the function attribute is evaluated in the subtree
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      1-apr-88 (seputis)
**          initial creation
[@history_template@]...
*/
static bool
opt_mapfunc(
	OPS_SUBQUERY	    *subquery,
	OPO_CO		    *cop,
	OPE_IEQCLS	    eqcls,
	OPE_BMEQCLS         *func_eqcmap,
	OPE_BMEQCLS         *temp_eqcmap)
{
    bool	no_lower;	    /* TRUE if function attribute is not calculated at
                                    ** this node */

    no_lower = TRUE;
    if (cop->opo_outer && BTtest((i4)eqcls, (char *)&cop->opo_outer->opo_maps->opo_eqcmap))
    {
	no_lower = FALSE;
	if (opt_mapfunc(subquery, cop->opo_outer, eqcls, func_eqcmap, temp_eqcmap))
	    return(TRUE);
    }
    if (cop->opo_inner && BTtest((i4)eqcls, (char *)&cop->opo_inner->opo_maps->opo_eqcmap))
	return(opt_mapfunc(subquery, cop->opo_inner, eqcls, func_eqcmap, temp_eqcmap));

    if (no_lower)
    {	/* the function attribute must be available at this node or
        ** not at all, find the set of equivalence classes which are
	** available at this node, and determine if it is a super set of the
        ** set of equivalence classes required to evaluate the function attribute */
	if (cop->opo_outer)
	{
	    if (cop->opo_inner)
	    {
		MEcopy ((PTR)&cop->opo_outer->opo_maps->opo_eqcmap, sizeof(OPE_BMEQCLS),
		    (PTR)temp_eqcmap);
		BTor((i4)BITS_IN(OPE_BMEQCLS), (PTR)&cop->opo_inner->opo_maps->opo_eqcmap,
		    (PTR)temp_eqcmap);
	    }
	    else
		temp_eqcmap = &cop->opo_outer->opo_maps->opo_eqcmap;
	}
	else
	{
	    if (cop->opo_inner)
		temp_eqcmap = &cop->opo_inner->opo_maps->opo_eqcmap;
	    else
	    {	/* this is an ORIG node in which the available equivalence
                ** classes are in the variable desciptor, the ORIG node
                ** has equilvalence classes in opo_maps->opo_eqcmap which are the
                ** same as the project-restrict */
		if (cop->opo_sjpr == DB_ORIG)
		    temp_eqcmap = &subquery->ops_vars.opv_base->
			opv_rt[cop->opo_union.opo_orig]->opv_maps.opo_eqcmap;
		else
		    temp_eqcmap = NULL;
	    }
	}
	return(BTsubset((char *)func_eqcmap, (char *)temp_eqcmap, 
	    BITS_IN(OPE_BMEQCLS)));
    }
    return(FALSE);
}

/*{
** Name: opt_init	- initialize the state variable for tree to text conversion
**
** Description:
**
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
[@PARAM_DESCR@]...
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      4-apr-88 (seputis)
**          initial creation
**	06-Jul-06 (kiria01) b116230
**	    psl_print becomes PSL_RS_PRINT bit in new field psl_rsflags
**	    to allow for extra flags in PST_RSDM_NODE
[@history_template@]...
*/
static VOID
opt_init(
	OPT_TTCB	   *handle,
	OPS_SUBQUERY       *subquery,
	OPO_CO             *cop,
	ULM_RCB		   *ulmrcb)
{
    handle->subquery = subquery;
    handle->cop = cop;
    handle->opt_ulmrcb = ulmrcb;
    handle->joinstate = OPT_JNOTUSED; /* remainder of qualifications are not
				    ** used */
    MEcopy((PTR)&subquery->ops_global->ops_mstate.ops_ulmrcb, sizeof(ULM_RCB),
	(PTR)ulmrcb);		    /* use copy of ULM_RCB in case
                                    ** it gets used by other parts
                                    ** of OPF */
    ulmrcb->ulm_streamid_p = &subquery->ops_global->ops_mstate.ops_streamid; /* allocate
				    ** memory from the main memory
                                    ** stream */
    MEfill(sizeof(handle->qnodes), (u_char)0, (PTR)&handle->qnodes);
    if (cop->opo_sjpr == DB_SJ)
    {	/* a join has equivalence classes from the left and right sides
        ** but the boolean factor for the join has been removed so it 
        ** needs be to created; use temporary space in the handle state
        ** control block to create each join, one conjunct at a time
        ** to be placed into the qualification */

	/* initialize PST_VAR node for the outer subtree */
	handle->qnodes.leftvar.pst_sym.pst_type = PST_VAR; /* create VAR node type */
	handle->qnodes.leftvar.pst_sym.pst_len = sizeof(PST_VAR_NODE);
	MEfill(sizeof(handle->qnodes.leftvar.pst_sym.pst_value.pst_s_var.pst_atname), 
	    ' ', 
	    (PTR)&handle->qnodes.leftvar.pst_sym.pst_value.pst_s_var.pst_atname);
				    /* init attribute name */
	/* initialize PST_VAR node for the inner subtree */
	handle->qnodes.rightvar.pst_sym.pst_type = PST_VAR; /* create VAR node type */
	handle->qnodes.rightvar.pst_sym.pst_len = sizeof(PST_VAR_NODE);
	MEfill(sizeof(handle->qnodes.rightvar.pst_sym.pst_value.pst_s_var.pst_atname), 
	    ' ', 
	    (PTR)&handle->qnodes.rightvar.pst_sym.pst_value.pst_s_var.pst_atname);
				    /* init attribute name */

	/* initialize an operator node for the join */
	handle->qnodes.eqnode.pst_left = &handle->qnodes.leftvar; /* initialize the temp
                                    ** query tree nodes so that a join
				    ** is specified */
	handle->qnodes.eqnode.pst_right = &handle->qnodes.rightvar;
	handle->qnodes.eqnode.pst_sym.pst_type = PST_BOP;
	handle->qnodes.eqnode.pst_sym.pst_len = sizeof(PST_OP_NODE);
	handle->qnodes.eqnode.pst_sym.pst_value.pst_s_op.pst_opno =
	    subquery->ops_global->ops_cb->ops_server->opg_eq;
	handle->qnodes.eqnode.pst_sym.pst_value.pst_s_op.pst_opinst =
	handle->qnodes.eqnode.pst_sym.pst_value.pst_s_op.pst_oplcnvrtid = 
	handle->qnodes.eqnode.pst_sym.pst_value.pst_s_op.pst_oprcnvrtid = ADI_NOFI;
	handle->qnodes.eqnode.pst_sym.pst_value.pst_s_op.pst_distinct = PST_DONTCARE;
	handle->qnodes.eqnode.pst_sym.pst_value.pst_s_op.pst_opmeta = PST_NOMETA;

	MEcopy((PTR)&cop->opo_outer->opo_maps->opo_eqcmap, sizeof(OPE_BMEQCLS),
	    (PTR)&handle->bmjoin);
	BTand((i4)(BITS_IN(OPE_BMEQCLS)), (char *)&cop->opo_inner->opo_maps->opo_eqcmap,
	    (char *)&handle->bmjoin);		/* find the possible joining
						** equivalence classes */
    }

    /* initialize PST_VAR node for the resdom list */
    handle->qnodes.resexpr.pst_sym.pst_type = PST_VAR; /* create VAR node type */
    handle->qnodes.resexpr.pst_sym.pst_len = sizeof(PST_VAR_NODE);
    MEfill(sizeof(handle->qnodes.resexpr.pst_sym.pst_value.pst_s_var.pst_atname), 
	' ', 
	(PTR)&handle->qnodes.resexpr.pst_sym.pst_value.pst_s_var.pst_atname);
				/* init attribute name */

    /* initialize an operator node for the join */
    handle->qnodes.resnode.pst_right = &handle->qnodes.resexpr; /* initialize the temp
				** query tree nodes so that a join
				** is specified */
    handle->qnodes.resnode.pst_sym.pst_type = PST_RESDOM;
    handle->qnodes.resnode.pst_sym.pst_len = sizeof(PST_RSDM_NODE);
    handle->qnodes.resnode.pst_sym.pst_value.pst_s_rsdm.pst_rsflags = PST_RS_PRINT;
    handle->qnodes.resnode.pst_sym.pst_value.pst_s_rsdm.pst_rsupdt = FALSE;
    handle->qnodes.resnode.pst_sym.pst_value.pst_s_rsdm.pst_rsno = 0;
    handle->reqcls = OPE_NOEQCLS; /* traverse the equivalence classes
				** which are retrieved and required
				** by the parent of this CO node */

    handle->jeqcls = OPE_NOEQCLS;
    handle->bfno = OPB_NOBF;
    handle->opt_vno = OPV_NOVAR;
    handle->jeqclsp = NULL;
}

/*{
** Name: opt_funcattr	- return text for function attribute
**
** Description:
**
** Inputs:
**	textcb                          state variable for tree to text 
**                                      conversion
**      varno                           range variable number of node
**      atno                            attribute number of node
**
** Outputs:
**      tablep                          ptr to table.attribute name
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      1-apr-88 (seputis)
**          initial creation
[@history_template@]...
*/
static ULD_TSTRING *
opt_funcattr(
	OPT_TTCB	    *textcbp,
	OPZ_IATTS	    attno,
	OPO_CO              *cop,
	ULM_SMARK	    *markp)
{
    OPS_SUBQUERY    *subquery;
    ULD_TSTRING	    *textp;
    OPZ_ATTS	    *attrp;
    OPZ_FATTS       *fattrp;	    /* ptr to func attribute being analyzed */
    OPE_BMEQCLS	    *func_eqcmap;   /* ptr to equivalence classes needed to
				    ** evaluate the function attribute */
    OPE_BMEQCLS     temp_eqcmap;    /* temporary bitmap used for evaluating
                                    ** whether function attribute can be
			            ** evaluated in this subtree */

    subquery = textcbp->subquery;
    textp = NULL;
    attrp = subquery->ops_attrs.opz_base->opz_attnums[attno];
    fattrp = subquery->ops_funcs.opz_fbase->opz_fatts[attrp->opz_func_att];
    func_eqcmap = &fattrp->opz_eqcm;

    if (opt_mapfunc(subquery, cop, attrp->opz_equcls, func_eqcmap, &temp_eqcmap))
    {	/* the CO subtree has enough equivalence classes
        ** to evaluate this function attribute */
	DB_STATUS	status;
	OPT_TTCB	temp_handle;
	DB_ERROR	error;
	ULM_RCB		ulmrcb;

	opu_mark(subquery->ops_global, NULL, markp);
	opt_init(&temp_handle, subquery, cop, &ulmrcb); /* initialize the temp
				    ** state variable so that the function
			            ** function attribute parse tree can be
			            ** processed out of line */
	status = uld_tree_to_text((PTR)&temp_handle,
	    (i4)PSQ_RETRIEVE, /* set up as a retrieve */
	    NULL, /* no target relation name */
	    subquery->ops_global->ops_adfcb, 
	    textcbp->opt_ulmrcb, 
	    OPT_MAXTEXT/* max number of characters in one string */, 
	    TRUE /* use pretty printing where possible */, 
	    fattrp->opz_function->pst_right, /* ptr to function attribute expression */ 
	    opt_varnode /* routine to print VAR NODE information */,
	    NULL /* tablename routine */,
	    NULL /* routine to get the next conjunct to attach */,
	    NULL /* no resdom list is needed */, 
	    &textp, &error,
	    subquery->ops_global->ops_adfcb->adf_qlang);
	if (DB_FAILURE_MACRO(status))
	    opx_verror(E_DB_SEVERE, E_OP068A_BAD_TREE, error.err_code);
    }
    return(textp);
}

/*{
** Name: opt_varnode	- return var node ID information
**
** Description:
**      This is a routine called by the query tree to text conversion 
**      routines used to abstract the range table information of the 
**      caller.  It will produce a string which represents the equivalence
**      class for the joinop attribute.
[@comment_line@]...
**
** Inputs:
**	textcb                          state variable for tree to text 
**                                      conversion
**      varno                           range variable number of node
**      atno                            attribute number of node
**
** Outputs:
**      tablep                          ptr to table.attribute name
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      1-apr-88 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opt_varnode(
	OPT_TTCB            *textcb,
	i4		    varno,
	i4		    atno,
	char		    **namepp)
{
    OPS_SUBQUERY    *subquery;
    OPZ_BMATTS	    *attrmap;	    /* bitmap of attributes in equivalence
				    ** class */
    i4		    totalsize;	    /* size of string used to contain name */
    i4		    twice;	    /* make 2 passes, once to calculate
				    ** length and once to move the data */
    OPT_NAME	    attrname;	    /* temporary in which to build
				    ** attribute name */
    OPT_NAME	    tablename;	    /* temporary in which to build the
                                    ** table name */
    i2		    attrlength;	    /* attribute name length */
    i2		    tablelength;    /* table name length */
    i4		    current;	    /* offset in result buffer of name */
    char	    *namep;	    /* ptr to base of result buffer */
    OPZ_AT	    *abase;
    OPV_BMVARS	    *bmvars;	    /* bitmap to determine attribute subset
				    ** of equivalence class to use */
    OPO_CO	    *func_cop;      /* this COP provides the scope in which the
                                    ** function attribute will be evaluated */
    subquery = textcb->subquery;
    abase = subquery->ops_attrs.opz_base;
    attrmap = &subquery->ops_eclass.ope_base->ope_eqclist[
	abase->opz_attnums[atno]->opz_equcls]->ope_attrmap;
    totalsize = 0;
    current = 0;
    switch (textcb->joinstate)
    {
    case OPT_JOUTER:		    /* consider only attributes from the outer
                                    ** for this variable in the join */
	func_cop = textcb->cop->opo_outer;
	bmvars = func_cop->opo_variant.opo_local->opo_bmvars;
	textcb->joinstate = OPT_JINNER; /* next attribute from inner */
	break;
    case OPT_JINNER:		    /* consider only attributes from the inner
                                    ** for this variable in the join */
	func_cop = textcb->cop->opo_inner;
	bmvars = func_cop->opo_variant.opo_local->opo_bmvars;
	textcb->joinstate = OPT_JNOTUSED; /* no next attribute */
	break;
    case OPT_JNOTUSED:
	func_cop = textcb->cop;
	bmvars = func_cop->opo_variant.opo_local->opo_bmvars;
	break;
    case OPT_JBAD:
    default:
	opx_error(E_OP068B_BAD_VAR_NODE);
    }
    for (twice = 2; twice>0; twice--)
    {	/* make 2 passes since this routine is not performance critical 
	** - the first pass calculates the size of the memory to allocate
        ** - the second pass allocates memory and fills it
        */
	bool	    first_time;
	i4	    attrcount;
	i4	    new_attno;

	attrcount = 0;
	first_time = TRUE;
	for (new_attno = -1;
	     (new_attno = BTnext((i4)new_attno, (char *)attrmap, (i4)OPZ_MAXATT)) >= 0;)
	{   /* look thru the list of joinop attribute associated with this
	    ** equivalence class and determine which ones exist in the
	    ** subtree associated with this CO node */
	    OPZ_ATTS		*attrp;
	    ULD_TSTRING		*func_list;
	    ULM_SMARK		mark;

	    attrp = abase->opz_attnums[new_attno];
	    func_list = NULL;

	    if ((   (attrp->opz_attnm.db_att_id != OPZ_FANUM)
		    &&
		    BTtest((i4)attrp->opz_varnm, (char *)bmvars) /* if this is a non function
						** attribute then the var needs to be available
                                                ** for this attribute to be included */
                )
                ||
	        (   (attrp->opz_attnm.db_att_id == OPZ_FANUM)
		    &&
		    (func_list = opt_funcattr(textcb, new_attno, func_cop, &mark)) /* return func_list
						** if enough equivalence classes exist 
                                                ** for evaluation of the function attribute */
                )
	       )
	    {   /* range variable for this attribute is in subtree */
		opt_ptname(subquery, attrp->opz_varnm, (OPT_NAME *)&tablename, FALSE); /* get the table name */
		opt_paname(subquery, new_attno, &attrname); /* get the attribute name */
		attrlength = STlength((char *)&attrname);
		tablelength= STlength((char *)&tablename);
		attrcount++;
		if (twice == 2)
		{   /* on the first pass just add up storage requirements */
		    if (first_time)
			first_time = FALSE;
		    else
			totalsize += 1;		/* separate names in same equivalence class
						** by commas */
		    if (func_list)
		    {	/* calculate length of text for function attribute
                        ** FIXME - use mark and release to restore memory */
			for (;func_list; func_list = func_list->uld_next)
			    totalsize += func_list->uld_length;
			opu_release(subquery->ops_global, NULL, &mark); /* release memory
						** used to compute text */
		    }
		    else
			totalsize += attrlength + 1 + tablelength; /* add one for the period
						** qualifier */
		}
		else
		{   /* second pass used allocated memory */
		    if (first_time)
			first_time = FALSE;
		    else
			namep[current++] = ',';	/* separate names in same equivalence class
						** by commas */
		    if (func_list)
		    {	/* move the function attribute text to the variable text descriptor
                        ** FIXME - memory lost - use mark and release to restore memory */
			for (;func_list; func_list = func_list->uld_next)
			{
			    MEcopy((PTR)&func_list->uld_text[0], func_list->uld_length,
				(PTR)&namep[current]);
			    current += func_list->uld_length;
			}
			opu_release(subquery->ops_global, NULL, &mark); /* release memory
						** used to compute text */
		    }
		    else
		    {
			MEcopy((PTR)&tablename, tablelength, (PTR)&namep[current]);
			current += tablelength;
			namep[current++] = '.';	    /* add period qualifier */
			MEcopy((PTR)&attrname, attrlength, (PTR)&namep[current]);
			current += attrlength;
		    }
		}
	    }
	}
	if (twice == 2)
	{   /* first time thru allocate temporary memory for the name */
	    if (attrcount > 1)
	    {	/* more that one attribute so add brackets */
		totalsize += 2;
	    }
	    namep = (char *) opu_tmemory(subquery->ops_global, 
		(i4)(totalsize+1)); /* add one for NULL */
	    if (attrcount > 1)
		namep[current++] = '{';
	}
	else
	{
	    if (attrcount > 1)
		namep[current++] = '}';		/* add closing bracket */
	    namep[current++] = 0;		/* null terminate */

	}
    }
    *namepp = namep;
}

/*{
** Name: opt_conjunct	- return the next conjunct to attach to the text
**
** Description:
**      This is a co-routine which will return the next conjunct to 
**      attach to a qualification text string being constructed
**
** Inputs:
**      handle                          state variable for text string
**                                      construction
**
** Outputs:
**      qtreepp                         return location for next conjunct
**	Returns:
**	    TRUE - if no more conjuncts exist to be added to the text
**	Exceptions:
**
**
** Side Effects:
**
**
** History:
**      4-apr-88 (seputis)
**          initial creation
[@history_line@]...
[@history_template@]...
*/
static bool
opt_conjunct(
	OPT_TTCB           *handle,
	PST_QNODE          **qtreepp)
{

    if ((handle->jeqcls >= 0)		/* if the last conjunct was a join */
	&&
	((handle->jeqcls = BTnext((i4)handle->jeqcls, 
				  (char *)&handle->bmjoin,
				  (i4)BITS_IN(handle->bmjoin)
                                 )	/* and there are still joins to be
					** processed */
          ) >= 0
        )
       )
    {
	opt_etoa(handle, handle->jeqcls); /* these are the joining clauses
					** which were not used */
	*qtreepp = &handle->qnodes.eqnode;
    }

    handle->joinstate = OPT_JNOTUSED; /* remainder of qualifications are not
				    ** used */
    if ((handle->bfno = BTnext((i4)handle->bfno, 
	    (char *)handle->cop->opo_variant.opo_local->opo_bmbf, 
	    (i4)OPB_MAXBF)
	 ) >= 0)
    {
	*qtreepp = handle->subquery->ops_bfs.opb_base->opb_boolfact[handle->bfno]->opb_qnode;
					/* ptr to boolean factor element
					** next expression to obtain convert */
	return(FALSE);			/* end of list not reached */
    }
    else
	return(TRUE);			/* end of conjunct list is reached */
}

/*{
** Name: opt_jconjunct	- return the next JOIN conjunct to attach to the text
**
** Description:
**      This is a co-routine which will return the next JOIN conjunct to 
**      attach to a qualification text string being constructed.  Conjuncts
**      in this list are special since the participate in the join operation
**      in some way, as opposed to being evaluated a qualification.
**
** Inputs:
**      handle                          state variable for text string
**                                      construction
**
** Outputs:
**      qtreepp                         return location for next conjunct
**	Returns:
**	    TRUE - if no more conjuncts exist to be added to the text
**	Exceptions:
**
**
** Side Effects:
**
**
** History:
**      4-apr-88 (seputis)
**          initial creation
[@history_line@]...
[@history_template@]...
*/
static bool
opt_jconjunct(
	OPT_TTCB           *handle,
	PST_QNODE          **qtreepp)
{
    if (handle->jeqclsp
	&&
	((*handle->jeqclsp) != OPE_NOEQCLS)
       )
    {
	opt_etoa(handle, *handle->jeqclsp); /* get the next EQCLS participating in the 
					    ** multi attribute join */
	++handle->jeqclsp;		/* point to next equivalence class in list */
	*qtreepp = &handle->qnodes.eqnode; /* ptr to join element
					** to convert */
	return(FALSE);			/* end of list not reached */
    }
    else
	return(TRUE);			/* end of conjunct list is reached */
}

/*{
** Name: opt_tablename	- return table name
**
** Description:
**      This routine is used to return a table name and an alias to the
**      text printing routines.  The alias is only required if there are
**      multiple references to the same range variable. 
[@comment_line@]...
**
** Inputs:
**      global                          state variable for string to tree
**					to text conversion
**      namepp                          ptr to descriptor containing
**                                      the name only
**      aliaspp                         ptr to range name only needed if
**                                      same table referenced twice in the
**                                      same query, otherwise NULL
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
[@history_template@]...
*/
static bool
opt_tablename(
	OPT_TTCB           *global,
	ULD_TSTRING        **namepp,
	char               **aliaspp)
{
    *aliaspp = NULL;
    *namepp = &global->opt_tdesc;
    global->opt_tdesc.uld_next = NULL;	/* ptr to next element or NULL */
    global->opt_vno = BTnext((i4)global->opt_vno,
	(char *)global->cop->opo_variant.opo_local->opo_bmvars,
	global->subquery->ops_vars.opv_rv); /* get next variable to consider */
    if (global->opt_vno >= 0)
    {
	opt_ptname(global->subquery, (OPV_IVARS)global->opt_vno, 
	    (OPT_NAME *)&global->opt_tdesc.uld_text[0], FALSE);
					/* get the table name */
	global->opt_tdesc.uld_length
	    = STlength((char *)&global->opt_tdesc.uld_text[0]);
	return (FALSE);			/* end of list not reached */
    }
    else
	return(TRUE);			/* end of list is reached */
}

/*{
** Name: opt_resdom	- return next resdom to include in list
**
** Description:
**      This is a co-routine used to construct the text for the 
**      resdom target list.
**
** Inputs:
**      handle                          ptr to text state variable
**      resdomp                         ptr to resdom node from which
**                                      a name is required
**                                      NULL if the next resdom is
**                                      required
**
** Outputs:
**      namep                           return name of resdom
**	Returns:
**	    ptr to resdom to analyze, or NULL if no more resdoms exist
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      13-apr-88 (seputis)
**          initial creation
[@history_line@]...
[@history_template@]...
*/
static PST_QNODE *
opt_resdom(
	OPT_TTCB           *handle,
	PST_QNODE          *resdomp,
	char               **namep)
{
    OPS_SUBQUERY	*subquery;

    subquery = handle->subquery;
    /* only simple equivalence classes are needed for interior nodes */

    if ((handle->reqcls = BTnext((i4)handle->reqcls, 
	    (char *)&handle->cop->opo_maps->opo_eqcmap, 
	    (i4)OPE_MAXEQCLS  )
	) >= 0)
    {
	OPZ_IATTS	new_attr;
	OPZ_ATTS	*attrp;

	/* name all attribute of temporaries by using the "e" followed
        ** by the equivalence class number, since at any node this
        ** will represent a unique attribute */

	handle->qnodes.resnode.pst_sym.pst_value.pst_s_rsdm.pst_rsname[0]
	    = 'e';		/* all resdom names begin
				** with an 'e' */
	CVla((i4)handle->reqcls, (char *)&handle->qnodes.resnode.pst_sym.
	    pst_value.pst_s_rsdm.pst_rsname[1]);
	*namep = &handle->qnodes.resnode.pst_sym.
	    pst_value.pst_s_rsdm.pst_rsname[0]; /* return ptr to name
				** of resdom */
	handle->qnodes.resnode.pst_sym.pst_value.pst_s_rsdm.pst_rsno 
	    = handle->reqcls;

	new_attr = 
	handle->qnodes.resexpr.pst_sym.pst_value.pst_s_var.pst_atno.db_att_id =
	    BTnext((i4)-1, (char *)&subquery->ops_eclass.
	    ope_base->ope_eqclist[handle->reqcls]->ope_attrmap,
	    (i4)OPZ_MAXATT);
	attrp = subquery->ops_attrs.opz_base->opz_attnums[new_attr];
	STRUCT_ASSIGN_MACRO(attrp->opz_dataval, 
	    handle->qnodes.resexpr.pst_sym.pst_dataval);
	STRUCT_ASSIGN_MACRO(attrp->opz_dataval, 
	    handle->qnodes.resnode.pst_sym.pst_dataval);
	handle->qnodes.resexpr.pst_sym.pst_value.pst_s_var.pst_vno 
	    = attrp->opz_varnm;
	resdomp = &handle->qnodes.resnode;
    }
    else
	resdomp = NULL;		/* return indicator that the end of the
				** list has been found */
    return(resdomp);
}

/*{
** Name: opt_pbfs	- print boolean factor text for node
**
** Description:
**      This routine will print the text for the expressions which will be
**      executed at this node
**
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
[@PARAM_DESCR@]...
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      4-apr-88 (seputis)
**          initial creation
[@history_template@]...
*/
VOID
opt_treetext(
	OPS_SUBQUERY       *subquery,
	OPO_CO             *cop)
{
    OPT_TTCB	    handle;
    OPS_STATE	    *global;
    ULD_TSTRING	    *tstring;
    ULM_RCB	    ulmrcb;
    OPB_BMBF	    *bmbfp;
    OPE_IEQCLS	    maxeqcls;
    DB_STATUS	    status;
    DB_ERROR	    error;
    PST_QNODE	    *qual;	/* first qualification to be printed */


    if (!cop->opo_variant.opo_local)
	return;
    bmbfp = cop->opo_variant.opo_local->opo_bmbf;
    if (!bmbfp)
	return;

    opt_init(&handle, subquery, cop, &ulmrcb);	/* initialize the state variable
				    ** used to control the tree to text conversion
				    */

    tstring = NULL;
    maxeqcls = subquery->ops_eclass.ope_ev;
    global = subquery->ops_global;
    {   /* print the query without the qualification and print the
	** resdom list given the attributes which are required by
	** the parent */
	PST_QNODE	    *saveleft;
	PST_QNODE	    *saveright;
	i4		    mode;
	saveleft = subquery->ops_root->pst_left;
	subquery->ops_root->pst_left = NULL;
	saveright = subquery->ops_root->pst_right;
	subquery->ops_root->pst_right = NULL; 
	handle.opt_vno = OPV_NOVAR;

	if (subquery->ops_bestco == cop)
	    mode = subquery->ops_mode;
	else
	    mode = PSQ_RETRIEVE;

	status = uld_tree_to_text((PTR)&handle, 
	    mode, /* query mode */
	    NULL, /* no target relation name */
	    global->ops_adfcb, &ulmrcb, 
	    OPT_MAXTEXT /* max number of characters in one string */, 
	    TRUE /* use pretty printing where possible */, 
	    subquery->ops_root /* query tree root */, 
	    opt_varnode /* routine to print VAR NODE information */,
	    opt_tablename /* tablename routine */,
	    NULL /* routine to get the next conjunct to attach */, 
	    opt_resdom /* no resdom list is needed */, 
	    &tstring, &error,
	    global->ops_adfcb->adf_qlang);

	subquery->ops_root->pst_right = saveright ;	/* restore qualification */
	subquery->ops_root->pst_left = saveleft ; /* restore resdom list */
    }
    if (DB_FAILURE_MACRO(status))
	opx_verror(E_DB_SEVERE, E_OP068A_BAD_TREE, error.err_code);
    if (tstring)
	opt_dstring(subquery, "\n    ", tstring, OPT_MAXTEXT);

    tstring = NULL;
    if (cop->opo_sjpr == DB_SJ)
    {	/* a join has equivalence classes from the left and right sides
        ** but the boolean factor for the join has been removed so it 
        ** needs be to created; use temporary space in the handle state
        ** control block to create each join, one conjunct at a time
        ** to be placed into the qualification */

	if (cop->opo_sjeqc >= 0)
	{   /* a joining equivalence class exists, print out the part of the
            ** qualification which deals with a join */
	    if (cop->opo_sjeqc < maxeqcls)
	    {	/* a single equivalence class is found */
		opt_etoa(&handle, cop->opo_sjeqc);
		handle.jeqclsp = NULL;
	    }
	    else
	    {	/* a multi-attribute join is specified */
		handle.jeqclsp = &subquery->ops_msort.opo_base->
		    opo_stable[cop->opo_sjeqc - maxeqcls]
		    ->opo_eqlist->opo_eqorder[0];
		opt_etoa(&handle, *handle.jeqclsp);/* at this point only exact eqcls
						    ** list of <= 2 elements is expected */
		++handle.jeqclsp;		/* update to point to next eqcls to be
						** printed */
	    }

	    tstring = NULL;

	    status = uld_tree_to_text((PTR)&handle,
		(i4)PSQ_RETRIEVE, /* set up as a retrieve */
		NULL, /* no target relation name */
		global->ops_adfcb, &ulmrcb, 
		OPT_MAXTEXT /* max number of characters in one string */, 
		TRUE /* use pretty printing where possible */, 
		&handle.qnodes.eqnode, /* ptr to join element */ 
		opt_varnode /* routine to print VAR NODE information */,
		NULL /* tablename routine */,
		opt_jconjunct /* routine to get the next conjunct to attach */, 
		NULL /* no resdom list is needed */, 
		&tstring, &error,
		global->ops_adfcb->adf_qlang);
	    if (DB_FAILURE_MACRO(status))
		opx_verror(E_DB_SEVERE, E_OP068A_BAD_TREE, error.err_code);
	    if (tstring)
		opt_dstring(subquery, "\n    join on ", tstring, OPT_MAXTEXT);
	}
	handle.jeqcls = BTnext((i4)-1, (char *)&handle.bmjoin, (i4)OPE_MAXEQCLS);
	if (handle.jeqcls >= 0)
	{
	    opt_etoa(&handle, handle.jeqcls);	    /* these are the joining clauses
						    ** which were not used */
	    qual = &handle.qnodes.eqnode;
	}
	else
	    qual = NULL;
    }
    else
    {
	handle.jeqcls = OPE_NOEQCLS;
	qual = NULL;
    }

    handle.bfno = OPB_NOBF;
    if (qual
	||
	(   ((handle.bfno = BTnext((i4)-1, (char *)bmbfp, (i4)OPB_MAXBF)) >= 0)
	    &&
	    (qual = subquery->ops_bfs.opb_base->opb_boolfact[handle.bfno]->opb_qnode)
	)
       )
    {
	tstring = NULL;
	status = uld_tree_to_text((PTR)&handle, 
	    (i4)PSQ_RETRIEVE, /* set up as a retrieve */
	    NULL, /* no target relation name */
	    global->ops_adfcb, &ulmrcb, 
	    OPT_MAXTEXT /* max number of characters in one string */, 
	    TRUE /* use pretty printing where possible */, 
	    qual, /* ptr to boolean factor element or join element */ 
	    opt_varnode /* routine to print VAR NODE information */,
	    NULL /* tablename routine */,
	    opt_conjunct /* routine to get the next conjunct to attach */, 
	    NULL /* no resdom list is needed */, 
	    &tstring, &error,
	    global->ops_adfcb->adf_qlang);
	if (DB_FAILURE_MACRO(status))
	    opx_verror(E_DB_SEVERE, E_OP068A_BAD_TREE, error.err_code);
	if (tstring)
	    opt_dstring(subquery, "\n    where ", tstring, OPT_MAXTEXT);
    }
}
