/*
**	Copyright (c) 1986, 2004 Ingres Corporation
*/

#include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<cv.h>		/* 6-x_PC_80x86 */
#include	<si.h>
# include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<iltypes.h>
#include	<ilops.h>

/**
** Name:    iamprint.c -	Interpreted Frame Object Debug Print Module.
**
** Description:
**	Contains routines used to print various parts of an interpreted frame
**	object for debugging purposes.	Defines:
**
**	IIAMpsPrintStmt()		print il statement.
**	IIAMpasPrintAddressedStmt()	print il statement with their address.
**	IIAMpcPrintConsts()		print constants table.
**
** History:
**	Revision 6.4/03
**	09/20/92 (emerson)
**		Added new IL opcodes in IIAMpasPrintAddressedStmt
**		(for bug 39582).
**
**	Revision 6.4/02
**	11/07/91 (emerson)
**		Added logic (in IIAMpasPrintAddressedStmt)
**		to print modifier bits (for bugs 39581, 41013, and 41014).
**
**	Revision 6.4
**	04/07/91 (emerson)
**		Modifications for local procedures
**		(in IIAMpasPrintAddressedStmt).
**	05/07/91 (emerson)
**		Added logic to print NEXTPROC statement
**		(in IIAMpasPrintAddressedStmt).
**
**	Revision 6.3/03/01
**	01/09/91 (emerson)
**		Remove 32K limit on IL (allow up to 2G of IL).
**
**	Revision 6.0  87/07  wong
**	Added support to output TEXT type strings.
**	Print ADE compiled expression operands offset by one to agree with IL
**	offsets (which start at one, not zero.)	 Added last instruction offset
**	to IL_EXPR and IL_LEXPR.  Made changes for 6.0 query generation.
**	Added support for printing of default operators with four operands.
**	Added `activate' option operand for IL_MENUITEM and IL_KEYACT.
**
**	Revision 5.1  86/10/15	21:11:56  wong
**	IL_KEYACT has INT operand.
**	(10/09/86)  Added `explanation' operand for IL_MENUITEM and IL_KEYACT.
**	(10/03/86)  Changed format of "IL_PARAM".
**	(10/01/86)  Changes for IL_PARM.
**	(09/26/86)  Added BEGSUBMU special.
**	(09/25/86)  Added QRY_PRED.
**	(09/25/86)  Fixed list of constants.
**	(09/24/86)  Added 'IIAMpcPrintConsts()' and cleanup.
**	(09/18/86)  Modified from "interp/ipprint.c".
**	4-oct-1988 (Joe)
**		Added new entry point IIAMpasPrintAddressedStmt to
**		help with debugging the interpreter.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**       4-Jan-2007 (hanal04) Bug 118967
**          IL_EXPR and IL_LEXPR now make use of spare operands in order
**          to store an i4 size and i4 offset.
**      25-Mar-2008 (hanal04) Bug 118967
**          Correction to earlier change spotted whilst resolving problems
**          on su9.
*/

static VOID print_expr();
static VOID print_sid();
FUNC_EXTERN VOID IIAMpasPrintAddressedStmt();

/*{
** Name:    IIAMpsPrintStmt() - Print IL Statement.
**
** Description:
**	Prints to it's output file, one IL statement (operator and operands.)
**
** Inputs:
**	fp	{FILE *}  Output file pointer.
**	iltab	{IL_OPTAB []}  IL operator look-up table.
**	stmt	{IL []}	 The IL statement.
**
** History:
**	07/86 (agh) -- Written.
**	09/86 (jhw) -- Modified.
*/
VOID
IIAMpsPrintStmt (fp, iltab, stmt)
FILE		*fp;
IL_OPTAB	iltab[];
register IL	*stmt;
{
    IIAMpasPrintAddressedStmt(fp, iltab, stmt, NULL);
}


/*{
** Name:	IIAMpasPrintAddressedStmt - Print IL with address.
**
** Description:
**	This prints a single IL statement to the output file optionally
**	preceded by its address.
**
**	The address is printed if the value of top is not NULL.  If
**	address printing is turned on, any SIDs printed are printed
**	both as value as given in the IL, and the resulting address.
**	For example:
**
**		6:   ....
**		10:  GOTO -2 (6:)
**
** Inputs:
**	fp	{FILE *}  Output file pointer.
**
**	iltab	{IL_OPTAB []}  IL operator look-up table.
**
**	stmt	{IL []}	 The IL statement.
**
**	top	{IL *} The top of the list of IL statements.  If this
**		is not NULL, then address printing is turned on.
**
** History:
**	4-oct-1988 (Joe)
**		This routine was added as a new entry point instead of
**		a modification of IIAMpsPrintStmt so that
**		existing callers of IIAMpsPrintStmt would not have
**		to change.
**	01/09/91 (emerson)
**		Remove 32K limit on IL (allow up to 2G of IL).
**		This entails checking for the modifier bit ILM_LONGSID
**		in the opcode and masking it out.
**	04/07/91 (emerson)
**		Modifications for local procedures: handle new op-codes.
**	05/07/91 (emerson)
**		Added logic to print NEXTPROC statement
**	11/07/91 (emerson)
**		Added logic to print modifier bits
**		(for bugs 39581, 41013, and 41014: array performance).
**	09/20/92 (emerson)
**		Added new IL opcodes QRYROWGET and QRYROWCHK
**		to the appropriate case (for bug 39582).
*/
VOID
IIAMpasPrintAddressedStmt(fp, iltab, stmt, top)
FILE		*fp;
IL_OPTAB	iltab[];
register IL	*stmt;
register IL	*top;
{
    i4	    expr_size;
    char    buf[32];
    bool    print_addr = (top != NULL);
    i4	    offset;
    IL      il_op = *stmt & ILM_MASK;
    IL      il_op_modifiers = *stmt & ~ILM_MASK;

    /*
    ** Each line of an expression has the form:
    **	opcode (num_operands) operand1 operand2 ...
    **
    ** If currently within an expression, we must print out the variable
    ** length line.  Otherwise the line size is fixed, and handled
    ** by the cases within the 'switch' statement.
    */

    STcopy(iltab[il_op].il_name, buf);
    CVupper(buf);
    if (print_addr)
    {
	SIfprintf(fp, ERx("%6d:   "), (i4) (stmt  - top));
    }
    if (il_op_modifiers == 0)
    {
        SIfprintf(fp, ERx("%s "), buf);
    }
    else
    {
        SIfprintf(fp, ERx("%s(%4x) "), buf, il_op_modifiers);
    }

    switch (il_op)
    {
      /*
      ** Sample IL statement:  DBCONST STRING(-12)
      */
      case IL_DBCONST:
	SIfprintf(fp, ERx("STRING(%d)\n"), ILgetOperand(stmt, 1));
	break;

      /*
      ** Sample IL statement:  RESCOL STRING(-14) STRING(-7)
      */
      case IL_RESCOL:
	SIfprintf(fp, ERx("STRING(%d) STRING(%d)\n"),
	    ILgetOperand(stmt, 1), ILgetOperand(stmt, 2));
	break;

      case IL_GETFORM:
      case IL_PUTFORM:
      case IL_GETROW:
      case IL_PUTROW:
	SIfprintf(fp, ERx("STRING(%d) VALUE(%d)\n"),
	    ILgetOperand(stmt, 1), ILgetOperand(stmt, 2));
	break;

      /*
      ** Sample IL statement:  PVELM STRING(14) 0
      */
      case IL_PVELM:
	SIfprintf(fp, ERx("VALUE(%d) %d\n"),
	    ILgetOperand(stmt, 1), ILgetOperand(stmt, 2));
	break;

      /*
      ** Sample IL statement:  QRY VALUE(12) STRING(-6) STRING(0)
      */
      case IL_QRY:
	SIfprintf(fp, ERx("VALUE(%d) STRING(%d) STRING(%d)\n"),
	    ILgetOperand(stmt, 1), ILgetOperand(stmt, 2),
	    ILgetOperand(stmt, 3));
	break;

      case IL_DOT:
	SIfprintf(fp, ERx("VALUE(%d) STRING(%d) VALUE(%d)\n"),
	    ILgetOperand(stmt, 1), ILgetOperand(stmt, 2),
	    ILgetOperand(stmt, 3));
	break;

      case IL_ARRAY:
	SIfprintf(fp, ERx("VALUE(%d) VALUE(%d) VALUE(%d) BOOL(%d)\n"),
	    ILgetOperand(stmt, 1), ILgetOperand(stmt, 2),
	    ILgetOperand(stmt, 3), ILgetOperand(stmt, 4));
	break;

      case IL_QRYEND:
	SIfprintf(fp, ERx("DML(%d) MODE(%d) STRING(%d)\n"),
	    ILgetOperand(stmt, 1), ILgetOperand(stmt, 2), ILgetOperand(stmt, 3)
	);
	break;

      /*
      ** Sample IL statements:	GOTO 86 (relative offset)
      */
      case IL_GOTO:
      case IL_UNLD2:
      case IL_VALIDATE:
      case IL_DISPLAY:
      case IL_ENDMENU:
      case IL_NEXTPROC:
	SIfprintf(fp, ERx("%d"), (offset = ILgetOperand(stmt, 1)));
	if (print_addr)
	    print_sid(fp, top, stmt, offset);
	SIfprintf(fp, ERx("\n"));
	break;
    
      /*
      **		    STHD 24 (line number)
      */
      case IL_BEGMENU:
      case IL_BEGSUBMU:
      case IL_STHD:
	SIfprintf(fp, ERx("%d\n"), ILgetOperand(stmt, 1));
	break;

      case IL_LSTHD:
	SIfprintf(fp, ERx("%d %d\n"), ILgetOperand(stmt, 1), ILgetOperand(stmt, 2));
	break;

      /*
      ** Sample IL statement:  IF VALUE(12) 86
      */
      case IL_IF:
      case IL_QRYSINGLE:
      case IL_QRYROWGET:
      case IL_QRYROWCHK:
      case IL_VALFLD:
      case IL_CALLPL:
	SIfprintf(fp, ERx("VALUE(%d) %d"), ILgetOperand(stmt, 1),
		(offset = ILgetOperand(stmt, 2)));
	if (print_addr)
	    print_sid(fp, top, stmt, offset);
	SIfprintf(fp, ERx("\n"));
	break;

      /*
      ** Sample IL statement:  QRYGEN VALUE(11) 0 86
      */
      case IL_QRYNEXT:
      case IL_QRYGEN:
	SIfprintf(fp, ERx("VALUE(%d) %d %d"), ILgetOperand(stmt, 1),
		ILgetOperand(stmt, 2),
		(offset = ILgetOperand(stmt, 3)));
	if (print_addr)
	    print_sid(fp, top, stmt, offset);
	SIfprintf(fp, ERx("\n"));
	break;

      case IL_QRYPRED:
	SIfprintf(fp, ERx("%d  %d\n"), ILgetOperand(stmt, 1),
				       ILgetOperand(stmt, 2));
	break;

      /*
      ** Sample IL statement:  FLDACT STRING(-2) 101
      */
      case IL_ARR1UNLD:
	SIfprintf(fp, ERx("VALUE(%d) VALUE(%d) %d"), ILgetOperand(stmt, 1),
		ILgetOperand(stmt, 2),
		(offset = ILgetOperand(stmt, 3)));
	if (print_addr)
	    print_sid(fp, top, stmt, offset);
	SIfprintf(fp, ERx("\n"));
	break;

      case IL_FLDACT:
      case IL_FLDENT:
      case IL_UNLD1:
      case IL_ARR2UNLD:
	SIfprintf(fp, ERx("VALUE(%d) %d"), ILgetOperand(stmt, 1),
		(offset = ILgetOperand(stmt, 2)));
	if (print_addr)
	    print_sid(fp, top, stmt, offset);
	SIfprintf(fp, ERx("\n"));
	break;

      /*
      ** Sample IL statement:  VALROW VALUE(23) VALUE(12,0) 86
      */
      case IL_VALROW:
	SIfprintf(fp, ERx("VALUE(%d) VALUE(%d) %d"),
		ILgetOperand(stmt, 1), ILgetOperand(stmt, 2),
		(offset = ILgetOperand(stmt, 3)));
	if (print_addr)
	    print_sid(fp, top, stmt, offset);
	SIfprintf(fp, ERx("\n"));
	break;

      /*
      ** Sample IL statement:  COLACT STRING(-2) STRING(-10) 86
      */
      case IL_COLACT:
      case IL_COLENT:
	SIfprintf(fp, ERx("STRING(%d) STRING(%d) %d"),
		ILgetOperand(stmt, 1), ILgetOperand(stmt, 2),
		(offset = ILgetOperand(stmt, 3)));
	if (print_addr)
	    print_sid(fp, top, stmt, offset);
	SIfprintf(fp, ERx("\n"));
	break;

      /*
      ** Sample IL statement:  MENUITEM STRING(-2) VALUE(0) 86
      */
      case IL_MENUITEM:
	SIfprintf(fp, ERx("STRING(%d) STRING(%d) INT(%d) INT(%d) %d"),
		ILgetOperand(stmt, 1), ILgetOperand(stmt, 2),
		ILgetOperand(stmt, 3), ILgetOperand(stmt, 4),
		(offset = ILgetOperand(stmt, 5)));
	if (print_addr)
	    print_sid(fp, top, stmt, offset);
	SIfprintf(fp, ERx("\n"));
	break;

      case IL_KEYACT:
	SIfprintf(fp, ERx("INT(%d) STRING(%d) INT(%d) %d"),
		ILgetOperand(stmt, 1), ILgetOperand(stmt, 2),
		ILgetOperand(stmt, 3), ILgetOperand(stmt, 4),
		(offset = ILgetOperand(stmt, 5)));
	if (print_addr)
	    print_sid(fp, top, stmt, offset);
	SIfprintf(fp, ERx("\n"));
	break;

      case IL_ACTIMOUT:
	SIfprintf( fp, ERx("INT(%d) %d\n"),
		ILgetOperand(stmt, 1),
		(offset = ILgetOperand(stmt, 2))
	);
	if (print_addr)
	    print_sid(fp, top, stmt, offset);
	SIfprintf(fp, ERx("\n"));
	break;

      /*
      ** Sample IL statement:  INQFRS VALUE(23) VALUE(12) VALUE(7) 16
      */
      case IL_INQELM:
      case IL_INQFRS:
      case IL_SETFRS:
	SIfprintf(fp,
	    ERx("VALUE(%d) VALUE(%d) VALUE(%d) %d\n"),
	    ILgetOperand(stmt, 1), ILgetOperand(stmt, 2),
	    ILgetOperand(stmt, 3), ILgetOperand(stmt, 4));
	break;

      /*
      ** Sample IL statement:  DUMP 2 STRING(-83)
      */
      case IL_DUMP:
	SIfprintf(fp, ERx("%d STRING(%d)\n"), ILgetOperand(stmt, 1),
	    ILgetOperand(stmt, 2));
	break;

      /*
      ** Sample IL statement:  QRYSZE 4 2
      */
      case IL_QRYSZE:
	SIfprintf(fp, ERx("%d %d\n"), ILgetOperand(stmt, 1), ILgetOperand(stmt, 2));
	break;

      case IL_PARAM:
	SIfprintf(fp, ERx("VALUE(%d) VALUE(%d) %d %d %d\n"),
		ILgetOperand(stmt, 1), ILgetOperand(stmt, 2),
		ILgetOperand(stmt, 3), ILgetOperand(stmt, 4),
		ILgetOperand(stmt, 5)
	);
	break;

      case IL_EXPR:
	expr_size = ILgetI4Operand(stmt, 1);
	SIfprintf(fp, ERx("%d %d\n"), expr_size, ILgetI4Operand(stmt, 3));
	print_expr(fp, (stmt + iltab[il_op].il_stmtsize), expr_size);
	break;

      case IL_LEXPR:
	expr_size = ILgetI4Operand(stmt, 2);
	SIfprintf(fp, ERx("VALUE(%d) %d %d\n"), ILgetOperand(stmt, 1),
		expr_size, ILgetI4Operand(stmt, 4));
	print_expr(fp, (stmt + iltab[il_op].il_stmtsize), expr_size);
	break;

      default:
	switch (iltab[il_op].il_stmtsize)
	{
	  /*
	  ** No VALUEs for these IL statements.
	  ** Sample IL statement:  DISPLAY
	  */
	  case 1:
	    SIfprintf(fp, ERx("\n"));
	    break;

	  /*
	  ** One VALUE for these IL statements.
	  ** Sample IL statement:  RETFRM VALUE(12)
	  */
	  case 2:
	    SIfprintf(fp, ERx("VALUE(%d)\n"), ILgetOperand(stmt, 1));
	    break;
	  /*
	  ** Two VALUEs for these IL statements.
	  ** Sample IL statement:  HELPFORMS VALUE(12) VALUE(20)
	  */
	  case 3:
	    SIfprintf(fp, ERx("VALUE(%d) VALUE(%d)\n"),
		ILgetOperand(stmt, 1), ILgetOperand(stmt, 2));
	    break;
	  case 4:
	    SIfprintf(fp, ERx("VALUE(%d) VALUE(%d) VALUE(%d)\n"),
		ILgetOperand(stmt, 1), ILgetOperand(stmt, 2),
		ILgetOperand(stmt, 3));
	    break;
	  case 5:
	    SIfprintf(fp, ERx("VALUE(%d) VALUE(%d) VALUE(%d) VALUE(%d)\n"),
		ILgetOperand(stmt, 1), ILgetOperand(stmt, 2),
		ILgetOperand(stmt, 3), ILgetOperand(stmt, 4));
	    break;
	  case 6:
	    SIfprintf(fp, ERx("VALUE(%d) VALUE(%d) VALUE(%d) VALUE(%d) VALUE(%d)\n"),
		ILgetOperand(stmt, 1), ILgetOperand(stmt, 2),
		ILgetOperand(stmt, 3), ILgetOperand(stmt, 4),
		ILgetOperand(stmt, 5));
	    break;
	  case 7:
	    SIfprintf(fp, ERx("VALUE(%d) VALUE(%d) VALUE(%d) VALUE(%d) VALUE(%d) VALUE(%d)\n"),
		ILgetOperand(stmt, 1), ILgetOperand(stmt, 2),
		ILgetOperand(stmt, 3), ILgetOperand(stmt, 4),
		ILgetOperand(stmt, 5), ILgetOperand(stmt, 6));
	    break;
	  case 8:
	    SIfprintf(fp, ERx("VALUE(%d) VALUE(%d) VALUE(%d) VALUE(%d) VALUE(%d) VALUE(%d) VALUE(%d)\n"),
		ILgetOperand(stmt, 1), ILgetOperand(stmt, 2),
		ILgetOperand(stmt, 3), ILgetOperand(stmt, 4),
		ILgetOperand(stmt, 5), ILgetOperand(stmt, 6),
		ILgetOperand(stmt, 7));
	    break;
	  case 9:
	    SIfprintf(fp, ERx("VALUE(%d) VALUE(%d) VALUE(%d) VALUE(%d) VALUE(%d) VALUE(%d) VALUE(%d) VALUE(%d)\n"),
		ILgetOperand(stmt, 1), ILgetOperand(stmt, 2),
		ILgetOperand(stmt, 3), ILgetOperand(stmt, 4),
		ILgetOperand(stmt, 5), ILgetOperand(stmt, 6),
		ILgetOperand(stmt, 7), ILgetOperand(stmt, 8));
	    break;
	} /* end switch */
	break;
    } /* end switch */

    return;
}

static VOID
print_sid(fp, top, stmt, offset)
FILE	*fp;
IL	*top;
IL	*stmt;
i4	offset;
{
    if (*stmt & ILM_LONGSID)
	SIfprintf(fp, ERx("(LONGSID)"));
    else
	SIfprintf(fp, ERx("(%d)"), (i4)(stmt-top) + offset);
}

static VOID
print_expr (fp, code, size)
register FILE	*fp;
register IL	*code;
i4		size;
{
    register IL *cend = code + size;

    while (code < cend)
    {
	register i4	numops;
	register i4	i;

	numops = ILgetOperand(code, 1);
	SIfprintf(fp, ERx("\t%d (%d)"), *code, numops);
	/* Print operands -- Note that the DBVs are offset by one to
	**	agree with the offset in the IL code.  ADE compiled
	**	expression offsets are `true' offsets starting at 0.
	*/
	for (i = 0 ; i < numops ; ++i)
	    SIfprintf(fp, ERx(" %d"), ILgetOperand(code, i + 2) + 1);
	SIfprintf(fp, ERx("\n"));
	code += numops + 2;
    }
}

/*{
** Name:	IAOMpcPrintConsts() -	Print IL Constants Table.
**
** Description:
**	This routine prints all the intermediate language constants in
**	a table where the constants are all of the same type and are
**	represented as strings.
**
** Input:
**	fp	{FILE *}  Output file pointer.
**	type	{char *}  The name of the type of the constants.
**	consts	{char **}  The constants table.
**	text	{bool}  TEXT type strings (or just EOS-terminated strings.)
**
** History:
**	09/86 (jhw) -- Written.
**	12/87 (jhw) -- Added support to output TEXT type strings.
*/
VOID
IIAMpcPrintConsts (fp, type, consts, text)
FILE	*fp;
char	*type;
char	**consts;
bool	text;
{
    register char	**cp;

    SIfprintf(fp, ERx("%s\nBEGINLIST\n"), type);

    if ((cp = consts) != NULL)
    {
	register i4	offset = 1;

	while (*cp != NULL)
	    if (!text)
		SIfprintf(fp, ERx("LISTELM \"%s\"\t;_%d\n"), *cp++, offset++);
	    else
	    { /* TEXT type */
		register DB_TEXT_STRING	*dp = (DB_TEXT_STRING *)(*cp++);

		SIfprintf(fp, ERx("LISTELM %d\"%.*s\"\t;_%d\n"), 
				dp->db_t_count, dp->db_t_count, dp->db_t_text, offset++);
	    }
    }
    SIfprintf(fp, ERx("ENDLIST\n"));
}
