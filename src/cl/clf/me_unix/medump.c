# include	<compat.h>
# include	<gl.h>
# include	<clconfig.h>
# include 	<me.h>
# include	<meprivate.h>
# include	<si.h>

/*
**
**	MEdump.c
**		contains the routines
**			MEdump     -- print the specified list
**
** History:
**		16-Dec-1986 (daveb)
**		Changed to call MExdump.
**	20-nov-92 (pearl)
**		Add #ifdef for "CL_BUILD_WITH_PROTOS" and prototyped function
**		headers.
**      08-feb-1993 (pearl)
**          Add #include of me.h.
**      8-jun-93 (ed)
**              changed to use CL_PROTOTYPED
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	11-aug-93 (ed)
**	    unconditional prototypes
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


/*
**Copyright (c) 2004 Ingres Corporation
**
**	Name:
**		MEdump.c
**
**	Function:
**		MEdump
**
**	Arguments:
**		i4 chain;
**
**	Result:
**		Print to stdout the allocated and free lists for this process.
**
**		Returns STATUS: OK, ME_00_CMP, ME_BD_CHAIN, ME_OUT_OF_RANGE.
**
**	Side Effects:
**		None
**
**	History:
**		03/83 - (gb)
**			written
**	21-may-90 (blaise)
**	    Add <clconfig.h> include to pickup correct ifdefs in <meprivate.h>.
**      23-may-90 (blaise)
**          Add "include <compat.h>."
**	8-jun-93 (ed)
**	    removed SIprintf extern which mismatched prototype
**      15-may-1995 (thoro01)
**          Added NO_OPTIM hp8_us5 to file.
**      19-jun-1995 (hanch04)
**          remove NO_OPTIM, installed HP patch PHSS_5504 for c89
**	08-nov-1995 (canor01)
**	    Cast SIprintf to match prototype and clean up compiler warning.
*/
 


STATUS
MEdump(
	i4	chain)
{
    return( MExdump( chain, (i4 (*)())SIprintf ) );
}
