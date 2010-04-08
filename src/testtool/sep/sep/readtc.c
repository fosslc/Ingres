/*
**    Include file
*/

#include <compat.h>
#include <lo.h>
#include <er.h>
#include <si.h>
#include <tc.h>

#define readtc_c

#include <sepdefs.h>
#include <sepfiles.h>
#include <fndefs.h>

/*
** History:
**	20-apr-1994 (donj)
**	    Created
**	31-may-1994 (donj)
**	    Took out #include for sepparam.h. QA_C was complaining about all
**	    the unecessary GLOBALREF's.
**	22-jun-1994 (donj)
**	    Added a timeout_try_again function to send an TC_EOQ then try a
**	    TCgetc() again if we get a timeout.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      05-Sep-2000 (hanch04)
**          replace nat and longnat with i4
*/

/*
**  defines
*/

#define TC_RETRY_TIMEOUT  10

/*
**  Reference global variables
*/

GLOBALREF    i4            tracing ;
GLOBALREF    i4            SEPtcsubs ;

GLOBALREF    FILE         *traceptr ;

GLOBALREF    SEPFILE      *sepResults ;

GLOBALREF    TCFILE       *SEPtcinptr ;

#define simple_trace_if(sw,str) \
 if (tracing&sw) { SIfprintf(traceptr,ERx(str)); SIflush(traceptr); }


/*
**  Read_TCresults
*/

i4
Read_TCresults(	TCFILE *tcoutptr,TCFILE *tcinptr,i4 waiting,bool *fromBOS,
		i4 *sendEOQ_ptr,bool waitforit,STATUS *tcerr)
{
    i4                     c = 0 ;
    i4                     lastc = 0 ;
    i4                     last_TC = 0 ;
    i4                     tmp_waiting = waiting ;
    bool                   keepgoing = TRUE ;
    bool                   nothing_yet = TRUE ;
    bool                   timeout_try_again = TRUE ;


    simple_trace_if(TRACE_DIALOG,"Read_TCresults> dialog fm fe (recording) >\n");

    for (*tcerr = OK; (keepgoing && (*tcerr == OK)); lastc = c)
    {
	if (!waitforit)
	    if (TCempty(tcoutptr))
	    {
		keepgoing = FALSE;
		continue;
	    }

	c = SEP_TCgetc_trace(tcoutptr, tmp_waiting);
	tmp_waiting = waiting; /* reset to timeout value */

	if (((c == TC_EQR)||(c == TC_EOF)) &&
	    (lastc != '\n'))
	{
	    simple_trace_if(TRACE_DIALOG,"\n");
	}

	switch (c)
	{
	   case TC_EQR:
			keepgoing = nothing_yet;

			tmp_waiting = (keepgoing&&(waiting > TC_RETRY_TIMEOUT))
				    ? TC_RETRY_TIMEOUT : waiting;

			last_TC = c;
			break;

	   case TC_EOF:
			keepgoing = (SEPtcsubs == 0) ? FALSE : keepgoing;
			*fromBOS  = FALSE;

			if (SEPtcsubs <= 0)
			{
			    *tcerr = INGRES_FAIL;
			}
			else if (*sendEOQ_ptr == SEPtcsubs)
			{
			    endOfQuery(SEPtcinptr,ERx("Read_TC1"));
			    *sendEOQ_ptr = 0;
			}

			last_TC = c;
			break;

	   case TC_TIMEDOUT:

			if (nothing_yet && last_TC == TC_EQR)
			{
			    c = last_TC;
			    keepgoing = FALSE;
			}
			else if (timeout_try_again)
			{
			    keepgoing = TRUE;
			    endOfQuery(SEPtcinptr,ERx("Read_TC2"));
			    timeout_try_again = FALSE;
			}
			else
			{
			    last_TC = c;
			    keepgoing = FALSE;
			}

			break;

	   case TC_EOQ:
			last_TC = c;
			break;

	   case TC_BOS:
			endOfQuery(tcinptr,ERx("Read_TC2"));
			*fromBOS = TRUE;
			last_TC = c;
			break;

	   default:
			nothing_yet = FALSE;
			SEPputc(c, sepResults);
			break;
	}
    }

    SEPputc(SEPENDFILE, sepResults);
    SEPrewind(sepResults, TRUE);
    simple_trace_if(TRACE_DIALOG,"Read_TCresults> End.>\n");
    return (c);
}
