/*
**  Copyright (C) 2000 Ingres Corporation
*/

/*
**  Name: rmcmdgca.h
**
**  Description:
**	This header file contains all necessary defines to implement
**	the GCA layer of RMCMD.
**
**  History:
**	01-feb-2000 (somsa01)
**	    Created.
*/

/*
** GCA_LCB: Listen control block.
*/

typedef struct
{

    i4			state;		/* Current state */
    i4			sp;		/* State stack counter */
    i4			ss[5];		/* State stack */
    u_i4		flags;

#define LCB_SHUTDOWN	0x0001

    STATUS		stat;		/* Non-GCA status result */
    STATUS		*statusp;	/* Current result status */
    GCA_PARMLIST	parms;		/* GCA request parameters */
    i4			assoc_id;
    i4			protocol;

} GCA_LCB;

/*
** Right now, this server does not support any GCA connections,
** so only a few listen control blocks are required to support
** Name Server bedchecks and GCM operations.
*/

#define  LCB_MAX	2


/*
** State actions.
*/

#define LSA_LABEL	0	/* label */
#define LSA_INIT	1	/* initialize */
#define LSA_GOTO	2	/* goto unconditionally */
#define LSA_GOSUB	3	/* call subroutine */
#define LSA_RETURN	4	/* return from subroutine */
#define LSA_EXIT	5	/* terminate thread */
#define LSA_IF_RESUME	6	/* if INCOMPLETE */
#define LSA_IF_TIMEOUT	7	/* if TIMEOUT */
#define LSA_IF_ERROR	8	/* if status not OK */
#define LSA_IF_SHUT	9	/* if SHUTDOWN requested */
#define LSA_SET_SHUT	10	/* SHUTDOWN server */
#define LSA_CLEAR_ERR	11	/* set status to OK */
#define LSA_LOG		12	/* log an error */
#define LSA_CHECK	13	/* Background checks */
#define LSA_LISTEN	14	/* post a listen */
#define LSA_LS_RESUME	15	/* resume a listen */
#define LSA_REPOST	16	/* repost listen */
#define LSA_LS_DONE	17	/* complete a listen */
#define LSA_NEGOTIATE	18	/* validate client request */
#define LSA_REJECT	19	/* reject client request */
#define LSA_RQRESP	20	/* respond to a request */
#define LSA_DISASSOC	21	/* disassociate */
#define LSA_DA_RESUME	22	/* resume a disassoc */

static  char    *lsn_sa_names[] =
{
    "LSA_LABEL",	"LSA_INIT",		"LSA_GOTO",
    "LSA_GOSUB",	"LSA_RETURN",		"LSA_EXIT",
    "LSA_IF_RESUME",	"LSA_IF_TIMEOUT",	"LSA_IF_ERROR",
    "LSA_IF_SHUT",	"LSA_SET_SHUT",
    "LSA_CLEAR_ERR",	"LSA_LOG",		"LSA_CHECK",
    "LSA_LISTEN",	"LSA_LS_RESUME",	"LSA_REPOST",
    "LSA_LS_DONE",	"LSA_NEGOTIATE",	"LSA_REJECT",
    "LSA_RQRESP",	"LSA_DISASSOC",		"LSA_DA_RESUME",
};

/*
** State labels
*/

#define LBL_NONE	0
#define LBL_LS_CHK	1
#define LBL_LS_RESUME	2
#define LBL_SHUTDOWN	3
#define LBL_RESPOND	4
#define LBL_ERROR	5
#define LBL_DONE	6
#define LBL_DA_CHK	7
#define LBL_DA_RESUME	8
#define LBL_EXIT	9

#define LBL_MAX		10


/*
**  Name: lsn_states
**
**  Description:
**	Each state in GCN has one action and one branch state identified
**	by a label.  If a state action does not branch, execution falls
**	through to the subsequent state.
**
**  History:
**	01-feb-2000 (somsa01)
**	    Created.
*/

static struct
{

    i4	action;
    i4	label;

} lsn_states[] =
{
    /*
    ** Initialize
    */

	LSA_CLEAR_ERR,	LBL_NONE,
	LSA_INIT,	LBL_EXIT,		/* If listening */

    /*
    ** Post and process GCA_LISTEN
    */

	LSA_LISTEN,	LBL_NONE,
	LSA_REPOST,	LBL_NONE,

    LSA_LABEL,	LBL_LS_CHK,

	LSA_IF_RESUME,	LBL_LS_RESUME,		/* if INCOMPLETE */
	LSA_LS_DONE,	LBL_NONE,
	LSA_IF_TIMEOUT,	LBL_DONE,
	LSA_IF_ERROR,	LBL_ERROR,
	LSA_NEGOTIATE,	LBL_NONE,
	LSA_IF_SHUT,	LBL_SHUTDOWN,		/* if SHUTDOWN requested */
	LSA_REJECT,	LBL_NONE,
	LSA_GOTO,	LBL_RESPOND,

    /*
    ** Resume a GCA_LISTEN request
    */

    LSA_LABEL,	LBL_LS_RESUME,

	LSA_LS_RESUME,	LBL_NONE,
	LSA_GOTO,	LBL_LS_CHK,

    /*
    ** Shutdown requested.
    */

    LSA_LABEL,	LBL_SHUTDOWN,

	LSA_SET_SHUT,	LBL_NONE,
	LSA_GOTO,	LBL_RESPOND,

    /*
    ** Respond to client request.
    */

    LSA_LABEL,	LBL_RESPOND,

	LSA_RQRESP,	LBL_NONE,
	LSA_IF_ERROR,	LBL_ERROR,
	LSA_GOTO,	LBL_DONE,

    /*
    ** Log error, disconnect and exit.
    */

    LSA_LABEL,	LBL_ERROR,

	LSA_LOG,	LBL_NONE,
	LSA_CLEAR_ERR,	LBL_NONE,

    /*
    ** Disconnect and exit.
    */

    LSA_LABEL,	LBL_DONE,

	LSA_DISASSOC,	LBL_NONE,

    LSA_LABEL,	LBL_DA_CHK,

	LSA_IF_RESUME,	LBL_DA_RESUME,		/* if INCOMPLETE */
	LSA_LOG,	LBL_NONE,
	LSA_GOTO,	LBL_EXIT,

    /*
    ** Resume a GCA_LISTEN request
    */

    LSA_LABEL,	LBL_DA_RESUME,

	LSA_DA_RESUME,	LBL_NONE,
	LSA_GOTO,	LBL_DA_CHK,

    /*
    ** Terminate thread.
    */

    LSA_LABEL,	LBL_EXIT,

	LSA_CHECK,	LBL_NONE,
	LSA_CLEAR_ERR,	LBL_NONE,
	LSA_EXIT,	LBL_NONE,

};

int NumStates = (sizeof(lsn_states)/sizeof(lsn_states[0]));
