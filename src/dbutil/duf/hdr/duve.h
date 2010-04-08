/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: duve.h - constants & structures for verifydb
**
** Description:
**      contains constants and structures need for verifydb.
**	Definitions in this include file are dependent on:
**	    DB_MAXNAME	    -  defined in dbms.h
**	    DU_ERROR	    -  defined in duerr.h
**	    DU_FILE	    -  defined in duvfiles.h
**
** History
**      24-Jun-1988 (teg)
**          Initial creation for verifydb project
**	24-Aug-88   (teg)
**	    Created new operation DUVE_FORCED.
**	25-Aug-1988 (teg)
**	    updated to meet RTI coding standards
**	05-Sep-1988 (teg)
**	    added stucture DUVE_DPNDS and DUVE_STAT
**	01-Feb-1989 (teg)
**	    added structure DUVEUSR
**	28-Feb-1989 (teg)
**	    added structures/constants for PURGEDB function
**	16-Jun-89 (teg)
**	    removed constant DU_FORCED_CONSISTENT (also defined in dudbms.qh)
**	    to avoid UNIX Compiler warnings.
**	14-sep-89 (teg)
**	    added constant for Xtable operation.
**	08-dec-1989 (teg)
**	    incorporated ingresug change 90871 changes from swan::blaise
**	    which was:
**		Removed superfluous typedefs before structs; some compilers 
**		don't like these
**      29-mar-90 (teg)
**          added support for ACCESSCHECK operation.
**	08-aug-90 (teresa)
**	    added constants: DUVE_WARN, DUVE_SPECIAL and DUVE_FATAL.
**	15-feb-91 (teresa)
**	    added DUVE_TREE to fix bug 35940
**	20-sep-91 (teresa)
**	    move DU_IDENTICAL to dudbms.qsh for ingres63p change 263293
**	7-sep-93 (robf)
**	    Add DUVE_LABEL_PURGE for label_purge operation
**	20-dec-93 (robf)
**          Enhancements for new secure catalog checks.
**	    - Add DU_PROFLST for profile caching
**	30-dec-93 (andre)
**	    defined DUVE_TEST_PERF and DUVE_QRY_PERF
**      18-jan-94 (andre)
**          defined DUVE_FIXIT and added duvefixit to DUVE_CB
**	31-jan-94 (jrb)
**	    Added duve_type to DU_DIRLST for MLSort support (for purgedb).
**	    - Add duveprof/duve_profcnt to of duve structure.
**      19-dec-94 (sarjo01) from 08-nov-94 (nick)
**          altered contents of DUVE_DIRLST structure - we now keep
**          additional flags.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*
**  Forward and/or External typedef/struct references.
*/
typedef struct _DU_RELINFO	DU_RELINFO;
typedef struct _DUVE_RELINFO	DUVE_RELINFO;
typedef struct _DU_D_DPNDS	DU_D_DPNDS;
typedef struct _DU_I_DPNDS      DU_I_DPNDS;
typedef struct _DUVE_DPNDS	DUVE_DPNDS;
typedef struct DUVE_ADD_SCHEMA_ DUVE_ADD_SCHEMA;
typedef struct DUVE_DROP_OBJ_	DUVE_DROP_OBJ;
typedef struct DUVE_FIXIT_	DUVE_FIXIT;
typedef struct _DU_STAT		DU_STAT;
typedef struct _DUVE_STAT	DUVE_STAT;
typedef struct _DU_DBNAME	DU_DBNAME;
typedef struct _DU_DBLIST	DU_DBLIST;
typedef struct _DU_LOCINFO	DU_LOCINFO;
typedef struct _DUVE_LOCINFO	DUVE_LOCINFO;
typedef struct _DU_USR		DU_USR;
typedef struct _DU_USRLST	DU_USRLST;
typedef struct _DU_PROF		DU_PROF;
typedef struct _DU_PROFLST	DU_PROFLST;
typedef struct _DU_GRP		DU_GRP;
typedef struct _DU_GRPLST	DU_GRPLST;
typedef struct _DU_ROLE		DU_ROLE;
typedef struct _DU_ROLELST	DU_ROLELST;
typedef struct _DU_DBINFO	DU_DBINFO;
typedef struct _DUVE_DBINFO	DUVE_DBINFO;
typedef struct _DU_DIRLST	DU_DIRLST;
typedef struct _DUVE_DIRLST	DUVE_DIRLST;
typedef struct _DUVE_TREE	DUVE_TREE;
typedef struct _DU_INTEGRITY    DU_INTEGRITY;
typedef struct _DUVE_INTEGRITY  DUVE_INTEGRITY;
typedef struct _DU_SCHEMA	DU_EVENT;
typedef struct _DUVE_EVENT	DUVE_EVENT;
typedef struct _DU_SCHEMA	DU_PROCEDURE;
typedef struct _DUVE_PROCEDURE  DUVE_PROCEDURE;
typedef struct _DU_SCHEMA	DU_SYNONYM;
typedef struct _DUVE_SYNONYM	DUVE_SYNONYM;
typedef struct DUVE_PERF_	DUVE_PERF;
/**[@forward_function_references@]
*/

/*
**  Defines of other constants.
*/

#define			DU_BUFMAX 	256	/* largest input line */
#define			DU_DATABASE_DATABASE "iidbdb"
#define                 DU_STARMAXNAME	    32	/* star names are 32 char long*/
#define			DUVE_MX_MSG_PARAMS  5	/* max number of paramater sets
						   for duve_talk() */
/*
**	general flags for use with all utilities 
*/
#define		        DUVE_BAD	0xff	/* error return code */
#define			DUVE_NO		0xee	/* yes/no permission = NO */
#define			DUVE_YES	E_DB_OK	/* yes/no permission = YES */
#define			DUVE_FATAL	0xdd	/* Indicates prev. error was 
						** fatal, abort processing */
#define			DUVE_WARN	0xcc	/* indicates prev. error not
						** fatal, print server's msg
						** and continue processing.
						*/
#define			DUVE_SPECIAL	0xbb	/* indicates prev error was
						** non-fatal and duve_ingerr
						** has already printed warning,
						** so suppress server message */
#define			DUVE_BAD_TYPE	-7	/* unexpcted object type was 
						** found in IIDBDEPENDS tuple
						*/
#define			DUVE_ADD	-6	/* indicates a schema has to
						** to be created 
						*/
#define			DU_INTRNL_ER	-5
#define			DU_NOTKNOWN	-4
#define			DU_NOSPECIFY	-3	/* parameter not specified */
#define			DUVE_DROP	-2      /* indicates a table is marked
						** for delete during system
						** catalogs checks
						*/

#define			DU_INVALID	-1	/* invalid parameter */

#define			DU_SQL_OK	0	/* sqlca.sqlcode for no error */

#define			DU_MODE		1
#define			DU_SCOPE	2
#define			DU_OPER		3


						/*
						** measuring performance of a
						** test (use duve_cb.
						** duve_test_perf_numbers)
						*/
#define			DUVE_TEST_PERF	10

						/*
						** measuring performance of a
						** query (use duve_cb.
						** duve_qry_perf_numbers)
						*/
#define			DUVE_QRY_PERF	11

#define		        DU_SQL_NONE     100	/* sqlca.sqlcode value for
						** "0 ROWS" */

/*
**	limits
*/
#define			DU_NAMEMAX	10	/*max # names allowed in input*/


/*
**[@global_variable_references@]
*/

/*}
** Name: DUVE_RELINFO - iirelation information that is used by several tests
**
** Description:
**      This structure contains information obtained from iirelation that
**	is referenced by other tests.  It saves the following fields from
**	iirelation so that multiple retrieves are not required.
**		du_id	=   reltid, reltidx
**		du_tab	=   relid
**		du_own  =   relowner
**		du_stat =   relstat
**		du_stat2=   relstat2
**	        du_attno=   relatts
**		du_qid1/
**		du_qid2 =   relqid1, relqid2 (or querry id from iiprotect or
**					      from iiintegrity)
**
** History:
**      28-Jul-1988 (teg)
**	    created for verifydb
**	25-Aug-1988 (teg)
**	    updated to meet RTI coding standards
**	08-dec-1989 (teg)
**	    incorporated ingresug change 90871 changes from swan::blaise
**	    which was:
**		Removed superfluous typedefs before structs; some compilers 
**		don't like these
**	03-dec-93 (andre)
**	    added du_stat2 to hold relstat2
**	20-dec-93 (robf)
**          Added du_stat2
*/
struct _DU_RELINFO
{
    DB_TAB_ID	du_id;			      /* base and index id for table */
    char	du_tab[sizeof(DB_TAB_NAME)+1]; /* null terminated table name */
    char	du_own[sizeof(DB_OWN_NAME)+1]; /* null terminated table name */
    i4		du_stat;		      /* holds relstat */
    i4		du_stat2;		      /* holds relstat2 */
    i2		du_attno;		      /* number of cols in table */
    i2		du_idxcnt;		      /* number of 2ndary indexes */
    i4		du_tbldrop;		      /* pos  -> drop table from db */
    i4          du_schadd;                    /* pos  -> add schema */	
    i4		du_qid1,du_qid2;	      /* querrytext id for view,
					      ** protection or integrity */
} ;

struct _DUVE_RELINFO
{
    DU_RELINFO	rel[1];	    /* array of DU_RELINFO */
};

/*}
** Name: DU_D_DPNDS - structure describing dependent object
**
** Description:
**      This structure contains a description of a dependent object found in one
**	or more IIDBDEPENDS tuples.  One or more independent object info
**	elements (described by DU_I_DPNDS) may be associated with each dependent
**	object info element.  Last independent object info element in the list
**	of those associated with a given dependent object info element will
**	contain the address of the latter making it fairly painless to reach
**	the dependent object info element given any independent object info
**	element.
**
** History:
**	07-dec-93 (andre)
**	    written
**	11-jan-94 (andre)
**	    defined DU_UNEXPECTED_DEP_TYPE
*/
struct _DU_D_DPNDS
{
    DB_TAB_ID	du_deid;	/* base and index id for dependent object */
    i4		du_dtype;	/* type of dependent object */
    i4		du_dqid;	/* dependent object secondary id */
    i4		du_dflags;
				/*
				** object on which the object described by this 
				** element depends does not exist OR is marked
				** for dropping
				*/
#define	DU_MISSING_INDEP_OBJ		0x01

				/*
				** this object has been marked for destruction
				*/
#define	DU_DEP_OBJ_WILL_BE_DROPPED	0x02

				/*
				** object described by this element does not
				** exist
				*/
#define	DU_NONEXISTENT_DEP_OBJ		0x04

				/*
				** dependent object type was not one of the 
				** expected ones - this means that either 
				** ckdbdpnds() needs to be updated to reflect 
				** new types of dependent objects or that 
				** IIDBDEPENDS got corrupted
				*/
#define	DU_UNEXPECTED_DEP_TYPE		0x08

    DU_I_DPNDS	*du_indep;	/* first element of indep object info list */
};

/*}
** Name: DU_I_DPNDS - structure describing independent object
**
** Description:
**      This structure contains a description of an independent object found in
**	one or more IIDBDEPENDS tuples.  One or more independent object info
**	elements (described by DU_I_DPNDS) may be associated with each dependent
**	object info element.  Last independent object info element in the list
**	of those associated with a given dependent object info element will
**	contain the address of the latter making it fairly painless to reach
**	the dependent object info element given any independent object info
**	element.
**
** History:
**	07-dec-93 (andre)
**	    written
**	11-jan-94 (andre)
**	    defined DU_UNEXPECTED_INDEP_TYPE
*/
struct _DU_I_DPNDS
{
    DB_TAB_ID	du_inid;	/* base and index id for independent object */
    i4		du_itype;	/* type of independent object */
    i4          du_iqid;	/* independent object secondary id */
    i4		du_iflags;
				/* 
				** last element of a list of independent object
				** info elements; if set
				*/
#define	DU_LAST_INDEP			0x01

				/*
				** this object has been marked for destruction
				*/
#define	DU_INDEP_OBJ_WILL_BE_DROPPED	0x02

				/*
				** object described by this element does not 
				** exist
				*/
#define	DU_NONEXISTENT_INDEP_OBJ	0x04

				/*
				** independent object type was not one of the 
				** expected ones - this means that either 
				** ckdbdpnds() needs to be updated to reflect 
				** new types of independent objects or that 
				** IIDBDEPENDS got corrupted
				*/
#define	DU_UNEXPECTED_INDEP_TYPE	0x08

    union
    {
	/*
	** if du_iflags & DU_LAST_INDEP, du_dep will point at the dependent 
	** object info element to which it (and other elements in the list 
	** belong; otherwise, du_inext will point at the next element of the 
	** independent object info list assocciated with a given dependent 
	** object info element)
	*/
	DU_I_DPNDS	*du_inext;
	DU_D_DPNDS	*du_dep;
    } du_next;
};

/*}
** Name: DUVE_DPNDS - iidbdepends information
**
** Description:
**      This structure contains information obtained from iidbdpends that
**	is referenced by other tests.  It saves the following fields from
**	iidbdepends so that multiple retrieves are not required.
**
** History:
**      05-Sep-1988 (teg)
**	    created for verifydb
**	08-dec-1989 (teg)
**	    incorporated ingresug change 90871 changes from swan::blaise
**	    which was:
**		Removed superfluous typedefs before structs; some compilers 
**		don't like these
**	07-dec-93 (andre)
**	    DUVE_DPNDS will contain a pointer to DU_D_DPNDS and DU_I_DPNDS as 
**	    well as numbers of used elements in the dependent and independent 
**	    info lists.
**
**	    IIDBDEPENDS cache will consist of an array of dependent object info
**	    elements with associated independent object info elements hanging 
**	    off each dependent object info element.
**
**	    Array of independent object info elements will be sorted by du_inid,
**	    du_itype, du_i_qid (will be populated by selecting from iidbdepends
**	    which is a btree on independent object id, type, and qid) which will
**	    allow us to use binary search to look for any given element; array 
**	    of dependent object ids will NOT be in any particular order, so we 
**	    will have to scan it to find a given element.  Since we expect to 
**	    have multiple independent info object elements to correspond to a 
**	    given dependennt object info element, decoupling dependent and 
**	    independent info elements is expected to make seacrhes for dependent
**	    object element faster (searches for independent object elements will
**	    be fast because we will be able to employ binary search.)
**
** 	(as of 12/06/93) IIDBDEPENDS is expected to describe the following types
** 	of dependencies:
**   	  - view depends on underlying objects
**   	  - dbproc depends on underlying objects (tables/views, dbevents, other
**     	    dbprocs, synonyms)
**   	  - permit depends on a table (non-GRANT compat. perms)
**   	  - permit depends on a privilege descriptor contained in IIPRIV
**   	  - rules created to enforce a constraint depend on that constraint
**   	  - dbprocs created to enforce a constraint depend on that
**     	    constraint
**   	  - indices created to enforce a constraint depend on that constraint
**   	  - REFERENCES constraints depend on UNIQUE constraints on the
**          <referencing columns>
**   	  - REFERENCES constraints depend on REFERENCES privilege on
**          <referenced columns> (if the owner of the <referenced table> is
**          different from that of the <referencing table>)
*/
struct _DUVE_DPNDS
{
    i4			duve_indep_cnt;	/* 
					** number of elements used in the
    					** list pointed to by duve_indep
    					*/
    i4			duve_dep_cnt;	/*
					** number of elements used in the
					** list pointed to by duve_dep
					*/
    DU_I_DPNDS		*duve_indep;	/* list of indep object info elements */
    DU_D_DPNDS		*duve_dep;	/* list of dep object info elements */
};

/*}
** Name: DUVE_STAT  - iistatistics information
**
** Description:
**      This structure contains information obtained from iistatistics and
**	iihistogram (for these tables must be examined in conjunction with
**	each other.
**
** History:
**      07-Sep-1988 (teg)
**	    created for verifydb
**	08-dec-1989 (teg)
**	    incorporated ingresug change 90871 changes from swan::blaise
**	    which was:
**		Removed superfluous typedefs before structs; some compilers 
**		don't like these
*/
struct _DU_STAT
{
    DB_TAB_ID	du_sid;		/* base and index id for table receiving stats*/
    i4		du_satno;	/* attribute receiving statistics */
    i4		du_numcel;	/* num of histogram cells for this statistic --
				** nonzero only when it needs to be modified */
    i4		du_sdrop;	/* drop statistics */
    i4		du_hdrop;	/* drop histogram */
    i4	du_rptr;	/* pointer to iirelation cache entry */
} ;

struct _DUVE_STAT
{
    DU_STAT	stats[1];	    /* array of DU_STAT */
};

/*}
** Name: DUVE_LOCINFO  - database location information
**
** Description:
**      This structure contains information obtained from iilocations in the
**	iidbdb.
**
** History:
**      29-Sep-1988 (teg)
**	    created for verifydb
**	08-dec-1989 (teg)
**	    incorporated ingresug change 90871 changes from swan::blaise
**	    which was:
**		Removed superfluous typedefs before structs; some compilers 
**		don't like these
**	20-Feb-2009 (kschendel) b122041
**	    Make duloc a (null terminated) string, since that's how it comes
**	    back from esql.
*/

struct _DU_LOCINFO
{
    char	    duloc[DB_MAXNAME+1];    /* location name */
    i4		    du_lstat;		    /* location status --
					    **	DU_DBS_LOC - database loc
					    **	DU_SORTS_LOC - sort loc
					    **	DU_JNLS_LOC - journal loc
					    **	DU_CKPS_LOC - chekcpoint loc
					    **/
    i4		    du_lockill;		    /* nonzero indicates invalid loc */
};

struct _DUVE_LOCINFO
{
    DU_LOCINFO    du_locname[1];		    /* array of DU_LOCINFO */
};

/*}
** Name: DU_DBLIST - list of databases
**
** Description:
**      This structure contains a list of database names collected from the
**	iidbdb's iidatabase catalog.  This list can include all of the
**	databases in an installation, or all of the databases owned by a given
**	dba.
**
** History:
**      29-Sep-1988 (teg)
**	    created for verifydb
**      08-Feb-1988 (teg)
**	    added compat level checks to verifydb
**	08-dec-1989 (teg)
**	    incorporated ingresug change 90871 changes from swan::blaise
**	    which was:
**		Removed superfluous typedefs before structs; some compilers 
**		don't like these
**      29-mar-90 (teg)
**          added du_cdb, du_dbservice and du_dbmstype for bug 20971.
**	12-Nov-2009 (kschendel) SIR 122882
**	    Change compat level to u_i4.
*/

struct _DU_DBNAME
{
    char	du_db[DB_MAXNAME+1];	    /* database name */
    char	du_dba[DB_MAXNAME+1];	    /* database owner */
    u_i4	du_lvl;			    /* Database major compat level */
    char        du_cdb[DU_STARMAXNAME+1];   /* name of CDB associated with DDB
                                            -- garbage if du_db is not a ddb */
    char        du_dbmstype[DU_STARMAXNAME+1]; /* type of server CDB built on
                                            -- garbage if du_db is not a ddb */
    i4          du_dbservice;               /* copy of iidatabase.dbservice --
                                            ** has bit U_1USR_DDB set if DDB */
    i4		du_minlvl;		    /* database minor compat level */
};
struct _DU_DBLIST
{
    DU_DBNAME	du_dblist[1];
};

/*}
** Name: DU_USRLST - list of INGRES users known to this installation.
**
** Description:
**      This structure contains a list of user names collected from the
**	iidbdb's iiuser catalog.
**
** History:
**      01-Feb-1989 (teg)
**	    created for verifydb
**	08-dec-1989 (teg)
**	    incorporated ingresug change 90871 changes from swan::blaise
**	    which was:
**		Removed superfluous typedefs before structs; some compilers 
**		don't like these
**	06-Jun-94 (teresa)
**	    added constant DUVE_USER_NAME for bug 60761.
**	10-jan-1996 (toumi01; from 1.1 axp_osf port)
**	    Make du_usrstat i4 (32-bits), not long (64-bits).
*/

#define DUVE_USER_NAME	1   /* bitmask parameter for routine  duve_usrfind() */
struct _DU_USR
{
    char	du_user[DB_MAXNAME+1];
    i4		du_usrstat;
};
struct _DU_USRLST
{
    DU_USR	du_usrlist[1];
};

/*}
** Name: DU_PROFLST - list of INGRES profiles known to this installation.
**
** Description:
**      This structure contains a list of profiles collected from the
**	iidbdb's iiprofile catalog.
**
** History:
**      20-dec-93 (robf)
**	    Created 
**	10-jan-1996 (toumi01; from 1.1 axp_osf port)
**	    Make du_profstat i4 (32-bits), not long (64-bits).
*/

struct _DU_PROF
{
    char	du_profile[DB_MAXNAME+1];
    i4		du_profstat;
};
struct _DU_PROFLST
{
    DU_PROF	du_proflist[1];
};

/*}
** Name: DU_GRPLST - list of user groups known to this installation.
**
** Description:
**      This structure contains a list of user group names collected from the
**	iidbdb's iiusergroup catalog.
**
** History:
**      06-Jun-94 (teresa)
**	    created for bug 60761
*/

#define DUVE_GROUP_NAME	2   /* bitmask parameter for routine  duve_usrfind() */
struct _DU_GRP
{
    char	du_usergroup[DB_MAXNAME+1];
};
struct _DU_GRPLST
{
    DU_GRP      du_grplist[1];
};

/*}
** Name: DU_ROLELST - list of INGRES roles known to this installation.
**
** Description:
**      This structure contains a list of role names collected from the
**	iidbdb's iirole catalog.
**
** History:
**      06-Jun-94 (teresa)
**          created for bug 60761
*/

#define DUVE_ROLE_NAME	4   /* bitmask parameter for routine  duve_usrfind() */
struct _DU_ROLE
{
    char	du_role[DB_MAXNAME+1];
};
struct _DU_ROLELST
{
    DU_ROLE     du_rolelist[1];
};

/*}
** Name: DUVE_DBINFO - Information about databases in this installation.
**
** Description:
**      This structure contains information about databases in the installation.
**	The database information is collected from the iidatabase catalog.
**
** History:
**      01-Feb-1989 (teg)
**	    created for verifydb
**	08-dec-1989 (teg)
**	    incorporated ingresug change 90871 changes from swan::blaise
**	    which was:
**		Removed superfluous typedefs before structs; some compilers 
**		don't like these
*/

struct _DU_DBINFO
{
    char	du_database[DB_MAXNAME+1];
    char	du_dbown[DB_MAXNAME+1];
    i4		du_dbid;
    u_i4	du_tid;
    i4		du_access;
    bool        du_badtuple;
};
struct _DUVE_DBINFO
{
    DU_DBINFO  du_db[1];
};

/*}
** Name: DUVE_DIRLST - list of directories associated with this db
**
** Description:
**      This structure contains information about the directories that comprise
**	this database, mapping logical location information to physical
**	location information.
**
** History:
**      01-Mar-1989 (teg)
**	    created for verifydb
**	08-dec-1989 (teg)
**	    incorporated ingresug change 90871 changes from swan::blaise
**	    which was:
**		Removed superfluous typedefs before structs; some compilers 
**		don't like these
**	31-jan-94 (jrb)
**	    Added duve_type for MLSort support (for purgedb).  The location
**	    type is defined in DU_LOCATIONS/du_status.
**      19-dec-94 (sarjo01) from 08-nov-1994 (nick)
**          Changed duvedefault to duvedirflags and defined the flags
**          it contains.
*/
struct _DU_DIRLST
{
    char	duvedb[DB_MAXNAME+1];	    /* db name */
    char	duvedba[DB_MAXNAME+1];	    /* dba name */
    char	duveloc[DB_MAXNAME+1];	    /* location logical name */
    char	duve_area[DB_AREA_MAX+1];   /* physical mapping to location */
    i4	        duve_type;   		    /* location type */
# define        DUVE_DEFAULT    0x1         /* default location */
# define        DUVE_ALIAS      0x2         /* alias location */
/* Can conatin another flag DU_EXT_RAW 0x10 (defined in dudbms.h) 
   which signifies, this location is RAW (b113078) */
    i4          duvedirflags;
} ;

struct _DUVE_DIRLST
{
    DU_DIRLST	dir[1];	    /* array of database directory info */
};

/*}
** Name: DUVE_TREE - list of tree tuples that must be dropped
**
** Description:
**      This structure contains the table id and tree id (both are
**	2 word identifiers) to identify a specific tree.  Table id
**	alone is not sufficient, as there can be multiple quel permits
**	on the same table.
**
** History:
**	15-feb-91 (teresa)
**	    added to fix bug 35940
*/
struct _DUVE_TREE
{
    DB_TAB_ID	    tabid;	    /* table id */
    DB_TREE_ID	    treeid;	    /* tree id */
    i2		    treemode;	    /* type of tree: (DB_EVENT, DB_RULE, etc) */
};

/*{
** Name: DUVE_INTEGRITY - iiintegrity information
**
** Description:
**     This structure contains information obtained from iiintegrity,
**     iirelation and iiqrytext.
**
** History:
**      06-dec-93 (anitap)
**          Created for verifydb for FIPS' constraints project.
**	25-jan-94 (anitap)
**	    Changed du_txtstring and du_rtxtstring to be pointers to
**	    accomodate query text tuples which are more than one tuple in
**	    length. 
*/
struct _DU_INTEGRITY
{
    char                du_tabname[DB_MAXNAME + 1];
                                       /* name of table on which constraint
                                       ** has to be dropped and recreated 
                                       */

    char                du_ownname[DB_MAXNAME + 1];
					/* owner of above table */

    char                du_consname[DB_MAXNAME+ 1];
                                        /* name of constraint to be droppped */

    char                *du_txtstring;
                                       /* text of constraint to be created */

    char                du_rtabname[DB_MAXNAME + 1];
                                       /* name of table on which referential
                                       ** constraint has to be created.
                                       */

    char                du_rownname[DB_MAXNAME + 1];
                                        /* owner of above table */

    char                *du_rtxtstring;
                                       /* text of referential constraint
                                       ** dependent on unique constraint */

    i4                  du_dcons;        /* drop constraint */

    i4			du_acons;	/* add constraint */

    i4                  du_rcons;       /* add referential constraint which
                                        ** got dropped when the unique
                                        ** constraint which it depended upon
                                        ** was dropped.
                                        */
};    

struct _DUVE_INTEGRITY
{
    DU_INTEGRITY        integrities[1]; /* array of DU_INTEGRITY */
};

/*{
** Name: DUVE_EVENT - iischema information
**
** Description:
**
** History:
**	13-dec-93 (anitap)
**	    Created for verifydb for CREATE SCHEMA project.
**
*/
struct _DU_SCHEMA
{
    char           du_schname[DB_MAXNAME + 1]; /* name of schema
	                                       ** to be created
                                               */
};

struct _DUVE_EVENT
{
    DU_EVENT   events[1];             /* array of DU_SCHEMA */
};

struct _DUVE_PROCEDURE
{
    DU_PROCEDURE   procs[1];             /* array of DU_SCHEMA */
};

struct _DUVE_SYNONYM
{
    DU_SYNONYM   syns[1];	      /* array of DU_SCHEMA */
};


/*}
** Name: DUVE_PERF - structure used to determine performance characteristics of
**		     a query or a set of queries
**
** Description:
**      This structure will contain information about performance
**	characteristics of a query or a set of queries.
**
** History:
**	30-dec-93 (andre)
**	    written
*/
struct DUVE_PERF_
{
    i4	dio_before;	    /* used to measure dio */
    i4	dio_after;	    /* used to measure dio */
    i4	bio_before;	    /* used to measure bio */
    i4	bio_after;	    /* used to measure bio */
    i4	ms_before;	    /* used to measure ms_cpu */
    i4	ms_after;	    /* used to measure ms_cpu */
};

/*}
** Name: DUVE_ADD_SCHEMA - description of a schema that needs to be created
**
** Description:
**	VERIFYDB may discover that there exists a named object whose 
**	corresponding schema does not exist.  This structure will serve to 
**	describe such missing schema which would then be created.  
**
**	Note that in 6.5 name of the schema is the same as that of its owner 
**	which makes it quite simple to figure out the name of the schema owner.
**	In subsequent release when we add support for users to own multiple 
**	schemas, a different approach may need to be taken (e.g. to create a 
**	schema owned by some mysterious user or prompt the user for the schema 
**	owner name.)
**
** History:
**	18-jan-94 (andre)
**	    written
*/
struct DUVE_ADD_SCHEMA_
{
    DUVE_ADD_SCHEMA	*duve_next;
    char		duve_schema_name[DB_MAXNAME + 1];
};

/*}
** Name: DUVE_DROP_OBJ - description of an object that neesds to be dropped
**
** Description:
**	VERIFYDB may determine that an object needs to be dropped because, for 
**	instance, an object on which it depends no longer exists.  This 
**	structure will contain a description of an object that needs to be 
**	dropped or, in case of database procedures, marked dormant (which 
**	involves deleting dependency information and updating IIPROCEDURE 
**	tuple to indicate that the dbproc is now dormant.
**
** History:
**      18-jan-94 (andre)
**          written
**	28-jan-94 (andre)
**	    added duve_drop_flags and defined DUVE_DBP_DROP over it.
*/
struct DUVE_DROP_OBJ_
{
    DUVE_DROP_OBJ	*duve_next;
    DB_TAB_ID		duve_obj_id;
    i4			duve_obj_type;
    i4			duve_secondary_id;
    i4			duve_drop_flags;

					/*
					** this flag will be set to indicate 
					** that this dbproc needs to be dropped
					** (usually, we just mark them dormant)
					** because, for instance, its query text
					** is missing or IIPROCEDURE_PARAMETER 
					** does not contain description of 
					** some/all of its parameters, etc.
					*/
#define		DUVE_DBP_DROP	    0x01
};

/*}
** Name: DUVE_FIXIT - header of lists of description of objects that need to be 
**		      dropped and schemas that need to be created after VERIFYDB
**		      finished looking at system catalogs
**
** Description:
**	This structure contains pointers to the list of descriptions of objects
**	that should be dropped and the list of descriptions of schemas that 
**	need to be created.
**
** History:
**      18-jan-94 (andre)
**          written
*/
struct DUVE_FIXIT_
{
    DUVE_ADD_SCHEMA	*duve_schemas_to_add;
    DUVE_DROP_OBJ	*duve_objs_to_drop;
};
/*}
** Name: DUVE_CB - Verifydb control block
**
** Description:
**	Verifydb control block is how verifydb keeps track of what it is
**	doing.  It contains the following types of information:
**	    1.  Command line parameteres (mode,scope,operation,user,flags)
**	    2.  General Housekeeping (pointers to internal structures, file
**		    control blocks, open database info, etc)
**	    3.  Pointers to Memory Cache (to avoid mulitiple I/Os to catalogs
**		    verifydb has already walked.
**	    4.  Purge operation information
**
**      This structure also supplies information about mode and message
**	type, that is used in verifydb's communication process.
**
**	The mode and message type are used to determine whether or not the
**	message should be output to the user's terminal (or stdout) and/or
**	to the verifydb log file.
**
**	Message types are:
**	    DUVE_ALWAYS  -- output msg to stdout and to log
**	    DUVE_DEBUG	 -- output msg to log
**	    DUVE_MODESENS-- examine mode before outputting msg.  If mode
**			    permits msg, then output it.  Else dont.
**	    DUVE_IO	 -- only output msg in interactive mode. If mode not
**			    interative, output msg that corrective action only
**			    permitted in interactive mode
**	    DUVE_ASK	 -- only output msg in interactive mode, but return
**			    DUVE_YES for DUVE_RUN, and DUVE_SILENT
**	    DUVE_LEGAL	 -- issue legal warning, prompt for continue.
**
**      The duve_msg pointer indicates the message control block (all Database
**	Utility Facility members use this to format msgs) that verifydb uses.
**
**	This structure is central to all verifydb processing.
**
** History:
**      13-Jul-1988 (teg)
**	    created for verifydb
**	25-Aug-1988 (teg)
**	    updated to meet RTI coding standards
**	05-Sep-1988 (teg)
**	    added DUVE_DPNDS to control block
**	19-Jan-1989 (teg)
**	    added new message type of DUVE_LEGAL
**	02-Feb-1989 (teg)
**	    added new structures to implement iidbdb catalog checks.
**	14-sep-89 (teg)
**	    added new constant DUVE_XTABLE
**	27-dec-89 (teg)
**	    added du_ownstat to determine if VERIFYDB user is INGRES superuser.
**      28-mar-90 (teg)
**          added new constant DUVE_ACCESS
**	15-feb-91 (teresa)
**	    duvetree becomes type DUVE_TREE for bug 35940
**	21-apr-92 (teresa)
**	    add du_exception_stat to fix bug 42660.
**	23-oct-92 (teresa)
**	    add duve_verbose for sir 42498.
**	07-dec-93 (andre)
**	    removed duve_dcnt (DUVE_DPNDS.duve_cnt will serve its purpose);
**	    changes duvedpnds from (DUVE_DPNDS *) to DUVE_DPNDS
**	10-dec-93 (teresa)
**	    added duve_dbms_test flag to control block for functionality to 
**	    suppress messages that cause spurious diffs when running tests but 
**	    are not considered significant difs.  This includes messages like: 
**	     S_DU1619_NO_VIEW, S_DU030C_CLEAR_VBASE, S_DU035C_CLEAR_VBASE, 
**	     S_DU165E_BAD_DB and S_DU167A_WRONG_CDB_ID
**	    It seems ashame to have to hack the code like this for testing, but
**	    the suprious difs wreck enough havic that the "powers that be" 
**	    deemed it necessary to address these difs.
**	18-jan-94 (andre)
**	    added duvefixit to DUVE_CB
**	19-feb-94 (anitap)
**	    added the following fields for delimited support for "-u" and 
**	    "-o" flags for verifydb command line: duve_unorm_tbl, 
**	    duve_norm_tbl, duve_unorm_usr.
*/
/* messge types */
#define                 DUVE_ALWAYS	0  /* always output to stdout & log */
#define                 DUVE_DEBUG	1  /* output only to stdout */
#define                 DUVE_MODESENS	2  /* use mode to decide whether or
					   ** not to output msg */
#define                 DUVE_IO		3  /* output only if mode is
					   ** RUNINTERACTIVE and prompt for
					   ** yes/no response, otherwise
					   ** generate msg that fix only allowed
					   ** in interactive mode */
#define			DUVE_ASK	4  /* Like mode sensitive, but also
					   ** prompt for yes/no response */
#define			DUVE_LEGAL	5  /* like DUVE_ASK, but use
					   ** different prompt */
typedef struct _DUVE_CB
{

    /*****************************************/
    /** Command Line Parameters		    **/
    /*****************************************/
    i4              duve_mode;          /* mode operator evoked verifydb with */
#define                 DUVE_RUN	1
#define                 DUVE_SILENT     2
#define                 DUVE_IRUN	3
#define                 DUVE_REPORT	4
    i4		    duve_scope;		/* scope operator evoked verifydb with*/
#define                 DUVE_SPECIFY	1
#define                 DUVE_DBA	2
#define                 DUVE_INSTLTN    3
    char	    **du_scopelst;	/* array of strings containing names
					** of databases specified as part of
					** the -sdbname parameter
					*/
    i4		    duve_nscopes;	/* number of strings in array
					**  scopelist
					*/
    i4  	    duve_operation;	/* code for specified operation --
					** for verifydb options are:
					**   DBMS_CATALOGS, FORCE_CONSIST,
					**   PURGE, TEMP_PURGE, EXPIRED_PURGE,
					**   TABLE, INVALID, NOTSPECIFIED,
					**   LABEL_PURGE
					*/
#define			DUVE_CATALOGS	1
#define			DUVE_MKCONSIST	2
#define			DUVE_FORCED	7   /* force consistent chgs operation
					    ** during execution */
#define			DUVE_PURGE	3
#define			DUVE_TEMP	4
#define			DUVE_EXPIRED	5
#define			DUVE_TABLE	6
#define			DUVE_DROPTBL	8   /* drop table from sys cats */
#define			DUVE_XTABLE	9   /* strict algorythm for patch table */
#define                 DUVE_ACCESS     10  /* do access check of target list */
#define			DUVE_REFRESH	11  /* refresh_ldbs */
#define			DUVE_LABEL_PURGE 12 /* label_purge */
    char	    **duve_ops;		/* array of strings containing a name
					** specified in the operation statement
					** (for verifydb, contains only 1
					**  element, which is name of table for
					**  TABLE operation)
					*/
    char	    duve_norm_tbl[DB_MAXNAME + 1];
					/* unnormalized table name */

    char	    duve_unorm_tbl[DB_MAXNAME + 1];
					/* normalized table name */

    i4  	    duve_numops;	/* number of strings in array
					**  oplist
					*/
    char	    duve_user[DB_MAXNAME+1];/* user name if -u flag is selected -- 
					** null if no -u specified on input line
					*/
    char	    duve_unorm_usr[DB_MAX_DELIMID + 4];
				       /* unnormalized user name */

    char            duve_auser[DB_MAXNAME+1];/* user name if -a flag is 
                                        ** selected --  null if no -a 
                                        ** specified on input line.  -a flag
                                        ** specifies access user for access
                                        ** check operation.
                                        */
    char	    duve_schema[DB_MAXNAME+1];/* schema name to be created */
    char            duve_lgfile[DB_MAXNAME+1];/* Name of alternate log file */
    bool	    duve_debug;		/* set to true by undocumented parameter
					** of -d on verifydb input line */
    bool	    duve_simulate;	/* set to true when undocumented parameter
					** of TEST_RUNI used.  This is for
					** testing verifydb (simulates Yes
					** answer from user when testing
					** VERIFYDB.)  This should NEVER be used
					** for real!!!
					*/
    bool	    duve_relstat;	/* set to true by undocumented parameter
				        ** of -rel on verifydb input line --
					** Used in conjunction with -odbms_catalogs
					** WILL CAUSE THE TCB_ZOPTSTAT BIT IN
					** IIRELATION.RELSTAT TO BE CLEARED IF
					** OPTIMIZER STATISTICS DONT EXIST. 
					** note:  this flag forces mode of
					**	  -mrunsilent.
					*/
    bool	    duve_altlog;	/* set to true by undocumented verifydb
					** parameter of -lf.  If true, then
					** duve_lgfile contains a name of the
					** log file string */
    bool	    duve_skiplog;	/* set to true by undocumented verifydb
					** parameter of -nolog */
    bool	    duve_timestamp;	/* set to true by undocumented verifydb
					** parameter of -timestamp */
    bool	    duve_verbose;	/* set to true if verbose flag is 
					** specified on the command line for a
					** table operation
					*/
    bool	    duve_test;		/* true if running verifydb tests.  This
					** standardizes the timestamp to
					** "TESTDAY 00:00:00" AND surpresses
					** output to stdout.
					*/
    bool	    duve_sep;		/* true if running verifydb tests in sep
					** mode. This standardizes the timestamp
					** to "TESTDAY 00:00:00", surpresses
					** output to stdout and then redirects
					** log file output to STDOUT so SEP can
					** trap results.  (Log file output and
					** stdout output are not the same.)
					*/
    DU_STATUS	    duve_dbms_test;	/* If this field is nonzero, then 
					** certain error messages will be 
					** repressed by duve_talk() in an 
					** attempt to reduce spurious difs.
					** however, if the field is set to the
					** value of the error message, then
					** that particular error message will
					** not be prepressed.
					*/
    /*****************************************/
    /** Housekeeping Information	    **/
    /*****************************************/
    char	    *du_utilid;		/* char string containing utility name*/
    char	    duveowner[DB_MAXNAME+1];  /* name of verifydb task owner */
    char	    du_opendb[DB_MAXNAME+1];  /* name of any current open database */
    DU_ERROR	    *duve_msg;          /* duve error control block */
    DU_FILE	    duve_log;		/* file descriptor for verifydb log */
    i4              du_ownstat;         /* owner status.  ie iiuser.status
					** for duveowner */
    i4	    du_exception_stat;	/* status from exception handler */
#define		DUVE_USR_INTERRUPT	1
#define		DUVE_EXCEPTION		2
    i4	    duve_sessid;	/* identifies which session verifydb is
					** in if its using multiple sessions */
#define		DUVE_MASTER_SESSION	1
#define		DUVE_SLAVE_SESSION	2
                                                                 
    /*****************************************/
    /** Memory Cache (to Reduce I/Os)	    **/
    /**	    [FOR DBMS_CATALOGS]		    **/
    /*****************************************/
    DUVE_RELINFO    *duverel;		/* pointer to array of critical info 
					** obtained from iirelation */
    i4	    duve_cnt;		/* num of elements in relinfo arrray--
					** entries are numbered from 0 thru
					** duve_cnt-1
					*/
    DUVE_DPNDS	    duvedpnds;		/* iidbdpends cache */
    DUVE_FIXIT	    duvefixit;		/* 
					** descriptions of objects that need 
					** to be dropped and schemas that need 
					** to be created 
					*/
    DUVE_STAT	    *duvestat;		/* iistatistics/iihistogram cache */
    i4	    duve_scnt;		/* number of entries in duvestat */
    DUVE_TREE	    *duvetree;		/* pointer to iitree cache -- this is
					** allocated by cktree, and deallocated
					** by it at the end of functioning.
					** However, it is possible to take an
					** error exit that doesn't do the
					** deallocate.  Therefore, the address
					** of the allocated memory is placed
					** here so that duve_close will dealloc
					** on the obscure path that cktree
					** doesn't and there are more databases
					** to check -- we dont want to run into
					** a memory allocation error, so we
					** clean up after ourself
					*/
    DUVE_INTEGRITY   *duveint;		/* iiintegrity cache */
    i4	     duve_intcnt;	/* number of entries in duveint */
	
    DUVE_EVENT      *duveeves;		/* iievent cache */
    i4	    duve_ecnt;		/* number of entries in duveeves */

    DUVE_PROCEDURE  *duveprocs;		/* iiprocedure cache */
    i4	    duve_prcnt;		/* number of entries in duveprocs */	

    DUVE_SYNONYM   *duvesyns;		/* iisynonym cache */
    i4	   duve_sycnt;	        /* number of entries in duvesyns */

    DU_DBLIST	    *duve_list;		/* pointer to cache of list of database
					** names, all belonging to a given dba 
					** or all the dbs in an installation
					*/
    i4	    duve_ncnt;		/* number of elements in database name
					** cache -- entries are numbered from 
					** 0 thru duve_ncnt-1
					*/
    DU_USRLST	    *duveusr;		/* pointer to cache of list of user
					** names
					*/
    i4	    duve_ucnt;		/* number of elements in database user
					** cache - elements are numbered
					** 0 thru duve_ucnt-1
					*/
    DU_GRPLST	    *duvegrp;		/* group cache */
    i4	    duve_gcnt;		/* number of elements in group cache */
    DU_ROLELST	    *duverole;		/*  role cache */
    i4	    duve_rcnt;		/* number of elements in role cache */
    DUVE_LOCINFO    *duve_locs;		/* pointer to cache of all of the
					** location names known to this 
					** installation
					*/
    i4	    duve_lcnt;		/* number of entries in location cache -
					** elements are numbered 0 thru 
					** duve_lcnt-1
					*/
    DUVE_DBINFO	    *duvedbinfo;	/* pointer to cache of information from
					** iidatabase iidbdb system catalog */
    i4	    duve_dbcnt;		/* number of entries in duvedbinfo
					** cache. Entries are numbered 0 thru
					** duve_dbcnt-1
					*/
    DU_PROFLST	    *duveprof;		/* pointer to cache of list of profile
					** names
					*/
    i4	    duve_profcnt;	/* number of elements in database 
					** profile cache - elements are numbered
					** 0 thru duve_ucnt-1
					*/
    /*****************************************/
    /**  PURGE LIST INFORMATION		    **/
    /*****************************************/
    DUVE_DIRLST	    *duve_dirs;		/* pointer to list of directories
					** associated with this db */
    i4	    duve_pcnt;		/* number of directories assoc with this
					** database */
    i4	    duve_delflg;	/* flag, set to true when doing a delete
					** of a file found via a previous 
					** iiqef_listfiles procedure call,
					** otherwise its false.
					*/

    /*********************************************************/
    /* --		VERIFYDB TRACES 		  -- */
    /*							     */
    /* These tracepoints are used mostly for testing.	     */
    /*							     */
    /* To set verifydb trace points, define a logical named  */
    /* II_VERIFYDB_TRACE to a single trace point or a series */
    /* of them.						     */
    /*      EXAMPLE 1 : set trace ponit 17		     */
    /*          define/job II_VERIFYDB_TRACE "17"	     */
    /*      EXAMPLE 2 : set trace points 01 and 11 and 12    */
    /*		define/job II_VERIFYDB_TRACE "1,11,12"	     */
    /*********************************************************/
#define	MAX_TRACE_PTS		30

    i4		    duve_trace[MAX_TRACE_PTS];

#define DUVE01_SKIP_IIRELIDX 1	    /* Skip the iirel_idx checks -- this trace
				    ** is implemented to workaround a known 6.5
				    ** bug */

    /* debug info -- */
    DUVE_PERF	duve_test_perf_numbers;
    DUVE_PERF	duve_qry_perf_numbers;

}   DUVE_CB;
/*[@type_definitions@]*/
