/*
** Copyright (c) 1986-2003 Ingres Corporation
*/
#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <cm.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <ulm.h>
#include    <ulf.h>
#include    <adf.h>
#include    <ade.h>
#include    <adfops.h>
#include    <adp.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmscb.h>
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
#include    <bt.h>
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
#define        OPC_XQUAL 	TRUE
#include    <opxlint.h>
#include    <opclint.h>

/**
**
**  Name: OPCQUAL.C - Routines to compile ADF code to qualify tuples
**
**  Description:
{@comment_line@}...
**
**	External Routines:
**          opc_cxest() - Estimate the size of a CX from a qtree
**          opc_cqual() - Compile a qtree into an open CX.
**          opc_target() - Compile a target list into a CX.
**          opc_estdefault() - Estimate the size of default values for an append
**          opc_cdefault() - Compile the default values into a CX for an append
**          opc_crupcurs() - Compile a replace cursor statement into a CX.
**          opc_rdmove() - Compile code to move a resdom to it's final
**								destination
**	    opc_lvar_row() - Fill in an operand for a local variable.
[@func_list@]...
**
**	Internal Routines:
**	    opc_cqual1() - workhorse function for qtree compilation
**          opc_cresdoms() - Compile a list of resdoms into a CX
[@func_list@]...
**
**
**  History:
**      23-aug-86 (eric)
**          created
**	20-feb-89 (paul)
**	    Enhanced opc_crupcurs and opc_rdmove to support expressions
**	    involving before and after row images during rule processing.
**	    Also enhanced these routines to support the generation of
**	    parameter lists for CALLPROC actions from a target list tree.
**	24-mar-89 (paul)
**	    Fixed problem in generating CALLPROC parameter expressions
**	    during rule processing. The check for query type at the
**	    start of opc_rdmove does not apply for rules.
**	17-apr-89 (jrb)
**	    Properly initialized prec field(s).
**	 5-jun-89 (paul)
**	    Fix UNIX compiler warnings.
**	21-jun-1989 (nancy) -- bug #6248 fix in opc_cresdoms (see comment)
**	18-jul-89 (jrb)
**	    More precision initialization work for decimal project.
**	25-sep-89 (paul)
**	    Fix bug 8054: Allow TIDs to be used as parameters to procedures
**	    called from rules.
**	15-dec-89 (jrb)
**	    Added support for DB_ALL_TYPE in fi descs. (For outer join project.)
**	27-apr-90 (stec)
**	    Changed calls to opc_adtransform(), which now requires calclen
**	    boolean parm (related to fix for bug 21338).
**	14-nov-90 (stec)
**	    Changed opc_rdmove() code to use temporary DSH row for coercion
**	    (bug 33557).
**	09-jan-91 (stec)
**	    Modified opc_target() to fix bug 35209.
**	16-jan-91 (stec)
**	    Modified opc_cresdoms(), opc_cdefault(), opc_rdmove(),
**	    opc_crupcurs() to add missing opr_prec initialization.
**      11-dec-1991 (fred)
**          Added support for large objects.  When dealing with large objects,
**	    use ADE_REDEEM instead of ADE_MECOPY when called from top level
**	    of query.  This operation should ONLY be used when about to send
**	    data out of the server.
**      18-mar-92 (anitap)
**          Modified opc_cqual() and opc_cresdoms() to fix bug 42363. See the
**          routines for further details.
**      17-jul-92 (anitap)
**          Cast naked NULLs in opc_cqual() and opc_rupcurs().
**      14-aug-92 (anitap)
**          Modified opc_cqual() to fix a bug which is related to bug
**          45876/46196.
**      26-oct-92 (jhahn)
**          Added opc_proc_insert_targetlist.
**	09-nov-92 (jhahn)
**	    Added handling for byrefs.
**      16-nov-92 (terjeb)
**          Fully typed first argument to MEcmp function in
**          opc_proc_insert_target_list.
**	17-nov-1992 (fred)
**	    Modified opc_rdmove() to correctly recognize when a user object
**	    was being updated.  Fixes large object cursor problem...
**	14-dec-92 (jhahn)
**	    Did some cleanup for  handling of byrefs. Added procedure
**	    opc_lvar_info.
**	12-feb-93 (jhahn)
**	    Support for statement level rules.
**	24-mar-93 (jhahn)
**	    Added fix for support of statement level rules. (FIPS)
**      23-apr-1993 (stevet)
**          Fixed up a couple of places where db_datatype == DB_DEC_TYPE
**          is changed to abs(db_datatype) == DB_DEC_TYPE.  Also
**          incorporate Eric's changes to check for precision in
**          a number of places. (B49871)
**	17-may-93 (jhahn)
**	    Moved code for allocating before row for replace actions from
**	    opc_addrules to opc_crupcurs.
**      29-Jun-1993 (fred)
**          Fixed bug 50552 in opc_target().
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	25-oct-95 (inkdo01)
**	    Only trigger possible PMQUEL change if PM chars are there.
**      10-nov-95 (inkdo01)
**	    Changes to support replacement of QEN_ADF instances by pointers.
**	10-nov-95 (inkdo01)
**	    Changes to support reorganization of QEF_AHD, QEN_NODE structs.
**      06-mar-96 (nanpr01)
**          Removed the dependency of DB_MAXTUP. This is now replace with
**	    global->ops_cb->ops_server->opg_maxtup which is the max tuple
**	    limit for the installation.
**	28-feb-1997(angusm)
**	    back out previous change: due to convoluted logic lower down,
**	    this causes wrong function to be selected for string compare
**	    in presence of non-default collation sequence (regression of
**	    bug 53379)
**	26-mar-97 (inkdo01)
**	    Changes to support non-CNF Boolean expressions. opc_cqual is
**	    a stub to accept external calls, old opc_cqual becomes static
**	    opc_cqual1, containing new AND/OR logic.
**	22-jul-98 (shust01)
**	    In opc_proc_insert_target_list(), moved reinitialization of
**	    resdom inside outer for loop (where looping over "set of" list,
**	    looking for actual to project). This will have us start at the
**	    root of the tree for every instance of the loop.  Prior to this,
**	    there was a chance of SEGVing because we would do a MEcmp of a
**	    resdom...->pst_rsname for pst_type = PST_TREE, which isn't good
**	    since the resdom structure for a PST_TREE isn't the full size of
**	    the structure so we could be looking at unallocated storage.
**	15-jul-98 (inkdo01)
**	    Changes to reduce stack frame size of opc_cqual1 (introduced for
**	    non_CNF support). "non-CNF" changes almost doubled frame size,
**	    causing stack overflow problems for this very recursive function.
**      04-Feb-99 (hanal04)
**          Union of CNF and NONCNF selects lead to state where 
**          cadf->opc_noncnf = FALSE even though we have a subquery where 
**          OPS_NONCNF was set in opj_normalise(). Check the Compilation 
**          phase global state var for OPS_NONCNF. b94280. 
**      21-apr-1999 (hanch04)
**	    Added st.h
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**      08-jul-99 (gygro01)
**	    Fix for bug 93275 (ingsrv 533) in opc_cqual. In some special
**	    cases the resop of a PST_BOP/PST_UOP is still defined as not
**	    null datatype, but the input oprand is nullable. This happen
**	    if the variable of the PST_VAR is not null, but part of a eqc
**	    and the eqc is nullable.
**      18-Jan-1999 (hanal04) Bug 92148 INGSRV494
**          Enhanced OPC to support compilation of the non-cnf case
**          in opc_crupcurs().
**	11-nov-2002 (stial01) Set QEQP_NEED_TRANSACTION if we encounter
**	    resolve_table function (b109101)
**	28-nov-02 (inkdo01 for hayke02)
**	    Deke opcadf into coercing to meaningful scale for decimal datatype
**	    (fixes 109194).
**	08-Jan-2003 (hanch04)
**	    Back out last change.  Causes E_OP0791_ADE_INSTRGEN errors.
**	16-jan-2003 (somsa01 for hayke02/inkdo01)
**	    Deke opcadf into coercing to meaningful scale for decimal datatype
**	    (fixes 109194).
**	09-jul-2003 (devjo01)
**	    Modify fix to b109101 to open a transaction in all cases except
**	    a singleton select of a dbmsinfo function.  At least one site
**	    was depending on a singleton select of a constant opening a
**	    transaction. (See 12767690;01).
**	17-nov-03 (inkdo01)
**	    Separate opc_temprow removes dependence of CX intermediate 
**	    results buffer being 1st entry in DSH row array.
**      18-Nov-2003 (hanal04) Bug 111311 INGSRV2601
**          Modify existing kludge in opc_cqual1() to prevent E_OP0791.
**      02-Dec-2003 (hanal04) Bug 111377 INGSRV2620
**          Further modifications to the kludge in opc_cqual1().
**          Also extended those changes to opc_cresdoms() which contains
**          the same kudge.
**	06-aug-2004 (hayke02, vansa02) Bug 111966 INGSRV2756
**	    Add four bytes (2 * DB_ELMSZ) to the unicode operand length in
**	    addition to two bytes for varchar length (DB_CNTSIZE) if the length
**	    of unicode string in comparison is zero or if the unicode string
**	    contains a partial character (one byte length) alone. We are adding
**	    the extra bytes because a repeat query will be compiled using the
**	    length of the first string.
**	17-dec-04 (inkdo01)
**	    Added many collID's.
**	4-july-04 (inkdo01)
**	    Numerous changes for CX optimization.
**	06-Jul-06 (kiria01) b116230
**	    psl_print becomes PSL_RS_PRINT bit in new field psl_rsflags
**	    to allow for extra flags in PST_RSDM_NODE
**	17-Aug-2006 (kibro01) Bug 116481
**	    Mark that opc_adtransform is not being called from pqmat
**      10-sep-2008 (gupsh01,stial01)
**          Don't normalize string constants in opc_cqual1.
**	20-Oct-2008 (kiria01) b121098
**	    Pass to opc_qual1() and extra parameter derived from the parent
**	    PST_QNODE as to what coersion FI need be applied for this operand.
**	    Initially, this will be used just to identify those operands
**	    whose datatype would imply coercion but that pst_resolve() has
**	    determined don't because the types involved are dtfamily related.
**	3-Jun-2009 (kschendel) b122118
**	    Minor cleanups, delete unused adbase params.
**      16-sep-2009 (joea)
**          Change opc_cqual1 to always emit the constant for boolean types.
**      29-oct-2009 (joea)
**          Change opc_crupcurs1 to always emit the constant for boolean types.
**	11-Nov-2009 (kiria01) SIR 121883
**	    With PST_INLIST, check for PST_CONST on RHS as the flag
**	    may be applied to PST_SUBSEL to aid servers to distinguish
**	    simple = from IN and <> from NOT IN.
**	07-Dec-2009 (troal01)
**	    Consolidated DMU_ATTR_ENTRY, DMT_ATTR_ENTRY, and DM2T_ATTR_ENTRY
**	    to DMF_ATTR_ENTRY. This change affects this file.
**	30-Mar-2010 (kschendel)
**	    Fix a couple new compiler warnings.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**	04-May-2010 (kiria01) b123680
**	    Use stack local copy of fi descriptor and add the missing dt_family
**	    processing to ensure that views involving ANSI datatypes were
**	    reconstituted correctly.
**/

/*
**  Definition of static variables and forward static functions.
*/

static ADE_OPERAND 	labelinit = {0, 0, ADE_LABBASE, 0, 0, 0};

static i4
opc_cqual1(
	OPS_STATE   *global,
	OPC_NODE    *cnode,
	PST_QNODE   *root,
	OPC_ADF	    *cadf,
	i4	    segment,
	ADE_OPERAND *resop,
	ADI_FI_ID   rescnvtid,
	bool	    incase);

static VOID
opc_cescape(
	OPS_STATE   *global,
	OPC_ADF	    *cadf,
	PST_QNODE   *root,
	i4	    segment);

static VOID
opc_cresdoms(
	OPS_STATE   *global,
	OPC_NODE    *cnode,
	PST_QNODE   *root,
	OPC_ADF	    *cadf,
	i4	    segment,
	ADE_OPERAND *resop);

static VOID
opc_crupcurs1(
	OPS_STATE	*global,
	PST_QNODE	*root,
	OPC_ADF		*cadf,
	ADE_OPERAND	*resop,
	RDR_INFO	*rel,
	OPC_NODE	*cnode);

static VOID
opc_seq_setup(
	OPS_STATE	*global,
	OPC_NODE	*cnode,
	OPC_ADF		*cadf,
	PST_QNODE	*root,
	QEF_SEQUENCE	**seqp,
	i4		*localbuf);

static bool
opc_expranal(
	OPS_SUBQUERY	*subquery,
	OPC_NODE	*cnode,
	PST_QNODE	*rootp,
	PST_QNODE	**nodepp,
	PST_QNODE	**subexlistp,
	i4		*subex_countp);

static bool
opc_exprsrch(
	OPS_SUBQUERY	*subquery,
	OPC_NODE	*cnode,
	PST_QNODE	**subexpp,
	PST_QNODE	**nodepp,
	PST_QNODE	**subexlistp,
	i4		*subex_countp,
	bool		*gotone);

static bool
opc_compops(
	OPS_SUBQUERY	*subquery,
	PST_QNODE	*leftop,
	PST_QNODE	*rightop,
	PST_QNODE	*srchp);

/*
**  Defines of constants used in this file
*/
#define    OPC_UNKNOWN_LEN    0
#define    OPC_UNKNOWN_PREC   0
#define    OPC_UNORM_EXP_FACTOR	4
#define    OPC_UNORM_RESULT_SIZE	400


/*{
** Name: OPC_CXEST	- Calculate the info to estimate the size of a CX
**
** Description:
**      This routine calculates the information that is required to estimate
**      the size of a ADE CX. The routine that actually does the estimation
**      is ade_cx_space() so this routine is really a precursor to that
**      step. Opc_adbegin() has the call to ade_cx_space() so, typically,
**      a call to opc_cx_est() and then opc_adbegin() is all that is needed
**      to begin compiling a CX.
**
**      The bulk of the routine (as you can plainly see) is a switch statement
**      on the root node type. Each case handles the various nodes that
**      opc_cqual() is able to handle. Each case is responsible for
**      incrementing the output parameters by the amount necessary for the
**      node type in question. Once done, the case breaks to the recursive
**      calls to the left and right subtrees.
**
**	As mentioned above, there is a relationship between opc_cx_est()
**	and opc_cqual(). The intent is that opc_cx_est() should estimate
**	the size of query trees that opc_cqual() can compile. In other words,
**	for each case in opc_cx_est() there should be a corresponding case
**	in opc_qual(). There is two exceptions to this: the first is the
**	case for PST_AOP. Opc_cqual() cannot correctly compile the code needed
**	for PST_AOPs. That is the job for opc_agtarget(), however it is an
**	obvious, simple extension to handle PST_AOPs in here for opc_agtarget().
**
**	The other exception is PST_CURVAL. Nothing is executed for PST_CURVAL
**	nodes, but the case exists to prevent having PST_CURVALs generate
**	unknown node errors. Opc_cqual() cannot handle PST_CURVAL nodes
**	but opc_cqual()'s sister routine, opc_crupcurs(), can handle them.
**	Note that PST_CURVAL nodes only appear in replace cursor/update cursor
**	statements.
**
**	As you can see, there are really relationships between opc_cx_est()
**	on the one hand, and opc_cqual(), opc_aggtarget(), and opc_crupcurs()
**	on the other. However, the primary relationship is between opc_cx_est()
**	and opc_cqual().
**
** Inputs:
**	global -
**	    This is the query-wide state variable for OPF/OPC
**	cnode -
**	    This holds information about the current CO/QEN node that
**	    is being compiled.
**	root -
**	    The root of the query tree to be estimated for compiling.
**	ninstr -
**	nops -
**	nconst -
**	szconst -
**	    These are pointers to a nats (or a i4) that have been
**	    initialized to zero (or what ever value is desired)
**
** Outputs:
**	ninstr -
**	    This will be incremented by the number of  instructions
**	    needed to compile the qtree pointed to by root.
**	nops -
**	    This will be incremented by the number of operands
**	    needed to compile the qtree pointed to by root.
**	nconst -
**	    This will be incremented by the number of constants
**	    needed to compile the qtree pointed to by root.
**	szconst -
**	    This will be incremented by the total size of constants
**	    needed to compile the qtree pointed to by root.
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
**      24-aug-86 (eric)
**          written
**      15-feb-89 (eric)
**          added more comments.
**	15-dec-89 (jrb)
**	    Added support for DB_ALL_TYPE in fi descs. (For outer join project.)
**	26-apr-1999 (shero03)
**	    Support PST_MOP node
**	10-sep-99 (inkdo01)
**	    Add support for case function (PST_CASEOP node, et al).
**	15-mar-02 (inkdo01)
**	    Add support for sequence operators.
**	10-nov-03 (inkdo01)
**	    Add a base entry for each sequence operator.
**	9-jan-04 (inkdo01)
**	    Iterate down PST_CONST chain (because of IN-lists) to avoid
**	    recursion-induced stack overflow.
**	19-dec-2006 (dougi)
**	    Skip PST_VAR extra logic for common subex's to avoid SEGV.
**	6-jun-07 (kibro01) b118384
**	    Because we sometimes call opc_cxest with cnode set to NULL
**	    so we can estimate requirement for bases, protect the use of
**	    cnode within here by checking it against NULL first.
**	09-Oct-2008 (kiria01) b118384
**	    Corrected the calculations relating to VLUP/VLT lengths.
**	    Corrected the space calculation for IN processing as it
**	    only regarded the first entry.
**	    Reduced the tail recursion to flatten exection depth and
**	    parameter passing.
**      12-Feb-2009 (hanal04) Bug 121645
**          Bump cnode->opc_below_rows for PST_CONST VLUPs.
**          Also check for cnode NULL before trying.
[@history_template@]...
*/
VOID
opc_cxest(
		OPS_STATE   *global,
		OPC_NODE    *cnode,
		PST_QNODE   *root,
		i4	    *ninstr,
		i4	    *nops,
		i4	    *nconst,
		i4	    *szconst)
{
    while (root)
    {
    
	switch (root->pst_sym.pst_type)
	{
	case PST_AND:
	case PST_OR:
	    *ninstr += 1;
	    break;
    
	case PST_AOP:
	    *ninstr += 2;
	    *nops += 4;
	    /* fall through to the rest of the operators, ie. don't put in
	    ** a break here.
	    */
    
	case PST_UOP:
	case PST_BOP:
	case PST_COP:
	    *ninstr += 1;
	    if (root->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_fitype !=
								    ADI_COMPARISON
		)
	    {
		/* Only the non-comparison operators have a result operands.
		** Comparison operators only change the ADF execution state,
		*/
		*nops += 1;

		/* If operator type indicates VLT - incr bases required */
		if (cnode && root->pst_sym.pst_dataval.db_length < 0)
		    cnode->opc_below_rows++;
	    }
    
	    if (root->pst_right != NULL)
	    {
		DB_DT_ID	    dtr;
		DB_DT_ID	    dt2;
    
		*nops += 1;
    
		dtr = root->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_dtresult;
		dt2 = root->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_dt[1];
    
		if (dtr != dt2  &&  dtr != DB_ALL_TYPE  &&  dt2 != DB_ALL_TYPE)
		{
		    *ninstr += 1;
		    *nops += 2;
		}
	    }
	    if (root->pst_left != NULL)
	    {
		DB_DT_ID	    dtr;
		DB_DT_ID	    dt1;
    
		*nops += 1;
    
		dtr = root->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_dtresult;
		dt1 = root->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_dt[0];
    
		if (dtr != dt1  &&  dtr != DB_ALL_TYPE  &&  dt1 != DB_ALL_TYPE)
		{
		    *ninstr += 1;
		    *nops += 2;
		}
	    }
	    break;
    
	case PST_MOP:
	{
	    ADI_FI_DESC	*fi = root->pst_sym.pst_value.pst_s_op.pst_fdesc;
	    i4		i;
	    DB_DT_ID	dt1;
	    DB_DT_ID	dt2;
	    PST_QNODE       *lqnode;
	    PST_QNODE	*resqnode;

	    /* If operator type indicates VLT - incr bases required */
	    if (cnode && root->pst_sym.pst_dataval.db_length < 0)
		cnode->opc_below_rows++;

	    *ninstr += 1;
	    resqnode = root;
	    lqnode = root->pst_right;
	    i = 0;
	    while (lqnode)
	    {	
    
		*nops += 1;
    
		dt1 = DB_ALL_TYPE;			/* In case varargs */
		if (i < fi->adi_numargs)
		    dt1 = fi->adi_dt[i];
		dt2 = lqnode->pst_sym.pst_dataval.db_datatype;
    
		if (dt1 != abs(dt2)  &&  dt1 != DB_ALL_TYPE)
		{
		    *ninstr += 1;
		    *nops += 2;
		}
		resqnode = resqnode->pst_left;
		if (resqnode)
		    lqnode = resqnode->pst_right;
		else
		    break;
		i += 1;
	    }
	    break;
	}
    
	case PST_OPERAND:
	case PST_CASEOP:
	case PST_WHLIST:
	    break;	/* place holders */
    
	case PST_WHOP:
		/* If the case "else", just the assignment, otherwise, the assignment
		** and 2 branches. */
		if (root->pst_left) 
		{
		    *ninstr += 1;
		    *nops += 2;
		}
		else
		{
		    *ninstr += 3;
		    *nops += 4;
		}
	    break;
    
	case PST_CONST:
	    *nconst += 1;
	    if (root->pst_sym.pst_dataval.db_length >= 0)
		*szconst += root->pst_sym.pst_dataval.db_length;
	    else
		/*
		** A VLUP base increment added for the unknown length item.
		*/
		if(cnode)
                {
                    cnode->opc_below_rows++;
                }
	    break;
    
	case PST_RESDOM:
	    *ninstr += 1;
	    *nops += 2;
	    break;
    
	case PST_VAR:
	{
	    i4		attno;
	    OPS_SUBQUERY	*subqry = global->ops_cstate.opc_subqry;
	    OPZ_ATTS	*att;
	    PST_QNODE	*func_tree;
    
	    if (root->pst_sym.pst_value.pst_s_var.pst_vno == -1)
		break;		/* skip for common subex results */
    
	    /* if (this is a function att and it's not available) */
	    attno = root->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id;
	    att = subqry->ops_attrs.opz_base->opz_attnums[attno];
	    if (att->opz_attnm.db_att_id == OPZ_FANUM &&
		    cnode &&
		    cnode->opc_ceq &&
		    cnode->opc_ceq[att->opz_equcls].opc_attavailable == FALSE
		)
	    {
		/* call opc_cxest on the func att tree; */
		func_tree = subqry->ops_funcs.opz_fbase->
			    opz_fatts[att->opz_func_att]->opz_function->pst_right;
		if (!root->pst_left && !root->pst_right)
		{
		    /* Avoid tail-end recursion if possible */
		    root = func_tree;
		    /* Skip to top of loop */
		    continue;
		}
		opc_cxest(global, cnode, func_tree, ninstr, nops, nconst, szconst);
	    }
	    break;
	}
    
	case PST_CURVAL:
	case PST_QLEND:
	case PST_TREE:
	case PST_RULEVAR:
	    /* A rule variable is similar to a curval or attno in terms of space  */
	    /* taken. */
	    break;
    
	case PST_SEQOP:
	    *ninstr += 1;
	    *nops += 2;
	    if (cnode)
		cnode->opc_below_rows++;
	    break;
    
	default:
	    opx_error(E_OP0681_UNKNOWN_NODE);
	}
    
	/* Replaced tail recursion to flatten execution and
	** reduce parameter passing */
	if (!root->pst_left)
	    root = root->pst_right;
	else
	{
	    if (root->pst_right)
		opc_cxest(global, cnode, root->pst_right, ninstr,
		          nops, nconst, szconst);
	    root = root->pst_left;
	}
    }
}

/*{
** Name: OPC_CQUAL - external entry for compiling qtree fragment.
**
** Description:
**      This routine acts as an external stub for calls to opc_cqual1.This
**	is used to compile qtree fragments into ADF code. Stub was introduced
**	to insulate external callers from changes to support non-CNF Boolean
**	expressions.
[@comment_line@]...
**
** Inputs:
**	global -
**	    This is the query-wide state variable for OPF/OPC
**	cnode -
**	    This information about the current CO/QEN node that is being
**	    compiled.
**	root -
**	    The root of the sub-tree that is to be compiled.
**	cadf -
**	    A pointer to the OPC layer (version) of the open CX.
**	segment -
**	    The segment (init, main, finit, virgin) of the CX to compile
**	    the instructions into.
**	resop -
**	    The opr_dt field tells what datatype the caller would like
**	    the result to be. The opr_offset field is init'ed to -1 here to
**	    indicate to opc_cqual1 that a result offset is NOT being sent
**	    in. The remaining fields are uninitialized and are for output.
**	tidatt -
**	    a pointer to i4  that is initialized to FALSE.
**
** Outputs:
**	cadf -
**	    The CX now holds the appropriate instructions for root.
**	resop -
**	    opr_length, opr_prec, opr_base, and opr_offset will be set for
**	    those qnodes that produce a valued output (ie. an output with a
**	    tangible output). These nodes include: some of the op nodes
**	    (PST_UOP, PST_BOP, and PST_COP) depending on the op's adi_fitype,
**	    consts, resdoms, vars.
**	tidatt -
**	    This will be set to TRUE if a root contains a var node that refers
**	    to a tid attribute of a table.
**
**	Returns:
**	    Whether the expression that was compiled was a constant expression
**	    (OPC_CNSTEXPR) or an expression that contains var nodes
**	    (OPC_NOTCONST).
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      26-mar-97 (inkdo01)
**	    Written as part of liberation of Ingres from conjunctive normal
**	    form Boolean expressions.
**	9-sep-99 (inkdo01)
**	    Init resop->opr_offset to -1 to indicate that opc_cqual1 fills 
**	    it in (for case function implementation).
**	27-oct-99 (inkdo01)
**	    Init opc_whenlvl for compiling "or"s inside case when's.
[@history_template@]...
*/
i4
opc_cqual(
		OPS_STATE   *global,
		OPC_NODE    *cnode,
		PST_QNODE   *root,
		OPC_ADF	    *cadf,
		i4	    segment,
		ADE_OPERAND *resop,
		i4	    *tidatt)
{
    ADE_OPERAND	truelab, falselab;
    i4		result;
    bool	noncnf;


    /* Labels must be set up here (to show them to be end of chain), and
    ** noncnf must be set according to subquery flag. Then, just pass call
    ** on to opc_cqual1. */

    STRUCT_ASSIGN_MACRO(labelinit, truelab);
    STRUCT_ASSIGN_MACRO(labelinit, falselab);
    cadf->opc_tidatt = tidatt;
    cadf->opc_truelab = (char *)&truelab;
    cadf->opc_falselab = (char *)&falselab;
    cadf->opc_whenlvl = 0;
    if (resop != (ADE_OPERAND *)NULL) resop->opr_offset = OPR_NO_OFFSET;
						/* init result offset */
    result = opc_cqual1(global, cnode, root, cadf, segment, resop, (ADI_FI_ID)0, FALSE);
    opc_adlabres(global, cadf, &truelab);	/* resolve label chains */
    opc_adlabres(global, cadf, &falselab);
    return(result);
}

/*{
** Name: OPC_CQUAL1	- Compile a qtree into an open CX.
**
** Description:
**      This routine will compile "simple" query tree fragments into an
**	opened CX. Opc_cqual() cannot handle the "complicated" query trees
**	that use aggregates. Opc_cqual() also cannot handle the curval nodes
**	that appear in replace/update cursor statements; this is because curval
**	nodes require extra procedure parameters and I didn't want to clutter
**	up opc_cqual(), see opc_crupcurs().
**
**	Opc_cqual() also cannot handle root nodes, which is why I said
**	"query tree fragments" above. Compiling an entire qtree (root node
**	and all) is the task for OPC as a whole and, as such, is too
**	complicated a task for just opc_cqual(). Anyway, opc_cqual() can compile
**      any qtree that doesn't contain aggregate nodes (either aops or byops),
**      curval nodes, or root nodes. Please refer to opc_agtarget() for the
**	compilation of aggregate nodes, and to opc_crupcurs() for the
**	compilation of curval nodes.
**
**      Opc_cqual() is basically one big switch statement, with one case
**      for each node type that it handles. With a couple of exceptions,
**      this routine is recursive because qtrees are essentially recursive.
**      Compiling AND nodes is not recursive because is was easy to make it
**      non-recursive. Compiling RESDOM nodes was made non-recursive (even
**      though it was non-trivial to do so) to save stack space. With the
**      larger tables, and hence target lists, that we are beginning to
**      handle, it became necessary to reduce our stack space usage.
[@comment_line@]...
**
** Inputs:
**	global -
**	    This is the query-wide state variable for OPF/OPC
**	cnode -
**	    This information about the current CO/QEN node that is being
**	    compiled.
**	root -
**	    The root of the sub-tree that is to be compiled.
**	cadf -
**	    A pointer to the OPC layer (version) of the open CX.
**	segment -
**	    The segment (init, main, finit, virgin) of the CX to compile
**	    the instructions into.
**	resop -
**	    The opr_dt field tells what datatype the caller would like
**	    the result to be. If opr_offset is -1, the remaining fields are
**	    uninitialized and are for output. If opr_offset is > 0, the
**	    other fields have been init'ed (by case function processing)
**	    and will NOT be filled in.
**	tidatt (cadf->opc_tidatt) -
**	    a pointer to i4  that is initialized to FALSE.
**
** Outputs:
**	cadf -
**	    The CX now holds the appropriate instructions for root.
**	resop -
**	    opr_length, opr_prec, opr_base, and opr_offset will be set for
**	    those qnodes that produce a valued output (ie. an output with a
**	    tangible output). These nodes include: some of the op nodes
**	    (PST_UOP, PST_BOP, and PST_COP) depending on the op's adi_fitype,
**	    consts, resdoms, vars. If this call is processing a case function
**	    result expression, the above fields will all be pre-initialized,
**	    indicating that the results should be materialized in that 
**	    location.
**	tidatt (cadf->opc_tidatt) -
**	    This will be set to TRUE if a root contains a var node that refers
**	    to a tid attribute of a table.
**
**	Returns:
**	    Whether the expression that was compiled was a constant expression
**	    (OPC_CNSTEXPR) or an expression that contains var nodes
**	    (OPC_NOTCONST).
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      24-aug-86 (eric)
**          written
**      15-feb-89 (eric)
**          recoded somewhat to save stack space, which includes moving
**	    resdom compilation to opc_cresdoms(), and added some comments.
**      8-mar-89 (eric)
**          changed to evaluate fatts if they are available, but their
**          underlying eqcs are. This allows ORIG nodes to evaluate qual
**          clauses that use fatts.
**	25-jul-89 (jrb)
**	    Now passes db_data field of DB_DATA_VALUEs to opc_adcalclen so that
**	    result length calculations based on the actual value of the inputs.
**      18-mar-92 (anitap)
**          Changed to initialize opr_len and opr_prec to UNKNOWN if the
**          operator type is comparison, else to the actual len and prec
**          of the node. Fix for bug 42363. Ths bug involved equivalence
**          classes. Because of the way that eq classes are treated in OPF, a
**          char of 9 bytes was being substituted in place of 6 char byte
**          fld in the resdom which involved concatenation. This led to
**          wrong results.
**      17-jul-92 (anitap)
**          Cast naked NULL being passed into opc_adcalclen().
**      14-aug-92 (anitap)
**          Changed to check that the source and the result data were
**          both either nullable or non-nullable. Otherwise, need to call
**          opc_adtransform(). Refer to opc_adtranform() for details.
**          This is part of fix for bug 45876/46196.
**	25-oct-95 (inkdo01)
**	    Only trigger possible PMQUEL change if PM chars are there.
**	28-feb-1997(angusm)
**	    back out previous change: due to convoluted logic lower down,
**	    this causes wrong function to be selected for string compare
**	    in presence of non-default collation sequence (regression of
**	    bug 53379)
**	26-mar-97 (inkdo01)
**	    Changed to static func called from within opcqual. External calls
**	    go through stub function opc_cqual. This insulates callers from
**	    added parms for support of non-conjunctive normal form Boolean
**	    expressions. New logic in AND/OR cases supports non-CNF exprs.
**	15-jul-98 (inkdo01)
**	    Refinements to non-CNF changes to remove 3 parms to opc_cqual1.
**	    This reduces the size of its stack frame and should permit higher
**	    degrees of recursion.
**      04-Feb-99 (hanal04)
**          Union of CNF and NONCNF selects lead to state where 
**          cadf->opc_noncnf = FALSE even though we have a subquery where 
**          OPS_NONCNF was set in opj_normalise(). Check the Compilation 
**          phase global state var for OPS_NONCNF. b94280. 
**      08-jul-99 (gygro01)
**	    Fix for bug 93275 (ingsrv 533) in opc_cqual. In some special
**	    cases the resop of a PST_BOP/PST_UOP is still defined as not
**	    null datatype, but the input oprand is nullable. This happen
**	    if the variable of the PST_VAR is not null, but part of a eqc
**	    and the eqc is nullable.
**	9-sep-99 (inkdo01)
**	    Added support of case function (PST_CASEOP), including code
**	    for pre-set result operand.
**	27-oct-99 (inkdo01)
**	    Added opc_whenlvl to show non-CNF contents of case when clauses.
**	23-feb-00 (hayke02)
**	    Removed opc_noncnf - we now test for OPS_NONCNF in the
**	    subquery's ops_mask directly. opc_noncnf was being set using
**	    global->ops_subquery->ops_mask, when it should have been set
**	    using gloabl->ops_cstate.opc_subqry->ops_mask in order to
**	    reflect whether the individual subqueries have or haven't
**	    been CNF transformed in opj. This change fixes bug 100484.
**	6-jul-00 (inkdo01)
**	    Keep random functions out of virgin segment.
**	27-jul-00 (inkdo01)
**	    Save "when" label across case expressions.
**	11-sep-00 (inkdo01)
**	    Fix for bug in handling nested "case" expressions (102584).
**	15-mar-02 (inkdo01)
**	    Add support for sequence operators.
**	1-may-02 (inkdo01)
**	    Added (begrudgingly) parm to indicate whether we're inside a case
**	    expression. Allows us to force constant case exprs into MAIN segment
**	    because case's are too complex to guarantee they should be in VIRGIN.
**	    It stinks that another parm is required for this highly unlikely
**	    eventuality, so if another way can be found it should be implemented.
**	27-may-02 (inkdo01)
**	    Generalized above random function change to keep generic classes of
**	    functions (including random's and uuid_create) out of virgin segment.
**	25-jul-02 (inkdo01)
**	    Add support to normalize any non-data base Unicode values (e.g. 
**	    constants/host variables).
**	7-aug-02 (inkdo01)
**	    Some fine tuning to pre-determine result length of Unicode constants.
**	28-nov-02 (inkdo01 for hayke02)
**	    Deke opcadf into coercing to meaningful scale for decimal datatype
**	    (fixes 109194).
**	8-jan-03 (inkdo01)
**	    Use Alison's NEED_TRANSACTION flag with sequence operations, too,
**	    in case someone is doing a "nextval" in a constant query (still need
**	    transaction).
**	24-Apr-03 (gupsh01, stial01)
**	    We should not set the QEQP_NEED_TRANSACTION flag for ADI_DBMSI_OP case 
**	    only.  We now set this flag for all other transaction. Earlier we were 
**	    setting this flag for every ADI_NORM_FUNC,  except ADI_RESTAB_OP 
**	    operations, but long_byte function with constant arguments also 
**	    requires a transaction, hence gives out error.
**	09-jul-2003 (devjo01)
**	    Modify fix to b109101 to open a transaction in all cases except
**	    a singleton select of a dbmsinfo function.
**      18-Nov-2003 (hanal04) Bug 111311 INGSRV2601
**          Modify existing kludge to prevent later failures in 
**          adc_1lenchk_rti() for nullable unicode constants.
**	02-Dec-2003 (hanal04) Bug 111377 INGSRV2620
**	    Further modify the *4 buffer length kludge to cap it at
**          DB_MAXSTRING.
**	14-dec-03 (inkdo01)
**	    Add code to support iterative IN-list structures.
**	16-mar-04 (hayke02)
**	    Modify IN-list change above so that we now don't deal with 'not in'
**	    clauses. This change fixes problem INGSRV 2758, bug 111968.
**      23-mar-2004 (huazh01)
**          Modify the fix for b98666. We cannot use the result location
**          ('resop') if its datatype is not the same as the datatype of
**          the current node.
**          This fixes b111640, INGSRV2676.
**	26-mar-04 (inkdo01)
**	    Fix problem with multiple work results in nested CASE.
**	17-may-04 (hayke02)
**	    Remove IN-list code.
**      22-sep-04 (wanfr01)
**        Bug 111327, INGSRV2610
**        Do not blindly use the preallocated result location if the
**        operator is ii_iftrue().  ii_iftrue() requires the result
**        location be a specific length.
**	1-oct-04 (inkdo01)
**	    Changes to IN-list packing to fix "not x in (...)" and IN-lists
**	    in a nonCNF expression.
**	22-nov-04 (inkdo01)
**	    Slight adjustment to the above to fix bad results from IN-lists 
**	    other ANDs and ORs.
**	02-dec-2004 (gupsh01)
**	    Added support for handling variable length user defined 
**	    unicode variables, in which case operator length is always 
**	    ADE_LEN_UNKNOWN.
**    02-feb-2005 (gupsh01)
**        Separated length calculation for NCHAR and NVARCHAR types to avoid 
**        problems with coercion from nchar to char(1) types which resulted 
**        in negative results.
**	08-apr-05 (inkdo01)
**	    Fix errors in ii_iftrue ops issued inside case expressions
**	    (bug 114011).
**	29-june-05 (inkdo01)
**	    Special code in PST_VAR to handle common subexpression 
**	    optimization.
**	28-sep-05 (inkdo01)
**	    Fix to opc_adtransform call for CASE THEN/ELSE expressions 
**	    to project result to correct location (fixes bug 115287).
**	6-mar-06 (dougi)
**	    Fine tune length computations for NCHAR/NVARCHAR.
**	13-Mar-2006 (kschendel)
**	    Fix hard-coded # of operands in adinstr calls.
**	11-may-06 (dougi)
**	    Add coercion, if necessary, to pre-computed expression.
**	10-oct-2006 (dougi)
**	    Further fine-tune Unicode lengths to assure data length divisible
**	    by 2 (code point size).
**	7-Nov-2006 (kschendel) b122118
**	    Fix goofed test for "did we compile result into caller resop",
**	    caused more case expression problems by failing to move
**	    computation to result.
**	    Also fix several places where "do we need to coerce" was
**	    wrong, probably cut-n-paste from an incorrect or inappropriate
**	    original, caused excessive coercions to be generated especially
**	    for decimal.
**	13-nov-2006 (dougi)
**	    Fix bad integration of 480173. It caused trouble with case exprs.
**	7-mar-2007 (dougi)
**	    Tidy up collation IDs for UOP/BOP parameter handling.
**	21-mar-2007 (gupsh01)
**	    Modify the unicode lengths to ensure that the maximum allowed
**	    unicode characters can be handled.
**	30-apr-2007 (dougi)
**	    Add support for UTF8-enabled server.
**	14-Jun-2007 (kiria01) b118101
**	    Be more specific when optimising out coercions with DECIMAL by
**	    concidering precision and scale too.
**	27-Aug-2007 (gupsh01) 
**	    When UTF8 character set is being normalized do not expand the space
**	    for normalization as UTF8 is in normal form C which should occupy 
**	    same amount of space as input.
**	27-Aug-2007 (gupsh01)
**	    Set the result length to the input length for utf8unorm only.
**	29-Aug-2007 (gupsh01)
**	    In UTF8 character set, adjust lengths for the case when
**	    variable length types are coerced to fixed length before 
**	    normalizing.
**	5-sep-2007 (dougi)
**	    Minor change to prevent ADE_UNORM on results of aggregate 
**	    computations presented as fake constant parse tree nodes.
**      15-Nov-2007 (hanal04) Bug 119458
**          date_part() and many other routines have AD_FIXED lenspecs.
**          Don't compile a different resop length in these instances.
**	27-Mar-2008 (gupsh01)
**	    Just like unicode case, prevent adding a transformation 
**	    between similar types for UTF8.
**	28-Mar-2008 (gupsh01)
**	    For UTF8 installations make sure that we don't attempt to 
**	    transform from non UTF8 types (BUG 120172).
**	25-Jun-2008 (kiria01) SIR120473
**	    Move pst_isescape into new u_i2 pst_pat_flags.
**      27-Jul-2008 (gupsh01)
**	    When processing a unicode constant, allow for expansion
**	    of copying the normalization target length from resdom.
**	    Use defines OPC_UNORM_RESULT_SIZE and OPC_UNORM_EXP_FACTOR 
**	    instead of hard coded lengths. (Bug 120707).
**      10-sep-2008 (gupsh01,stial01)
**          Don't normalize string constants in opc_cqual1.
**	12-Oct-2008 (kiria01) SIR121012
**	    Convert a constant boolean literal into the appropriate
**	    execution state setting with ADE_SETFALSE or ADE_SETTRUE.
**	    These constants can only have got into the parse tree by
**	    the predicate functions being called from adf_func - thus
**	    we set the constant state as appropriate instead.
**	    Also check resop for NULL where overlooked
**	07-Jan-2009 (kiria01) SIR121012
**	    Not forgetting the NULLable bools too.
**	01-Nov-2009 (kiria01) b122822
**	    Added support for ADE_COMPAREN & ADE_NCOMPAREN instructions
**	    for big IN lists.
**	03-Nov-2009 (kiria01) b122822
**	    Support both sorted and traditional INLIST form for generality
**	11-Jun-2010 (kiria01) b123908
**	    Don't access non-existant fields from PST_OPERAND nodes.
*/
#define	    OPC_CNSTEXPR    1
#define	    OPC_NOTCONST    2
static i4
opc_cqual1(
		OPS_STATE   *global,
		OPC_NODE    *cnode,
		PST_QNODE   *root,
		OPC_ADF	    *cadf,
		i4	    segment,
		ADE_OPERAND *resop,
		ADI_FI_ID   rescnvtid,
		bool	    incase)
{
	/* find the current subqry struct */
    OPS_SUBQUERY    *subqry = global->ops_cstate.opc_subqry;

	/* ops is the array of operands that all cases will use to compile
	** their nodes. By ADF tradition, ops[0] holds the result if there is
	** one.
	*/
    ADE_OPERAND	    ops[ADI_MAX_OPERANDS+1];

	/* the return code for this call.
	*/
    i4		    ret = OPC_NOTCONST;

    if (root == NULL)
	return(OPC_CNSTEXPR);

    switch (root->pst_sym.pst_type)
    {
     case PST_AND:
	/* 2 streams of logic - old logic assumes expression is in
	** conjunctive normal form. New logic assumes it isn't, and that
	** AND/OR execution may involve branching through ADF expr */

        /* b94280 - Check the Comp. phase global state var for OPS_NONCNF */
        if (!(subqry && (subqry->ops_mask & OPS_NONCNF)) &&
	    cadf->opc_whenlvl == 0)
	{	/* the old-fashioned way */
	    /* AND nodes form a list, rather than a tree, so lets walk down the
	    ** list and compile all the clauses and AND instructions. We
	    ** could code this recursively, like for OR nodes, but this is
	    ** faster and takes less stack space.
	    */
	    for (; root != NULL && root->pst_sym.pst_type == PST_AND;
		root = root->pst_right)
	    {
		(VOID) opc_cqual1(global, cnode, root->pst_left, cadf,
			    segment, resop, (ADI_FI_ID)0, FALSE);
		opc_adinstr(global, cadf, ADE_AND, segment, 0, resop, 0);
	    }
	    if (root != NULL && root->pst_sym.pst_type != PST_QLEND)
	    {
		opx_error(E_OP0681_UNKNOWN_NODE);
	    }
	}
	else
	{	/* the new-fashioned way */
	    ADE_OPERAND	locallab;
	    char	*truelab;

	    /* Left subtree of AND being true should jump to the AND
	    ** (skipping what's in between). Falses on either side just
	    ** join the false list. */
	    STRUCT_ASSIGN_MACRO(labelinit, locallab);
	    truelab = cadf->opc_truelab;	/* save across call */
	    cadf->opc_truelab = (char *)&locallab;
	    (VOID) opc_cqual1(global, cnode, root->pst_left, cadf,
			    segment, resop, (ADI_FI_ID)0, FALSE);
	    opc_adlabres(global, cadf, &locallab);
	    cadf->opc_truelab = truelab;		/* restore after call */
	    opc_adinstr(global, cadf, ADE_ANDL, segment, 1,
		(ADE_OPERAND *)cadf->opc_falselab, 0);
	    (VOID) opc_cqual1(global, cnode, root->pst_right, cadf,
			    segment, resop, (ADI_FI_ID)0, FALSE);
	}

	break;

     case PST_OR:
	/* 2 streams of logic - old logic assumes expression is in
	** conjunctive normal form. New logic assumes it isn't, and that
	** AND/OR execution may involve branching through ADF expr */
        if (!(subqry && (subqry->ops_mask & OPS_NONCNF)) &&
	    cadf->opc_whenlvl == 0)
	{	/* the old-fashioned way */
	    /* first compile the left, then compile the or instruction, finally
	    ** compile the right. Unlike AND nodes, OR nodes form a true binary
	    ** tree (not a list), so the best, least complicated solution is
	    ** recursive.
	    */
	    (VOID) opc_cqual1(global, cnode, root->pst_left, cadf,
		    segment, resop, (ADI_FI_ID)0, FALSE);
	    opc_adinstr(global, cadf, ADE_OR, segment, 0, resop, 0);
	    (VOID) opc_cqual1(global, cnode, root->pst_right, cadf,
		    segment, resop, (ADI_FI_ID)0, FALSE);
	}
	else
	{	/* the new-fashioned way */
	    ADE_OPERAND	locallab;
	    char	*falselab;

	    /* Left subtree of OR being false should jump to the OR
	    ** (skipping what's in between). Trues on either side just
	    ** join the true list. */
	    STRUCT_ASSIGN_MACRO(labelinit, locallab);
	    falselab = cadf->opc_falselab;	/* save across call */
	    cadf->opc_falselab = (char *)&locallab;
	    (VOID) opc_cqual1(global, cnode, root->pst_left, cadf,
			    segment, resop, (ADI_FI_ID)0, FALSE);
	    opc_adlabres(global, cadf, &locallab);
	    cadf->opc_falselab = falselab;	/* restore after call */
	    opc_adinstr(global, cadf, ADE_ORL, segment, 1,
		(ADE_OPERAND *)cadf->opc_truelab, 0);
	    (VOID) opc_cqual1(global, cnode, root->pst_right, cadf,
			    segment, resop, (ADI_FI_ID)0, FALSE);
	}

	break;

     case PST_CASEOP:
     {
	PST_QNODE	*whlistp, *thenp;
	ADE_OPERAND	localbrf, localbr, whentrue, whenfalse;
	ADE_OPERAND	result, *oldres;
	char		*oldtrue, *oldfalse, *oldwhen;
	i4		align, oldoff;
	bool		noelse = FALSE, iftrue;

	/* The case function contains several parts, each of which generates
	** code into the ADF expression. For "simple" case functions, the 
	** source expression is compiled to produce a value in a temporary
	** location for the upcoming comparisons (if I can figure out how to
	** do it. Otherwise, we'll recompile the source expression for each
	** when clause). 
	**
	** Then for each when clause, the comparison or search expression is 
	** compiled, followed by a "BRANCHF" (to bypass the result evaluation
	** when the comparison is FALSE), followed by the result expression 
	** for the when clause, followed by a "BRANCH" (to exit the case).
	**
	** To avoid compiling a MECOPY for each when clause to move the result 
	** to the eventual result location, the "resop" parm sent to compile
	** the result expressions is filled in with the actual target location 
	** information of the case function. 
	*/

	oldres = resop;		/* save around the case function */

	/* Need to allocate the result location here. Note, if resop
	** already contains a valid location (opr_offset >= 0), it is
	** because this is a nested case, but new result must still be
	** init'ed. */

	result.opr_dt = root->pst_sym.pst_dataval.db_datatype;
	result.opr_len = root->pst_sym.pst_dataval.db_length;
	result.opr_prec = root->pst_sym.pst_dataval.db_prec;
	result.opr_collID = root->pst_sym.pst_dataval.db_collID;
	result.opr_base = cadf->opc_row_base[OPC_CTEMP];
	result.opr_offset = global->ops_cstate.opc_temprow->opc_olen;

	/* Re-align the temp buffer. */
	align = result.opr_len % sizeof(ALIGN_RESTRICT);
	if (align != 0) align = sizeof(ALIGN_RESTRICT) - align;
	global->ops_cstate.opc_temprow->opc_olen += align + result.opr_len;

	/* If resop not init'ed yet, make it the local result. */
	if (resop == NULL || resop->opr_offset == OPR_NO_OFFSET)
	{
	    resop = &result;
	}
	oldoff = resop->opr_offset;

	/* Save old label anchors and set up new ones. */
	oldtrue = cadf->opc_truelab;
	oldfalse = cadf->opc_falselab;
	oldwhen = cadf->opc_whenlab;
	STRUCT_ASSIGN_MACRO(labelinit, localbr);
	cadf->opc_whenlab = (char *)&localbr;

	/* Simple case setup goes here (if I figure out how to do it). */


	/* Loop over the when clauses, building code as we go. */
	for (whlistp = root->pst_left; whlistp != (PST_QNODE *)NULL;
		whlistp = whlistp->pst_left)
	{
	    /* Reset label variables in case "and"/"or" are present in "when"s. */
	    cadf->opc_truelab = (char *)&whentrue;
	    cadf->opc_falselab = (char *)&whenfalse;
	    STRUCT_ASSIGN_MACRO(labelinit, whentrue);
	    STRUCT_ASSIGN_MACRO(labelinit, whenfalse);
	    if (noelse = (whlistp->pst_right->pst_left != (PST_QNODE *)NULL))
	    {
		/* This stuff only executes for "when" clauses (i.e., with 
		** comparison expressions to compile). */

		/* Prepare label. */
		STRUCT_ASSIGN_MACRO(labelinit, localbrf);
		/* Build the comparison code. */
		/* Since "when"s of "case" with a source expression are currently 
		** compiled as "<source expr> = <comparand expr>", no distinction
		** is made between SEARCHED and SIMPLE cases. */
		if (TRUE || root->pst_sym.pst_value.pst_s_case.pst_casetype ==
				PST_SEARCHED_CASE)
		{
		    cadf->opc_whenlvl++;	/* to indicate noncnf processing */
		    (VOID) opc_cqual1(global, cnode, whlistp->pst_right->pst_left,
			cadf, segment, (ADE_OPERAND *)NULL, (ADI_FI_ID)0, TRUE);
					/* compile serach expression */
		    cadf->opc_whenlvl--;
		}
		else
		{
		}

		/* Emit "BRANCHF" operator (to skip to next when). */
		opc_adinstr(global, cadf, ADE_BRANCHF, segment, 1, &localbrf, 0);
	    }

	    /* Compile corresponding result expression, first resolving any
	    ** OR true labels encountered in the when expression. */
	    opc_adlabres(global, cadf, &whentrue);

	    if ((thenp = whlistp->pst_right->pst_right)->pst_sym.pst_type
		== PST_BOP && thenp->pst_sym.pst_value.pst_s_op.pst_opno
		== global->ops_cb->ops_server->opg_iftrue)
	    {
		/* THEN starts with ii_iftrue - ... */
		iftrue = TRUE;
		oldoff = resop->opr_offset;
		resop->opr_offset = OPR_NO_OFFSET;
	    }
	    else iftrue = FALSE;

	    (VOID) opc_cqual1(global, cnode, thenp, cadf, segment, resop, (ADI_FI_ID)0, TRUE);

	    if (iftrue)
	    {
		/* Now we must copy the stupid thing to resop. */
		STRUCT_ASSIGN_MACRO(*resop, ops[0]);
		STRUCT_ASSIGN_MACRO(*resop, ops[1]);
		ops[0].opr_offset = resop->opr_offset = oldoff;
		opc_adinstr(global, cadf, ADE_MECOPY, segment, 2, ops, 1);
	    }

	    /* Emit unconditional "BRANCH" operator (to exit case). */
	    if (whlistp->pst_left)	/* only if NOT last when or else */
		opc_adinstr(global, cadf, ADE_BRANCH, segment, 1, 
					(ADE_OPERAND *)cadf->opc_whenlab, 0);

	    /* Resolve FALSE branch to next when (or else). If this was the
	    ** else, there's no label to resolve & we just drop out of loop. */
	    opc_adlabres(global, cadf, &whenfalse);
	    if (noelse) opc_adlabres(global, cadf, &localbrf);
	}	/* end of when loop */

	/* If there was no else clause, we must assign NULL to the result here. */
	if (noelse)
	{
	    /* This case doesn't arise, because the parser forces a "else NULL"
	    ** in the absence of a coded "else". */
	}

	/* Finally, resolve the unconditional branch here. */
	opc_adlabres(global, cadf, (ADE_OPERAND *)cadf->opc_whenlab);

	cadf->opc_truelab = oldtrue;
	cadf->opc_falselab = oldfalse;
	cadf->opc_whenlab = oldwhen;
	if (resop == oldres)
	    break;
	else resop = oldres;

	/* See if result must now be coerced. */
	if ((resop->opr_dt == result.opr_dt ||
		resop->opr_dt == -result.opr_dt) &&
	    (resop->opr_len == OPC_UNKNOWN_LEN ||
	     (resop->opr_len == result.opr_len &&
	      (resop->opr_prec == result.opr_prec ||
	      abs(result.opr_dt) != DB_DEC_TYPE) &&
	     resop->opr_dt == result.opr_dt)))
	{
	    STRUCT_ASSIGN_MACRO(result, *resop);
				/* no coercion - just copy result opnd */
	}
	else
	{
	    /* Must coerce result into desired form. */
	    opc_adtransform(global, cadf, &result, resop, segment, (bool)TRUE,
			(bool)FALSE);
	}

	break;
     }	/* end of PST_CASEOP */
	
     case PST_UOP:
     case PST_BOP:
     case PST_COP:
     case PST_MOP:
     {
	ADI_FI_DESC	    fi = *root->pst_sym.pst_value.pst_s_op.pst_fdesc;
	i4		    opnum;  /* The number of operands used in 'ops' */
	i4		    align;  /* used to compute the alignment of results. */
    	i4		    i;
    	PST_QNODE	    *lqnode;
        PST_QNODE           *resqnode;
	PST_QNODE	    *inlist;
	i4		    lret;
	PTR		    data[ADI_MAX_OPERANDS];
	PTR		    *dataptr = &data[0];
        ADE_OPERAND	    *opsptrs[ADI_MAX_OPERANDS];
	ADE_OPERAND	    **opsptr = &opsptrs[0];
	ADI_FI_ID	    cnvrtid;
	i1		    leftvisited = FALSE;
	i1		    comparison;
	i1		    constexpr = TRUE;  /* assume constant expression */
	i4		    xorin = 0;
	bool		    notin;
	bool		    result_at_resop = FALSE;

	/* This is the meat and potatoes of query tree compilation.  Here
	** we compile those instructions needed to compute zero, one, and
	** two operand operators. We begin by assuming that there are
	** zero operands.
	*/
	opnum = 0;

	if (incase)
	    constexpr = FALSE;	/* case exprs must go into MAIN */

	/*
	**  ADI_DBMSI_OP : If query is not a dbmsinfo() query, like ADI_RESTAB_OP
	**  resolve_table() function eg,
	**    select resolve_table('tableowner', 'tablename')
	**  is what gets sent for 'help tableowner.tablename'
	**  This looks like a constant function but isn't really since
	**  it requires catalog information. Another example will be
	**  if we are using a 
	**	select c(long_byte('abcdefg',10)) 
	**  query, this will create a transaction as we involve temporary tables.
	**  Set a flag in qp_status indicating we need to start a transaction 
	**  to process this query
	*/
	if (fi.adi_fitype == ADI_NORM_FUNC
	    && (root->pst_sym.pst_value.pst_s_op.pst_opno == ADI_DBMSI_OP))
	{
	    /*
	    ** Set xorin to QEQP_NEED_TRANSACTION if this is dbmsinfo, and
	    ** we haven't already decided to open a transaction.  This is
	    ** to allow us to cancel out QEQP_NEED_TRANSACTION being asserted
	    ** due to the recursive calls processiong the argument to
	    ** dbmsinfo.
	    */
	    xorin = QEQP_NEED_TRANSACTION &
	     ( global->ops_cstate.opc_qp->qp_status ^ QEQP_NEED_TRANSACTION );
	}

	/* if this operator type is not a comparison then there must
	** be a result. Increment this to save space for the result
	** operand (note that in ADF, the zeroth operand is always for
	** for the result operand, if there is one).
	*/
	if (fi.adi_fitype == ADI_COMPARISON)
	{
	    comparison = TRUE;
	}
	else
	{
	    opnum = 1;
	    comparison = FALSE;
	}

	i = 0;
	if (root->pst_sym.pst_type == PST_MOP)
	{
            resqnode = root;
            lqnode = resqnode->pst_right;
	    cnvrtid = resqnode->pst_sym.pst_value.pst_s_op.pst_oprcnvrtid;
	}
	else
	{
	    lqnode = root->pst_left;    /* Start down the left side */
	    cnvrtid = root->pst_sym.pst_value.pst_s_op.pst_oplcnvrtid;
	    if (lqnode == NULL)
	    {
		lqnode = root->pst_right;   /* do the right side */
		leftvisited = TRUE;     /* say we've already been there */
		cnvrtid = root->pst_sym.pst_value.pst_s_op.pst_oprcnvrtid;
		i = 1;          /* skip to 2nd operand */
	    }
	}

	if (root->pst_sym.pst_value.pst_s_op.pst_flags & PST_INLIST &&
		root->pst_right->pst_sym.pst_type == PST_CONST)
	{
	    inlist = root->pst_right->pst_left;
	    if (root->pst_sym.pst_value.pst_s_op.pst_opno == 
		global->ops_cb->ops_server->opg_ne)
		    notin = TRUE;
	    else notin = FALSE;

	    /* Generate mass compare instructions for IN lists. */
	    if (root->pst_sym.pst_value.pst_s_op.pst_flags & PST_INLISTSORT)
	    {
		ADE_OPERAND *Pops;
		i4 cnt = 1; /* For LHS */
		inlist = root->pst_right;
		do
		    cnt++; /* count in the RHSs */
		while (inlist = inlist->pst_left);
		inlist = root->pst_right;
		Pops = (ADE_OPERAND*)MEreqmem(0, sizeof(ADE_OPERAND)*cnt, FALSE, NULL);
		/* Generate LHS we are not interested in the const state of the
		** LHS as the parser will not generate PST_INLIST in that case. */
		Pops[0].opr_dt = abs(root->pst_left->pst_sym.pst_dataval.db_datatype);
		Pops[0].opr_len = OPC_UNKNOWN_LEN;
		Pops[0].opr_prec = OPC_UNKNOWN_PREC;
		Pops[0].opr_collID = root->pst_left->pst_sym.pst_dataval.db_collID;
		Pops[0].opr_offset = OPR_NO_OFFSET;
		(VOID) opc_cqual1(global, cnode, lqnode,
                        cadf, segment, &Pops[0], cnvrtid, FALSE);
		for (i = 1; i < cnt; i++)
		{
		    /* This is a list of constants that will be in sorted order
		    ** so compile them into the CX and fill in ops[1]..ops[n] */
		    opc_adconst(global, cadf, &inlist->pst_sym.pst_dataval, &Pops[i],
			    inlist->pst_sym.pst_value.pst_s_cnst.pst_cqlang, ADE_SVIRGIN);
		    inlist = inlist->pst_left;
		}
		opc_adinstr(global, cadf, notin?ADE_NCOMPAREN
						 :ADE_COMPAREN, segment, cnt, Pops, 0);
		MEfree((PTR)Pops);
		/* Drop out - nothing more to do. */
		break;
	    }
	}
	else inlist = (PST_QNODE *) NULL;

	while (lqnode)
	{
	    /* Compile this node. */

	    /* Figure out what the desired datatype for the result of the
	    ** left side is and store it where the recursive call to opc_cqual1
	    ** will look to see if coercions are required.
	    ** The numargs check is for VARARGS user functions.
	    */
	    ops[opnum].opr_dt = DB_ALL_TYPE;
	    if (i < fi.adi_numargs)
		ops[opnum].opr_dt = fi.adi_dt[i];

	    if (ops[opnum].opr_dt == DB_ALL_TYPE)
	    {
		ops[opnum].opr_dt = lqnode->pst_sym.pst_dataval.db_datatype;
	    }
	    else if (lqnode->pst_sym.pst_dataval.db_datatype < 0)
	    {
		ops[opnum].opr_dt = -ops[opnum].opr_dt;
	    }

	    /* Fix for bug 42363. Check for comparison operator and if
            ** found initialize the len and prec to UNKNOWN, else
            ** initialize to the appropriate len and prec for that
            ** node.
            */
            if (comparison)
            {
		ops[opnum].opr_len = OPC_UNKNOWN_LEN;
		ops[opnum].opr_prec = OPC_UNKNOWN_PREC;
		if (lqnode)
		    ops[opnum].opr_collID = lqnode->
					pst_sym.pst_dataval.db_collID;
		else ops[opnum].opr_collID = -1;
            }
            else
            {
               ops[opnum].opr_len = lqnode->pst_sym.pst_dataval.db_length;
               ops[opnum].opr_prec = lqnode->pst_sym.pst_dataval.db_prec;
               ops[opnum].opr_collID = lqnode->pst_sym.pst_dataval.db_collID;
	    }

	    ops[opnum].opr_offset = OPR_NO_OFFSET;	/* show no result loc. */
	    /* Trick it into using reasonable scale if coercing to DECIMAL
	    ** for a binary operation (major kludge). */
	    if ((root->pst_sym.pst_type == PST_BOP) &&
		comparison &&
		(!leftvisited &&
		(abs(root->pst_right->pst_sym.pst_dataval.db_datatype) ==
							    DB_DEC_TYPE) &&
		(abs(root->pst_right->pst_sym.pst_dataval.db_datatype) !=
			abs(root->pst_left->pst_sym.pst_dataval.db_datatype)) &&
		((abs(root->pst_left->pst_sym.pst_dataval.db_datatype) ==
							    DB_VCH_TYPE) ||
		(abs(root->pst_left->pst_sym.pst_dataval.db_datatype) ==
							    DB_CHA_TYPE) ||
		(abs(root->pst_left->pst_sym.pst_dataval.db_datatype) ==
							    DB_TXT_TYPE) ||
		(abs(root->pst_left->pst_sym.pst_dataval.db_datatype) ==
							    DB_CHR_TYPE))) ||
		(leftvisited &&
		(abs(root->pst_left->pst_sym.pst_dataval.db_datatype) ==
							    DB_DEC_TYPE) &&
		(abs(root->pst_right->pst_sym.pst_dataval.db_datatype) !=
			abs(root->pst_left->pst_sym.pst_dataval.db_datatype)) &&
		((abs(root->pst_right->pst_sym.pst_dataval.db_datatype) ==
							    DB_VCH_TYPE) ||
		(abs(root->pst_right->pst_sym.pst_dataval.db_datatype) ==
							    DB_CHA_TYPE) ||
		(abs(root->pst_right->pst_sym.pst_dataval.db_datatype) ==
							    DB_TXT_TYPE) ||
		(abs(root->pst_right->pst_sym.pst_dataval.db_datatype) ==
							    DB_CHR_TYPE))))
	    {
		global->ops_gmask |= OPS_DECCOERCION;
		if (!leftvisited)
		    ops[opnum].opr_prec =
				root->pst_right->pst_sym.pst_dataval.db_prec;
		else
		    ops[opnum].opr_prec =
				root->pst_left->pst_sym.pst_dataval.db_prec;
	    }
	    lret = opc_cqual1(global, cnode, lqnode,
			cadf, segment, &ops[opnum], cnvrtid, FALSE);
	    global->ops_gmask &= (~OPS_DECCOERCION);
	    if (lret != OPC_CNSTEXPR)
	    	constexpr = FALSE;

	    *dataptr = lqnode->pst_sym.pst_dataval.db_data;
	    dataptr++;
	    *opsptr = &ops[opnum];
	    opsptr++;
	    i++;
	    opnum += 1;

     	    if (root->pst_sym.pst_type == PST_MOP)
            {
		resqnode = resqnode->pst_left;
        	if (!resqnode)
                    break;
                lqnode = resqnode->pst_right;   /* point to the operand */
		/* We should set cnvrtid from resqnode->pst_sym.pst_value
		** .pst_s_op.pst_oprcnvrtid except this is a PST_OPERAND
		** node that doesn't have pst_oprcnvrtid! For now it is
		** sufficient that the value is not ADI_NILCOERCE */
		cnvrtid = 0;
            }
	    else
	    {
	    	if (leftvisited)
		    break;
		leftvisited = TRUE;
	    	lqnode = root->pst_right;  /* go to right side */
		cnvrtid = root->pst_sym.pst_value.pst_s_op.pst_oprcnvrtid;
	    }
	}

	if (constexpr && !comparison &&
		fi.adi_fitype != ADI_COMPARISON
		&&
		!(fi.adi_fiflags & ADI_F32_NOTVIRGIN)
	    )
	{
	    /* If both the left and right trees are constant expressions and
	    ** we are not doing a comparison, then this must be a constant
	    ** expression too. This means that we
	    ** can save execution time by compiling this operation into the
	    ** VIRGIN segment. This will insure that the operation will be
	    ** executed only once during the query execution. It is OK to
	    ** execute a constant expression once during a query, since it
	    ** after all, constant
	    **
	    ** The exceptions to this rule are several parameter-less and 
	    ** constant parm functions (random's and uuid_create for now) that
	    ** are expected to return a different value each time the MAIN
	    ** code segment is executed.
	    */
	    ret = OPC_CNSTEXPR;
	    segment = ADE_SVIRGIN;
	}

	/* if no result is required */
	if (comparison)
	{
	    /* These operators have no result except the change in ADF
	    ** state that they cause.
	    */
	    resop = NULL;
	}
	else
	{
	    /* Otherwise a result is required. If result location has already 
	    ** been determined (as inside a case function), stick it there. Else
	    ** fill in the result operand slot in the ops array.
            **
            ** You cannot do this for ii_iftrue() however because it requires
            ** the result location have a specific length.
            **
            ** If the lengths are different it may be because the ADI LENSPEC
            ** is fixed in which case we must not override the length with that
            ** from from the resop.
	    */
	    if (resop && resop->opr_offset != OPR_NO_OFFSET &&
		resop->opr_dt == root->pst_sym.pst_dataval.db_datatype &&
                resop->opr_len == root->pst_sym.pst_dataval.db_length &&
                root->pst_sym.pst_value.pst_s_op.pst_opno !=
                 global->ops_cb->ops_server->opg_iftrue &&
		(abs(resop->opr_dt) != DB_DEC_TYPE ||
			resop->opr_prec == root->pst_sym.pst_dataval.db_prec))
	    {
		result_at_resop = TRUE;
		STRUCT_ASSIGN_MACRO(*resop, ops[0]);
	    }
	    else
	    {
		ops[0].opr_dt = 
		    root->pst_sym.pst_dataval.db_datatype;
		/* Update temp copy lenspec if dtfamily result */
		if (abs(fi.adi_dtresult) == DB_DTE_TYPE)
		{
		    adi_coerce_dtfamily_resolve(global->ops_adfcb, &fi, 
				fi.adi_dt[0], ops[0].opr_dt, &fi);
		}

		/* This fixes for bug 93275 (ingsrv 533). Problem occurs
		** on PST_BOP and PST_UOP with resop, if the PST_VAR for
		** the PST_BOP/PST_UOP is defined as not null and the variable
		** is part of an equivalance class, which is defined as nullable.
		** This will change the input oprand to nullable (in the PST_VAR
		** section), but the resop (opr[0]) still gets set from the PST_BOP
		** /PST_UOP datatype, which is not null.
		** The fix checks, if one of the input operand is nullable and the
		** resop isn't. In this case the resop gets changed to nullable.
		** Don't screw with UDF's that want exact-type params
		** (really ought to carry the nonullskip flag in the FIdesc!)
		*/
		if (fi.adi_npre_instr != ADE_0CXI_IGNORE
		  && ops[0].opr_dt > 0 && opnum > 1)
		{
		    for (i = 1; i < opnum; ++i)
		    {
			if (ops[i].opr_dt < 0)
			{
			    ops[0].opr_dt = -ops[0].opr_dt;
			    break;
			}
		    }
		}
		ops[0].opr_base = cadf->opc_row_base[OPC_CTEMP];
		ops[0].opr_offset = global->ops_cstate.opc_temprow->opc_olen;
		ops[0].opr_collID = root->pst_sym.pst_dataval.db_collID;

		/* calculate the length of the result */
		if (opnum == 1)
		{
		    opc_adcalclen(global, cadf,
			&fi.adi_lenspec,
			0, (ADE_OPERAND **)NULL, &ops[0], (PTR *)NULL);
		}
		else
		{
		    opc_adcalclen(global, cadf,
			&fi.adi_lenspec,
			opnum-1, opsptrs, &ops[0], data);
		}

		/* increase the temp results buffer to hold our new value. */
		if (ops[0].opr_len > 0)
		{
		    align = ops[0].opr_len % sizeof (ALIGN_RESTRICT);
		    if (align != 0)
			align = sizeof (ALIGN_RESTRICT) - align;
		    global->ops_cstate.opc_temprow->opc_olen += ops[0].opr_len + align;
		}
	    }
	}

	/* if this operand has an escape char (eg. the like operator) then
	** compile that into the CX. Code copied to separate function to reduce
	** opc_cqual1 stack frame size.
	*/
	if (root->pst_sym.pst_value.pst_s_op.pst_pat_flags != AD_PAT_DOESNT_APPLY)
	    opc_cescape(global, cadf, root, segment);

	/* Now that we have completely filled in ops and opnum, we can
	** compile the instruction for this operator.
	*/
	if (resop == NULL)
	{
	    opc_adinstr(global, cadf,
			    root->pst_sym.pst_value.pst_s_op.pst_opinst,
						    segment, opnum, ops, 0);
	}
	else
	{
	    opc_adinstr(global, cadf,
			    root->pst_sym.pst_value.pst_s_op.pst_opinst,
						    segment, opnum, ops, 1);

	    /* If we compiled the result into the exact resop location
	    ** above, we're done.
	    */
	    if (result_at_resop)
		break;

	    /* If (the datatype/length/precision either don't matter or
	    **	    are the same
	    **	  )
	    */
	    if ((resop->opr_dt == ops[0].opr_dt ||
		 rescnvtid == ADI_NILCOERCE ||
		 resop->opr_dt == -ops[0].opr_dt
		) &&
	        (resop->opr_len == OPC_UNKNOWN_LEN ||
		 (resop->opr_len == ops[0].opr_len &&
		  (resop->opr_prec == ops[0].opr_prec ||
		   abs(ops[0].opr_dt) != DB_DEC_TYPE
		  ) &&
		  (resop->opr_dt == ops[0].opr_dt ||
		   rescnvtid == ADI_NILCOERCE
		  )
		 )
	        )
	       )
	    {
		/* assign the base and index numbers to resop. */
		STRUCT_ASSIGN_MACRO(ops[0], *resop);
	    }
	    else
	    {
		/* NOTE: if rescnvrtid is >0, pst_resolve() has previously
		** determined which coercion FI is required for this operand
		** thus opc_adtransform() which we are about to call could
		** use this information to bypass most of its processing and
		** instead could just generate the coercion FI.
		** For the moment, we just use the rescnvrtid==ADI_NILCOERCE
		** above to avoid the unnecessary (& maybe incorrect) coercion
		** and leave to another day, making fuller use of the
		** cnvrtid information that is available. */

		/* otherwise coerce (transform) the result of this operator
		** into the requested type. Note: resop is set in
		** opc_adtransform ONLY if it hasn't already been set. 
		*/
		opc_adtransform(global, cadf, &ops[0],
		    resop, segment, 
		    (bool)(resop->opr_offset == OPR_NO_OFFSET),
			(bool) FALSE);
	    }
	}

	/* Magic loop to iterate over [not] in-list rather than recurse.
	** Hopefully, this will eliminate the IN-list blown stack
	** problems forever. */
	if (inlist != (PST_QNODE *) NULL)
	{
	    ADE_OPERAND	locallab;
	    char	*savelab;
	    i4		opcode;

	    STRUCT_ASSIGN_MACRO(labelinit, locallab);
	    if (notin)
	    {
		opcode = ADE_ANDL;
		savelab = cadf->opc_truelab;
		cadf->opc_truelab = (char *)&locallab;
	    }
	    else
	    {
		opcode = ADE_ORL;
		savelab = cadf->opc_falselab;
		cadf->opc_falselab = (char *)&locallab;
	    }
 
	    for (; inlist; inlist = inlist->pst_left)
	    {
		/* Insert a OR/AND followed by another "="/"<>" comparison
		** for each remaining PST_CONST in the constant list. */
		opc_adinstr(global, cadf, opcode, segment, 1, 
		    (ADE_OPERAND *) ((notin) ? 
			cadf->opc_truelab : cadf->opc_falselab), 0);
	    
        	ops[1].opr_len = OPC_UNKNOWN_LEN;
        	ops[1].opr_prec = OPC_UNKNOWN_PREC;
        	ops[1].opr_collID = -1;
		ops[1].opr_offset = OPR_NO_OFFSET;	/* show no result loc. */
		/* Trick it into using reasonable scale if coercing to DECIMAL
		** for a binary operation (major kludge). */
		if ((abs(root->pst_left->pst_sym.pst_dataval.db_datatype) ==
							    DB_DEC_TYPE) &&
		    (abs(inlist->pst_sym.pst_dataval.db_datatype) !=
			abs(root->pst_left->pst_sym.pst_dataval.db_datatype)) &&
		    ((abs(inlist->pst_sym.pst_dataval.db_datatype) ==
							    DB_VCH_TYPE) ||
		    (abs(inlist->pst_sym.pst_dataval.db_datatype) ==
							    DB_CHA_TYPE) ||
		    (abs(inlist->pst_sym.pst_dataval.db_datatype) ==
							    DB_TXT_TYPE) ||
		    (abs(inlist->pst_sym.pst_dataval.db_datatype) ==
							    DB_CHR_TYPE)))
		{
		    global->ops_gmask |= OPS_DECCOERCION;
		    ops[1].opr_prec = root->pst_left->
					pst_sym.pst_dataval.db_prec;
		}
		lret = opc_cqual1(global, cnode, inlist,
			cadf, segment, &ops[1], (ADI_FI_ID)0, FALSE);
					/* materialize/coerce constant */
		global->ops_gmask &= (~OPS_DECCOERCION);

		opc_adinstr(global, cadf,
			    root->pst_sym.pst_value.pst_s_op.pst_opinst,
						    segment, 2, ops, 0);
					/* perform "="/"<>" comparison */
	    }
	    opc_adlabres(global, cadf, &locallab);
	    if (notin)
		cadf->opc_truelab = savelab;
	    else cadf->opc_falselab = savelab;
	}

	global->ops_cstate.opc_qp->qp_status ^= xorin;
	break;
     }

     case PST_CONST:
     {
	/* parms and parm_no are for repeat query parameter const nodes. */
	OPC_BASE	    *parms;
	i4		    parm_no;
	i4		    oldseg = segment;
	i4		    align;
	ADE_OPERAND	    uniop;
	bool		    unorm = FALSE;
	bool		    utf8unorm = FALSE;
	bool		    aggresult = FALSE;

	/* first, figure out what type of constant this is */
	parm_no = root->pst_sym.pst_value.pst_s_cnst.pst_parm_no;

	/* if it is known that a relevant Quel pattern matching character
	** may be present, set the query language appropriately */
	opc_adseglang(global, cadf,
		    root->pst_sym.pst_value.pst_s_cnst.pst_cqlang, segment);

	/* Before proceeding, determine if result is Unicode type. */
	if (resop && (global->ops_adfcb->adf_utf8_flag & AD_UTF8_ENABLED) &&
		(abs(resop->opr_dt) == DB_VCH_TYPE ||
		 abs(resop->opr_dt) == DB_CHA_TYPE ||
		 abs(resop->opr_dt) == DB_TXT_TYPE ||
		 abs(resop->opr_dt) == DB_CHR_TYPE))
	    utf8unorm = TRUE;

	if ((resop && (abs(resop->opr_dt) == DB_NCHR_TYPE ||
		abs(resop->opr_dt) == DB_NVCHR_TYPE)) ||
		utf8unorm)
	    unorm = TRUE;

	/* FIX ME need to remove all unorm/utf8unorm code below */
	unorm = utf8unorm = FALSE;

	switch (root->pst_sym.pst_value.pst_s_cnst.pst_tparmtype)
	{
	 case PST_RQPARAMNO:
	    /* if this is a user generated repeat query parameter (as opposed
	    ** to an optimizer generated one) then do any compiling into the
	    ** virgin segment. See the comment in the operator case above.
	    */
	    if (parm_no <= global->ops_qheader->pst_numparm)
	    {
		ret = OPC_CNSTEXPR;
		segment = ADE_SVIRGIN;
	    }
	    else aggresult = TRUE;		/* result of agg comp */

	    /* this is a repeat query parameter, so add the special QEF
	    ** QEF supported CX base for the parameter in question and
	    ** fill in ops[0] to refer to it.
	    */
	    parms = global->ops_cstate.opc_rparms;

	    ops[0].opr_dt = parms[parm_no].opc_dv.db_datatype;
	    ops[0].opr_prec = parms[parm_no].opc_dv.db_prec;
	    ops[0].opr_collID = parms[parm_no].opc_dv.db_collID;
	    ops[0].opr_len = parms[parm_no].opc_dv.db_length;
	    ops[0].opr_offset = parms[parm_no].opc_offset;
	    ops[0].opr_base = cadf->opc_repeat_base[parm_no];

	    opc_adparm(global, cadf, parm_no,
			root->pst_sym.pst_value.pst_s_cnst.pst_cqlang, segment);

	    if (ops[0].opr_base < ADE_ZBASE ||
		    ops[0].opr_base >= ADE_ZBASE + cadf->opc_qeadf->qen_sz_base
		)
	    {
		opc_adbase(global, cadf, QEN_PARM, parm_no);
		ops[0].opr_base = cadf->opc_repeat_base[parm_no];
	    }
	    opc_adseglang(global, cadf,
		    root->pst_sym.pst_value.pst_s_cnst.pst_cqlang, segment);
	    break;

	 case PST_LOCALVARNO:
	    /* this is a database procedure local variable reference.
	    ** Opc_lvar_row will fill in ops[0] for us.
	    */
	    opc_lvar_row(global, parm_no, cadf, &ops[0]);
	    opc_adseglang(global, cadf,
		    root->pst_sym.pst_value.pst_s_cnst.pst_cqlang, segment);
	    break;

	 case PST_USER:
	    /* This is a constant expression, so do any compiling into the
	    ** virgin segment. See the comment in the operator case above.
	    ** One special case however, TRUE or FALSE should be instanciated
	    ** as instructions ADE_SETTRUE or ADE_SETFALSE instead of going
	    ** into VIRGIN segment.
	    */
	    if (root->pst_sym.pst_dataval.db_datatype == DB_BOO_TYPE)
	    {
		opc_adinstr(global, cadf, !*(char*)root->pst_sym.pst_dataval.db_data
				? ADE_SETFALSE
				: ADE_SETTRUE, ADE_SMAIN, 0, 0, 0);
	    }
	    else if (root->pst_sym.pst_dataval.db_datatype == -DB_BOO_TYPE)
	    {
		opc_adinstr(global, cadf,
			(((char*)root->pst_sym.pst_dataval.db_data)[1] & ADF_NVL_BIT) ||
					!*(char*)root->pst_sym.pst_dataval.db_data
				? ADE_SETFALSE
				: ADE_SETTRUE, ADE_SMAIN, 0, 0, 0);
	    }
		ret = OPC_CNSTEXPR;
		segment = ADE_SVIRGIN;

		/* This is a constant that appears directly in the const
		** node, so compile it into the CX and fill in ops[0]/
		*/
		opc_adconst(global, cadf, &root->pst_sym.pst_dataval, &ops[0],
			    root->pst_sym.pst_value.pst_s_cnst.pst_cqlang,
								    segment);

	    break;

	 default:
	    opx_error(E_OP0897_BAD_CONST_RESDOM);
	}

	/* If this is a fake CONST constructed to pass an aggregate result,
	** no Unicode normalization is required. */
	if (aggresult)
	    unorm = utf8unorm = FALSE;

	if (resop != NULL)
	{
	    /* First prepare intermediate result for Unicode constant.
	    ** Constant is materialized here, then normalized into the
	    ** result location. */
	    if (unorm) 
	    {
		STRUCT_ASSIGN_MACRO(ops[0], uniop);
		uniop.opr_base = cadf->opc_row_base[OPC_CTEMP];
		uniop.opr_offset = global->ops_cstate.opc_temprow->opc_olen;
		if (resop->opr_len != OPC_UNKNOWN_LEN)
		{
		    /* For nchar and nvarchar we need to quadruple
		    ** the space to account for normalization bug.
		    */
		    if ((abs(uniop.opr_dt) == DB_NCHR_TYPE) ||
		    (abs(uniop.opr_dt) == DB_NVCHR_TYPE))
		    {
		      if(uniop.opr_dt < 0)
		      {
		        if (uniop.opr_dt == -DB_NVCHR_TYPE)
		        {
		          uniop.opr_len = 
			    ((resop->opr_len - 1 - DB_CNTSIZE) * OPC_UNORM_EXP_FACTOR) +
		          	1 + DB_CNTSIZE;
		          if(uniop.opr_len > DB_MAXSTRING)
		          {
		            uniop.opr_len = DB_MAXSTRING + DB_CNTSIZE + 1;
		    	  }
		    	  else if (uniop.opr_len == DB_CNTSIZE + 1)
		    	  {
		    	    uniop.opr_len = DB_CNTSIZE + 1 + ( DB_ELMSZ * 2);
		    	  }
		    	}
		    	else if (uniop.opr_dt == -DB_NCHR_TYPE)
		    	{
		    	  uniop.opr_len = ((resop->opr_len - 1) * OPC_UNORM_EXP_FACTOR) + 1;
		    	  if(uniop.opr_len > DB_MAXSTRING)
		    	  {
		    	    uniop.opr_len = DB_MAXSTRING + 1;
		    	  }
		    	  else if (uniop.opr_len == 1)
		    	  {
		    	    uniop.opr_len = 1 + ( DB_ELMSZ * 2);
		    	  }
		    	}
		      }
		      else
		      {
		        if (uniop.opr_dt == DB_NVCHR_TYPE)
		        {
		    	  uniop.opr_len = ((resop->opr_len - DB_CNTSIZE) * OPC_UNORM_EXP_FACTOR) 
		    			  + DB_CNTSIZE;
		    	  if(uniop.opr_len > DB_MAXSTRING)
		    	  {
		    	    uniop.opr_len = DB_MAXSTRING + DB_CNTSIZE;
		    	  }
		    	  else if (uniop.opr_len == DB_CNTSIZE)
		          {
		    	    uniop.opr_len = DB_CNTSIZE + ( DB_ELMSZ * 2);
		    	  }
		    	}
		        else if (uniop.opr_dt == DB_NCHR_TYPE)
		        {
		    	  uniop.opr_len = resop->opr_len * OPC_UNORM_EXP_FACTOR;
		    	  if(uniop.opr_len > DB_MAXSTRING)
		    	  {
		    	    uniop.opr_len = DB_MAXSTRING;
		    	  }
		    	  else if (uniop.opr_len == 0)
		    	  {
		    	    uniop.opr_len = DB_ELMSZ * 2;
		    	  }
		    	}
		      }
		   }
		   else
		    uniop.opr_len = resop->opr_len;
					/* set norm. target len from resdom */
		}
		else if (ops[0].opr_len != ADE_LEN_UNKNOWN) 
		{
                    /* otherwise, quadruple to make space
                    ** for normalize (KLUDGE alert!) 
		    ** Don't do this for UTF8 character set case as 
		    ** UTF8 is in normal form C which should occupy 
		    **same amount of space as input.
		    */
		  if (!(utf8unorm))
		  {
		    if(uniop.opr_dt < 0)
		    {
			/* (adc_dv->db_length - DB_CNTSIZE) % DB_ELMSZ must
			** equal zero or adc_1lenchk_rti() will 
			** return E_AD2005_BAD_DTLEN. Prevent this kludge
			** from causing later errors.
			*/
			uniop.opr_len = ((uniop.opr_len - 1 - DB_CNTSIZE) * OPC_UNORM_EXP_FACTOR) 
					+ 1 + DB_CNTSIZE;
			if(uniop.opr_len > DB_MAXSTRING)
                        {
			    uniop.opr_len = DB_MAXSTRING + 1;
			}
			else if (uniop.opr_len == DB_CNTSIZE + 1)
			{
				uniop.opr_len = DB_CNTSIZE + 1 + ( DB_ELMSZ * 2);
			}
		    }
		    else
		    {
			uniop.opr_len = ((uniop.opr_len - DB_CNTSIZE) * OPC_UNORM_EXP_FACTOR)
					+ DB_CNTSIZE;
			if(uniop.opr_len > DB_MAXSTRING)
                        {
			    uniop.opr_len = DB_MAXSTRING;
			}
			else if (uniop.opr_len == DB_CNTSIZE)
			{
				uniop.opr_len = DB_CNTSIZE + (DB_ELMSZ * 2);
			}
		    }
		  }
		}
		else if ( root->pst_sym.pst_value.pst_s_cnst.pst_tparmtype 
				== PST_RQPARAMNO)
		{
		  /* KLUDGE again */
		  uniop.opr_len = OPC_UNORM_RESULT_SIZE;
		  if (utf8unorm)
		  {
		    /* For UTF8 convert c and char to varchar. 
		    ** This will prevent frequent blank filling 
		    ** of normalized value that has just been 
		    ** allotted an arbitrary large length.
		    */
		    switch ( uniop.opr_dt)
		    { 
			case DB_CHA_TYPE:
			case DB_CHR_TYPE:
		          uniop.opr_dt = DB_VCH_TYPE;
			break;
			case -DB_CHA_TYPE:
			case -DB_CHR_TYPE:
		          uniop.opr_dt = -DB_VCH_TYPE;
			break;
		    }
		  }
		}
 
		/* Update next loc for the aligned memory */
		align = (uniop.opr_len % sizeof (ALIGN_RESTRICT));
		if (align != 0)
			align = sizeof (ALIGN_RESTRICT) - align;
		global->ops_cstate.opc_temprow->opc_olen += 
					    align + uniop.opr_len;
	    }

	    /* if the actual and intended result datatype/length/precisions
	    ** are the same, ignoring nullability (which is why we have the
	    ** second comparison with the negation), then no coercion is
	    ** needed.
	    */
	    if ((resop->opr_dt == ops[0].opr_dt ||
		 resop->opr_dt == -ops[0].opr_dt
	        ) &&
	        (resop->opr_len == OPC_UNKNOWN_LEN ||
		 utf8unorm ||
		 abs(resop->opr_dt) == DB_NCHR_TYPE ||
		 abs(resop->opr_dt) == DB_NVCHR_TYPE ||
		 (resop->opr_len == ops[0].opr_len &&
		  (resop->opr_prec == ops[0].opr_prec ||
		   abs(ops[0].opr_dt) != DB_DEC_TYPE
		  ) &&
		  resop->opr_dt == ops[0].opr_dt
		 )
	        )
	       )
	    {
		/* If resop->opr_offset isn't preset, just assign ops[0]
		** to resop. Otherwise, this is a case function result and
		** must be copied to the result location. */
		if (resop->opr_offset == OPR_NO_OFFSET)
		{
		    if (unorm)
		    {
			/* Allocate work space and normalize the constant into it. */
			STRUCT_ASSIGN_MACRO(ops[0], ops[1]);
			STRUCT_ASSIGN_MACRO(uniop, ops[0]);
			opc_adinstr(global, cadf, ADE_UNORM, segment, 2, ops, 1);
		    }
		    STRUCT_ASSIGN_MACRO(ops[0], *resop);
		}
		else
		{
		    /* Emit MECOPY to copy VAR value to result location. */
		    STRUCT_ASSIGN_MACRO(ops[0], ops[1]);
		    STRUCT_ASSIGN_MACRO(*resop, ops[0]);
		    opc_adinstr(global, cadf, (unorm) ? ADE_UNORM: ADE_MECOPY, 
			oldseg, 2, ops, 1);
		}
	    }
	    else
	    {
		/* otherwise coerce the actual result to the requested
		** datatype. Note, we don't set result attributes if this
		** is a case function result.
		*/
		if (resop->opr_offset == OPR_NO_OFFSET)
		{
		    bool adnorm = FALSE;

		    opc_adtransform(global, cadf, &ops[0], resop, segment,
			(bool)TRUE, (bool)FALSE);

		    if  ((unorm && 
			 !(utf8unorm) &&
			 !((abs(resop->opr_dt) == DB_NCHR_TYPE && 
			   abs(ops[0].opr_dt) != DB_NVCHR_TYPE || 
			   abs(resop->opr_dt) == DB_NVCHR_TYPE && 
			   abs(ops[0].opr_dt) != DB_NCHR_TYPE) &&
			   abs(resop->opr_dt) != abs(ops[0].opr_dt)
			  )) 
			 &&
			   (abs(resop->opr_dt) != abs(ops[0].opr_dt))
			)
			  adnorm = TRUE;

		    /* Make sure that we don't attempt to normalize 
		    ** non UTF8 types (BUG 120172).
		    */
		    if ((utf8unorm) &&
			(abs(resop->opr_dt) != abs(ops[0].opr_dt)) &&
		 	(abs(resop->opr_dt) == DB_CHR_TYPE || 
			 abs(resop->opr_dt) == DB_CHA_TYPE || 
			 abs(resop->opr_dt) == DB_VCH_TYPE || 
			 abs(resop->opr_dt) == DB_TXT_TYPE) && 
			(abs(ops[0].opr_dt) == DB_CHR_TYPE || 
			 abs(ops[0].opr_dt) == DB_CHA_TYPE || 
			 abs(ops[0].opr_dt) == DB_VCH_TYPE || 
			 abs(ops[0].opr_dt) == DB_TXT_TYPE)
		       )
		    {
			if ( abs(resop->opr_dt) == DB_CHR_TYPE || 
			         abs(resop->opr_dt) == DB_CHA_TYPE )
			{
			    if (abs(ops[0].opr_dt) != DB_VCH_TYPE)
			          adnorm = TRUE;	
			}
			else if ( abs(resop->opr_dt) == DB_VCH_TYPE )
			{
			    if (!( abs(ops[0].opr_dt) == DB_CHR_TYPE || 
			              abs(ops[0].opr_dt) == DB_CHA_TYPE )) 
			         adnorm = TRUE;
			}
		    }

		    if (adnorm)
		    {
			/* Constant was coerced, but not from a non-Unicode
			** type (in that case, UNORM is generated inside
			** opc_adtransform). */
			uniop.opr_dt = resop->opr_dt;
						/* the coerced result type */
			if (utf8unorm)
		        {

			  /* Adjust the length for unicode normalization */
			  /* If we are going from varchar to char then
			  ** adjust the length to avoid extra blanks.
			  */
		 	  if  ((uniop.opr_len > DB_CNTSIZE) && 
				( ((abs(resop->opr_dt) == DB_CHA_TYPE) || 
				(abs(resop->opr_dt) == DB_CHR_TYPE))
			       && ((abs(ops[0].opr_dt) == DB_VCH_TYPE) || 
				   (abs(ops[0].opr_dt) == DB_TXT_TYPE)) 
			        ))
			    uniop.opr_len -= DB_CNTSIZE;
			  else if ( ((abs(resop->opr_dt) == DB_VCH_TYPE) || 
				     (abs(resop->opr_dt) == DB_TXT_TYPE))
			       && ((abs(ops[0].opr_dt) == DB_CHA_TYPE) || 
				   (abs(ops[0].opr_dt) == DB_CHR_TYPE)) 
				  )
			    uniop.opr_len += DB_CNTSIZE;
		        }
			STRUCT_ASSIGN_MACRO(*resop, ops[1]);
			STRUCT_ASSIGN_MACRO(uniop, ops[0]);
			opc_adinstr(global, cadf, ADE_UNORM, segment, 2, ops, 1);
			STRUCT_ASSIGN_MACRO(uniop, *resop);
		    }
		}
		else
		    opc_adtransform(global, cadf, &ops[0], resop, oldseg,
				(bool)FALSE, (bool)FALSE);
	    }
	}
	global->ops_cstate.opc_qp->qp_status |= QEQP_NEED_TRANSACTION;
	break;
     }

     case PST_SEQOP:
     {
	QEF_SEQUENCE	*seqp;
	i4		localbuf;

	/* Call opc_seqop_setup to locate existing QEF_SEQUENCE, or create new one.
	** Then fill in ops[0] with info, copy as needed & break. */
	opc_seq_setup(global, cnode, cadf, root, &seqp, &localbuf);

	ops[0].opr_offset = 0;
	ops[0].opr_base = localbuf;
	ops[0].opr_dt = root->pst_sym.pst_dataval.db_datatype;
	ops[0].opr_len = root->pst_sym.pst_dataval.db_length;
	ops[0].opr_prec = root->pst_sym.pst_dataval.db_prec;
	ops[0].opr_collID = root->pst_sym.pst_dataval.db_collID;

	/* If resop->opr_offset isn't preset, just assign ops[0]
	** to resop. Otherwise, this is a case function result and
	** must be copied to the result location. */
	if (resop->opr_offset == OPR_NO_OFFSET)
		STRUCT_ASSIGN_MACRO(ops[0], *resop);
	else
	{
	    /* Emit MECOPY to copy VAR value to result location. */
	    STRUCT_ASSIGN_MACRO(ops[0], ops[1]);
	    STRUCT_ASSIGN_MACRO(*resop, ops[0]);
	    opc_adinstr(global, cadf, ADE_MECOPY, segment, 2, ops, 1);
	}
	global->ops_cstate.opc_qp->qp_status |= QEQP_NEED_TRANSACTION;
	break;
     }

     case PST_RESDOM:
	/* Like AND nodes, RESDOM nodes form a list, rather than a
	** binary tree. Because a fair number of variables are required
	** to evaluate resdoms, it has been placed in a routine of its
	** own to lighten the load (reduce the amount of memory allocated
	** for local vars) on the other recursive calls to opc_cqual1().
	*/
	opc_cresdoms(global, cnode, root, cadf, segment, resop);
	break;

     case PST_VAR:
     {
	/* attno, zatt, eqcno and func_tree are for compiling var nodes. */
	i2		    attno;
	OPZ_ATTS	    *zatt;
	OPE_IEQCLS	    eqcno;
	PST_QNODE	    *func_tree;

	/* First check if this is PST_VAR representing pre-evaluated
	** subexpression. */
	if (root->pst_sym.pst_value.pst_s_var.pst_vno == OPV_NOVAR)
	{
	    /* Special case - look up operand offset in cnode offset
	    ** array, then build operand descriptor directly. */
	    ops[0].opr_dt = root->pst_sym.pst_dataval.db_datatype;
	    ops[0].opr_len = root->pst_sym.pst_dataval.db_length;
	    ops[0].opr_prec = root->pst_sym.pst_dataval.db_prec;
	    ops[0].opr_collID = root->pst_sym.pst_dataval.db_collID;
	    ops[0].opr_offset = cnode->opc_subex_offset[root->
			pst_sym.pst_value.pst_s_var.pst_atno.db_att_id];
	    ops[0].opr_base = cadf->opc_row_base[OPC_CTEMP];

	    /* Check for need to coerce. */
	    if (!((resop->opr_dt == ops[0].opr_dt || 
		     resop->opr_dt == -ops[0].opr_dt) &&
        	(resop->opr_len == OPC_UNKNOWN_LEN ||
	             (resop->opr_len == ops[0].opr_len &&
		      (resop->opr_prec == ops[0].opr_prec ||
		       abs(ops[0].opr_dt) != DB_DEC_TYPE) &&
		      resop->opr_dt == ops[0].opr_dt))))
		opc_adtransform(global, cadf, &ops[0], resop,
			segment, (bool)(resop->opr_offset == OPR_NO_OFFSET),
			FALSE);

	    if (resop->opr_offset == OPR_NO_OFFSET)
			STRUCT_ASSIGN_MACRO(ops[0], *resop);
	    break;
	}

	/* Next, find the EQ class that this var node belongs to */
	attno = root->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id;
	zatt = subqry->ops_attrs.opz_base->opz_attnums[attno];
	eqcno = zatt->opz_equcls;

	/* Now find the attribute that we have for this EQ class. Because
	** of the way that OPF and EQ classes work, the attribute that
	** this var node refers to and the att that we have may not be
	** the same. They will be equal however. Because of this,
	** we do this dereferencing step through the EQ class and the
	** available attributes for that eq class to find the data.
	*/
	if (cnode->opc_ceq[eqcno].opc_eqavailable == FALSE)
	{
	    opx_error(E_OP0889_EQ_NOT_HERE);
	}
	else
	{
	    attno = cnode->opc_ceq[eqcno].opc_attno;
	}

	if (attno != OPC_NOATT)
	{
	    zatt = subqry->ops_attrs.opz_base->opz_attnums[attno];

	    /* if this is a function attribute that hasn't been evaluated yet */
	    if (zatt->opz_attnm.db_att_id == OPZ_FANUM &&
			cnode->opc_ceq[zatt->opz_equcls].opc_attavailable == FALSE
		)
	    {
		/* then call opc_cqual1 recursively to evaluate this var node */
		func_tree = subqry->ops_funcs.opz_fbase->
			    opz_fatts[zatt->opz_func_att]->opz_function->pst_right;
		(VOID) opc_cqual1(global, cnode, func_tree, cadf, segment,
			    resop, (ADI_FI_ID)0, FALSE);
	    }
	    else
	    {
		/* otherwise, check that we have the attribute in question at
		** hand, and do the setup to return the att info.
		*/
		if (cnode->opc_ceq[zatt->opz_equcls].opc_attavailable == FALSE)
		{
		    opx_error(E_OP0888_ATT_NOT_HERE);
		}

		/* Fill in ops[0] for the attribute */
		ops[0].opr_dt = cnode->opc_ceq[eqcno].opc_eqcdv.db_datatype;
		ops[0].opr_prec = cnode->opc_ceq[eqcno].opc_eqcdv.db_prec;
		ops[0].opr_len = cnode->opc_ceq[eqcno].opc_eqcdv.db_length;
		ops[0].opr_collID = cnode->opc_ceq[eqcno].opc_eqcdv.db_collID;
		ops[0].opr_offset = cnode->opc_ceq[eqcno].opc_dshoffset;
		ops[0].opr_base =
			    cadf->opc_row_base[cnode->opc_ceq[eqcno].opc_dshrow];
		if (ops[0].opr_base < ADE_ZBASE ||
			ops[0].opr_base >= ADE_ZBASE + cadf->opc_qeadf->qen_sz_base
		    )
		{
		    /* if the CX base that we need hasn't already been added
		    ** to this CX then add it now.
		    */
		    opc_adbase(global, cadf, QEN_ROW,
					cnode->opc_ceq[eqcno].opc_dshrow);
		    ops[0].opr_base =
			    cadf->opc_row_base[cnode->opc_ceq[eqcno].opc_dshrow];
		}

            	/* added the check for len for bug 42363 (related to eq
	        ** classes). If the actual and intended result datatypes
	        ** are the same, we decide if we need to coerce
		** depending upon the following 6 cases:
           	** 1. When both the datatypes are not equal -- coercion.
            	** 2  When both the datatypes are equal and we are
		**    dealing with comparison operator -- no coercion.
            	** 3. When the datatypes are equal, their lengths are
            	**    equal, both are nullable or both are non-nullable,
		**    dealing with non-comparison operator -- no coercion
            	** 4. When the datatypes are equal and the lengths
            	**    are not, both are either nullable or 
		**    non-nullable, we are dealing with a non-comparison
            	**    operator -- coercion. Fix for bug 42363.
		** 5. When the datatypes are equal and the lengths
            	**    are not, one of them is nullable and the other is
            	**    non-nullable, but we are dealing with a 
	     	**    non-comparison operator -- coercion. Fix for bug 
		**    45876/46196.
                ** 6. When the datatypes are equal and the lengths are 
		**    equal, but one of them is nullable and the other
		**    is not, dealing with non-comparison operator 
		**    -- coercion.
            	*/
		if ((resop->opr_dt == ops[0].opr_dt || 
		     resop->opr_dt == -ops[0].opr_dt
		    ) &&
        	    (resop->opr_len == OPC_UNKNOWN_LEN ||
	             (resop->opr_len == ops[0].opr_len &&
		      (resop->opr_prec == ops[0].opr_prec ||
		       abs(ops[0].opr_dt) != DB_DEC_TYPE
		      ) &&
		      resop->opr_dt == ops[0].opr_dt
		     )
		    )
		   )
		{
		    /* If resop->opr_offset isn't preset, just assign ops[0]
		    ** to resop. Otherwise, this is a case function result and
		    ** must be copied to the result location. */
		    if (resop->opr_offset == OPR_NO_OFFSET)
			STRUCT_ASSIGN_MACRO(ops[0], *resop);
		    else
		    {
			/* Emit MECOPY to copy VAR value to result location. */
			STRUCT_ASSIGN_MACRO(ops[0], ops[1]);
			STRUCT_ASSIGN_MACRO(*resop, ops[0]);
			opc_adinstr(global, cadf, ADE_MECOPY, segment, 2, ops, 1);
		    }
		}
		else
		{
		    /* otherwise coerce the attribute to the requested type.
		    ** This covers cases 1, 4, 5 & 6. Note, we only set result 
		    ** operand attributes if this isn't a case function result.
		    */
		    opc_adtransform(global, cadf, &ops[0], resop,
			segment, (bool)(resop->opr_offset == OPR_NO_OFFSET),
			(bool)FALSE);
		}

		/* If this attribute is a tid, then set tidatt. Note that we don't
		** care if the var node refered to a tid att, or of the eq class
		** for the var node contains a tid att. We only care if the ACTUAL
		** data being returned is a tid att. This is needed by QEF because
		** DMF deals with tids in a non-orthogonal way.
		*/
		if (subqry->ops_attrs.opz_base->opz_attnums[attno]->opz_attnm.db_att_id
									== DB_IMTID
		    )
		{
		    *(cadf->opc_tidatt) = TRUE;
		}
	    }
	}
	else
	{
	    /* otherwise, check that we have the attribute in question at
	    ** hand, and do the setup to return the att info.
	    */
	    if (cnode->opc_ceq[eqcno].opc_attavailable == FALSE)
	    {
		opx_error(E_OP0888_ATT_NOT_HERE);
	    }

	    /* Fill in ops[0] for the attribute */
	    ops[0].opr_dt = cnode->opc_ceq[eqcno].opc_eqcdv.db_datatype;
	    ops[0].opr_prec = cnode->opc_ceq[eqcno].opc_eqcdv.db_prec;
	    ops[0].opr_len = cnode->opc_ceq[eqcno].opc_eqcdv.db_length;
	    ops[0].opr_collID = cnode->opc_ceq[eqcno].opc_eqcdv.db_collID;
	    ops[0].opr_offset = cnode->opc_ceq[eqcno].opc_dshoffset;
	    ops[0].opr_base = 
			cadf->opc_row_base[cnode->opc_ceq[eqcno].opc_dshrow];
	    if (ops[0].opr_base < ADE_ZBASE || 
		    ops[0].opr_base >= ADE_ZBASE + cadf->opc_qeadf->qen_sz_base
		)
	    {
		/* if the CX base that we need hasn't already been added
		** to this CX then add it now.
		*/
		opc_adbase(global, cadf, QEN_ROW,
				    cnode->opc_ceq[eqcno].opc_dshrow);
		ops[0].opr_base = 
			cadf->opc_row_base[cnode->opc_ceq[eqcno].opc_dshrow];
	    }

	    /* if the actual and intended result datatypes are the same,
	    ** ignoring nullability (which is why we have the second comparison
	    ** with the negation), then no coercion is needed.
	    */
	    if ((resop->opr_dt == ops[0].opr_dt || 
		 resop->opr_dt == -ops[0].opr_dt
		) &&
		(resop->opr_len == OPC_UNKNOWN_LEN ||
		 (resop->opr_len == ops[0].opr_len &&
		  (resop->opr_prec == ops[0].opr_prec ||
		   abs(ops[0].opr_dt) != DB_DEC_TYPE
		  ) &&
		  resop->opr_dt == ops[0].opr_dt
		 )
		)
	       )
	    {
		/* If resop->opr_offset isn't preset, just assign ops[0]
		** to resop. Otherwise, this is a case function result and
		** must be copied to the result location. */
		if (resop->opr_offset == OPR_NO_OFFSET)
		    STRUCT_ASSIGN_MACRO(ops[0], *resop);
		else
		{
		    /* Emit MECOPY to copy VAR value to result location. */
		    STRUCT_ASSIGN_MACRO(ops[0], ops[1]);
		    STRUCT_ASSIGN_MACRO(*resop, ops[0]);
		    opc_adinstr(global, cadf, ADE_MECOPY, segment, 2, ops, 1);
		}
	    }
	    else
	    {
		/* otherwise coerce the attribute to the requested type.
		** Note we only set result attributes if this is NOT
		** a case function result.  */
		opc_adtransform(global, cadf, &ops[0], resop,
		    segment, (bool)(resop->opr_offset == OPR_NO_OFFSET),
			(bool)FALSE);
	    }

	    /* This attribute is not a TID */
	    *(cadf->opc_tidatt) = FALSE;
	}
	global->ops_cstate.opc_qp->qp_status |= QEQP_NEED_TRANSACTION;
	break;
     }

     case PST_TREE:
     case PST_QLEND:
	/* No work is needed for TREE, QLEND nodes */
	break;

     default:
	opx_error(E_OP0681_UNKNOWN_NODE);
    }

    return(ret);
}

/*{
** Name: OPC_CESCAPE - compile escape indicators into expression.
**
** Description:
**	This routine simply compiles the escape character into an expression. 
**	This code used to be integral in the PST_UOP/COP/BOP case of opc_cqual1,
**	but was moved here to reduce the stack frame size of opc_cqual1 (which 
**	can produce large sequences of recursive calls).
**
** Inputs:
**	global -
**	    This is the query wide state variable for OPF/OPC.
**	root -
**	    This top resdom to be compiled.
**	cadf -
**	    The OPC version of the adf struct to compile the resdoms into.
**	segment -
**	    Which segment to compile the resdoms into (main, init, finit,
**	    virgin, etc).
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
**      15-jul-98 (inkdo01)
**          created
**	    Move pst_isescape into new u_i2 pst_pat_flags.
**	    Widened pst_escape.
*/

static VOID
opc_cescape(
	OPS_STATE   *global,
	OPC_ADF	    *cadf,
	PST_QNODE   *root,
	i4	    segment)

{
	/* escape_ops and escape_dv are for compiling escape instrctions. */
    ADE_OPERAND	    escape_ops[2];  
    DB_DATA_VALUE   escape_dv;
    u_i4	    pat_flags = root->pst_sym.pst_value.pst_s_op.pst_pat_flags;
    i4		    nops = 1;

    escape_dv.db_prec = 0;
    escape_dv.db_collID = -1;
    escape_dv.db_data = (char*)&root->pst_sym.pst_value.pst_s_op.pst_escape;
    if ((pat_flags & AD_PAT_ISESCAPE_MASK) == AD_PAT_HAS_UESCAPE)
    {
	escape_dv.db_datatype = DB_NCHR_TYPE;
	escape_dv.db_length = sizeof(UCS2);
    }
    else
    {
	escape_dv.db_datatype = DB_CHA_TYPE;
	escape_dv.db_length = CMbytecnt((char*)escape_dv.db_data);
    }
    opc_adconst(global, cadf, &escape_dv, &escape_ops[0], DB_SQL, segment);
    if (pat_flags & ~AD_PAT_ISESCAPE_MASK)
    {
	DB_DATA_VALUE sf_dv;
	sf_dv.db_prec = 0;
	sf_dv.db_collID = -1;
	sf_dv.db_data = (char*)&root->pst_sym.pst_value.pst_s_op.pst_pat_flags;
	sf_dv.db_datatype = DB_INT_TYPE;
	sf_dv.db_length = sizeof(u_i2);
	opc_adconst(global, cadf, &sf_dv, &escape_ops[1], DB_SQL, segment);
	nops++;
    }
    opc_adinstr(global, cadf, ADE_ESC_CHAR, segment, nops, escape_ops, 0);
}

/*{
** Name: OPC_CRESDOMS	- Compile resdoms into a CX.
**
** Description:
**      This routine takes a list of resdoms and compiles them into a
**      given CX. Unlike other query tree nodes, resdoms form a list
**	rather than a tree. Because of this, we evaluate the resdoms
**	in a loop rather than recursively, for reasons of speed.
**
**	There are two main actions that need to take place for the proper
**	evaluation of resdoms. The first is that the expression that lives
**	off of the right of each resdom. Next, the result of the expression
**	should be placed in the output buffer.
**
**	There are several important considerations when doing these two
**	steps. First, the correct output buffer to be used is determined
**	by the statement that is being executed and the value of the
**	pst_ttargtype field that is in the resdom. Typically, the output
**	buffer might be the output buffer that SCF provides to QEF 
**	(ie. QEN_OUTPUT) for select/retrieves, or it might be a DSH row
**	to hold the new row for appends/replaces. Where the resdom value
**	the result of the resdom expresssion) is to be placed in the
**	output buffer is determined by the pst_ntargno and pst_ttargtype
**	fields in the resdom. Please refer to opc_rdmove() for more info.
**
**	The next important consideration is the evaluation of resdoms
**	expressions. All of the resdom expressions must all be evaluated 
**	before any of the resulting values can be moved into the output
**	buffer. This is because some replaces update their values "in place".
**	In other words, they replace the new values into the same row
**	buffer that was used to read the old row into. This is done for
**	reasons of speed (again). If we were to move a resdom value into
**	an "in place" output buffer before all of the resdom expressions
**	were evaluated, then we might find ourselves evaluating the rest
**	(or some of the rest) of the resdoms expressions using *new* values
**	rather than *old* values (this is considered bad).
**	
**
** Inputs:
**	global -
**	    This is the query wide state variable for OPF/OPC.
**	cnode -
**	    Information about the current CO/QEN node, in this case
**	    the top CO/QEN node for this action/subquery.
**	root -
**	    This top resdom to be compiled.
**	cadf -
**	    The OPC version of the adf struct to compile the resdoms into.
**	segment -
**	    Which segment to compile the resdoms into (main, init, finit,
**	    virgin, etc).
**	resop -
**	    a pointer to an uninitialized operand.
**	tidatt (cadf->opc_tidatt) -
**	    a pointer to an uninitialized nat.
**
**
** Outputs:
**	cadf -
**	    The CX that is within 'cadf' now holds the instructions for
**	    the resdoms.
**	resop -
**	    now holds the base, offset, datatype, and length for the
**	    top (root) resdom.
**	tidatt (cadf->opc_tidatt) -
**	    True if and tids were referenced, false otherwise.
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
**      19-jan-89 (eric)
**          created
**      17-feb-89 (eric)
**          fixed a problem when indexing into the base array. QEF's base
**          array numbers are relative to ADE_ZBASE. Ie. the ADE_ZBASE base
**          in ADF is 0 in QEF bases.
**	21-jun-1989 (nancy)
**	    Fixed bug 6248 -- garbage returned to fe in view when view had
**	    aggregate in view def.  Start at 1st printing resdom for cx.
**	16-jan-91 (stec)
**	    Added missing opr_prec initialization.
**      11-dec-1991 (fred)
**	    Added support for large objects.  When dealing with large objects,
**	    opc_rdmove will use ADE_REDEEM instead of ADE_MECOPY when
**	    called from top level of query.  In this case, the size is unknown,
**	    so opc_rdmove will set the result length to ADE_LEN_UNKNOWN.  In
**	    this case, the offset should be reset to zero.  ADE will handle the
**	    conversion.
**      18-mar-92 (anitap)
**          Added opr_prec and opr_len initialization as part of the fix
**          for bug 42363. See opc_cqual() for details.
**	26-mar-97 (inkdo01)
**	    Added truelab, falselab, noncnf parms to support recursive calls 
**	    to modified opc_cqual1. Part of non-CNF boolexpr support.
**	15-jul-98 (inkdo01)
**	    Dropped truelab, falselab, noncnf parms as part of changes to 
**	    reduce opc_cqual1 stack frame size.
**	7-aug-02 (inkdo01)
**	    Added code to compute result length acc. to resdom size for 
**	    expressions that end up as Unicode.
**	4-sept-02 (gupsh01)
**	    Fixed for DB_LNVCHR_TYPE case. This code may not fix the length
**	    calculation for the resultant NVCHR types after normalization
**	    correctly.  
**	02-Oct-02 (gupsh01)
**	    The length of the unicode types as calculated for the normalization
**	    output, should be limited to DB_MAXSTRING, as that is the maximum 
**	    size of the length of string supported by the session.
**	03-Oct-2002 (gupsh01)
**	    Fix previous change add brackets missing after if statement.  
**	03-Oct-2002 (gupsh01)
**          Fixed length calculation for normalization result data for nullable
**          unicode types nchar and nvarchar.
**	25-oct-02 (inkdo01)
**	    Yet another adjustment for UNORM. NVCHR being assigned to NCHR
**	    needs to have length added to result description.
**	02-dec-2003 (hanal04)
**	    Correct the *4 buffer length kludge based on findings in 
**          opc_qual1()
**	25-apr-2005 (gupsh01)
**	    Fix problems with length calculation for the max result length.
**	    (Bug 114391)
**	30-jun-2005 (gupsh01)
**	    Normalization length calculations for the case when source is a 
**	    unicode type and result is a non unicode type.
**	30-june-05 (inkdo01)
**	    Changes to support PST_SUBEX_RSLT for CX optimization.
**	6-mar-06 (dougi)
**	    Fine tune length computations for NCHAR/NVARCHAR.
**	10-oct-2006 (dougi)
**	    Further fine-tune Unicode lengths to assure data length divisible
**	    by 2 (code point size).
**	22-jan-2007 (gupsh01)
**	    Fix the length calculation for the nullable types.
**	30-apr-2007 (dougi)
**	    Add support for UTF8-enabled server.
**	08-aug-2007 (toumi01)
**	    Fix AD_UTF8_ENABLED flag test && vs. & typo.
**	17-aug-2007 (gupsh01)
**	    For UTF8 enabled server fix the missing result lenght calculation.
**	31-aug-2007 (dougi)
**	    Remove special UTF8 code from this function - it isn't relevant.
**	29-feb-2008 (dougi, gupsh01)
**	    Put back the above UTF8 code.
**	28-mar-2008 (gupsh01)
**	    Always adjust for nullability for UTF8 result lengths(bug 120171).
**	03-apr-2008 (gupsh01)
**	    Increasing the size of the logical key may allow incorrect key
**	    length for a varchar datatype.
**	15-apr-2008 (gupsh01)
**	    Make sure that we do not reduce the maximum values allowed in the
**	    char/varchar column in the UTF8 installs. Bug 120215.
**	29-jul-2008 (gupsh01)
**	    Use defines OPC_UNORM_EXP_FACTOR instead of hard coded lengths.
**	27-oct-2009 (gupsh01)
**	    Fixed the length calculation for nvarchar result when the result length
**	    exceeds DB_MAXSTRING.
*/
static VOID
opc_cresdoms(
	OPS_STATE   *global,
	OPC_NODE    *cnode,
	PST_QNODE   *root,
	OPC_ADF	    *cadf,
	i4	    segment,
	ADE_OPERAND *resop)
{
    OPS_SUBQUERY    *subqry = global->ops_cstate.opc_subqry;
    ADE_OPERAND	    ops[2];	/* operands for compiling the resdoms. */
    i4		    align;	/* used to keep expressions aligned */
    i4		    nresdoms;	/* the number of resdoms under root */
    i4		    resdomno;	/* the current resdom number be used */
    PST_QNODE	    *resdom;	/* the current resdom being compiled */
    i4		    qenbaseno;	/* offset into the qen_base array for a CX */

	/* OPC_RDINFO is an array of information about the resdoms that
	** are to be compiled. We use this array to help us evaluate
	** all of the resdom expressions before we move the resdom values
	** into the output buffer
	*/
    struct OPC_RDINFO
    {
	    /* Opc_srcop holds the info about the evaluated resdom
	    ** expression.
	    */
	ADE_OPERAND	    opc_srcop;

	    /* Opc_resdom points to the resdom that is to be/has been
	    ** evaluated.
	    */
	PST_QNODE	    *opc_resdom;
    } *rdinfo, *rdi;

    /* first, count up the number of resdoms, so we know how big to make
    ** the rdinfo array
    */
    nresdoms = 0;
    for (resdom = root; 
	    resdom != NULL && resdom->pst_sym.pst_type == PST_RESDOM;
	    resdom = resdom->pst_left
	)
    {
	if (!(resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsflags&PST_RS_PRINT))
	{
	    /* if this is a non-printing resdom, then let's skip it. */
	    continue;
	}

	nresdoms += 1;
    }	    

    /* allocate rdinfo for nresdoms number of array elements; */
    rdinfo = (struct OPC_RDINFO *) 
		    opu_Gsmemory_get(global, nresdoms * sizeof (*rdinfo));

    /* initialize the resdom info list, this includes evaluating
    ** the expression off of each of the resdoms.
    */
    resdom = root;
    while (resdom != NULL && resdom->pst_sym.pst_type == PST_RESDOM &&
		    !(resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsflags&PST_RS_PRINT)
	  )
    {
	    /* bug 6248 -- start on first printing resdom (nancy) */
	    resdom = resdom->pst_left;
    }
    for (resdomno = nresdoms - 1; resdomno >= 0; resdomno -= 1)
    {
	/* find the current element in the OPC_RDINFO array and fill it with
	** the current resdom
	*/
	rdi = &rdinfo[resdomno];
	rdi->opc_resdom = resdom;

	/* Now let's compile the current resdom and place the expression
	** information into the opc_srcop field. Added the initialization of
	** opr_prec and opr_len as a fix for bug 42363. 
	*/
	rdi->opc_srcop.opr_dt = 
			resdom->pst_right->pst_sym.pst_dataval.db_datatype;
	rdi->opc_srcop.opr_len = OPC_UNKNOWN_LEN;
        rdi->opc_srcop.opr_prec = OPC_UNKNOWN_PREC;
        rdi->opc_srcop.opr_collID = -1;
	rdi->opc_srcop.opr_offset = OPR_NO_OFFSET;	/* show no result loc. */
	if (abs(rdi->opc_srcop.opr_dt) == DB_NCHR_TYPE ||
	    abs(rdi->opc_srcop.opr_dt) == DB_NVCHR_TYPE)
	{
	    DB_DT_ID	restype = resdom->pst_sym.pst_dataval.db_datatype;

	    /* Assign result length from resdom (should match column that the
	    ** data source is to be used with). If one is nullable and the other isn't,
	    ** increment/decrement as needed. All this is to make Unicode normalization
	    ** operate on a large enough buffer. */
	    rdi->opc_srcop.opr_len = resdom->pst_sym.pst_dataval.db_length;
	    if (abs(restype) == DB_NCHR_TYPE &&
		abs(rdi->opc_srcop.opr_dt) == DB_NVCHR_TYPE)
		rdi->opc_srcop.opr_len += 2;	/* add length for result NVCHR */
	    if ((abs(restype) == DB_LNVCHR_TYPE) ||
     		  (( abs(restype) != DB_NVCHR_TYPE ) &&
             		( abs(restype) != DB_NCHR_TYPE )))
	    {
		rdi->opc_srcop.opr_len = 
		  min((OPC_UNORM_EXP_FACTOR * resdom->pst_right->pst_sym.pst_dataval.db_length), 
		      DB_MAXSTRING); /* kludge */
		
		if (rdi->opc_srcop.opr_dt < 0 )
	            rdi->opc_srcop.opr_len++;
	    }
	     else switch (rdi->opc_srcop.opr_dt) {
	      case DB_NCHR_TYPE:
	      case DB_NVCHR_TYPE:
		if (abs(restype) != DB_NCHR_TYPE )
		{
		  rdi->opc_srcop.opr_len = 
		    ((rdi->opc_srcop.opr_len - DB_CNTSIZE) * OPC_UNORM_EXP_FACTOR) 
			+ DB_CNTSIZE;
		  if (rdi->opc_srcop.opr_len > DB_MAXSTRING)
		    rdi->opc_srcop.opr_len = DB_MAXSTRING + DB_CNTSIZE;   
		}
		else if ( abs(restype) != DB_NVCHR_TYPE)
		{
		  rdi->opc_srcop.opr_len = (rdi->opc_srcop.opr_len) * OPC_UNORM_EXP_FACTOR;
		  if (rdi->opc_srcop.opr_len > DB_MAXSTRING)
		    rdi->opc_srcop.opr_len = DB_MAXSTRING;   
		}

		break;
	      case -DB_NCHR_TYPE:
	      case -DB_NVCHR_TYPE:
		if (abs(restype) != DB_NCHR_TYPE)
	        {
		  rdi->opc_srcop.opr_len = 
			((rdi->opc_srcop.opr_len - 1 - DB_CNTSIZE) * OPC_UNORM_EXP_FACTOR) + 
				1 + DB_CNTSIZE;
		  if (rdi->opc_srcop.opr_len > DB_MAXSTRING)
                   rdi->opc_srcop.opr_len = DB_MAXSTRING + 1 + DB_CNTSIZE;
	        }
		else if (abs(restype) != DB_NVCHR_TYPE )
		{
		  rdi->opc_srcop.opr_len = 
			((rdi->opc_srcop.opr_len - 1) * OPC_UNORM_EXP_FACTOR) + 1;
		  if (rdi->opc_srcop.opr_len > DB_MAXSTRING)
                   rdi->opc_srcop.opr_len = DB_MAXSTRING + 1;
		}

		break;
	    }
	}
	/* Else, same stuff for UTF8 db and char/varchar. */
        else if ((global->ops_adfcb->adf_utf8_flag & AD_UTF8_ENABLED) &&
		(abs(rdi->opc_srcop.opr_dt) == DB_VCH_TYPE ||
		abs(rdi->opc_srcop.opr_dt) == DB_CHA_TYPE ||
		abs(rdi->opc_srcop.opr_dt) == DB_TXT_TYPE ||
		abs(rdi->opc_srcop.opr_dt) == DB_CHR_TYPE) &&
		abs(resdom->pst_sym.pst_dataval.db_datatype) != DB_LVCH_TYPE)
	{
	    DB_DT_ID    restype = resdom->pst_sym.pst_dataval.db_datatype;
	    i4 maxutf8str = DB_UTF8_MAXSTRING;

	    /* Assign result length from resdom (should match column that the
	    ** data source is to be used with). If one is nullable and the other
	    ** isn't, increment/decrement as needed. All this is to make Unicode
	    ** normalization operate on a large enough buffer. */

	    /* First check if the operation is between valid character types 
	    ** This could be an operation between character types and date 
	    ** datatype. Try to make the best guess about the result length. 
	    */
	    if (( abs(restype) != DB_VCH_TYPE ) &&
		 ( abs(restype) != DB_TXT_TYPE ) &&
		 ( abs(restype) != DB_CHR_TYPE ) &&
		 ( abs(restype) != DB_CHA_TYPE ))
	    {
		/* If the result type is a logical key type then
		** do not change the size of the normalized result
		*/
		if (( abs(restype) == DB_LOGKEY_TYPE ) ||
		    ( abs(restype) == DB_TABKEY_TYPE ))
	    	  rdi->opc_srcop.opr_len = 
		    min(resdom->pst_right->pst_sym.pst_dataval.db_length, 
			  DB_MAXSTRING);
		else
	    	  rdi->opc_srcop.opr_len = 
		    min(max(resdom->pst_sym.pst_dataval.db_length,
		    	     resdom->pst_right->pst_sym.pst_dataval.db_length), 
			DB_MAXSTRING);
	
	    }
	    else /* Assign results from resdom */
	    {
	        rdi->opc_srcop.opr_len = resdom->pst_sym.pst_dataval.db_length;

		if (resdom->pst_sym.pst_dataval.db_datatype < 0)
		  rdi->opc_srcop.opr_len--;
	    }

		
	    /* Adjust length for nullability (bug 120171) */
	    if (rdi->opc_srcop.opr_dt < 0 )
	    {
	      maxutf8str++;
	      rdi->opc_srcop.opr_len ++;    
	    }

	    if (abs(rdi->opc_srcop.opr_dt) == DB_VCH_TYPE ||
	         abs(rdi->opc_srcop.opr_dt) == DB_TXT_TYPE)
	    {
	      maxutf8str += 2;
	      if ((abs(restype) == DB_CHA_TYPE || 
		   abs(restype) == DB_CHR_TYPE))
	        rdi->opc_srcop.opr_len += 2;    
	    }

	    if (rdi->opc_srcop.opr_len > maxutf8str)
	      rdi->opc_srcop.opr_len = maxutf8str;
	}

	(VOID) opc_cqual1(global, cnode, resdom->pst_right, 
		cadf, segment, &rdi->opc_srcop, (ADI_FI_ID)0, FALSE);

	/* If this RESDOM is a subexpression result from CX
	** optimization, save the result offset into the subexpression
	** offset array. */
	if (resdom->pst_sym.pst_value.pst_s_rsdm.pst_ttargtype ==
			PST_SUBEX_RSLT)
	    cnode->opc_subex_offset[resdom->pst_sym.pst_value.pst_s_rsdm.
			pst_ntargno] = rdi->opc_srcop.opr_offset;

	/* if the expression below this resdom is just a var node, and 
	** it is getting the vars data from the same buffer that the results
	** are going to (in practice, this can only happen to updates. Selects,
	** inserts, and deletes cannot fulfill this condition).
	** Then move the var node information (the attribute) from its
	** current location (the result buffer) to an intermediate
	** buffer. Remember this intermediate location in opc_srcop
	** for future reference.
	**
	** This is done because we allow updates to happen "in place" ie. in
	** the same buffer that the original tuple was read into. Given
	** this, we have to be careful about how we process target list
	*/
	qenbaseno = rdi->opc_srcop.opr_base - ADE_ZBASE;
	if (resdom->pst_right->pst_sym.pst_type == PST_VAR &&
		cadf->opc_qeadf->qen_base[qenbaseno].qen_index == 
				    cadf->opc_qeadf->qen_output &&
		cadf->opc_qeadf->qen_output >= 0
	    )
	{
	    if (cadf->opc_qeadf->qen_base[qenbaseno].qen_array != QEN_ROW)
	    {
		opx_error(E_OP068C_BAD_TARGET_LIST);
	    }

	    STRUCT_ASSIGN_MACRO(rdi->opc_srcop, ops[1]);

	    ops[0].opr_dt = ops[1].opr_dt;
	    ops[0].opr_prec = ops[1].opr_prec;
	    ops[0].opr_len = ops[1].opr_len;
	    ops[0].opr_collID = ops[1].opr_collID;
	    ops[0].opr_base = cadf->opc_row_base[OPC_CTEMP];
	    ops[0].opr_offset = global->ops_cstate.opc_temprow->opc_olen;

	    /* increase the temp results buffer to hold our new value. */
	    if (ops[0].opr_len > 0)
	    {
		align = ops[0].opr_len % sizeof (ALIGN_RESTRICT);
		if (align != 0)
		    align = sizeof (ALIGN_RESTRICT) - align;
		global->ops_cstate.opc_temprow->opc_olen += ops[0].opr_len + align;
	    }

	    opc_adinstr(global, cadf, ADE_MECOPY, segment, 2, ops, 1);

	    STRUCT_ASSIGN_MACRO(ops[0], rdi->opc_srcop);
	}

	/* find the next printable resdom if there is one */
	do
	{
	    resdom = resdom->pst_left;
	} while (resdom != NULL && resdom->pst_sym.pst_type == PST_RESDOM &&
		    !(resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsflags&PST_RS_PRINT)
		);
    }

    /* for each resdom, move the expression into its result location; 
    ** First, let's set the offset of the output operand to zero, since
    ** that's where the first resdom goes.
    */
    ops[0].opr_offset = 0;
    for (resdomno = 0; resdomno < nresdoms; resdomno += 1)
    {
	rdi = &rdinfo[resdomno];
	STRUCT_ASSIGN_MACRO(rdi->opc_srcop, ops[1]);

	/* Now that all of the resdoms have been evaluated, we can
	** move the current result expression to the output with out interfering
	** with the rest of the resdoms.
	*/
	if (subqry->ops_result == OPV_NOGVAR)
	{
	    opc_rdmove(global, rdi->opc_resdom, ops, 
					cadf, segment, (RDR_INFO *)NULL, FALSE);
	}
	else
	{
	    opc_rdmove(global, rdi->opc_resdom, ops, cadf, segment, 
			    global->ops_rangetab.opv_base->
				opv_grv[subqry->ops_result]->opv_relation, FALSE);
	}

	/* calculate where the next resdom should go in the output buffer */
	if ((ops[0].opr_len != ADE_LEN_UNKNOWN)
		&& (ops[0].opr_len != ADE_LEN_LONG))
	{
	    if (ops[0].opr_offset != ADE_LEN_UNKNOWN)
		ops[0].opr_offset = ops[0].opr_offset + ops[0].opr_len;
	    else
		ops[0].opr_offset = ops[0].opr_len; /* reset offset= 0 */
	}
	else
	{
	    /* 
	    ** When we don't know how long the last one was (it was a
	    ** redeem), then set the next one to be placed at an
	    ** unknown offset.  ade_execute_cx() understands how to
	    ** deal with this situation.  Also, see a few lines above
	    ** where after placing the first item at an unknown place,
	    ** we set its next's offset to just its length,
	    ** effectively restarting the offset calculations at zero.
	    */
	    
	    ops[0].opr_offset = ADE_LEN_UNKNOWN;
	}
    }

    /* return the output operand for the last (top most) resdom */
    STRUCT_ASSIGN_MACRO(ops[0], *resop);
}

/*{
** Name: OPC_TARGET	- Compile a target list into a CX.
**
** Description:
**      This routine compiles any target list that doesn't contain an  
**      aggregate. For the compilation of aggregate target lists, see 
**      opc_agtarget() in OPCAGG.C.
**        
**      Most of the heavy work of compiling target lists is done by opc_cqual() 
**      (and opc_cxest()), so this routine mostly coordinates things. The 
**      first thing to do is to estimate the size of the CX that we will 
**      be building, and then begin the compilation. 
**        
**      Next, we need to figure out where the results of the target list should 
**      go. This depends on what type of query is it. For example, select/ 
**      retrieves often goto to the user (ie. get returned to SCF from QEF). 
**	Other possibilities include selecting into a DBP local var, or repeat
**      query parameter. Inserts and updates go into relations (you could have
**      guessed that). Because selects can go to various places, the resdoms 
**      are marked with their destination. 
**        
**      Next we compile the target list. This is done in two different ways 
**      depending on whether a top sort node is on top of our CO tree for 
**      the current query. If a top sort node is present, then it already 
**      evaluated all of the target list expressions. Top sort nodes do this 
**      because they need to sort the target list, rather than sorting eqc's 
**      like other sort nodes do. Anyway, because the top sort node already 
**      evaluated the target list expressions, we just need to compile 
**      compile instructions to copy the expressions from the sort node 
**      DSH row to the output location (determined earlier in this routine). 
**      If a top sort node is *not* present at the top of the CO tree 
**      (strangely ehough, QEF has "top sort nodes" that are allowed in 
**      places other than the top of the QEN tree) then we call opc_cqual() 
**      to do the heavy work. 
**        
**      Last, but not least, we compile instructions to fill unspecified 
**      attributes in with default values for insert/append statements, and
**      we end the compilation. 
[@comment_line@]...
**
** Inputs:
**	global -
**	    The query wide state variable for OPF/OPC
**	cnode -
**	    This gives information about the current top CO/QEN node.
**	root -
**	    A pointer to the target list.
**	qadf -
**	    A pointer to an uninitialized QEN_ADF struct.
**
** Outputs:
**	qadf -
**	    This will be filled in with values that will allow QEF to call
**	    ADF to evaluate the target list expressions.
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none except for those that opx_error raises.
**
** History:
**      28-aug-86 (eric)
**          written
**	09-jan-91 (stec)
**	    Fixed bug 35209. Add QEN_OUT row only when resdoms are printable.
**      29-Jun-1993 (fred)
**          Fixed bug 50552.  Altered loop which computes resdom list.
**          It now moves from the physically lowest offset to the highest.
**          This is necessary to make sure that variable length target
**          list entities (i.e. large objects) are correctly accounted for.
**      10-nov-95 (inkdo01)
**	    Changes to support replacement of QEN_ADF instances by pointers.
**      18-mar-2004 (huazh01)
**          For query that references to Ingres sequence, compute the number
**          of the base required by counting the number of PST_SEQOP node 
**          included in the query.
**          This fixes INGSRV2731, b111903.
**	4-july-05 (inkdo01)
**	    Optimize target list projection CX.
**      09-jun-2009 (coomi01) b121805,b117256
**          Generate code for SEQOPS
[@history_template@]...
*/
VOID
opc_target(
		OPS_STATE   *global,
		OPC_NODE    *cnode,
		PST_QNODE   *root,
		QEN_ADF	    **qadf)
{
	/* standard variables to shorten line length */
    OPS_SUBQUERY    *subqry = global->ops_cstate.opc_subqry;
    QEF_AHD	    *ahd = subqry->ops_compile.opc_ahd;

	/* Ninstr, nops, nconst, szconst, and max_base for the basis of
	** our size estimate for the CX.
	*/
    i4		    ninstr, nops, nconst;
    i4	    szconst;
    i4		    max_base;

	/* Opc_cqual usually returns an operand describing the compiled
	** qtree node. In the case of resdoms however, nothing is returned
	** so we allocate a dummy operand for a place holder.
	*/
    ADE_OPERAND	    opdummy;

	/* Outsize and outrow hold the size of location of the output for
	** the target list.
	*/
    i4		    outsize;
    i4		    outrow;

	/* Dmap is the default map, which tells us which attributes need
	** default values for an insert/append.
	*/
    i1		    *dmap;

	/* like opdummy, tidatt is a placeholder parameter to opc_cqual(). */
    i4		    tidatt;

	/* cadf is OPC version of the adf struct. It holds the qef adf struct
	** and provides a way to find out what bases hold what rows.
	*/
    OPC_ADF	    cadf;

	/* When a top sort node in on top of the CO/QEN tree, 'resdom' points
	** to each resdom to allow us to copy the data correctly. 'qn' points
	** the top sort node. Rsno holds the resdom number for each resdom.
	** tsoffsets tells us where the top sort node put each target list
	** expression. Finally, targ_offset is used to calculate where to
	** put each target list expression in the output buffer. If the top
	** node of the CO/QEN tree isn't a top sort node, then none of these
	** vars will be used.
	*/
    PST_QNODE	    *resdom;
    QEN_NODE	    *qn;
    ADE_OPERAND	    ops[2];
    i4		    rsno;
    OPC_TGPART	    *tsoffsets = subqry->ops_compile.opc_sortinfo.opc_tsoffsets;
    i4		    targ_offset;

    	/*
	** When filling the target list, it is necessary that the list
	** be compiled from the lowest offset to the highest.  This is
	** necessary for the case when the query returns one or more
	** large objects.  In this case, ade_execute_cx() will
	** understand how to manage the variable sized objects
	** returned, but to do so, requires that it start at the
	** beginning of the tuple.
	**
	** Consequently, we will construct the list of resdoms to
	** build in the first pass, and then build from the last to
	** the first (which is from lowest offset to highest).
	** opc_rdmove() will handle the offset management.
	*/
    PST_QNODE       **rd_list;
    i4              rd_max;
    i4              i;
    QEF_SEQUENCE    *seqp;
    i4              localbuf;


    /* if there is nothing to compile, then return */
    if (root == NULL)
    {
	(*qadf) = (QEN_ADF *)NULL;
	return;
    }

    /* initialize an QEN_ADF struct; */
    ninstr = 0;
    nops = 0;
    nconst = 1;
    szconst = 0;
    opc_cxest(global, cnode, root, &ninstr, &nops, &nconst, &szconst);

    /* if this is an append then lets estimate the size of the default values 
    ** for any atts that won't been given by the target list.
    */
    if (subqry->ops_mode == PSQ_APPEND)
    {
	dmap = opc_estdefault(global, root, &ninstr, &nops, &nconst, &szconst);
    }

    max_base = cnode->opc_below_rows + 2;

    for (resdom = root; resdom; resdom = resdom->pst_left)
    {
        if (resdom->pst_right && resdom->pst_right->pst_sym.pst_type ==
             PST_SEQOP)
        {
             max_base = max_base + 1; 
        }
    }

    opc_adalloc_begin(global, &cadf, qadf, ninstr, nops, nconst, szconst,
	max_base, OPC_STACK_MEM);

    /* Let's figure out where the results of this query will go. */
    switch (ahd->ahd_atype)
    {
     case QEA_GET:
     case QEA_RETROW:
	{
	    /*
	    ** This decision will be based on the printing resdoms in the 
	    ** target list; ones that are non-printing are to be ignored
	    ** as far as target list processing is concerned. This fixes
	    ** bug 35209.
	    */

	    bool    print;

	    /* Find first printing resdom in the target list. */
	    for (resdom = root, print = FALSE;
		 resdom->pst_sym.pst_type != PST_TREE;
		 resdom = resdom->pst_left)
	    {
		if (resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsflags&PST_RS_PRINT)
		{
		    print = TRUE;
		    break;
		}
	    }

	    if (print == FALSE)
	    {
		/* Target list without printing resdoms is invalid. */
		opx_error(E_OP068C_BAD_TARGET_LIST);
	    }

	    switch (resdom->pst_sym.pst_value.pst_s_rsdm.pst_ttargtype)
	    {
	     case PST_ATTNO:
	     case PST_USER:
		/*
		** The case of PST_ATTNO is allowed here as a hack for OPF.
		** The understanding is that if the resdom type is PST_ATTNO
		** and we're getting data, then treat it as a PST_USER.
		*/
		opc_adbase(global, &cadf, QEN_OUT, 0);
		break;

	     case PST_LOCALVARNO:
	     case PST_RQPARAMNO:
	     case PST_RESROW_COL:
		break;

	     default:
		opx_error(E_OP0897_BAD_CONST_RESDOM);
	    }
	}
	break;

     case QEA_PUPD:
	if (cnode->opc_qennode->qen_type == QE_ORIG)
	{
	    /* The top qen node is the orig node for the result relation.
	    ** This means that we do the update on top of the row that
	    ** was just read, rather that making a copy.
	    */
	    outrow = cnode->opc_qennode->qen_row;
	    outsize = cnode->opc_qennode->qen_rsz;

	    ahd->qhd_obj.qhd_qep.ahd_reprow = -1;
	    ahd->qhd_obj.qhd_qep.ahd_repsize = -1;
	}
	else
	{
	    /* We must make a copy of the result row */
	    outsize = global->ops_rangetab.opv_base->
		opv_grv[subqry->ops_result]->opv_relation->rdr_rel->tbl_width;
	    opc_ptrow(global, &outrow, outsize);

	    /*
	    ** Set the ahd_reprow to the row number where QEF can find the
	    ** row to be replace in it original (pre-replaced) form.
	    */
	    ahd->qhd_obj.qhd_qep.ahd_reprow = ahd->qhd_obj.qhd_qep.ahd_tidrow;
	    ahd->qhd_obj.qhd_qep.ahd_repsize = outsize;
	}
	(*qadf)->qen_output = outrow;
	opc_adbase(global, &cadf, QEN_ROW, outrow);
	break;

     default:
	outsize = global->ops_rangetab.opv_base->
	    opv_grv[subqry->ops_result]->opv_relation->rdr_rel->tbl_width;
        opc_ptrow(global, &outrow, outsize);
	(*qadf)->qen_output = outrow;
	opc_adbase(global, &cadf, QEN_ROW, outrow);
	break;
    }

    /* Finally, let's compile the target list */
    if (cnode != NULL &&
	cnode->opc_qennode != NULL && 
	cnode->opc_qennode->qen_type == QE_TSORT)
    {
	/*
	** There is a top sort node for this qen tree which has already
	** computed the target list. All we have to do now is to copy
	** the target list from the sorter to the output row.
	*/
	qn = cnode->opc_qennode;
	opc_adbase(global, &cadf, QEN_ROW, qn->qen_row);
	
	/*
	** Figure out the size of the target row 
	** that is returned to the user.
	*/
	rd_max = 0;
	targ_offset = 0;

	rd_list= (PST_QNODE **) opu_Gsmemory_get(global,
						 sizeof(*rd_list)
						     * DB_MAX_COLS);

	for (resdom = root; 
	     resdom != NULL && resdom->pst_sym.pst_type != PST_TREE;
	     resdom = resdom->pst_left)
	{
	    if (resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsflags&PST_RS_PRINT)
	    {
		rd_list[rd_max] = resdom;
		rd_max += 1;
	    }
	}
	for (rd_max -= 1;
	     rd_max >= 0;
	     rd_max -= 1)
	{

	    resdom = rd_list[rd_max];
	    rsno = resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsno;

	    /* Bug 121805
	    ** Check for PST_SEQOPs - they were not materialized with the
	    ** sort (duplicates elimination might waste sequence values) and
	    ** must be generated here. 
	    */
	    if (resdom->pst_right->pst_sym.pst_type == PST_SEQOP)
	    {
		/* Call opc_seqop_setup to locate existing QEF_SEQUENCE, 
		** or create new one. Then fill in ops[0] with info, copy 
		** as needed & break.
		*/
		opc_seq_setup(global, cnode, &cadf, resdom->pst_right, 
							&seqp, &localbuf);

		ops[1].opr_offset = 0;
		ops[1].opr_base = localbuf;
		ops[1].opr_dt = resdom->pst_right->pst_sym.
						pst_dataval.db_datatype;
		ops[1].opr_len = resdom->pst_right->pst_sym.
						pst_dataval.db_length;
		ops[1].opr_prec = resdom->pst_right->pst_sym.
						pst_dataval.db_prec;
		ops[1].opr_collID = resdom->pst_right->pst_sym.
						pst_dataval.db_collID;


		ops[0].opr_base = cadf.opc_out_base;
		ops[0].opr_offset = targ_offset;
		ops[0].opr_dt = resdom->pst_sym.pst_dataval.db_datatype;
		ops[0].opr_len = resdom->pst_sym.pst_dataval.db_length;
		ops[0].opr_prec = resdom->pst_sym.pst_dataval.db_prec;
		ops[0].opr_collID = resdom->pst_sym.pst_dataval.db_collID;

		if (subqry->ops_result == OPV_NOGVAR)
		{
		    opc_rdmove(global, resdom, ops, &cadf, ADE_SMAIN, 
			(RDR_INFO *)NULL, FALSE);
		}
		else
		{
		    opc_rdmove(global, resdom, ops, &cadf, ADE_SMAIN,
			global->ops_rangetab.opv_base->
			opv_grv[subqry->ops_result]->opv_relation, FALSE);
		}
		targ_offset = ops[0].opr_offset + ops[0].opr_len;
		
		continue;		/* skip the rest */
	    }

	    ops[0].opr_dt   = ops[1].opr_dt   = tsoffsets[rsno].opc_tdatatype;
	    ops[0].opr_len  = ops[1].opr_len  = tsoffsets[rsno].opc_tlength;
	    ops[0].opr_prec = ops[1].opr_prec = tsoffsets[rsno].opc_tprec;
	    ops[0].opr_collID = ops[1].opr_collID = tsoffsets[rsno].opc_tcollID;
	    ops[1].opr_offset = tsoffsets[rsno].opc_toffset;

	    ops[0].opr_offset = targ_offset;

	    ops[0].opr_base = cadf.opc_out_base;
	    ops[1].opr_base = cadf.opc_row_base[qn->qen_row];

	    if (subqry->ops_result == OPV_NOGVAR)
	    {
		opc_rdmove(global, resdom, ops, &cadf, ADE_SMAIN, 
		    (RDR_INFO *)NULL, FALSE);
	    }
	    else
	    {
		opc_rdmove(global, resdom, ops, &cadf, ADE_SMAIN,
		    global->ops_rangetab.opv_base->
			opv_grv[subqry->ops_result]->opv_relation, FALSE);
	    }

	    /* calculate where the next resdom should go in the output buffer */
	    if ((ops[0].opr_len != ADE_LEN_UNKNOWN)
		&& (ops[0].opr_len != ADE_LEN_LONG))
	    {
		if (ops[0].opr_offset != ADE_LEN_UNKNOWN)
		    targ_offset = ops[0].opr_offset + ops[0].opr_len;
		else
		    targ_offset = ops[0].opr_len;
	    }
	    else
	    {

		/* 
		** When we don't know how long the last one was (it was a
		** redeem), then set the next one to be placed at an
		** unknown offset.  ade_execute_cx() understands how to
		** deal with this situation.  Also, see a few lines above
		** where after placing the first item at an unknown place,
		** we set its next's offset to just its length,
		** effectively restarting the offset calculations at zero.
		*/
		
		targ_offset = ADE_LEN_UNKNOWN;
	    }
	}
    }
    else
    {
	PST_QNODE	*subexlistp = NULL;
	ADE_OPERAND	resop;

	/* Locate common subexpressions in target list for preevaluation
	** and replace in original expressions by PST_VARs. */
	opc_expropt(subqry, cnode, &root, &subexlistp);

	/* Next compile evaluation of common subexpressions. */
	if (subexlistp != (PST_QNODE *) NULL)
	{
	    resop.opr_len = OPC_UNKNOWN_LEN;
	    resop.opr_prec = OPC_UNKNOWN_PREC;
	    opc_cqual(global, cnode, subexlistp, &cadf, ADE_SMAIN, 
						&resop, &tidatt);
	}

	/* Finally compile modified target list. */
	(VOID) opc_cqual(global, cnode, root, &cadf, ADE_SMAIN, 
	    &opdummy, &tidatt);
    }

    /*
    ** If this is an append then lets add the default values 
    ** for any atts that haven't been given by the target list.
    */
    if (subqry->ops_mode == PSQ_APPEND)
    {
	opc_cdefault(global, dmap, 
	    cadf.opc_row_base[cadf.opc_qeadf->qen_output], &cadf);
    }

    opc_adend(global, &cadf);
}

/*{
** Name: OPC_ESTDEFAULT	- Estimate the size of default values for an append.
**
** Description:
**      Similiar to opc_cxest(), this routine estimates the size of a CX 
**      that will be required to hold instructions to fill in unspecified 
**	attributes with default values. This routine also returns a bit map
**      that helps the caller figure out which attributes where not specified 
**      by the user in their query. 
**      
**      The instructions for the default values are compiled by opc_cdefault(). 
[@comment_line@]...
**
** Inputs:
**	global -
**	    The query wide state variable for OPF/OPC
**	root -
**	    The root of the target list (not the root node).
**	ninstr -
**	nops -
**	nconst -
**	szconst -
**	    These should be initialized to any value that the caller wishes.
**
** Outputs:
**	ninstr -
**	nops -
**	nconst -
**	szconst -
**	    These will be incremented by the number of instructions, operands,
**	    constants, and the size of the constants that will be required
**	    to compile in the default values.
**
**	Returns:
**	    a bit map of which attributes need default values.
**	Exceptions:
**	    none except for those that opx_error raises.
**
** Side Effects:
**	    none
**
** History:
**      10-oct-86 (eric)
**          written
**	09-jun-90 (eric)
**	    Fixed bug where we were clearing bits on all target list attributes
**	    indicating that they didn't need to be defaulted; changed so that
**	    non-printing resdoms didn't get their bits cleared.  Fixes a paribas
**	    bug.
**      05-jul-95 (inkdo01)
**          Added null termination test to RESDOM list for-loop
[@history_template@]...
*/
i1 *
opc_estdefault(
		OPS_STATE   *global,
		PST_QNODE   *root,
		i4	    *ninstr,
		i4	    *nops,
		i4	    *nconst,
		i4	    *szconst)
{
	/* variables to keep line length down */
    OPS_SUBQUERY	*sq = global->ops_cstate.opc_subqry;

	/* Rel points to the relation description of the result relation */
    RDR_INFO		*rel;

	/* dmap and dmap_size point to the default map (and it's size) that
	** we will be returning
	*/
    i1			*dmap;
    i4			dmap_size;

	/* This holds each of the attribute numbers for the result relation
	** as we determine whether it was specified by the user, and if not,
	** it's CX size requirements.
	*/
    i4			attno;

    rel = global->ops_rangetab.opv_base->opv_grv[sq->ops_result]->opv_relation;

    /* initialize the map of atts that need default values. */
    dmap_size = ((rel->rdr_rel->tbl_attr_count + 1) / BITSPERBYTE) + 1;
    dmap = (i1 *) opu_Gsmemory_get(global, dmap_size);
    MEfill(dmap_size, (u_char)0, (PTR)dmap);
    BTnot((i4)(rel->rdr_rel->tbl_attr_count + 1), (char *)dmap);
    BTclear((i4)0, (char *)dmap);

    /* clear the bits for atts that are in the target list */
    for (; root && root->pst_sym.pst_type != PST_TREE; root = root->pst_left)
    {
	if (root->pst_sym.pst_value.pst_s_rsdm.pst_rsflags&PST_RS_PRINT)
	    BTclear((i4) root->pst_sym.pst_value.pst_s_rsdm.pst_rsno, dmap);
    }

    /* increase the estimate for each att that needs a default value. */
    for (attno = -1; 
	    (attno = BTnext((i4)attno, (char *)dmap, (i4)(rel->rdr_rel->tbl_attr_count+ 1))) != -1
		&& attno <= rel->rdr_rel->tbl_attr_count;
	)
    {
	*ninstr += 2;
	*nops += 3;
	*nconst += 1;
	if (attno == DB_IMTID)
	{
	    *szconst += DB_TID8_LENGTH;
	}
	else
	{
	    *szconst += rel->rdr_attr[attno]->att_width;
	}
    }

    return(dmap);
}

/*{
** Name: OPC_CDEFAULT	- Compile in default values in atts for appends.
**
** Description:
**      As opc_estdefault() is similiar to opc_cxest(), this routine is
**      similiar to opc_cqual(). The difference is that this doesn't compile 
**      query tree nodes, but instead compiles instructions to fill in  
**      attributes with default values. 
[@comment_line@]...
**
** Inputs:
**	global -
**	    the query wide state variable for OPF/OPC
**	dmap -
**	    The bit map of attributes that need default values.
**	outbase -
**	    the CX base array number for the output buffer
**	cadf -
**	    the OPC version of the CX, whose compilation has already been begun
**
** Outputs:
**	cadf -
**	    The CX will hold the instructions to fill in the attributes with
**	    their default values. Note that the CX compilation will not be
**	    ended by this routine.
**
**	Returns:
**	    none
**	Exceptions:
**	    none except for those that opx_error raises.
**
** Side Effects:
**	    none
**
** History:
**      10-oct-86 (eric)
**          written
**	16-jan-91 (stec)
**	    Added missing opr_prec initialization.
[@history_template@]...
*/
VOID
opc_cdefault(
		OPS_STATE   *global,
		i1	    *dmap,
		i4	    outbase,
		OPC_ADF	    *cadf)
{
	/* varaibles to keep line length down. */
    OPS_SUBQUERY	*sq = global->ops_cstate.opc_subqry;

	/* a pointer to the result relation description. */
    RDR_INFO		*rel;

	/* this will hold each of the attribute numbers that need default
	** values.
	*/
    i4			attno;

	/* this will hold the default values */
    DB_DATA_VALUE	dv;

	/* the operands for the CX instructions. */
    ADE_OPERAND		ops[2];

    rel = global->ops_rangetab.opv_base->opv_grv[sq->ops_result]->opv_relation;
    MEfill(sizeof (dv), (u_char)0, (PTR)&dv);

    /* for each att that needs a default value. */
    for (attno = -1; 
	    (attno = BTnext((i4)attno, (char *)dmap, (i4)(rel->rdr_rel->tbl_attr_count+ 1))) != -1
		&& attno <= rel->rdr_rel->tbl_attr_count;
	)
    {
	if (attno == DB_IMTID || 
		rel->rdr_attr[attno]->att_flags & DMU_F_NDEFAULT
	    )
	{
	    opx_error(E_OP0800_NODEFAULT);
	}

	ops[0].opr_dt = (DB_DT_ID) rel->rdr_attr[attno]->att_type;
	ops[0].opr_prec = rel->rdr_attr[attno]->att_prec;
	ops[0].opr_len = rel->rdr_attr[attno]->att_width;
	ops[0].opr_collID = rel->rdr_attr[attno]->att_collID;
	ops[0].opr_base = outbase;
	ops[0].opr_offset = rel->rdr_attr[attno]->att_offset;

	/* Get the default value for this att; */
	opc_adempty(global,
		    &dv,
		    ops[0].opr_dt,
		    (i2) ops[0].opr_prec,
		    (i4) ops[0].opr_len);

	/* compile in the default value as a const; */
	opc_adconst(global, cadf, &dv, &ops[1], DB_QUEL, ADE_SMAIN);

	/* compile an instruction to move the const to the result row; */
	opc_adinstr(global, cadf, ADE_MECOPY, ADE_SMAIN, 2, ops, 1);
    }
}

/*{
** Name: OPC_RDMOVE	- Set up the operation and result operand 
**							    to move a resdom.
**
** Description:
**      This routine takes the description of a resdom expression that QEF/ADF
**	will evaluate, and compiles instructions to move it (the expression)
**      into the output buffer for this query (subqry struct/action really). 
**      
**      The first thing to do is to figure out where in the output buffer 
**      this resdom expression is supposed to go. This is determined by  
**      the target type that is stored in the resdom node. If the target type 
**      is a PST_USER than we put the result into the users (SCF actually) 
**      output buffer. If it's PST_ATTNO then we put it into a DSH row 
**      to allow the row to be inserted or updated into the result relation. 
**      The cases go on from there are are described in the switch statement. 
**        
**      Once we have the destination figured out, we either compile a coercion
**      or an MEcopy to move the source expression to its destination.  If the
**	resdom is for a peripheral type, then instead of using an ADE_MECOPY
**	instruction, use an ADE_REDEEM on the coupon data structure.
[@comment_line@]...
**
** Inputs:
**	global -
**	    This is the query wide state variable for OPF/OPC
**	resdom -
**	    The resdom qtree node that we are compiling.
**	ops -
**	    this is a pointer to an array of 2 ADE_OPERANDs. The second (ie
**	    the [1]) element must be filled in to describe where the source 
**	    resdom expression will be. If the resdom target type is PST_USER
**	    then the opr_offset of the first (ie. the [0]) element must hold 
**	    the next unused offset in the output buffer, in target list order.
**	    All other fields in the first element ([0]) can be uninitialized.
*	cadf -
**	    The opened OPC version of the CX.
**	Segment -
**	    The segment (init, main, finit, etc) to place the instruction into.
**	rel -
**	    the result relation description
**
** Outputs:
**	cadf -
**	    This now holds the instructions for the resdom expression moving.
**	ops -
**	    the first ([0]) element now describes the final resting place
**	    for the resdom expression.
**
**	Returns:
**	    none
**	Exceptions:
**	    none, except those that opx_error raises.
**
** Side Effects:
**	    none
**
** History:
**      11-feb-87 (eric)
**          written
**      8-aug-88 (eric)
**          Update the qen_uoutput base for replace cursors.
**	20-feb-89 (paul)
**	    Enhanced to support the generation of CALLPROC parameter arrays
**	    from a target list tree.
**      20-Nov-89 (fred)
**          Added support for large objects.  When dealing with large objects,
**	    use ADE_REDEEM instead of ADE_MECOPY when called from top level
**	    of query.  This operation should ONLY be used when about to send
**	    data out of the server (i.e. when rel == NULL).
**	14-nov-90 (stec)
**	    Changed code to use temporary DSH row for coercion (bug 33557).
**	    See comments in code.
**	16-jan-91 (stec)
**	    Added missing opr_prec initialization and check for precision
**	    equality for decimal datatype in an if statement condition.
**	09-nov-92 (jhahn)
**	    Added handling for byrefs.
**	17-nov-1992 (fred)
**	    Updated large object changes so that the redeem operation is used
**	    when objects are being sent out of the server.  The condition
**	    mentioned above is incorrect -- the correct thing is to check the
**	    resdom target type -- PST_USER is the way to go.
**	14-dec-92 (jhahn)
**	    Did some cleanup for  handling of byrefs.
**	24-mar-93 (jhahn)
**	    Added fix for support of statement level rules. (FIPS)
**	23-oct-98 (inkdo01)
**	    Added support of PST_RESROW_COL for return row building in
**	    row producing procs.
**	22-dec-00 (inkdo01)
**	    Added "leavebase" for inline aggregate code generation.
**	9-apr-01 (inkdo01)
**	    Do ADE_UNORM instead of MECOPY for insert/update of Unicode columns.
**	25-apr-01 (inkdo01)
**	    Set "for_user" flag for "return row" result values so BLOBs are
**	    handled in row producing procedures.
**	13-mar-02 (inkdo01)
**	    Treat ATTNO coming from projection aggregate as USER (fixes bug 107320).
**      10-may-02 (inkdo01, gupsh01)
**          Added code to handle the character to unicode coercion correctly.
**	30-jul-02 (inkdo01)
**	    Refined Unicode normalization logic to match generalized normalization 
**	    for other Unicode constant/coercion references.
**	30-june-05 (inkdo01)
**	    Added support for PST_SUBEX_RSLT for CX optimization.
**	03-Nov-2006 (thaju02) (B117040)
**	    For rpp, do not set ops[0].opr_offset here. Leave it to 
**	    opc_cresdoms(); mimic what we do for sql select queries. 
**	30-apr-2007 (dougi)
**	    Add support for UTF8-enabled server.
**	7-may-2008 (dougi)
**	    Add support for QEF_CP_PARAMs in table procedure.
*/
VOID
opc_rdmove(
		OPS_STATE	*global,
		PST_QNODE	*resdom,
		ADE_OPERAND	*ops,
		OPC_ADF		*cadf,
		i4		segment,
		RDR_INFO	*rel,
		bool		leavebase)
{
	/* variable to keep line length down. */
    OPS_SUBQUERY    *subqry = global->ops_cstate.opc_subqry;
    QEF_AHD	    *ahd;
    DMT_ATT_ENTRY   *att;

	/* opno holds the operator id (MEcopy or coercion) to move the
	** resdom expression.
	*/
    ADI_FI_ID	    opno;

	/* holds return errors from adi_fecoerce(). */
    DB_STATUS	    err;

	/* Parms hold a description of the repeat query parameters for
	** this query. This is used when we move resdom expressions into
	** a repeat query parameter. Parm_no holds the destination repeat
	** query parameter number.
	*/
    OPC_BASE	    *parms;
    i4		    parm_no;

	/* Ntargno is the target list number in the resdom. */
    i4		    ntargno;
    i4		    bits; /* Holds datatype bits from adi_dtinfo() */
    i4		    for_user = FALSE; /* is this resdom for the user? */
    bool	    unorm = FALSE;
    bool	    utf8unorm = FALSE;

    if (resdom->pst_sym.pst_value.pst_s_rsdm.pst_ttargtype ==
						PST_SUBEX_RSLT)
	return;				/* no need for these guys */

    if (subqry)
	ahd = subqry->ops_compile.opc_ahd;

    /* Note that no subqry implies that the resdom cannot be PST_ATTNO. **
    ** The same goes for AGGFs with no rel. */
    if (resdom->pst_sym.pst_value.pst_s_rsdm.pst_ttargtype == PST_ATTNO &&
	    (ahd->ahd_atype == QEA_GET || (ahd->ahd_atype == QEA_AGGF &&
		rel == NULL))
	)
    {
	/* PST_ATTNO resdoms in GET actions is allowed here as a hack 
	** convience to OPF. The understanding is that if the resdom type is 
	** PST_ATTNO and we're getting data, then treat it as a PST_USER. 
	*/
	resdom->pst_sym.pst_value.pst_s_rsdm.pst_ttargtype = PST_USER;
    }

    switch (resdom->pst_sym.pst_value.pst_s_rsdm.pst_ttargtype)
    {
     case PST_USER:
	if (!leavebase)
	    ops[0].opr_base = cadf->opc_qeadf->qen_uoutput + ADE_ZBASE;
	ops[0].opr_dt = resdom->pst_sym.pst_dataval.db_datatype;
	ops[0].opr_prec = resdom->pst_sym.pst_dataval.db_prec;
	ops[0].opr_len = resdom->pst_sym.pst_dataval.db_length;
	ops[0].opr_collID = resdom->pst_sym.pst_dataval.db_collID;
	for_user = TRUE;
	/* note: if this is a retrieve, ops[0].opr_offset was 
	** filled in during the call to opc_cqual
	** to handle the resdom->pst_left resdom.
	*/
	break;

     case PST_LOCALVARNO:
	opc_lvar_row(global, resdom->pst_sym.pst_value.pst_s_rsdm.pst_ntargno,
								cadf, &ops[0]);
	break;

     case PST_RQPARAMNO:
	parm_no = resdom->pst_sym.pst_value.pst_s_rsdm.pst_ntargno;
	parms = global->ops_cstate.opc_rparms;

	ops[0].opr_dt = parms[parm_no].opc_dv.db_datatype;
	ops[0].opr_prec = parms[parm_no].opc_dv.db_prec;
	ops[0].opr_len = parms[parm_no].opc_dv.db_length;
	ops[0].opr_collID = parms[parm_no].opc_dv.db_collID;
	if (!leavebase)
	{
	    ops[0].opr_base = cadf->opc_repeat_base[parm_no];
	    ops[0].opr_offset = parms[parm_no].opc_offset;
	}

	if (!leavebase && (ops[0].opr_base < ADE_ZBASE || 
		ops[0].opr_base >= ADE_ZBASE + cadf->opc_qeadf->qen_sz_base)
	    )
	{
	    opc_adbase(global, cadf, QEN_PARM, parm_no);
	    ops[0].opr_base = cadf->opc_repeat_base[parm_no];
	}
	break;

     case PST_ATTNO:
	/* Place the results of this resdom at the offset for the
	** result attribute being replaced or appended to
	*/
	ntargno = resdom->pst_sym.pst_value.pst_s_rsdm.pst_ntargno;
	if (ntargno < 1 || ntargno > rel->rdr_rel->tbl_attr_count)
	{
	    opx_error(E_OP0897_BAD_CONST_RESDOM);
	}
	att = rel->rdr_attr[ntargno];

	ops[0].opr_dt = (DB_DT_ID) att->att_type;
	ops[0].opr_len = att->att_width;
	ops[0].opr_prec = att->att_prec;
	ops[0].opr_collID = att->att_collID;
	if (!leavebase)
	 ops[0].opr_offset = att->att_offset;
	if (!leavebase)
	 if (cadf->opc_qeadf->qen_output > 0)
	 {
	    /* if there is an output row then place the result there. So,
	    ** lets add the base info for it.
	    */
	    ops[0].opr_base = cadf->opc_row_base[cadf->opc_qeadf->qen_output];

	    if (ops[0].opr_base < ADE_ZBASE || 
		    ops[0].opr_base >= ADE_ZBASE + cadf->opc_qeadf->qen_sz_base
		)
	    {
		opc_adbase(global, cadf, QEN_ROW,
					cadf->opc_qeadf->qen_output);
		ops[0].opr_base = 
			    cadf->opc_row_base[cadf->opc_qeadf->qen_output];
	    }
	 }
	 else
	 {
	    /* If there isn't an output row the we use the user output
	    ** base. Replace cursors are an example of this.
	    */
	    ops[0].opr_base = cadf->opc_qeadf->qen_uoutput + ADE_ZBASE;
	 }

	/* Set flag indicating RESDOM is UTF8. */
	if ((global->ops_adfcb->adf_utf8_flag & AD_UTF8_ENABLED) &&
		(abs(resdom->pst_sym.pst_dataval.db_datatype) == DB_VCH_TYPE ||
		 abs(resdom->pst_sym.pst_dataval.db_datatype) == DB_CHA_TYPE ||
		 abs(resdom->pst_sym.pst_dataval.db_datatype) == DB_TXT_TYPE ||
		 abs(resdom->pst_sym.pst_dataval.db_datatype) == DB_CHR_TYPE))
	    utf8unorm = TRUE;

	/* If updating/inserting a Unicode constant, value move is done
	** by ADE_UNORM (normalization). This assumes that Unicode columns
	** are inserted/updated by a single constant (host variable). 
	** Moreover, the constant must NOT be Unicode type, otherwise, the coercion
	** code tags on the normalization. */
	if (resdom->pst_right->pst_sym.pst_type == PST_CONST &&
	    (subqry->ops_mode == PSQ_APPEND || 
			subqry->ops_mode == PSQ_REPLACE) &&
		(abs(resdom->pst_sym.pst_dataval.db_datatype) == DB_NCHR_TYPE ||
		 abs(resdom->pst_sym.pst_dataval.db_datatype) == DB_NVCHR_TYPE) &&
	        (abs(resdom->pst_right->pst_sym.pst_dataval.db_datatype) != DB_NCHR_TYPE &&
	        abs(resdom->pst_right->pst_sym.pst_dataval.db_datatype) != DB_NVCHR_TYPE))
	    unorm = TRUE;
	/* This handles insert/select and update from, in which the source value
	** is NOT Unicode. */
	if (resdom->pst_right->pst_sym.pst_type == PST_VAR &&
	    (subqry->ops_mode == PSQ_APPEND || 
			subqry->ops_mode == PSQ_REPLACE) &&
		(abs(resdom->pst_sym.pst_dataval.db_datatype) == DB_NCHR_TYPE ||
		 abs(resdom->pst_sym.pst_dataval.db_datatype) == DB_NVCHR_TYPE) &&
	    abs(resdom->pst_right->pst_sym.pst_dataval.db_datatype) != DB_NCHR_TYPE &&
	    abs(resdom->pst_right->pst_sym.pst_dataval.db_datatype) != DB_NVCHR_TYPE)
	    unorm = TRUE;

	break;

     case PST_DBPARAMNO:
     {
	OPC_PST_STATEMENT   *opc_pst;
	QEF_CP_PARAM	    *param;
	QEN_NODE	    *qenp = global->ops_cstate.opc_curnode;
	bool		    is_byref;
		
	/* This RESDOM is actually a parameter list entry in a CALLPROC	    */
	/* statement. The processing is somewhat more complicated since we  */
	/* must create a parameter list entry with values bound directly to */
	/* the parameter values in the current DSH. The DSH context will    */
	/* change after the CALLPROC is executed and ADF can no longer be   */
	/* used to evaluate parameter values from the old DSH. We handle    */
	/* this by creating DB_DATA_VALUES in the parameter list at this    */
	/* time. */

	/* First move the parameter name to the QEF_USR_PARAM entry. The    */
	/* offset of the correct parameter list in base is given in the	    */
	/* action header for the statement we are currently compiling. */

	opc_pst = (OPC_PST_STATEMENT *)global->ops_statement->pst_opf;

	if (opc_pst->opc_stmtahd->ahd_atype == QEA_PROC_INSERT)
	    break;      /* ops[0] has already been set up for proc_insert's */

	/* Address QEF_CP_PARAM ptr in either CALLPROC action header
	** or TPROC node header. */
	if (qenp && qenp->qen_type == QE_TPROC)
	    param = &qenp->node_qen.qen_tproc.tproc_params[resdom->
			    pst_sym.pst_value.pst_s_rsdm.pst_rsno - 1];
	else param = &opc_pst->opc_stmtahd->qhd_obj.qhd_callproc.
		ahd_params[resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsno - 1];

	param->ahd_parm.parm_dbv.db_length = ops[1].opr_len;
	param->ahd_parm.parm_dbv.db_prec = ops[1].opr_prec;
	param->ahd_parm.parm_dbv.db_collID = ops[1].opr_collID;
	param->ahd_parm.parm_dbv.db_datatype = ops[1].opr_dt;

	STRUCT_ASSIGN_MACRO(ops[1], ops[0]);
	param->ahd_parm_row = OPC_CTEMP;
	if (!leavebase)
	    ops[0].opr_base = cadf->opc_row_base[OPC_CTEMP];
	param->ahd_parm_offset = ops[0].opr_offset =
	    global->ops_cstate.opc_temprow->opc_olen;

	if (ops[0].opr_len > 0)
	{
	    i4	    align;
		
	    align = ops[0].opr_len % sizeof (ALIGN_RESTRICT);
	    if (align != 0)
		align = sizeof (ALIGN_RESTRICT) - align;
	    global->ops_cstate.opc_temprow->opc_olen += ops[0].opr_len + align;
	}
	break;
    }

    case PST_RESROW_COL:

	if (cadf->opc_qeadf->qen_uoutput < 0)
	    opc_adbase(global, cadf, QEN_OUT, 0);

	ops[0].opr_base = cadf->opc_qeadf->qen_uoutput + ADE_ZBASE;
	ops[0].opr_dt = resdom->pst_sym.pst_dataval.db_datatype;
	ops[0].opr_prec = resdom->pst_sym.pst_dataval.db_prec;
	ops[0].opr_len = resdom->pst_sym.pst_dataval.db_length;
	ops[0].opr_collID = resdom->pst_sym.pst_dataval.db_collID;
	global->ops_cstate.opc_retrowoff += ops[0].opr_len;
	for_user = TRUE;
	break;
     	
     default:
	opx_error(E_OP0897_BAD_CONST_RESDOM);
    }

    /*
    **	Determine if the output type is a peripheral type.
    **	We will need this information later.
    */
    
    if ((err = adi_dtinfo(global->ops_adfcb, ops[0].opr_dt, &bits))
	    != E_DB_OK)
    {
	opx_verror(err, E_OP0783_ADI_FICOERCE,
		global->ops_adfcb->adf_errcb.ad_errcode);
    }
    
    /*
    ** If (type and length of the resdom != type and length of the
    ** attribute and prec of same for decimal datatype).
    **
    ** The if-true statement block was modified to coerce data
    ** into the temporary dsh row rather than the output row.
    ** This will prevent ADF problems caused by the fact that
    ** the data part of GCF buffers is currently unaligned
    ** (bug 33557). 
    **
    ** Also, a Unicode normalization skips all this. The ADE_UNORM
    ** operator does length resolution, too.
    */
    if (ops[0].opr_dt  != ops[1].opr_dt	    ||
	/* Don't coerce a long type to change length from unknown... */
	(((bits & AD_PERIPHERAL) == 0)
	         && (ops[0].opr_len != ops[1].opr_len))    ||
	/*
	** At this point opr_dt && opr_len are equal, so for
	** decimal datatype we want to check precision.
	*/
	(abs(ops[0].opr_dt) == DB_DEC_TYPE && 
	 ops[0].opr_prec != ops[1].opr_prec))
    {
	ADE_OPERAND ops_save;

	/* Find the function instance id to do the conversion; */
	if ((err = adi_ficoerce(global->ops_adfcb, 
		ops[1].opr_dt, ops[0].opr_dt, &opno)) != E_DB_OK
	    )
	{
	    opx_verror(err, E_OP0783_ADI_FICOERCE,
		global->ops_adfcb->adf_errcb.ad_errcode);
	}

	/* Coerce into temporary row only when 
	** we are dealing with an output row.
	*/
	if ((!leavebase && cadf->opc_qeadf->
	    qen_base[ops[0].opr_base - ADE_ZBASE].qen_array == QEN_OUT) || unorm)
	{
	    /* Save the ultimate target */
	    STRUCT_ASSIGN_MACRO(ops[0], ops_save);

	    /* Coerce into the temporary buffer. */
	    ops[0].opr_base = cadf->opc_row_base[OPC_CTEMP];
	    ops[0].opr_offset = global->ops_cstate.opc_temprow->opc_olen;

	    /*
	    ** If output length is not known, then insert a fixed
	    ** amount.
	    */

	    if ((bits & AD_PERIPHERAL) &&
		((ops[0].opr_len == ADE_LEN_UNKNOWN) ||
	         (ops[0].opr_len == ADE_LEN_LONG)))
	    {
		ops[0].opr_len = sizeof(ADP_PERIPHERAL);
		if (ops[0].opr_dt < 0)
		    ops[0].opr_len += 1;
	    }

	    /* increase the temp results buffer to hold our new value. */
	    if (ops[0].opr_len > 0)
	    {
		i4	    align;

		align = ops[0].opr_len % sizeof (ALIGN_RESTRICT);
		if (align != 0)
		    align = sizeof (ALIGN_RESTRICT) - align;
		global->ops_cstate.opc_temprow->opc_olen += ops[0].opr_len + align;
	    }

	    opc_adinstr(global, cadf, opno, segment, 2, ops, 1);

	    /* Make the temp buf the source and restore ultimate target */
	    STRUCT_ASSIGN_MACRO(ops[0], ops[1]);
	    STRUCT_ASSIGN_MACRO(ops_save, ops[0]);

	    if (	(bits & AD_PERIPHERAL)	    /* Peripheral DT */
		&&	(for_user)		    /* Destined for the user */
		&&	(subqry->ops_sqtype == OPS_MAIN))   /* "main" query */
	    {
		opno = ADE_REDEEM;
		ops[0].opr_len = ADE_LEN_LONG;
		
		/*
		** Mark that this QP will return large objects.
		*/
		
		global->ops_cstate.opc_qp->qp_status |=
					QEQP_LARGE_OBJECTS;
	    }
	    else
	    {
	      if (unorm)
                opno = ADE_UNORM;
              else
                opno = ADE_MECOPY;
	
	    }
	}
    }
    else
    {
	if (	(bits & AD_PERIPHERAL)	    /* Peripheral DT */
	    &&	(for_user)		    /* Destined for the user */
	    &&	(subqry->ops_sqtype == OPS_MAIN))   /* "main" query */
	{
	    opno = ADE_REDEEM;
	    ops[0].opr_len = ADE_LEN_LONG;
	    
	    /*
	    ** Mark that this QP will return large objects.
	    */
	    
	    global->ops_cstate.opc_qp->qp_status |=
				    QEQP_LARGE_OBJECTS;
	}
	else if (unorm)
	    opno = ADE_UNORM;
	else
	    opno = ADE_MECOPY;
    }

    opc_adinstr(global, cadf, opno, segment, 2, ops, 1);
    if (opno == ADE_REDEEM)
	ops[0].opr_len = ADE_LEN_LONG;
}

/*{
** Name: OPC_PROC_INSERT_TARGET_LIST	- Compiles target list for proc insert.
**
** Description:
**
** Inputs:
**	global -
**	    the query wide state variable for OPF/OPC
**	root -
**	    the root of the tree to be compiled.
**	cadf -
**	    The OPC version of the open CX.
**	rel -
**	    the result relation description for this query.
**	num_params_passed -
**	    the number of actual parameters in this procedure call.
**	attr_list -
**	    attribute list describing  parameters in the procedure being called.
**	temp_table_base -
**	    base for row to be inserted.
**	offset_list -
**	    offsets for parameters to be inserted.
**
** Outputs:
**	cadf -
**	    This will hold the ADF instructions that are needed for the
**	    expression evalutation.
**
**	Returns:
**	    none
**	Exceptions:
**	    none, except those that opx_error() raises.
**
** Side Effects:
**	    none
**
** History:
**	08-oct-92 (jhahn)
**	    Created
**	31-jul-96 (inkdo01)
**	    Added default coercions for actual lists which don't quite match 
**	    formal "set of" list.
**	11-mar-97 (inkdo01)
**	    Augment error detect to flag actual parms not covered by formal
**	    "set of" list.
**	22-jul-98 (shust01)
**	    moved reinitialization of resdom inside outer for loop (where
**	    looping over "set of" list, looking for actual to project).
**	    This will have us start at the root of the tree for every
**	    instance of the loop.  Prior to this, there was a chance of
**	    SEGVing because we would do a MEcmp of a resdom...->pst_rsname
**	    for pst_type = PST_TREE, which isn't good since the resdom
**	    structure for a PST_TREE isn't the full size of the structure,
**	    so we could be looking at unallocated storage.
[@history_template@]...
*/
VOID
opc_proc_insert_target_list(
    OPS_STATE	*global,
    PST_QNODE	*root,
    OPC_ADF	*cadf,
    RDR_INFO	*rel,
    i4		num_params_passed,
    i4		num_params_declared,
    DMF_ATTR_ENTRY *attr_list[],
    i4		temp_table_base,
    i4	offset_list[])

{
    i4			decl_param_no, passed_params_left;
    PST_QNODE		*resdom;
    ADE_OPERAND		ops[3];
    DMF_ATTR_ENTRY	*attr;
    DB_DATA_VALUE	dv;
    bool		found;

    /* First check for actual parms with no corresponding entry in 
    ** "set of" list. This is just plain wrong. */
    resdom = (PST_QNODE *) NULL;
    for (passed_params_left = num_params_passed;
	passed_params_left > 0;
	passed_params_left--)
    {
	if (resdom != NULL && resdom->pst_sym.pst_type != PST_TREE)
	    resdom = resdom->pst_left;
	else resdom = root;
	for (found = FALSE, decl_param_no = 0;
	    decl_param_no < num_params_declared && !found;
	    decl_param_no++)
	{
	    attr = attr_list[decl_param_no];
	    if (MEcmp((PTR) attr->attr_name.db_att_name,
		      (PTR) resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsname,
		      DB_ATT_MAXNAME) == 0) found = TRUE;
	}
	if (!found)
	{	/* flag error */
	    OPT_NAME	elem_name;
	    STncpy((char *)&elem_name,
		resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsname, DB_ATT_MAXNAME);
	    ((char *)(&elem_name))[ DB_ATT_MAXNAME ] = '\0';
	    opx_1perror(E_OP08B1_PARAM_INVALID, (PTR)&elem_name);
	}
    }

    /* Now loop over "set of" list, looking for actual to project (or
    ** else, the corresponding default value). */
    MEfill(sizeof (dv), (u_char)0, (PTR)&dv);
    for (decl_param_no = 0;
	 decl_param_no < num_params_declared;
	 decl_param_no++)
    {
        resdom = (PST_QNODE *) NULL;
	attr = attr_list[decl_param_no];
	for (passed_params_left = num_params_passed;
	     passed_params_left > 0;
	     passed_params_left--)
	{
	    if (resdom != NULL && resdom->pst_sym.pst_type != PST_TREE)
		resdom = resdom->pst_left;
	    else
		resdom = root;
	    if (MEcmp((PTR) attr->attr_name.db_att_name,
		      (PTR) resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsname,
		      DB_ATT_MAXNAME) == 0)
	    {
		ops[0].opr_dt = ops[1].opr_dt = attr->attr_type;
		ops[0].opr_prec = attr->attr_precision;
		ops[0].opr_len = attr->attr_size;
		ops[0].opr_collID = attr->attr_collID;
		ops[0].opr_offset = offset_list[decl_param_no];
		ops[0].opr_base = temp_table_base;
		opc_crupcurs(global, resdom->pst_right,
			     cadf, &ops[1], rel, (OPC_NODE *) NULL);
		opc_rdmove(global, resdom, ops, cadf, 
				ADE_SMAIN, (RDR_INFO *) NULL, FALSE);
		break;
	    }
	}
	if (passed_params_left == 0)
	{
	    /* No match amongst actuals - if NOT NULL NOT DEFAULT, it's an error.
	    ** Otherwise, compile in the default/null value. */
	    if (attr->attr_flags_mask & DMU_F_KNOWN_NOT_NULLABLE &&
		attr->attr_flags_mask & DMU_F_NDEFAULT)
	    {
		OPT_NAME	elem_name;
		STncpy((char *)&elem_name,
		    (char *)&attr->attr_name.db_att_name, DB_ATT_MAXNAME);
		((char *)(&elem_name))[ DB_ATT_MAXNAME ] = '\0';
		opx_1perror(E_OP08B0_NODEFAULT, (PTR)&elem_name);
	    }

	    ops[0].opr_dt = attr->attr_type;
	    ops[0].opr_len = attr->attr_size;
	    ops[0].opr_prec = attr->attr_precision;
	    ops[0].opr_collID = attr->attr_collID;
	    ops[0].opr_base = temp_table_base;
	    ops[0].opr_offset = offset_list[decl_param_no];

	    /* Get the default value for this att; */
	    opc_adempty(global,
		    &dv,
		    ops[0].opr_dt,
		    (i2) ops[0].opr_prec,
		    (i4) ops[0].opr_len);

	    /* If this one is nullable with no default, turn on the nullind. */
	    if (attr->attr_flags_mask & DMU_F_NDEFAULT)
	     *((u_char *)dv.db_data + dv.db_length-1) = 1;

	    /* compile in the default value as a const; */
	    opc_adconst(global, cadf, &dv, &ops[1], DB_SQL, ADE_SMAIN);

	    /* compile an instruction to move the const to the result row; */
	    opc_adinstr(global, cadf, ADE_MECOPY, ADE_SMAIN, 2, ops, 1);
	}	/* end of no actual parm match */
	
    }	    /* end of formal parameter list */

}

/*{
** Name: OPC_CRUPCURS	- Compile a ADE_CX for a replace cursor statement.
**
** Description:
**      This routine is now a stub for opc_crupcurs1 (in the same way that
**	opc_cqual is a stub for entering opc_qual1). It permits the init
**	of some fields in OPC_ADF, specifically for compiling CASE expressions
**	which have branching requirements. Making this a stub allows these
**	initializations to be incorporated without modifying the many
**	external calls to opc_crupcurs. 
[@comment_line@]...
**
** Inputs:
**	global -
**	    the query wide state variable for OPF/OPC
**	root -
**	    the root of the tree to be compiled.
**	cadf -
**	    The OPC version of the open CX.
**	resop -
**	    a pointer to an operand that holds the desired result type but
**	    is otherwise uninitialized.
**	rel -
**	    the result relation description for this query.
**	cnode -
**	    Ptr to current node state structure (but only for compiling
**	    parameter CX for table procs).
**
** Outputs:
**	cadf -
**	    This will hold the ADF instructions that are needed for the
**	    expression evalutation.
**	resop -
**	    holds a description of the expression.
**
**	Returns:
**	    none
**	Exceptions:
**	    none, except those that opx_error() raises.
**
** Side Effects:
**	    none
**
** History:
**	19-june-00 (inkdo01)
**	    Add support for CASE expression (changing opc_crupcurs into 
**	    static opc_crupcurs1 with an extern stub for opc_crupcurs to
**	    init OPC_ADF - same as for opc_qual).
**	9-may-2008 (dougi)
**	    Changes for table procedures, including support of PST_VAR
**	    (but only under a table procedure node).
*/
VOID
opc_crupcurs(
		OPS_STATE	*global,
		PST_QNODE	*root,
		OPC_ADF		*cadf,
		ADE_OPERAND	*resop,
		RDR_INFO	*rel,
		OPC_NODE	*cnode)
{
    ADE_OPERAND		truelab, falselab;

    /* Simply initialize the T/F label anchors, in the event that 
    ** expression contains nested CASE expressions. */

    STRUCT_ASSIGN_MACRO(labelinit, truelab);
    STRUCT_ASSIGN_MACRO(labelinit, falselab);
    cadf->opc_tidatt = (i4 *)NULL;
    cadf->opc_truelab = (char *) &truelab;
    cadf->opc_falselab = (char *) &falselab;
    cadf->opc_whenlvl = 0;
    if (resop != (ADE_OPERAND *)NULL) resop->opr_offset = OPR_NO_OFFSET;
						/* init result offset */
    
    opc_crupcurs1(global, root, cadf, resop, rel, cnode);

    opc_adlabres(global, cadf, &truelab);	/* resolve label chains */
    opc_adlabres(global, cadf, &falselab);

}

/*{
** Name: OPC_CRUPCURS1	- Compile a ADE_CX for a replace cursor statement.
**
** Description:
**      This routine does the same thing as opc_cqual() with two important 
**      exceptions. First, this routine handles CURVAL nodes where opc_cqual() 
**      doesn't. Second, this routine doesn't handle VAR nodes, whereas 
**      opc_cqual() does. This routine was originally created to handle the 
**      qtree compilation needs of replace cursor statements, and replace 
**      cursor query trees can contain CURVAL nodes, and *cannot* contain 
**      VAR nodes. This routine was created to support the correct error 
**      conditions, and because special parameters are required by CURVAL 
**      nodes that don't make sense for more general query trees. 
**	Since its inception, if statements for DBP's have started using this
**	routine, since their needs exactly matched the needs of replace
**	cursor qtree compilation.
**        
**      I won't go into a long description of this routine, instead, please 
**      see the description of opc_cqual(). 
[@comment_line@]...
**
** Inputs:
**	global -
**	    the query wide state variable for OPF/OPC
**	root -
**	    the root of the tree to be compiled.
**	cadf -
**	    The OPC version of the open CX.
**	resop -
**	    a pointer to an operand that holds the desired result type but
**	    is otherwise uninitialized.
**	rel -
**	    the result relation description for this query.
**	cnode -
**	    Ptr to current node state structure (but only for compiling
**	    parameter CX for table procs).
**
** Outputs:
**	cadf -
**	    This will hold the ADF instructions that are needed for the
**	    expression evalutation.
**	resop -
**	    holds a description of the expression.
**
**	Returns:
**	    none
**	Exceptions:
**	    none, except those that opx_error() raises.
**
** Side Effects:
**	    none
**
** History:
**      17-Feb-86 (eric)
**          created
**	20-feb-89 (paul)
**	    Enhanced to support PST_RULEVAR query tree nodes.
**	25-jul-89 (jrb)
**	    Now passes db_data field of DB_DATA_VALUEs to opc_adcalclen so that
**	    result length calculations based on the actual value of the inputs.
**	15-dec-89 (jrb)
**	    Added support for DB_ALL_TYPE in fi descs. (For outer join project.)
**	16-jan-91 (stec)
**	    Added missing opr_prec initialization.
**      17-jul-92 (anitap)
**          Cast naked NULL passed into opc_adcalcen().
**	14-dec-92 (jhahn)
**	    Added support for byrefs.
**	12-feb-93 (jhahn)
**	    Added statement level rules. (FIPS)
**	17-may-93 (jhahn)
**	    Moved code for allocating before row for replace actions from
**	    opc_addrules to here.
**	10-nov-95 (inkdo01)
**	    Changes to support reorganization of QEF_AHD, QEN_NODE structs.
**      06-mar-96 (nanpr01)
**          Removed the dependency of DB_MAXTUP. This is now replace with
**	    global->ops_cb->ops_server->opg_maxtup which is the max tuple
**	    limit for the installation. Also init the ttb_pagesize to -1.
**      20-mar-96 (nanpr01)
**          Call opc_pagesize to determine the temporary table page_size
**          based on tuple length. This is need for variable page size
**          project.
**      18-Jan-1999 (hanal04) Bug 92148 INGSRV494
**          Handle the non-cnf case to ensure the expression is evaluated
**          correctly. The new logic mirrors code used in opc_qual1().
**	10-Mar-1999 (shero03)
**	    Support 4 arguments
**	23-feb-00 (hayke02)
**	    Removed opf_noncnf - we now test for PST_NONCNF in
**	    global->ops_statement->pst_stmt_mask directly. This change
**	    fixes bug 100484.
**	19-june-00 (inkdo01)
**	    Add support for CASE expression (changing opc_crupcurs into 
**	    static opc_crupcurs1 with an extern stub for opc_crupcurs to
**	    init OPC_ADF - same as for opc_qual).
**	27-jul-00 (inkdo01)
**	    Save "when" label across case expressions.
**	11-sep-00 (inkdo01)
**	    Fix for bug in handling nested "case" expressions (102584).
**	14-nov-00 (inkdo01)
**	    Another "case" fix (Startrak 10457740;1).
**	14-dec-03 (inkdo01)
**	    Add code to support iterative IN-list structures.
**	3-feb-04 (inkdo01)
**	    Changed DB_TID to DB_TID8 for table partitioning (also 
**	    fixed an error message FIXME while I was here).
**	16-mar-04 (hayke02)
**	    Modify IN-list change above so that we now don't deal with 'not in'
**	    clauses. This change fixes problem INGSRV 2758, bug 111968.
**	26-mar-04 (inkdo01)
**	    Fix problem with multiple work results in nested CASE.
**	22-nov-04 (inkdo01)
**	    Slight adjustment to the above to fix bad results from IN-lists 
**	    other ANDs and ORs.
**	08-apr-05 (inkdo01)
**	    Fix errors in ii_iftrue ops issued inside case expressions
**	    (bug 114011).
**	22-june-06 (dougi) SIR 116219
**	    Add support for OUTPUT parameters (as well as BYREF).
**	03-Apr-2007 (kiria01) b117754
**	    Handle MOP operands correctly.
**	20-march-2008 (dougi)
**	    Identify positional parameters and support compilation of 
**	    DBPARAMs from table procedures.
**	25-Jun-2008 (kiria01) SIR120473
**	    Move pst_isescape into new u_i2 pst_pat_flags.
**	    Made pst_opmeta into u_i2 and widened pst_escape.
**	26-Oct-2008 (kiria01) SIR121012
**	    Check resop before de-referencing in handling RESDOMs. When
**	    handling target list, the caller does not need the information
**	07-Jan-2009 (kiria01) SIR121012
**	    Convert a constant boolean literal into the appropriate
**	    execution state setting with ADE_SETFALSE or ADE_SETTRUE.
**	    These constants can only have got into the parse tree by
**	    the predicate functions being called from adf_func - thus
**	    we set the constant state as appropriate instead.
**	    This applies the same change as in opc_cqual1.
**	01-Nov-2009 (kiria01) b122822
**	    Added support for ADE_COMPAREN & ADE_NCOMPAREN instructions
**	    for big IN lists.
**	03-Nov-2009 (kiria01) b122822
**	    Support both sorted and traditional INLIST form for generality
*/
static VOID
opc_crupcurs1(
		OPS_STATE	*global,
		PST_QNODE	*root,
		OPC_ADF		*cadf,
		ADE_OPERAND	*resop,
		RDR_INFO	*rel,
		OPC_NODE	*cnode)
{
    ADE_OPERAND	    ops[ADI_MAX_OPERANDS + 1];
    i4		    opnum;
    i4		    align;
    OPC_BASE	    *parms;
    i4		    parm_no;
    i4		    attno;

    if (root == NULL)
	return;

    switch (root->pst_sym.pst_type)
    {
     case PST_AND:
        /* 92148 - Handle non-cnf case */
        if(global->ops_statement->pst_stmt_mask & PST_NONCNF)
        {
            ADE_OPERAND locallab;
            char        *truelab;
 
            /* Left subtree of AND being true should jump to the AND
            ** (skipping what's in between). Falses on either side just
            ** join the false list. */
            STRUCT_ASSIGN_MACRO(labelinit, locallab);
            truelab = cadf->opc_truelab;        /* save across call */
            cadf->opc_truelab = (char *)&locallab;
            opc_crupcurs1(global, root->pst_left, cadf, resop, rel, cnode);
            opc_adlabres(global, cadf, &locallab);
            cadf->opc_truelab = truelab;                /* restore after call */
            opc_adinstr(global, cadf, ADE_ANDL, ADE_SMAIN, 1, 
			(ADE_OPERAND *)cadf->opc_falselab, 0);
            opc_crupcurs1(global, root->pst_right, cadf, resop, rel, cnode);
        }
        else
        {
	    /* first compile the left, then compile the and instruction, finally
	    ** compile the right.
	    */
	    opc_crupcurs1(global, root->pst_left, cadf, resop, rel, cnode);
	    opc_adinstr(global, cadf, ADE_AND, ADE_SMAIN, 0, resop, 0);
	    opc_crupcurs1(global, root->pst_right, cadf, resop, rel, cnode);
        }
	break;

     case PST_OR:
        /* 92148 - Handle non-cnf case */
        if(global->ops_statement->pst_stmt_mask & PST_NONCNF)
        {
            ADE_OPERAND locallab;
            char        *falselab;
 
            /* Left subtree of OR being false should jump to the OR
            ** (skipping what's in between). Trues on either side just
            ** join the true list. */
            STRUCT_ASSIGN_MACRO(labelinit, locallab);
            falselab = cadf->opc_falselab;      /* save across call */
            cadf->opc_falselab = (char *)&locallab;
            opc_crupcurs1(global, root->pst_left, cadf, resop, rel, cnode);
            opc_adlabres(global, cadf, &locallab);
            cadf->opc_falselab = falselab;      /* restore after call */
            opc_adinstr(global, cadf, ADE_ORL, ADE_SMAIN, 1, 
			(ADE_OPERAND *)cadf->opc_truelab, 0);
            opc_crupcurs1(global, root->pst_right, cadf, resop, rel, cnode);
        }
        else
        {
	    /* first compile the left, then compile the or instruction, finally
	    ** compile the right.
	    */
	    opc_crupcurs1(global, root->pst_left, cadf, resop, rel, cnode);
	    opc_adinstr(global, cadf, ADE_OR, ADE_SMAIN, 0, resop, 0);
	    opc_crupcurs1(global, root->pst_right, cadf, resop, rel, cnode);
        }
	break;

     case PST_CASEOP:
     {
	PST_QNODE	*whlistp, *thenp;
	ADE_OPERAND	localbrf, localbr, whentrue, whenfalse;
	ADE_OPERAND	result, *oldres;
	char		*oldtrue, *oldfalse, *oldwhen;
	i4		align, oldoff;
	bool		noelse = FALSE, iftrue;

	/* The case function contains several parts, each of which generates
	** code into the ADF expression. For "simple" case functions, the 
	** source expression is compiled to produce a value in a temporary
	** location for the upcoming comparisons (if I can figure out how to
	** do it. Otherwise, we'll recompile the source expression for each
	** when clause). 
	**
	** Then for each when clause, the comparison or search expression is 
	** compiled, followed by a "BRANCHF" (to bypass the result evaluation
	** when the comparison is FALSE), followed by the result expression 
	** for the when clause, followed by a "BRANCH" (to exit the case).
	**
	** To avoid compiling a MECOPY for each when clause to move the result 
	** to the eventual result location, the "resop" parm sent to compile
	** the result expressions is filled in with the actual target location 
	** information of the case function. 
	*/

	oldres = resop;		/* save around the case function */

	/* Need to allocate the result location here. Note, if resop
	** already contains a valid location (opr_offset >= 0), it is
	** because this is a nested case, but new result must still be
	** init'ed. */

	result.opr_dt = root->pst_sym.pst_dataval.db_datatype;
	result.opr_len = root->pst_sym.pst_dataval.db_length;
	result.opr_prec = root->pst_sym.pst_dataval.db_prec;
	result.opr_collID = root->pst_sym.pst_dataval.db_collID;
	result.opr_base = cadf->opc_row_base[OPC_CTEMP];
	result.opr_offset = global->ops_cstate.opc_temprow->opc_olen;

	/* Re-align the temp buffer. */
	align = result.opr_len % sizeof(ALIGN_RESTRICT);
	if (align != 0) align = sizeof(ALIGN_RESTRICT) - align;
	global->ops_cstate.opc_temprow->opc_olen += align + result.opr_len;

	/* If resop not init'ed yet, make it the local result. */
	if (resop == NULL || resop->opr_offset == OPR_NO_OFFSET)
	{
	    resop = &result;
	    STRUCT_ASSIGN_MACRO(*resop, *oldres);
	}

	/* Save old label anchors and set up new ones. */
	oldtrue = cadf->opc_truelab;
	oldfalse = cadf->opc_falselab;
	oldwhen = cadf->opc_whenlab;
	STRUCT_ASSIGN_MACRO(labelinit, localbr);
	cadf->opc_whenlab = (char *)&localbr;

	/* Simple case setup goes here (if I figure out how to do it). */


	/* Loop over the when clauses, building code as we go. */
	for (whlistp = root->pst_left; whlistp != (PST_QNODE *)NULL;
		whlistp = whlistp->pst_left)
	{
	    /* Reset label variables in case "and"/"or" are present in "when"s. */
	    cadf->opc_truelab = (char *)&whentrue;
	    cadf->opc_falselab = (char *)&whenfalse;
	    STRUCT_ASSIGN_MACRO(labelinit, whentrue);
	    STRUCT_ASSIGN_MACRO(labelinit, whenfalse);
	    if (noelse = (whlistp->pst_right->pst_left != (PST_QNODE *)NULL))
	    {
		/* This stuff only executes for "when" clauses (i.e., with 
		** comparison expressions to compile). */

		/* Prepare label. */
		STRUCT_ASSIGN_MACRO(labelinit, localbrf);
		/* Build the comparison code. */
		/* Since "when"s of "case" with a source expression are currently 
		** compiled as "<source expr> = <comparand expr>", no distinction
		** is made between SEARCHED and SIMPLE cases. */
		if (TRUE || root->pst_sym.pst_value.pst_s_case.pst_casetype ==
				PST_SEARCHED_CASE)
		{
		    cadf->opc_whenlvl++;	/* to indicate noncnf processing */
		    (VOID) opc_crupcurs1(global, whlistp->pst_right->pst_left,
			cadf, (ADE_OPERAND *)NULL, rel, cnode);
					/* compile serach expression */
		    cadf->opc_whenlvl--;
		}
		else
		{
		}

		/* Emit "BRANCHF" operator (to skip to next when). */
		opc_adinstr(global, cadf, ADE_BRANCHF, ADE_SMAIN, 1, &localbrf, 0);
	    }

	    /* Compile corresponding result expression, first resolving any
	    ** OR true labels encountered in the when expression. */
	    opc_adlabres(global, cadf, &whentrue);

	    if ((thenp = whlistp->pst_right->pst_right)->pst_sym.pst_type
		== PST_BOP && thenp->pst_sym.pst_value.pst_s_op.pst_opno
		== global->ops_cb->ops_server->opg_iftrue)
	    {
		/* THEN starts with ii_iftrue - ... */
		iftrue = TRUE;
		oldoff = resop->opr_offset;
		resop->opr_offset = OPR_NO_OFFSET;
	    }
	    else iftrue = FALSE;

	    (VOID) opc_crupcurs1(global, thenp, cadf, resop, rel, cnode);

	    if (iftrue)
	    {
		/* Now we must copy the stupid thing to resop. */
		STRUCT_ASSIGN_MACRO(*resop, ops[0]);
		STRUCT_ASSIGN_MACRO(*resop, ops[1]);
		ops[0].opr_offset = resop->opr_offset = oldoff;
		opc_adinstr(global, cadf, ADE_MECOPY, ADE_SMAIN, 2, ops, 1);
	    }

	    /* Emit unconditional "BRANCH" operator (to exit case). */
	    if (whlistp->pst_left)	/* only if NOT last when or else */
		opc_adinstr(global, cadf, ADE_BRANCH, ADE_SMAIN, 1, 
					(ADE_OPERAND *)cadf->opc_whenlab, 0);

	    /* Resolve FALSE branch to next when (or else). If this was the
	    ** else, there's no label to resolve & we just drop out of loop. */
	    opc_adlabres(global, cadf, &whenfalse);
	    if (noelse) opc_adlabres(global, cadf, &localbrf);
	}	/* end of when loop */

	/* If there was no else clause, we must assign NULL to the result here. */
	if (noelse)
	{
	    /* This case doesn't arise, because the parser forces a "else NULL"
	    ** in the absence of a coded "else". */
	}

	/* Finally, resolve the unconditional branch here. */
	opc_adlabres(global, cadf, (ADE_OPERAND *)cadf->opc_whenlab);

	cadf->opc_truelab = oldtrue;
	cadf->opc_falselab = oldfalse;
	cadf->opc_whenlab = oldwhen;

	if (oldres == resop)
	    break;
	else resop = oldres;

	/* See if result must now be coerced. */
	if ((resop->opr_dt == result.opr_dt ||
		resop->opr_dt == -result.opr_dt) &&
	    (resop->opr_len == OPC_UNKNOWN_LEN ||
	     (resop->opr_len == result.opr_len &&
	      (resop->opr_prec == result.opr_prec ||
	      abs(result.opr_dt) != DB_DEC_TYPE) &&
	     resop->opr_dt == result.opr_dt)))
	{
	    STRUCT_ASSIGN_MACRO(result, *resop);
				/* no coercion - just copy result opnd */
	}
	else
	{
	    /* Must coerce result into desired form. */
	    opc_adtransform(global, cadf, &result, resop, ADE_SMAIN, (bool)TRUE,
			(bool)FALSE);
	}

	break;
     }	/* end of PST_CASEOP */
	

     case PST_UOP:
     case PST_BOP:
     case PST_COP:
     case PST_MOP:
     {
	PTR		data[ADI_MAX_OPERANDS];
	PTR		*dataptr = &data[0];
        ADE_OPERAND	*opsptrs[ADI_MAX_OPERANDS];
        ADE_OPERAND	**opsptr = &opsptrs[0];
	PST_QNODE	*lqnode;
	PST_QNODE	*resqnode;
	PST_QNODE	*inlist;
	i4		i;
	i2		leftvisited = FALSE;
	bool		notin;
	u_i4		pat_flags;

	opnum = 0;
	i = 0;

	if (root->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_fitype !=
								ADI_COMPARISON
	    )
	{
	    opnum = 1;
	}

	if (root->pst_sym.pst_type == PST_MOP)
	{
	    resqnode = root;
	    lqnode = resqnode->pst_right;
	}
	else
	{
	    lqnode = root->pst_left;	/* Start down the left side */
	    if (lqnode == NULL)
	    {
		lqnode = root->pst_right;	/* no left, so do right side */
		leftvisited = TRUE;         /* say we did the left side */
	    }
	}
		
	if (root->pst_sym.pst_value.pst_s_op.pst_flags & PST_INLIST &&
		root->pst_right->pst_sym.pst_type == PST_CONST)
	{
	    inlist = root->pst_right->pst_left;
	    if (root->pst_sym.pst_value.pst_s_op.pst_opno == 
		global->ops_cb->ops_server->opg_ne)
		    notin = TRUE;
	    else notin = FALSE;

	    /* Generate mass compare instructions for IN lists. */
	    if (root->pst_sym.pst_value.pst_s_op.pst_flags & PST_INLISTSORT)
	    {
		ADE_OPERAND *Pops;
		i4 cnt = 1; /* For LHS */
		inlist = root->pst_right;
		do
		    cnt++; /* count in the RHSs */
		while (inlist = inlist->pst_left);
		inlist = root->pst_right;
		Pops = (ADE_OPERAND*)MEreqmem(0, sizeof(ADE_OPERAND)*cnt, FALSE, NULL);
		/* Generate LHS we are not interested in the const state of the
		** LHS as the parser will not generate PST_INLIST in that case. */
		Pops[0].opr_dt = abs(root->pst_left->pst_sym.pst_dataval.db_datatype);
		Pops[0].opr_len = OPC_UNKNOWN_LEN;
		Pops[0].opr_prec = OPC_UNKNOWN_PREC;
		Pops[0].opr_collID = root->pst_left->pst_sym.pst_dataval.db_collID;
		Pops[0].opr_offset = OPR_NO_OFFSET;
		opc_crupcurs1(global, lqnode, cadf, &Pops[0], rel, cnode);
		for (i = 1; i < cnt; i++)
		{
		    /* This is a list of constants that will be in sorted order
		    ** so compile them into the CX and fill in ops[1]..ops[n] */
		    opc_adconst(global, cadf, &inlist->pst_sym.pst_dataval, &Pops[i],
			    inlist->pst_sym.pst_value.pst_s_cnst.pst_cqlang, ADE_SMAIN);
		    inlist = inlist->pst_left;
		}
		opc_adinstr(global, cadf, notin?ADE_NCOMPAREN
						 :ADE_COMPAREN, ADE_SMAIN, cnt, Pops, 0);
		MEfree((PTR)Pops);
		/* Drop out - nothing more to do. */
		break;
	    }
	}
	else inlist = (PST_QNODE *) NULL;

	while (lqnode)
	{	       

	    ops[opnum].opr_dt = 
		    root->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_dt[i];

	    if (ops[opnum].opr_dt == DB_ALL_TYPE)
	    {
		ops[opnum].opr_dt = lqnode->pst_sym.pst_dataval.db_datatype;
	    }
	    else if (lqnode->pst_sym.pst_dataval.db_datatype < 0)
	    {
		ops[opnum].opr_dt = -ops[opnum].opr_dt;
	    }

	    ops[opnum].opr_offset = OPR_NO_OFFSET;
	    opc_crupcurs1(global, lqnode, cadf, &ops[opnum], rel, cnode);

	    *dataptr = lqnode->pst_sym.pst_dataval.db_data;
	    dataptr++;
	    *opsptr = &ops[opnum];
	    opsptr++;
	    i++;
	    opnum += 1;

     	    if (root->pst_sym.pst_type == PST_MOP)
	    {
		resqnode = resqnode->pst_left;
		if (resqnode)
		    lqnode = resqnode->pst_right;   /* point to the operand */
		else
		    break;
	    }
	    else
	    {
	    	if (leftvisited)
		    break;
		leftvisited = TRUE;
	    	lqnode = root->pst_right;  /* go to right side */
	    }
	}


	if (root->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_fitype ==
								ADI_COMPARISON
	    )
	{
	    /* These operators have no result except the change in ADF
	    ** state that they cause.
	    */
	    resop = NULL;
	}
	else
	{
	    ADI_FI_DESC	fi = *root->pst_sym.pst_value.pst_s_op.pst_fdesc;
	    ops[0].opr_dt = 
		    root->pst_sym.pst_dataval.db_datatype;
	    ops[0].opr_base = cadf->opc_row_base[OPC_CTEMP];
	    ops[0].opr_offset = global->ops_cstate.opc_temprow->opc_olen;
	    /* Update temp copy lenspec if dtfamily result */
	    if (abs(fi.adi_dtresult) == DB_DTE_TYPE)
	    {
		adi_coerce_dtfamily_resolve(global->ops_adfcb, &fi, 
			fi.adi_dt[0], ops[0].opr_dt, &fi);
	    }
	    
	    /* calculate the length of the result */
	    opc_adcalclen(global, cadf, &fi.adi_lenspec,
		    opnum - 1, opsptrs, &ops[0], data);

	    /* increase the temp results buffer to hold our new value. */
	    if (ops[0].opr_len > 0)
	    {
		align = ops[0].opr_len % sizeof (ALIGN_RESTRICT);
		if (align != 0)
		    align = sizeof (ALIGN_RESTRICT) - align;
		global->ops_cstate.opc_temprow->opc_olen += ops[0].opr_len + align;
	    }
	}

	pat_flags = root->pst_sym.pst_value.pst_s_op.pst_pat_flags;
	if (pat_flags != AD_PAT_DOESNT_APPLY)
	{
	    ADE_OPERAND	    escape_ops[2];
	    DB_DATA_VALUE	    escape_dv;
	    i4		    nops = 1;

	    escape_dv.db_prec = 0;
	    escape_dv.db_collID = -1;
	    escape_dv.db_data = 
			(char*)&root->pst_sym.pst_value.pst_s_op.pst_escape;
	    if ((pat_flags & AD_PAT_ISESCAPE_MASK) == AD_PAT_HAS_UESCAPE)
	    {
		escape_dv.db_datatype = DB_NCHR_TYPE;
		escape_dv.db_length = sizeof(UCS2);
	    }
	    else
	    {
		escape_dv.db_datatype = DB_CHA_TYPE;
		escape_dv.db_length = CMbytecnt((char*)escape_dv.db_data);
	    }
	    opc_adconst(global, cadf, &escape_dv, &escape_ops[0],
						DB_SQL, ADE_SMAIN);
	    if (pat_flags & ~AD_PAT_ISESCAPE_MASK)
	    {
		DB_DATA_VALUE sf_dv;
		sf_dv.db_prec = 0;
		sf_dv.db_collID = -1;
		sf_dv.db_data = (char*)&root->pst_sym.pst_value.pst_s_op.pst_pat_flags;
		sf_dv.db_datatype = DB_INT_TYPE;
		sf_dv.db_length = sizeof(u_i2);
		opc_adconst(global, cadf, &sf_dv, &escape_ops[1],
						DB_SQL, ADE_SMAIN);
		nops++;
	    }
	    opc_adinstr(global, cadf, ADE_ESC_CHAR, ADE_SMAIN, 
						nops, escape_ops, 0);
	}

	if (resop == NULL)  
	{
	    opc_adinstr(global, cadf, 
			    root->pst_sym.pst_value.pst_s_op.pst_opinst, 
						    ADE_SMAIN, opnum, ops, 0);
	}
	else
	{
	    opc_adinstr(global, cadf, 
			    root->pst_sym.pst_value.pst_s_op.pst_opinst, 
						    ADE_SMAIN, opnum, ops, 1);

	    /* if the actual and intended result datatypes are the same,
	    ** ignoring nullability (which is why we have the second comparison
	    ** with the negation), then no coercion is needed.
	    */
	    if (resop->opr_dt == 
			root->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_dtresult
		    || resop->opr_dt == -root->pst_sym.pst_value.pst_s_op.
							pst_fdesc->adi_dtresult
		    || root->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_dtresult
				     == DB_ALL_TYPE
		)
	    {
		/* assign the base and index numbers to resop. */
		STRUCT_ASSIGN_MACRO(ops[0], *resop);
	    }
	    else
	    {
		opc_adtransform(global, cadf, &ops[0],
		    resop, ADE_SMAIN, (bool)TRUE, (bool)FALSE);
	    }
	}

	/* Magic loop to iterate over [not] in-list rather than recurse.
	** Hopefully, this will eliminate the IN-list blown stack
	** problems forever. */
	if (inlist != (PST_QNODE *) NULL)
	{
	    ADE_OPERAND	locallab;
	    char	*savelab;
	    i4		opcode;

	    STRUCT_ASSIGN_MACRO(labelinit, locallab);
	    if (notin)
	    {
		opcode = ADE_ANDL;
		savelab = cadf->opc_truelab;
		cadf->opc_truelab = (char *)&locallab;
	    }
	    else
	    {
		opcode = ADE_ORL;
		savelab = cadf->opc_falselab;
		cadf->opc_falselab = (char *)&locallab;
	    }
 
	    for (; inlist; inlist = inlist->pst_left)
	    {
		/* Insert a OR/AND followed by another "="/"<>" comparison
		** for each remaining PST_CONST in the constant list. */
		opc_adinstr(global, cadf, opcode, ADE_SMAIN, 1, 
		    (ADE_OPERAND *) ((notin) ? 
			cadf->opc_truelab : cadf->opc_falselab), 0);
	    
        	ops[1].opr_len = OPC_UNKNOWN_LEN;
        	ops[1].opr_prec = OPC_UNKNOWN_PREC;
        	ops[1].opr_collID = -1;
		ops[1].opr_offset = OPR_NO_OFFSET;	/* show no result loc. */
		/* Trick it into using reasonable scale if coercing to DECIMAL
		** for a binary operation (major kludge). */
		if ((abs(root->pst_left->pst_sym.pst_dataval.db_datatype) ==
							    DB_DEC_TYPE) &&
		    (abs(inlist->pst_sym.pst_dataval.db_datatype) !=
			abs(root->pst_left->pst_sym.pst_dataval.db_datatype)) &&
		    ((abs(inlist->pst_sym.pst_dataval.db_datatype) ==
							    DB_VCH_TYPE) ||
		    (abs(inlist->pst_sym.pst_dataval.db_datatype) ==
							    DB_CHA_TYPE) ||
		    (abs(inlist->pst_sym.pst_dataval.db_datatype) ==
							    DB_TXT_TYPE) ||
		    (abs(inlist->pst_sym.pst_dataval.db_datatype) ==
							    DB_CHR_TYPE)))
		{
		    global->ops_gmask |= OPS_DECCOERCION;
		    ops[1].opr_prec = root->pst_left->
					pst_sym.pst_dataval.db_prec;
		}
		opc_crupcurs1(global, inlist, cadf, &ops[1], rel, cnode);
					/* materialize/coerce constant */
		global->ops_gmask &= (~OPS_DECCOERCION);

		opc_adinstr(global, cadf,
			    root->pst_sym.pst_value.pst_s_op.pst_opinst,
						    ADE_SMAIN, 2, ops, 0);
					/* perform "="/"<>" comparison */
	    }
	    opc_adlabres(global, cadf, &locallab);
	    if (notin)
		cadf->opc_truelab = savelab;
	    else cadf->opc_falselab = savelab;
	}

	break;
     }

     case PST_CONST:
	parm_no = root->pst_sym.pst_value.pst_s_cnst.pst_parm_no;
	switch (root->pst_sym.pst_value.pst_s_cnst.pst_tparmtype)
	{
	 case PST_RQPARAMNO:
	    /*
	    ** We currently don't allow repeat replace cursors, but when we
	    ** do, this will let me know to add that functionality.
	    */
	    opx_error(E_OP0880_NOT_READY);
	    parms = global->ops_cstate.opc_rparms;


	    ops[0].opr_dt = parms[parm_no].opc_dv.db_datatype;
	    ops[0].opr_prec = parms[parm_no].opc_dv.db_prec;
	    ops[0].opr_len = parms[parm_no].opc_dv.db_length;
	    ops[0].opr_collID = parms[parm_no].opc_dv.db_collID;
	    ops[0].opr_offset = parms[parm_no].opc_offset;
	    ops[0].opr_base = cadf->opc_repeat_base[parm_no];

	    if (ops[0].opr_base < ADE_ZBASE || 
		    ops[0].opr_base >= ADE_ZBASE + cadf->opc_qeadf->qen_sz_base
		)
	    {
		opc_adbase(global, cadf, QEN_PARM, parm_no);
		ops[0].opr_base = cadf->opc_repeat_base[parm_no];
	    }
	    break;

	 case PST_LOCALVARNO:
	    /*
	    ** We currently don't allow replace cursors in procedures, but 
	    ** when we do, this will let me know to add that functionality.
	    */
	    opc_lvar_row(global, parm_no, cadf, &ops[0]);
	    break;

	 case PST_USER:
	    /* This is a constant expression, so do any compiling into the
	    ** virgin segment. See the comment in the operator case above.
	    ** One special case however, TRUE or FALSE should be instanciated
	    ** as instructions ADE_SETTRUE or ADE_SETFALSE instead of going
	    ** into VIRGIN segment.
	    */
	    if (root->pst_sym.pst_dataval.db_datatype == DB_BOO_TYPE)
	    {
		opc_adinstr(global, cadf, !*(char*)root->pst_sym.pst_dataval.db_data
				? ADE_SETFALSE
				: ADE_SETTRUE, ADE_SMAIN, 0, 0, 0);
	    }
	    else if (root->pst_sym.pst_dataval.db_datatype == -DB_BOO_TYPE)
	    {
		opc_adinstr(global, cadf,
			(((char*)root->pst_sym.pst_dataval.db_data)[1] & ADF_NVL_BIT) ||
					!*(char*)root->pst_sym.pst_dataval.db_data
				? ADE_SETFALSE
				: ADE_SETTRUE, ADE_SMAIN, 0, 0, 0);
	    }

            opc_adconst(global, cadf, &root->pst_sym.pst_dataval, &ops[0],
			root->pst_sym.pst_value.pst_s_cnst.pst_cqlang, ADE_SMAIN);
	    break;

	 default:
	    opx_error(E_OP0897_BAD_CONST_RESDOM);
	}

	if (resop != NULL)  
	{
	    /* if the actual and intended result datatypes are the same,
	    ** ignoring nullability (which is why we have the second comparison
	    ** with the negation), then no coercion is needed.
	    **
	    ** Special test is made on opr_offset to distinguish case
	    ** results which may require coercion (not just MECOPY). Even
	    ** though opc_cqual1 tests check for length match, the crupcurs
	    ** test didn't. 
	    */
	    if ((root->pst_sym.pst_dataval.db_datatype == resop->opr_dt ||
		root->pst_sym.pst_dataval.db_datatype == -resop->opr_dt) &&
		resop->opr_offset == OPR_NO_OFFSET ||
		(resop->opr_len == ops[0].opr_len &&
		(resop->opr_prec == ops[0].opr_prec ||
		 abs(resop->opr_dt) != DB_DEC_TYPE) &&
		resop->opr_dt == ops[0].opr_dt))
	    {
		/* If resop->opr_offset isn't preset, just assign ops[0]
		** to resop. Otherwise, this is a case function result and
		** must be copied to the result location. */
		if (resop->opr_offset == OPR_NO_OFFSET)
		    STRUCT_ASSIGN_MACRO(ops[0], *resop);
		else
		{
		    /* Emit MECOPY to copy constant to result location. */
		    STRUCT_ASSIGN_MACRO(ops[0], ops[1]);
		    STRUCT_ASSIGN_MACRO(*resop, ops[0]);
		    opc_adinstr(global, cadf, ADE_MECOPY, ADE_SMAIN, 2, ops, 1);
		}
	    }
	    else
	    {
		/* Otherwise, coerce the actual result to the requested
		** type (now rather than at runtime). Note, we don't set 
		** result attributes if this is a case expression result. 
		*/
		if (resop->opr_offset == OPR_NO_OFFSET)
		    opc_adtransform(global, cadf, &ops[0],
			resop, ADE_SMAIN, (bool)TRUE, (bool)FALSE);
		else 
		    opc_adtransform(global, cadf, &ops[0],
			resop, ADE_SMAIN, (bool)FALSE, (bool)FALSE);
	    }
	}
	break;

     case PST_CURVAL:
	attno = root->pst_sym.pst_value.pst_s_curval.pst_curcol.db_att_id;

	if (cadf->opc_qeadf->qen_output < 0)
	{
	    ops[0].opr_base = cadf->opc_qeadf->qen_uoutput + ADE_ZBASE;
	}
	else
	{
	    ops[0].opr_base = cadf->opc_row_base[cadf->opc_qeadf->qen_output];
	}
	ops[0].opr_offset = rel->rdr_attr[attno]->att_offset;
	ops[0].opr_dt = rel->rdr_attr[attno]->att_type;
	ops[0].opr_prec = rel->rdr_attr[attno]->att_prec;
	ops[0].opr_len = rel->rdr_attr[attno]->att_width;
	ops[0].opr_collID = rel->rdr_attr[attno]->att_collID;

	if (resop != NULL)  
	{
	    /* if the actual and intended result datatypes are the same,
	    ** ignoring nullability (which is why we have the second comparison
	    ** with the negation), then no coercion is needed.
	    **
	    ** Special test is made on opr_offset to distinguish case
	    ** results which may require coercion (not just MECOPY). Even
	    ** though opc_cqual1 tests check for length match, the crupcurs
	    ** test didn't. 
	    */
	    if ((root->pst_sym.pst_dataval.db_datatype == resop->opr_dt ||
		root->pst_sym.pst_dataval.db_datatype == -resop->opr_dt) &&
		resop->opr_offset == OPR_NO_OFFSET ||
		(resop->opr_len == ops[0].opr_len &&
		(resop->opr_prec == ops[0].opr_prec ||
		 abs(resop->opr_dt) != DB_DEC_TYPE) &&
		resop->opr_dt == ops[0].opr_dt))
	    {
		/* If resop->opr_offset isn't preset, just assign ops[0]
		** to resop. Otherwise, this is a case function result and
		** must be copied to the result location. */
		if (resop->opr_offset == OPR_NO_OFFSET)
		    STRUCT_ASSIGN_MACRO(ops[0], *resop);
		else
		{
		    /* Emit MECOPY to copy constant to result location. */
		    STRUCT_ASSIGN_MACRO(ops[0], ops[1]);
		    STRUCT_ASSIGN_MACRO(*resop, ops[0]);
		    opc_adinstr(global, cadf, ADE_MECOPY, ADE_SMAIN, 2, ops, 1);
		}
	    }
	    else
	    {
		/* Otherwise, coerce the actual result to the requested
		** type (now rather than at runtime). Note, we don't set 
		** result attributes if this is a case expression result. 
		*/
		if (resop->opr_offset == OPR_NO_OFFSET)
		    opc_adtransform(global, cadf, &ops[0],
			resop, ADE_SMAIN, (bool)TRUE, (bool)FALSE);
		else 
		    opc_adtransform(global, cadf, &ops[0],
			resop, ADE_SMAIN, (bool)FALSE, (bool)FALSE);
	    }
	}
	break;

     case PST_RULEVAR:
    {
	/*
	** This node represents a reference from a rule list to a before or
	** after value of a row updated by an outer query. At present these
	** references are by attribute numbers for the target table.
	** We assume we have the row number for the before and after images
	** as well as a descriptor for the target table.
	*/
		
	attno = root->pst_sym.pst_value.pst_s_rule.pst_rltno;

	/* If we have a reference to a before value and we have not yet
	** allocated a before row then allocate one.
	*/
	if (root->pst_sym.pst_value.pst_s_rule.pst_rltype == PST_BEFORE &&
	    global->ops_cstate.opc_before_row == -1)
	{
	    QEF_AHD	    *firing_ahd	= global->ops_cstate.opc_rule_ahd;

	    if (firing_ahd->ahd_flags & QEF_STATEMENT_UNIQUE)
	    {
		/* If we have a rule that referes to a before image TID
		** and we have a temptable for handling statement level
		** unique constraints. We need to add space for the TID
		** in the temptable. fixme this comment is garbage now.
		*/
		global->ops_cstate.opc_tempTableRow->opc_olen
		    += DB_TID8_LENGTH;
		if (global->ops_cstate.opc_tempTableRow->opc_olen
		    > global->ops_cb->ops_server->opg_maxtup)
		    /* We don't have room to add space for a tid. */
		    opx_error(E_OP0200_TUPLEOVERFLOW);
		opc_tempTable(global,
			      &firing_ahd->qhd_obj.qhd_qep.
			      u1.s1.ahd_failed_b_temptable,
			      global->ops_cstate.opc_tempTableRow->opc_olen,
			      NULL);
		firing_ahd->qhd_obj.qhd_qep.u1.s1.ahd_b_row =
		    global->ops_cstate.opc_before_row =
			firing_ahd->qhd_obj.qhd_qep.u1.s1.ahd_failed_b_temptable->
			    ttb_tuple;
		firing_ahd->qhd_obj.qhd_qep.u1.s1.ahd_failed_b_temptable->
		    ttb_pagesize = opc_pagesize(global->ops_cb->ops_server,
		     (OPS_WIDTH) global->ops_cstate.opc_tempTableRow->opc_olen);
		opc_adbase(global, cadf, QEN_ROW,
			   global->ops_cstate.opc_before_row);
	    }
	    /* if PUPD & reprow != -1, have both old and new rows.	    */
	    /* if reprow == -1, must make a copy of the old row since   */
	    /* this will be an update in place. */
	    else if (firing_ahd->ahd_atype == QEA_PUPD
		&& firing_ahd->qhd_obj.qhd_qep.ahd_reprow != -1)
	    {
		firing_ahd->qhd_obj.qhd_qep.u1.s1.ahd_b_row =
		    global->ops_cstate.opc_before_row =
			firing_ahd->qhd_obj.qhd_qep.ahd_reprow;
	    }
	    else
	    {
		/* Allocate space for a copy of the old row values */
		opc_ptrow(global, &global->ops_cstate.opc_before_row,
			  firing_ahd->qhd_obj.qhd_qep.ahd_repsize);
		firing_ahd->qhd_obj.qhd_qep.u1.s1.ahd_b_row =
		    global->ops_cstate.opc_before_row;
		opc_adbase(global, cadf, QEN_ROW,
			   global->ops_cstate.opc_before_row);
	    }
	    
	}
	/*
	** TID is a special case. The TID is not stored as part of the record
	** image but stored in a separate DSH_ROW. If this is a reference to
	** TID, make the appropriate adjustments.
	**
	** NOTE: We are continuing to assume that TID is attribute number 0.
	**	 We also assume that TID is represented by an integer.
	*/
	if (attno == DB_IMTID)
	{
	    /*
	    ** Request is for the TID attribute.
	    ** Set the base depending on whether the request is for 
	    ** a BEFORE or AFTER value.
	    */
	    if (root->pst_sym.pst_value.pst_s_rule.pst_rltype == PST_BEFORE)
	    {
		ops[0].opr_base =
		    cadf->opc_row_base[global->ops_cstate.opc_bef_tid];
		if (ops[0].opr_base < ADE_ZBASE || 
		    ops[0].opr_base >= ADE_ZBASE + cadf->opc_qeadf->qen_sz_base
		)
		{
		    opc_adbase(global, cadf, QEN_ROW,
			global->ops_cstate.opc_bef_tid);
		    ops[0].opr_base =
			cadf->opc_row_base[global->ops_cstate.opc_bef_tid];
		}
	    }
	    else
	    {
		ops[0].opr_base =
		    cadf->opc_row_base[global->ops_cstate.opc_aft_tid];
		if (ops[0].opr_base < ADE_ZBASE || 
		    ops[0].opr_base >= ADE_ZBASE + cadf->opc_qeadf->qen_sz_base
		)
		{
		    opc_adbase(global, cadf, QEN_ROW,
			global->ops_cstate.opc_aft_tid);
		    ops[0].opr_base =
			cadf->opc_row_base[global->ops_cstate.opc_aft_tid];
		}
	    }

	    ops[0].opr_offset = 0;
	    ops[0].opr_dt = DB_TID8_TYPE;
	    ops[0].opr_len = DB_TID8_LENGTH;
	    ops[0].opr_prec = 0;
	    ops[0].opr_collID = -1;
	}
	else
	{
	    /*
	    ** Request is for a user attribute (or security label in B1 system).
	    ** Set the base depending on whether the request is for a BEFORE or
	    ** AFTER value.
	    */
	    if (root->pst_sym.pst_value.pst_s_rule.pst_rltype == PST_BEFORE)
	    {
		ops[0].opr_base =
		    cadf->opc_row_base[global->ops_cstate.opc_before_row];
		if (ops[0].opr_base < ADE_ZBASE || 
		    ops[0].opr_base >= ADE_ZBASE + cadf->opc_qeadf->qen_sz_base)
		{
		    opc_adbase(global, cadf, QEN_ROW,
			global->ops_cstate.opc_before_row);
		    ops[0].opr_base =
			cadf->opc_row_base[global->ops_cstate.opc_before_row];
		}
	    }
	    else
	    {
		ops[0].opr_base =
		    cadf->opc_row_base[global->ops_cstate.opc_after_row];
                if (ops[0].opr_base < ADE_ZBASE ||
                    ops[0].opr_base >= ADE_ZBASE + cadf->opc_qeadf->qen_sz_base
                )
                {
                    opc_adbase(global, cadf, QEN_ROW,
			global->ops_cstate.opc_after_row);
		    ops[0].opr_base =
			cadf->opc_row_base[global->ops_cstate.opc_after_row];
		}
	    }

	    /* Set the datatype information for the attribute being requested */
	    ops[0].opr_offset = rel->rdr_attr[attno]->att_offset;
	    ops[0].opr_dt = rel->rdr_attr[attno]->att_type;
	    ops[0].opr_len = rel->rdr_attr[attno]->att_width;
	    ops[0].opr_prec = rel->rdr_attr[attno]->att_prec;
	    ops[0].opr_collID = rel->rdr_attr[attno]->att_collID;
	}

	/*
	** Determine if we have to convert this type to another type. This 
	** will occur if this value is being assigned to a RESDOM of a	   
	** different type or, possibly, when compared to a value of another
	** type.
	*/
	if (resop != NULL)  
	{
	    /*
	    ** If the actual and intended result datatypes are the same,
	    ** ignoring nullability (which is why we have the second comparison
	    ** with the negation), then no coercion is needed.
	    **
	    ** Special test is made on opr_offset to distinguish case
	    ** results which may require coercion (not just MECOPY). Even
	    ** though opc_cqual1 tests check for length match, the crupcurs
	    ** test didn't. 
	    */
	    if ((root->pst_sym.pst_dataval.db_datatype == resop->opr_dt ||
		root->pst_sym.pst_dataval.db_datatype == -resop->opr_dt) &&
		resop->opr_offset == OPR_NO_OFFSET ||
		(resop->opr_len == ops[0].opr_len &&
		(resop->opr_prec == ops[0].opr_prec ||
		 abs(resop->opr_dt) != DB_DEC_TYPE) &&
		resop->opr_dt == ops[0].opr_dt))
	    {
		/* If resop->opr_offset isn't preset, just assign ops[0]
		** to resop. Otherwise, this is a case function result and
		** must be copied to the result location. */
		if (resop->opr_offset == OPR_NO_OFFSET)
		    STRUCT_ASSIGN_MACRO(ops[0], *resop);
		else
		{
		    /* Emit MECOPY to copy constant to result location. */
		    STRUCT_ASSIGN_MACRO(ops[0], ops[1]);
		    STRUCT_ASSIGN_MACRO(*resop, ops[0]);
		    opc_adinstr(global, cadf, ADE_MECOPY, ADE_SMAIN, 2, ops, 1);
		}
	    }
	    else
	    {
		/* Otherwise, coerce the actual result to the requested
		** type (now rather than at runtime). Note, we don't set 
		** result attributes if this is a case expression result. 
		*/
		if (resop->opr_offset == OPR_NO_OFFSET)
		    opc_adtransform(global, cadf, &ops[0],
			resop, ADE_SMAIN, (bool)TRUE, (bool)FALSE);
		else 
		    opc_adtransform(global, cadf, &ops[0],
			resop, ADE_SMAIN, (bool)FALSE, (bool)FALSE);
	    }
	}
	break;
    }
     case PST_RESDOM:
    {
	bool is_byref;
	bool is_output;

	/* first, let's compile the rest of the resdoms */
	opc_crupcurs1(global, root->pst_left, cadf, &ops[0], rel, cnode);

	if (root->pst_sym.pst_value.pst_s_rsdm.pst_rsno == DB_IMTID)
	{
	    /* If this is the tid resdom hack on the target list of a replace
	    ** then let's not do anything since tid's can't be replaced
	    */
	    break;
	}

	is_byref = root->pst_sym.pst_value.pst_s_rsdm.pst_ttargtype ==
	    PST_BYREF_DBPARAMNO;
	is_output = root->pst_sym.pst_value.pst_s_rsdm.pst_ttargtype ==
	    PST_OUTPUT_DBPARAMNO;
	if (is_byref || is_output ||
	    root->pst_sym.pst_value.pst_s_rsdm.pst_ttargtype == PST_DBPARAMNO)
	{
	    OPC_PST_STATEMENT *opc_pst =
		(OPC_PST_STATEMENT *)global->ops_statement->pst_opf;
	    QEF_CP_PARAM *param;
	    PST_QNODE	*right = root->pst_right;
	    QEN_NODE	*qenp = global->ops_cstate.opc_curnode;
	    bool	tproc = FALSE;

	    /* Address QEF_CP_PARAM ptr in either CALLPROC action header
	    ** or TPROC node header. */
	    if (qenp && qenp->qen_type == QE_TPROC)
	    {
		param = &qenp->node_qen.qen_tproc.tproc_params[root->
			    pst_sym.pst_value.pst_s_rsdm.pst_rsno - 1];
		tproc = TRUE;
	    }
	    else param = &opc_pst->opc_stmtahd->qhd_obj.qhd_callproc.
		ahd_params[root->pst_sym.pst_value.pst_s_rsdm.pst_rsno - 1];

	    param->ahd_parm.parm_name = (char *) opu_qsfmem(global, DB_PARM_MAXNAME);
	    MEcopy(root->pst_sym.pst_value.pst_s_rsdm.pst_rsname, DB_PARM_MAXNAME,
		   param->ahd_parm.parm_name);
	    param->ahd_parm.parm_nlen = DB_PARM_MAXNAME;
	    param->ahd_parm.parm_flags = 0;
	    if (is_byref)
		param->ahd_parm.parm_flags = QEF_IS_BYREF;
	    else if (is_output)
		param->ahd_parm.parm_flags = QEF_IS_OUTPUT;

	    /* Check for and flag positional (unnamed) parameters. */
	    if (STncmp(" ", param->ahd_parm.parm_name, 1) == 0)
		param->ahd_parm.parm_flags |= QEF_IS_POSPARM;

	    if (right->pst_sym.pst_type == PST_CONST &&
		right->pst_sym.pst_value.pst_s_cnst.pst_tparmtype ==
		PST_LOCALVARNO)
	    {
		DB_DATA_VALUE *var_info;

		/* If this is a resdom representing the passing of a local
		** variable to a procedure don't generate code to copy the
		** variable, simply pass a discription and pointer to it in
		** the ahd_params array for the associated callproc action.
		** This is critical if we are passing a parameter by reference.
		** QEF needs to be able to update the actual variable - not
		** a copy.
		*/
		opc_lvar_info(global,
			      right->pst_sym.pst_value.pst_s_cnst.pst_parm_no,
			      &param->ahd_parm_row,
			      &param->ahd_parm_offset,
			      &var_info);
		STRUCT_ASSIGN_MACRO(*var_info, param->ahd_parm.parm_dbv);
			      
		break; /* We are done with this node */
	    }
	    ops[1].opr_len = root->pst_sym.pst_dataval.db_length;
	}
	/* Now let's do this resdom */
	ops[1].opr_dt = root->pst_sym.pst_dataval.db_datatype;
	ops[1].opr_offset = OPR_NO_OFFSET;
	opc_crupcurs1(global, root->pst_right, cadf, &ops[1], rel, cnode);
	opc_rdmove(global, root, ops, cadf, ADE_SMAIN, rel, FALSE);
	if (resop)
	    resop->opr_offset = ops[0].opr_offset + ops[0].opr_len;
	break;
    }

    case PST_VAR:
    {
	/* attno, zatt, eqcno and func_tree are for compiling var nodes. */
	OPS_SUBQUERY	    *subqry = global->ops_cstate.opc_subqry;
	i2		    attno;
	OPZ_ATTS	    *zatt;
	OPE_IEQCLS	    eqcno;
	PST_QNODE	    *func_tree;
	QEN_NODE	    *qenp = global->ops_cstate.opc_curnode;

	/* opc_crupcurs() wasn't intended to handle PST_VARs. So only
	** proceed if this is the parameter building CX for a table
	** procedure. */
	if (!qenp || qenp->qen_type != QE_TPROC)
	    opx_error(E_OP0681_UNKNOWN_NODE);

	/* First, find the EQ class that this var node belongs to. */
	attno = root->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id;
	zatt = subqry->ops_attrs.opz_base->opz_attnums[attno];
	eqcno = zatt->opz_equcls;

	/* Now find the attribute that we have for this EQ class. Because
	** of the way that OPF and EQ classes work, the attribute that
	** this var node refers to and the att that we have may not be
	** the same. They will be equal however. Because of this,
	** we do this dereferencing step through the EQ class and the
	** available attributes for that eq class to find the data.
	*/
	if (cnode->opc_ceq[eqcno].opc_eqavailable == FALSE)
	{
	    opx_error(E_OP0889_EQ_NOT_HERE);
	}
	else
	{
	    attno = cnode->opc_ceq[eqcno].opc_attno;
	}

	if (attno != OPC_NOATT)
	{
	    zatt = subqry->ops_attrs.opz_base->opz_attnums[attno];

	    /* if this is a function attribute that hasn't been evaluated yet */
	    if (zatt->opz_attnm.db_att_id == OPZ_FANUM &&
			cnode->opc_ceq[zatt->opz_equcls].opc_attavailable == FALSE
		)
	    {
		/* then call opc_cqual1 recursively to evaluate this var node */
		func_tree = subqry->ops_funcs.opz_fbase->
			    opz_fatts[zatt->opz_func_att]->opz_function->pst_right;
		(VOID) opc_cqual1(global, cnode, func_tree, cadf, ADE_SMAIN,
			    resop, (ADI_FI_ID)0, FALSE);
		global->ops_cstate.opc_qp->qp_status |= QEQP_NEED_TRANSACTION;
		break;
	    }
	    else
	    {
		/* otherwise, check that we have the attribute in question at
		** hand, and do the setup to return the att info.
		*/
		if (cnode->opc_ceq[zatt->opz_equcls].opc_attavailable == FALSE)
		{
		    opx_error(E_OP0888_ATT_NOT_HERE);
		}
	    }
	}

	/* All the rest is common to OPC_NOATT and !OPC_NOATT cases. */
	{
	    if (attno == OPC_NOATT && 
			cnode->opc_ceq[eqcno].opc_attavailable == FALSE)
	    {
		/* Again, verify availability of attr. */
		opx_error(E_OP0888_ATT_NOT_HERE);
	    }

	    /* Fill in ops[0] for the attribute */
	    ops[0].opr_dt = cnode->opc_ceq[eqcno].opc_eqcdv.db_datatype;
	    ops[0].opr_prec = cnode->opc_ceq[eqcno].opc_eqcdv.db_prec;
	    ops[0].opr_len = cnode->opc_ceq[eqcno].opc_eqcdv.db_length;
	    ops[0].opr_collID = cnode->opc_ceq[eqcno].opc_eqcdv.db_collID;
	    ops[0].opr_offset = cnode->opc_ceq[eqcno].opc_dshoffset;
	    ops[0].opr_base = 
			cadf->opc_row_base[cnode->opc_ceq[eqcno].opc_dshrow];
	    if (ops[0].opr_base < ADE_ZBASE || 
		    ops[0].opr_base >= ADE_ZBASE + cadf->opc_qeadf->qen_sz_base
		)
	    {
		/* if the CX base that we need hasn't already been added
		** to this CX then add it now.
		*/
		opc_adbase(global, cadf, QEN_ROW, 
				    cnode->opc_ceq[eqcno].opc_dshrow);
		ops[0].opr_base = 
			cadf->opc_row_base[cnode->opc_ceq[eqcno].opc_dshrow];
	    }

	    /* if the actual and intended result datatypes are the same,
	    ** ignoring nullability (which is why we have the second comparison
	    ** with the negation), then no coercion is needed.
	    */
	    if ((resop->opr_dt == ops[0].opr_dt || 
		 resop->opr_dt == -ops[0].opr_dt
		) &&
		(resop->opr_len == OPC_UNKNOWN_LEN ||
		 (resop->opr_len == ops[0].opr_len &&
		  (resop->opr_prec == ops[0].opr_prec ||
		   abs(ops[0].opr_dt) != DB_DEC_TYPE
		  ) &&
		  resop->opr_dt == ops[0].opr_dt
		 )
		)
	       )
	    {
		/* If resop->opr_offset isn't preset, just assign ops[0]
		** to resop. Otherwise, this is a case function result and
		** must be copied to the result location. */
		if (resop->opr_offset == OPR_NO_OFFSET)
		    STRUCT_ASSIGN_MACRO(ops[0], *resop);
		else
		{
		    /* Emit MECOPY to copy VAR value to result location. */
		    STRUCT_ASSIGN_MACRO(ops[0], ops[1]);
		    STRUCT_ASSIGN_MACRO(*resop, ops[0]);
		    opc_adinstr(global, cadf, ADE_MECOPY, ADE_SMAIN, 2, ops, 1);
		}
	    }
	    else
	    {
		/* otherwise coerce the attribute to the requested type.
		** Note we only set result attributes if this is NOT
		** a case function result.  */
		opc_adtransform(global, cadf, &ops[0], resop,
		    ADE_SMAIN, (bool)(resop->opr_offset == OPR_NO_OFFSET),
			(bool)FALSE);
	    }
	}
	global->ops_cstate.opc_qp->qp_status |= QEQP_NEED_TRANSACTION;
	break;
     }

     case PST_QLEND:
	break;

     case PST_TREE:
	/* set the offset to zero for the parent resdom. */
	resop->opr_offset = 0;
	break;

     default:
	opx_error(E_OP0681_UNKNOWN_NODE);
    }
}

/*{
** Name: OPC_CPARMRET	- compile a CX to return OUT, INOUT parameters
**	for a rule invocation.
**
** Description:
**      This routine is called during the building of the QEA_CALLPROC
**	action header used to implement a rule invocation. With "before"
**	triggers, the rule may return values to overlay those in the current
**	row buffer. OUT/INOUT formal parameter support gets the values into
**	the actual parameter buffers, but not back to the row buffers to
**	which they must be copied. This function uses the RESDOM/RULEVAR pairs
**	for the rule invocation to generate a CX that gets the values the
**	rest of the way back.
**
** Inputs:
**	global -
**	    the query wide state variable for OPF/OPC
**	root -
**	    the root of the assignment to be compiled.
**	cadf -
**	    The OPC version of the open CX.
**	resop -
**	    a pointer to an operand that holds the desired result type but
**	    is otherwise uninitialized.
**	rel -
**	    the result relation description for this query.
**
** Outputs:
**	cadf -
**	    This will hold the ADF instructions that are needed for the
**	    expression evalutation.
**	resop -
**	    holds a description of the expression.
**
**	Returns:
**	    TRUE	- if copy code was generated
**	    FALSE	- if no code was generated (all the actual 
**			  parameters are expressions, so can't be INOUT/OUT)
**	Exceptions:
**	    none, except those that opx_error() raises.
**
** Side Effects:
**	    none
**
** History:
**	15-june-06 (dougi)
**	    Written to compile CX that copies OUT/INOUT parm values back
**	    to row buffers of calling rule invocations.
*/
bool
opc_cparmret(
		OPS_STATE	*global,
		PST_QNODE	*root,
		OPC_ADF		*cadf,
		ADE_OPERAND	*resop,
		RDR_INFO	*rel)

{
    PST_QNODE	*rsdmp;
    PST_QNODE	*rulevp;
    i4		attno;
    ADE_OPERAND	ops[2];
    OPC_PST_STATEMENT	*opc_pst = (OPC_PST_STATEMENT *)global->
						ops_statement->pst_opf;
    QEF_CP_PARAM	*param;
    bool	retval = FALSE;


    /* Loop over RESDOM list looking for rhs' that are simple PST_RULEVARs.
    ** These will be compiled as assignments from the actual parameter 
    ** buffer back to the row buffer from which the value originated. If
    ** the rhs isn't a PST_RULEVAR, it is an expression and cannot be a 
    ** INOUT/OUT parameter. */

    for (rsdmp = root; rsdmp && 
	rsdmp->pst_sym.pst_type == PST_RESDOM; rsdmp = rsdmp -> pst_left)
    {
	if ((rulevp = rsdmp->pst_right) == (PST_QNODE *) NULL ||
	    rulevp->pst_sym.pst_type != PST_RULEVAR ||
	    (rulevp->pst_sym.pst_value.pst_s_rule.pst_rltargtype == PST_ATTNO &&
		rulevp->pst_sym.pst_value.pst_s_rule.pst_rltno == 0))
	    continue;			/* skip, unless it's a RULEVAR for a 
					** non-TID parameter */

	retval = TRUE;

	/* Set up result buffer. */
	if (rulevp->pst_sym.pst_value.pst_s_rule.pst_rltype == PST_BEFORE)
	{
	    ops[0].opr_base =
		    cadf->opc_row_base[global->ops_cstate.opc_before_row];
	    if (ops[0].opr_base < ADE_ZBASE || 
		ops[0].opr_base >= ADE_ZBASE + cadf->opc_qeadf->qen_sz_base)
	    {
		opc_adbase(global, cadf, QEN_ROW,
			global->ops_cstate.opc_before_row);
		ops[0].opr_base =
			cadf->opc_row_base[global->ops_cstate.opc_before_row];
	    }
	}
	else if (rulevp->pst_sym.pst_value.pst_s_rule.pst_rltype == PST_AFTER)
	{
	    ops[0].opr_base =
		    cadf->opc_row_base[global->ops_cstate.opc_after_row];
	    if (ops[0].opr_base < ADE_ZBASE || 
		ops[0].opr_base >= ADE_ZBASE + cadf->opc_qeadf->qen_sz_base)
	    {
		opc_adbase(global, cadf, QEN_ROW,
			global->ops_cstate.opc_after_row);
		ops[0].opr_base =
			cadf->opc_row_base[global->ops_cstate.opc_after_row];
	    }
	}

	/* Set the datatype information for the attribute being requested */
	attno = rulevp->pst_sym.pst_value.pst_s_rule.pst_rltno;
	ops[0].opr_offset = rel->rdr_attr[attno]->att_offset;
	ops[0].opr_dt = rel->rdr_attr[attno]->att_type;
	ops[0].opr_len = rel->rdr_attr[attno]->att_width;
	ops[0].opr_prec = rel->rdr_attr[attno]->att_prec;
	ops[0].opr_collID = rel->rdr_attr[attno]->att_collID;

	/* Now access actual parameter information. */
	STRUCT_ASSIGN_MACRO(ops[0], ops[1]);
	param = &opc_pst->opc_stmtahd->qhd_obj.qhd_callproc.
		ahd_params[rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsno - 1];
	ops[1].opr_offset = param->ahd_parm_offset;
	ops[1].opr_base = cadf->opc_row_base[param->ahd_parm_row];
	if (ops[1].opr_base < ADE_ZBASE || 
		ops[1].opr_base >= ADE_ZBASE + cadf->opc_qeadf->qen_sz_base)
	{
	    opc_adbase(global, cadf, QEN_ROW, param->ahd_parm_row);
	    ops[1].opr_base = cadf->opc_row_base[param->ahd_parm_row];
	}
	opc_adinstr(global, cadf, ADE_MECOPY, ADE_SMAIN, 2, ops, 1);

    }
    return(retval);
}

/*{
** Name: OPC_LVAR_INFO	- Returns information about local variable.
**
** Description:
**      This routine finds in the DSH row related info
**      for a DB procedure local variables. 
[@comment_line@]...
**
** Inputs:
**	global -
**	    query wide state info
**	lvarno -
**	    local variable number
**	rowno -
**	    pointer to rowno to be filled in.
**	offset -
**	    pointer to offset to be filled in.
**	var_info -
**	    a pointer to row info pointer be filled in.
**	
**
** Outputs:
**	rowno -
**	    will be filled in with rowno for lvar
**	offset -
**	    will be filled in with offset for lvar
**	cadf -
**	    will be filled in with pointer to description of lvazr.
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
**      12-nov-92 (jhahn)
**          created (from opc_lvar_row)
[@history_template@]...
*/
VOID
opc_lvar_info(
	OPS_STATE	*global,
	i4		lvarno,
	i4		*rowno,
	i4		*offset,
	DB_DATA_VALUE	**row_info)
{
    PST_STATEMENT	*decvar_stmt;
    i4			adjusted_varno;
    OPC_PST_STATEMENT	*opc_pst;
    PST_DECVAR		*decvar;
    i4			first_rowno;

    decvar = global->ops_procedure->pst_parms;
    if (lvarno >= decvar->pst_first_varno &&
	    lvarno < decvar->pst_first_varno + decvar->pst_nvars
	)
    {
	first_rowno = global->ops_cstate.opc_pvrow_dbp;
    }
    else
    {
	for (decvar_stmt = global->ops_cstate.opc_topdecvar;
		decvar_stmt != NULL;
	        decvar_stmt = opc_pst->opc_ndecvar
	    )
	{
	    opc_pst = (OPC_PST_STATEMENT *) decvar_stmt->pst_opf;
	    decvar = decvar_stmt->pst_specific.pst_dbpvar;

	    if (lvarno >= decvar->pst_first_varno &&
		    lvarno < decvar->pst_first_varno + decvar->pst_nvars
		)
	    {
		first_rowno = opc_pst->opc_lvrow;
		break;
	    }
	}

	if (decvar_stmt == NULL)
	{
	    opx_error(E_OP0898_ILL_LVARNO);
	}
    }
    adjusted_varno = lvarno - decvar->pst_first_varno;

    *rowno = adjusted_varno + first_rowno;
    *offset = 0;
    *row_info = &decvar->pst_vardef[adjusted_varno];
}

/*{
** Name: OPC_LVAR_ROW	- Fill in the DSH row info for a local var
**
** Description:
**      This routine fills in the DSH row related info into an ADF_OPERAND 
**      for a DB procedure local variables. 
[@comment_line@]...
**
** Inputs:
**	global -
**	    query wide state info
**	lvarno -
**	    local variable number
**	cadf -
**	    The OPC adf struct for the current CX.
**	ops -
**	    a pointer to the operand to be filled in.
**	
**
** Outputs:
**	ops -
**	    the operand will be filled in with the local var info
**	cadf -
**	    a base array element may be added for the local vars row.
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
**      28-may-88 (eric)
**          created
**      9-nov-89 (ERIC)
**          Added support for multiple DECVAR statements in a single DB
**          procedure. This means that we first check the DBP procedure
**          parameters decvar structure, then check the linked list of
**          DECVAR statements for the local variable in question.
**	12-Nov-92 (jhahn)
**	    Split off part that finds row info into opc_lvar_info.
**	12-dec-2007 (dougi)
**	    Support NULL cadf parm for first/offset "n" support.
*/
VOID
opc_lvar_row(
		OPS_STATE	*global,
		i4		lvarno,
		OPC_ADF		*cadf,
		ADE_OPERAND	*ops)
{
    i4			rowno, offset;
    DB_DATA_VALUE	*row_info;

    opc_lvar_info(global, lvarno, &rowno, &offset, &row_info);

    ops[0].opr_dt = row_info->db_datatype;
    ops[0].opr_prec = row_info->db_prec;
    ops[0].opr_len = row_info->db_length;
    ops[0].opr_collID = row_info->db_collID;
    ops[0].opr_offset = offset;
    if (cadf == (OPC_ADF *) NULL)
	ops[0].opr_base = rowno;
    else
    {
	ops[0].opr_base = cadf->opc_row_base[rowno];

	if (ops[0].opr_base < ADE_ZBASE || 
	    ops[0].opr_base >= ADE_ZBASE + cadf->opc_qeadf->qen_sz_base)
	{
	    opc_adbase(global, cadf, QEN_ROW, rowno);
	    ops[0].opr_base = cadf->opc_row_base[rowno];
	}
    }
}


/*{
** Name: OPC_SEQ_SETUP	- Locate existing sequence descriptor or build 
**	new one.
**
** Description:
**      This routine attempts to locate a QEF_SEQUENCE structure for a 
**	supplied sequence operator. If found, it is returned; otherwise,
**	a new one is constructed and added to the QP chain.
**
** Inputs:
**	global -
**	    query wide state info
**	cadf -
**	    The OPC adf struct for the current CX.
**	root -
**	    a pointer to the sequence operator parse tree node
**	
**
** Outputs:
**	seqp -
**	    a pointer to the QEF_SEQUENCE structure located or built here
**	localbuf -
**	    the ADF CX buffer number of the buffer in which the sequence
**	    value will be returned
**	cadf -
**	    a base array element may be added for the local vars row.
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
**      15-mar-02 (inkdo01)
**	    Written for sequence support.
**	25-mar-04 (inkdo01)
**	    Fix for 111990 - forgot qs_ahdnext linkage for mult. seqs.
**	8-june-05 (inkdo01)
**	    Can't share sequence descriptors if one is for nextval and the
**	    other is for currval (bug 114646).
**	30-Apr-2010 (kiria01) b123665
**	    Attach the sequence to the correct subquery - instead of using
**	    ops_osubquery use global->ops_cstate.opc_subqry otherwise the
**	    wrong ahd_seq is updated.
*/
static VOID
opc_seq_setup(
	OPS_STATE	*global,
	OPC_NODE	*cnode,
	OPC_ADF		*cadf,
	PST_QNODE	*root,
	QEF_SEQUENCE	**seqp,
	i4		*localbuf)

{
    /* standard variables to shorten line length */
    OPS_SUBQUERY	*subqry = global->ops_cstate.opc_subqry;
    QEF_AHD		*ahd = subqry->ops_compile.opc_ahd;
    QEF_SEQUENCE	*wseqp;
    QEF_SEQ_LINK	*linkp;
    PST_SEQOP_NODE	*nodep = &root->pst_sym.pst_value.pst_s_seqop;
    i4			dshrow;

    /* First look for the desired sequence in the current action header. */
    for (linkp = ahd->qhd_obj.qhd_qep.ahd_seq; linkp; linkp = linkp->qs_ahdnext)
    {
	wseqp = linkp->qs_qsptr;
	if (MEcmp((PTR)&wseqp->qs_seqname, (PTR)&nodep->pst_seqname, 
		DB_SEQ_MAXNAME) == 0 &&
	    MEcmp((PTR)&wseqp->qs_owner, (PTR)&nodep->pst_owner, 
		DB_OWN_MAXNAME) == 0 &&
	    (nodep->pst_seqflag & PST_SEQOP_NEXT && wseqp->qs_flag & QS_NEXTVAL ||
	    nodep->pst_seqflag & PST_SEQOP_CURR && wseqp->qs_flag == 0))
	    break;		/* got a match */
    }

    if (linkp)
    {
	/* We've got one and the operator matches (currval or nextval). */
	*seqp = wseqp;
	*localbuf = cadf->opc_row_base[wseqp->qs_rownum];
	return;
    }

    /* Not already in action header, maybe it's in another in our plan. */
    for (wseqp = global->ops_cstate.opc_qp->qp_sequences; wseqp;
		wseqp = wseqp->qs_qpnext)
    {
	if (MEcmp((PTR)&wseqp->qs_seqname, (PTR)&nodep->pst_seqname, 
		DB_SEQ_MAXNAME) == 0 &&
	    MEcmp((PTR)&wseqp->qs_owner, (PTR)&nodep->pst_owner, 
		DB_OWN_MAXNAME) == 0 &&
	    (nodep->pst_seqflag & PST_SEQOP_NEXT && wseqp->qs_flag & QS_NEXTVAL ||
	    nodep->pst_seqflag & PST_SEQOP_CURR && wseqp->qs_flag == 0))
	    break;		/* got a match */
    }
	
    /* If not found, get new dsh row buffer and in any event, we need
    ** to map the dsh row to our ADF CX. */
    if (wseqp == NULL)
	opc_ptrow(global, &dshrow, root->pst_sym.pst_dataval.db_length);
    else dshrow = wseqp->qs_rownum;

    opc_adbase(global, cadf, QEN_ROW, dshrow);
    *localbuf = cadf->opc_row_base[dshrow];

    /* If there is a sequence, add to action header list and return
    ** the right stuff to caller. */
    if (wseqp)
    {
	/* We've got one and the operator matches (currval or nextval). */
	*seqp = wseqp;
	
	linkp = (QEF_SEQ_LINK *) opu_qsfmem(global, sizeof(QEF_SEQ_LINK));
	linkp->qs_qsptr = wseqp;
	linkp->qs_ahdnext = ahd->qhd_obj.qhd_qep.ahd_seq;
	ahd->qhd_obj.qhd_qep.ahd_seq = linkp;
	return;
    }

    /* If we get here, there was no sequence definition anywhere. Make
    ** one and attach it to the QP header and the current action header. */
    linkp = (QEF_SEQ_LINK *) opu_qsfmem(global, (sizeof(QEF_SEQ_LINK) +
				sizeof(QEF_SEQUENCE)));
    wseqp = (QEF_SEQUENCE *)((char *)linkp + sizeof(QEF_SEQ_LINK));
    linkp->qs_qsptr = wseqp;
    if (nodep->pst_seqflag & PST_SEQOP_CURR)
    {
	/* Tedious but necessary - currval's must follow nextval's. */
	QEF_SEQ_LINK	**linkpp;

	for (linkpp = &ahd->qhd_obj.qhd_qep.ahd_seq; *linkpp; 
		linkpp = &(*linkpp)->qs_ahdnext);
	(*linkpp) = linkp;
    }
    else
    {
	linkp->qs_ahdnext = ahd->qhd_obj.qhd_qep.ahd_seq;
	ahd->qhd_obj.qhd_qep.ahd_seq = linkp;
    }
    wseqp->qs_qpnext = global->ops_cstate.opc_qp->qp_sequences;
    global->ops_cstate.opc_qp->qp_sequences = wseqp;

    /* Copy all the appropriate values to the new sequence descriptor. */
    STRUCT_ASSIGN_MACRO(nodep->pst_seqname, wseqp->qs_seqname);
    STRUCT_ASSIGN_MACRO(nodep->pst_owner, wseqp->qs_owner);
    STRUCT_ASSIGN_MACRO(nodep->pst_seqid, wseqp->qs_id);
    wseqp->qs_rownum = dshrow;
    opc_ptcb(global, &wseqp->qs_cbnum, sizeof(DMS_CB));
    if (nodep->pst_seqflag & PST_SEQOP_NEXT)
	wseqp->qs_flag |= QS_NEXTVAL;

    *seqp = wseqp;
}


/*{
** Name: OPC_EXPROPT	- optimize expression(s) contained in a parse tree
**	fragment.
**
** Description:
**      This function is the root of a sequence of functions that 
**	recursively analyze a parse tree fragment for common subexpressions.
**	The subexpressions are removed to be evaluated once and replaced
**	by special PST_VAR nodes that reference the subexpression results.
**
** Inputs:
**	subquery	- OPS_SUBQUERY state structure ptr
**
**	cnode		- information about current node/action header
**
**	rootp		- ptr to ptr to root of parse tree fragment
**			  (to allow subexprs to be replaced by PST_VARs)
**	
**	subexlistp	- ptr to chain of RESDOMs defining the extracted
**			  subexpressions (NULL to start)
**
** Outputs:
**	rootp		- ptr to (possibly) modified parse tree fragment
**
**	subexlistp	- ptr to chain of RESDOMs defining the extracted
**			  subexpressions
**
**	Returns:
**	    TRUE	- if at least one subexpression was extracted (and
**			  must therefore be compiled into the CX)
**
**	    FALSE	- otherwise
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      28-june-05 (inkdo01)
**	    Written for CX optimization feature.
**	22-mar-10 (smeke01) b123457
**	    Added trace point op214 calls to opu_qtprint.
*/

bool
opc_expropt(
	OPS_SUBQUERY	*subquery,
	OPC_NODE	*cnode,
	PST_QNODE	**rootp,
	PST_QNODE	**subexlistp)

{
    i4		subex_count = 0;
    i4		i;
    bool	dummy;


    /* Init. offset vector, then call opc_expranal() to perform
    ** expression optimization. */
    for (i = 0; i < 200; i++)
	cnode->opc_subex_offset[i] = 0;

    dummy = opc_expranal(subquery, cnode, (*rootp), rootp, 
		subexlistp, &subex_count);

    if (FALSE && subex_count)
    {
	OPS_SUBQUERY	*oldsq;
	oldsq = subquery->ops_global->ops_subquery;
	subquery->ops_global->ops_subquery = NULL;
# ifdef OPT_F086_DUMP_QTREE2
	if (subquery->ops_global->ops_cb->ops_check &&
	    opt_strace(subquery->ops_global->ops_cb, OPT_F086_DUMP_QTREE2))
	{	/* op214 - dump parse tree in op146 style */
	    opu_qtprint(subquery->ops_global, *subexlistp, "extracted subexpressions");
	}
# endif
	opt_pt_entry(subquery->ops_global, NULL, *subexlistp,
		"extracted subexpressions");
# ifdef OPT_F086_DUMP_QTREE2
	if (subquery->ops_global->ops_cb->ops_check &&
	    opt_strace(subquery->ops_global->ops_cb, OPT_F086_DUMP_QTREE2))
	{	/* op214 - dump parse tree in op146 style */
	    opu_qtprint(subquery->ops_global, *rootp, "transformed tree fragment");
	}
# endif
	opt_pt_entry(subquery->ops_global, NULL, *rootp,
		"transformed tree fragment");
	subquery->ops_global->ops_subquery = oldsq;
    }
}


/*{
** Name: OPC_EXPRANAL	- recursively analyze parse tree fragments.
**
** Description:
**      This function recursively descends a parse tree fragment looking
**	for nodes that might anchor common subexpressions. A search is 
**	performed with such nodes to find the same expression elsewhere
**	in the fragment.
**
** Inputs:
**	subquery	- OPS_SUBQUERY state structure ptr
**
**	cnode		- information about current node/action header
**
**	root		- ptr to root of parse tree fragment to optimize
**
**	nodepp		- ptr to ptr to current node under analysis
**	
**	subexlistp	- ptr to chain of RESDOMs defining the extracted
**			  subexpressions (NULL to start)
**
** Outputs:
**	root		- ptr to (possibly) modified parse tree fragment
**
**	subexlistp	- ptr to chain of RESDOMs defining the extracted
**			  subexpressions
**
**	Returns:
**	    TRUE	- if current node has continued optimization 
**			  potential (i.e., is a leaf node that might 
**			  be part of a common subexpression)
**
**	    FALSE	- otherwise
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      28-june-05 (inkdo01)
**	    Written for CX optimization feature.
*/

static bool
opc_expranal(
	OPS_SUBQUERY	*subquery,
	OPC_NODE	*cnode,
	PST_QNODE	*rootp,
	PST_QNODE	**nodepp,
	PST_QNODE	**subexlistp,
	i4		*subex_countp)

{
    PST_QNODE	*nodep = (*nodepp);
    PST_QNODE	*leftop, *rightop;
    bool	dummy;


    /* Recursively descend the parse tree fragment anchored in nodep.
    ** If unoptimizeable nodes are encountered, return FALSE. If
    ** potentially useful nodes are encountered (e.g. PST_VAR, 
    ** PST_CONST, etc.), return TRUE. If we're on a UOP, BOP, AOP
    ** with acceptible operands, call opc_exprsrch() to look for
    ** another occurrence of the same subexpression. Otherwise,
    ** return FALSE.
    */

    switch (nodep->pst_sym.pst_type) {

      case PST_AOP:
	/* Check first for count(*). */
	if (nodep->pst_sym.pst_value.pst_s_op.pst_opno == ADI_CNTAL_OP)
	    return(FALSE);	/* 'til I figure out how to handle it */
      case PST_BOP:
      case PST_UOP:
      case PST_AND:
      case PST_OR:
      case PST_NOT:

	/* Recurse down the fragment and analyze upon return. */
	if (!opc_expranal(subquery, cnode, rootp, &nodep->pst_left, 
		subexlistp, subex_countp))
	    return(FALSE);

	if (nodep->pst_right && !opc_expranal(subquery, cnode, rootp,
		&nodep->pst_right, subexlistp, subex_countp))
	    return(FALSE);

	/* If current node is PST_BOP, PST_UOP or PST_AOP, search
	** for another. Otherwise, return FALSE. */
	if (nodep->pst_sym.pst_type == PST_BOP ||
	    nodep->pst_sym.pst_type == PST_UOP ||
	    nodep->pst_sym.pst_type == PST_AOP)
	{
	    PST_QNODE	**workpp;
	    bool	gotone = FALSE;
	    DB_ATT_ID	attrid;

	    /* Look for matches. If any were found, replacement of
	    ** matches by PST_VAR will have been done. However, here
	    ** we must add the subexpr to the subexpr list and replace
	    ** it by a PST_VAR. */
	    dummy = opc_exprsrch(subquery, cnode, nodepp, &rootp,
		subexlistp, subex_countp, &gotone);

	    if (gotone)
	    {
		attrid.db_att_id = *subex_countp - 1;
		for (workpp = subexlistp; (*workpp); 
			workpp = &(*workpp)->pst_left);
				/* find insert point in RESDOM list */
		/* Generate RESDOM to anchor subexpression and mark it
		** as subexpression result c/w ordinal number. */
		*workpp = opv_resdom(subquery->ops_global, nodep, 
			&nodep->pst_sym.pst_dataval);
		(*workpp)->pst_sym.pst_value.pst_s_rsdm.
					pst_ttargtype = PST_SUBEX_RSLT;
		(*workpp)->pst_sym.pst_value.pst_s_rsdm.
					pst_ntargno = attrid.db_att_id;
		/* Replace original subexpression with PST_VAR. */
		*nodepp = opv_varnode(subquery->ops_global, 
			&nodep->pst_sym.pst_dataval, OPV_NOVAR,
			&attrid);
	    }
	    return(gotone);
	}
	else return(FALSE);

      case PST_VAR:
      case PST_CONST:
      case PST_COP:
      case PST_QLEND:
      case PST_TREE:
      case PST_SEQOP:
	/* These are all ok. */
	return(TRUE);

      case PST_RESDOM:
	/* RESDOMs differ in that they separate distinct expressions
	** for analysis. Each one can be independently optimized. A
	** failure to optimize one just requires skipping to the next. */
	if (nodep->pst_right)
	    dummy = opc_expranal(subquery, cnode, rootp, &nodep->pst_right,
		subexlistp, subex_countp);

	if (nodep->pst_left)
	    dummy = opc_expranal(subquery, cnode, rootp, &nodep->pst_left,
		subexlistp, subex_countp);

	return(TRUE);

      default:
	/* All the rest just return FALSE. */
	return(FALSE);
    }  /* end of switch */

}


/*{
** Name: OPC_EXPRSRCH	- recursively analyze parse tree fragments looking
**	for match on subexpression.
**
** Description:
**      This function recursively descends a parse tree fragment looking
**	for nodes that match the subexpression being sought. If one is 
**	found, a transformation is performed to save the subexpression
**	under a special RESDOM and replace it by PST_VARs representing
**	the result of the subexpression.
**
** Inputs:
**	subquery	- OPS_SUBQUERY state structure ptr
**
**	cnode		- information about current node/action header
**
**	subexpp		- ptr to ptr to subexpression being sought
**
**	nodepp		- ptr to ptr to current node under analysis
**	
**	subexlistp	- ptr to chain of RESDOMs defining the extracted
**			  subexpressions (NULL to start)
**
**	gotone		- ptr to bool that indicates whether match 
**			  was found
**
** Outputs:
**	subexlistp	- ptr to chain of RESDOMs defining the extracted
**			  subexpressions
**
**	gotone		- TRUE if a match was found, else FALSE
**
**	Returns:
**	    TRUE	- if current node has continued optimization 
**			  potential (i.e., is a leaf node that might 
**			  be part of a common subexpression)
**
**	    FALSE	- otherwise
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      28-june-05 (inkdo01)
**	    Written for CX optimization feature.
**	24-march-2008 (dougi)
**	    Don't optimize away multiple executions of random(), uuid().
**	10-sep-2008 (gupsh01)
**	    Don't optimize unorm operator.
**	26-Apr-2010 (kschendel) b123636
**	    + (concat) isn't commutative if it's string concatenation.
**	    Don't optimize blobs, see comments inline.
**	25-May-2010 (kiria01) b123805
**	    Don't treat as the same two sub-expressions that differ in their
**	    result type, length, precision or collation ID. This will speed up
**	    most checks as well as sifting out a genuine mis-match that would
**	    otherwise slip through.
*/

static bool
opc_exprsrch(
	OPS_SUBQUERY	*subquery,
	OPC_NODE	*cnode,
	PST_QNODE	**subexpp,
	PST_QNODE	**nodepp,
	PST_QNODE	**subexlistp,
	i4		*subex_countp, 
	bool		*gotone)

{
    PST_QNODE	*nodep = (*nodepp);
    PST_QNODE	*srchp = (*subexpp);
    PST_QNODE	*leftop, *rightop;
    ADI_OP_ID	opno;
    DB_ATT_ID	attrid;
    DB_DT_ID	dt;
    i4		dtbits;
    DB_STATUS	status;
    bool	dummy, comm;


    /* Recursively descend the parse tree fragment anchored in nodep.
    ** If unoptimizeable nodes are encountered, return FALSE. If
    ** potentially useful nodes are encountered (e.g. PST_VAR, 
    ** PST_CONST, etc.), return TRUE. If we're on a UOP, BOP, AOP
    ** with acceptible operands, start checking if it matches the
    ** target subexpression.
    */
    if (nodep == srchp)
	return(FALSE);		/* skip the "identity" comparison */

    if (*subex_countp >= 200)
	return(FALSE);		/* unlikely to hit this limit */

    switch (nodep->pst_sym.pst_type) {

      case PST_AOP:
	/* Check first for count(*). */
	if (nodep->pst_sym.pst_value.pst_s_op.pst_opno == ADI_CNTAL_OP)
	    return(FALSE);	/* 'til I figure out how to handle it */
      case PST_BOP:
      case PST_UOP:
      case PST_AND:
      case PST_OR:
      case PST_NOT:

	/* Recurse down the fragment and analyze upon return. */
	if (!opc_exprsrch(subquery, cnode, subexpp, &nodep->pst_left, 
		subexlistp, subex_countp, gotone))
	    return(FALSE);

	if (nodep->pst_right && !opc_exprsrch(subquery, cnode, subexpp,
		&nodep->pst_right, subexlistp, subex_countp, gotone))
	    return(FALSE);

	/* Check if current node is same type (and pst_opno) as
	** target subexpression. */
	if (srchp->pst_sym.pst_type != nodep->pst_sym.pst_type ||
	    (opno = srchp->pst_sym.pst_value.pst_s_op.pst_opno) != 
		nodep->pst_sym.pst_value.pst_s_op.pst_opno)
	    return(FALSE);

	/* Also check for identical result datatypes, lengths and
	** precisions. Partly as this will often summarize mismatched
	** sub-expression but specifically needed if an explicit coercion
	** has used that pinned the result type. Usually, the result types
	** will follow from the operands and operator but cast operators
	** may have the result type/len set to match attribute type/len
	** in default processing and similar contexts. */
	if (srchp->pst_sym.pst_dataval.db_datatype !=
			nodep->pst_sym.pst_dataval.db_datatype ||
	    srchp->pst_sym.pst_dataval.db_length !=
			nodep->pst_sym.pst_dataval.db_length ||
	    srchp->pst_sym.pst_dataval.db_prec !=
			nodep->pst_sym.pst_dataval.db_prec ||
	    srchp->pst_sym.pst_dataval.db_collID !=
			nodep->pst_sym.pst_dataval.db_collID)
	    break;

	/* Check for functions that must be executed individually
	** (random(), uuid()). */
	if (nodep->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_fiflags & 
							ADI_F32_NOTVIRGIN)
	    return(FALSE);

	/* Don't optimize for Unorm operation */
	if (opno == ADI_UNORM_OP )
	    return(FALSE);

	/* Don't optimize blobs.  Re-use of blob temps can get DMF
	** into trouble, especially if the query is INSERT.
	** (DMF assumes that it can dispose of a blob temporary after it's
	** done inserting it into a row.)  The whole question of blob
	** temporary lifetime needs to be dealt with before this restriction
	** be eliminated.
	*/
	dt = abs(nodep->pst_sym.pst_dataval.db_datatype);
	status = adi_dtinfo(subquery->ops_global->ops_adfcb, dt, &dtbits);
	if (status != E_DB_OK || dtbits & AD_PERIPHERAL)
	    return (FALSE);

	/* Now we're getting somewhere. Compare operand types. */
	comm = FALSE;		/* Probably not commutative */
	if (opno == ADI_MUL_OP)
	    comm = TRUE;
	else if (opno == ADI_ADD_OP)
	{
	    comm = TRUE;
	    /* String add is concatenation, which is NOT commutative.
	    ** If dtfamily weren't meaningless except for dates we could use
	    ** it here!  oh well.
	    */
	    if (dt == DB_CHR_TYPE || dt == DB_CHA_TYPE
	      || dt == DB_TXT_TYPE || dt == DB_VCH_TYPE
	      || dt == DB_BYTE_TYPE || dt == DB_VBYTE_TYPE
	      || dt == DB_NCHR_TYPE || dt == DB_NVCHR_TYPE)
		comm = FALSE;
	}

	if (!((leftop = nodep->pst_left)->pst_sym.pst_type ==
		srchp->pst_left->pst_sym.pst_type && 
	    ((rightop = nodep->pst_right) == NULL ||
	    rightop->pst_sym.pst_type ==
		srchp->pst_right->pst_sym.pst_type) ||
			/* left/right operand types match */
	    comm && (leftop = nodep->pst_right) != NULL &&
	    leftop->pst_sym.pst_type ==
		srchp->pst_right->pst_sym.pst_type &&
	    (rightop = nodep->pst_left)->pst_sym.pst_type ==
		srchp->pst_right->pst_sym.pst_type))
			/* or commutative and left matches right 
			** and right matches left */
	    return(FALSE);

	if (rightop == NULL || leftop->pst_sym.pst_type !=
	    rightop->pst_sym.pst_type)
	    comm = FALSE;	/* commutativity is only of further
				** use if operands are same type */

	/* Operand types match - now check for operand contents. */
	if (!opc_compops(subquery, leftop, rightop, srchp) &&
	    (comm == FALSE || !opc_compops(subquery, rightop, leftop, srchp)))
	    return(FALSE);

	/* If we get here, the expressions match and we can do
	** the transformation. Just replace nodep by the appropriate
	** PST_VAR - the expression gets added to the list back in
	** opc_expranal(). */
	if (!(*gotone))
	{
	    *gotone = TRUE;
	    (*subex_countp)++;
	}
	attrid.db_att_id = *subex_countp - 1;
	*nodepp = opv_varnode(subquery->ops_global, 
		&nodep->pst_sym.pst_dataval, OPV_NOVAR, &attrid);
	return(TRUE);

      case PST_VAR:
      case PST_CONST:
      case PST_COP:
      case PST_QLEND:
      case PST_TREE:
      case PST_SEQOP:
	/* These are all ok. */
	return(TRUE);

      case PST_RESDOM:
	/* Search each RESDOM expression in turn for matching subexpression. */
	if (nodep->pst_right)
	    dummy = opc_exprsrch(subquery, cnode, subexpp, &nodep->pst_right,
		subexlistp, subex_countp, gotone);

	if (nodep->pst_left)
	    dummy = opc_exprsrch(subquery, cnode, subexpp, &nodep->pst_left,
		subexlistp, subex_countp, gotone);

	return(TRUE);

      default:
	/* All the rest just return FALSE. */
	return(FALSE);
    }  /* end of switch */
}


/*{
** Name: OPC_COMPOPS	- compares operands of 2 operators to determine
**	if expressions are identical.
**
** Description:
**      This function compares one (if unary) or 2 (if binary) pairs of
**	operands to determine if the containing operators are identical
**	and can be reduced by CX optimization.
**
** Inputs:
**	subquery	- OPS_SUBQUERY state structure ptr
**
**	leftop		- ptr to left operand of 1st expression
**
**	rightop		- ptr to right operand of 1st expression or NULL
**
**	srchop		- ptr to comparand operator
**
** Outputs:
**
**	Returns:
**	    TRUE	- if operands all match
**
**	    FALSE	- otherwise
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      28-june-05 (inkdo01)
**	    Written for CX optimization feature.
*/

static bool
opc_compops(
	OPS_SUBQUERY	*subquery,
	PST_QNODE	*leftop,
	PST_QNODE	*rightop,
	PST_QNODE	*srchp)

{


    /* Compare left operands to start. Initially, we only check
    ** PST_VARs, PST_CONSTs, PST_SEQOPs and PST_COPs. Other operand types
    ** will be added later, as needed. */
    switch (leftop->pst_sym.pst_type) {

      case PST_VAR:
	/* Varno and attid must match. */
	if (leftop->pst_sym.pst_value.pst_s_var.pst_vno !=
	    srchp->pst_left->pst_sym.pst_value.pst_s_var.pst_vno ||
	    leftop->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id !=
	    srchp->pst_left->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id)
	    return(FALSE);

	break;

      case PST_CONST:
	/* Constant types and values must match. */
	if (leftop->pst_sym.pst_value.pst_s_cnst.pst_tparmtype !=
	    srchp->pst_left->pst_sym.pst_value.pst_s_cnst.pst_tparmtype ||
	    leftop->pst_sym.pst_value.pst_s_cnst.pst_parm_no !=
	    srchp->pst_left->pst_sym.pst_value.pst_s_cnst.pst_parm_no ||
	    leftop->pst_sym.pst_dataval.db_datatype !=
	    srchp->pst_left->pst_sym.pst_dataval.db_datatype ||
	    leftop->pst_sym.pst_dataval.db_length !=
	    srchp->pst_left->pst_sym.pst_dataval.db_length ||
	    leftop->pst_sym.pst_dataval.db_prec !=
	    srchp->pst_left->pst_sym.pst_dataval.db_prec) 
	    return(FALSE);

	if (leftop->pst_sym.pst_dataval.db_data == NULL &&
	    srchp->pst_left->pst_sym.pst_dataval.db_data != NULL ||
	    leftop->pst_sym.pst_dataval.db_data != NULL &&
	    srchp->pst_left->pst_sym.pst_dataval.db_data == NULL)
	    return(FALSE);

	/* Finally, check constant values. */
	if (leftop->pst_sym.pst_value.pst_s_cnst.pst_parm_no == 0 &&
	    opu_dcompare(subquery->ops_global, &leftop->pst_sym.pst_dataval,
		&srchp->pst_left->pst_sym.pst_dataval) != 0)
	    return(FALSE);

	break;

      case PST_SEQOP:
	/* Sequence operators must have the same sequence and operator. */
	if (leftop->pst_sym.pst_value.pst_s_seqop.pst_seqid.db_tab_base !=
		srchp->pst_left->pst_sym.pst_value.pst_s_seqop.pst_seqid.
					db_tab_base ||
	    leftop->pst_sym.pst_value.pst_s_seqop.pst_seqid.db_tab_index !=
		srchp->pst_left->pst_sym.pst_value.pst_s_seqop.pst_seqid.
					db_tab_index ||
	    leftop->pst_sym.pst_value.pst_s_seqop.pst_seqflag !=
		srchp->pst_left->pst_sym.pst_value.pst_s_seqop.pst_seqflag)
	    return(FALSE);

	break;

      case PST_COP:
	/* Constant operations must have the same pst_opinst - they 
	** have no operands. */
	if (leftop->pst_sym.pst_value.pst_s_op.pst_opinst !=
		srchp->pst_left->pst_sym.pst_value.pst_s_op.pst_opinst)
	    return(FALSE);

	break;

      default:
	/* Nothing else is supported yet. */
	return(FALSE);
    }	/* end of switch */

    if (rightop == NULL)
	return(TRUE);

    /* Repeat the exercise for the right operands. */
    switch (rightop->pst_sym.pst_type) {

      case PST_VAR:
	/* Varno and attid must match. */
	if (rightop->pst_sym.pst_value.pst_s_var.pst_vno !=
	    srchp->pst_right->pst_sym.pst_value.pst_s_var.pst_vno ||
	    rightop->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id !=
	    srchp->pst_right->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id)
	    return(FALSE);

	break;

      case PST_CONST:
	/* Constant types and values must match. */
	if (rightop->pst_sym.pst_value.pst_s_cnst.pst_tparmtype !=
	    srchp->pst_right->pst_sym.pst_value.pst_s_cnst.pst_tparmtype ||
	    rightop->pst_sym.pst_value.pst_s_cnst.pst_parm_no !=
	    srchp->pst_right->pst_sym.pst_value.pst_s_cnst.pst_parm_no ||
	    rightop->pst_sym.pst_dataval.db_datatype !=
	    srchp->pst_right->pst_sym.pst_dataval.db_datatype ||
	    rightop->pst_sym.pst_dataval.db_length !=
	    srchp->pst_right->pst_sym.pst_dataval.db_length ||
	    rightop->pst_sym.pst_dataval.db_prec !=
	    srchp->pst_right->pst_sym.pst_dataval.db_prec) 
	    return(FALSE);

	if (rightop->pst_sym.pst_dataval.db_data == NULL &&
	    srchp->pst_right->pst_sym.pst_dataval.db_data != NULL ||
	    rightop->pst_sym.pst_dataval.db_data != NULL &&
	    srchp->pst_right->pst_sym.pst_dataval.db_data == NULL)
	    return(FALSE);

	/* Finally, check constant values. */
	if (rightop->pst_sym.pst_value.pst_s_cnst.pst_parm_no == 0 &&
	    opu_dcompare(subquery->ops_global, &rightop->pst_sym.pst_dataval,
		&srchp->pst_right->pst_sym.pst_dataval) != 0)
	    return(FALSE);

	break;

      case PST_SEQOP:
	/* Sequence operators must have the same sequence and operator. */
	if (rightop->pst_sym.pst_value.pst_s_seqop.pst_seqid.db_tab_base !=
		srchp->pst_right->pst_sym.pst_value.pst_s_seqop.pst_seqid.
					db_tab_base ||
	    rightop->pst_sym.pst_value.pst_s_seqop.pst_seqid.db_tab_index !=
		srchp->pst_right->pst_sym.pst_value.pst_s_seqop.pst_seqid.
					db_tab_index ||
	    rightop->pst_sym.pst_value.pst_s_seqop.pst_seqflag !=
		srchp->pst_right->pst_sym.pst_value.pst_s_seqop.pst_seqflag)
	    return(FALSE);

	break;

      case PST_COP:
	/* Constant operations must have the same pst_opinst - they 
	** have no operands. */
	if (rightop->pst_sym.pst_value.pst_s_op.pst_opinst !=
		srchp->pst_right->pst_sym.pst_value.pst_s_op.pst_opinst)
	    return(FALSE);

	break;

      default:
	/* Nothing else is supported yet. */
	return(FALSE);
    }	/* end of switch */

    /* If it falls off the end, they match! */
    return(TRUE);
}
