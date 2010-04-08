/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
#include	<si.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
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
** Name:    cgout.c -	Code Generator Output Module.
**
** Description:
**	The outputting routines of the code generator which produces
**	C code from the OSL intermediate language.
**
**	This file defines:
**
**	IICGoinIndent() -	Change level of indentation
**	IICGoflFlush() -	Flush an output file.
**	IICGoprPrint() -	Print out a string
**
** History:
**	Revision 6.0  87/02  arthur
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


/*
** To maintain nice indenting in the program, an array of tabs (IICGoiaIndArray)
** is maintained to indent over the proper amount.  The array contains
** as many tabs as necessary to indent over. A null byte is placed
** in the array to change the level.
*/
static i4	indptr = 0;	/* subscript of null byte in array of tabs */

/*{
** Name:    IICGoinIndent() -	Change level of indentation
**
**	Change level of indentation in an output file.
**	Maximum number of levels is determined by size of 'IICGoiaIndArray'
**	array (currently 15).
**
** Inputs:
**	level	{nat}	Level to move to relative to current one.
**
** Returns:
**	{STATUS}  OK if new level is within acceptable range, else FAIL.
**
** History:
**	18-feb-1987 (agh)
**		First written
*/
STATUS
IICGoinIndent(level)
i4	level;
{
    if (indptr + level < 0 || indptr + level >= 15)
	return FAIL;
    IICGoiaIndArray[indptr] = '\t';
    indptr += level;
    IICGoiaIndArray[indptr] = EOS;
    return OK;
}

/*{
** Name:    IICGoflFlush() -	Flush an output file.
**
** Description:
**	Flushes the output file.
*/
VOID
IICGoflFlush()
{
    if (CG_of_ptr != NULL)
    {
	SIflush(CG_of_ptr);
	if (CG_of_name != NULL)
	    SIclose(CG_of_ptr);
    }
    return;
}

/*{
** Name:    IICGoprPrint() -	Print out a string
**
** Description:
**	Print out a constant or string to the output file.
**	Starts a new line in the output file if the string, added to the
**	current contents of the buffer, is >= 256 bytes.
**
** Inputs:
**	cp	{char *}  The string.
**
** Side effects:
**	Prints to the output file.
*/
VOID
IICGoprPrint(cp)
char	*cp;
{
    i4	len;

    len = STlength(cp);
    IICGotlTotLen += len;
    if (IICGotlTotLen >= CGBUFSIZE-1)
    {
	SIputc('\n', CG_of_ptr);
	SIfprintf(CG_of_ptr, "%s", IICGoiaIndArray);
	IICGotlTotLen = len + STlength(IICGoiaIndArray);
    }
    /*
    ** Print directly to the output file.
    */
    SIfprintf(CG_of_ptr, "%s", cp);
    return;
}
