/*
**	dnode.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/


/* # include's */
# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<afe.h>
# include	<frame.h>
# include	"dnode.h"
# include	"derive.h"
# include	<lo.h>
# include	<si.h>
# include	<er.h>
# include	<me.h>
# include	<ex.h>
# include	<erfi.h>

/**
**  Name:	dnode.c - Derivation parser support routines.
**
**  Description:
**
**	Support routines for the derivation parser are located
**	in this file.  Also, routines to build the derivation
**	execution tree and to build compiled expressions can
**	be found here.
**
**	This file defines:
**
**	IIFVcdChkDerive		Check derivation tree, etc.
**	IIFVbsBuildSupport	Build derivation suuport tree strucutres.
**	IIFVcmnCompMathNode	Compile a math node.
**	IIFVcanCompAggNode	Compile an aggregate node.
**	IIFVsdsSetDeriveSource	Set derivation source relationship.
**	IIFVdtDrvTrace		Setup trace info for derivation parsing.
**	IIFVdtpDrvTracePrint	Print out derivation trace info.
**	IIFVddDbgDump		Routine to dump out field dependencies.
**	IIFVdnDumpNode		Dump information about a DNODE.
**	IIFVmnMakeNode		Create a DNODE for the derivation parser.
**	IIFVmsMathSetup		Set up a math node.
**	IIFVasAggSetup		Set up aggregate for runtime execution.
**	IIFVccConstChk		Check for constant derivations.
**	IIFVcpConstProp		Check form for derived constant propagation.
**	IIFVdrDrvReset		Circularity check cleanup.
**	IIFVdcdDrvChkDep	Do circularity check on a field/column.
**	IIFVdccDrvCirChk	Check for derived circular references in a form.
**
**  To do:
**	Update compiled expression with db_prec values when
**	ADE_OPERAND is updated for decimal datatype.
**
**  History:
**	06/20/89 (dkh) - Initial version.
**	10/02/89 (dkh) - Changed IIFVdcDeriveCheck() to IIFVcdChkDerive() and
**			 IIFVdsDumpSupport() to IIFVddDbgDump().
**	11/07/89 (dkh) - Null terminated string used in error E_FI20C6_8390.
**	11/16/89 (dkh) - Added support for decimal.
**	16-jan-90 (bruceb)
**		Added expected param on call to IIFVdeErr(FI20AD).
**	03/29/90 (dkh) - Fixed IIFVbsBuildSupport() to not stomp on hdr->fhdrv
**			 if it already exists.
**	18-apr-90 (bruceb)
**		Lint cleanup; removed 'drv' arg from IIFVcmn and IIFVcan.
**	07-sep-90 (bruceb)
**		Call IIFVdeErr(8396) with only 1 param (agg) since that
**		routine tacks on the field name.
**	19-oct-1990 (mgw)
**		Fixed #include of local dnode.h and derive.h to use ""
**		instead of <>
**	03/24/91 (dkh) - Fixed problem with call to CVfa().
**	04-jun-1991 (mgw) Bug # 36970
**		ade_cx_space() wasn't allocating enough space for the compile
**		of derived fields with more than 4 operands in ade_instr_gen()
**		so I made a new routine, IIFVcaiCallAdeInst() and called it
**		from IIFVcmnCompMathNode() each place where ade_instr_gen()
**		used to be called. This was possible since all 3 calls had
**		the same behavior on error. The IIFVcaiCallAdeInst() routine
**		will allocate more space for ade_instr_gen() if needed.
**	12/27/91 (dkh) - Fixed bug 41636.
**	06/13/92 (dkh) - Added support for decimal datatype for 6.5.
**	08/31/93 (dkh) - Fixed up compiler complaints due to prototyping.
**	12-Nov-1993 (tad)
**		Bug #56449
**		Replace %x with %p for pointer values where necessary.
**      24-sep-96 (hanch04)
**              Global data moved to data.c
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/* define's */
# define	NOCOLNO		BADCOLNO


GLOBALREF	i4	IIFVorcOprCnt ;
GLOBALREF	i4	IIFVodcOpdCnt ;


/* extern's */
GLOBALREF	FLDTYPE	*vl_curtype;
GLOBALREF	FRAME	*vl_curfrm;
GLOBALREF	FLDHDR	*vl_curhdr;
GLOBALREF	FIELD	*IIFVcfCurFld;

GLOBALREF	bool	IIFVddpDebugPrint;
GLOBALREF	bool	IIFVocOnlyCnst;
GLOBALREF	bool	vl_syntax;
GLOBALREF	bool	IIFVcvConstVal;

GLOBALREF	DB_DATA_VALUE	*IIFVdc1DbvConst;
GLOBALREF	DB_DATA_VALUE	*IIFVdc2DbvConst;

GLOBALREF	char	IIFVfloat_str[];

FUNC_EXTERN	FIELD	*FDfind();
FUNC_EXTERN	FIELD	*FDfndfld();
FUNC_EXTERN	FLDTYPE	*FDgettype();
FUNC_EXTERN	FLDVAL	*FDgetval();
FUNC_EXTERN	FLDHDR	*FDgethdr();
FUNC_EXTERN	FLDHDR	*IIFDgchGetColHdr();
FUNC_EXTERN	ADF_CB	*FEadfcb();
FUNC_EXTERN	VOID	IIFVdnDumpNode();
FUNC_EXTERN	VOID	IIFVbsBuildSupport();
FUNC_EXTERN	VOID	IIFVddDbgDump();
FUNC_EXTERN	VOID	IIFVssSetSupport();
FUNC_EXTERN	VOID	IIFVsdsSetDeriveSource();
FUNC_EXTERN	VOID	vl_typstr();
FUNC_EXTERN	DB_LANG	IIAFdsDmlSet();
FUNC_EXTERN	VOID	IIFVcaiCallAdeInst();	/* call ade_instr_gen() and
						** re-allocate space as needed
						*/


/* static's */
static	i4		oper_inx = 0;		/* count of operators seen */
static	i4		operand_inx = 0;	/* count of operands seen */
static	ADE_EXCB	*excb = NULL;		/* compile execution ptr */
static	PTR		cxspace = NULL;		/* space for compile expr */
static	i4		cxsize = 0;		/* size of cxspace */
static	i4		cxcomp = FALSE;		/* are we doing cx or not */
static	bool		trace_inited = FALSE;	/* tracing boolean */
static	FILE		*trfile = NULL;		/* trace FILE pointer */


/*{
** Name:	IIFVcdChkDerive - Check derivation tree, etc.
**
** Description:
**	This routine descends the derivation tree that is passed
**	in and does various datatype compatibility checks and
**	to compile the tree for faster execution.
**
** Inputs:
**	node		Root DNODE of derivation tree.
**
** Outputs:
**
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	06/20/89 (dkh) - Initial version.
*/
VOID
IIFVcdChkDerive(node)
DNODE	*node;
{
	DB_DATA_VALUE	fdbv;
	bool		canco = FALSE;
	ADF_CB		*adfcb;
	char		fld_type[100];
	char		drv_type[100];
	i4		used;

	/*
	**  Need to do datatype coercion test to see if we
	**  can coerce from expression type to derived field type.
	**  Also need to check for NULLable vs. non-NULLable.
	*/
	adfcb = FEadfcb();
	fdbv.db_datatype = vl_curtype->ftdatatype;
	fdbv.db_length = vl_curtype->ftlength;
	fdbv.db_prec = vl_curtype->ftprec;
	if (afe_cancoerce(adfcb, node->result, &fdbv, &canco) != OK)
	{
		/*
		**  Error trying to figure out coercion.
		**  Cancel entire derivation.
		*/
		if (IIFVcfCurFld->fltag == FREGULAR)
		{
			IIFVdeErr(E_FI20C0_8384, 0);
		}
		else
		{
			IIFVdeErr(E_FI20C1_8385, 1,
			    (PTR)IIFVcfCurFld->fld_var.fltblfld->tfhdr.fhdname);
		}
		return;
	}
	if (!canco)
	{
		/*
		**  If can't coerce, put out error message.
		*/
		vl_typstr(&fdbv, fld_type);
		vl_typstr(node->result, drv_type);
		if (IIFVcfCurFld->fltag == FREGULAR)
		{
			IIFVdeErr(E_FI20C2_8386, 2, (PTR) fld_type,
				(PTR) drv_type);
		}
		else
		{
			IIFVdeErr(E_FI20C3_8387, 3,
			    (PTR) IIFVcfCurFld->fld_var.fltblfld->tfhdr.fhdname,
			    (PTR) fld_type, (PTR) drv_type);
		}
	}
	if (AFE_NULLABLE(node->result->db_datatype) &&
		!AFE_NULLABLE(fdbv.db_datatype))
	{
		if (IIFVcfCurFld->fltag == FREGULAR)
		{
			IIFVdeErr(E_FI20C4_8388, 0);
		}
		else
		{
			IIFVdeErr(E_FI20C5_8389, 1,
			    (PTR)IIFVcfCurFld->fld_var.fltblfld->tfhdr.fhdname);
		}
	}

	IIFVdnDumpNode(node);

	oper_inx = 0;
	operand_inx = 0;
	cxcomp = FALSE;

	/*
	**  Only do CX if not just doing syntax checking and there are real
	**  field values (not just constants) and there is at least
	**  one operator.
	*/
	if (!vl_syntax && !IIFVocOnlyCnst && IIFVorcOprCnt)
	{
		cxcomp = TRUE;
		/*
		**  Allocate space for CX, etc.
		*/
		if (ade_cx_space(adfcb, IIFVorcOprCnt + 2, IIFVodcOpdCnt + 2,
			0, (i4)0, &cxsize) != OK)
		{
			/*
			**  Can't determine size of CX.
			*/
			if (IIFVcfCurFld->fltag == FREGULAR)
			{
				IIFVdeErr(E_FI208A_8330, 0);
			}
			else
			{
				IIFVdeErr(E_FI2094_8340, 1,
					(PTR)IIFVcfCurFld->fld_var.fltblfld->
					tfhdr.fhdname);
			}
		}
		if ((cxspace = MEreqmem(0, cxsize, TRUE,
			(STATUS *)NULL)) == NULL)
		{
			/*
			**  Never returns.
			*/
			IIUGbmaBadMemoryAllocation(ERx("cx-alloc"));
		}
		if ((excb = (ADE_EXCB *) MEreqmem(0,
			(sizeof(ADE_EXCB) + (ADE_ZBASE +
			IIFVodcOpdCnt) * sizeof(PTR)), TRUE,
			(STATUS *)NULL)) == NULL)
		{
			/*
			**  Never returns.
			*/
			IIUGbmaBadMemoryAllocation(ERx("excb-alloc"));
		}
		if (ade_bgn_comp(adfcb, cxspace, cxsize) != OK)
		{
			/*
			**  Can't initialize for CX.
			*/
			if (IIFVcfCurFld->fltag == FREGULAR)
			{
				IIFVdeErr(E_FI208B_8331, 0);
			}
			else
			{
				IIFVdeErr(E_FI2095_8341, 1,
					(PTR)IIFVcfCurFld->fld_var.fltblfld->
					tfhdr.fhdname);
			}
		}
		excb->excb_bases[ADE_CXBASE] = cxspace;
	}
	IIFVbsBuildSupport(node);
	if (IIFVocOnlyCnst)
	{
		vl_curhdr->fhdrv->drvflags |= fhDRVCNST;
	}
	else if (cxcomp)
	{
		/*
		**  Only compiling stuff if there are any
		**  operators for now.
		*/
		if (ade_end_comp(adfcb, cxspace, &used) != OK)
		{
			/*
			**  Can't close out CX compilation.
			*/
			if (IIFVcfCurFld->fltag == FREGULAR)
			{
				IIFVdeErr(E_FI208C_8332, 0);
			}
			else
			{
				IIFVdeErr(E_FI2096_8342, 1,
					(PTR)IIFVcfCurFld->fld_var.fltblfld->
					tfhdr.fhdname);
			}
		}
		excb->excb_cx = cxspace;
		excb->excb_seg = ADE_SMAIN;
		excb->excb_nbases = ADE_ZBASE + IIFVodcOpdCnt - 1;
		node->drv_excb = excb;
	}
	return;
}



/*{
** Name:	IIFVbsBuildSupport - Build derivation support structures.
**
** Description:
**	Top entry point to build derivation support structures
**	and to compile the execution tree.
**
** Inputs:
**	node		Root DNODE of derivation tree.
**
** Outputs:
**
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	06/20/89 (dkh) - Initial version.
*/
VOID
IIFVbsBuildSupport(node)
DNODE	*node;
{
	FLDHDR	*hdr;
	DERIVE	*drv;

	/*
	**  The deplist is currently ignored.
	**  Ignoring aggregates for now as well.
	*/
	hdr = vl_curhdr;
	if (hdr->fhdrv == NULL)
	{
		if ((drv = (DERIVE *) MEreqmem(0, sizeof(DERIVE), TRUE,
			(STATUS *) NULL)) == NULL)
		{
			IIUGbmaBadMemoryAllocation(ERx("IIFVbsBuildSupport"));
			return;
		}
		hdr->fhdrv = drv;
	}
	else
	{
		drv = hdr->fhdrv;
	}

	switch(node->nodetype)
	{
	    case ICONST_NODE:
	    case FCONST_NODE:
	    case SCONST_NODE:
	    case DCONST_NODE:
		/*
		**  Just a constant node, return.
		*/
		if (cxcomp)
		{
			/*
			**  Set up constant DBV into excb array.
			*/
			excb->excb_bases[ADE_ZBASE + node->res_inx] =
				node->result->db_data;
		}
		break;

	    case PLUS_NODE:
	    case MINUS_NODE:
	    case MULT_NODE:
	    case DIV_NODE:
	    case EXP_NODE:
	    case UMINUS_NODE:
	    case MAX_NODE:
	    case MIN_NODE:
	    case AVG_NODE:
	    case SUM_NODE:
	    case CNT_NODE:
	    case SFLD_NODE:
	    case TCOL_NODE:
		IIFVsdsSetDeriveSource(drv, node);
		break;

	    default:
		break;
	}

	IIFVddDbgDump(drv);
	return;
}


/*{
** Name:	IIFVcmnCompMathNode - Compile a math node.
**
** Description:
**	Compile a math node in the derivation tree, including
**	any necessary coercions before executing the math
**	operation.
**
** Inputs:
**	node		The math node to be compiled.
**
** Outputs:
**
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	08/10/89 (dkh) - Initial version.
*/
VOID
IIFVcmnCompMathNode(node)
DNODE	*node;
{
	ADE_OPERAND	adeops[3];
	DNODE		*lr;
	ADF_CB		*adfcb;
	DB_DT_ID	left_datatype;
	i4		left_length;
	i2		left_prec;
	i4		left_inx;
	DB_DT_ID	right_datatype;
	i4		right_length;
	i2		right_prec;
	i4		right_inx;
	i4		opcnt = 3;
	i4		qmap;
	i4		unalign;

	adfcb = FEadfcb();

	if (node->nodetype == UMINUS_NODE)
	{
		opcnt = 2;
	}
	/*
	**  Need to check if a coercion needs to be done first.
	**  Do left side, then right side.
	*/
	lr = node->left;
	left_datatype = lr->result->db_datatype;
	left_length = lr->result->db_length;
	left_prec = lr->result->db_prec;
	left_inx = lr->res_inx;
	if (node->left->cresult)
	{
		adeops[1].opr_dt = left_datatype;
		adeops[1].opr_len = left_length;
		adeops[1].opr_prec = left_prec;
		adeops[1].opr_base = ADE_ZBASE + left_inx;
		adeops[1].opr_offset = 0;
		adeops[0].opr_dt = left_datatype = lr->cresult->db_datatype;
		adeops[0].opr_len = left_length = lr->cresult->db_length;
		adeops[0].opr_prec = left_prec = lr->cresult->db_prec;
		adeops[0].opr_base = ADE_ZBASE + lr->cres_inx;
		adeops[0].opr_offset = 0;
		left_inx = lr->cres_inx;
		IIFVcaiCallAdeInst(adfcb, lr->coer_fid, ADE_SMAIN, 2, adeops,
					&qmap, &unalign);
	}
	if (opcnt > 2)
	{
		lr = node->right;
		right_datatype = lr->result->db_datatype;
		right_length = lr->result->db_length;
		right_prec = lr->result->db_prec;
		right_inx = lr->res_inx;
	}
	if (opcnt > 2 && node->right->cresult)
	{
		adeops[1].opr_dt = right_datatype;
		adeops[1].opr_len = right_length;
		adeops[1].opr_prec = right_prec;
		adeops[1].opr_base = ADE_ZBASE + right_inx;
		adeops[1].opr_offset = 0;
		adeops[0].opr_dt = right_datatype = lr->cresult->db_datatype;
		adeops[0].opr_len = right_length = lr->cresult->db_length;
		adeops[0].opr_prec = right_prec = lr->cresult->db_prec;
		adeops[0].opr_base = ADE_ZBASE + lr->cres_inx;
		adeops[0].opr_offset = 0;
		right_inx = lr->cres_inx;
		IIFVcaiCallAdeInst(adfcb, lr->coer_fid, ADE_SMAIN, 2, adeops,
					&qmap, &unalign);
	}

	adeops[0].opr_dt = node->result->db_datatype;
	adeops[0].opr_len = node->result->db_length;
	adeops[0].opr_prec = node->result->db_prec;
	adeops[0].opr_base = ADE_ZBASE + node->res_inx;
	adeops[0].opr_offset = 0;
	adeops[1].opr_dt = left_datatype;
	adeops[1].opr_len = left_length;
	adeops[1].opr_prec = left_prec;
	adeops[1].opr_base = ADE_ZBASE + left_inx;
	adeops[1].opr_offset = 0;
	if (opcnt > 2)
	{
		adeops[2].opr_dt = right_datatype;
		adeops[2].opr_len = right_length;
		adeops[2].opr_prec = right_prec;
		adeops[2].opr_base = ADE_ZBASE + right_inx;
		adeops[2].opr_offset = 0;
	}
	IIFVcaiCallAdeInst(adfcb, node->fdesc->adi_finstid, ADE_SMAIN, opcnt,
				adeops, &qmap, &unalign);
}



/*{
** Name:	IIFVcanCompAggNode - Compile an aggregate node.
**
** Description:
**	Compile an aggregate node of a derivation tree.
**
** Inputs:
**	node		The aggregate node to compile.
**	aggval		DAGGAL structure for the specific aggregate being
**			compiled.
**	src		DBV of column being aggregated.
**
** Outputs:
**
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	08/10/89 (dkh) - Initial version.
**	30-aug-01 (inkdo01)
**	    Change detail F.I. opcode to generic ADE_AGNEXT opcode because
**	    of performance changes in backend ADF.
*/
VOID
IIFVcanCompAggNode(node, aggval, src)
DNODE		*node;
DAGGVAL		*aggval;
DB_DATA_VALUE	*src;
{
	ADE_OPERAND	adeops[3];
	ADE_EXCB	*aggexcb;
	PTR		aggcxspace;
	i4		aggcxsize;
	ADF_CB		*adfcb;
	i4		qmap;
	i4		unalign;
	i4		used;

	adfcb = FEadfcb();

	if (ade_cx_space(adfcb,  4, 4, 0, (i4)0, &aggcxsize) != OK)
	{
		/*
		**  Can't set up agg CX.
		*/
		IIFVdeErr(E_FI208E_8334, 0);
	}
	if ((aggcxspace = MEreqmem(0, aggcxsize, TRUE, (STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("aggcx-alloc"));
	}
	if ((aggexcb = (ADE_EXCB *) MEreqmem(0,
		(sizeof(ADE_EXCB) + (ADE_ZBASE + 4) * sizeof(PTR)),
		TRUE, (STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("aggcx-alloc"));
	}
	if (ade_bgn_comp(adfcb, aggcxspace, aggcxsize) != OK)
	{
		/*
		**  Can't initialize aggregate CX.
		*/
		IIFVdeErr(E_FI208F_8335, 0);
	}

	adeops[0].opr_dt = 0;
	adeops[0].opr_len = 0;
	adeops[0].opr_base = ADE_ZBASE + 0;
	adeops[0].opr_offset = 0;
	if (ade_instr_gen(adfcb, aggcxspace, ADE_AGBGN, ADE_SINIT, 1,
		adeops, &qmap, &unalign) != OK)
	{
		/*
		**  Can't put instruction into aggregate CX.
		*/
		IIFVdeErr(E_FI2090_8336, 0);
	}
	adeops[0].opr_dt = node->result->db_datatype;
	adeops[0].opr_len = node->result->db_length;
	adeops[0].opr_prec = node->result->db_prec;
	adeops[0].opr_base = ADE_ZBASE + 2;
	adeops[0].opr_offset = 0;
	adeops[1].opr_dt = 0;
	adeops[1].opr_len = 0;
	adeops[1].opr_prec = 0;
	adeops[1].opr_base = ADE_ZBASE + 0;
	adeops[1].opr_offset = 0;
	if (ade_instr_gen(adfcb, aggcxspace, ADE_NAGEND, ADE_SFINIT, 2,
		adeops, &qmap, &unalign) != OK)
	{
		/*
		**  Can't put instruction into aggregate CX.
		*/
		IIFVdeErr(E_FI2090_8336, 0);
	}
	adeops[0].opr_dt = 0;
	adeops[0].opr_len = 0;
	adeops[0].opr_prec = 0;
	adeops[0].opr_base = ADE_ZBASE + 0;
	adeops[0].opr_offset = 0;
	adeops[1].opr_dt = src->db_datatype;
	adeops[1].opr_len = src->db_length;
	adeops[1].opr_prec = src->db_prec;
	adeops[1].opr_base = ADE_ZBASE + 1;
	adeops[1].opr_offset = 0;
	if (ade_instr_gen(adfcb, aggcxspace, ADE_AGNEXT, ADE_SMAIN, 2, 
		adeops, &qmap, &unalign) != OK)
	{
		/*
		**  Can't put instruction into aggregate CX.
		*/
		IIFVdeErr(E_FI2090_8336, 0);
	}
	if (ade_end_comp(adfcb, aggcxspace, &used) != OK)
	{
		/*
		**  Can't finalize aggregate CX.
		*/
		IIFVdeErr(E_FI2091_8337, 0);
	}
	aggval->agg_excb = aggexcb;

	aggexcb->excb_cx = aggcxspace;
	aggexcb->excb_seg = ADE_SINIT;
	aggexcb->excb_bases[ADE_CXBASE] = aggcxspace;
	aggexcb->excb_bases[ADE_ZBASE + 0] = (PTR) &(aggval->adfagstr);
	aggexcb->excb_bases[ADE_ZBASE + 2] = aggval->aggdbv.db_data;
	aggexcb->excb_nbases = ADE_ZBASE + 2;
	/*
	**  Execution phase needs to fill in ADE_ZBASE + 1 to
	**  point to DBV holding values to be aggregated.
	*/
}





/*{
** Name:	IIFVsdsSetDeriveSource - Set derivation source relationship.
**
** Description:
**	Descends derivation tree and sets up derivation dependency
**	information between the derived field and its source fields.
**	Also triggers compiling various nodes for faster execution.
**
** Inputs:
**	drv		DERIVE structure for field containing derivation
**			being processed.
**	node		Root DNODE for derivation tree.
**
** Outputs:
**
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	06/20/89 (dkh) - Initial version.
*/
VOID
IIFVsdsSetDeriveSource(drv, node)
DERIVE	*drv;
DNODE	*node;
{
	DRVREL		*drvrel;
	FIELD		*fld;
	FLDHDR		*hdr;
	FLDCOL		*col;
	TBLFLD		*tf;
	DERIVE		*ndrv;
	DAGGVAL		**aggval;
	DRVAGG		*drvagg;
	FLDTYPE		*type;
	DB_DATA_VALUE	src;

	/*
	**  Do left, then right branch first to ease
	**  compilation process.
	*/
	if (node->left != NULL)
	{
		IIFVsdsSetDeriveSource(drv, node->left);
	}
	if (node->right != NULL)
	{
		IIFVsdsSetDeriveSource(drv, node->right);
	}

	if (cxcomp)
	{
		/*
		**  This puts the result value for the node
		**  into the CX data array.  Works for both
		**  constant values and field values.
		*/
		excb->excb_bases[ADE_ZBASE + node->res_inx] =
			node->result->db_data;
		/*
		**  Set up coerced results if necessary.
		*/
		if (node->cresult)
		{
			excb->excb_bases[ADE_ZBASE + node->cres_inx] =
				node->cresult->db_data;
		}
	}
	if ((fld = node->fld) != NULL)
	{
		if ((drvrel = (DRVREL *) MEreqmem(0, sizeof(DRVREL), TRUE,
			(STATUS *) NULL)) == NULL)
		{
			IIUGbmaBadMemoryAllocation(ERx("IIFVsdsSetDeriveSource"));
			return;
		}
		drvrel->colno = node->colno;
		drvrel->fld = node->fld;
		drvrel->next = drv->srclist;
		drv->srclist = drvrel;

		/*
		**  Start setting the dependent chain.
		*/
		if (fld->fltag == FREGULAR)
		{
			hdr = FDgethdr(fld);
		}
		else /* if (fld->fltag == FTABLE) */
		{
			tf = fld->fld_var.fltblfld;
			col = tf->tfflds[node->colno];
			hdr = &(col->flhdr);
			type = &(col->fltype);
			if (node->nodetype == TCOL_NODE)
			{
				/*
				**  Note that field depends on
				**  a tf column.
				*/
				drv->drvflags |= fhDRVTFCDEP;
			}
		}
		if ((ndrv = hdr->fhdrv) == NULL)
		{
			if ((ndrv = (DERIVE *) MEreqmem(0,
				sizeof(DERIVE), TRUE,
				(STATUS *) NULL)) == NULL)
			{
				IIUGbmaBadMemoryAllocation(ERx("IIFVsdsSetDeriveSource"));
				return;
			}
			hdr->fhdrv = ndrv;
		}
		if ((drvrel = (DRVREL *) MEreqmem(0, sizeof(DRVREL),
			TRUE, (STATUS *) NULL)) == NULL)
		{
			IIUGbmaBadMemoryAllocation(ERx("IIFVsdsSetDeriveSource"));
			return;
		}
		drvrel->fld = IIFVcfCurFld;
		if (IIFVcfCurFld->fltag == FTABLE)
		{
			drvrel->colno = vl_curhdr->fhseq;
		}
		else
		{
			drvrel->colno = NOCOLNO;
		}
		drvrel->next = ndrv->deplist;
		ndrv->deplist = drvrel;

		/*
		**  Set aggregate dependent flag for field that is
		**  being processed.
		*/
		if (node->workspace != NO_AGG)
		{
			drv->drvflags |= fhDRVAGGDEP;
		}

		/*
		**  Now it's time to set up aggregate info.
		*/
		if (!vl_syntax && node->workspace != NO_AGG)
		{
			hdr->fhd2flags |= fdAGGSRC;
			tf->tfhdr.fhd2flags |= fdAGGSRC;
			node->agghdr = hdr;
			if ((drvagg = ndrv->agginfo) == NULL)
			{
				if ((ndrv->agginfo = (DRVAGG *)MEreqmem(0,
					sizeof(DRVAGG), TRUE,
					(STATUS *)NULL)) == NULL)
				{
					/*
					**  This never returns.
					*/
					IIUGbmaBadMemoryAllocation(ERx("IIFVsdsSetDeriveSource"));
				}
				drvagg = ndrv->agginfo;
			}
			switch(node->nodetype)
			{
			    case MAX_NODE:
				aggval = &(drvagg->maxagg);
				break;

			    case MIN_NODE:
				aggval = &(drvagg->minagg);
				break;

			    case AVG_NODE:
				aggval = &(drvagg->avgagg);
				break;

			    case SUM_NODE:
				aggval = &(drvagg->sumagg);
				break;

			    case CNT_NODE:
				aggval = &(drvagg->cntagg);
				break;
			}
			/*
			**  If there is no reference for this
			**  aggregate yet, then set it up.
			**  Otherwise, no need to do anything.
			*/
			if (*aggval == NULL)
			{
				if ((*aggval = (DAGGVAL *)MEreqmem(0,
					sizeof(DAGGVAL), TRUE,
					(STATUS *)NULL)) == NULL)
				{
					/*
					**  Never returns.
					*/
					IIUGbmaBadMemoryAllocation(ERx("IIFVsdsSetDeriveSource"));
				}

				MEcopy((PTR) node->result,
					(u_i2) sizeof(DB_DATA_VALUE),
					(PTR) &((*aggval)->aggdbv));
				if (((*aggval)->aggdbv.db_data = MEreqmem(0,
					(*aggval)->aggdbv.db_length, TRUE,
					(STATUS *)NULL)) == NULL)
				{
					/*
					**  Never returns.
					*/
					IIUGbmaBadMemoryAllocation(ERx("IIFVsdsSetDeriveSource"));
				}
				(*aggval)->adfagstr.adf_agfi = node->fid;
				(*aggval)->adfagstr.adf_agwork.db_length =
					node->workspace;
				if (node->workspace &&
				    ((*aggval)->adfagstr.adf_agwork.db_data =
				    MEreqmem(0, node->workspace, TRUE,
				    (STATUS *)NULL)) == NULL)
				{
					/*
					**  Never returns.
					*/
					IIUGbmaBadMemoryAllocation(ERx("IIFVsdsSetDeriveSource"));
				}
				/*
				**  Now do CX for aggregate.  Aggregates
				**  are always compiled.
				*/
				src.db_datatype = type->ftdatatype;
				src.db_length = type->ftlength;
				src.db_prec = type->ftprec;
				IIFVcanCompAggNode(node, *aggval, &src);
			}
		}
	}
	else if (cxcomp)
	{
		/*
		**  Check to see if its an operator node
		**  that needs to be compiled.
		*/
		switch(node->nodetype)
		{
		    case PLUS_NODE:
		    case MINUS_NODE:
		    case MULT_NODE:
		    case DIV_NODE:
		    case EXP_NODE:
		    case UMINUS_NODE:
			IIFVcmnCompMathNode(node);
			break;
		    default:
			break;
		}
	}

	return;
}



/*{
** Name:	IIFVdtDrvTrace - Setup trace info for derivation parsing.
**
** Description:
**	Print out trace information about the start of derivation parsing.
**
**	NEED TO WORD WRAP A LONG DERIVATION STRING.
**	MAY WANT TO ADD A TIME STAMP AT SOME LATER POINT.
**
** Inputs:
**	frm		Form containing field being parsed.
**	fld		Field to parse (can be simple or table field).
**	hdr		FLDHDR of field/column to be parsed.
**	type		FLDTYPE of field/column to be parsed.
**
** Outputs:
**
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	08/20/89 (dkh) - Initial version.
*/
VOID
IIFVdtDrvTrace(frm, fld, hdr, type)
FRAME	*frm;
FIELD	*fld;
FLDHDR	*hdr;
FLDTYPE	*type;
{
	LOCATION	loc;
	char		buf[MAX_LOC + 1];

	if (!IIFVddpDebugPrint)
	{
		return;
	}
	if (!trace_inited)
	{
		STcopy(ERx("iiprtfrs.log"), buf);
		/*
		**  If we can't open dump file, then just
		**  turn off tracing for now.
		*/
		if (LOfroms(FILENAME, buf, &loc) != OK)
		{
			IIFVddpDebugPrint = FALSE;
			return;
		}
		if (SIopen(&loc, ERx("w"), &trfile) != OK)
		{
			IIFVddpDebugPrint = FALSE;
			return;
		}
		trace_inited = TRUE;
	}

	/*
	**  Now dump out some stuff about the field that's being
	**  parsed (for derivations).
	*/
	SIfprintf(trfile, ERx("Preparing to parse derivation string for "));
	if (fld->fltag == FREGULAR)
	{
		SIfprintf(trfile, ERx("field `%s' in form `%s'.\n"),
			hdr->fhdname, frm->frname);
	}
	else
	{
		SIfprintf(trfile, ERx("column `%s' of table field `%s' in form `%s'.\n"),
			hdr->fhdname, fld->fld_var.fltblfld->tfhdr.fhdname,
			frm->frname);
	}
	SIfprintf(trfile, ERx("Derivation string is:\n `%s'\n"),
		type->ftvalstr);
}


/*{
** Name:	IIFVdtpDrvTracePrint - Print out derivation trace info.
**
** Description:
**	Given a trace string, print it to "trfile" if tracing is set.
**	String to print out is assumed to be less than 80 chars for now.
**
**	MAY WANT TO ADD TIME STAMP LATER ON.
**
** Inputs:
**	string		Trace string to print.
**
** Outputs:
**
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	08/20/89 (dkh) - Initial version.
*/
VOID
IIFVdtpDrvTracePrint(string)
char	*string;
{
	if (IIFVddpDebugPrint)
	{
		SIfprintf(trfile, string);
	}
}


/*{
** Name:	IIFVddDbgDump - Routine to dump out field dependencies.
**
** Description:
**	Debugging routine to dump out field dependenices of a derived
**	field.
**
**	Should someday indent the output and to dump the DAGGVAL struct too.
**
**	MAY WANT TO ADD TIMESTAMP IN THE FUTURE.
**
** Inputs:
**	drv		DERIVE structure for derived field.
**
** Outputs:
**
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	06/20/89 (dkh) - Initial version.
*/
VOID
IIFVddDbgDump(drv)
DERIVE	*drv;
{
	DRVREL	*srclist;
	DRVAGG	*agginfo;

	if (!IIFVddpDebugPrint)
	{
		return;
	}

	SIfprintf(trfile, ERx("Dumping DERIVE at `%p'.\n"), drv);
	SIfprintf(trfile, ERx("Member `srclist' points to `%p'.\n"),
		drv->srclist);
	if ((srclist = drv->srclist) != NULL)
	{
		SIfprintf(trfile, ERx("Dumping `srclist' chain.\n"));
	}
	while (srclist != NULL)
	{
		SIfprintf(trfile, ERx("Dumping `DRVREL' at `%p'.\n"), srclist);
		SIfprintf(trfile, ERx("Member `fld' points to `%p'.\n"),
			srclist->fld);
		SIfprintf(trfile, ERx("Member `colno' has value `%x'.\n"),
			srclist->colno);
		SIfprintf(trfile, ERx("Member `next' points to `%p'.\n"),
			srclist->next);
		srclist = srclist->next;
	}
	SIfprintf(trfile, ERx("Member `deplist' points to `%p'.\n"),
		drv->deplist);
	if ((srclist = drv->deplist) != NULL)
	{
		SIfprintf(trfile, ERx("Dumping `deplist' chain.\n"));
	}
	while (srclist != NULL)
	{
		SIfprintf(trfile, ERx("Dumping `DRVREL' at `%p'.\n"), srclist);
		SIfprintf(trfile, ERx("Member `fld' points to `%p'.\n"),
			srclist->fld);
		SIfprintf(trfile, ERx("Member `colno' has value `%x'.\n"),
			srclist->colno);
		SIfprintf(trfile, ERx("Member `next' points to `%p'.\n"),
			srclist->next);
		srclist = srclist->next;
	}
	SIfprintf(trfile, ERx("Member `agginfo' points to `%p'.\n"),
		agginfo = drv->agginfo);
	if (drv->agginfo != NULL)
	{
		SIfprintf(trfile, ERx("Dumping `agginfo'.\n"));
		SIfprintf(trfile, ERx("Member `aggflags' = `%p'.\n"),
			agginfo->aggflags);
		SIfprintf(trfile, ERx("Member cntagg' points to `%p'.\n"),
			agginfo->cntagg);
		SIfprintf(trfile, ERx("Member maxagg' points to `%p'.\n"),
			agginfo->maxagg);
		SIfprintf(trfile, ERx("Member minagg' points to `%p'.\n"),
			agginfo->minagg);
		SIfprintf(trfile, ERx("Member sumagg' points to `%p'.\n"),
			agginfo->sumagg);
		SIfprintf(trfile, ERx("Member avgagg' points to `%p'.\n"),
			agginfo->avgagg);
	}
	SIfprintf(trfile, ERx("Member `drvflags' = `%x'.\n"), drv->drvflags);
}


/*{
** Name:	IIFVdnDumpNode - Dump information about a DNODE.
**
** Description:
**	Routine just dumps out the various members of the DNODE structure.
**
** Inputs:
**	node		DNODE to dump out.
**
** Outputs:
**
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	06/20/89 (dkh) - Initial version.
*/
VOID
IIFVdnDumpNode(node)
DNODE	*node;
{
	char	*nodetype;

	if (!IIFVddpDebugPrint)
	{
		return;
	}

	SIfprintf(trfile, ERx("Derivation string `%s'.\n"),
		vl_curtype->ftvalstr);
	SIfprintf(trfile, ERx("Dumping Node at `%p'.\n"), node);
	switch (node->nodetype)
	{
	    case ICONST_NODE:
		nodetype = ERx("ICONST");
		break;

	    case FCONST_NODE:
		nodetype = ERx("FCONST");
		break;

	    case SCONST_NODE:
		nodetype = ERx("SCONST");
		break;

	    case PLUS_NODE:
		nodetype = ERx("PLUS");
		break;

	    case MINUS_NODE:
		nodetype = ERx("MINUS");
		break;

	    case MULT_NODE:
		nodetype = ERx("MULT");
		break;

	    case DIV_NODE:
		nodetype = ERx("DIV");
		break;

	    case EXP_NODE:
		nodetype = ERx("EXP");
		break;

	    case UMINUS_NODE:
		nodetype = ERx("UMINUS");
		break;

	    case MAX_NODE:
		nodetype = ERx("MAX");
		break;

	    case MIN_NODE:
		nodetype = ERx("MIN");
		break;

	    case AVG_NODE:
		nodetype = ERx("AVG");
		break;

	    case SUM_NODE:
		nodetype = ERx("SUM");
		break;

	    case CNT_NODE:
		nodetype = ERx("COUNT");
		break;

	    case SFLD_NODE:
		nodetype = ERx("SIMPE FIELD");
		break;

	    case TCOL_NODE:
		nodetype = ERx("TF COLUMN");
		break;

	    case DCONST_NODE:
		nodetype = ERx("DCONST");
		break;

	    default:
		nodetype = ERx("UNKNOWN");
		break;
	}

	SIfprintf(trfile, ERx("Node type is `%s'.\n"), nodetype);
	SIfprintf(trfile, ERx("Member `fld' points to `%p'.\n"), node->fld);
	SIfprintf(trfile, ERx("Member `colno' has value `%x'.\n"), node->colno);
	SIfprintf(trfile, ERx("Member `result' points to `%p'.\n"),
		node->result);
	SIfprintf(trfile, ERx("Member `cresult' points to `%p'.\n"),
		node->cresult);
	SIfprintf(trfile, ERx("Member `fdesc' points to `%p'.\n"), node->fdesc);
	SIfprintf(trfile, ERx("Member `left' points to `%p'.\n"), node->left);
	SIfprintf(trfile, ERx("Member `right' points to `%p'.\n"),
		node->right);
	SIfprintf(trfile, ERx("Member `agghdr' points to `%p'.\n"),
		node->agghdr);
	SIfprintf(trfile, ERx("Member `workspace' = `%p'.\n"), node->workspace);
	SIfprintf(trfile, ERx("Member `fid' = `%x'.\n"), node->fid);
	SIfprintf(trfile, ERx("Member `drv_excb' points to `%p'.\n"),
		node->drv_excb);
	SIfprintf(trfile, ERx("Member `res_inx' = `%x'.\n"), node->res_inx);
	SIfprintf(trfile, ERx("Member `cres_inx' = `%x'.\n"), node->cres_inx);
	SIfprintf(trfile, ERx("Member `coer_fid' = `%x'.\n\n"),
		node->coer_fid);
	if (node->left)
	{
		SIfprintf(trfile, ERx("Dumping left branch.\n"));
		IIFVdnDumpNode(node->left);
	}
	else
	{
		SIfprintf(trfile, ERx("Left branch is NULL.\n"));
	}
	if (node->right)
	{
		SIfprintf(trfile, ERx("Dumping right branch.\n"));
		IIFVdnDumpNode(node->right);
	}
	else
	{
		SIfprintf(trfile, ERx("Right branch is NULL.\n"));
	}
}


/*{
** Name:	IIVFmnMakeNode - Create a DNODE for the derivation parser.
**
** Description:
**	Create and return pointer to a DNODE structure.  Called by
**	the derivaiton parser as it parses a derivation formula
**	and creates a derivation tree.
**
** Inputs:
**	type		Type of node to create.  See dnode.h for list of
**			valid nodes.
**	value		If a constant node, this contains the constant value.
**	sfldnm		If a SFLD_NODE, this is the name of the simple field.
**			If a TCOL_NODE, this is the name of the table field.
**	colnm		If a TCOL_NODE, this is the name of the tf column.
**
** Outputs:
**
**	Returns:
**		node	Pointer to a created DNODE strucuture.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	06/20/89 (dkh) - Initial version.
*/
DNODE *
IIFVmnMakeNode(type, value, sfldnm, colnm)
i4		type;
IIFVdstype	*value;
char		*sfldnm;
char		*colnm;
{
	DNODE		*node;
	DB_DATA_VALUE	tdbv;
	DB_DATA_VALUE	dbv;
	DB_DATA_VALUE	*ndbv;
	ADF_CB		*adfcb;
	FIELD		*fld;
	TBLFLD		*tf;
	FLDCOL		*col;
	i4		i4val;
	f8		f8val;
	DB_TEXT_STRING	*text;
	FLDTYPE		*fldtype;
	FLDVAL		*fldval;
	bool		junk;
	char		fld_dtype[100];
	char		fld_cval[fdVSTRLEN + 1];
	bool		bypass = FALSE;

	adfcb = FEadfcb();
	if ((node = (DNODE *) MEreqmem(0, sizeof(DNODE), TRUE,
		(STATUS *)NULL)) == NULL)
	{
	    IIUGbmaBadMemoryAllocation(ERx("IIFVmnMakeNode"));
	    return(NULL);
	}

	node->nodetype = type;
	node->workspace = NO_AGG;

	switch(type)
	{
	    case ICONST_NODE:
	    case FCONST_NODE:
	    case SCONST_NODE:
	    case DCONST_NODE:
		if (type == ICONST_NODE)
		{
			i4val = value->I4_type;
			dbv.db_datatype = DB_INT_TYPE;
			dbv.db_length = sizeof(i4);
			dbv.db_prec = 0;
			dbv.db_data = (PTR) &i4val;
		}
		else if (type == FCONST_NODE)
		{
			f8val = value->F8_type;
			dbv.db_datatype = DB_FLT_TYPE;
			dbv.db_length = sizeof(f8);
			dbv.db_prec = 0;
			dbv.db_data = (PTR) &f8val;
		}
		else if (type == DCONST_NODE)
		{
			dbv.db_datatype = DB_DEC_TYPE;
			dbv.db_length = value->dec_type->db_length;
			dbv.db_prec = value->dec_type->db_prec;
			dbv.db_data = value->dec_type->db_data;
		}
		else
		{
			text = (DB_TEXT_STRING *) value->string_type;
			dbv.db_datatype = DB_VCH_TYPE;
			dbv.db_length = text->db_t_count + DB_CNTSIZE;
			dbv.db_prec = 0;
			dbv.db_data = (PTR) text;
		}
		
		tdbv.db_datatype = vl_curtype->ftdatatype;
		tdbv.db_length = vl_curtype->ftlength;
		tdbv.db_prec = vl_curtype->ftprec;

		/*
		**  Setup constant value and do any
		**  necessary conversions.  Put out
		**  error and return NULL if can't
		**  convert.
		**
		**  We make a special case here for floats and decimal
		**  being converted to money.  Don't do it.
		**  This is necessary to preserve money semantics
		**  in arithmetic operations.  Converting to
		**  money too early will cause precision to
		**  be lost.
		*/
		if ((ndbv = (DB_DATA_VALUE *) MEreqmem(0, sizeof(DB_DATA_VALUE),
			TRUE, (STATUS *)NULL)) == NULL)
		{
			IIUGbmaBadMemoryAllocation(ERx("IIFVmnMakeNode"));
			return(NULL);
		}
		if ((type == FCONST_NODE || type == DCONST_NODE) &&
			abs(tdbv.db_datatype) == DB_MNY_TYPE)
		{
			bypass = TRUE;
			tdbv.db_length = dbv.db_length;
			tdbv.db_datatype = dbv.db_datatype;
			tdbv.db_prec = dbv.db_prec;
		}
		MEcopy((PTR) &tdbv, (u_i2) sizeof(DB_DATA_VALUE), (PTR) ndbv);
		if ((ndbv->db_data = (PTR) MEreqmem(0, tdbv.db_length, FALSE,
			(STATUS *)NULL)) == NULL)
		{
			IIUGbmaBadMemoryAllocation(ERx("IIFVmnMakeNode"));
			return(NULL);
		}
		if (bypass)
		{
			MEcopy(dbv.db_data, (u_i2)dbv.db_length, ndbv->db_data);
		}
		else
		{
			if (afe_cvinto(adfcb, &dbv, ndbv) != OK)
			{
				/*
				**  Can't convert, return NULL.
				*/
				vl_typstr(ndbv, fld_dtype);
				if (type == ICONST_NODE)
				{
					CVla(i4val, fld_cval);
				}
				else if (type == FCONST_NODE ||
					type == DCONST_NODE)
				{
					STcopy(IIFVfloat_str, fld_cval);
				}
				else
				{
					/*
					**  Must be a SCONST_NODE.
					*/
					MEcopy((PTR) text->db_t_text,
						(u_i2) text->db_t_count,
						(PTR) fld_cval);
					fld_cval[text->db_t_count] = EOS;
				}
				if (IIFVcfCurFld->fltag == FREGULAR)
				{
					IIFVdeErr(E_FI20C6_8390, 2,
						(PTR)fld_cval, (PTR)fld_dtype);
				}
				else
				{
					IIFVdeErr(E_FI20C7_8391, 3,
					  (PTR)IIFVcfCurFld->fld_var.fltblfld->
						tfhdr.fhdname, (PTR)fld_cval,
						(PTR)fld_dtype);
				}
				MEfree(node);
				MEfree(ndbv->db_data);
				MEfree(ndbv);
				node = NULL;
				break;
			}
		}
		node->result = ndbv;
		node->res_inx = IIFVodcOpdCnt++;
		break;

	    case PLUS_NODE:
	    case MINUS_NODE:
	    case MULT_NODE:
	    case DIV_NODE:
	    case EXP_NODE:
	    case UMINUS_NODE:
		/*
		**  Coercion of datatypes and result datatypes will
		**  be done by separate call from parser.
		IIFVorcOprCnt++;
		*/
		break;

	    case MAX_NODE:
	    case MIN_NODE:
	    case AVG_NODE:
	    case SUM_NODE:
	    case CNT_NODE:
		/*
		**  Setup of aggregate info, etc. will be
		**  done via a separate call from parser.
		*/
		break;

	    case SFLD_NODE:
		/*
		**  Look for field name.
		**  If not found, error and return NULL.
		*/
		IIFVocOnlyCnst = FALSE;
		if ((fld = FDfndfld(vl_curfrm, sfldnm, &junk)) == NULL)
		{
			/*
			**  Can't find field, put out error message
			**  and return NULL.
			*/
			if (IIFVcfCurFld->fltag == FREGULAR)
			{
				IIFVdeErr(E_FI209F_8351, 1, (PTR) sfldnm);
			}
			else
			{
				fld = IIFVcfCurFld;
				IIFVdeErr(E_FI20A0_8352, 2,
				    (PTR) fld->fld_var.fltblfld->tfhdr.fhdname,
				    (PTR) sfldnm);
			}
			MEfree(node);
			node = NULL;
			break;
		}
		if (fld->fltag != FREGULAR)
		{
			/*
			**  Not a simple field, put out error message
			**  and return NULL.
			*/
			if (IIFVcfCurFld->fltag == FREGULAR)
			{
				IIFVdeErr(E_FI20A1_8353, 1, (PTR) sfldnm);
			}
			else
			{
				fld = IIFVcfCurFld;
				IIFVdeErr(E_FI20A2_8354, 2,
				    (PTR) fld->fld_var.fltblfld->tfhdr.fhdname,
				    (PTR) sfldnm);
			}
			MEfree(node);
			node = NULL;
			break;
		}
		if (IIFVcfCurFld->fltag == FTABLE)
		{
			/*
			**  A table field column can not depend on
			**  a simple field.  Put out error message
			**  and return NULL.
			*/
			IIFVdeErr(E_FI20A3_8355, 2,
			    (PTR) IIFVcfCurFld->fld_var.fltblfld->tfhdr.fhdname,
			    (PTR) sfldnm);
			MEfree(node);
			node = NULL;
			break;
		}
		if (fld == IIFVcfCurFld)
		{
			/*
			**  A simple field can't depend on itself.
			*/
			IIFVdeErr(E_FI20A4_8356, 0);
			MEfree(node);
			node = NULL;
			break;
		}
		node->fld = fld;
		node->colno = NOCOLNO;
		fldtype = FDgettype(fld);
		fldval = FDgetval(fld);
		/*
		**  Point directly at field value for simple field.
		**  If initializing form for real.  Otherwise,
		**  allocate DBV with no data allocated.
		*/
		if (vl_syntax)
		{
			if ((node->result = (DB_DATA_VALUE *) MEreqmem(0,
				sizeof(DB_DATA_VALUE), TRUE,
				(STATUS *)NULL)) == NULL)
			{
				/*
				**  Never returns.
				*/
				IIUGbmaBadMemoryAllocation(ERx("makenode-sf"));
			}
			node->result->db_datatype = fldtype->ftdatatype;
			node->result->db_prec = fldtype->ftprec;
			node->result->db_length = fldtype->ftlength;
		}
		else
		{
			/*
			**  Note that this is a little dangerous
			**  with respect to constant value propagation.
			**  However, IIFVccConstChk() has taken this
			**  into account.  If the routine is changed,
			**  this may need to change as well.
			*/
			node->result = fldval->fvdbv;
		}
		node->res_inx = IIFVodcOpdCnt++;

		break;

	    case TCOL_NODE:
		/*
		**  Look for table field & column.
		**  If not found, error and return NULL.
		*/
		IIFVocOnlyCnst = FALSE;
		if ((fld = FDfndfld(vl_curfrm, sfldnm, &junk)) == NULL)
		{
			/*
			**  Can't find table field, put out error
			**  message and return NULL.
			*/
			if (IIFVcfCurFld->fltag == FREGULAR)
			{
				IIFVdeErr(E_FI20A5_8357, 1, (PTR) sfldnm);
			}
			else
			{
				fld = IIFVcfCurFld;
				IIFVdeErr(E_FI20A6_8358, 2,
				    (PTR) fld->fld_var.fltblfld->tfhdr.fhdname,
				    (PTR) sfldnm);
			}
			MEfree(node);
			node = NULL;
			break;
		}
		if (fld->fltag != FTABLE)
		{
			/*
			**  Not a table field, put out error
			**  message and return NULL.
			*/
			if (IIFVcfCurFld->fltag == FREGULAR)
			{
				IIFVdeErr(E_FI20A7_8359, 1, (PTR) sfldnm);
			}
			else
			{
				fld = IIFVcfCurFld;
				IIFVdeErr(E_FI20A8_8360, 2,
				    (PTR) fld->fld_var.fltblfld->tfhdr.fhdname,
				    (PTR) sfldnm);
			}
			MEfree(node);
			node = NULL;
			break;
		}
		if (IIFVcfCurFld->fltag == FREGULAR)
		{
			/*
			**  A simple field can not depend on
			**  just a table field column.  But a
			**  simple field can depend on an
			**  aggregate of a table field column.
			*/
			IIFVdeErr(E_FI20A9_8361, 2, (PTR) sfldnm, (PTR) colnm);
			MEfree(node);
			node = NULL;
			break;
		}
		if (IIFVcfCurFld != fld)
		{
			/*
			**  A table field column can only depend
			**  on another column in the SAME table
			**  field.
			*/
			IIFVdeErr(E_FI20AA_8362, 3,
			    (PTR) IIFVcfCurFld->fld_var.fltblfld->tfhdr.fhdname,
			    (PTR) sfldnm, (PTR) colnm);
			MEfree(node);
			node = NULL;
			break;
		}
		tf = fld->fld_var.fltblfld;
		if ((col = (FLDCOL *) FDfind((i4 **) tf->tfflds, colnm,
			tf->tfcols, IIFDgchGetColHdr)) == NULL)
		{
			/*
			**  Can't find table field, put out error
			**  message and return NULL.
			*/
			if (IIFVcfCurFld->fltag == FREGULAR)
			{
				IIFVdeErr(E_FI20AB_8363, 2, (PTR) sfldnm,
					(PTR) colnm);
			}
			else
			{
				fld = IIFVcfCurFld;
				IIFVdeErr(E_FI20AC_8364, 3,
				    (PTR) fld->fld_var.fltblfld->tfhdr.fhdname,
				    (PTR) sfldnm, (PTR) colnm);
			}
			MEfree(node);
			node = NULL;
			break;
		}
		tf = fld->fld_var.fltblfld;
		node->fld = fld;
		if ((node->colno = col->flhdr.fhseq) == vl_curhdr->fhseq)
		{
			/*
			**  A table field column can not depend
			**  on itself.
			*/
			IIFVdeErr(E_FI20AD_8365, 1, (PTR) sfldnm);
			MEfree(node);
			node = NULL;
			break;
		}
		fldtype = &(col->fltype);
		if ((node->result = (DB_DATA_VALUE *) MEreqmem(0,
			sizeof(DB_DATA_VALUE), TRUE, (STATUS *)NULL)) == NULL)
		{
			IIUGbmaBadMemoryAllocation(ERx("IIFVmnMakeNode"));
			break;
		}
		if (!vl_syntax && (node->result->db_data = (PTR) MEreqmem(0,
			fldtype->ftlength, TRUE, (STATUS *)NULL)) == NULL)
		{
			IIUGbmaBadMemoryAllocation(ERx("IIFVmnMakeNode"));
			break;
		}
		ndbv = node->result;
		ndbv->db_datatype = fldtype->ftdatatype;
		ndbv->db_length = fldtype->ftlength;
		ndbv->db_prec = fldtype->ftprec;
		node->res_inx = IIFVodcOpdCnt++;
		break;

	    default:
		/*
		**  Unknown node type requested, put out
		**  error message, free memory and return NULL.
		*/
		i4val = type;
		if (IIFVcfCurFld->fltag == FREGULAR)
		{
			IIFVdeErr(E_FI20AE_8366, 1, (PTR) &i4val);
		}
		else
		{
			fld = IIFVcfCurFld;
			IIFVdeErr(E_FI20AF_8367, 2,
				(PTR) fld->fld_var.fltblfld->tfhdr.fhdname,
				(PTR) &i4val);
		}
		MEfree(node);
		node = NULL;
		break;
	}

	return(node);
}



/*{
** Name:	IIFVmsMathSetup - Set up a math node.
**
** Description:
**	Create and return pointer to a math operator node.
**	Checks for datatype compatibility and ADF function instance
**	information for execution is also done at this time.
**
** Inputs:
**	nodetype	Type of node to create.  Must be a math operator type.
**	left		Branch containing left operand.
**	right		Branch containing right operand.
**
** Outputs:
**
**	Returns:
**		node	Pointer to math node that's been created.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	06/20/89 (dkh) - Initial version.
*/
DNODE *
IIFVmsMathSetup(nodetype, left, right)
i4	nodetype;
DNODE	*left;
DNODE	*right;
{
	i4		cvlen;
	char		*operator;
	DB_DATA_VALUE	*oparr[2];
	DB_DATA_VALUE	*coparr[2];
	DB_DATA_VALUE	resdbv;
	DB_DATA_VALUE	cldbv;
	DB_DATA_VALUE	crdbv;
	AFE_OPERS	ops;
	AFE_OPERS	cops;
	ADI_OP_ID	opid;
	ADI_FI_DESC	fdesc;
	ADF_CB		*adfcb;
	DNODE		*node;
	i4		is_uminus = FALSE;

	adfcb = FEadfcb();
	if ((node = (DNODE *) MEreqmem(0, sizeof(DNODE), TRUE,
		(STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("IIFVmsMathSetup"));
		return(NULL);
	}

	node->nodetype = nodetype;
	node->workspace = NO_AGG;

	switch(nodetype)
	{
	    case PLUS_NODE:
		operator = ERx("+");
		break;

	    case MINUS_NODE:
		operator = ERx("-");
		break;

	    case MULT_NODE:
		operator = ERx("*");
		break;

	    case DIV_NODE:
		operator = ERx("/");
		break;

	    case EXP_NODE:
		operator = ERx("**");
		break;

	    case UMINUS_NODE:
		operator = ERx(" -");
		is_uminus = TRUE;
		break;

	    default:
		/*
		**  Bad node type passed, put out error message
		**  and return NULL;
		*/
		cvlen = nodetype;
		if (IIFVcfCurFld->fltag == FREGULAR)
		{
			IIFVdeErr(E_FI20B0_8368, 1, (PTR) &cvlen);
		}
		else
		{
			IIFVdeErr(E_FI20B1_8369, 2,
			    (PTR)IIFVcfCurFld->fld_var.fltblfld->tfhdr.fhdname,
			    (PTR) &cvlen);
		}
		return(NULL);
	}
	IIFVorcOprCnt++;

	/*
	**  Find operator id.
	*/
	if (afe_opid(adfcb, operator, &opid) != OK)
	{
		/*
		**  Can't find operator in ADF list.  Put
		**  out error and return NULL.
		*/
		if (IIFVcfCurFld->fltag == FREGULAR)
		{
			IIFVdeErr(E_FI20B2_8370, 1, (PTR) operator);
		}
		else
		{
			IIFVdeErr(E_FI20B3_8371, 2,
			    (PTR) IIFVcfCurFld->fld_var.fltblfld->tfhdr.fhdname,
			    (PTR) operator);
		}
		return(NULL);
	}

	/*
	**  Set up to find function instance descriptor.
	*/
	ops.afe_ops = oparr;
	ops.afe_ops[0] = left->result;
	cops.afe_ops = coparr;
	cops.afe_ops[0] = &cldbv;
	if (is_uminus)
	{
		ops.afe_opcnt = 1;
		cops.afe_opcnt = 1;
	}
	else
	{
		ops.afe_opcnt = 2;
		ops.afe_ops[1] = right->result;
		cops.afe_ops[1] = &crdbv;
		cops.afe_opcnt = 2;
	}
	if (afe_fdesc(adfcb, opid, &ops, &cops, &resdbv, &fdesc) != OK)
	{
		/*
		**  Can't do math operation on the datatypes in
		**  the left and right branches, they may not be
		**  compatible.  Put out error
		**  message and return NULL;
		*/
		if (IIFVcfCurFld->fltag == FREGULAR)
		{
			IIFVdeErr(E_FI20B4_8372, 1, (PTR) operator);
		}
		else
		{
			IIFVdeErr(E_FI20B5_8373, 2,
			    (PTR) IIFVcfCurFld->fld_var.fltblfld->tfhdr.fhdname,
			    (PTR) operator);
		}
		return(NULL);
	}

	if (cldbv.db_datatype != left->result->db_datatype)
	{
		if ((left->cresult = (DB_DATA_VALUE *) MEreqmem(0,
			sizeof(DB_DATA_VALUE), TRUE, (STATUS *) NULL)) == NULL)
		{
			IIUGbmaBadMemoryAllocation(ERx("IIFVmsMathSetup"));
			return(NULL);
		}
		MEcopy((PTR) &cldbv, (u_i2) sizeof(DB_DATA_VALUE),
			(PTR) left->cresult);
		if ((left->cresult->db_data = (PTR) MEreqmem(0,
			cldbv.db_length, TRUE, (STATUS *)NULL)) == NULL)
		{
			MEfree(left->cresult);
			IIUGbmaBadMemoryAllocation(ERx("IIFVmsMathSetup"));
			return(NULL);
		}
		left->cres_inx = IIFVodcOpdCnt++;

		/*
		**  Since we are given a datatype to coerce to, there
		**  must be a coerce fid available.
		*/
		(VOID) adi_ficoerce(adfcb, left->result->db_datatype,
			left->cresult->db_datatype, &(left->coer_fid));
	}

	if (!is_uminus && crdbv.db_datatype != right->result->db_datatype)
	{
		if ((right->cresult = (DB_DATA_VALUE *) MEreqmem(0,
			sizeof(DB_DATA_VALUE), TRUE, (STATUS *) NULL)) == NULL)
		{
			IIUGbmaBadMemoryAllocation(ERx("IIFVmsMathSetup"));
			return(NULL);
		}
		MEcopy((PTR) &crdbv, (u_i2) sizeof(DB_DATA_VALUE),
			(PTR) right->cresult);
		if ((right->cresult->db_data = (PTR) MEreqmem(0,
			crdbv.db_length, TRUE, (STATUS *)NULL)) == NULL)
		{
			MEfree(right->cresult);
			IIUGbmaBadMemoryAllocation(ERx("IIFVmsMathSetup"));
			return(NULL);
		}
		right->cres_inx = IIFVodcOpdCnt++;
		/*
		**  Since we are given a datatype to coerce to, there
		**  must be a coerce fid available.
		*/
		(VOID) adi_ficoerce(adfcb, right->result->db_datatype,
			right->cresult->db_datatype, &(right->coer_fid));
	}

	if ((node->result = (DB_DATA_VALUE *) MEreqmem(0,
		sizeof(DB_DATA_VALUE), TRUE, (STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("IIFVmsMathSetup"));
		return(NULL);
	}

	MEcopy((PTR) &resdbv, (u_i2) sizeof(DB_DATA_VALUE), (PTR) node->result);
	if ((node->result->db_data = (PTR) MEreqmem(0,
		resdbv.db_length, TRUE, (STATUS *)NULL)) == NULL)
	{
		MEfree(node->result);
		IIUGbmaBadMemoryAllocation(ERx("IIFVmsMathSetup"));
		return(NULL);
	}
	node->res_inx = IIFVodcOpdCnt++;

	if ((node->fdesc = (ADI_FI_DESC *) MEreqmem(0, sizeof(ADI_FI_DESC),
		TRUE, (STATUS *)NULL)) == NULL)
	{
		MEfree(node->result->db_data);
		MEfree(node->result);
		IIUGbmaBadMemoryAllocation(ERx("IIFVmsMathSetup"));
		return(NULL);
	}

	MEcopy((PTR) &fdesc, (u_i2) sizeof(ADI_FI_DESC), (PTR) node->fdesc);

	node->left = left;

	if (!is_uminus)
	{
		node->right = right;
	}

	return(node);
}




/*{
** Name:	IIFVasAggSetup - Set up aggregate for runtime execution.
**
** Description:
**	Given an aggregate node, sets up necessary runtime execution
**	information such as function instance information and
**	workspace sizes.
**
** Inputs:
**	node		Pre-allocated aggregate node.
**	tfname		Name of table field.
**	colname		Name of column to run aggregate on.
**
** Outputs:
**
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	06/20/89 (dkh) - Initial version.
*/
VOID
IIFVasAggSetup(node, tfname, colname)
DNODE	*node;
char	*tfname;
char	*colname;
{
	ADI_FI_ID	fid;
	i4		cvlen;
	char		*agg;
	DB_DATA_VALUE	resdbv;
	DB_DATA_VALUE	tdbv;
	ADF_CB		*adfcb;
	FIELD		*fld;
	TBLFLD		*tf;
	FLDCOL		*col;
	FLDTYPE		*type;
	i4		workspace;
	bool		junk;
	DB_LANG		olang;
	i4		need_null = TRUE;

	adfcb = FEadfcb();

	IIFVocOnlyCnst = FALSE;
	if (IIFVcfCurFld->fltag == FTABLE)
	{
		/*
		**  Don't allow a table field column to
		**  depend on the aggregate of a table
		**  field column.
		*/
		IIFVdeErr(E_FI20BC_8380, 3,
			(PTR) IIFVcfCurFld->fld_var.fltblfld->tfhdr.fhdname,
			(PTR) tfname, (PTR) colname);
		return;
	}

	/*
	**  Find table field and column name.  Complain if neither one
	**  is found.
	*/
	if ((fld = FDfndfld(vl_curfrm, tfname, &junk)) == NULL)
	{
		/*
		**  Can't find table field.  Put out error
		**  message and jump out of parser.
		*/
		if (IIFVcfCurFld->fltag == FREGULAR)
		{
			IIFVdeErr(E_FI20B6_8374, 1, tfname);
		}
		else
		{
			/*
			**  This is probably not necessary
			**  due to check at beginning of routine.
			*/
			IIFVdeErr(E_FI20B7_8375, 2,
			    (PTR) IIFVcfCurFld->fld_var.fltblfld->tfhdr.fhdname,
			    (PTR) tfname);
		}
		return;
	}

	if (fld->fltag != FTABLE)
	{
		/*
		**  Not a table field.  Put out error
		**  message and jump put of parser.
		*/
		if (IIFVcfCurFld->fltag == FREGULAR)
		{
			IIFVdeErr(E_FI20B8_8376, 1, (PTR) tfname);
		}
		else
		{
			/*
			**  This is probably not necessary
			**  due to check at beginning of routine.
			*/
			IIFVdeErr(E_FI20B9_8377, 2,
			    (PTR) IIFVcfCurFld->fld_var.fltblfld->tfhdr.fhdname,
			    (PTR) tfname);
		}
		return;
	}
	tf = fld->fld_var.fltblfld;
	if ((col = (FLDCOL *) FDfind((i4 **) tf->tfflds, colname,
		tf->tfcols, IIFDgchGetColHdr)) == NULL)
	{
		/*
		**  Can't find column in table field.  Put out
		**  error message and jump out of parser.
		*/
		if (IIFVcfCurFld->fltag == FREGULAR)
		{
			IIFVdeErr(E_FI20BA_8378, 2, (PTR) tfname, (PTR)colname);
		}
		else
		{
			/*
			**  This is probably not necessary
			**  due to check at beginning of routine.
			*/
			IIFVdeErr(E_FI20BB_8379, 3,
			    (PTR) IIFVcfCurFld->fld_var.fltblfld->tfhdr.fhdname,
			    (PTR) tfname, (PTR) colname);
		}
		return;
	}
	node->colno = col->flhdr.fhseq;
	node->fld = fld;

	type = &(col->fltype);

	switch(node->nodetype)
	{
	    case SUM_NODE:
		agg = ERx("sum");
		break;

	    case MAX_NODE:
		agg = ERx("max");
		break;

	    case MIN_NODE:
		agg = ERx("min");
		break;

	    case AVG_NODE:
		agg = ERx("avg");
		break;

	    case CNT_NODE:
		agg = ERx("count");
		need_null = FALSE;
		break;

	    default:
		/*
		**  Bad node type passed, put out error message
		**  and jump out of parser.
		*/
		cvlen = node->nodetype;
		IIFVdeErr(E_FI20BD_8381, 1, (PTR) &cvlen);
		return;
	}

	tdbv.db_datatype = type->ftdatatype;
	tdbv.db_length = type->ftlength;
	tdbv.db_prec = type->ftprec;

	/*
	**  Change language to be SQL to get correct semantics
	**  for aggregates.
	*/
	olang = IIAFdsDmlSet(DB_SQL);
	if (afe_agfind(adfcb, agg, &tdbv, &fid,
		&resdbv, &workspace) != OK)
	{
		/*
		**  Reset language before exiting.
		*/
		(VOID) IIAFdsDmlSet(olang);
		IIFVdeErr(E_FI20CC_8396, 1, agg);
	}

	(VOID) IIAFdsDmlSet(olang);

	node->fid = fid;
	node->workspace = workspace;

	/*
	**  Next check is necessary since call to afe_agfind does not
	**  appear to set the nullability of the result DBV correctly.
	*/
	if (!AFE_NULLABLE_MACRO(resdbv.db_datatype) && need_null)
	{
		AFE_MKNULL_MACRO(&resdbv);
	}

	if ((node->result = (DB_DATA_VALUE *) MEreqmem(0,
		sizeof(DB_DATA_VALUE), TRUE, (STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("IIFVasAggSetup"));
		return;
	}

	MEcopy((PTR) &resdbv, (u_i2) sizeof(DB_DATA_VALUE), (PTR) node->result);
	if ((node->result->db_data = (PTR) MEreqmem(0,
		resdbv.db_length, TRUE, (STATUS *)NULL)) == NULL)
	{
		MEfree(node->result);
		IIUGbmaBadMemoryAllocation(ERx("IIFVasAggSetup"));
		return;
	}
	node->res_inx = IIFVodcOpdCnt++;

	return;
}




/*{
** Name:	IIFVccConstChk - Check for constant derivations.
**
** Description:
**	Check the source fields for a derived field.  If all source
**	fields (directly or indirectly) are constant derivations,
**	then this derived field can also be marked as being a
**	constant derivation.
**
**	Note that in order for this to work correctly, this routine
**	needs to do evaluation of constant derivations via a call
**	to IIFVddeDoDeriveEval().  In addition, it is assume that
**	IIFVcvConstVal is set to TRUE before this routine is called.
**
**	This correctly handles (node) result values pointing directly
**	to simple fields.  If this changes, all change corresponding
**	code in IIFVmnMakeNode().
**
** Inputs:
**	hdr		FLDHDR structure for derived field to check.
**
** Outputs:
**
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	08/18/89 (dkh) - Initial version.
*/
VOID
IIFVccConstChk(frm, hdr, type)
FRAME	*frm;
FLDHDR	*hdr;
FLDTYPE	*type;
{
	DRVREL		*dlist;
	FLDHDR		*thdr;
	FLDCOL		*col;
	FLDTYPE		*ttype;
	DB_DATA_VALUE	*tdb;

	if (hdr->fhdrv != NULL)
	{
		/*
		**  If field is already a constant, then just return;
		*/
		if (hdr->fhdrv->drvflags & fhDRVCNST)
		{
			/*
			**  Evaluate constant value if necessary.
			*/
			if (!(hdr->fhdrv->drvflags & fhDRVDCNST) &&
				IIFVddeDoDeriveEval(frm,(DNODE *)type->ftvalchk,
					hdr, &tdb) == OK)
			{
				hdr->fhdrv->drvflags |= fhDRVDCNST;
			}
			return;
		}
		/*
		**  Return if this field depends on no other fields.
		*/
		if (hdr->fhdrv->srclist == NULL)
		{
			return;
		}

		/*
		**  Otherwise, we need to check all of the field's
		**  source values to see if they are all constants
		**  too.
		*/
		for (dlist = hdr->fhdrv->srclist; dlist != NULL;
			dlist = dlist->next)
		{
			if (dlist->colno == NOCOLNO)
			{
				thdr = &(dlist->fld->fld_var.flregfld->flhdr);
				ttype = &(dlist->fld->fld_var.flregfld->fltype);
			}
			else
			{
				col = dlist->fld->fld_var.fltblfld->
					tfflds[dlist->colno];
				thdr = &(col->flhdr);
				ttype = &(col->fltype);
			}

			/*
			**  Before checking field to see if
			**  is a constant derivation do recursive
			**  call to handle arbitrary nesting
			**  of constant field dependencies.
			*/
			IIFVccConstChk(frm, thdr, ttype);

			/*
			**  If not a constant derivation value.
			**  just return.
			*/
			if (!(thdr->fhdrv->drvflags & fhDRVCNST))
			{
				return;
			}
		}

		/*
		**  Since all of this field's sources are some sort of
		**  constant values, evaluate it now so we have
		**  the correct value even if this field references
		**  some field that is farther down on the frfld chain.
		**  If evaluation failed, don't mark this field as
		**  a constant type of field.
		*/
		if (IIFVddeDoDeriveEval(frm, (DNODE *)type->ftvalchk, hdr,
			&tdb) == OK)
		{
			hdr->fhdrv->drvflags |= fhDRVCNST;
			hdr->fhdrv->drvflags |= fhDRVDCNST;
		}
	}
}



/*{
** Name:	IIFVcpConstProp - Check form for derived constant propagation.
**
** Description:
**	Checks all fields/columns in form for derived constant propagation.
**	In essence, if a derived fields depends on other fields (directly
**	or indirectly) that are just constant derivations then the
**	derived field can also be considered a constant derivation.
**
**	IIFVcvConstVal is set to TRUE before calling IIFVccConstChk()
**	so that constant evaluation works correctly.  It is set to
**	FALSE before this routine exits.
**
**	The stack variables "data1" and "data2" are necessary since
**	the evaluation of constant derivations does not use the
**	values from the field/column DBV.  This is to avoid problems
**	with where to get column values and to eliminate a second pass
**	through all the fields in the form to clean up the fields.
**
** Inputs:
**	frm		Form to check.
**
** Outputs:
**
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	08/18/89 (dkh) - Initial version.
*/
VOID
IIFVcpConstProp(frm)
FRAME	*frm;
{
	i4		i;
	i4		j;
	i4		fldno;
	i4		maxcols;
	FIELD		**fld;
	TBLFLD		*tbl;
	FLDCOL		**cols;
	DB_DATA_VALUE	val1;
	DB_DATA_VALUE	val2;
	i4		data1[DB_MAXSTRING/sizeof(i4) + 1];
	i4		data2[DB_MAXSTRING/sizeof(i4) + 1];

	val1.db_data = (PTR) data1;
	val2.db_data = (PTR) data2;
	IIFVdc1DbvConst = &val1;
	IIFVdc2DbvConst = &val2;

	IIFVcvConstVal = TRUE;
	/*
	**  Routine does a check for propagating constant derivations.
	**  through intermediate fields.
	*/
	for (i = 0, fld = frm->frfld, fldno = frm->frfldno; i < fldno;
		i++, fld++)
	{
		if ((*fld)->fltag == FREGULAR)
		{
			IIFVccConstChk(frm, &((*fld)->fld_var.flregfld->flhdr),
				&((*fld)->fld_var.flregfld->fltype));
		}
		else
		{
			tbl = (*fld)->fld_var.fltblfld;
			for (j = 0, cols = tbl->tfflds, maxcols = tbl->tfcols;
				j < maxcols; j++, cols++)
			{
				IIFVccConstChk(frm, &((*cols)->flhdr),
					&((*cols)->fltype));
			}
		}
	}
	IIFVcvConstVal = FALSE;
}




/*{
** Name:	IIFVdrDrvReset - Circularity check cleanup.
**
** Description:
**	Clean up the forms structures after doing a circularity
**	check on the derived fields in the form.
**
** Inputs:
**	frm		Form to clean up.
**
** Outputs:
**
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	06/20/89 (dkh) - Initial version.
*/
VOID
IIFVdrDrvReset(frm)
FRAME	*frm;
{
	i4	i;
	i4	j;
	i4	fldno;
	i4	maxcols;
	FIELD	**fld;
	TBLFLD	*tbl;
	FLDCOL	**cols;

	for (i = 0, fld = frm->frfld, fldno = frm->frfldno; i < fldno;
		i++, fld++)
	{
		if ((*fld)->fltag == FREGULAR)
		{
			(*fld)->fld_var.flregfld->flhdr.fhdflags &=
				~(fdVALCHKED);
			(*fld)->fld_var.flregfld->flhdr.fhd2flags &=
				~(fdVISITED);
		}
		else
		{
			tbl = (*fld)->fld_var.fltblfld;
			for (j = 0, cols = tbl->tfflds, maxcols = tbl->tfcols;
				j < maxcols; j++, cols++)
			{
				(*cols)->flhdr.fhdflags &= ~(fdVALCHKED);
				(*cols)->flhdr.fhd2flags &= ~(fdVISITED);
			}
		}
	}
}


/*{
** Name:	IIFVdcdDrvChkDep - Do circularity check on a field/column.
**
** Description:
**	Do circularity check on a specific field/column.
**
** Inputs:
**	hdr		FLDHDR structure for field/column to check.
**
** Outputs:
**
**	Returns:
**		OK	No circular references detected.
**		FAIL	A circular reference detected.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	06/20/89 (dkh) - Initial version.
*/
STATUS
IIFVdcdDrvChkDep(hdr)
FLDHDR	*hdr;
{
	DRVREL	*sources;
	FIELD	*fld;
	FLDHDR	*shdr;

	if (!(hdr->fhd2flags & fdDERIVED))
	{
		return(OK);
	}
	if (hdr->fhdflags & fdVALCHKED)
	{
		return(OK);
	}
	else if (hdr->fhd2flags & fdVISITED)
	{
		return(FAIL);
	}
	else
	{
		hdr->fhd2flags |= fdVISITED;
		for (sources = hdr->fhdrv->srclist; sources != NULL;
			sources = sources->next)
		{
			fld = sources->fld;
			if (fld->fltag == FREGULAR)
			{
				shdr = &(fld->fld_var.flregfld->flhdr);
			}
			else
			{
				shdr = &(fld->fld_var.fltblfld->
					tfflds[sources->colno]->flhdr);
			}
			if (IIFVdcdDrvChkDep(shdr) == FAIL)
			{
				return(FAIL);
			}
		}

		hdr->fhdflags |= fdVALCHKED;

		return(OK);
	}
}


/*{
** Name:	IIFVdccDrvCirChk - Check for circular references in a form.
**
** Description:
**	Check for derived field circular references in a form.
**	Stops on the first circular reference.
**
** Inputs:
**	frm		Form to check.
**
** Outputs:
**
**	Returns:
**		OK	No circular refernces found.
**		FAIL	At least one circular reference found.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	06/20/89 (dkh) - Initial version.
*/
STATUS
IIFVdccDrvCircChk(frm)
FRAME	*frm;
{
	i4	i;
	i4	j;
	i4	fldno;
	i4	maxcols;
	FIELD	**fld;
	TBLFLD	*tbl;
	FLDCOL	**cols;
	STATUS	retval = OK;

	for (i = 0, fld = frm->frfld, fldno = frm->frfldno; i < fldno;
		i++, fld++)
	{
		if ((*fld)->fltag == FREGULAR)
		{
			if ((retval = IIFVdcdDrvChkDep(&((*fld)->
				fld_var.flregfld->flhdr))) == FAIL)
			{
				frm->frcurfld = i;
				break;
			}
		}
		else
		{
			tbl = (*fld)->fld_var.fltblfld;
			for (j = 0, cols = tbl->tfflds, maxcols = tbl->tfcols;
				j < maxcols; j++, cols++)
			{
				if ((retval = IIFVdcdDrvChkDep(&((*cols)->
					flhdr))) == FAIL)
				{
					frm->frcurfld = i;
					tbl->tfcurcol = j;
					break;
				}
			}
		}
	}
	IIFVdrDrvReset(frm);
	return(retval);
}


/*{
** Name:	IIFVcaiCallAdeInst - calls ade_instr_gen for IIVMcmnCompMathNode
**
** Description:
**	There are 3 places where IIVMcmnCompMathNode() wants to call
**	ade_instr_gen() to compile an instruction. They each behave identically
**	on error so they're collected here to save space. Also, new capability
**	has been added to allocate more cxspace in case the previous cxsize
**	estimate was inaddequate (for bug 36970).
**
** Inputs:
**	ADF_CB	*adfcb		the adfcb from IIVMcmnCompMathNode
**	i4	ccoer_fid	the instruction to be compiled
**	i4	ade_seg		which segment of the CX to generate the
**				instruction in
**	i4	ade_nops	number of operands in the adeops array
**	ADE_OPERANDS adeops[]	array of ADE_OPERANDs one for each operand
**				the instruction requires
**
** Outputs:
**	i4	*qmap		query bit map
**	i4	*unalign	offset into adeops of operand in error if there
**				is an error
**
** History:
**	04-jun-1991 (mgw)
**		Written
*/

VOID
IIFVcaiCallAdeInst(adfcb, ccoer_fid, ade_seg, ade_nops, adeops, qmap, unalign)
ADF_CB		*adfcb;
i4		ccoer_fid;
i4		ade_seg;
i4		ade_nops;
ADE_OPERAND	adeops[];
i4		*qmap;
i4		*unalign;
{
	PTR	new_cxspace;
	STATUS	new_stat;

	while (ade_instr_gen(adfcb, cxspace, ccoer_fid, ade_seg, ade_nops,
		adeops, qmap, unalign) != OK)
	{
		if (adfcb->adf_errcb.ad_errcode != E_AD5506_NO_SPACE)
		{
			/*
			**  Can't put instruction into CX.
			*/
			if (IIFVcfCurFld->fltag == FREGULAR)
			{
				IIFVdeErr(E_FI208D_8333, 0);
			}
			else
			{
				IIFVdeErr(E_FI2097_8343, 1,
					  (PTR)IIFVcfCurFld->fld_var.fltblfld->
					  tfhdr.fhdname);
			}
		}
		else
		{	/* allocate more space */
			if ((new_cxspace =
			         MEreqmem(0, cxsize + cxsize/10,
			                  TRUE, (STATUS *) NULL)) == NULL)
			{
				/* Never returns */
				IIUGbmaBadMemoryAllocation(ERx("cx-realloc"));
			}
			MEcopy(cxspace, (u_i2)cxsize, new_cxspace);
			MEfree(cxspace);
			cxsize += cxsize/10;
			cxspace = new_cxspace;
			if (ade_inform_space(adfcb, cxspace, cxsize) != OK)
			{
				FEafeerr(adfcb);
				EXsignal(EXFEBUG, 1,
					ERx("IIFVcmnCompMathNode(realloc)"));
			}
			excb->excb_bases[ADE_CXBASE] = cxspace;
			excb->excb_cx = cxspace;
		}
	}
}
