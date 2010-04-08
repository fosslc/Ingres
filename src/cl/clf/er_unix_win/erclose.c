/*
**Copyright (c) 2004 Ingres Corporation
*/


#include	<compat.h>
#include	<gl.h>
#include	<me.h>
#include	<er.h>
#include	<cs.h>
#include	"erloc.h"

/* Local define */

#define	ALLCLASS    (i4)-1	

/* Global references */

/*{
** Name: ERclose - Close message files (fast and slow).
**
** Description:
**	Close fast and slow message files and release all memory for fast
**	message and return status.
**	
** Inputs:
**	None.
**
** Outputs:
**	None.
**
** Returns:
**	OK			If everthing worked.
**	ER_NOFREE
**	ER_BADCLASS
**	FAIL
**
** Exceptions:
**
** Side Effects:
**	None.
**
** History:
**	01-oct-1986 - (kobayashi)
**	Create new for 5.0.
**      27-sep-1990  (jkb)
**	include cs.h for reetrancy stuff
**	06-feb-1993 (smc)
**	Cast MEfree parm to PTR to clear cc warnings.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	01-jun-1995 (canor01)
**	    semaphore protect MEfree calls in server
**	03-jun-1996 (canor01)
**	    With new ME for operating-system threads, semaphore protection
**	    no longer needed.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
STATUS
ERclose()
{
	ERMULTI		*m;
	i4		st= OK;
	STATUS status;

	/* Release memory for all class */

	if ((status = ERrelease(ALLCLASS)) != OK)
	{
		st = status;
	}

	/* close all run time files both fast and slow */

	for (m = &ERmulti[0]; m < &ERmulti[ER_MAXLANGUAGE]; m++)
	{
		if (m->language != 0)
		{
			/* Release class table to use again. */
			if (m->class != 0)
			{
			    if (MEfree((PTR)m->class) != OK)
			    {
				st = ER_NOFREE;
    			    }
			    m->class = 0;
			    m->number_class = 0;
			}

			/* close fast run time file */

			st = cer_close(&(m->ERfile[ER_FASTSIDE]));

			/* close fast run time file */

			st = cer_close(&(m->ERfile[ER_SLOWSIDE]));
		}
		m->language = 0;
		m->deflang = 0;
	}
	return(st);
}
