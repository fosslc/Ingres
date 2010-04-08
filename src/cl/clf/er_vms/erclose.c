/*
**    Copyright (c) 1986, 2000 Ingres Corporation
*/


#include	<compat.h>
#include	<gl.h>
#include	<me.h>
#include	<er.h>
#ifndef	VMS
#include	<si.h>	    /* Needed for "erloc.h", if not VMS */
#endif
#include	<cs.h>	    /* Needed for "erloc.h" */
#include	"erloc.h"

/* Local define */

#define	ALLCLASS    (i4)-1	

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
**      16-jul-93 (ed)
**	    added gl.h
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	07-feb-01 (kinte01)
**	    Add casts to quiet compiler warnings
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
