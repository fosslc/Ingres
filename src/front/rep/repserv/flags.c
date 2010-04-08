/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <cm.h>
# include <st.h>
# include <cv.h>
# include <lo.h>
# include <si.h>
# include <er.h>
# include <gl.h>
# include <iicommon.h>
# include "rsconst.h"
# include "repserv.h"
# include "conn.h"
# include "repevent.h"
# include "recover.h"

/**
** Name:	flags.c - Replicator Server flags (options)
**
** Description:
**	Defines
**		set_flags	- set flags
**		file_flags	- set flags from file
**		com_flags	- set flags from command line
**		check_flags	- check startup flags
**
** History:
**	03-jan-97 (joea)
**		Created based on setflags.c in replicator library.
**	09-may-97 (joea)
**		Return immediately if flag length is less than 4.
**	25-nov-97 (joea)
**		Rename read_q flags to avoid name problems on MVS.
**	03-feb-98 (joea)
**		Rename RSdb_event_timeout to RSdbevent_timeout.
**	21-apr-98 (joea)
**		Eliminate SES/NSE flags. Replace RSlocaldb, RSlocalowner,
**		RSdb_open by RSlocal_conn.
**	22-jul-98 (abbjo03)
**		Move GLOBALDEFs of RSqep and RSqbm/RSqbt here so that they won't
**		interfere with the DistQ API.
**	23-aug-99 (abbjo03)
**		Remove QEP flag which is no longer used.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      23-Aug-2005 (inifa01)INGREP174/B114410
**          The RepServer takes shared locks on base, shadow and archive tables
**          when Replicating FP -> URO.  causing deadlock and lock waits.
**          - The resolution to this problem adds flag -NLR and opens a separate
**          session running with readlock=nolock if this flag is set.
**	    Added If flag -NLR set RSnolock_read.
**/

# define MAX_QBM_QBT		999999


GLOBALDEF i4	RSqbm_read_must_break = READ_Q_MUST_BREAK_DFLT;
GLOBALDEF i4	RSqbt_read_try_to_break = READ_Q_TRY_BREAK_DFLT;
GLOBALDEF bool  RSnolock_read = FALSE;

/*{
** Name:	set_flags - set flags
**
** Description:
**	Takes flag passed in (from file or command line) and sets
**	appropriate global variable(s) to the appropriate value(s).
**
** Inputs:
**	flag	- string containing a flag and optionally a value
**
** Outputs:
**	none
**
** Returns:
**	none
*/
void
set_flags(
char	*flag)
{
	i4	i;
	i4	flag_len;
	i4	flag_numeric;
	char	*p;

	STtrmwhite(flag);
	flag_len = STlength(flag);
	if (flag_len < 4)	/* all flags are 4 or more */
	{
		/* Unknown flag %s */
		messageit(3, 31, flag);
		return;
	}

	for (i = 0, p = flag; *p != EOS && i < 4; CMnext(p), i++)
		;
	i = p - flag;
	CVal(&flag[i], &flag_numeric);

	if (STbcompare(flag, flag_len, ERx("-LGL"), 4, 1) == 0)
	{
		RSlog_level = flag_numeric;
		if (RSlog_level < 0 || RSlog_level > 5)
			RSlog_level = 3;
		/* -LGL!  Log level is %d */
		messageit(2, 4, RSlog_level);
		return;
	}
	if (STbcompare(flag, flag_len, ERx("-PTL"), 4, 1) == 0)
	{
		RSprint_level = flag_numeric;
		if (RSprint_level < 0 || RSprint_level > 5)
			RSprint_level = 3;
		/* -PTL!  Print level is %d */
		messageit(2, 5, RSprint_level);
		return;
	}
	if (STbcompare(flag, flag_len, ERx("-SVR"), 4, 1) == 0)
	{
		if (flag_numeric > 0)
			RSserver_no = (i2)flag_numeric;
		/* -SVR!  Server number is %d */
		messageit(2, 6, (i4)RSserver_no);
		if (RSserver_no < 1 || RSserver_no > 999)
		{
			messageit(1, 8);
			RSshutdown(FAIL);
		}
		return;
	}
	/* note: this server flag should be changed to a server action */
	if (STbcompare(flag, flag_len, ERx("-CLQ"), 4, 1) == 0)
	{
		if (RSlocal_conn.status == CONN_OPEN)
		{
			/* -CLQ!  Unquiet all databases and cdds's */
			messageit(2, 7);

			/* unquiet all targets and cdds's */
			unquiet_all(USER_QUIET);
		}
		return;
	}
	if (STbcompare(flag, flag_len, ERx("-EMX"), 4, 1) == 0)
	{
		RSerr_max = (i4)flag_numeric;
		if (RSerr_max < 0)
			RSerr_max = 100;
		/* -EMX!  Maximum error count set to %d */
		messageit(2, 9, RSerr_max);
		return;
	}
	if (STbcompare(flag, flag_len, ERx("-NLR"), 4, 1) == 0)
	{
		RSnolock_read = TRUE;
		/* -NLR  use readlock=nolock on local read of shadow/archive */
		messageit(2, 115);
		return;
	}
	if (STbcompare(flag, flag_len, ERx("-IDB"), 4, 1) == 0)
	{
		char	db[DB_MAXNAME*2+3];
		char	*p;

		STlcopy(&flag[i], db, sizeof(db) - 1);
		STtrmwhite(db);
		/* -IDB!  Local database set to %s */
		messageit(2, 15, db);

		p = STindex(db, ERx(":"), 0);
		if (p == NULL)	/* no :: */
		{
			*RSlocal_conn.vnode_name = EOS;
			STlcopy(db, RSlocal_conn.db_name, DB_MAXNAME);
		}
		else
		{
			*p = EOS;
			STlcopy(db, RSlocal_conn.vnode_name, DB_MAXNAME);
			CMnext(p);
			CMnext(p);
			STlcopy(p, RSlocal_conn.db_name, DB_MAXNAME);
		}
		return;
	}
	if (STbcompare(flag, flag_len, ERx("-OWN"), 4, 1) == 0)
	{
		STlcopy(&flag[i], RSlocal_conn.owner_name, DB_MAXNAME);
		/* -OWN!  Local database owner set to %s */
		messageit(2, 16, RSlocal_conn.owner_name);
		return;
	}
	if (STbcompare(flag, flag_len, ERx("-MLE"), 4, 1) == 0)
	{
		RSerr_mail = TRUE;
		/* -MLE!  Send mail messages on error */
		messageit(2, 17);
		return;
	}
	if (STbcompare(flag, flag_len, ERx("-NML"), 4, 1) == 0)
	{
		RSerr_mail = FALSE;
		/* -NML!  Do not send mail messages on error */
		messageit(2, 18);
		return;
	}
	if (STbcompare(flag, flag_len, ERx("-QIT"), 4, 1) == 0)
	{
		RSquiet = TRUE;
		/* -QIT!  Server mode set to quiet */
		messageit(2, 19);
		return;
	}
	if (STbcompare(flag, flag_len, ERx("-NQT"), 4, 1) == 0)
	{
		RSquiet = FALSE;
		/* -NQT!  Server mode set to active (non-quiet) */
		messageit(2, 20);
		return;
	}
	if (STbcompare(flag, flag_len, ERx("-MON"), 4, 1) == 0)
	{
		RSmonitor = TRUE;
		/* -MON!  Monitor events now being broadcast */
		messageit(2, 27);
		return;
	}
	if (STbcompare(flag, flag_len, ERx("-NMO"), 4, 1) == 0)
	{
		RSmonitor = FALSE;
		/* -NMN!  Monitor events silent */
		messageit(2, 28);
		return;
	}
	if (STbcompare(flag, flag_len, ERx("-QBT"), 4, 1) == 0)
	{
		RSqbt_read_try_to_break = (i4)flag_numeric;
		if (RSqbt_read_try_to_break < 0 ||
				RSqbt_read_try_to_break > MAX_QBM_QBT)
			RSqbt_read_try_to_break = READ_Q_TRY_BREAK_DFLT;
		/* -QBT!  Read_q try to break after %d */
		messageit(2, 40, RSqbt_read_try_to_break);
		return;
	}
	if (STbcompare(flag, flag_len, ERx("-QBM"), 4, 1) == 0)
	{
		RSqbm_read_must_break = (i4)flag_numeric;
		if (RSqbm_read_must_break < 0 ||
				RSqbm_read_must_break > MAX_QBM_QBT)
			RSqbm_read_must_break = READ_Q_MUST_BREAK_DFLT;
		/* -QBM!  Read_q must break after %d */
		messageit(2, 41, RSqbm_read_must_break);
		return;
	}
	if (STbcompare(flag, flag_len, ERx("-SGL"), 4, 1) == 0)
	{
		RSsingle_pass = TRUE;
		/* -SGL!  Single pass flag set*/
		messageit(2, 42);
		return;
	}
	if (STbcompare(flag, flag_len, ERx("-TOT"), 4, 1) == 0)
	{
		RStime_out = (i4)flag_numeric;
		if (RStime_out < 0 || RStime_out > 999)
			RStime_out = QRY_TIMEOUT_DFLT;
		/* -TOT!  Time out set to %d */
		messageit(2, 61, RStime_out);
		return;
	}
	if (STbcompare(flag, flag_len, ERx("-ORT"), 4, 1) == 0)
	{
		RSopen_retry = (i4)flag_numeric;
		if (RSopen_retry < 0)
			RSopen_retry = 0;
		/* -ORT!  Open retry is %d */
		messageit(2, 62, RSopen_retry);
		return;
	}
	if (STbcompare(flag, flag_len, ERx("-CTO"), 4, 1) == 0)
	{
		RSconn_timeout = (i4)flag_numeric;
		if (RSconn_timeout < 0)
			RSconn_timeout = CONN_NOTIMEOUT;
		/* -CTO!  Connection timeout is %d */
		messageit(2, 85, RSconn_timeout);
		return;
	}
	if (STbcompare(flag, flag_len, ERx("-SCR"), 4, 1) == 0)
	{
		RSskip_check_rules = TRUE;
		/* -SCR!  Rule check is diabled */
		messageit(2, 76);
		return;
	}
	if (STbcompare(flag, flag_len, ERx("-NSR"), 4, 1) == 0)
	{
		RSskip_check_rules = FALSE;
		/* -NSR!  Rule checking is enabled */
		messageit(2, 77);
		return;
	}
	if (STbcompare(flag, flag_len, ERx("-EVT"), 4, 1) == 0)
	{
		RSdbevent_timeout = (i4)flag_numeric;
		if (RSdbevent_timeout < 0 || RSdbevent_timeout > 9999)
			RSdbevent_timeout = 0;
		/* -EVT!  Dbevent timeout is %d */
		messageit(2, 63, RSdbevent_timeout);
		return;
	}
	if (STbcompare(flag, flag_len, ERx("-TPC"), 4, 1) == 0)
	{
		RStwo_phase = (flag_numeric ? TRUE : FALSE);
		/* -TPC!  Two Phase Commit is %d */
		messageit(2, 84, (i4)RStwo_phase);
		return;
	}
	if (STbcompare(flag, flag_len, ERx("-QDB"), 4, 1) == 0)
	{
		if (RSlocal_conn.status == CONN_OPEN)
		{
			/* -QDB!  Quiet database %d */
			messageit(2, 87, flag_numeric);

			/* quiet specified target database */
			quiet_db((i2)flag_numeric, USER_QUIET);
		}
		return;
	}
	if (STbcompare(flag, flag_len, ERx("-UDB"), 4, 1) == 0)
	{
		if (RSlocal_conn.status == CONN_OPEN)
		{
			/* -UDB!  Unquiet database %d */
			messageit(2, 88, flag_numeric);

			/* unquiet specified target database */
			unquiet_db((i2)flag_numeric, USER_QUIET);
		}
		return;
	}
	if (STbcompare(flag, flag_len, ERx("-QCD"), 4, 1) == 0)
	{
		if (RSlocal_conn.status == CONN_OPEN)
		{
			/* -QCD!  Quiet CDDS %d */
			messageit(2, 89, flag_numeric);

			/* quiet specified cdds */
			quiet_cdds((i2 *)NULL, (i2)flag_numeric, USER_QUIET);
		}
		return;
	}
	if (STbcompare(flag, flag_len, ERx("-UCD"), 4, 1) == 0)
	{
		if (RSlocal_conn.status == CONN_OPEN)
		{
			/* -UCD!  Unquiet cdds %d */
			messageit(2, 90);

			/* unquiet specified cdds */
			unquiet_cdds((i2 *)NULL, (i2)flag_numeric, USER_QUIET);
		}
		return;
	}

	/* Unknown flag %s */
	messageit(3, 31, flag);
}


/*{
** Name:	file_flags - Set flags from file
**
** Description:
**	Reads the runrepl.opt file for flag lines and passes them on to
**	set_flags.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**	none
*/
void
file_flags()
{
	char		fbuff[80];
	FILE		*config;
	LOCATION	loc;
	char		filename[MAX_LOC+1];

	STcopy(ERx("runrepl.opt"), filename);
	LOfroms(FILENAME, filename, &loc);
	if (SIfopen(&loc, ERx("r"), SI_TXT, SI_MAX_TXT_REC, &config) != OK)
	{
		/* config file not found */
		messageit(3, 65, filename);
		return;
	}

	while (SIgetrec(fbuff, sizeof(fbuff), config) != ENDFILE)
		set_flags(fbuff);

	SIclose(config);
}


/*{
** Name:	com_flags - set flags from command line
**
** Description:
**	Reads the command line server startup flags and passes them to
**	set_flags.
**
** Inputs:
**	argc - the number of arguments on the command line
**	argv - the command line arguments
**
** Outputs:
**	none
**
** Returns:
**	none
*/
void
com_flags(
i4	argc,
char	*argv[])
{
	i4	i;

	for (i = 1; i < argc; ++i)
		set_flags(argv[i]);
}


/*{
** Name:	check_flags - ensure all required startup flags are set
**
** Description:
**	Check if IDB, OWN, and SVR flags are set before we attempt to open
**	local db in RepServer or DistServ.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**	TRUE - all flags are set
**	FALSE - at least one isn't set
*/
bool
check_flags()
{
	bool	ret_val = TRUE;

	if (*RSlocal_conn.db_name == EOS)
	{
		ret_val = FALSE;
		messageit(1, 1812);
	}

	if (*RSlocal_conn.owner_name == EOS)
	{
		ret_val = FALSE;
		messageit(1, 1813);
	}

	if (RSserver_no == -1)
	{
		ret_val = FALSE;
		messageit(1, 1814);
	}

	if (RSqbm_read_must_break < RSqbt_read_try_to_break)
	{
		RSqbm_read_must_break = RSqbt_read_try_to_break;
		messageit(2, 99, RSqbm_read_must_break);
	}

	return (ret_val);
}
