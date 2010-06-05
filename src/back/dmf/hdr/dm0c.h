/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

/**
** Name: DM0C.H - Database configuration file structures and routines.
**
** Description:
**      This file describes the structure of the configuration file and
**	the types configuration records that reside in the configuration file.
**	It also defines the calls to the configuration file management routines.
**
**	For information on the format of the config file - see the CONFIG FILE
**	FORMAT document.
**
** History: 
**      03-dec-1985 (derek)
**          Created new for Jupiter.
**	21-jan-1989 (mikem)
**	    added DSC_NOSYNC_MASK
**	12-may-1989 (anton)
**	    Local collation support
**	28-jun-1989 (rogerk)
**	    Added new config version #'s.
**	16-aug-1989 (derek)
**	    Added DSC_B1_SECURE to denote a Multi-level secure database.
**	20-aug-1989 (ralph)
**	    Added DM0C_CVCFG for config file conversion
**	 6-dec-1989 (rogerk)
**	    Added DM0C_DUMP flag.
**	    Added declaration of dm0c_dmp_filename.
**	17-may-90 (bryanp)
**	    Added DSC_DUMP_DIR_EXISTS flag to dsc_status to communicate fact
**	    that a dump directory has been created (once a dump directory has
**	    been created, we begin to keep an up-to-date backup of the config
**	    file in the dump directory area).
**	    Added DM0C_TRYDUMP flag to dm0c_open() to inform it that it should
**	    open and read the config file in the dump directory.
**	24-sep-1990 (rogerk)
**	    Merged 6.3 changes into 6.5.
**	25-feb-1991 (rogerk)
**	    Added DSC_DISABLE_JOURNAL config file flag to indicate that
**	    journaling has been disabled (by alterdb -disable_journaling).
**	    This was added as part of the Archiver Stability project.
**	25-mar-1991 (rogerk)
**	    Added DSC_NOLOGGING config file flag to indicate that the database
**	    is being acted upon in a "set nologging" state.  If an error
**	    occurs which requires recovery while in this state, the database
**	    will be marked inconsistent.  Added DSC_ERR_NOLOGGING, OPN_NOLOGGING
**	    to indicate this type of inconsistency.
**	05-may-1992 (ananth)
**	    Increasing IIrelation tuple size project.
**	 7-jul-1992 (rogerk)
**	    Prototyping DMF.
**	29-August-1992 (rmuth)
**	    File extend project, add flag indicating if the DB's 
**	    DBMS catalogs have been converted or not.
**	22-sep-1992 (ananth)
**	    Get rid of core catalog section from config file.
**	14-dec-1992 (rogerk)
**	    Reduced Logging Project: Changed database inconsistency values.
**	30-feb-1993 (rmuth)
**	    Prototyping DI showed that the cnf_di field is of type (char *)
**	    should be (DI_IO *)
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	24-may-1993 (robf)
**	    Secure 2.0: Add security id support, remove xORANGE
**	26-jul-1993 (bryanp)
**	    Replace all uses of DM_LKID with LK_LKID.
**	22-nov-1993 (jnash)
**	    Add jnl_first_jnl_seq to track sequence number of last
**	    checkpoint from which a rolldb +j may occur.
**      13-Dec-1994 (liibi01) 
**          Cross integration from 6.4 (Bug 56364).
**          Added DSC_RFP_FAIL to mark database inconsistent due to
**          rollforward failure.
**      24-jan-1195 (stial01)
**          BUG 66473: ckp_mode changed to bit pattern, added CKP_TABLE
**	22-jan-1997 (hanch04)
**	    New DSC version for OI2.0, DSC_V4
**	04-feb-1998 (hanch04)
**	    New DSC version for OI2.5, DSC_V5
**	    Replaced DM0C_OJNL with DM0C_V3JNL,DM0C_V4JNL, and added DM0C_V4DMP
**      28-may-1998 (stial01)
**          DM0C_DSC: Config file changes for FUTURE (3.0) support of VPS
**          system catalogs, added dsc_ii*_relpgsize.
**	18-dec-1998 (hanch04)
**	    Increase CKP_OHISTORY 16 to CKP_HISTORY 99
**      15-Mar-1999 (hanal04)
**          - Added DM0C_CONFIG_WAIT, timeout value for use with lock 
**          requests on config file. 
**          - Added DM0C_TIMEOUT, used to specify the use of a timeout
**          When requesting a lock on the config file. b55788.
**      27-May-1999 (hanal04)
**          Removed and reworked above change for bug 55788. Added
**          DM0C_CNF_LOCKED to indicate caller has already locked the
**          CNF file. b97083.
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      21-mar-2001 (stial01)
**          Made CNF_V6, DSC_V6 current config file version for ingres 2.6
**      27-mar-2001 (stial01)
**          Moved dm0c_ucollation to reserved part of DM0C_DSC
**	18-Dec-2003 (schka24)
**	    Bump DMF version for partitioned tables.
**      12-Apr-2004 (stial01)
**          Define cnf_length as SIZE_TYPE.
**	29-Dec-2004 (schka24)
**	    Remove core-catalog versions (DSC_xxx) to dmf.h for upgradedb
**	    visibility.
**      05-Aug-2005 (hanje04)
**          Back out change 478041 as it should never have been X-integrated
**          into main in the first place.
**      25-oct-2005 (stial01)
**          New db inconsistency codes DSC_INDEX_INCONST, DSC_TABLE_INCONST
**      12-apr-2007 (stial01)
**          Added support for rollforwarddb -incremental
**      09-oct-2007 (stial01)
**          Added DM0C_RFP_NEEDLOCK
**      14-may-2008 (stial01)
**          Removed DM0C_INCR_RFP_INCONS
**	18-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code.
**      03-dec-2008 (stial01)
**          Fixed define DM0C_PGS_HEADER for underlying MAXNAME dependency
**      21-apr-2009 (stial01)
**          Added DM0C_ODSC_32, DM0C_V6EXT for conversion to DM0C_V7
**	12-Nov-2009 (kschendel) SIR 122882
**	    Re-interpret all cmptlvl's as a u_i4 instead of a char[4].
**	    This does not change the config file layouts or versions.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add DM0C_UPDATE_JNL flag for dm0c_close,
**	    set if journal content has changed.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**	29-APr-2010 (kschendel)
**	    Bump the config version for long ID's.
**/

/*
**  Forward and/or External typedef/struct references.
*/

typedef struct _DM0C_CNF	DM0C_CNF;
typedef struct _DM0C_DSC	DM0C_DSC;
typedef struct _DM0C_ODSC	DM0C_ODSC;
typedef struct _DM0C_ODSC_32	DM0C_ODSC_32;
typedef struct _DM0C_EXT	DM0C_EXT;
typedef struct _DM0C_V5EXT	DM0C_V5EXT;
typedef struct _DM0C_V6EXT	DM0C_V6EXT;
typedef struct _DM0C_OEXT	DM0C_OEXT;
typedef struct _DM0C_V3JNL	DM0C_V3JNL;
typedef struct _DM0C_V4JNL	DM0C_V4JNL;
typedef struct _DM0C_JNL	DM0C_JNL;
typedef struct _DM0C_V4DMP	DM0C_V4DMP;
typedef struct _DM0C_DMP	DM0C_DMP;
typedef struct _DM0C_TAB	DM0C_TAB;
typedef struct _DM0C_RELATION	DM0C_RELATION;


/*
**  Forward function references.
*/

/*
**  Defines of constants.
*/

/*
**  Flag values of DM0C_OPEN
*/

#define	    DM0C_TRYRFC		0x01
#define	    DM0C_NOLOCK		0x02
#define	    DM0C_PARTIAL	0x04
#define	    DM0C_CVCFG		0x08
#define	    DM0C_TRYDUMP	0x10
#define	    DM0C_READONLY	0x20
#define	    DM0C_IO_READ	0x40
#define     DM0C_CNF_LOCKED     0x80
#define     DM0C_RFP_NEEDLOCK  0x100

/*
**  Flag values for DM0C_CLOSE.
*/

#define	    DM0C_UPDATE		0x01
#define	    DM0C_LEAVE_OPEN	0x02
#define	    DM0C_COPY		0x04
#define	    DM0C_DUMP		0x08
#define	    DM0C_UPDATE_JNL	0x10

/*
** Number of pages to read when requesting just the descriptor
** information of the config file.
*/

#define	    DM0C_PGS_HEADER (((sizeof(DM0C_DSC)+sizeof(DM0C_JNL)+sizeof(DM0C_DMP)+5*(sizeof(DM0C_EXT)))/DM_PG_SIZE ) + 1)

/*
** Number of bytes in default config file created by CREATEDB
*/
#define	    DM0C_FILE_LEN	(DM0C_PGS_HEADER * DM_PG_SIZE)


/*}
** Name: DM0C_CNF - Configuration file control block.
**
** Description:
**      This structure descibes the in-memory format of the configuration
**	file.
**
** History:
**      26-oct-1986 (Derek)
**          Created for Jupiter.
**	24-jan-1989 (EdHsu)
**	    Added DM0C_DMP control block to the config file to support
**	    online backup.
**	30-jan-1992 (bonobo)
**	    Removed the redundant typedefs to quiesce the ANSI C 
**	    compiler warnings.
**	22-sep-1992 (ananth)
**	    Get rid of core catalog section from config file.
**	26-jul-1993 (bryanp)
**	    Replace all uses of DM_LKID with LK_LKID.
**	05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with
**	    DM_SVCB.
**	03-may-2005 (hayke02)
**	    Add pool_type to match obj_pool_type in DM_OBJECT.
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	    Define CNF_CB as DM_CNF_CB.
[@history_template@]...
*/
struct _DM0C_CNF
{
    DMP_DCB         *cnf_q_next;            
    DMP_DCB         *cnf_q_prev;
    SIZE_TYPE	    cnf_length;		    /* Length of the control block. */
    i2              cnf_type;               /* Type of control block for
                                            ** memory manager. */
#define                 CNF_CB              DM_CNF_CB
    i2              cnf_s_reserved;         /* Reserved for future use. */
    PTR	            cnf_l_reserved;         /* Reserved for future use. */
    PTR             cnf_owner;              /* Owner of control block for
                                            ** memory manager.  CNF will be
                                            ** owned by the server. */
    i4         cnf_ascii_id;           /* Ascii identifier for aiding
                                            ** in debugging. */
#define                 CNF_ASCII_ID        CV_C_CONST_MACRO('#', 'C', 'N', 'F')
    DMP_DCB	    *cnf_dcb;		    /* DCB for this database. */
    DM0C_JNL	    *cnf_jnl;		    /* Journal information. */
    DM0C_DMP	    *cnf_dmp;		    /* Dump information	*/
    DM0C_DSC	    *cnf_dsc;		    /* Database information. */
    DM0C_EXT	    *cnf_ext;		    /* Extent information. */
    i4	    cnf_bytes;		    /* Number of bytes in cnf_data */
    i4	    cnf_c_ext;		    /* Number of extent items. */
    LK_LKID	    cnf_lkid;		    /* Configuration file lock. */
    i4	    cnf_lock_list;	    /* Lock list used. */
    i4	    cnf_free_bytes;	    /* Number of bytes left to add
                                            ** extent items. Zeroed if entire
					    ** config file is not read. */
    DI_IO	    *cnf_di;		    /* DI context for config file. */
    char	    *cnf_data;		    /* Configuartion file contents. */
};

/*}
** Name: DM0C_DSC - Configuration file database decription entry.
**
** Description:
**      This structure defines the basic information known about a
**	database by DMF.
**
** History:
**     03-dec-1985 (derek)
**	  Created new for Jupiter.
**     15-jun-1988 (teg)
**	  added new fields to dm0c_dsc used in "patching" config file to valid
**     21-jan-1989 (mikem)
**	  added DSC_NOSYNC_MASK
**     24-jan-1989 (EdHsu)
**	  Added DSC_DUMP to dsc_status to support online backup.
**     12-may-1989 (anton)
**	  Added collation for database
**     17-may-90 (bryanp)
**	    Added DSC_DUMP_DIR_EXISTS flag to dsc_status to communicate fact
**	    that a dump directory has been created (once a dump directory has
**	    been created, we begin to keep an up-to-date backup of the config
**	    file in the dump directory area).
**	25-mar-1991 (rogerk)
**	    Added DSC_NOLOGGING config file flag to indicate that the database
**	    is being acted upon in a "set nologging" state.  If an error
**	    occurs which requires recovery while in this state, the database
**	    will be marked inconsistent.  Added DSC_ERR_NOLOGGING, OPN_NOLOGGING
**	    to indicate this type of inconsistency.
**	05-may-1992 (ananth)
**	    Increasing IIrelation tuple size project.
**	29-August-1992 (rmuth)
**	    File extend add a flag indicating if the dbms catalogs have 
**	    been converted. DSC_65_SMS_DBMS_CATS_CONV_DONE
**	22-sep-1992 (ananth)
**	    Get rid of core catalog section from config file.
**	21-jun-1993 (jnash)
**	    Add DSC_TRACE_INCONS.
**	22-jan-1997 (hanch04)
**	    New DSC version for OI2.0, DSC_V4
**	03-feb-1998 (hanch04)
**	    New DSC version for OI2.5, DSC_V5, CNF_V5
**	19-mar-2001 (stephenb)
**	    Add dm0c_ucollation for unicode collation.
**	25-Aug-2004 (schka24)
**	    New iiattribute conversion flag to go along with V7.
**	13-Jun-2006 (jenjo02)
**	    Add DSC_IIATT_V9_CONV_DONE, DSC_IIIND_V9_CONV_DONE conversion
**	    flags for Ingres2007.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Added DSC_NOMVCC status, DSC_STATUS,
**	    DSC_INCONST_CODE defines.
**	29-Apr-2010 (kschendel)
**	    Rather than adding Vx_CONV_DONE flags indefinitely, define a
**	    pair of _A_{att,rel}_DONE and _B_{att,rel}_DONE flags, plus
**	    one more flag saying which is to be used (A or B).  When
**	    a db is upgraded, it uses (say) the A flags and clears the
**	    B flags.  The next time it will be the other way around.
**	    Bump up the config version for long ID's (all the definitions
**	    were done, just needed to change VCURRENT).
*/
struct _DM0C_DSC
{
    i4         length;             /* Length of entry. */
    i4         type;               /* Type of entry. */
#define                 DM0C_T_DSC      6L
    i4         dsc_cnf_version;    /* Version of this config file.
					** High 2 bytes give major version
					** number and low 2 bytes give minor
					** version. */
#define			CNF_V1		0x00010001  /* original config format */
#define			CNF_V2		0x00020001  /* jan/88 32-index format */
#define			CNF_V3		0x00030001  /* jun/89 online dump */
#define			CNF_V4		0x00040001  
#define			CNF_V5		0x00050001  /* 64-bit file sys */
#define			CNF_V6		0x00060001  /* raw partition support */
						    /* and new page types */
#define			CNF_V7		0x00070001  /* long names */
#define			CNF_VCURRENT	CNF_V7	    /* current config format */
    i4	    dsc_c_version;	/* Version of DMF that created this
				** database.  (essentially means
				** core catalog version).  The
				** actual defines are in dmf.h so
				** that upgradedb can see them too.
				*/
    i4	    dsc_version;	/* Current version of database.
				** This actually turns out to be somewhat
				** useless, because it's been confused with
				** dsc_c_version.  I (kschendel) think this
				** was originally meant to track the
				** dbcmptlvl in iidbdb.iidatabase, but
				** nothing updates dsc_version.
				** Do not use.
				*/
    i4	    dsc_status;		/* Database status. */
#define			DSC_VALID	    0x01
#define			DSC_JOURNAL	    0x02
#define			DSC_CKP_INPROGRESS  0x04
#define			DSC_DUMP	    0x08
#define			DSC_ROLL_FORWARD    0x10
#define			DSC_SM_INCONSISTENT 0x20
#define			DSC_PARTIAL_ROLL    0x40
#define			DSC_NOMVCC 	    0x80
				/* MVCC usage in database is disabled */
#define			DSC_PRETEND_CONSIST 0x100 /* pretend DSC_VALID bit set*/
#define			DSC_DUMP_DIR_EXISTS 0x200   /* dump direc. created */
#define			DSC_DISABLE_JOURNAL 0x400
				/* Journaling has been disabled - presumably in
				** order to recover from a journaling problem */
#define			DSC_NOLOGGING 	    0x800
				/* Database is open by a Set Nologging session.
				** If an error is encountered by that session
				** the database will be marked inconsistent. */

/* A general note about the "conversion done" flags: they are only
** relevant if the core-catalog version dsc_c_version is some non-current
** version.  The flags need not be set in newly created databases.
** Their purpose is simply to skip non-redoable steps in core catalog
** conversions.
** Historically, the conversion-done flags were left on after conversion,
** and we simply defined new ones.  Rather than use up bits this way, I've
** changed the scheme so that there are two flag pairs, used alternately.
** Initially a database is on the "A" cycle; the "B" flag is always clear
** when the database is new.  If the database goes through a core catalog
** upgrade, the _A_CONV_DONE flags are tested and set; the _B_CONV_DONE
** flags are forcibly cleared; and the "B" flag is set.  The next time
** through a core catalog upgrade, the opposite happens:  the _B_ flags
** are used, the _A_ flags are cleared, and the "B" flag is cleared.
** In this way the database flip-flops between using the two flag pairs.
**
** (The A vs B flag has to be specific to each database, because some
** databases might skip a version.  E.g. v9 db1 might be upgraded to v10 and
** then v11, while v9 db2 might go straight to v11.  It's unlikely but
** certainly possible.)
*/
#define			DSC_B_IIREL_CONV_DONE 0x1000
				/* "B" cycle:  is iirelation conversion done?
				** Cleared for new DB's and the A cycle.
				*/

#define			DSC_65_SMS_DBMS_CATS_CONV_DONE	0x2000
				/* If set indicates that the DBMS catalogs
				** have been converted to 6.5 Space management
				** scheme.  Core-catalog conversions that
				** rewrite the catalog may clear this flag
				** so that SMS can be redone.
				*/

#define			DSC_B_IIATT_CONV_DONE 0x4000
				/* "B" cycle:  is iiattribute conversion done?
				** Cleared for new DB's and the A cycle.
				*/

/* 0x8000, 0x10000 not used, cleared as of config V7 / DSC V9 */
#define			DSC_NOTUSED (0x8000 | 0x10000)
				/* Note, for DSC_V10 and future, these two
				** could be used to track iiindex conversion
				** done if an iiindex conversion is needed.
				*/

#define			DSC_A_IIREL_CONV_DONE 0x20000
				/* "A" cycle:  is iirelation conversion done?
				** Cleared for new DB's and the B cycle.
				*/

#define			DSC_A_IIATT_CONV_DONE 0x40000
				/* "A" cycle:  is iiattribute conversion done?
				** Cleared for new DB's and the B cycle.
				*/

#define			DSC_INCREMENTAL_RFP    0x80000

#define			DSC_CONV_B	0x100000
				/* Set if the database is on the "B" conversion
				** cycle.  Always cleared initially.  See
				** the conversion-done discussion above.
				*/

#define DSC_STATUS "\
VALID,JOURNAL,CKP,DUMP,ROLL_FORWARD,SM_INCON,\
PARTIAL_ROLL,NOMVCC,PRETEND,CFG_BACKUP,JOURNAL_DISABLED,\
NOLOGGING,B_IIREL_CONV,65_CATS,B_IIATT_CONV,nu8000,nu10000,\
A_IIREL_CONV,A_IIATT_CONV,INCR_RFP,B_CONV_CYCLE"

    i4         dsc_open_count;     /* Count of concurrent server opens.
                                        ** This is used to determine if a 
                                        ** a database is inconsistent.  For 
				        ** first openner this count must be 
                                        ** zero or database may have become 
                                        ** inconsistent because recovery
                                        ** could not read log to recover.  Any
                                        ** database open at the time of the
                                        ** recovery failure must be considered
                                        ** inconsistent. */
    u_i4       dsc_sync_flag;      /* Used for Unix systems to determine
                                        ** if synchronous writes should be
                                        ** guaranteed. */
#define                       DSC_NOSYNC_MASK  0x0001	/* Do not sync if set */
    i4	    dsc_type;		/* Type of database. */
#define			DSC_PRIVATE	1L
#define			DSC_PUBLIC	2L
#define			DSC_DISTRIBUTED	3L
    i4	    dsc_access_mode;	/* Database access mode. */
#define			DSC_READ	1L
#define			DSC_WRITE	2L
    i4	    dsc_dbid;		/* Database identifier(date). */
    i4	    dsc_sysmod;		/* Sysmod date. */
    i4	    dsc_ext_count;	/* Number of extents. */
    u_i4       dsc_last_table_id;  /* Last table id used. */
    i4	    dsc_dbaccess;	/* Database flags in iidbdb */
    i4	    dsc_dbservice;	/* Database service permissions */
    u_i4    dsc_dbcmptlvl;	/* Database major compatability level */
    i4	    dsc_1dbcmptminor;	/* Database minor compatability level */
    i4	    dsc_patchcnt;	/* # times config file was 'patched'
					** by setting dsc_status || DSC_VALID */
    i4	    dsc_1st_patch;	/* time (in #secs since 1-Jan-1970) of
					** 1st config file patch to DSC_VALID */
    i4	    dsc_last_patch;	/* time (in #secs since 1-Jan-1970) of
					** last config file patch to DSC_VALID*/
    i4	    dsc_inconst_code;	/* indicate which logic set database
					** inconsistent */
#define		DSC_OPENDB_INCONSIST	1L /* set by the RCP when it cannot
					    ** recover a database because the
					    ** database cannot be opened. */
#define		DSC_RECOVER_INCONST	2L  /* set by the RCP when it fails
					    ** to recover a database due to 
					    ** an unexpected LG or Recovery
					    ** protocol problem. */
#define		DSC_REDO_INCONSIST	3L  /* set by the RCP when it fails to
					    ** recover a database due to an
					    ** error in REDO processing. */
#define		DSC_UNDO_INCONSIST	4L  /* set by the RCP when it fails to
					    ** recover a database due to an
					    ** error in UNDO processing. */
#define		DSC_DM2D_ALREADY_OPEN	5L  /* set by dm2d_open_db (dm2d.c) when
					    ** this is the 1st time this db is
					    ** being opened, but the config file
					    ** open count is nonzero.  This
					    ** implies dmfrecovery could not 
					    ** read config file during attempted
					    ** recovery */
#define		DSC_WILLING_COMMIT	7L  /* set by RCP when unable to restore
					    ** a transaction back to the willing
					    ** commit state.
					    */
#define		DSC_ERR_NOLOGGING	8L  /* database was marked inconsistent
					    ** because a transaction failed
					    ** while the database was in
					    ** Set Nologging state.
					    */
#define		DSC_OPN_NOLOGGING	9L  /* database was marked inconsistent
					    ** since when we went to open the
					    ** database for the first time we
					    ** saw that the nologging bit had
					    ** been left on - meaning that
					    ** a session exited abnormally.
					    */
#define		DSC_TRACE_INCONS	10L /* database marked inconsistent
					    ** via DM1435.
					    */

#define         DSC_RFP_FAIL            11L /* database was marked inconsistent
                                            ** because rollforward failed
                                            ** ( i.e. restore from tape with
                                            ** wrong sequence )
                                            */
#define		DSC_INDEX_INCONST	12  /* database was marked inconsistent
					    ** because RCP offline recovery
					    ** failed in redo/undo recovery
					    ** for a secondary index
					    ** Index marked inconsistent.
					    ** See iircp.log for details
					    ** OR
					    ** rollforwarddb failed in dmp/jnl
					    ** processing for a secondary index
					    ** Index marked inconsistent.
					    ** See errlog.log or rfpdb.dbg
					    ** for details
					    */

#define		DSC_TABLE_INCONST	13  /* database was marked inconsistent
					    ** because rollforwarddb failed in
					    ** dmp/jnl processing for a table.
					    ** The table was marked inconsistent
					    ** See errlog.log, rfpdb.dbg
					    ** for details
					    */
#define DSC_INCONST_CODE "\
0,OPENDB,RECOVER,REDO,UNDO,ALREADY_OPEN,6,WILLING_COMMIT,\
ERR_NOLOGGING,OPN_NOLOGGING,TRACE_INCONS,RFP_FAIL,\
INDEX_INCONS,TABLE_INCONS"

    i4	    dsc_inconst_time;	/* time (#secs since 1-Jan-1970) of when
					** config file status set ~DSC_VALID */
    i4	    dsc_iirel_relprim;	/* Relprim value for iirelation. */
    DB_DB_NAME	    dsc_name;		/* Name of the database. */
    DB_OWN_NAME	    dsc_owner;		/* Owner of the database. */
    char	    dsc_collation[DB_COLLATION_MAXNAME];/* collation name */
    char	    dsc_extra1[8];	/* Unused now */
    i4         dsc_iirel_relpgsize; /* Relpgsize value for iirelation */
    i4         dsc_iiatt_relpgsize;  /* Relpgsize value for iiattribute */
    i4         dsc_iiind_relpgsize;  /* Relpgsize value for iiindexes  */
    i2         dsc_iirel_relpgtype;  /* Relpgtype value for iirelation */
    i2         dsc_iiatt_relpgtype;  /* Relpgtype value for iiattribute */
    i2         dsc_iiind_relpgtype;  /* Relpgtype value for iiindexes  */
    char       dsc_ucollation[DB_COLLATION_MAXNAME]; /* unicode collation */
    char	    dsc_extra[6];	/* Future expansion. */
};

/*}
** Name: DM0C_ODSC_32 - Database decription entry in config files with
**           cnf_version CNF_V4, CNF_V5, CNF_V6
**
** Description:
**      This structure defines the basic information known about a
**	database by DMF.
**
** History:
**	21-apr-2009 (stial01)
**	    Created for increasing DB_MAXNAME project.
**	30-Apr-2010 (kschendel)
**	    Fix goof in dbname definition.
*/
struct _DM0C_ODSC_32
{
    i4      length;             /* Length of entry. */
    i4      type;               /* Type of entry. */
    i4      dsc_cnf_version;    /* Version of config file */
    i4	    dsc_c_version;	/* Version of DMF that created the db */ 
    i4	    dsc_version;	/* Current version of database */
    i4	    dsc_status;		/* Database status. */
    i4      dsc_open_count;     /* Count of concurrent server opens */
    u_i4    dsc_sync_flag;      /* Unix systems sync writes guaranteed */
    i4	    dsc_type;		/* Type of database. */
    i4	    dsc_access_mode;	/* Database access mode. */
    i4	    dsc_dbid;		/* Database identifier(date). */
    i4	    dsc_sysmod;		/* Sysmod date. */
    i4	    dsc_ext_count;	/* Number of extents. */
    u_i4       dsc_last_table_id;  /* Last table id used. */
    i4	    dsc_dbaccess;	/* Database flags in iidbdb */
    i4	    dsc_dbservice;	/* Database service permissions */
    u_i4    dsc_dbcmptlvl;	/* Database major compatability level */
    i4	    dsc_1dbcmptminor;	/* Database minor compatability level */
    i4	    dsc_patchcnt;	/* # times config file was 'patched'
					** by setting dsc_status || DSC_VALID */
    i4	    dsc_1st_patch;	/* time (in #secs since 1-Jan-1970) of
					** 1st config file patch to DSC_VALID */
    i4	    dsc_last_patch;	/* time (in #secs since 1-Jan-1970) of
					** last config file patch to DSC_VALID*/
    i4	    dsc_inconst_code;	/* indicate which logic set database
					** inconsistent */

    i4	    dsc_inconst_time;	/* time (#secs since 1-Jan-1970) of when
					** config file status set ~DSC_VALID */
    i4	    dsc_iirel_relprim;	/* Relprim value for iirelation. */
    char    dsc_name[DB_OLDMAXNAME_32];	/* Name of the database. */
    char    dsc_owner[DB_OLDMAXNAME_32];	/* Owner of the database. */
    char    dsc_collation[DB_OLDMAXNAME_32];    /* collation name */
    char    dsc_extra1[8];	/* Unused now */
    i4         dsc_iirel_relpgsize; /* Relpgsize value for iirelation */
    i4         dsc_iiatt_relpgsize;  /* Relpgsize value for iiattribute */
    i4         dsc_iiind_relpgsize;  /* Relpgsize value for iiindexes  */
    i2         dsc_iirel_relpgtype;  /* Relpgtype value for iirelation */
    i2         dsc_iiatt_relpgtype;  /* Relpgtype value for iiattribute */
    i2         dsc_iiind_relpgtype;  /* Relpgtype value for iiindexes  */
    char	    dsc_ucollation[DB_OLDMAXNAME_32]; /* unicode collation name */
    char	    dsc_extra[6];	/* Future expansion. */
};

/*}
** Name: DM0C_ODSC - Database decription entry in config files with
**	    version #s <= CNF_V3.
**
** Description:
**      This structure defines the basic information known about a
**	database by DMF.
**
** History:
**	24-sep-1992 (ananth)
**	    Created for increasing DB_MAXNAME project.
*/
struct _DM0C_ODSC
{
    i4         length;             /* Length of entry. */
    i4         type;               /* Type of entry. */
#define                 DM0C_T_ODSC      1L
    i4	    dsc_status;		/* Database status. */
    i4         dsc_open_count;     /* Count of concurrent server opens. */
    u_i4       dsc_sync_flag;      /* Used for Unix systems to determine
                                        ** if synchronous writes should be
                                        ** guaranteed. */
    char	    dsc_name[DB_OLDMAXNAME];
					/* Name of the database. */
    char	    dsc_owner[DB_OLDMAXNAME];
					/* Owner of the database. */
    i4	    dsc_type;		/* Type of database. */
    i4	    dsc_access_mode;	/* Database access mode. */
    i4	    dsc_c_version;	/* Version of DMF that created this
					** database. */
    i4	    dsc_version;	/* Current version of database */
    i4	    dsc_dbid;		/* Database identifier(date). */
    i4	    dsc_sysmod;		/* Sysmod date. */
    i4	    dsc_ext_count;	/* Number of extents. */
    u_i4       dsc_last_table_id;  /* Last table id used. */
    i4         dsc_cnf_version;    /* Version of this config file.
					** High 2 bytes give major version
					** number and low 2 bytes give minor
					** version. */
    i4	    dsc_dbaccess;	/* Database flags in iidbdb */
    i4	    dsc_dbservice;	/* Database service permissions */
    u_i4    dsc_dbcmptlvl;	/* Database major compatability level */
    i4	    dsc_1dbcmptminor;	/* Database minor compatability level */
    i4	    dsc_patchcnt;	/* # times config file was 'patched'
					** by setting dsc_status || DSC_VALID */
    i4	    dsc_1st_patch;	/* time (in #secs since 1-Jan-1970) of
					** 1st config file patch to DSC_VALID */
    i4	    dsc_last_patch;	/* time (in #secs since 1-Jan-1970) of
					** last config file patch to DSC_VALID*/
    i4	    dsc_inconst_code;	/* indicate which logic set database
					** inconsistent */
    i4	    dsc_inconst_time;	/* time (#secs since 1-Jan-1970) of when
					** config file status set ~DSC_VALID */
    char	    dsc_collation[DB_OLDMAXNAME];   /* collation name */
};

/*}
** Name: DM0C_EXT - Configuration file extent entry.
**
** Description:
**      This structure describes the a configuration file extent entry.
**      The extent entries describes disk location that can be used to
**	locate tables.
**
** History:
**     03-dec-1985 (derek)
**          Created new for Jupiter.
**	12-Mar-2001 (jenjo02)
**	    Created raw_next from "extra".
**	02-Apr-2001 (jenjo02)
**	    Removed raw_next, not needed.
*/
struct _DM0C_EXT
{
    i4         	    length;             /* The length of the entry. */
    i4	    	    type;		/* The type of the entry. */
#define			DM0C_T_EXT	4L
    i4	    	    extra[2];		/* Future expansion. */
    DMP_LOC_ENTRY   ext_location;	/* Location information. */
};


/*}
** Name: DM0C_V6EXT - Configuration file extent entry in config files
**	with version #s <= CNF_V6.
**
** Description:
**      This structure describes the a configuration file extent entry.
**      The extent entries describes disk location that can be used to
**	locate tables.
**
** History:
**      21-apr-2009 (stial01)
**	    Created for increasing DB_MAXNAME > 32
*/
struct _DM0C_V6EXT
{
    i4         	    length;             /* The length of the entry. */
    i4	    	    type;		/* The type of the entry. */
#define			DM0C_T_EXT	4L
    i4	    	    extra[2];		/* Future expansion. */
    struct _DMP_V6LOC_ENTRY		    /* Location entry for the root 
					    ** directory of a database.  This
					    ** entry is used to locate the 
					    ** database at open time. */
    {
      char	    logical[DB_OLDMAXNAME_32];
      i4	    flags;		    /* Location flags. */
      i4	    phys_length;	    /* Length of physical 
                                            ** location name. */
      DM_PATH       physical;		    /* Physical location name. */
      i4   	    raw_start;	    	    /* Starting block of raw loc */
      i4   	    raw_blocks;	    	    /* Number of blocks for raw loc */
      i4	    raw_total_blocks;	    /* Total blocks in raw area */
    }		    ext_location;	/* Location information. */
};


/*}
** Name: DM0C_V5EXT - Configuration file extent entry in config files

/*}
** Name: DM0C_V5EXT - Configuration file extent entry in config files
**	with version #s <= CNF_V5.
**
** Description:
**      This structure describes the a configuration file extent entry.
**      The extent entries describes disk location that can be used to
**	locate tables.
**
** History:
**	12-mar-1999 (nanpr01)
**	    Created for increasing DB_MAXNAME project.
*/
struct _DM0C_V5EXT
{
    i4         length;             /* The length of the entry. */
    i4	    type;		/* The type of the entry. */
    i4	    extra[2];		/* Future expansion. */
    struct _V5DMP_LOC_ENTRY
    {
	char	    logical[DB_OLDMAXNAME_32];
	i4	    flags;
	i4	    phys_length;
	DM_PATH	    physical;
    }		    ext_location;	/* Location information. */

};

/*}
** Name: DM0C_OEXT - Configuration file extent entry in config files
**	with version #s <= CNF_V3.
**
** Description:
**      This structure describes the a configuration file extent entry.
**      The extent entries describes disk location that can be used to
**	locate tables.
**
** History:
**	24-sep-1992 (ananth)
**	    Created for increasing DB_MAXNAME project.
*/
struct _DM0C_OEXT
{
    i4         length;             /* The length of the entry. */
    i4	    type;		/* The type of the entry. */
    i4	    extra[2];		/* Future expansion. */
    struct _ODMP_LOC_ENTRY
    {
	char	    logical[DB_OLDMAXNAME];	
	i4	    flags;
	i4	    phys_length;
	DM_PATH	    physical;
    }		    ext_location;	/* Location information. */

};

/*}
** Name: DM0C_V3JNL - Configuration file journal entry.
**
** Description:
**      This structure describes information about journaling operations
**	that have been performed against the database.
**
** History:
**      26-oct-1986 (Derek)
**          Created for Jupiter.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
[@history_template@]...
*/
struct _DM0C_V3JNL
{
    i4         length;             /* The length of the entry. */
    i4	    type;		/* The type of the entry. */
#define			DM0C_T_JNL	2L
    i4         jnl_count;          /* Number of journal entries 
                                        ** in table. */
    i4	    jnl_ckp_seq;	/* Current checkpoint 
                                        ** sequence number. */
    i4	    jnl_fil_seq;	/* Current journal file 
                                        ** sequence number. */
    i4	    jnl_blk_seq;	/* Current journal block 
                                        ** sequence number. */ 
    i4	    jnl_bksz;		/* Journal block size. */
    i4	    jnl_blkcnt;		/* Initial allocation. */
    i4	    jnl_maxcnt;		/* Target allocation. */
    i4	    jnl_update;		/* Date of last journal update. */
    LG_OLA	    jnl_la;		/* Last log record copied to jnl. */
    i4	    jnl_extra[4];	/* Future expansion. */
    struct _OJNL_CKP
#define			CKP_OHISTORY	16
    {
	i4	    ckp_sequence;	/*  Checkpoint sequence. */
	DB_TAB_TIMESTAMP
         	    ckp_date;		/*  8 byte timestamp of this 
                                        **  checkpoint. */
	i4	    ckp_f_jnl;		/*  First journal sequence. */
	i4	    ckp_l_jnl;		/*  Last journal sequence. */
    }		    jnl_history[CKP_OHISTORY];	
                                        /*  Checkpoint/journal history table. */
    struct _OJNL_CNODE_INFO
    {
	i4	    cnode_fil_seq;	/*  Current cluster journal sequence. */
	i4	    cnode_blk_seq;	/*  Current cluster jnl block seq. */
	LG_OLA	    cnode_la;		/*  Last log record copied to jnl. */
    }		    jnl_node_info[DM_CNODE_MAX];	
                                        /*  Cluster journal info table. */
};

/*}
** Name: DM0C_V4JNL - Configuration file journal entry.
**
** Description:
**      This structure describes information about journaling operations
**	that have been performed against the database.
**
** History:
**	30-jan-1989 (EdHsu)
**	    Created for online backup
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	22-nov-1993 (jnash)
**	    Add jnl_first_jnl_seq to track last checkpoint from which 
**	    rolldb +j may take place.  Decrement jnl_extra.
**	03-feb-1998 (hanch04)
**	    Created for upgradeing to OI2.5 with new LG_LA struct.
*/
struct _DM0C_V4JNL
{
    i4         length;             /* The length of the entry. */
    i4	    type;		/* The type of the entry. */
#define			DM0C_T_JNL	2L
    i4         jnl_count;          /* Number of journal entries 
                                        ** in table. */
    i4	    jnl_ckp_seq;	/* Current checkpoint 
                                        ** sequence number. */
    i4	    jnl_fil_seq;	/* Current journal file 
                                        ** sequence number. */
    i4	    jnl_blk_seq;	/* Current journal block 
                                        ** sequence number. */ 
    i4	    jnl_bksz;		/* Journal block size. */
    i4	    jnl_blkcnt;		/* Initial allocation. */
    i4	    jnl_maxcnt;		/* Target allocation. */
    i4	    jnl_update;		/* Date of last journal update. */
    LG_OLA	    jnl_la;		/* Last log record copied to jnl. */
    i4	    jnl_first_jnl_seq;	/* Seq no of checkpoint when db 
					** was initially journaled (or re-
					** journaling was re-enabled). */
    i4	    jnl_extra[3];	/* Future expansion. */
    struct _JNL_V4CKP
#define			CKP_OHISTORY	16
    {
	i4	    ckp_sequence;	/*  Checkpoint sequence. */
	DB_TAB_TIMESTAMP
         	    ckp_date;		/*  8 byte timestamp of this 
                                        **  checkpoint. */
	i4	    ckp_f_jnl;		/*  First journal sequence. */
	i4	    ckp_l_jnl;		/*  Last journal sequence. */
	i4	    ckp_jnl_valid;	/*  Mark if ckp is valid */
	i4	    ckp_mode;		/*  Ckp mode */
#define			CKP_OFFLINE	0x01
#define			CKP_ONLINE	0x02
#define                 CKP_TABLE       0x04
    }		    jnl_history[CKP_OHISTORY];	
                                        /*  Checkpoint/journal history table. */
    struct _JNL_V4CNODE_INFO
    {
	i4	    cnode_fil_seq;	/*  Current cluster journal sequence. */
	i4	    cnode_blk_seq;	/*  Current cluster jnl block seq. */
	LG_OLA	    cnode_la;		/*  Last log record copied to jnl. */
    }		    jnl_node_info[DM_CNODE_MAX];	
                                        /*  Cluster journal info table. */
};

/*}
** Name: DM0C_JNL - Configuration file journal entry.
**
** Description:
**      This structure describes information about journaling operations
**	that have been performed against the database.
**
** History:
**	30-jan-1989 (EdHsu)
**	    Created for online backup
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	22-nov-1993 (jnash)
**	    Add jnl_first_jnl_seq to track last checkpoint from which 
**	    rolldb +j may take place.  Decrement jnl_extra.
[@history_template@]...
*/
struct _DM0C_JNL
{
    i4         length;             /* The length of the entry. */
    i4	    type;		/* The type of the entry. */
#define			DM0C_T_JNL	2L
    i4         jnl_count;          /* Number of journal entries 
                                        ** in table. */
    i4	    jnl_ckp_seq;	/* Current checkpoint 
                                        ** sequence number. */
    i4	    jnl_fil_seq;	/* Current journal file 
                                        ** sequence number. */
    i4	    jnl_blk_seq;	/* Current journal block 
                                        ** sequence number. */ 
    i4	    jnl_bksz;		/* Journal block size. */
    i4	    jnl_blkcnt;		/* Initial allocation. */
    i4	    jnl_maxcnt;		/* Target allocation. */
    i4	    jnl_update;		/* Date of last journal update. */
    LG_LA	    jnl_la;		/* Last log record copied to jnl. */
    i4	    jnl_first_jnl_seq;	/* Seq no of checkpoint when db 
					** was initially journaled (or re-
					** journaling was re-enabled). */
    i4	    jnl_extra[3];	/* Future expansion. */
    struct _JNL_CKP
#define			CKP_HISTORY	99
    {
	i4	    ckp_sequence;	/*  Checkpoint sequence. */
	DB_TAB_TIMESTAMP
         	    ckp_date;		/*  8 byte timestamp of this 
                                        **  checkpoint. */
	i4	    ckp_f_jnl;		/*  First journal sequence. */
	i4	    ckp_l_jnl;		/*  Last journal sequence. */
	i4	    ckp_jnl_valid;	/*  Mark if ckp is valid */
	i4	    ckp_mode;		/*  Ckp mode */
#define			CKP_OFFLINE	0x01
#define			CKP_ONLINE	0x02
#define                 CKP_TABLE       0x04
    }		    jnl_history[CKP_HISTORY];	
                                        /*  Checkpoint/journal history table. */
    struct _JNL_CNODE_INFO
    {
	i4	    cnode_fil_seq;	/*  Current cluster journal sequence. */
	i4	    cnode_blk_seq;	/*  Current cluster jnl block seq. */
	LG_LA	    cnode_la;		/*  Last log record copied to jnl. */
    }		    jnl_node_info[DM_CNODE_MAX];	
                                        /*  Cluster journal info table. */
};

/*}
** Name: DM0C_V4DMP - Configuration file dump entry.
**
** Description:
**      This structure describes information about dumping operations
**	that have been performed against the database.
**
** History:
**      06-jan-1989 (Edhsu)
**          Created for Terminator.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	03-feb-1998 (hanch04)
**	    Created for upgradeing to OI2.5 with new LG_LA struct.
*/
struct _DM0C_V4DMP
{
    i4         length;             /* The length of the entry. */
    i4	    type;		/* The type of the entry. */
#define			DM0C_T_DMP	5L 
    i4         dmp_count;          /* Number of dump entries 
                                        ** in table. */
    i4	    dmp_ckp_seq;	/* Current checkpoint 
                                        ** sequence number. */
    i4	    dmp_fil_seq;	/* Current dump file 
                                        ** sequence number. */
    i4	    dmp_blk_seq;	/* Current dump block 
                                        ** sequence number. */ 
    i4	    dmp_bksz;		/* Dump block size. */
    i4	    dmp_blkcnt;		/* Initial allocation. */
    i4	    dmp_maxcnt;		/* Target allocation. */
    i4	    dmp_update;		/* Date of last dump update. */
    LG_OLA	    dmp_la;		/* Last log record copied to dmp. */
    i4	    dmp_extra[4];	/* Future expansion. */
    struct _DMP_V4CKP
#define			CKP_OHISTORY	16
    {
	i4	    ckp_sequence;	/*  Checkpoint sequence. */
	DB_TAB_TIMESTAMP
         	    ckp_date;		/*  8 byte timestamp of this 
                                        **  checkpoint. */
	i4	    ckp_f_dmp;		/*  First dump sequence. */
	i4	    ckp_l_dmp;		/*  Last dump sequence. */
	i4	    ckp_mode;		/*  ckp mode */
	i4	    ckp_dmp_valid;	/*  valid bit.	*/
    }		    dmp_history[CKP_OHISTORY];	
                                        /*  Checkpoint/dump history table. */
    struct _DMP_CNODE_V4INFO
    {
	i4	    cnode_fil_seq;	/*  Current cluster dump sequence. */
	i4	    cnode_blk_seq;	/*  Current cluster dmp block seq. */
	LG_OLA	    cnode_la;		/*  Last log record copied to dmp. */
    }		    dmp_node_info[DM_CNODE_MAX];	
                                        /*  Cluster dump info table. */
};

/*}
** Name: DM0C_DMP - Configuration file dump entry.
**
** Description:
**      This structure describes information about dumping operations
**	that have been performed against the database.
**
** History:
**      06-jan-1989 (Edhsu)
**          Created for Terminator.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
[@history_template@]...
*/
struct _DM0C_DMP
{
    i4         length;             /* The length of the entry. */
    i4	    type;		/* The type of the entry. */
#define			DM0C_T_DMP	5L 
    i4         dmp_count;          /* Number of dump entries 
                                        ** in table. */
    i4	    dmp_ckp_seq;	/* Current checkpoint 
                                        ** sequence number. */
    i4	    dmp_fil_seq;	/* Current dump file 
                                        ** sequence number. */
    i4	    dmp_blk_seq;	/* Current dump block 
                                        ** sequence number. */ 
    i4	    dmp_bksz;		/* Dump block size. */
    i4	    dmp_blkcnt;		/* Initial allocation. */
    i4	    dmp_maxcnt;		/* Target allocation. */
    i4	    dmp_update;		/* Date of last dump update. */
    LG_LA	    dmp_la;		/* Last log record copied to dmp. */
    i4	    dmp_extra[4];	/* Future expansion. */
    struct _DMP_CKP
#define			CKP_HISTORY	99
    {
	i4	    ckp_sequence;	/*  Checkpoint sequence. */
	DB_TAB_TIMESTAMP
         	    ckp_date;		/*  8 byte timestamp of this 
                                        **  checkpoint. */
	i4	    ckp_f_dmp;		/*  First dump sequence. */
	i4	    ckp_l_dmp;		/*  Last dump sequence. */
	i4	    ckp_mode;		/*  ckp mode */
	i4	    ckp_dmp_valid;	/*  valid bit.	*/
    }		    dmp_history[CKP_HISTORY];	
                                        /*  Checkpoint/dump history table. */
    struct _DMP_CNODE_INFO
    {
	i4	    cnode_fil_seq;	/*  Current cluster dump sequence. */
	i4	    cnode_blk_seq;	/*  Current cluster dmp block seq. */
	LG_LA	    cnode_la;		/*  Last log record copied to dmp. */
    }		    dmp_node_info[DM_CNODE_MAX];	
                                        /*  Cluster dump info table. */
};

/*
** Name: DM0C_RELATION 
**
** Description:
**	This structure is used as a placeholder for core catalog 
**	IIrelation tuple information that used to be stored in the 
**	config file. It is the exact size the DMP_RELATION structure
**	was in 6.4. Since then DMP_RELATION has been increased in size
**	and this structure was created as a placeholder for it. The
**	only field that is required in this structure is relprim. All
**	other fields from DMP_RELATION have been replaced by a filler
**	field. All this was done to avoid having to convert the config
**	file when DMP_RELATION was changed.
**
**	Note: This structure must NEVER be changed.
**
** History:
**	05-may-1992 (ananth)
**	    Increasing IIrelation tuple size project.
*/
struct _DM0C_RELATION
{
    char    relfiller1[76];
    i4	    relprim;
    char    relfiller2[116];
};
 
/*}
** Name: DM0C_TAB - Configuration file table entry.
**
** Description:
**      This structure used to describes the configuration file format for
**	a table description that is stored in the configuration file.
**	It is still maintained since it is needed when converting config
**	files from old formats to the current one.
**
** History:
**	03-dec-1985 (derek)
**          Created new for jupiter.
**	05-may-1992 (ananth)
**	    Increasing IIrelation tuple size project.
**	    Replace DMP_RELATION and DMP_ATTRIBUTE with DM0C_RELATION and
**	    DM0C_ATTRIBUTE.
**	22-sep-1992 (ananth)
**	    Get rid of core catalog section from config file.
*/
struct _DM0C_TAB
{
    i4	    length;	        /* Length of this entry. */
    i4	    type;		/* Type of entry. */
#define                 DM0C_T_TAB	3L
    DM0C_RELATION   tab_relation;
};

/*
** Function Prototypes
*/
FUNC_EXTERN DB_STATUS	dm0c_close(
				    DM0C_CNF	    *config,
				    i4	    flags,
				    DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS	dm0c_open(
				    DMP_DCB	    *dcb,
				    i4	    flags,
				    i4	    lock_list,
				    DM0C_CNF	    **config,
				    DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS	dm0c_mk_consistnt(DM0C_CNF  *cnf);
FUNC_EXTERN DB_STATUS	dm0c_extend(DM0C_CNF *config, DB_ERROR *dberr);
FUNC_EXTERN VOID	dm0c_dmp_filename(i4 sequence, char *filename);

