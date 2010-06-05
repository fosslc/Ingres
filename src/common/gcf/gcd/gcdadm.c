/*
** Copyright (c) 2003, 2010 Ingres Corporation.
*/

#include <compat.h>
#include <cv.h>
#include <gc.h>
#include <gl.h>
#include <er.h>
#include <me.h>
#include <pc.h>
#include <sp.h>
#include <mo.h>
#include <pc.h>
#include <si.h>
#include <st.h>
#include <tr.h>
#include <tm.h>
#include <qu.h>

#include <iicommon.h>

#include <gca.h>
#include <gcm.h>
#include <gcu.h>
#include <gcd.h>
#include <gcdint.h>
#include <gcadm.h>
#include <gcadmint.h>

/*
** Name: gcdadm.c  
**
** Description:
**	GCADM interface with GCD server and iimonitor. 
**
** History:
**	 9-10-2003 (wansh01) 
**	    Created.
**	 7-Jul-2004 (wansh01) 
**	    Support SHOW commands.
**	 6-Aug-04 (gordy)
**	    Enhanced session info display.
**	 7-Oct-2004 (wansh01) 
**	    Support SHOW SERVER, SHOW SYSTEM SESSIONS FORMATTED commands.
**	20-Apr-2005 ( wansh01) 
**	    Seperate GCD_SESS_INFO to GCD_POOL_INFO to hold pool information
**	    and GCD_BUFF to hold output information. 
**	    added gcd_alloc_gcd_pool() to allocate memory for pool info.  
**	21-Apr-06 (usha)
**	    Added more commands.
**	        [force] option for stop server command instead of crash server.
**	        Tracing enable/disable.
**	        display statistics for various sessions.
**          Pooled sessions are treated as subtype of SYSTEM sessions.		
**	22-Jun-06 (usha)
**	    Formatted the "show server" command o/p to look like
**	    DBMS server "show server" o/p. Removed gcd_server_count() method.
**	 5-Jul-06 (usha)
**	    Get GCA association IDs from API connection handle.
**	14-Jul-06 (usha)
**	    Added support for "set trace" commands.
**      23-Aug-06 (usha)
**	    Added support for stats command.
**	27-Sep-06 (usha)
**	    Re-worked on the entire code to scale well as per Gordy's 
**	    GCN ADMIN implementation. 
**	 5-Dec-07 (gordy)
**	    Moved client connection limit to GCC level.
**	09-Sep-08 (usha) Bug 120877
**	    Fixed the user message when setting the trace level for the 
**	    GCD server. I noticed this error while testing dynamic tracing
**	    for DAS server for SD issue 130705. 
**	18-Aug-2009 (drivi01)
**	    Fix precedence warnings in effort to port to Visual Studio 2008.
**	21-Jul-09 (gordy)
**	    Remove string length restrictions.
**	31-Mar-10 (gordy & rajus01) SD issue 142688, Bug 123270
**	    iimonitor connections to DAS server hang on VMS platform at the
**	    3rd attempt after 2 successful attempts. The hang doesn't seem
**	    to occur on other plaforms but E_GCFFFF_IN_PROCESS error gets 
**	    logged in errlog.log.
**      22-Apr-10 (rajus01) SIR 123621
	    Display process ID.
*/

/*
** GCD ADM Session control block.
*/

typedef struct
{
    QUEUE	sess_q;  
    i4		aid;
    i4		state;
    char	*user_id;
    QUEUE	rsltlst_q;
    char	msg_buff[ ER_MAX_LEN ];
} GCD_SESS_CB;

static QUEUE		gcd_sess_q;
static char		*user_default = "<default>";
static char		*unknown = "<unknown>";

/*
** Result list queue entries.  Provides GCADM_RSLT
** element as required for ADM result list and
** (variable length) buffer for referenced message.
*/

typedef struct
{
    GCADM_RSLT  rslt;
    char        msg_buff[1];
} ADM_RSLT;

/*
** Command components: token, keywords, etc.
*/

#define GCD_TOKEN		0x00 

#define GCD_KW_ALL		0x01
#define GCD_KW_ADM		0x02
#define GCD_KW_CLOSED		0x03
#define GCD_KW_CRASH		0x04
#define GCD_KW_FORCE		0x05
#define GCD_KW_FORMAT		0x06
#define GCD_KW_FORMATTED 	0x07
#define GCD_KW_HELP		0x08
#define GCD_KW_LSTN		0x09
#define GCD_KW_OPEN		0x0A
#define GCD_KW_POOL		0x0B
#define GCD_KW_REGISTER		0x0C
#define GCD_KW_REMOVE		0x0D
#define GCD_KW_SERVER		0x0E
#define GCD_KW_SESS		0x0F
#define GCD_KW_SET		0x10
#define GCD_KW_SHOW		0x11
#define GCD_KW_SHUT		0x12
#define GCD_KW_SHUTDOWN		0x13
#define GCD_KW_STATS		0x14
#define GCD_KW_STOP		0x15
#define GCD_KW_SYSTEM		0x16
#define GCD_KW_TRACE		0x17
#define GCD_KW_USER		0x18
#define GCD_KW_LOG		0x19
#define GCD_KW_NONE		0x1A
#define GCD_KW_LEVEL		0x1B


static ADM_KEYWORD keywords[] =
{
    { "ALL",		GCD_KW_ALL },
    { "ADMIN",		GCD_KW_ADM },
    { "CLOSED",		GCD_KW_CLOSED },
    { "CRASH",		GCD_KW_CRASH },
    { "FORCE",		GCD_KW_FORCE },
    { "FORMAT",		GCD_KW_FORMAT },
    { "FORMATTED",	GCD_KW_FORMATTED },
    { "HELP",		GCD_KW_HELP },
    { "LISTEN",		GCD_KW_LSTN },
    { "OPEN",		GCD_KW_OPEN },
    { "POOLED",		GCD_KW_POOL },
    { "REGISTER",	GCD_KW_REGISTER },
    { "REMOVE",		GCD_KW_REMOVE },
    { "SERVER",		GCD_KW_SERVER },
    { "SESSIONS",	GCD_KW_SESS },
    { "SET",		GCD_KW_SET },
    { "SHOW",		GCD_KW_SHOW },
    { "SHUT",		GCD_KW_SHUT },
    { "SHUTDOWN",	GCD_KW_SHUTDOWN },
    { "STATS",		GCD_KW_STATS },
    { "STOP",		GCD_KW_STOP },
    { "SYSTEM",		GCD_KW_SYSTEM },
    { "TRACE",		GCD_KW_TRACE },
    { "USER",		GCD_KW_USER },
    { "LOG",		GCD_KW_LOG },
    { "NONE",		GCD_KW_NONE },
    { "LEVEL",		GCD_KW_LEVEL },
};


#define	GCD_CMD_HELP			1
#define	GCD_CMD_SHOW_SVR		2
#define	GCD_CMD_SHOW_SVR_LSTN		3
#define	GCD_CMD_SHOW_SVR_SHUTDOWN	4
#define	GCD_CMD_SET_SVR_SHUT		5
#define	GCD_CMD_SET_SVR_CLSD		6
#define	GCD_CMD_SET_SVR_OPEN		7
#define	GCD_CMD_STOP_SVR		8
#define	GCD_CMD_STOP_SVR_FORCE		9
#define	GCD_CMD_CRASH_SVR		10
#define	GCD_CMD_REGISTER_SVR		11
#define	GCD_CMD_SHOW_ADM_SESS		12
#define	GCD_CMD_SHOW_USER_SESS		13
#define	GCD_CMD_SHOW_SYS_SESS		14
#define	GCD_CMD_SHOW_ALL_SESS		15
#define	GCD_CMD_SHOW_USER_SESS_FMT	16
#define	GCD_CMD_SHOW_ADM_SESS_FMT	17
#define	GCD_CMD_SHOW_SYS_SESS_FMT	18
#define	GCD_CMD_SHOW_ALL_SESS_FMT	19
#define	GCD_CMD_SHOW_USER_SESS_STATS	20
#define	GCD_CMD_SHOW_ADM_SESS_STATS	21
#define	GCD_CMD_SHOW_SYS_SESS_STATS	22
#define	GCD_CMD_SHOW_ALL_SESS_STATS	23
#define GCD_CMD_FRMT_USER_SESS          24
#define GCD_CMD_FRMT_SYS_SESS           25
#define GCD_CMD_FRMT_ADM_SESS           26
#define GCD_CMD_FRMT_ALL_SESS           27
#define	GCD_CMD_FORMAT_SESSID		28
#define	GCD_CMD_REMOVE_SESSID		29
#define	GCD_CMD_REMOVE_POOL_SESS	30
#define	GCD_CMD_SET_TRACE_LOG_NONE	31
#define	GCD_CMD_SET_TRACE_LOG_FNAME	32
#define	GCD_CMD_SET_TRACE_LEVEL	        33
#define	GCD_CMD_SET_TRACE_ID_LEVEL      34


/* Help - List all the commands */
static ADM_ID cmd_help[] =
	{ GCD_KW_HELP };

/* Server States Management Commands */

       /* Set Server State Commands */
static ADM_ID cmd_set_svr_shut[] =
	{ GCD_KW_SET, GCD_KW_SERVER, GCD_KW_SHUT };
static ADM_ID cmd_set_svr_clsd[] = 
	{ GCD_KW_SET, GCD_KW_SERVER, GCD_KW_CLOSED };
static ADM_ID cmd_set_svr_open[] = 
	{ GCD_KW_SET, GCD_KW_SERVER, GCD_KW_OPEN };

       /* Show  Server State Commands */
static ADM_ID cmd_show_svr[] = 
	{ GCD_KW_SHOW, GCD_KW_SERVER };
static ADM_ID cmd_show_svr_lstn[] = 
	{ GCD_KW_SHOW, GCD_KW_SERVER, GCD_KW_LSTN };
static ADM_ID cmd_show_svr_shutdown[] = 
	{ GCD_KW_SHOW, GCD_KW_SERVER, GCD_KW_SHUTDOWN};

/* Normal/Forceful Server Shutdown commands */
static ADM_ID cmd_stop_svr[] = 
	{ GCD_KW_STOP, GCD_KW_SERVER };
static ADM_ID cmd_stop_svr_force[] = 
	{GCD_KW_STOP, GCD_KW_SERVER, GCD_KW_FORCE };
static ADM_ID cmd_crash_svr[] = 
	{ GCD_KW_CRASH, GCD_KW_SERVER };
static ADM_ID cmd_register_svr[] = 
	{ GCD_KW_REGISTER, GCD_KW_SERVER };

/* Server Sessions Management Commands */

    /* Show Stats Commands */
static ADM_ID cmd_show_sess_stats[] = 
	{ GCD_KW_SHOW, GCD_KW_SESS, GCD_KW_STATS };
static ADM_ID cmd_show_user_sess_stats[] = 
	{ GCD_KW_SHOW, GCD_KW_USER, GCD_KW_SESS, GCD_KW_STATS };
static ADM_ID cmd_show_all_sess_stats[] = 
	{ GCD_KW_SHOW, GCD_KW_ALL, GCD_KW_SESS, GCD_KW_STATS };
static ADM_ID cmd_show_adm_sess_stats[] = 
	{ GCD_KW_SHOW, GCD_KW_ADM,GCD_KW_SESS , GCD_KW_STATS };
static ADM_ID cmd_show_sys_sess_stats[] = 
	{ GCD_KW_SHOW, GCD_KW_SYSTEM, GCD_KW_SESS, GCD_KW_STATS };

    /* Show Session Commands */
static ADM_ID cmd_show_all_sess[] = 
	{ GCD_KW_SHOW, GCD_KW_ALL,GCD_KW_SESS };
static ADM_ID cmd_show_adm_sess[] = 
	{ GCD_KW_SHOW, GCD_KW_ADM,GCD_KW_SESS };
static ADM_ID cmd_show_user_sess[] = 
	{ GCD_KW_SHOW, GCD_KW_USER,GCD_KW_SESS };
static ADM_ID cmd_show_sess[] = 
	{ GCD_KW_SHOW, GCD_KW_SESS };
static ADM_ID cmd_show_sys_sess[] = 
	{ GCD_KW_SHOW, GCD_KW_SYSTEM,GCD_KW_SESS };

static ADM_ID cmd_show_all_sess_fmt[] = 
	{ GCD_KW_SHOW, GCD_KW_ALL, GCD_KW_SESS, GCD_KW_FORMATTED };
static ADM_ID cmd_show_adm_sess_fmt[] = 
	{ GCD_KW_SHOW, GCD_KW_ADM,GCD_KW_SESS , GCD_KW_FORMATTED };
static ADM_ID cmd_show_user_sess_fmt[] = 
	{ GCD_KW_SHOW, GCD_KW_USER, GCD_KW_SESS, GCD_KW_FORMATTED };
static ADM_ID cmd_show_sess_fmt[] = 
	{ GCD_KW_SHOW, GCD_KW_SESS, GCD_KW_FORMATTED };
static ADM_ID cmd_show_sys_sess_fmt[] = 
	{ GCD_KW_SHOW, GCD_KW_SYSTEM,GCD_KW_SESS, GCD_KW_FORMATTED };

    /* Format Session Commands */
static ADM_ID cmd_format_all[] = 
	{ GCD_KW_FORMAT, GCD_KW_ALL };
static ADM_ID cmd_format_all_sess[] = 
	{ GCD_KW_FORMAT, GCD_KW_ALL, GCD_KW_SESS };
static ADM_ID cmd_format_user[] = 
	{ GCD_KW_FORMAT, GCD_KW_USER };
static ADM_ID cmd_format_user_sess[] = 
	{GCD_KW_FORMAT,GCD_KW_USER,GCD_KW_SESS };
static ADM_ID cmd_format_sess[] = 
	{ GCD_KW_FORMAT, GCD_KW_SESS };

static ADM_ID cmd_format_sys[] = 
	{ GCD_KW_FORMAT, GCD_KW_SYSTEM };
static ADM_ID cmd_format_sys_sess[] = 
	{ GCD_KW_FORMAT, GCD_KW_SYSTEM, GCD_KW_SESS };
static ADM_ID cmd_format_adm[] = 
	{ GCD_KW_FORMAT, GCD_KW_ADM };
static ADM_ID cmd_format_adm_sess[] = 
	{ GCD_KW_FORMAT, GCD_KW_ADM, GCD_KW_SESS };

static ADM_ID cmd_format_sid[] = 
	{ GCD_KW_FORMAT, GCD_TOKEN };

     /* Remove Session Commands */
static ADM_ID cmd_remove_sid[] = 
	{ GCD_KW_REMOVE, GCD_TOKEN };
static ADM_ID cmd_rmv_pool_sess[] = 
	{ GCD_KW_REMOVE, GCD_KW_POOL, GCD_KW_SESS };

/* Trace commands */
static ADM_ID cmd_set_trace_log_none[] = 
	{ GCD_KW_SET, GCD_KW_TRACE, GCD_KW_LOG, GCD_KW_NONE };
static ADM_ID cmd_set_trace_log_fname[] = 
	{ GCD_KW_SET, GCD_KW_TRACE, GCD_KW_LOG, GCD_TOKEN };
static ADM_ID cmd_set_trace_level[] = 
	{ GCD_KW_SET, GCD_KW_TRACE, GCD_KW_LEVEL, GCD_TOKEN };
static ADM_ID cmd_set_trace_id_level[] = 
	{ GCD_KW_SET, GCD_KW_TRACE, GCD_TOKEN, GCD_TOKEN };

static ADM_COMMAND commands[] =
{
    { GCD_CMD_HELP,
	ARR_SIZE( cmd_help ), cmd_help },
    { GCD_CMD_SET_SVR_SHUT,
	ARR_SIZE( cmd_set_svr_shut ), cmd_set_svr_shut },
    { GCD_CMD_SET_SVR_CLSD,
	ARR_SIZE( cmd_set_svr_clsd ), cmd_set_svr_clsd },
    { GCD_CMD_SET_SVR_OPEN,
	ARR_SIZE( cmd_set_svr_open ), cmd_set_svr_open },
    { GCD_CMD_STOP_SVR,
	ARR_SIZE( cmd_stop_svr ), cmd_stop_svr },
    { GCD_CMD_CRASH_SVR,
	ARR_SIZE( cmd_crash_svr ), cmd_crash_svr },
    { GCD_CMD_REGISTER_SVR,
	ARR_SIZE( cmd_register_svr ), cmd_register_svr },
    { GCD_CMD_SHOW_SVR,
	ARR_SIZE( cmd_show_svr ), cmd_show_svr },
    { GCD_CMD_SHOW_SVR_LSTN,
	ARR_SIZE( cmd_show_svr_lstn ), cmd_show_svr_lstn },
    { GCD_CMD_SHOW_SVR_SHUTDOWN,
	ARR_SIZE( cmd_show_svr_shutdown ), cmd_show_svr_shutdown },
    { GCD_CMD_SHOW_USER_SESS,
	ARR_SIZE( cmd_show_user_sess ), cmd_show_user_sess },
    { GCD_CMD_SHOW_USER_SESS,
	ARR_SIZE( cmd_show_sess ), cmd_show_sess },
    { GCD_CMD_SHOW_USER_SESS_FMT,
	ARR_SIZE( cmd_show_sess_fmt ), cmd_show_sess_fmt},
    { GCD_CMD_SHOW_USER_SESS_FMT,
	ARR_SIZE( cmd_show_user_sess_fmt ), cmd_show_user_sess_fmt},
    { GCD_CMD_SHOW_ADM_SESS,
	ARR_SIZE( cmd_show_adm_sess ), cmd_show_adm_sess},
    { GCD_CMD_SHOW_ADM_SESS_FMT,
	ARR_SIZE( cmd_show_adm_sess_fmt ), cmd_show_adm_sess_fmt},
    { GCD_CMD_SHOW_SYS_SESS,
	ARR_SIZE( cmd_show_sys_sess ), cmd_show_sys_sess},
    { GCD_CMD_SHOW_SYS_SESS_FMT,
	ARR_SIZE( cmd_show_sys_sess_fmt ), cmd_show_sys_sess_fmt},
    { GCD_CMD_SHOW_ALL_SESS,
	ARR_SIZE( cmd_show_all_sess ), cmd_show_all_sess},
    { GCD_CMD_SHOW_ALL_SESS_FMT,
	ARR_SIZE( cmd_show_all_sess_fmt ), cmd_show_all_sess_fmt},
    { GCD_CMD_REMOVE_POOL_SESS,
	ARR_SIZE( cmd_rmv_pool_sess ), cmd_rmv_pool_sess},
    { GCD_CMD_SHOW_ALL_SESS_FMT,
	ARR_SIZE( cmd_format_all ), cmd_format_all },
    { GCD_CMD_SHOW_ALL_SESS_FMT,
	ARR_SIZE( cmd_format_all_sess ), cmd_format_all_sess },
    { GCD_CMD_SHOW_USER_SESS_FMT,
	ARR_SIZE( cmd_format_user ), cmd_format_user },
    { GCD_CMD_SHOW_USER_SESS_FMT,
	ARR_SIZE( cmd_format_user_sess ), cmd_format_user_sess },
    { GCD_CMD_SHOW_ADM_SESS_FMT,
	ARR_SIZE( cmd_format_adm ), cmd_format_adm },
    { GCD_CMD_SHOW_ADM_SESS_FMT,
	ARR_SIZE( cmd_format_adm_sess ), cmd_format_adm_sess },
    { GCD_CMD_SHOW_SYS_SESS_FMT,
	ARR_SIZE( cmd_format_sys ), cmd_format_sys },
    { GCD_CMD_SHOW_SYS_SESS_FMT,
	ARR_SIZE( cmd_format_sys_sess ), cmd_format_sys_sess },
    { GCD_CMD_FRMT_ADM_SESS,
        ARR_SIZE( cmd_format_adm ), cmd_format_adm},
    { GCD_CMD_FRMT_ADM_SESS,
        ARR_SIZE( cmd_format_adm_sess ), cmd_format_adm_sess },
    { GCD_CMD_FRMT_USER_SESS,
        ARR_SIZE( cmd_format_user ), cmd_format_user },
    { GCD_CMD_FRMT_USER_SESS,
        ARR_SIZE( cmd_format_user_sess ), cmd_format_user_sess },
    { GCD_CMD_FRMT_SYS_SESS,
        ARR_SIZE( cmd_format_sys ), cmd_format_sys },
    { GCD_CMD_FRMT_SYS_SESS,
        ARR_SIZE( cmd_format_sys_sess ), cmd_format_sys_sess },
    { GCD_CMD_FRMT_ALL_SESS,
        ARR_SIZE( cmd_format_all ), cmd_format_all },
    { GCD_CMD_FRMT_ALL_SESS,
        ARR_SIZE( cmd_format_all_sess ), cmd_format_all_sess },
    { GCD_CMD_FORMAT_SESSID,
	ARR_SIZE( cmd_format_sid ), cmd_format_sid},
    { GCD_CMD_REMOVE_SESSID,
	ARR_SIZE( cmd_remove_sid ), cmd_remove_sid},
    { GCD_CMD_SHOW_USER_SESS_STATS,
	ARR_SIZE( cmd_show_sess_stats ), cmd_show_sess_stats },
    { GCD_CMD_SHOW_USER_SESS_STATS,
	ARR_SIZE( cmd_show_user_sess_stats ), cmd_show_user_sess_stats },
    { GCD_CMD_SHOW_ADM_SESS_STATS,
	ARR_SIZE( cmd_show_adm_sess_stats ), cmd_show_adm_sess_stats },
    { GCD_CMD_SHOW_SYS_SESS_STATS,
	ARR_SIZE( cmd_show_sys_sess_stats ), cmd_show_sys_sess_stats },
    { GCD_CMD_SHOW_ALL_SESS_STATS,
	ARR_SIZE( cmd_show_all_sess_stats ), cmd_show_all_sess_stats },
    { GCD_CMD_SET_TRACE_LOG_NONE,
	ARR_SIZE( cmd_set_trace_log_none ) , cmd_set_trace_log_none},
    { GCD_CMD_SET_TRACE_LOG_FNAME,
	ARR_SIZE( cmd_set_trace_log_fname ) , cmd_set_trace_log_fname},
    { GCD_CMD_SET_TRACE_LEVEL,
	ARR_SIZE( cmd_set_trace_level ), cmd_set_trace_level},
    { GCD_CMD_SET_TRACE_ID_LEVEL,
	ARR_SIZE( cmd_set_trace_id_level ), cmd_set_trace_id_level},

};

#define ADM_SVR_OPEN_TXT        "User connections now allowed"
#define ADM_SVR_CLSD_TXT        "User connections now disabled"
#define ADM_SVR_SHUT_TXT        "Server will stop"
#define ADM_SVR_STOP_TXT        "Server stopping"
#define ADM_SVR_REG_TXT         "Name Server registered"
#define ADM_REG_ERR_TXT         "Name Server registration failed"
#define ADM_LOG_OPEN_TXT        "Logging enabled"
#define ADM_LOG_CLOSE_TXT       "Logging disabled"
#define ADM_TRACE_ERR_TXT       "Error setting trace level"
#define ADM_BAD_TRACE_TXT       "Invalid trace id"
#define ADM_BAD_SID_TXT         "Invalid session id"
#define ADM_BAD_COMMAND_TXT     "Illegal command"
#define ADM_REM_TXT             "Session removed"
#define ADM_REM_POOL_TXT        "Pool session(s) removed"
#define ADM_REM_SYSERR_TXT      "System sessions cannot be removed"
#define ADM_REM_ADMERR_TXT      "Admin sessions cannot be removed"
#define ADM_REM_POOLERR_TXT     "Error removing pooled sessions"

static char *helpText[] =
{
    "Server:",
    "     SET SERVER SHUT | CLOSED | OPEN",
    "     STOP SERVER [FORCE]",
    "     SHOW SERVER [LISTEN | SHUTDOWN]",
    "     REGISTER SERVER",
    "Session:",
    "     SHOW [USER] | ADMIN | SYSTEM | ALL SESSIONS [FORMATTED | STATS]",
    "     FORMAT [USER] | ADMIN | SYSTEM | ALL [SESSIONS]",
    "     FORMAT <session_id>",
    "     REMOVE POOLED SESSIONS",
    "Others:",
    "     SET TRACE LOG <path> | NONE",
    "     SET TRACE LEVEL | <id> <lvl>",
    "     HELP",
    "     QUIT",
};

/*
**  GCD_CMD_MAX_TOKEN is always one more than the max tokens in a 
**  valid command line
*/ 
#define GCD_CMD_MAX_TOKEN 	4

typedef struct
{
    char  proto[GCC_L_PROTOCOL + 1];            /* Protocol */
    char  host[GCC_L_NODE + 1];                 /* Node */
    char  port[GCC_L_PORT + 1];                 /* Port */
    char  addr[GCC_L_PORT + 1];                 /* Addr */
}GCD_PNP_INFO;

/*
** Server info for admin SHOW SERVER command
*/
typedef struct
{
#define GCD_PNP_LEN             12            /* PCT entries */

   GCD_PNP_INFO  gcd_pnp_info[GCD_PNP_LEN];
   i4   gcd_pnp_info_len;

} GCD_SVR_INFO;

static GCD_SVR_INFO gcd_svr_info ZERO_FILL;
static bool srv_info_init = FALSE;

typedef struct
{
    char  tr_id[5];
    char  tr_classid[32];

}GCD_TRACE_INFO;

static GCD_TRACE_INFO trace_ids[]=
{
    { "GCA",  "exp.gcf.gca.trace_level" },
    { "GCS",  "exp.gcf.gcs.trace_level" },
    { "API",  "exp.aif.api.trace_level" },
};

#define GCDS_INIT      0               /* init */
#define GCDS_WAIT      1               /* wait */
#define GCDS_PROCESS   2               /* process      */
#define GCDS_EXIT      3               /* exit         */
#define GCDS_CNT       4

static  char    *sess_states[ GCDS_CNT ] =
{
    "INIT", "WAIT", "PROCESS",
    "EXIT",
};

/* Duplicate from gcdgcc.c */

#define TLS_IDLE	0
#define TLS_OPEN	1
#define TLS_LSN		2
#define TLS_RECV	3
#define TLS_RECVX	4
#define TLS_SEND	5
#define TLS_SENDX	6
#define TLS_SHUT	7
#define TLS_DISC	8

#define TLS_CNT		9

static char *tls_states[TLS_CNT] =
{
   "IDLE", "OPEN", "LSN", "RECV", "RECVX", "SEND",
   "SENDX" , "SHUT", "DISC"
};

/*
** Forward references
*/

static void		adm_rslt( GCD_SESS_CB *, PTR , char * );
static void		adm_rsltlst( GCD_SESS_CB *, PTR, i4 );
static void		adm_error( GCD_SESS_CB *, PTR,  STATUS );
static void		adm_done( GCD_SESS_CB *, PTR );
static void		adm_stop( GCD_SESS_CB *, PTR, char * );

static void		adm_rqst_cb( PTR );
static void		adm_rslt_cb( PTR );
static void		adm_stop_cb( PTR );
static void		adm_exit_cb( PTR );

static void		error_log( STATUS );
static STATUS           end_session( GCD_SESS_CB *, STATUS );
static i4               build_rsltlst( QUEUE *, char *, i4 );

static void             rqst_show_server( GCD_SESS_CB *, PTR);
static i4 		rqst_show_adm( QUEUE *, PTR, u_i2, i4 ); 
static i4 		rqst_show_pool( QUEUE *, PTR, u_i2, i4 ); 
static i4               rqst_show_sessions( QUEUE *, PTR, u_i2, i4 );
static void             rqst_set_trace( GCD_SESS_CB *, PTR, char *, i4);


static i4 		verify_sid( PTR );
FUNC_EXTERN i4          gcd_sess_info( PTR, i4 );

static void             rqst_sys_stats( GCD_CCB * , char *, i4, char *, i4 );

FUNC_EXTERN VOID 	gcd_get_gca_id( PTR conn_hndl, i4 *gca_assoc_id ); 



/*
** Name: adm_stop_cb
**
** Description:
**	Stop server at end of request.
**
** Input:
**	ptr 	pointer to GCADM_RQST_PARMS. 
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 16-Sept-2003 (wansh01) 
**	    Created.
*/

static void 
adm_stop_cb( PTR ptr )
{

    GCADM_CMPL_PARMS    *rslt_cb = (GCADM_CMPL_PARMS *)ptr;
    GCD_SESS_CB         *sess_cb = (GCD_SESS_CB *)rslt_cb->rslt_cb_parms;
    GCADM_DONE_PARMS    done_parms;
    STATUS              status;
    
    if ( GCD_global.gcd_trace_level >= 4 )  
	TRdisplay( "%4d     GCD ADM stop callback\n", sess_cb->aid );
    
    /*
    ** Complete the request and close the session.
    */
    sess_cb->state = GCDS_EXIT;

    MEfill( sizeof( done_parms ), 0, (PTR)&done_parms );
    done_parms.gcadm_scb = (PTR)rslt_cb->gcadm_scb;
    done_parms.rqst_status = FAIL;
    done_parms.rqst_cb_parms = (PTR)sess_cb;
    done_parms.rqst_cb = (GCADM_RQST_FUNC)adm_exit_cb;

    if ( (status = gcadm_rqst_done( &done_parms )) != OK )
    {
        if ( GCD_global.gcd_trace_level >= 1 )
            TRdisplay( "%4d     GCD ADM gcadm_rqst_done() failed: 0x%x\n",
                       sess_cb->aid, status );
	error_log ( E_GC4821_GCD_ADM_SESS_ERR );
        error_log( status );
    }

    return;

}


/*
** Name: adm_exit_cb
**
** Description:
**	 stop complete callback routine.
**
** Input:
**	ptr 	pointer to GCADM_CMPL_PARMS. 
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 16-Sept-2003 (wansh01) 
**	    Created.
*/

static void 
adm_exit_cb( PTR ptr )
{
    GCADM_RQST_PARMS    *gcadm_rqst = (GCADM_RQST_PARMS *)ptr;
    GCD_SESS_CB         *sess_cb = (GCD_SESS_CB *)gcadm_rqst->rqst_cb_parms;

    if ( GCD_global.gcd_trace_level >= 4 )
        TRdisplay( "%4d     GCD ADM stopping server n", sess_cb->aid );

    end_session( sess_cb, OK );
    GCshut();
    return;


}


/*
** Name: end_session
**
** Description:
**      Free session resources.
**
** Input:
**      sess_cb         Session control block.
**      status          Session status.
**
** Output:
**      None.
**
** Returns:
**      void
**
** History:
**      28-Sep-06 (usha)
**          Created.
**	21-Jul-09 (gordy)
**	    Release user ID dynamic storage.
*/

static STATUS
end_session( GCD_SESS_CB *sess_cb, STATUS status )
{
    if ( status != OK  &&  status != FAIL )
    {
        if ( GCD_global.gcd_trace_level >= 1 )
            TRdisplay( "%4d     GCD ADM gcadm_session() failed: 0x%x\n",
                       sess_cb->aid, status );

	error_log ( E_GC4821_GCD_ADM_SESS_ERR );
        error_log( status );
    }

    if ( sess_cb->user_id != user_default )  MEfree( (PTR)sess_cb->user_id );
    MEfree( (PTR)QUremove( &sess_cb->sess_q ) );
    GCD_global.adm_sess_count--; 
    return;
}


/*
** Name: adm_rslt_cb
**
** Description:
**	 result complete callback routine.
**
** Input:
**	ptr 	pointer to GCADM_CMPL_PARMS. 
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 16-Sept-2003 (wansh01) 
**	    Created.
*/

static void 
adm_rslt_cb( PTR ptr )
{
    GCADM_CMPL_PARMS  	*rslt_cb = (GCADM_CMPL_PARMS *)ptr;
    GCD_SESS_CB		*sess_cb = (GCD_SESS_CB *)rslt_cb->rslt_cb_parms;
    QUEUE               *q;

    if ( GCD_global.gcd_trace_level >= 4 )
	TRdisplay( "%4d     GCD ADM complete cb: 0x%x\n",
		   sess_cb->aid, rslt_cb->rslt_status );

    /*
    ** Empty result list queue and release resources.
    ** (watch out for static memory allocation error result entry)
    */
    while( (q = QUremove( sess_cb->rsltlst_q.q_next )) )  MEfree( (PTR)q );

    if ( rslt_cb->rslt_status == OK )
        adm_done( sess_cb, rslt_cb->gcadm_scb );
    else
        end_session( sess_cb, rslt_cb->rslt_status );

    return;
}


/*
** Name: adm_rqst_cb
**
** Description:
**	request complete callback routine.
**
** Input:
**	ptr 	pointer to GCADM_RQST_PARMS.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 16-Sept-2003 (wansh01) 
**	    Created.
**	30-Sep-05 (gordy)
**	    Adding block around case which declares local variable.
**	09-Sep-08 (usha) Bug 120877
**	    Fixed the user message when setting the trace level for the 
**	    GCD server. I noticed this error while testing dynamic tracing
**	    for DAS server for SD issue 130705. 
*/

static void 
adm_rqst_cb( PTR ptr )
{
    GCADM_RQST_PARMS	*gcadm_rqst = (GCADM_RQST_PARMS *)ptr;
    GCD_SESS_CB		*sess_cb = (GCD_SESS_CB *)gcadm_rqst->rqst_cb_parms;
    char		*token[ GCD_CMD_MAX_TOKEN ];
    ADM_ID		token_ids[ GCD_CMD_MAX_TOKEN ]; 
    ADM_COMMAND		*rqst_cmd;
    char                request_buff[ ER_MAX_LEN ];
    u_i2		token_count;
    u_i2		keyword_count = ARR_SIZE ( keywords );
    u_i2		command_count = ARR_SIZE ( commands );
    i4			sess_flag = 0;

    if ( GCD_global.gcd_trace_level >= 4 )
	TRdisplay( "%4d     GCD ADM request callback: 0x%x\n", 
		   sess_cb->aid, gcadm_rqst->sess_status );

    if (  gcadm_rqst->sess_status != OK )				  
    {
	end_session( sess_cb, gcadm_rqst->sess_status );
	/* if exit the last admin session try to test for shutdown */
	if ( GCD_global.adm_sess_count == 0)  gcd_gca_activate();
	return;
    }

    sess_cb->state = GCDS_PROCESS;

     /*
    ** Prepare and process request text.
    */
    /* EOS request and trim trailing space */  
    gcadm_rqst->request[ gcadm_rqst->length ] = EOS; 
    STtrmwhite ( gcadm_rqst->request );

    token_count = gcu_words ( gcadm_rqst->request, request_buff, token, ' ',
				GCD_CMD_MAX_TOKEN);

    gcadm_keyword ( token_count, token, 
		keyword_count, keywords, token_ids, GCD_TOKEN ); 

    if ( ! gcadm_command( token_count, token_ids, 
			  command_count, commands, &rqst_cmd  ) ) 
    {
        if ( GCD_global.gcd_trace_level >= 5 )
	    TRdisplay( "%4d     GCD ADM gcadm_cmd failed\n", -1 );  
        adm_rslt( sess_cb, gcadm_rqst->gcadm_scb, ADM_BAD_COMMAND_TXT );
        return;
    }

    if ( GCD_global.gcd_trace_level >= 5 )
	TRdisplay( "%4d     GCD ADM command: %d\n", 
		   sess_cb->aid, rqst_cmd->command);  

    switch ( rqst_cmd->command ) 
    {
    case GCD_CMD_HELP:  
    {
	i4 i, max_len;

        for( i = max_len = 0; i < ARR_SIZE( helpText ); i++ )
            max_len = build_rsltlst(&sess_cb->rsltlst_q, helpText[i], max_len);

        adm_rsltlst( sess_cb, gcadm_rqst->gcadm_scb, max_len );
        break;
    }

    case GCD_CMD_SHOW_SVR:
	rqst_show_server( sess_cb, gcadm_rqst->gcadm_scb );
	break;

    case GCD_CMD_SHOW_SVR_LSTN:
	 adm_rslt( sess_cb, gcadm_rqst->gcadm_scb,
             ( GCD_global.flags & GCD_SVR_CLOSED )? "CLOSED" : "OPEN" );
	break;

    case GCD_CMD_SHOW_SVR_SHUTDOWN:
	 adm_rslt( sess_cb, gcadm_rqst->gcadm_scb,
                (GCD_global.flags & GCD_SVR_QUIESCE) ? "PENDING" :
                ((GCD_global.flags & GCD_SVR_SHUT) ?  "STOPPING" : "OPEN"));
	break;

    case GCD_CMD_SET_SVR_OPEN:
	GCD_global.flags &= ~( GCD_SVR_QUIESCE | 
				 GCD_SVR_CLOSED | GCD_SVR_SHUT );
	adm_rslt( sess_cb, gcadm_rqst->gcadm_scb, ADM_SVR_CLSD_TXT );
	break;

    case GCD_CMD_SET_SVR_CLSD:
	GCD_global.flags |= GCD_SVR_CLOSED;  
        adm_rslt( sess_cb, gcadm_rqst->gcadm_scb, ADM_SVR_CLSD_TXT );
	break;

    case GCD_CMD_SET_SVR_SHUT:
	GCD_global.flags |= GCD_SVR_QUIESCE | GCD_SVR_CLOSED; 
	adm_rslt( sess_cb, gcadm_rqst->gcadm_scb, ADM_SVR_SHUT_TXT );
	break;

    case GCD_CMD_STOP_SVR:
	GCD_global.flags |= GCD_SVR_SHUT | GCD_SVR_CLOSED; 
	adm_stop( sess_cb, gcadm_rqst->gcadm_scb, ADM_SVR_STOP_TXT );
	break;

    case GCD_CMD_STOP_SVR_FORCE:
    case GCD_CMD_CRASH_SVR:
	PCexit( FAIL ); 
	break;

    case GCD_CMD_REGISTER_SVR:
    {
	GCADM_SCB * scb; 
	STATUS status; 

	scb = ( GCADM_SCB *)gcadm_rqst->gcadm_scb;

	MEfill( sizeof(scb->parms), 0, (PTR) &(scb->parms));

	scb->parms.gca_rg_parm.gca_srvr_class = GCD_SRV_CLASS;
	scb->parms.gca_rg_parm.gca_l_so_vector = 0;
	scb->parms.gca_rg_parm.gca_served_object_vector = NULL;
	scb->parms.gca_rg_parm.gca_installn_id = "";
	scb->parms.gca_rg_parm.gca_modifiers = GCA_RG_NS_ONLY;
	    
	gca_call( &scb->gca_cb, GCA_REGISTER,
		(GCA_PARMLIST *)&scb->parms,
		GCA_SYNC_FLAG,
		NULL, -1, &status);

	if ( status != OK || (status = scb->parms.gca_rg_parm.gca_status) != OK )
	{
	    gcu_erlog( 0, GCD_global.language, 
		   status, &scb->parms.gca_rg_parm.gca_os_status, 0, NULL );
	    adm_rslt( sess_cb, gcadm_rqst->gcadm_scb, ADM_REG_ERR_TXT );
	}
	else 
	    adm_rslt( sess_cb, gcadm_rqst->gcadm_scb, ADM_SVR_REG_TXT );
	break;
    }

    case GCD_CMD_REMOVE_POOL_SESS:
	if ( gcd_pool_flush( GCD_global.pool_cnt ) ) 
	    adm_rslt( sess_cb, gcadm_rqst->gcadm_scb, ADM_REM_POOL_TXT );
	else 
	    adm_rslt( sess_cb, gcadm_rqst->gcadm_scb, ADM_REM_POOLERR_TXT );
	break;
	  
    case GCD_CMD_REMOVE_SESSID: 
    {
	PTR sid;
	CVaxptr( token[1], &sid ); 
   	sess_flag = verify_sid( sid );
	switch ( sess_flag )  
	{
	case GCD_SESS_USER: 

	    gcd_sess_abort( (GCD_CCB *)sid, E_GC481E_GCD_SESS_ABORT );
	    adm_rslt( sess_cb, gcadm_rqst->gcadm_scb, ADM_REM_TXT );
	    break;	

	case GCD_SESS_SYS: 	
	    adm_rslt( sess_cb, gcadm_rqst->gcadm_scb, ADM_REM_SYSERR_TXT );
	    break;	

	case GCD_SESS_POOL: 	
	
	    gcd_pool_remove( (GCD_CIB *) sid ); 
	    adm_rslt( sess_cb, gcadm_rqst->gcadm_scb, ADM_REM_POOL_TXT );
	    break;	

	case GCD_SESS_ADMIN: 	
	    adm_rslt( sess_cb, gcadm_rqst->gcadm_scb, ADM_REM_ADMERR_TXT );
	    break;	

	default:  
	    adm_rslt( sess_cb, gcadm_rqst->gcadm_scb, ADM_BAD_SID_TXT );	
	}
	break;
    }

    case GCD_CMD_SHOW_ADM_SESS_FMT:
    case GCD_CMD_SHOW_ADM_SESS_STATS:
    case GCD_CMD_SHOW_ADM_SESS:
    case GCD_CMD_FRMT_ADM_SESS:
    {
        i4 max_len = rqst_show_adm( &sess_cb->rsltlst_q,
                                    NULL, rqst_cmd->command, 0 );
        if ( max_len )
            adm_rsltlst( sess_cb, gcadm_rqst->gcadm_scb, max_len );
        else
            adm_done( sess_cb, gcadm_rqst->gcadm_scb );
        break;
    }
    
    case GCD_CMD_SHOW_USER_SESS:
    case GCD_CMD_SHOW_USER_SESS_STATS:
    case GCD_CMD_SHOW_USER_SESS_FMT:
    case GCD_CMD_FRMT_USER_SESS:
    case GCD_CMD_SHOW_SYS_SESS:
    case GCD_CMD_SHOW_SYS_SESS_STATS:
    case GCD_CMD_SHOW_SYS_SESS_FMT:
    case GCD_CMD_FRMT_SYS_SESS:
    {
        i4 max_len = rqst_show_sessions( &sess_cb->rsltlst_q, NULL,
                                         rqst_cmd->command, 0 );

        max_len = rqst_show_pool( &sess_cb->rsltlst_q,
                                 NULL, rqst_cmd->command, max_len );
        if ( max_len )
            adm_rsltlst( sess_cb, gcadm_rqst->gcadm_scb, max_len );
        else
            adm_done( sess_cb, gcadm_rqst->gcadm_scb );
        break;
    }

    case GCD_CMD_SHOW_ALL_SESS:
    case GCD_CMD_SHOW_ALL_SESS_STATS:
    case GCD_CMD_SHOW_ALL_SESS_FMT:
    case GCD_CMD_FRMT_ALL_SESS:
    {
        i4 max_len = 0;

        max_len = rqst_show_sessions( &sess_cb->rsltlst_q, NULL,
                                      rqst_cmd->command, max_len );
        max_len = rqst_show_adm( &sess_cb->rsltlst_q,
                                 NULL, rqst_cmd->command, max_len );
        max_len = rqst_show_pool( &sess_cb->rsltlst_q,
                                 NULL, rqst_cmd->command, max_len );
        if ( max_len )
            adm_rsltlst( sess_cb, gcadm_rqst->gcadm_scb, max_len );
        else
            adm_done( sess_cb, gcadm_rqst->gcadm_scb );
        break;
    }
    case GCD_CMD_FORMAT_SESSID: 
    {
        i4      max_len;
        PTR     sid = NULL;

        CVaxptr( token[1], &sid );  /* Decode session ID */

        if( sid == NULL )
        {
            adm_rslt( sess_cb, gcadm_rqst->gcadm_scb, ADM_BAD_SID_TXT );
            break;
        }

        /*
        ** See if SID matches an admin session.  If not,
        ** no message will be formatted and resulting max
        ** message length will still be zero.
        */
        max_len = rqst_show_adm(&sess_cb->rsltlst_q, sid, rqst_cmd->command, 0);

        /*
        ** If not an admin session, see if user/sys session.
        */
        if ( ! max_len )
            max_len = rqst_show_sessions( &sess_cb->rsltlst_q,
                                          sid, rqst_cmd->command, 0 );

        if ( ! max_len )
            max_len = rqst_show_pool(&sess_cb->rsltlst_q, sid, rqst_cmd->command, 0);
        /*
        ** Return results if SID found.  Otherwise, bad SID error message.
        */
        if ( max_len )
            adm_rsltlst( sess_cb, gcadm_rqst->gcadm_scb, max_len );
        else
            adm_rslt( sess_cb, gcadm_rqst->gcadm_scb, ADM_BAD_SID_TXT );
        break;
    }
    case GCD_CMD_SET_TRACE_LOG_NONE:
     /* Turn off tracing */
    {
        CL_ERR_DESC     err_cl;
        STATUS          cl_stat;

        if( ( cl_stat = TRset_file( TR_F_CLOSE, NULL, 0, &err_cl ) ) == OK )
            adm_rslt( sess_cb, gcadm_rqst->gcadm_scb, ADM_LOG_CLOSE_TXT );
        else if ( cl_stat == TR_BADCLOSE )
            adm_rslt( sess_cb, gcadm_rqst->gcadm_scb, ADM_LOG_CLOSE_TXT );
        else
        {
            STprintf( sess_cb->msg_buff, "Error disabling logging: %0x\n",
                                                                cl_stat);
            adm_rslt( sess_cb, gcadm_rqst->gcadm_scb, sess_cb->msg_buff );
        }
    break;
    }
    case GCD_CMD_SET_TRACE_LOG_FNAME:
     {
        /* Turn on tracing */
        CL_ERR_DESC     err_cl;
        TRset_file( TR_F_CLOSE, NULL, 0, &err_cl );

        if( TRset_file( TR_F_OPEN,
                 token[3], (i4)STlength( token[3] ), &err_cl ) == OK )
            adm_rslt( sess_cb, gcadm_rqst->gcadm_scb, ADM_LOG_OPEN_TXT );
        else
        {
            STprintf( sess_cb->msg_buff,
                         "Error opening log file %s\n",token[3]);
            adm_rslt( sess_cb, gcadm_rqst->gcadm_scb, sess_cb->msg_buff );
        }
    break;
    }

    case GCD_CMD_SET_TRACE_LEVEL:
    {
        i4 trace_lvl;
        /* set gcd_trace_level */
        CVan( token[3], &trace_lvl );
        GCD_global.gcd_trace_level = trace_lvl;
        STprintf( sess_cb->msg_buff, "Tracing GCD at level %d\n", trace_lvl );
        adm_rslt( sess_cb, gcadm_rqst->gcadm_scb, sess_cb->msg_buff );
        break;
    }
    case GCD_CMD_SET_TRACE_ID_LEVEL:
    {
       /* set trace levels of various components( GCA, GCS, API )
       ** for the server. 
       */
        i4 trace_lvl;
        CVan( token[3], &trace_lvl );
        rqst_set_trace( sess_cb, gcadm_rqst->gcadm_scb, token[2], trace_lvl );
        break;
    }
    default:  
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d     GCD invalid command: %d\n", 
			sess_cb->aid, rqst_cmd->command );
	adm_error( sess_cb, gcadm_rqst->gcadm_scb, 
			E_GC4820_GCD_ADM_INIT_ERR );
	break;
    }

    return;
}
 
 
/*
** Name: adm_rslt
**
** Description:
**	Return result message to ADM client.
**
** Input:
**	sess_cb		GCD ADM session control block.
**	scb		GCADM session control block.
**	msg_buff	Result message to be returned.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 16-Nov-2003 (wansh01) 
**	    Created.
*/

static void 
adm_rslt( GCD_SESS_CB *sess_cb, PTR scb, char *msg_buff  )
{
    GCADM_RSLT_PARMS  	rslt_parms;  
    STATUS		status; 

    if ( GCD_global.gcd_trace_level >= 4 )
	TRdisplay( "%4d     GCD ADM return result: '%s'\n", 
		   sess_cb->aid, msg_buff );

    MEfill( sizeof( rslt_parms ), 0, (PTR)&rslt_parms );
    rslt_parms.gcadm_scb = (PTR)scb; 
    rslt_parms.rslt_cb = (GCADM_CMPL_FUNC)adm_rslt_cb;
    rslt_parms.rslt_cb_parms = (PTR)sess_cb; 
    rslt_parms.rslt_len = strlen( msg_buff );
    rslt_parms.rslt_text = msg_buff; 	

    if ( (status = gcadm_rqst_rslt( &rslt_parms )) != OK )
    {
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d     GCD ADM result error: 0x%x\n", 
		       sess_cb->aid, status );

	error_log ( E_GC4821_GCD_ADM_SESS_ERR ); 
	error_log ( status ); 
    }

    return;
}

 
/*
** Name:  adm_error
**
** Description:
**	Return error to ADM client.
**
** Input:
**	sess_cb		GCD ADM session control block.
**	scb		GCADM session control block.
**	error_code	Error code.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 16-Nov-2003 (wansh01) 
**	    Created.
*/

static void
adm_error( GCD_SESS_CB *sess_cb, PTR scb, STATUS error_code )
{
    GCADM_ERROR_PARMS	error_parms;
    STATUS		status; 
    char		*buf, *post;

    if ( GCD_global.gcd_trace_level >= 4 )
	TRdisplay( "%4d     GCD ADM return error: 0x%x\n", 
		   sess_cb->aid, error_code );

    buf = sess_cb->msg_buff;
    post = sess_cb->msg_buff + sizeof(sess_cb->msg_buff);

    /* Fill buffer with error message. */
    gcu_erfmt( &buf, post, 1, error_code, (CL_ERR_DESC *)0, 0, NULL );

    MEfill( sizeof( error_parms ), 0xff, (PTR)&error_parms );
    error_parms.gcadm_scb = (PTR)scb;
    error_parms.rslt_cb = (GCADM_CMPL_FUNC)adm_rslt_cb;
    error_parms.rslt_cb_parms = (PTR)sess_cb;
    error_parms.error_sqlstate = NULL;    
    error_parms.error_code =  error_code;
    error_parms.error_len = strlen( sess_cb->msg_buff ); 
    error_parms.error_text = sess_cb->msg_buff;

    if ( (status = gcadm_rqst_error( &error_parms)) != OK )
    {
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d     GCD ADM gcadm_rqst_error failed: 0x%x\n", 
		       sess_cb->aid, status );

	error_log ( E_GC4821_GCD_ADM_SESS_ERR ); 
	error_log ( status ); 
    }

    return;
}


/*
** Name: adm_done 
**
** Description:
**	Complete processing of an ADM client request, and
**	optionally end the session (status != OK).
**
** Input:
**	sess_cb		GCD ADM Session control block.
**	scb		GCADM Session control block.
**	cb_status	Request status.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 16-Nov-2003 (wansh01) 
*/

static void 
adm_done( GCD_SESS_CB *sess_cb, PTR scb )
{
    GCADM_DONE_PARMS	done_parms;  
    STATUS		status; 

    /*
    ** Complete the request and wait for next.
    */
    sess_cb->state = GCDS_WAIT;

    MEfill( sizeof( done_parms ), 0, (PTR)&done_parms );
    done_parms.gcadm_scb = (PTR)scb;
    done_parms.rqst_cb = (GCADM_RQST_FUNC)adm_rqst_cb;
    done_parms.rqst_cb_parms = (PTR)sess_cb;
    done_parms.rqst_status = OK;

    if ( (status = gcadm_rqst_done( &done_parms )) != OK )
    {
        if ( GCD_global.gcd_trace_level >= 1 )
            TRdisplay( "%4d     GCD ADM gcadm_rqst_done() failed: 0x%x\n",
                       sess_cb->aid, status );

	error_log ( E_GC4821_GCD_ADM_SESS_ERR );
        error_log ( status );
    }

    return;

}

/*
** Name: error_log
**
** Description:
**	function to log errors.
**
** Input:
**	error_code	Error code.
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	25-Mar-2004 ( wansh01 ) 
**	    Created.
*/

static VOID
error_log( STATUS error_code )
{
    gcu_erlog( 0, 1, error_code, NULL, 0, NULL );
    return;
}

/*
** Name: gcd_adm_session
**
** Description:
**	 gcadm session. 
**
** Input:
**	aid     Association ID. 
**	gca_cb	GCA control block.
**	user	Client user ID.
**
** Output:
**	void 
**
** Returns:
**	status 
**
** History:
**	 16-Nov-2003 (wansh01) 
**	    Created.
**	21-Jul-09 (gordy)
**	    Dynamically allocate storage for user ID.
*/

STATUS  
gcd_adm_session( i4 aid, PTR gca_cb, char *user )
{
    GCD_SESS_CB		*sess_cb;
    GCADM_SESS_PARMS    gcadm_sess;
    STATUS		status; 

    if ( GCD_global.gcd_trace_level >= 4 )
	TRdisplay( "%4d   GCD ADM New session: %s\n", aid, user );

    sess_cb = (GCD_SESS_CB *)MEreqmem(0, sizeof(GCD_SESS_CB), TRUE, &status);
    if ( sess_cb == NULL )
    {
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d     GCD ADM mem alloc failed: 0x%x\n", 
		       aid, status );
	error_log( E_GC4808_NO_MEMORY  );
	return( E_GC4808_NO_MEMORY );
    }

    sess_cb->aid = aid;
    sess_cb->user_id = STalloc( user );
    if ( ! sess_cb->user_id )  sess_cb->user_id = user_default;
    QUinit( &sess_cb->rsltlst_q );
    sess_cb->state = GCDS_INIT;

    QUinsert( &sess_cb->sess_q, &gcd_sess_q );
    GCD_global.adm_sess_count++; 

    MEfill( sizeof( gcadm_sess ), 0, (PTR)&gcadm_sess );
    gcadm_sess.assoc_id = aid; 
    gcadm_sess.gca_cb   = gca_cb;
    gcadm_sess.rqst_cb  = (GCADM_RQST_FUNC )adm_rqst_cb;
    gcadm_sess.rqst_cb_parms = (PTR)sess_cb;


    if( !srv_info_init )
    {
        GCD_PIB * pib, *next_pib;
        int i = 0;

        if ( GCD_global.gcd_trace_level >= 4 )
            TRdisplay( "%4d     GCD ADM : Initializing server info \n");

        gcd_svr_info.gcd_pnp_info_len = 0;

        for( next_pib = (GCD_PIB *)GCD_global.pib_q.q_next;
              (PTR)next_pib  != (PTR)(&GCD_global.pib_q);)
        {
            i = gcd_svr_info.gcd_pnp_info_len;
            pib = (GCD_PIB * ) next_pib;
            next_pib = (GCD_PIB * )pib->q.q_next;

            STlcopy( pib->pid, gcd_svr_info.gcd_pnp_info[i].proto,
                        sizeof(gcd_svr_info.gcd_pnp_info[i].proto) - 1 );
            STlcopy( pib->host, gcd_svr_info.gcd_pnp_info[i].host,
                        sizeof(gcd_svr_info.gcd_pnp_info[i].host) - 1 );
            STlcopy( pib->port, gcd_svr_info.gcd_pnp_info[i].port,
                        sizeof(gcd_svr_info.gcd_pnp_info[i].port) - 1 );
            STlcopy( pib->addr, gcd_svr_info.gcd_pnp_info[i].addr,
                        sizeof(gcd_svr_info.gcd_pnp_info[i].addr) - 1 );

            ++gcd_svr_info.gcd_pnp_info_len;
        }

        srv_info_init = TRUE;
    }

    if ( (status = gcadm_session( &gcadm_sess )) != OK )
    {
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d     GCD ADM gcadm_session() failed: 0x%x\n", 
		       aid, status );

	end_session(sess_cb, status );
	status = E_GC4821_GCD_ADM_SESS_ERR;
    }

    return( status );

}


/*
** Name: gcd_adm_int  
**
** Description:
**	init gcadm. 
**
** Input:
**	void 
**
** Output:
**	void 
**
** Returns:
**	void
**
** History:
**	 16-Nov-2003 (wansh01) 
*/

STATUS  
gcd_adm_init ()
{
    GCADM_INIT_PARMS gcadm_init_parms;
    STATUS  status; 

    if ( GCD_global.gcd_trace_level >= 4 )
	TRdisplay( "%4d   GCD ADM Initialize\n", -1 );

    QUinit( &gcd_sess_q );
    MEfill( sizeof( gcadm_init_parms ), 0, (PTR)&gcadm_init_parms );
    gcadm_init_parms.alloc_rtn    = NULL;
    gcadm_init_parms.dealloc_rtn  = NULL;
    gcadm_init_parms.errlog_rtn   = error_log;
    gcadm_init_parms.msg_buff_len = GCD_MSG_BUFF_LEN;

    if ( (status =  gcadm_init( &gcadm_init_parms )) != OK )
    {
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d     GCD ADM gcadm_init() failed: 0x%x\n", 
		       -1, status );

	error_log ( E_GC4820_GCD_ADM_INIT_ERR ); 
	error_log ( status ); 
	status = E_GC4820_GCD_ADM_INIT_ERR;
    }	

    return ( status ); 
}


/*
** Name: gcd_adm_term
**
** Description:
**	term gcadm. 
**
** Input:
**	void 
**
** Output:
**	void 
**
** Returns:
**	status 
**
** History:
**	 16-Nov-2003 (wansh01) 
*/

STATUS  
gcd_adm_term ()
{
    STATUS  status; 
    
    if ( GCD_global.gcd_trace_level >= 4 )
	TRdisplay( "%4d   GCD ADM Terminate\n", -1 );

    if ( (status = gcadm_term()) != OK )
    {
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d     GCD ADM gcadm_term() failed: 0x%x\n", 
		       -1, status );
    }

    return( status ); 
}


/*
** Name: adm_stop
**
** Description:
**	Return message to ADM client and stop server.
**
** Input:
**	sess_cb		GCD ADM Session control block.
**	scb		GCADM Session control block.
**	msg_buff	Result message.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 16-Nov-2003 (wansh01) 
**	    Created.
*/

static void 
adm_stop( GCD_SESS_CB *sess_cb, PTR scb, char * msg_buff )
{
    GCADM_RSLT_PARMS	rslt_parms;  
    STATUS		status; 

    if ( GCD_global.gcd_trace_level >= 4 )
	TRdisplay( "%4d     GCD ADM stopping server: '%s'\n", 
		   sess_cb->aid, msg_buff);

    MEfill( sizeof( rslt_parms ), 0, (PTR)&rslt_parms );
    rslt_parms.gcadm_scb = (PTR)scb; 
    rslt_parms.rslt_cb = (GCADM_CMPL_FUNC)adm_stop_cb;
    rslt_parms.rslt_cb_parms = (PTR)sess_cb; 
    rslt_parms.rslt_len = strlen( msg_buff );
    rslt_parms.rslt_text = msg_buff; 	

    if ( (status = gcadm_rqst_rslt( &rslt_parms )) != OK )
    {
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d     GCD ADM gcadm_rqst_rslt() failed: 0x%x\n", 
		       sess_cb->aid, status );
	error_log ( E_GC4821_GCD_ADM_SESS_ERR ); 
	error_log ( status ); 
    }

    return;
}

/*
** Name: adm_rsltlst
**
** Description:
**	Return a list of messages to ADM client.
**
** Input:
**	sess_cb	GCD ADM Session control block.
**	scb	GCADM Session control block.
**	i4  	Max message length.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 08-Jul-2004 (wansh01) 
*/

static void 
adm_rsltlst( GCD_SESS_CB *sess_cb, PTR scb, i4 lst_maxlen )
{
    GCADM_RSLTLST_PARMS	rsltlst_parms; 
    STATUS		status; 

    if ( GCD_global.gcd_trace_level >= 4 )
	TRdisplay( "%4d     GCD ADM return list: %d\n", 
		   sess_cb->aid, lst_maxlen );

    MEfill( sizeof( rsltlst_parms ), 0, (PTR)&rsltlst_parms );
    rsltlst_parms.gcadm_scb = (PTR)scb; 
    rsltlst_parms.rslt_cb = (GCADM_CMPL_FUNC)adm_rslt_cb;
    rsltlst_parms.rslt_cb_parms = (PTR)sess_cb;
    rsltlst_parms.rsltlst_max_len = lst_maxlen; 
    rsltlst_parms.rsltlst_q  = &sess_cb->rsltlst_q; 
	 

    if ( (status = gcadm_rqst_rsltlst( &rsltlst_parms )) != OK )
    {
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d     GCD ADM gcadm_rqst_rsltlst() failed: 0x%x\n", 
		       sess_cb->aid, status );

	error_log ( E_GC4821_GCD_ADM_SESS_ERR ); 
	error_log ( status ); 
    }

    return; 
}


/*
** Name: rqst_show_adm
**
** Description:
**      Display ADMIN session info.  Adds entry to result
**      list queue.  If session ID is provided, only info
**      for matching session is displayed.  Formatting is
**      dependent on command type.  Updates max message
**      length.
**
** Input:
**      rsltlst         Result list QUEUE.
**      sid             Session ID, may be NULL.
**      cmd             Command type.
**      max_len         Max message length.
**
** Output:
**      None.
**
** Returns:
**      i4              Max message length.
**
** History:
**	08-Jul-04 (wansh01) 
**	    Created.
**	 6-Aug-04 (gordy)
**	    Enhanced session info display.
**	29-Sep-06 (rajus01)
**	     Re-worked on the routine based ong Gordy's changes for GCN.
*/

static i4 
rqst_show_adm( QUEUE *rsltlst, PTR sid, u_i2 cmd, i4 max_len )
{
    QUEUE	*q;

    for( q = gcd_sess_q.q_next; q != &gcd_sess_q; q = q->q_next )
    {
        GCD_SESS_CB *sess_cb = (GCD_SESS_CB *)q;

        if ( sid  &&  sid != (PTR)sess_cb )  continue;

        /*
        ** Format and return session info heading.
        */
        switch( cmd )
        {
        case GCD_CMD_FRMT_ADM_SESS:
        case GCD_CMD_FRMT_ALL_SESS:
        case GCD_CMD_FORMAT_SESSID:
            STprintf( sess_cb->msg_buff,
                      ">>>>>Session %p:%d<<<<<", sess_cb, sess_cb->aid );
            break;

        case GCD_CMD_SHOW_ADM_SESS_STATS:
        case GCD_CMD_SHOW_ALL_SESS_STATS:
            /* !!! TODO: format stats - fall through for now */

        default :
            STprintf( sess_cb->msg_buff,
                      "Session %p:%d (%-24s) Type: ADMIN state: %s",
                      sess_cb, sess_cb->aid, sess_cb->user_id,
                      sess_states[ sess_cb->state ] );
            break;
        }

        max_len = build_rsltlst( rsltlst, sess_cb->msg_buff, max_len );

        /*
        ** Return FORMATTED info
        */
	switch( cmd )
        {
        case GCD_CMD_SHOW_ADM_SESS_FMT:
        case GCD_CMD_SHOW_ALL_SESS_FMT:
        case GCD_CMD_FRMT_ADM_SESS:
        case GCD_CMD_FRMT_ALL_SESS:
        case GCD_CMD_FORMAT_SESSID:
            STprintf( sess_cb->msg_buff,
             "    No additional format info available for this admin session" );
            max_len = build_rsltlst( rsltlst, sess_cb->msg_buff, max_len );

            break;
        }


        if ( sid )  break;
     }

    return( max_len );
}

/*
** Name: rqst_show_pool
**
** Description:
**      Display POOL session info.  Adds entry to result
**      list queue.  If session ID is provided, only info
**      for matching session is displayed.  Formatting is
**      dependent on command type.  Updates max message
**      length.
**
** Input:
**      rsltlst         Result list QUEUE.
**      sid             Session ID, may be NULL.
**      cmd             Command type.
**      max_len         Max message length.
**
** Output:
**      None.
**
** Returns:
**      i4              Max message length.
**
** History:
**	29-Sep-06 (rajus01)
**	    Created.
**	21-Jul-09 (gordy)
**	    Release session info dynamic resources.
*/

static i4 
rqst_show_pool( QUEUE *rsltlst, PTR sid, u_i2 cmd, i4 max_len )
{
    GCD_SESS_INFO       *info, *iptr = NULL;
    char                msg_buff[ER_MAX_LEN ];
    i4                  count, cnt_flag = 0, info_flag = 0;

    switch( cmd )
    {
    case GCD_CMD_SHOW_ALL_SESS:
    case GCD_CMD_SHOW_SYS_SESS:
    case GCD_CMD_SHOW_ALL_SESS_STATS:
    case GCD_CMD_SHOW_ALL_SESS_FMT:
    case GCD_CMD_SHOW_SYS_SESS_FMT:
    case GCD_CMD_FRMT_ALL_SESS:
    case GCD_CMD_FRMT_SYS_SESS:
    case GCD_CMD_FORMAT_SESSID:
        cnt_flag = GCD_POOL_COUNT;
        info_flag = GCD_POOL_INFO;
        break;
    case GCD_CMD_SHOW_USER_SESS:
    case GCD_CMD_SHOW_USER_SESS_FMT:
    case GCD_CMD_SHOW_USER_SESS_STATS:
    case GCD_CMD_FRMT_USER_SESS:
        cnt_flag = -1;
    }

    if ( cnt_flag < 0 )  return( max_len );

    if ( ! (count = gcd_pool_info( NULL, cnt_flag )) )
        return( max_len );

    TRdisplay("rqst_show_pool: Call MEreqmem\n" );
    iptr = (GCD_SESS_INFO *)MEreqmem( 0, count * sizeof( GCD_SESS_INFO ),
                                      TRUE, NULL);

    TRdisplay("rqst_show_pool: Call gcc_sess_info again\n" );

    /*
    ** Now load and display active session info.
    */
    count = iptr ? gcd_pool_info( (PTR)iptr, info_flag ) : 0;

    for( info = iptr; count > 0; count--, info++ )
    {
        if ( sid  &&  sid != info->sess_id )  continue;
        /*
        ** Format and return session info heading.
        */
        switch( cmd )
        {
        case GCD_CMD_FRMT_ALL_SESS:
        case GCD_CMD_FRMT_SYS_SESS:
        case GCD_CMD_FORMAT_SESSID:
            STprintf( msg_buff,
                      ">>>>>Session %p:%d<<<<<", info->sess_id, info->assoc_id );
            break;

        case GCD_CMD_SHOW_ALL_SESS_STATS:
        case GCD_CMD_SHOW_SYS_SESS_STATS:
	    STprintf( msg_buff, "Session %p:%d (%-24s) Type: %s state: %s DB: %s LOCAL: %s AUTOCOMMIT: %s", info->sess_id, info->assoc_id,  info->user_id, "SYSTEM", info->state, info->database, info->isLocal?"'yes'":"'no'", info->autocommit?"'on'":"'off'" ); 
	    break;
        case GCD_CMD_SHOW_SYS_SESS:
        case GCD_CMD_SHOW_SYS_SESS_FMT:
        case GCD_CMD_SHOW_ALL_SESS:
        case GCD_CMD_SHOW_ALL_SESS_FMT:
            STprintf( msg_buff,
                     "Session %p:%d (%-24s) Type: SYSTEM state: %s",
                     info->sess_id, info->assoc_id, info->user_id,
                     info->state );
            break;
        default :
            break;
        }

        max_len = build_rsltlst( rsltlst, msg_buff, max_len );

        /*
        ** Return FORMATTED info
        */
	switch( cmd )
        {
        case GCD_CMD_SHOW_ALL_SESS_FMT:
        case GCD_CMD_SHOW_SYS_SESS_FMT:
        case GCD_CMD_FRMT_ALL_SESS:
        case GCD_CMD_FRMT_SYS_SESS:
        case GCD_CMD_FORMAT_SESSID:
	    if( STlength( info->database ) > 0 )
            {
	        STprintf( msg_buff,"Database: %s" , info->database ); 
                max_len = build_rsltlst( rsltlst, msg_buff, max_len );
	    }
            break;
        }


        if ( sid )  break;
    }

    if ( iptr )  MEfree( (PTR)iptr );
    return( max_len );
}


/*
** Name: rqst_show_sessions
**
** Description:
**  Display usr/sys session info.  Adds entry to result
**  list queue.  If session ID is provided, only info
**  for matching session is displayed.  Formatting is
**  dependent on command type.  Updates max message
**  length.
**
** Input:
**      rsltlst         Result list QUEUE.
**      sid             Session ID, may be NULL.
**      cmd             Command type.
**      max_len         Max message length.
**
**
** Output:
**      None.
**
** Returns:
**      i4      Max message length.
**
** History:
**      08-Jul-04 (wansh01)
**          Created.
**       6-Aug-04 (gordy)
**          Enhanced session info display.
*/

static i4
rqst_show_sessions( QUEUE *rsltlst, PTR sid, u_i2 cmd, i4 max_len )
{
    GCD_SESS_INFO       *info, *iptr = NULL;
    char                msg_buff[ER_MAX_LEN ];
    i4                  count, cnt_flag, info_flag;

    /*
    ** First determine number of active sessions.
    */
    switch( cmd )
    {
    case GCD_CMD_SHOW_USER_SESS:
    case GCD_CMD_SHOW_USER_SESS_STATS:
    case GCD_CMD_SHOW_USER_SESS_FMT:
    case GCD_CMD_FRMT_USER_SESS:
        cnt_flag = GCD_USER_COUNT;
        info_flag = GCD_USER_INFO;
        break;

    case GCD_CMD_SHOW_SYS_SESS:
    case GCD_CMD_SHOW_SYS_SESS_STATS:
    case GCD_CMD_SHOW_SYS_SESS_FMT:
    case GCD_CMD_FRMT_SYS_SESS:
        cnt_flag = GCD_SYS_COUNT;
        info_flag = GCD_SYS_INFO;
        break;

    case GCD_CMD_SHOW_ALL_SESS:
    case GCD_CMD_SHOW_ALL_SESS_STATS:
    case GCD_CMD_SHOW_ALL_SESS_FMT:
    case GCD_CMD_FRMT_ALL_SESS:
    case GCD_CMD_FORMAT_SESSID:
        cnt_flag = GCD_ALL_COUNT;
        info_flag = GCD_ALL_INFO;
        break;
    }

    if ( ! (count = gcd_sess_info( NULL, cnt_flag )) )
    {
        STprintf( msg_buff, "No user sessions" );
        max_len = build_rsltlst( rsltlst, msg_buff, max_len );
        return( max_len );
    }

    TRdisplay("rqst_show_sessions: Call MEreqmem\n" );
    iptr = (GCD_SESS_INFO *)MEreqmem( 0, count * sizeof( GCD_SESS_INFO ),
                                      TRUE, NULL);

    TRdisplay("rqst_show_sessions: Call gcd_sess_info again\n" );

    /*
    ** Now load and display active session info.
    */
    if ( ! iptr  ||  ! (count = gcd_sess_info( (PTR)iptr, info_flag )) ) 
	return( max_len );

    for( info = iptr; count > 0; count--, info++ )
    {
        if ( sid  &&  sid != info->sess_id )  continue;

        /*
        ** Format and return session info heading.
        */
        switch( cmd )
        {
        case GCD_CMD_FRMT_USER_SESS:
        case GCD_CMD_FRMT_SYS_SESS:
        case GCD_CMD_FRMT_ALL_SESS:
        case GCD_CMD_FORMAT_SESSID:
            if ( info->assoc_id < 0 )
                STprintf( msg_buff, ">>>>>Session %p<<<<<", info->sess_id );
            else
                STprintf( msg_buff, ">>>>>Session %p:%d<<<<<",
                          info->sess_id, info->assoc_id );
            break;

        case GCD_CMD_SHOW_SYS_SESS_STATS:
        case GCD_CMD_SHOW_ALL_SESS_STATS:
        case GCD_CMD_SHOW_USER_SESS_STATS:
        {
	    GCD_CCB *ccb = (GCD_CCB *)info->sess_id;
            if( info->is_system )
                STprintf( msg_buff,
                  "Session %p  (%-24s) Type: SYSTEM  state: %s, Server Class: %s GCA Addr: %s", info->sess_id, info->user_id, info->state, info->srv_class,
		  info->gca_addr );
            else
	       STprintf( msg_buff,
		"Session %p:%d (%-20s) Type: USER  state: %s, MSGIN: %d MSGOUT: %d DTIN: %dKb DTOUT: %dKb", ccb, info->assoc_id, ccb->cib->username, info->state, ccb->gcc.gcd_msgs_in, ccb->gcc.gcd_msgs_out, ccb->gcc.gcd_data_in, ccb->gcc.gcd_data_out );
            break;
        }
        default:
            if( info->is_system )
            {
                if ( info->assoc_id <= 0 )
                    STprintf( msg_buff, "Session %p (%-24s) Type: %s state: %s",
                          info->sess_id, info->user_id, "SYSTEM", info->state );
                else
                    STprintf( msg_buff,
                          "Session %p:%d (%-24s) Type: %s state: %s",
                          info->sess_id, info->assoc_id, info->user_id,
                          "SYSTEM", info->state );
            }
            else
                STprintf( msg_buff,
                  "Session %p:%d (%-24s) Type: %s state: %s",
                  info->sess_id, info->assoc_id, info->user_id,
                  "USER", info->state );

            break;
        }

        max_len = build_rsltlst( rsltlst, msg_buff, max_len );

        /*
        ** Return FORMATTED info
        */
        switch( cmd )
        {
        case GCD_CMD_SHOW_USER_SESS_FMT:
        case GCD_CMD_FRMT_USER_SESS:
        case GCD_CMD_SHOW_SYS_SESS_FMT:
        case GCD_CMD_FRMT_SYS_SESS:
        case GCD_CMD_SHOW_ALL_SESS_FMT:
        case GCD_CMD_FRMT_ALL_SESS:
        case GCD_CMD_FORMAT_SESSID:
        {
    	    GCD_CCB		*ccb;
	    char		conType[15];
	    ccb = (GCD_CCB *)info->sess_id;

            if ( ccb->gcc.pib->pid[0] )   	
    	    {
	        STprintf( msg_buff,
		    " Protocol: %s, Host:  %s,  Addr: %s,  Port: %s",
	            ccb->gcc.pib->pid,ccb->gcc.pib->host, ccb->gcc.pib->addr,
	            ccb->gcc.pib->port);
                max_len = build_rsltlst( rsltlst, msg_buff, max_len );
            }
	    
            if( ccb->cib != NULL )
            {
                if( STlength(ccb->cib->database) > 0 )
                {
                    STprintf( msg_buff," Database: %s ", ccb->cib->database );
                    max_len = build_rsltlst( rsltlst, msg_buff, max_len );
                }

	        if  ( ccb->cib->isLocal )
                    if ( ccb->msg.dtmc )
                        STcopy("Local DTMC", conType);
                    else
                        STcopy("Local", conType);
    	        else
        	    if ( ccb->msg.dtmc )
            	        STcopy("Remote DTMC", conType);
        	    else
            	        STcopy("Remote", conType);

                STprintf( msg_buff," Connection type: %s ", conType );	
                max_len = build_rsltlst( rsltlst, msg_buff, max_len );
                STprintf( msg_buff,
		    " Client username: %s,  host: %s,  address: %s",
                    ccb->client_user, ccb->client_host, ccb->client_addr);
                max_len = build_rsltlst( rsltlst, msg_buff, max_len );
	        STprintf(msg_buff,
                    " Network protocol: %s, node: %s,  port: %s",
                    ccb->gcc.rmt_addr.protocol, ccb->gcc.rmt_addr.node_id,
                ccb->gcc.rmt_addr.port_id);
                max_len = build_rsltlst( rsltlst, msg_buff, max_len );
	    }
	}
        break;
        }

        if ( sid )  break;
    }

    MEfree( (PTR)iptr );
    return( max_len );
}
/*
** Name: build_rsltlst
**
** Description:
**      Add entry to result list queue.
**
** Input:
**      rsltlst_q       Result list queue.
**      msg_buff        Result message buffer.
**      max_len         Maximum prior message length.
**
** Output:
**	None.
**
** Returns:
**	i4	Updated maximum messge length.	
**
** History:
**	 08-Jul-04 (wansh01) 
*/

static i4    
build_rsltlst( QUEUE *rsltlst_q, char *msg_buff, i4 max_len )
{
    ADM_RSLT    *rslt;
    i4          msg_len = strlen( msg_buff );

    /*
    ** Allocate result list entry with space for message text.
    */
    if ( ! (rslt = (ADM_RSLT *)MEreqmem( 0, sizeof( ADM_RSLT ) + msg_len,
                                         0, NULL )) )
    {
        if ( GCD_global.gcd_trace_level >= 1 )
            TRdisplay( "build_rsltlst entry: memory alloc failed\n" );
    }
    else
    {
        if ( GCD_global.gcd_trace_level >= 1 )
            TRdisplay( "build_rsltlst entry: %p, %d, '%s'\n",
                        rslt, msg_len, msg_buff );

        rslt->rslt.rslt_len = msg_len;
        rslt->rslt.rslt_text = rslt->msg_buff;
        STcopy( msg_buff, rslt->msg_buff );
    }

    QUinsert( &(rslt->rslt.rslt_q), rsltlst_q->q_prev );
    return( max( msg_len, max_len ) );

}

/*
** Name: gcd_sess_info
**
** Description:
**	calculate user sessions / system sessions 
**
** Input:
**	info	GCD_SESS_INFO
**	flag 	flag to indicate user/system/all sessions 
**
** Output:
**	None.
**
** Returns:
**	count
**
** History:
**	 08-Jul-04 (wansh01) 
**	    Created.
**	21-Jul-09 (gordy)
**	    User ID and database are now pointer references.
*/

i4  
gcd_sess_info( PTR info, i4 flag ) 
{
    GCD_SESS_INFO	*sess = ( GCD_SESS_INFO * )info;
    i4 			count=0;
    GCD_CCB     	*ccb, *next_ccb;
    bool		selected = FALSE, is_system = FALSE;

    if ( GCD_global.gcd_trace_level >= 4 )
    	TRdisplay (" gcd_sess_info entry flag=%d\n", flag);

    for (
	  next_ccb = (GCD_CCB *)GCD_global.ccb_q.q_next;
	  (PTR)next_ccb != (PTR)(&GCD_global.ccb_q );
	)
    {
	ccb  =  (GCD_CCB  *)next_ccb;
	next_ccb = (GCD_CCB * )ccb->q.q_next;
	
	switch( flag )
        {
	    case GCD_SYS_COUNT:
	        if (!( ccb->gcc.flags & GCD_GCC_CONN ) )
                {
                    count++;
                    is_system = TRUE;
                }
            break;
            case GCD_SYS_INFO:
	        if (!( ccb->gcc.flags & GCD_GCC_CONN ) )
                {
                    selected = TRUE;
                    count++;
                    is_system = TRUE;
                }
            break;
            case GCD_USER_COUNT:
	        if (( ccb->gcc.flags & GCD_GCC_CONN ) ) 
                    count++;
            break;
            case GCD_USER_INFO:
	        if (( ccb->gcc.flags & GCD_GCC_CONN ) ) 
                {
                    selected = TRUE;
                    count++;
                }
             break;
             case GCD_ALL_COUNT:
	        if (!( ccb->gcc.flags & GCD_GCC_CONN ) )
                    is_system = TRUE;
                count++;
                break;
              case GCD_ALL_INFO:
	        if (!( ccb->gcc.flags & GCD_GCC_CONN ) )
                    is_system = TRUE;
                selected = TRUE;
                count++;
              break;
	}

        if( selected )
        {
	     i4		gca_id = 0; 
	     GCD_CIB	*cib = ccb->cib;

	     sess->sess_id = (PTR) ccb;
	     sess->user_id = unknown;
	     sess->database = unknown;
             sess->is_system = is_system;

             if( sess->is_system )
             {
                 rqst_sys_stats( ccb, sess->srv_class, sizeof(sess->srv_class),
				 sess->gca_addr, sizeof( sess->gca_addr ) );
                 STcopy( tls_states[ccb->gcc.state], sess->state );
             }

	     if ( (ccb->msg.state == MSG_S_PROC)  || 
	    	  (ccb->msg.state == MSG_S_READY)
	   	) 
	         STprintf( sess->state, "%s i/o: %s",
		     sess_states[ ccb->msg.state ], 
		     (ccb->msg.flags & GCD_MSG_ACTIVE) ? "DBMS" : "CLIENT" );
	     else 
	         STprintf( sess->state, "%s ", sess_states[ ccb->msg.state ] ); 

	     if ( cib && ( cib->conn ) )
             {
	         gcd_get_gca_id( ccb->cib->conn, &gca_id );
                 sess->assoc_id = gca_id;

		 if ( cib->username )  sess->user_id = cib->username;
		 if ( cib->database )  sess->database = cib->database;
             }

             sess++;
        }

        is_system = FALSE;
	selected = FALSE;
    }

     if ( GCD_global.gcd_trace_level >= 4 )
        TRdisplay (" gcd_sess_info count =%d\n", count);

        return ( count );
}
/*
** Name: verify_sid
**
** Description:
**	verify user provided isession id.
**
** Input:
**	i4 	session id 
**
** Output:
**	None.
**
** Returns:
**	boolean  sid_found 
**
** History:
**	 08-Sept-04 (wansh01) 
*/

static i4  
verify_sid( PTR  sid )
{
    GCD_CCB     *ccb, *next_ccb;
    i4  	sid_sess; 


	if ( GCD_global.gcd_trace_level >= 4 )
	    TRdisplay (" verify_sid entry sid =%p\n", sid );

	sid_sess = 0; 
	for (
	  next_ccb = (GCD_CCB *)GCD_global.ccb_q.q_next;
	  (PTR)next_ccb != (PTR)(&GCD_global.ccb_q );
	)
	{
	    ccb  =  (GCD_CCB  *)next_ccb;
	    next_ccb = (GCD_CCB * )ccb->q.q_next;
	    if ( ccb == (GCD_CCB *)sid )   
	    {
		if ( ccb->gcc.flags & GCD_GCC_CONN )    
		    sid_sess = GCD_SESS_USER; 
		else 
		    sid_sess = GCD_SESS_SYS; 
		break; 
	    }
	}

	if ( sid_sess == 0 && sid != NULL ) 
	     sid_sess = GCD_SESS_ADMIN;
	else
	     sid_sess = GCD_SESS_POOL;

	if ( GCD_global.gcd_trace_level >= 4 )
	    TRdisplay ("verify_sid  sid_sess =%x\n", sid_sess );
	
	return ( sid_sess );
}

/*
** Name: rqst_show_server
**
** Description:
**	show server information 
**
** Input:
**      sess_cb         GCD ADM session control block.
**      scb             GCADM session control block.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	08-Jul-04 (wansh01) 
**	 5-Dec-07 (gordy)
**	    Minor changes to client connection metrics.
**      22-Apr-10 (rajus01) SIR 123621
	    Display process ID.
*/

static void 
rqst_show_server( GCD_SESS_CB *sess_cb, PTR scb )
{
    int i=0;
    i4  max_len = 0;
    PID proc_id;

    PCpid( &proc_id );
    STprintf( sess_cb->msg_buff,"     Server:     %s " ,GCD_global.gcd_lcl_id );
    max_len = build_rsltlst( &sess_cb->rsltlst_q, sess_cb->msg_buff, max_len );

    STprintf( sess_cb->msg_buff,"     Class:      %s " , GCD_SRV_CLASS );
    max_len = build_rsltlst( &sess_cb->rsltlst_q, sess_cb->msg_buff, max_len );

    STprintf( sess_cb->msg_buff,"     Object:     %s " , "*" );
    max_len = build_rsltlst( &sess_cb->rsltlst_q, sess_cb->msg_buff, max_len );

    STprintf( sess_cb->msg_buff,"     Pid:        %d " , proc_id );
    max_len = build_rsltlst( &sess_cb->rsltlst_q, sess_cb->msg_buff, max_len );

    STprintf( sess_cb->msg_buff," " ); 
    max_len = build_rsltlst( &sess_cb->rsltlst_q, sess_cb->msg_buff, max_len );

    STprintf( sess_cb->msg_buff,
		" Connected Sessions ( includes system and admin sessions ) " );
    max_len = build_rsltlst( &sess_cb->rsltlst_q, sess_cb->msg_buff, max_len );

    STprintf( sess_cb->msg_buff,"     Current:    %d", 
		GCD_global.adm_sess_count + gcd_sess_info(NULL, GCD_SYS_COUNT ));
    max_len = build_rsltlst( &sess_cb->rsltlst_q, sess_cb->msg_buff, max_len );

    STprintf( sess_cb->msg_buff,"     Pooled:     %d", GCD_global.pool_cnt ); 
    max_len = build_rsltlst( &sess_cb->rsltlst_q, sess_cb->msg_buff, max_len );

    if ( GCD_global.pool_max == -1 ) 
    	STprintf( sess_cb->msg_buff,"     Pool Limit: %s ", "none"); 
    else 
    if ( GCD_global.pool_max == 0 ) 
    	STprintf( sess_cb->msg_buff,"     Pool Limit: %s ", "disabled"); 
    else 
    	STprintf( sess_cb->msg_buff,
		"     Pool Limit: %d ", GCD_global.pool_max); 
    max_len = build_rsltlst( &sess_cb->rsltlst_q, sess_cb->msg_buff, max_len );

    STprintf( sess_cb->msg_buff," " ); 
    max_len = build_rsltlst( &sess_cb->rsltlst_q, sess_cb->msg_buff, max_len );

    STprintf( sess_cb->msg_buff," Active Sessions  ( includes user sessions only ) " );
    max_len = build_rsltlst( &sess_cb->rsltlst_q, sess_cb->msg_buff, max_len );

    STprintf( sess_cb->msg_buff,
    	    "     Current:    %d ", GCD_global.client_cnt ); 
    max_len = build_rsltlst( &sess_cb->rsltlst_q, sess_cb->msg_buff, max_len );

    if ( GCD_global.client_max <= 0 ) 
    	STprintf( sess_cb->msg_buff,"     Limit:      %s ", "none"); 
    else 
    	STprintf( sess_cb->msg_buff,
	    "     Limit:      %d ", GCD_global.client_max); 
    max_len = build_rsltlst( &sess_cb->rsltlst_q, sess_cb->msg_buff, max_len );

    STprintf( sess_cb->msg_buff," " );
    max_len = build_rsltlst( &sess_cb->rsltlst_q, sess_cb->msg_buff, max_len );

    STprintf( sess_cb->msg_buff," Protocols ");
    max_len = build_rsltlst( &sess_cb->rsltlst_q, sess_cb->msg_buff, max_len );

    for(i=0; i< gcd_svr_info.gcd_pnp_info_len; i++ )
    {
        STprintf( sess_cb->msg_buff,"     %s  host:%s addr:%s port:%s ",
                gcd_svr_info.gcd_pnp_info[i].proto,
                gcd_svr_info.gcd_pnp_info[i].host,
                gcd_svr_info.gcd_pnp_info[i].addr,
                gcd_svr_info.gcd_pnp_info[i].port );
        max_len = build_rsltlst( &sess_cb->rsltlst_q,
				 sess_cb->msg_buff, max_len );
    }
    adm_rsltlst( sess_cb, scb, max_len );
    return;
}

/*
** Name: rqst_set_trace
**
** Description:
**       Set trace variables for GCD
**
** Input:
**      sess            ptr to GCD_SESS_CB
**      scb             GCADM session control block.
**      trace_id        Trace ID.
**      level           Trace level.
**
** Output:
**      None.
**
** Returns:
**      void
**
** History:
**       14-Jul-06 (rajus01)
**	    Created.
*/

static  VOID
rqst_set_trace( GCD_SESS_CB *sess_cb, PTR scb, char *trace_id, i4 level ) 
{
    char        val[ 128 ];
    i4          i;

    for( i = 0; i < ARR_SIZE( trace_ids ); i++ )
        if ( ! STbcompare( trace_id, 0, trace_ids[i].tr_id, 0, TRUE ) )
        {
            STprintf( val, "%d", level );

            if ( MOset( MO_WRITE, trace_ids[i].tr_classid, "0", val ) != OK )
                adm_rslt( sess_cb, scb, ADM_TRACE_ERR_TXT );
            else
            {
                STprintf( sess_cb->msg_buff,
                       "Tracing %s at level %d\n", trace_ids[i].tr_id, level );
                adm_rslt( sess_cb, scb, sess_cb->msg_buff );
            }
            break;
        }

    if ( i >= ARR_SIZE( trace_ids ) )
        adm_rslt( sess_cb, scb, ADM_BAD_TRACE_TXT );

    return;
} 

/*
** Name: gcd_get_gca_id
**
** Description:
**	 Use MO to get GCA association ID from API connection handle.
**
** Input:
**	api_conn_hndl   ptr
**	gca_assoc_id    i4
**
** Output:
**      GCA association ID.
**
** Returns:
**      None
**
** History:
**       05-Jul-06 (usha)
**	     Created.
*/

FUNC_EXTERN VOID
gcd_get_gca_id( PTR api_conn_hndl, i4 *gca_assoc_id ) 
{
    STATUS mo_stat;
    char buf[25];
    i4 buf_len = sizeof(buf);
    i4   perms = 0;
    char ptr_buf[16];

    *gca_assoc_id = -1;

    if( api_conn_hndl != NULL )
    {
        MOptrout( 0, api_conn_hndl, sizeof( ptr_buf ), ptr_buf );
        mo_stat = MOget(MO_READ, IIAPI_MIB_CONN_GCA_ASSOC, ptr_buf, &buf_len, 
								buf, &perms ); 
        if( mo_stat == OK )
        {
            CVan(buf, gca_assoc_id );
            return;
        }
     }

   return;
}

/*
** Name: rqst_sys_stats
**
** Description:
**      Display system sessions stats.
**
** Input:
**      ccb             ptr to GCD_CCB *
**      srv_class       Server class
**      srv_class_len   Size of server class buffer.
**      gca_addr        GCA address
**      gca_addr_len    Size of GCA address buffer.
**
** Output:
*       GCA server class, GCA server addr.
**
** Returns:
**      None
**
** History:
**       23-Aug-06 (rajus01)
**	    Created.
**	21-Jul-09 (gordy)
**	    Add buffer length parameters and limit copies into buffers.
*/

static void
rqst_sys_stats( GCD_CCB * ccb, char *srv_class, i4 srv_class_len, 
		char *gca_addr, i4 gca_addr_len )
{
    i4          perms = 0;
    char        srv_class_buf[30];
    i4          srv_class_buf_len = sizeof(srv_class_buf);
    char        srv_lsaddr_buf[30];
    i4          srv_lsaddr_buf_len = sizeof(srv_lsaddr_buf);
    char        id_buf[32];
    i4          id_buf_len = sizeof(id_buf);
    char        aid_buf[32];
    i4          gca_cb_id = -1;

    if( ccb != NULL )
    {
        if( ccb->cib )
            STprintf(aid_buf, "%d", ccb->cib->id);
        else
            STprintf(aid_buf, "%d", 0);
	  
        if( MOget(MO_READ, GCA_MIB_CLIENT, aid_buf, &id_buf_len, id_buf, &perms ) == OK )
        {
            CVan( id_buf, &gca_cb_id );
            STprintf(id_buf, "%d", gca_cb_id);
            MOget(MO_READ, GCA_MIB_CLIENT_SRV_CLASS, id_buf, &srv_class_buf_len, srv_class_buf, &perms );
            MOget(MO_READ, GCA_MIB_CLIENT_LISTEN_ADDRESS, id_buf, &srv_lsaddr_buf_len, srv_lsaddr_buf, &perms );
            STlcopy( srv_class_buf, srv_class, srv_class_len - 1 );
            STlcopy( srv_lsaddr_buf, gca_addr, gca_addr_len - 1 );
        }
    }

    return;
}
