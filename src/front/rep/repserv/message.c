/*
** Copyright (c) 2010 Ingres Corporation
*/
# include <stdarg.h>
# include <compat.h>
# include <st.h>
# include <si.h>
# include <lo.h>
# include <tm.h>
# include <pc.h>
# include <gv.h>
# include <er.h>
# include <ex.h>
# include <iiapi.h>
# include <sapiw.h>
# include <generr.h>
# include "rsconst.h"
# include "conn.h"
# include "repevent.h"
# include "distq.h"

/**
** Name:	message.c - message routines
**
** Description:
**	Defines
**		messageit		- output a message
**		messageit_text		- get message text
**		RSerror_check		- check for DBMS errors
**		RSerrlog_open		- open the server error log
**		RSerrlog_close		- close the server error log
**
** History:
**	16-dec-96 (joea)
**		Created based on message.sc in replicator library.
**	06-feb-97 (joea)
**		Correct default for RSprint_sess_nos. Replace parts of RLIST by
**		REP_TARGET.
**	25-feb-97 (joea) bug 64558
**		Eliminate leftover COMMIT in get_error_check_string() since it
**		causes MST's to be partially committed when a collision is
**		found in the middle of the MST.
**	03-mar-97 (joea) bug 76482
**		Move the local ROLLBACK in get_error_check_string() to the if
**		(in_transmit ...) block to allow SkipRow to work.
**	30-apr-97 (joea)
**		Change copyright message CA-Ingres to OpenIngres.
**	11-dec-97 (joea)
**		Replace REP_TARGET RStarget global by RS_TARGET global.
**	20-apr-98 (mcgem01)
**		Product name change to Ingres.
**	21-apr-98 (joea)
**		Convert to use OpenAPI instead of ESQL.
**	08-jun-98 (abbjo03)
**		Remove error_check which is now unused.
**	23-jul-98 (abbjo03)
**		In error_get_string, test for success before concatenating the
**		error message. Use SYSTEM_PRODUCT_NAME in copyright message.
**	14-aug-98 (abbjo03)
**		In error_get_string, initialize errParm.ge_status.
**	04-nov-98 (abbjo03)
**		In error_get_string, deal with a IIAPI_ST_NO_DATA return from
**		IIsw_getErrorInfo.
**	03-dec-98 (abbjo03)
**		Eliminate unused RSerr_ret global.
**	19-jan-99 (abbjo03)
**		In error_get_string, call IIsw_close before returning. Do not
**		recurse into RSping.
**	23-feb-99 (abbjo03)
**		Replace II_PTR error handle parameters with IIAPI_GETEINFOPARM
**		error parameter blocks.
**	09-mar-99 (abbjo03)
**		In RSerror_check, move call to RSping after rolling back the
**		transaction. In RSerror_check and messageit, if we have reached
**		max errors, first output the current error info.
**      21-apr-1999 (hanch04)
**        Replace STrindex with STrchr
**	23-aug-99 (abbjo03)
**		Replace globaldef Version by GV_VER to have the version compiled
**		into the executable.
**	12-oct-99 (abbjo03)
**		Add global RSoptim_coll_resol in order not to roll back the
**		target transaction when using optimistic resolution.
**	24-nov-99 (abbjo03)
**		Rename RSoptim_coll_resol as RSonerror_norollback_target so
**		that it can be used in IIRSrlt_ReleaseTrans.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	30-mar-2004 (gupsh01)
**	    Added getQinfoParm as input parameter to II_sw... function.
**	20-apr-2004 (abbjo03)
**	    Ensure stdout is flushed after writing messages.
**	12-Feb-2010 (frima01) SIR 123269
**	    Correct copyright message to use global Reldate.
**/

# define RS_ERROR_LOG		ERx("replicat.log")


GLOBALDEF i4	RSerr_cnt = 0;
GLOBALDEF i4	RSerr_max = 100;
GLOBALDEF i4	RSlog_level = 3;
GLOBALDEF i4	RSprint_level = 3;
GLOBALDEF bool	RSonerror_norollback_target = FALSE;


GLOBALREF bool	RSin_transmit;
GLOBALREF char	Reldate[];
/* Note:  This is the only module that uses the following global */
GLOBALREF RS_TARGET	*RStarget;


static FILE	*logfile;
static char	copyright_msg[] =
	ERx("%s Replicator %s - Copyright (c) 1993, %s Ingres Corporation\n");


FUNC_EXTERN void build_messageit_string(char *msg_prefix, char *timestamp,
	char *msg_text, i4 msg_num, i4 msg_level, va_list ap);
FUNC_EXTERN void get_message_text(char *msg_text, i4 msg_num, short err_code,
	va_list ap);
FUNC_EXTERN void maillist(char *msg_string);
FUNC_EXTERN void word_wrap(char *msg_string);


/*{
** Name:	messageit - output a message
**
** Description:
**	Outputs messages to the log file and stdout.  According to the print
**	level or log level, a message will be output or ignored.
**
** Inputs:
**	msg_level	- the print and logging level of the message
**	msg_num		- the number that identifies the message
**	...		- variable argument list corresponding to format
**			  control string conversion specifications
**
** Outputs:
**	dest_string - the formatted result string
**
** Returns:
**	none
*/
void
messageit(
i4	msg_level,
i4	msg_num, ...)
{
	va_list	ap;
	char	dest_string[1000];
	char	msg_text[1000];
	char	msg_prefix[10];
	char	timestamp[26];
	char	db_string[DB_MAXNAME*2+4];

	if (msg_level > RSlog_level && msg_level > RSprint_level)
		return;

	va_start(ap, msg_num);
	build_messageit_string(msg_prefix, timestamp, msg_text, msg_num,
		msg_level, ap);
	va_end(ap);

	if (RSlocal_conn.status == CONN_OPEN)
		STprintf(db_string, ERx("%s%s%s "), RSlocal_conn.vnode_name,
			*RSlocal_conn.vnode_name == EOS ? ERx("") : ERx("::"),
			RSlocal_conn.db_name);
	else
		*db_string = EOS;

	STprintf(dest_string, ERx("%sserver %d:  %s -- %s -- %d -- %s"),
		db_string, (i4)RSserver_no, msg_prefix, timestamp, msg_num,
		msg_text);
	word_wrap(dest_string);

	if (msg_level <= RSlog_level && logfile)
	{
		SIfprintf(logfile, ERx("%s\n"), dest_string);
		SIflush(logfile);
	}
	if (msg_level <= RSprint_level)
	{
		SIprintf(ERx("%s\n"), dest_string);
		SIflush(stdout);
	}
	if (RSlocal_conn.status == CONN_OPEN && msg_level == 1)
		maillist(dest_string);

	if (msg_level == 1 && msg_num != 1044)	/* too many errors */
	{
		if (RSerr_cnt++ >= RSerr_max)
		{
			if (RSerr_cnt < 2 * RSerr_max)
				messageit(1, 1044, RSerr_max);
			else
				SIprintf("ABORTING -- TOO MANY ERRORS\n");
			RSshutdown(FAIL);
		}
		if (RSlocal_conn.status == CONN_OPEN && msg_num != 1212)
			RSping(FALSE);
	}
}


/*{
** Name:	messageit_text - get message text
**
** Description:
**	Fills in a string with the expanded message text (parameters
**	filled in, but no prefix or timestamp)
**
** Inputs:
**	msg_num	- the number that identifies the message
**	...	- variable argument list corresponding to format
**		  control string conversion specifications
**
** Outputs:
**	dest_string - the formatted result string
**
** Returns:
**	none
*/
void
messageit_text(
char	*dest_string,
i4	msg_num, ...)
{
	va_list	ap;
	va_start(ap, msg_num);

	get_message_text(dest_string, msg_num, 0, ap);
	va_end(ap);
}


/*{
** Name:	RSerror_check - check for DBMS errors
**
** Description:
**	Looks at the DBMS error and number of rows returned, and reports any
**	discrepancy.
**
** Inputs:
**	msg_num		- the number that identifies the message
**	row_ind		- indicates acceptable row counts for the most recent
**			  query:
**				1	- exacly one row
**				2	- one or more rows
**				-1	- zero or one row
**				other	- any number of rows
**	stmtHandle	- statement handle
**	errParm		- error parameter block
**	...		- variable argument list corresponding to format
**			  control string conversion specifications
**
** Outputs:
**	rowCount	- row count
**
** Returns:
**	STATUS
*/
STATUS
RSerror_check(
i4			msg_num,
i4			row_ind,
II_PTR			stmtHandle,
IIAPI_GETQINFOPARM	*getQinfoParm,
IIAPI_GETEINFOPARM	*errParm,
II_LONG			*rowCount,
...)
{
	va_list	ap;
	char	dest_string[2000];
	char	msg_text[1000];
	char	db_string[DB_MAXNAME*2+4];
	SYSTIME	now;
	char	timestamp[26];
	char	err_string[1000];
	STATUS	status;
	i4	err_type = OK;
	bool	rolled_back = FALSE;
	II_LONG	cnt;
	IIAPI_GETEINFOPARM	errorParm;

	if (rowCount == NULL)
		rowCount = &cnt;
	if (errParm->ge_errorHandle)
	{
		status = (STATUS)errParm->ge_errorCode;
		if (status != OK)
			err_type = RS_ERR_DBMS;
	}
	/* determine error code based on rows returned */
	else
	{
		*rowCount = IIsw_getRowCount(getQinfoParm);
		switch ((i4)*rowCount)
		{
		case 0:
			if (row_ind == ROWS_SINGLE_ROW ||
					row_ind == ROWS_ONE_OR_MORE)
				err_type = RS_ERR_ZERO_ROWS;
			break;

		case 1:	/* OK in all cases */
			break;

		default:	/* many rows */
			if (row_ind == ROWS_ZERO_OR_ONE ||
					row_ind == ROWS_SINGLE_ROW)
				err_type = RS_ERR_TOO_MANY_ROWS;
			break;
		}
		status = err_type;
	}

	if (errParm->ge_errorHandle && errParm->ge_message &&
			*errParm->ge_message != EOS)
		STprintf(err_string, ERx(" -- %d -- %s"), errParm->ge_errorCode,
			errParm->ge_message);
	else
		*err_string = EOS;
	IIsw_close(stmtHandle, errParm);
	if (err_type == OK)
		return (OK);

	va_start(ap, rowCount);
	get_message_text(msg_text, msg_num, (short)err_type, ap);
	va_end(ap);

	if (RSlocal_conn.status == CONN_OPEN)
		STprintf(db_string, ERx("%s%s%s "), RSlocal_conn.vnode_name,
			*RSlocal_conn.vnode_name == EOS ? ERx("") : ERx("::"),
			RSlocal_conn.db_name);
	else
		*db_string = EOS;

	TMnow(&now);
	TMstr(&now, timestamp);
	STprintf(dest_string, ERx("%sserver %d:  %s -- %s -- %d -- %s%s"),
		db_string, (i4)RSserver_no, "Error", timestamp, msg_num,
		msg_text, err_string);
	word_wrap(dest_string);

	++RSerr_cnt;
	if (RSin_transmit && RStarget->error_mode != EM_SKIP_ROW &&
		!STequal(errParm->ge_SQLSTATE, II_SS08006_CONNECTION_FAILURE) &&
		!STequal(errParm->ge_SQLSTATE, II_SS50005_GW_ERROR) &&
		!STequal(errParm->ge_SQLSTATE, II_SS50006_FATAL_ERROR))
	{
		if (!RSonerror_norollback_target)
			IIsw_rollback(&RSconns[RStarget->conn_no].tranHandle,
				&errorParm);
		IIsw_rollback(&RSlocal_conn.tranHandle, &errorParm);
		rolled_back = TRUE;
	}

	if (msg_num != 1212)	/* don't recurse into RSping */
		RSping(FALSE);

	if (RSlog_level >= 1 && logfile)
	{
		SIfprintf(logfile, ERx("%s\n"), dest_string);
		SIflush(logfile);
	}
	if (RSprint_level >= 1)
		SIprintf(ERx("%s\n"), dest_string);
	maillist(dest_string);
	if (rolled_back)
		IIsw_commit(&RSlocal_conn.tranHandle, &errorParm);

	if (RSerr_cnt >= RSerr_max)
	{
		if (RSerr_cnt < 2 * RSerr_max)
			messageit(1, 1044, RSerr_max);
		else
			SIprintf("ABORTING -- TOO MANY ERRORS\n");
		RSshutdown(FAIL);
	}

	return (status);
}


/*{
** Name:	RSerrlog_open - open the server error log file
**
** Description:
**	Opens the RepServer error log file in append mode and issues the
**	startup message.
**
**	The determination of the copyright dates is taken from FEcopyright().
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
RSerrlog_open()
{
	LOCATION	loc;
	char		filename[MAX_LOC+1];
	char		*cur_year;

	STcopy(RS_ERROR_LOG, filename);
	LOfroms(FILENAME, filename, &loc);
	if (SIfopen(&loc, ERx("a"), SI_TXT, SI_MAX_TXT_REC, &logfile) != OK)
	{
		SIprintf("REPSERV:  Unable to open log file -- ABORTING\n");
		PCexit(FAIL);
	}

	if ((cur_year = STrchr(Reldate, '-')) == NULL)
		EXsignal(EXFEBUG, 1, ERx("RSerrlog_open"));
	else
		++cur_year;

	SIfprintf(logfile, copyright_msg, SYSTEM_PRODUCT_NAME, GV_VER,
		cur_year);
	SIflush(logfile);
}


void
RSerrlog_close()
{
	if (logfile)
		SIclose(logfile);
}
