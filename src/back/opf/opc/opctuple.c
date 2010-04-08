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
#include    <adfops.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <scf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <qefmain.h>
#include    <qefnode.h>
#include    <qefact.h>
#include    <qefqp.h>
#include    <qefqeu.h>
#include    <me.h>
#include    <bt.h>
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
#define        OPC_TUPLE	TRUE
#include    <opxlint.h>
#include    <opclint.h>

/**
**
**  Name: OPCTUPLE.C - Routines that play with tuples
**
**  Description:
{@comment_line@}...
**
{@func_list@}...
**
**
**  History:    
**      26-aug-86 (eric)
**          written
**	18-jul-89 (jrb)
**	    Initialized precision fields properly for decimal project.
**	24-may-90 (stec)
**	    Added checks to use OPC_EQ, OPC_ATT contents only when filled in.
**	20-jun-90 (stec)
**	    Fixed a potential problem related to the fact that BTcount()
**	    in opc_materialize(), opc_emsize() was counting bits in the whole bitmap.
**	    Modified opc_eminstrs(), opc_ecinstrs() to remove the initial 
**	    check testing if OPC_ATT struct is filled in. Fixes bug 30901.
**	29-jan-91 (stec)
**	    Modified interface to opc_materialize().
**	25-feb-92 (rickh)
**	    Modified opc_eminstrs to call opc_getFattDataType when processing
**	    a function attribute.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**      10-nov-95 (inkdo01)
**	    Changes to support replacement of QEN_ADF instances by pointers.
**	01-oct-2001 (devjo01)
**	    Fixes for b105914.  (64 bit porting problems with QEF hash code).
**	17-dec-04 (inkdo01)
**	    Added various collID's.
**	10-may-05 (toumi01) problem INGSRV3289 bug 114467.
**	    Based on work on issue 14138885 it turns out that the qentjoin.c
**	    change to test for ADE_BOTHNULL is incorrect, in that it "fixes"
**	    the query reported by issue 14110170 but not the broader case.
**	    Instead, in opctuple.c choose ADE_COMPARE vs. ADE_BYCOMPARE based
**	    on the nullability of the argument tuples, not the theoretical
**	    nullability of the entire equivalence class, which can be turned
**	    off by inclusion of a not-nullable tuple that is not involved in
**	    the immediate comparison.
**	23-may-05 (toumi01)
**	    Back out above change re BYCOMPARE vs. COMPARE, which is not a
**	    correct general fix and breaks some non-outerjoin queries.
**	3-Jun-2009 (kschendel) b122118
**	    Minor cleanup: delete unused adbase parameters.
[@history_template@]...
**/

/*{
** Name: OPC_MATERIALIZE	- Materialize a given set of attributes.
**
** Description:
{@comment_line@}...
**
** Inputs:
**	alloc_trow	-   When TRUE a new row is to be allocated. This will
**			    be the row into which data is to be materialized.
**			    Otherwise, the row number passed in via prowno is
**			    to be used, i.e., caller will determine into which
**			    row the data is to be moved. We need this capability
**			    for hold file sharing.
**	projone		- When TRUE and there's nothing to materialize, 
**			generate code to materialize the constant 1 to assure
**			that something is there. This is for exchange nodes
**			to pass some indication of rows being read in some
**			circumstances (e.g. count(*)) when nothing else is
**			materialized.
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
**      26-aug-86 (eric)
**          written
**	14-nov-86 (daved)
**	    set output row for materialized row (qen_output).
**	24-may-90 (stec)
**	    Modified test for opc_eqavailable combined with
**	    opc_attavailable. Since opc_eqavailable is essentially
**	    boolean in its nature and its value is checked
**	    before the mentioned test, I have removed it. In
**	    other words the code now checks if opc_eqavailable
**	    is FALSE and continues if so, else it checks
**	    opc_attavailable rather than checking both fields
**	    again.
**	20-jun-90 (stec)
**	    Fixed a potential problem related to the fact that BTcount()
**	    in opc_jqual was counting bits in the whole bitmap. Number
**	    of bits counted is determined by ops_eclass.ope_ev in the
**	    subquery struct.
**	16-jan-91 (stec)
**	    Added missing opr_prec initialization.
**	29-jan-91 (stec)
**	    Added the "alloc_drow" (allocate destination row) to the interface.
**	    This determines if the destination DSH row is to be allocated by
**	    this routine.
**      10-nov-95 (inkdo01)
**	    Changes to support replacement of QEN_ADF instances by pointers.
**	21-nov-00 (inkdo01)
**	    Add actual datatype to MECOPY operands for completeness.
**	03-oct-01 (inkdo01)
**	    A little tidying up (removing ineffectual tests, etc.).
**	31-dec-04 (inkdo01)
**	    Support projone parameter for exchange nodes.
[@history_template@]...
*/
VOID
opc_materialize(
		OPS_STATE	*global,
		OPC_NODE	*cnode,
		QEN_ADF		**qadf,
		OPE_BMEQCLS	*eqcmp,
		OPC_EQ		*ceq,
		i4		*prowno,
		i4		*prowsz,
		i4		align_tup,
		bool		alloc_drow,
		bool		projone)
{
    OPS_SUBQUERY	*subqry = global->ops_cstate.opc_subqry;
    QEF_QP_CB		*qp = global->ops_cstate.opc_qp;
    ADE_OPERAND		ops[2];
    i4			ninstr;
    i4			rowno;
    i4			rowsz;
    OPE_IEQCLS		eqcno;
    i4			align;
    i4			nops;
    OPC_ADF		cadf;
    bool		noeqcs = FALSE;

    /* Is there anything to materialize.*/
    if ((ninstr = BTcount((char *)eqcmp, (i4)subqry->ops_eclass.ope_ev)) == 0)
    {
	*qadf = (QEN_ADF *)NULL;
	if (projone)
	{
	    ninstr = 1;
	    noeqcs = TRUE;	/* set flag, then project a 1 */
	}
	else return;
    }

    /* Begin the adf */
    nops = ninstr * 2;
    opc_adalloc_begin(global, &cadf, qadf, ninstr, nops, 0, (i4) 0, 
	cnode->opc_below_rows, OPC_STACK_MEM);

    /* Choose a row to put the materialization into and init the row size */
    rowsz = 0;
    if (alloc_drow == TRUE)
	rowno = qp->qp_row_cnt;
    else
	rowno = *prowno;

    /* put the destination row in spot ADE_ZBASE + 0; */
    opc_adbase(global, &cadf, QEN_ROW, rowno);

    if (!noeqcs)
     for (eqcno = 0; eqcno < subqry->ops_eclass.ope_ev; eqcno += 1)
    {
	if (BTtest((i4)eqcno, (char *)eqcmp) == FALSE)
	{
	    ceq[eqcno].opc_eqavailable = FALSE;
	    ceq[eqcno].opc_attno = OPZ_NOATTR;
	    continue;
	}
	if (ceq[eqcno].opc_eqavailable == FALSE)
	{
	    /*
	    ** No error is required here, since we are 
	    ** materializing the intersection of eqcmp and ceq
	    */
	    continue;
	}

	if (ceq[eqcno].opc_attavailable != TRUE)
	{
	    opx_error(E_OP0888_ATT_NOT_HERE);
	}

	/* if (the row for the current att isn't in the row to base map) */
	if (cadf.opc_row_base[ceq[eqcno].opc_dshrow] < ADE_ZBASE ||
		cadf.opc_row_base[ceq[eqcno].opc_dshrow] >= 
						ADE_ZBASE + (*qadf)->qen_sz_base
	    )
	{
	    /* add the row to the base array and give it a mapping */
	    opc_adbase(global, &cadf, QEN_ROW, ceq[eqcno].opc_dshrow);
	}

	/* compile the instruction to move the att into the destination row */
	ops[0].opr_len = ceq[eqcno].opc_eqcdv.db_length;
	ops[0].opr_prec = ceq[eqcno].opc_eqcdv.db_prec;
	ops[0].opr_collID = ceq[eqcno].opc_eqcdv.db_collID;
	ops[0].opr_base = cadf.opc_row_base[rowno];
	ops[0].opr_offset = rowsz;
	ops[0].opr_dt = ceq[eqcno].opc_eqcdv.db_datatype;
		/* be tidy and record type, even for MECOPY */
	STRUCT_ASSIGN_MACRO(ops[0], ops[1]);
	ops[1].opr_base = cadf.opc_row_base[ceq[eqcno].opc_dshrow];
	ops[1].opr_offset = ceq[eqcno].opc_dshoffset;
	opc_adinstr(global, &cadf, ADE_MECOPY, ADE_SMAIN, 2, ops, 1);

	/*
	** Update the dshrow and dshoffset for the current
	** att to reflect where the att now currently sits
	*/
	ceq[eqcno].opc_dshrow = rowno;
	ceq[eqcno].opc_dshoffset = rowsz;

	/*
	** Add space to the row so that the next att won't 
	** stomp on this att and so that it will be aligned
	*/
	if (align_tup == TRUE)
	{
	    align = ops[0].opr_len % sizeof (ALIGN_RESTRICT);
	    if (align != 0)
		align = sizeof (ALIGN_RESTRICT) - align;
	}
	else
	{
	    align = 0;
	}
	rowsz += ops[0].opr_len + align;
    }
    else
    {
	/* Nothing to materialize, but projone flag is set. Project
	** a 1 into the result buffer. */
	DB_DATA_VALUE	intdv;
	i1	one = 1;

	intdv.db_datatype = DB_INT_TYPE;
	intdv.db_length = 1;
	intdv.db_prec = 0;
	intdv.db_collID = -1;
	intdv.db_data = (char *)&one;

	opc_adconst(global, &cadf, &intdv, &ops[1], DB_SQL, ADE_SINIT);
					/* allocate int 0 constant */

	STRUCT_ASSIGN_MACRO(ops[1], ops[0]);
	ops[0].opr_base = cadf.opc_row_base[rowno];
	ops[0].opr_offset = 0;
	opc_adinstr(global, &cadf, ADE_MECOPY, ADE_SMAIN, 2, ops, 1);
	rowsz = sizeof(ALIGN_RESTRICT);
    }

    /*
    ** If a row was allocated by this routine
    ** add it to the row list in the cnode.
    */
    if (alloc_drow == TRUE)
    {
	opc_ptrow(global, &rowno, rowsz);
	*prowsz = rowsz;
	*prowno = rowno;
    }
    else
    {
	if (rowsz > *prowsz)
	    opx_error(E_OP08A1_TOO_SHORT);
    }

    /* set output row number */
    (*qadf)->qen_output = rowno;

    /* close the adf; */
    opc_adend(global, &cadf);
}

/*{
** Name: OPC_HASH_MATERIALIZE	- Materialize a given set of attributes for 
**			a hash join.
**
** Description:	This function materializes a build or probe row in
**		preparation for a hash join. It is essentially the same
**		as opc_materialize except for the following:
**		  - it performs the ADF hash function on the join key
**		    columns and stores the result at the beginning of
**		    the output buffer,
**		  - it materializes the key columns ahead of the 
**		    remaining columns from the join source using common
**		    attr type (so hash works)
**
{@comment_line@}...
**
** Inputs:
**	qadf		- addr of QEN_ADF ptr in hash node where 
**			  materialization code will be built
**	eqcmp		- bit map of attributes to materialize
**	keqcmp		- bit map of key attributes
**	ceq		- addr of info array for attrs
**	cmplist		- base addr of key compare-list array
**	oj		- TRUE if left, right or full join
**
** Outputs:
**	prowno		- number of materialization buffer
**	prowsz		- size of materialized row
**
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**	26-feb-99 (inkdo01)
**	    Cloned from opc_materialize for hash join support.
**	7-may-99 (inkdo01)
**	    Project key columns using merged attr descriptors.
**	26-nov-99 (inkdo01)
**	    Permit any number of key columns, but if more than 1 they are
**	23-dec-99 (inkdo01)
**	    Make hash result nullable, in case key cols are null.
**	    treated as a single contiguous "char".
**	24-nov-00 (inkdo01)
**	    Use MECOPYN for nullable operands to leave data portion of
**	    target 0's (as initted prior to ADF call) so hash is repeatable
**	    on null values.
**	20-mar-01 (inkdo01)
**	    Minor fix to add precision to single key decimal join cols.
**	22-mar-01 (inkdo01)
**	    Fix alignment of join key to handle different ALIGN_RESTRICT's.
**	23-aug-01 (devjo01)
**	    Rework previous fix to work with 64 bit platforms. b105598.
**	01-oct-2001 (devjo01)
**	    Take advantage of move & rename of QEN_HASH_DUP to remove
**	    stand-in structure introduced with previous version.
**	03-oct-01 (inkdo01)
**	    A little tidying up (removing ineffectual tests, etc.).
**	9-oct-01 (inkdo01)
**	    Fix varchar/text = varchar/text joins by trimming first (bug 106003).
**	14-dec-01 (inkdo01)
**	    Do same fix for NVCHR.
**	11-sep-02 (inkdo01)
**	    Retract 106003 fix for TEXT type. Trailing blanks are insignificant for 
**	    varchar, but NOT for text.
**	13-feb-03 (inkdo01)
**	    Coerce date columns from date to date (rather than plain ol' MECOPY)
**	    to allow status bit normalization so same values hash together (fixes
**	    109553).
**	18-feb-03 (inkdo01)
**	    Add TEXT to the list of types that must be coerced prior to hash.
**	11-Nov-2005 (schka24)
**	    Always allocate the outer-join byte, makes it easier for
**	    QEF to assume that there's something there.
**	7-Dec-2005 (kschendel)
**	    We're building a compare-list now instead of att descriptors,
**	    adjust here.
**	07-Aug-2006 (gupsh01)
**	    Added case for ANSI date/time types.
**	17-Aug-2006 (kibro01) Bug 116481
**	    Mark that opc_adtransform is not being called from pqmat
**	22-Nov-2006 (kschendel) SIR 122512
**	    Don't generate the final HASH opcode, join will do it now
**	    (using a different algorithm to avoid join/partition resonance).
**	 1-feb-07 (hayke02)
**	    Do not use the trimmed varchar and nvarchar buffers higher up the
**	    query tree. This prevents the trimmed values being used in the
**	    result set, or joins after the hash join. This change fixes bug
**	    116239.
**	22-Mar-07 (wanfr01)
**	    Bug 117979
**	    Need third parameter for adi_resolve
**	 2-oct-07 (hayke02)
**	    Modify the fix for bug 116239 so we pass up the modified att buffer
**	    when coerce is TRUE. This allows coerced att buffers to be used
**	    higher up the query tree. This change fixes bug 119224.
**	7-May-2008 (kschendel) SIR 122512
**	    Remove outer-join flags, they are now part of the row header.
**	23-Jun-2008 (kschendel) SIR 122512
**	    Force alignment to multiple of "hash dup align".
**	30-jul-08 (hayke02)
**	    Modify the fix for 116239 so that we now do continue for varchar
**	    and nvarchar eqcs if keysdone is TRUE, the eqc is in the keqcmp
**	    and now if oj is TRUE, indicating that we are materializing rows
**	    for a left, right or full join. This prevents incorrect results
**	    or E_SC039B errors due to bad dsh row and offset values. This
**	    change fixes bug 120656.
**	 6-aug-08 (hayke02)
**	    Modify previous fix so that it is only applied to non-OPC_COTOP
**	    nodes. This prevents diffs in qp346.sep and qp400.sep.
**	17-sep-08 (hayke02)
**	    Back out fixes for bug 120656 and instead fix in opj_exact() by
**	    switching off hash joins for OJs on varchar/nvarchar atts.
**	22-sept-2008 (dougi)
**	    Adjust Keith's fix to correct problems in bugs 116239, 119224 and 
**	    120656.
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	11-Mar-2010 (kschendel) SIR 123275
**	    Take ANSIDATE out of the coercion-needed list.  Unlike ingresdates,
**	    ansidates are pure dates with no time part, and there's nothing
**	    to normalize.  (The ansidate-to-ansidate coercion ends up just
**	    moving bits around and wasting CPU time, but changes nothing.)
*/
VOID
opc_hash_materialize(
		OPS_STATE	*global,
		OPC_NODE	*cnode,
		QEN_ADF		**qadf,
		OPE_BMEQCLS	*eqcmp,
		OPE_BMEQCLS	*keqcmp,
		OPC_EQ		*ceq,
		DB_CMP_LIST	*cmplist,
		i4		*prowno,
		i4		*prowsz,
		bool		oj)
{
    OPS_SUBQUERY	*subqry = global->ops_cstate.opc_subqry;
    QEF_QP_CB		*qp = global->ops_cstate.opc_qp;
    ADE_OPERAND		ops[2];
    DB_CMP_LIST		*cmp;
    ADI_FI_DESC		*vchfidp, *nvchfidp;
    ADI_RSLV_BLK	adi_rslv_blk;
    i4			opcode;
    i4			ninstr;
    i4			rowno;
    i4			rowsz, keystart, keysz;
    OPE_IEQCLS		eqcno;
    i4			nops;
    i4			i;
    OPC_ADF		cadf;
    DB_STATUS		status;
    bool		keysdone = FALSE;
    bool		coerce;

    /* Is there anything to materialize.*/
    if (BTcount((char *)eqcmp, (i4)subqry->ops_eclass.ope_ev) == 0)
    {
	*qadf = (QEN_ADF *)NULL;
	return;
    }

    /* Bits to init. */
    vchfidp = nvchfidp = NULL;	/* show adi_resolve calls not done */
    adi_rslv_blk.adi_num_dts = 1;
    adi_rslv_blk.adi_op_id = 0;
    adi_rslv_blk.adi_fidesc = (ADI_FI_DESC *) NULL;

    /* Begin the adf */
    ninstr = BTcount((char *)eqcmp, (i4)subqry->ops_eclass.ope_ev) + 1;
    nops = ninstr * 2;
    opc_adalloc_begin(global, &cadf, qadf, ninstr, nops, 0, (i4) 0, 
	cnode->opc_below_rows, OPC_STACK_MEM);

    /* Choose a row to put the materialization into, init the row size and 
    ** align the start of the key area.  Need to allow enough space
    ** for hash result and a duplicate value chain ( see QEF_HASH_DUP )
    */
    rowsz = QEF_HASH_DUP_SIZE;		/* Size of just the row header */
    keystart = rowsz;	
    rowno = qp->qp_row_cnt;

    /* put the destination row in spot ADE_ZBASE + 0; */
    opc_adbase(global, &cadf, QEN_ROW, rowno);
	
    for ( ; ; )	/* Outer loop forces keys to materialize first */
    {
	for (eqcno = 0, i = 0; eqcno < subqry->ops_eclass.ope_ev; eqcno += 1)
	{
	    /* Process key attrs on first pass (!keysdone) and ignore
	    ** them on second pass (keysdone). */

	    if (!keysdone && BTtest((i4)eqcno, (char *)keqcmp) == FALSE) 
						continue;
	    else if (keysdone && BTtest((i4)eqcno, (char *)keqcmp) == TRUE
		    &&
		    abs(ceq[eqcno].opc_eqcdv.db_datatype) != DB_VCH_TYPE
		    &&
		    abs(ceq[eqcno].opc_eqcdv.db_datatype) != DB_NVCHR_TYPE)
						continue;
	    if (BTtest((i4)eqcno, (char *)eqcmp) == FALSE)
	    {
		ceq[eqcno].opc_eqavailable = FALSE;
		ceq[eqcno].opc_attno = OPZ_NOATTR;
		continue;
	    }

	    if (ceq[eqcno].opc_eqavailable == FALSE)
	    {
		/*
		** No error is required here, since we are 
		** materializing the intersection of eqcmp and ceq
		*/
		continue;
	    }

	    if (ceq[eqcno].opc_attavailable != TRUE)
	    {
		opx_error(E_OP0888_ATT_NOT_HERE);
	    }

	    /* if (the row for the current att isn't in the row to base map) */
	    if (cadf.opc_row_base[ceq[eqcno].opc_dshrow] < ADE_ZBASE ||
		cadf.opc_row_base[ceq[eqcno].opc_dshrow] >= 
						ADE_ZBASE + (*qadf)->qen_sz_base
	    )
	    {
		/* add the row to the base array and give it a mapping */
		opc_adbase(global, &cadf, QEN_ROW, ceq[eqcno].opc_dshrow);
	    }

	    /* compile the instruction to move the att into the dest row */
	    coerce = FALSE;
	    opcode = 0;
	    if (!keysdone)
	    {
		DB_DT_ID	keytype;

		/* If this is a key, use the normalized key descriptor to
		** project. */
		cmp = &cmplist[i++];		/* Comparison descriptor */
		ops[0].opr_dt = cmp->cmp_type;
		ops[0].opr_len = cmp->cmp_length;
		ops[0].opr_prec = cmp->cmp_precision;
		ops[0].opr_collID = cmp->cmp_collID;
		keytype = abs(cmp->cmp_type);
		/* Various data types have idiosyncracies that require type to
		** type coercions to remedy. For example, TEXT columns may 
		** contain unpredictable residue past the length-defined 
		** portion of the value. The coercion sends it through code 
		** that cleans it up. */
		if (cmp->cmp_type != ceq[eqcno].opc_eqcdv.db_datatype ||
		    cmp->cmp_length != ceq[eqcno].opc_eqcdv.db_length ||
		    keytype == DB_CHR_TYPE || keytype == DB_VBYTE_TYPE ||
		    keytype == DB_DTE_TYPE || keytype == DB_TXT_TYPE ||
		    keytype == DB_TME_TYPE ||
		    keytype == DB_TMWO_TYPE || keytype == DB_TMW_TYPE ||
		    keytype == DB_TSWO_TYPE || keytype == DB_TSW_TYPE ||
		    keytype == DB_TSTMP_TYPE || keytype == DB_INYM_TYPE ||
		    keytype == DB_INDS_TYPE || keytype == DB_DEC_TYPE)
		 coerce = TRUE;
		/* Check for varchar join columns and force a "trim" 
		** to strip trailing blanks for hash. Otherwise, these guys
		** may not join even though they should. */
		if (keytype == DB_VCH_TYPE || keytype == DB_NVCHR_TYPE)
		{
		    if (adi_rslv_blk.adi_op_id == 0)
			status = adi_opid(global->ops_adfcb, "trim", 
				&adi_rslv_blk.adi_op_id);
		    if (keytype == DB_VCH_TYPE)
		    {
			if (vchfidp == NULL)
			{
			    adi_rslv_blk.adi_dt[0] = DB_VCH_TYPE;
			    adi_rslv_blk.adi_fidesc = NULL;
			    status = adi_resolve(global->ops_adfcb, &adi_rslv_blk, FALSE);
			    vchfidp = adi_rslv_blk.adi_fidesc;
			}
			opcode = vchfidp->adi_finstid;
		    }
		    else if (keytype == DB_NVCHR_TYPE)
		    {
			if (nvchfidp == NULL)
			{
			    adi_rslv_blk.adi_dt[0] = DB_NVCHR_TYPE;
			    adi_rslv_blk.adi_fidesc = NULL;
			    status = adi_resolve(global->ops_adfcb, &adi_rslv_blk, FALSE);
			    nvchfidp = adi_rslv_blk.adi_fidesc;
			}
			opcode = nvchfidp->adi_finstid;
		    }
		}
	    }
	    else
	    {
		/* Non-key - just get attr info from row descriptor. */
		ops[0].opr_dt = ceq[eqcno].opc_eqcdv.db_datatype;
		ops[0].opr_len = ceq[eqcno].opc_eqcdv.db_length;
		ops[0].opr_prec = ceq[eqcno].opc_eqcdv.db_prec;
		ops[0].opr_collID = ceq[eqcno].opc_eqcdv.db_collID;
	    }
	    ops[0].opr_base = cadf.opc_row_base[rowno];
	    ops[0].opr_offset = rowsz;
	    ops[1].opr_base = cadf.opc_row_base[ceq[eqcno].opc_dshrow];
	    ops[1].opr_offset = ceq[eqcno].opc_dshoffset;
	    ops[1].opr_dt = ceq[eqcno].opc_eqcdv.db_datatype;
	    ops[1].opr_len = ceq[eqcno].opc_eqcdv.db_length;
	    ops[1].opr_prec = ceq[eqcno].opc_eqcdv.db_prec;
	    ops[1].opr_collID = ceq[eqcno].opc_eqcdv.db_collID;
	    if (coerce)
	     /* Transformation is necessary */
	     opc_adtransform(global, &cadf, &ops[1], &ops[0], ADE_SMAIN, 
						(bool)FALSE, (bool)FALSE);
	    if (!coerce || opcode != 0)
	    {
		/* If no coercion, just copy - or possibly throw in a
		** "trim" if source is varchar or text. */

		if (opcode == 0)
		 if (!keysdone && ops[1].opr_dt < 0) 
		    opcode = ADE_MECOPYN;	/* NULLable MECOPY */
		 else opcode = ADE_MECOPY;
		opc_adinstr(global, &cadf, opcode, ADE_SMAIN, 2, ops, 1);
	    }

	    /*
	    ** Update the dshrow and dshoffset for the current
	    ** att to reflect where the att now currently sits
	    */
	    if (keysdone || BTtest((i4)eqcno, (char *)eqcmp) == FALSE ||
		coerce ||
		(abs(ceq[eqcno].opc_eqcdv.db_datatype) != DB_VCH_TYPE &&
		 abs(ceq[eqcno].opc_eqcdv.db_datatype) != DB_NVCHR_TYPE))
	    {
		ceq[eqcno].opc_dshrow = rowno;
		ceq[eqcno].opc_dshoffset = rowsz;
	    }
	    if (!keysdone)
	    {
		/* If key column, update attr description, too. */
		ceq[eqcno].opc_eqcdv.db_datatype = cmp->cmp_type;
		ceq[eqcno].opc_eqcdv.db_length = cmp->cmp_length;
		ceq[eqcno].opc_eqcdv.db_prec = cmp->cmp_precision;
		ceq[eqcno].opc_eqcdv.db_collID = cmp->cmp_collID;
		/* And stash offset into the compare-list.  This
		** ends up happening twice, once for build and once
		** for probe, but the offsets are necessarily the same
		** for both sides!
		*/
		cmp->cmp_offset = rowsz - keystart;
	    }

	    rowsz += ops[0].opr_len;
	}	/* end of eqcmp loop */

	if (keysdone) break;
	keysz = rowsz - keystart;
	keysdone = TRUE;
    }	/* end of outer loop - drives inner loop twice */

    /* Don't generate the actual hash opcode, hash-join has its
    ** own hashing ideas...
    ** Do however force alignment of the row size to a multiple required
    ** by the row length in the QEF_HASH_DUP header, which is maintained
    ** in units of QEF_HASH_DUP_ALIGN.
    */

    rowsz = DB_ALIGNTO_MACRO(rowsz,QEF_HASH_DUP_ALIGN);
				/* align the buffer */
    opc_ptrow(global, &rowno, rowsz);
    *prowsz = rowsz;
    *prowno = rowno;

    /* set output row number */
    (*qadf)->qen_output = rowno;

    /* close the adf; */
    opc_adend(global, &cadf);
}

/*{
** Name: OPC_KCOMPARE	- Compile adf code to compare keys.
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
**      10-nov-95 (inkdo01)
**	    Changes to support replacement of QEN_ADF instances by pointers.
[@history_template@]...
*/
VOID
opc_kcompare(
		OPS_STATE	*global,
		OPC_NODE	*cnode,
		QEN_ADF		**qadf,
		OPO_ISORT	eqcno,
		OPC_EQ		*ceq1,
		OPC_EQ		*ceq2)
{
    i4			ninstr;
    i4			nops;
    OPC_ADF		cadf;

    /* if there isn't anything to compare, then don't */
    if (eqcno == OPE_NOEQCLS)
    {
	(*qadf) = (QEN_ADF *)NULL;
    }
    else
    {
	/* Begin the adf; */
	ninstr = nops = 0;
	opc_ecsize(global, eqcno, &ninstr, &nops);
	opc_adalloc_begin(global, &cadf, qadf, ninstr, nops, 0, (i4) 0, 
	    				cnode->opc_below_rows, OPC_STACK_MEM);

	opc_ecinstrs(global, cnode, &cadf, eqcno, ceq1, ceq2);

	/* close the adf; */
	opc_adend(global, &cadf);
    }
}

/*{
** Name: OPC_ECSIZE	- Figure out the CX size to compare an eq class.
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
**      9-apr-87 (eric)
**          written
[@history_template@]...
*/
VOID
opc_ecsize(
		OPS_STATE	*global,
		OPO_ISORT	eqcno,
		i4		*ninstr,
		i4		*nops)
{
    OPS_SUBQUERY	*subqry = global->ops_cstate.opc_subqry;
    i4			neqcs;
    OPO_EQLIST		*eqlist;

    if (eqcno >= subqry->ops_eclass.ope_ev)
    {
	eqlist = subqry->ops_msort.opo_base->
	    opo_stable[eqcno-subqry->ops_eclass.ope_ev]->opo_eqlist;
	for (neqcs = 0; eqlist->opo_eqorder[neqcs] != OPE_NOEQCLS; neqcs += 1)
	{
	    /* Null body */
	}
    }
    else
    {
	neqcs = 1;
    }

    *ninstr += 2 * neqcs;
    *nops += neqcs * 2;
}

/*{
** Name: OPC_EMSIZE	- Figure out the CX size to compare the EQs in a map.
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
**      9-apr-87 (eric)
**          written
**	20-jun-90 (stec)
**	    Fixed a potential problem related to the fact that BTcount()
**	    in opc_jqual was counting bits in the whole bitmap. Number
**	    of bits counted is determined by ops_eclass.ope_ev in the
**	    subquery struct.
[@history_template@]...
*/
VOID
opc_emsize(
		OPS_STATE	*global,
		OPE_BMEQCLS	*eqcmp,
		i4		*ninstr,
		i4		*nops)
{
    OPS_SUBQUERY    *subqry = global->ops_cstate.opc_subqry;
    i4		    neqcs;

    neqcs = BTcount((char *)eqcmp, (i4)subqry->ops_eclass.ope_ev);
    *ninstr += 2 * neqcs;
    *nops += neqcs * 2;
}

/*{
** Name: OPC_ECINSTRS	- Compile the instructions to compare an eq class.
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
**      9-apr-87 (eric)
**          written
**	24-may-90 (stec)
**	    Added checks to use OPC_EQ, OPC_ATT contents only when filled in.
**	20-jun-90 (stec)
**	    Removed first test for opc_attavailable (in OPC_ATT), since we
**	    can have a case in which an attribute is a function attribute
**	    and it may not be filled initially, the OPC_ATT struct for the
**	    attribute will be initialized subsequently by calling opc_qual().
**	16-jan-91 (stec)
**	    Added missing opr_prec initialization.
[@history_template@]...
*/
VOID
opc_ecinstrs(
		OPS_STATE	*global,
		OPC_NODE	*cnode,
		OPC_ADF		*cadf,
		OPO_ISORT	ineqcno,
		OPC_EQ		*ceq1,
		OPC_EQ		*ceq2)
{
    OPS_SUBQUERY	*subqry = global->ops_cstate.opc_subqry;
    ADE_OPERAND		ops[2];
    OPE_IEQCLS		eqcno;
    OPZ_IATTS		attno1;
    OPZ_IATTS		attno2;
    OPZ_ATTS		**attnums;
    OPZ_FATTS		**fatts;
    OPE_EQCLIST		**eqclist;
    OPZ_FATTS		*fatt;
    i4			tidatt;
    OPC_EQ		*save_ceq;
    OPO_ISORT		*eqorder;
    OPE_IEQCLS		teqcls[2];
    ADI_FI_ID		opno;

    /* Init some vars to keep line length down */
    attnums = (OPZ_ATTS **) subqry->ops_attrs.opz_base->opz_attnums;
    fatts = (OPZ_FATTS **) subqry->ops_funcs.opz_fbase->opz_fatts;
    eqclist = (OPE_EQCLIST **) subqry->ops_eclass.ope_base->ope_eqclist;

    save_ceq = cnode->opc_ceq;

    if (ineqcno >= subqry->ops_eclass.ope_ev)
    {
	eqorder = &subqry->ops_msort.opo_base->
			opo_stable[ineqcno - subqry->ops_eclass.ope_ev]->
							opo_eqlist->opo_eqorder[0];
    }
    else
    {
	teqcls[0] = ineqcno;
	teqcls[1] = OPE_NOEQCLS;
	eqorder = &teqcls[0];
    }

    /* for (each eqcno) */
    for ( ; (eqcno = *eqorder) != OPE_NOEQCLS; eqorder += 1)
    {
	if (ceq1[eqcno].opc_eqavailable != TRUE || 
	    ceq2[eqcno].opc_eqavailable != TRUE
	    )
	{
	    opx_error(E_OP0889_EQ_NOT_HERE);
	}

	attno1 = ceq1[eqcno].opc_attno;
	attno2 = ceq2[eqcno].opc_attno;

	/* Lets fill in the first operand */
	ops[0].opr_dt	= ceq1[eqcno].opc_eqcdv.db_datatype;
	ops[0].opr_prec = ceq1[eqcno].opc_eqcdv.db_prec;
	ops[0].opr_len	= ceq1[eqcno].opc_eqcdv.db_length;
	ops[0].opr_collID = ceq1[eqcno].opc_eqcdv.db_collID;

	if (ceq1[eqcno].opc_attavailable == TRUE)
	{
	    if (cadf->opc_row_base[ceq1[eqcno].opc_dshrow] < ADE_ZBASE ||
		    cadf->opc_row_base[ceq1[eqcno].opc_dshrow] >= 
					ADE_ZBASE + cadf->opc_qeadf->qen_sz_base
		)
	    {
		/* add the row to the base array; */
		opc_adbase(global, cadf, QEN_ROW, ceq1[eqcno].opc_dshrow);
	    }

	    ops[0].opr_base = cadf->opc_row_base[ceq1[eqcno].opc_dshrow];
	    ops[0].opr_offset = ceq1[eqcno].opc_dshoffset;
	}
	else if (attno1 == OPC_NOATT)
	{
	    opx_error(E_OP0888_ATT_NOT_HERE);
	}
	else if (attnums[attno1]->opz_attnm.db_att_id == OPZ_FANUM)
	{
	    /* otherwise attno 1 is a function attribute that hasn't
	    ** been materialized yet.
	    */
	    if (attnums[attno1]->opz_func_att < 0)
		opx_error(E_OP0888_ATT_NOT_HERE);
	    fatt = fatts[attnums[attno1]->opz_func_att];
	    cnode->opc_ceq = ceq1;

	    opc_cqual(global, cnode, fatt->opz_function->pst_right, 
					    cadf, ADE_SMAIN, &ops[0], &tidatt);
	}
	else
	{
	    opx_error(E_OP0888_ATT_NOT_HERE);
	}

	/* Now lets fill in the second operand */
	ops[1].opr_dt	= ceq2[eqcno].opc_eqcdv.db_datatype;
	ops[1].opr_prec = ceq2[eqcno].opc_eqcdv.db_prec;
	ops[1].opr_len	= ceq2[eqcno].opc_eqcdv.db_length;
	ops[1].opr_collID = ceq2[eqcno].opc_eqcdv.db_collID;

	if (ceq2[eqcno].opc_attavailable == TRUE)
	{
	    if (cadf->opc_row_base[ceq2[eqcno].opc_dshrow] < ADE_ZBASE ||
		    cadf->opc_row_base[ceq2[eqcno].opc_dshrow] >= 
					ADE_ZBASE + cadf->opc_qeadf->qen_sz_base
		)
	    {
		/* add the row to the base array; */
		opc_adbase(global, cadf, QEN_ROW, ceq2[eqcno].opc_dshrow);
	    }

	    ops[1].opr_base = cadf->opc_row_base[ceq2[eqcno].opc_dshrow];
	    ops[1].opr_offset = ceq2[eqcno].opc_dshoffset;
	}
	else if (attno2 == OPC_NOATT)
	{
	    opx_error(E_OP0888_ATT_NOT_HERE);
	}
	else if (attnums[attno2]->opz_attnm.db_att_id == OPZ_FANUM)
	{
	    /* otherwise attno 2 is a function attribute that hasn't
	    ** been materialized yet.
	    */
	    if (attnums[attno2]->opz_func_att < 0)
		opx_error(E_OP0888_ATT_NOT_HERE);
	    fatt = fatts[attnums[attno2]->opz_func_att];
	    cnode->opc_ceq = ceq2;

	    opc_cqual(global, cnode, fatt->opz_function->pst_right, 
					    cadf, ADE_SMAIN, &ops[1], &tidatt);
	}
	else
	{
	    opx_error(E_OP0888_ATT_NOT_HERE);
	}

	/* compile the instruction to compare the two key attributes. */
	if (eqclist[eqcno]->ope_nulljoin == TRUE)
	{
	    /* We want to include NULLs in the result of joins here, so
	    ** we use ADE_BYCOMPARE since it assumes that NULL = NULL
	    */
	    opno = ADE_BYCOMPARE;
	}
	else
	{
	    /* We want to exclude NULLs in the result of this join. So
	    ** we use ADE_COMPARE which assumes that NULL != NULL
	    */
	    opno = ADE_COMPARE;
	}
	opc_adinstr(global, cadf, opno, ADE_SMAIN, 2, ops, 0);
    }
    opc_adinstr(global, cadf, ADE_AND, ADE_SMAIN, 0, ops, 0);

    cnode->opc_ceq = save_ceq;
}

/*{
** Name: OPC_EMINSTRS	- Compile the instructions to compare the EQCs in a map
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
**      9-apr-87 (eric)
**          written
**	24-may-90 (stec)
**	    Added checks to use OPC_EQ, OPC_ATT contents only when filled in.
**	20-jun-90 (stec)
**	    Removed first test for opc_attavailable (in OPC_ATT), since we
**	    can have a case in which an attribute is a function attribute
**	    and it may not be filled initially, the OPC_ATT struct for the
**	    attribute will be initialized subsequently by calling opc_qual().
**	    Fixes bug 30901.
**	16-jan-91 (stec)
**	    Added missing opr_prec initialization.
**	25-feb-92 (rickh)
**	    Modified opc_eminstrs to call opc_getFattDataType when processing
**	    a function attribute.
[@history_template@]...
*/
VOID
opc_eminstrs(
		OPS_STATE	*global,
		OPC_NODE	*cnode,
		OPC_ADF		*cadf,
		OPE_BMEQCLS	*eqcmp,
		OPC_EQ		*ceq1,
		OPC_EQ		*ceq2)
{
    OPS_SUBQUERY	*subqry = global->ops_cstate.opc_subqry;
    ADE_OPERAND		ops[2];
    OPE_IEQCLS		eqcno;
    OPZ_IATTS		attno1;
    OPZ_IATTS		attno2;
    OPZ_ATTS		**attnums;
    OPZ_FATTS		**fatts;
    OPE_EQCLIST		**eqclist;
    OPZ_FATTS		*fatt;
    i4			tidatt;
    OPC_EQ		*save_ceq;
    ADI_FI_ID		opno;

    /* Init some vars to keep line length down */
    attnums = (OPZ_ATTS **) subqry->ops_attrs.opz_base->opz_attnums;
    fatts = (OPZ_FATTS **) subqry->ops_funcs.opz_fbase->opz_fatts;
    eqclist = (OPE_EQCLIST **) subqry->ops_eclass.ope_base->ope_eqclist;

    save_ceq = cnode->opc_ceq;

    /* for (each eqcno) */
    for (eqcno = -1;
	    (eqcno = (OPE_IEQCLS) BTnext((i4) eqcno, (char *)eqcmp, 
			    (i4)subqry->ops_eclass.ope_ev)) != -1;
	)
    {
	if (ceq1[eqcno].opc_eqavailable != TRUE || 
	    ceq2[eqcno].opc_eqavailable != TRUE
	    )
	{
	    opx_error(E_OP0889_EQ_NOT_HERE);
	}

	attno1 = ceq1[eqcno].opc_attno;
	attno2 = ceq2[eqcno].opc_attno;

	/* Lets fill in the first operand */
	ops[0].opr_dt	= ceq1[eqcno].opc_eqcdv.db_datatype;
	ops[0].opr_prec = ceq1[eqcno].opc_eqcdv.db_prec;
	ops[0].opr_len	= ceq1[eqcno].opc_eqcdv.db_length;
	ops[0].opr_collID = ceq1[eqcno].opc_eqcdv.db_collID;

	if (ceq1[eqcno].opc_attavailable == TRUE)
	{
	    if (cadf->opc_row_base[ceq1[eqcno].opc_dshrow] < ADE_ZBASE ||
		    cadf->opc_row_base[ceq1[eqcno].opc_dshrow] >= 
					ADE_ZBASE + cadf->opc_qeadf->qen_sz_base
		)
	    {
		/* add the row to the base array; */
		opc_adbase(global, cadf, QEN_ROW, ceq1[eqcno].opc_dshrow);
	    }

	    ops[0].opr_base = cadf->opc_row_base[ceq1[eqcno].opc_dshrow];
	    ops[0].opr_offset = ceq1[eqcno].opc_dshoffset;
	}
	else if (attno1 == OPC_NOATT)
	{
	    opx_error(E_OP0888_ATT_NOT_HERE);
	}
	else if (attnums[attno1]->opz_attnm.db_att_id == OPZ_FANUM)
	{
	    /* otherwise attno 1 is a function attribute that hasn't
	    ** been materialized yet.
	    */
	    if (attnums[attno1]->opz_func_att < 0)
		opx_error(E_OP0888_ATT_NOT_HERE);
	    fatt = fatts[attnums[attno1]->opz_func_att];
	    cnode->opc_ceq = ceq1;

	    /* fill in the target data value based on this function att */

	    opc_getFattDataType( global, attno1, &ops[ 0 ] );

	    opc_cqual(global, cnode, fatt->opz_function->pst_right, 
					    cadf, ADE_SMAIN, &ops[0], &tidatt);
	}
	else
	{
	    opx_error(E_OP0888_ATT_NOT_HERE);
	}

	/* Now lets fill in the second operand */
	ops[1].opr_dt	= ceq2[eqcno].opc_eqcdv.db_datatype;
	ops[1].opr_prec = ceq2[eqcno].opc_eqcdv.db_prec;
	ops[1].opr_len	= ceq2[eqcno].opc_eqcdv.db_length;
	ops[1].opr_collID = ceq2[eqcno].opc_eqcdv.db_collID;

	if (ceq2[eqcno].opc_attavailable == TRUE)
	{
	    if (cadf->opc_row_base[ceq2[eqcno].opc_dshrow] < ADE_ZBASE ||
		    cadf->opc_row_base[ceq2[eqcno].opc_dshrow] >= 
					ADE_ZBASE + cadf->opc_qeadf->qen_sz_base
		)
	    {
		/* add the row to the base array; */
		opc_adbase(global, cadf, QEN_ROW, ceq2[eqcno].opc_dshrow);
	    }

	    ops[1].opr_base = cadf->opc_row_base[ceq2[eqcno].opc_dshrow];
	    ops[1].opr_offset = ceq2[eqcno].opc_dshoffset;
	}
	else if (attno2 == OPC_NOATT)
	{
	    opx_error(E_OP0888_ATT_NOT_HERE);
	}
	else if (attnums[attno2]->opz_attnm.db_att_id == OPZ_FANUM)
	{
	    /* otherwise attno 2 is a function attribute that hasn't
	    ** been materialized yet.
	    */
	    if (attnums[attno2]->opz_func_att < 0)
		opx_error(E_OP0888_ATT_NOT_HERE);
	    fatt = fatts[attnums[attno2]->opz_func_att];
	    cnode->opc_ceq = ceq2;

	    opc_cqual(global, cnode, fatt->opz_function->pst_right, 
					    cadf, ADE_SMAIN, &ops[1], &tidatt);
	}
	else
	{
	    opx_error(E_OP0888_ATT_NOT_HERE);
	}

	/* compile the instruction to compare the two key attributes. */
	if (eqclist[eqcno]->ope_nulljoin == TRUE)
	{
	    /* We want to include NULLs in the result of joins here, so
	    ** we use ADE_BYCOMPARE since it assumes that NULL = NULL
	    */
	    opno = ADE_BYCOMPARE;
	}
	else
	{
	    /* We want to exclude NULLs in the result of this join. So
	    ** we use ADE_COMPARE which assumes that NULL != NULL
	    */
	    opno = ADE_COMPARE;
	}
	opc_adinstr(global, cadf, opno, ADE_SMAIN, 2, ops, 0);
    }
    opc_adinstr(global, cadf, ADE_AND, ADE_SMAIN, 0, ops, 0);

    cnode->opc_ceq = save_ceq;
}
