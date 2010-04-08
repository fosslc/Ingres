/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
#include	<lo.h>
#include	<si.h>
#include	<st.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<iltypes.h>
#include	<ilops.h>
/* Interpreted Frame Object definition. */
#include	<ade.h>
#include	<frmalign.h>
#include	<fdesc.h>
#include	<ilrffrm.h>
#include	<abfrts.h>
#include	<ioi.h>
#include	<ifid.h>
#include	<ilrf.h>

#include	"cggvars.h"
#include	"cgilrf.h"
#include	"cgil.h"
#include	"cgout.h"
#include	"cglabel.h"
#include	"cgerror.h"

/**
** Name:	cgcall.c -	Code Generator Call/Return Module.
**
** Description:
**	Generates C code from the IL statements for calling and returning
**	from a frame or procedure.
**	Includes a different IICGstmtGen() routine for
**	each type of statement.
**	This file defines:
**
**	IICGprBuildPrm()	build parameter object list for a call.
**	IICG4glCallGen()	generate a call to a 4GL frame or procedure.
**	IICGsuCalSubSys()	generate a call to a sub-system.
**	IICGsyCalSys()		generate a call to the system.
**	IICGexitGen()		exit from an application
**	IICGreturnGen()		return from an 4GL frame or procedure
**
** History:
**	Revision 6.0  87/06  arthur
**	Initial revision.
**
**	Revision 6.1  88/09  wong
**	Modified 'IICGexitGen()' and 'IICGreturnGen()' to use CGLM_FORM0
**	to goto proper label.  Was going to end of display submenu for attached
**	submenus.  Jupiter bug #3435.
**
**	Revision 6.2  89/05  wong
**	Corrected 'IICGsuCalSubSys()' and 'IICGsyCalSys()' to
**	not clear/redisplay on return.
**
**	Revision 6.3/03/01
**	01/09/91 (emerson)
**		Remove 32K limit on IL (allow up to 2G of IL).
**		This entails introducing a modifier bit into the IL opcode,
**		which must be masked out to get the opcode proper.
**	02/17/91 (emerson)
**		Changed FD_VERSION to PR_VERSION for bug 35946 (see fdesc.h).
**	02/26/91 (emerson)
**		Remove 2nd argument to IICGgadGetAddr (cleanup:
**		part of fix for bug 36097).
**
**	Revision 6.4
**	03/22/91 (emerson)
**		Fix interoperability bug 36589:
**		Change all calls to abexeprog to remove the 3rd parameter
**		so that images generated in 6.3/02 will continue to work.
**		(Generated C code may contain calls to abexeprog).
**		This parameter was introduced in 6.3/03/00, but all calls
**		to abexeprog specified it as TRUE.  See abfrt/abrtexe.c
**		for further details.
**	04/07/91 (emerson)
**		Modifications for local procedures (in IICG4glCallGen).
**	05/07/91 (emerson)
**		"Uniqueify" local procedure names within the executable,
**		and remove logic that declared the local procedure name
**		as a function (in IICG4glCallGen).
**	06/25/91 (emerson)
**		Fix for bug 38071 in IICGprBuildPrm.
**	07/03/91 (emerson)
**		Fix a syntactic error noted by more than one porter:
**		code (PTR *)&x instead of &((PTR)x).
**	08/19/91 (emerson)
**		"Uniqueify" names by appending the frame id (generally 5
**		decimal digits) instead of the symbol name.  This will reduce
**		the probability of getting warnings (identifier longer than
**		n characters) from the user's C compiler.  (Bug 39317).
**
**	Revision 6.5
**	29-jun-92 (davel)
**		Minor change - added new argument to call to IIORcqConstQuery().
**	26-aug-92 (davel)
**		add support for EXECUTE PROCEDURE to IICG4glCallGen().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
** Has an ABRTSPRM structure been built for the current call to a
** frame or procedure?
** Does the call include passing a query between frames?
*/
static bool	paramBuilt = FALSE;
static bool	queryPassed = FALSE;

/*{
** Name:	IICGprBuildPrm() -	Build ABRTSPRM struct for a call
**
** Description:
**	Builds an ABRTSPRM structure for a call on another frame or procedure.
**
** IL statements:
**	PARAM VALUE VALUE INT INT INT
**	TL1ELM VALUE
**	  ...
**	ENDLIST
**	PVELM VALUE INT
**	  ...
**	ENDLIST
**
**	In the PARAM statement, the first VALUE is the place to put the
**	return value, the second the QRY structure for the
**	query being passed.  Either or both of these can be NULL.
**	The first in-line integer is the number of elements in the list
**	of formals.  The second INT is the number of elements in the
**	parameter vector.  A PV structure will be built for each such element.
**	The third INT is non-zero if the parameter list contains a '.all'.
**
**	The first list contains the name of each formal (which can be missing.)
**
**	The second list contains a PVELM for each parameter.
**	The VALUE is the value of the parameter, and the INT indicates
**	whether the parameter is being passed by reference.  A 1 here
**	indicates a call by reference, a 0 by value.
**
** Inputs:
**	stmt	The IL statement for which to generate code.
**
** History:
**	05-may-1987
**		First written. (agh)
**	06/25/91 (emerson)
**		Fix for bug 38071: whenever there was a mixture of 
**		positional and keyword parameters, the array of pointers
**		to formals was too small (number of *keyword* parameters
**		instead of total number of parameters), and so the 
**		generated code caused memory corruption.
*/
VOID
IICGprBuildPrm ( stmt )
register IL	*stmt;
{
    register IL	*next;
    IL		retdbdv;	/* DB_DATA_VALUE for return value */
    i4		fsize;		/* number of formals */
    i4		pvsize;		/* number of PV structures */
    i4		count = 0;
    char	buf[CGSHORTBUF];

    fsize = ILgetOperand(stmt, 3);
    pvsize = ILgetOperand(stmt, 4);
    IICGobbBlockBeg();
    IICGostStructBeg(ERx("ABRTSPV"), ERx("pv"), (pvsize == 0) ? 1 : pvsize,
		CG_NOINIT
    );
    if (fsize != 0)
	IICGostStructBeg(ERx("char *"), ERx("formal"), pvsize, CG_NOINIT);
    IICGostStructBeg(ERx("ABRTSPRM"), ERx("rtsprm"), 0, CG_NOINIT);
    IICGostStructBeg(ERx("ABRTSOPRM"), ERx("ortsprm"), 0, CG_NOINIT);

    /*
    ** Next, assign values to the members of the ABRTSPRM
    ** and ABRTSOPRM structures, and fill in the vectors.
    ** The values for these vectors become known as
    ** successive IL statements are read in.
    */
    IICGovaVarAssign( ERx("rtsprm.pr_zero"), _Zero );
    CVna((i4) PR_VERSION, buf);
    IICGovaVarAssign(ERx("rtsprm.pr_version"), buf);
    IICGovaVarAssign(ERx("rtsprm.pr_oldprm"), ERx("&ortsprm"));
    CVna((i4) ILgetOperand(stmt, 4), buf);
    IICGovaVarAssign(ERx("rtsprm.pr_argcnt"), buf);
    IICGovaVarAssign(ERx("rtsprm.pr_formals"),
			(fsize == 0) ? ERx("(char **) NULL") : ERx("formal")
    );
    IICGovaVarAssign(ERx("rtsprm.pr_actuals"), ERx("pv"));
    CVna(ABPIGNORE, buf);
    IICGovaVarAssign(ERx("rtsprm.pr_flags"),
			(ILgetOperand(stmt, 5) == 0 ? _Zero : buf)
    );

    retdbdv = ILgetOperand(stmt, 1);
    if (ILNULLVAL(retdbdv))
    {
	IICGovaVarAssign( ERx("rtsprm.pr_rettype"), _Zero );
	IICGovaVarAssign(ERx("rtsprm.pr_retvalue"), ERx("(i4 *) NULL"));
    }
    else
    {
	CVna((i4) DB_DBV_TYPE, buf);
	IICGovaVarAssign(ERx("rtsprm.pr_rettype"), buf);
	IICGovaVarAssign(ERx("rtsprm.pr_retvalue"),
		STprintf(buf, ERx("(i4 *) %s"),
				IICGgadGetAddr(retdbdv)
		)
	);
    }

    /*
    ** Fill in the ABRTSOPRM structure.  All of its elements, except
    ** possibly for a query being passed between frames, are NULL.
    */
    IICGovaVarAssign(ERx("ortsprm.pr_tlist"), ERx("NULL"));
    IICGovaVarAssign(ERx("ortsprm.pr_argv"), ERx("NULL"));
    IICGovaVarAssign(ERx("ortsprm.pr_tfld"), ERx("NULL"));
    IICGovaVarAssign(ERx("ortsprm.pr_qry"),
		ILNULLVAL(ILgetOperand(stmt, 2)) ? ERx("(QRY *) NULL")
		: IICGgvlGetVal(ILgetOperand(stmt, 2), DB_QUE_TYPE)
    );
    if (!ILNULLVAL(ILgetOperand(stmt, 2)))
	queryPassed = TRUE;

    next = IICGgilGetIl(stmt);
    if (fsize != 0)
    {
	register i4	i;

	/*
	** If there are more PVs than formals, fill in NULL for
	** the non-corresponding formals, which must come first
	** in the array.  This would be relevant in a future
	** version of 4GL which allowed mixed lists of parameters
	** with both positional and keyword-designated elements.
	*/
	for (i = pvsize - fsize; i > 0; i--)
	{
	    IICGoaeArrEleAssign(ERx("formal"), (char *) NULL, count++,
				_NullPtr
	    );
	}

	/*
	** Read in the 1-valued list of values of formals.
	*/
	while ((*next&ILM_MASK) == IL_TL1ELM)
	{
	    IICGoaeArrEleAssign(ERx("formal"), (char *) NULL, count++,
				IICGgvlGetVal(ILgetOperand(next,1), DB_CHR_TYPE)
	    );
	    next = IICGgilGetIl(next);
	}
	if ((*next&ILM_MASK) != IL_ENDLIST)
	    IICGerrError(CG_ILMISSING, ERx("endlist"), (char *) NULL);
	next = IICGgilGetIl(next);
    }

    /*
    ** Read in the 2-valued list of PV value, and whether passed
    ** by reference (1) or by value (0).
    */
    count = 0;
    while ((*next&ILM_MASK) == IL_PVELM)
    {
	/*
	** DB_DATA_VALUEs are passed as the abpvvalues for the PV.
	** The abpvtype is always DB_DBV_TYPE.
	*/
	if (ILgetOperand(next, 2) == 1)
	   _VOID_ STprintf(buf, ERx("-(%d)"), DB_DBV_TYPE);
	else
	   _VOID_ CVna((i4) DB_DBV_TYPE, buf);
	IICGoaeArrEleAssign(ERx("pv"), ERx("abpvtype"), count, buf);
	IICGoaeArrEleAssign( ERx("pv"), ERx("abpvsize"), count, _Zero );
	IICGoaeArrEleAssign(ERx("pv"), ERx("abpvvalue"), count++, 
		STprintf(buf, ERx("(char *) %s"),
			IICGgadGetAddr(ILgetOperand(next, 1))
		)
	);
	next = IICGgilGetIl(next);
    }
    if ((*next&ILM_MASK) != IL_ENDLIST)
	IICGerrError(CG_ILMISSING, ERx("endlist"), (char *) NULL);
    paramBuilt = TRUE;
    return;
}

/*{
** Name:	IICG4glCallGen() - Generate call to 4GL frame or procedure.
**
** Description:
**	Generates code for the 4GL CALLFRAME and CALLPROC statements when
**	they involve a call to an 4GL frame, main procedure, or local procedure.
**
** IL statements:
**	CALLF  VALUE or
**	CALLP  VALUE or
**	CALLPL VALUE SID
**
**	VALUE is the name of the frame or procedure being called.
**	Note that for CALLPL, VALUE must be a constant.
**	SID is not used by code generation.
**
** Inputs:
**	stmt	The IL statement for which to generate code.
**
** History:
**	15-apr-87 (agh)
**		First written.
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Added logic for CALLPL.
**	05/07/91 (emerson)
**		"Uniqueify" local procedure names within the executable.
**		Also removed logic to declare the local procedure name
**		as a function.  (cgfiltop.c now contains logic to generate
**		declarations of local procedures as static functions).
**	29-jun-92 (davel)
**		Added new arguemnt for call  to IIORcqConstQuery().
**	26-aug-92 (davel)
**		add support for ILM_EXEPROC modifier flag (for EXECUTE
**		PROCEDURE).
**  01-dec-1998 (rodjo04) bug 94361
**      Added call to CVlower() to normalize the procedure name
**      when generating a call to IIARlpcLocalProcCall. This was
**      introduced by bug 90148.
*/
VOID
IICG4glCallGen ( stmt )
register IL	*stmt;
{
    bool	IICGqmdChkMDQry();
    char	*abf_func, *called_proc;
    char	full_called_proc[CGBUFSIZE];
    ILREF	name_ref;
    ILOP	op = *stmt & ILM_MASK;

    name_ref = ILgetOperand(stmt, 1);

    if (op == IL_CALLF)
    {
	abf_func = ERx("abrtscall");
    }
    else if ((*stmt&(ILM_MASK|ILM_EXEPROC)) == (IL_CALLP|ILM_EXEPROC))
    {
	abf_func = ERx("IIARdbProcCall");
    }
    else if (op == IL_CALLP)
    {
	abf_func = ERx("abrtsretcall");
    }
    else /* (op == IL_CALLPL) */
    {
	abf_func = ERx("IIARlpcLocalProcCall");

	/*
	** Get the name of the called local procedure.
	** It must be in a char const.
	** "Uniqueify" it across the entire executable
	** by appending the frame id of the parent frame or global procedure.
	*/
	(VOID)IIORcqConstQuery( &IICGirbIrblk, - name_ref, DB_CHR_TYPE,
				(PTR *)&called_proc, (i4 *)NULL );
    CVlower(called_proc);
	(VOID)STprintf( full_called_proc, ERx("%s_%d"), called_proc, CG_fid );
    }

    IICGosbStmtBeg();
    IICGocbCallBeg(abf_func);
    IICGstaStrArg(name_ref);
    IICGocaCallArg(paramBuilt ? ERx("&rtsprm") : ERx("(ABRTSPRM *) NULL"));

    if (op == IL_CALLPL)
    {
        IICGocaCallArg(full_called_proc);
        IICGocaCallArg(ERx("IIdbdvs"));
    }
    IICGoceCallEnd();
    IICGoseStmtEnd();

    IICGrcsResetCstr();
    if (paramBuilt)
    {
	/*
	** Close the block opened to declare structures for the call.
	*/
	IICGobeBlockEnd();
	paramBuilt = FALSE;
    }
    if (queryPassed)
    {
	/*
	** Close the block(s) opened to declare structures for the query
	** passed as part of the call.	(Multiple blocks will have been
	** opened if the query was a master-detail query.)
	*/
	if (IICGqmdChkMDQry())
	    IICGobeBlockEnd();	/* child block */
	IICGobeBlockEnd();  /* query block */
	queryPassed = FALSE;
    }
}

/*{
** Name:	IICGsuCalSubSys() - 4GL CALL <subsystem> Code Generaation.
**
** Description:
**	Generates C code for the 4GL 'call subsystem' statement.
**
** IL statements:
**	CALSUBSYS VALUE VALUE INT
**	TL1ELM VALUE
**	  ...
**	ENDLIST
**
**	In the CALSUBSYS statement, the first VALUE is the name of the
**	Ingres subsystem to be run; the second is a format string
**	containing the arguments for the subsystem.
**	The INT operand contains the number of arguments.
**	The CALSUBSYS is followed by a list of TL1ELM statements, each
**	of which contains one of the arguments, which must be in a variable
**	(regular or temporary) rather than appear as a constant.
**	The list is closed by an ENDLIST statement.
**
** Inputs:
**	stmt	{IL *}  The IL statement for which to generate code.
**
** History:
**	05/87 (agh) -- Written.
**	05/89 (jhw) -- Remove clear screen redisplay after sub-system call.
**		This is no longer required because 'abexeprog()' handles
**		the FRS redraw.  JupBug #6107.
*/
VOID
IICGsuCalSubSys ( stmt )
register IL	*stmt;
{
	register IL	*next;
	char		buf[CGSHORTBUF];

	IICGosbStmtBeg();
	IICGocbCallBeg(ERx("IIclrscr"));
	IICGoceCallEnd();
	IICGoseStmtEnd();

	IICGosbStmtBeg();
	IICGocbCallBeg(ERx("abexeprog"));
	IICGstaStrArg(ILgetOperand(stmt, 1));
	IICGocaCallArg(ILNULLVAL(ILgetOperand(stmt, 2)) ? _NullPtr :
		IICGgvlGetVal(ILgetOperand(stmt, 2), DB_CHR_TYPE)/* constant */
	);
	CVna((i4) ILgetOperand(stmt, 3), buf);
	IICGocaCallArg(buf);

	/*
	** Read in the 1-valued list of arguments for the subsystem call.
	*/
	for ( next = IICGgilGetIl(stmt) ; (*next&ILM_MASK) == IL_TL1ELM ;
			next = IICGgilGetIl(next) )
	{
		IL	val;
		DB_DT_ID type;

		val = ILgetOperand(next, 1);
		if ( (type = abs(IICGvltValType(val))) == DB_CHR_TYPE ||
				type == DB_CHA_TYPE || type == DB_VCH_TYPE ||
					type == DB_TXT_TYPE )
			IICGstaStrArg(val);
		else
			IICGocaCallArg(IICGgvlGetVal(val, type));
	}
	if ( (*next&ILM_MASK) != IL_ENDLIST )
		IICGerrError(CG_ILMISSING, ERx("endlist"), (char *) NULL);

	IICGoceCallEnd();
	IICGoseStmtEnd();

	IICGrcsResetCstr();
}

/*{
** Name:	IICGsyCalSys() -	4GL 'call system' Code Generation.
**
** Description:
**	Generates C code for the 4Gl 'call system' statement.
**
** IL statements:
**	CALSYS VALUE
**
**	The VALUE is the operating system command to be executed,
**	or the NULL VALUE if no command is specified.
**
** Inputs:
**	stmt	{IL *}  The IL statement for which to generate code.
**
** History:
**	05/87 (agh) -- Written.
**	05/89 (jhw) -- Remove clear screen redisplay after system call.  This is
**		no longer required because 'abexesys()' handles the FRS redraw.
*/
VOID
IICGsyCalSys ( stmt )
register IL	*stmt;
{
	IL	val;

	IICGosbStmtBeg();
		IICGocbCallBeg(ERx("IIclrscr"));
		IICGoceCallEnd();
	IICGoseStmtEnd();

	val = ILgetOperand(stmt, 1);
	IICGosbStmtBeg();
		IICGocbCallBeg(ERx("abexesys"));
		if (ILNULLVAL(val))
			IICGocaCallArg( _NullPtr );
		else
			IICGstaStrArg(val);
		IICGoceCallEnd();
	IICGoseStmtEnd();
	IICGrcsResetCstr();
}

/*{
** Name:	IICGexitGen() -	Exit from an application.
**
** Description:
**	Generates code for the 4GL EXIT statement, which exits
**	from an application.
**
** IL statements:
**	EXIT
**
** Inputs:
**	stmt	{IL *}  The IL statement for which to generate code.
**
** History:
**	05/87 (agh) - Written.
**	09/88 (jhw) - Modified to use CGLM_FORM0 to goto proper label.
*/
VOID
IICGexitGen ( stmt )
IL	*stmt;
{
	IICGovaVarAssign(ERx("IIbrktype"), ERx("2"));
	IICGpxgPrefixGoto( CGL_FDEND, CGLM_FORM0 );
}

/*{
** Name:	IICGreturnGen() -	Return from an 4GL frame or procedure.
**
** Description:
**	Generates code for an 4GL RETURN statement when returning from
**	an 4GL frame or procedure.
**
** IL statements:
**	RETFRM VALUE
**	[ RETPROC VALUE ]
**
**		The VALUE is the value being returned, or NULL if none.
**
** Inputs:
**	stmt	{IL *}  The IL statement for which to generate code.
**
** History:
**	05/87 (agh) - Written.
**	09/88 (jhw) - Modified to use CGLM_FORM0 to goto proper label.
*/
VOID
IICGreturnGen ( stmt )
register IL	*stmt;
{
	/*
	** If there is a return value, set the local (DB_DATA_VALUE *)
	** to point to it; else set it to NULL.
	*/
	IICGovaVarAssign( ERx("IIrvalue"),
		(ILNULLVAL(ILgetOperand(stmt, 1))
			? ERx("NULL")
			: IICGgadGetAddr(ILgetOperand(stmt, 1))
		)
	);

	IICGovaVarAssign( ERx("IIbrktype"), _One );
	IICGpxgPrefixGoto( CGL_FDEND, CGLM_FORM0 );
}
