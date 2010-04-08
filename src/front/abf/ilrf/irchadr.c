
/*
**	Copyright (c) 1986, 2004 Ingres Corporation
*/

#include <compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include <fe.h>
# include	<er.h>
#include "irstd.h"
#include "irmagic.h"

/**
** Name:	irchadr.c - check address
**
** Description:
**	check address for IL array range
** History:
**	13-sep-93 (kchin)
**		Removed cast of stmt and frm->il to i4 in IIORcaCheckAddr(), 
**		as these two are holding addresses, a cast to i4 will result
**	        in truncation of 64-bit address.  Since the statement is 
**		computing IL-sized offset (not byte offset), the statement
**		can be coded as:
**			offset = stmt - frm->il;
**	13-May-2005 (kodse01)
**	    replace %ld with %d for old nat and long nat variables.
*/

#ifdef ILRFDTRACE
extern FILE *Fp_debug;
#endif

/*{
** Name:	IIORcaCheckAddr -	Check an IL statement ptr
**
** Description:
**	Checks a pointer to an IL statement to be sure that it
**	is within the current block of IL code.
**
** Inputs:
**	irblk		IRBLK of client
**	stmt		The IL statement just calculated.
**
** Outputs:
**	Returns:
**		OK		address is legal
**		ILE_CLIENT	not a legal client
**		ILE_ARGBAD	statement out of range
**		ILE_CFRAME	no current frame
**
** History:
**	8/86 (rlm)	written
*/
IIORcaCheckAddr(irblk,stmt)
IRBLK	*irblk;
IL	*stmt;
{
	IFRMOBJ *frm;
	i4 offset;

	/* check magic number, that there is a current frame */
	CHECKMAGIC(irblk,ERx("ca"));
	CHECKCURRENT(irblk);

	frm = ((IR_F_CTL *)(irblk->curframe))->d.m.frame;
	offset = stmt - frm->il;

#ifdef ILRFDTRACE
	fprintf(Fp_debug,ERx("offset %d, array length %d\n"),offset,frm->num_il);
#endif

	if (offset >= frm->num_il || offset < 0)
		return (ILE_ARGBAD);
	return(OK);
}
