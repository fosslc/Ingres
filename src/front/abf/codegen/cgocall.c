/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<si.h>
#include	<st.h>
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

#include	"cggvars.h"
#include	"cgil.h"
#include	"cgout.h"
#include	"cgerror.h"


/**
** Name:    cgocall.c -	Code Generator Call Procedures Module.
**
** Description:
**	The generation section of the code generator for outputting
**	code to call procedures.
**	Defines:
**
**	IICGocbCallBeg()
**	IICGocaCallArg()
**	IICGoceCallEnd()
**	IICGtocToCstr()
**	IICGstaStrArg()
**	IICGrcsResetCstr()
**	IICGortReturnStmt()
**
** History:
**	Revision 6.3/03/01
**	02/26/91 (emerson)
**		Remove 2nd argument to IICGgadGetAddr (cleanup:
**		part of fix for bug 36097).
**
**	Revision 6.0  87/02  arthur
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


static i4  argcnt = 0;		/* 0 for the first element in a proc arg list */
/*
** Flag to indicate whether a call to IICGrcsResetCstr() is required.
** Can be set to TRUE by IICGstaStrArg(). Reset to FALSE by IICGrcsResetCstr().
*/
static bool resetReqed = FALSE;

/*{
** Name:    IICGocbCallBeg() -	Generate beginning of a procedure call
**
** Description:
**	Generates the beginning of a procedure call.
**
** Inputs:
**	name		Name of the procedure.
**
** History:
**	20-feb-1987 (agh)
**		First written.  Adapted from newosl/olcall.c.
*/
VOID
IICGocbCallBeg(name)
char	*name;
{
    char	buf[CGBUFSIZE];

    IICGoprPrint( STprintf(buf, ERx("%s("), name) );
    argcnt = 1;
    return;
}

/*{
** Name:    IICGocaCallArg() -	Generate element in proc argument list
**
** Description:
**	Generates an element in a procedure argument list.
**	Uses the static 'argcnt'.
**
** Inputs:
**	arg		The argument as a string.
**
*/
VOID
IICGocaCallArg (arg)
char	*arg;
{
    if (argcnt != 1)
	IICGoprPrint(ERx(", "));
    IICGoprPrint(arg);
    ++argcnt;
    return;
}

/*{
** Name:    IICGoceCallEnd() -	Generate close of proc argument list
**
** Description:
**	Generates the close of a procedure argument list.
**
** Inputs:
**	None.
**
*/
VOID
IICGoceCallEnd()
{
    IICGoprPrint(ERx(")"));
    return;
}

/*{
** Name:    IICGtocToCstr() -	Generate code to convert to a C string
**
** Description:
**	Generates C code which will convert a character string value
**	stored in a DB_DATA_VALUE into a null-terminated C string.
**	The code generated here always appears within a parameter list
**	to another procedure.
**
** Inputs:
**	val	Index of the string-oriented DB_DATA_VALUE.
**
*/
VOID
IICGtocToCstr (val)
IL	val;
{
    i4	old_arg;

    IICGocaCallArg(ERx(""));
    old_arg = argcnt;
    IICGocbCallBeg(ERx("IIARtocToCstr"));
    IICGocaCallArg(IICGgadGetAddr(val));
    IICGoceCallEnd();
    /*
    ** Restore number of arguments for outer procedure.
    */
    argcnt = old_arg;
    return;
}

/*{
** Name:    IICGstaStrArg() -	Generate code for a string argument
**
** Description:
**	Generates C code which will convert a character string value
**	stored in a DB_DATA_VALUE into a null-terminated C string.
**	The code generated here always appears within a parameter list
**	to another procedure.
**	Generates C code which will, at runtime, reset the buffer
**	used by the runtime system to convert string-oriented
**	DB_DATA_VALUEs into null-terminated C strings.
**
** Inputs:
**	val	Index of the string-oriented DB_DATA_VALUE.
**
*/
VOID
IICGstaStrArg (val)
register IL	val;
{
    if (ILISCONST(val))
    {
	IICGocaCallArg(IICGgvlGetVal(val, DB_CHR_TYPE));
    }
    else
    {
	IICGtocToCstr(val);
	resetReqed = TRUE;
    }
    return;
}

/*{
** Name:    IICGrcsResetCstr() - Generate code to reset RTS buffer
**
** Description:
**	Generates C code which will, at runtime, reset the buffer
**	used by the runtime system to convert string-oriented
**	DB_DATA_VALUEs into null-terminated C strings.
**
** History:
**
*/
VOID
IICGrcsResetCstr()
{
    if (resetReqed)
    {
	IICGosbStmtBeg();
	IICGocbCallBeg(ERx("IIARrcsResetCstr"));
	IICGoceCallEnd();
	IICGoseStmtEnd();
	resetReqed = FALSE;
    }
    return;
}

/*{
** Name:    IICGortReturnStmt() - Generate code for a C 'return' stmt
**
** Description:
**	Generates code for a C 'return' statement.
**
** Inputs:
**	None.
**
*/
VOID
IICGortReturnStmt()
{
    IICGosbStmtBeg();
    IICGocbCallBeg(ERx("return"));
    IICGocaCallArg(ERx("NULL"));
    IICGoceCallEnd();
    IICGoseStmtEnd();
    return;
}
