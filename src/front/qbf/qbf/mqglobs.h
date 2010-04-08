/*
**	MQGLOBS.h  - This file contains the GLOBAL VARIABLES
**		     used in MQBF.
**
**      History:
**		10-aug-1987 (danielt) removed User, UserName, and Dba globals
**		22-apr-88 (kenl)
**			Added extern of allow_updates.
**		23-nov-88 (kenl)
**			Added extern of allow_appends.  This came from the
**			IBM group (ham) in order to allow appends when there
**			are no keys.  Previously, when no keys for a table
**			existed you could not update or append.
**
**		03-mar-89 (kenl) bug #4876
**			Removed MAXFLNM from extern of mqflnm[].
**		05-dec-89 (kenl)
**			Added externs for mq_editjoin, mq_joindef, and 
**			mq_lastsaved.
**		10/11/90 (emerson)
**			Replaced global variable allow_appends and allow_updates
**			by IIQFnkaNoKeyForAppends and IIQFnkuNoKeyForUpdates
**			(with opposite sense) because the names are misleading:
**			we now have other reasons for disallowing appends
**			(certain joins on logical keys), and we might have
**			other reasons for disallowing appends or updates
**			in the future.  Keeping separate flags allows us
**			to give specific error messages.  (bug 8593).
**		10/11/90 (emerson)
**			Added global variables IIQFsmmSystemUserLkeyMM,
**			IIQFsmdSystemUserLkeyMD, and IIQFsddSystemUserLkeyDD
**			for logical key support (bug 8593).
**		18-dec-92 (sylviap)
**			Added externs for mq_nogo
**		27-jun-95 (teresak)
**			Added undocumented support for II_QBF_DISABLE_VIEW_CHECK
**			to allow append, update, and deletes of single table 
**			views (SIR 37839)
**		15-oct-97(angusm)
**			change allow_views to bool: safer.
**	Copyright (c) 2004 Ingres Corporation
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/



/* Global variables */
GLOBALREF	char    *IIQFrfRoleFlag;/* role id for -R */
GLOBALREF	char    *IIQFpwPasswd;  /* password for -P */
GLOBALREF	char    *IIQFgidGroupID;/* group id for -G */
GLOBALREF	char	*mquname;	/* name for -uusername flag */
GLOBALREF	i4	tblfield;	/* flag for table field usage */
GLOBALREF	i4	qdef_type;	/* master/detail or 1-1 qdef? */
GLOBALREF	char	mq_db[];	/* name of database */
GLOBALREF	i4	mq_func;	/* command line function */
GLOBALREF	i4	mq_terse;	/* terse error output */
GLOBALREF	i4	mq_prompt;	/* prompt user for missing arguments */
GLOBALREF	i4	mq_input;	/* frame driver mode */
GLOBALREF	i4	mq_forms;	/* get form from forms catalogues */
GLOBALREF	bool	mq_debug;
GLOBALREF	char	mq_frame[];	/* allows calls on relation, form */
GLOBALREF	char	mq_tbl[];
GLOBALREF	char	mqflnm[];	/* name for temp file for writing QDEF*/
GLOBALREF	i4	mqfile;
GLOBALREF	i4	mq_mwhereend;	/* ptr to end of master where clause */
GLOBALREF	i4	mq_dwhereend;	/* ptr to end of detail where clause */
GLOBALREF	bool	mq_uflag;	/* was -uusername used ? */
GLOBALREF	bool	onetbl;		/* flag for single table qdef */
GLOBALREF	bool	mq_tflag;	/* flag for table field format
					   used with single table qdef*/
GLOBALREF	bool	mq_qflag;	/* -qqdef specified on command line */
GLOBALREF	bool	mq_editjoin;	/* +J flag specified - JoinDef edit */
GLOBALREF	bool	mq_nogo;	/* +N flag specified - JoinDef edit 
					** but do not allow 'GO' - modify/add
					** data */
GLOBALREF	char	*mqmrec;	/* buffer for master rec for retrieves*/
GLOBALREF	char	*mqdrec;	/* buffer for detail rec for retrieves*/
GLOBALREF 	char	*mqumrec;	/* buffer for master rec for updates */
GLOBALREF	char	*mqudrec;	/* buffer for detail rec for updates */
GLOBALREF	bool	mq_name;	/* name entered on command line */
GLOBALREF	bool	mq_going;	/* go done on object */
GLOBALREF	bool	mq_noload;	/* don't load names into catalogs */


# ifdef		DEBUG
	extern	
FILE	*mqoutf;
# endif
/*
** added for iiqbf call
**
** 7-16-82 (jrc)
*/
GLOBALREF	bool	mq_equel;	/* called with iiqbf interface ? */
GLOBALREF	i4	mq_env[2];	/* starting environment */
GLOBALREF	i4	mq_status;	/* status for equel call */

/*
** added for qbf call in subproc
** 9/8/82 (jrc)
*/
GLOBALREF char	*mq_xpipe;		/* the transmit pipe */

/*
** added to qbf for testing
** 10/4/82 (jrc)
*/
GLOBALREF	char	*mq_itest;	/* testing input file */
GLOBALREF	char	mq_otest[];	/* testing output file */


/*
** Globals for parameterized getform statements
** to get _RECORD and _STATE info and store tids
*/
GLOBALREF	i4	mq_record;
GLOBALREF	i4	mqrstate;
GLOBALREF	char	mq_tids[];

GLOBALREF	i4	any_views;
/* 
** NOTE: This variable is for interpreting the undocumented environment 
** variable II_QBF_DISABLE_VIEW_CHECK which will allow inserts, deletes, 
** and updates for single table views within QBF.
*/
GLOBALREF 	bool	allow_views;
GLOBALREF	i4	IIQFnkuNoKeyForUpdates;
GLOBALREF	i4	IIQFnkaNoKeyForAppends;
GLOBALREF	i4	IIQFsmmSystemUserLkeyMM;
GLOBALREF	i4	IIQFsmdSystemUserLkeyMD;
GLOBALREF	i4	IIQFsddSystemUserLkeyDD;
GLOBALREF	i4	any_cats;
GLOBALREF	char	qbf_name[];	/* qbfname - used with -f flag */
GLOBALREF	char	mq_joindef[];	/* JoinDef - used for JoinDef dup */
GLOBALREF	char	mq_lastsaved[];	/* name of last JoinDef saved */
GLOBALREF	bool	mq_both;	/* table or qdef and form on cmd line*/
GLOBALREF	bool	mq_cfrm;	/* called with compiled form (-C flag)*/

GLOBALREF	i2	mqrowstat;	/* status of row in table field (hidden	
					** column to differentiate NEW and 
					** CHANGED)
					*/

GLOBALREF	i4	mqnummasters;	/* number of master tables in qdef */
GLOBALREF	i4	mqnumdetails;	/* number of detail tables in qdef */
GLOBALREF	char	mq_qown[];	/* owner of current qdef */

GLOBALREF	bool	mq_lookup;	/* should QBF use the QBFNAME, QDEFNAME,
					** TABLE order to look up the name 
					** the user has specified?
					*/

GLOBALREF	bool	mqztesting;
GLOBALREF	char	*mq_ztest;	/* added for switcher and abf and VMS
					** testing. mq_ztest points to string
					** containing "infilename,outfilename"
					*/
GLOBALREF	FILE	*mq_tfile;	/* temp file for updates */

GLOBALREF	bool	mq_qdframes;	/* are compiled qdef frames initialized */

/* added by Azad for interrupt cleanup to keep track of current update file */
GLOBALREF	i4	Upd_exists;	/* update file exists */
GLOBALREF	QDESC	*QG_mqqry;	/* current qg query */

GLOBALREF	bool	Mq_intransaction;	/* currently in a transaction */

/** added for modularization (kira) **/
GLOBALREF	bool 	mq_deffrm;	/* indicates whether there is a
					   default form for qexec */
GLOBALREF	i4	Qcode;		/* return code from qexec */
