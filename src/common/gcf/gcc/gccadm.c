/*
** Copyright (c) 2003, 2010 Ingres Corporation.
*/

#include <compat.h>
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
#include <gcm.h>
#include <gcu.h>
#include <gcccl.h>
#include <gcc.h>
#include <gccal.h>
#include <gcctl.h>
#include <gccer.h>
#include <gccgref.h>
#include <gcadm.h>
#include <gcadmint.h>

/*
** Name: gccadm.c  
**
** Description:
**	GCADM interface with GCC server and iimonitor. 
**
** History:
**	 9-10-2003 (wansh01) 
**	    Created.
**	 7-Jul-2004 (wansh01) 
**	    Support SHOW commands.
**	 6-Aug-04 (gordy)
**	    Enhanced session info display.
**	24-Apr-06 (usha)
**	    Added more commands.
**              [force] option for stop server command instead of crash server.
**              Tracing enable/disable.
**              display statistics for various sessions.
**	        Removed all references to INBOUND/OUTBOUND session types. 
**	        They will be treated as sub-types of USER sessions.
**	22-Jun-06 (usha)
**	    Formatted the disply o/p of "show server" command to look
**	    similar to DBMS server "show server" command o/p. Remove
**	    gcc_server_count() method.
**	 7-Jul-06 (usha)
**	    Added support to display registry info in the 'show server'
**	    command o/p
**	18-Aug-06 (usha)
**	    Added support for 'set trace' commands.
**	03-Oct-06 (usha)
**	    Re-worked the entire code to scale well as per Gordy's GCN ADMIN
**	    implementation.
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	21-Jul-09 (gordy)
**	    Remove username length restrictions.
**	22-Apr-10 (rajus01) SIR 123621
**	    Display process ID.
*/

/*
** GCC ADM Session control block.
*/

typedef struct
{
    QUEUE	sess_q;  
    i4		aid;
    i4		state;
    char	*user_id;
    QUEUE	rsltlst_q;
    char	msg_buff[ ER_MAX_LEN ];
} GCC_SESS_CB;

static QUEUE            gcc_sess_q;
static char		*user_default = "<default>";

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

#define GCC_TOKEN 		0x00

#define GCC_KW_ALL		0x01
#define GCC_KW_ADM		0x02
#define GCC_KW_CLOSED		0x03
#define GCC_KW_CRASH		0x04
#define GCC_KW_FORCE		0x05
#define GCC_KW_FORMAT		0x06
#define GCC_KW_FORMATTED 	0x07
#define GCC_KW_HELP		0x08
#define GCC_KW_LSTN		0x09
#define GCC_KW_OPEN		0x0A
#define GCC_KW_REGISTER		0x0B
#define GCC_KW_REMOVE		0x0C
#define GCC_KW_SERVER		0x0D
#define GCC_KW_SESS		0x0E
#define GCC_KW_SET		0x0F
#define GCC_KW_SHOW		0x10
#define GCC_KW_SHUT		0x11
#define GCC_KW_SHUTDOWN		0x12
#define GCC_KW_STATS		0x13
#define GCC_KW_STOP		0x14
#define GCC_KW_SYSTEM		0x15
#define GCC_KW_TRACE		0x16
#define GCC_KW_USER		0x17
#define	GCC_KW_LOG		0x18
#define	GCC_KW_NONE		0x19
#define	GCC_KW_LEVEL		0x1A



static ADM_KEYWORD keywords[] =
{
    { "ALL",		GCC_KW_ALL },
    { "ADMIN",		GCC_KW_ADM },
    { "CLOSED",		GCC_KW_CLOSED },
    { "CRASH",		GCC_KW_CRASH },
    { "FORCE",		GCC_KW_FORCE },
    { "FORMAT",		GCC_KW_FORMAT },
    { "FORMATTED",	GCC_KW_FORMATTED },
    { "HELP",		GCC_KW_HELP },
    { "LISTEN",		GCC_KW_LSTN },
    { "OPEN",		GCC_KW_OPEN },
    { "REGISTER",	GCC_KW_REGISTER },
    { "REMOVE",		GCC_KW_REMOVE },
    { "SERVER",		GCC_KW_SERVER },
    { "SESSIONS",	GCC_KW_SESS },
    { "SET",		GCC_KW_SET },
    { "SHOW",		GCC_KW_SHOW },
    { "SHUT",		GCC_KW_SHUT },
    { "SHUTDOWN",	GCC_KW_SHUTDOWN },
    { "STATS",		GCC_KW_STATS },
    { "STOP",		GCC_KW_STOP },
    { "SYSTEM",		GCC_KW_SYSTEM },
    { "TRACE",		GCC_KW_TRACE },
    { "USER",		GCC_KW_USER },
    { "LOG",		GCC_KW_LOG },
    { "NONE",		GCC_KW_NONE },
    { "LEVEL",		GCC_KW_LEVEL },
};


#define	GCC_CMD_HELP			1
#define	GCC_CMD_SHOW_SVR		2
#define	GCC_CMD_SHOW_SVR_LSTN		3
#define	GCC_CMD_SHOW_SVR_SHUT		4
#define	GCC_CMD_SET_SVR_SHUT		5
#define	GCC_CMD_SET_SVR_CLSD		6
#define	GCC_CMD_SET_SVR_OPEN		7
#define	GCC_CMD_STOP_SVR		8
#define	GCC_CMD_STOP_SVR_FORCE		9
#define	GCC_CMD_CRASH_SVR		10
#define	GCC_CMD_REGISTER_SVR		11
#define	GCC_CMD_SHOW_ADM_SESS		12
#define	GCC_CMD_SHOW_USER_SESS		13
#define	GCC_CMD_SHOW_SYS_SESS		14
#define	GCC_CMD_SHOW_ALL_SESS		15
#define	GCC_CMD_SHOW_USER_SESS_FMT	16
#define	GCC_CMD_SHOW_ADM_SESS_FMT	17
#define	GCC_CMD_SHOW_SYS_SESS_FMT	18
#define	GCC_CMD_SHOW_ALL_SESS_FMT	19
#define	GCC_CMD_SHOW_USER_SESS_STATS	20
#define	GCC_CMD_SHOW_ADM_SESS_STATS	21
#define	GCC_CMD_SHOW_SYS_SESS_STATS	22
#define	GCC_CMD_SHOW_ALL_SESS_STATS	23
#define GCC_CMD_FRMT_ADM_SESS           24
#define GCC_CMD_FRMT_USER_SESS          25
#define GCC_CMD_FRMT_SYS_SESS           26
#define GCC_CMD_FRMT_ALL_SESS           27
#define	GCC_CMD_FRMT_SESSID		28
#define	GCC_CMD_REMOVE_SESSID		29
#define	GCC_CMD_SET_TRACE_LOG_NONE	30
#define	GCC_CMD_SET_TRACE_LOG_FNAME	31
#define	GCC_CMD_SET_TRACE_LEVEL		32
#define	GCC_CMD_SET_TRACE_ID_LEVEL	33

/* Help - List all the commands */
static ADM_ID cmd_help[] = 
	{ GCC_KW_HELP };

/* Server States Management Commands */
    /* Set Commands */
static ADM_ID cmd_set_svr_shut[] = 
	{ GCC_KW_SET, GCC_KW_SERVER, GCC_KW_SHUT };
static ADM_ID cmd_set_svr_clsd[] = 
	{ GCC_KW_SET, GCC_KW_SERVER, GCC_KW_CLOSED };
static ADM_ID cmd_set_svr_open[] =
	{ GCC_KW_SET, GCC_KW_SERVER, GCC_KW_OPEN };
    /* Show Commands */
static ADM_ID cmd_show_svr[] = 
	{ GCC_KW_SHOW, GCC_KW_SERVER };
static ADM_ID cmd_show_svr_lstn[] =
	{ GCC_KW_SHOW, GCC_KW_SERVER, GCC_KW_LSTN };
static ADM_ID cmd_show_svr_shut[] =
{ GCC_KW_SHOW, GCC_KW_SERVER, GCC_KW_SHUTDOWN};

/* Normal/Forceful Server Shutdown Commands */
static ADM_ID cmd_stop_svr[] = 
	{ GCC_KW_STOP, GCC_KW_SERVER };
static ADM_ID cmd_stop_svr_force[] =
	{ GCC_KW_STOP, GCC_KW_SERVER, GCC_KW_FORCE };
static ADM_ID cmd_crash_svr[] = 
	{ GCC_KW_CRASH, GCC_KW_SERVER };

/* Register Server Command */
static ADM_ID cmd_register_svr[] =
	{ GCC_KW_REGISTER, GCC_KW_SERVER };

/* Server Sessions Managment Commands */
      /* Show Commands */
static ADM_ID cmd_show_sess[] = 
	{ GCC_KW_SHOW, GCC_KW_SESS };
static ADM_ID cmd_show_sess_fmt[] = 
	{ GCC_KW_SHOW, GCC_KW_SESS, GCC_KW_FORMATTED };
static ADM_ID cmd_show_all_sess[] = 
	{ GCC_KW_SHOW, GCC_KW_ALL,GCC_KW_SESS };
static ADM_ID cmd_show_all_sess_fmt[] = 
	{ GCC_KW_SHOW, GCC_KW_ALL, GCC_KW_SESS, GCC_KW_FORMATTED };
static ADM_ID cmd_show_adm_sess[] = 
	{ GCC_KW_SHOW, GCC_KW_ADM,GCC_KW_SESS };
static ADM_ID cmd_show_adm_sess_fmt[] = 
	{ GCC_KW_SHOW, GCC_KW_ADM,GCC_KW_SESS, GCC_KW_FORMATTED };
static ADM_ID cmd_show_user_sess[] = 
	{ GCC_KW_SHOW, GCC_KW_USER,GCC_KW_SESS };
static ADM_ID cmd_show_user_sess_fmt[] = 
	{ GCC_KW_SHOW, GCC_KW_USER, GCC_KW_SESS, GCC_KW_FORMATTED };
static ADM_ID cmd_show_sys_sess[] = 
	{ GCC_KW_SHOW, GCC_KW_SYSTEM,GCC_KW_SESS };
static ADM_ID cmd_show_sys_sess_fmt[] = 
	{ GCC_KW_SHOW, GCC_KW_SYSTEM,GCC_KW_SESS, GCC_KW_FORMATTED };
static ADM_ID cmd_show_sess_stats[] =
	{ GCC_KW_SHOW, GCC_KW_SESS, GCC_KW_STATS };
static ADM_ID cmd_show_user_sess_stats[] =
	{ GCC_KW_SHOW, GCC_KW_USER, GCC_KW_SESS, GCC_KW_STATS };
static ADM_ID cmd_show_adm_sess_stats[] = 
	{ GCC_KW_SHOW, GCC_KW_ADM, GCC_KW_SESS, GCC_KW_STATS };
static ADM_ID cmd_show_sys_sess_stats[] =
	{ GCC_KW_SHOW, GCC_KW_SYSTEM, GCC_KW_SESS, GCC_KW_STATS };
static ADM_ID cmd_show_all_sess_stats[] = 
	{ GCC_KW_SHOW, GCC_KW_ALL, GCC_KW_SESS, GCC_KW_STATS };

     /* Format Commands - User sessions */
static ADM_ID cmd_format_user_sess[] = 
	{ GCC_KW_FORMAT, GCC_KW_USER, GCC_KW_SESS };
static ADM_ID cmd_format_sess[] = 
	{ GCC_KW_FORMAT,GCC_KW_SESS };
static ADM_ID cmd_format_user[] = 
	{ GCC_KW_FORMAT, GCC_KW_USER };

     /* Format Commands - Admin sessions */
static ADM_ID cmd_format_adm_sess[] =
	{ GCC_KW_FORMAT, GCC_KW_ADM, GCC_KW_SESS };
static ADM_ID cmd_format_adm[] = 
	{ GCC_KW_FORMAT, GCC_KW_ADM };

     /* Format Commands - System sessions */
static ADM_ID cmd_format_sys_sess[] = 
	{ GCC_KW_FORMAT, GCC_KW_SYSTEM, GCC_KW_SESS };
static ADM_ID cmd_format_sys[] = 
	{ GCC_KW_FORMAT, GCC_KW_SYSTEM };

     /* Format Commands - user, admin, system sessions */
static ADM_ID cmd_format_all[] = 
	{ GCC_KW_FORMAT, GCC_KW_ALL };
static ADM_ID cmd_format_all_sess[] =
	{ GCC_KW_FORMAT, GCC_KW_ALL, GCC_KW_SESS };
 
    /* Format Commands - on a given session ID */
static ADM_ID cmd_format_sessid[] = 
	{ GCC_KW_FORMAT, GCC_TOKEN };

    /* Remove Command - On a given session ID */
static ADM_ID cmd_remove_sessid[] = 
	{ GCC_KW_REMOVE, GCC_TOKEN };

/* Trace Commands */
static ADM_ID cmd_set_trace_log_none[] = 
	{ GCC_KW_SET, GCC_KW_TRACE, GCC_KW_LOG, GCC_KW_NONE };
static ADM_ID cmd_set_trace_log_fname[] = 
	{ GCC_KW_SET, GCC_KW_TRACE, GCC_KW_LOG, GCC_TOKEN };
static ADM_ID cmd_set_trace_level[] = 
	{ GCC_KW_SET, GCC_KW_TRACE, GCC_KW_LEVEL, GCC_TOKEN };
static ADM_ID cmd_set_trace_id_level[] = 
	{ GCC_KW_SET, GCC_KW_TRACE, GCC_TOKEN, GCC_TOKEN };

static ADM_COMMAND commands[] =
{
    { GCC_CMD_HELP,
	ARR_SIZE( cmd_help ), cmd_help },
    { GCC_CMD_SET_SVR_SHUT,
	ARR_SIZE( cmd_set_svr_shut ), cmd_set_svr_shut },
    { GCC_CMD_SET_SVR_CLSD,
	ARR_SIZE( cmd_set_svr_clsd ), cmd_set_svr_clsd },
    { GCC_CMD_SET_SVR_OPEN,
	ARR_SIZE( cmd_set_svr_open ), cmd_set_svr_open },
    { GCC_CMD_STOP_SVR,
	ARR_SIZE( cmd_stop_svr ), cmd_stop_svr },
    { GCC_CMD_STOP_SVR_FORCE,
	ARR_SIZE( cmd_stop_svr_force ), cmd_stop_svr_force },
    { GCC_CMD_CRASH_SVR,
	ARR_SIZE( cmd_crash_svr ), cmd_crash_svr },
    { GCC_CMD_REGISTER_SVR,
	ARR_SIZE( cmd_register_svr ), cmd_register_svr },
    { GCC_CMD_SHOW_SVR,
	ARR_SIZE( cmd_show_svr ), cmd_show_svr },
    { GCC_CMD_SHOW_SVR_LSTN,
	ARR_SIZE( cmd_show_svr_lstn ), cmd_show_svr_lstn },
    { GCC_CMD_SHOW_SVR_SHUT,
	ARR_SIZE( cmd_show_svr_shut ), cmd_show_svr_shut },
    { GCC_CMD_SHOW_USER_SESS,
	ARR_SIZE( cmd_show_user_sess ) , cmd_show_user_sess },
    { GCC_CMD_SHOW_USER_SESS,
	ARR_SIZE( cmd_show_sess ), cmd_show_sess },
    { GCC_CMD_SHOW_USER_SESS_FMT, 
	ARR_SIZE( cmd_show_sess_fmt ), cmd_show_sess_fmt},
    { GCC_CMD_SHOW_USER_SESS_FMT,
	ARR_SIZE( cmd_show_user_sess_fmt ), cmd_show_user_sess_fmt},
    { GCC_CMD_SHOW_ADM_SESS,
	ARR_SIZE( cmd_show_adm_sess ), cmd_show_adm_sess},
    { GCC_CMD_SHOW_ADM_SESS_FMT,
	ARR_SIZE( cmd_show_adm_sess_fmt ), cmd_show_adm_sess_fmt},
    { GCC_CMD_SHOW_SYS_SESS,
	ARR_SIZE( cmd_show_sys_sess ), cmd_show_sys_sess},
    { GCC_CMD_SHOW_SYS_SESS_FMT,
	ARR_SIZE( cmd_show_sys_sess_fmt ), cmd_show_sys_sess_fmt},
    { GCC_CMD_SHOW_ALL_SESS, 
	ARR_SIZE( cmd_show_all_sess ), cmd_show_all_sess},
    { GCC_CMD_SHOW_ALL_SESS_FMT,
	ARR_SIZE( cmd_show_all_sess_fmt ), cmd_show_all_sess_fmt},
    { GCC_CMD_SHOW_ALL_SESS_FMT, 
	ARR_SIZE( cmd_show_all_sess ), cmd_show_all_sess},
    { GCC_CMD_SHOW_USER_SESS_STATS,
	ARR_SIZE( cmd_show_sess_stats ), cmd_show_sess_stats},
    { GCC_CMD_SHOW_USER_SESS_STATS,
	ARR_SIZE( cmd_show_user_sess_stats ), cmd_show_user_sess_stats},
    { GCC_CMD_SHOW_ADM_SESS_STATS,
	ARR_SIZE( cmd_show_adm_sess_stats ), cmd_show_adm_sess_stats},
    { GCC_CMD_SHOW_SYS_SESS_STATS, 
	ARR_SIZE( cmd_show_sys_sess_stats ), cmd_show_sys_sess_stats},
    { GCC_CMD_SHOW_ALL_SESS_STATS, 
	ARR_SIZE( cmd_show_all_sess_stats ), cmd_show_all_sess_stats},
    { GCC_CMD_FRMT_ADM_SESS,
	ARR_SIZE( cmd_format_adm ), cmd_format_adm},
    { GCC_CMD_FRMT_ADM_SESS,
        ARR_SIZE( cmd_format_adm_sess ), cmd_format_adm_sess },
    { GCC_CMD_FRMT_USER_SESS,
        ARR_SIZE( cmd_format_user ), cmd_format_user },
    { GCC_CMD_FRMT_USER_SESS,
        ARR_SIZE( cmd_format_user_sess ), cmd_format_user_sess },
    { GCC_CMD_FRMT_SYS_SESS,
        ARR_SIZE( cmd_format_sys ), cmd_format_sys },
    { GCC_CMD_FRMT_SYS_SESS,
        ARR_SIZE( cmd_format_sys_sess ), cmd_format_sys_sess },
    { GCC_CMD_FRMT_ALL_SESS,
        ARR_SIZE( cmd_format_all ), cmd_format_all },
    { GCC_CMD_FRMT_ALL_SESS,
        ARR_SIZE( cmd_format_all_sess ), cmd_format_all_sess },
    { GCC_CMD_FRMT_SESSID,
	ARR_SIZE( cmd_format_sessid ), cmd_format_sessid},
    { GCC_CMD_REMOVE_SESSID,
	ARR_SIZE( cmd_remove_sessid ), cmd_remove_sessid},
    { GCC_CMD_SET_TRACE_LOG_NONE,
	ARR_SIZE( cmd_set_trace_log_none ), cmd_set_trace_log_none},
    { GCC_CMD_SET_TRACE_LOG_FNAME, 
	ARR_SIZE( cmd_set_trace_log_fname ), cmd_set_trace_log_fname},
    { GCC_CMD_SET_TRACE_LEVEL,
	ARR_SIZE( cmd_set_trace_level ), cmd_set_trace_level},
    { GCC_CMD_SET_TRACE_ID_LEVEL,
	ARR_SIZE( cmd_set_trace_id_level ) , cmd_set_trace_id_level},
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
#define ADM_REM_TXT		"Session removed"
#define ADM_REM_SYSERR_TXT	"System sessions cannot be removed"
#define ADM_REM_ADMERR_TXT	"Admin sessions cannot be removed"


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
    "Others:",
    "     SET TRACE LOG <path> | NONE",
    "     SET TRACE LEVEL | <id> <lvl>",
    "     HELP",
    "     QUIT",
};

/*
** GCC_CMD_MAX_TOKEN is max tokens in valid command line
*/

#define GCC_CMD_MAX_TOKEN 	4	

typedef struct
{
    char  proto[GCC_L_PROTOCOL + 1];		/* Protocol */
    char  host[GCC_L_NODE + 1];    		/* Node */
    char  port[GCC_L_PORT + 1];    		/* Port */
    char  addr[GCC_L_PORT + 1];    		/* Addr */

}GCC_PNP_INFO;

/*
** Server info for admin SHOW SERVER command
*/
typedef struct
{

#define	GCC_REG_TYP_LEN		7
#define	GCC_PNP_LEN		13            /* PCT entries */

   char registry_type[GCC_REG_TYP_LEN + 1];
   GCC_PNP_INFO  gcc_pnp_info[GCC_PNP_LEN]; 
   GCC_PNP_INFO  gcc_reg_info[GCC_PNP_LEN]; 
   i4	gcc_pnp_info_len;
   i4	gcc_reg_info_len;

} GCC_SVR_INFO;

typedef struct
{
   char  tr_id[5];
   char  tr_classid[32];

}GCC_TRACE_INFO;

static GCC_TRACE_INFO trace_ids[]=
{
     { "GCA",  "exp.gcf.gca.trace_level" },
     { "GCS",  "exp.gcf.gcs.trace_level" },
};

#define	GCC_USER_COUNT		1
#define GCC_SYS_COUNT		2
#define GCC_ALL_COUNT		3
#define	GCC_USER_INFO		4
#define GCC_SYS_INFO		5
#define GCC_ALL_INFO		6
#define	GCC_SESS_USER		7 
#define	GCC_SESS_SYS		8
#define	GCC_SESS_ADMIN		9

static GCC_SVR_INFO gcc_svr_info ZERO_FILL;

static bool srv_info_init = FALSE;

#define GCCS_INIT      0               /* init */
#define GCCS_WAIT      1               /* wait */
#define GCCS_PROCESS   2               /* process      */
#define GCCS_EXIT      3               /* exit         */
#define GCCS_CNT       4

static  char    *sess_states[ GCCS_CNT ] =
{
    "INIT", "WAIT", "PROCESS",
    "EXIT",
};

static char *al_states [ SAMAX ] =
{
    "SACLD", "SAIAL", "SARAL", "SADTI", "SADSP", "SACTA", "SACTP",
    "SASHT", "SADTX", "SADSX", "SARJA", "SARAA", "SAQSC", "SACLG",
    "SARAB", "SACAR", "SAFSL", "SARAF", "SACMD",
};

static char *tl_states[ STMAX ] =
{
    "STCLD", "STWNC", "STWCC", "STWCR", "STWRS", "STOPN", "STWDC",
    "STSDP", "STOPX", "STSDX", "STWND", "STCLG",
};

/*
** Forward references
*/

static void		adm_rslt( GCC_SESS_CB *, PTR , char * );
static void		adm_rsltlst( GCC_SESS_CB *, PTR, i4 );
static void		adm_error( GCC_SESS_CB *, PTR,  STATUS );
static void		adm_done( GCC_SESS_CB *, PTR );
static void		adm_stop( GCC_SESS_CB *, PTR, char * );

static void		adm_rqst_cb( PTR );
static void		adm_rslt_cb( PTR );
static void		adm_stop_cb( PTR );
static void		adm_exit_cb( PTR );

static void		error_log( STATUS );
static STATUS           end_session( GCC_SESS_CB *, STATUS );
static i4		build_rsltlst( QUEUE *, char *, i4 );


static void		rqst_show_server( GCC_SESS_CB *, PTR);
static i4		rqst_show_adm( QUEUE *, PTR, u_i2, i4 );
static i4               rqst_show_sessions( QUEUE *, PTR, u_i2, i4 );
static void             rqst_set_trace( GCC_SESS_CB *, PTR, char *, i4 );

static i4 		verify_sid( PTR );
static i4 		gcc_sess_info( PTR, i4 );
static void 		rqst_sys_stats( GCC_CCB * , char *, char * );


/*
** Name: adm_stop_cb
**
** Description:
**	Stop result complete callback routine.
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
    
    GCADM_CMPL_PARMS	*rslt_cb = (GCADM_CMPL_PARMS *)ptr;
    GCC_SESS_CB		*sess_cb = (GCC_SESS_CB *)rslt_cb->rslt_cb_parms;
    GCADM_DONE_PARMS    done_parms;
    STATUS              status;


    if ( IIGCc_global->gcc_trace_level >= 4 )  
	TRdisplay( "%4d     GCC ADM stop callback\n", sess_cb->aid );

    /*
    ** Complete the request and close the session.
    */
    sess_cb->state = GCCS_EXIT;

    MEfill( sizeof( done_parms ), 0, (PTR)&done_parms );
    done_parms.gcadm_scb = (PTR)rslt_cb->gcadm_scb;
    done_parms.rqst_status = FAIL;
    done_parms.rqst_cb_parms = (PTR)sess_cb;
    done_parms.rqst_cb = (GCADM_RQST_FUNC)adm_exit_cb;

    if ( (status = gcadm_rqst_done( &done_parms )) != OK )
    {
    	    if ( IIGCc_global->gcc_trace_level >= 1 )  
            TRdisplay( "%4d     GCC ADM gcadm_rqst_done() failed: 0x%x\n",
                       sess_cb->aid, status );

	error_log ( E_GC200B_ADM_SESS_ERR ); 
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
    GCADM_RQST_PARMS	*gcadm_rqst = (GCADM_RQST_PARMS *)ptr;
    GCC_SESS_CB 	*sess_cb = (GCC_SESS_CB *)gcadm_rqst->rqst_cb_parms;

    if ( IIGCc_global->gcc_trace_level >= 4 )
	TRdisplay( "%4d     GCC ADM stopping server n", sess_cb->aid );

    end_session( sess_cb, OK );
    GCshut();
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
    GCC_SESS_CB		*sess_cb = (GCC_SESS_CB *)rslt_cb->rslt_cb_parms;
    QUEUE               *q;

    if ( IIGCc_global->gcc_trace_level >= 4 )
	TRdisplay( "%4d     GCC ADM complete cb: 0x%x\n",
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
**	    Adding block around case which declares local variables.
*/

static void 
adm_rqst_cb( PTR ptr )
{
    GCADM_RQST_PARMS	*gcadm_rqst = (GCADM_RQST_PARMS *)ptr;
    GCC_SESS_CB		*sess_cb = (GCC_SESS_CB *)gcadm_rqst->rqst_cb_parms;
    char		*token[ GCC_CMD_MAX_TOKEN ];
    ADM_ID		token_ids[ GCC_CMD_MAX_TOKEN ]; 
    ADM_COMMAND		*rqst_cmd;
    u_i2		token_count;
    u_i2		keyword_count = ARR_SIZE ( keywords );
    u_i2		command_count = ARR_SIZE ( commands );
    i4			sess_flag = 0;

    if ( IIGCc_global->gcc_trace_level >= 4 )
	TRdisplay( "%4d     GCC ADM request callback: 0x%x\n", 
		   sess_cb->aid, gcadm_rqst->sess_status );

    if (  gcadm_rqst->sess_status != OK )				  
    {
	end_session( sess_cb, gcadm_rqst->sess_status ); 

	/* if exit the last admin session try to test for shutdown */

	if ( ! IIGCc_global->gcc_conn_ct &&
	     ! IIGCc_global->gcc_admin_ct &&
		IIGCc_global->gcc_flags & GCC_QUIESCE )
	{
		GCshut();

   	}

	return;
    }

    sess_cb->state = GCCS_PROCESS;

    /*
    ** Prepare and process request text.
    */ 
    /* EOS request and trim trailing space */  
    gcadm_rqst->request[ gcadm_rqst->length ] = EOS; 
    STtrmwhite ( gcadm_rqst->request );

    if ( IIGCc_global->gcc_trace_level >= 4 )
	TRdisplay( "%4d     GCC ADM request: %s\n", 
		   sess_cb->aid, gcadm_rqst->request );

    token_count = gcu_words ( gcadm_rqst->request, NULL, token, ' ',
				GCC_CMD_MAX_TOKEN);

    gcadm_keyword ( token_count, token, 
		keyword_count, keywords, token_ids, GCC_TOKEN ); 

    if ( ! gcadm_command( token_count, token_ids, 
			  command_count, commands, &rqst_cmd  ) ) 
    {
        if ( IIGCc_global->gcc_trace_level >= 5 )
	    TRdisplay( "%4d     GCC ADM gcadm_cmd failed\n", -1 );  
	adm_rslt( sess_cb, gcadm_rqst->gcadm_scb, ADM_BAD_COMMAND_TXT );
        return;
    }

    if ( IIGCc_global->gcc_trace_level >= 5 )
	TRdisplay( "%4d     GCC ADM command: %d\n", 
		   sess_cb->aid, rqst_cmd->command);  

    switch ( rqst_cmd->command ) 
    {
    case GCC_CMD_HELP:  
    {
	i4 i, max_len;

	for( i = max_len = 0; i < ARR_SIZE( helpText ); i++ )
            max_len = build_rsltlst(&sess_cb->rsltlst_q, helpText[i], max_len);

        adm_rsltlst( sess_cb, gcadm_rqst->gcadm_scb, max_len );
        break;
    }

    case GCC_CMD_SHOW_SVR:
	rqst_show_server( sess_cb, gcadm_rqst->gcadm_scb );
	break;

    case GCC_CMD_SHOW_SVR_LSTN:
	adm_rslt( sess_cb, gcadm_rqst->gcadm_scb,
	     ( IIGCc_global->gcc_flags & GCC_CLOSED )? "CLOSED" : "OPEN" ); 
	break;

    case GCC_CMD_SHOW_SVR_SHUT:
	adm_rslt( sess_cb, gcadm_rqst->gcadm_scb,
                (IIGCc_global->gcc_flags & GCC_QUIESCE) ? "PENDING" :
                ((IIGCc_global->gcc_flags & GCC_STOP) ?  "STOPPING" : "OPEN"));

	break;

    case GCC_CMD_SET_SVR_OPEN:
	IIGCc_global->gcc_flags &= ~(GCC_QUIESCE | GCC_CLOSED | GCC_STOP);
	adm_rslt( sess_cb, gcadm_rqst->gcadm_scb, ADM_SVR_OPEN_TXT );
	break;

    case GCC_CMD_SET_SVR_CLSD:
	IIGCc_global->gcc_flags |= GCC_CLOSED;  
	adm_rslt( sess_cb, gcadm_rqst->gcadm_scb, ADM_SVR_CLSD_TXT );
	break;

    case GCC_CMD_SET_SVR_SHUT:
	IIGCc_global->gcc_flags |= GCC_QUIESCE | GCC_CLOSED; 
	adm_rslt( sess_cb, gcadm_rqst->gcadm_scb, ADM_SVR_SHUT_TXT );
	break;

    case GCC_CMD_STOP_SVR:
	IIGCc_global->gcc_flags |= GCC_STOP | GCC_CLOSED; 
	adm_stop( sess_cb, gcadm_rqst->gcadm_scb, ADM_SVR_STOP_TXT );
	break;

    case GCC_CMD_STOP_SVR_FORCE:
    case GCC_CMD_CRASH_SVR:
	PCexit( FAIL ); 
	break;

    case GCC_CMD_REGISTER_SVR:
    {
	GCADM_SCB * scb; 
	STATUS status; 

	scb = ( GCADM_SCB *)gcadm_rqst->gcadm_scb;

	MEfill( sizeof(scb->parms), 0, (PTR) &(scb->parms));

	scb->parms.gca_rg_parm.gca_srvr_class = GCA_COMSVR_CLASS;
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
	    gcu_erlog( 0, 0,
		   status, &scb->parms.gca_rg_parm.gca_os_status, 0, NULL ); 
	    adm_rslt( sess_cb, gcadm_rqst->gcadm_scb, ADM_REG_ERR_TXT );
	}
	else 
	    adm_rslt( sess_cb, gcadm_rqst->gcadm_scb, ADM_SVR_REG_TXT );
	break;
    }

    case GCC_CMD_REMOVE_SESSID: 
    {
	PTR sid;
	CVaxptr( token[1], &sid ); 
   	sess_flag = verify_sid( sid );
	switch ( sess_flag )  
	{
	case GCC_SESS_USER: 
        {
	    gcc_sess_abort( (GCC_CCB *)sid );
	    adm_rslt( sess_cb, gcadm_rqst->gcadm_scb, ADM_REM_TXT );
	    break;	
	}
	case GCC_SESS_SYS: 	
	    adm_rslt( sess_cb, gcadm_rqst->gcadm_scb, ADM_REM_SYSERR_TXT );
	    break;	
	case GCC_SESS_ADMIN: 	
	    adm_rslt( sess_cb, gcadm_rqst->gcadm_scb, ADM_REM_ADMERR_TXT );
	    break;	
	default:  
	    adm_rslt( sess_cb, gcadm_rqst->gcadm_scb, ADM_BAD_SID_TXT );
        break;
        }
    }

    case GCC_CMD_SHOW_ADM_SESS_FMT:
    case GCC_CMD_SHOW_ADM_SESS_STATS:
    case GCC_CMD_SHOW_ADM_SESS:
    case GCC_CMD_FRMT_ADM_SESS:
    {
        i4 max_len = rqst_show_adm( &sess_cb->rsltlst_q,
                                    NULL, rqst_cmd->command, 0 );
        if ( max_len )
            adm_rsltlst( sess_cb, gcadm_rqst->gcadm_scb, max_len );
        else
            adm_done( sess_cb, gcadm_rqst->gcadm_scb );
        break;
    }
    case GCC_CMD_SHOW_USER_SESS:
    case GCC_CMD_SHOW_USER_SESS_STATS:
    case GCC_CMD_SHOW_USER_SESS_FMT:
    case GCC_CMD_FRMT_USER_SESS:
    case GCC_CMD_SHOW_SYS_SESS:
    case GCC_CMD_SHOW_SYS_SESS_STATS:
    case GCC_CMD_SHOW_SYS_SESS_FMT:
    case GCC_CMD_FRMT_SYS_SESS:
    {
        i4 max_len = rqst_show_sessions( &sess_cb->rsltlst_q, NULL,
                                         rqst_cmd->command, 0 );
        if ( max_len )
            adm_rsltlst( sess_cb, gcadm_rqst->gcadm_scb, max_len );
        else
            adm_done( sess_cb, gcadm_rqst->gcadm_scb );
        break;
    }

    case GCC_CMD_SHOW_ALL_SESS:
    case GCC_CMD_SHOW_ALL_SESS_STATS:
    case GCC_CMD_SHOW_ALL_SESS_FMT:
    case GCC_CMD_FRMT_ALL_SESS:
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

    case GCC_CMD_FRMT_SESSID: 
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

        /*
        ** Return results if SID found.  Otherwise, bad SID error message.
        */
        if ( max_len )
            adm_rsltlst( sess_cb, gcadm_rqst->gcadm_scb, max_len );
        else
            adm_rslt( sess_cb, gcadm_rqst->gcadm_scb, ADM_BAD_SID_TXT );
        break;
    }
    case GCC_CMD_SET_TRACE_LOG_NONE:
    /* Turn off tracing */
    {
        CL_ERR_DESC	err_cl;
	STATUS		cl_stat;

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
    case GCC_CMD_SET_TRACE_LOG_FNAME:
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
    case GCC_CMD_SET_TRACE_LEVEL:
    {
	i4 trace_lvl;
        /* set gcc_trace_level */
        CVan( token[3], &trace_lvl );
        IIGCc_global->gcc_trace_level = trace_lvl;
        STprintf( sess_cb->msg_buff, "Tracing GCC at level %d\n", trace_lvl );
        adm_rslt( sess_cb, gcadm_rqst->gcadm_scb, sess_cb->msg_buff );
        break;
    }
    case GCC_CMD_SET_TRACE_ID_LEVEL:
    {
        /* set trace levels of various components( GCA, GCS )
        ** for the server.
        */
        i4 trace_lvl;

        CVan( token[3], &trace_lvl );
        rqst_set_trace( sess_cb, gcadm_rqst->gcadm_scb, token[2], trace_lvl );
        break;

    }
    default:  
	if ( IIGCc_global->gcc_trace_level >= 1 )
	    TRdisplay( "%4d     GCC invalid command: %d\n", 
			sess_cb->aid, rqst_cmd->command );
	adm_error( sess_cb, gcadm_rqst->gcadm_scb, 
			E_GC200C_ADM_INIT_ERR );
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
**	sess_cb		GCC ADM session control block.
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
adm_rslt( GCC_SESS_CB *sess_cb, PTR scb, char *msg_buff  )
{
    GCADM_RSLT_PARMS  	rslt_parms;  
    STATUS		status; 

    if ( IIGCc_global->gcc_trace_level >= 4 )
	TRdisplay( "%4d     GCC ADM return result: '%s'\n", 
		   sess_cb->aid, msg_buff );

    MEfill( sizeof( rslt_parms ), 0, (PTR)&rslt_parms );
    rslt_parms.gcadm_scb = (PTR)scb; 
    rslt_parms.rslt_cb = (GCADM_CMPL_FUNC)adm_rslt_cb;
    rslt_parms.rslt_cb_parms = (PTR)sess_cb; 
    rslt_parms.rslt_len = strlen( msg_buff );
    rslt_parms.rslt_text = msg_buff; 	

    if ( (status = gcadm_rqst_rslt( &rslt_parms )) != OK )
    {
	if ( IIGCc_global->gcc_trace_level >= 1 )
	    TRdisplay( "%4d     GCC ADM result error: 0x%x\n", 
		       sess_cb->aid, status );

	error_log ( E_GC200B_ADM_SESS_ERR ); 
	error_log ( status ); 
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
**	sess_cb		GCC ADM session control block.
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
adm_error( GCC_SESS_CB *sess_cb, PTR scb, STATUS error_code )
{
    GCADM_ERROR_PARMS	error_parms;
    STATUS		status; 
    char		*buf, *post;

    if ( IIGCc_global->gcc_trace_level >= 4 )
	TRdisplay( "%4d     GCC ADM return error: 0x%x\n", 
		   sess_cb->aid, error_code );

    buf = sess_cb->msg_buff;
    post = sess_cb->msg_buff + sizeof( sess_cb->msg_buff );

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
	if ( IIGCc_global->gcc_trace_level >= 1 )
	    TRdisplay( "%4d     GCC ADM gcadm_rqst_error failed: 0x%x\n", 
		       sess_cb->aid, status );

	error_log ( E_GC200B_ADM_SESS_ERR ); 
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
**	sess_cb		GCC ADM Session control block.
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
adm_done( GCC_SESS_CB *sess_cb, PTR scb )
{
    GCADM_DONE_PARMS	done_parms;  
    STATUS		status; 

    /*
    ** Complete the request and wait for next.
    */
    sess_cb->state = GCCS_WAIT;

    MEfill( sizeof( done_parms ), 0, (PTR)&done_parms );
    done_parms.gcadm_scb = (PTR)scb; 
    done_parms.rqst_cb = (GCADM_RQST_FUNC)adm_rqst_cb;
    done_parms.rqst_cb_parms = (PTR)sess_cb;
    done_parms.rqst_status = OK; 

    if ( (status = gcadm_rqst_done( &done_parms )) != OK )
    {
	if ( IIGCc_global->gcc_trace_level >= 1 )
	    TRdisplay( "%4d     GCC ADM gcadm_rqst_done() failed: 0x%x\n", 
		       sess_cb->aid, status );

	error_log ( E_GC200B_ADM_SESS_ERR ); 
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
**      21-Sep-06 (usha)
**          Created.
**	21-Jul-09 (gordy)
**	    Free user ID resources.
*/

static STATUS
end_session( GCC_SESS_CB *sess_cb, STATUS status )
{
    if ( status != OK  &&  status != FAIL )
    {
	if ( IIGCc_global->gcc_trace_level >= 1 )
            TRdisplay( "%4d     GCC ADM gcadm_session() failed: 0x%x\n",
                       sess_cb->aid, status );

	error_log ( E_GC200B_ADM_SESS_ERR ); 
        error_log( status );
    }

    if ( sess_cb->user_id != user_default )  MEfree( sess_cb->user_id );
    MEfree( (PTR)QUremove( &sess_cb->sess_q ) );
    IIGCc_global->gcc_admin_ct--; 
    return;
}


/*
** Name: gcc_adm_session
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
**	16-Nov-2003 (wansh01) 
**	    Created.
**	21-Jul-09 (gordy)
**	    Dynamically allocate space for username.
*/

STATUS  
gcc_adm_session( i4 aid, PTR gca_cb, char *user )
{
    GCC_SESS_CB		*sess_cb;
    GCADM_SESS_PARMS    gcadm_sess;
    STATUS		status; 

    if ( IIGCc_global->gcc_trace_level >= 4 )
	TRdisplay( "%4d   GCC ADM New session: %s\n", aid, user );

     if( !srv_info_init )
     {
	GCC_PIB * pib, *next_pib;
        int i = 0;
        int j = 0;

	if ( IIGCc_global->gcc_trace_level >= 4 )
            TRdisplay( "%4d     GCC ADM : Initializing server info \n");

        gcc_svr_info.gcc_pnp_info_len = 0;

        for( next_pib = (GCC_PIB *)IIGCc_global->gcc_pib_q.q_next;
              (PTR)next_pib  != (PTR)(&IIGCc_global->gcc_pib_q);)
        {
            pib = (GCC_PIB * ) next_pib;
            next_pib = (GCC_PIB * )pib->q.q_next;

           if( pib->flags & GCC_REG_MASTER )
           {
               STlcopy( pib->pid, gcc_svr_info.gcc_reg_info[j].proto,
                        sizeof(gcc_svr_info.gcc_reg_info[j].proto) - 1 );
               STlcopy( pib->host, gcc_svr_info.gcc_reg_info[i].host,
                        sizeof(gcc_svr_info.gcc_reg_info[j].host) - 1 );
               STlcopy( pib->addr, gcc_svr_info.gcc_reg_info[j].port,
                        sizeof(gcc_svr_info.gcc_reg_info[j].port) - 1 );
               STlcopy( pib->addr, gcc_svr_info.gcc_reg_info[j].addr,
                        sizeof(gcc_svr_info.gcc_reg_info[j].port) - 1 );
               ++j;
          }
          else
          {
            STlcopy( pib->pid, gcc_svr_info.gcc_pnp_info[i].proto,
                        sizeof(gcc_svr_info.gcc_pnp_info[i].proto) - 1 );
            STlcopy( pib->host, gcc_svr_info.gcc_pnp_info[i].host,
                        sizeof(gcc_svr_info.gcc_pnp_info[i].host) - 1 );
            STlcopy( pib->port, gcc_svr_info.gcc_pnp_info[i].port,
                        sizeof(gcc_svr_info.gcc_pnp_info[i].port) - 1 );
            STlcopy( pib->addr, gcc_svr_info.gcc_pnp_info[i].addr,
                        sizeof(gcc_svr_info.gcc_pnp_info[i].addr) - 1 );
            ++i;
         }

        }

        gcc_svr_info.gcc_pnp_info_len = i;
        gcc_svr_info.gcc_reg_info_len = j;

        switch( IIGCc_global->gcc_reg_mode )
        {
            case GCC_REG_NONE:
               STlcopy( GCC_REG_NONE_STR, gcc_svr_info.registry_type,
                        sizeof(gcc_svr_info.registry_type) - 1 );
               break;
            case GCC_REG_SLAVE:
               STlcopy( GCC_REG_SLAVE_STR, gcc_svr_info.registry_type,
                        sizeof(gcc_svr_info.registry_type) - 1 );
               break;
            case GCC_REG_PEER:
               STlcopy( GCC_REG_PEER_STR, gcc_svr_info.registry_type,
                        sizeof(gcc_svr_info.registry_type) - 1 );
               break;
            case GCC_REG_MASTER:
               STlcopy( GCC_REG_MASTER_STR, gcc_svr_info.registry_type,
                        sizeof(gcc_svr_info.registry_type) - 1 );
               break;
            default:
               break;
        }

        srv_info_init = TRUE;

    }

    sess_cb = (GCC_SESS_CB *)MEreqmem(0, sizeof(GCC_SESS_CB), TRUE, &status);

    if ( sess_cb == NULL )
    {
	if ( IIGCc_global->gcc_trace_level >= 1 )
	    TRdisplay( "%4d     GCC ADM mem alloc failed: 0x%x\n", 
		       aid, status );
	error_log( E_GC2008_OUT_OF_MEMORY  );
	return( E_GC2008_OUT_OF_MEMORY );
    }

    sess_cb->aid = aid;
    sess_cb->user_id = STalloc( user );
    if ( ! sess_cb->user_id )  sess_cb->user_id = user_default;
    QUinit( &sess_cb->rsltlst_q );
    sess_cb->state = GCCS_INIT;

    QUinsert( &sess_cb->sess_q, &gcc_sess_q );
    IIGCc_global->gcc_admin_ct++; 

    MEfill( sizeof( gcadm_sess ), 0, (PTR)&gcadm_sess );
    gcadm_sess.assoc_id = aid; 
    gcadm_sess.gca_cb   = gca_cb;
    gcadm_sess.rqst_cb  = (GCADM_RQST_FUNC )adm_rqst_cb;
    gcadm_sess.rqst_cb_parms = (PTR)sess_cb;

    if ( (status = gcadm_session( &gcadm_sess )) != OK )
    {
	if ( IIGCc_global->gcc_trace_level >= 1 )
	    TRdisplay( "%4d     GCC ADM gcadm_session() failed: 0x%x\n", 
		       aid, status );

	end_session( sess_cb, status );
	status = E_GC200B_ADM_SESS_ERR;
    }

    return( status );

}


/*
** Name: gcc_adm_int  
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
gcc_adm_init ()
{
    GCADM_INIT_PARMS gcadm_init_parms;
    STATUS  status; 
    i4	    i;

    if ( IIGCc_global->gcc_trace_level >= 4 )
	TRdisplay( "%4d   GCC ADM Initialize\n", -1 );

    QUinit( &gcc_sess_q );

    MEfill( sizeof( gcadm_init_parms ), 0, (PTR)&gcadm_init_parms );
    gcadm_init_parms.alloc_rtn    = NULL;
    gcadm_init_parms.dealloc_rtn  = NULL;
    gcadm_init_parms.errlog_rtn   = error_log;
    gcadm_init_parms.msg_buff_len = GCC_L_BUFF_DFLT;

    if ( (status =  gcadm_init( &gcadm_init_parms )) != OK )
    {
	if ( IIGCc_global->gcc_trace_level >= 1 )
	    TRdisplay( "%4d     GCC ADM gcadm_init() failed: 0x%x\n", 
		       -1, status );

	error_log ( E_GC200C_ADM_INIT_ERR ); 
	error_log ( status ); 
	status = E_GC200C_ADM_INIT_ERR;
    }	

    return ( status ); 
}


/*
** Name: gcc_adm_term
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
gcc_adm_term ()
{
    STATUS  status; 
    
    if ( IIGCc_global->gcc_trace_level >= 4 )
	TRdisplay( "%4d   GCC ADM Terminate\n", -1 );

    if ( (status = gcadm_term()) != OK )
    {
	if ( IIGCc_global->gcc_trace_level >= 1 )
	    TRdisplay( "%4d     GCC ADM gcadm_term() failed: 0x%x\n", 
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
**	sess_cb		GCC ADM Session control block.
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
adm_stop( GCC_SESS_CB *sess_cb, PTR scb, char * msg_buff )
{
    GCADM_RSLT_PARMS	rslt_parms;  
    STATUS		status; 

    if ( IIGCc_global->gcc_trace_level >= 4 )
	TRdisplay( "%4d     GCC ADM stopping server: '%s'\n", 
		   sess_cb->aid, msg_buff);

    MEfill( sizeof( rslt_parms ), 0, (PTR)&rslt_parms );
    rslt_parms.gcadm_scb = (PTR)scb; 
    rslt_parms.rslt_cb = (GCADM_CMPL_FUNC)adm_stop_cb;
    rslt_parms.rslt_cb_parms = (PTR)sess_cb; 
    rslt_parms.rslt_len = strlen( msg_buff );
    rslt_parms.rslt_text = msg_buff; 	

    if ( (status = gcadm_rqst_rslt( &rslt_parms )) != OK )
    {
	if ( IIGCc_global->gcc_trace_level >= 1 )
	    TRdisplay( "%4d     GCC ADM gcadm_rqst_rslt() failed: 0x%x\n", 
		       sess_cb->aid, status );
	error_log ( E_GC200B_ADM_SESS_ERR ); 
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
**	sess_cb	GCC ADM Session control block.
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
adm_rsltlst( GCC_SESS_CB *sess_cb, PTR scb, i4 lst_maxlen )
{
    GCADM_RSLTLST_PARMS	rsltlst_parms; 
    STATUS		status; 

    if ( IIGCc_global->gcc_trace_level >= 4 )
	TRdisplay( "%4d     GCC ADM return list: %d\n", 
		   sess_cb->aid, lst_maxlen );

    rsltlst_parms.gcadm_scb = (PTR)scb; 
    rsltlst_parms.rslt_cb = (GCADM_CMPL_FUNC)adm_rslt_cb;
    rsltlst_parms.rslt_cb_parms = (PTR)sess_cb;
    rsltlst_parms.rsltlst_max_len = lst_maxlen; 
    rsltlst_parms.rsltlst_q  = &sess_cb->rsltlst_q; 
	 

    if ( (status = gcadm_rqst_rsltlst( &rsltlst_parms )) != OK )
    {
	if ( IIGCc_global->gcc_trace_level >= 1 )
	    TRdisplay( "%4d     GCC ADM gcadm_rqst_rsltlst() failed: 0x%x\n", 
		       sess_cb->aid, status );

	error_log ( E_GC200B_ADM_SESS_ERR ); 
	error_log ( status ); 
    }

    return; 
}


/*
** Name: rqst_show_adm
**
** Description:
**	Display ADMIN session info.  Adds entry to result
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
**	None.
**
** Returns:
**      i4		Max message length.	
**
** History:
**	08-Jul-04 (wansh01) 
**	    Created.
**	 6-Aug-04 (gordy)
**	    Enhanced session info display.
**	23-Sep-06 (rajus01)
**	    Re-worked on the routine based ong Gordy's changes for GCN.
*/

static i4 
rqst_show_adm( QUEUE *rsltlst, PTR sid, u_i2 cmd, i4 max_len )
{
    QUEUE	*q;
  
     for( q = gcc_sess_q.q_next; q != &gcc_sess_q; q = q->q_next )
     {
        GCC_SESS_CB *sess_cb = (GCC_SESS_CB *)q;

        if ( sid  &&  sid != (PTR)sess_cb )  continue;

        /*
        ** Format and return session info heading.
        */
        switch( cmd )
        {
        case GCC_CMD_FRMT_ADM_SESS:
        case GCC_CMD_FRMT_ALL_SESS:
        case GCC_CMD_FRMT_SESSID:
            STprintf( sess_cb->msg_buff,
                      ">>>>>Session %p:%d<<<<<", sess_cb, sess_cb->aid );
            break;

        case GCC_CMD_SHOW_ADM_SESS_STATS:
        case GCC_CMD_SHOW_ALL_SESS_STATS:
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
        case GCC_CMD_SHOW_ADM_SESS_FMT:
        case GCC_CMD_SHOW_ALL_SESS_FMT:
        case GCC_CMD_FRMT_ADM_SESS:
        case GCC_CMD_FRMT_ALL_SESS:
        case GCC_CMD_FRMT_SESSID:
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
**	None.
**
** Returns:
**	i4	Max message length.	
**
** History:
**	08-Jul-04 (wansh01) 
**	    Created.
**	 6-Aug-04 (gordy)
**	    Enhanced session info display.
**	21-Jul-09 (gordy)
**	    Release dynamic resources.
*/

static i4 
rqst_show_sessions( QUEUE *rsltlst, PTR sid, u_i2 cmd, i4 max_len )
{
    GCC_SESS_INFO	*info, *iptr = NULL;
    char		msg_buff[ER_MAX_LEN ];
    i4			count, cnt_flag, info_flag;

    /*
    ** First determine number of active sessions.
    */
    
    switch( cmd )
    {
    case GCC_CMD_SHOW_USER_SESS:
    case GCC_CMD_SHOW_USER_SESS_STATS:
    case GCC_CMD_SHOW_USER_SESS_FMT:
    case GCC_CMD_FRMT_USER_SESS:
        cnt_flag = GCC_USER_COUNT;
        info_flag = GCC_USER_INFO;
        break;

    case GCC_CMD_SHOW_SYS_SESS:
    case GCC_CMD_SHOW_SYS_SESS_STATS:
    case GCC_CMD_SHOW_SYS_SESS_FMT:
    case GCC_CMD_FRMT_SYS_SESS:
        cnt_flag = GCC_SYS_COUNT;
        info_flag = GCC_SYS_INFO;
        break;

    case GCC_CMD_SHOW_ALL_SESS:
    case GCC_CMD_SHOW_ALL_SESS_STATS:
    case GCC_CMD_SHOW_ALL_SESS_FMT:
    case GCC_CMD_FRMT_ALL_SESS:
    case GCC_CMD_FRMT_SESSID:
        cnt_flag = GCC_ALL_COUNT;
        info_flag = GCC_ALL_INFO;
        break;
    }

    if ( ! (count = gcc_sess_info( NULL, cnt_flag )) )
    {
        STprintf( msg_buff, "No user sessions" );
        max_len = build_rsltlst( rsltlst, msg_buff, max_len );
        return( max_len );
    }

    TRdisplay("rqst_show_sessions: Call MEreqmem\n" );
    iptr = (GCC_SESS_INFO *)MEreqmem( 0, count * sizeof(GCC_SESS_INFO),
                                      TRUE, NULL);

    TRdisplay("rqst_show_sessions: Call gcc_sess_info again\n" );

    /*
    ** Now load and display active session info.
    */

    count = iptr ? gcc_sess_info( (PTR)iptr, info_flag ) : 0;

    for( info = iptr; count > 0; count--, info++ )
    {
        TRdisplay("rqst_show_sessions: Looping %d\n", count );
        if ( sid  &&  sid != info->sess_id )  continue;

        /*
        ** Format and return session info heading.
        */
        switch( cmd )
        {
        case GCC_CMD_FRMT_USER_SESS:
        case GCC_CMD_FRMT_SYS_SESS:
        case GCC_CMD_FRMT_ALL_SESS:
        case GCC_CMD_FRMT_SESSID:
            if ( info->assoc_id < 0 )
                STprintf( msg_buff, ">>>>>Session %p<<<<<", info->sess_id );
            else
                STprintf( msg_buff, ">>>>>Session %p:%d<<<<<",
                          info->sess_id, info->assoc_id );
            break;

        case GCC_CMD_SHOW_SYS_SESS_STATS:
        case GCC_CMD_SHOW_ALL_SESS_STATS:
        case GCC_CMD_SHOW_USER_SESS_STATS:
	if( info->is_system )
	    STprintf( msg_buff,
	    "Session %p  (%-24s) Type: SYSTEM  Server Class: %s GCA Addr: %s", info->sess_id, info->user_id, info->srv_class, info->gca_addr ); 
        else
            STprintf( msg_buff,"Session %p:%d (%-15s) Type: USER  state: %s  MSGIN: %d MSGOUT: %d DTIN: %dKb DTOUT: %dKb", info->sess_id, info->assoc_id, info->user_id, info->state, info->msgs_in, info->msgs_out, info->data_in, info->data_out);
	    break;
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
        case GCC_CMD_SHOW_USER_SESS_FMT:
        case GCC_CMD_FRMT_USER_SESS:
        case GCC_CMD_SHOW_SYS_SESS_FMT:
        case GCC_CMD_FRMT_SYS_SESS:
        case GCC_CMD_SHOW_ALL_SESS_FMT:
        case GCC_CMD_FRMT_ALL_SESS:
        case GCC_CMD_FRMT_SESSID:
	 if( info->is_system ) 
         {
             STprintf( msg_buff,
                       "    Server Class: %s   Server Addr: %s",
                        info->srv_class, info->gca_addr );
             max_len = build_rsltlst( rsltlst, msg_buff, max_len );
         }
         else
         {
            if ( info->target_db  &&  *info->target_db )
            {
	        STprintf( msg_buff,"    Database: %s ", info->target_db ); 
                max_len = build_rsltlst( rsltlst, msg_buff, max_len );
            }

	    if(info->trg_addr.n_sel[0] )
            {
	        STprintf( msg_buff,
		    "    Target Addr: Protocol: %s,  node: %s,  port: %s",
	    	    info->trg_addr.n_sel, info->trg_addr.node_id,
	    	    info->trg_addr.port_id);
                max_len = build_rsltlst( rsltlst, msg_buff, max_len );
	    }
	    if(info->lcl_addr.n_sel[0] )
            {
                STprintf( msg_buff,
		"    Local Addr: Protocol: %s,  node: %s,  port: %s",
	        info->lcl_addr.n_sel, info->lcl_addr.node_id,
	        info->lcl_addr.port_id);
                max_len = build_rsltlst(rsltlst, msg_buff, max_len );
            }

	    if(info->rmt_addr.n_sel[0] )
            {
                STprintf( msg_buff,
		"    Remote Addr: Protocol: %s,  node: %s,  port: %s",
	        info->rmt_addr.n_sel, info->rmt_addr.node_id,
	        info->rmt_addr.port_id);
                max_len = build_rsltlst( rsltlst, msg_buff, max_len );
	    }

         }
        break;
        }

        if ( sid )  break;
    }

    if ( iptr )  MEfree( (PTR)iptr );
    return( max_len );
}

/*
** Name: build_rsltlst
**
** Description:
**	Add entry to result list queue.
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
**      i4		Updated maximum messge length.	
**
** History:
**	 08-Jul-04 (wansh01) 
*/

static i4    
build_rsltlst( QUEUE *rsltlst_q, char *msg_buff, i4 max_len  )
{
    ADM_RSLT	*rslt;
    i4 		msg_len = strlen( msg_buff );

    /*
    ** Allocate result list entry with space for message text.
    */
    if ( ! (rslt = (ADM_RSLT *)MEreqmem( 0, sizeof( ADM_RSLT ) + msg_len,
                                         0, NULL )) )
    {
        if ( IIGCc_global->gcc_trace_level >= 1 )
            TRdisplay( "build_rsltlst entry: memory alloc failed\n" );
    }
    else
    {
        if ( IIGCc_global->gcc_trace_level >= 4 )
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
** Name: rqst_show_server
**
** Description:
**	show server information 
**
** Input:
**      sess_cb         GCC ADM session control block.
**      scb             GCADM session control block.
** Output:
**	None.
**
** Returns:
**	void 
**
** History:
**	 08-Jul-04 (wansh01) 
**	22-Apr-10 (rajus01) SIR 123621
**	    Display process ID.
*/

static void 
rqst_show_server( GCC_SESS_CB *sess_cb, PTR scb )
{
    int i=0;
    i4	max_len = 0;
    PID proc_id; 

    PCpid( &proc_id );

    STprintf( sess_cb->msg_buff,
		"     Server:     %s " ,IIGCc_global->gcc_lcl_id );
    max_len = build_rsltlst( &sess_cb->rsltlst_q,
                             sess_cb->msg_buff, max_len );

    STprintf( sess_cb->msg_buff,"     Class:      %s " , GCA_COMSVR_CLASS );
    max_len = build_rsltlst( &sess_cb->rsltlst_q,
                             sess_cb->msg_buff, max_len );

    STprintf( sess_cb->msg_buff,"     Object:     %s " , "*" );
    max_len = build_rsltlst( &sess_cb->rsltlst_q,
                             sess_cb->msg_buff, max_len );

    STprintf( sess_cb->msg_buff,"     Pid:        %d " , proc_id );
    max_len = build_rsltlst( &sess_cb->rsltlst_q,
                             sess_cb->msg_buff, max_len );

    STprintf( sess_cb->msg_buff," " ); 
    max_len = build_rsltlst( &sess_cb->rsltlst_q,
                             sess_cb->msg_buff, max_len );

    STprintf( sess_cb->msg_buff," Connected Sessions ( includes system and admin sessions ) " );
    max_len = build_rsltlst( &sess_cb->rsltlst_q,
                             sess_cb->msg_buff, max_len );

    STprintf( sess_cb->msg_buff,"     Current:    %d", IIGCc_global->gcc_admin_ct + gcc_sess_info(NULL, GCC_SYS_COUNT));
    max_len = build_rsltlst( &sess_cb->rsltlst_q,
                             sess_cb->msg_buff, max_len );

    STprintf( sess_cb->msg_buff," " ); 
    max_len = build_rsltlst( &sess_cb->rsltlst_q,
                             sess_cb->msg_buff, max_len );

    STprintf( sess_cb->msg_buff," Active Sessions  ( includes user sessions only ) " );
    max_len = build_rsltlst( &sess_cb->rsltlst_q,
                             sess_cb->msg_buff, max_len );

    STprintf( sess_cb->msg_buff,"     Inbound sessions: "); 
    max_len = build_rsltlst( &sess_cb->rsltlst_q,
                             sess_cb->msg_buff, max_len );

    STprintf( sess_cb->msg_buff,"       Current:  %d ", IIGCc_global->gcc_ib_conn_ct); 
    max_len = build_rsltlst( &sess_cb->rsltlst_q,
                             sess_cb->msg_buff, max_len );

    STprintf( sess_cb->msg_buff,"       Limit:    %d ", IIGCc_global->gcc_ib_max); 
    max_len = build_rsltlst( &sess_cb->rsltlst_q,
                             sess_cb->msg_buff, max_len );

    STprintf( sess_cb->msg_buff,"     Outbound sessions: "); 
    max_len = build_rsltlst( &sess_cb->rsltlst_q,
                             sess_cb->msg_buff, max_len );

    STprintf( sess_cb->msg_buff,"       Current:  %d ", IIGCc_global->gcc_ob_conn_ct); 
    max_len = build_rsltlst( &sess_cb->rsltlst_q,
                             sess_cb->msg_buff, max_len );

    STprintf( sess_cb->msg_buff,"       Limit:    %d ", IIGCc_global->gcc_ob_max); 
    max_len = build_rsltlst( &sess_cb->rsltlst_q,
                             sess_cb->msg_buff, max_len );

    STprintf( sess_cb->msg_buff," " ); 
    max_len = build_rsltlst( &sess_cb->rsltlst_q,
                             sess_cb->msg_buff, max_len );

    STprintf( sess_cb->msg_buff," Protocols "); 
    max_len = build_rsltlst( &sess_cb->rsltlst_q,
                             sess_cb->msg_buff, max_len );

    for(i=0; i< gcc_svr_info.gcc_pnp_info_len; i++ )
    {
	STprintf( sess_cb->msg_buff,"     %s  host:%s addr:%s port:%s ", 
		gcc_svr_info.gcc_pnp_info[i].proto, 
                gcc_svr_info.gcc_pnp_info[i].host, 
                gcc_svr_info.gcc_pnp_info[i].addr, 
                gcc_svr_info.gcc_pnp_info[i].port ); 
        max_len = build_rsltlst( &sess_cb->rsltlst_q,
                             sess_cb->msg_buff, max_len );

    }

    STprintf( sess_cb->msg_buff," Registry Protocols "); 
    max_len = build_rsltlst( &sess_cb->rsltlst_q,
                             sess_cb->msg_buff, max_len );
    if( gcc_svr_info.gcc_reg_info_len > 0 ) 
    {

        for(i=0; i< gcc_svr_info.gcc_reg_info_len; i++ )
        {
	    STprintf( sess_cb->msg_buff,"     %s  host:%s addr:%s port:%s ", 
		gcc_svr_info.gcc_reg_info[i].proto, 
                gcc_svr_info.gcc_reg_info[i].host, 
                gcc_svr_info.gcc_reg_info[i].addr, 
                gcc_svr_info.gcc_reg_info[i].port ); 
            max_len = build_rsltlst( &sess_cb->rsltlst_q,
                             sess_cb->msg_buff, max_len );
        }
    }
    else
    {
	STprintf( sess_cb->msg_buff,"     Not Enabled"); 
        max_len = build_rsltlst( &sess_cb->rsltlst_q,
                             sess_cb->msg_buff, max_len );
    }

    adm_rsltlst( sess_cb, scb, max_len );
    return;
}

/*
** Name: gcc_sess_info
**
** Description:
**	calculate user/system sessions 
**
** Input:
**	info	GCC_SESS_INFO
**	i4 	flag to indicate user/system/all sessions 
**
** Output:
**	None.
**
** Returns:
**	count
**
** History:
**	08-Jul-04 (wansh01) 
**	    Created.
**	21-Jul-09 (gordy)
**	    Reference existing data rather than copying.
*/

i4  
gcc_sess_info( PTR info, i4 flag ) 
{
    GCC_SESS_INFO	*sess = (GCC_SESS_INFO *) info;
    i4			cei;
    GCC_CCB		*ccb;
    i4			count = 0;
    bool		selected = FALSE, is_system = FALSE;

    if ( IIGCc_global->gcc_trace_level >= 4 )
	TRdisplay (" gcc_sess_info entry flag=%d\n", flag);

    for( cei = 0; cei < IIGCc_global->gcc_max_ccb; cei++ )
    {
	if( ( ccb = IIGCc_global->gcc_tbl_ccb[ cei ] )  !=  NULL )
	{
	    switch( flag )
            {
                case GCC_SYS_COUNT:
		    if ((!( ccb ->ccb_hdr.flags & CCB_IB_CONN ) && 
		         !( ccb->ccb_hdr.flags & CCB_OB_CONN ) ) )
		    {
			count++;
			is_system = TRUE;
		    }
		    break;
		case GCC_SYS_INFO:
		    if ((!( ccb ->ccb_hdr.flags & CCB_IB_CONN ) && 
		         !( ccb->ccb_hdr.flags & CCB_OB_CONN ) ) )
		    {
			selected = TRUE;
			count++;
			is_system = TRUE;
		    }
		    break;
                case GCC_USER_COUNT:
	    	    if ( ( ccb ->ccb_hdr.flags & CCB_IB_CONN ) || 
		 	 ( ccb->ccb_hdr.flags & CCB_OB_CONN ) )
	    	        count++;
		    break;
                case GCC_USER_INFO:
	    	    if ( ( ccb ->ccb_hdr.flags & CCB_IB_CONN ) || 
		 	 ( ccb->ccb_hdr.flags & CCB_OB_CONN ) )
		    {
			selected = TRUE;
	    	        count++;
		    }
		    break;
                case GCC_ALL_COUNT:
		    if ((!( ccb ->ccb_hdr.flags & CCB_IB_CONN ) && 
		         !( ccb->ccb_hdr.flags & CCB_OB_CONN ) ) )
			is_system = TRUE;
		    count++;
		    break;
                case GCC_ALL_INFO:
		    if ((!( ccb ->ccb_hdr.flags & CCB_IB_CONN ) && 
		         !( ccb->ccb_hdr.flags & CCB_OB_CONN ) ) )
			is_system = TRUE;
		    selected = TRUE;
		    count++;
		    break;
	    }
        
            if( selected )
            {
	        sess->sess_id = (PTR) ccb; 
	        sess->assoc_id = ccb->ccb_aae.assoc_id;
		sess->is_system = is_system;
		sess->user_id = ccb->ccb_hdr.username;
                if( sess->is_system )
                {
		    TRdisplay("Found system sessions in gcc_sess_info\n");
                    rqst_sys_stats( ccb, sess->srv_class, sess->gca_addr );
                    if( ccb->ccb_hdr.trg_addr.n_sel[0] )
			sess->state = tl_states[ ccb->ccb_tce.t_state ];
                    else
			sess->state = al_states[ ccb->ccb_aae.a_state ];
	        }
		else
	        {
		    TRdisplay("Found user sessions in gcc_sess_info\n");
		    sess->target_db = ccb->ccb_hdr.target;

	            if( ccb->ccb_hdr.flags & CCB_IB_CONN )
			sess->state = "INBOUND";
	            else  if( ccb->ccb_hdr.flags & CCB_OB_CONN )
			sess->state = "OUTBOUND";
		    else
		        sess->state = "<unknown>";

    		    if ( ccb->ccb_hdr.trg_addr.n_sel[0] )  	
    		    {
		        STcopy(ccb->ccb_hdr.trg_addr.n_sel,
				 sess->trg_addr.n_sel);
		        STcopy(ccb->ccb_hdr.trg_addr.node_id, 
		       		 sess->trg_addr.node_id);
		        STcopy(ccb->ccb_hdr.trg_addr.port_id, 
				 sess->trg_addr.port_id);
		    }
		
    		    if ( ccb->ccb_hdr.lcl_addr.n_sel[0] )  	
    		    {
		        STcopy(ccb->ccb_hdr.lcl_addr.n_sel, 
					sess->lcl_addr.n_sel);
		        STcopy(ccb->ccb_hdr.lcl_addr.node_id, 
					sess->lcl_addr.node_id);
		        STcopy(ccb->ccb_hdr.lcl_addr.port_id, 
					sess->lcl_addr.port_id);
		    }

    		    if ( ccb->ccb_hdr.rmt_addr.n_sel[0] )  	
    		    {
		        STcopy(ccb->ccb_hdr.rmt_addr.n_sel, 
					sess->rmt_addr.n_sel);
		        STcopy(ccb->ccb_hdr.rmt_addr.node_id, 
					sess->rmt_addr.node_id);
		        STcopy(ccb->ccb_hdr.lcl_addr.port_id, 
					sess->rmt_addr.port_id);
		    }

		    /* Stats info */
                    sess->msgs_in = ccb->ccb_tce.msgs_in;
                    sess->msgs_out = ccb->ccb_tce.msgs_out;
                    sess->data_in = ccb->ccb_tce.data_in;
                    sess->data_out = ccb->ccb_tce.data_out;

               }

	       sess++;
	    }
        }
     
        is_system = FALSE;
	selected = FALSE;
     }

    if ( IIGCc_global->gcc_trace_level >= 4 )
	TRdisplay (" gcc_sess_info count =%d\n", count);

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
    GCC_CCB     *ccb;
    i4  	sid_sess = 0; 
    i4  	cei; 


	if ( IIGCc_global->gcc_trace_level >= 4 )
	    TRdisplay (" verify_sid entry sid =%p\n", sid );

	sid_sess = 0; 

	for( cei = 0; cei < IIGCc_global->gcc_max_ccb; cei++ )
	{
	    if( ( ccb= IIGCc_global->gcc_tbl_ccb[ cei ]) ==  (GCC_CCB *) sid )
	    {
		if ( ccb->ccb_hdr.flags & CCB_IB_CONN )
		    sid_sess = GCC_SESS_USER;
		else 
		if ( ccb->ccb_hdr.flags & CCB_OB_CONN )
		    sid_sess = GCC_SESS_USER;
		else 
		    sid_sess = GCC_SESS_SYS;
		break; 
	    }
	}

	if ( sid_sess == 0 && sid != NULL )
	    sid_sess = GCC_SESS_ADMIN;

	if ( IIGCc_global->gcc_trace_level >= 4 )
	    TRdisplay (" verify_sid  sid_sess =%x\n", sid_sess );
	
	return ( sid_sess );
}

/*
** Name: rqst_set_trace
**
** Description:
**       Set trace variables for GCC
**
** Input:
**      sess            ptr to GCC_SESS_CB
**      scb             GCADM session control block.
**      trace_id        Trace ID.
**      level           Trace level.
**
**
** Output:
**      None.
**
** Returns:
**      void 
**
** History:
**       04-Aug-06 (rajus01)
**	    Created.
*/

static void
rqst_set_trace( GCC_SESS_CB *sess_cb, PTR scb, char *trace_id, i4 level )
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
** Name: rqst_sys_stats
**
** Description:
**      Display system sessions stats.
**
** Input:
**      ccb             ptr to GCC_CCB *
**      srv_class       char *
**      gca_addr        char *
**
** Output:
*       GCA server class, GCA server addr.
**
** Returns:
**      None
**
** History:
**       04-Aug-06 (rajus01)
**          Created.
*/

static void
rqst_sys_stats( GCC_CCB * ccb, char *srv_class, char *gca_addr )
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

 STprintf(aid_buf, "%d", ccb->ccb_aae.assoc_id);
        if( MOget(MO_READ, GCA_MIB_CLIENT, aid_buf, &id_buf_len, id_buf, &perms ) == OK )
        {
            CVan( id_buf, &gca_cb_id );
            STprintf(id_buf, "%d", gca_cb_id);
            MOget(MO_READ, GCA_MIB_CLIENT_SRV_CLASS, id_buf, &srv_class_buf_len, srv_class_buf, &perms );
            MOget(MO_READ, GCA_MIB_CLIENT_LISTEN_ADDRESS, id_buf, &srv_lsaddr_buf_len, srv_lsaddr_buf, &perms );
            STcopy(srv_class_buf, srv_class);
            STcopy(srv_lsaddr_buf, gca_addr);
        }
    }

    return;
}

