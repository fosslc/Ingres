/*
** Copyright (c) 2004 Ingres Corporation
** All rights reserved.
*/

#define DEBUG TRUE

#ifdef VMS 
#include <descrip.h>
#include <jpidef.h>
#include <prvdef.h>
#include <ssdef.h>
#endif /* VMS */

/* Global flags
**	is_db_open - this is used to really tell if there is a db to close.
**		This can occur when looking at GARP locks and then choosing
**		to look at regular INGRES locks.  Two routines open_db and
**		close_db manipulate and check this flag.
**	flag_db - set if user specified to check a certain database only.
**	flag_file - set if user wants output directed to a file.
**	frs_timeout - determines the refresh interval for various screens.
**	flag_locktype - set if user wants to check for a certain locktype.
**	flag_all_locks - set if user wants to examine all INGRES locktypes.
**	flag_nl - set if user wants to print GRANTED NL locks.
**		form with the GARP field is displayed.
**	flag_table - set if user wants to examine locks on a particular table.
**		Must specify a database.
**	flag_cluster - set if running on a vaxcluster
**	flag_nonprot_lklists - set if user wants to see nonactive lock lists 
**      flag_inactive_tx - set if user wants to see inactive transactions
**	flag_debug_log - set if user wants to log debugging to logfile
**			 for now, this debugging is NOT DOCUMENTED!
**
** History:
**      8 May 1992  jpk   modify VMS include syntax to conform to standards
**     13 Dec 1992  jpk   support DB_MAXNAME 24 -> 32 project
**     08 Dec 1994  (liibi01) Cross integration from 6.4, add GCN #defines.
**                        Also add support for LK_EVCONNECT.
**     20 Apr 1995 nick  add support for LK_CKP_TXN (bug 67888)
**     21-aug/1995 (albany)  Bug #70787
**	    Changed globaldef of Vmslkstat from char * to char to match its
**	    globaldef in utility.qsc.
**      1 Nov 1995  nick  Add LK_AUDIT and LK_ROW
**                        Remove unused variables
**                        Add SXF definitions.
**                        Define the string we use to print unknown keys here
**                        rather than use it as a literal in several files.
**	21-feb-1996 (toumi01)
**	    For axp_osf expand session id from 9 to 16.
**      22-nov-1996 (dilma04) Added LN_PH_PAGE and LN_VALUE for row level 
**                            loking project.
**      23-mar-1999 (hweho01)
**          Defined HEXID_SIZE to 16 for ris_u64 (AIX 64-bit platform).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      08-sep-2000 (hanje04)
**          Defined HEXID_SIZE to 16 for axp_lnx (Alpha Linux)
**	13-oct-2001 (somsa01)
**          Defined HEXID_SIZE to 16 for LP64.
**	05-Mar-2002 (jenjo02)
**	    Added LK_SEQUENCE.
*/

/*	Global variables, flags	*/
    GLOBALREF   i4      lock_id;
    GLOBALREF   i4	dbname_id;
    GLOBALREF   char	tbl_name[FE_MAXNAME + 1];
    GLOBALREF   PTR	ptr_tablename;
    GLOBALREF   i4	ing_locktype;
    GLOBALREF   char	data_base_name[FE_MAXNAME + 1];
    GLOBALREF   PTR	ptr_dbname;
    GLOBALREF   bool	is_db_open;
    GLOBALREF   bool	flag_db;
    GLOBALREF   i4   	frs_timeout;
    GLOBALREF   bool  	flag_locktype;
    GLOBALREF   bool  	flag_all_locks;
    GLOBALREF   bool	flag_nl;
    GLOBALREF   bool	flag_table;
    GLOBALREF   bool	flag_cluster;	/* are we on a VAXcluster? */
    GLOBALREF   bool	flag_nonprot_lklists;
    GLOBALREF   bool	flag_inactive_tx;
    GLOBALREF   bool	flag_debug_log;
    GLOBALREF   bool	flag_standalone;
    GLOBALREF	bool	flag_client;	/* client machine diff. from server? */
    GLOBALREF	bool	flag_internal_sess;	/* display internal sessions? */


GLOBALREF char *Lock_type[];
GLOBALREF u_i4	Lock_type_size;

GLOBALREF char *Lkmode[];
GLOBALREF u_i4	Lkmode_size;

GLOBALREF char *Lkstate[];
GLOBALREF u_i4	Lkstate_size;

GLOBALREF char *Invalid[];
GLOBALREF u_i4	Invalid_size;

#ifdef VMS
GLOBALREF  char	Vmslkstat[];
#endif

GLOBALREF char Lklststcl[];

GLOBALREF char Lklstst[];

/*
** Lock names that user specifies on command line or on options form
** This allows the user to specify the lock_type on the command line
** and not the lock type number.
*/
#define		LN_DATABASE	"database"
#define		LN_TABLE	"table"
#define		LN_PAGE		"page"
#define		LN_EXTEND_FILE	"extend"
#define		LN_BM_PAGE	"svpage"
#define         LN_PH_PAGE      "phpage"
#define		LN_CREATE_TABLE	"createtable"
#define		LN_OWNER_ID	"name"
#define		LN_CONFIG	"config"
#define		LN_DB_TEMP_ID	"dbtblid"
#define		LN_SV_DATABASE  "svdatabase"
#define		LN_SV_TABLE	"svtable"
#define		LN_SS_EVENT	"event"
#define		LN_TBL_CONTROL	"control"
#define		LN_JOURNAL	"journal"
#define		LN_OPEN_DB	"opendb"
#define		LN_CKP_DB	"checkpoint"
#define		LN_CKP_CLUSTER	"ckpcluster"
#define         LN_BM_LOCK	"buffermgr"
#define         LN_BM_DATABASE	"bufmgrdb"
#define         LN_BM_TABLE	"bufmgrtable"
#define		LN_CONTROL	"syscontrol"
#define         LN_EVENT        "eventcontrol"
#define         LN_CKP_TXN      "ckptxn"
#define         LN_AUDIT        "audit"
#define         LN_ROW          "row"
#define		LN_VALUE 	"value"
#define		LN_SEQUENCE 	"sequence"

/*
** define lock classes - specific INGRES locktypes are mapped into
** these classes for the sorting routine l_ressort.  Note that the
** values assigned matter because these are used as part of the sorting
** process - this causes database locks to appear first, table locks second
** etc...
*/
#define		DATABASE_LOCK	1
#define		TABLE_LOCK	2
#define		PAGE_LOCK	3
#define 	ROW_LOCK        4
#define         VALUE_LOCK      5
#define		OTHER_LOCK	6

/* define sizes of things not otherwise defined, e.g. process id */
#if defined(axp_osf) || defined(ris_u64) || defined(axp_lnx) || defined(LP64)
#define		HEXID_SIZE	16	/* needs to be longer for axp_osf */
#else /* axp_osf */
#define		HEXID_SIZE	8	/* four bytes print as 8 hex digits */
#endif /* axp_osf */

/* GCN size information */
# define    GCN_OBJ_MAX_LEN     256
# define    GCN_TYP_MAX_LEN     32
# define    GCN_VAL_MAX_LEN     64

/*
** really defined in lkdef.h, private to server, so update here
** if value in lkdef.h changes
*/
#define		LLB_NONPROTECT	0x002


/* Globally known macros */
#define POP_MSG(M) dispmsg(M, MSG_POPUP, (u_i4)0)

/* to make refresh_status calls more readable */
#define	VISIBLE		TRUE
#define INVISIBLE	FALSE

/* generic max size for cs_state, cs_mask, activity */
#define	MMAXSTR		61

/* old ingres size was 24, still used by format convents in iimonitor etc. */
#define	OLDINGSIZE	24
#define INGSIZE		DB_MAXNAME

/*
** These values are currently private to the server ...
**
** The following lock type definitions are to be used as the first element
** of a lock key of type LK_AUDIT.
*/
# define	SXAS_LOCK		1
# define	SXAP_LOCK		2
# define	SXLC_LOCK		3	/* SXL cache lock */

/*
** These are sub-lock id types used for the SXAS state lock
*/
# define	SXAS_STATE_LOCK	        1	/* Main state lock */
# define	SXAS_SAVE_LOCK		2	/* Save lock */

/*
** These are sub-lock id types for the SXAP physical lock
*/
# define	SXAP_SHMCTL	1
# define	SXAP_ACCESS	2
# define	SXAP_QUEUE	3

/*
** Used to display unmappable key values
*/
# define	KEY_UNKNOWN	ERx("?? =")
