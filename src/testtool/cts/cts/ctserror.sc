/*
** ctserror.sc: Error handling procedure for MTS and CTS
**
** Called by the CTS/MTS routines whenever an INGRES error
** is detected. Checks for non-serious error messages like
** deadlock and "table start not found", and just returns if
** it finds them. (Those are the messages that cause unnecessary 
** diffs because they appear an unpredictable number of times.)
** Other messages are displayed before returning. 
**
** 14-may-1991 (lauraw)
**	Added (from Stever's version of MTS)
** 16-may-1991 (lauraw)
**	Took out set_ingres/errortype. Changed to test inquire_ingres
**	result, not sqlca.sqlcode because of generic/dbms error
**	ambiguites. Changed test of ERR_SELECT
**	to test for specific "table 'start' not found", because we
**	DO want to see messages for other tables not found. Added
**	test for "data3_tmp not found". 
** 27-Aug-1993 (fredv)
**	Included <st.h>.
** 02-Mar-1995 (canor01)
**	Added checks for other forms of deadlock that may be reported
*/

# include	<compat.h>
# include	<si.h>
# include	<st.h>

exec sql begin declare section;

# define	ERR_DEADLOCK	4700	
# define	ERR_LOGFULL 	4706	
# define 	ERR_SELECT	2117

# define	SQL_DEADLOCK	-49900

# define	IGNORE_CASE	1

exec sql end declare section;

extern		i4 traceflag;      /*flag to set trace on/off */
extern		int	ing_err;

CTS_errors()
{
	exec sql begin declare section;
		char	errmsg[256];
		int	ingcode;
	exec sql end declare section;

	exec sql include sqlca;

	/* Get text of error msg */

	exec sql inquire_ingres (:errmsg = ERRORTEXT, :ingcode = DBMSERROR);

	/* If tracing, print any error message */

	if (traceflag)
	{
		SIprintf("ing_err: %d\n", ing_err);
		SIprintf("errmsg: %s\n", errmsg);
		SIprintf("sqlca.sqlcode: %d\n", sqlca.sqlcode);
		SIprintf("ingcode: %d\n", ingcode);
		SIflush(stdout);
	}

	/* RDF deadlock (196674) or other deadlock? */
	if ( ingcode == 196674 || sqlca.sqlcode == SQL_DEADLOCK )
		ingcode = ERR_DEADLOCK;

	/* If it's a deadlock or log full error, the slave code will handle it, 
	** so just return ...
	*/

	if ((ingcode == ERR_DEADLOCK) || (ingcode == ERR_LOGFULL))
		return;

	/* Return if it's an error selecting on the "start" table.
	** (Sometimes it takes several tries before the slaves can
	** successfully find this table. The unpredictable number
	** of "table 'start' not found" messages would cause diffs.)
	*/ 

	else if (STbcompare("E_US0845 Table 'start' does not exist or is not owned by you",60,errmsg,60,IGNORE_CASE) == 0)
		return;

	/* Some tests (not sure which) try to drop tables they don't
	** own...
	*/

	else if (STbcompare("E_US0AC1 DROP: 'data3_tmp' does not exist or is not owned by you",60,errmsg,60,IGNORE_CASE) == 0)
		return;

	/* If we're not gone by now, print the error message and leave */

	else
	{
                SIprintf("%s\n", errmsg);
                SIflush(stdout);
		return;
	}
}
