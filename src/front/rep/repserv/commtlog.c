/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <st.h>
# include <cv.h>
# include <lo.h>
# include <si.h>
# include <me.h>
# include <er.h>
# include <gl.h>
# include <iicommon.h>
# include <iiapi.h>
# include <sapiw.h>
# include "conn.h"
# include "recover.h"

/**
** Name:	commtlog.c - commit log
**
** Description:
**	Defines
**		RStpc_recover		- two-phase commit recovery
**		check_commit_log	- check commit log
**		get_commit_log_entries	- get commit log entries
**		compress_entries	- compress commit log entries
**
** FIXME:
**	Use TM instead of a SELECT to find the time differences.
**
** History:
**	16-dec-96 (joea)
**		Created based on ckcmtlog.sc and recovcmt.c in replicator
**		library.
**	22-jan-97 (joea)
**		The shadow table name is no longer written to the commit log.
**		Use consistent naming for table_name.
**      13-Jun-97 (fanra01)
**          Moved and changed declaration of entries as this exceeds the
**          allowable stack size of a process started from the service control
**          manager on NT.
**	15-Apr-98 consi01 bug 90570 ingrep 32
**	    Changed memory copy call from MEcopy to MEcopylng as MEcopy is
**	    limited to 64k bytes and a worst case for compress_entries is
**	    just under 1.2 meg
**	21-apr-98 (joea)
**		Convert to use OpenAPI instead of ESQL.
**	19-may-98 (joea)
**		Correct order of arguments to STcopy in get_commit_log_entries.
**		Use defines for recovery entry numbers.
**	08-jul-98 (abbjo03)
**		Add new arguments to IIsw_selectSingleton.
**	24-feb-99 (abbjo03)
**		Replace II_PTR error handle parameters with IIAPI_GETEINFOPARM
**		error parameter blocks.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
[@history_template@]...
**/

# define	MAX_WORDS	11


GLOBALDEF FILE	*RScommit_fp;		/* two phase commit log file */
GLOBALDEF bool	RSdo_recover = FALSE;


static bool	first_call = TRUE;

static COMMIT_ENTRY	entries[MAX_ENTRIES];


static STATUS check_commit_log(void);
static STATUS compress_entries(COMMIT_ENTRY *entries, i4  *last);


/*{
** Name:	RStpc_recover - two-phase commit recovery
**
** Description:
**	Completes or rolls back in progress commits (using
**	recover_from_log), then resets the two phase commit log file.
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
RStpc_recover()
{
	STATUS		err;
	char		filename[MAX_LOC+1];
	LOCATION	loc;
	bool		create_file = TRUE;
	STATUS		status;
	STATUS		log_entries = 0;

	STcopy(ERx("commit.log"), filename);
	LOfroms(FILENAME, filename, &loc);

	if (first_call)
	{
		if (SIfopen(&loc, ERx("r"), SI_TXT, SI_MAX_TXT_REC,
			&RScommit_fp) == OK)
		{
			log_entries = check_commit_log();
			SIclose(RScommit_fp);
		}
	}
	else
	{
		SIclose(RScommit_fp);
	}

	status = SIfopen(&loc, ERx("r"), SI_TXT, SI_MAX_TXT_REC, &RScommit_fp);
	if (status == OK)
	{
		if (!first_call || (first_call && log_entries))
		{
			err = recover_from_log();
			if (err)	/* preserve log for hand edit */
			{
				SIclose(RScommit_fp);
				status = SIfopen(&loc, ERx("a"), SI_TXT,
					SI_MAX_TXT_REC, &RScommit_fp);
				create_file = FALSE;
			}
			else
			{
				RSdo_recover = FALSE;
			}
		}
	}
	if (create_file)
	{
		if (status == OK)
		{
			SIclose(RScommit_fp);
			LOdelete(&loc);
		}
		status = SIfopen(&loc, ERx("w"), SI_TXT, SI_MAX_TXT_REC,
			&RScommit_fp);
		SIfprintf(RScommit_fp, ERx("%s!%s\n"), RSlocal_conn.db_name,
			RSlocal_conn.owner_name);
		SIflush(RScommit_fp);
	}
	if (status != OK)
	{
		/* Cannot open two phase commit log file -- ABORTING */
		messageit(1, 1167);
		RSshutdown(FAIL);
	}
	first_call = FALSE;
}


/*{
** Name:	check_commit_log - check commit log
**
** Description:
**	Checks to see if all transactions older than nseconds in the
**	commit log are complete.  If so, returns 0.  If not, returns
**	non-zero.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**	OK, 1705, 1159 or error from get_commit_log_entries
**
** History:
**      13-Jun-97 (fanra01)
**          Moved and changed declaration of entries as this exceeds the
**          allowable stack size of a process started from the service control
**          manager on NT.
**	26-mar-2004 (gupsh01)
**	    Added getQinfoParm as the input parameter to II_sw functions.
*/
static STATUS
check_commit_log()
{
	i4			last = 0;
	STATUS			status;
	i4			i;
	STATUS			msg_no;
	i4			delta_seconds;
	char			stmt[256];
	IIAPI_DATAVALUE		cdata;
	II_PTR			stmtHandle;
	IIAPI_GETEINFOPARM	errParm;
	IIAPI_GETQINFOPARM	getQinfoParm;

	status = get_commit_log_entries(entries, &last);
	if (status)
		return (status);

	for (i = 0; i < last; i++)
	{
		if (entries[i].entry_no != RS_2PC_END &&
			entries[i].entry_no != RS_NPC_END &&
			entries[i].entry_no != 0)
		{
			STprintf(stmt, ERx("SELECT INT4(INTERVAL('seconds', \
DATE('now') - DATE('%s')))"),
				entries[i].timestamp);
			SW_COLDATA_INIT(cdata, delta_seconds);
			(void)IIsw_selectSingleton(RSlocal_conn.connHandle,
				&RSlocal_conn.tranHandle, stmt, 0, NULL, NULL,
				1, &cdata, NULL, NULL, &stmtHandle, 
				&getQinfoParm, &errParm);
			IIsw_close(stmtHandle, &errParm);
			if (delta_seconds > 0)
			{
				/*
				** entry numbers between 7 and 9
				** are used when two-phase is off
				*/
				if (entries[i].entry_no >= RS_NPC_BEGIN &&
					entries[i].entry_no <= RS_NPC_END)
					msg_no = 1705;
				else
					msg_no = 1159;

				messageit(2, msg_no,
					(i4)entries[i].source_db_no,
					entries[i].dist_trans_id,
					entries[i].entry_no);
				return (msg_no);
			}
		}
	}

	IIsw_commit(&RSlocal_conn.tranHandle, &errParm);
	return (OK);
}


/*{
** Name:	get_commit_log_entries - get commit log entries
**
** Description:
**	Reads commit logs & builds array of structures showing the
**	last step recorded for each commit.
**
** Inputs:
**	entries	- pointer to base of COMMIT_ENTRY array.
**
** Outputs:
**	last	- count of entries read in
**
** Returns:
**	OK, 1515, 1158 or error from compress_entries.
*/
STATUS
get_commit_log_entries(
COMMIT_ENTRY	*entries,
i4		*last)
{
	COMMIT_ENTRY	entry;
	i4		source_db_no;
	i4		target_db_no;
	STATUS		ret_val;
	i4		i;
	bool		done;
	char		buf[SI_MAX_TXT_REC+1];
	i4		count;
	char		*words[MAX_WORDS];

	*last = 0;
	ret_val = SIgetrec(buf, (i4)sizeof(buf), RScommit_fp);
	count = MAX_WORDS;
	RPgetwords(ERx("!"), buf, &count, words);
	if (count != 2)
	{
		messageit(1, 1515, buf);
		return (1515);
	}

	while (SIgetrec(buf, (i4)sizeof(buf), RScommit_fp) != ENDFILE)
	{
		count = MAX_WORDS;
		RPgetwords(ERx("!"), buf, &count, words);
		if (count == 0)
			continue;	/* ignore blank lines */
		if (count < MAX_WORDS)
		{
			messageit(1, 1515, buf);
			return (1515);
		}
		CVan(words[0], &entry.entry_no);
		CVan(words[1], &source_db_no);
		entry.source_db_no = (i2)source_db_no;
		CVan(words[2], &entry.dist_trans_id);
		CVan(words[3], &target_db_no);
		entry.target_db_no = (i2)target_db_no;
		STcopy(words[4], entry.table_owner);
		STcopy(words[5], entry.table_name);
		CVan(words[6], &entry.table_no);
		CVan(words[7], &entry.trans_id);
		CVan(words[8], &entry.seq_no);
		STcopy(words[9], entry.timestamp);

		switch (entry.entry_no)
		{
		case RS_2PC_BEGIN:
		case RS_NPC_BEGIN:
			entries[*last] = entry;		/* struct */
			++(*last);
			/* leave extra room just in case! */
			if (*last >= MAX_ENTRIES)
			{
				ret_val = compress_entries(entries, last);
				if (ret_val)
					return (ret_val);
			}
			break;

		case RS_PREP_COMMIT:
		case RS_REMOTE_COMMIT:
		case RS_2PC_END:
		case RS_NPC_REM_COMMIT:
		case RS_NPC_END:
			done = FALSE;
			for (i = 0; i < *last && !done; i++)
			{
				if (entries[i].source_db_no ==
						entry.source_db_no &&
					entries[i].dist_trans_id ==
						entry.dist_trans_id &&
					entries[i].entry_no ==
						entry.entry_no - 1)
				{
					entries[i].entry_no = entry.entry_no;
					STcopy(entry.table_owner,
						entries[*last].table_owner);
					STcopy(entry.table_name,
						entries[*last].table_name);
					entries[*last].table_no =
						entry.table_no;
					STcopy(entry.timestamp,
						entries[*last].timestamp);
					done = TRUE;
				}
			}
			if (!done)
			{
				messageit(1, 1158, entry.source_db_no,
					entry.dist_trans_id, entry.entry_no);
				return (1158);
			}
			break;
		}
	}

	return (OK);
}


/*{
** Name:	compress_entries - compress entries
**
** Description:
**	compresses log entries
**
** Inputs:
**	entries	- pointer to array of commit entries to compress
**	last	- index of last entry in the array
**
** Outputs:
**	entries	- pointer to compressed array of commit entries
**	last	- updated index of last entry in the array
**
** Returns:
**	1160 - Too many transactions
**	OK - success
**
** History:
**	15-Apr-98 consi01 bug 90570 ingrep 32
**	    Changed memory copy call from MEcopy to MEcopylng as MEcopy is
**	    limited to 64k bytes and a worst case for compress_entries is
**	    just under 1.2 meg
*/
static STATUS
compress_entries(
COMMIT_ENTRY	*entries,
i4		*last)
{
	i4	i;


	for (i = 0; i < *last; i++)
	{
		if (entries[i].entry_no == RS_2PC_END ||
			entries[i].entry_no == RS_NPC_END)
		{
			MEcopy(&entries[i+1], (i4)((*last - i) *
				sizeof(COMMIT_ENTRY)), &entries[i]);
			--(*last);
		}
	}

	if (*last > MAX_ENTRIES)
	{
		messageit(1, 1160);
		return (1160);
	}

	return (OK);
}
