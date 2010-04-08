/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: DM2D.H - Physical database operation definitions.
**
** Description:
**      This file defines the routines and constants needed to perform
**	physical operations on databases.
**
** History:
**      27-nov-1985 (derek)
**          Created new for Jupiter.
**	 6-feb-1989 (rogerk)
**	    Added DM2D_BMSINGLE flag for shared buffer manager support.
**	20-aug-1989 (ralph)
**	    Added DM2D_CVCFG for config file conversion
**	10-jan-1990 (rogerk)
**	    Added DM2D_OFFLINE_CSP option to open_db.  This indicates that
**	    the database is being opened by the CSP during startup recovery.
**	    When the csp opens a database, it will pass in either the
**	    DM2D_CSP or DM2D_OFFLINE_CSP flag.
**	28-jan-1991 (rogerk)
**	    Added DM2D_IGNORE_CACHE_PROTOCOL flag for open_db.  This directs
**	    it to bypass the cache/database protocols for fast commit.  This
**	    is used to allow AUDITDB (or other read-only utilities) to access
**	    a database while it is being used by fast commit servers.
**	25-feb-1991 (rogerk)
**	    Added DM2D_NOLOGGING flag for open_db.  This allows callers to
**	    open databases in READ_ONLY mode without requiring any log
**	    records to be written - so that utilities may connect to
**	    databases while the logfile is full.  This was added as part
**	    of the Archiver Stability Project.
**	22-mar-1991 (bryanp)
**	    B36630: Added FUNC_EXTERN for dm2d_check_db_backup_status().
**	7-July-1992 (rmuth)
**	    Prototyping DMF.
**	14-dec-92 (jrb)
**	    Change proto for dm2d_extend_db.
**	18-jan-1993 (rogerk)
**	    Reduced Logging Project: Remove DM2D_OFFLINE_CSP, add DM2D_RECOVER.
**	26-apr-1993 (bryanp)
**	    6.5 Cluster support:
**		Remove DM2D_CSP flag. The handling of locking during CSP
**		    recovery is now controlled by the recovery code, and so this
**		    flag is no longer useful. When the CSP calls the recovery
**		    code, the recovery code now asserts the DM2D_RCP flag when
**		    opening the database.
**		Add a DM2D_CANEXIST flag, if the dcb already exists then add_db
**		    just returns a pointer to it.
**	23-aug-1993 (rogerk)
**	    Renamed DM2D_RCP to DM2D_ONLINE_RCP as it is set only when doing
**	    online recovery processing.
**	29-sep-93 (jrb)
**	    Added DM2D_ADMIN_DB to indicate when a db is being opened for 
**	    administrative purposes only.
**	10-oct-93 (jrb)
**	    Added proto for dm2d_alter_db and added DM2D_ALTER_INFO struct.
**	22-nov-93 (jrb)
**	    Removed DM2D_ADMIN_DB; no longer needed.
**	23-may-1994 (andys)
**	    Add prototype for dm2d_check_db_backup_status	[bug 60702]
**	20-jun-1994 (jnash)
**	    Add DM2D_SYNC_CLOSE dm2d_open_db() access mode.  Thus far 
**	    used only by rollforwarddb, causes underlying files to be
**	    opened without disk synchronization.  Files are sync'd
**	    during table close.  Requires X access.  Recovery
**	    must be considered case-by-case.
**	24-apr-1995 (cohmi01)
**	    Add prototype dm2d_get_dcb()
**      12-feb-1996 (ICL phil.p)
**          Introduced DM2D_DMCM to support a database being opened with the
**          Distributed Multi_Cache Management (DMCM) protocol.
** 	08-apr-1997 (wonst02)
** 	    Pass back the access mode from the .cnf file.
**	16-feb-1999 (nanpr01)
**	    Support update mode locks.
**      27-May-1999 (hanal04)
**          Added DM2D_CNF_LOCKED to indicate the caller has the CNF file
**          locked. 
**          Also added new parameter to dm2d_add_db() prototype. lock_list
**          should contain the lock list id if DM2D_CNF_LOCKED is set. b97083.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	02-Apr-2001 (jenjo02)
**	    Added raw_start to dm2d_extend_db() prototype.
**	11-May-2001 (jenjo02)
**	    Deleted unneeded loc_raw_blocks from DM2D_LOC_ENTRY.
**	    Added raw_pct to dm2d_extend_db() prototype.
**	17-Jan-2004 (schka24)
**	    Prototype release-tcb routine extracted from close DB.
**	14-Jan-2005 (jenjo02)
**	    Add "release_flag" to dm2d_release_user_tcb() prototype.
**	13-May-2008 (kibro01) b120369
**	    Add DM2D_ADMIN_CMD
**	18-Nov-2008 (jonj)
**	    SIR 120874: Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code.
**       8-Jun-2009 (hanal04) Code Sprint SIR 122168 Ticket 387
**          Added DM2D_MUST_LOG to flag DBs that this DBMS must log all
**          activity. SET NOLOGGING is blocked.
**	25-Aug-2009 (kschendel) 121804
**	    Define dm2d-row-lock for gcc 4.3.
**/

/*
**  Defines of other constants.
*/

/*
**      Flag constants to dm2d_add_db and dm2d_open_db:
**	    Some of these are used by only add, some only by open and some
**	    by both.
*/

#define		DM2D_JOURNAL        0x000001L	/* Database journaled */
#define		DM2D_NOJOURNAL	    0x000002L	/* Not journaled */
#define		DM2D_SINGLE	    0x000004L	/* Open in single-server mode */
#define		DM2D_MULTIPLE	    0x000008L	/* Other servers may open db */
#define		DM2D_FASTCOMMIT	    0x000010L	/* Fast Commit in use */
#define		DM2D_BMSINGLE	    0x000020L	/* Open in single-cache mode */
#define		DM2D_NLK	    0x000040L	/* Don't lock that database */
#define		DM2D_JLK	    0x000080L	/* Just get db lock, don't
						** actually open the database */
#define		DM2D_NOWAIT	    0x000100L	/* Return if cant get db lock */
#define		DM2D_NLG	    0x000200L	/* Open db without logging an
						** opendb or informing the
						** logging system */
#define		DM2D_RFP	    0x000400L	/* Open by rollforwarddb */
#define		DM2D_ONLINE_RCP	    0x000800L	/* Open by RCP */
#define         DM2D_DMCM           0x001000L   /* DB open with DMCM */
#define		DM2D_RECOVER	    0x002000L	/* Open for recovery work */
#define		DM2D_NLK_SESS	    0x004000L	/* Special 2PC reconnect flag:
						** db open is not locked nor
						** logged */
#define		DM2D_CVCFG	    0x008000L	/* OK to convert config file 
						** to new release format. */
#define		DM2D_NOCK	    0x010000L	/* Dont check database valid
						** status in the config file -
						** allows an inconsistent db
						** to be opened. */
#define		DM2D_FORCE	    0x020000L	/* force config file status 
						** to valid if inconsistent */
#define		DM2D_IGNORE_CACHE_PROTOCOL  0x040000L 
						/* Bypass database/cache
						** locking protocols. */
#define		DM2D_NOLOGGING	    0x080000L	/* Database is being opened
						** in read-only mode and
						** we don't want to log
						** anything.  This should
						** only be used by utilities
						** which don't require any
						** database updates. */
#define		DM2D_CANEXIST	    0x00100000L	/* If DB has already been added
						** then add_db returns a pointer
						** to the existing dcb. If not
						** set then this condition is
						** an error. */
#define		DM2D_SYNC_CLOSE	    0x00200000L	/* Don't do synch writes on
						** database updates,
						** instead sync during
						** dm2d_close_db(). */
#define         DM2D_CNF_LOCKED     0x00400000L /* The caller already has the
                                                ** CNF file locked. */
#define         DM2D_ADMIN_CMD      0x00800000L /* Admin-type command like
						** verifydb. */
#define         DM2D_MUST_LOG       0x01000000L /* Must log all activity
                                                ** for this DB in this DBMS
                                                */

/*
**  The following constants depend on values in the logical interface and
**  in the actual control blocks.  If any of the these are changed they all
**  have to be changed, or code has to be inserted to map from one set of
**  constants to another set of constants.
*/

/*
**      Access mode constants.
*/

#define			DM2D_A_READ	1L
#define			DM2D_A_WRITE	2L

/*
**	Database type constants.
*/

#define			DM2D_TPRIVATE	    1L
#define			DM2D_TPUBLIC	    2L
#define			DM2D_TDISTRIBUTED   3L

/*
**	Lock mode constants.
*/

#define			DM2D_X		LK_X
#define			DM2D_U		LK_U
#define			DM2D_SIX	LK_SIX
#define			DM2D_S		LK_S
#define			DM2D_IX		LK_IX
#define			DM2D_IS		LK_IS
#define			DM2D_N		LK_N


/*}
** Name: DM2D_LOC_ENTRY - Describes a location list entry.
**
** Description:
**      This structure defines the information needed to define a location
**	that will be used to locate data by DMF.  The data that can be located
**	in a location includes: system tables, user tables, log file, journal
**	file.  This structure is the same as the logical interface, if either
**	changes something has to be resolved.
**
** History:
**     16-jan-1986 (derek)
**          Created new for Jupiter.
**	10-mar-1999 (nanpr01)
**	    Added raw location information.
**	11-May-2001 (jenjo02)
**	    Deleted unneeded loc_raw_blocks from DM2D_LOC_ENTRY.
*/
typedef struct _DM2D_LOC_ENTRY
{
    i4         	    loc_flags;          /* Not Used. */
    DB_LOC_NAME	    loc_name;		/* Name of the location. */
    i4	    	    loc_l_extent;	/* Length of the extent name. */
    char	    loc_extent[128];	/* Physical extent name. */
}   DM2D_LOC_ENTRY;


/*}
** Name: DM2D_ALTER_INFO - Info passed to dm2d_alter_db
**
** Description:
**	This structure defines the information needed by dm2d_alter_db. 
**
** History:
**     16-oct-1993 (jrb)
**          Created for MLSorts.
*/
typedef struct _DM2D_ALTER_INFO
{
    i4	    	lock_list;		/* just what it says */
    i4		lock_no_wait;		/* boolean: wait on locks? */
    i4		logging;		/* boolean: should we log? */
    i4		locking;		/* boolean: should we lock? */
    DB_DB_NAME	    	*name;			/* db name */
    char		*db_loc;		/* db location */
    i4		l_db_loc;		/* length of above location */
    DB_LOC_NAME	    	*location_name;		/* location to alter */
    i4		alter_op;		/* what kind of alter */

#define		DM2D_TAS_ALTER	    0x00000001L /* Used in dm2d_alter_db to
						** indicate changes to access,
						** service, and table id 
						** fields in the config file.
						*/
#define		DM2D_EXT_ALTER      0x000000002 /* Used in dm2d_alter_db to
						** indicate changes to the
						** extent bits in the config
						** file.
						*/
    union					/* op specific stuff */
    {
	struct 
	{
	    i4         db_service;         /* Database service. */
	    i4         db_access;          /* Database access. */
	    i4         tbl_id;             /* New next table id. */
	} tas_info;

	struct 
	{
	    i4	    drop_loc_type;      /* Bits to reset in extent */
	    i4	    add_loc_type;	/* Bits to set in extent */
	} ext_info;

    } alter_info;

}   DM2D_ALTER_INFO;

/*
**  Forward and/or External function references.
*/

FUNC_EXTERN DB_STATUS	dm2d_add_db(
				i4             flag,
				i4             *mode,
				DB_DB_NAME          *name,
				DB_OWN_NAME         *owner,
				i4             loc_count,
				DM2D_LOC_ENTRY      *location,
				DMP_DCB		    **db_id,
                                i4             *lock_list,
				DB_ERROR 	     *dberr );

FUNC_EXTERN DB_STATUS	dm2d_close_db(
				DMP_DCB		    *db_id,
				i4             lock_list,
				i4             flag,
				DB_ERROR 	     *dberr );

FUNC_EXTERN DB_STATUS	dm2d_release_user_tcbs(
				DMP_DCB		*db_id,
				i4		lock_list,
				i4		log_id,
				i4		release_flag,
				DB_ERROR	*dberr);

FUNC_EXTERN DB_STATUS	dm2d_del_db(
				DMP_DCB		    *db_id,
				DB_ERROR 	     *dberr );

FUNC_EXTERN DB_STATUS	dm2d_open_db(
				DMP_DCB		    *db_id,
				i4            access_mode,
				i4            lock_mode,
				i4            lock_list,
				i4            flag,
				DB_ERROR 	     *dberr );

FUNC_EXTERN DB_STATUS	dm2d_extend_db(
				i4             lock_list,
				DB_DB_NAME          *name,
				char                *db_loc,
				i4             l_db_loc,
				DB_LOC_NAME         *location_name,
				char                *area,
				i4             l_area,
				char                *path,
				i4             l_path,
				i4             loc_type,
				i4		raw_pct,
				i4	       *raw_start,
				i4	       *raw_blocks,
				DB_ERROR 	     *dberr );

DB_STATUS		dm2d_alter_db(
				DM2D_ALTER_INFO	    *dm2d,
				DB_ERROR 	     *dberr );

FUNC_EXTERN DB_STATUS 	dm2d_check_db_backup_status(
				DMP_DCB         *dcb,
				DB_ERROR 	     *dberr );

FUNC_EXTERN DB_STATUS   dm2d_check_db_backup_status(
    				DMP_DCB     *dcb,
				DB_ERROR 	     *dberr );

FUNC_EXTERN DB_STATUS 	dm2d_get_dcb(
				i4		dbid,
				DMP_DCB		**pdcb,
				DB_ERROR 	     *dberr );

FUNC_EXTERN bool	dm2d_row_lock(
				DMP_DCB		*dcb);
