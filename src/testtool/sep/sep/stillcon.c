/*
**    Include file
*/

#include <compat.h>
#include <si.h>
#include <st.h>
#include <lo.h>
#include <er.h>

#define stillcon_c

#include <sepdefs.h>
#include <sepfiles.h>
#include <fndefs.h>

/*
** History:
**	26-apr-1994 (donj)
**	    Created
**	31-may-1994 (donj)
**	    Took out #include for sepparam.h. QA_C was complaining about all
**	    the unecessary GLOBALREF's.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
**  defines
*/

/*
**  Reference global variables
*/

GLOBALREF    char          buffer_1 [] ;
GLOBALREF    char         *Con_with ;

GLOBALREF    i4            tracing ;
GLOBALREF    i4            testLevel ;

GLOBALREF    FILE         *traceptr ;

GLOBALREF    bool          SEPdisconnect ;

STATUS
Still_want_have_connection( char *cmd, STATUS *ierr )
{
    STATUS                 serr  = OK ;
    char                  *Still = ERx("Still_want_have_connection>") ;

    /*
    **  Check if Ingres is alive or user wants to disconnect or tolerance level
    **  was reached.
    */

    if ((*ierr == INGRES_FAIL) || SEPdisconnect || (testLevel <= 0))
    {
	if (SEPdisconnect)
	{
	    STpolycat(3,Con_with,cmd,ERx(" terminated by user"),buffer_1);
	}
	else
	{
	    STpolycat(3,Con_with,cmd,ERx(" was completed"),buffer_1);
	}

	*ierr = (*ierr == INGRES_FAIL) ? OK : FAIL;

        if (tracing&TRACE_DIALOG)
	{
            SIfprintf(traceptr,ERx("%s (%d) %s\n"),Still,*ierr,buffer_1);
	    SIflush(traceptr);
	}

	serr = FAIL;
    }
    else if (tracing&TRACE_DIALOG)
    {
        SIfprintf(traceptr,ERx("%s (%d) No Disconnect\n"),Still,*ierr);
	SIflush(traceptr);
    }

    return (serr);
}
