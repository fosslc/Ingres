/*
** Copyright (c) 2004 Ingres Corporation
*/

# include 	<compat.h>
# include 	<er.h>
# include	<si.h>
# include	<st.h>
# include	<equel.h>
# include	<equtils.h>
# include	<eqesql.h>
# include	<ereq.h>

/*
+* Filename:	esqlca.c
** Purpose:	Defines utilities for managing the SQLCA structure at preprocess
**		time.
** Defines:
**		esqlca		- Utility to save WHENEVER state.
**		esqlcaDump	- Dump esqlca_store.
-*		esqlca_store[]	- Array of five WHENEVER states.
** 
** History:
**	17-jul-1985	- Written (ncg)
**	23-apr-1986	- Modified for native SQL (ncg)
**	03-may-1988	- Added MESSAGE condition (whc)
**	14-dec-1989	- Added EVENT condition. (bjb)
**	20-apr-1991	- Change default of MESSAGE to CONTINUE. (kathryn)
**	19-jul-1991	- Change EVENT to DBEVENT. (teresal)
**      12-Dec-95 (fanra01)
**          Added definitions for extracting data for DLLs on windows NT
**      15-mar-96 (thoda04)
**          Cleaned up warning that esqlcaDump() did not return anything
**	08-oct-96 (mcgem01)
**	    Global data moved to eqdata.c
**
** The following is an example of internal calls generated when the three
** WHENEVER statements are parsed:
**
** 		EXEC SQL WHENEVER NOT FOUND GOTO lab1;
** 		EXEC SQL WHENEVER SQLWARNING CONTINUE;
** 		EXEC SQL WHENEVER SQLERROR CALL proc1;
** 		EXEC SQL WHENEVER SQLMESSAGE CALL SQLPRINT;
**		EXEC SQL WHENEVER DBEVENT CALL SQLPRINT;
**
** Internally Generates:
**		esqlca( sqcaNOTFOUND, sqcaGOTO, "lab1" );
**		esqlca( sqcaWARNING, sqcaCONTINUE, NULL );
**		esqlca( sqcaERROR, sqcaCALL, "proc1" );
**		esqlca( sqcaMESSAGE, sqcaCALL, "sqlprint" );
**		esqlca( sqcaEVENT, sqcaCALL, "sqlprint" );
**
** Which will store in the 5 element array:
**	esqlca_store[0] : { sqcaGOTO, "lab1" }		- Not Found
**	esqlca_store[1] : { sqcaCALL, "proc1" }		- Error
**	esqlca_store[2] : { sqcaCONTINUE, NULL }	- Warning
**	esqlca_store[3] : { sqcaCALL, "sqlprint" }	- Message
**	esqlca_store[4] : { sqcaCALL, "sqlprint" }	- DBevent
**
** Which will generate to output when an SQL statement is parsed:
**	
**	if (NOT FOUND)
**		goto lab1;
**	else if (ERROR)
**		proc1();
**	else if (DBEVENT)
**		IILQesEvStat(1,0);	-- Consume the database event
**		IIsqPrint(&sqlca);
**
** 1)   Code is generated differently for EXECUTE PROCEDURE statements: NOT
**	FOUND is ignored, GOTO adds a break.  Also, MESSAGE is ignored
**	everywhere except for EXECUTE PROCEDURE.  The default SQLMESSAGE
**	action is SQLPRINT.
** 2)	DBEVENT is ignored on a GET DBEVENT statement.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/* Store action on conditions - NOTFOUND, ERROR, WARNING, MESSAGE, DBEVENT */
GLOBALREF SQLCA_ELM esqlca_store[];

/*
+* Procedure:	esqlca
** Purpose:	Save data for WHENEVER flags for SQLCA.
** Parameters:
**	what	- i4	- Why are we being called:
**			  NOT_FOUND, WARNING, ERROR, MESSAGE, DBEVENT.
**	action	- i4	- What action to take on a WHENEVER statement:
**			  GOTO, CALL, CONTINUE and STOP.
**	label	- char *- If GOTO or CALL then label or function name.
** Return Values:
-*	None
*/

void
esqlca( what, action, label )
register i4	what;
i4	action;
char	*label;
{
    i4  i;

    switch (what)
    {
      case sqcaINIT:
	for (i = 0; i < sqcaDEFAULT; i++)
	    esqlca_store[i].sq_action = sqcaCONTINUE;
	return;
	
      case sqcaNOTFOUND:
      case sqcaERROR:
      case sqcaWARNING:
      case sqcaMESSAGE:
      case sqcaEVENT:
        /* Use as an index to save values from WHENEVER */
	esqlca_store[what].sq_action = action;
	if (label)
	    label = STalloc(label); /* save the label */
	esqlca_store[what].sq_label = label;
	return;

      case sqcaDEFAULT:		/* Default case means internal error */
      default:
	return;
    }
}

/*
+* Procedure:	esqlcaDump
** Purpose:	Dump esqlca_store[].
** Parameters:
**	None
** Return Values:
-*	None
*/

i4
esqlcaDump()
{
    register	SQLCA_ELM	*sq;
    register	i4		i;
    register	FILE	      	*df = eq->eq_dumpfile;
    static	char		*wnames[] = { ERx("NotFound"),
					      ERx("SqlError"),
					      ERx("SqlWarning"),
					      ERx("SqlMessage"),
					      ERx("Dbevent") };
    static	char		*waction[] = { ERx("Continue"),
					       ERx("Stop"),
					       ERx("Goto"),
					       ERx("Call") };
    SIfprintf( df, ERx("SQLCA Structure Dump:\n") );
    SIfprintf( df, ERx("  - Condition  - Action -\n") );
    for (i = 0, sq = esqlca_store; i < sqcaDEFAULT; sq++, i++)
    {
	SIfprintf( df, ERx("  [%10s] : %8s  %s\n"), wnames[i], 
		   waction[sq->sq_action], 
		   sq->sq_label ? sq->sq_label : ERx("") );
    }
    SIflush( df );
    return(0);
}
