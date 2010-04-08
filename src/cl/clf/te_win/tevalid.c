# include	<compat.h>
# include	<gl.h>
# include       <me.h>
# include	<cs.h>

/*
**  TEvalid
**	return validity of terminal name
**
**  Arguments
**	ttyname  --  terminal name to check;  this should be the name only,
**			no path should be on the front
**
**  Returns TRUE if the given terminal name is in a valid format,
**	begins with the prefix "tty" (ttyh3, ttyi4, ...), "co" (console)
**	or "cu" (call unix port).
**  FALSE otherwise
**
** History:
**	29-may-90 (blaise)
**	    Integrated changes from ingresug:
**		Include <me.h> before calling MEcmp.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	23-may-95 (tutto01)
** 	    Added semaphor protection for non-reentrant call to ttyname.
*/

#ifdef _REENTRANT
GLOBALREF       CS_SEMAPHORE    CL_misc_sem;
#endif /* _REENTRANT */

bool
TEvalid(ttyname)
char	*ttyname;
{
bool	stat;

#ifdef _REENTRANT
        gen_Psem( &CL_misc_sem );
#endif /* _REENTRANT */
 
        stat = (!MEcmp(ttyname, "tty", 3) || !MEcmp(ttyname, "co", 2)
                                         || !MEcmp(ttyname, "cu", 2));
#ifdef _REENTRANT
        gen_Vsem( &CL_misc_sem );
#endif /* _REENTRANT */

	return(stat);
}
