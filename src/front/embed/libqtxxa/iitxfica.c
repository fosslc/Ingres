/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include       <gl.h>
# include       <iicommon.h>
# include       <iicx.h>
# include       <iicxxa.h>
# include       <iitxapi.h>

/*
**  Name: iitxicfl.c    Flag routine, shipped as an object and linked only
** 			into TUXEDO/INGRES ICAS servers.
**
**  Description:
**      Of the three entry points in here, only the IItux_is_icas
**	function will return a TRUE
**	
**  Defines:
**
**	IItux_is_tms()
**	IItux_is_as()
**	IItux_is_icas()
**
**
**  Notes:
**	<Specific notes and examples>
-*
**  History:
**	11-Aug-1993	(mhughes)
**	    First Implementation
*/

/*
**   Name: IItux_is_tms()
**
**   Description: This is linked only into TUXEDO ICAS executables and
**		  just returns FALSE to say that, no this isn't a TMS
**		  process.
**
**   Inputs:
**       None.
**
**   Outputs:
**       Returns: FALSE (always).
**
**   History:
**       11-Aug-1993	(mhughes)
**	    First implementation.
*/

bool
IItux_is_tms()
{
    return( FALSE );
}


/*
**   Name: IItux_is_as()
**
**   Description: This is linked only into TUXEDO ICAS executables and
**		  just returns TRUE to say that, no this isn't an ICAS
**		  process.
**
**   Inputs:
**       None.
**
**   Outputs:
**       Returns: FALSE (always).
**
**   History:
**       11-Aug-1993	(mhughes)
**	    First implementation.
*/

bool
IItux_is_as()
{
    return( FALSE );
}


/*
**   Name: IItux_is_icas()
**
**   Description: This is linked only into TUXEDO ICAS executables and
**		  just returns FALSE to say that, yes this is an ICAS
**		  process.
**
**   Inputs:
**       None.
**
**   Outputs:
**       Returns: TRUE (always).
**
**   History:
**       11-Aug-1993	(mhughes)
**	    First implementation.
*/

bool
IItux_is_icas()
{
    return( TRUE );
}
