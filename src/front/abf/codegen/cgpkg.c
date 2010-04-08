/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<cm.h>
#include	<st.h>
#include	<si.h>
#include	<lo.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<er.h>
#include	<iltypes.h>
#include	<ilops.h>
/* Interpreted Frame Object definition. */
#include	<ade.h>
#include	<adf.h>
#include	<afe.h>
#include	<frmalign.h>
#include	<fdesc.h>
#include	<ilrffrm.h>
/* ILRF Block definition. */
#include	<abfrts.h>
#include	<ioi.h>
#include	<ifid.h>
#include	<ilrf.h>

#include	"cgerror.h"
#include	"cgil.h"
#include	"cggvars.h"
#include	"cgilrf.h"
#include	"cgout.h"


/**
** Name:    cgpkg.c -	Code Generator Data Abstractions Module.
**
** Description:
**	Data abstractions.
**	Hides implementation of an operand.
**	Actually, information on the data type and length of a
**	particular operand are stored in the corresponding DB_DATA_VALUE.
**
** History:
**	Revision 6.0  87/06  arthur
**	Initial revision.
**
**	Revision 6.3/03/00
**	11/19/90 (emerson)
**		Fix for bug 34529 in function IICGgvlGetVal.
**
**	Revision 6.3/03/01
**	02/26/91 (emerson)
**		Fix for bug 36097: Cleaned up function IICGgadGetAddr
**		(changed description, removed 2nd argument, and streamlined
**		the implementation) and created new function IICGgdaGetDataAddr.
**
**	Revision 6.4
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Call IICGdgDbdvGet instead of IIORdgDbdvGet.
**		"Uniqueify" constant names
**		(IICGgvlGetVal and IICGgdaGetDataAddr).
**	08/19/91 (emerson)
**		"Uniqueify" names by appending the frame id (generally 5
**		decimal digits) instead of the symbol name.  This will reduce
**		the probability of getting warnings (identifier longer than
**		n characters) from the user's C compiler.  (Bug 39317).
**
**	Revision 6.4/02
**	09/21/91 (emerson)
**		Fix for bug 39418 in IICGpscPrintSconst.
**
**	Revision 6.5
**	30-jun-92 (davel)
**		added decimal datatype support.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

IL_DB_VDESC	*IICGdgDbdvGet();

/*{
** Name:    IICGvltValType() -	Return the type of a VALUE
**
** Description:
**	Returns the DB_xxx_TYPE of a VALUE (e.g., DB_INT_TYPE, DB_FLT_TYPE,
**	DB_CHR_TYPE, DB_DEC_TYPE, etc.).
**
** Inputs:
**	val	{ILREF}  Index of the VALUE in the DB_DATA_VALUE array.
**
** Returns:
**	{DB_DT_ID}  The type of the VALUE.
*/
DB_DT_ID
IICGvltValType (val)
ILREF	val;
{
    return IICGdgDbdvGet(val)->db_datatype;
}

/*{
** Name:    IICGvllValLen() -	Return the length of a DB_DATA_VALUE
**
** Description:
**	Returns the length of a DB_DATA_VALUE.
**
** Inputs:
**	val	{ILREF}  Index of the VALUE in the DB_DATA_VALUE array.
**
** Returns:
**	{nat}	The length of the data area of the DB_DATA_VALUE.
*/
i4
IICGvllValLength(val)
ILREF	val;
{
    i4			retval;
    IL_DB_VDESC		*dbdv;

    dbdv = IICGdgDbdvGet(val);
    retval = (i4)dbdv->db_length;
    if (AFE_NULLABLE_MACRO(dbdv->db_datatype))
	retval -= 1;
    return (retval);
}

/*{
** Name:    IICGgvlGetVal() -	Get the data of an operand
**
** Description:
**	Returns a string representing a reference to the data of an operand.
**	If the operand is a constant, the return value is a string
**	representation of that value.  If the operand is a DB_DATA_VALUE,
**	the return value is a reference to the corresponding DB_DATA_VALUE
**	in the local array.
**
** Inputs:
**	val	{ILREF}  The operand.  If positive, this is the index of the
**			 value's DB_DATA_VALUE in the base array.  If negative,
**			 this is the offset of the value in the appropriate
**			 constants table.
**	type	{DB_DT_ID}  The type of constant:  DB_INT_TYPE, DB_FLT_TYPE,
**			 DB_CHR_TYPE, DB_DEC_TYPE, or the DB_HEX_TYPE.
**
** Returns:
**	{char *}  A string representation of the value.
**
** History
**	11/90 (Mike S)
**		Check datatype if a non-literal is passed as a char
**	11/19/90 (emerson)
**		Generate the right number of left parentheses for
**		comparisons against int, float, and pointer global constants.
**		(Problem was introduced by previous fix).  (Bug 34529).
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		"Uniqueify" names of constants
**		(See notes in ConstArrs in cgfiltop.c).
**	30-jun-92 (davel)
**		added support for decimal datatype.
**	14-feb-08 (smeke01) b119934
**		Cast i8s to i4s to maintain compatibility with abf runtime.
*/
static char	Buf[CGBUFSIZE];

char *
IICGgvlGetVal (val, type)
ILREF		val;
DB_DT_ID	type;
{
	register char	*format;
	char		ref[CGSHORTBUF];
	bool skip_length = FALSE;	/* Skip the length word in a varchar */

	if (type == DB_QUE_TYPE)
	{
		format = ERx("((QRY *) IIdbdvs[%d].db_data)");
		return STprintf(Buf, format, val - 1);
	}
	if (ILISCONST(val))
	{
		val = -val;

		switch (type)
		{
		  case DB_INT_TYPE:
			format = ERx("IICints_%d[%d]");
			return STprintf(Buf, format, CG_fid, val - 1);

		  case DB_FLT_TYPE:
			format = ERx("IICflts_%d[%d]");
			return STprintf(Buf, format, CG_fid, val - 1);

		  case DB_DEC_TYPE:
			format = ERx("IICdecs%d_%d");
			return STprintf(Buf, format, val - 1, CG_fid);

		  case DB_TXT_TYPE:
		  case DB_VCH_TYPE:
			format = ERx("IIChx%d_%d.hx_val");
			return STprintf(Buf, format, val - 1, CG_fid);

		  case DB_CHR_TYPE:
		  default:
			format = ERx("IICst%d_%d");
			return STprintf(Buf, format, val - 1, CG_fid);

		} /* end switch - control never gets here */
	}

	switch (type)
	{
	  case DB_INT_TYPE:
	  {
		int len = IICGvllValLength(val);
		if ( len <= 4 )
		  _VOID_ STprintf(ref, ERx("*(((i%d *)"), len);
		else
		  _VOID_ STprintf(ref, ERx("(i4) *(((i%d *)"), len); 
		break;
	  }

 	  case DB_FLT_TYPE:
		_VOID_ STprintf(ref, ERx("*(((f%d *)"), IICGvllValLength(val));
		break;

	  case DB_TXT_TYPE:
	  case DB_VCH_TYPE:
		_VOID_ STcopy(ERx("((_PTR_"), ref);
		break;

	  case DB_CHR_TYPE:
		switch (IICGvltValType(val))
		{
		  case DB_TXT_TYPE:
		  case DB_VCH_TYPE:
			skip_length = TRUE;
			break;
		}
		/* Fall through ... */
	  case DB_DEC_TYPE:
	  default:
		_VOID_ STcopy(ERx("(((char *)"), ref);
		break;
	} /* end switch */
	if (skip_length)
	{
		format = STcat(ref, ERx(" IIdbdvs[%d].db_data)+2)"));
	}
	else
	{
		format = STcat(ref, ERx(" IIdbdvs[%d].db_data))"));
	}
	return STprintf(Buf, format, val - 1);
}

/*{
** Name:    IICGginGIndex() -	Get an array or tablefield index.
**
** Description:
**
** Inputs:
**	val	{ILREF}  The operand.  
**
** Side Effects:
**	may output a line setting up the index value.
*/
static char	Ibuf[CGBUFSIZE];

char *
IICGginGetIndex (val, defval, msg)
ILREF		val;
char		*defval;
char		*msg;
{
    char *p;

    if (ILNULLVAL(val))
    {
	p = defval;
    }
    else if (ILISCONST(val))
    {
	p = IICGgvlGetVal(val, DB_INT_TYPE);
    }
    else
    {
    	char		lbuf[CGSHORTBUF];

	IICGosbStmtBeg();
	IICGoprPrint(ERx("IIrownum = "));
	IICGocbCallBeg(ERx("IIARivlIntVal"));
	IICGocaCallArg(IICGgadGetAddr(val));
	IICGocaCallArg(defval);
	IICGocaCallArg(STprintf(lbuf, ERx("\"%s\""), msg));
	IICGoceCallEnd();
	IICGoseStmtEnd();
	return ERx("IIrownum");
    }
    STlcopy(p, Ibuf, sizeof(Ibuf) - 1);
    return Ibuf;
}

/*{
** Name:    IICGgadGetAddr() -	Get the address of the DBDV of an operand
**
** Description:
**	Returns a string representing the address of the DBDV of an operand.
**
** Inputs:
**	val	The operand.  Must be nonnegative.  This is the index of the
**		value's DB_DATA_VALUE in the base array.
**
** Returns:
**	{char *}  A string representation of the address.
**
** History:
**	02/26/91 (emerson)
**		Fix for bug 36097: Changed description to reflect the fact that
**		this function gets the address of the DBDV, not the data itself.
**		Removed logic that handled the case where val was negative
**		(i.e. represented a constant); there are no DBDVs associated
**		with constant operands.  Removed irrelevant second argument.
*/
static char	addr_buf[CGSHORTBUF];

char *
IICGgadGetAddr (val)
ILREF		val;
{
    return STprintf(addr_buf, ERx("&IIdbdvs[%d]"), val - 1);
}

/*{
** Name:    IICGgdaGetDataAddr() - Get the address of an operand's data
**
** Description:
**	Returns a string representing the address of an operand's data.
**
** Inputs:
**	val	The operand.  If positive, this is the index of the
**		value's DB_DATA_VALUE in the base array.  If negative, this
**		is the offset of the value in the appropriate constants table.
**	type	The type of constant:  DB_INT_TYPE, DB_FLT_TYPE, DB_CHR_TYPE,
**		DB_DEC_TYPE, or the DB_HEX_TYPE.
**
** Returns:
**	{char *}  A string representation of the address.
**
** History:
**	02/26/91 (emerson)
**		Created from old IICGgadGetAddr.  (Fix for bug 36097).
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		"Uniqueify" names of constants
**		(See notes in ConstArrs in cgfiltop.c).
**	30-jun-92 (davel)
**		added support for decimal datatype.
*/
char *
IICGgdaGetDataAddr (val, type)
ILREF		val;
DB_DT_ID	type;
{
	register char	*format;

	if (!ILISCONST(val))
	{
		format = ERx("IIdbdvs[%d].db_data");
		return STprintf(addr_buf, format, val - 1);
	}

	val = -val;
	switch (type)
	{
	  case DB_INT_TYPE:
		format = ERx("_PTR_ &IICints_%d[%d]");
		return STprintf(Buf, format, CG_fid, val - 1);

	  case DB_FLT_TYPE:
		format = ERx("_PTR_ &IICflts_%d[%d]");
		return STprintf(Buf, format, CG_fid, val - 1);

	  case DB_TXT_TYPE:
	  case DB_VCH_TYPE:
		format = ERx("_PTR_ &IIChx%d_%d");
		return STprintf(Buf, format, val - 1, CG_fid);

	  case DB_DEC_TYPE:
		format = ERx("_PTR_ IICdecs%d_%d");
		return STprintf(Buf, format, val - 1, CG_fid);

	  case DB_CHR_TYPE:
	  default:
		format = ERx("_PTR_ IICst%d_%d");
		return STprintf(Buf, format, val - 1, CG_fid);
	}
}

/*{
** Name:    IICGpscPrintSconst() -	Print a string constant
**
** Description:
**	Given the value returned for the constant by ILRF, this routine outputs
**	a C language string constant.  This involves (1) enclosing the entire
**	constant within double quotes; and (2) escaping any special characters
**	within the string.  See the description of the _print_class table below
**	for a discussion of which characters are "special" and how they are
**	printed.
**
**	If the C string constant is longer than CGBUFSIZE-1 characters, the
**	input string is output as individual characters, terminated by an EOS.
**
**	This routine is also used to print out hexadecimal constants as
**	C strings.  Since hexadecimal constants can included embedded EOSs,
**	the 'len' argument indicates the length of the entire constant.
**
**	Note:  In no case should the output string be larger than CGBUFSIZE-1
**	characters.  This is the maximum, which must account for the new-line
**	that will be added by the output routine.
**
** Inputs:
**	str	{char *}  The original string or hex constant.
**	len	{nat}  Length of the constant.
**
** History:
**	08/87 (jhw) -- Re-written.
**	09/21/91 (emerson)
**		Fix for bug 39418: Don't use CMprint to determine which
**		characters are "special"; use the new _print_class table instead
**    7-dec-93 (rudyw)
**		Fix for bug 47515. Remove ? from list of backslashed chars.
**		Preserve trigraph protection while avoiding compiler warnings
**		by defaulting the ? character to be output in octal format.
*/
static	char	_OctalFmt[]	= ERx("\\%03o");
static	char	_OctalRep[]	= ERx("\\nnn");	/* fixed size */

/*
** The following table maps an input value of type u_char
** into a code of type char that indicates how the input value
** is to be represented in a C string or character literal.
** The table is initialized on the first call to IICGpscPrintSconst.
**
** The meaning of the codes are:
** (1) '\000' (0) indicates that the input value should be represented
**     by an octal escape sequence.
** (2) ' ' (space) indicates that the input value should be represented
**     by itself (it's not a "special" character).
** (3) any other code value indicates that the input value should be represented
**     by the code value, preceded by a '\' (backslash).
**
** Input values which fall into cases (2) and (3) must work on all
** possible C compilers that customers might use to compile our generated C.
** For (2), I have chosen space, letters, numbers, '_' (underscore - valid
** in an identifier), and all special characters valid as "punctuation"
** (e.g. as an expression operator or in a preprocessor directive).
** For (3), I have chosen the control characters documented in 2.4.3
** in appendix A of Kernighan and Ritchie: '\n', '\t', '\b', '\r', '\f'.
** I also precede each of \ ' " ? by \.  (I precede ? by \ so that trigraphs
** won't cause problems with ANSI-compliant compilers; otherwise, e.g.
** the string "??(" could get interpreted as "[").  The \ is superflous
** but harmless before ' in string literals and before " and ? in
** character literals.
** Note (7-dec-93): The preceding explanation on ? has been rendered obsolete
** by the fix for bug 47515 (See History entry), but is left as history.
**
** Note that the generated C source file need not be portable across platforms.
** (The 4GL compilation and the C compilation are always done as a unit).
** If portability *were* an issue, we'd have problems going between platforms
** with different character sets: characters that fall into case (1)
** would cause problems where the application is interested in the characters
** rather than their binary values, and characters that fall into case (2) & (3)
** would cause problems where the application is interested in the characters'
** binary values.
**
** One final note: we could simplify this routine a lot by always using
** octal escape sequences or always representing the string as a list
** of integer values, but readability of the generated C code would suffer
** considerably.
*/
static	char	_print_class[1 << BITSPERBYTE] = {0};

VOID
IICGpscPrintSconst (str, length)
char	*str;
i4	length;
{
    char		obuf[CGBUFSIZE];	/* output buffer */
    register char	*ocp;			/* pointer into output buffer */
    register char	*icp;			/* pointer into input string */
    register char	ic, c;
    register i4	len;			/* length */
    register char	*endobuf;		/* marker for end of obuf */

    /*
    ** Initialize _print_class table on first call.
    */
    if (_print_class[(u_char)' '] == 0)
    {
	for (icp = ERx("abcdefghijklmnopqrstuvwxyz"); ic = *icp; icp++)
	{
	    _print_class[(u_char)ic] = ' ';
	}
	for (icp = ERx("ABCDEFGHIJKLMNOPQRSTUVWXYZ"); ic = *icp; icp++)
	{
	    _print_class[(u_char)ic] = ' ';
	}
	for (icp = ERx("0123456789"); ic = *icp; icp++)
	{
	    _print_class[(u_char)ic] = ' ';
	}
	for (icp = ERx(" |!#$%^&*()_+-={}[]:~;<>,./"); ic = *icp; icp++)
	{
	    _print_class[(u_char)ic] = ' ';
	}
	_print_class[(u_char)'\n'] = 'n';
	_print_class[(u_char)'\t'] = 't';
	_print_class[(u_char)'\b'] = 'b';
	_print_class[(u_char)'\r'] = 'r';
	_print_class[(u_char)'\f'] = 'f';

	_print_class[(u_char)'\\'] = '\\';
	_print_class[(u_char)'\''] = '\'';
	_print_class[(u_char)'\"'] = '\"';
    }

    endobuf = &obuf[CGBUFSIZE-1];

    /*
    ** Print out an input string as a C string or byte by byte.
    **
    ** If special characters and non-printing characters cause the
    ** output string to be too long to print as a single C string,
    ** then print the input string byte by byte.
    */
    if ((len = length) < CGBUFSIZE - 2 - 1)
    {
	icp = str;
	ocp = obuf;

	*ocp++ = '"';
	while (len-- > 0)
	{
	    ic = *icp++;
	    c = _print_class[(u_char)ic];
	    if (c == ' ') 		/* printable, non-special character */
	    { 
		if (ocp >= endobuf)
		{
		    break;
		}
		*ocp++ = ic;
	    }
	    else if (c == 0)		/* octal escape sequence required */
	    {
		if (ocp + sizeof(_OctalRep)-2 >= endobuf)
		{
		    break;
		}
		_VOID_ STprintf(ocp, _OctalFmt, (u_char)ic);
		ocp += sizeof(_OctalRep)-1;
	    }
	    else			/* 2-character escape sequence */
	    {
		if (ocp + 1 >= endobuf)
		{
		    break;
		}
		*ocp++ = '\\';
		*ocp++ = c;
	    }
	} /* end while */
    }

    if (len < 0 && ocp + 1 < endobuf)
    {
	*ocp++ = '"';
	*ocp = EOS;
    }
    else
    { /* String too large to print as C string */
	icp = str;
	len = length;
	ocp = obuf;
	while (len-- > 0)
	{
	    if (ocp + sizeof(ERx("'\\nnn',"))-1 >= endobuf)
	    {
		*ocp = EOS;
		IICGoprPrint(ocp = obuf);
	    }

	    *ocp++ = '\'';
	    ic = *icp++;
	    c = _print_class[(u_char)ic];
	    if (c == ' ') 		/* printable, non-special character */
	    {
		*ocp++ = ic;
	    }
	    else if (c == 0)		/* octal escape sequence required */
	    {
		_VOID_ STprintf(ocp, _OctalFmt, (u_char)ic);
		ocp += sizeof(_OctalRep)-1;
	    }
	    else			/* 2-character escape sequence */
	    {
		*ocp++ = '\\';
		*ocp++ = c;
	    }
	    *ocp++ = '\'';
	    *ocp++ = ',';
	} /* end while */
	if (ocp + sizeof(ERx("'\\0'")) >= endobuf)
	{
	    *ocp = EOS;
	    IICGoprPrint(ocp = obuf);
	}
	*ocp++ = '\'';
	*ocp++ = '\\';
	*ocp++ = '0';
	*ocp++ = '\'';
	*ocp = EOS;
    }
    IICGoprPrint(obuf);
}
