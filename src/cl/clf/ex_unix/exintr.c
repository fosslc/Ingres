# include	<compat.h>
# include	<gl.h>
# include	<systypes.h>
# include	<clconfig.h>
# include	<clsigs.h>
# include	<ex.h>
# include	"exi.h"


/*
**Copyright (c) 2004 Ingres Corporation
**
** EXinterrupt() -- Disable, enable or reset exception handling.
**	
**	History:
**	
**		Revision 1.2  86/04/25  16:58:58  daveb
**		Add casts for 4.2 version to pass lint.
**		
**		Revision 1.2  86/02/25  21:14:20  perform
**		intr_count to EXintr_count.  EXchkintr() now a macro.
**		Use IN_in_backend rather than cover routine INinbackend().
** 
**		Revision 1.2  85/06/20  12:43:07  daveb
**		Add missing 'cases' for sys V
**		
**		Revision 3.0  85/05/13  17:09:24  wong
**		Merged frontend and backend versions of 'EXinterrupt()' by
**		using internal CL routine 'INin_backend()' to distinguish
**		between the frontend and backend functionality.
**		
**	5-Mar-85 (lin)
**	6-Jan-1989 (daveb)
**		Add EX_DELIVER to allow polling for interrupts without
**		a window of vulnerability to signals between and EX_ON and
**		an EX_OFF.
**	16-may-90 (blaise)
**	    Integrated changes from 61 and ug:
**		Remove dead variables i_EXigint and i_EXighup.
**	25-mar-91 (kirke)
**	    Added #include <systypes.h> because HP-UX's signal.h includes
**	    sys/types.h.  Substituted clsigs.h for signal.h as per standard.
**	26-apr-1993 (fredv)
**	    Moved <clconfig.h> and <clsigs.h> before <ex.h> to avoid
**		redefine of EXCONTINUE in the system header sys/context.h
**		of AIX.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	16-jan-1996 (toumi01; from 1.1 axp_osf port)
**	    (schte01) added 02-sep-93 (kchin)
**	    Cast (long) 3rd... EXsignal args to avoid truncation.
*/

int 	i_EXcatch();

/* Zero means deliver immediately, nz means log and deliver later */
GLOBALDEF i4	EXintr_count ZERO_FILL;

/* Use these to count SIGINTS and SIGQUITS while interrupts are masked. */
GLOBALDEF i4	EXsigints ZERO_FILL;
GLOBALDEF i4	EXsigquits ZERO_FILL;

/*
** EXinterrupt
**
**	Change interrupt status.  This routine is used to mask and unmask
**	keyboard interrupts.
*/

VOID
EXinterrupt(new_state)
i4	new_state;
{
	switch (new_state)
	{
	case EX_OFF:
		EXintr_count++;
		break;

	case EX_ON:
		if ( --EXintr_count > 0 )
			break;
		/* else fall through ... */

	case EX_RESET:
		EXintr_count = 0;
		/* keep falling into... */
		
	case EX_DELIVER:
		if( EXintr_count > 1 )
			break;
			
		if ( EXsigints > 0 )
		{
			EXsigints = 0;
			EXsignal( EXINTR, 1, (long)SIGINT );
		}
		if ( EXsigquits > 0 )
		{
			EXsigquits = 0;
			EXsignal( EXQUIT, 1, (long)SIGQUIT );
		}
		break;
	}		/* end switch( new_state ) */
}
