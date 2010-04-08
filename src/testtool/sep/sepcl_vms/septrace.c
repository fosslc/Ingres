/*
    Include files
*/

#include <compat.h>
#include <cm.h>
#include <er.h>
#include <ex.h>
#include <lo.h>
#include <si.h>
#include <st.h>
#include <tc.h>

#define septrace_c

#include <sepdefs.h>
#include <fndefs.h>

/*
** History:
**	08-feb-1993 (donj)
**	    SEP_TCgetc_trace() was only incrementing and decrementing
**	    SEPtcsubs when tracing was set.
**      15-feb-1993 (donj)
**          Add a ME_tag arg to SEP_LOfroms.
**	 4-may-1993 (donj)
**	    NULLed out some char ptrs, Added prototyping of functions.
**	 5-may-1993 (donj)
**	    Added arg3# to STindex() call.
**	 7-may-1993 (donj)
**	    Add more prototyping.
**       7-may-1993 (donj)
**          Fix SEP_MEalloc().
**      19-may-1993 (donj)
**	    Added tracing for CL ME tables. (MEMORY,HASH).
**       2-jun-1993 (donj)
**          Isolated Function definitions in a new header file, fndefs.h. Added
**          definition for module, and include for fndefs.h. Added empty tracing
**	    flag to provoke deletion of the trace file after normal exit.
**	 8-sep-1993 (donj)
**	    Add a flush to the TCget_trace function.
**      15-oct-1993 (donj)
**          Make function prototyping unconditional.
**       5-dec-1993 (donj)
**          Put timeout value in tracing for tcgetc call to help track down
**          bug 57514 (put in the comment but didn't add the code until 17-dec.
**	13-mar-2001	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from testtool code as the use is no longer allowed
*/

/*
** Global variables
*/

GLOBALREF    i4            tracing ;
GLOBALREF    i4            trace_nr ;
GLOBALREF    FILE         *traceptr ;
GLOBALREF    char         *trace_file ;
GLOBALREF    LOCATION      traceloc ;
GLOBALREF    char          SEPpidstr [] ;

/*
    handling of TC connections
*/
GLOBALREF    i4            SEPtcsubs ;

/*
GLOBALREF    STATUS        SEPtclink ;

GLOBALREF    PID           SEPsonpid ;
GLOBALREF    char          SEPpidstr [] ;

GLOBALREF    LOCATION      SEPtcinloc ;
GLOBALREF    LOCATION      SEPtcoutloc ;
GLOBALREF    char          SEPinname [] ;
GLOBALREF    char          SEPoutname [] ;
GLOBALREF    TCFILE       *SEPtcinptr ;
GLOBALREF    TCFILE       *SEPtcoutptr ;
*/

STATUS
SEP_Trace_Open()
{
    STATUS                 ret_val ;

    if (trace_file == NULL)
	trace_file = SEP_MEalloc(SEP_ME_TAG_NODEL, MAX_LOC+1, TRUE, NULL);

    STprintf(trace_file, ERx("tr%s%d.stf"), SEPpidstr, ++trace_nr);
    if ((ret_val = SEP_LOfroms(FILENAME & PATH,trace_file,&traceloc,SEP_ME_TAG_NODEL)) == OK)
	ret_val = SIopen(&traceloc,ERx("w"),&traceptr);

    return (ret_val);
}


STATUS
SEP_Trace_Set(char *argv2)
{
    i4                     arg_len ;
    STATUS                 ret_val ;
    LOCATION               septraceloc ;
    FILE                  *septraceptr = NULL ;
    char                   trace_buffer [81] ;
    char                  *tcptr1 = NULL ;
    char                  *tcptr2 = NULL ;

    ret_val = OK;

    if (argv2 == NULL || *argv2 == EOS)
	tracing |= (TRACE_TOKENS|TRACE_LEX);
    else if (CMcmpnocase(argv2, ERx("@")) == 0)
    {

	if ((ret_val = SEP_LOfroms(FILENAME & PATH, ++argv2, &septraceloc, SEP_ME_TAG_NODEL))
	    == OK)
	    if ((ret_val = SIopen(&septraceloc,ERx("r"),&septraceptr)) == OK)
	    {
		while ((ret_val = SIgetrec(trace_buffer, 80, septraceptr))
		       == OK)
		{
		    tcptr1 = SEP_CMlastchar(trace_buffer,0);
		    *tcptr1 = EOS;
		    if ((ret_val = SEP_Trace_Set(trace_buffer)) != OK)
			break;
		}
		SIclose(&septraceptr);
		if (ret_val == ENDFILE)
		    ret_val = OK;
	    }
    }
    else if (CMcmpnocase(argv2, ERx("(")) == 0)
    {
	tcptr1 = SEP_CMlastchar(++argv2,0);
	if (CMcmpnocase(tcptr1,ERx(")")) == 0)
	    *tcptr1 = EOS;

	ret_val = SEP_Trace_Set(argv2);
    }
    else if ((tcptr1 = STindex(argv2,ERx(","),0)) != NULL)
    {
	tcptr2 = argv2;
	*tcptr1 = EOS;
	if ((ret_val = SEP_Trace_Set(tcptr2)) == OK)
	     ret_val = SEP_Trace_Set(++tcptr1);
    }
    else
    {
	arg_len = STlength(argv2);

	if (STbcompare(ERx("all"),    3, argv2, arg_len, TRUE) == 0)
	    tracing |= (TRACE_TOKENS|TRACE_LEX    |TRACE_PARM  |
                        TRACE_SEPCM |TRACE_GRAMMAR|TRACE_DIALOG|
                        TRACE_ME_TAG|TRACE_ME_HASH|TRACE_NONE   );
	else if (STbcompare(ERx("tokens"), 6, argv2, arg_len, TRUE) == 0)
	    tracing |= TRACE_TOKENS;
	else if (STbcompare(ERx("lex"),    3, argv2, arg_len, TRUE) == 0)
	    tracing |= TRACE_LEX;
	else if (STbcompare(ERx("parm"),   4, argv2, arg_len, TRUE) == 0)
	    tracing |= TRACE_PARM;
	else if (STbcompare(ERx("param"),  5, argv2, arg_len, TRUE) == 0)
	    tracing |= TRACE_PARM;
	else if (STbcompare(ERx("dialog"), 6, argv2, arg_len, TRUE) == 0)
	    tracing |= TRACE_DIALOG;
	else if (STbcompare(ERx("sepcm"),  5, argv2, arg_len, TRUE) == 0)
	    tracing |= TRACE_SEPCM;
	else if (STbcompare(ERx("grammar"),7, argv2, arg_len, TRUE) == 0)
	    tracing |= TRACE_GRAMMAR;
	else if (STbcompare(ERx("memory"), 6, argv2, arg_len, TRUE) == 0)
            tracing |= TRACE_ME_TAG;
        else if (STbcompare(ERx("hash"),   4, argv2, arg_len, TRUE) == 0)
            tracing |= TRACE_ME_HASH;
	else if (STbcompare(ERx("none"),   4, argv2, arg_len, TRUE) == 0)
	    tracing |= TRACE_NONE;
	else
	    ret_val = FAIL;
    }

    return (ret_val);
}


STATUS
SEP_Trace_Clr(char *argv2)
{
    i4                     arg_len ;
    STATUS                 ret_val ;

    ret_val = OK;

    if (argv2 == NULL || *argv2 == EOS)
	tracing = FALSE;
    else
    {
	arg_len = STlength(argv2);

	if (STbcompare(ERx("all"),    3, argv2, arg_len, TRUE) == 0)
	    tracing = FALSE;
	else if (STbcompare(ERx("tokens"), 6, argv2, arg_len, TRUE) == 0)
	    tracing &= ~TRACE_TOKENS;
	else if (STbcompare(ERx("lex"),    3, argv2, arg_len, TRUE) == 0)
	    tracing &= ~TRACE_LEX;
	else if (STbcompare(ERx("parm"),   4, argv2, arg_len, TRUE) == 0)
	    tracing &= ~TRACE_PARM;
	else if (STbcompare(ERx("param"),  5, argv2, arg_len, TRUE) == 0)
	    tracing &= ~TRACE_PARM;
	else if (STbcompare(ERx("dialog"), 6, argv2, arg_len, TRUE) == 0)
	    tracing &= ~TRACE_DIALOG;
	else if (STbcompare(ERx("sepcm"),  5, argv2, arg_len, TRUE) == 0)
	    tracing &= ~TRACE_SEPCM;
	else if (STbcompare(ERx("grammar"),7, argv2, arg_len, TRUE) == 0)
	    tracing &= ~TRACE_GRAMMAR;
        else if (STbcompare(ERx("memory"), 6, argv2, arg_len, TRUE) == 0)
            tracing &= ~TRACE_ME_TAG;
        else if (STbcompare(ERx("hash"),   4, argv2, arg_len, TRUE) == 0)
            tracing &= ~TRACE_ME_HASH;
	else if (STbcompare(ERx("none"),   4, argv2, arg_len, TRUE) == 0)
	    tracing &= ~TRACE_NONE;
	else
	    ret_val = FAIL;
    }

    return (ret_val);
}

/*
**  SEP_TCgetc_trace
*/

i4
SEP_TCgetc_trace(TCFILE *tcoutptr,i4 waiting)
{
    i4                    c ;

    EXinterrupt(EX_OFF);
    c = TCgetc(tcoutptr, waiting);
    EXinterrupt(EX_ON);

    if (tracing&TRACE_DIALOG)
    {
	switch (c)
	{
	   case TC_EOF:
			SIfprintf(traceptr, ERx("\n(TC_EOF%d)\n"), SEPtcsubs--);
			break;
	   case TC_TIMEDOUT:
			SIfprintf(traceptr, ERx("\n(TC_TIMEDOUT%d<%d>)\n"),
				  SEPtcsubs, waiting);
			break;
	   case TC_EOQ:
			SIfprintf(traceptr, ERx("\n(TC_EOQ%d)\n"), SEPtcsubs);
			break;
	   case TC_EQR:
			SIfprintf(traceptr, ERx("\n(TC_EQR%d)\n"), SEPtcsubs);
			break;
	   case TC_BOS:
			SIfprintf(traceptr, ERx("\n(TC_BOS%d)\n"), ++SEPtcsubs);
			break;
	   default:
			SIfprintf(traceptr, "%c",c);
			break;
	}
	SIflush(traceptr);
    }
    else if (c == TC_EOF) SEPtcsubs--;
    else if (c == TC_BOS) SEPtcsubs++;

    return (c);
}
