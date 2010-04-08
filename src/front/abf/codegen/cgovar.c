/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<cv.h>		/* 6-x_PC_80x86 */
#include	<si.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<er.h>
/* Interpreted Frame Object definition. */
#include	<ade.h>
#include	<frmalign.h>
#include	<fdesc.h>
#include	<iltypes.h>
#include	<ilrffrm.h>

#include	"cggvars.h"
#include	"cgout.h"
#include	"cgerror.h"


/**
** Name: cgvar.c -	Code Generator Variable References and Target Lists Module.
**
** Description:
**	The generation section of the code generator for outputting
**	variable references and target lists.
**	Defines:
**
**	IICGocvCVar()
**	IICGovaVarAssign()
**	IICGoaeArrEleAssign()
**	IICGosmStructMemAssign()
**	IICGiaeIntArrEle()
**	IICGfaeFltArrEle()
**	IICGhxcHexConst()
**	IICGdccDecConst()
**
** History:
**	Revision 6.0  87/02/25  arthur
**	Initial revision.  Adapted from "newosl/olvar.c."
**
**	Revision 6.4
**	04/07/91 (emerson)
**		Modifications for local procedures (in IICGhxcHexConst).
**	05/07/91 (emerson)
**		Cosmetic change in IICGhxcHexConst.
**	08/19/91 (emerson)
**		"Uniqueify" names by appending the frame id (generally 5
**		decimal digits) instead of the symbol name.  This will reduce
**		the probability of getting warnings (identifier longer than
**		n characters) from the user's C compiler.  (Bug 39317).
**
**	Revision 6.5
**	30-jun-92 (davel)
**		Added IICGdccDecConst() in support of decimal constants.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


/*
** Name:    VarType() -	Return string version of a type
**
** Description:
**	Returns the string version of a type.
**
** Inputs:
**	type		Type, expressed as a DB_xxx_TYPE.
**
** Returns:
**	{char *}  String for corresponding type.
*/
static char *
VarType(type)
i4	type;
{
    switch (type)
    {
      case DB_INT_TYPE:
	return ERx("int");

      case DB_FLT_TYPE:
	return ERx("double");

      case DB_CHR_TYPE:
      case DB_DEC_TYPE:
	return ERx("char");

      default:
	return ERx("ERROR");
    }
}

/*{
** Name:    IICGocvCVar() -	Generate a C variable
**
** Description:
**	Generates a C variable.  This routine can generate several
**	types of variable declarations including arrays and initialized
**	variables.
**
** Inputs:
**	name		The name of the C variable.
**	type		The type of variable, expressed as a DB_xxx_TYPE.
**	rows		The number of rows to give the variable.  If 0,
**			then the variable is not an array.  If rows is
**			OLZEROARR then the variable is an array whose size is
**			unknown.
**	init		CG_INIT if variable is to be initialized,
**			else CG_NOINIT.
**
*/
static bool ifirst = 0;
static i4    istate = 0;

VOID
IICGocvCVar(name, type, rows, init)
char	*name;
i4	type;
i4	rows;
i4	init;
{
    char	buf[CGBUFSIZE];

    ifirst = TRUE;
    istate = init;
    IICGosbStmtBeg();
    STprintf(buf, ERx("%s\t%s"), VarType(type), name);
    IICGoprPrint(buf);
    if (rows > 0)
    {
	STprintf(buf, ERx("[%d]"), rows);
	IICGoprPrint(buf);
    }
    if (init == CG_INIT)
	IICGoprPrint(ERx(" = "));
    else
	IICGoseStmtEnd();

    return;
}

/*{
** Name:    IICGovaVarAssign() -	Assign a value to a C variable
**
** Description:
**	Assigns a value to a C variable.
**
** Inputs:
**	name		Name of the variable.
**	value		Value on right hand side of assignment.
**
*/
IICGovaVarAssign(name, value)
char	*name;
char	*value;
{
    char	buf[CGBUFSIZE];

    IICGosbStmtBeg();
    STprintf(buf, ERx("%s = %s"), name, value);
    IICGoprPrint(buf);
    IICGoseStmtEnd();
    return;
}

/*{
** Name:    IICGoaeArrEleAssign() -	Assign a value to a C array element
**
** Description:
**	Assigns a value to an element of a C array.
**
** Inputs:
**	name		Name of the structure variable.
**	member		Name of the member of the structure.
**	index		Index in the array.
**	value		Value on right hand side of assignment.
**
*/
VOID
IICGoaeArrEleAssign(name, member, index, value)
char	*name;
char	*member;
i4	index;
char	*value;
{
    char	buf[CGBUFSIZE];

    IICGosbStmtBeg();
    if (member == NULL)
	STprintf(buf, ERx("%s[%d] = %s"), name, index, value);
    else
	STprintf(buf, ERx("%s[%d].%s = %s"), name, index, member, value);
    IICGoprPrint(buf);
    IICGoseStmtEnd();
    return;
}

/*{
** Name: IICGosmStructMemAssign	-	Assign a value to a member of a C struct
**
** Description:
**	Assigns a value to a member of a C structure.
**
** Inputs:
**	name		Name of the structure variable.
**	member		Name of the member of the structure.
**	value		Value on right hand side of assignment.
**
*/
VOID
IICGosmStructMemAssign(name, member, value)
char	*name;
char	*member;
char	*value;
{
	char	buf[CGBUFSIZE];

	IICGosbStmtBeg();
	STprintf(buf, ERx("%s.%s = %s"), name, member, value);
	IICGoprPrint(buf);
	IICGoseStmtEnd();
	return;
}

/*{
** Name: IICGiaeIntArrEle	-	Generate element in an array of integers
**
** Description:
**	Generates an element in an array of integers.
**	Up to 10 elements are printed out on each line.
**	The corresponding routine IICGfaeFltArrEle() generates an
**	element in an array of floats.
**
** Inputs:
**
**	val	The element.
**	last	CG_LAST if this is the last element in the array,
**		else CG_NOTLAST.
**
*/
VOID
IICGiaeIntArrEle(val, last)
i4	val;
i4	last;
{
	static i4	ilLineLen = 0;	/* number of elements so far on line */
	char		buf[CGSHORTBUF];

	CVna(val, buf);
	if (ilLineLen == 0)
		IICGosbStmtBeg();
	IICGoprPrint(buf);
	if (++ilLineLen <= 9 && last == CG_NOTLAST)
	{
		IICGoprPrint(ERx(", "));
	}
	else
	{
		ilLineLen = 0;
		IICGoeeEndEle(last);
	}
	return;
}

/*{
** Name: IICGfaeFltArrEle	-	Generate element in an array of floats
**
** Description:
**	Generates an element in an array of floats.
**	The corresponding routine IICGiaeIntArrEle() generates an
**	element in an array of integers.
**
** Inputs:
**
**	val	The element, as a string constant.
**	last	CG_LAST if this is the last element in the array,
**		else CG_NOTLAST.
**
*/
VOID
IICGfaeFltArrEle(val, last)
char	*val;
i4	last;
{
	IICGosbStmtBeg();
	IICGoprPrint(val);
	IICGoeeEndEle(last);
	return;
}

/*{
** Name: IICGhxcHexConst	-	Generate a struct for a hex constant
**
** Description:
**	Generates a structure for a hex constant.
**	Each struct contains the length and value of the hex constant.
**
** Inputs:
**	len	The length of the hex constant, in bytes.
**	val	The value of the hex constant.
**	index	Index of the hex constant among the set of them.
**
** History:
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		"Uniqueify" names of constants.  (See ConstArrs in cgfiltop.c).
**	05/07/91 (emerson)
**		Cosmetic change to prevent indentation of elements
**		in hex constants from getting screwed up.
**		(Print the integer member of the structure by hand:
**		IICGiaeIntArrEle doesn't work if the last integer
**		in a structure is followed by non-integers).
*/
VOID
IICGhxcHexConst(len, val, index)
i4	len;
char	*val;
i4	index;
{
	char	buf1[CGSHORTBUF];
	char	buf2[CGSHORTBUF];

	/*
	** Declare struct.  Add 1 to hx_len for EOS.
	*/
	STprintf(buf1, ERx("static struct {i2 hx_len; char hx_val[%d];}"),
		(len + 1));
	STprintf(buf2, ERx("IIChx%d_%d"), index, CG_fid);
	IICGostStructBeg(buf1, buf2, 0, CG_INIT);
	IICGosbStmtBeg();
	CVna(len, buf1);
	IICGoprPrint(buf1);
	IICGoprPrint(ERx(", "));
	IICGpscPrintSconst(val, len);
	IICGoleLineEnd();
	IICGosdStructEnd();
	return;
}

/*{
** Name: IICGdccDecConst - Generate a struct for a decimal constant
**
** Description:
**	Generates a structure for a decimal constant.
**	The decimal constants are represented as a char string which
**	contains the encoded decimal values.
**
** Inputs:
**	ps	The precision/scale of the decimal constants.
**	strval	The value of the decimal constant as an ASCII string. 
**		This is shown in a comment after the data is declared.
**	val	The internal value of the decimal constant.
**	index	Index of the decimal constant among the set of them.
**
** History:
**	30-jun-92 (davel)
**		Written.
*/
VOID
IICGdccDecConst(ps, strval, val, index)
i2	ps;
char	*strval;
char	*val;
i4	index;
{
	i4 	len;
	char	buf[CGSHORTBUF];

	STprintf( buf, ERx("IICdecs%d_%d"), index, CG_fid );
	IICGostStructBeg(ERx("static char"), buf, 0, CG_ZEROINIT);
	IICGosbStmtBeg();
	/* sure would be nice to have a DB_PS_TO_LEN_MACRO in dbms.h */
	len = DB_PREC_TO_LEN_MACRO(DB_P_DECODE_MACRO(ps));
	IICGpscPrintSconst(val, len);
	IICGoprPrint(ERx("\t\t/* Decimal constant "));
	IICGoprPrint(strval);
	IICGoprPrint(ERx("\t*/"));
	IICGoeeEndEle(CG_LAST);
	IICGosdStructEnd();
	return;
}
