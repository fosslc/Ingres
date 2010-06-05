/* 
** Copyright (c) 2003, 2010 Ingres Corporation.
*/

#include <compat.h>
#include <gc.h>
#include <gl.h>
#include <er.h>
#include <me.h>
#include <pc.h>
#include <sp.h>
#include <mo.h>
#include <si.h>
#include <st.h>
#include <tr.h>
#include <tm.h>
#include <qu.h>

#include <iicommon.h>

#include <gca.h>
#include <gcu.h>
#include <gcn.h>
#include <gcnint.h>
#include <gcadm.h>

/*
** Name: gcnsadm.c  
**
** Description:
**	GCADM interface with GCN server and iimonitor. 
**
** History:
**	 9-10-2003 (wansh01) 
**	    Created.
**	 7-Jul-2004 (wansh01) 
**	    Support Drop, SHOW commands.
**	 6-Aug-04 (gordy)
**	    Enhanced session info display.
**	 15-Sep-04 (wansh01)
**	    Enhanced session info display.
**	 24-Apr-06 (usha)
**	    Added more commands.
**              [force] option for stop server command instead of crash server.
**              Tracing enable/disable.
**              display statistics for various sessions.
**	 	formatted display of various sessions.	
**	 22-Jun-06 (usha)
**	    Formatted display o/p by 'show server' command to make it similar
**	    to DBMS server display o/p. Implemented REGISTER SERVER command.
**	  4-Aug-06 (usha)
**	    Added support for "set trace" commands to GCN admin interface.
**	 18-Aug-06 (usha)
**	    Took out "Remove <sid>" from help command for all servers.
**	 23-Aug-06 (usha)
**	    Added support for "format <sid>" command. Other format commands
**	    yet to be done.
**	 20-Sep-06 (Gordy and Usha)
**	    Gordy re-worked the entire code due to scalability issues 
**	    with the original implementation. Fixed some minor problems 
**	    with tracing commands, added some additional messages for format 
**	    commands, and tested new changes.  
**	08-Oct-06 ( Usha )
**	    Added High Water to indicate maximum number of active sessions
**	    ever reached.
**	11-Aug-09 (gordy)
**	    Remove string length restrictions.
**      22-Apr-10 (rajus01) SIR 123621
	    Display process ID.
*/


/*
** GCN ADM Session control block.
*/

typedef struct
{
    QUEUE	sess_q;  
    i4		aid;
    i4		state;
    char	*user_id;
    QUEUE	rsltlst_q;
    char	msg_buff[ ER_MAX_LEN ];
} GCN_SESS_CB;

static QUEUE	gcn_sess_q;
static char	*user_default = "<default>";


/*
** Result list queue entries.  Provides GCADM_RSLT
** element as required for ADM result list and
** (variable length) buffer for referenced message.
*/

typedef struct
{
    GCADM_RSLT	rslt;
    char	msg_buff[1];
} ADM_RSLT;


/*
** Command components: token, keywords, etc.
*/

#define GCN_TOKEN 		0x00

#define GCN_KW_ALL		0x01
#define GCN_KW_ADMIN		0x02
#define GCN_KW_CLOSED		0x03
#define GCN_KW_CRASH		0x04
#define GCN_KW_FORCE		0x05
#define GCN_KW_FORMAT		0x06
#define GCN_KW_FORMATTED	0x07
#define GCN_KW_HELP		0x08
#define GCN_KW_LSTN		0x09
#define GCN_KW_LCL		0x0A
#define GCN_KW_OPEN		0x0B
#define GCN_KW_REG		0x0C
#define GCN_KW_REMOVE		0x0D
#define GCN_KW_RMT		0x0E
#define GCN_KW_SERVER		0x0F
#define GCN_KW_SESS		0x10
#define GCN_KW_SET		0x11
#define GCN_KW_SHOW		0x12
#define GCN_KW_SHUT		0x13
#define GCN_KW_SHUTDOWN		0x14
#define GCN_KW_STATS		0x15
#define GCN_KW_STOP		0x16
#define GCN_KW_SYSTEM		0x17
#define GCN_KW_TICKETS		0x18
#define GCN_KW_TRACE		0x19
#define GCN_KW_USER		0x1A
#define GCN_KW_LOG    		0x1B
#define GCN_KW_NONE   		0x1C
#define GCN_KW_LEVEL  		0x1D


static ADM_KEYWORD keywords[] =
{
    { "ALL",		GCN_KW_ALL },
    { "ADMIN",		GCN_KW_ADMIN },
    { "CLOSED",		GCN_KW_CLOSED },
    { "CRASH",		GCN_KW_CRASH },
    { "FORCE",		GCN_KW_FORCE },
    { "FORMAT",		GCN_KW_FORMAT },
    { "FORMATTED",	GCN_KW_FORMATTED },
    { "HELP",		GCN_KW_HELP },
    { "LISTEN",		GCN_KW_LSTN },
    { "LOCAL",		GCN_KW_LCL },
    { "OPEN",		GCN_KW_OPEN },
    { "REGISTER",	GCN_KW_REG },
    { "REMOTE", 	GCN_KW_RMT },
    { "REMOVE",		GCN_KW_REMOVE },
    { "SERVER",		GCN_KW_SERVER },
    { "SESSIONS",	GCN_KW_SESS },
    { "SET",		GCN_KW_SET },
    { "SHOW",		GCN_KW_SHOW },
    { "SHUT",		GCN_KW_SHUT },
    { "SHUTDOWN",	GCN_KW_SHUTDOWN },
    { "STATS",	 	GCN_KW_STATS },
    { "STOP",		GCN_KW_STOP },
    { "SYSTEM",		GCN_KW_SYSTEM },
    { "TICKETS",	GCN_KW_TICKETS },
    { "TRACE",  	GCN_KW_TRACE },
    { "USER",		GCN_KW_USER },
    { "LOG",		GCN_KW_LOG },
    { "NONE",		GCN_KW_NONE },
    { "LEVEL",		GCN_KW_LEVEL },
};


#define	GCN_CMD_HELP				1
#define	GCN_CMD_SHOW_SVR			2
#define	GCN_CMD_SHOW_SVR_LSTN			3
#define	GCN_CMD_SHOW_SVR_SHUT			4
#define	GCN_CMD_SET_SVR_SHUT			5
#define	GCN_CMD_SET_SVR_CLSD			6
#define	GCN_CMD_SET_SVR_OPEN			7
#define	GCN_CMD_STOP_SVR			8
#define	GCN_CMD_STOP_SVR_FORCE			9
#define	GCN_CMD_CRASH_SVR			10
#define	GCN_CMD_REG_SVR				11
#define	GCN_CMD_SHOW_ADM_SESS			12
#define	GCN_CMD_SHOW_USR_SESS			13
#define	GCN_CMD_SHOW_SYS_SESS			14
#define	GCN_CMD_SHOW_ALL_SESS			15
#define	GCN_CMD_SHOW_USR_SESS_FRMT		16
#define	GCN_CMD_SHOW_ADM_SESS_FRMT		17
#define	GCN_CMD_SHOW_SYS_SESS_FRMT		18
#define	GCN_CMD_SHOW_ALL_SESS_FRMT		19
#define	GCN_CMD_SHOW_USR_SESS_STATS		20
#define	GCN_CMD_SHOW_ADM_SESS_STATS		21
#define	GCN_CMD_SHOW_SYS_SESS_STATS		22
#define	GCN_CMD_SHOW_ALL_SESS_STATS		23
#define	GCN_CMD_FRMT_ADM_SESS			24
#define	GCN_CMD_FRMT_USR_SESS			25
#define	GCN_CMD_FRMT_SYS_SESS			26
#define	GCN_CMD_FRMT_ALL_SESS			27
#define	GCN_CMD_FRMT_SESSID			28
#define	GCN_CMD_REM_SESSID			29
#define	GCN_CMD_REM_ALL_TKTS			30
#define	GCN_CMD_REM_LCL_TKTS			31
#define	GCN_CMD_REM_RMT_TKTS			32
#define	GCN_CMD_SET_TRACE_LOG_NONE      	33
#define	GCN_CMD_SET_TRACE_LOG_FNAME     	34
#define	GCN_CMD_SET_TRACE_LEVEL         	35
#define	GCN_CMD_SET_TRACE_ID_LEVEL      	36

/* Installation Password Tokens Managment Commands */
static ADM_ID cmd_rmv_tkts[] = 
	{ GCN_KW_REMOVE, GCN_KW_TICKETS };
static ADM_ID cmd_rmv_all_tkts[] = 
	{ GCN_KW_REMOVE, GCN_KW_ALL, GCN_KW_TICKETS };
static ADM_ID cmd_rmv_lcl_tkts[] = 
	{ GCN_KW_REMOVE, GCN_KW_LCL,GCN_KW_TICKETS}; 
static ADM_ID cmd_rmv_rmt_tkts[] = 
	{ GCN_KW_REMOVE, GCN_KW_RMT,GCN_KW_TICKETS };

/* Help - List all the commands */
static ADM_ID cmd_help[] = 
	{ GCN_KW_HELP };

/* Server States Management Commands */
     /* Set state Commands */
static ADM_ID cmd_set_svr_shut[] = 
	{ GCN_KW_SET, GCN_KW_SERVER, GCN_KW_SHUT };
static ADM_ID cmd_set_svr_clsd[] = 
	{ GCN_KW_SET, GCN_KW_SERVER, GCN_KW_CLOSED };
static ADM_ID cmd_set_svr_open[] = 
	{ GCN_KW_SET, GCN_KW_SERVER, GCN_KW_OPEN };
     /* Show state Commands */
static ADM_ID cmd_show_svr[]  = 
	{ GCN_KW_SHOW, GCN_KW_SERVER };
static ADM_ID cmd_show_svr_lstn[] =
	{ GCN_KW_SHOW, GCN_KW_SERVER, GCN_KW_LSTN };
static ADM_ID cmd_show_svr_shut[] =
	{ GCN_KW_SHOW, GCN_KW_SERVER, GCN_KW_SHUTDOWN};

/* Normal/Forecful Server Shutdown  Commands */
static ADM_ID cmd_stop_svr[] = 
	{ GCN_KW_STOP, GCN_KW_SERVER };
static ADM_ID cmd_stop_svr_force[] =
	{ GCN_KW_STOP, GCN_KW_SERVER, GCN_KW_FORCE };
static ADM_ID cmd_crash_svr[] = 
	{ GCN_KW_CRASH, GCN_KW_SERVER };

/* Register Server Command */
static ADM_ID cmd_reg_svr[] = 
	{ GCN_KW_REG, GCN_KW_SERVER };

/* Server Sessions Management Commands */
    /* Show  Commands - unformatted o/p */
static ADM_ID cmd_show_sess[] = 
	{ GCN_KW_SHOW, GCN_KW_SESS };
static ADM_ID cmd_show_usr_sess[] = 
	{ GCN_KW_SHOW, GCN_KW_USER, GCN_KW_SESS };
static ADM_ID cmd_show_all_sess[] = 
	{ GCN_KW_SHOW, GCN_KW_ALL, GCN_KW_SESS };
static ADM_ID cmd_show_adm_sess[] = 
	{ GCN_KW_SHOW, GCN_KW_ADMIN, GCN_KW_SESS };
static ADM_ID cmd_show_sys_sess[] = 
	{ GCN_KW_SHOW, GCN_KW_SYSTEM, GCN_KW_SESS };

    /* Show Commands - formatted o/p */
static ADM_ID cmd_show_sess_frmt[] = 
	{ GCN_KW_SHOW, GCN_KW_SESS, GCN_KW_FORMATTED };
static ADM_ID cmd_show_usr_sess_frmt[] = 
	{ GCN_KW_SHOW, GCN_KW_USER, GCN_KW_SESS, GCN_KW_FORMATTED };
static ADM_ID cmd_show_admin_sess_frmt[] = 
	{ GCN_KW_SHOW, GCN_KW_ADMIN, GCN_KW_SESS, GCN_KW_FORMATTED };
static ADM_ID cmd_show_sys_sess_frmt[] = 
	{ GCN_KW_SHOW, GCN_KW_SYSTEM, GCN_KW_SESS, GCN_KW_FORMATTED };
static ADM_ID cmd_show_all_sess_frmt[] = 
	{ GCN_KW_SHOW, GCN_KW_ALL, GCN_KW_SESS, GCN_KW_FORMATTED };

    /* Show Commands - statistics */
static ADM_ID cmd_show_sess_stats[] = 
	{ GCN_KW_SHOW, GCN_KW_SESS, GCN_KW_STATS };
static ADM_ID cmd_show_usr_sess_stats[] = 
	{ GCN_KW_SHOW, GCN_KW_USER, GCN_KW_SESS, GCN_KW_STATS };
static ADM_ID cmd_show_admin_sess_stats[] = 
	{ GCN_KW_SHOW, GCN_KW_ADMIN, GCN_KW_SESS, GCN_KW_STATS };
static ADM_ID cmd_show_sys_sess_stats[] = 
	{ GCN_KW_SHOW, GCN_KW_SYSTEM, GCN_KW_SESS, GCN_KW_STATS };
static ADM_ID cmd_show_all_sess_stats[] = 
	{ GCN_KW_SHOW, GCN_KW_ALL, GCN_KW_SESS, GCN_KW_STATS };
          
   /* Format  Commands  - Admin sessions */
static ADM_ID cmd_fmt_adm[] = 
	{ GCN_KW_FORMAT, GCN_KW_ADMIN };
static ADM_ID cmd_fmt_adm_sess[] = 
	{ GCN_KW_FORMAT, GCN_KW_ADMIN, GCN_KW_SESS };
   /* Format  Commands  - User sessions */
static ADM_ID cmd_fmt_usr[] = 
	{ GCN_KW_FORMAT, GCN_KW_USER };
static ADM_ID cmd_fmt_usr_sess[] = 
	{ GCN_KW_FORMAT, GCN_KW_USER, GCN_KW_SESS };
   /* Format  Commands  - System sessions */
static ADM_ID cmd_fmt_sys[] = 
	{ GCN_KW_FORMAT, GCN_KW_SYSTEM };
static ADM_ID cmd_fmt_sys_sess[] = 
	{ GCN_KW_FORMAT, GCN_KW_SYSTEM, GCN_KW_SESS };
   /* Format  Commands  - user, admin, system sessions */
static ADM_ID cmd_fmt_all[] = 
	{ GCN_KW_FORMAT, GCN_KW_ALL };
static ADM_ID cmd_fmt_all_sess[] = 
	{ GCN_KW_FORMAT, GCN_KW_ALL, GCN_KW_SESS };
   /* Format  Command  - on a given sessions ID */
static ADM_ID cmd_fmt_sessid[] = 
	{ GCN_KW_FORMAT, GCN_TOKEN };

   /* Remove  Command - on a given session ID */
static ADM_ID cmd_rmv_sessid[] = 
	{ GCN_KW_REMOVE,GCN_TOKEN };

/* Trace Command */
static ADM_ID cmd_set_trace_log_none[] = 
	{ GCN_KW_SET, GCN_KW_TRACE, GCN_KW_LOG, GCN_KW_NONE };
static ADM_ID cmd_set_trace_log_fname[] = 
	{ GCN_KW_SET, GCN_KW_TRACE, GCN_KW_LOG, GCN_TOKEN };
static ADM_ID cmd_set_trace_level[] = 
	{ GCN_KW_SET, GCN_KW_TRACE, GCN_KW_LEVEL, GCN_TOKEN };
static ADM_ID cmd_set_trace_id_level[] = 
	{ GCN_KW_SET, GCN_KW_TRACE, GCN_TOKEN, GCN_TOKEN };

static ADM_COMMAND commands[] =
{
    { GCN_CMD_REM_ALL_TKTS, 
	ARR_SIZE( cmd_rmv_tkts ), cmd_rmv_tkts },
    { GCN_CMD_REM_ALL_TKTS, 
	ARR_SIZE( cmd_rmv_all_tkts ), cmd_rmv_all_tkts },
    { GCN_CMD_REM_LCL_TKTS, 
	ARR_SIZE( cmd_rmv_lcl_tkts ), cmd_rmv_lcl_tkts },
    { GCN_CMD_REM_RMT_TKTS, 
	ARR_SIZE( cmd_rmv_rmt_tkts ), cmd_rmv_rmt_tkts },
    { GCN_CMD_REG_SVR, 
	ARR_SIZE( cmd_reg_svr ), cmd_reg_svr },
    { GCN_CMD_HELP, 
	ARR_SIZE( cmd_help ), cmd_help },
    { GCN_CMD_SET_SVR_SHUT, 
	ARR_SIZE( cmd_set_svr_shut ), cmd_set_svr_shut },
    { GCN_CMD_SET_SVR_CLSD, 
	ARR_SIZE( cmd_set_svr_clsd ), cmd_set_svr_clsd },
    { GCN_CMD_SET_SVR_OPEN, 
	ARR_SIZE( cmd_set_svr_open ), cmd_set_svr_open },
    { GCN_CMD_STOP_SVR,    
	ARR_SIZE( cmd_stop_svr ), cmd_stop_svr },
    { GCN_CMD_STOP_SVR_FORCE,
	ARR_SIZE( cmd_stop_svr_force ), cmd_stop_svr_force },
    { GCN_CMD_CRASH_SVR,    
	ARR_SIZE( cmd_crash_svr ), cmd_crash_svr },
    { GCN_CMD_SHOW_SVR,	    
	ARR_SIZE( cmd_show_svr ), cmd_show_svr },
    { GCN_CMD_SHOW_SVR_LSTN,
	ARR_SIZE( cmd_show_svr_lstn ), cmd_show_svr_lstn },
    { GCN_CMD_SHOW_SVR_SHUT,
	ARR_SIZE( cmd_show_svr_shut ), cmd_show_svr_shut },
    { GCN_CMD_SHOW_USR_SESS, 
	ARR_SIZE( cmd_show_sess ), cmd_show_sess },
    { GCN_CMD_SHOW_USR_SESS, 
	ARR_SIZE( cmd_show_usr_sess ), cmd_show_usr_sess },
    { GCN_CMD_SHOW_SYS_SESS, 
	ARR_SIZE( cmd_show_sys_sess ), cmd_show_sys_sess },
    { GCN_CMD_SHOW_ADM_SESS, 
	ARR_SIZE( cmd_show_adm_sess ), cmd_show_adm_sess },
    { GCN_CMD_SHOW_ALL_SESS, 
	ARR_SIZE( cmd_show_all_sess ), cmd_show_all_sess },
    { GCN_CMD_SHOW_USR_SESS_STATS, 
	ARR_SIZE( cmd_show_sess_stats ), cmd_show_sess_stats },
    { GCN_CMD_SHOW_USR_SESS_STATS, 
	ARR_SIZE( cmd_show_usr_sess_stats ), cmd_show_usr_sess_stats },
    { GCN_CMD_SHOW_ADM_SESS_STATS, 
	ARR_SIZE( cmd_show_admin_sess_stats ), cmd_show_admin_sess_stats },
    { GCN_CMD_SHOW_SYS_SESS_STATS, 
	ARR_SIZE( cmd_show_sys_sess_stats ), cmd_show_sys_sess_stats },
    { GCN_CMD_SHOW_ALL_SESS_STATS, 
	ARR_SIZE( cmd_show_all_sess_stats ), cmd_show_all_sess_stats },
    { GCN_CMD_SHOW_USR_SESS_FRMT, 
	ARR_SIZE( cmd_show_sess_frmt ), cmd_show_sess_frmt },
    { GCN_CMD_SHOW_USR_SESS_FRMT, 
	ARR_SIZE( cmd_show_usr_sess_frmt ), cmd_show_usr_sess_frmt },
    { GCN_CMD_SHOW_ADM_SESS_FRMT, 
	ARR_SIZE( cmd_show_admin_sess_frmt ), cmd_show_admin_sess_frmt },
    { GCN_CMD_SHOW_SYS_SESS_FRMT, 
	ARR_SIZE( cmd_show_sys_sess_frmt ), cmd_show_sys_sess_frmt },
    { GCN_CMD_SHOW_ALL_SESS_FRMT, 
	ARR_SIZE( cmd_show_all_sess_frmt ), cmd_show_all_sess_frmt },
    { GCN_CMD_FRMT_ADM_SESS, 
	ARR_SIZE( cmd_fmt_adm ), cmd_fmt_adm },
    { GCN_CMD_FRMT_ADM_SESS, 
	ARR_SIZE( cmd_fmt_adm_sess ), cmd_fmt_adm_sess },
    { GCN_CMD_FRMT_USR_SESS, 
	ARR_SIZE( cmd_fmt_usr ), cmd_fmt_usr },
    { GCN_CMD_FRMT_USR_SESS, 
	ARR_SIZE( cmd_fmt_usr_sess ), cmd_fmt_usr_sess },
    { GCN_CMD_FRMT_SYS_SESS, 
	ARR_SIZE( cmd_fmt_sys ), cmd_fmt_sys },
    { GCN_CMD_FRMT_SYS_SESS, 
	ARR_SIZE( cmd_fmt_sys_sess ), cmd_fmt_sys_sess },
    { GCN_CMD_FRMT_ALL_SESS, 
	ARR_SIZE( cmd_fmt_all ), cmd_fmt_all },
    { GCN_CMD_FRMT_ALL_SESS, 
	ARR_SIZE( cmd_fmt_all_sess ), cmd_fmt_all_sess },
    { GCN_CMD_FRMT_SESSID, 
	ARR_SIZE( cmd_fmt_sessid ) , cmd_fmt_sessid },
    { GCN_CMD_REM_SESSID, 
	ARR_SIZE( cmd_rmv_sessid ), cmd_rmv_sessid },
    { GCN_CMD_SET_TRACE_LOG_NONE, 
	ARR_SIZE( cmd_set_trace_log_none ), cmd_set_trace_log_none },
    { GCN_CMD_SET_TRACE_LOG_FNAME, 
	ARR_SIZE( cmd_set_trace_log_fname ), cmd_set_trace_log_fname },
    { GCN_CMD_SET_TRACE_LEVEL, 
	ARR_SIZE( cmd_set_trace_level ), cmd_set_trace_level },
    { GCN_CMD_SET_TRACE_ID_LEVEL, 
	ARR_SIZE( cmd_set_trace_id_level ), cmd_set_trace_id_level },
};

#define	ADM_SVR_OPEN_TXT	"User connections now allowed"
#define	ADM_SVR_CLSD_TXT	"User connections now disabled"
#define	ADM_SVR_SHUT_TXT	"Server will stop"
#define	ADM_SVR_STOP_TXT	"Server stopping" 
#define	ADM_SVR_REG_TXT		"Name Server registered"
#define	ADM_REG_ERR_TXT		"Name Server registration failed"
#define	ADM_LOG_OPEN_TXT	"Logging enabled"
#define	ADM_LOG_CLOSE_TXT	"Logging disabled" 
#define	ADM_TRACE_ERR_TXT	"Error setting trace level"
#define	ADM_BAD_TRACE_TXT	"Invalid trace id"
#define	ADM_BAD_SID_TXT		"Invalid session id"
#define	ADM_BAD_COMMAND_TXT	"Illegal command"

static char *helpText[] =
{
    "Server:",
    "     SET SERVER SHUT | CLOSED | OPEN", 
    "     STOP SERVER [FORCE]",
    "     SHOW SERVER [LISTEN | SHUTDOWN]", 
    "     REGISTER SERVER",
    "Session:",
    "     SHOW [USER] | ADMIN | SYSTEM | ALL SESSIONS [FORMATTED | STATS]", 
    "     FORMAT USER | ADMIN | SYSTEM | ALL [SESSIONS]",
    "     FORMAT <session_id>",
    "Others:",
    "     REMOVE [ALL] | LOCAL | REMOTE TICKETS",
    "     SET TRACE LOG <path> | NONE",
    "     SET TRACE LEVEL | <id> <lvl>",
    "     HELP",
    "     QUIT",
};


/*
** GCN_CMD_MAX_TOKEN is max tokens in valid command line
*/ 

#define GCN_CMD_MAX_TOKEN 	4


/*
** Sub-system tracing ID's
*/

typedef struct
{
   char  tr_id[5];
   char  tr_classid[32];

} GCN_TRACE_INFO;

static GCN_TRACE_INFO trace_ids[]=
{
   { "GCA",  "exp.gcf.gca.trace_level" },
   { "GCS",  "exp.gcf.gcs.trace_level" },
};

#define GCNS_INIT      0               /* init */
#define GCNS_WAIT      1               /* wait */
#define GCNS_PROCESS   2               /* process      */
#define GCNS_EXIT      3               /* exit         */

static char		*sess_states[] = { "INIT", "WAIT", "PROCESS", "EXIT" };


/*
** Forward references
*/

static void		adm_error( GCN_SESS_CB *, PTR,  STATUS );
static void		adm_stop( GCN_SESS_CB *, PTR, char * );
static void		adm_rslt( GCN_SESS_CB *, PTR , char * );
static void		adm_rsltlst( GCN_SESS_CB *, PTR, i4 );
static void		adm_done( GCN_SESS_CB *, PTR );

static void		adm_rqst_cb( PTR );
static void		adm_rslt_cb( PTR );
static void		adm_stop_cb( PTR );
static void		adm_exit_cb( PTR );

static void		rqst_show_server( GCN_SESS_CB *, PTR );
static i4		rqst_show_adm( QUEUE *, PTR, u_i2, i4 );
static i4		rqst_show_sessions( QUEUE *, PTR, u_i2, i4 );
static void		rqst_set_trace( GCN_SESS_CB *sess_cb, 
					PTR scb, char *trace_id, i4 level );

static void		error_log( STATUS );
static STATUS		end_session( GCN_SESS_CB *, STATUS );
static i4		build_rsltlst( QUEUE *, char *, i4 );


/*
** Name: gcn_adm_init  
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
gcn_adm_init()
{
    GCADM_INIT_PARMS	gcadm_init_parms;
    STATUS		status; 
    i4			i;

    if ( IIGCn_static.trace >= 4 )
	TRdisplay( "%4d   GCN ADM Initialize\n", -1 );

    QUinit( &gcn_sess_q );

    MEfill( sizeof( gcadm_init_parms ), 0, (PTR)&gcadm_init_parms );
    gcadm_init_parms.alloc_rtn    = NULL;
    gcadm_init_parms.dealloc_rtn  = NULL;
    gcadm_init_parms.errlog_rtn   = error_log;
    gcadm_init_parms.msg_buff_len = GCN_MSG_BUFF_LEN;

    if ( (status = gcadm_init( &gcadm_init_parms )) != OK )
    {
	if ( IIGCn_static.trace >= 1 )
	    TRdisplay( "%4d     GCN ADM gcadm_init() failed: 0x%x\n", 
		       -1, status );

	error_log( E_GC0167_GCN_ADM_INIT_ERR ); 
	error_log( status ); 
	status = E_GC0167_GCN_ADM_INIT_ERR;
    }	

    return ( status ); 
}


/*
** Name: gcn_adm_term
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
gcn_adm_term()
{
    STATUS  status; 
    
    if ( IIGCn_static.trace >= 4 )
	TRdisplay( "%4d   GCN ADM Terminate\n", -1 );

    if ( (status = gcadm_term()) != OK )
    {
	if ( IIGCn_static.trace >= 1 )
	    TRdisplay( "%4d     GCN ADM gcadm_term() failed: 0x%x\n", 
		       -1, status );
    }

    return( status ); 
}


/*
** Name: gcn_adm_session
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
**	11-Aug-09 (gordy)
**	    Dynamically allocate user ID.
*/

STATUS  
gcn_adm_session( i4 aid, PTR gca_cb, char *user )
{
    GCN_SESS_CB		*sess_cb;
    GCADM_SESS_PARMS    gcadm_sess;
    STATUS		status; 

    if ( IIGCn_static.trace >= 4 )
	TRdisplay( "%4d   GCN ADM New session: %s\n", aid, user );

    sess_cb = (GCN_SESS_CB *)MEreqmem( 0, sizeof(GCN_SESS_CB), TRUE, &status );

    if ( ! sess_cb )
    {
	if ( IIGCn_static.trace >= 1 )
	    TRdisplay( "%4d     GCN ADM mem alloc failed: 0x%x\n", 
		       aid, status );

	if ( sess_cb )  MEfree( (PTR)sess_cb );
	error_log( E_GC0121_GCN_NOMEM  );
	return( E_GC0121_GCN_NOMEM );
    }

    sess_cb->aid = aid;
    sess_cb->user_id = STalloc( user );
    if ( ! sess_cb->user_id )  sess_cb->user_id = user_default;
    QUinit( &sess_cb->rsltlst_q );
    sess_cb->state = GCNS_INIT;

    QUinsert( &sess_cb->sess_q, &gcn_sess_q );
    IIGCn_static.adm_sess_count++; 

    MEfill( sizeof( gcadm_sess ), 0, (PTR)&gcadm_sess );
    gcadm_sess.assoc_id = aid; 
    gcadm_sess.gca_cb   = gca_cb;
    gcadm_sess.rqst_cb  = (GCADM_RQST_FUNC )adm_rqst_cb;
    gcadm_sess.rqst_cb_parms = (PTR)sess_cb;

    if ( (status = gcadm_session( &gcadm_sess )) != OK )
    {
	end_session( sess_cb, status );
	status = E_GC0168_GCN_ADM_SESS_ERR;
    }

    return( status );
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
*/

static void 
adm_rqst_cb( PTR ptr )
{
    GCADM_RQST_PARMS	*gcadm_rqst = (GCADM_RQST_PARMS *)ptr;
    GCN_SESS_CB		*sess_cb = (GCN_SESS_CB *)gcadm_rqst->rqst_cb_parms;
    ADM_COMMAND		*rqst_cmd;
    char		*token[ GCN_CMD_MAX_TOKEN + 1 ];
    ADM_ID		token_ids[ GCN_CMD_MAX_TOKEN + 1 ]; 
    u_i2		token_count;
    u_i2		keyword_count = ARR_SIZE ( keywords );
    u_i2		command_count = ARR_SIZE ( commands );

    if ( IIGCn_static.trace >= 4 )
	TRdisplay( "%4d     GCN ADM request callback: 0x%x\n", 
		   sess_cb->aid, gcadm_rqst->sess_status );

    if (  gcadm_rqst->sess_status != OK )				  
    {
	end_session( sess_cb, gcadm_rqst->sess_status );

	/*
	** Need to drive GCN end-of-session processing
	** to satisfy quiesce shutdown testing.  New
	** sessions proceed directly to exit when the
	** server is active, so start a new session
	** when last admin session ends.
	*/
	if ( IIGCn_static.adm_sess_count == 0)  gcn_start_session();
	return;
    }

    sess_cb->state = GCNS_PROCESS;

    /* 
    ** Prepare and process request text.
    */
    gcadm_rqst->request[ gcadm_rqst->length ] = EOS; 
    STtrmwhite( gcadm_rqst->request );

    if ( IIGCn_static.trace >= 4 )
	TRdisplay( "%4d     GCN ADM request: %s\n", 
		   sess_cb->aid, gcadm_rqst->request );  

    token_count = gcu_words( gcadm_rqst->request, 
    			     NULL, token, ' ', GCN_CMD_MAX_TOKEN + 1 );

    gcadm_keyword( token_count, token, 
		   keyword_count, keywords, token_ids, GCN_TOKEN ); 

    if ( ! gcadm_command( token_count, token_ids, 
			  command_count, commands, &rqst_cmd  ) ) 
    {
        if ( IIGCn_static.trace >= 5 )
	    TRdisplay( "%4d     GCN ADM gcadm_cmd failed\n", -1 );  
	adm_rslt( sess_cb, gcadm_rqst->gcadm_scb, ADM_BAD_COMMAND_TXT );
        return;
    }

    if ( IIGCn_static.trace >= 5 )
	TRdisplay( "%4d     GCN ADM command: %d\n", 
		   sess_cb->aid, rqst_cmd->command );  

    switch( rqst_cmd->command ) 
    {
    case GCN_CMD_HELP:  
    {
	i4 i, max_len;

	for( i = max_len = 0; i < ARR_SIZE( helpText ); i++ )
	    max_len = build_rsltlst(&sess_cb->rsltlst_q, helpText[i], max_len);

	adm_rsltlst( sess_cb, gcadm_rqst->gcadm_scb, max_len );
	break;
    }
    case GCN_CMD_SHOW_SVR:
	rqst_show_server( sess_cb, gcadm_rqst->gcadm_scb );
	break;

    case GCN_CMD_SHOW_SVR_LSTN:
	adm_rslt( sess_cb, gcadm_rqst->gcadm_scb, 
		(IIGCn_static.flags & GCN_SVR_CLOSED) ? "CLOSED" : "OPEN" );
	break;

    case GCN_CMD_SHOW_SVR_SHUT:
	adm_rslt( sess_cb, gcadm_rqst->gcadm_scb, 
		(IIGCn_static.flags & GCN_SVR_QUIESCE) ? "PENDING" :
		((IIGCn_static.flags & GCN_SVR_SHUT) ?  "STOPPING" : "OPEN") );
	break;

    case GCN_CMD_SET_SVR_OPEN:
	IIGCn_static.flags &= ~( GCN_SVR_QUIESCE | 
				 GCN_SVR_CLOSED | GCN_SVR_SHUT );
	adm_rslt( sess_cb, gcadm_rqst->gcadm_scb, ADM_SVR_OPEN_TXT );
	break;

    case GCN_CMD_SET_SVR_CLSD:
	IIGCn_static.flags |= GCN_SVR_CLOSED;  
	adm_rslt( sess_cb, gcadm_rqst->gcadm_scb, ADM_SVR_CLSD_TXT );
	break;

    case GCN_CMD_SET_SVR_SHUT:
	IIGCn_static.flags |= GCN_SVR_QUIESCE | GCN_SVR_CLOSED; 
	adm_rslt( sess_cb, gcadm_rqst->gcadm_scb, ADM_SVR_SHUT_TXT );
	break;

    case GCN_CMD_STOP_SVR:
	IIGCn_static.flags |= GCN_SVR_SHUT | GCN_SVR_CLOSED; 
	adm_stop( sess_cb, gcadm_rqst->gcadm_scb, ADM_SVR_STOP_TXT );
	break;

    case GCN_CMD_STOP_SVR_FORCE:
    case GCN_CMD_CRASH_SVR:
	PCexit( FAIL ); 
	break;

    case GCN_CMD_REG_SVR:
    {
	CL_ERR_DESC sys_err;

	/* 
	** This operation stores the listen address of the Name Server
        ** in the symbol table. II_GCNXX_PORT gets set to listen_addr. 
        */
        if ( GCnsid( GC_SET_NSID, IIGCn_static.listen_addr, 
		     STlength( IIGCn_static.listen_addr ), &sys_err ) == OK )
            adm_rslt( sess_cb, gcadm_rqst->gcadm_scb, ADM_SVR_REG_TXT );
        else
        {
            gcu_erlog( 0, IIGCn_static.language,
		       E_GC0123_GCN_NSID_FAIL, &sys_err, 0, NULL );
            adm_rslt( sess_cb, gcadm_rqst->gcadm_scb, ADM_REG_ERR_TXT );
        }
	break;
    }
    case GCN_CMD_SHOW_ADM_SESS:
    case GCN_CMD_SHOW_ADM_SESS_STATS:
    case GCN_CMD_SHOW_ADM_SESS_FRMT:
    case GCN_CMD_FRMT_ADM_SESS:
    {
	i4 max_len = rqst_show_adm( &sess_cb->rsltlst_q, 
				    NULL, rqst_cmd->command, 0 );
	if ( max_len )
	    adm_rsltlst( sess_cb, gcadm_rqst->gcadm_scb, max_len );
	else
	    adm_done( sess_cb, gcadm_rqst->gcadm_scb );
	break;
    }
    case GCN_CMD_SHOW_USR_SESS:
    case GCN_CMD_SHOW_USR_SESS_STATS:
    case GCN_CMD_SHOW_USR_SESS_FRMT:
    case GCN_CMD_FRMT_USR_SESS:
    case GCN_CMD_SHOW_SYS_SESS:
    case GCN_CMD_SHOW_SYS_SESS_STATS:
    case GCN_CMD_SHOW_SYS_SESS_FRMT:
    case GCN_CMD_FRMT_SYS_SESS:
    {
    	i4 max_len = rqst_show_sessions( &sess_cb->rsltlst_q, NULL,
					 rqst_cmd->command, 0 );
	if ( max_len )
	    adm_rsltlst( sess_cb, gcadm_rqst->gcadm_scb, max_len );
	else
	    adm_done( sess_cb, gcadm_rqst->gcadm_scb );
	break;
    }
    case GCN_CMD_SHOW_ALL_SESS:
    case GCN_CMD_SHOW_ALL_SESS_STATS:
    case GCN_CMD_SHOW_ALL_SESS_FRMT:
    case GCN_CMD_FRMT_ALL_SESS:
    {
	i4 max_len = 0;

    	max_len = rqst_show_sessions( &sess_cb->rsltlst_q, NULL,
				      rqst_cmd->command, max_len );
	max_len = rqst_show_adm( &sess_cb->rsltlst_q, 
				 NULL, rqst_cmd->command, max_len );
	if ( max_len )
	    adm_rsltlst( sess_cb, gcadm_rqst->gcadm_scb, max_len );
	else
	    adm_done( sess_cb, gcadm_rqst->gcadm_scb );
	break;
    }
    case GCN_CMD_FRMT_SESSID:
    {
    	i4	max_len;
	PTR	sid = NULL;

	CVaxptr( token[1], &sid );	/* Decode session ID */

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

	/*
	** Return results if SID found.  Otherwise, bad SID error message.
	*/
	if ( max_len )
	    adm_rsltlst( sess_cb, gcadm_rqst->gcadm_scb, max_len );
	else
	    adm_rslt( sess_cb, gcadm_rqst->gcadm_scb, ADM_BAD_SID_TXT );
        break;
    }
    case GCN_CMD_SET_TRACE_LOG_NONE:
    {
	CL_ERR_DESC	err_cl;
	STATUS		cl_stat;

	if ( (cl_stat = TRset_file( TR_F_CLOSE, NULL, 0, &err_cl )) == OK )
	   adm_rslt( sess_cb, gcadm_rqst->gcadm_scb, ADM_LOG_CLOSE_TXT );
	else  if ( cl_stat == TR_BADCLOSE )
	    adm_rslt( sess_cb, gcadm_rqst->gcadm_scb, ADM_LOG_CLOSE_TXT );
	else
	{
	    STprintf( sess_cb->msg_buff, 
	    	      "Error disabling logging: %0x\n", cl_stat);
	    adm_rslt( sess_cb, gcadm_rqst->gcadm_scb, sess_cb->msg_buff );
	}
	break;
    }
    case GCN_CMD_SET_TRACE_LOG_FNAME:
    {
	CL_ERR_DESC	err_cl;

       TRset_file( TR_F_CLOSE, NULL, 0, &err_cl );

       if ( TRset_file( TR_F_OPEN, 
       			token[3], (i4)STlength( token[3] ), &err_cl ) == OK )
           adm_rslt( sess_cb, gcadm_rqst->gcadm_scb, ADM_LOG_OPEN_TXT );
       else
       {
           STprintf( sess_cb->msg_buff, 
	   	     "Error opening log file %s\n", token[3] );
           adm_rslt( sess_cb, gcadm_rqst->gcadm_scb, sess_cb->msg_buff );
       }
       break;
    }
    case GCN_CMD_SET_TRACE_LEVEL:
    {
	i4 trace_lvl;

        CVan( token[3], &trace_lvl );
        IIGCn_static.trace = trace_lvl;
        STprintf( sess_cb->msg_buff, "Tracing GCN at level %d\n", trace_lvl );
        adm_rslt( sess_cb, gcadm_rqst->gcadm_scb, sess_cb->msg_buff );
        break;
    }
    case GCN_CMD_SET_TRACE_ID_LEVEL:
    {
	i4 trace_lvl;

        CVan( token[3], &trace_lvl );
    	rqst_set_trace( sess_cb, gcadm_rqst->gcadm_scb, token[2], trace_lvl );
	break;
    }
    case GCN_CMD_REM_ALL_TKTS:  
    {
	i4 ticket_count;
	i4 max_len = 0;

	ticket_count  = gcn_drop_tickets( GCN_LTICKET );
	STprintf(sess_cb->msg_buff, "Removed %d LOCAL tickets", ticket_count);
	max_len = build_rsltlst( &sess_cb->rsltlst_q,
				 sess_cb->msg_buff,  max_len );

	ticket_count = gcn_drop_tickets( GCN_RTICKET );
	STprintf(sess_cb->msg_buff, "Removed %d REMOTE tickets", ticket_count);
	max_len = build_rsltlst( &sess_cb->rsltlst_q, 
				 sess_cb->msg_buff, max_len );

	adm_rsltlst( sess_cb, gcadm_rqst->gcadm_scb, max_len );
	break;
    }
    case GCN_CMD_REM_LCL_TKTS:  
    {
	i4 ticket_count;
	i4 max_len = 0;

	ticket_count  = gcn_drop_tickets( GCN_LTICKET );
	STprintf(sess_cb->msg_buff, "Removed %d LOCAL tickets", ticket_count);
	max_len = build_rsltlst( &sess_cb->rsltlst_q,
				 sess_cb->msg_buff,  max_len );

	adm_rslt( sess_cb, gcadm_rqst->gcadm_scb, sess_cb->msg_buff );
	break;
    }
    case GCN_CMD_REM_RMT_TKTS:  
    {
	i4 ticket_count;
	i4 max_len = 0;

	ticket_count = gcn_drop_tickets( GCN_RTICKET );
	STprintf(sess_cb->msg_buff, "Removed %d REMOTE tickets", ticket_count);
	max_len = build_rsltlst( &sess_cb->rsltlst_q, 
				 sess_cb->msg_buff, max_len );

	adm_rslt( sess_cb, gcadm_rqst->gcadm_scb, sess_cb->msg_buff );
	break;
    }
    default:  
	if ( IIGCn_static.trace >= 1 )
	    TRdisplay( "%4d     GCN unknown command: %d\n", 
			sess_cb->aid, rqst_cmd->command );
	adm_error( sess_cb, gcadm_rqst->gcadm_scb, E_GC0166_GCN_ADM_INT_ERR );
	break;
    }

    return;
}
 
 
/*
** Name: adm_error
**
** Description:
**	Return error to ADM client.
**
** Input:
**	sess_cb		GCN ADM session control block.
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
adm_error( GCN_SESS_CB *sess_cb, PTR scb, STATUS error_code )
{
    GCADM_ERROR_PARMS	error_parms;
    STATUS		status; 
    char		*buf, *post;

    if ( IIGCn_static.trace >= 4 )
	TRdisplay( "%4d     GCN ADM return error: 0x%x\n", 
		   sess_cb->aid, error_code );

    buf = sess_cb->msg_buff;
    post = sess_cb->msg_buff + sizeof( sess_cb->msg_buff );;

    /* Fill buffer with error message. */
    gcu_erfmt( &buf, post, 1, error_code, (CL_ERR_DESC *)0, 0, NULL );

    MEfill( sizeof( error_parms ), 0, (PTR)&error_parms );
    error_parms.gcadm_scb = (PTR)scb;
    error_parms.rslt_cb = (GCADM_CMPL_FUNC)adm_rslt_cb;
    error_parms.rslt_cb_parms = (PTR)sess_cb;
    error_parms.error_sqlstate = NULL;    
    error_parms.error_code =  error_code;
    error_parms.error_len = strlen( sess_cb->msg_buff ); 
    error_parms.error_text = sess_cb->msg_buff;

    if ( (status = gcadm_rqst_error( &error_parms)) != OK )
    {
	if ( IIGCn_static.trace >= 1 )
	    TRdisplay( "%4d     GCN ADM gcadm_rqst_error failed: 0x%x\n", 
		       sess_cb->aid, status );

	error_log( E_GC0168_GCN_ADM_SESS_ERR ); 
	error_log( status ); 
    }

    return;
}


/*
** Name: adm_stop
**
** Description:
**	Return message to ADM client and stop server.
**
** Input:
**	sess_cb		GCN ADM Session control block.
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
adm_stop( GCN_SESS_CB *sess_cb, PTR scb, char *msg_buff )
{
    GCADM_RSLT_PARMS	rslt_parms;  
    STATUS		status; 

    if ( IIGCn_static.trace >= 4 )
	TRdisplay( "%4d     GCN ADM stopping server: '%s'\n", 
		   sess_cb->aid, msg_buff);

    MEfill( sizeof( rslt_parms ), 0, (PTR)&rslt_parms );
    rslt_parms.gcadm_scb = (PTR)scb; 
    rslt_parms.rslt_cb = (GCADM_CMPL_FUNC)adm_stop_cb;
    rslt_parms.rslt_cb_parms = (PTR)sess_cb; 
    rslt_parms.rslt_len = strlen( msg_buff );
    rslt_parms.rslt_text = msg_buff; 	

    if ( (status = gcadm_rqst_rslt( &rslt_parms )) != OK )
    {
	if ( IIGCn_static.trace >= 1 )
	    TRdisplay( "%4d     GCN ADM gcadm_rqst_rslt() failed: 0x%x\n", 
		       sess_cb->aid, status );
	error_log( E_GC0168_GCN_ADM_SESS_ERR ); 
	error_log( status ); 
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
**	sess_cb		GCN ADM session control block.
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
adm_rslt( GCN_SESS_CB *sess_cb, PTR scb, char *msg_buff  )
{
    GCADM_RSLT_PARMS  	rslt_parms;  
    STATUS		status; 

    if ( IIGCn_static.trace >= 4 )
	TRdisplay( "%4d     GCN ADM return result: '%s'\n", 
		   sess_cb->aid, msg_buff );

    MEfill( sizeof( rslt_parms ), 0, (PTR)&rslt_parms );
    rslt_parms.gcadm_scb = (PTR)scb; 
    rslt_parms.rslt_cb = (GCADM_CMPL_FUNC)adm_rslt_cb;
    rslt_parms.rslt_cb_parms = (PTR)sess_cb; 
    rslt_parms.rslt_len = strlen( msg_buff );
    rslt_parms.rslt_text = msg_buff; 	

    if ( (status = gcadm_rqst_rslt( &rslt_parms )) != OK )
    {
	if ( IIGCn_static.trace >= 1 )
	    TRdisplay( "%4d     GCN ADM result error: 0x%x\n", 
		       sess_cb->aid, status );

	error_log( E_GC0168_GCN_ADM_SESS_ERR ); 
	error_log( status ); 
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
**	sess_cb	GCN ADM Session control block.
**	scb	GCADM Session control block.
**	max_len	Max message length.
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
adm_rsltlst( GCN_SESS_CB *sess_cb, PTR scb, i4 max_len )
{
    GCADM_RSLTLST_PARMS	rsltlst_parms; 
    STATUS		status; 

    if ( IIGCn_static.trace >= 4 )
	TRdisplay("%4d     GCN ADM return list: %d\n", sess_cb->aid, max_len);

    MEfill( sizeof( rsltlst_parms ), 0, (PTR)&rsltlst_parms );
    rsltlst_parms.gcadm_scb = (PTR)scb; 
    rsltlst_parms.rslt_cb = (GCADM_CMPL_FUNC)adm_rslt_cb;
    rsltlst_parms.rslt_cb_parms = (PTR)sess_cb;
    rsltlst_parms.rsltlst_max_len = max_len; 
    rsltlst_parms.rsltlst_q = &sess_cb->rsltlst_q; 
	 
    if ( (status = gcadm_rqst_rsltlst( &rsltlst_parms )) != OK )
    {
	if ( IIGCn_static.trace >= 1 )
	    TRdisplay( "%4d     GCN ADM gcadm_rqst_rsltlst() failed: 0x%x\n", 
		       sess_cb->aid, status );

	error_log( E_GC0168_GCN_ADM_SESS_ERR ); 
	error_log( status ); 
    }

    return; 
}


/*
** Name: adm_done
**
** Description:
**      Complete processing of an ADM client request, and
**      optionally end the session (status != OK).
**
** Input:
**      sess_cb         GCN ADM Session control block.
**      scb             GCADM Session control block.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**       16-Nov-2003 (wansh01)
*/

static void
adm_done( GCN_SESS_CB *sess_cb, PTR scb )
{
    GCADM_DONE_PARMS	done_parms;  
    STATUS		status; 

    /*
    ** Complete the request and wait for next.
    */
    sess_cb->state = GCNS_WAIT;

    MEfill( sizeof( done_parms ), 0, (PTR)&done_parms );
    done_parms.gcadm_scb = scb; 
    done_parms.rqst_cb = (GCADM_RQST_FUNC)adm_rqst_cb;
    done_parms.rqst_cb_parms = (PTR)sess_cb;
    done_parms.rqst_status = OK; 

    if ( (status = gcadm_rqst_done( &done_parms )) != OK )
    {
	if ( IIGCn_static.trace >= 1 )
	    TRdisplay( "%4d     GCN ADM gcadm_rqst_done() failed: 0x%x\n", 
		       sess_cb->aid, status );

	error_log( E_GC0168_GCN_ADM_SESS_ERR ); 
	error_log( status ); 
    }

    return;
} 


/*
** Name: adm_rslt_cb
**
** Description:
**	 Result complete callback routine.
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
    GCN_SESS_CB		*sess_cb = (GCN_SESS_CB *)rslt_cb->rslt_cb_parms;
    QUEUE		*q;

    if ( IIGCn_static.trace >= 4 )
	TRdisplay( "%4d     GCN ADM result callback: 0x%x\n",
		   sess_cb->aid, rslt_cb->rslt_status );

    /*
    ** Empty result list queue and release resources.
    ** (watch out for static memory allocation error result entry)
    */
    while( (q = QUremove( sess_cb->rsltlst_q.q_next )) )  MEfree( (PTR)q );

    if ( rslt_cb->rslt_status == OK )
	adm_done( sess_cb, rslt_cb->gcadm_scb );
    else
    {
	end_session( sess_cb, rslt_cb->rslt_status );

	/*
	** Need to drive GCN end-of-session processing
	** to satisfy quiesce shutdown testing.  New
	** sessions proceed directly to exit when the
	** server is active, so start a new session
	** when last admin session ends.
	*/
	if ( IIGCn_static.adm_sess_count == 0)  gcn_start_session();
    }

    return;
}


/*
** Name: adm_stop_cb
**
** Description:
**	 Stop result complete callback routine.
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
adm_stop_cb( PTR ptr )
{
    GCADM_CMPL_PARMS  	*rslt_cb = (GCADM_CMPL_PARMS *)ptr;
    GCN_SESS_CB 	*sess_cb = (GCN_SESS_CB *)rslt_cb->rslt_cb_parms;
    GCADM_DONE_PARMS	done_parms;  
    STATUS		status; 

    if ( IIGCn_static.trace >= 4 )
	TRdisplay( "%4d     GCN ADM stop callback\n", sess_cb->aid );

    /*
    ** Complete the request and close the session.
    */
    sess_cb->state = GCNS_EXIT;

    MEfill( sizeof( done_parms ), 0, (PTR)&done_parms );
    done_parms.gcadm_scb = (PTR)rslt_cb->gcadm_scb; 
    done_parms.rqst_status = FAIL; 
    done_parms.rqst_cb_parms = (PTR)sess_cb;
    done_parms.rqst_cb = (GCADM_RQST_FUNC)adm_exit_cb;

    if ( (status = gcadm_rqst_done( &done_parms )) != OK )
    {
	if ( IIGCn_static.trace >= 1 )
	    TRdisplay( "%4d     GCN ADM gcadm_rqst_done() failed: 0x%x\n", 
		       sess_cb->aid, status );

	error_log( E_GC0168_GCN_ADM_SESS_ERR ); 
	error_log( status ); 
    }

    return;
}


/*
** Name: adm_exit_cb
**
** Description:
**	Signal server shutdown at end of request.
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
adm_exit_cb( PTR ptr )
{
    GCADM_RQST_PARMS	*gcadm_rqst = (GCADM_RQST_PARMS *)ptr;
    GCN_SESS_CB		*sess_cb = (GCN_SESS_CB *)gcadm_rqst->rqst_cb_parms;

    if ( IIGCn_static.trace >= 4 )  
	TRdisplay( "%4d     GCN ADM stopping server\n", sess_cb->aid );

    end_session( sess_cb, OK );
    GCshut();
    return;
}


/*
** Name: rqst_show_server
**
** Description:
**	show server information 
**
** Input:
**	sess_cb		GCN ADM session control block.
**	scb		GCADM session control block.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 08-Oct-04 (wansh01) 
**	    Created.
**	 22-Jun-06 (rajus01)
**	    Formatted display o/p by 'show server' command to make it similar
**	    to DBMS server display o/p. Implemented REGISTER SERVER command.
**      22-Apr-10 (rajus01) SIR 123621
** 	    Display process ID.          
*/

static void
rqst_show_server( GCN_SESS_CB *sess_cb, PTR scb )
{
    char	*registry_type;
    i4		usr_sess_cnt;
    i4		sys_sess_cnt;
    i4		max_len = 0;
    PID		proc_id;

    usr_sess_cnt = gcn_sess_info( NULL, GCN_USER_COUNT );
    sys_sess_cnt = gcn_sess_info( NULL, GCN_SYS_COUNT );
    PCpid( &proc_id );

    switch( IIGCn_static.registry_type )
    {
	case GCN_REG_NONE:	registry_type = "none";		break;
	case GCN_REG_SLAVE:	registry_type = "slave";	break;
	case GCN_REG_PEER:	registry_type = "peer";		break;
	case GCN_REG_MASTER:	registry_type = "master";	break;
    	default:		registry_type = "?";		break;
    }

    STprintf( sess_cb->msg_buff,
    	      "     Server:     %s ", IIGCn_static.listen_addr ); 
    max_len = build_rsltlst( &sess_cb->rsltlst_q, 
			     sess_cb->msg_buff, max_len );

    STprintf( sess_cb->msg_buff, "     Class:      %s ", GCA_IINMSVR_CLASS ); 
    max_len = build_rsltlst( &sess_cb->rsltlst_q, 
			     sess_cb->msg_buff, max_len );

    STprintf( sess_cb->msg_buff, "     Object:     %s ", "*" ); 
    max_len = build_rsltlst( &sess_cb->rsltlst_q, 
			     sess_cb->msg_buff, max_len );

    STprintf( sess_cb->msg_buff,"     Pid:        %d " , proc_id );
    max_len = build_rsltlst( &sess_cb->rsltlst_q, sess_cb->msg_buff, max_len );


    max_len = build_rsltlst( &sess_cb->rsltlst_q, " ", max_len );
    max_len = build_rsltlst( &sess_cb->rsltlst_q, " Registry ", max_len );

    STprintf( sess_cb->msg_buff,
    	      "     Server:     %s ", IIGCn_static.registry_addr ); 
    max_len = build_rsltlst( &sess_cb->rsltlst_q, 
			     sess_cb->msg_buff, max_len );

    STprintf( sess_cb->msg_buff,
    	      "     Id:         %s ", IIGCn_static.registry_id ); 
    max_len = build_rsltlst( &sess_cb->rsltlst_q, 
			     sess_cb->msg_buff, max_len );

    STprintf( sess_cb->msg_buff, "     Type:       %s ", registry_type ); 
    max_len = build_rsltlst( &sess_cb->rsltlst_q, 
			     sess_cb->msg_buff, max_len );

    max_len = build_rsltlst( &sess_cb->rsltlst_q, " ", max_len );

    STprintf( sess_cb->msg_buff,
    	      " Connected Sessions (includes system and admin sessions) " ); 
    max_len = build_rsltlst( &sess_cb->rsltlst_q, 
			     sess_cb->msg_buff, max_len );

    STprintf( sess_cb->msg_buff, "     Current:    %d", 
    	      usr_sess_cnt + sys_sess_cnt + IIGCn_static.adm_sess_count ); 
    max_len = build_rsltlst( &sess_cb->rsltlst_q, 
			     sess_cb->msg_buff, max_len );

    max_len = build_rsltlst( &sess_cb->rsltlst_q, " ", max_len );

    STprintf( sess_cb->msg_buff,
    	      " Active Sessions " ); 
    max_len = build_rsltlst( &sess_cb->rsltlst_q, 
			     sess_cb->msg_buff, max_len );

    STprintf( sess_cb->msg_buff, "     Current:    %d", usr_sess_cnt ); 
    max_len = build_rsltlst( &sess_cb->rsltlst_q, 
			     sess_cb->msg_buff, max_len );

    STprintf( sess_cb->msg_buff,
    	      "     Limit:      %d ", IIGCn_static.max_user_sessions ); 
    max_len = build_rsltlst( &sess_cb->rsltlst_q, 
			     sess_cb->msg_buff, max_len );

    STprintf( sess_cb->msg_buff, "     High Water: %d ",
					 IIGCn_static.highwater ); 
    max_len = build_rsltlst( &sess_cb->rsltlst_q, 
			     sess_cb->msg_buff, max_len );

    max_len = build_rsltlst( &sess_cb->rsltlst_q, " ", max_len );

    STprintf(sess_cb->msg_buff, " Local vnode:    %s", IIGCn_static.lcl_vnode); 
    max_len = build_rsltlst( &sess_cb->rsltlst_q, 
			     sess_cb->msg_buff, max_len );

    STprintf(sess_cb->msg_buff, " Remote vnode:   %s", IIGCn_static.rmt_vnode); 
    max_len = build_rsltlst( &sess_cb->rsltlst_q, 
			     sess_cb->msg_buff, max_len );

    STprintf( sess_cb->msg_buff, 
    	      " Default class:  %s" , IIGCn_static.svr_type ); 
    max_len = build_rsltlst( &sess_cb->rsltlst_q, 
    			     sess_cb->msg_buff, max_len );

    adm_rsltlst( sess_cb, scb, max_len );
    return;
}


/*
** Name: rqst_show_adm
**
** Description:
**	Display ADMIN session info.  Adds entry to result 
**	list queue.  If session ID is provided, only info 
**	for matching session is displayed.  Formatting is 
**	dependent on command type.  Updates max message 
**	length.
**
** Input:
**	rsltlst		Result list QUEUE.
**	sid		Session ID, may be NULL.
**	cmd		Command type.
**	max_len		Max message length.
**
** Output:
**	None.
**
** Returns:
**	i4		Max message length.
**
** History:
**	08-Jul-04 (wansh01) 
**	    Created.
**	 6-Aug-04 (gordy)
**	    Enhanced session info display.
*/

static i4 
rqst_show_adm( QUEUE *rsltlst, PTR sid, u_i2 cmd, i4 max_len )
{
    QUEUE *q;

    for( q = gcn_sess_q.q_next; q != &gcn_sess_q; q = q->q_next )
    {
	GCN_SESS_CB *sess_cb = (GCN_SESS_CB *)q;

	if ( sid  &&  sid != (PTR)sess_cb )  continue;

	/*
	** Format and return session info heading.
	*/
	switch( cmd )
	{
	case GCN_CMD_FRMT_ADM_SESS:
	case GCN_CMD_FRMT_ALL_SESS:
	case GCN_CMD_FRMT_SESSID:
	    STprintf( sess_cb->msg_buff, 
	    	      ">>>>>Session %p:%d<<<<<", sess_cb, sess_cb->aid );
	    break;

	case GCN_CMD_SHOW_ADM_SESS_STATS:
	case GCN_CMD_SHOW_ALL_SESS_STATS:
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
	case GCN_CMD_SHOW_ADM_SESS_FRMT:
	case GCN_CMD_SHOW_ALL_SESS_FRMT:
	case GCN_CMD_FRMT_ADM_SESS:
	case GCN_CMD_FRMT_ALL_SESS:
	case GCN_CMD_FRMT_SESSID:
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
** Name: rqst_show_sessions
**
** Description:
**	Display usr/sys session info.  Adds entry to result 
**	list queue.  If session ID is provided, only info 
**	for matching session is displayed.  Formatting is 
**	dependent on command type.  Updates max message 
**	length.
**
** Input:
**	rsltlst		Result list QUEUE.
**	sid		Session ID, may be NULL.
**	cmd		Command type.
**	max_len		Max message length.
**
** Output:
**      None.
**
** Returns:
**	i4		Max message length.
**
** History:
**      08-Jul-04 (wansh01)
**          Created.
**       6-Aug-04 (gordy)
**          Enhanced session info display.
**	11-Aug-09 (gordy)
**	    Free resources.
*/

static i4 
rqst_show_sessions( QUEUE *rsltlst, PTR sid, u_i2 cmd, i4 max_len )
{
    GCN_SESS_INFO	*info, *iptr = NULL; 
    char		msg_buff[ ER_MAX_LEN ];
    i4			count, cnt_flag, info_flag;

    /*
    ** First determine number of active sessions.
    */
    switch( cmd )
    {
    case GCN_CMD_SHOW_USR_SESS:
    case GCN_CMD_SHOW_USR_SESS_STATS:
    case GCN_CMD_SHOW_USR_SESS_FRMT:
    case GCN_CMD_FRMT_USR_SESS:
    	cnt_flag = GCN_USER_COUNT;
	info_flag = GCN_USER_INFO;
	break;

    case GCN_CMD_SHOW_SYS_SESS:
    case GCN_CMD_SHOW_SYS_SESS_STATS:
    case GCN_CMD_SHOW_SYS_SESS_FRMT:
    case GCN_CMD_FRMT_SYS_SESS:
    	cnt_flag = GCN_SYS_COUNT;
	info_flag = GCN_SYS_INFO;
	break;

    case GCN_CMD_SHOW_ALL_SESS:
    case GCN_CMD_SHOW_ALL_SESS_STATS:
    case GCN_CMD_SHOW_ALL_SESS_FRMT:
    case GCN_CMD_FRMT_ALL_SESS:
    case GCN_CMD_FRMT_SESSID:
    	cnt_flag = GCN_ALL_COUNT;
	info_flag = GCN_ALL_INFO;
	break;
    }

    if ( ! (count = gcn_sess_info( NULL, cnt_flag )) )  
    {
	STprintf( msg_buff, "No user sessions" ); 
	max_len = build_rsltlst( rsltlst, msg_buff, max_len );
	return( max_len );
    }

    iptr = (GCN_SESS_INFO *)MEreqmem( 0, count * sizeof( GCN_SESS_INFO ), 
    				      TRUE, NULL);

    /*
    ** Now load and display active session info.
    */
    count = iptr ? gcn_sess_info( (PTR)iptr, info_flag ) : 0;

    for( info = iptr; count > 0; count--, info++ )  
    {
	if ( sid  &&  sid != info->sess_id )  continue;

	/*
	** Format and return session info heading.
	*/
	switch( cmd )
	{
	case GCN_CMD_FRMT_USR_SESS:
	case GCN_CMD_FRMT_SYS_SESS:
	case GCN_CMD_FRMT_ALL_SESS:
	case GCN_CMD_FRMT_SESSID:
	    if ( info->assoc_id < 0 )
		STprintf( msg_buff, ">>>>>Session %p<<<<<", info->sess_id );
	    else
		STprintf( msg_buff, ">>>>>Session %p:%d<<<<<", 
			  info->sess_id, info->assoc_id );
	    break;

	case GCN_CMD_SHOW_USR_SESS_STATS:
	case GCN_CMD_SHOW_SYS_SESS_STATS:
	case GCN_CMD_SHOW_ALL_SESS_STATS:
	    /* !!! TODO: format stats - fall through for now */

	default:
	    if ( info->assoc_id < 0 )
		STprintf( msg_buff, "Session %p (%-24s) Type: %s state: %s",
			  info->sess_id, info->user_id, 
			  (info->is_system) ? "SYSTEM" : "USER", info->state );
	    else
		STprintf( msg_buff, "Session %p:%d (%-24s) Type: %s state: %s", 
			  info->sess_id, info->assoc_id, info->user_id,
			  (info->is_system) ? "SYSTEM" : "USER", info->state );
	    break;
	}

	max_len = build_rsltlst( rsltlst, msg_buff, max_len );

	/*
	** Return FORMATTED info
	*/
	switch( cmd )
	{
	case GCN_CMD_SHOW_USR_SESS_FRMT:
	case GCN_CMD_FRMT_USR_SESS:
	case GCN_CMD_SHOW_SYS_SESS_FRMT:
	case GCN_CMD_FRMT_SYS_SESS:
	case GCN_CMD_SHOW_ALL_SESS_FRMT:
	case GCN_CMD_FRMT_ALL_SESS:
	case GCN_CMD_FRMT_SESSID:
            if ( info->is_system )
            {
		if ( info->assoc_id > 0 )
		{
		    STprintf( msg_buff, 
			      "    Server Class: %s   Server Addr: %s", 
			      info->svr_class, info->svr_addr ); 
		    max_len = build_rsltlst( rsltlst, msg_buff, max_len );
	        }
	        else
		{
		    STprintf( msg_buff, 
		    "    No format info available for this system session" ); 
		    max_len = build_rsltlst( rsltlst, msg_buff, max_len );
		}
	    }
	    else
	    {
		    STprintf( msg_buff, 
			     "    No format info available for this user session" ); 
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
** Name: rqst_set_trace
**
** Description:
**	 Set trace variables for GCN	
**
** Input:
**	sess		ptr to GCN_SESS_CB
**	scb		GCADM session control block.
**	trace_id	Trace ID.
**	level		Trace level.
**
** Output:
**	None.
**
** Returns:
**	void 
**
** History:
**	 04-Aug-06 (rajus01) 
**	    Created.
*/

static void
rqst_set_trace( GCN_SESS_CB *sess_cb, PTR scb, char *trace_id, i4 level )
{
    char	val[ 128 ];
    i4		i;

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
** Name: end_session
**
** Description:
**	Free session resources.
**
** Input:
**	sess_cb		Session control block.
**	status		Session status.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	15-Sep-06 (gordy)
**	    Created.
**	11-Aug-09 (gordy)
**	    Free username dynamic resources.
*/

static STATUS
end_session( GCN_SESS_CB *sess_cb, STATUS status )
{
    if ( status != OK  &&  status != FAIL )
    {
	if ( IIGCn_static.trace >= 1 )
	    TRdisplay( "%4d     GCN ADM gcadm_session() failed: 0x%x\n", 
		       sess_cb->aid, status );

	error_log( E_GC0168_GCN_ADM_SESS_ERR ); 
	error_log( status ); 
    }

    QUremove( &sess_cb->sess_q );
    if ( sess_cb->user_id != user_default )  MEfree( (PTR)sess_cb->user_id );
    MEfree( (PTR)sess_cb );
    IIGCn_static.adm_sess_count--; 
    return;
}


/*
** Name: build_rsltlst
**
** Description:
**	Add entry to result list queue.
**
** Input:
**	rsltlst_q	Result list queue.
**	msg_buff	Result message buffer.
**	max_len		Maximum prior message length.
**
** Output:
**	None.
**
** Returns:
**	i4		Updated maximum message length.
**
** History:
**	 08-Jul-04 (wansh01) 
*/

static i4    
build_rsltlst( QUEUE *rsltlst_q, char *msg_buff, i4 max_len )
{
    ADM_RSLT	*rslt;
    i4		msg_len = strlen( msg_buff ); 

    /*
    ** Allocate result list entry with space for message text.
    */
    if ( ! (rslt = (ADM_RSLT *)MEreqmem( 0, sizeof( ADM_RSLT ) + msg_len, 
    					 0, NULL )) )
    {
	if ( IIGCn_static.trace >= 1 )
	    TRdisplay( "build_rsltlst entry: memory alloc failed\n" );
    }
    else
    {
	if ( IIGCn_static.trace >= 4 )
	    TRdisplay( "build_rsltlst entry: %p, %d, '%s'\n", 
	    		rslt, msg_len, msg_buff ); 

    	rslt->rslt.rslt_len = msg_len;
	rslt->rslt.rslt_text = rslt->msg_buff;
	STcopy( msg_buff, rslt->msg_buff );
    }

    QUinsert( &(rslt->rslt.rslt_q), rsltlst_q->q_prev );
    return( max( msg_len, max_len ) ); 
}


