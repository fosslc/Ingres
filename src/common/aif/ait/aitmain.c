/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<pc.h>
# include	<si.h>
# include	<iiapi.h>
# include	"aitfunc.h"
# include	"aitdef.h"
# include	"aitproc.h"

/*
** Name: aitmain.c - API tester
**
** Description:
**	This file contains the main routine to run API tester.
**
**	main() 	the main routine of API tester
**
** Ming hints
PROGRAM	= apitest
NEEDLIBS= APILIB GCFLIB ADFLIB CUFLIB COMPAT
**
** History:
**	21-Feb-95 (feige)
**	    Created.
**	27-Mar-95 (gordy)
**	    Extracted common processing so that a winmain() 
**	    function can be created for the Windows port.
**	11-Apr-95 (gordy)
**	    Declare SIprintf() just in case si.h fails to do so.
**	19-May-95 (gordy)
**	    Fixed include statements.
**	22-Dec-1997 (kosma01)
**	    No need to declare SIprintf() as now si.h does.
**	14-Mar-03 (gordy)
**	    Added Ming hints.  Cast SIprintf to quite compiler warning.
*/





/*
** Name: main() - the main routine of API tester
**
** Description:
**	This function is the main routine to run API tester. 
**
** Inputs:
**	argc		Number of command line parameters
**	argv		Array of command line parameter strings
**
** Outputs:
**	None.
** 
** Returns:
**	int		Status code
**
** History:
**	21-Feb-95 (feige)
**	    Created.
**	27-Mar-95 (gordy)
**	    Extracted common processing so that a winmain() 
**	    function can be created for the Windows port.
**	11-Apr-95 (gordy)
**	    Added message function to AITinit() call.
*/

int
main( int argc, char **argv )
{
    AITCB	*aitcb;

    if ( ( aitcb = AITinit( argc, argv, (void (*)())SIprintf ) ) )
    {
	/*
	** Process until done.
	*/
	while( AITprocess( aitcb ) ) 
	{
	    /*
	    ** We don't have anything else
	    ** to do while processing.
	    */
	}

	AITterm( aitcb );
    }

    PCexit( 0 );	/* Should not return... */
    return( 0 );	/* ...but just in case */
}
