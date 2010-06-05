/*
** Copyright (c) 1993, 2001 Ingres Corporation
*/
#ifndef    COMMON_INCLUDE
#define                 COMMON_INCLUDE  0
#include <gl.h>
/**
** Name: IICOMMON.H - Types and names used by ingres both DBMS and FEs
**
** Description:
**	Types and names used by ingres, both DBMS and FEs
**	New types being defined in this file should be prefixed with "ii"
**
** History:
**      14-feb-93 (ed)
**          written
**      05-Apr-1993 (fred)
**  	    Added datatype id's for the byte, byte varying, and long
**          byte datatyes.
**      13-Jul-93 (fredv)
**          Extend my 6.4 change to bug 5839 to all platforms.
**          A similar problem was logged as bug b43804 on hp9000/800 with
**          the ELOT437 character set and was reproduced on Sun4 and DEC
**          Ultrix, but it worked on the RS6000. In general, whatever platforms
**          (except RS6000) use the character sets (elot437, ibmpc437, ibmpc850
**          kanjieuc, shiftjis, slav852) that have 0200, 0201, 0202, 0203 as
**          real characters will run into the same problem.
**          This change has been used on RS6000 for 2 years and was tested on
**          HP9000/800 with the most recent 6.4/04/00 MR code.
**	    This changed is also tested in 6.5 for three integrations.
**	30-Jul-93 (larrym)
**	    Added DB_XA_MAXGTRIDSIZE and DB_XA_MAXBQUALSIZE
**	17-aug-93 (andre)
**	    defined DB_MAX_DELIMID
**	24-aug-93 (ed)
**	    moved DB_MAX_HIST_LENGTH from dbdbms.h for adf
**	    moved DB_EVDATA_MAX from dbdbms.h for w4gl
**      22-Sep-93 (iyer)
**          Add a new extended DB_XA_EXTD_DIS_TRAN_ID struct. This
**          struct now includes the original DB_XA_DIS_TRAN_ID struct and
**          two additional members: branch_seqnum and branch_flag. In the
**          struct DB_DIS_TRAN_ID which defines a union of the two types
**          distributed transaction IDs, the member of type DB_XA_DIS_TRAN_ID
**          has been substituted with the extended tran id i.e. 
**          DB_XA_EXTD_DIS_TRAN_ID. This change has also been reflected in
**          in the DB_DIS_TRAN_ID_EQL_MACRO. 
**	31-mar-1995 (jenjo02)
**	    Added DB_NDMF to list of DB_LANG values for API project
**	10-jan-96 (emmag)
**	    Integration of JASMINE changes.
**	17-jan-1996 (toumi01; from 1.1 axp_osf port) (schte01)
**	    Changed DB_XA_DIS_TRAN_ID to reflect xid_t in xa.h (X/Open
**	    standard).	Changed first 3 fields from i4 to long.
**  	16-mar-1996 (angusm)
**          bug 48458: need MULTINATIONAL date format with 4-digit year.
**          Add new setting "MULTINATIONAL4".
**	18-mar-1997 (rosga02)
**          Integrated changes for SEVMS
**          18-jan-1996 (harch02)
**            macro EQUAL_SECID_EMPTY was missing some brackets needed for 74213
**	7-jun-96 (stephenb)
**	    Add replicated field to DB_ATTS
**      23-jul-1996 (ramra01 for bryanp)
**          Add new DB_ATTS fields for alter table support.
**	3-jun-97 (stephenb)
**	    Add rep_key_seq field to DB_ATTS.
**	05-dec-1997 (shero03)
**	    Add URS as a new facility
**	25-Aug-1997 (marol01)
**	    Add ICE as a new facility
**	24-May-1999 (shero03)
**	    Fixed maximum facitilies.
**	24-Jun-1999 (shero03)
**	    Add ISO4.
**      16-Mar-99 (hanal04)
**          Add constants for page sizes. b95887.
**	15-Dec-1999 (jenjo02)
**	    Added XA_XID_GTRID_EQL_MACRO
**      06-Jun-2000 (fanra01)
**          Bug 101345
**          Add new query language type.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Jan-2001 (gupsh01)
**	    Added new data type, for unicode, nchar
**	    and nvarchar. Also added the type DB_NQTXT_TYPE, 
**	    to denote query strings that are in Unicode format.
**	26-feb-2001 (somsa01)
**	    Increased DB_MAXSTRING and DB_GW4_MAXSTRING to 32,000, and
**	    changed DB_GW3_MAXTUP to be 32,002.
**	01-Mar-2001 (gupsh01)
**	    Added constants for the unicode data type.
**      09-Mar-2001 (gupsh01)
**          Corrected the value for constant DB_ELMSZ.
**	21-Dec-2001 (gordy)
**	    Added macros for assignment of GCA_COL_ATT and DB_DATA_VALUE.
**	07-Jan-2002 (toumi01)
**	    Changed DB_MAX_COLS from 300 to 1024.
**       2-Sep-2003 (hanal04) Bug 110825 INGCBT487.
**          Prevent truncation of error messages. Currently E_US100D is only
**          tuncated by the front end, once the timestamp is added. Whilst
**          we are bumping ER_MAX_LEN we'll also bump DB_ERR_SIZE.
**	25-Jan-2004 (schka24)
**	    Define a handy int-to-name translator struct.
**      18-feb-2004 (stial01)
**          Redefine attribute offset,length,key_offset as i4.
**      23-dec-2004 (srisu02)
**          Removed DB_MAXVARS from dbdbms.h and added in iicommon.h
**          as it is being referenced in front!misc!hdr xf.qsh
**	2-Dec-2005 (kschendel)
**	    Define an analogue of ME_ALIGN_MACRO that works strictly on
**	    sizes;  ME_ALIGN_MACRO works, but some compilers issue warnings.
**	2-Jun-2006 (bonro02)
**	    Include <gl.h> for GL_MAXNAME used in this header.
**	25-Aug-2006 (jonj)
**	    Added DB_FILE_MAX define of 12 bytes.
**      16-oct-2006 (stial01)
**          Added locator datatypes.
**	13-Feb-2007 (kschendel)
**	    Added volatile_read macro.
**	2-Mar-2007 (kschendel) b122041
**	    I should have added a generic align-to in dec 2005, do it now.
**      20-jul-2007 (stial01)
**          unicode locators class should be NCHAR in datatypes table (b118793)
**	29-jul-2008 (gupsh01)
**	    Increase the maximumn allowable character length of nvarchar
**	    types to 16000. 
**      10-sep-2008 (gupsh01)
**          Added utf8 expansion factors
**    21-Jan-2009 (horda03) Bug 121519
**        Due to the way filenames are constructed for table IDs, the range
**        of possible table_id's (for permanent tables) is 0 to 0x4643C824. 
**        By increasing the permitted table range, DB_TAB_MASK is just a mask
**        now. DB_TAB_ID_MAX is the max table id. Also, added definitions 
**        for the min/max temp table IDs too. See dm2f.c for more details on
**        the max table id.
**      28-jan-2009 (stial01)
**          Added DB_IITYPE_LEN, DB_IISTRUCT_LEN
**      06-oct-2009 (joea)
**          Change DB_DT_ID_MACRO to provide SQL and QUEL precedence values for
**          BOOLEAN.  Add db_booltype member to DB_ANYTYPE union and defines
**          DB_FALSE/DB_TRUE.  Add defines for DB_DEF_ID_FALSE/TRUE.  Add
**          comment on historical conflict between CPY_DUMMY_TYPE and
**          -DB_BOO_TYPE, and change value of DB_BOO_TYPE to 38.
**      21-oct-2009 (joea)
**          Add DB_BOO_LEN.
**	12-Nov-2009 (kschendel) SIR 122882
**	    Move database version definitions here from dudbms.qsh.
**      22-Feb-2010 (maspa05) 123293
**        added SVR_CLASS_MAXNAME for the size of a server_class name, used
**        to be hard-coded to 24
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**     22-apr-2010 (stial01)
**          Use DB_EXTFMT_SIZE for register table newcolname IS 'ext_format'
**/

#define                  P2K		 2048
#define                  P4K		 4096
#define                  P8K		 8192
#define                 P16K		16384
#define                 P32K		32768
#define                 P64K		65536


/*
**	Maxmimum length of a name within the DBMS, with few exceptions.
*/
#define    DB_MAXVARS		128 /* Max vars in a single query */

#define	   DB_MAXNAME		GL_MAXNAME
#define	   DB_EXTFMT_SIZE	64 /* IMA iigw07_attribute.classid  */
				   /* SXA iigw06_attribute.auditname */
				   /* register table colname IS 'ext_format' */
				   /* a reserved name identifying metadata */

#define	   DB_OBJ_MAXNAME	DB_MAXNAME /* object name (can be table) */
#define	   DB_TAB_MAXNAME	DB_MAXNAME /* table name */
#define	   DB_ATT_MAXNAME	DB_MAXNAME /* column name */
#define    DB_DBP_MAXNAME	DB_MAXNAME /* procedure name */
#define	   DB_PARM_MAXNAME	DB_MAXNAME /* procedure param name */
#define	   DB_RULE_MAXNAME	DB_MAXNAME /* rule name */
#define	   DB_CONS_MAXNAME	DB_MAXNAME /* constraint name */
#define	   DB_SEQ_MAXNAME	DB_MAXNAME /* sequence name */

/* collation name 64, big enough and still works with dbmsinfo('collation') */
#define	   DB_COLLATION_MAXNAME	64  /* collation name */

/* Owner/schema names will remain 32 */
#define	   DB_OWN_MAXNAME	32  /* owner name */
#define	   DB_SCHEMA_MAXNAME	32  /* schema name, must be username */

/* The following will remain at 32 */
#define    DB_OLDMAXNAME_32	32
#define	   DB_DB_MAXNAME	32  /* dbname,limited by DI_FILENAME_MAX*/
#define    DB_NODE_MAXNAME	32  /* node name */
#define	   DB_LOC_MAXNAME	32  /* ingres location name */
#define	   DB_EVENT_MAXNAME	32  /* event name */
#define	   DB_ALARM_MAXNAME	DB_EVENT_MAXNAME  /* alarm name */

/* The following are not names */
#define    DB_TYPE_MAXLEN	32  /* various 'types' */
				    /* datatypes, e.g. 'integer' */
				    /* privileges (same as SXA_MAX_PRIV_LEN) */
				    /* dbms e.g. 'ingres' */
				    /* should be same as ADF_MAXNAME */
#define    DB_CAP_MAXLEN	32  /* cap_capability in iidbcapabilities */
#define    DB_CAPVAL_MAXLEN	32  /* cap_value in iidbcapabilities */
#define	   DB_DATE_OUTLENGTH	25  /* date output length */ 

#define			DB_IITYPE_LEN	32 /* as returned by iitypename() */
#define			DB_IISTRUCT_LEN 16 /* as returned by iistructure() */

#define                 DB_GW1_MAXNAME		DB_MAXNAME  /* for gateways */
#define                 DB_GW1_MAXNAME_32	32  /* for scsqncr.c */

#define                 DB_TRAN_MAXNAME  64 /* for distributed transaction id */

#define			DB_XA_MAXGTRIDSIZE	64 /* for XA's transaction id */
#define			DB_XA_MAXBQUALSIZE	64
#define                 DB_XA_XIDDATASIZE 	DB_XA_MAXGTRIDSIZE + \
						DB_XA_MAXBQUALSIZE
					    /* 
					    ** max len of a delimited identifier
					    ** it must be big enough to 
					    ** accomodate DB_MAXNAME "s escaped
					    ** with "s + the opening and 
					    ** closing " (admittedly a very 
					    ** worst case scenario)
					    */
#define			DB_MAX_DELIMID ((DB_MAXNAME << 1) + 2)

#define                 SVR_CLASS_MAXNAME     24 /* length of a server_class */

/*
** Constant: DB_EVDATA_MAX - Maximum user text data associated with an event.
**	     This constant is documented so do not change without updating
** the documentation.
*/
#define	DB_EVDATA_MAX		256

/* typedef buffers for name + EOS */
typedef char	DB_DB_STR[DB_DB_MAXNAME + 1];	     /* database name + EOS */
typedef char	DB_LOC_STR[DB_LOC_MAXNAME + 1];	     /* location name + EOS */
typedef char	DB_OWN_STR[DB_OWN_MAXNAME + 1];	     /* owner name + EOS */
typedef char	DB_TAB_STR[DB_TAB_MAXNAME + 1];	     /* table name + EOS */
typedef char	DB_OBJ_STR[DB_MAXNAME + 1];	     /* object name + EOS */
typedef char	DB_ATT_STR[DB_ATT_MAXNAME + 1];	     /* column name + EOS */
typedef char	DB_DBP_STR[DB_DBP_MAXNAME + 1];	     /* proc name + EOS */
typedef char	DB_PARM_STR[DB_PARM_MAXNAME + 1];    /* proc parm + EOS */
typedef char	DB_NODE_STR[DB_NODE_MAXNAME + 1];    /* node name + EOS */
typedef char	DB_DELIM_STR[DB_MAX_DELIMID + 1];    /* delim name + EOS */
typedef char    DB_COLLATION_STR[DB_COLLATION_MAXNAME + 1];    /* collation name + EOS */

typedef char	DB_CAP_STR[DB_CAP_MAXLEN + 1];       /* cap + EOS */
typedef char	DB_CAPVAL_STR[DB_CAPVAL_MAXLEN + 1]; /* capval + EOS */
typedef char	DB_TYPE_STR[DB_TYPE_MAXLEN + 1];     /* datetype + EOS */
typedef char	DB_DATE_STR[DB_DATE_OUTLENGTH + 1];    /* date + EOS */


/* Alignment macros that aren't machine dependent */

#define			DB_ALIGN_SZ  sizeof(ALIGN_RESTRICT)

/* DB_ALIGN_MACRO(size) is size rounded up to DB_ALIGN_SZ, ie
** sizeof ALIGN_RESTRICT.  We assume that DB_ALIGN_SZ is a power of 2.
*/

#define DB_ALIGN_MACRO(s) ( ((s) + (DB_ALIGN_SZ-1)) & (~(DB_ALIGN_SZ-1)) )

/* A similar numeric-align but to a user specified alignment
** rather than assuming just align-restrict.
*/
#define DB_ALIGNTO_MACRO(s,a) ( ((s) + ((a)-1)) & (~((a)-1)) )

/* Un-mutexed read of a volatile object into some variable.
** Note that this is equivalent to a STATEMENT, not an EXPRESSION.
** This is more appropriate for a compat.h placement, but since it's
** not platform specific I'll put it here.
** Usage: VOLATILE_ASSIGN_MACRO(thing, result_var)
** (notice the order, to match struct-assign-macro.)
*/

#define VOLATILE_ASSIGN_MACRO(thing, var) \
    do { (var) = (thing); } while ( (var) != (thing) );


/* Database version definitions.
** This version is found in dbcmptlvl, in the iidatabase row.
** Originally the versions were 4-character strings, such as "6.0a" or
** "10.0".  The problem with using strings is that as the version number
** grows, 4 characters is not enough.  In addition, it's harder to compare
** strings (e.g. "10.0" is actually less than the older version strings.)
** So, starting with 10.0, the version changes to a u_i4, using the
** (decimal) format xxxyyzz (xxx = major version, yy = minor version,
** xx = build / patch / service pack).
**
** Note that dbcmptlvl DOES NOT need to change with every minor release
** or service pack.  It only needs to change when something in the catalogs
** changes, ie upgradedb has to be run.
*/

/* Old-style versions. */
#define		DU_0DBV60		CV_C_CONST_MACRO('6','.','0',' ')
#define	        DU_1DBV61A		CV_C_CONST_MACRO('6','.','1','a')
#define		DU_2DBV62		CV_C_CONST_MACRO('6','.','2',' ')
#define		DU_3DBV63		CV_C_CONST_MACRO('6','.','3',' ')
#define		DU_4DBV70		CV_C_CONST_MACRO('7','.','0',' ')
#define		DU_5DBV634		CV_C_CONST_MACRO('6','.','3','4')
#define		DU_6DBV80		CV_C_CONST_MACRO('8','.','0',' ')
#define		DU_6DBV85		CV_C_CONST_MACRO('8','.','5',' ')
#define		DU_6DBV86		CV_C_CONST_MACRO('8','.','6',' ')
#define		DU_6DBV865		CV_C_CONST_MACRO('9','.','0',' ')
#define		DU_6DBV902		CV_C_CONST_MACRO('9','.','0','2')
#define		DU_6DBV904		CV_C_CONST_MACRO('9','.','0','4')
#define		DU_6DBV910		CV_C_CONST_MACRO('9','.','1','0')
#define		DU_6DBV920		CV_C_CONST_MACRO('9','.','2','0')
#define		DU_6DBV930		CV_C_CONST_MACRO('9','.','3','0')

/* This one has never seen the light of release, but it can exist on
** development databases built against 10.0 main.
*/
#define		DU_6DBV1000		CV_C_CONST_MACRO('1','0','.','0')

/* All the old-style versions taken as a u_i4 are larger than this: */
#define		DU_OLD_DBCMPTLVL	0x20000000

/* Make a new version number: */
#define		DU_MAKE_VERSION(x,y,z)	(x*10000 + (y%100)*100 + (z%100))

/* New style versions. */
#define		DU_DBV1000		DU_MAKE_VERSION(10,0,0)
#define		DU_CUR_DBCMPTLVL	DU_DBV1000

/*}
** Name: DB_DATE - Buffer to hold an INGRES date.
**
** Description:
**      This buffer can only be processed by ADF.  The size of the buffer
**	is available outside of ADF to make dates more usable by our own
**	internal software.  Whenever used, this structure should always be
**	on an aligned boundary.
**
** History:
**	22-mar-89 (neil)
**          Written for Terminator (rules).
*/
# define	DB_DTE_LEN	12

typedef struct _DB_DATE
{
    char	db_date[DB_DTE_LEN];
} DB_DATE;

/*
**      The error statuses that are used by the entire database manager.
**	These are constrained by the rules that
**	    1) If the operation worked, the value MUST be even; and
**	    2) if code A is less severe than code B, than A < B
*/

#define                 E_DB_OK         0   /* operation succeeded */
#define                 E_DB_INFO       2   /* succeed, info msg returned */
#define                 E_DB_WARN       4   /* succeeded, warn msg returned */
#define                 E_DB_ERROR      5   /* failed due to caller error */
#define			E_DB_SEVERE	7   /* failed, session fatal */
#define                 E_DB_FATAL      9   /* failed, server fatal */

#define			DB_SUCCESS_MACRO(status) (((status) & 1) == 0)
#define			DB_FAILURE_MACRO(status) ((status) & 1)

#define			DB_STATUS	i4 /* type returned by entry points */

#define			DB_ERR_SIZE	1500 /* max length of an error msg */


/*}
** Name: DB_ERRTYPE - used for procedure passing of error codes
**
** Description:
**      Defines error code type used for interprocedure passing of error codes
**      within system.
**
** History:
**      11-apr-88 (seputis)
**          initial creation
[@history_template@]...
*/
typedef i4 DB_ERRTYPE;

/*}
** Name: DB_ERROR - The type of all errors for entry control blocks
**
** Description:
**      This structure defines the structure to be used in all control blocks
**      for returning errors from one facility to another.  This structure
**      contains four (4) members:  
**
**	o err_code will contain the code which 
**        identifies the error to the calling facility.  
**	o err_data can contain other information which may be useful to the 
**	  calling facility or to the calling facility's programmer.
**	o err_file points to the source filename (__FILE__) where the 
**	  error occurred or was noted.
**	o err_line is the source line number (__LINE__) within that file.
**
** History:
**     09-Dec-1985 (fred)
**          Created.
**	19-Jun-2008 (jonj)
**	    Sir 120874: Add err_file, err_line, CLRDBERR(),
**	    SETDBERR() macros to reveal error source.
[@history_line@]...
*/
typedef struct _DB_ERROR
{
    DB_ERRTYPE	err_code;	    /* the error being returned */
	/* # defines are defined by each facility */
    i4		err_data;	    /* defined on a per facility per error basis */
    PTR		err_file;	    /* Source file name where error thrown */
    i4		err_line;	    /* Source line number within that file */
}   DB_ERROR;

/* Initialize a DB_ERROR structure */
#define	CLRDBERR(e) \
    (e)->err_code = (e)->err_data = (e)->err_line = 0, \
    (e)->err_file = NULL

/* Value a DB_ERROR structure */
#define	SETDBERR(e, d, c) \
	(e)->err_line = __LINE__, \
	(e)->err_file = __FILE__, \
        (e)->err_code = (DB_ERRTYPE)c, \
        (e)->err_data = (i4)d


#define                 DB_MAX_COLS     1024	/* Max # of cols in table   */
#define                 DB_GW2_MAX_COLS 1024	/* ......... for gateways */
#define			DB_MAXTUP	2008	/* Maximum size of a tuple */

#define			DB_MAXTUP_DOC	2000	/* Max documented tuple size */

#define			DB_GW3_MAXTUP	32002	/* .......... for gateways */
		    /*                  \___/
		    **                    |
		    ** Note that this   --+   is large enough to hold a
		    ** VARCHAR(32000) ... 32000 + DB_CNTSIZE.
		    */
#define                 DB_AREA_MAX     128     /* Max # bytes in AREA */
#define                 DB_FILE_MAX     12      /* Max # bytes in a filename */


/*}
** Name: 
**
** Description:
**	The set of implementation defined Collation IDs
**	NOTE: these must be consistent with the "collname_array[]" in
**	pslsgram.yi.
**
** History:
**	9-dec-04 (inkdo01)
**	    Added for (limited) column level collation support.
**	26-apr-2007 (dougi)
**	    Added DB_UNICODE_FRENCH_COLL for UTF8 project.
**	11-May-2009 (kiria01) b122051
**	    Converted to table macro
*/
#define DB_COLL_MACRO \
_DEFINE(DB_UNSET_COLL,		       -1, "unset")\
_DEFINE(DB_NOCOLLATION,			0, "none")\
_DEFINE(DB_UNICODE_COLL,		1, "unicode")\
_DEFINE(DB_UNICODE_CASEINSENSITIVE_COLL,2, "unicode_case_insensitive")\
_DEFINE(DB_SQLCHAR_COLL,		3, "sql_character")\
_DEFINE(DB_MULTI_COLL,			4, "multi")\
_DEFINE(DB_SPANISH_COLL,		5, "spanish")\
_DEFINE(DB_UNICODE_FRENCH_COLL,		6, "unicode_french")\
_DEFINEEND

#define _DEFINE(n,v,t) n=v,
#define _DEFINEEND
typedef enum _DB_COLL_ID { DB_COLL_MACRO DB_COLL_LIMIT } DB_COLL_ID;
#undef _DEFINEEND
#undef _DEFINE

#define DB_COLL_UNSET(dbv) ((u_i2)(dbv).db_collID >= DB_COLL_LIMIT)



/*}
** Name: DB_TRAN_ID - Transaction id.
**
** Description:
**      This structure defines a transaction id.
**
** History:
**     28-oct-85 (jeff)
**          written
*/
typedef struct _DB_TRAN_ID
{
    u_i4            db_high_tran;       /* High part of transaction id */
    u_i4            db_low_tran;        /* Low part of transaction id  */
}   DB_TRAN_ID;
/*}
** Name: DB_INGRES_DIS_TRAN_ID - Distributed Transaction id for Ingres.
**
** Description:
**      This structure defines a distributed transaction id for Ingres.
**
** History:
**     22-Sep-92 (nandak)
**          written
*/
typedef struct _DB_INGRES_DIS_TRAN_ID
{
    DB_TRAN_ID	    db_tran_id;		  
    char	    db_tran_name[DB_TRAN_MAXNAME];	
}   DB_INGRES_DIS_TRAN_ID;
/*}
** Name: DB_XA_DIS_TRAN_ID - Distributed Transaction id for XA.
**
** Description:
**      This structure defines a distributed transaction id for XA.
**
** History:
**     22-Sep-92 (nandak)
**          written
**	   16-jan-96 (schte01)
**	    Changed DB_XA_DIS_TRAN_ID to reflect xid_t in xa.h (X/Open standard).
**      Changed first 3 fields from i4 to long.
**
**      12-Mar-2004 (hanal04) Bug 111923 INGSRV2751
**          Modify all XA xid fields to i4 (int for supplied files)
**          to ensure consistent use across platforms.
*/
typedef struct _DB_XA_DIS_TRAN_ID
{
    i4              formatID;             /* Format ID-set by TP mon */
    i4              gtrid_length;         /* Global transaction ID   */
                                          /* length                  */
    i4              bqual_length;         /* Branch Qualifier length */
    char            data[DB_XA_XIDDATASIZE];
                                          /* Data has the gtrid and  */
                                          /* bqual concatenated      */
                                          /* If gtrid_length is 4 and*/
                                          /* bqual_length is 4, the  */
                                          /* 1st 8 bytes in data will*/
                                          /* contain gtrid and bqual.*/
}   DB_XA_DIS_TRAN_ID;

/*}
** Name: DB_XA_EXTD_DIS_TRAN_ID - Extended Distributed Transaction id for XA.
**
** Description:
**      This structure defines an extended distributed transaction id for XA.
**      In addition to the XA XID passed by the TP monitor, to differentiate
**      the local processes working on behalf of the same XID two additional
**      fields have been added. The first member called the branch_seqnum
**      holds a unique sequence number assigned to an AS process within a
**      TMS group. The second member called the branch_flag holds information
**      on the status of the what could be termed as the local 2PC as 
**      opposed to the Global 2PC for the entire transaction.  
**
** History:
**     22-Sep-93 (iyer)
**          written
*/
typedef struct _DB_XA_EXTD_DIS_TRAN_ID
{
    DB_XA_DIS_TRAN_ID    db_xa_dis_tran_id; /* XA Distributed tran id   */
    i4                   branch_seqnum;     /* Branch local tran seq num */
    i4                   branch_flag;       /* Local 2PC transaction flag */
#define DB_XA_EXTD_BRANCH_FLAG_NOOP  0x0000 /* For ENCINA: TUX middle trans */
#define DB_XA_EXTD_BRANCH_FLAG_FIRST 0x0001 /* For FIRST TUX "AS" prepared */
#define DB_XA_EXTD_BRANCH_FLAG_LAST  0x0002 /* For LAST TUX "AS" prepared */ 
#define DB_XA_EXTD_BRANCH_FLAG_2PC   0x0004 /* Is TUX tran  2PC */
#define DB_XA_EXTD_BRANCH_FLAG_1PC   0x0008 /* Is TUX tran  1PC */

}   DB_XA_EXTD_DIS_TRAN_ID;

/*}
** Name: DB_DIS_TRAN_ID - Distributed Transaction id.
**
** Description:
**      This structure defines a distributed transaction id.
**
** History:
**     22-Sep-92 (nandak)
**          written
**     22-Sep-93 (iyer)
**          Modified to replace DB_XA_DIS_TRAN_ID with DB_XA_EXTD_DIS_TRAN_ID.
**          The union will have the extended structure which will be
**          propagated throughout the back-end.  
*/
typedef struct _DB_DIS_TRAN_ID
{
    i4         db_dis_tran_id_type; /* Type of transaction ID     */
#define  DB_INGRES_DIS_TRAN_ID_TYPE  1   /* Ingres's distributed XID   */
#define  DB_XA_DIS_TRAN_ID_TYPE      2   /* XA's distributed XID       */
    union
    {
        DB_INGRES_DIS_TRAN_ID   db_ingres_dis_tran_id;
        DB_XA_EXTD_DIS_TRAN_ID  db_xa_extd_dis_tran_id;
    }   db_dis_tran_id;
} DB_DIS_TRAN_ID;

/*}
** Name: DB_DIS_TRAN_ID_EQL_MACRO 
**
** Description:
**      This macro checks for the equality of two transaction ids.
**
** History:
**     22-Sep-92 (nandak)
**          Created.
**     22-Sep-93 (iyer)
**          Modified to change qualifers for all DB_XA_DIS_TRAN_ID members
**          which is now part of the extended structure DB_XA_EXTD_DIS_TRAN_ID.
**          Added additional lines to check for equality for 
**          branch_seqnum and branch_flag.
*/
#define DB_DIS_TRAN_ID_EQL_MACRO(a,b)  \
 ( (((a).db_dis_tran_id_type == DB_INGRES_DIS_TRAN_ID_TYPE && \
        (a).db_dis_tran_id.db_ingres_dis_tran_id.db_tran_id.db_high_tran ==  \
        (b).db_dis_tran_id.db_ingres_dis_tran_id.db_tran_id.db_high_tran  && \
        (a).db_dis_tran_id.db_ingres_dis_tran_id.db_tran_id.db_low_tran ==   \
        (b).db_dis_tran_id.db_ingres_dis_tran_id.db_tran_id.db_low_tran) ||  \
 ((a).db_dis_tran_id_type == DB_XA_DIS_TRAN_ID_TYPE &&    \
  (a).db_dis_tran_id.db_xa_extd_dis_tran_id.branch_seqnum == \
  (b).db_dis_tran_id.db_xa_extd_dis_tran_id.branch_seqnum  && \
  (a).db_dis_tran_id.db_xa_extd_dis_tran_id.branch_flag == \
  (b).db_dis_tran_id.db_xa_extd_dis_tran_id.branch_flag  && \
  (a).db_dis_tran_id.db_xa_extd_dis_tran_id.db_xa_dis_tran_id.formatID == \
  (b).db_dis_tran_id.db_xa_extd_dis_tran_id.db_xa_dis_tran_id.formatID  && \
  (a).db_dis_tran_id.db_xa_extd_dis_tran_id.db_xa_dis_tran_id.gtrid_length == \
  (b).db_dis_tran_id.db_xa_extd_dis_tran_id.db_xa_dis_tran_id.gtrid_length && \
  (a).db_dis_tran_id.db_xa_extd_dis_tran_id.db_xa_dis_tran_id.bqual_length == \
  (b).db_dis_tran_id.db_xa_extd_dis_tran_id.db_xa_dis_tran_id.bqual_length && \
 !MEcmp((a).db_dis_tran_id.db_xa_extd_dis_tran_id.db_xa_dis_tran_id.data, \
        (b).db_dis_tran_id.db_xa_extd_dis_tran_id.db_xa_dis_tran_id.data, \
   (a).db_dis_tran_id.db_xa_extd_dis_tran_id.db_xa_dis_tran_id.gtrid_length + \
   (a).db_dis_tran_id.db_xa_extd_dis_tran_id.db_xa_dis_tran_id.bqual_length)))?1 :0)

/*}
** Name: XA_XID_GTRID_EQL_MACRO 
**
** Description:
**      This macro checks for the equality of two XA GTRIDs.
**
** 	To be equivalent, both must match on formatID 
**	and GlobalTRansactionIDentifier.
**
** History:
**	15-Dec-1999 (jenjo02)
**	    Created for shared LG transactions.
*/
#define XA_XID_GTRID_EQL_MACRO(a,b)  \
  ( ((a).db_dis_tran_id_type == DB_XA_DIS_TRAN_ID_TYPE && \
     (b).db_dis_tran_id_type == DB_XA_DIS_TRAN_ID_TYPE && \
     (a).db_dis_tran_id.db_xa_extd_dis_tran_id.db_xa_dis_tran_id.formatID == \
     (b).db_dis_tran_id.db_xa_extd_dis_tran_id.db_xa_dis_tran_id.formatID  && \
     (a).db_dis_tran_id.db_xa_extd_dis_tran_id.db_xa_dis_tran_id.gtrid_length == \
     (b).db_dis_tran_id.db_xa_extd_dis_tran_id.db_xa_dis_tran_id.gtrid_length && \
    !MEcmp((a).db_dis_tran_id.db_xa_extd_dis_tran_id.db_xa_dis_tran_id.data, \
           (b).db_dis_tran_id.db_xa_extd_dis_tran_id.db_xa_dis_tran_id.data, \
           (a).db_dis_tran_id.db_xa_extd_dis_tran_id.db_xa_dis_tran_id.gtrid_length) \
    ) ? TRUE : FALSE )

/*}
** Name: DB_LANG - Query language id.
**
** Description:
**	This defines a query language id, which can stand for any of the
**	supported query languages.
**
** History:
**      09-oct-85 (jeff)
**          written
**	02-jan-86 (jeff)
**	    moved from parser header to dbms.h
**	31-mar-1995 (jenjo02)
**	    Added DB_NDMF to list of DB_LANG values for API project
**	10-jan-96 (emmag)
**	    Add DB_ODQL to list of DB_LANG values for JASMINE.   
*/
typedef i4  DB_LANG;
#define			DB_NOLANG	(DB_LANG) 0000	/* Qry lang not spec */
#define			DB_QUEL		(DB_LANG) 0001	/* Qry lang is QUEL */
#define			DB_SQL		(DB_LANG) 0002	/* Qry lang is SQL */
#define			DB_NDMF		(DB_LANG) 0003  /* NativeDMF */
#define                 DB_ODQL         (DB_LANG) 0004  /* ODB-II */
#define                 DB_ICE          (DB_LANG) 0005  /* ICE */

/*}
** Name: DB_OBJ_LOGKEY_INTERNAL - INGRES internal rep. of object_logical_keys.
**
** Description:
**	This typedef describes the INGRES internal format of a logical_key.
**
**	These logical_keys are 16 bytes. Values assigned to a table_logical_key
**	within a tuple are guaranteed to be unique among values assigned to
**	table_logical_key's in all tuples within an installation.
**
**	Values assigned to object_logical_key's and are created from the 8 byte 
**	unique id maintained in the relation relation (and the table control
**	block), the database id, and the relation id.  The high and low order 4
**	bytes of the unique id are taken straight from the low and high order
**	4 bytes of the unique id in the relation relation.  The relation id
**	is taken from the relation identifier in the relation relation.  The
**	database identifier is taken from the iidbdb's db_id.
**
**	The members of this structure have been placed in an order such that
**	the low and high order id's will most significantly affect sorting.
**	Also by placing them in this order the low and high id's can be used
**	as the 8 byte histogram value for the optimizer.
**
**	It is assumed that there is no padding done in between the
**	members of this structure as this structure is used to read and write
**	logical_key's to/from tuple buffers.
**
**
** History:
**	14-mar-89 (mikem)
**	    created.
*/
typedef struct _DB_OBJ_LOGKEY_INTERNAL	DB_OBJ_LOGKEY_INTERNAL;

struct _DB_OBJ_LOGKEY_INTERNAL
{
    u_i4	    olk_low_id;		/* low order i4 of 8 byte id */
    u_i4	    olk_high_id;	/* high order i4 of 8 byte id */
    u_i4	    olk_rel_id;		/* relation id from iirelation */
    u_i4	    olk_db_id;		/* database id from iidbdb */
};

/* length of a object_logical_key */

#define DB_LEN_OBJ_LOGKEY	16

/*}
** Name: DB_TAB_LOGKEY_INTERNAL - INGRES internal rep. of table_logical_keys.
**
** Description:
**	This typedef describes the internal format of a table_logical_key.
**
**	These logical_keys are 8 bytes.  Values assigned to a table_logical_key
**	within a tuple are guaranteed to be unique among values assigned to
**	table_logica_key's in all tuples within a relation.  These values are
**	not guaranteed unique across relations.
**
**	Values assigned to table_logical_key's are created from the 8 byte 
**	unique id maintained in the relation relation (and the table control
**	block).  The low order 4 bytes are taken straight from the low order
**	4 bytes of the unique id in the relation relation.  The high order
**	2 bytes are taken from the high order 4 bytes of the unique id in the
**	relation relation (the 2 low order bytes of the relation relation's
**	4 bytes are used).
**
**	The members of this structure have been placed in an order such that
**	the low and high order id's will most significantly affect sorting.
**	Also by placing them in this order the low and high id's can be used
**	as the 8 byte histogram value for the optimizer.
**
**	It is assumed that there is no padding done in between the
**	members of this structure as this structure is used to read and write
**	logical_key's to/from tuple buffers.
**
** History:
**      14-mar-89 (mikem)
**	    created.
**      17-may-89 (mikem)
**	    changed size of logical key to 8 bytes from 6 bytes.  This will
**	    make it easier to use in the system catalogues (no alignment 
**	    problems).
*/
typedef struct _DB_TAB_LOGKEY_INTERNAL	DB_TAB_LOGKEY_INTERNAL;

struct _DB_TAB_LOGKEY_INTERNAL
{
    u_i4	    tlk_low_id;		/* low order i4 of 8 byte id */
    u_i4	    tlk_high_id;	/* high order i2 of 8 byte id */
};

#define DB_LEN_TAB_LOGKEY	8

/*}
** Name: DB_DT_ID - Datatype id.
**
** Description:
**      All datatype ids to be used in INGRES must be declared as DB_DT_IDs.
**      Legal values for a DB_DT_ID are in the range 1-127, with certain numbers
**      excluded (see #defines below).  Furthermore, the negation of these IDs
**	are also legal, and represent the nullable versions of the corresponding
**	datatypes.  The limited range of 1-127 has to do with the method the
**      DBMS uses for determining which datatypes a given datatype can be
**      automatically converted (coerced) into.  That method uses a 128 bit long
**      bit-mask to represent some set of datatypes.
**
**	All DB_DATA_VALUEs and DB_EMBEDDED_DATAs will be "of" one of the
**	datatypes whose IDs are defined here.  Note that the semantics of the
**	data may differ depending on whether it is a DB_DATA_VALUE or a
**	DB_EMBEDDED_DATA.  For instance, take the datatype ID DB_CHR_TYPE:
**	When used as a DB_DATA_VALUE type, it limits the number of characters
**	to DB_CHAR_MAX, and limits the set of legal characters to the printable
**	ones.  However, when used as a DB_EMBEDDED_DATA type, these limitations
**	do not apply.
**
** History:
**      19-feb-86 (thurston)
**          Initial creation.
**      03-mar-86 (thurston)
**          Put the (DB_DT_ID) cast on the #define for DB_NODT to please lint.
**	12-mar-87 (thurston)
**	    Added DB_BOO_TYPE and changed the definition of DB_NULLBASE_TYPE.
**	25-apr-88 (thurston)
**	    Added DB_DEC_TYPE.
**	22-nov-89 (jrb)
**	    Added DB_ALL_TYPE.
**      05-Apr-1993 (fred)
**  	    Added DB_{BYTE,VBYTE,LBYTE}_TYPE's.
**	24-Jan-2001 (gupsh01)
**	    Added new data types for unicode, DB_NCHR_TYPE and DB_NVCHR_TYPE,
**	    DB_LNVCHR_TYPE and DB_NQTXT_TYPE.	
**	28-jun-2001 (gupsh01)
**	    Moved the define for UCS2 to compat.h .
**	25-nov-2003 (gupsh01)
**	    Added datatype DB_UTF8_TYPE for internal use.
**	20-apr-06 (dougi)
**	    Added the ANSI date/time types.
**	08-Mar-2007 (kiria01) b117892
**	    Move DB_DT_ID constants into a table macro for clarity
**	    and to enable construction of safe lookup tables and
**	    strings. See adirslv.c for examples or such lookups.
**	22-Mar-2007 (kiria01) b117894/b117895
**	    Split Quel and SQL precedences apart to limit destablising
**	    Quel and relegate TEXT to beyond char/varchar to reduce
**	    use of TEXT as an intermediate form.(b117894)
**	    Move C type precedence to beyond char/varchar to reduce
**	    its use as an intermediate (space removing) form.(b117895)
**	15-Oct-2007 (kiria01) b117790
**	    Adjust precedences to relegate money. This has involved
**	    moving numeric lower down and explicitly adding money into
**	    the table - it was previously 0. The precedence for long varbyte
**	    has also been corrected as otherwise it would be a candidate
**	    implicit coercion.
**	08-Mar-2008 (kiria01) b120043
**	    Having added further arithmetic operators for Dates it is
**	    necessary to assign a precedence to avoid biasing from FLT
**	    to DTE. Note that the remaining date types (apparently ANSI)
**	    remain unchanged as all the fancy work (even for ANSI) is
**	    done through DTE using the dtfamily support. The interval
**	    types however need to be biased too as otherwise restrictive
**	    coercions will occur instead of the general DTE forms.
**	12-Jun-2008 (kiria01) b120500
**	    Document change to UDT priority.
**	27-Oct-2008 (kiria01) SIR120473
**	    Added PAT datatype
**	20-Apr-2009 (kiria01) SIR121788
**	    Set the locator bits to be useful for mappint to their base types
*/

/*
** DB_DT_ID - defined as an i2 for storage but has associated enumerated
**	      constants in defined below as enum DB_DT_ID_enum =. These
**	      values are build through the table macro DB_DT_ID_MACRO
*/
typedef i2 DB_DT_ID;

/*
**        DB_NODT          0        Used to represent NO datatype.
**
**  Names of datatype ID codes that can exist in relations.  The first six of
**  these are based on 4.0 definitions in ingsym.h.
**        DB_INT_TYPE     30        "i"
**        DB_FLT_TYPE     31        "f"
**        DB_CHR_TYPE     32        "c"
**        DB_TXT_TYPE     37        "text"
**        DB_DTE_TYPE      3        "date"
**        DB_MNY_TYPE      5        "money"
**
**        DB_DEC_TYPE     10        "decimal"
**        DB_LOGKEY_TYPE  11        Type of a 16 byte logical key, known
**                                   to users as: "object_key"
**        DB_TABKEY_TYPE  12        Type of a 8 byte logical key - known
**                                   to users as: "table_key"
**        DB_BIT_TYPE     14        Bit (fixed)
**        DB_VBIT_TYPE    15        Bit varying(ansi)
**        DB_LBIT_TYPE    16        Long Bit Type
**        DB_CPN_TYPE     17        Coupon -- temporary
**        DB_CHA_TYPE     20        "char"
**        DB_VCH_TYPE     21        "varchar"
**        DB_LVCH_TYPE    22        "long varchar"
**        DB_BYTE_TYPE    23        "byte"
**        DB_VBYTE_TYPE   24        "byte varying"
**        DB_LBYTE_TYPE   25        "long byte"
**        DB_NCHR_TYPE    26        "unicode nchar"
**        DB_NVCHR_TYPE   27        "unicode nvarchar"
**        DB_LNVCHR_TYPE  28        "unicode long nvarchar"
**        DB_LNLOC_TYPE   29        "lnvchr locator"
**
** New types for SQL standard date/time **
**        DB_ADTE_TYPE     4        ANSI date type
**        DB_TMWO_TYPE     6        time without time zone
**        DB_TMW_TYPE      7        time with time zone
**        DB_TME_TYPE      8        Ingres time (local time zone)
**        DB_TSWO_TYPE     9        timestamp without time zone
**        DB_TSW_TYPE     18        timestamp with time zone
**        DB_TSTMP_TYPE   19        Ingres timestamp (local time zone)
**        DB_INYM_TYPE    33        interval (year-month)
**        DB_INDS_TYPE    34        interval (day-second)
**        DB_LBLOC_TYPE   35        lbyte locator
**        DB_LCLOC_TYPE   36        lvchar locator
**        DB_BOO_TYPE     38        "boolean"
**
** The following types are not stored in relations **
**
**
**        CPY_DUMMY_TYPE     -1        "Dummy type"
**                This "datatype" is only used in a COPY statement for a "dummy"
**                domain, e.g., dummy=d0.  CPY_DUMMY_TYPE is defined in copy.h
**                Historically, DB_BOO_TYPE was type 1 but was not stored in
**                relations.   Now DB_BOO_TYPE is type 38 and *is* stored.
**
**        DB_DIF_TYPE         2        "Different"
**                This datatype, DB_DIF_TYPE, is used by the sort compare routine
**                to manage unsortable datatypes.  If the sort compare routine sees
**                a datatype of type DB_DIF_TYPE, it merely returns that that
**                attribute is different.  The sorter is then expected to figure out
**                what to do.  This datatype was added initially for Large Object
**                support -- but may be used in other places where a datatype exists
**                which is not a valid candidate for sorting.
**
**
**        DB_ALL_TYPE        39        All datatypes!
**                This datatype is used as a "wildcard" when dealing with ADF function
**                instances: if an input to an ADF function instance has datatype
**                DB_ALL_TYPE, then it handle all datatypes; if the result type of an
**                ADF function instance has datatype DB_ALL_TYPE, then the result type
**                can be determined by calling adi_res_type().
**
**  Datatype IDs in the 40-49 range have been reserved for frontend use.
**  On occasion, some non-ADF backend facilities may use some of these, such
**  as longtext, for convenience.
**
**        DB_DYC_TYPE        40   dynamic char string (for frontends)
**                The DB_DYC_TYPE datatype is needed by the frontends (namely ABF) to
**                do dynamic allocation of character string data.  It is unusual in
**                that it comes in two parts:  The first is an 8 byte fixed size piece
**                that holds the # bytes allocated, the # bytes used, and a pointer to
**                this memory that holds the character data. 
**
**        DB_LTXT_TYPE        41  longtext" (for frontends)
**                The semantics of the DB_LTXT_TYPE are as follows:  The first 2 bytes
**                are treated as an u_i2 which holds the number of following bytes that
**                are relevant. The longtext can hold ANY 8-bit character, including
**                NULL.  This datatype will be used mostly by the frontend people to
**                format displays or input, and by the DBMS to represent the typeless
**                NULL value.  ADF will guarantee the existence of coercions from/into
**                longtext into/from all other datatypes, which makes this a very
**                convenient way to represent a typeless NULL value.
**
**        DB_QUE_TYPE        42   query type (for frontends)
**                This datatype will not actually be recognized by ADF.  However, no
**                recognizable datatype can have a datatype ID that onflicts with
**                this.  For some reason, the frontends need to reserve a datatype ID
**                for their own use, thus, this existence of this constant.
**
**        DB_DMY_TYPE        43   dummy type (for frontends)
**                The copy command allows the user to specify a field as being of type
**                d", which tells INGRES to ignore that field.  This datatype ID is
**                used to represent that. Note that it is different from DB_NODT, which
**                basically signifies an illegal datatype. 
**
**        DB_DBV_TYPE        44   data value type (for frontends)
**                This datatype ID is used when the .db_data (or .ed_data for embedded
**                values) points to a fully specified DB_DATA_VALUE.  The .db_length
**                ed_length) is to be ignored.  Note that this datatype ID has no
**                nullable" counterpart.
**
**        DB_OBJ_TYPE        45   object reference
**                This datatype will not be recognized by ADF.  However, no
**                recognizable datatype can have a datatype ID that conflicts with
**                this.  The datatype represents on object reference to a Sapphire
**                object.  The .db_data of this will point to a sapphire object
**                reference.
**
**        DB_HDLR_TYPE        46  FE Data Handler
**                This datatype will not be recognized by ADF.  However, no
**                recognizable datatype can have a datatype ID that conflicts with
**                this.  The datatype represents a host language application
**                DATAHANDLER for handling large objects.  The .db_data of this
**                datatype will point to an FE structure (IISQ_HDLR) which contains a
**                function pointer and a PTR argument.
**
**        DB_UTF8_TYPE        47  UTF8 data type
**                A formatted copy command to copy out nchar and nvarchar types
**                into/from a text file, required unicode data to be formatted 
**                in UTF8 translation format.
**
**
**  Datatype IDs in the 50-63 range may be used for various reasons.
**
**        DB_NULBASE_TYPE     50  base type for typeless NULL
**                This datatype ID exists as a help to code that needs to check the
**                base" datatype, and therefore masks out the nullability bit.
**
**        DB_QTXT_TYPE        51  Type for qry text when sent thru GCF
**
**        DB_TFLD_TYPE        52  Type for table field as described
**                                to user through Dynamic SQL.
**
**        DB_DEC_CHR_TYPE     53  Type for decimal literal value represented by a
**                                null-terminated string
**
**        DB_NQTXT_TYPE       54  Type for qry text containing a nchar
**                                or nvarchar text when send thru GCF
**
**/

/* Data classes */
#define DB_DC_ID_MACRO \
_DEFINE(NONE,    0        /* No class */)\
_DEFINE(ALL,     1        /* All */)\
_DEFINE(NUMB,    2        /* Numeric */)\
_DEFINE(BOOL,    3        /* Boolean */)\
_DEFINE(BIT,     4        /* Bit */)\
_DEFINE(BYTE,    5        /* Byte */)\
_DEFINE(CHAR,    6        /* Character */)\
_DEFINE(NCHAR,   7        /* Unicode */)\
_DEFINE(C,       8        /* Spaces ignored */)\
_DEFINE(DATE,    9        /* DATE */)\
_DEFINE(INTV,   10        /* Interval */)\
_DEFINEEND

/* Storage classes */
#define DB_SC_ID_MACRO \
_DEFINE(BASE,    0        /* Scalar */)\
_DEFINE(PAD,     1        /* Fixed and padded */)\
_DEFINE(VAR,     2        /* Varying */)\
_DEFINE(LV,      3        /* Long Varying */)\
_DEFINE(LOC,     4        /* Locator */)\
_DEFINEEND

/*
** Data types table
** Defined to keep datatype info together and allow building
** of safe lookup arrays.
**
** Data class is intended to pool together similar types that
** differ only in their length characteristics but where coersion
** should be a fairly neutral action.
**
** Storage class is intended to identify length charateristics
**
** Precedence is used by adi_resolve to determine coercion biases.
** Four columns are defined differing in the application of 'varchar'
** precedence over char and SQL over Quel. Basicly, the lower the
** prec, the more preferable.
**
**  The order of preference is:
**
**    Most Preferable
**           Quel      Rank     SQL
**       ============  ====  ==========
**    -    Abstract     1    Abstract - Managed in adirslv.c
**   /|\
**    |      Float      3      nchar
**    |     Decimal     4    nvarchar
**    |       Int       5      char
**    |  nchar & date+  6     varchar
**    |    nvarchar     7      float
**    |        C        8     decimal
**    |      Text       9       int
**    |      char       10   c & date+
**    |     varchar     11     text
**    |    longtext     12     money
**    |      byte       13   longtext
**    |     varbyte     14     byte
**    |                 15    varbyte
**    |  long nvarchar  16 long nvarchar
**    |  long varchar   17 long varchar
**    |  long varbyte   18 long varbyte
**    |     boolean     19    boolean
**    |       UDT       98      UDT   - Managed in adirslv.c
**    Least Preferable
**        DB_ALL_TYPE   99  DB_ALL_TYPE
**
** The philosophy adopted is to try promote actions that the user
** expects. If they have selected unicode then don't downgrade to
** char/c/text form. If quel, then bias coercions to C/Text but
** if SQL, bias towards char/varchar.
*/

/*                 Data Storage QuelPrec SQLPrec               */
/*      Name    ID Class Class  cha vch cha vch Comment        */
#define DB_DT_ID_MACRO \
_DEFINE(NODT,    0, NONE, BASE,  0,  0,  0,  0  /* Used to represent NO dt */)\
_DEFINE(1,       1, NONE, BASE,  0,  0,  0,  0  /* "dummy" COPY domain */)\
_DEFINE(DIF,     2, NONE, BASE,  0,  0,  0,  0  /* "Different" */)\
_DEFINE(DTE,     3, DATE, BASE,  6,  6, 10, 10  /* "date" */)\
_DEFINE(ADTE,    4, NONE, BASE,  0,  0,  0,  0  /* ANSI date type */)\
_DEFINE(MNY,     5, NUMB, BASE,  0,  0, 12, 12  /* "money" */)\
_DEFINE(TMWO,    6, NONE, BASE,  0,  0,  0,  0  /* time without time zone */)\
_DEFINE(TMW,     7, NONE, BASE,  0,  0,  0,  0  /* time with time zone */)\
_DEFINE(TME,     8, NONE, BASE,  0,  0,  0,  0  /* Ingres time (local time zone) */)\
_DEFINE(TSWO,    9, NONE, BASE,  0,  0,  0,  0  /* timestamp without time zone */)\
_DEFINE(DEC,    10, NUMB, BASE,  4,  4,  8,  8  /* "decimal" */)\
_DEFINE(LOGKEY, 11, BYTE, BASE,  0,  0,  0,  0  /* Type of a 16 byte object_key */)\
_DEFINE(TABKEY, 12, BYTE, BASE,  0,  0,  0,  0  /* Type of a 8 byte table_key  */)\
_DEFINE(PAT,    13, NONE, BASE,  0,  0,  0,  0  /* Type for compiled pattern */)\
_DEFINE(BIT,    14, BIT,  PAD,   0,  0,  0,  0  /* Bit (fixed) */)\
_DEFINE(VBIT,   15, BIT,  VAR,   0,  0,  0,  0  /* Bit varying(ansi)*/)\
_DEFINE(LBIT,   16, BIT,  LV,    0,  0,  0,  0  /* Long Bit Type */)\
_DEFINE(CPN,    17, BYTE, BASE,  0,  0,  0,  0  /* Coupon -- temporary */)\
_DEFINE(TSW,    18, NONE, BASE,  0,  0,  0,  0  /* timestamp with time zone */)\
_DEFINE(TSTMP,  19, NONE, BASE,  0,  0,  0,  0  /* Ingres timestamp (local time zone) */)\
_DEFINE(CHA,    20, CHAR, PAD,  10, 11,  5,  6  /* "char" */)\
_DEFINE(VCH,    21, CHAR, VAR,  11, 10,  6,  5  /* "varchar" */)\
_DEFINE(LVCH,   22, CHAR, LV,   17, 17, 17, 17  /* "long varchar" */)\
_DEFINE(BYTE,   23, BYTE, PAD,  13, 13, 14, 14  /* Byte */)\
_DEFINE(VBYTE,  24, BYTE, VAR,  14, 14, 15, 15  /* byte varying */)\
_DEFINE(LBYTE,  25, BYTE, LV,   18, 18, 18, 18  /* Long byte */)\
_DEFINE(NCHR,   26, NCHAR,PAD,   6,  7,  3,  4  /* unicode nchar */)\
_DEFINE(NVCHR,  27, NCHAR,VAR,   7,  6,  4,  3  /* unicode nvarchar */)\
_DEFINE(LNVCHR, 28, NCHAR,LV,   16, 16, 16, 16  /* unicode long nvarchar */)\
_DEFINE(LNLOC,  29, NCHAR,LOC,   0,  0,  0,  0  /* unicode locator */)\
_DEFINE(INT,    30, NUMB, BASE,  5,  5,  9,  9  /* "i" */)\
_DEFINE(FLT,    31, NUMB, BASE,  3,  3,  7,  7  /* "f" */)\
_DEFINE(CHR,    32, C,    VAR,   8,  8, 10, 10  /* "c" */)\
_DEFINE(INYM,   33, NONE, BASE,  6,  6, 10, 10  /* interval (year-month) */)\
_DEFINE(INDS,   34, NONE, BASE,  6,  6, 10, 10  /* interval (day-second) */)\
_DEFINE(LBLOC,  35, BYTE, LOC,   0,  0,  0,  0  /* lbyte locator */)\
_DEFINE(LCLOC,  36, CHAR, LOC,   0,  0,  0,  0  /* lvchar locator */)\
_DEFINE(TXT,    37, CHAR, VAR,   9,  9, 11, 11  /* "text" */)\
_DEFINE(BOO,    38, BOOL, BASE, 19, 19, 19, 19  /* "boolean" */)\
_DEFINE(ALL,    39, ALL,  BASE, 99, 99, 99, 99  /* All datatypes! */)\
_DEFINE(DYC,    40, CHAR, VAR,   0,  0,  0,  0  /* dynamic char string */)\
_DEFINE(LTXT,   41, CHAR, VAR,  12, 12, 13, 13  /* "longtext"      */)\
_DEFINE(QUE,    42, CHAR, PAD,   0,  0,  0,  0  /* query type      */)\
_DEFINE(DMY,    43, NONE, BASE,  0,  0,  0,  0  /* dummy type      */)\
_DEFINE(DBV,    44, ALL,  BASE,  0,  0,  0,  0  /* data value type */)\
_DEFINE(OBJ,    45, BYTE, BASE,  0,  0,  0,  0  /* object reference */)\
_DEFINE(HDLR,   46, BYTE, BASE,  0,  0,  0,  0  /* FE Data Handler */)\
_DEFINE(UTF8,   47, NCHAR,PAD,   0,  0,  0,  0  /* UTF8 data type */)\
_DEFINE(48,     48, NONE, BASE,  0,  0,  0,  0  /* Unused */)\
_DEFINE(49,     49, NONE, BASE,  0,  0,  0,  0  /* Unused */)\
_DEFINE(NULBASE,50, NONE, BASE,  0,  0,  0,  0  /* base type for typeless NULL  */)\
_DEFINE(QTXT,   51, CHAR, BASE,  0,  0,  0,  0  /* Type for qry text  */)\
_DEFINE(TFLD,   52, CHAR, BASE,  0,  0,  0,  0  /* Type for table     */)\
_DEFINE(DEC_CHR,53, NUMB, BASE,  0,  0,  0,  0  /* Type for decimal literal */)\
_DEFINE(NQTXT,  54, NCHAR,PAD,   0,  0,  0,  0  /* Type for qry text */)\
_DEFINEEND

/*
**  The special datatype ID number 99 cannot be used for anything, since it is
**  currently used as an illegal type in some frontend code.  That code should
**  be updated to use the DB_NODT constant instead of 99.
*/
#define                 DB_DT_ILLEGAL   ((DB_DT_ID) 99) /* Temporarily rsrvd */
						      /* for FE illegal type */

/*
**  These constants are used in relation to NULLable datatypes.
*/

#define			DB_NUL_TYPE	(-(DB_NULBASE_TYPE))
						       /* null type       */
						       /* (for frontends) */
        /* This datatype is used by the frontends when a typeless NULL constant
        ** is needed. Since all INGRES datatypes will be nullable in Jupiter,
        ** this can be changed into a NULL value of whatever datatype, as
        ** needed.
	*/
/*
** Define the actual enumerated values for the DB_*_CLASS,
** DB_*_CLASS and DB_*_TYPE constants.
*/
#define _DEFINEEND

#define _DEFINE(dc, v) DB_##dc##_CLASS=v,
enum DB_DC_ID_enum {DB_DC_ID_MACRO DB_MAX_CLASS };
#undef _DEFINE
#define _DEFINE(sc, v) DB_##sc##_SCLASS=v,
enum DB_SC_ID_enum {DB_SC_ID_MACRO DB_MAX_SCLASS };
#undef _DEFINE
#define _DEFINE(cl, v) == v && v+1
# if !(0 DB_DC_ID_MACRO !=0)
#  error "DB_DC_ID_MACRO values not contiguous or not increasing"
# endif
# if !(0 DB_SC_ID_MACRO !=0)
#  error "DB_SC_ID_MACRO values not contiguous or not increasing"
# endif
#undef _DEFINE
#define _DEFINE(ty, v, dc, sc, q, qv, s, sv) DB_##ty##_TYPE=v,
enum DB_DT_ID_enum {DB_DT_ID_MACRO DB_MAX_TYPE, DB_NODT=DB_NODT_TYPE};
#undef _DEFINE
#define _DEFINE(ty, v, dc, sc, q, qv, s, sv) == v && v+1
# if !(0 DB_DT_ID_MACRO !=0)
#  error "DB_DT_ID_MACRO values not contiguous or not increasing"
# endif
#undef _DEFINE

#undef _DEFINEEND

/*}
** Name: DB_BIT_STRING - BIT VARYING datatype.
**
** Description:
**	This structure parallels the one below for character strings.
**	It defines the structure used to hold bit strings of varying length.
**	It holds a 4 byte length field which contains the count of valid bits,
**	followed by that many bits of data.
**
**	The length specified in the db_b_data section is present only to have
**	the compiler allow us to reference it as an array.  Its actual length
**	is specified in the aforementioned length field.
**
** History:
**	21-sep-1992 (fred)
**	   Created.
*/
typedef struct _DB_BIT_STRING
{
    u_i4            db_b_count;         /* The number of bits in the string */
# define	DB_BCNTSIZE	sizeof(u_i4)
    u_char          db_b_bits[1];       /* The actual bit string */
}   DB_BIT_STRING;

/*}
** Name: DB_TEXT_STRING - TEXT, LONGTEXT, and VARCHAR datatypes.
**
** Description:
**      This structure defines the internal structure of the TEXT, LONGTEXT,
**      and VARCHAR datatypes. A DB_TEXT_STRING consists of a 2-byte count
**      followed by an array of characters.  The count tells how many bytes
**      there are.  The count has nothing to do with the amount of space
**      allocated for the string (although the count+2 should never exceed the
**      number of bytes allocated).  The number of bytes allocated for this
**      structure should be 2 greater than the maximum number of bytes that
**      the string will ever be expected to hold. 
**
**	The declared size of the db_t_text field in this structure is bogus.
**	It's only there so that one can refer to the field as an array of
**	characters.
**
**	Semantics:
**	---------
**	    A "text" data value can hold from 0 to DB_MAXSTRING ascii (ebcdic,
**      on ebcdic machines) characters except NULL.  On ascii machines this
**      means any 8 bit byte in the range 0x01 through 0xEF.  On ebcdic
**      machines, 0x01 through 0xFF. (WARNING:  Currently on ebcdic machines,
**      because of the way INGRES handles pattern matching characters, if the
**      user attempts to use the characters 0xEB through 0xEE in a text data
**      value, they will be mis-interpretted as pattern matching characters. 
**
**	    A "varchar" data value can hold from 0 to DB_MAXSTRING bytes.
**      These characters can be any 8 bit bytes, including NULL.  That is, bit
**      patterns from 0x00 up through 0xFF are all considered legal characters
**      by varchar. 
**
**	    A "longtext" data value is ONLY FOR INTERNAL INGRES USE and is NOT
**      ALLOWED TO BE STORED IN A RELATION.  Its semantics are exactly those of
**      of the "varchar" datatype, except that its length is unlimitted.  This
**      datatype will be used by the Frontends for displaying and inputting data
**      values whose display or input forms could be longer than the longest
**      posible text or varchar string. (e.g. displaying a varchar data value
**      comprised of DB_MAXSTRING non-printing characters would take
**      4 * DB_MAXSTRING characters to do.)   It will also be used in the DBMS
**	to represent the typeless NULL value, since ADF will guarantee the
**	existence of coercions from/into "longtext" into/from all other
**	datatypes, which makes this a very convenient way to represent a
**	typeless NULL value.
**
** History:
**	08-nov-85 (jeff)
**          Converted from VAR_STRING in old ingres.h.
**	29-sep-86 (thurston)
**	    Extended the comments documenting the use of DB_TEXT_STRING to
**	    include the VARCHAR and LONGTEXT datatypes.
**	02-dec-86 (thurston)
**	    Changed the .db_t_text member to be u_char instead of char, and
**	    changed the .db_t_count member to be u_i2 instead of i2.
*/

typedef struct _DB_TEXT_STRING
{
    u_i2            db_t_count;         /* The number of chars in the string */
    u_char          db_t_text[1];       /* The actual string */
}   DB_TEXT_STRING;

/*
** Name: DEFINE_DB_TEXT_STRING - for defining local VARCHAR data.
**
**	    Added a macro to portably define locally instantiated VARCHAR
**	    dbms data. This is for temporary storage structures only.
**	    Give it a variable name and a literal string in double quotes
**	    and it will allocate the amount of space required at compile
**	    time and initialise the varchar structure. This negates the
**	    need to assign the leading count field using a cast, which
**	    can bus error if a simple char array (char name[] = "string";)
**	    was used (as this is NOT guaranteed to be on a word boundary).
**	    The base types of these fields MUST be kept the same as in struct
**	    _DB_TEXT_STRING. It must be used in a declaration list.
** History:
**	29-feb-96 (morayf)
**	    Created.
*/

#define DEFINE_DB_TEXT_STRING(name,str) \
                struct _##name { \
                                u_i2 db_t_count; \
                                u_char db_t_text[sizeof(str)]; \
                        } name = { sizeof(str)-1, str };



/*
**  These are various constants regarding the string datatypes:
*/

#define		DB_CNTSIZE	    2       /* Size of the db_t_count field */
#define		DB_MAXSTRING	    32000	/* Max # chars in an INGRES */
						/*     string (`c', `text', */
						/*   `char', or `varchar'). */
#define		DB_GW4_MAXSTRING    32000	/* .......... for gateways. */
	    /*
	    ** <<< NOTE >>> ADF now accepts a new session startup parameter for
	    **		    determining what the maximum size INGRES string will
	    **		    be for that session.  This allows the INGRES DBMS to
	    **		    always pass in DB_MAXSTRING to each ADF session,
	    **		    since INGRES only supports 2000 chars, while one of
	    **		    our Frontends linking with ADF and running against
	    **		    an INGRES Gateway to a DBMS that supports more than
	    **		    this may pass in any number up to DB_GW4_MAXSTRING.
	    */
#define		DB_CHAR_MAX	DB_MAXSTRING	/* ON ITS WAY OUT.  */
						/* This WAS used to repre-  */
						/* sent the max # chars in  */
						/* a `c' or `char' string,  */
						/* when that was different  */
						/* from the max # chars in  */
						/* a `text' or `varchar'.   */

#define	        DB_UTF8_MAXSTRING   DB_MAXSTRING/2 /* For UTF8 installations limit the
						   ** max size of chars in a Ingres strings
						   ** to half of the max allowed. UTF8 strings
						   ** are internally processed as unicode strings
						   ** and this could be double the size of input
						   */

/* Unicode standard states at http://unicode.org/faq/normalization.html#12 That
** normalization to any of the normal forms could cause expansion to the source
** string as follows:
**
**   	UTF 	Factor 	Sample
** NFC 	8 	3X 	U+1D160
** 	16,32 	3X 	U+FB2C
** NFD 	8 	3X 	U+0390
** 	16,32 	4X 	U+1F82
**	
** Based on this the following expansion factors are declared.
*/
#define		DB_MAXUTF8_EXP_FACTOR	3
#define		DB_MAXUTF16_EXP_FACTOR	4
/*
** Some definitions for unicode data type
*/
#define DB_ELMSZ   (sizeof(UCS2))
#define DB_MAX_NVARCHARLEN     (DB_MAXSTRING / DB_ELMSZ)
#define DB_MAX_NCHARLEN  (DB_MAXSTRING / DB_ELMSZ)

/*}
** Name: DB_NVCHR_SRING - A list of Unicode code points
**
** Description:
**      This structure defines the "nvarchar" type. This is different as
**    it uses a two byte array of code points to store the data.
**
** History:
**	24-Jan-2001 (gupsh01)
**	    Created.
*/
typedef       struct _DB_NVCHR_STRING
{
	i2   count;			/*  Number of valid elements  */
	UCS2    element_array[1];	/*  The elements themselves   */
} DB_NVCHR_STRING;


/*
**  Names of internal QUEL pattern-matching characters,
**  corresponding to *, ?, [, ].
*/

#ifdef    EBCDIC

#define                 DB_PAT_ANY      0xebL /* Match any seq. of chars */
#define			DB_PAT_ONE	0xecL /* Match any single char */
#define			DB_PAT_LBRAC	0xedL /* Start set of chars to match */
#define			DB_PAT_RBRAC	0xeeL /* End set of chars to match */

#else  /* EBCDIC */

#define                 DB_PAT_ANY      0x01L /* Match any seq. of chars */
#define			DB_PAT_ONE	0x02L /* Match any single char */
#define			DB_PAT_LBRAC	0x03L /* Start set of chars to match */
#define			DB_PAT_RBRAC	0x04L /* End set of chars to match */

#endif /* EBCDIC */


/*}
** Name: DB_DATA_VALUE - INGRES DBMS's internal representation of
**                       a data value.
**
** Description:
**      This structure is how the INGRES DBMS represents any data value.
**      It contains the datatype and length of the data value, as well
**      as a pointer to the actual data.  (Often, this is a pointer into
**      a tuple.)
**
** History:
**	18-feb-86 (thurston)
**          Initial creation.
**	28-feb-86 (thurston)
**          Added the "db_" prefix to all members, and
**          added the "db_prec" member (currently unused).
**	01-apr-86 (thurston)
**	    Changed the definition of the .db_data member to be the
**	    generic PTR instead of a DB_ANYTYPE.
**	01-sep-86 (seputis)
**          moved fields around for alignment
**	9-dec-04 (inkdo01)
**	    Added db_collID to define collation.
*/
typedef struct _DB_DATA_VALUE
{
    PTR		    db_data;		/* Ptr to the actual data.
                                        ** Often, this will be a ptr
                                        ** into a tuple.
                                        */
    i4              db_length;          /* Number of bytes of storage
                                        ** reserved for the actual data.
                                        */
    DB_DT_ID        db_datatype;        /* The datatype of the data
                                        ** value represented here.
                                        */
    i2              db_prec;            /* Precision of the data value.
                                        ** Most datatypes will not need
                                        ** this, however, there are some
                                        ** that will; such as DECIMAL.
                                        ** In the DECIMAL case, the high
					** order byte will represent the
					** value's precision (total # of
					** significant digits), and the
					** low order byte will hold the
					** value's scale (the # of these
					** digits that are to the right
					** of an implied decimal point.
					** The three MACROs defined below
					** can be used to encode/decode
					** precision and scale into/from
					** this field, and the fourth one
					** can be used to derive the length
					** given the precision.
					*/
    i2		    db_collID;		/* Collation ID for this 
					** character field */
}   DB_DATA_VALUE;

/*
** Name: DB_COPY_DV_TO_ATT
**
** Description:
**	Copy the info in DB_DATA_VALUE to GCA_COL_ATT.  This
**	permits GCA to redefine its unused db_data member as
**	long as the other members (db_datatype, db_length,
**	db_prec) keep the same name and type.
**
** Input:
**	dv	DB_DATA_VALUE *		source DB datavalue.
**	att	GCA_COL_ATT *		dest GCA column attribute.
*/
#ifdef BYTE_ALIGN

#define DB_COPY_DV_TO_ATT( dv, att ) \
{ \
    MECOPY_CONST_MACRO( (PTR)&((DB_DATA_VALUE *)dv)->db_datatype, \
			(u_i2)sizeof(((DB_DATA_VALUE *)dv)->db_datatype), \
			(PTR)&((GCA_COL_ATT *)att)->gca_attdbv.db_datatype ); \
    MECOPY_CONST_MACRO( (PTR)&((DB_DATA_VALUE *)dv)->db_length, \
			(u_i2)sizeof(((DB_DATA_VALUE *)dv)->db_length), \
			(PTR)&((GCA_COL_ATT *)att)->gca_attdbv.db_length ); \
    MECOPY_CONST_MACRO( (PTR)&((DB_DATA_VALUE *)dv)->db_prec, \
			(u_i2)sizeof(((DB_DATA_VALUE *)dv)->db_prec), \
			(PTR) &((GCA_COL_ATT *)att)->gca_attdbv.db_prec ); \
}

#else

#define DB_COPY_DV_TO_ATT( dv, att ) \
{ \
    ((GCA_COL_ATT *)att)->gca_attdbv.db_datatype = \
					((DB_DATA_VALUE *)dv)->db_datatype; \
    ((GCA_COL_ATT *)att)->gca_attdbv.db_length = \
					((DB_DATA_VALUE *)dv)->db_length; \
    ((GCA_COL_ATT *)att)->gca_attdbv.db_prec = \
					((DB_DATA_VALUE *)dv)->db_prec; \
}

#endif

/*
** Name: DB_COPY_ATT_TO_DV
**
** Description:
**	Copy the info in GCA_COL_ATT to DB_DATA_VALUE.  This
**	permits GCA to redefine its unused db_data member as
**	long as the other members (db_datatype, db_length,
**	db_prec) keep the same name and type.
**
** Input:
**	att	GCA_COL_ATT *		source GCA column attribute.
**	dv	DB_DATA_VALUE *		destination DB datavalue.
*/
#ifdef BYTE_ALIGN

#define	DB_COPY_ATT_TO_DV( att, dv ) \
{ \
    MECOPY_CONST_MACRO( (PTR)&((GCA_COL_ATT *)att)->gca_attdbv.db_datatype, \
			(u_i2)sizeof(((DB_DATA_VALUE *)dv)->db_datatype), \
			(PTR)&((DB_DATA_VALUE *)dv)->db_datatype ); \
    MECOPY_CONST_MACRO( (PTR)&((GCA_COL_ATT *)att)->gca_attdbv.db_length, \
			(u_i2)sizeof(((DB_DATA_VALUE *)dv)->db_length), \
			(PTR)&((DB_DATA_VALUE *)dv)->db_length ); \
    MECOPY_CONST_MACRO( (PTR)&((GCA_COL_ATT *)att)->gca_attdbv.db_prec, \
			(u_i2)sizeof(((DB_DATA_VALUE *)dv)->db_prec), \
			(PTR)&((DB_DATA_VALUE *)dv)->db_prec ); \
}

#else

#define	DB_COPY_ATT_TO_DV( att, dv ) \
{ \
    ((DB_DATA_VALUE *)dv)->db_datatype = \
				((GCA_COL_ATT *)att)->gca_attdbv.db_datatype; \
    ((DB_DATA_VALUE *)dv)->db_length = \
				((GCA_COL_ATT *)att)->gca_attdbv.db_length; \
    ((DB_DATA_VALUE *)dv)->db_prec = \
				((GCA_COL_ATT *)att)->gca_attdbv.db_prec; \
}

#endif


/*
**  MACROs for encoding/decoding a DECIMAL value's precision and scale
**  into/from their i2 representation; such as the .db_prec field in a
**  DB_DATA_VALUE; also one macro to go from precision to length.
**
** History:
**	18-jul-88 (thurston)
**	    Initial creation.
**	06-jul-89 (jrb)
**	    Added DB_LN_FRM_PR_MACRO, DB_MAX_DECPREC, and DB_MAX_DECLEN
**	19-sep-89 (jrb)
**	    Changed DB_LN_FRM_PR_MACRO to DB_PREC_TO_LEN_MACRO which is more
**	    readable.
**	16-jan-2007 (dougi)
**	    Update max precision to 39 (20 bytes).
*/

    /*
    ** Given integer values for precision and scale, returns an i2 holding
    ** precision in the high byte and scale in the low byte.
    */
#define	    DB_PS_ENCODE_MACRO(p,s)	((i2) (((p) * 256) + (s)))

    /*
    ** Given an i2 holding precision in the high byte and scale in the low
    ** byte, returns the precision:
    */
#define	    DB_P_DECODE_MACRO(ps)	((i2) ((ps) / 256))

    /*
    ** Given an i2 holding precision in the high byte and scale in the low
    ** byte, returns the scale.
    */
#define	    DB_S_DECODE_MACRO(ps)	((i2) ((ps) % 256))

    /*
    ** Given the precision (the REAL precision, not the precision/scale combo)
    ** returns a i4  with the number of bytes required to hold the value.
    */
#define	    DB_PREC_TO_LEN_MACRO(prec)	((i4) ((prec)/2+1))

    /*
    **	This is the maximum value for precision and length for the DECIMAL
    **	datatype across all platforms.
    */
#define	    DB_MAX_DECPREC		CL_MAX_DECPREC
#define	    DB_MAX_DECLEN		DB_PREC_TO_LEN_MACRO(DB_MAX_DECPREC)

/*}
** Description:
**      Constants for FLOAT datatype.  Use to determine the internal 
**      datatype that float(N) will map to. The precisions are in mantissa
**	bits (as per ANSI/ISO SQL) as specified for IEEE compliant floats.
**      
**         Value for N                                  length
**         ===========                                  ======
**            1         to DB_MAX_F4PREC                  4
**      DB_MAX_F4PREC+1 to DB_MAX_F8PREC                  8
**
** History:
**      16-jun-92 (stevet)
**          Added DB_MAX_F4PREC and DB_MAX_F8PREC.
**	3-oct-01 (inkdo01)
**	    Change F4PREC to binary precision, like F8PREC.
*/

#define	    DB_MAX_F4PREC		23
#define	    DB_MAX_F8PREC		53


/*}
** Name: DB_EMBEDDED_DATA - Specifies a value of an embedded language
**			     canonical type.
**
** Description:
**      This structure is how the INGRES EQLs represent any embedded value.
**      It contains the datatype and length of the value, as well as a pointer
**      to the actual data, and a pointer to a null indicator.
**
** History:
**      02-dec-86 (thurston)
**          Initial creation.
**	31-dec-86 (thurston)
**	    Added comment that an embedded type can now be a DB_VCH_TYPE as
**	    well.
**	10-feb-87 (thurston)
**	    Added the DB_EMB_NULL constant.
**      17-Nov-2003 (hanal04) Bug 111282 INGEMB112
**          Changed ed_length to unsigned to prevent overflow when
**          using NVARCHARs.
*/
typedef struct _DB_EMBEDDED_DATA
{
    i2		    ed_type;		/* The type of the data.           */
					/* This can be equal to any of the */
					/* following DB_DT_ID's:           */
					/*	DB_INT_TYPE                */
					/*	DB_FLT_TYPE                */
					/*	DB_CHR_TYPE                */
					/*	DB_VCH_TYPE                */
					/*	DB_DBV_TYPE                */
					/*	DB_NUL_TYPE                */

    u_i2	    ed_length;		/* The length of the data, in bytes. */

    PTR		    ed_data;		/* The pointer to the data. */

    i2		    *ed_null;		/* The pointer to the null indicator. */

#define			DB_EMB_NULL (-1)    /* If the i2 pointed to by     */
					    /* .ed_null is equal to this   */
					    /* value, then we have a null- */
					    /* valued data item.	   */
}   DB_EMBEDDED_DATA;


/*}
** Name: DB_ANYTYPE - A union that can hold any of the INGRES intrinsic types.
**
** Description:
**      This union can hold any of the INGRES intrinsic types.  An intrinsic
**      type is one that can be passed back and forth between the server and
**	its frontends.  There are a couple of other types in here also.
**
**	The declared size of 1 on the db_ctype member is bogus.  It is there
**	only so that the member can be referred to as an array of characters.
**	When the db_ctype and db_textype members are used, it is expected that
**	it will be to reference strings longer than one character.  Typically,
**	one will have a pointer to a DB_ANYTYPE, and when it points to an
**	array of characters one will use the db_ctype member.
**
** History:
**     08-nov-85 (jeff)
**          Converted from the anytype union in the old ingres.h.
**	31-Jan-2004 (schka24)
**	    Add i8 to the list.
*/
typedef union _DB_ANYTYPE
{
    i1              db_booltype;        /* boolean intrinsic type */
#define DB_FALSE ((i1)FALSE)
#define DB_TRUE ((i1)TRUE)
#define DB_BOO_LEN (sizeof(i1))
    i1              db_i1type;          /* i1 intrinsic type */
    i2              db_i2type;          /* i2 intrinsic type */
    i4              db_i4type;          /* i4 intrinsic type */
    i8		    db_i8type;		/* i8 intrinsic type */
    f4              db_f4type;          /* f4 intrinsic type */
    f8              db_f8type;          /* f8 intrinsic type */
    char            db_ctype[1];        /* c intrinsic type */
    char            *db_cptype;         /* pointer to c intrinsic type */
    char            **db_cpptype;       /* ptr to ptr to c intrinsic type */
    DB_TEXT_STRING    db_textype;       /* text intrinsic type */
    DB_TEXT_STRING    *db_textptype;    /* pointer to text intrinsic type */
    DB_TEXT_STRING    **db_texpptype;   /* ptr to ptr to text intrinsic type */
}   DB_ANYTYPE;


/*}
** Name: DB_DATE_FMT - The multinational date format.
**
** Description:
**      This typedef defines the format style to use for date input
**      and output.  At present, there are four possible settings for
**      this which are defined by the symbolic constants DB_US_DFMT
**      (use for a SET DATE_FORMAT "US" command), DB_FIN_DFMT (use for
**      a SET DATE_FORMAT "FINLAND" or SET DATE_FORMAT "SWEEDEN" command),
**      DB_MLTI_DFMT (use for a SET DATE_FORMAT "MULTINATIONAL" command),
**      DB_ISO_DFMT (use for a SET DATE_FORMAT "ISO" command), and DB_GERM_DFMT
**	(use for a SET DATE_FORMAT "GERMAN" command).
**
**      The following table shows some of the various input and output formats
**      for these different settings:  (In all cases below, "dd" represents
**      the two digit day of the month, "mm" represents the 2 digit month
**      of the year, "mmm" represents the three character abbreviation for
**      the month, "yy" represents the two digit year, and "yyyy" represents
**      the full four digit year.)
**
**            setting      output format    legal input formats
**          ------------   -------------    -------------------
**          DB_US_DFMT      dd-mmm-yyyy         dd-mmm-yyyy
**                                              mm/dd/yy
**                                              mmddyy
**
**          DB_MLTI_DFMT    dd/mm/yy            dd-mmm-yyyy
**                                              dd/mm/yy
**                                              mmddyy
**
**          DB_MLT4_DFMT    dd/mm/yyyy          dd-mmm-yyyy
**                                              dd/mm/yy
**                                              mmddyy
**
**          DB_FIN_DFMT     yyyy-mm-dd          yyyy-mm-dd
**                                              mm/dd/yy
**                                              mmddyy
**
**          DB_ISO_DFMT     yymmdd              dd-mmm-yyyy
**                                              mm/dd/yy
**                                              yymmdd
**
**          DB_ISO4_DFMT   yyyymmdd            dd-mmm-yyyy
**                                              mm/dd/yy
**                                              yymmdd
**
**          DB_GERM_DFMT    ddmmyyyy            dd-mmm-yyyy
**                                              mm/dd/yy
**                                              dmmyy
**                                              ddmmyy
**                                              dmmyyyy
**                                              ddmmyyyy
**
**          DB_YMD_DFMT     yyyy-mmm-dd		yyyy-mmm-dd
**                                              mm/dd
**                                              mmdd
**                                              yymdd
**                                              yymmdd
**                                              yyyymdd
**                                              yyyymmdd
**
**          DB_MDY_DFMT     mmm-dd-yyyy         mmm-dd-yyyy
**                                              mm/dd
**                                              mmdd
**                                              mddyy
**                                              mmddyy
**                                              mddyyyy
**                                              mmddyyyy
**
**          DB_DMY_DFMT     dd-mmm-yyyy         dd-mmm-yyy
**                                              dd/mm
**                                              ddmm
**                                              ddmyy
**                                              ddmmyy
**                                              ddmyyyy
**                                              ddmmyyyy
**
** History:
**     21-feb-86 (thurston)
**          Initial creation.
**      03-mar-86 (thurston)
**          Put the (DB_DATE_FMT) cast on the #defines to please lint.
**	19-may-88 (thurston)
**	    Added DB_GERM_DFMT.
**      05-may-92 (stevet)
**          Added DB_YMD_DFMT, DB_MDY_DFMT, DB_DMY_DFMT.
**	24-Jun-1999 (shero03)
**	    Added ISO4
**	01-aug-2006 (gupsh01)
**	    Added DB_ANSI_DFMT for internal use.
*/
typedef i4  DB_DATE_FMT;
#define                 DB_US_DFMT      (DB_DATE_FMT) 0	/* US style dates    */
							/* (default)         */

#define                 DB_MLTI_DFMT    (DB_DATE_FMT) 1 /* European style    */
							/* dates             */

#define                 DB_FIN_DFMT     (DB_DATE_FMT) 2 /* Finnish & Swedish */
							/* conventions       */

#define                 DB_ISO_DFMT     (DB_DATE_FMT) 3 /* Peter Madam's ISO */
							/* date format       */

#define                 DB_GERM_DFMT    (DB_DATE_FMT) 4 /* German date       */
							/* format            */

#define                 DB_YMD_DFMT     (DB_DATE_FMT) 5 /* YMD date format   */

#define                 DB_MDY_DFMT     (DB_DATE_FMT) 6 /* MDY date format   */

#define                 DB_DMY_DFMT     (DB_DATE_FMT) 7 /* DMY date format   */

#define			DB_MLT4_DFMT	(DB_DATE_FMT) 8 /* Multinational */

#define                 DB_ISO4_DFMT    (DB_DATE_FMT) 9 /* ISO date with yyyy */

#define                 DB_ANSI_DFMT    (DB_DATE_FMT) 10 /* ANSI date with yyyy-mm-dd */
														/* w 4-digit yr */


/*}
** Name: DB_MONY_FMT - The multinational money format, including precision.
**
** Description:
**      This structure holds a multinational money format and precision.
**      It includes the money symbol, whether it is leading or trailing,
**      and how many digits to be printed after the decimal.
**
** History:
**     21-feb-86 (thurston)
**          Initial creation.
**     02-apr-86 (thurston)
**	    Changed the names of two symbolic constants because they were
**	    not unique to 8 characters:  DB_MONY_LEAD becomes DB_LEAD_MONY,
**	    and DB_MONY_TRAIL becomes DB_TRAIL_MONY.
**     16-apr-86 (thurston)
**	    Added the DB_MNYRSVD_SIZE constant, and the .db_mny_rsvd member
**	    in order to keep the structure aligned properly.
**	01-Jul-1999 (shero03)
**	    Added DB_NONE_MONY for SIR 92541.
*/
#define                 DB_MAXMONY      4   /* The maximum number of  */
                                            /* characters for a money */
                                            /* sign.                  */

#define		DB_MNYRSVD_SIZE (DB_ALIGN_SZ - ((DB_MAXMONY+1) % DB_ALIGN_SZ))
					    /* Size of the db_mny_rsvd  */
					    /* field necessary to align */
					    /* following members of the */
					    /* DB_MONY_FMT struct.      */

typedef struct _DB_MONY_FMT
{
    char            db_mny_sym[DB_MAXMONY+1]; /* The money sign to
                                        ** use when converting money
                                        ** data values to either the
                                        ** text or c datatypes.  This
                                        ** can be up to DB_MAXMONEY
                                        ** characters long; the +1
                                        ** is supplied because this
                                        ** will be a null terminated
                                        ** string.
                                        */
    char            db_mny_rsvd[DB_MNYRSVD_SIZE]; /* Required to align
					** rest of this struct.
					*/
    i4              db_mny_lort;        /* Tells whether money
                                        ** symbol should be leading
                                        ** or trailing, as follows:
                                        */
#define                 DB_LEAD_MONY    0   /* Money symbol will be leading */
#define                 DB_TRAIL_MONY   1   /* Money symbol will be trailing */
#define                 DB_NONE_MONY    2   /* No Money symbol */
    i4              db_mny_prec;        /* Tells how many digits to
                                        ** after the decimal to show
                                        ** when converting money data
                                        ** values to either the text
                                        ** or c datatypes.  Legal
                                        ** values are 0, 1, and 2.
                                        */
}   DB_MONY_FMT;

/*}
** Name: DB_DECIMAL - Decimal marker specifier
**
** Description:
**      This structure defines a decimal marker specifier, which tells what
**      character to use as a decimal point when scanning query text.
**
** History:
**     10-oct-85 (jeff)
**          written
**     02-jan-86 (jeff)
**	    moved here from a parser header
*/
typedef struct _DB_DECIMAL
{
    i4              db_decspec;         /* TRUE if decimal marker specified */
    i4              db_decimal;         /* The character to use as a marker */
} DB_DECIMAL;

/*}
** Name: DB_FACILITY - facility type
**
** Description:
**      This set of definitions provides a global set of definitions
**      for facility id's throughout the DBMS.  Also, a facilities
**      id is related to it's error mask in that the error mask
**      should be define as the facility id shifted left 16 bits
**      (e.g. #define E_SC_MASK (E_SCF_ID << 16) )
**
** History:
**      11-apr-88 (seputis)
**          initial creation
**	06-aug-92 (markg)
**	    Added new facility type for SXF.
**      05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**	16-jun-97 (stephenb)
**	    Add new facility code for CUF (why wasn't it here before ?)
**	    Also add error mask, here's a good place for it.
[@history_line@]...
[@history_template@]...
*/
typedef i4  DB_FACILITY;
#define		    DB_MIN_FACILITY     1

#define			DB_CLF_ID	    1	/*  the cl */
#define			DB_ADF_ID	    2	/* note that these are the */
#define			DB_DMF_ID	    3	/* same as the error constant */
#define			DB_OPF_ID	    4	/* prefixes and should be kept */
#define			DB_PSF_ID	    5	/* that way for ease of */
#define			DB_QEF_ID	    6	/* identification and maintenance */
#define			DB_QSF_ID	    7	/* as specified above */
#define			DB_RDF_ID	    8
#define			DB_SCF_ID	    9
#define			DB_ULF_ID	    10
#define			DB_DUF_ID	    11
#define			DB_GCF_ID	    12
#define			DB_RQF_ID	    13
#define			DB_TPF_ID	    14
#define     DB_GWF_ID     15
#define			DB_SXF_ID	    16
#define			DB_URS_ID	    17
#define			DB_ICE_ID	    18
#define			DB_CUF_ID	    19

#define		    DB_MAX_FACILITY	19

#define                 E_CUF_MASK      (DB_CUF_ID << 16)
/*}
** Name: DB_QRY_ID - ID for query text stored in iiqrytext relation
**
** Description:
**      This structure defines the ID for the query text stored in the IIQRYTEXT
**	relation.  It's really just a timestamp.
**
** History:
**     15-apr-86 (jeff)
**          written
*/
typedef struct _DB_QRY_ID
{
    u_i4            db_qry_high_time;
    u_i4	    db_qry_low_time;
}   DB_QRY_ID;


/*
** Name: DB_SQLSTATE - an sqlstate string
**
** Description:
**      string corresponding to an SQLSTATE - an array of 5 chars
**
** History:
**	14-oct-92 (andre)
**	    written
*/
#define		DB_SQLSTATE_STRING_LEN	5
typedef	struct DB_SQLSTATE_
{
    char	db_sqlstate[DB_SQLSTATE_STRING_LEN];
} DB_SQLSTATE;
/******************************************************************************/
/* The following structures should be in dbdbms.h, but are placed here due to
** tuple compare routines in ADF used by DMF */
/******************************************************************************/

/*}
** Name: DB_CMP_LIST - A field comparison entry.
**
** Description:
**      This structure describes the type of structure passed to the 
**	sort compare record routine.
**
** History:
**     07-jul-1987 (jennifer)
**          Created new for jupiter
**	9-dec-04 (inkdo01)
**	    Removed cmp_flags (never used) and added cmp_collID to define
**	    collation to be used for comparison.
*/
typedef struct _DB_CMP_LIST
{
    i4		    cmp_offset;		/* Offset to field. */
    i4		    cmp_length;		/* Length of the field. */
    i2		    cmp_type;           /* The data type. */
    i2		    cmp_precision;	/* Precision of the field. */
    i2		    cmp_direction;	/* Comparison direction. */
    i2		    cmp_collID;		/* Collation for comparing character
					** fields. */
}   DB_CMP_LIST; 

/* Partitioning definitions.
**
** These definitions are back-end only, but ADF needs to know about
** partitioning.  So they are declared here.
**
** To support value-based horizontal partitioning, we need to be
** able to compute a partition number given a row.  (Actually, given
** specific columns in the row, or some equivalent.)
** While it will be some higher level's job to apply multi-dimensional
** partitioning and run the show, when it comes down to actually
** manipulating and testing data values it's ADF's job.
**
** There are four types of partitioning available:
**
** AUTOMATIC partitioning -- this is in fact not value based, so
** it's pretty simple!
**
** HASH partitioning -- we take a bunch of column values and spit
** out a number (a u_i4).  This number is taken MOD the actual number
** of partitions.
**
** RANGE partitioning -- the input values are compared to the entries
** in a range breaks table to find range containing the input.  The
** break-values table is sorted to make this easier.
**
** LIST partitioning -- similar to range, except that instead of looking
** for a containing range, we look for an exact match value in the breaks
** table.  There will be an "else" or default entry to use if nothing
** matches.
**
** ADF needs to know the layout of the input row, where the relevant
** column values are, and their types.  It also needs to know the layout
** of the breaks table.  That's what is defined here.
**
*/

/* Notice! Do not reorder these without checking and possibly fixing all
** the places that say: DBDS_RULE_xxx Order Sensitive
*/
#define	DBDS_RULE_AUTOMATIC	0	/* Automatic (value independent) */
#define DBDS_RULE_HASH		1	/* Hash on column value */
#define DBDS_RULE_LIST		2	/* Explicitly listed column values */
#define DBDS_RULE_RANGE		3	/* Column value range */
#define DBDS_RULE_MAX		3	/* Highest rule number */

/* DB_PART_LIST - Partition column descriptor list.
**
** A DB_PART_LIST describes one partitioning column for a particular
** partitioning dimension.  It describes the type of column, where it
** is in a break-value, and where it is in a row image.  It also
** specifies the column number (attribute number), although ADF probably
** isn't too interested in that.
**
** Typically there will be an array of these things, since partitioning
** can be done on multiple columns.
**
** History:
**	30-Dec-2003 (schka24)
**	    Create for partitioning.
**	10-Mar-2004 (schka24)
**	    Make row offset an i4 for 256K rows.
**	9-dec-04 (inkdo01)
**	    Add collID for collation of character columns.
*/

typedef struct _DB_PART_LIST
{
    i2		type;			/* DT_xxx_TYPE data type */
    i2		length;			/* Length including any null bytes */
    i2		precision;		/* Precision if relevant */
    i2		att_number;		/* Attribute number */

/* Offsets everywhere else are i2's, might as well do it here too */

/* Note that internal format break-values can be arranged in any convenient
** manner, as they are not stored as such on disk in the catalogs.  E.g.,
** the top levels may choose to natively-align them...
*/
    i2		val_offset;		/* Offset within a break-value */
    i2		collationID;		/* Collation ID (for char's) */
/* A row-image offset may or may not be aligned to anything.
** The upper level callers get to define what "row image" means;
** it might be a native row as retrieved from DMF, or it might be
** something put together by the query compiler -- doesn't matter to us...
*/
    i4		row_offset;		/* Offset within a "row image" */
} DB_PART_LIST;

/* DB_PART_BREAKS - Partitioning break table entry
**
** Range and list distribution schemes use a break values table to
** determine which partition to send a particular row to.
** List distribution schemes look for a value equal to the row
** value, while range schemes look for a range that the row falls
** into.
**
** The (possibly multicolumn) values that a break table entry points
** to are arranged in some reasonably convenient and aligned manner;
** the val_offset member of each DB_PART_LIST entry gives that
** column's offset within the actual value.
**
** RDF and parser facilities find it convenient to keep around the
** original, untranslated text form of each value.  Since the break
** values are sorted into order when they are read, it's simplest to
** keep the text value with the rest of the entry.  I say "text value",
** but of course break values can be multi-column, so each value may
** have multiple text strings!  So we actually point to a little
** array of text pointers, with the text for the N-th column component
** of the value being pointed to by the N-th array entry.  (Rather
** wasteful if break values are just a single column -- oh well.)
**
** Please note that breaks tables that are generated by or for the use
** of query compiling and runtime may not have any untranslated texts
** around -- only the internal form value.  In this case the text-pointer
** would be null.
**
** The break lookup gives a partition sequence, a one-origined
** sequence number within this partitioning dimension.  A higher
** level will need to apply the dimension stride to get an actual
** partition number.
**
** History:
**	30-Dec-2003 (schka24)
**	    Create for partitioned tables.
*/

typedef struct _DB_PART_BREAKS {
    PTR		val_base;		/* Start of break constant value */
    DB_TEXT_STRING **val_text;		/* Untranslated constant text array */
					/* Optional.  Each array entry points
					** to a the text for a component of
					** the break value.
					*/
    i2		oper;			/* DBDS_OP_xxx testing operator */
    u_i2	partseq;		/* Partition sequence if match */
} DB_PART_BREAKS;

/* Operator codes are assigned such that op XOR 4 gives the reverse
** test.  i.e. < is paired with >=, <= is paired with >.  Likewise,
** OP & (~1) transforms both LT and LE into LT, and both GT and GE into GE
** (i.e. it's a way to check that two ops are in the "same direction").
** We don't need to deal with = and <>, distribution schemes only use =
** (for LIST).
** The DEFAULT entry is defined as an operator rather than a goofy value.
**
** Do Not fool with the ordering of these codes unless you fix the
** places where the codes are used.
**
** I could perhaps have used the ADI_xx_OP definitions from
** common!hdr!adfops.h, but they are assigned in some non-obvious
** manner, and I was uncomfortable with hardwiring those assignments
** into a system catalog.  (These opcodes appear in the iidistval catalog.)
*/
#define DBDS_OP_LT	0		/* Less than */
#define DBDS_OP_LTEQ	1		/* Less than or equal */
#define DBDS_OP_EQ	2		/* Equal to for list */
/* 3 would be not equal to, not used, saved just in case! */
#define DBDS_OP_GTEQ	4		/* Greater than or equal */
#define DBDS_OP_GT	5		/* Greater than */
#define DBDS_OP_DFLT	6		/* Default entry */

/*}
** Name: DB_PART_DIM - partition dimension description
**
** Description:
**	This structure describes one dimension of a horizontal partitioning
**	scheme.
**
**	A partitioning scheme is a bit like a multidimensional array.
**	Each dimension has a size (i.e. number of partitions), and
**	some rule for distributing rows into the partitions.
**	Most rules are value dependent in some manner, so we need
**	to know what columns supply the partitioning value.
**	For RANGE and LIST distributions, we also need the break
**	values (constants) that define which rows go into what partitions.
**
**	The DB_PART_DIM structure is not stand-alone, it's generally
**	found as a member of the overall DB_PART_DEF structure.
**
** History:
**	30-Dec-2003 (schka24)
**	    Teach everyone about partitioning.
**	20-Jan-2004 (schka24)
**	    Add the partitioning value size.
*/

typedef struct _DB_PART_DIM {
    i2		distrule;		/* Distribution rule, DBDS_RULE_xxx */

/* The stride is the number of physical partitions that actually make
** up one logical partition at this dimension.  The term "stride"
** is from multi-dimension array terminology;  another way to interpret
** stride is the number of physical partitions that you cover going
** from partition-sequence N to sequence N+1 at this dimension.
** Absolute partition numbers have to fit in an i2, so an i2 is OK here.
*/
    u_i2	stride;			/* Stride at this dimension */

/* Define the size (number of logical partitions) at this dimension */
    u_i2	nparts;			/* Number of partitions in dimension */

/* Value-based distribution schemes may use multiple columns to generate
** the distribution value, keep the number of columns here.  For
** value-independent (automatic) distributions, this is zero.
*/
    i2		ncols;			/* Number of columns in distribution
					** value */

/* RANGE and LIST distributions work off of a break values table.
** There can be more than one break value (or range) that maps to
** a partition, so we need the number of break entries.
*/
    i2		nbreaks;		/* Number of break value entries */

/* For any value-based distribution scheme, the total length of a
** partitioning value is useful.  This is the internal form of a
** partitioning value, ie the offset+length of the last value in
** the DB_PART_LIST, rounded up to ALIGN_RESTRICT.
*/
    u_i2	value_size;		/* Value size for this dimension */

/* The breaks table can (optionally) contain pointers to the original
** un-interpreted value text strings.  When copying a partition definition,
** you can ask for a copy of those text strings.  This is much easier
** if we know how much space is taken up by those text strings.
** Partition definitions don't have to have text strings;  for copies
** that don't, this member would be zero.
*/
    i4		text_total_size;	/* Size of raw text for this dimension */

/* Each partition has a name, whether user assigned or system generated.
** Store a pointer to an array of nparts DB_NAME's here.  RDF will
** always have the names, but it may be that other copies of the same
** structure may not need the names and may omit them.  Or not.  (A
** copy for QEF's use might omit the names, for instance.)
** (DB_NAME typedef not defined till dbdbms.h, use struct instead)
*/
    struct _DB_NAME	*partnames;	/* Pointer to partition name array */

/* Value-based distribution schemes need to know what columns generate
** the distribution value.  Define a pointer to an array of DB_PART_LIST
** entries, one per column.  Each entry has type and offset stuff in it.
** This will be NULL for automatic (value-independent) distributions.
*/
    DB_PART_LIST *part_list;		/* Pointer to column info array */

/* For RANGE and LIST, we need the range breaks or list values.
** There will be nbreaks entries in this array.
** See above for the structure of a breaks table entry.
** For distributions that don't use a breaks table, this will be NULL.
*/
    DB_PART_BREAKS *part_breaks;	/* Pointer to break values array */
} DB_PART_DIM;

/* Name: DB_PART_DEF - Partitioning scheme definition
**
** Description:
**	This structure defines a partitioning scheme for a partitioned
**	table.  There is some header info, such as the number of
**	dimensions, and then one DB_PART_DIM entry for each dimension.
**	Thus, this is a variable sized structure.
**
** History:
**	30-Dec-2003 (schka24)
**	    Teach everyone about partitioning.
**	5-Aug-2004 (schka24)
**	    Distinguish "one part" and "n parts" structure compatibility.
*/

typedef struct _DB_PART_DEF {

/* Number of (physical) partitions and number of dimensions (levels) are
** copied out of relnparts and relnpartlevels, for convenience.
** (Note that level == dimension when talking about partitions.)
*/
    u_i2	nphys_parts;		/* Number of physical partitions */
    i2		ndims;			/* Number of partitioning dimensions */

/* Useful flags... */
    u_i2	part_flags;		/* Partition info flags */
#define	DB_PARTF_KEY_ONE_PART	0x0001	/* Partitioning scheme is direct-
					** plan compatible with the master
					** table storage structure, ie
					** rows with a given key are known
					** to all be in the same physical
					** partition.
					*/
#define DB_PARTF_ONEPIECE	0x0002	/* This partition definition has
					** been created via the RDF copy-
					** partition function, and therefore
					** is known to inhabit one single
					** block of memory in its entirety.
					** (Might be a convenience to users
					** which have to copy it around more,
					** it's easy to copy a one-piece
					** partition definition!)
					*/
#define DB_PARTF_KEY_N_PARTS	0x0004	/* Partitioning scheme is weakly
					** direct-plan compatible with master
					** table storage structure.
					** Rows with a given key might be in
					** multiple partitions, but we can
					** figure out which ones.
					*/
/* The struct/partition checker can figure out when partitioning "looks like"
** an extension of the storage structure, but at present nobody cares.
*/

    i2		extra;			/* Notused...filler */
    DB_PART_DIM dimension[1];		/* Dimension info array */
} DB_PART_DEF;

/*}
** Name: DB_TAB_ID - Table id.
**
** Description:
**      This structure defines an id which uniquely identifies a table.
**
**	For base tables and partitioned master tables, db_tab_base is
**	set and db_tab_index is zero.
**
**	For physical partitions of a base table, db_tab_base is the
**	same as the master, and db_tab_index is the partition ID with
**	the sign-bit set.  (Not negated - just the sign bit set.)
**
**	For secondary indexes, db_tab_base is the same as the base table,
**	and db_tab_index is the secondary index table ID.
**
**	So the proper test for:
**	    is a base table:  db_tab_index == 0
**	    is an index:      db_tab_index > 0
**	    is a partition:   db_tab_index < 0
**		(or db_tab_index & DB_TAB_PARTITION_MASK )
**
**	Normal table ID numbers cannot exceed 0x3ffffff, an arbitrary
**	constant in dm2uuti.c.
**
**	Temporary table ID numbers start at -65536 (0xffff0000) and
**	count up to -1, then wrap.
**
** History:
**     08-jul-85 (jeff)
**          written
**	19-nov-92 (rickh)
**	    Recast DB_SCHEMA_ID, DB_CONSTRAINT_ID, DB_RULE_ID, and DB_DEF_ID
**	    as DB_TAB_IDs since we allocate them that way and coerce them
**	    into IIDBDEPENDS tuples as such.
**	18-Jan-2004 (schka24)
**	    Our scheme for signed partition id's isn't going to work so well
**	    with unsigned tabid's -- make them i4.
**	26-Jan-2004 (jenjo02)
**	    Added DB_TAB_PARTITION_MASK define.
*/
typedef struct	_DB_TAB_ID
{
    i4	    db_tab_base;
    i4	    db_tab_index;
} DB_TAB_ID;

/* This constant is the maximum table ID number, and it can also be used
** for clearing out the sign bit (or any other flags we invent!) from
** a table ID number in db_tab_index.
*/
#define         DB_TAB_MASK           0x7FFFFFFF        /* Mask for Table ID */
#define         DB_TAB_ID_MAX         0x4643C824        /* Maximum table ID */
#define		DB_TAB_PARTITION_MASK 0x80000000	/* db_tab_index is a partition */
#define         DB_TEMP_TAB_ID_MIN    0x80000000        /* Temp Table min ID */
#define         DB_TEMP_TAB_ID_MAX    0xFFFFFFFF        /* Temp Table max ID */

/*}
** Name: DB_DEF_ID - ID for query text stored in IIDEFAULT relation
**
** Description:
**      This structure defines the id a default tuple as stored in IIDEFAULT.
**	Currently, it's an 8 byte quantity allocated the same way as a table id.
**
** History:
**     27-jul-92 (rickh)
**          written for FIPS CONSTRAINTS
**	19-nov-92 (rickh)
**	    Recast DB_SCHEMA_ID, DB_CONSTRAINT_ID, DB_RULE_ID, and DB_DEF_ID
**	    as DB_TAB_IDs since we allocate them that way and coerce them
**	    into IIDBDEPENDS tuples as such.  Added COPY_DEFAULT_ID.
**	25-jan-93 (rblumer)
**	    added several canonical DEF_IDs (for UNKNOWN, logical keys,
**	    INITIAL_USER, and DBA); added IS_CANON_DEF_ID macro.
**	9-mar-93 (rickh)
**	    Added canonical default ids for use by UPGRADEDB in cases where
**	    a UDT must be flagged and where the datatype is unrecognized.
*/
typedef struct _DB_TAB_ID	DB_DEF_ID;

/* macros for manipulating the canonical default values listed below
 */
#define SET_CANON_DEF_ID(var,defid)		\
	{  var.db_tab_base = defid;		\
	   var.db_tab_index = 0L;		\
	}

#define EQUAL_CANON_DEF_ID(var,defid)		\
    ((var.db_tab_base == defid) && (var.db_tab_index == 0L))

#define	COPY_DEFAULT_ID( source, target )		\
	target.db_tab_base = source.db_tab_base;	\
	target.db_tab_index = source.db_tab_index;

/*	for the canonical default values, db_def_id2 is 0 and db_def_id1
**	is one of the following:
**
**	(NOTE: DB_DEF_ID_0      is the INGRES default for numeric types;
**	       DB_DEF_ID_BLANK  is the INGRES default for char and date types;
**	       DB_DEF_ID_TABKEY is the INGRES default for the table_key  type;
**	       DB_DEF_ID_LOGKEY is the INGRES default for the object_key type;
**	       DB_DEF_NOT_SPECIFIED is the 'default' default for all datatypes,
**	       			      which is the NULL value (SQL92 semantics))
*/
#define DB_DEF_NOT_DEFAULT	0	/* NOT DEFAULT--mandatory column     */
#define DB_DEF_NOT_SPECIFIED	1	/* no default was specified ==> NULL */
#define DB_DEF_UNKNOWN		2	/* unknown - used for calculated
					** 	     columns in views
					*/
#define DB_DEF_ID_0		3	/* dbd_default_value = "0"   */
#define DB_DEF_ID_BLANK		4	/* dbd_default_value = "' '" */
#define DB_DEF_ID_TABKEY	5	/* dbd_default_value =  8 null bytes  */
#define DB_DEF_ID_LOGKEY	6	/* dbd_default_value = 16 null bytes  */
#define DB_DEF_ID_NULL		7	/* dbd_default_value = "NULL"         */
#define DB_DEF_ID_USERNAME	8	/* dbd_default_value = "CURRENT_USER" */
#define DB_DEF_ID_CURRENT_DATE	9	/* dbd_default_value = "CURRENT_DATE" */
#define DB_DEF_ID_CURRENT_TIMESTAMP 10	/* dbd_default_value = "CURRENT_TIMESTAMP" */
#define DB_DEF_ID_CURRENT_TIME	11	/* dbd_default_value = "CURRENT_TIME" */
#define DB_DEF_ID_SESSION_USER	12	/* dbd_default_value = "SESSION_USER" */
#define DB_DEF_ID_SYSTEM_USER	13	/* dbd_default_value = "SYSTEM_USER"  */
#define DB_DEF_ID_INITIAL_USER	14	/* dbd_default_value = "INITIAL_USER" */
#define DB_DEF_ID_DBA		15	/* dbd_default_value = "$DBA"         */
#define	DB_DEF_ID_LOCAL_TIMESTAMP 16	/* dbd_default_value = "LOCAL_TIMESTAMP" */
#define	DB_DEF_ID_LOCAL_TIME	17	/* dbd_default_value = "LOCAL_TIME"   */
#define	DB_DEF_ID_FALSE         18      /* dbd_default_value = "FALSE"   */
#define	DB_DEF_ID_TRUE          19      /* dbd_default_value = "TRUE"   */

/*
** when converting IIATTRIBUTE on the first 65 connection to a 64 database,
** DMF will fill in the following two values.  DB_DEF_ID_NEEDS_CONVERTING
** is used to flag UDTs:  later on during UPGRADEDB, QEF will allocate
** default IDs for these UDTs and update the attribute's IIATTRIBUTE
** record accordingly.  DB_DEF_ID_UNRECOGNIZED is a bit of paranoia:  it
** could happen that a datatype turns up that the DMF conversion logic
** doesn't recognize;  it seems better to flag these values than to ignore
** them or force them to some known type or error out.
*/

#define	DB_DEF_ID_NEEDS_CONVERTING	97
#define	DB_DEF_ID_UNRECOGNIZED		98

#define DB_MAX_RESERVED_DEF_ID	99	/* largest id reserved for system */

#define IS_CANON_DEF_ID(var)		\
    ((var.db_tab_base <= DB_MAX_RESERVED_DEF_ID) && (var.db_tab_index == 0L))


/*}
** Name: DB_IIDEFAULT - tuple in iidefault
**
** Description:
**	Layout of tuple in iidefault system relation.
**
**
** History:
**     27-jul-92 (rickh)
**          written for FIPS CONSTRAINTS
**     26-oct-92 (rblumer)
**          split IS_LITERAL into QUOTED_LITERAL and UNQUOTED_LITERAL
**	05-dec-92 (rblumer)
**	    Remove deftype field and related define's.
**	25-jan-93 (rblumer)
**	    changed DB_TEXT_STRING to a u_i2 and a char array;
**	    since this is an on-disk structure, using DB_TEXT_STRING causes
**	    padding/alignment problems on some systems. 
**	22-feb-93 (rickh)
**	    Upped the length of the value string in IIDEFAULT by 1 byte to
**	    force 4 byte alignment.  Also accounted for the null indicator
**	    byte.
*/
#define	DB_MAX_COLUMN_DEFAULT_LENGTH	1501

typedef struct _DB_IIDEFAULT
{
    DB_DEF_ID	dbd_def_id;
    u_i2	dbd_def_length;	    /* actual length of text in dbd_def_value
				    ** (this is a varchar field in catalog)
				    */
    char	dbd_def_value[DB_MAX_COLUMN_DEFAULT_LENGTH + 1 ];
				    /* we add 1 to account for the null
				    ** indicator byte
				    */
}   DB_IIDEFAULT;

/*}
** Name: DB_ATT_NAME - Attribute name
**
** Description:
**      This structure defines an attribute (column) name.
**
** History:
**     08-jul-85 (jeff)
**          written
*/
typedef struct _DB_ATT_NAME
{
    char            db_att_name[DB_ATT_MAXNAME];    /* attribute name */
} DB_ATT_NAME;

/*}
** Name: DB_ATTS - Attribute descriptor.
**
** Description:
**	This structure is used to define attributes of a record.
**  This must be the same as the DMP_ATTS defined by DMF.  It
**  describes each attribute of a record.
**
** History:
**	10-jun-86 (jennifer)
**	    Initial creation.
**	22-feb-87 (thurston)
**	    Added the ADT_F_NDEFAULT constant.
**	07-apr-87 (derek)
**	    Added the .key_offset and .extra fields.
**	21-oct-92 (rickh)
**	    Added default id fields.  Moved to ADF.H.  Replaced all
**	    references to DMP_ATTS with this structure.
**	7-jun-96 (stephenb)
**	    Add replicated field.
**      23-jul-1996 (ramra01 for bryanp)
**          Add new DB_ATTS fields for alter table support. 
**	3-jun-97 (stephenb)
**	    Add rep_key_seq field.
**	3-mar-2004 (gupsh01)
**	    Added ver_altcol for alter table alter column support. 
**	9-dec-04 (inkdo01)
**	    Added collID to define collation ID.
**	29-Sept-09 (troal01)
**		Added geomtype and srid.
*/

typedef struct _DB_ATTS
{
    i4              offset;             /* Offset in bytes. */
    i4              length;             /* Length of attribute. */
    i2              type;               /* Type of attribute. */
    i2              precision;          /* Precision of attribute. */
    i2              key;                /* Indicates part of key. */
    i2              flag;               /* Reserved for future use. */
    i4		    key_offset;		/* Offset in bytes to key field. */
    DB_DEF_ID	    defaultID;		/* default id */
    DB_IIDEFAULT    *defaultTuple;	/* points at storage for the default,
					** null if default tuple not made yet */
    i2		    collID;		/* collation ID for character flds */
    bool	    replicated;		/* is attribute replicated ? */
    i4	    	    rep_key_seq;	/* sequence in replication key */
    i4	    intl_id;		/* unique internal column id */
    i4	    ver_added;		/* Version Added metadata value */
    i4	    ver_dropped;	/* Version Dropped metadata value */
    i4	    val_from;		/* previous internal_id value */
    i4	    ordinal_id;		/* ordinal column id adjusted by any
					** add or drop column actions
					*/
    i4	    ver_altcol;			/* Version Modified metadata value */
    i4		srid;				/* Spatial reference id */
    i2		geomtype;			/* Geometry data type */
    i2	            attnmlen;	/* blank trimmed length of name (future) */
    char	    *attnmstr;
}   DB_ATTS;


/*}
** Name: DB_DEBUG_CB - Debug control block.
**
** Description:
**      Each facility should have an entry point that uses this control block
**      to determine what trace flag should be set.
**	Cannot be put into dbdbms.h because of adf.
**
** History:
**     04-dec-85 (jeff)
**          written
*/
typedef struct _DB_DEBUG_CB
{
    i4	    db_trswitch;	/* Turn trace point on or off? */
#define                 DB_TR_NOCHANGE  0L  /* Don't affect trace point */
#define			DB_TR_ON	1L  /* Turn the trace point on */
#define			DB_TR_OFF	2L  /* Turn the trace point off */
    i4         db_trace_point;     /* Trace point id. (flag # in cmd) */
    i4         db_value_count;     /* Number of values in value array */
    i4         db_vals[2];         /* Values (interpreted by facility) */
}   DB_DEBUG_CB;
/******************************************************************************/
/* The above structures should be in dbdbms.h, but are placed here due to
** tuple compare routines in ADF used by DMF */
/******************************************************************************/

/*}
** Name: DB_MAX_HIST_LENGTH - maximum length of a histogram boundary value
**
** Description:
**	This defines the maximum length of a single histogram boundary value
**	(also known as an "interval").
**
** History:
**      18-jan-93 (rganski)
**          Initial creation.
[@history_template@]...
*/
#define             DB_MAX_HIST_LENGTH              1950

/*
** Name: Subject status bits and masks.
**
** Description:
**     These define various privilege and status bits used for subjects, 
**     currently set for users, profiles and roles. 
**     They are placed here instead of dudbms.qsh as code in common and 
**     front also uses them.
**
** History:
**	8-jul-94 (robf)
**          Moved from dudbms.qsh
*/

#define	DU_UCREATEDB	1	/* 0x00000001 - can create data bases */
#define	DU_UDRCTUPDT	2	/* 0x00000002 - can specify direct update */
#define	DU_UUPSYSCAT	4	/* 0x00000004 - can update system catalogs */
#define	DU_UTRACE	16	/* 0x00000010 - can use trace flags */
#define	DU_UQRYMODOFF	32	/* 0x00000020 - can turn off qrymod */
#define	DU_UAPROCTAB	64	/* 0x00000040 - can use arbitrary proctab */
#define	DU_USECURITY	32768	/* 0x00008000 - can perform security functions
						replaces super user. */
#define	DU_UOPERATOR	512	/* 0x00000200 - Can perform operator type
						operations such as backup
						without being DBA.  */
#define	DU_UAUDIT	1024	/* 0x00000400 - AUDIT all activity for this
						user. 	*/
#define	DU_USYSADMIN	2048	/* 0x00000800 - Can perform system
						administration functions,
						such as creating locations. */
#define	DU_UDOWNGRADE	4096	/* 0x00001000 - Can downgrade data. */
#define DU_UAUDITOR		0x00002000 	/* Audit */
#define DU_UALTER_AUDIT		0x00004000 	/* Can alter audit system */
#define	DU_UUSUPER        	0x00008000 	/* ingres superuser OBSOLETE */
#define DU_UMAINTAIN_USER	0x00010000 	/* Can maintain users */
#define DU_UWRITEDOWN    	0x00020000 	/* WRITE_DOWN privilege */
#define DU_UINSERTDOWN    	0x00040000 	/* INSERT_DOWN privilege */
#define DU_UWRITEUP    		0x00080000 	/* WRITE_UP privilege */
#define DU_UINSERTUP 	   	0x00100000 	/* INSERT_UP privilege */
#define DU_UMONITOR		0x00200000	/* DBMS MONITOR privilege */
#define DU_UWRITEFIXED          0x00800000	/* WRITE_FIXED privilege*/
#define DU_UAUDIT_QRYTEXT       0x01000000	/* AUDIT Query text for this 
						** subject
						*/
/*
** All SECURITY_AUDIT privileges
*/
#define DU_UAUDIT_PRIVS (DU_UAUDIT|DU_UAUDIT_QRYTEXT)

/*
** All privileges, all privileges one might need.
*/
# define DU_UALL_PRIVS (DU_USECURITY |\
			DU_UAUDITOR |\
			DU_UMAINTAIN_USER |\
			DU_UALTER_AUDIT |\
			DU_UOPERATOR |\
			DU_UCREATEDB |\
			DU_UTRACE |\
			DU_USYSADMIN |\
			DU_UINSERTDOWN |\
			DU_UINSERTUP |\
			DU_UWRITEDOWN |\
			DU_UWRITEUP |\
			DU_UWRITEFIXED \
			)

/*
** Name: INTXLATE - Integer code-to-name translator structure
**
** Description:
**	With all of the coded values flying around here, it's often
**	useful to have a lookup table to translate the codes into
**	text strings, for debugging or error messages.  In those
**	cases where a direct array indexing is impossible or inadvisable,
**	this lookup utility structure can be used to define a bunch
**	of code-to-name mappings.
**
** History:
**	25-Jan-2004 (schka24)
**	    Everyone needs a lookup structure definition!
*/

 typedef struct _INTXLATE
{
	i4	code;		/* The coded value */
	char	*string;	/* Its textual translation */
} INTXLATE;

#endif /* common_include */
