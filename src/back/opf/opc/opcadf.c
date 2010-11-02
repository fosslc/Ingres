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
#include    <ade.h>
# include   <adp.h>
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
#define        OPC_XADF		TRUE
#include    <opxlint.h>
#include    <opclint.h>


/*
**  Defines of constants
*/
#define	    OPC_PMYES		1
#define	    OPC_PMNO		2
#define	    OPC_PMUNSET		3
#define     MAXUI2              (2 * MAXI2 + 1)         /* Largest u_i2 */

/*
**  Definition of static variables and forward static functions.
*/

static i4 cxhead_size = -1;		/* Filled in upon first use */

static VOID opc_adbegin (
	OPS_STATE	*global,
	OPC_ADF		*cadf,
	QEN_ADF		*qadf,
	i4		ninstr,
	i4		nops,
	i4		nconst,
	i4		szconst,
	i4		max_base,
	i4		mem_flag
);

static VOID
opc_adexpand(
	OPS_STATE   *global,
	OPC_ADF	    *cadf );

static VOID
opc_bsmap(
	OPS_STATE   *global,
	OPC_ADF	    *cadf,
	i4	    *parray,
	i4	    *pindex,
	i4	    baseno );

FUNC_EXTERN DB_STATUS	    adi_resolve();

/**
**
**  Name: OPCADF.C - Routines for building QEN_ADF and related structs
**
**  Description:
**      This files contains routines that make requests of ADF, esp ADE.
**	All of the routines in this file have are named 'opc_ad*'.
**      In most cases, the correlation between the 'opc_ad*' routine and
**      the ADF routine should be obvious; for example opc_adbegin() calls
**      ade_bgn_comp(). I have attemped to localize all calls to adf to this
**      file. This, hopefully, should make thinks easier if ADF is forced to 
**      change its interface. 
[@comment_line@]...
**
**	The external routines are:
**          opc_adalloc_begin() - Allocate a QEN_ADF, then call adbegin.
**          opc_adalloct_begin() - Allocate a QEN_ADF, then call adbegin.
**				Used if CX might be empty.
**          opc_adconst() - Compile a constant into an OPC_ADF
**          opc_adinstr() - Compile an instruction into an OPC_ADF
**          opc_adtransform() - Compile instruction(s) to convert datatypes
**          opc_adend() - End an OPC_ADF struct compilation.
**          opc_adcalclen() - Calculate the length of a datatype.
**          opc_adempty() - Fill in a DB_DATA_VALUE with an empty value.
**          opc_adkeybld() - Build a relation key (directly, not compiled).
**          opc_adseglang() - Set a query lang in a CX segment.
**	    opc_resolve() - Finds a datatype for an outer join column.
[@func_list@]...
**
**	The internal routines are:
**          opc_adexpand() - Expand the memory that a CX uses.
**          opc_adbase() - Add a base element to an OPC_ADF struct
**          opc_bsmap() - The work horse for opc_adbase()
[@func_list@]...
**
**
**  History:
**      23-aug-86 (eric)
**          created
**	02-sep-88 (puree)
**	    Remove qp_cb_cnt.  Rename the follwing QEF fields:
**		    qp_cb_num	to  qp_cb_cnt.
**		    qen_adf	to  qen_ade_cx
**		    qen_param	to  qen_pcx_idx
**	    Modify opc_adend to handle CXs with VLUP.
**	19-feb-89 (paul)
**	    Added rules support. Upgrade opc_bsmap to support QEF_USR_PARAM
**	    arrays for CALLPROC statements.
**	17-apr-89 (jrb)
**	    Properly initialized prec field(s).
**	03-jan-90 (stec)
**	    Corrected opc_adbegin to look at multiple PST_DECVAR stmts
**	27-apr-90 (stec)
**	    Changed interface to opc_adtransform (related to bug 21338).
**	15-may-90 (stec)
**	    Changed code in opc_adtransform (bug 21338).
**	16-jan-91 (stec)
**	    Added missing opr_prec initialization to opc_adcalclen().
**      01-mar-92 (anitap)
**          Fixed bug 39924 in opc_adexpand(). Related to MEcopy().
**      18-mar-92 (anitap)
**          Fixed bug 42363 concerned with equivalence class. See routinee
**          opc_cqual() in OPCQUAL.C for details. Added a check in
**          opc_adtransform().
**      14-aug-92 (anitap)
**          Fixed bug 45876/46196 which was a regression due to fix for 
**	    bug 42363. See opc_adtransform() for details.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	15-oct-1993 (rog)
**	    Call ulm_copy() instead of MEcopy() as a more general fix for
**	    Anita's fix for 39924 above.
**	03-feb-94 (rickh)
**	    Replaced 9999 diagnostics with real error messages.
**	25-oct-95 (inkdo01)
**	    Reversed default query, CX language to DB_SQL
**	25-oct-95 (inkdo01)
**	    Check for "long" data types in an operand
**	7-nov-95 (inkdo01)
**	    Changed to initially allocate CX in local memory, then copy
**	    to QSF memory during opc_adend call.
**	12-sep-97 (kitch01)
**		Amend opc_adempty to zero fill allocated memory to fix bug 84651.
**      20-apr-2001 (gupsh01)
**          Added support for long nvarchar. 
**	28-nov-02 (inkdo01 for hayke02)
**	    Override default scale when coercing to decimal (fix 109194).
**	08-Jan-2003 (hanch04)
**	    Back out last change.  Causes E_OP0791_ADE_INSTRGEN errors.
**	16-jan-2003 (somsa01 for hayke02/inkdo01)
**	    Override default scale when coercing to decimal (fix 109194).
**	17-nov-03 (inkdo01)
**	    Separate opc_temprow removes dependence of CX intermediate 
**	    results buffer being 1st entry in DSH row array.
**	17-dec-04 (inkdo01)
**	    Added various collID's.
**	17-Feb-2005 (schka24)
**	    Kill silly duplication of sizeof(ADE_CXHEAD), which was wrong
**	    anyway;  ask ADE for the size.
**      15-dec-2006 (stial01)
**          opc_adtransform: DB_LNLOC_TYPE is a unicode type
**	2-Jun-2009 (kschendel) b122118
**	    Remove dead code, unused adbase parameters.
**	04-May-2010 (kiria01) b123680
**	    Correct the bad stack referencing code in dt_family processing
**	16-Jun-2009 (thich01)
**	    Treat GEOM type the same as LBYTE.
**	20-Aug-2009 (thich01)
**	    Treat all spatial types the same as LBYTE.
**      08-Jun-2010 (horda03) b123878
**          For nullable string to decimal transformation, need to increase the
**          length of the datatype by 1, otherwise E_OP0791 gets reported.
[@history_template@]...
**/

/*
**  Defines of constants used in this file
*/
#define    OPC_UNKNOWN_LEN    0

/* Remember to change the value of OPC_NULL_BYTE_LEN when it
** is changed in ADF
*/
#define    OPC_NULL_BYTE_LEN   1


/*{
** Name: OPC_ADALLOC_BEGIN - allocate a QEN_ADF structure.
**
** Description:
**      Opc_adalloc_begin allocates a QEN_ADF structure from QSF
**	memory, then passes it to opc_adbegin for initialization. This
**	allows QEN_ADFs to be separate from the structures which contain
**	them, reducing the storage overhead of query plans. 
**
**	This entry point should be used when the caller is certain
**	that real CX code will be generated.  Other callers should use
**	opc-adalloct-begin, which allocates the QEN_ADF in optimizer
**	memory and only copies it to QSF if something happens.
**
[@comment_line@]...
**
** Inputs:
**  global -
**	Pointer to the query wide information for OPF and OPC.
**  cadf -
**	The address of an uninitialized piece of memory that will serve
**	as the OPC_ADF.
**  qadf, ninstr, nops, nconst, szconst, max_base, mem_flag - all just
**	get passed on to opc_adbegin (see it for their descriptions).
**
** Outputs:
**  qadf -
**	Address of newly allocated QEN_ADF structure will be returned
**	here. 
**
**	Returns:
**	    nothing
**	Exceptions:
**	    Only those for error conditions.
**
** Side Effects:
**	    none
**
** History:
**	7-nov-95 (inkdo01)
**	    Written as part of the query plan storage reduction effort. 
**	12-Dec-2005 (kschendel)
**	    Save pointer to qen-adf pointer so that we can zap it at "end"
**	    time, if nothing was generated.
[@history_template@]...
*/
VOID
opc_adalloc_begin(
		OPS_STATE   *global,
		OPC_ADF	    *cadf,
		QEN_ADF	    **qadf,
		i4	    ninstr,
		i4	    nops,
		i4	    nconst,
		i4	    szconst,
		i4	    max_base,
		i4	    mem_flag )
{
    /* All this does is allocate QEN_ADF memory and call straight on to
    ** opc_adbegin to format it. */

    *qadf = (QEN_ADF *) opu_qsfmem(global,(i4)sizeof(QEN_ADF));
    opc_adbegin(global, cadf, *qadf, ninstr, nops, nconst, szconst,
	max_base, mem_flag);
    cadf->opc_pqeadf = qadf;		/* For "end" time cleanup */
    cadf->opc_qenfromqsf = TRUE;

}

/*{
** Name: OPC_ADALLOCT_BEGIN - trial-allocate a QEN_ADF structure.
**
** Description:
**      Opc_adalloct_begin allocates a QEN_ADF structure from optimizer
**	memory, then passes it to opc_adbegin for initialization.  This
**	allows QEN_ADFs to be separate from the structures which contain
**	them, reducing the storage overhead of query plans.
**
**	This entry point can be used in all cases.  However if the caller
**	is certain that a nonempty CX will be generated, using
**	opc_adalloc_begin instead will be a little faster.
**
[@comment_line@]...
**
** Inputs:
**  global -
**	Pointer to the query wide information for OPF and OPC.
**  cadf -
**	The address of an uninitialized piece of memory that will serve
**	as the OPC_ADF.
**  qadf, ninstr, nops, nconst, szconst, max_base, mem_flag - all just
**	get passed on to opc_adbegin (see it for their descriptions).
**
** Outputs:
**  qadf -
**	Address of newly allocated QEN_ADF structure will be returned
**	here. 
**
**	Returns:
**	    nothing
**	Exceptions:
**	    Only those for error conditions.
**
** Side Effects:
**	    none
**
** History:
**	12-Dec-2005 (kschendel)
**	    Cloned from opc_adalloc_begin to genericize qen-adf handling.
[@history_template@]...
*/
VOID
opc_adalloct_begin(
		OPS_STATE   *global,
		OPC_ADF	    *cadf,
		QEN_ADF	    **qadf,
		i4	    ninstr,
		i4	    nops,
		i4	    nconst,
		i4	    szconst,
		i4	    max_base,
		i4	    mem_flag )
{
    /* All this does is allocate QEN_ADF memory and call straight on to
    ** opc_adbegin to format it.
    ** Note that opu-memory doesn't zero like qsfmem does.
    */

    *qadf = (QEN_ADF *) opu_memory(global,(i4)sizeof(QEN_ADF));
    MEfill(sizeof(QEN_ADF), 0, (PTR) *qadf);
    opc_adbegin(global, cadf, *qadf, ninstr, nops, nconst, szconst,
	max_base, mem_flag);
    cadf->opc_pqeadf = qadf;		/* For "end" time cleanup */
    cadf->opc_qenfromqsf = FALSE;

}

/*{
** Name: OPC_ADBEGIN	- Initialize an OPC_ADF struct for compilation.
**
** Description:
**      Opc_adbegin initializes an OPC_ADF struct so that it can be used for
**      compilation. Once begun, the OPC_ADF can be used in calls to  
**      opc_adconst(), opc_adinstr() as well as others. 
**        
**      The first step in this process is to initialize the QEN_ADF struct. 
**      This is done by allocating space for the base array, calling
**      ade_bgn_comp(), and initializing the other random fields in the 
**      QEN_ADF. 
**        
**      The next step in the OPC_ADF game plan is to initialize the rest of the
**      random fields in it. Finally, the QEN row that holds the temporary space
**      that may be needed during compilation is added to the base array.
[@comment_line@]...
**
** Inputs:
**  global -
**	Pointer to the query wide information for OPF and OPC.
**  cadf -
**	The address of an uninitialized piece of memory that will serve
**	as the OPC_ADF.
**  qadf -
**	The address of an uninitialized piece of memory that will serve
**	as the QEN_ADF.
**  ninstr -
**	An estimate on how many instructions will be compiled into the
**	OPC_ADF (and, therefore, into the CX)
**  nops -
**	Same as ninstr only for operands
**  nconst -
**	Same as for ninstr only for constants
**  szconst -
**	An estimate on the total size of the constants that will be compiled
**	into the CX.
**  max_base -
**	An upper bound on the number of base elements that will be needed
**	by this CX.
**  mem_flag -
**	This tells where temporary memory should be allocated from. If
**	mem_flag is OPC_STACK_MEM then opu_Gsmemory_get will be called
**	which will get the memory from the stack ULM memory stream. In this
**	situation, the 'cadf' will be unusable after the current QEN node is
**	finished being compiled. Otherwise mem_flag should be OPC_REG_MEM
**	and opu_memory() will be called. This memory will stay around until
**	the entire query plan is completed. If there is *any* doubt then you
**	should use OPC_REG_MEM, since OPC_STACK_MEM exists for performance
**	reasons only. In most cases, however, OPC_STACK_MEM can be used. An
**	easy way to tell is to look where the 'cadf' is allocated; if it's
**	allocated off of the (real) stack, then OPC_STACK_MEM can be used
**	safely. Note that this doesn't affect the allocation of the
**	memory for the CX, since it will always be allocated from QSF.
**
** Outputs:
**  cadf -
**  qadf -
**	Both the cadf and qadf structs will be completely initialized and
**	ready for instructions, constants, and such to be compiled in.
**  global->ops_cstate.opc_qp
**	The QP will indicate that another control block will be needed.
**
**	Returns:
**	    nothing
**	Exceptions:
**	    Only those for error conditions.
**
** Side Effects:
**	    none
**
** History:
**      23-aug-86 (eric)
**          written
**	02-sep-88 (puree)
**	    rename qen_param to qen_pcx_idx.  It is used as an index (relative
**	    to 1) to the VLUP descriptor array in QEF DSH.
**	22-sep-88 (linda,eric) bug #3311 -- Although a db procedure is not a
**	    repeat query, it may have parameters.  Therefore the condition for
**	    whether or not to use the nrepeats base should be "does it have
**	    any parameters", not "is it a db procedure".
**	19-feb-89 (paul)
**	    Initialize CALLPROC parameter base array.
**	03-jan-90 (stec)
**	    Corrected opc_adbegin to look at multiple PST_DECVAR stmts -
**	    bug 9363.
**	25-oct-95 (inkdo01)
**	    Reversed default query, CX language to DB_SQL
**	20-nov-95 (inkdo01)
**	    Re-reversed default query, CX language to accursed DB_QUEL 
**	    to fix fe problems.
**	15-jul-98 (inkdo01)
**	    Changes to support non-CNF changes.
**	23-feb-00 (hayke02)
**	    Removed opc_noncnf - we now test for OPS_NONCNF in the
**	    subquery's ops_mask directly in opc_cqual1(). This change
**	    fixes bug 100484.
[@history_template@]...
*/
static VOID
opc_adbegin(
		OPS_STATE   *global,
		OPC_ADF	    *cadf,
		QEN_ADF	    *qadf,
		i4	    ninstr,
		i4	    nops,
		i4	    nconst,
		i4	    szconst,
		i4	    max_base,
		i4	    mem_flag )
{
    i4	    cxsize;
    DB_STATUS	    err;
    i4		    sz_maxbase;
    QEF_QP_CB	    *qp = global->ops_cstate.opc_qp;
    u_char	    uchar_value;
    PST_PROCEDURE   *dbp_proc;

    /* If we don't know the CX header size, ask for it (one time only) */
    if (cxhead_size == -1)
    {
	err = ade_cxinfo(global->ops_adfcb, NULL,
			ADE_09REQ_SIZE_OF_CXHEAD, &cxhead_size);
	if (err != E_DB_OK)
	    opx_verror(err, E_OP078F_ADE_BGNCOMP,
				global->ops_adfcb->adf_errcb.ad_errcode);
    }

    /* first, figure out how much memory to allocate for the bases */
    if (nconst > 0)
    {
	if (global->ops_procedure->pst_isdbp == TRUE)
	{
	    PST_STATEMENT	*decvar_stmt;

	    dbp_proc = global->ops_procedure;
	    if (dbp_proc->pst_parms != NULL)
	    {
		max_base += dbp_proc->pst_parms->pst_nvars;
	    }

	    for (decvar_stmt = global->ops_cstate.opc_topdecvar;
		    decvar_stmt != NULL;
		    decvar_stmt =
		      ((OPC_PST_STATEMENT *)decvar_stmt->pst_opf)->opc_ndecvar
		)
	    {
		max_base += decvar_stmt->pst_specific.pst_dbpvar->pst_nvars;
	    }
	}
	else if (global->ops_parmtotal > 0 && global->ops_procedure != NULL &&
		     global->ops_procedure->pst_isdbp == FALSE
		)
	{
	    /* If repeat query parameters might be used, then 
	    ** increase max_base so that it is large enough to hold any 
	    ** repeat query parameters that may come up.
	    */
	    max_base += nconst;	/* For the VLUPs */
	    max_base += ninstr;	/* For the VLTs */
	}
    }
	    
    max_base += 1;  /* Add one for the temp row that is always added below */

    /* Now lets figure out how much memory to allocate for the CX */
    if ((err = ade_cx_space(global->ops_adfcb, ninstr, nops, nconst, szconst,
							&cxsize)) != E_DB_OK
	)
    {
	opx_verror(err, E_OP078E_ADE_CXSPACE,
				    global->ops_adfcb->adf_errcb.ad_errcode);
    }

    /* Now lets initialize the qadf */
    cxsize = cxsize*3/4;     	/* * .75 to fudge it down a tad */
    if (cxsize < cxhead_size) cxsize = cxhead_size;
				/* but CXHEAD is minimum size */
    qadf->qen_ade_cx = opu_memory(global, (i4)cxsize);
    if ((err = ade_bgn_comp(global->ops_adfcb, qadf->qen_ade_cx, cxsize)) 
								    != E_DB_OK
	)
    {
	opx_verror(err, E_OP078F_ADE_BGNCOMP,
				global->ops_adfcb->adf_errcb.ad_errcode);
    }
    qadf->qen_pos = qp->qp_cb_cnt;
    qadf->qen_sz_base = 0;
    qadf->qen_max_base = max_base;
    sz_maxbase = max_base * sizeof (*qadf->qen_base);
    qadf->qen_base = (QEN_BASE *)opu_memory(global, (i4)sz_maxbase);
    qadf->qen_pcx_idx = 0;  /* initialized VLUP descriptor array index */
    qadf->qen_uoutput = qadf->qen_output = qadf->qen_input = -1;
    qadf->qen_mask = 0;

    /* tell the QP about the new CB that we need */
    qp->qp_cb_cnt += 1;

    /* Now initialize the cadf */
    cadf->opc_qeadf = qadf;

    /* Initialize the CALLPROC paramater base structure */
    cadf->opc_ncparms = 0;
    cadf->opc_cparm_base = 0;
    
    cadf->opc_nrows = qp->qp_row_cnt;
    cadf->opc_truelab = NULL;
    cadf->opc_falselab = NULL;
    cadf->opc_labels = FALSE;
    cadf->opc_inhash = FALSE;

    if (mem_flag == OPC_STACK_MEM)
    {
	cadf->opc_row_base = (i4*) opu_Gsmemory_get(global, 
				sizeof (*cadf->opc_row_base) * cadf->opc_nrows);
    }
    else
    {
	cadf->opc_row_base = (i4*) opu_memory(global, 
				sizeof (*cadf->opc_row_base) * cadf->opc_nrows);
    }
    uchar_value = 0;
    uchar_value = ~uchar_value;
    MEfill(sizeof (*cadf->opc_row_base) * cadf->opc_nrows,
			    (u_char) uchar_value, (PTR)cadf->opc_row_base);

    /*
    ** 22-sep-88 bug #3311 -- changed if condition from
    ** if (cadf->ops_procedure->pst_isdbp == FALSE)
    */
    if (global->ops_statement->pst_type == PST_QT_TYPE &&
	global->ops_parmtotal > 0 &&
	global->ops_procedure != NULL &&
	global->ops_procedure->pst_isdbp == FALSE)
    {
	cadf->opc_nrepeats = global->ops_parmtotal + 1;
	if (mem_flag == OPC_STACK_MEM)
	{
	    cadf->opc_repeat_base = (i4*) opu_Gsmemory_get(global, 
		sizeof (*cadf->opc_repeat_base) * cadf->opc_nrepeats);
	}
	else
	{
	    cadf->opc_repeat_base = (i4*) opu_memory(global, 
		sizeof (*cadf->opc_repeat_base) * cadf->opc_nrepeats);
	}
	uchar_value = 0;
	uchar_value = ~uchar_value;
	MEfill(sizeof (*cadf->opc_repeat_base) * cadf->opc_nrepeats,
		(u_char) uchar_value, (PTR)cadf->opc_repeat_base);
    }
    else
    {
	cadf->opc_nrepeats = 0;
	cadf->opc_repeat_base = (i4 *) NULL;
    }

    cadf->opc_key_base = -1;
    cadf->opc_out_base = -1;
    cadf->opc_vtemp_base = -1;
    cadf->opc_dmf_alloc_base = -1;
    cadf->opc_memflag = mem_flag;
    cadf->opc_qlang[ADE_SINIT] = DB_QUEL;
    cadf->opc_qlang[ADE_SMAIN] = DB_QUEL;
    cadf->opc_qlang[ADE_SFINIT] = DB_QUEL;
    cadf->opc_qlang[ADE_SVIRGIN] = DB_QUEL;
    cadf->opc_cxlang[ADE_SINIT] = DB_QUEL;
    cadf->opc_cxlang[ADE_SMAIN] = DB_QUEL;
    cadf->opc_cxlang[ADE_SFINIT] = DB_QUEL;
    cadf->opc_cxlang[ADE_SVIRGIN] = DB_QUEL;

    /*
    ** Now that the qadf and cadf are completely initialized, we can add the
    ** temp row as our first base. We do this here because the temp row may
    ** be needed by *any* CX by opc_adinstr() to align data.
    */
    opc_adbase(global, cadf, QEN_ROW, OPC_CTEMP);
}

/*{
** Name: OPC_ADCONST	- Compile a constant into an OPC_ADF
**
** Description:
**      Opc_adconst() compiles a given constant in the CX that is referred to 
**      by the given OPC_ADF. Also, we keep track of wether pattern matching 
**      is desired by the instruction that is about to be compiled. 
[@comment_line@]...
**
** Inputs:
**  global -
**	global state info for this query
**  cadf -
**	The OPC_ADF struct whose compilation has been begun.
**  dv -
**	The constant
**  adfop -
**	A piece of uninitialized memory.
**  qlang -
**	The query language id for the constant being compiled.
**  seg -
**	The segment that the constant will be used in
**
** Outputs:
**  cadf->opc_qeadf->qen_ade_cx
**	This CX will hold the constant.
**  adfop -
**	This adf operand to describe the compiled constant.
**
**	Returns:
**	    none
**	Exceptions:
**	    Only in error conditions.
**
** Side Effects:
**	    none
**
** History:
**      23-aug-86 (eric)
**          written
[@history_template@]...
*/
VOID
opc_adconst(
		OPS_STATE	*global,
		OPC_ADF		*cadf,
		DB_DATA_VALUE	*dv,
		ADE_OPERAND	*adfop,
		DB_LANG		qlang,
		i4		seg)
{
    DB_STATUS	    err;
    QEN_ADF	    *qadf = cadf->opc_qeadf;

    if ((err = ade_const_gen(global->ops_adfcb, qadf->qen_ade_cx, dv, adfop)) 
								    != E_DB_OK)
    {
	if (global->ops_adfcb->adf_errcb.ad_errcode == E_AD5506_NO_SPACE)
	{
	    opc_adexpand(global, cadf);
	    opc_adconst(global, cadf, dv, adfop, qlang, seg);
	}
	else
	{
	    opx_verror(err, E_OP0790_ADE_CONSTGEN,
		global->ops_adfcb->adf_errcb.ad_errcode);
	}
    }

    cadf->opc_qlang[seg] = qlang;

    if ((qlang != DB_SQL && qlang != DB_QUEL) || seg < 1 || seg > ADE_SMAX)
    {
	opx_error(E_OP0887_CONSTTYPE);
    }
}

/*{
** Name: OPC_ADPARM	- "Compile" a repeat query parameter into an OPC_ADF
**
** Description:
**      Opc_adparm() compiles a given repeat query parameter in the CX that is
**	referred to by the given OPC_ADF. What this really means is that we
**      record what the query lang that is being used. 
[@comment_line@]...
**
** Inputs:
**  global -
**	global state info for this query
**  cadf -
**	The OPC_ADF struct whose compilation has been begun.
**  parm_no -
**	The parameter number
**  qlang -
**	The query language id for the constant being compiled.
**  seg -
**	The segment that the constant will be used in
**
** Outputs:
**
**	Returns:
**	    none
**	Exceptions:
**	    Only in error conditions.
**
** Side Effects:
**	    none
**
** History:
**      23-aug-86 (eric)
**          written
[@history_template@]...
*/
VOID
opc_adparm(
		OPS_STATE	*global,
		OPC_ADF		*cadf,
		i4		parm_no,
		DB_LANG		qlang,
		i4		seg)
{
    cadf->opc_qlang[seg] = qlang;

    if (qlang != DB_SQL && qlang != DB_QUEL)
    {
	opx_error(E_OP0887_CONSTTYPE);
    }
}

/*{
** Name: OPC_ADINSTR	- Compile an instruction into an OPC_ADF
**
** Description:
**      Opc_adinstr() compiles an instruction into a CX that is referred to
**      by the given OPC_ADF. Most of the real work here is done by  
**      ade_instr_gen(). The interesting part of this routine is in handling 
**      the errors that ade_instr_gen() returns. If we are informed that 
**      the CX doesn't have enough space for the constant, then we 
**      automatically allocate a larger CX and recompile the instruction.
**	If one of the operands is unaligned then we align it and recompile
**      the instruction. 
[@comment_line@]...
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
**      23-aug-86 (eric)
**          written
**      20-Nov-89 (fred)
**	    Added code to support instructions on BIG datatypes.  These may need
**	    workspaces...
**	25-oct-95 (inkdo01)
**	    Check for "long" data types in an operand
**	08-Mar-1999 (shero03)
**	    Support functions with 4 operands
**      20-apr-2001 (gupsh01)
**          Added support for long nvarchar.
**	22-june-01 (inkdo01)
**	    Change nops computation for binary OLAP aggregates.
**	24-Mar-2005 (schka24)
**	    Simplify query constant stuff a little.
**      10-nov-2006 (huazh01)
**          rewrite the code which requests and uses aligned space for
**          an unaligned output operand. The original code may produce
**          wrong result if an aggregate operand needs to align its
**          output space.
**          This fixes bug 116999.
**      28-nov-2006 (huazh01)
**          Correct the x-integration for b116999. 
**          This fixes B117208.
**      11-jun-2008 (huazh01)
**          remove the fix to b117208 and b116999. Their fixes are
**          in opc_baggtarg_begin() now. 
**          bug 120492. 
**	6-aug-2008 (dougi)
**	    Use MECOPYN for hash related stuff.
*/
VOID
opc_adinstr(
		OPS_STATE	*global,
		OPC_ADF		*cadf,
		i4		icode,
		i4		seg,
		i4		nops,
		ADE_OPERAND	ops[],
		i4		noutops)
{
    QEF_QP_CB	    *qp = global->ops_cstate.opc_qp;
    QEN_ADF	    *qadf = cadf->opc_qeadf;
    DB_STATUS	    err;
    i4		    unaligned;
    i4		    align;
    i4		    opno; 
    i4		    qconst_flag;
    i4		    copy_opno;
    DB_DT_ID	    tempdt;
    ADE_OPERAND	    alops[ADI_MAX_OPERANDS];
    ADE_OPERAND	    big_ops[ADI_MAX_OPERANDS+2];
    ADI_FI_DESC	    *fidesc = 0;

#ifdef xDEVTEST
    for (opno = 0; opno < nops, opno += 1)
    {
	if (ops[opno].opr_base < 0 || ops[opno].opr_base >= qadf->qen_max_base)
	    opx_error(E_OP0791_ADE_INSTRGEN);
    }
#endif

    if (cadf->opc_inhash)
	copy_opno = ADE_MECOPYN;
    else copy_opno = ADE_MECOPY;

    /* check for PERIPHERAL data type and set flag */
    for (opno = 0; opno < nops; opno++)
    {
	tempdt = abs(ops[opno].opr_dt);
	if (tempdt == DB_LVCH_TYPE || tempdt == DB_LBYTE_TYPE || 
			tempdt == DB_LNVCHR_TYPE || tempdt == DB_GEOM_TYPE   ||
                        tempdt == DB_POINT_TYPE  || tempdt == DB_MPOINT_TYPE ||
                        tempdt == DB_LINE_TYPE   || tempdt == DB_MLINE_TYPE  ||
                        tempdt == DB_POLY_TYPE   || tempdt == DB_MPOLY_TYPE  ||
                        tempdt == DB_GEOMC_TYPE )
	{
	    qadf->qen_mask |= QEN_HAS_PERIPH_OPND;
	    break;
	}
    }

    /* set which segment is being used in the QEN_ADF */
    switch (seg)
    {
     case ADE_SVIRGIN:
	qadf->qen_mask |= QEN_SVIRGIN;
	break;

     case ADE_SINIT:
	qadf->qen_mask |= QEN_SINIT;
	break;

     case ADE_SMAIN:
	qadf->qen_mask |= QEN_SMAIN;
	break;

     case ADE_SFINIT:
	qadf->qen_mask |= QEN_SFINIT;
	break;
    }

    /* Make sure that the pattern matching semantics are correct for the
    ** constants that we have compiled.
    ** This may emit unnecessary pmquel-controlling operations, but ade
    ** optimizes them away at the end of code generation.
    */
    if (cadf->opc_qlang[seg] == DB_SQL && cadf->opc_cxlang[seg] == DB_QUEL)
    {
	cadf->opc_cxlang[seg] = DB_SQL;
	opc_adinstr(global, cadf, ADE_NO_PMQUEL, seg, 0, ops, 0);
    }
    else if (cadf->opc_qlang[seg] == DB_QUEL && cadf->opc_cxlang[seg] == DB_SQL)
    {
	cadf->opc_cxlang[seg] = DB_QUEL;
	opc_adinstr(global, cadf, ADE_PMQUEL, seg, 0, ops, 0);
    }

    if ((icode > 0) &&
	    (err = adi_fidesc(global->ops_adfcb, icode, &fidesc)) != E_DB_OK)
    {
	opx_verror(err, E_OP0791_ADE_INSTRGEN,
			    global->ops_adfcb->adf_errcb.ad_errcode);
    }
    else if ((icode == ADE_REDEEM) ||
	(fidesc && (fidesc->adi_fiflags & ADI_F4_WORKSPACE) &&
	!(fidesc->adi_fiflags & ADI_F16_OLAPAGG)))
    {
	i4		i;
	i4		exp_ops;
	
	if (fidesc)
	{
	    exp_ops = fidesc->adi_numargs + 1 /* result */ + 1;
	}
	else
	{
	    if (icode == ADE_REDEEM)
		exp_ops = 3;
	}

	/*
	** Because this routine may be called recursively after an expansion,
	** make sure we haven't added the workspace already.  If so,
	** skip doing so...
	*/
	if (nops < exp_ops)
	{
	    for (i = 0; i < nops; i++)
	    {
		STRUCT_ASSIGN_MACRO(ops[i], big_ops[i]);
	    }

	    ops = big_ops;

	    ops[nops].opr_base = cadf->opc_row_base[OPC_CTEMP];
	    ops[nops].opr_offset = global->ops_cstate.opc_temprow->opc_olen;
	    ops[nops].opr_dt = 0;
	    ops[nops].opr_prec = 0;
	    ops[nops].opr_collID = -1;
	    ops[nops].opr_len = (i4) (sizeof(ADP_LO_WKSP) +
				    (fidesc ?
				         fidesc->adi_agwsdv_len :
				         DB_MAXTUP));
	    {
		i4		align;

		align = ops[nops].opr_len % sizeof(ALIGN_RESTRICT);
		if (align != 0)
		{
		    align = sizeof(ALIGN_RESTRICT) - align;
		}
		global->ops_cstate.opc_temprow->opc_olen +=
					ops[nops].opr_len + align;
	    }
	    nops += 1;
	}
    }
    qconst_flag = 0;
    if ((err = ade_instr_gen(global->ops_adfcb, qadf->qen_ade_cx, icode, seg, 
			nops, ops, &qconst_flag, &unaligned)) == E_DB_OK)
    {
	/* Light flag for runtime if we might need a query constants block */
	if (qconst_flag)
	    qp->qp_status |= QEQP_NEED_QCONST;
    }
    else
    {
	/* Instr gen failed.  Recover if out of space, or unaligned */
	if (global->ops_adfcb->adf_errcb.ad_errcode == E_AD5506_NO_SPACE)
	{
	    opc_adexpand(global, cadf);
	    opc_adinstr(global, cadf, icode, seg, nops, ops, noutops);
	}
	else if (global->ops_adfcb->adf_errcb.ad_errcode == E_AD5505_UNALIGNED)
	{
	    if (unaligned >= noutops)
	    {
		/* The unaligned operand is for input, so lets move it to an
		** aligned memory and recompile the original instruction using
		** the aligned operand.
		**
		** First, set up the new operands to move the unaligned 
		** operand to aligned memory.
		*/
		STRUCT_ASSIGN_MACRO(ops[unaligned], alops[0]);
		STRUCT_ASSIGN_MACRO(ops[unaligned], alops[1]);
		alops[0].opr_base = cadf->opc_row_base[OPC_CTEMP];
		alops[0].opr_offset = global->ops_cstate.opc_temprow->opc_olen;
	    
		/* get the aligned memory */
		align = (alops[0].opr_len % sizeof (ALIGN_RESTRICT));
		if (align != 0)
		    align = sizeof (ALIGN_RESTRICT) - align;
		global->ops_cstate.opc_temprow->opc_olen += 
						    align + alops[0].opr_len;

		/* align the operand */
		opc_adinstr(global, cadf, copy_opno, seg, 2, alops, 1);

		/* now recompile the operation using the aligned memory */
		STRUCT_ASSIGN_MACRO(alops[0], ops[unaligned]);
		opc_adinstr(global, cadf, icode, seg, nops, ops, noutops);
		STRUCT_ASSIGN_MACRO(alops[1], ops[unaligned]);
	    }
	    else
	    {
		/* The unaligned operand is for output, so lets get aligned 
		** space for the output, recompile the original instruction
		** using the aligned space, and then MEcopy the results the
		** the unaligned space.
		**
		** First, set up the new operands to move the unaligned 
		** operand to aligned memory.
                */
		STRUCT_ASSIGN_MACRO(ops[unaligned], alops[0]);
		STRUCT_ASSIGN_MACRO(ops[unaligned], alops[1]);
		alops[1].opr_base = cadf->opc_row_base[OPC_CTEMP];
		alops[1].opr_offset = global->ops_cstate.opc_temprow->opc_olen;

		/* get the aligned memory */
		align = (alops[1].opr_len % sizeof (ALIGN_RESTRICT));
		if (align != 0)
		    align = sizeof (ALIGN_RESTRICT) - align;

		global->ops_cstate.opc_temprow->opc_olen += 
						    align + alops[1].opr_len;

		/* recompile the operation using the aligned memory */
		STRUCT_ASSIGN_MACRO(alops[1], ops[unaligned]);
		opc_adinstr(global, cadf, icode, seg, nops, ops, noutops);
		STRUCT_ASSIGN_MACRO(alops[0], ops[unaligned]);

                /* copy the results to the unaligned spot */
		opc_adinstr(global, cadf, copy_opno, seg, 2, alops, 1);
	    }
	}
	else
	{
	    opx_verror(err, E_OP0791_ADE_INSTRGEN,
		    global->ops_adfcb->adf_errcb.ad_errcode);
	}
    }

}

/*{
** Name: OPC_ADLABRES	- resolve a chain of label operands all pointing to
**	the same location.
**
** Description:
**      Opc_adlabres() calls adecompile to run down a chain of label operands 
**	in a CX, all of which resolve to the current location in the CX.
[@comment_line@]...
**
** Inputs:
**  global -
**	global state info for this query
**  cadf -
**	The OPC_ADF struct whose compilation has been begun.
**  labop -
**	The anchor of the label operand chain to be resolved.
**
** Outputs:
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
**      26-mar-97 (inkdo01)
**          written
**	13-sep-99 (inkdo01)
**	    Set opc_labels flag to indicate presence of "branch" instrs
**	    whose offsets must be verified at end of CX.
[@history_template@]...
*/
VOID
opc_adlabres(
		OPS_STATE	*global,
		OPC_ADF		*cadf,
		ADE_OPERAND	*labop)
{
    QEN_ADF	    *qadf = cadf->opc_qeadf;

    ade_lab_resolve(global->ops_adfcb, qadf->qen_ade_cx, labop);
    cadf->opc_labels = TRUE;
}

/*{
** Name: OPC_ADEXPAND	- Expand the memory that a CX uses.
**
** Description:
{@comment_line@}...
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
**      23-aug-86 (eric)
**          written
**      01-mar-92 (anitap)
**          Fixed bug 39924. MEcopy() is not sufficient for copying large
**          CXs which need contiguous memory. The size of the source that can
**          be copied is just u_i2. Thus call MEcopy() till all the source is
**          copied (if size larger than u_i2).
**      15-feb-93 (ed)
**          fix prototype casting problem
**      15-oct-1993 (rog)
**          Move Anita's MEcopy() fix above into ulm_copy() and call it.
[@history_template@]...
*/
static VOID
opc_adexpand(
	OPS_STATE   *global,
	OPC_ADF	    *cadf )
{
    QEN_ADF	*qadf = cadf->opc_qeadf;
    i4	old_size, new_size;
    i4		err;
    PTR		new;

    /* first, lets get the old CX size from ADF and calculate the new size
    */
    if ((err = ade_cxinfo(global->ops_adfcb, (PTR)qadf->qen_ade_cx, 
			    ADE_01REQ_BYTES_ALLOCATED, (PTR)&old_size)) != E_DB_OK
	)
    {
	/* EJLFIX: Get a better error message */
	opx_verror(err, E_OP0792_ADE_INFORMSP,
				global->ops_adfcb->adf_errcb.ad_errcode);
    }
    new_size = old_size + (old_size / 2);

    new = opu_memory(global, (i4) new_size);

    MEcopy((PTR)qadf->qen_ade_cx, (i4)old_size, (PTR)new);

    /* EJLFIX: Get QSF to deallocate qadf->qen_ade_cx; */
    qadf->qen_ade_cx = new;

    if ((err = ade_inform_space(global->ops_adfcb, qadf->qen_ade_cx, new_size)) 
								    != E_DB_OK
	)
    {
	opx_verror(err, E_OP0792_ADE_INFORMSP,
				global->ops_adfcb->adf_errcb.ad_errcode);
    }
}

/*{
** Name: OPC_ADBASE	- Add a base element to an OPC_ADF struct
**
** Description:
{@comment_line@}...
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
**      24-aug-86 (eric)
**          written
**	2-Jun-2009 (kschendel) b122118
**	    fid and aglen parameters obsolete, delete.
[@history_template@]...
*/
VOID
opc_adbase(
		OPS_STATE   *global,
		OPC_ADF	    *cadf,
		i4	    array,
		i4	    index)
{
    QEN_ADF	*qadf = cadf->opc_qeadf;
    QEN_BASE	*new;
    i4		baseno;

    baseno = qadf->qen_sz_base;

    if (baseno >= qadf->qen_max_base)
    {
	opx_error(E_OP0890_MAX_BASE);
    }

    /* update the row and repeat base maps in the cadf */
    opc_bsmap(global, cadf, &array, &index, baseno);

    new = &qadf->qen_base[baseno];
    new->qen_array = array;
    new->qen_index = index;
    new->qen_parm_row = 0;

    if (array == QEN_OUT)
    {
	qadf->qen_uoutput = baseno;
    }
    else if (array == QEN_DMF_ALLOC)
    {
	qadf->qen_input = baseno;
    }

    qadf->qen_sz_base += 1;
}

/*{
** Name: OPC_BSMAP	- Fill in the various mappings for base info in a cadf
**
** Description:
{@comment_line@}...
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
**      19-jan-87 (eric)
**          created
**	19-feb-89 (paul)
**	    Enhanced for rules processing. Added support for bases for CALLPROC
**	    parameter arrays.
**      15-may-94 (eric)
**          Added support for the QEN_HAS_PARAMS but in CXs. This is required
**          for QEF to streamline how it initializes CXs.
**	2-Jun-2009 (kschendel) b122118
**	    QEN_ADF_AG obsolete, delete it.
[@history_template@]...
*/
static VOID
opc_bsmap(
	OPS_STATE   *global,
	OPC_ADF	    *cadf,
	i4	    *parray,
	i4	    *pindex,
	i4	    baseno )
{
    QEF_QP_CB	*qp = global->ops_cstate.opc_qp;
    i4		old_nrows;
    i4		*old_row_base;
    OPC_BASE	*cbase;
    i4		array = *parray;
    i4		index = *pindex;
    u_char	uchar_value;

    switch (array)
    {
     case QEN_ROW:
	if (index < 0)
	{
	    opx_error(E_OP0891_MAX_ROWS);
	}

	if (index >= cadf->opc_nrows)
	{
	    old_nrows = cadf->opc_nrows;
	    old_row_base = cadf->opc_row_base;

	    if (index >= qp->qp_row_cnt)
	    {
		cadf->opc_nrows = index + 1;
	    }
	    else
	    {
		cadf->opc_nrows = qp->qp_row_cnt;
	    }
	    if (cadf->opc_memflag == OPC_STACK_MEM)
	    {
		cadf->opc_row_base = (i4*) opu_Gsmemory_get(global, 
				sizeof (*cadf->opc_row_base) * cadf->opc_nrows);
	    }
	    else
	    {
		cadf->opc_row_base = (i4*) opu_memory(global, 
				sizeof (*cadf->opc_row_base) * cadf->opc_nrows);
	    }

	    uchar_value = 0;
	    uchar_value = ~uchar_value;
	    MEfill(sizeof (*cadf->opc_row_base) * cadf->opc_nrows, 
		(u_char) uchar_value, (PTR)cadf->opc_row_base);

	    MEcopy((PTR)old_row_base, (sizeof (*cadf->opc_row_base) * 
		old_nrows), (PTR)cadf->opc_row_base);
	}

	cadf->opc_row_base[index] = baseno + ADE_ZBASE;
	break;

     case QEN_PLIST:
	/* Add a base for a CALLPROC parameter array. This is modeled after */
	/* the code used to add a row base. */
	if (index < 0)
	{
	    opx_error(E_OP0891_MAX_ROWS);
	}

	if (index >= cadf->opc_ncparms)
	{
	    old_nrows = cadf->opc_ncparms;
	    old_row_base = cadf->opc_cparm_base;

	    if (index >= qp->qp_cp_cnt)
	    {
		cadf->opc_ncparms = index + 1;
	    }
	    else
	    {
		cadf->opc_ncparms = qp->qp_cp_cnt;
	    }
	    if (cadf->opc_memflag == OPC_STACK_MEM)
	    {
		cadf->opc_cparm_base = (i4*) opu_Gsmemory_get(global, 
				sizeof (*cadf->opc_cparm_base) *
				cadf->opc_ncparms);
	    }
	    else
	    {
		cadf->opc_cparm_base = (i4*) opu_memory(global, 
				sizeof (*cadf->opc_cparm_base) *
				cadf->opc_ncparms);
	    }

	    uchar_value = 0;
	    uchar_value = ~uchar_value;
	    MEfill(sizeof (*cadf->opc_cparm_base) * cadf->opc_ncparms, 
		(u_char) uchar_value, (PTR)cadf->opc_cparm_base);

	    MEcopy((PTR)old_row_base, (sizeof (*cadf->opc_cparm_base) * 
		old_nrows), (PTR)cadf->opc_cparm_base);
	}

	cadf->opc_qeadf->qen_mask |= QEN_HAS_PARAMS;
	cadf->opc_cparm_base[index] = baseno + ADE_ZBASE;
	break;

     case QEN_PARM:
	if (index < 0 || index >= cadf->opc_nrepeats)
	{
	    opx_error(E_OP0892_MAX_REPEATS);
	}

	cadf->opc_repeat_base[index] = baseno + ADE_ZBASE;

	cbase = &global->ops_cstate.opc_rparms[index];
	if (cbase->opc_bvalid == TRUE)
	{
	    *parray = array = cbase->opc_array;
	    *pindex = index = cbase->opc_index;

	    if (array == QEN_PARM)
	    {	/* If the CX contains parameters, mark VLUP descriptor 
		** array index with -1.  It will be replaced by the real
		** value in opc_adend.
		*/
		cadf->opc_qeadf->qen_pcx_idx = -1;
		cadf->opc_qeadf->qen_mask |= QEN_HAS_PARAMS;
	    }
	    else
	    {
		opc_bsmap(global, cadf, parray, pindex, baseno);
	    }
	}
	else
	{
	    opx_error(E_OP0893_NV_REPEAT);
	}
	break;

     case QEN_KEY:
	cadf->opc_key_base = baseno + ADE_ZBASE;
	break;

     case QEN_OUT:
	cadf->opc_out_base = baseno + ADE_ZBASE;
	break;

     case QEN_VTEMP:
	cadf->opc_vtemp_base = baseno + ADE_ZBASE;
	break;

     case QEN_DMF_ALLOC:
	cadf->opc_dmf_alloc_base = baseno + ADE_ZBASE;
	break;

     default:
	/* EJLFIX: add an error here */
	break;
    }
}

/*{
** Name: OPC_ADTRANSFORM	- Compile code to convert datatypes.
**
** Description:
{@comment_line@}...
**
** Inputs:
**	setresop	-   boolean flag. When set to TRUE, base,
**			    offset, length and possibly nullability of
**			    of the datatype of the result operand
**			    will be set.
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
**      24-aug-86 (eric)
**          written
**	27-apr-90 (stec)
**	    Changed interface to opc_adtransform - added
**	    the calclen flag (related to bug 21338).
**	15-may-90 (stec)
**	    Changed calclen flag to mean "set result operand".
**	    Changed code to init certain fields in resop only
**	    when the flag is set. Related to bug 21338.
**      18-mar-92 (anitap)
**          As a fix for bug 42363, added the check to see if we are dealing
**          with the special case when datatypes are equal, but the lengths
**          are not (for non-comparison operators only), and so 
**	    opc_adtransform() is called from opc_cqual(). See opc_cqual() for 
**	    details.
**	16-jul-92 (rickh)
**	    Added two NULLs to the end of the opc_adcalclen call to make it
**	    agree with the function's definition.
**      14-aug-92 (anitap)
**          Fixed bug 45876/46196. Added check for the condition when the
**          result is non-nullable and the source is nullable and their
**          lengths are not equal for non-comparison operators.
**	08-Mar-1999 (shero03)
**	    Support 4 operands in opc_adcalclen
**	26-jul-02 (inkdo01)
**	    Add code to force normalization of any value coerced to Unicode.
**	01-oct-02 (guphs01)
**	    Added code to take care of the long nvarchar case when generating
**	    an instruction to normalize any value coerced to Unicode.
**	28-nov-02 (inkdo01 for hayke02)
**	    Override default scale when coercing to decimal (fix 109194).
**	9-Feb-2005 (schka24)
**	    Avoid generating float, int coercions when it's just
**	    non-nullable -> nullable (of the same basic length), do move
**	    instead.
**	17-Aug-2006 (kibro01) Bug 116481
**	    If a partition is created using an integer and it is accessed
**	    using a string (which is usually automatically a varchar) then
**	    allow the coercion if we are being called from pqmat
**	16-Oct-2006 (gupsh01)
**	    Modified result calculation in case where the function 
**	    instance requires datatype family resolution.
**	6-mar-2007 (dougi)
**	    Fix coercions from/to Unicode types to use proper collation IDs.
**	27-apr-2007 (dougi)
**	    Add support for UTF8-enabled server.
**	06-jan-2009 (gupsh01)
**	    On UTF-8 installations, when coercing between a LOB-Locator 
**	    and a long type do not introduce Unicode normalization.
**      24-mar-2010 (thich01)
**          Add a check for DB_GEOM_TYPE when src and res types are not equal.
**
**      07-Jun-2010 (horda03) b123878
**          For nullable decimal transforms, add 1 to the length (for the null
**          byte).
**	09-Aug-2010 (gupsh01)
**	    For VLUP input, if a coercion of a non-unicode type to a unicode
**	    type, triggers a unorm operation, ensure that we carry out the
**	    result length calculation for the unorm field. 
*/
VOID
opc_adtransform(
		OPS_STATE	*global,
		OPC_ADF		*cadf,
		ADE_OPERAND	*srcop,
		ADE_OPERAND	*resop,
		i4		seg,
		bool		setresop,
		bool		pqmat_retry_char)
{
    ADE_OPERAND	    ops[2];
    DB_STATUS	    ret;
    i4		    align;
    i4		    abs_srctype, abs_restype;	/* abs(data types) */
    ADE_OPERAND     *opptrs[ADI_MAX_OPERANDS];
    ADI_FI_ID	    fid;
    ADI_FI_DESC	    *fdesc;
    ADE_OPERAND	    uniop;
    i2		    saved_prec;
    bool	    unorm = FALSE;
    bool	    resuni = FALSE;
    bool	    srcuni = FALSE;
    i4		    orig_dt = 0;
    bool	    resutf8 = FALSE, srcutf8 = FALSE;
    bool	    unorm_init = FALSE;

    /* Get basic data types without nullability, used a lot here */
    abs_srctype = abs(srcop->opr_dt);
    abs_restype = abs(resop->opr_dt);

    /* Set flag indicating Unicode result - prepares the normalization code. */
    if ((global->ops_adfcb->adf_utf8_flag & AD_UTF8_ENABLED) &&
	(abs_srctype == DB_CHA_TYPE ||
	 abs_srctype == DB_VCH_TYPE ||
	 abs_srctype == DB_TXT_TYPE ||
	 abs_srctype == DB_CHR_TYPE ||
	 abs_srctype == DB_LVCH_TYPE ||
         abs_srctype == DB_LCLOC_TYPE ||
	 abs_srctype == DB_LTXT_TYPE))
	srcutf8 = TRUE;

    if ((global->ops_adfcb->adf_utf8_flag & AD_UTF8_ENABLED) &&
	(abs_restype == DB_CHA_TYPE ||
	 abs_restype == DB_VCH_TYPE ||
	 abs_restype == DB_TXT_TYPE ||
	 abs_restype == DB_CHR_TYPE ||
	 abs_restype == DB_LVCH_TYPE ||
         abs_restype == DB_LCLOC_TYPE ||
	 abs_restype == DB_LTXT_TYPE))
	resutf8 = TRUE;

    if (srcutf8 ||
	abs_srctype == DB_NCHR_TYPE || 
        abs_srctype == DB_NVCHR_TYPE ||
        abs_srctype == DB_LNVCHR_TYPE ||
	abs_srctype == DB_LNLOC_TYPE)

    {
	srcuni = TRUE;	
    }

    if (resutf8 ||
	abs_restype == DB_NCHR_TYPE || 
	abs_restype == DB_NVCHR_TYPE ||
	abs_restype == DB_LNVCHR_TYPE ||
	abs_restype == DB_LNLOC_TYPE)
    {
	resuni = TRUE;	
	/* Set result collation ID while we're in here. */
	if (srcuni)
	    resop->opr_collID = srcop->opr_collID;
	else resop->opr_collID = 0;
    }
	
    if (resuni && !srcuni) 
	unorm = TRUE;	/* As result is a unicode type, but src is not,
			** set flag to normalize the data.
		        */ 

    if (setresop == TRUE)
    {
	/*
	** First, let's set the data type, offset, and base info in the resop.
	** If the source data is nullable then make sure that the result is
	** nullable so we don't lose information. If the source is not 
	** nullable then the result can be either nullable or not since no
	** information would be lost either way.
	*/
	if (srcop->opr_dt < 0 && resop->opr_dt > 0)
	{
	    resop->opr_dt = -resop->opr_dt;

	    /* Fix for bug 45876/46196. When the result is non-nullable 
	    ** and the source is nullable and the lengths are equal, we
	    ** need to increment the length of the result to match its 
	    ** datatype (nullable). This is done for non-comparison  
	    ** operators only. This is to compensate for the decrement
	    ** operation done in ade_lenchk() in ADF. The bug was 
	    ** because the source operand was coerced to the 
	    ** len of the result operand - 1 (to take care of 
	    ** nullability). By incrementing the len of the result 
	    ** operand to correspond to the length of the 
	    ** nullable source, we get the right result.
            */
           if (resop->opr_len != OPC_UNKNOWN_LEN &&
		srcop->opr_dt == resop->opr_dt)
            {
               resop->opr_len += OPC_NULL_BYTE_LEN;
            }
	}

	resop->opr_offset = global->ops_cstate.opc_temprow->opc_olen;
	resop->opr_base = cadf->opc_row_base[OPC_CTEMP];
    }

    /*
    ** Now lets figure out how to convert the resop. To do
    ** this, find the func inst id to do the conversion;
    */
    if ((ret = adi_ficoerce(global->ops_adfcb, srcop->opr_dt,
	    resop->opr_dt, &fid)) != E_DB_OK
	)
    {
	/*
	** If we are within pqmat partition materialisation we have to 
	** allow char to integer coercion, but since more coercions are
	** to be available in the next major release, we'll just add this
	** one localised here
	*/
	if (pqmat_retry_char && 
		(abs(srcop->opr_dt)==DB_CHR_TYPE ||
		 abs(srcop->opr_dt)==DB_VCH_TYPE))
	{
		ret=adi_ficoerce(global->ops_adfcb, DB_LTXT_TYPE,
			resop->opr_dt, &fid);
		if (ret == E_DB_OK)
		{
		    orig_dt=srcop->opr_dt;
		    srcop->opr_dt=DB_LTXT_TYPE;
		}
	}
	if (ret != E_DB_OK)
	{
		opx_verror(ret, E_OP0783_ADI_FICOERCE,
		    global->ops_adfcb->adf_errcb.ad_errcode);
	}
    }

    /* find the length of the result */
    if (setresop == TRUE)
    {
	if ((ret = adi_fidesc(global->ops_adfcb, fid, &fdesc)) != E_DB_OK)
	{
	    /* EJLFIX: get a better error no */
	    opx_verror(ret, E_OP0783_ADI_FICOERCE,
		global->ops_adfcb->adf_errcb.ad_errcode);
	}

       /* Fix for bug 42363. When the datatypes are not equal, we need
       ** to calculate the length. This check has been added so that we
       ** do not calculate the length when the datatypes are equal (for
       ** the special case of non-comparison operators).
       */

       /*
        * Check if both types are in the GEOM Family
        * If so, the types are effectively equal.
       */
       if ((abs_srctype != abs_restype &&
           !(adi_dtfamily_retrieve(abs_srctype) == DB_GEOM_TYPE &&
             adi_dtfamily_retrieve(abs_restype) == DB_GEOM_TYPE))
               || (resop->opr_len == OPC_UNKNOWN_LEN))
       {
       	  PTR	 dataptr = NULL;
	  ADI_FI_DESC newfidesc;

	  opptrs[0] = srcop;
	  if (global->ops_gmask & OPS_DECCOERCION)
		saved_prec = resop->opr_prec;

	  /* date family coercion */
	  if ((abs(fdesc->adi_dtresult) == DB_DTE_TYPE) && 
	      (abs_restype != DB_DTE_TYPE))
	  {
  		if ((ret = adi_coerce_dtfamily_resolve(global->ops_adfcb, fdesc, 
				abs_srctype, abs_restype, &newfidesc)) != E_DB_OK)
		{
	    	  /* EJLFIX: get a better error no */
	    	  opx_verror(ret, E_OP0783_ADI_FICOERCE, 
			global->ops_adfcb->adf_errcb.ad_errcode);
		}
		else 
  		  fdesc = &newfidesc;
	  }

	  opc_adcalclen(global, cadf, &fdesc->adi_lenspec, 1, opptrs, 
			  resop, &dataptr );
	  if (global->ops_gmask & OPS_DECCOERCION)
	  {
		resop->opr_prec = saved_prec;
		resop->opr_len =
			    DB_PREC_TO_LEN_MACRO(DB_P_DECODE_MACRO(saved_prec));

                if (resop->opr_dt < 0)
                {
                   /* Don't forget the NULL byte */
                   resop->opr_len++;
                }
	  }

	  /* get the temp space to hold the data; */
	  if (resop->opr_len > 0)
	  {
	    align = resop->opr_len % sizeof (ALIGN_RESTRICT);
	    if (align != 0)
		align = sizeof (ALIGN_RESTRICT) - align;
	    global->ops_cstate.opc_temprow->opc_olen += resop->opr_len + align;
	  }

	  /* For VLUP parameters, if coercing into unicode types, 
	  ** Make sure that calclen sets the length for the unorm
	  ** operand as well.
	  */
	  if (unorm)
	  {
		STRUCT_ASSIGN_MACRO(*resop, uniop); /* init Unicode operand */
		uniop.opr_base = cadf->opc_row_base[OPC_CTEMP];
						/* temp result buffer */
		uniop.opr_offset = global->ops_cstate.opc_temprow->opc_olen;

		/* Update temp buffer offset past Unicode operand. */
		align = uniop.opr_len % sizeof (ALIGN_RESTRICT);
		if (align != 0)
		  align = sizeof (ALIGN_RESTRICT) - align;
		global->ops_cstate.opc_temprow->opc_olen += uniop.opr_len + align;

        	opc_adcalclen(global, cadf, &fdesc->adi_lenspec, 1, opptrs, 
			  &uniop, &dataptr );

		unorm_init = TRUE;
          }
       }	
       else if (resop->opr_len > 0)
	{
	/* get the temp space to hold the data; */
	    align = resop->opr_len % sizeof (ALIGN_RESTRICT);
	    if (align != 0)
		align = sizeof (ALIGN_RESTRICT) - align;
	    global->ops_cstate.opc_temprow->opc_olen += resop->opr_len + align;
	}
    }

    /* If Unicode result, allocate intermediate result for coercion so that
    ** normalization is done into resop. */
    if (unorm && !(unorm_init))
    {
	STRUCT_ASSIGN_MACRO(*resop, uniop);	/* init Unicode operand */
	uniop.opr_base = cadf->opc_row_base[OPC_CTEMP];
						/* temp result buffer */
	uniop.opr_offset = global->ops_cstate.opc_temprow->opc_olen;

	/* Update temp buffer offset past Unicode operand. */
	align = uniop.opr_len % sizeof (ALIGN_RESTRICT);
	if (align != 0)
	    align = sizeof (ALIGN_RESTRICT) - align;
	global->ops_cstate.opc_temprow->opc_olen += uniop.opr_len + align;
    }

    /* compile the code the convert the data; */
    if (unorm)
    {
	/* Make sure that uniop is set */
	STRUCT_ASSIGN_MACRO(uniop, ops[0]);
    }
    else 
	STRUCT_ASSIGN_MACRO(*resop, ops[0]);
    STRUCT_ASSIGN_MACRO(*srcop, ops[1]);

    /* Check special cases for ints and floats.  (Maybe could add decimal
    ** to this list.)  Other data types do special stuff when coercing,
    ** even when coercing-to-self:  dates get normalized, varchars are
    ** zero padded, etc.
    ** This business of apparently coercing a data type to itself is
    ** usually avoided by callers, but happens fairly often when nullability
    ** is introduced.
    */
    if ( (abs_srctype == DB_INT_TYPE || abs_srctype == DB_FLT_TYPE)
      && abs_srctype == abs_restype)
    {
	/* Check if "coercing" non-nullable into nullable, this can be done
	** with a straight move preceded by a 1CXI_SET_SKIP prefix to clear
	** the result null byte.
	** Also check if "coercing" a type to itself exactly (typically caused
	** by making resop nullable above, because callers usually aren't
	** that stupid); this can be done with a regular move including the
	** null byte.
	*/
	if (srcop->opr_dt > 0 && resop->opr_dt < 0
	  && srcop->opr_len == resop->opr_len-1)
	    fid = ADE_MECOPYN;
	else if (srcop->opr_dt == resop->opr_dt && srcop->opr_len == resop->opr_len)
	    fid = ADE_MECOPY;
    }

    opc_adinstr(global, cadf, fid, seg, 2, ops, 1);

    if (unorm)
    {
	/* Normalize coercion result into final result location. */
	STRUCT_ASSIGN_MACRO(uniop, ops[1]);
	STRUCT_ASSIGN_MACRO(*resop, ops[0]);
	opc_adinstr(global, cadf, ADE_UNORM, seg, 2, ops, 1);
    }
	
}

/*{
** Name: OPC_ADEND	- End a compilation
**
** Description:
{@comment_line@}...
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
**      27-aug-86 (eric)
**          written
**	02-sep-88 (puree)
**	    handle VLUP/VLT, one per CX allowed.
**	7-nov-95 (inkdo01)
**	    Changed to initially allocate CX in local memory, then copy
**	    to QSF memory during opc_adend call.
**    19-feb-96 (inkdo01)
**        Added minor bit of error checking to above change.
**	13-sep-99 (inkdo01)
**	    Added call to "ade_verify_labs" to verify label offsets
**	    in branch instructions.
**	23-aug-04 (inkdo01)
**	    New code to scan all operands in CX changing local buffer
**	    indexes to global array buffer indexes.
**	12-Dec-2005 (kschendel)
**	    Handle copying QEN_ADF to QSF memory here, if necessary.
**	    Also, clean out QEN_ADF pointer in node/ahd if it turns out
**	    that no code was generated.
**	    MEcopy no longer has a size limit, fix here.
[@history_template@]...
*/
VOID
opc_adend(
		OPS_STATE   *global,
		OPC_ADF	    *cadf)
{
    QEF_QP_CB	*qp = global->ops_cstate.opc_qp;
    QEN_ADF	*qadf = cadf->opc_qeadf;
    QEN_ADF	*new_qadf;
    i4	cxsize, size;
    DB_STATUS	err;
    QEN_BASE	*new_base;
    PTR		new_cx;

    /*  If this CX contains user paramter, update number of CX with parameter.
    **  Also, set index to VLUP descriptor in the CX. */
    if (qadf->qen_pcx_idx == -1)
    {
	++qp->qp_pcx_cnt;
	qadf->qen_pcx_idx = qp->qp_pcx_cnt;
    }

    if ((err = ade_end_comp(global->ops_adfcb, qadf->qen_ade_cx, &cxsize)) 
								    != E_DB_OK
	)
    {
	opx_verror(err, E_OP0793_ADE_ENDCOMP,
				global->ops_adfcb->adf_errcb.ad_errcode);
    }

    /* If CX contains labels, call label verification to assure they point
    ** to instrs in same code segment (not to data or some other segment).
    ** This functionality could be added to ade_end_comp, but it gets called 
    ** from unknown (to me) front end modules, and I don't want to track
    ** them down, too. */
    if (cadf->opc_labels) ade_verify_labs(global->ops_adfcb, qadf->qen_ade_cx);

    /* Re-allocate CX and QEN_BASE in qsf memory, then copy 'em over. */
    if (cxsize <= cxhead_size)
    {
	/* oops, nothing there.  Clean away the QEN_ADF pointer. */
	*(cadf->opc_pqeadf) = NULL;	/* Null out QEN_ADF ptr in node */
	return;
    }

    /* Got some stuff here - allocate in QSF, then copy */
    size = DB_ALIGN_MACRO(qadf->qen_sz_base * sizeof(QEN_BASE)) + cxsize;
    if (! cadf->opc_qenfromqsf)
    {
	size = DB_ALIGN_MACRO(size) + sizeof(QEN_ADF);
    }
    new_base = (QEN_BASE *)opu_qsfmem(global, size);
			/* allocate base array */
    MEcopy((PTR)qadf->qen_base, (i4)qadf->qen_sz_base * 
		sizeof(QEN_BASE), (PTR)new_base);
    qadf->qen_base = new_base;
    qadf->qen_max_base = qadf->qen_sz_base;
    new_cx = (PTR) ME_ALIGN_MACRO( (new_base + qadf->qen_sz_base), DB_ALIGN_SZ);
    MEcopy(qadf->qen_ade_cx, (i4)cxsize, new_cx);
    qadf->qen_ade_cx = new_cx;
    if ((err = ade_inform_space(global->ops_adfcb, qadf->qen_ade_cx, cxsize))
              != E_DB_OK)
			/* stick new length back into CX */
    {
      opx_verror(err, E_OP0793_ADE_ENDCOMP,
                              global->ops_adfcb->adf_errcb.ad_errcode);
    }
    if (! cadf->opc_qenfromqsf)
    {
	/* Have to copy over the QEN_ADF as well */
	new_qadf = (QEN_ADF *) ME_ALIGN_MACRO( (new_cx + cxsize), DB_ALIGN_SZ);
	MEcopy((PTR) qadf, sizeof(QEN_ADF), (PTR) new_qadf);
	*(cadf->opc_pqeadf) = new_qadf;		/* Fix up node or ahd */
	qadf = new_qadf;
    }

    /* The following code transforms (if requested) the base array definitions
    ** so that the CX operands directly reference the DSH global row array.
    ** Removing the indirection (through the CX local array to the global
    ** array allows more flexibility in QEF handling of buffers and a 
    ** reduction in the amount of data to be moved around. */
    if (qp->qp_status & QEQP_GLOBAL_BASEARRAY)
    {
	i4	base[20];	/* save memory alloc for most cases */
	i4	*basep = &base[0];
	i4	i;

	if (qadf->qen_sz_base > 20)
	    basep = (i4 *)opu_memory(global, sizeof(i4) * qadf->qen_sz_base);
				/* In the unlikely event that there's 
				** more than 20 entries */
	/* Loop over base array, setting up transform array. */
	for (i = 0; i < qadf->qen_sz_base; i++)
	    switch (new_base[i].qen_array) {
	      case QEN_ROW:
		basep[i] = new_base[i].qen_index;
		break;

	      case QEN_KEY:
		if (qp->qp_key_row == -1)
		    opc_ptrow(global, &qp->qp_key_row, qp->qp_key_sz);
		basep[i] = qp->qp_key_row;
		break;

	      case QEN_DMF_ALLOC:
		opc_ptrow(global, &qadf->qen_input, 0);
		basep[i] = qadf->qen_input;
		break;

	      case QEN_OUT:
		opc_ptrow(global, &qadf->qen_uoutput, 0);
		basep[i] = qadf->qen_uoutput;
		break;

	      case QEN_PARM:
		opc_ptrow(global, &new_base[i].qen_parm_row, 0);
		basep[i] = new_base[i].qen_parm_row;
		break;

	      case QEN_VTEMP:
		/* VTEMP uses base array entry to stow computed length. */
		opc_ptrow(global, &new_base[i].qen_index, 0);
		basep[i] = new_base[i].qen_index;
		break;

	      default:
		basep[i] = -1;	/* signals not to transform */
		break;
	    }

	ade_global_bases(global->ops_adfcb, qadf->qen_ade_cx, basep);
				/* this function transforms the operands 
				** in the CX */
    }

}

/*{
** Name: OPC_ADCALCLEN	- Calculate the length of a datatype
**
** Description:
{@comment_line@}...
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
**      16-sep-86 (eric)
**          written
**	17-jul-89 (jrb)
**	    Proper handling of opr_prec and db_prec fields added.
**	25-jul-89 (jrb)
**	    Extended interface to accept PTRs to actual data of input operands.
**	    This was done so that the result length could be calculated based
**	    on the input values as well as the input types, precisions, and
**	    lengths.  Note that these data pointers may not be valid for
**	    parameters which are not known at this time (i.e. not constants)
**	    but this is a problem left to PSF and ADF.
**	08-Mar-1999 (shreo03)
**	    Support 4 operands in adi_0calclen
**	6-mar-2007 (dougi)
**	    Fix coercions from/to Unicode types to use proper collation IDs.
*/
VOID
opc_adcalclen(
		OPS_STATE	*global,
		OPC_ADF		*cadf,
		ADI_LENSPEC	*lenspec,
		i4		numops,
		ADE_OPERAND	*op[],
		ADE_OPERAND	*resop,
		PTR		data[])
{
    DB_DATA_VALUE   dv[ADI_MAX_OPERANDS];
    DB_DATA_VALUE   dvres;
    DB_DATA_VALUE   *pdv[ADI_MAX_OPERANDS];
    ADE_OPERAND	    ops[ADI_MAX_OPERANDS+2];
    i4		    opno;
    i4		    i;
    DB_STATUS	    err;

    dvres.db_datatype = resop->opr_dt;
    dvres.db_prec = 0;
    dvres.db_length = 0;
    dvres.db_collID = resop->opr_collID;
    dvres.db_data = NULL;

    for (i = 0; i < numops; i++)
    {	
        if (op[i] == NULL)
        {
	    pdv[i] = NULL;
        }
        else
        {
	    dv[i].db_datatype = op[i]->opr_dt;
	    dv[i].db_prec = op[i]->opr_prec;
	    dv[i].db_length = op[i]->opr_len;
	    dv[i].db_collID = op[i]->opr_collID;
	    dv[i].db_data = data[i];
	    pdv[i] = &dv[i];
        }
    }

    if ((err = adi_0calclen(global->ops_adfcb, lenspec, numops, 
		pdv, &dvres)) != E_DB_OK)
    {
	if (global->ops_adfcb->adf_errcb.ad_errcode == E_AD2022_UNKNOWN_LEN)
	{
	    /* This operation results in a VLT (variable length temporary).
	    ** This means that we need to reset some of the fields in resop
	    ** to reflect this.
	    */
	    opc_adbase(global, cadf, QEN_VTEMP, 0);
	    resop->opr_len = dvres.db_length;
	    resop->opr_prec = dvres.db_prec;
	    resop->opr_collID = dvres.db_collID;
	    resop->opr_base = cadf->opc_vtemp_base;
	    resop->opr_offset = 0;

	    /* Now lets set up the operands to compile a calclen instruction
	    ** into the init segment of the current CX.
	    */
	    ops[0].opr_dt = lenspec->adi_lncompute;
	    ops[0].opr_len = lenspec->adi_fixedsize;
	    ops[0].opr_prec = lenspec->adi_psfixed;
	    ops[0].opr_collID = -1;
	    ops[0].opr_base = cadf->opc_vtemp_base;
	    ops[0].opr_offset = 0;

	    STRUCT_ASSIGN_MACRO(*resop, ops[1]);

	    opno = 2;
	    if (numops > 0 && op[0] != NULL)
	    {
		STRUCT_ASSIGN_MACRO(*op[0], ops[opno]);
		opno += 1;
	    }

	    if (numops > 1 && op[1] != NULL)
	    {
		STRUCT_ASSIGN_MACRO(*op[1], ops[opno]);
		opno += 1;
	    }

	    opc_adinstr(global, cadf, ADE_CALCLEN, ADE_SVIRGIN, opno, ops, 2);
	}
	else
	{
	    /* EJLFIX: get a better error no */
	    opx_verror(err, E_OP0793_ADE_ENDCOMP,
				    global->ops_adfcb->adf_errcb.ad_errcode);
	}
    }

    resop->opr_len = dvres.db_length;
    resop->opr_prec = dvres.db_prec;
    resop->opr_collID = dvres.db_collID;
}

/*{
** Name: OPC_ADEMPTY	- Fill in a DB_DATA_VALUE with an empty value.
**
** Description:
{@comment_line@}...
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
**      10-oct-86 (eric)
**          written
[@history_template@]...
**	21-sep-97 (kitch01)
**	    Bug 84651. Since we are filling DB_DATA_VALUE with an empty value
**	    make sure that the memory area pointed to by dv->db_data is really
**	    empty. This bug shows up way down the line because db_data contained
**	    garbage. 
*/
VOID
opc_adempty(
		OPS_STATE	*global,
		DB_DATA_VALUE	*dv,
		DB_DT_ID	type,
		i2		prec,
		i4		length)
{
    DB_STATUS	    err;

    if (dv->db_data == NULL)
    {
	dv->db_data = opu_Gsmemory_get(global, length);
    }
    else
    {
	if (length > dv->db_length)
	    dv->db_data = opu_Gsmemory_get(global, length);
    }
	MEfill(length, (u_char)0, (PTR)dv->db_data);
    dv->db_datatype = type;
    dv->db_length = length;
    dv->db_prec = prec;
    dv->db_collID = -1;

    if ((err = adc_getempty(global->ops_adfcb, dv)) != E_DB_OK)
    {
	/* EJLFIX: get a better error no */
	opx_verror(err, E_OP0793_ADE_ENDCOMP,
				global->ops_adfcb->adf_errcb.ad_errcode);
    }
}

/*{
** Name: OPC_ADKEYBLD	- Build a relation key (directly, not compiled)
**
** Description:
{@comment_line@}...
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
**      31-mar-87 (eric)
**          written
**      1-sep-88 (seputis)
**          passed in qop for escape support
**	25-Jun-2008 (kiria01) SIR120473
**	    Move pst_isescape into new u_i2 pst_pat_flags.
[@history_template@]...
*/
VOID
opc_adkeybld(
		OPS_STATE	    *global,
		i4		    *key_type,
		ADI_OP_ID	    opid,
		DB_DATA_VALUE	    *kdv,
		DB_DATA_VALUE	    *lodv,
		DB_DATA_VALUE	    *hidv,
		PST_QNODE	    *qop)
{
    ADC_KEY_BLK	    keyblk;
    DB_STATUS	    err;

    MEfill(sizeof (keyblk), (u_char)0, (PTR)&keyblk);

    if (qop->pst_sym.pst_value.pst_s_op.pst_pat_flags & AD_PAT_HAS_ESCAPE)
	keyblk.adc_escape = qop->pst_sym.pst_value.pst_s_op.pst_escape;
    keyblk.adc_pat_flags = qop->pst_sym.pst_value.pst_s_op.pst_pat_flags;

    if (kdv == NULL)
    {
	keyblk.adc_kdv.db_data = NULL;
	keyblk.adc_kdv.db_datatype = 0;
	keyblk.adc_kdv.db_length = 0;
	keyblk.adc_kdv.db_prec = 0;
	keyblk.adc_kdv.db_collID = -1;
    }
    else
    {
	STRUCT_ASSIGN_MACRO(*kdv, keyblk.adc_kdv);
    }

    if (lodv == NULL)
    {
	STRUCT_ASSIGN_MACRO(*kdv, keyblk.adc_lokey);
	keyblk.adc_lokey.db_data = NULL;
    }
    else
    {
	STRUCT_ASSIGN_MACRO(*lodv, keyblk.adc_lokey);
    }

    if (hidv == NULL)
    {
	STRUCT_ASSIGN_MACRO(*kdv, keyblk.adc_hikey);
	keyblk.adc_hikey.db_data = NULL;
    }
    else
    {
	STRUCT_ASSIGN_MACRO(*hidv, keyblk.adc_hikey);
    }

    keyblk.adc_opkey = opid;

    if ((err = adc_keybld(global->ops_adfcb, &keyblk)) != E_DB_OK)
    {
	/* EJLFIX: get a better error no */
	opx_verror(err, E_OP0793_ADE_ENDCOMP,
				global->ops_adfcb->adf_errcb.ad_errcode);
    }

    *key_type = keyblk.adc_tykey;
}

/*{
** Name: OPC_ADSEGLANG	- Set a query language in a CX segment.
**
** Description:
**      This routine marks a segment of a CX as using Quel or SQL. This 
**      information gets marked in the OPC_ADF structure. This information 
**      is used during the next instruction that gets compiled into the 
**      given segment to insure that the language is set correctly. If 
**      the language is not set correctly, then a ADE_PMQUEL or a 
**      ADE_PMNOQUEL will get compiled in. See opc_adinstr() for more details. 
[@comment_line@]...
**
** Inputs:
**  global -
**	global state info for this query
**  cadf -
**	The OPC_ADF struct whose compilation has been begun.
**  qlang -
**	The query language id for the segment.
**  seg -
**	The segment that the query language applies to.
**
** Outputs:
**	none
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    causes ADE_PMQUEL or ADE_PMNOQUEL to be compiled into CXs.
**
** History:
**      6-dec-89 (eric)
**          created
[@history_template@]...
*/
VOID
opc_adseglang(
		OPS_STATE	*global,
		OPC_ADF		*cadf,
		DB_LANG		qlang,
		i4		seg)
{
    if ((qlang != DB_SQL && qlang != DB_QUEL) || seg < 1 || seg > ADE_SMAX)
    {
	opx_error(E_OP0887_CONSTTYPE);
    }

    cadf->opc_qlang[seg] = qlang;
}

/*{
** Name: OPC_RESOLVE 	- Find the datatype info for an outer join column.
**
** Description:
**      This routine determines the datatype info based on two outer join
**	columns. Resulting datatype has to be able to respresent data coming
**	from either one of the two join columns.
**
** Inputs:
**	global	-	global state info for this query
**	idv	-	ptr to db_data_value of the inner.
**	odv	-	ptr to odb_data_value of the outer.
**	rdv	-	ptr to db_data_value of the result.
**
** Outputs:
**	rdv	-	initialized db_data_value of the result.
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none.
**
** History:
**      29-may-90 (stec)
**          created
**	03-feb-94 (rickh)
**	    Replaced 9999 diagnostics with real error messages.
**	08-Mar-1999 (shero03)
**	    Support 4 operands
**	16-jul-03 (hayke02)
**	    Pass FALSE to adi_resolve() for new parameter varchar_precedence.
**	    This change fixes bug 109134.
**	15-apr-2007 (dougi)
**	    Wee hack to allow outer joins involving new date types.
**	29-Jun-2007 (kschendel) b122118
**	    Fill in collation ID, otherwise junk propagates and dirties up
**	    op150 output (no other bad effects observed).
**	19-Feb-2010 (kiria01) b123308
**	    ADI_LONGER now handles decimals so ADI_DECBLEND is not needed.
*/
VOID
opc_resolve(
		OPS_STATE	*global,
		DB_DATA_VALUE	*idv,
		DB_DATA_VALUE	*odv,
		DB_DATA_VALUE	*rdv)
{
    ADI_RSLV_BLK	adi_rslv_blk;
    ADI_FI_DESC		*fi_desc;
    ADI_FI_DESC		*best_fidesc;
    ADI_FI_ID		adi_fid;
    DB_STATUS		status;
    DB_DATA_VALUE	leftop;
    DB_DATA_VALUE	rightop;
    DB_DATA_VALUE	tempop;
    i4			i;
    DB_DATA_VALUE	*opptrs[ADI_MAX_OPERANDS];
    DB_DT_ID		dts[ADI_MAX_OPERANDS];
    ADI_LENSPEC		lenspec;
    bool		nullable;

    /* Before doing anything, see if resolution is even needed. */
    if (abs(idv->db_datatype) == abs(odv->db_datatype))
    {
	i4	ol, il;

	if (idv->db_datatype < 0)
	    il = idv->db_length-1;
	else il = idv->db_length;

	if (odv->db_datatype < 0)
	    ol = odv->db_length-1;
	else ol = odv->db_length;

	if (il == ol)
	{
	    /* Same types, same lengths, no coercion needed. */
	    STRUCT_ASSIGN_MACRO(*idv, *rdv);
	    if (!(idv->db_datatype == odv->db_datatype ||
		idv->db_datatype < 0))
	    {
		rdv->db_length++;
		rdv->db_datatype = -rdv->db_datatype;
	    }
	    return;
	}
    }

    /* set up to call ADF to resolve this node */
    adi_rslv_blk.adi_op_id   = ADI_FIND_COMMON;
    adi_rslv_blk.adi_num_dts = 2;
    adi_rslv_blk.adi_dt[0]   = odv->db_datatype;
    adi_rslv_blk.adi_dt[1]   = idv->db_datatype;

    status = adi_resolve(global->ops_adfcb, &adi_rslv_blk, FALSE);
    if (status != E_DB_OK)
    {
	/* FIXME */
	opx_error( E_OP079B_ADI_RESOLVE );
    }

    /* we now have the "best" fi descriptor */
    best_fidesc = adi_rslv_blk.adi_fidesc;
    
    /*
    ** In some cases coercion per se may not be needed, but
    ** data type may have to be changed to nullable.
    */
 
    status = adi_res_type(global->ops_adfcb, best_fidesc, 
    		adi_rslv_blk.adi_num_dts, adi_rslv_blk.adi_dt,
	        &rdv->db_datatype);
    if (status != E_DB_OK)
    {
	/* FIXME */
	opx_error( E_OP079B_ADI_RESOLVE );
    }

    nullable = (rdv->db_datatype < 0);

    /* set the result datatype for result */
    if (nullable == TRUE)
    {
	rdv->db_datatype = -best_fidesc->adi_dt[0];
    }
    else
    {
	rdv->db_datatype = best_fidesc->adi_dt[0];
    }

    /* FIXME don't leave collation ID as garbage, but it's not
    ** at all clear to me what it should be.  For now just take
    ** the outer.
    */
    rdv->db_collID = odv->db_collID;

    /*
    ** At this point we have result datatype and its nullability set. Now let
    ** us figure out the result length.  First we must get the lengths of the
    ** operands after conversion, then we must use these lengths to figure out
    ** the length of the result.
    */
 
    /* Assume no coercion for now */
    STRUCT_ASSIGN_MACRO(*odv, leftop);
    STRUCT_ASSIGN_MACRO(*idv, rightop);

    /* Get coercion id for the outer, if needed */
    if ((best_fidesc->adi_dt[0] != abs(odv->db_datatype)) &&
	(best_fidesc->adi_dt[0] != DB_ALL_TYPE))
    {
	status = adi_ficoerce(global->ops_adfcb, odv->db_datatype,
	    best_fidesc->adi_dt[0], &adi_fid);
	if (status != E_DB_OK)
	{
	    /* FIXME */
	    opx_error( E_OP0783_ADI_FICOERCE );
	}

	/* Look up coercion in function instance table */
	status = adi_fidesc(global->ops_adfcb, adi_fid, &fi_desc);
	if (status != E_DB_OK)
	{
	    /* FIXME */
	    opx_error( E_OP0781_ADI_FIDESC );
	}

	if (nullable == TRUE)
	{
	    tempop.db_datatype = -best_fidesc->adi_dt[0];
	}
	else
	{
	    tempop.db_datatype = best_fidesc->adi_dt[0];
	}

	opptrs[0] = &leftop;
	/* Get length after coercion */
	status = adi_0calclen(global->ops_adfcb, &fi_desc->adi_lenspec, 1,
	    opptrs, &tempop);
	if (status != E_DB_OK 
	    && global->ops_adfcb->adf_errcb.ad_errcode != E_AD2022_UNKNOWN_LEN)
	{
	    /* FIXME */
	    opx_error( E_OP0782_ADI_CALCLEN );
	}

	STRUCT_ASSIGN_MACRO(tempop, leftop);
    }

    /* Get coercion id for the inner, if needed */
    if ((best_fidesc->adi_dt[1] != abs(idv->db_datatype)) &&
	(best_fidesc->adi_dt[1] != DB_ALL_TYPE))
    {
	status = adi_ficoerce(global->ops_adfcb, idv->db_datatype,
	    best_fidesc->adi_dt[1], &adi_fid);
	if (status != E_DB_OK)
	{
	    /* FIXME */
	    opx_error( E_OP0783_ADI_FICOERCE );
	}

	/* Look up coercion in function instance table */
	status = adi_fidesc(global->ops_adfcb, adi_fid, &fi_desc);
	if (status != E_DB_OK)
	{
	    /* FIXME */
	    opx_error( E_OP0781_ADI_FIDESC );
	}

	if (nullable == TRUE)
	{
	    tempop.db_datatype = -best_fidesc->adi_dt[1];
	}
	else
	{
	    tempop.db_datatype = best_fidesc->adi_dt[1];
	}

	opptrs[0] = &rightop;
	/* Get length after coercion */
	status = adi_0calclen(global->ops_adfcb, &fi_desc->adi_lenspec, 1,
	    opptrs, &tempop);
	if (status != E_DB_OK
	    && global->ops_adfcb->adf_errcb.ad_errcode != E_AD2022_UNKNOWN_LEN)
	{
	    /* FIXME */
	    opx_error( E_OP0782_ADI_CALCLEN );
	}

	STRUCT_ASSIGN_MACRO(tempop, rightop);
    }

    lenspec.adi_lncompute = ADI_LONGER;
    lenspec.adi_fixedsize = 0;
    lenspec.adi_psfixed	= 0;

    opptrs[0] = &leftop;
    opptrs[1] = &rightop;
    status = adi_0calclen(global->ops_adfcb, &lenspec, 2, opptrs, rdv);

    if (status != E_DB_OK	
	&& global->ops_adfcb->adf_errcb.ad_errcode != E_AD2022_UNKNOWN_LEN)
    {
	/* FIXME */
	opx_error( E_OP0782_ADI_CALCLEN );
    }

    return;
}
