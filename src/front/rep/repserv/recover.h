/*
** Copyright (c) 2004 Ingres Corporation
*/
# ifndef RECOVER_H_INCLUDED
# define RECOVER_H_INCLUDED
# include <compat.h>
# include <si.h>
# include <gl.h>
# include <iicommon.h>

/**
** Name:	recover.h - two-phase commit and recovery
**
** Description:
**	Structures, defines and functions related to two-phase commit and
**	log recovery.
**
** History:
**	16-dec-96 (joea)
**		Created based on recover.sh in replicator library.
**	22-jan-97 (joea)
**		Don't write the shadow table name to the commit log. Use
**		consistent naming for table_name.
**	15-may-98 (joea)
**		Rename COMMIT_ENTRY members for clarity and consistency. Add
**		defines for recovery entry numbers.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

# define MAX_ENTRIES		10000

# define RS_2PC_BEGIN		1
# define RS_PREP_COMMIT		2
# define RS_REMOTE_COMMIT	3
# define RS_2PC_END		4
# define RS_NPC_BEGIN		7
# define RS_NPC_REM_COMMIT	8
# define RS_NPC_END		9


typedef struct
{
	i4	entry_no;
	i4	dist_trans_id;
	i2	source_db_no;
	i4	trans_id;
	i4	seq_no;
	i2	target_db_no;
	i4	table_no;
	char	table_owner[DB_MAXNAME+1];
	char	table_name[DB_MAXNAME+1];
	char	timestamp[26];
} COMMIT_ENTRY;


/* 0 = two phase commit disabled, 1 = two phase commit on (default) */

GLOBALREF bool	RStwo_phase;

GLOBALREF bool	RSdo_recover;

GLOBALREF FILE	*RScommit_fp;


FUNC_EXTERN STATUS get_commit_log_entries(COMMIT_ENTRY *entries, i4  *last);
FUNC_EXTERN STATUS recover_from_log(void);
FUNC_EXTERN void RStpc_recover(void);

# endif
