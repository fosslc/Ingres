/*
**      Copyright (c) 2004 Ingres Corporation
**      All rights reserved.
*/

/*
** globals.h  the global variables are declared here and referenced
**	elsewhere
**
**	6/28/89	tomt	created
**	9/6/89	tomt	added frs_timeout global, eliminated install globs
**	3/13/90 tomt    changed empty lock list global to nonprotect and
**                      added log transaction filter
**	15-may-90 (kirke)
**	    Changed type of frs_timeout to i4  to match its actual usage.
**	11/1/95	nick	removed unused variables
*/
#include <compat.h>
#include <dbms.h>
#include <fe.h>

/*	Global variables, flags	*/

GLOBALDEF i4	lock_id;
GLOBALDEF i4	dbname_id;
GLOBALDEF char	tbl_name[FE_MAXNAME + 1];
GLOBALDEF char	*ptr_tablename = tbl_name;
GLOBALDEF i4	ing_locktype;
GLOBALDEF char	data_base_name[FE_MAXNAME + 1];
GLOBALDEF char	*ptr_dbname = data_base_name;

GLOBALDEF bool	is_db_open = FALSE;
GLOBALDEF bool	flag_db = FALSE;
GLOBALDEF bool 	flag_file = FALSE;
GLOBALDEF i4  	frs_timeout = 0;
GLOBALDEF bool 	flag_locktype = FALSE;
GLOBALDEF bool 	flag_all_locks = FALSE;
GLOBALDEF bool	flag_nl = FALSE;
GLOBALDEF bool	flag_table = FALSE;
GLOBALDEF bool	flag_cluster = FALSE; /* indicates if we are on a VAXcluster */
GLOBALDEF bool	flag_nonprot_lklists = FALSE;
GLOBALDEF bool	flag_inactive_tx = FALSE;/* do NOT show inactive xactions */
GLOBALDEF bool	flag_debug_log = FALSE;	/* do NOT log debugging to logfile */
GLOBALDEF bool	flag_standalone = FALSE;/* DO open databases */
GLOBALDEF bool	flag_client = FALSE;	/* DO call LG and LK */
GLOBALDEF bool	flag_internal_sess = FALSE;	/* do NOT display intrnl sess */
