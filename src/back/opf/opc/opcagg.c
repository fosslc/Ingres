/*
**Copyright (c) 2004 Ingres Corporation
*/

/*
NO_OPTIM = rs4_us5
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <ulm.h>
#include    <ulf.h>
#include    <adf.h>
#include    <ade.h>
#include    <adfops.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <scf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <qefnode.h>
#include    <qefact.h>
#include    <qefqp.h>
#include    <qefqeu.h>
#include    <qefmain.h>
#include    <me.h>
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
#include    <opckey.h>
#include    <opcd.h>
#define        OPC_AGG		TRUE
#include    <opxlint.h>
#include    <opclint.h>
/**
**
**  Name: OPCAGG.C - Do the compilation for simple and aggregate functions.
**
**  Description:
**      This file holds routines that compile the instructions needed to 
**      evaluate simple and function aggs, remote or not, and do projection 
**      processing for aggs. 
[@comment_line@]...
**
**	External Functions:
**          opc_agtarget() - Do the target list processing for an agg.
**          opc_agproject() - Do the projection processing for an agg func
[@func_list@]...
**
**	Internal Functions:
**          opc_baggtarg_begin() - Begin agg target list compilation
**          opc_iaggtarg_init() - Compile the init segment for an agg.
**          opc_maggtarg_main() - Compile the main segment for an agg.
**          opc_faggtarg_finit() - Compile the finit segment for an agg.
**          opc_bycompare() - Compile code to compare byvals.
[@func_list@]...
**
**
**  History:
**      3-oct-86 (eric)
**          written
**      13-dec-88 (seputis)
**          made lint changes
**	17-apr-89 (jrb)
**	    Properly initialized prec field(s).
**	15-dec-89 (jrb)
**	    Added DB_ALL_TYPE support for outer join project.
**	16-jan-90 (stec)
**	    Minor cleanup in the include section of the module.
**      18-mar-92 (anitap)
**          Added opr_prec and opr_len initializatons to fix bug 42363.
**          Added in routine opc_maggtarg_main(). See opc_cqual() for details. 
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**      10-nov-95 (inkdo01)
**	    Changes to support replacement of QEN_ADF instances by pointers.
**      21-jan-1999 (hanch04)
**          replace nat and longnat with i4
**	20-dec-00 (inkdo01)
**	    Changes to generate inline ADF code for aggregates & get rid of 
**	    Aggregate workspace structure.
**	23-jan-01 (inkdo01)
**	    Changes to generate code for hash aggregation.
**	6-feb-01 (inkdo01)
**	    (Hopefully last) changes for hash aggregation overflow handling.
**	25-jun-2001 (somsa01 for inkdo01)
**	    In opc_iaggtarg_init(), when materializing the expression for
**	    each byval, make sure we tell opc_adinstr() to apply the proper
**	    null semantics to the hash value when not performing a top sort.
**	28-sep-2001 (devjo01)
**	    On 64 bit platforms, logic used to calculate total key length
**	    of a consolidated set of all character hash keys, would overstate
**	    the key length by four bytes, leading to incorrect query results.
**	    b105914.
**      29-Nov-2001 (hweho01)
**          Turned off optimization for rs4_us5 platform to eliminate errors
**          on the aggregates select. Symptom: incorrect results returned
**          in be/qp12.sep , be/udt09.sep, be/dtw06.sep tests.
**          Compiler: C for AIX Compiler V.5.0.1.2.
**	06-Jul-06 (kiria01) b116230
**	    psl_print becomes PSL_RS_PRINT bit in new field psl_rsflags
**	    to allow for extra flags in PST_RSDM_NODE
**	17-Aug-2006 (kibro01) Bug 116481
**	    Mark that opc_adtransform is not being called from pqmat
**	7-nov-2006 (dougi)
**	    Must add support for collation IDs to handle case insensitive
**	    grouping, max, min.
**	27-Feb-2009 (kiria01) b121720
**	    Supplement the various switches related to alignment of CX
**	    data with the missing datatypes that have alignment needs.
**	    Notably this should equal or exceed the needs of the ADF
**	    function ad0_is_unaligned().
**	2-Jun-2009 (kschendel) b122118
**	    Delete obsolete adbase parameters.
**	27-Jul-2009 (kschendel) b122304
**	    32-bit build test caught the fact that the offset initialization
**	    for byvals assumed the original definition of a QEF_HASH_DUP,
**	    ie a pointer plus an i4, but never actually used the symbol,
**	    except in iaggtarg-init.  It just happened to work on LP64.
**	    (and in the mainline, before my qef-hash-dup additions.)
**	    Fix things so that byvals start at the proper hash-keys offset
**	    for hash agg, and at zero for sorted aggs.
**/

/*
**  Forward and/or External function references.
*/
static VOID
opc_bycompare(
	OPS_STATE	*global,
	OPC_ADF		*cadf,
	PST_QNODE	*byvals,
	RDR_INFO	*rel,
	i4		pout_base,
	i4		agg_base,
	i4		proj_src_base );

static VOID
opc_baggtarg_begin(
	OPS_STATE   *global,
	OPC_NODE    *cnode,
	PST_QNODE   *aghead,
	QEN_ADF	    **qadf,
	QEN_ADF	    **hqadf,
	QEN_ADF	    **oqadf,
	OPC_ADF	    *cadf,
	OPC_ADF	    *hcadf,
	OPC_ADF	    *ocadf,
	OPC_TGINFO  *tginfo,
	i4	    *topsort,
	QEN_NODE    **qn,
	OPC_TGINFO  **sortinfo,
	PST_QNODE   **byvals,
	i4	    *workbase,
	i4	    *hashbase,
	i4	    *outbase,
	i4	    *oflwbase, 
	i4	    *oflwwork, 
	i4	    *bycount,
	PST_QNODE   **subexlistp);

static VOID
opc_iaggtarg_init(
	OPS_STATE   *global,
	OPC_NODE    *cnode,
	PST_QNODE   *aghead,
	OPC_ADF	    *cadf,
	OPC_ADF	    *hcadf,
	OPC_ADF	    *ocadf,
	OPC_TGINFO  *tginfo,
	i4	    topsort,
	PST_QNODE   *byvals,
	OPC_TGINFO  *sortinfo,
	i4	    workbase,
	i4	    hashbase,
	i4	    outbase,
	i4	    oflwbase, 
	QEN_NODE    *qn,
	DB_CMP_LIST *hashkeys,
	PST_QNODE   *subexlistp);

static VOID
opc_maggtarg_main(
	OPS_STATE   *global,
	OPC_NODE    *cnode,
	PST_QNODE   *aghead,
	OPC_ADF	    *cadf,
	OPC_ADF	    *ocadf,
	OPC_TGINFO  *tginfo,
	i4	    topsort,
	PST_QNODE   *byvals,
	OPC_TGINFO  *sortinfo,
	i4	    workbase,
	i4	    outbase,
	i4	    oflwbase, 
	QEN_NODE    *qn,
	PST_QNODE   *subexlistp);

static VOID
opc_faggtarg_finit(
	OPS_STATE   *global,
	QEF_QEP	    *ahd_qepp,
	PST_QNODE   *aghead,
	OPC_ADF	    *cadf,
	OPC_TGINFO  *tginfo,
	PST_QNODE   *byvals,
	i4	    workbase,
	i4	    outbase);



/*
**  Defines of constants used in this file
*/
#define    OPC_UNKNOWN_LEN    0
#define    OPC_UNKNOWN_PREC   0


/*{
** Name: OPC_AGTARGET	- Do the target list processing for an aggregate.
**
** Description:
**      This routine compiles the target list for an aggregate action,
**      simple or function, remote or not, into the ahd_current CX. 
**      This file does not handle the projection processing for the
**      aggregate however.  
**      
**      This routine is split into four phases: The beginning phase, 
**      the init phase, the main phase, and the finit phase. The beginning 
**      phase sets up some variables that describes the aggregate and 
**      begins the compilation of the CX.  
**        
**      The init phase compiles that appropriate instructions into the 
**      init segment of our CX. These instructions begin the aggregate 
**      processing for each agg in question and, if this is an agg function,
**      materialize the next set of by values from the QEN tree.
**        
**      By this time, you can probably guess that the phase compiles  
**      instructions into the main segment of our CX. These instructions 
**      do two things. First, if this is an agg function, then we compare 
**      the current set of by values (the ones that we just got from the 
**      QEN tree) with the ones that we materialized in the init segment. 
**      The second step, which is executed only if we passed the first step, 
**      is to compile the aggregate operator using the current set of  
**      values from the QEN tree.
**        
**      During the finit phase, we compile agend instructions into the
**      finit segment of our CX. 
**        
**      For information on how these three segments are used, please refer 
**      to the QEF documentation. 
[@comment_line@]...
**
** Inputs:
**  global -
**	This is the query-wide state variable. It is used mostly to find
**	the current subquery struct, repeat query parameter info, and
**	as a parameter to all OPC routines.
**  cnode -
**	This holds information about the state of compilation of this
**	action header/subquery structs QEN tree. Among other things,
**	this describes where the data can be found that the QEN tree
**	returns.
**  aghead -
**	This is the AGHEAD node from the query tree that we are compiling.
**  ahd_qepp -
**	This is a pointer to the action specific structure for aggregates.
**	It contains ptrs to the ahd_current and ahd_bycomp ADF expressions
**	and several other fields init'ed for aggregates and hash aggs.
**
** Outputs:
**  qadf -
**	This will be completely initialized; ready for QEF execution.
**
**	Returns:
**	    nothing
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      3-oct-86 (eric)
**          written
**      10-nov-95 (inkdo01)
**	    Changes to support replacement of QEN_ADF instances by pointers.
**	20-dec-00 (inkdo01)
**	    Changes to generate inline ADF code for aggregates & get rid of 
**	    Aggregate workspace structure.
**	23-jan-01 (inkdo01)
**	    Changes to generate code for hash aggregation.
**	6-feb-01 (inkdo01)
**	    Changes for hash aggregation overflow handling.
**	21-apr-01 (inkdo01)
**	    Add overflow ADF code work buffer to action header.
[@history_template@]...
*/
VOID
opc_agtarget(
		OPS_STATE   *global,
		OPC_NODE    *cnode,
		PST_QNODE   *aghead,
		QEF_QEP	    *ahd_qepp)
{
    i4			outbase;
    i4			workbase;
    i4			hashbase;
    i4			oflwbase;
    i4			oflwwork;
    i4			bycount;
    PST_QNODE		*byvals;
    i4			agstruct_base;
    OPC_ADF		cadf;
    OPC_ADF		hcadf;
    OPC_ADF		ocadf;
    QEN_ADF	        **qadf = &ahd_qepp->ahd_current;
    QEN_ADF	        **hqadf = &ahd_qepp->ahd_by_comp;
    QEN_ADF	        **oqadf = &ahd_qepp->u1.s2.ahd_agoflow;
    PST_QNODE		*subexlistp = NULL;
    i4			topsort;
    QEN_NODE		*qn;
    OPC_TGINFO		tginfo;
    OPC_TGINFO		*sortinfo;

    /* if there is nothing to compile, then return */
    if (aghead == NULL)
    {
	(*qadf) = (QEN_ADF *)NULL;
	return;
    }

    /* begin the aggregate target list compilation */
    opc_baggtarg_begin(global, cnode, aghead, qadf, hqadf, oqadf, &cadf, &hcadf,
		&ocadf, &tginfo, &topsort, &qn, &sortinfo, &byvals, &workbase, 
		&hashbase, &outbase, &oflwbase, &oflwwork, &bycount,
		&subexlistp);

    /* Save stuff into QEF_QEP. */
    ahd_qepp->u1.s2.ahd_agwbuf = workbase;
    ahd_qepp->u1.s2.ahd_aghbuf = hashbase;
    ahd_qepp->u1.s2.ahd_agobuf = oflwbase;
    ahd_qepp->u1.s2.ahd_agowbuf = oflwwork;
    ahd_qepp->u1.s2.ahd_bycount = bycount;
    if (global->ops_cstate.opc_subqry->ops_sqtype == OPS_HFAGG)
    {
	/* Allocate a DB_CMP_LIST entry for the concatenated by-list
	** values.  We just need one since we can treat them as one
	** giant BYTE(n) value.
	*/
	ahd_qepp->u1.s2.ahd_aghcmp = (DB_CMP_LIST *) opu_qsfmem(global,
			sizeof (DB_CMP_LIST));
    }
    else ahd_qepp->u1.s2.ahd_aghcmp = (DB_CMP_LIST *) NULL;

    /* compile the init segment. */
    opc_iaggtarg_init(global, cnode, aghead, &cadf, &hcadf, &ocadf, &tginfo, 
		topsort, byvals, sortinfo, workbase, hashbase, outbase, 
		oflwbase, qn, ahd_qepp->u1.s2.ahd_aghcmp,
		subexlistp);


    /* compile the main segment */
    opc_maggtarg_main(global, cnode, aghead, &cadf, &ocadf, &tginfo, topsort, 
		byvals, sortinfo, workbase, outbase, oflwbase, qn, subexlistp);


    /* compile the finit segment */
    opc_faggtarg_finit(global, ahd_qepp, aghead, &cadf, &tginfo, byvals, 
				  workbase, outbase);
}

/*{
** Name: OPC_BAGGTARG_BEGIN	- Begin aggregate target list compilation
**
** Description:
**      This routine initializes various variables that describes the agg 
**      (or aggs) that we are compiling, and we begin the compilation of 
**      our CX. 
[@comment_line@]...
**
** Inputs:
**	global -
**	    query wide info.
**	cnode -
**	    This is used to locate the QEN tree that we just compiled,
**	    and to find it's results.
**	aghead -
**	    The AGHEAD node for our agg.
**	qadf -
**	    QEN_ADF structure ptr for agg computation CX
**	cadf -
**	    OPC_ADF structure ptr for agg computation CX
**	hqadf -
**	    QEN_ADF structure ptr for hashno computation CX (hash agg)
**	hcadf -
**	    OPC_ADF structure ptr for hashno computation CX (hash agg)
**	oqadf -
**	    QEN_ADF structure ptr for overflow row CX (hash agg)
**	ocadf -
**	    OPC_ADF structure ptr for overflow row CX (hash agg)
**	tginfo -
**	topsort -
**	qn -
**	sortinfo -
**	byvals -
**	workbase -
**	outbase -
**	    These are all pointers to uninitialized memory so sufficient size.
**	subexlistpp - 
**	    Ptr to ptr to list of common subexpressions extracted for 
**	    preevaluation.
**
** Outputs:
**	qadf -
**	    This will be initialized by ADF.
**	cadf -
**	    This will be initialized to begin compilation.
**	tginfo -
**	    This will describe the result of this target list
**	topsort -
**	    This will be TRUE if there is a sort node on top of the QEN
**	    tree. Otherwise it will be FALSE.
**	qn -
**	    This is the top QEN node if it is a top sort node.
**	sortinfo -
**	    This describes the data that we will get from the sort node
**	    if topsort is TRUE. Otherwise, this will be NULL.
**	byvals -
**	    This points to the RESDOMS for the by vals if this is an
**	    aggregate function.
**	workbase -
**	    This is the workarea base number for the CX.
**	outbase -
**	    This is the output base number for the CX.
**
**	Returns:
**	    nothing
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-AUG-87 (eric)
**          created
**      19-jun-95 (inkdo01)
**          changed ahd_any from 2 valued i4  to 32-bit ahd_qepflag
**      10-nov-95 (inkdo01)
**	    Changes to support replacement of QEN_ADF instances by pointers.
**	20-dec-00 (inkdo01)
**	    Changes to generate inline ADF code for aggregates & get rid of 
**	    Aggregate workspace structure.
**	23-jan-01 (inkdo01)
**	    Changes to generate code for hash aggregation.
**	6-feb-01 (inkdo01)
**	    Changes for hash aggregation overflow handling.
**	30-mar-01 (inkdo01)
**	    Changes to make stupid Quel simple aggs with no input rows work.
**	21-apr-01 (inkdo01)
**	    Add overflow ADF code work buffer to action header.
**	30-may-01 (inkdo01)
**	    Changes to roll f4 sum's into a f8.
**	21-june-01 (inkdo01)
**	    Changes to get binary aggregates working.
**	10-aug-01 (inkdo01)
**	    Slight fix to 104820 to account for f8 workfield in agg buffer
**	    when doing sum(f4).
**	17-sep-01 (inkdo01)
**	    Fix SEGV in binary aggregate support.
**	6-feb-02 (inkdo01)
**	    Fix alignment for Unicode types (must be 2 byte aligned).
**	17-oct-02 (toumi01) bug 108555 
**	    Fix alignment for aggregate varchars (must be 2 byte aligned).
**	24-apr-03 (inkdo01)
**	    Fix alignment for text types (grrr) (must be 2 byte aligned).
**	4-july-05 (inkdo01)
**	    CX optimization (extract common subexpressions).
**	9-jan-06 (dougi)
**	    Tiny change to make counts for avg be i8.
**	2-Nov-2006 (kschendel)
**	    Topsort below hashagg has to set topsort result row into
**	    hashing CADF as well as the main agg CADF.  (Topsort qen-row
**	    is accessed by both cx's.)
**	14-Mar-2007 (kschendel) SIR 122513
**	    Set partition-separate agg flag in action header if needed.
**      11-jun-2008 (huazh01)
**          Add alignment code for object_key and table_key types.
**          bug 120492.
**	7-Jul-2008 (kschendel) SIR 122512
**	    Enforce round-up of hash agg overflow size to multiples of
**	    QEF_HASH_DUP_ALIGN (probably 8).
**	18-Mar-2010 (kiria01) b123438
**	    Added SINGLETON aggregate for scalar sub-query support.
[@history_template@]...
*/
static VOID
opc_baggtarg_begin(
	OPS_STATE   *global,
	OPC_NODE    *cnode,
	PST_QNODE   *aghead,
	QEN_ADF	    **qadf,
	QEN_ADF	    **hqadf,
	QEN_ADF	    **oqadf,
	OPC_ADF	    *cadf,
	OPC_ADF	    *hcadf,
	OPC_ADF	    *ocadf,
	OPC_TGINFO  *tginfo,
	i4	    *topsort,
	QEN_NODE    **qn,
	OPC_TGINFO  **sortinfo,
	PST_QNODE   **byvals,
	i4	    *workbase, 
	i4	    *hashbase, 
	i4	    *outbase,
	i4	    *oflwbase,
	i4	    *oflwwork,
	i4	    *bycount,
	PST_QNODE   **subexlistpp )
{
    OPS_SUBQUERY	*subqry = global->ops_cstate.opc_subqry;
    QEF_AHD		*ahd = subqry->ops_compile.opc_ahd;
    i4			naops;
    PST_QNODE		*aop_resdom;
    PST_QNODE		*aop, *aopparm, *bynode;
    i4			ninstr, nops, nconst;
    i4		szconst;
    i4			max_base;
    i4			outsize, oflwsize;
    i4			worksize, hashsize;
    i4			outrow;
    DB_DT_ID		resdt;
    i4			aggbufsz, worklen, align, agwslen;
    i4			rsno;
    bool		quelsagg = FALSE;
    bool		sumf4;
    bool		binagg;

    if (subqry->ops_mask & OPS_PCAGG)
	ahd->qhd_obj.qhd_qep.ahd_qepflag |= AHD_PART_SEPARATE;

    opc_tloffsets(global, aghead->pst_left, tginfo, FALSE);

    /* initialize some variables that will keep line length down */
    if (cnode != NULL && cnode->opc_qennode != NULL && 
			cnode->opc_qennode->qen_type == QE_TSORT
	)
    {
	*topsort = TRUE;
	*qn = cnode->opc_qennode;
	*sortinfo = &subqry->ops_compile.opc_sortinfo;
    }
    else
    {
	*topsort = FALSE;
	*qn = NULL;
	*sortinfo = NULL;
    }

    if (subqry->ops_sqtype == OPS_FAGG ||
	    subqry->ops_sqtype == OPS_RFAGG ||
	    subqry->ops_sqtype == OPS_HFAGG
	)
    {
	*byvals = subqry->ops_byexpr;

	/* Stop the expression list before the BY columns - we're not
	** optimizing the BY list. */
	for (aop_resdom = aghead->pst_left;
	    aop_resdom != NULL && aop_resdom->pst_left != *byvals && 
				aop_resdom->pst_sym.pst_type != PST_TREE;
	    aop_resdom = aop_resdom->pst_left);
	aop_resdom->pst_left = NULL;
    }
    else
    {
	*byvals = NULL;
	if (aghead->pst_sym.pst_value.pst_s_root.pst_qlang == DB_QUEL)
	    quelsagg = TRUE;
    }

    /* Locate common subexpressions in aggregate list for preevaluation
    ** and replace in original expressions by PST_VARs. */
    opc_expropt(subqry, cnode, &aghead->pst_left, subexlistpp);
    if ((*byvals))
	aop_resdom->pst_left = *byvals;	/* reset expression list w. 
					** BY list */

    /* Determine room for byvals (and hash result?) in workspace buffer. */
    *bycount = 0;
    if (*byvals == NULL) aggbufsz = 0;
    else
    {
	if (subqry->ops_sqtype == OPS_HFAGG)
	{
	    /* Hash agg starts keys (byvals) after hash row header; this
	    ** is guaranteed to be worst-case aligned.
	    */
	    aggbufsz = QEF_HASH_DUP_SIZE;
	}
	else
	{
	    aggbufsz = 0;		/* Sorted agg starts byvals at zero */
	}

        for (bynode = *byvals; bynode && bynode->pst_sym.pst_type != PST_TREE;
						bynode = bynode->pst_left)
	{
	    /* Compute alignment factor for bylist entry. */
	    worklen = 1;
	    switch (bynode->pst_sym.pst_dataval.db_datatype) {
	      case DB_INT_TYPE:
	      case DB_FLT_TYPE:
	      case DB_MNY_TYPE:
              case DB_TABKEY_TYPE:
              case DB_LOGKEY_TYPE:
		worklen = bynode->pst_sym.pst_dataval.db_length;
		break;

	      case DB_NCHR_TYPE:
	      case DB_NVCHR_TYPE:
	      case -DB_NCHR_TYPE:
	      case -DB_NVCHR_TYPE:
		worklen = sizeof(UCS2);
		break;

	      case -DB_INT_TYPE:
	      case -DB_FLT_TYPE:
	      case -DB_MNY_TYPE:
              case -DB_TABKEY_TYPE:
              case -DB_LOGKEY_TYPE:
		worklen = bynode->pst_sym.pst_dataval.db_length-1;
		break;


	      case DB_DTE_TYPE:
	      case DB_ADTE_TYPE:
	      case DB_TMWO_TYPE:
	      case DB_TMW_TYPE:
	      case DB_TME_TYPE:
	      case DB_TSWO_TYPE:
	      case DB_TSW_TYPE:
	      case DB_TSTMP_TYPE:
	      case DB_INYM_TYPE:
	      case DB_INDS_TYPE:
	      case -DB_DTE_TYPE:
	      case -DB_ADTE_TYPE:
	      case -DB_TMWO_TYPE:
	      case -DB_TMW_TYPE:
	      case -DB_TME_TYPE:
	      case -DB_TSWO_TYPE:
	      case -DB_TSW_TYPE:
	      case -DB_TSTMP_TYPE:
	      case -DB_INYM_TYPE:
	      case -DB_INDS_TYPE:
		worklen = sizeof(i4);
		break;

	      case DB_PAT_TYPE:
	      case -DB_PAT_TYPE:
		worklen = sizeof(i2);
		break;
	    }
	    align = aggbufsz % worklen;
	    if (align != 0) aggbufsz += (worklen - align);
	    aggbufsz += bynode->pst_sym.pst_dataval.db_length;
	    (*bycount)++;
	}
    }

    /* count up the number of aops */
    oflwsize = hashsize = aggbufsz;
    naops = 0;
    for (aop_resdom = aghead->pst_left;
	    aop_resdom != NULL && aop_resdom != *byvals && 
				aop_resdom->pst_sym.pst_type != PST_TREE;
	    aop_resdom = aop_resdom->pst_left
	)
    {
	if (!(aop_resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsflags&PST_RS_PRINT))
	{
	    continue;
	}

	naops += 1;

	/* Compute work field offsets while we're here. */
	aop = aop_resdom->pst_right;
	aopparm = aop->pst_left;
	rsno = aop_resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsno;
	binagg = aop->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_fiflags &
			ADI_F16_OLAPAGG && aop->pst_right != NULL;

	/* All aop's require one instance of result operand in work buffer.
	** avg needs an extra counter (i4) and standard deviation & variance need
	** a counter (i4) and an extra float (f8) work field. 
	**
	** Start by allocating result (from resdom description) - aligned. */
	worklen = 1;
	switch (aop_resdom->pst_sym.pst_dataval.db_datatype) {
	  case DB_INT_TYPE:
	  case DB_FLT_TYPE:
	  case DB_MNY_TYPE:
	    worklen = aop_resdom->pst_sym.pst_dataval.db_length;
	    break;

	  case DB_VCH_TYPE:
	  case -DB_VCH_TYPE:
	  case DB_TXT_TYPE:
	  case -DB_TXT_TYPE:
	    worklen = DB_CNTSIZE;
	    break;

	  case DB_NCHR_TYPE:
	  case DB_NVCHR_TYPE:
	  case -DB_NCHR_TYPE:
	  case -DB_NVCHR_TYPE:
	    worklen = sizeof(UCS2);
	    break;

	  case -DB_INT_TYPE:
	  case -DB_FLT_TYPE:
	  case -DB_MNY_TYPE:
	    worklen = aop_resdom->pst_sym.pst_dataval.db_length-1;
	    break;

	  case DB_DTE_TYPE:
	  case DB_ADTE_TYPE:
	  case DB_TMWO_TYPE:
	  case DB_TMW_TYPE:
	  case DB_TME_TYPE:
	  case DB_TSWO_TYPE:
	  case DB_TSW_TYPE:
	  case DB_TSTMP_TYPE:
	  case DB_INYM_TYPE:
	  case DB_INDS_TYPE:
	  case -DB_DTE_TYPE:
	  case -DB_ADTE_TYPE:
	  case -DB_TMWO_TYPE:
	  case -DB_TMW_TYPE:
	  case -DB_TME_TYPE:
	  case -DB_TSWO_TYPE:
	  case -DB_TSW_TYPE:
	  case -DB_TSTMP_TYPE:
	  case -DB_INYM_TYPE:
	  case -DB_INDS_TYPE:
	    worklen = sizeof(i4);
	    break;

	  case DB_PAT_TYPE:
	  case -DB_PAT_TYPE:
	    worklen = sizeof(i2);
	    break;
	}

	/* Some aggregates (e.g. avg(date), some OLAP statistical aggs) require
	** workspace structure distinct from result type. They're always f8
	** aligned. This is also true for sum(float) aggs, even if the source
	** value is f4. */
	sumf4 = FALSE;				/* init flag */
	if ((agwslen = aop->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_agwsdv_len)
		!= 0 && 
	    aop->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_fiflags & ADI_F4_WORKSPACE)
	    worklen = sizeof(f8);
	else if (aop->pst_sym.pst_value.pst_s_op.pst_opno == ADI_SUM_OP 
		&& abs(aop->pst_sym.pst_dataval.db_datatype) == DB_FLT_TYPE) 
	{
	    worklen = sizeof(f8);
	    sumf4 = TRUE;
	}
	else agwslen = 0;
	align = aggbufsz % worklen;
	if (align != 0) 
	    aggbufsz += (worklen - align);
	tginfo->opc_tsoffsets[rsno].opc_aggwoffset = aggbufsz;

	/* Compute overflow buffer aggregate input offset - aligned. */
	if (aopparm)	/* e.g. count(*) has no parm */
	{
	    worklen = 1;
	    switch (aopparm->pst_sym.pst_dataval.db_datatype) {
	      case DB_INT_TYPE:
	      case DB_FLT_TYPE:
	      case DB_MNY_TYPE:
		worklen = aopparm->pst_sym.pst_dataval.db_length;
		break;

	      case DB_VCH_TYPE:
	      case -DB_VCH_TYPE:
	      case DB_TXT_TYPE:
	      case -DB_TXT_TYPE:
		worklen = DB_CNTSIZE;
		break;

	      case DB_NCHR_TYPE:
	      case DB_NVCHR_TYPE:
	      case -DB_NCHR_TYPE:
	      case -DB_NVCHR_TYPE:
		worklen = sizeof(UCS2);
		break;

	      case -DB_INT_TYPE:
	      case -DB_FLT_TYPE:
	      case -DB_MNY_TYPE:
		worklen = aopparm->pst_sym.pst_dataval.db_length-1;
		break;


	      case DB_DTE_TYPE:
	      case DB_ADTE_TYPE:
	      case DB_TMWO_TYPE:
	      case DB_TMW_TYPE:
	      case DB_TME_TYPE:
	      case DB_TSWO_TYPE:
	      case DB_TSW_TYPE:
	      case DB_TSTMP_TYPE:
	      case DB_INYM_TYPE:
	      case DB_INDS_TYPE:
	      case -DB_DTE_TYPE:
	      case -DB_ADTE_TYPE:
	      case -DB_TMWO_TYPE:
	      case -DB_TMW_TYPE:
	      case -DB_TME_TYPE:
	      case -DB_TSWO_TYPE:
	      case -DB_TSW_TYPE:
	      case -DB_TSTMP_TYPE:
	      case -DB_INYM_TYPE:
	      case -DB_INDS_TYPE:
		worklen = sizeof(i4);
		break;

	      case DB_PAT_TYPE:
	      case -DB_PAT_TYPE:
		worklen = sizeof(i2);
		break;
	    }

	    align = oflwsize % worklen;
	    if (align != 0)
		oflwsize += (worklen - align);
	    tginfo->opc_tsoffsets[rsno].opc_aggooffset = oflwsize;
	    oflwsize += aopparm->pst_sym.pst_dataval.db_length;
	}
	else tginfo->opc_tsoffsets[rsno].opc_aggooffset = -1;

	/* Now do the same for the 2nd parm (if OLAP binary aggregate). */
	if (binagg)
	{
	    PST_QNODE	*aopparm2 = aop->pst_right;

	    worklen = 1;
	    switch (aopparm2->pst_sym.pst_dataval.db_datatype) {
	      case DB_INT_TYPE:
	      case DB_FLT_TYPE:
	      case DB_MNY_TYPE:
		worklen = aopparm2->pst_sym.pst_dataval.db_length;
		break;

	      case DB_NCHR_TYPE:
	      case DB_NVCHR_TYPE:
	      case -DB_NCHR_TYPE:
	      case -DB_NVCHR_TYPE:
		worklen = sizeof(UCS2);
		break;

	      case -DB_INT_TYPE:
	      case -DB_FLT_TYPE:
	      case -DB_MNY_TYPE:
		worklen = aopparm2->pst_sym.pst_dataval.db_length-1;
		break;

	      case DB_DTE_TYPE:
	      case DB_ADTE_TYPE:
	      case DB_TMWO_TYPE:
	      case DB_TMW_TYPE:
	      case DB_TME_TYPE:
	      case DB_TSWO_TYPE:
	      case DB_TSW_TYPE:
	      case DB_TSTMP_TYPE:
	      case DB_INYM_TYPE:
	      case DB_INDS_TYPE:
	      case -DB_DTE_TYPE:
	      case -DB_ADTE_TYPE:
	      case -DB_TMWO_TYPE:
	      case -DB_TMW_TYPE:
	      case -DB_TME_TYPE:
	      case -DB_TSWO_TYPE:
	      case -DB_TSW_TYPE:
	      case -DB_TSTMP_TYPE:
	      case -DB_INYM_TYPE:
	      case -DB_INDS_TYPE:
		worklen = sizeof(i4);
		break;

	      case DB_PAT_TYPE:
	      case -DB_PAT_TYPE:
		worklen = sizeof(i2);
		break;
	    }

	    align = oflwsize % worklen;
	    if (align != 0)
		oflwsize += (worklen - align);
	    tginfo->opc_tsoffsets[rsno].opc_aggooffset2 = oflwsize;
	    oflwsize += aopparm2->pst_sym.pst_dataval.db_length;
	}
	else tginfo->opc_tsoffsets[rsno].opc_aggooffset2 = -1;

	if (agwslen != 0) 
	    aggbufsz += agwslen;
	else aggbufsz += aop_resdom->pst_sym.pst_dataval.db_length;
	if (quelsagg && aop_resdom->pst_sym.pst_dataval.db_datatype > 0)
	    aggbufsz++;		/* incr for nullind in Quel work field */
	if (sumf4)
	    aggbufsz += 4;

	/* Add extras for avg, stddev, variance. */
	switch (aop->pst_sym.pst_value.pst_s_op.pst_opno) {
	  case ADI_AVG_OP:
	  case ADI_SINGLETON_OP:
	    worklen = sizeof(i8);
	    align = aggbufsz % worklen;
	    if (align != 0) aggbufsz += (worklen - align);
	    break;

	  case ADI_STDDEV_SAMP_OP:
	  case ADI_STDDEV_POP_OP:
	  case ADI_VAR_SAMP_OP:
	  case ADI_VAR_POP_OP:
	    worklen = sizeof(f8);
	    align = aggbufsz % worklen;
	    if (align != 0) aggbufsz += (worklen - align);
	    worklen += sizeof(i4);
	    break;
	  default:
	    if (agwslen == 0 && aop_resdom->pst_sym.pst_dataval.db_datatype > 0
			&& aopparm && aopparm->pst_sym.pst_dataval.db_datatype < 0)
		aggbufsz++;		/* work field nullable, result isn't */
	    worklen = 0;
	    break;
	}
	aggbufsz += worklen;
    }

    if (naops == 1 && 
	    aghead->pst_left->pst_right->pst_sym.pst_value.pst_s_op.pst_opno 
								== ADI_ANY_OP
	)
    {
	ahd->qhd_obj.qhd_qep.ahd_qepflag |= AHD_ANY;
    }

    /* estimate the size of the cx; */
    ninstr = 0;
    nops = 0;
    nconst = 0;
    szconst = 0;
    opc_cxest(global, cnode, aghead->pst_left, 
					    &ninstr, &nops, &nconst, &szconst);
    opc_cxest(global, cnode, *byvals, &ninstr, &nops, &nconst, &szconst);
    if (subqry->ops_sqtype == OPS_SAGG)
	nconst += naops;	/* non-zero triggers right computation of 
				** maxbase in opc_adalloc_begin */
    max_base = 3 + cnode->opc_below_rows;

    /* begin the qadf, automatically adding a base to hold constant size
    ** temporarys
    */
    opc_adalloc_begin(global, cadf, qadf, 
		    ninstr, nops, nconst, szconst, max_base, OPC_STACK_MEM);

    /* Compute rounded up aggregate work buffer size. */
    align = aggbufsz % sizeof(ALIGN_RESTRICT);
    if (align != 0) aggbufsz += (sizeof(ALIGN_RESTRICT) - align);
    worksize = aggbufsz;

    /* Allocate work buffer. */
    opc_ptrow(global, workbase, aggbufsz);
    *oflwwork = *workbase;
    opc_adbase(global, cadf, QEN_ROW, *workbase);
    *workbase = cadf->opc_row_base[*workbase];

    if (subqry->ops_sqtype == OPS_FAGG)
    {
	/* add the row to put the aggregate tuple into */
	outsize = (i4) tginfo->opc_tlsize;
	opc_ptrow(global, &outrow, (i4)outsize);

	(*qadf)->qen_output = outrow;

	opc_adbase(global, cadf, QEN_ROW, outrow);
	*outbase = cadf->opc_row_base[outrow];
    }
    else if (subqry->ops_sqtype == OPS_RFAGG ||
		subqry->ops_sqtype == OPS_RSAGG ||
		subqry->ops_sqtype == OPS_HFAGG
	    )
    {
	opc_adbase(global, cadf, QEN_OUT, 0);
	*outbase = cadf->opc_out_base;

	/* If this is an RSAGG or an HFAGG/RFAGG then set all of the 
	** resdoms to return the
	** data to the user/caller. This hack is needed because setting
	** this in OPF is supposedly difficult.
	*/
	for (aop_resdom = aghead->pst_left;
		aop_resdom != NULL && 
				    aop_resdom->pst_sym.pst_type == PST_RESDOM;
		aop_resdom = aop_resdom->pst_left
	    )
	{
	    aop_resdom->pst_sym.pst_value.pst_s_rsdm.pst_ttargtype = PST_USER;
	}
    }

    if (subqry->ops_sqtype == OPS_HFAGG)
    {
	/* For hash aggregates, set up ADF CXs for overflow handling.
	** It should be same size as aggregation CX. */
	opc_adalloc_begin(global, ocadf, oqadf, ninstr, nops, nconst,
			szconst, max_base, OPC_STACK_MEM);

	/* Compute rounded up overflow buffer size.  The overflow row
	** uses the divided "row length" in the row header, so enforce
	** the alignment required by QEFH_SET_ROW_LENGTH_MACRO:
	*/
	oflwsize = DB_ALIGNTO_MACRO(oflwsize, QEF_HASH_DUP_ALIGN);
	/* Just in case standard alignment is more strict, do that too */
	oflwsize = DB_ALIGN_MACRO(oflwsize);

	/* Allocate overflow buffer. */
	opc_ptrow(global, oflwbase, oflwsize);
	opc_adbase(global, ocadf, QEN_ROW, *oflwbase);
	*oflwbase = ocadf->opc_row_base[*oflwbase];
	opc_adbase(global, ocadf, QEN_ROW, *oflwwork);
	*oflwwork = ocadf->opc_row_base[*oflwwork];

	/* Also for hash aggregates, setup the ADF CX that projects 
	** grouping expressions and computes hash number. */
	ninstr = 0;
	nops = 0;
	nconst = 0;
	szconst = 0;
	opc_cxest(global, cnode, *byvals, &ninstr, &nops, &nconst, &szconst);

	opc_adalloc_begin(global, hcadf, hqadf, ninstr, nops, nconst,
			szconst, max_base, OPC_STACK_MEM);

	/* Allocate work buffer. */
	opc_ptrow(global, hashbase, hashsize);
	opc_adbase(global, hcadf, QEN_ROW, *hashbase);
	*hashbase = hcadf->opc_row_base[*hashbase];
    }
    else
    {
	*hqadf = (QEN_ADF *) NULL;
	*oqadf = (QEN_ADF *) NULL;
    }

    if (*topsort)
    {
	opc_adbase(global, cadf, QEN_ROW, (*qn)->qen_row);
	if (subqry->ops_sqtype == OPS_HFAGG)
	    opc_adbase(global, hcadf, QEN_ROW, (*qn)->qen_row);
    }
}

/*{
** Name: OPC_IAGGTARG_INIT	- Compile the init segment of an agg target list
**
** Description:
**      This routine compiles the init segment of an aggregate target list. 
**      The instructions in this segment perform two functions. First, they 
**      begin the aggregate opcode processing for each agg in question. Second, 
**      if this is an agg function then we materialize the next (or first) 
**      set of by values from the QEN tree. 
[@comment_line@]...
**
** Inputs:
**	global -
**	    This is the query wide state variable.
**	cnode -
**	    This describes where to get the data from the QEN tree.
**	aghead -
**	    This is the AGHEAD qnode for our agg target list.
**	cadf -
**	    This is the CX that we are compiling.
**	hcadf -
**	    OPC_ADF structure ptr for hashno computation CX (hash agg)
**	ocadf -
**	    OPC_ADF structure ptr for overflow row CX (hash agg)
**	tginfo -
**	    This is a description of where the data should go for the
**	    target list.
**	topsort -
**	    This is true if the top QEN tree is a topsort node.
**	byvals -
**	    This is a pointer to the first RESDOM for the by values if this 
**	    is a func agg. Otherwise, this is NULL.
**	sortinfo -
**	    This is a description of the info coming from the top sort node
**	    if topsort is TRUE.
**	workbase -
**	    This is the workarea base number for the CX.
**	outbase -
**	    This is the base number for the results of the aggregate.
**	qn -
**	    This is the top QEN node.
**	subexlistpp - 
**	    Ptr to list of common subexpressions extracted for preevaluation.
**
** Outputs:
**	cadf -
**	    The init segment of this CX will be completely filled in.
**
**	Returns:
**	    nothing
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-aug-87 (eric)
**          created
**	17-jul-89 (jrb)
**	    Set precision fields correctly.  Also, fixed bug where earlier I
**	    had set opr_len wrong by accident.
**      21-july-89 (eric)
**          fixed bug 6275 by checking for non-printing resdoms when 
**	    materializing the by vals. If a by val resdom is non-printing then
**          we don't materialize it.
**      4-sept-89 (eric)
**          added fix for b7878. The problem here was that we were assuming that
**          the source and destination (sort and target list) offsets and 
**	    datatypes of the by vals would be the same. This was wrong.
**          We now get the source from sortinfo and the dest from tginfo.
**	20-dec-00 (inkdo01)
**	    Changes to generate inline ADF code for aggregates & get rid of 
**	    Aggregate workspace structure.
**	23-jan-01 (inkdo01)
**	    Changes to generate code for hash aggregation.
**	6-feb-01 (inkdo01)
**	    Changes for hash aggregation overflow handling.
**	30-mar-01 (inkdo01)
**	    Changes to make stupid Quel simple aggs with no input rows work.
**	30-may-01 (inkdo01)
**	    Changes to roll f4 sum's into a f8.
**	21-june-01 (inkdo01)
**	    Changes to get binary aggregates working.
**	25-jun-2001 (somsa01 for inkdo01)
**	    When materializing the expression for each byval, make sure we
**	    tell opc_adinstr() to apply the proper null semantics to the
**	    hash value when not performing a top sort.
**	30-aug-01 (inkdo01)
**	    changed AGBGN to OLAGBGN to save confusion.
**	17-sep-01 (inkdo01)
**	    Fix SEGV in binary aggregate support.
**	28-sep-2001 (devjo01)
**	    On 64 bit platforms, logic used to calculate total key length
**	    of a consolidated set of all character hash keys, would overstate
**	    the key length by four bytes, leading to incorrect query results.
**	    b105914.
**	4-oct-01 (inkdo01)
**	    Augment Jack's changes to remove alignment padding for hash agg.
**	    BY cols.
**	23-oct-01 (inkdo01)
**	    Fix hash agg of varchar/txt with varying nos. of trailing blanks.
**	14-dec-01 (inkdo01)
**	    Apply same logic for NVCHR type.
**	13-mar-02 (inkdo01)
**	    Changes to preparation of BY cols for non-hash aggs. Projection 
**	    aggregates can be done on column pairs that aren't the same type
**	    and this breaks qea_aggf. Fixes bug 107320.
**	1-may-02 (inkdo01)
**	    Init min(datecol) to minval, not max, because of goofy blank dates
**	    (fixes 107694).
**	11-sep-02 (inkdo01)
**	    Hash agg fix for TEXT columns retracted (trailing blanks are significant 
**	    for TEXT).
**	10-jan-03 (inkdo01)
**	    Hash agg with BY-list entry that is in an eqclass with something
**	    of diff. attributes (e.g. a i4 with an i2) needs to be coerced.
**	14-feb-03 (inkdo01)
**	    Hash agg on date column needs to move value to hash buffer with 
**	    date coercion so status byte can be normalized (fixes bug 109553).
**	18-feb-03 (inkdo01)
**	    Same sort of problem with TEXT columns. They can have junk past
**	    the defined value and need coercion to clean it up.
**	23-july-03 (inkdo01)
**	    Align dumb ol' Unicode for non-hash aggregation (fixes bug 110610).
**	7-Apr-2004 (schka24)
**	    Fill in temp table attr default stuff in case anyone looks.
**	4-july-05 (inkdo01)
**	    Added code to optimize CXs.
**	22-aug-05 (inkdo01)
**	    Minor fiddle to constant lengths to assure correct init.
**	7-Dec-2005 (kschendel)
**	    Build compare-list instead of attr list for hash agg, and we
**	    only need one concatenated entry.
**	27-apr-06 (toumi01) BUG 115713
**	    If count() aggregation is done on a nullable value, init a
**	    nullable int value to hold it, since ADE will always compile
**	    code that allows for nullable results for fields derived from
**	    nullable values, even in the special case of count(), where
**	    the result is always valued.
**	27-Nov-2006 (kschendel) SIR 122512
**	    Don't generate hash number in CX, let haggf code do it by hand.
**	22-Mar-07 (wanfr01)
**	    Bug 117979
**	    Need third parameter for adi_resolve
**	6-aug-2008 (dougi)
**	    Add support for opc_inhash flag to generate MECOPYN as needed.
*/
static VOID
opc_iaggtarg_init(
	OPS_STATE   *global,
	OPC_NODE    *cnode,
	PST_QNODE   *aghead,
	OPC_ADF	    *cadf,
	OPC_ADF	    *hcadf,
	OPC_ADF	    *ocadf,
	OPC_TGINFO  *tginfo,
	i4	    topsort,
	PST_QNODE   *byvals,
	OPC_TGINFO  *sortinfo,
	i4	    workbase, 
	i4	    hashbase, 
	i4	    outbase,
	i4	    oflwbase, 
	QEN_NODE    *qn,
	DB_CMP_LIST *hashkeys,
	PST_QNODE   *subexlistp)
{
    OPS_SUBQUERY	*subqry = global->ops_cstate.opc_subqry;
    PST_QNODE		*aop_resdom, *aop, *aopparm;
    ADE_OPERAND		ops[2];
    PST_QNODE		*byval;
    ADI_FI_DESC		*vchfidp, *nvchfidp;
    ADI_RSLV_BLK	adi_rslv_blk;
    i4			rsno, byvalno;
    ADE_OPERAND		opdummy;
    i4			tidatt;
    i4			aopno;
    i4			j;
    i4			worklen, align, offset, agwslen;
    ADE_OPERAND		int0, flt0, other0;
    ADE_OPERAND		resop;
    DB_STATUS		status;
    DB_DT_ID		resdt, bydt;
    bool		quelsagg = FALSE;
    bool		binagg;


    int0.opr_dt = flt0.opr_dt = 0;	/* init. */
    if ((subqry->ops_sqtype == OPS_SAGG || subqry->ops_sqtype == OPS_RSAGG)
	&& aghead->pst_sym.pst_value.pst_s_root.pst_qlang == DB_QUEL)
	quelsagg = TRUE;

    /* First compile evaluation of common subexpressions. */
    resop.opr_len = OPC_UNKNOWN_LEN;
    resop.opr_prec = OPC_UNKNOWN_PREC;
    if (subqry->ops_sqtype == OPS_HFAGG && subexlistp != (PST_QNODE *) NULL)
	opc_cqual(global, cnode, subexlistp, ocadf, ADE_SINIT, &resop, &tidatt);

    /* For (each aop) 
    ** {
    **	    init work fields as necessary (copy max/min, init counts, sums
    **	    for any, count, sum, avg, OLAP aggs).
    ** }
    */
    for (aop_resdom = aghead->pst_left;
	    (aop_resdom != NULL && 
		aop_resdom->pst_sym.pst_type != PST_TREE && 
		aop_resdom != byvals
	    );
	    aop_resdom = aop_resdom->pst_left
	)
    {
	if (!(aop_resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsflags&PST_RS_PRINT))
	{
	    continue;
	}

	aop = aop_resdom->pst_right;
	aopparm = aop->pst_left;
	aopno = aop->pst_sym.pst_value.pst_s_op.pst_opno;
	rsno = aop_resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsno;
	binagg = aop->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_fiflags &
			ADI_F16_OLAPAGG && aop->pst_right != NULL;

	/* If this is hash aggregate, generate code to materialize input
	** expression into overflow buffer (only executed if new row 
	** doesn't match group in hash table). If AOP is count(*), 
	** aopparm is NULL and no code is generated. */
	if (subqry->ops_sqtype == OPS_HFAGG && aopparm)
	{
	    ops[0].opr_base = oflwbase;
	    ops[0].opr_offset = tginfo->opc_tsoffsets[rsno].opc_aggooffset;
	    ops[0].opr_dt = aopparm->pst_sym.pst_dataval.db_datatype;
	    ops[0].opr_len = aopparm->pst_sym.pst_dataval.db_length;
	    ops[0].opr_prec = aopparm->pst_sym.pst_dataval.db_prec;

	    ops[1].opr_dt = aopparm->pst_sym.pst_dataval.db_datatype;
	    ops[1].opr_len = OPC_UNKNOWN_LEN;
	    ops[1].opr_prec = OPC_UNKNOWN_PREC;

	    opc_cqual(global, cnode, aopparm, ocadf, ADE_SINIT, 
							&ops[1], &tidatt);
	    opc_adinstr(global, ocadf, ADE_MECOPY, ADE_SINIT, 2, ops, 0);

	    /* If OLAP binary aggregate, materialize 2nd parm, too. */
	    if (binagg)
	    {
		PST_QNODE	*aopparm2 = aop->pst_right;

		ops[0].opr_offset = tginfo->opc_tsoffsets[rsno].opc_aggooffset2;
		ops[0].opr_dt = aopparm2->pst_sym.pst_dataval.db_datatype;
		ops[0].opr_len = aopparm2->pst_sym.pst_dataval.db_length;
		ops[0].opr_prec = aopparm2->pst_sym.pst_dataval.db_prec;

		ops[1].opr_dt = aopparm2->pst_sym.pst_dataval.db_datatype;
		ops[1].opr_len = OPC_UNKNOWN_LEN;
		ops[1].opr_prec = OPC_UNKNOWN_PREC;

		opc_cqual(global, cnode, aopparm2, ocadf, ADE_SINIT, 
							&ops[1], &tidatt);
		opc_adinstr(global, ocadf, ADE_MECOPY, ADE_SINIT, 2, ops, 0);
	    }
	}

	/* Now see if we need int/float 0 and we haven't alloc'ed them yet.
	** Believe it or not, we're actually going to share constants in an 
	** ADF expression! 
	**
	** Note that aggs with non-standard work fields (e.g. avg(date))
	** are treated differently - field is BYTE, to force init to 0's */

	resdt = abs(aop_resdom->pst_sym.pst_dataval.db_datatype);
	if ((agwslen = aop->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_agwsdv_len)
		!= 0 && 
	    aop->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_fiflags & ADI_F4_WORKSPACE) 
			resdt = DB_BYTE_TYPE;
	else agwslen = 0;

	if (int0.opr_dt == 0 && 
	    (resdt == DB_INT_TYPE || aopno == ADI_AVG_OP ||
	     aopno == ADI_SINGLETON_OP ||
	     aopno == ADI_STDDEV_SAMP_OP || aopno == ADI_VAR_SAMP_OP ||
	     aopno == ADI_STDDEV_POP_OP || aopno == ADI_VAR_POP_OP) &&
	    !(aopno == ADI_MIN_OP || aopno == ADI_MAX_OP))
	{
	    DB_DATA_VALUE	intdv;
	    i8			i0 = 0;

	    intdv.db_datatype = DB_INT_TYPE;
	    intdv.db_length = 8;
	    intdv.db_prec = 0;
	    intdv.db_data = (char *)&i0;

	    opc_adconst(global, cadf, &intdv, &int0, DB_SQL, ADE_SINIT);
					/* allocate int 0 constant */
	    STRUCT_ASSIGN_MACRO(int0, ops[1]);
	}

	if (flt0.opr_dt == 0 && 
	    (resdt == DB_FLT_TYPE || resdt == DB_MNY_TYPE || aopno == ADI_AVG_OP || 
	     aopno == ADI_STDDEV_SAMP_OP || aopno == ADI_VAR_SAMP_OP ||
	     aopno == ADI_STDDEV_POP_OP || aopno == ADI_VAR_POP_OP))
	{
	    DB_DATA_VALUE	fltdv;
	    f8			f0 = 0.0;

	    fltdv.db_datatype = DB_FLT_TYPE;
	    fltdv.db_length = 8;
	    fltdv.db_prec = 0;
	    fltdv.db_data = (char *)&f0;

	    opc_adconst(global, cadf, &fltdv, &flt0, DB_SQL, ADE_SINIT);
					/* allocate float 0.0 constant */
	    STRUCT_ASSIGN_MACRO(flt0, ops[1]);
	}

	/* Now initialize the work buffer field(s) as necessary for the 
	** aggregate operation. */
	if (aopno == ADI_MAX_OP || aopno == ADI_MIN_OP || aopno == ADI_SUM_OP)
	{
	    DB_DATA_VALUE	null_minmaxdv;
	    /* Max, min and sum start with null result (if result is nullable)
	    ** overlaid by the min/max (respectively) value for the result 
	    ** data type or (for sum) a 0, in the work buffer. The first 
	    ** non-null source value removes the indicator but leaves the
	    ** underlying data value intact for max/min/sum semantics. */
	    ops[0].opr_dt = tginfo->opc_tsoffsets[rsno].opc_tdatatype;
	    ops[0].opr_len = tginfo->opc_tsoffsets[rsno].opc_tlength;
	    if (aopno == ADI_SUM_OP && abs(ops[0].opr_dt) == DB_FLT_TYPE &&
		ops[0].opr_len < 8)
		ops[0].opr_len += 4;	/* sum flt4's into a flt8 */
	    ops[0].opr_prec = tginfo->opc_tsoffsets[rsno].opc_tprec;
	    ops[0].opr_offset = tginfo->opc_tsoffsets[rsno].opc_aggwoffset;
	    ops[0].opr_base = workbase;
	    if (ops[0].opr_dt > 0 && 
		(aopparm && aopparm->pst_sym.pst_dataval.db_datatype < 0 ||
		binagg && aop->pst_right->pst_sym.pst_dataval.db_datatype < 0))
	    {
		/* Workfield should be nullable. */
		ops[0].opr_dt = -ops[0].opr_dt;
		ops[0].opr_len++;
	    }

	    /* Init the initialization dv. */
	    null_minmaxdv.db_datatype = ops[0].opr_dt;
	    null_minmaxdv.db_length = ops[0].opr_len;
	    null_minmaxdv.db_prec = ops[0].opr_prec;
	    null_minmaxdv.db_data = opu_memory(global, ops[0].opr_len);

	    if (ops[0].opr_dt < 0 || quelsagg)
	    {
		bool	notnull = (ops[0].opr_dt > 0);

		/* Nullable result - copy null value. */
		if (notnull)
		{
		    ops[0].opr_dt = -ops[0].opr_dt;	/* nullable for Quel */
		    ops[0].opr_len++;
		}
		opc_adempty(global, &null_minmaxdv, ops[0].opr_dt, 
			ops[0].opr_prec, ops[0].opr_len);
		opc_adconst(global, cadf, &null_minmaxdv, &ops[1], DB_SQL, 
			ADE_SINIT);
		opc_adinstr(global, cadf, ADE_MECOPY, ADE_SINIT, 2, ops, 1);
		if (notnull)
		{
		    ops[0].opr_dt = -ops[0].opr_dt;	/* back to not null */
		    ops[0].opr_len--;
		}
	    }

	    if (aopno == ADI_MAX_OP || aopno == ADI_MIN_OP)
	    {
		/* In both cases, copy min (for max) or max (for min) value for
		** result datatype. If result is nullable, the min/max value 
		** is used by first real comparison with a non-null source
		** value (after the null indicator is erased). */
		DB_DATA_VALUE	*mindvp, *maxdvp;

		if (ops[0].opr_dt < 0)
		{
		    /* Treat as not nullable. */
		    ops[0].opr_dt = -ops[0].opr_dt;
		    ops[0].opr_len--;
		}

		mindvp = maxdvp = (DB_DATA_VALUE *) NULL;
		/* Get min value for max and max value for min. For min(date),
		** get min anyway, because of odd blank date semantics. */
		if (aopno == ADI_MAX_OP || ops[0].opr_dt == DB_DTE_TYPE)
		    mindvp = &null_minmaxdv;
		else maxdvp = &null_minmaxdv;
		adc_minmaxdv(global->ops_adfcb, mindvp, maxdvp);
		opc_adconst(global, cadf, &null_minmaxdv, &ops[1], DB_SQL, 
			ADE_SINIT);
	    }
	}

	/* The remaining aggs just zero counters and accumulators. */
	if (aopno == ADI_MAX_OP || aopno == ADI_MIN_OP) ;  /* null stmt */
	else if (resdt == DB_INT_TYPE)
	{
	    STRUCT_ASSIGN_MACRO(int0, ops[1]);
	    ops[1].opr_len = aop_resdom->pst_sym.pst_dataval.db_length;
	    if (aop_resdom->pst_sym.pst_dataval.db_datatype < 0)
		ops[1].opr_len--;	/* don't copy indicator */
	}
	else if (resdt == DB_FLT_TYPE || resdt == DB_MNY_TYPE) 
				STRUCT_ASSIGN_MACRO(flt0, ops[1]);
	else
	{
	    DB_DATA_VALUE	otherdv;

	    /* Result is some other type - build constant 0 of target type. */
	    if (agwslen == 0)
	    {
		/* Work field is same as result type. */
		ops[1].opr_len = aop_resdom->pst_sym.pst_dataval.db_length;
		ops[1].opr_prec = aop_resdom->pst_sym.pst_dataval.db_prec;
		ops[1].opr_dt = aop_resdom->pst_sym.pst_dataval.db_datatype;
		if (ops[1].opr_dt < 0)
		{
		    ops[1].opr_dt = -ops[1].opr_dt;
		    ops[1].opr_len--;
		}
	    }
	    else
	    {
		/* Funny agg with special workfield of type BYTE. */
		ops[1].opr_len = agwslen;
		ops[1].opr_prec = 0;
		ops[1].opr_dt = DB_BYTE_TYPE;
	    }

	    /* Build constant for moving, but only if not binary agg. */
	    if (!binagg)
	    {
		otherdv.db_data = (PTR) NULL;	/* part of opc_adempty protocol */
		opc_adempty(global, &otherdv, ops[1].opr_dt,
			ops[1].opr_prec, ops[1].opr_len);
		opc_adconst(global, cadf, &otherdv, &ops[1], DB_SQL, ADE_SINIT);
	    }
	}

	/* Compile the MECOPY of 1st workfield. */
	STRUCT_ASSIGN_MACRO(ops[1], ops[0]);
	if (ops[0].opr_dt < 0)
	{
	    /* Treat as not nullable because null aware aggs already have 
	    ** indicator set. */
	    ops[0].opr_dt = -ops[0].opr_dt;
	    ops[0].opr_len--;
	}

	ops[0].opr_base = workbase;
	ops[0].opr_offset = tginfo->opc_tsoffsets[rsno].opc_aggwoffset;

	/*
	** count(x) is by definition not-null, but ADE doesn't know that
	** and will build nullable count() aggregation code if we are
	** aggregating a nullable value, so we must INIT the null byte.
	*/
	if (aopno == ADI_CNT_OP &&
	    (aopparm && aopparm->pst_sym.pst_dataval.db_datatype < 0))
	{
	    ops[0].opr_dt = -ops[0].opr_dt;
	    ops[1].opr_dt = -ops[1].opr_dt;
	    ops[0].opr_len++;
	    ops[1].opr_len++;
	}

	opc_adinstr(global, cadf, (binagg ? ADE_OLAGBGN : ADE_MECOPY), 
		ADE_SINIT, (binagg ? 1 : 2), ops, 1);

	/* Avg, stddev and variance have 1 or 2 other fields to init. */
	if (!(aopno == ADI_AVG_OP || aopno == ADI_STDDEV_SAMP_OP ||
		aopno == ADI_SINGLETON_OP ||
		aopno == ADI_STDDEV_POP_OP || aopno == ADI_VAR_SAMP_OP ||
		aopno == ADI_VAR_POP_OP)) continue;	/* others are done */

	if (agwslen == 0)
	    offset = ops[0].opr_offset + aop_resdom->pst_sym.pst_dataval.db_length;
	else offset = ops[0].opr_offset + agwslen;
	if (aopno != ADI_AVG_OP && aopno != ADI_SINGLETON_OP)
	{
	    /* Stddev/variance have extra f8. */
	    worklen = sizeof(f8);
	    align = offset % worklen;
	    if (align != 0) offset += (worklen-align);
	    ops[0].opr_offset = offset;
	    opc_adinstr(global, cadf, ADE_MECOPY, ADE_SINIT, 2, ops, 1);
					/* note, these guys just copied a flt0
					** so rest of ops[0]/[1] are still ok */
	    offset += sizeof(f8);
	}

	/* Just the i4 counter for avg, stddev, variance. */
	STRUCT_ASSIGN_MACRO(int0, ops[1]);
	STRUCT_ASSIGN_MACRO(ops[1], ops[0]);
	worklen = sizeof(i4);
	align = offset % worklen;
	if (align != 0) offset += (worklen - align);
	ops[0].opr_offset = offset;
	ops[0].opr_base = workbase; 
	opc_adinstr(global, cadf, ADE_MECOPY, ADE_SINIT, 2, ops, 1);
    }

    /* if (this is an aggf) */
    if (subqry->ops_sqtype == OPS_FAGG ||
	    subqry->ops_sqtype == OPS_RFAGG ||
	    subqry->ops_sqtype == OPS_HFAGG
	)
    {
	OPC_ADF	*byadf;
	i4	bybase, byseg;
	i4	keystart;
	DB_DT_ID bytype;

	/* Materialize the by vals into the agg work buffer and possibly
	** (if OPS_HFAGG) run hash function. */
	if (subqry->ops_sqtype == OPS_HFAGG)
	{
	    keystart = QEF_HASH_DUP_SIZE;
	    /* Setup ADF expression. */
	    byadf = hcadf;
	    byadf->opc_inhash = TRUE;
	    bybase = hashbase;
	    byseg = ADE_SMAIN;
	    /* Prepare for possible varchar by columns. */
	    vchfidp = nvchfidp = NULL;
	    adi_rslv_blk.adi_num_dts = 1;
	    adi_rslv_blk.adi_op_id = 0;
	    adi_rslv_blk.adi_fidesc = (ADI_FI_DESC *) NULL;
	}
	else
	{
	    keystart = 0;
	    /* Setup ADF expression. */
	    byadf = cadf;
	    bybase = workbase;
	    byseg = ADE_SINIT;
	}
	offset = keystart;

	/* for (each byval) */
	for (byval = byvals;
		byval != NULL && byval->pst_sym.pst_type != PST_TREE;
		byval = byval->pst_left
	    )
	{
	    /* if this byval is a non-printing resdom, then don't 
	    ** materialize it (by definition).
	    */
	    if (!(byval->pst_sym.pst_value.pst_s_rsdm.pst_rsflags&PST_RS_PRINT))
	    {
		continue;
	    }

	    /* Materialize the byval - first determining output characteristics. */
	    byvalno = byval->pst_sym.pst_value.pst_s_rsdm.pst_rsno;
	    ops[0].opr_base = bybase;
	    bydt = ops[0].opr_dt = 
			(i4) tginfo->opc_tsoffsets[byvalno].opc_tdatatype;
	    bytype = abs(bydt);
	    ops[0].opr_len = (i4) tginfo->opc_tsoffsets[byvalno].opc_tlength;
	    ops[0].opr_prec = (i4) tginfo->opc_tsoffsets[byvalno].opc_tprec;

	    if (subqry->ops_sqtype != OPS_HFAGG)
	    {
		/* For non-hash agg., align the BY cols. */
		switch (bydt) {
		  case DB_INT_TYPE:
		  case DB_FLT_TYPE:
		  case DB_MNY_TYPE:
		    worklen = ops[0].opr_len;
		    break;

		  case -DB_INT_TYPE:
		  case -DB_FLT_TYPE:
		  case -DB_MNY_TYPE:
		    worklen = ops[0].opr_len - 1;
		    break;

		  case DB_NCHR_TYPE:
	  	  case DB_NVCHR_TYPE:
	    	  case -DB_NCHR_TYPE:
		  case -DB_NVCHR_TYPE:
		    worklen = sizeof(UCS2);
		    break;

		  case DB_DTE_TYPE:
		  case DB_ADTE_TYPE:
		  case DB_TMWO_TYPE:
		  case DB_TMW_TYPE:
		  case DB_TME_TYPE:
		  case DB_TSWO_TYPE:
		  case DB_TSW_TYPE:
		  case DB_TSTMP_TYPE:
		  case DB_INYM_TYPE:
		  case DB_INDS_TYPE:
		  case -DB_DTE_TYPE:
		  case -DB_ADTE_TYPE:
		  case -DB_TMWO_TYPE:
		  case -DB_TMW_TYPE:
		  case -DB_TME_TYPE:
		  case -DB_TSWO_TYPE:
		  case -DB_TSW_TYPE:
		  case -DB_TSTMP_TYPE:
		  case -DB_INYM_TYPE:
		  case -DB_INDS_TYPE:
		    worklen = sizeof(i4);
		    break;

		  case DB_PAT_TYPE:
		  case -DB_PAT_TYPE:
		    worklen = sizeof(i2);
		    break;
		  default:
		    worklen = 1;
		    break;
		}

		align = offset % worklen;
		if (align != 0) offset += (worklen - align);
	    }

	    ops[0].opr_offset = offset;
	    offset += ops[0].opr_len;

	    /* If we're on top of a sort, use opc_rdmove to generate MECOPY
	    ** from sort result buffer. Otherwise, materialize expression 
	    ** and explicitly MECOPY it to buffer. */
	    if (topsort == TRUE)
	    {
		ops[1].opr_dt = sortinfo->opc_tsoffsets[byvalno].opc_tdatatype;
		ops[1].opr_len = sortinfo->opc_tsoffsets[byvalno].opc_tlength;
		ops[1].opr_offset = sortinfo->opc_tsoffsets[byvalno].opc_toffset;
		ops[1].opr_base = byadf->opc_row_base[qn->qen_row];
		ops[1].opr_prec = sortinfo->opc_tsoffsets[byvalno].opc_tprec;

		if (subqry->ops_result == OPV_NOGVAR)
		{
		    opc_rdmove(global, byval, ops, byadf, byseg,
			(RDR_INFO *)NULL, TRUE);
		}
		else
		{
		    opc_rdmove(global, byval, ops, byadf, byseg,
			global->ops_rangetab.opv_base->
				opv_grv[subqry->ops_result]->opv_relation, TRUE);
		}
	    }
	    else
	    {
		i4	opcode = ADE_MECOPYN;

		/* Check for hash agg on varchar and get F.I. desc, then
		** fi opid to apply "trim" during projection to buffer. */
		if (subqry->ops_sqtype == OPS_HFAGG)
		 if (bytype == DB_VCH_TYPE || bytype == DB_NVCHR_TYPE)
		 {
		    if (adi_rslv_blk.adi_op_id == 0)
			status = adi_opid(global->ops_adfcb, "trim",
				&adi_rslv_blk.adi_op_id);
		    if (bytype == DB_VCH_TYPE)
		    {
			if (vchfidp == NULL)
			{
			    adi_rslv_blk.adi_dt[0] = DB_VCH_TYPE;
			    adi_rslv_blk.adi_fidesc = NULL;
			    status = adi_resolve(global->ops_adfcb, 
							&adi_rslv_blk, FALSE);
			    vchfidp = adi_rslv_blk.adi_fidesc;
			}
			opcode = vchfidp->adi_finstid;
		    }
		    if (bytype == DB_NVCHR_TYPE)
		    {
			if (nvchfidp == NULL)
			{
			    adi_rslv_blk.adi_dt[0] = DB_NVCHR_TYPE;
			    adi_rslv_blk.adi_fidesc = NULL;
			    status = adi_resolve(global->ops_adfcb, 
							&adi_rslv_blk, FALSE);
			    nvchfidp = adi_rslv_blk.adi_fidesc;
			}
			opcode = nvchfidp->adi_finstid;
		    }
		}

		ops[1].opr_dt = byval->pst_sym.pst_dataval.db_datatype;
		ops[1].opr_len = OPC_UNKNOWN_LEN;
                ops[1].opr_prec = OPC_UNKNOWN_PREC;
		opc_cqual(global, cnode, byval->pst_right,
				    byadf, byseg, &ops[1], &tidatt);
		if (subqry->ops_sqtype == OPS_HFAGG &&
		    ops[0].opr_dt == ops[1].opr_dt && 
		    ops[0].opr_len == ops[1].opr_len)
		{
		    /* If hash agg on date column, coerce from date to date
		    ** to apply status byte normalization (bug 109553). (TEXT
		    ** type, too.) */
		    if (abs(ops[1].opr_dt) == DB_DTE_TYPE ||
			abs(ops[1].opr_dt) == DB_TXT_TYPE)
			opc_adtransform(global, byadf, &ops[1], &ops[0], 
				byseg, FALSE, FALSE);
		    else opc_adinstr(global, byadf, opcode, byseg, 2, ops, 1);
		}
		else
		{
		    /* Some projection aggregates may not have same type/length
		    ** in source/target. Rdmove coerces them and fixes bug 107320. */
		    opc_rdmove(global, byval, ops, byadf, byseg, 
				(RDR_INFO *) NULL, TRUE);
		}
	    }
	}	/* end of byval loop */

	/* If hash aggregate, finish things up. */
	if (subqry->ops_sqtype == OPS_HFAGG)
	{
	    /* Build one big compare-list entry for the concatenated
	    ** by-values.  Note that we only get away with this because
	    ** the hash-agg runtime code is careful to zero out the
	    ** by-value (hash-value) materialization buffer, so that
	    ** any NULLs materialize as zeros plus the null-indicator.
	    ** Thus, two NULLs compare equal, as they should in by-lists.
	    ** (They also hash equal, which is equally important!)
	    */
	    hashkeys->cmp_offset = 0;
	    hashkeys->cmp_type = DB_BYTE_TYPE;
	    hashkeys->cmp_length = offset - keystart;
	    hashkeys->cmp_precision = 0;
	    hashkeys->cmp_collID = -1;
	    hashkeys->cmp_direction = 0;

	    opc_adend(global, hcadf);	/* the hashing CX is done */

	}
    }
}

/*{
** Name: OPC_MAGGTARG_MAIN	- Compile the main segment of an agg target list
**
** Description:
**      This routine compiles the main segment of an aggregate target list. 
**      The instructions in this segment perform two functions. First, if this
**	is an agg function then we compare the current set of values (the ones
**	that we just got from the QEN tree) with the ones that we materialized
**	in the init segment. The second step, which QEF/ADF only executes if
**	the first step is passed, is to compile the aggregate operator using
**	the current set of values from the QEN tree.
[@comment_line@]...
**
** Inputs:
**	global -
**	    This is the query wide state variable.
**	cnode -
**	    This describes where to get the data from the QEN tree.
**	aghead -
**	    This is the AGHEAD qnode for our agg target list.
**	cadf -
**	    This is the CX that we are compiling.
**	ocadf -
**	    OPC_ADF structure ptr for overflow row CX (hash agg)
**	tginfo -
**	    This describes the agg target list.
**	topsort -
**	    This is true if the top QEN tree is a topsort node.
**	byvals -
**	    This is a pointer to the first RESDOM for the by values if this 
**	    is a func agg. Otherwise, this is NULL.
**	sortinfo -
**	    This is a description of the info coming from the top sort node
**	    if topsort is TRUE.
**	workbase -
**	    This is the workarea base number for the CX.
**	outbase -
**	    This is the base number for the results of the aggregate.
**	qn -
**	    This is the top QEN node.
**	subexlistpp - 
**	    Ptr to list of common subexpressions extracted for preevaluation.
**
** Outputs:
**	cadf -
**	    The main segment of this CX will be completely filled in.
**
**	Returns:
**	    nothing
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-aug-87 (eric)
**          created
**	17-jul-89 (jrb)
**	    Set precision fields correctly.
**      21-july-89 (eric)
**          fixed bug 6275 by checking for non-printing resdoms when 
**	    compare the by vals. If a by val resdom is non-printing then
**          we don't compare it.
**      18-mar-92 (anitap)
**          As a fix for bug 42363, initialized opr_len and opr_prec to
**          unknown. This is relation to equivalence classes. See opc_cqual()
**          in OPCQUAL.C for details.
**	20-dec-00 (inkdo01)
**	    Changes to generate inline ADF code for aggregates & get rid of 
**	    Aggregate workspace structure.
**	6-feb-01 (inkdo01)
**	    Changes for hash aggregation overflow handling.
**	30-mar-01 (inkdo01)
**	    Changes to make stupid Quel simple aggs with no input rows work.
**	30-may-01 (inkdo01)
**	    Changes to roll f4 sum's into a f8.
**	21-june-01 (inkdo01)
**	    Changes to get binary aggregates working.
**	17-sep-01 (inkdo01)
**	    Fix SEGV in binary aggregate support.
**	6-feb-02 (inkdo01)
**	    Fix alignment for Unicode types (must be 2 byte aligned).
**	29-june-05 (inkdo01)
**	    Added code to optimize CXs.
**	25-Jul-2005 (schka24)
**	    Emit SETTRUE at end of CX so that sorted-aggf driver doesn't see
**	    a CX truth result that's been contaminated by CASE expr testing.
**	22-May-2006 (kschendel)
**	    SETTRUE in simple agg is pointless and screws up countstar
**	    optimization, which is still relevant for partitioned tables.
**	    (although maybe not for long.)
[@history_template@]...
*/
static VOID
opc_maggtarg_main(
	OPS_STATE   *global,
	OPC_NODE    *cnode,
	PST_QNODE   *aghead,
	OPC_ADF	    *cadf,
	OPC_ADF	    *ocadf,
	OPC_TGINFO  *tginfo,
	i4	    topsort,
	PST_QNODE   *byvals,
	OPC_TGINFO  *sortinfo,
	i4	    workbase, 
	i4	    outbase,
	i4	    oflwbase, 
	QEN_NODE    *qn,
	PST_QNODE   *subexlistp)
{
    OPS_SUBQUERY	*subqry = global->ops_cstate.opc_subqry;
    PST_QNODE		*byval;
    i4			rsno, naops;
    ADE_OPERAND		ops[4], opov, opov2, resop;
    i4			tidatt;
    i4			byvalno;
    i4			i;
    PST_QNODE		*aop_resdom, *aopparm;
    i4			aopno, opno;
    i4			nadfops;
    i4			oflwwork = oflwbase+1;
    i4			worklen, align, offset, agwslen;
    PST_QNODE		*aop;
    DB_DT_ID		bydt, resdt;
    bool		avg_std_var;
    bool		quelsagg = FALSE;
    bool		binagg;


    /* First compile evaluation of common subexpressions. */
    resop.opr_len = OPC_UNKNOWN_LEN;
    resop.opr_prec = OPC_UNKNOWN_PREC;
    if (subexlistp != (PST_QNODE *) NULL)
    opc_cqual(global, cnode, subexlistp, cadf, ADE_SMAIN, &resop, &tidatt);

    if ((subqry->ops_sqtype == OPS_SAGG || subqry->ops_sqtype == OPS_RSAGG)
	&& aghead->pst_sym.pst_value.pst_s_root.pst_qlang == DB_QUEL)
	quelsagg = TRUE;

    if (subqry->ops_sqtype == OPS_FAGG ||
	    subqry->ops_sqtype == OPS_RFAGG
	)
    {
	if (subqry->ops_sqtype == OPS_HFAGG)
	{
	    /* Hash agg starts keys (byvals) after hash row header; this
	    ** is guaranteed to be worst-case aligned.
	    */
	    offset = QEF_HASH_DUP_SIZE;
	}
	else
	{
	    offset = 0;		/* Sorted agg starts byvals at zero */
	}

	/* for (each byval) */
	for (byval = byvals;
		byval != NULL && byval->pst_sym.pst_type != PST_TREE;
		byval = byval->pst_left
	    )
	{
	    /* if this byval is a non-printing resdom, then don't 
	    ** materialize it (by definition).
	    */
	    if (!(byval->pst_sym.pst_value.pst_s_rsdm.pst_rsflags&PST_RS_PRINT))
	    {
		continue;
	    }

	    /* materialize the byval; */
	    if (topsort == TRUE)
	    {
		rsno = byval->pst_sym.pst_value.pst_s_rsdm.pst_rsno;

		ops[1].opr_dt = sortinfo->opc_tsoffsets[rsno].opc_tdatatype;
		ops[1].opr_len = sortinfo->opc_tsoffsets[rsno].opc_tlength;
		ops[1].opr_offset = sortinfo->opc_tsoffsets[rsno].opc_toffset;
		ops[1].opr_base = cadf->opc_row_base[qn->qen_row];
		ops[1].opr_prec = sortinfo->opc_tsoffsets[rsno].opc_tprec;
		ops[1].opr_collID = sortinfo->opc_tsoffsets[rsno].opc_tcollID;
	    }
	    else
	    {
		ops[1].opr_dt = byval->pst_sym.pst_dataval.db_datatype;
		ops[1].opr_len = OPC_UNKNOWN_LEN;
                ops[1].opr_prec = OPC_UNKNOWN_PREC;
		opc_cqual(global, cnode, byval->pst_right,
					    cadf, ADE_SMAIN, &ops[1], &tidatt);
	    }

	    /* compare the current and materialized byval for equality; */
	    byvalno = byval->pst_sym.pst_value.pst_s_rsdm.pst_rsno;
	    ops[0].opr_base = workbase;
	    bydt = ops[0].opr_dt = 
			(i4) tginfo->opc_tsoffsets[byvalno].opc_tdatatype;
	    ops[0].opr_len = (i4) tginfo->opc_tsoffsets[byvalno].opc_tlength;
	    ops[0].opr_prec = (i4) tginfo->opc_tsoffsets[byvalno].opc_tprec;
	    ops[0].opr_collID = tginfo->opc_tsoffsets[byvalno].opc_tcollID;

	    /* Align the BY column. */
	    switch (bydt) {
	      case DB_INT_TYPE:
	      case DB_FLT_TYPE:
	      case DB_MNY_TYPE:
		worklen = ops[0].opr_len;
		break;

	      case DB_NCHR_TYPE:
	      case DB_NVCHR_TYPE:
	      case -DB_NCHR_TYPE:
	      case -DB_NVCHR_TYPE:
		worklen = sizeof(UCS2);
		break;

	      case -DB_INT_TYPE:
	      case -DB_FLT_TYPE:
	      case -DB_MNY_TYPE:
		worklen = ops[0].opr_len - 1;
		break;

	      case DB_DTE_TYPE:
	      case DB_ADTE_TYPE:
	      case DB_TMWO_TYPE:
	      case DB_TMW_TYPE:
	      case DB_TME_TYPE:
	      case DB_TSWO_TYPE:
	      case DB_TSW_TYPE:
	      case DB_TSTMP_TYPE:
	      case DB_INYM_TYPE:
	      case DB_INDS_TYPE:
	      case -DB_DTE_TYPE:
	      case -DB_ADTE_TYPE:
	      case -DB_TMWO_TYPE:
	      case -DB_TMW_TYPE:
	      case -DB_TME_TYPE:
	      case -DB_TSWO_TYPE:
	      case -DB_TSW_TYPE:
	      case -DB_TSTMP_TYPE:
	      case -DB_INYM_TYPE:
	      case -DB_INDS_TYPE:
		worklen = sizeof(i4);
		break;

	      case DB_PAT_TYPE:
	      case -DB_PAT_TYPE:
		worklen = sizeof(i2);
		break;

	      default:
		worklen = 1;
		break;
	    }

	    align = offset % worklen;
	    if (align != 0) offset += (worklen - align);
	    ops[0].opr_offset = offset;
	    offset += ops[0].opr_len;

	    if (ops[0].opr_dt > 0) opno = ADE_COMPARE;
	    else opno = ADE_BYCOMPARE;
	    opc_adinstr(global, cadf, opno, ADE_SMAIN, 2, ops, 0);
	}
    }

    /* for (each aop) */
    for (aop_resdom = aghead->pst_left;
	    (aop_resdom != NULL && 
		aop_resdom->pst_sym.pst_type != PST_TREE && 
		aop_resdom != byvals
	    );
	    aop_resdom = aop_resdom->pst_left
	)
    {
	if (!(aop_resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsflags&PST_RS_PRINT))
	{
	    continue;
	}

	aop = aop_resdom->pst_right;
	aopparm = aop->pst_left;
	nadfops = aop->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_numargs;
	nadfops += 1;
	aopno = aop->pst_sym.pst_value.pst_s_op.pst_opno;
	rsno = aop_resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsno;
	avg_std_var = (aopno == ADI_AVG_OP || aopno == ADI_STDDEV_SAMP_OP ||
		aopno == ADI_SINGLETON_OP ||
		aopno == ADI_STDDEV_POP_OP || aopno == ADI_VAR_SAMP_OP ||
		aopno == ADI_VAR_POP_OP);
	binagg = aop->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_fiflags &
			ADI_F16_OLAPAGG && aop->pst_right != NULL;

	/* materialize the expression below the aop; */
	if (nadfops > 1)
	{
	    if (topsort == TRUE)
	    {
		ops[1].opr_dt = sortinfo->opc_tsoffsets[rsno].opc_tdatatype;
		ops[1].opr_len = sortinfo->opc_tsoffsets[rsno].opc_tlength;
		ops[1].opr_offset = sortinfo->opc_tsoffsets[rsno].opc_toffset;
		ops[1].opr_base = cadf->opc_row_base[qn->qen_row];
		ops[1].opr_prec = sortinfo->opc_tsoffsets[rsno].opc_tprec;
	    }
	    else
	    {
		ops[1].opr_dt = aop_resdom->pst_right->
				pst_sym.pst_value.pst_s_op.pst_fdesc->adi_dt[0];

		if (ops[1].opr_dt == DB_ALL_TYPE)
		{
		    ops[1].opr_dt = aop_resdom->pst_right->pst_left->
					    pst_sym.pst_dataval.db_datatype;
		}

		ops[1].opr_len = OPC_UNKNOWN_LEN;
                ops[1].opr_prec = OPC_UNKNOWN_PREC;
		opc_cqual(global, cnode, aop_resdom->pst_right->pst_left,
					    cadf, ADE_SMAIN, &ops[1], &tidatt);

		if (nadfops > 2 && binagg)
		{
		    /* Binary OLAP aggregate - prepare 2nd operand, too. */
		    ops[2].opr_dt = aop_resdom->pst_right->
				pst_sym.pst_value.pst_s_op.pst_fdesc->adi_dt[0];

		    if (ops[2].opr_dt == DB_ALL_TYPE)
		    {
			ops[2].opr_dt = aop_resdom->pst_right->pst_left->
					    pst_sym.pst_dataval.db_datatype;
		    }

		    ops[2].opr_len = OPC_UNKNOWN_LEN;
                    ops[2].opr_prec = OPC_UNKNOWN_PREC;
		    opc_cqual(global, cnode, aop_resdom->pst_right->pst_right,
					    cadf, ADE_SMAIN, &ops[2], &tidatt);
		}
	    }

	    /* For hash aggregates, locate source value in overflow buffer. */
	    if (subqry->ops_sqtype == OPS_HFAGG && aopparm)
	    {
		opov.opr_dt = aopparm->pst_sym.pst_dataval.db_datatype;
		opov.opr_len = aopparm->pst_sym.pst_dataval.db_length;
		opov.opr_offset = tginfo->opc_tsoffsets[rsno].opc_aggooffset;
		opov.opr_base = oflwbase;
		opov.opr_prec = aopparm->pst_sym.pst_dataval.db_prec;
	    }

	    /* For OLAP binary hash aggregates, locate 2nd source value 
	    ** in overflow buffer. */
	    if (subqry->ops_sqtype == OPS_HFAGG && binagg)
	    {
		PST_QNODE	*aopparm2 = aop->pst_right;

		opov2.opr_dt = aopparm2->pst_sym.pst_dataval.db_datatype;
		opov2.opr_len = aopparm2->pst_sym.pst_dataval.db_length;
		opov2.opr_offset = tginfo->opc_tsoffsets[rsno].opc_aggooffset2;
		opov2.opr_base = oflwbase;
		opov2.opr_prec = aopparm2->pst_sym.pst_dataval.db_prec;
	    }
	}

	/* evaluate the aop using the materialized expression; */
	ops[0].opr_base = workbase;
	ops[0].opr_offset = tginfo->opc_tsoffsets[rsno].opc_aggwoffset;

	/* Check for special aggregate workfield and setup ops[0] acc. */
	if ((agwslen = aop->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_agwsdv_len)
		== 0 || 
	    !(aop->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_fiflags & ADI_F4_WORKSPACE)) 
	{
	    ops[0].opr_dt = aop_resdom->pst_sym.pst_dataval.db_datatype;
	    ops[0].opr_len = aop_resdom->pst_sym.pst_dataval.db_length;
	    ops[0].opr_prec = aop_resdom->pst_sym.pst_dataval.db_prec;
	    agwslen = 0;

	    if (ops[0].opr_dt > 0 && !avg_std_var &&
		aopparm && aopparm->pst_sym.pst_dataval.db_datatype < 0)
	    {
		/* If agg source is nullable, workfield should be nullable. */
		ops[0].opr_dt = -ops[0].opr_dt;
		ops[0].opr_len++;
	    }
	}
	else
	{
	    ops[0].opr_dt = DB_BYTE_TYPE;
	    ops[0].opr_len = agwslen;
	    ops[0].opr_prec = 0;
	}

	/* Check for sum(f4) and change accumulator to f8. */
	if (aopno == ADI_SUM_OP && abs(ops[0].opr_dt) == DB_FLT_TYPE &&
	    ops[0].opr_len < 8)
	    ops[0].opr_len += 4;

	/* Set up extra operand(s) for avg, stddev, variance aggs. */
	if (avg_std_var)
	{
	    /* Locate next parm (aligned). */
	    if (aopno == ADI_AVG_OP || aopno == ADI_SINGLETON_OP) 
	    {
		ops[2].opr_dt = DB_INT_TYPE;
		ops[2].opr_len = sizeof(i4);
		ops[2].opr_prec = 0;
		ops[2].opr_base = workbase;
	    }
	    else /* stddev or variance */
	    {
		ops[2].opr_dt = DB_FLT_TYPE;
		ops[2].opr_len = sizeof(f8);
		ops[2].opr_prec = 0;
		ops[2].opr_base = workbase;
	    }
	    worklen = ops[2].opr_len;
	    offset = ops[0].opr_offset + ops[0].opr_len;
	    align = offset % worklen;
	    if (align != 0) offset += (worklen - align);
	    ops[2].opr_offset = offset;
	    nadfops++;

	    /* Finally, if stddev/variance, one more parm. */
	    if (aopno != ADI_AVG_OP && aopno != ADI_SINGLETON_OP)
	    {
		ops[3].opr_dt = DB_INT_TYPE;
		ops[3].opr_len = sizeof(i4);
		ops[3].opr_prec = 0;
		ops[3].opr_base = workbase;
		ops[3].opr_offset = offset + sizeof(f8);
		nadfops++;
	    }
	}

	if (quelsagg && ops[0].opr_dt > 0)
	{
	    /* Make Quel not null look like nullable. */
	    ops[0].opr_dt = -ops[0].opr_dt;
	    ops[0].opr_len++;
	}
	opc_adinstr(global, cadf, 
		aop_resdom->pst_right->pst_sym.pst_value.pst_s_op.pst_opinst, 
						    ADE_SMAIN, nadfops, ops, 1);

	/* If this is hash aggregate, insert same aggregation code in 
	** ahd_agoflow for aggregating from the overflow buffer. 
	** Note that if either operand data type differs from what the
	** function instance expects, the operand must be coerced first. */
	if (subqry->ops_sqtype == OPS_HFAGG)
	{
	    ops[0].opr_base = ops[2].opr_base = ops[3].opr_base = oflwwork;
						/* set agg work buffs */
	    if (nadfops > 1) 
	    {
		STRUCT_ASSIGN_MACRO(opov, ops[1]);
		if (abs(opov.opr_dt) != aop->pst_sym.pst_value.pst_s_op.pst_fdesc->
			adi_dt[0] && 
		    aop->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_dt[0] !=
			DB_ALL_TYPE)
		{
		    opov.opr_dt = (opov.opr_dt > 0) ? 
			aop->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_dt[0] :
			-aop->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_dt[0];
		    opc_adtransform(global, ocadf, &ops[1], &opov, ADE_SMAIN,
							TRUE, FALSE);
		    STRUCT_ASSIGN_MACRO(opov, ops[1]);
		}
	    }
	    if (binagg)
	    {
		/* Binary OLAP aggregate - do same for 2nd operand. */
		STRUCT_ASSIGN_MACRO(opov2, ops[2]);
		if (abs(opov2.opr_dt) != aop->pst_sym.pst_value.pst_s_op.pst_fdesc->
			adi_dt[1] &&
		    aop->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_dt[1] !=
			DB_ALL_TYPE)
		{
		    opov2.opr_dt = (opov2.opr_dt > 0) ? 
			aop->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_dt[1] :
			-aop->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_dt[1];
		    opc_adtransform(global, ocadf, &ops[2], &opov2, ADE_SMAIN,
							TRUE, FALSE);
		    STRUCT_ASSIGN_MACRO(opov2, ops[2]);
		}
	    }
	    opc_adinstr(global, ocadf, 
		aop_resdom->pst_right->pst_sym.pst_value.pst_s_op.pst_opinst, 
						    ADE_SMAIN, nadfops, ops, 1);
	}
    }
    if (subqry->ops_sqtype == OPS_HFAGG)
	opc_adend(global, ocadf);	/* end overflow CX */
    else if (subqry->ops_sqtype != OPS_SAGG)
    {
	/* For sorted aggs, the driver depends on the truth result of the
	** CX to decide whether the group is finished or not.  It thinks
	** that the truth value is from the BYCOMPARE's.  But, if the agg
	** accumulation contained CASE expressions, the CASE testing has
	** potentially changed the CX truth result!  If we get thru the
	** agg accumulation code, the BYCOMPARE's were obviously all TRUE.
	** So, emit a SETTRUE to reset the CX result back to TRUE.
	** Ideally we would omit this if there were no CASE exprs in the
	** accumulator part of the CX.
	*/
	opc_adinstr(global, cadf, ADE_SETTRUE, ADE_SMAIN, 0, 0, 0);
    }
}

/*{
** Name: OPC_FAGGTARG_FINIT	- Compile the finit segment of an agg 
**								target list
**
** Description:
**      This routine compiles the finit segment of an aggregate target list. 
**      The instructions in this segment performs the agend function that
**	is required for complete aggregate processing.
[@comment_line@]...
**
** Inputs:
**	global -
**	    This is the query wide state variable.
**	ahd_qepp -
**	    This is the aggregate action header being built.
**	aghead -
**	    This is the AGHEAD qnode for our agg target list.
**	cadf -
**	    This is the CX that we are compiling.
**	tginfo -
**	    This describes the aggregate target list.
**	byvals -
**	    This is a pointer to the first RESDOM for the by values if this 
**	    is a func agg. Otherwise, this is NULL.
**	workbase -
**	    This is the workarea base number for the CX.
**	outbase -
**	    This is the base number for the results of the aggregate.
**
** Outputs:
**	cadf -
**	    The finit segment of this CX will be completely filled in.
**
**	Returns:
**	    nothing
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-aug-87 (eric)
**          created
**	17-jul-89 (jrb)
**	    Set precision fields correctly.
**	20-dec-00 (inkdo01)
**	    Changes to generate inline ADF code for aggregates & get rid of 
**	    Aggregate workspace structure.
**	30-mar-01 (inkdo01)
**	    Changes to make stupid Quel simple aggs with no input rows work.
**	30-may-01 (inkdo01)
**	    Changes to roll f4 sum's into a f8.
**	19-june-01 (inkdo01)
**	    SAGGs use AOPs embedded pst_opparm instead of a PST_CONST addr'ed
**	    by pst_right (so binary aggs work ok).
**	21-june-01 (inkdo01)
**	    Changes to get binary aggregates working.
**	30-aug-01 (inkdo01)
**	    Changed AGEND to OLAGEND to save confusion.
**	17-sep-01 (inkdo01)
**	    Fix SEGV in binary aggregate support.
**	4-oct-01 (inkdo01)
**	    Remove alignment padding for hash agg. BY cols.
**      29-jun-2004 (huazh01)
**          Extend the fix for bug 110610 so that it covers
**          problem INGSRV2883, bug 112573.  
**	31-aug-04 (inkdo01)
**	    Replace ahd_agwbuf/hbuf/obuf/owbuf with global indexes.
[@history_template@]...
*/
static VOID
opc_faggtarg_finit(
	OPS_STATE   *global,
	QEF_QEP	    *ahd_qepp,
	PST_QNODE   *aghead,
	OPC_ADF	    *cadf,
	OPC_TGINFO  *tginfo,
	PST_QNODE   *byvals,
	i4	    workbase, 
	i4	    outbase)
{
    OPS_SUBQUERY	*subqry = global->ops_cstate.opc_subqry;
    PST_QNODE		*aop_resdom;
    PST_QNODE		*byval;
    i4			pno;	/* pst_parm_no */
    ADE_OPERAND		ops[4];
    i4			rsno, byvalno;
    i4			opno, aopno;
    i4			nops;
    i4			worklen, align, offset, agwslen;
    PST_QNODE		*aop, *aopparm;
    DB_DT_ID		bydt;
    bool		avg_std_var;
    bool		quelsagg = FALSE;
    bool		f8tof4;
    bool		binagg;

    /* Copy byvals to output (if necessary). */
    if (subqry->ops_sqtype == OPS_FAGG ||
	    subqry->ops_sqtype == OPS_RFAGG ||
	    subqry->ops_sqtype == OPS_HFAGG
	)
    {
	if (subqry->ops_sqtype == OPS_HFAGG)
	{
	    /* Hash agg starts keys (byvals) after hash row header; this
	    ** is guaranteed to be worst-case aligned.
	    */
	    offset = QEF_HASH_DUP_SIZE;
	}
	else
	{
	    offset = 0;			/* Sorted agg starts byvals at zero */
	}

	/* for (each byval) */
	for (byval = byvals;
		byval != NULL && byval->pst_sym.pst_type != PST_TREE;
		byval = byval->pst_left
	    )
	{
	    /* if this byval is a non-printing resdom, then don't 
	    ** materialize it (by definition).
	    */
	    if (!(byval->pst_sym.pst_value.pst_s_rsdm.pst_rsflags&PST_RS_PRINT))
	    {
		continue;
	    }

	    /* Setup source (in work buffer) and target (in out buffer) operands. */
	    byvalno = byval->pst_sym.pst_value.pst_s_rsdm.pst_rsno;
	    ops[0].opr_base = outbase;
	    ops[0].opr_dt = (i4) tginfo->opc_tsoffsets[byvalno].opc_tdatatype;
	    ops[0].opr_len = (i4) tginfo->opc_tsoffsets[byvalno].opc_tlength;
	    ops[0].opr_prec = (i4) tginfo->opc_tsoffsets[byvalno].opc_tprec;
	    ops[0].opr_offset = (i4) tginfo->opc_tsoffsets[byvalno].opc_toffset;

	    ops[1].opr_base = workbase;
	    bydt = ops[1].opr_dt = 
			(i4) tginfo->opc_tsoffsets[byvalno].opc_tdatatype;
	    ops[1].opr_len = (i4) tginfo->opc_tsoffsets[byvalno].opc_tlength;
	    ops[1].opr_prec = (i4) tginfo->opc_tsoffsets[byvalno].opc_tprec;

	    if (subqry->ops_sqtype != OPS_HFAGG)
	    {
		/* For non hash aggs., align the BY column. */
		switch (bydt) {
		  case DB_INT_TYPE:
		  case DB_FLT_TYPE:
		  case DB_MNY_TYPE:
		    worklen = ops[1].opr_len;
		    break;

		  case -DB_INT_TYPE:
		  case -DB_FLT_TYPE:
		  case -DB_MNY_TYPE:
		    worklen = ops[1].opr_len - 1;
		    break;

                  case DB_NCHR_TYPE:
                  case DB_NVCHR_TYPE:
                  case -DB_NCHR_TYPE:
                  case -DB_NVCHR_TYPE:
                    worklen = sizeof(UCS2);
                    break;

		  case DB_DTE_TYPE:
		  case DB_ADTE_TYPE:
		  case DB_TMWO_TYPE:
		  case DB_TMW_TYPE:
		  case DB_TME_TYPE:
		  case DB_TSWO_TYPE:
		  case DB_TSW_TYPE:
		  case DB_TSTMP_TYPE:
		  case DB_INYM_TYPE:
		  case DB_INDS_TYPE:
		  case -DB_DTE_TYPE:
		  case -DB_ADTE_TYPE:
		  case -DB_TMWO_TYPE:
		  case -DB_TMW_TYPE:
		  case -DB_TME_TYPE:
		  case -DB_TSWO_TYPE:
		  case -DB_TSW_TYPE:
		  case -DB_TSTMP_TYPE:
		  case -DB_INYM_TYPE:
		  case -DB_INDS_TYPE:
		    worklen = sizeof(i4);
		    break;

		  case DB_PAT_TYPE:
		  case -DB_PAT_TYPE:
		    worklen = sizeof(i2);
		    break;

		  default:
		    worklen = 1;
		    break;
		}

		align = offset % worklen;
		if (align != 0) offset += (worklen - align);
	    }

	    ops[1].opr_offset = offset;
	    offset += ops[1].opr_len;

	    opc_adinstr(global, cadf, ADE_MECOPY, ADE_SFINIT, 2, ops, 0);
	}
    }
    else if (aghead->pst_sym.pst_value.pst_s_root.pst_qlang == DB_QUEL)
	quelsagg = TRUE;

    /* for (each aop) */
    for (aop_resdom = aghead->pst_left;
	    (aop_resdom != NULL && 
		aop_resdom->pst_sym.pst_type != PST_TREE && 
		aop_resdom != byvals
	    );
	    aop_resdom = aop_resdom->pst_left
	)
    {
	if (!(aop_resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsflags&PST_RS_PRINT))
	{
	    continue;
	}

	aop = aop_resdom->pst_right;
	aopparm = aop->pst_left;
	opno = aop->pst_sym.pst_value.pst_s_op.pst_opinst;
	aopno = aop->pst_sym.pst_value.pst_s_op.pst_opno;
	avg_std_var = (aopno == ADI_AVG_OP || aopno == ADI_STDDEV_SAMP_OP ||
		aopno == ADI_SINGLETON_OP ||
		aopno == ADI_STDDEV_POP_OP || aopno == ADI_VAR_SAMP_OP ||
		aopno == ADI_VAR_POP_OP);
	binagg = aop->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_fiflags &
			ADI_F16_OLAPAGG && aop->pst_right != NULL;

	/* compile the agend instr; */
	if (subqry->ops_sqtype == OPS_SAGG)
	{
	    pno = aop->pst_sym.pst_value.pst_s_op.pst_opparm;
	    rsno = aop_resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsno;
	    
	    if (global->ops_procedure->pst_isdbp == TRUE)
	    {
		opc_lvar_row(global, pno, cadf, &ops[0]);
	    }
	    else
	    {
		opc_ptparm(global, aop, -1, -1);
		/* opc_ptparm(global, aop->pst_right, -1, -1); */
		opc_adbase(global, cadf, QEN_PARM, pno);

		ops[0].opr_base = cadf->opc_repeat_base[pno];
		ops[0].opr_offset = 
			    global->ops_cstate.opc_rparms[pno].opc_offset;
		ops[0].opr_dt = aop_resdom->pst_sym.pst_dataval.db_datatype;
		ops[0].opr_len = aop_resdom->pst_sym.pst_dataval.db_length;
		ops[0].opr_prec = aop_resdom->pst_sym.pst_dataval.db_prec;
	    }
	}
	else
	{
	    rsno = aop_resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsno;
	    ops[0].opr_base = outbase;
	    ops[0].opr_offset = (i4) tginfo->opc_tsoffsets[rsno].opc_toffset;
	    ops[0].opr_dt = (i4) tginfo->opc_tsoffsets[rsno].opc_tdatatype;
	    ops[0].opr_len = (i4) tginfo->opc_tsoffsets[rsno].opc_tlength;
	    ops[0].opr_prec = (i4) tginfo->opc_tsoffsets[rsno].opc_tprec;
	}

	/* Compute source operand (in work buffer). */
	ops[1].opr_base = workbase;
	ops[1].opr_offset = tginfo->opc_tsoffsets[rsno].opc_aggwoffset;

	/* Check for special aggregate workfield and setup ops[1] acc. */
	if ((agwslen = aop->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_agwsdv_len)
		== 0 || 
	    !(aop->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_fiflags & ADI_F4_WORKSPACE)) 
	{
	    ops[1].opr_dt = aop_resdom->pst_sym.pst_dataval.db_datatype;
	    ops[1].opr_len = aop_resdom->pst_sym.pst_dataval.db_length;
	    ops[1].opr_prec = aop_resdom->pst_sym.pst_dataval.db_prec;
	    agwslen = 0;
	    if (ops[1].opr_dt > 0 && !avg_std_var && 
		aopparm && aopparm->pst_sym.pst_dataval.db_datatype < 0)
	    {
		/* If agg source is nullable, workfield should be nullable. */
		ops[1].opr_dt = -ops[1].opr_dt;
		ops[1].opr_len++;
	    }
	}
	else
	{
	    ops[1].opr_dt = DB_BYTE_TYPE;
	    /* This is actually agwslen - but adecompile requires the length
	    ** to match the type. Oh, and the type isn't FLT either. It's
	    ** actually some composite type for OLAP binary aggs or for
	    ** avg(date), but again adecompile requires FLT ('cause that's 
	    ** how it is defined in the F.I. table). All this is because the 
	    ** operands get flipped between the "next" code and the "finit"
	    ** code. */
	    ops[1].opr_len = agwslen;
	    ops[1].opr_prec = 0;
	}

	/* Check for sum(f4) and change accumulator to f8. */
	if (aopno == ADI_SUM_OP && abs(ops[1].opr_dt) == DB_FLT_TYPE &&
	    ops[1].opr_len < 8)
	{
	    ops[1].opr_len += 4;
	    f8tof4 = TRUE;
	}
	else f8tof4 = FALSE;

	nops = 2;

	/* Avg, stddev, variance have extra parms, binary aggregates use 
	** ADE_OLAGEND opcode with aopno as extra parm. */
	if (!avg_std_var && !binagg) 
	    opno = ADE_MECOPY;
	else if (binagg)
	{
	    DB_DATA_VALUE	intdv;

	    intdv.db_datatype = DB_INT_TYPE;
	    intdv.db_length = 4;
	    intdv.db_prec = 0;
	    intdv.db_data = (char *)&opno;

	    opc_adconst(global, cadf, &intdv, &ops[2], DB_SQL, ADE_SFINIT);
					/* allocate int opno constant */
	    opno = ADE_OLAGEND;
	    nops = 3;
	}
	else
	{
	    if (aopno == ADI_AVG_OP) 
	    {
		opno = ADE_AVG;
		worklen = sizeof(i4);
		nops = 3;
		ops[2].opr_dt = DB_INT_TYPE;
	    }
	    else if (aopno == ADI_SINGLETON_OP) 
	    {
		opno = ADE_SINGLETON;
		worklen = sizeof(i4);
		nops = 3;
		ops[2].opr_dt = DB_INT_TYPE;
	    }
	    else 
	    {
		worklen = sizeof(f8);
		ops[2].opr_dt = DB_FLT_TYPE;
	    }

	    offset = ops[1].opr_offset + ops[1].opr_len;
	    align = offset % worklen;
	    if (align != 0) offset += (worklen - align);
	    ops[2].opr_base = workbase;
	    ops[2].opr_offset = offset;
	    ops[2].opr_len = worklen;
	    ops[2].opr_prec = 0;

	    if (aopno != ADI_AVG_OP && aopno != ADI_SINGLETON_OP)
	    {
		STRUCT_ASSIGN_MACRO(ops[2], ops[3]);
		ops[3].opr_offset += sizeof(f8);
		ops[3].opr_dt = DB_INT_TYPE;
		ops[3].opr_len = sizeof(i4);
		nops = 4;
	    }
	}

	/* If max/min and work field is nullable and result isn't, check for 
	** null result (so error can be issued). */
	if ((aopno == ADI_MAX_OP || aopno == ADI_MIN_OP) &&
		ops[0].opr_dt > 0 && ops[1].opr_dt < 0 &&
		aghead->pst_sym.pst_value.pst_s_root.pst_qlang != DB_QUEL)
		opc_adinstr(global, cadf, ADE_NULLCHECK, ADE_SFINIT, nops, ops, 1);

	/* Hack for stupid Quel. Quel returns empty value (0, blank, etc.) for
	** aggregates on empty set, rather than null (as SQL does). This used to
	** be done by ADE_AGEND/NAGEND operators, but with inline aggregation 
	** code, tricks are needed. The hack is that an empty value is copied
	** to the result field before the computed aggregate FOR QUEL ONLY. Then
	** a special MECOPY is executed which gets skipped if the source value 
	** is null (leaving the empty value result). */
	if (aghead->pst_sym.pst_value.pst_s_root.pst_qlang == DB_QUEL &&
		subqry->ops_sqtype == OPS_SAGG &&
		(opno == ADE_MECOPY && (ops[1].opr_dt < 0 || quelsagg)
		|| opno == ADE_AVG))
	{
	    DB_DATA_VALUE	emptydv;
	    ADE_OPERAND		saveop;
	    bool		notnull = (ops[1].opr_dt > 0);

	    STRUCT_ASSIGN_MACRO(ops[1], saveop);
	    STRUCT_ASSIGN_MACRO(ops[0], ops[1]);
	    emptydv.db_data = NULL;	/* part of opc_adempty protocol */
	    opc_adempty(global, &emptydv, ops[1].opr_dt, 
			ops[1].opr_prec, ops[1].opr_len);
	    opc_adconst(global, cadf, &emptydv, &ops[1], DB_QUEL, ADE_SFINIT);
	    opc_adinstr(global, cadf, ADE_MECOPY, ADE_SFINIT, 2, ops, 1);
	    STRUCT_ASSIGN_MACRO(saveop, ops[1]);
	    if (quelsagg && notnull)
	    {
		ops[1].opr_dt = -ops[1].opr_dt;
					/* make it look nullable for Quel */
		ops[1].opr_len++;
	    }
	    if (opno == ADE_AVG)
		opno = ADE_AVGQ;
	    else
	    {
		opno = ADE_MECOPYQA;
		if (f8tof4)
		{
		    /* even more hackery if sum(f4) which is how this flag
		    ** gets set.  Coerce the sum f8 (which is made to look
		    ** nullable, per above) into a nullable f4 temp, and
		    ** then do our quel-hack MECOPYQA from that temp into
		    ** the target.  It would be simpler if we could generate
		    ** a bare ADE_2CXI_SKIP but I don't think we can.
		    */
		    saveop.opr_dt = -DB_FLT_TYPE;
		    saveop.opr_len = sizeof(f4) + 1;
		    saveop.opr_prec = 0;
		    saveop.opr_base = cadf->opc_row_base[OPC_CTEMP];
		    saveop.opr_offset = global->ops_cstate.opc_temprow->opc_olen;
		    global->ops_cstate.opc_temprow->opc_olen += DB_ALIGN_MACRO(saveop.opr_len);
		    opc_adtransform(global, cadf, &ops[1], &saveop, ADE_SFINIT, FALSE, FALSE);
		    STRUCT_ASSIGN_MACRO(saveop, ops[1]);
		    f8tof4 = FALSE;
		}
	    }
	}

	if (f8tof4)
	    opc_adtransform(global, cadf, &ops[1], &ops[0], ADE_SFINIT,
			FALSE, FALSE);
	else opc_adinstr(global, cadf, opno, ADE_SFINIT, nops, ops, 1);
    }

    /* end the adf; */
    opc_adend(global, cadf);

    /* If global base array option and this is a hash aggregate, replace
    ** ahd_agwbuf/hbuf/obuf/owbuf with their respective global indexes. */
    if (global->ops_cstate.opc_qp->qp_status & QEQP_GLOBAL_BASEARRAY &&
	    subqry->ops_sqtype == OPS_HFAGG)
    {
	ahd_qepp->u1.s2.ahd_agwbuf = ahd_qepp->ahd_current->
		qen_base[ahd_qepp->u1.s2.ahd_agwbuf-ADE_ZBASE].qen_index;
	ahd_qepp->u1.s2.ahd_aghbuf = ahd_qepp->ahd_by_comp->
		qen_base[ahd_qepp->u1.s2.ahd_aghbuf-ADE_ZBASE].qen_index;
	ahd_qepp->u1.s2.ahd_agobuf = ahd_qepp->u1.s2.ahd_agoflow->
		qen_base[ahd_qepp->u1.s2.ahd_agobuf-ADE_ZBASE].qen_index;
	ahd_qepp->u1.s2.ahd_agowbuf = ahd_qepp->u1.s2.ahd_agoflow->
		qen_base[ahd_qepp->u1.s2.ahd_agowbuf-ADE_ZBASE].qen_index;
    }
}

/*{
** Name: OPC_AGPROJECT	- Do the projection processing for an agg func.
**
** Description:
**      This routine initializes and compiles all the instructions needed 
**      for projection processing for aggregates. This involves three steps.
**      First, we need to begin the compilation and initialize of local 
**      vars. Next, we compile instructions into the init segment that 
**	compares the by values 
**      in the aggregate target list row with the prevously projected by vals
**      previously projected. Finally, we compile instrs into the main segment
**	that project default by and agg values into the project row.
[@comment_line@]...
**
** Inputs:
**	global -
**	    This query wide state variable
**	cnode -
**	    This describes the current state of compiling the QEN tree. It
**	    is used mostly to find where the data the QEN trees returns is.
**	qadf -
**	    This is a pointer to an uninitialized QEN_ADF struct.
**	agg_row -
**	    This is the DSH row number for the aggregate target list tuple.
**	prjsrc_row -
**	    This is the DSH row number where we should place projected agg
**	    tuples.
**
** Outputs:
**	qadf -
**	    This will be completely initialized and compiled
**
**	Returns:
**	    nothing
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      3-oct-86 (eric)
**          written
**      10-nov-95 (inkdo01)
**	    Changes to support replacement of QEN_ADF instances by pointers.
[@history_template@]...
*/
VOID
opc_agproject(
		OPS_STATE	*global,
		OPC_NODE	*cnode,
		QEN_ADF		**qadf,
		i4		agg_row,
		i4		prjsrc_row)
{
    OPS_SUBQUERY	*subqry = global->ops_cstate.opc_subqry;
    i1			*dmap;
    RDR_INFO		*rel;
    PST_QNODE		*byvals;
    PST_QNODE		*byval;
    i4			nbyvals;
    i4			ninstr, nops, nconst;
    i4		szconst;
    i4			pout_row;
    i4			pout_base;
    i4			agg_base;
    ADE_OPERAND		ops[2];
    OPC_ADF		cadf;
    i4			projsrc_base;

    /* init some variables that will keep line length down. */
    rel = global->ops_rangetab.opv_base->
				    opv_grv[subqry->ops_result]->opv_relation;
    byvals = subqry->ops_byexpr;
    nbyvals = 0;
    for (byval = byvals;
	    byval != NULL && byval->pst_sym.pst_type != PST_TREE;
	    byval = byval->pst_left
	)
    {
	if (!(byval->pst_sym.pst_value.pst_s_rsdm.pst_rsflags&PST_RS_PRINT))
	{
	    continue;
	}

	nbyvals += 1;
    }

    /* estimate the size of the cx */
    ninstr = 0;
    nops = 0;
    nconst = 0;
    szconst = 0;
    dmap = opc_estdefault(global, byvals, &ninstr, &nops, &nconst, &szconst);
    ninstr += 2 * (nbyvals + 1);
    nops += (4 * nbyvals) + 2;

    /* begin the adf; */
    opc_adalloc_begin(global, &cadf, qadf, 
		    ninstr, nops, nconst, szconst, 4, OPC_STACK_MEM);

    /* add the agg project output row */
    opc_ptrow(global, &pout_row, (i4)rel->rdr_rel->tbl_width);
    (*qadf)->qen_output = pout_row;
    opc_adbase(global, &cadf, QEN_ROW, pout_row);
    pout_base = cadf.opc_row_base[pout_row];

    /* add the row that holds the agg target list */
    opc_adbase(global, &cadf, QEN_ROW, agg_row);
    agg_base = cadf.opc_row_base[agg_row];

    /* add the row that hold the projected by vals */
    opc_adbase(global, &cadf, QEN_ROW, prjsrc_row);
    projsrc_base = cadf.opc_row_base[prjsrc_row];

    /* compile the init (and some of the main) seqment */

    /* compile comparisons between the byvals in the agg row and project row; */
    opc_bycompare(global, &cadf, byvals, rel, pout_base, 
					    agg_base, projsrc_base);
    opc_adinstr(global, &cadf, ADE_AND, ADE_SINIT, 0, ops, 0);

    /* compile the main seqment */

    /* compile default values into the aop atts; */
    opc_cdefault(global, dmap, pout_base, &cadf);

    /* end the adf; */
    opc_adend(global, &cadf);
}

/*{
** Name: OPC_BYCOMPARE	- Compile code to compare byvals
**
** Description:
**      This routine compiles instructions into a given CX to compare 
**      two sets of by values. 
[@comment_line@]...
**
** Inputs:
**	global -
**	    This is the query wide state variable.
**	cadf -
**	    This is the CX to compile the instrs into.
**	byvals -
**	    This is a description of the by vals.
**	rel -
**	    This is a description of the agg func result table.
**	pout_base -
**	    This is the base number where we should place the proj tuple
**	agg_base -
**	    This is the agg target tuple
**	proj_src_base -
**	    This is the last set of by vals
**
** Outputs:
**	cadf -
**	    This will hold the new instrs
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      5-nov-86 (eric)
**          written
**	17-jul-89 (jrb)
**	    Set precision fields correctly.
**	3-feb-04 (inkdo01)
**	    Change TID buffer to 8 bytes for table partitioning.
[@history_template@]...
*/
static VOID
opc_bycompare(
	OPS_STATE	*global,
	OPC_ADF		*cadf,
	PST_QNODE	*byvals,
	RDR_INFO	*rel,
	i4		pout_base,
	i4		agg_base,
	i4		proj_src_base )
{
    i4			byvalno;
    DMT_ATT_ENTRY	*att;
    ADE_OPERAND		ops[2];
    i4			align;

    if (byvals->pst_left != NULL && 
	    byvals->pst_left->pst_sym.pst_type != PST_TREE
	)
    {
	opc_bycompare(global, cadf, byvals->pst_left, rel, 
					    pout_base, agg_base, proj_src_base);
    }

    /* if this is a non-printing byval resdom, then there is no need to
    ** compile in the comparisons.
    */
    if (!(byvals->pst_sym.pst_value.pst_s_rsdm.pst_rsflags&PST_RS_PRINT))
    {
	return;
    }

    ops[0].opr_base = proj_src_base;
    ops[1].opr_base = agg_base;

    byvalno = byvals->pst_sym.pst_value.pst_s_rsdm.pst_rsno;
    if (byvalno == DB_IMTID)
    {
	align = rel->rdr_rel->tbl_width % sizeof (ALIGN_RESTRICT);
	if (align != 0)
	    align = sizeof (ALIGN_RESTRICT) - align;
	ops[0].opr_offset = ops[1].opr_offset = 
					(i4) rel->rdr_rel->tbl_width + align;
	ops[0].opr_dt = ops[1].opr_dt = (i4) DB_TID8_TYPE;
	ops[0].opr_len = ops[1].opr_len = (i4) DB_TID8_LENGTH;
	ops[0].opr_prec = ops[1].opr_prec = 0;
	ops[0].opr_collID = -1;
    }
    else
    {
	att = rel->rdr_attr[byvalno];
	ops[0].opr_offset = ops[1].opr_offset = (i4) att->att_offset;
	ops[0].opr_dt = ops[1].opr_dt = (i4) att->att_type;
	ops[0].opr_len = ops[1].opr_len = (i4) att->att_width;
	ops[0].opr_prec = ops[1].opr_prec = att->att_prec;
	ops[0].opr_collID = ops[1].opr_collID = att->att_collID;
    }

    opc_adinstr(global, cadf, ADE_BYCOMPARE, ADE_SINIT, 2, ops, 0);

    ops[0].opr_base = pout_base;
    ops[1].opr_base = proj_src_base;
    opc_adinstr(global, cadf, ADE_MECOPY, ADE_SMAIN, 2, ops, 0);
}
