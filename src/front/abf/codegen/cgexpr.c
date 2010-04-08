/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<cv.h>		/* 6-x_PC_80x86 */
#include	<lo.h>
#include	<si.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<er.h>
#include	<iltypes.h>
#include	<ilops.h>
/* Interpreted Frame Object definition. */
#include	<ade.h>
#include	<frmalign.h>
#include	<fdesc.h>
#include	<ilrffrm.h>
/* ILRF Block definition. */
#include	<abfrts.h>
#include	<ioi.h>
#include	<ifid.h>
#include	<ilrf.h>

#include	"cggvars.h"
#include	"cgilrf.h"
#include	"cgil.h"
#include	"cgout.h"
#include	"cgerror.h"
#include	"cglabel.h"


/**
** Name:	cgexpr.c -	Code Generator Evaluate Expression Module.
**
** Description:
**	Generates C code to evaluate expressions.
**
** History:
**	Revision 6.0  87/06  arthur
**	Initial revision.
**
**	Revision 6.3/03/00  89/12  billc
**	Added support for arrays and records.
**
**	Revision 6.3/03/01
**	01/09/91 (emerson)
**		Remove 32K limit on IL (allow up to 2G of IL).
**		This entails introducing a modifier bit into the IL opcode,
**		which must be masked out to get the opcode proper.
**	02/26/91 (emerson)
**		Change one call to IICGgadGetAddr to IICGgdaGetDataAddr;
**		remove 2nd argument in remaining calls to IICGgadGetAddr 
**		(cleanup: part of fix for bug 36097).
**
**	Revision 6.4
**	04/07/91 (emerson)
**		Modifications for local procedures
**		(in IICGexpExprGen and IICGmvcMovconstGen).
**	08/19/91 (emerson)
**		"Uniqueify" names by appending the frame id (generally 5
**		decimal digits) instead of the symbol name.  This will reduce
**		the probability of getting warnings (identifier longer than
**		n characters) from the user's C compiler.  (Bug 39317).
**
**	Revision 6.4/02
**	11/07/91 (emerson)
**		Created IICGrogReleaseObjGen, and modified IICGdotDotGen
**		and IICGarrArrayGen (for bugs 39581, 41013, and 41014:
**		array performance).
**
**	Revision 6.5
**	29-jun-92 (davel)
**		Added support for decimal datatype.
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**       4-Jan-2007 (hanal04) Bug 118967
**          IL_EXPR and IL_LEXPR now make use of spare operands in order
**          to store an i4 size and i4 offset.
*/

/*{
** Name:	IICGepExprGen() -	Evaluate an expression
**
** Description:
**	Evaluates an expression by calling afc_execute_cx(), the
**	front-end version of the ADE execution routine.
**	If this is a logical expression, the logical result is placed
**	into the designated VALUE by afc_execute_cx().
**	EXPR and LEXPR are the only variable-length IL statements.
**
** IL statements:
**	EXPR INT INT
**	  oper1 oper2 ... opern
**	LEXPR VALUE INT INT
**	  oper1 oper2 ... opern
**
**	In the EXPR statement, the first INT contains the
**	size of the following expression in i2's; the second INT
**	contains the offset (in bytes) to the last instruction
**	in the expression.
**	In the LEXPR statement, the VALUE indicates the place to
**	put the result of the logical expression; the first INT
**	contains the size of the following expression in i2's;
**	the second INT contains the offset (in bytes) to the last
**	instruction in the expression.
**
** Code generated:
**	For evaluating an expression that does not return a logical result:
**
**	{
**		static i2 IIil[] = {
**			0, 0, function-id, num-operands, operand-1, ...
**		};
**		IIARevlExprEval(IIil, 0, &IIfrmalign_<symbol_name>, IIdbdvs,
**			(DB_DATA_VALUE *) NULL);
**	}
**
** Inputs:
**	stmt	{IL *}  The IL statement to execute.
**
** History:
**	06/87 (agh) -- Written.
**	06/90 (jhw) Added correction for negative last ADE instruction offsets.
**		Bug #31023.
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		"Uniqueify" IIfrmalign name.
**		(See ConstArrs in cgfiltop.c).
*/
VOID
IICGexpExprGen(stmt)
register IL	*stmt;
{
	register i4	ilsize;
	register i4	ilptr;/* index of next element in CX to be written out*/
	register i4	i;
	i4		last_ref;
	char		buf[CGBUFSIZE];

	IICGobbBlockBeg();
		IICGostStructBeg(ERx("static i2"), ERx("IIil"), 0, CG_ZEROINIT);
		ilsize = ILgetI4Operand( stmt, ((*stmt&ILM_MASK) == IL_LEXPR) ? 2 : 1 );
		ilptr = ((*stmt&ILM_MASK) == IL_LEXPR) ? 6 : 5;
		for ( i = ilsize ; --i >= 0 ; )
		{
			IICGiaeIntArrEle( (i4)ILgetOperand(stmt, ilptr),
				( i == 0 ) ? CG_LAST : CG_NOTLAST
			);
			++ilptr;
		}
		IICGosdStructEnd();

		IICGosbStmtBeg();
		IICGocbCallBeg(ERx("IIARevlExprEval"));
		IICGocaCallArg(ERx("IIil"));

		/* The last ADE instruction offset is in bytes and will be
		** negative if it is larger than MAX_2IL (this assumes two's-
		** complement integers.)  Correct it in this case . . .
		*/
		last_ref = ILgetI4Operand(stmt, ((*stmt&ILM_MASK) == IL_LEXPR) ? 4 : 3);
		if ( last_ref < 0 )
			last_ref += (MAX_2IL + 1) * 2;	/* 2's complement */

		_VOID_ CVna( last_ref, buf );
		IICGocaCallArg(buf);

		IICGocaCallArg( STprintf( buf, ERx("&IIfrmalign_%d"), CG_fid ));
		IICGocaCallArg(ERx("IIdbdvs"));		/* an arrary */
		if ( (*stmt&ILM_MASK) == IL_LEXPR )
		{
			IICGocaCallArg( STprintf( buf, ERx("&IIdbdvs[%d]"),
						((i4)ILgetOperand(stmt, 1)) - 1
					)
			);
		}
		else
		{
			IICGocaCallArg(ERx("(DB_DATA_VALUE *) NULL"));
		}
		IICGoceCallEnd();
		IICGoseStmtEnd();
	IICGobeBlockEnd();
	return;
}

/*{
** Name:	IICGasnAssignGen() -	OSL assignment statement
**
** Description:
**	Generates code for an OSL assignment statement.
**
** IL statements:
**	evaluate expression into VALUE2
**	ASSIGN VALUE1 VALUE2
**
**	VALUE1 (equivalent to the left-hand side of an OSL
**	assignment statement) is the place to put the result.
**	VALUE2 (the right-hand side) holds the actual result value.
**	The expression has previously been evaluated into VALUE2.
**	VALUE3 contains the function instance ID of the ADF routine
**	used to coerce from the RHS to the LHS.	 In ADF, even
**	assignments between the same type are treated as coercions.
**
** Code generated:
**
**	IIARasnDbdvAssign(&IIdbdvs[lhs-offset], &IIdbdvs[rhs-offset],
**		function-id);
**
** Inputs:
**	stmt	The IL statement to execute.
**
*/
VOID
IICGasnAssignGen(stmt)
register IL	*stmt;
{
	IL	lhs;
	IL	rhs;
	char	buf[CGSHORTBUF];

	lhs = ILgetOperand(stmt, 1);
	rhs = ILgetOperand(stmt, 2);
	IICGosbStmtBeg();
	IICGocbCallBeg(ERx("IIARasnDbdvAssign"));
	IICGocaCallArg(IICGgadGetAddr(lhs));
	IICGocaCallArg(IICGgadGetAddr(rhs));
	CVna((i4) ILgetOperand(stmt, 3), buf);
	IICGocaCallArg(buf);
	IICGoceCallEnd();
	IICGoseStmtEnd();
	return;
}

/*{
** Name:	IICGdotDotGen() -	OSL 'dot' statement
**
** Description:
**	Generates code for an OSL 'dot' statement.
**
** IL statements:
**	DOT VALUE1 VALUE2 VALUE3
**
**	VALUE1 is a record object.
**	VALUE2 is the name of the record attribute.
**	VALUE3 contains the dbdv to hold information about the
**		attribute.  If it's another embedded record, we'll twiddle
**		the dbdv and use it as a temp.  If it's a DB_DATA_VALUE,
**		VALUE3 will contain the value from the record attribute.
**
**	DOT may have the ILM_RLS_SRC and/or ILM_HOLD_TARG modifier bits set.
**
** Code generated:
**
**	if (IIARdotDotRef(&IIdbdvs[obj], "mbr-name", &IIdbdvs[targ], modifiers)
**	   != 0
**	{
**		goto end-of-display-loop;
**	}
**
** Inputs:
**	stmt	The IL statement to execute.
**
** History:
**	11/07/91 (emerson)
**		Modified to handle new modifiers
**		(for bugs 39581, 41013, and 41014: array performance).
*/
VOID
IICGdotDotGen(stmt)
register IL	*stmt;
{
	IL	obj;
	IL	targ;
	char	buf[CGBUFSIZE];

	obj = ILgetOperand(stmt, 1);
	targ = ILgetOperand(stmt, 3);
	CVna(*stmt & (ILM_RLS_SRC | ILM_HOLD_TARG), buf);

	IICGoibIfBeg();

	IICGocbCallBeg(ERx("IIARdotDotRef"));
	IICGocaCallArg(IICGgadGetAddr(obj));
	IICGstaStrArg(ILgetOperand(stmt, 2));
	IICGocaCallArg(IICGgadGetAddr(targ));
	IICGocaCallArg(buf);
	IICGoceCallEnd();

	IICGoicIfCond( (char *) NULL, CGRO_NE, _Zero );
	IICGoikIfBlock();
	IICGpxgPrefixGoto(CGL_FDEND, CGLM_FORM);
	IICGobeBlockEnd();
	return;
}

/*{
** Name:	IICGarrArrayGen() -	OSL 'arrayref' statement
**
** Description:
**	Generates code for an OSL 'arrayref' statement.
**
** IL statements:
**	ARRAYREF VALUE1 VALUE2 VALUE3
**
**	VALUE1 is an array object.
**	VALUE2 is the integer expression that subscripts it.
**	VALUE3 contains the dbdv to assign the array element to.
**
**	ARRAYREF may have any of the ILM_LHS, ILM_RLS_SRC, or ILM_HOLD_TARG
**	modifier bits set.
**
**	ARRAYREF is generated in place of the old ARRAY starting with
**	the 6.4/02 OSL compiler.  We should never see the old ARRAY any more,
**	but we handle it just in case.  Its format is:
**	
**	ARRAY VALUE1 VALUE2 VALUE3 VALUE4
**
**	VALUE1, VALUE2, and VALUE3 are as for ARRAYREF.
**	VALUE4 is a bool that indicates whether targ will be an LHS.
**		If it is, then indexing 1 element beyond the array-top is OK.
**
** Code generated:
**
**	IIARarrArrayRef(&IIdbdvs[obj], index, &IIdbdvs[targ], modifiers);
**
** Inputs:
**	stmt	The IL statement to execute.
**
** History:
**	11/07/91 (emerson)
**		Modified to handle new modifiers
**		(for bugs 39581, 41013, and 41014: array performance).
*/
VOID
IICGarrArrayGen(stmt)
register IL	*stmt;
{
	IL	obj;
	IL	targ;
	char	buf[CGSHORTBUF];
	char 	*ind;
	i4 	flags;

	char	*IICGginGetIndex();

	obj = ILgetOperand(stmt, 1);
	targ = ILgetOperand(stmt, 3);

	ind = IICGginGetIndex(ILgetOperand(stmt, 2), ERx("(i4) 0"), 
			ERx("[]")
		);

	if ((*stmt & ILM_MASK) == IL_ARRAY)
	{
		flags = ILgetOperand(stmt, 4); 
	}
	else
	{
		flags = *stmt & (ILM_LHS | ILM_RLS_SRC | ILM_HOLD_TARG);
	}
	CVna(flags, buf);

	IICGosbStmtBeg();
	IICGocbCallBeg(ERx("IIARarrArrayRef"));
	IICGocaCallArg(IICGgadGetAddr(obj));
	IICGocaCallArg(ind);
	IICGocaCallArg(IICGgadGetAddr(targ));
    	IICGocaCallArg(buf);
	IICGoceCallEnd();
	IICGoseStmtEnd();
}

/*{
** Name:	IICGrogReleaseObjGen() - OSL 'release obj' statement
**
** Description:
**	Generates code for an OSL 'release obj' statement.
**
** IL statements:
**	RELEASEOBJ VALUE
**
**	VALUE is an array or record object.
**
** Code generated:
**
**	IIARroReleaseObj(&IIdbdvs[obj]);
**
** Inputs:
**	stmt	The IL statement to execute.
**
** History:
**	11/07/91 (emerson)
**		Created (for bugs 39581, 41013, and 41014: array performance).
*/
VOID
IICGrogReleaseObjGen(stmt)
register IL	*stmt;
{
	IICGosbStmtBeg();
	IICGocbCallBeg(ERx("IIARroReleaseObj"));
	IICGocaCallArg(IICGgadGetAddr(ILgetOperand(stmt, 1)));
	IICGoceCallEnd();
	IICGoseStmtEnd();
}

/*{
** Name:	IICGmvcMovconstGen() -	IL 'movconst' statement
**
** Description:
**	Generates code for the IL 'movconst' statement.
**	This statement is used to move a constant into a temp.
**	If the constant is an integer or float, its value is copied
**	directly into the temp.	 If the constant is a character string,
**	the temp's data pointer is set to point to the constant, and
**	the temp's size is set to be the constant's size.
**
** IL statements:
**	MOVCONST VALUE VALUE
**
**	The first VALUE (equivalent to the left-hand side of an OSL
**	assignment statement) is the temp; the second VALUE (the right-hand
**	side) is the constant.
**
** Code generated:
**	1) For an integer constant:
**
**	IIdbdvs[lhs-offset].db_data = _PTR_ &IICints[offset-in-array-of-consts];
**
**	2) For a float constant:
**
**	IIdbdvs[lhs-offset].db_data = _PTR_ &IICflts[offset-in-array-of-consts];
**
**	3) For a char constant:
**
**	IIdbdvs[lhs-offset].db_data = _PTR_ &IICst-offset;
**	IIdbdvs[lhs-offset].db_length = <n>;
**
**	4) For a hex constant:
**
**	IIdbdvs[lhs-offset].db_data = _PTR_ &IIChx-offset;
**	IIdbdvs[lhs-offset].db_length = IIChx-offset.hx_len + DB_CNTSIZE;
**
**	5) For a decimal constant:
**
**	IIdbdvs[lhs-offset].db_data = _PTR_ &IICdecs-offset;
**	IIdbdvs[lhs-offset].db_length = <n>;
**	IIdbdvs[lhs-offset].db_prec = <n>;
**
**	6) For assignment of the NULL constant:
**
**	IIARnliNullInit(&IIdbdvs[lhs-offset]);
**
** Inputs:
**	stmt	{IL *}  The IL statement to execute.
**
** History:
**      09/89 (jhw) -- Modified Null constant (was -DB_LTXT_TYPE) for
**                      portability.
**	22-jun-90 (dieters) -- Changed declaration of 'type' from "nat" to
**		"DB_DT_ID" for mx3_us5 compiler, which was not correctly
**		extending the return value from 'IICGvltValType()'.
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		"Uniqueify" names of constants.  (See ConstArrs in cgfiltop.c).
**	29-jun-92 (davel)
**		added support for decimal datatype.
*/
VOID
IICGmvcMovconstGen(stmt)
register IL	*stmt;
{
	i4		tindex;
	i4		cindex;
	i4		ps;
	DB_DT_ID	type;
	PTR		constval = NULL;
	char		lhsbuf[CGSHORTBUF];
	char		rhsbuf[CGSHORTBUF];

	tindex = ILgetOperand(stmt, 1);
	cindex = ILgetOperand(stmt, 2);
	type = IICGvltValType(tindex);
	STcopy(IICGgdaGetDataAddr(cindex, type), rhsbuf);
	switch (type)
	{
	  case DB_INT_TYPE:
	  case DB_FLT_TYPE:
		IICGovaVarAssign(
			STprintf(lhsbuf, ERx("IIdbdvs[%d].db_data"), 
					(tindex - 1)), 
			rhsbuf
		);
		IICGovaVarAssign(
			STprintf(lhsbuf, ERx("IIdbdvs[%d].db_length"), 
					(tindex - 1)),
			STprintf(rhsbuf, ERx("sizeof(IIC%ss_%d[0])"),
			    (type == DB_INT_TYPE ? ERx("int") : ERx("flt")),
			    CG_fid)
		);
		break;

	  case -1*DB_LTXT_TYPE:	/* == `nullable':  Used for the Null constant */
		IICGosbStmtBeg();
		IICGocbCallBeg(ERx("IIARnliNullInit"));
		IICGocaCallArg(IICGgadGetAddr(tindex));
		IICGoceCallEnd();
		IICGoseStmtEnd();
		break;

	  case DB_VCH_TYPE:
	  case DB_TXT_TYPE:
		IICGovaVarAssign(
			STprintf(lhsbuf, ERx("IIdbdvs[%d].db_data"), 
					(tindex - 1)),
			rhsbuf
		);
		/*
		** Length is length of actual hex string (hx_len minus 1,
		** since a byte had been allowed for EOS), plus 2 for the
		** bytes indicating length.
		*/
		IICGovaVarAssign(
			STprintf(lhsbuf, ERx("IIdbdvs[%d].db_length"),
					(tindex - 1)),
			STprintf(rhsbuf, ERx("IIChx%d_%d.hx_len + DB_CNTSIZE"),
				(- (cindex)) - 1, CG_fid)
		);
		break;

	  case DB_DEC_TYPE:
		IICGovaVarAssign(
			STprintf(lhsbuf, ERx("IIdbdvs[%d].db_data"), 
					(tindex - 1)), 
			rhsbuf
		);
		/*
		** Get precision/scale and length of decimal constant.
		*/
		_VOID_ IIORcqConstQuery(&IICGirbIrblk, - (cindex), DB_DEC_TYPE,
			&constval, &ps);
		/* sure would be nice to have a DB_PS_TO_LEN_MACRO in dbms.h */
		CVna((i4) DB_PREC_TO_LEN_MACRO(DB_P_DECODE_MACRO(ps)), rhsbuf);
		IICGovaVarAssign(
			STprintf(lhsbuf, ERx("IIdbdvs[%d].db_length"),
					(tindex - 1)), 
			rhsbuf
		);
		CVna((i4)ps, rhsbuf);
		IICGovaVarAssign(
			STprintf(lhsbuf, ERx("IIdbdvs[%d].db_prec"),
					(tindex - 1)), 
			rhsbuf
		);
		break;

	  case DB_CHA_TYPE:
	  default:
		IICGovaVarAssign(
			STprintf(lhsbuf, ERx("IIdbdvs[%d].db_data"), 
					(tindex - 1)), 
			rhsbuf
		);
		/*
		** Get length of actual constant string.
		*/
		_VOID_ IIORcqConstQuery(&IICGirbIrblk, - (cindex), DB_CHR_TYPE,
			&constval, (i4 *)NULL);
		CVna((i4) STlength(constval), rhsbuf);
		IICGovaVarAssign(
			STprintf(lhsbuf, ERx("IIdbdvs[%d].db_length"),
					(tindex - 1)), 
			rhsbuf
		);
		break;
	}
	return;
}

/*{
** Name:	IICGdyaDycAllocGen() -	IL 'dycalloc' statement
**
** Description:
**	Generates code for the IL 'dyalloc' statement, which is used
**	in the allocation of a DB_DATA_VALUE for a dynamic string.
**
** IL statements:
**	DYCALLOC VALUE
**
**	The VALUE is the DB_DATA_VALUE for the dynamic string.
**
** Code generated:
**
**	IIdbdvs[offset].db_length = 0;
**
** Inputs:
**	stmt	The IL statement to execute.
**
*/
VOID
IICGdyaDycAllocGen(stmt)
register IL	*stmt;
{
	char	buf[CGSHORTBUF];

	IICGovaVarAssign(
		STprintf(buf, ERx("IIdbdvs[%d].db_length"),
					(i4) (ILgetOperand(stmt, 1) - 1)),
		_Zero
	);
	return;
}


/*{
** Name:	IICGdyfDycFreeGen() -	IL 'dycfree' statement
**
** Description:
**	Generates code for the IL 'dyfree' statement, which is used
**	to free up the data area of a DB_DATA_VALUE used for a dynamic string.
**
** IL statements:
**	DYCFREE VALUE
**
**	The VALUE is the DB_DATA_VALUE for the dynamic string.
**
** Code generated:
**
**	IIdbdvs[offset].db_length = 0;
**	MEfree(IIdbdvs[offset].db_data);
**
** Inputs:
**	stmt	The IL statement to execute.
**
*/
VOID
IICGdyfDycFreeGen(stmt)
register IL	*stmt;
{
	char	buf[CGSHORTBUF];
	i4	offset;

	offset = (i4) (ILgetOperand(stmt, 1) - 1);
	IICGovaVarAssign(
		STprintf(buf, ERx("IIdbdvs[%d].db_length"), offset),
		_Zero
	);

	IICGosbStmtBeg();
	IICGocbCallBeg(ERx("MEfree"));
	IICGocaCallArg(STprintf(buf, ERx("IIdbdvs[%d].db_data"), offset));
	IICGoceCallEnd();
	IICGoseStmtEnd();
	return;
}
