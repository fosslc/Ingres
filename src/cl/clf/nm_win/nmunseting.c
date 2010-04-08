#include <compat.h>
#include <gl.h>
#include <nm.h>
#include <lo.h>
#include <st.h>
#include "nmlocal.h"


/*{
** Name: NMzapIngAt- unset a value in the symbol.tbl.
**
** Description:
**    Undocumented in the CL spec; only called by the ingunset program.
**
**    Returns status.
**
**    This version rewrites the whole file whenever it changes.
**
** Inputs:
**	name				    symbol to be zapped.
**
** Output:
**	none
**
**      Returns:
**	    OK -- the symbol was deleted, or wasn't there in the first place
**	    FAIL
** History:
**      20-jul-87 (mmm)
**          Updated to meet jupiter standards.
**	24-may-90 (blaise)
**	    Integrated ingresug changes:
**	        Create this file from ingunset.c.
**	23-oct-1992 (jonb)
**	    Fixed unlinking code so it works properly if the sybol being
**	    zapped is the second one in the list.  (b47504)  Also removed
**	    some extraneous code.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	15-oct-94 (nanpr01)
**	    removed #include "nmerr.h". Bug # 63157. 
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	13-jan-2004 (somsa01)
**	    Added locking / unlocking the symbol.tbl file, as well as logging
**	    the successful deletion to the symbol.log file.
**	03-feb-2004 (somsa01)
**	    Backed out last changes for now.
**	06-apr-2004 (somsa01)
**	    Added logging of delete operation (good or bad) to the symbol.log
**	    file.
**      17-Nov-2004 (rigka01) bug 112953/INGSRV2955, cross Unix change 472415
**          Lock the symbol table while being updated to avoid corruption or
**          truncation.
*/

STATUS
NMzapIngAt(name)
register char	*name;
{
	register i4 	status	= OK;
	register SYM	*sp;
	register SYM	**lastsp;

        /* Lock the symbol table for update */
        if ( status = NMlocksyms() )
        {
            return (status);
        }
 
	/* Read the symbol table file once */
 
	if ( s_list == NULL && OK != (status = NMreadsyms()) )
	{
		NMunlocksyms();
		NMlogOperation("", NULL, NULL, NULL, status);

		return (status);
	}
 
	/*
	**  Search symbol table for the desired symbol, and cut out.
	**  Tedious linear search, but the list isn't very big.
	*/

	for ( lastsp = &s_list, sp = s_list; sp != NULL; 
	      lastsp = &sp->s_next, sp = sp->s_next )
	{
		if ( !STcompare(sp->s_sym, name) )
		{
			/* just cut out of the chain and rewrite */

			*lastsp = sp->s_next;
			status = NMwritesyms();
			break;
		}
	}

	NMunlocksyms();

	/* Log the deletion. */
	NMlogOperation("DELETE", name, NULL, NULL, status);

	return ( status );
}
