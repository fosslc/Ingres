/*
**  Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	qrhelp.h -	Header file for TM help.
**
** Description:
**	This file defines those structures, macros and externs used
**	by the 'help' requests in the terminal monitors.
**
** History:
**	04-aug-88 (bruceb)
**		Created.
**	10-aug-88 (bruceb)
**		Added IIQRgno; renamed IIQRgnn.  Changed IIQRlist structure.
**	13-oct-88 (bruceb)
**		Renamed from qrlist.h
**	18-may-89 (teresal)
**		Added help rule.
**	08-nov-89 (teresal)
**		Add help security_alarm.
**	27-dec-89 (teresal)
**		Add help synonym, help comment table, and help comment column.
**	22-mar-91 (kathryn)
**		Added help permit on procedure, help permit on event.
**	10-jul-1991 (kathryn) Comment change only.(event to dbevent)
**	9-jul-1992 (rogerl)
**		Add _PERM_VIEW, _PERM_IDX; add 'type' to IIQRlist; used only
**		for displaying 'synonym' type in 'help'
**	10-dec-1992 (rogerl)
**		Add defs for bit pos defs used in qrdiprintf()
**	21-jul-1993 (rogerl)
**		Add INSUF_PRIV.
**	10-dec-93 (robf)
**              Add _ALARM_DB, _ALARM_INST for database and installation
**	        security alarms
**	13-Jul-95 (ramra01)
**	 	Tune the Zap node in loop_end to leave the non existant
**		objects for proper error msg display in help (b66258)
**      21-jan-1999 (hanch04)
**          replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	26-dec-2001 (abbjo03)
**	    Move QRSTAT_STRUCT here from qrtabhlp.qsc.
**	16-apr-02 (inkdo01)
**	    Add HELP_SEQUENCE for sequence support.
**	20-Dec-2004 (kodse01)
**	    BUG 113657
**	    Added HELP_PERM_SEQ for sequence support.
**	26-Aug-2009 (kschendel) b121804
**	    Bool prototypes to keep gcc 4.3 happy.
**/

# define	HELP_OBJECT	1000	/* 'help x[, y]' command */
# define	HELP_TABLE	1001	/* 'help table x[, y]' command */
# define	HELP_VIEW	1002	/* 'help view x[, y]' command */
# define	HELP_INDEX	1003	/* 'help index x[, y]' command */
# define	HELP_INTEGRITY	1004	/* 'help integrity x[, y]' command */
# define	HELP_PERMIT	1005	/* 'help permit x[, y]' command */
# define	HELP_PROCEDURE	1006	/* 'help procedure x[, y]' command */
# define	HELP_LINK	1007	/* 'help link x[, y]' command */
# define	HELP_RULE	1008	/* 'help rule x[, y]' command */
# define	HELP_SECURITY_ALARM 1009 /* 'help security_alarm x[,y]' */
# define	HELP_SYNONYM 	1010 	/* 'help synonym x[, y]' command */
# define	HELP_COMMENT_TBL 1011 	/* 'help comment table x[, y]' */
# define	HELP_COMMENT_COL 1012 	/* 'help comment column z x[, y]' */
# define	HELP_PERM_EVENT  1013   /* 'help permit on dbevent  x[, y]' */
# define	HELP_PERM_PROC	 1014   /* 'help permit on procedure x[, y]' */
# define	HELP_PERM_VIEW	 1015   /* 'help permit on view x[, y]' */
# define	HELP_PERM_IDX	 1016   /* 'help permit on index x[, y]' */
					/* assume HELP_PERM is ok for tbl case*/
# define	HELP_CONSTRAINT	 1017   /* 'help constraint x[, y]' */
# define	HELP_DEFAULT	 1018   /* 'help default x[, y]' */
# define	HELP_ALARM_DB	 1019	/* 'help security_alarm on database x*/
# define	HELP_ALARM_INST	 1020	/* 'help security_alarm on current 
					    installation' command */
# define	HELP_SEQUENCE	 1021	/* 'help sequence x[, y]' command */
# define	HELP_PERM_SEQ	 1022	/* 'help permit on sequence x[, y]' command */

# define	HELP_MAX_VALUE	 1022	/* Maximum legal help value */

typedef struct _IIQRlist
{
	i4			exist;	/* 0 Node added thru loop proc */
					/* 1 Node added thru loop end  */
	char			*name;
	char			*owner;
	char			*type;
	struct _IIQRlist	*next;
} IIQRlist;


FUNC_EXTERN STATUS	IIQRmnl_MakeNameList();
FUNC_EXTERN VOID	IIQRznl_ZapNameList();
FUNC_EXTERN IIQRlist	*IIQRgnn_GetNextNode();
FUNC_EXTERN VOID	IIQRgno_GetNameOwner();
FUNC_EXTERN VOID	IIQRznd_ZapNode();
FUNC_EXTERN IIQRlist	*IIQRlnp_LastNodePtr();
FUNC_EXTERN IIQRlist	*IIQRfnp_FirstNodePtr();
FUNC_EXTERN VOID	IIQRnfn_NewFirstNode();
FUNC_EXTERN VOID	IIQRrnl_ResetNameList();
FUNC_EXTERN void	qraddstr(void *, char *);
FUNC_EXTERN bool	qrtokcompare(char *, char *);
FUNC_EXTERN bool	qrtokend(char *, char *);

/*
** If the owner field of an IIQRlist node is '*', than the name field
** contains a pattern match string that failed to find anything.
**
** If the owner field of an IIQRlist node is '+', than the name field
** contains a pattern match string that was too long to look for
** (more than FE_MAXNAME characters.)
**
** If the owner field of an IIQRlist node is '#', then the REAL user
** probably doesn't have permissions to view alarms on the object
*/
# define	NONE_FOUND	'*'
# define	TOO_LONG	'+'
# define	INSUF_PRIV	'#'

/*
** Possible state of which catalog to use to look for indices.
** Will always use iitables on INGRES, but not sure where to look
** when running against gateways.
*/
# define	NOT_YET_DETERMINED	0
# define	USE_IITABLES		1
# define	USE_IIINDEXES		2

/*
** Bit positions for flags corresponding to arguments caller wants
** treated as possibly delimited id
*/
# define 	QRARG1	1
# define 	QRARG2	2
# define 	QRARG3	4
# define 	QRARG4	8
