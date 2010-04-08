/*
** Copyright (c) 2004 Ingres Corporation
*/
# ifndef CONN_H_INCLUDED
# define CONN_H_INCLUDED
# include <compat.h>
# include <gl.h>
# include <iicommon.h>
# include <iiapi.h>
# include "rsstats.h"

/**
** Name:	conn.h - connections, sessions and message logging
**
** Description:
**	Structures, defines and functions related to Replicator Server
**	connections, sessions and message logging.
**
** History:
**	16-dec-96 (joea)
**		Created based on conn.sh in replicator library.
**	21-apr-98 (joea)
**		Convert to use OpenAPI instead of ESQL.
**	15-may-98 (joea)
**		Change arguments (and rename) RSconn_open_recovery.
**	07-jul-98 (abbjo03)
**		Add prefix to USER_NAME to avoid conflict with define in nmcl.h.
**	03-dec-98 (abbjo03)
**		Add name_case member to RS_CONN to hold case of regular DBMS
**		objects. Eliminate unused RSerr_ret global.
**	20-jan-99 (abbjo03)
**		Add notification connection. Add errParm and rowCount parameters
**		to RSerror_check.
**	05-may-99 (abbjo03)
**		Add RSconn_get_name_case.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	01-Mar-2005 (inifa01) INGREP168 bug 113683
**	    The replicator server fails on connecting to a target database with
**	    error  96 -- OPEN_DB:  DBMS error 15335425 selecting DBA name. 
**	    FIX: c471987 modified dbmsinfo() to return a varchar(64) instead of
**	    a varchar(32).  Modified size of buffer here accordingly.
**	29-mar-2004 (gupsh01)
**	    Modified length of name of RS_USER_NAME to accomodate length size of
**	    dbmsinfo(), which is now 64 instead of 32 (bug 465262).
**      23-Aug-2005 (inifa01)INGREP174/B114410
**          The RepServer takes shared locks on base, shadow and archive tables
**          when Replicating FP -> URO.  causing deadlock and lock waits.
**          - The resolution to this problem adds flag -NLR and opens a separate
**          session running with readlock=nolock if this flag is set.  
**          File updated to add declaration for RSlocal_conn_nlr() and RSnolock_read. 
**	22-Nov-2007 (kibro01) b119245
**	    Added RSrequiet to turn server back to -QIT
**/

# define UNDEFINED_CONN		0		/* undefined connection */
# define LOCAL_CONN		1		/* local connection */
# define RECOVER_CONN		2		/* recover connection */
# define NOTIFY_CONN		3		/* notification connection */

# define FIXED_CONNS		4		/* number of fixed conns */


/* connection status */

# define CONN_CLOSED		0		/* closed */
# define CONN_OPEN		1		/* open */
# define CONN_FAIL		(-1)		/* failed */

# define CONN_NORETRY		0		/* do not retry connections */
# define CONN_NOTIMEOUT		0		/* do not time out conns */

# define QRY_TIMEOUT_DFLT	60

/* RSerror_check row indicator codes */

# define ROWS_DONT_CARE		0
# define ROWS_ZERO_OR_ONE	(-1)
# define ROWS_SINGLE_ROW	1
# define ROWS_ONE_OR_MORE	2

# define RS_ERR_DBMS		1
# define RS_ERR_ZERO_ROWS	2
# define RS_ERR_TOO_MANY_ROWS	3


typedef struct
{
	i2	db_no;				/* database number */
	i4	status;				/* connection status */
	char	vnode_name[DB_MAXNAME+1];	/* vnode name */
	char	db_name[DB_MAXNAME+1];		/* database name */
	char	owner_name[DB_MAXNAME+1];	/* owner name */
	char	dbms_type[9];			/* dbms type */
	i4	target_type;			/* to indicate service level */
	bool	gateway;			/* gateway flag */
	bool	tables_valid;			/* integrity checks ever done */
	i4	last_needed;			/* bintime last needed */
	i4	open_retry;			/* open retry counter */
	i4	name_case;			/* case of regular objects */
	II_PTR	connHandle;			/* OpenAPI connection handle */
	II_PTR	tranHandle;			/* OpenAPI transaction handle */
} RS_CONN;

typedef struct
{
	short	len;
	char	name[DB_MAXNAME*2+2];
} RS_USER_NAME;

GLOBALREF RS_CONN	RSlocal_conn;
GLOBALREF RS_CONN	RSlocal_conn_nlr;
GLOBALREF RS_CONN	*RSconns;		/* connections array */
GLOBALREF i4		RSnum_conns;		/* size of array */


/* close inactive targets after this many seconds */

GLOBALREF i4	RSconn_timeout;


/* The number of class 1 errors detected */

GLOBALREF i4	RSerr_cnt;

/* The maximum number of class 1 errors allowed */

GLOBALREF i4	RSerr_max;

/* Does the server mail error messages when a replication error occurs? */

GLOBALREF bool	RSerr_mail;


/* is there some reason to process the replication again */

GLOBALREF bool	RSgo_again;

/* L Flag Log level 1-5, level of messages logged */

GLOBALREF i4	RSlog_level;

/* use readlock=nolock on local base/shadow/archive */

GLOBALREF bool  RSnolock_read;

/* try to re-open targets after this many replication cycles (0 = never) */

GLOBALREF i4	RSopen_retry;

/* P Flag, Print Level 1-5, level that messages are printed */

GLOBALREF i4	RSprint_level;

/* Does the Replicator Server process action 1 events */

GLOBALREF bool	RSquiet;
GLOBALREF bool	RSrequiet;

GLOBALREF i2	RSserver_no;

/* 0 = continue forever, 1 = make 1 pass & stop */

GLOBALREF bool	RSsingle_pass;

/* 0 = check rules, 1 = don't check */

GLOBALREF bool	RSskip_check_rules;

/* query timeout (set at open only!) */

GLOBALREF i4	RStime_out;


FUNC_EXTERN STATUS RSlocal_db_open(void);
FUNC_EXTERN STATUS RSlocal_db_open_nlr(void);
FUNC_EXTERN void quiet_db(i2 db_no, i4  quiet_type);
FUNC_EXTERN void RSshutdown(STATUS status);
FUNC_EXTERN void unquiet_db(i2 db_no, i4  quiet_type);


FUNC_EXTERN STATUS RSconns_init(void);
FUNC_EXTERN void RSconns_check_idle(void);
FUNC_EXTERN void RSconns_check_failed(void);
FUNC_EXTERN void RSconns_close(void);
FUNC_EXTERN STATUS RSconn_open(i4 conn_no);
FUNC_EXTERN STATUS RSconn_open_recovery(i4 conn_no, i4 highid, i4 lowid);
FUNC_EXTERN i4  RSconn_lookup(i2 db_no);
FUNC_EXTERN STATUS RSconn_get_name_case(RS_CONN *conn);
FUNC_EXTERN void RSconn_close(i4 conn_no, i4  status);


/* message logging */

FUNC_EXTERN void RSerrlog_open(void);
FUNC_EXTERN void RSerrlog_close(void);
FUNC_EXTERN void messageit(i4 msg_level, i4  msg_num, ...);
STATUS RSerror_check(i4 msg_num, i4  row_ind, II_PTR stmtHandle,
	IIAPI_GETQINFOPARM  *getQinfoParm, IIAPI_GETEINFOPARM *errParm, 
	II_LONG *rowCount, ...);


FUNC_EXTERN void check_pid(void);

# endif
