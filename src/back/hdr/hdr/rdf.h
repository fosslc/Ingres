/*
**Copyright (c) 2004 Ingres Corporation
**
*/

/**
** Name: RDF.H - Typedefs for Relation Description Facility.
**
** Description:
**      This file contains the structure typedefs and definitions of
**	symbolic constants for RDF external interface.
**
** History:
**     26-mar-86 (ac)
**          written
**     02-mar-87 (puree)
**	    modified for cache version and global server control block
**     11-apr-87 (puree)
**	    add unique database id field to RDR_RB.
**     09-aug-88 (mings)
**          add RDF_LDB_TABINFO opcode
**          add pointer rdr_r2_ldp_p (DD_LDB_DESC) to RDR_RB for titan.
**          add rdr_obj_desc (DD_OBJ_DESC) to RDR_INFO for titan.
**          add DB_DISTRIB to RDF_CCB, RDR_RB, RDF_CB structures to indicate
**              STAR or backend.
**	    add rdf_c2_ldberr_b to indicate LDB error and LDB return error 
**              rdf_c3_ldb_error to RDF_CB structure.
**     05-mar-89 (ralph)
**          GRANT Enhancements, Phase 1:
**          Added RDR_GROUP, RDR_APLID to rdr_types_mask flags;
**          Added RDR_REPLACE, RDR_PURGE to rdr_update_op flags;
**          Loaded rdr_qrytuple and rdr_qtuple_count for
**              iiapplication_id/iiusergroup tuples
**          New internal RDF error codes for groups/applids
**     06-mar-89 (neil)
**          Rules: modified structures and added flags for rule processing.
**          Added errors.
**     31-mar-89 (ralph)
**          GRANT Enhancements, Phase 2:
**          Added RDR_DBPRIV to rdr_types_mask flags;
**          Loaded rdr_qrytuple and rdr_qtuple_count for iidbpriv tuples
**     04-apr-89 (mings)
**	    add RDF_END_SESSION opcode
**     16-apr-89 (mings)
**	    add RDF_AUTOCOMMIT, RDF_NO_AUTOCOMMIT, RDF_COMMIT, RDF_TRANSACTION opcodes
**     26-apr-89 (mings)
**	    add register with refresh
**     21-mar-89 (mings)
**          added cluster info support
**     21-jun-89 (mings)
**          added RDR_DD10_OPEN_CDBASSO to force open privilege cdb association
**     17-jul-89 (jennifer)
**          Add in new defintions for manipulating iiuser, iilocation, 
**          iisecuritystate, and iidbaccess.
**      29-sep-89 (ralph)
**          Added E_RD016F_DLOCATION and E_RD0170_RLOCATION.
**          Removed RDR_DBACCESS and related messages.
**     2-oct-89 (teg)
**	    added new error msg constant E_RD013C_BAD_QRYTRE_RNGSIZ.
**     9-oct-89 (teg)
**	    merged titan/terminator RDF.H.  Ifdeffed out structs requiring
**	    rdfddb.h or types in TITAN's dbms.h that are not in local's dbms.h
**     13-Oct-89 (teg)
**          added RDR_REMOVE bit to fix SYSCON bug where it is not
**          possible to do remove table/view/link command cuz the
**          LDB that the link is to no longer exists
**     17-Oct-89 (teg)
**	    added rdr_2types_mask to rdr_rb because we are running out of bits.
**	24-oct-89 (neil)
**	    Alerters: Modified data structures, added flags and errors
**	    for event processing.
**     13-dec-89 (teg)
**	    upped values from RDF_NVAO and RDF_NVP for new RDF traces that take
**	    value pairs (RD0101 to RD0128) take value pairs, and RD0111 to
**	    RD0122 are currently implemented.
**	16-jan-90 (ralph)
**	    Added RDR_SECALM to rdr_types_mask flags in RDR_RB.
**	01-feb-90 (teg)
**	    Added RDR_COMMENT and RDR_SYNONYM to rdr_2types_mask in RDR_RB.
**	27-feb-90 (carol)
**	    Moved RDR_COMMENT to rdr_types_mask in RDR_RB.
**	19-apr-90 (teg)
**	    fix bug 8011.  The qrym_cnt field was sometimes used to indicate
**	    the amount of memory RDF allocated and was other times used to
**	    indicate the number of tuples that QEF found during a read.
**	    This has been broken into two separate fields.
**	21-may-90 (teg)
**	    creation of type RDF_INSTR for FIPS.
**	20-sep-91 (teresa)
**	    Pick up ingres63p change 260973:
**	      7-jan-91 (seputis)
**		Added RDR2_TIMEOUT to support the OPF ACTIVE flag.
**	18-dec-91 (teresa)
**	    merged STAR and Local rdf.h
**	26-jun-92 (andre)
**	    defined RDR_2DBP_STATUS over rdr_2types_mask
**	29-jun-92 (andre)
**	    defined RDR_2INDEP_OBJECT_LIST and RDR_2INDEP_PRIVILEGE_LIST
**	    over rdr_2types_mask
**	22-jul-92 (andre)
**	    undefined RDR_PERMIT_ALL and RDR_RETRIEVE_ALL and defined
**	    RDR_DROP_ALL over rdr_types_mask
**	29-sep-92 (andre)
**	    renamed RDR_2* masks over rdr_2types_mask to RDR2_*
**	28-oct-92 (jhahn)
**	    Added support for retrieving II_PROCEDURE_PARAMETER tuples
**	     for FIPS Constraints.
**	18-jan-93 (rganski)
**	    Character Histogram Enhancements project:
**	    Added sversion and shistlength fields to RDD_HISTO.
**	    Added new error E_RD010C_HIST_LENGTH.
**	06-apr-93 (ralph)
**	    DELIM_IDENT:
**	    Added rdf_dbxlate and rdf_cat_owner to RDF_CCB.
**	    Added rds_dbxlate and rds_cat_owner to RDF_SESS_CB.
**	07-jul-93 (teresa)
**	    added prototypes for RDF's external entry points: rdf_call and
**	    rdf_trace so that a newly prototyped SCF can link without lots of
**	    annoying warnings.
**	26-aug-93 (robf)
**	    Add RDR2_PROFILE entry for manipulating iiprofile catalog
**	18-feb-84 (teresa)
**	    defined E_RD0101_FLUSH_CACHE and E_RD010D_BAD_CKSUM for bug 59336.
**	4-mar-94 (robf)
**          Add RDR2_ROLEGRANT for manipulating iirolegrant catalog
**	25-apr-94 (rganski)
**	    Added rdr_histo_type to RDR_RB, as part of fix for b62184.
**	9-jan-96 (inkdo01)
**	    Added f4repf to RDD_HISTO for per-cell repetition factors.
**	07-feb-1997 (canor01)
**	    Added RDF_INV_ALERT_NAME for rdf cache synchronization event.
**	02-nov-1998 (stial01)
**	    Added for RDF trace point rd0013.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Jan-2001 (jenjo02)
**	    Added rdf_rdscb_offset, rdf_adf_cb, rdf_sess_id,
**	    rds_adf_cb, RDF_MAX_OP define.
**	1-Dec-2003 (schka24)
**	    Remove parameter holders unref'ed since 12-feb-93.
**	29-Dec-2003 (schka24)
**	    RDF support for partitioned tables.
**	2-Feb-2004 (schka24)
**	    Modify partition-copy call for PSF use;  expose "finish partdef".
**      12-Apr-2004 (stial01)
**          Define length as SIZE_TYPE.
**	5-Aug-2004 (schka24)
**	    Expose partition struct-compat checker as an RDF function.
**      7-oct-2004 (thaju02)
**          Replace i4 with SIZE_TYPE for memory pool > 2Gig.
**      22-Mar-2005 (thaju02)
**          Add RDR2_DBDEPENDS to access iidbdepends by key.
**	6-Jul-2006 (kschendel)
**	    Remove RDF_UCB, not used anywhere.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**      01-oct-2010 (stial01) (SIR 121123 Long Ids)
**          Store blank trimmed names in DMT_ATT_ENTRY
*/

/* 
** prototypes for external entry points 
*/

    /* rdf_call - Main entry point for RDF */
    FUNC_EXTERN	    DB_STATUS		rdf_call(i4 op_code, void * rdf_cb);

    /* rdf_trace - Call RDF trace operation. */
    FUNC_EXTERN	    DB_STATUS		rdf_trace(DB_DEBUG_CB *debug_cb);


/*
**	Miscellaneous defines
*/

#define			RDF_TYPES		i4

/* A random prime number for size of the attribute hash table. */
#define			RDD_SIZ_ATTHSH		31

/* Number of tuples retrieved from iitree/iiqrytext tables. Note: This
** number can be tuned later. */
#define			RDF_TUPLE_RETRIEVE	10

/* Rdf trace arguments */
#define                 RDF_NB		128	/* Number of flags in the vector. */
#define                 RDF_NVAO        12      /* Number of values to hold at
						** once in the trace vector. */
#define                 RDF_NVP         28      /* Number of flags which have 
						** values associated with them*/

/* Name of RDF cache synchronization event signaled by rdf_invalidate() */
#define			RDF_INV_ALERT_NAME	"IIRDFCacheSyncEvent"

/* old star values:
			RDF_NVAO    	5	
			RDF_NVP		10
*/

/*
**      RDF operation codes
*/
    /*********************************************************************/
    /* Evnironment management - server/facility/session startup/shutdown */
    /*********************************************************************/
#define	    RDF_STARTUP		7   /* Startup RDF for the server.*/
#define	    RDF_SHUTDOWN    	9   /* Shutdown RDF for the server. */
#define	    RDF_INITIALIZE	1   /* Initialize RDF for a facility. */
#define	    RDF_TERMINATE	2   /* Terminate RDF for a facility and 
				    ** release all the memory used for storing 
				    ** tables information referenced by that 
				    ** facility. */
#define	    RDF_BGN_SESSION	8   /* Begin RDF session */
#define	    RDF_END_SESSION	14  /* End RDF session */


    /*********************************************************************/
    /*  Cache /Catalog Management - Create/Destroy/Release Cache Objects */
    /*				    Also obtain info from catalogs       */
    /*********************************************************************/
#define	    RDF_GETDESC		3   /* Request description of a table. i.e 
				    ** core catalog information from DMF. */
#define	    RDF_UNFIX		4   /* release in-use lock on an RDF cache
				    ** object. */
#define	    RDF_GETINFO		5   /* Request system catalog info on an object.
				    ** RDF formats this info appropriately, and
				    ** it includes things like: procedures,
				    ** events, statistics, trees (view, integrity,
				    ** etc), rules. */
#define	    RDF_READTUPLES	33  /* read a set of tuples from a catalog */
#define	    RDF_INVALIDATE	10  /* Invalidate a table information block 
				    ** currently in the cache. */

    /**************************************************************/
    /* Distributed Operations -- Not supported Unless Distributed */
    /**************************************************************/
#define	    RDF_DDB		13  /* Retrieve DDB/LDB information */
#define	    RDF_AUTOCOMMIT  	15  /* Set auto commit on -- OBSOLETE */
#define	    RDF_NO_AUTOCOMMIT	16  /* Set auto commit off -- OBSOLETE */
#define	    RDF_COMMIT		17  /* commit the transaction -- OBSOLETE */
#define	    RDF_TRANSACTION	18  /* begin transaction */


    /********************************************/
    /* Utility Operations - Format Tuples, etc	*/
    /********************************************/
#define	    RDF_PART_COPY	11  /* Copy partition definition for caller */
#define	    RDF_PART_UPDATE	12  /* Partition catalogs update */
#define     RDF_PART_FINISH	19  /* "finish" a partition definition */
/* *Note* 20 thru 28 are tracepoint dumps, see rdfint.h */
#define	    RDF_PART_COMPAT	29  /* Partition/struct compatibility check */
#define	    RDF_UPDATE		6   /* A operation used to updating (delete or 
				    ** append) the qrymod information into
				    ** system catalogs during DDL statements. */
#define	    RDF_PACK_QTEXT	30  /* pack query text into IIQRYTEXT tuple 
				    ** format */
#define	    RDF_PACK_QTREE	31  /* pack a query tree into IITREE tuple 
				    ** format */
#define     RDF_ULM_TRACE       32

#define	    RDF_MAX_OP	RDF_READTUPLES	/* Highest valid operation code */


/*}
** Name: RDF_ERROR - type for error codes
**
** Description:
**      This type will be used for RDF and system error codes within rdf
**
**	RDF error ranges are as follows:
**
**	BOUNDARY:		RANGE:	TYPE:
** 	--------------------	------	----------------------------------	
**	E_RD0000_OK       		(NOTE: E_RD0000_OK is non-error)	
**				0-9F 	External errors - errors from 
**					underlaying facilities
**	E_RD0100_LOG_ERRORS 	
**				100-19F	Internal RDF errors & consistency checks
**	E_RD01FF_RECOVERABLE_ERRORS
**				200+	Recoverable errors 
**	S_RDF000_STRINGS	F000+	Message strings, not errors
**
**	Available message numbers -- check below to assure they're still
**	available.  These were available as of 22-feb-94:
**
**		ERROR:		
**				E_RD001E to E_RD001F
**				E_RD0022 to E_RD0024
**				E_RD0026 to E_RD0029
**				E_RD002C to E_RD002F
**				E_RD0035 to E_RD003F
**				E_RD0046 to E_RD004F
**				E_RD0051 to E_RD005F
**				E_RD0061 to E_RD006F
**				E_RD0072
**				E_RD0074 to E_RD0075
**				E_RD0077
**				E_RD007A to E_RD009F
**		WARNING:	
**				E_RD010E to E_RD010F
**				E_RD0111 to E_RD0113
**				E_RD0115
**				E_RD0117 to E_RD0153
**				E_RD0155 to E_RD015F
**				E_RD0161 to E_RD016E
**				E_RD0170 to E_RD01FF
**		NOT LOGGED:
**				E_RD0203 to E_RD020F
**				E_RD0213 to E_RD0245
**
**	NOTE:
**		Titan (early star) errors appear to violate range boundries.
**		Instead they range from E_RD0250 to E_RD027C.
**
** History:
**      11-nov-87 (seputis)
**          initial creation
**	2-oct-89 (teg)
**	    added new error msg constant E_RD013C_BAD_QRYTRE_RNGSIZ.
**	28-feb-90 (teg)
**	    added new consistency check msgs: E_RD013F_MISSING_RELINFO,
**	    E_RD0140_BAD_RELKEY_REQ, E_RD0141_BAD_ATTHASH_REQ
**	09-apr-90 (teg)
**	    add E_RD0142_ULH_ACCESS for SYNONYM support.
**	04-may-90 (teg)
**	    add E_RD0145_ALIAS_MEM and E_RD0202_ALIAS_DEGRADED for SYNONYMs.
**	22-feb-91 (teresa)
**	    added E_RD0146_ILLEGAL_TREEVSN
**	14-sep-92 (teresa)
**	    added E_RD0149_INVALID_TABLETYPE
**	15-feb-93 (teresa)
**	    added E_RD001C_CANT_READ_COL_DEF and E_RD001D_NO_SUCH_DEFAULT
**	21-oct-93 (andre)
**	    defined E_RD014A_NONEXISTENT_SYNONYM
[@history_template@]...
*/
typedef i4 RDF_ERROR;
#define		E_RD_MASK		    0x00080000L
#define		E_RD0000_OK		    0L
#define		E_RD0001_NO_MORE_MEM			(E_RD_MASK + 0x0001L)
#define		E_RD0002_UNKNOWN_TBL			(E_RD_MASK + 0x0002L)
#define		E_RD0003_BAD_PARAMETER			(E_RD_MASK + 0x0003L)
#define		E_RD0006_MEM_CORRUPT			(E_RD_MASK + 0x0006L)
#define		E_RD0007_MEM_NOT_OWNED			(E_RD_MASK + 0x0007L)
#define		E_RD0008_MEM_SEMWAIT			(E_RD_MASK + 0x0008L)
#define		E_RD0009_MEM_SEMRELEASE			(E_RD_MASK + 0x0009L)
#define		E_RD000A_MEM_NOT_FREE			(E_RD_MASK + 0x000AL)
#define		E_RD000B_MEM_ERROR			(E_RD_MASK + 0x000BL)
#define		E_RD000C_USER_INTR			(E_RD_MASK + 0x000CL)
#define		E_RD000D_USER_ABORT			(E_RD_MASK + 0x000DL)
#define		E_RD000E_DMF_ERROR			(E_RD_MASK + 0x000EL)
#define		E_RD000F_INFO_OUT_OF_DATE		(E_RD_MASK + 0x000FL)
/* usage count on cached object is 0, so that it may be deallocated and
** memory may be corrupted as a result, caused
** by internal ULH or RDF processing error 
*/
#define		E_RD0010_QEF_ERROR			(E_RD_MASK + 0x0010L)
#define		E_RD0011_NO_MORE_ROWS			(E_RD_MASK + 0x0011L)
#define		E_RD0012_BAD_HISTO_DATA			(E_RD_MASK + 0x0012L)
#define		E_RD0013_NO_TUPLE_FOUND			(E_RD_MASK + 0x0013L)
#define		E_RD0014_BAD_QUERYTEXT_ID		(E_RD_MASK + 0x0014L)
#define		E_RD0015_AMBIGUOUS_REPLACE		(E_RD_MASK + 0x0015L)
#define		E_RD0016_KEY_SEQ			(E_RD_MASK + 0x0016L)
#define		E_RD0017_TABLE_EXISTS			(E_RD_MASK + 0x0017L)
#define		E_RD0018_NONEXISTENT_TABLE		(E_RD_MASK + 0x0018L)
#define		E_RD0019_TABLE_ACCESS_CONFLICT		(E_RD_MASK + 0x0019L)
#define         E_RD001A_RESOURCE_ERROR			(E_RD_MASK + 0x001AL)
#define         E_RD001B_HISTOGRAM_NOT_FOUND		(E_RD_MASK + 0x001BL)
#define         E_RD001C_CANT_READ_COL_DEF		(E_RD_MASK + 0x001CL)
#define         E_RD001D_NO_SUCH_DEFAULT		(E_RD_MASK + 0x001DL)
#define		E_RD0020_INTERNAL_ERR			(E_RD_MASK + 0x0020L)
#define		E_RD0021_FILE_NOT_FOUND			(E_RD_MASK + 0x0021L)
#define         E_RD0025_USER_ERROR			(E_RD_MASK + 0x0025L)
/* this is equivalent to E_QE0025_USER_ERROR, it is passed back to the
** caller when it is received from QEF, no logging or error reporting will
** be made */
#define         E_RD002A_DEADLOCK			(E_RD_MASK + 0x002AL)
/* deadlock was detected calling query execution facility */
#define         E_RD002B_LOCK_TIMER_EXPIRED		(E_RD_MASK + 0x002BL)
/* timeout was detected calling query execution facility */
  
#define		E_RD0030_SCF_ERROR			(E_RD_MASK + 0x0030L)
#define		E_RD0031_SEM_INIT_ERR			(E_RD_MASK + 0x0031L)
#define		E_RD0032_NO_SEMAPHORE			(E_RD_MASK + 0x0032L)
#define         E_RD0034_RELEASE_SEMAPHORE		(E_RD_MASK + 0x0034L)
/* attempt to release ULH semaphore failed due to SCF error */


#define		E_RD0040_ULH_ERROR			(E_RD_MASK + 0x0040L)
#define		E_RD0041_BAD_FCB			(E_RD_MASK + 0x0041L)
#define		E_RD0042_CACHE_NOT_EMPTY		(E_RD_MASK + 0x0042L)
#define		E_RD0043_CACHE_FULL			(E_RD_MASK + 0x0043L)
#define		E_RD0044_BAD_INFO_BLK			(E_RD_MASK + 0x0044L)
#define         E_RD0045_ULH_ACCESS			(E_RD_MASK + 0x0045L)
/* failed to gain access to a ULH object due to a ULH error */

#define		E_RD0050_TRANSACTION_ABORTED		(E_RD_MASK + 0x0050L)
#define         E_RD0060_DMT_SHOW			(E_RD_MASK + 0x0060L)
/* Cannot access table information due to a non-recoverable DMT_SHOW error */

#define         E_RD0071_NO_PROT_INT			(E_RD_MASK + 0x0071L)
/* error by caller - asking for QTUPLES but no request for protect, integrity,
** rule or event tuples */
#define         E_RD0072_NOT_OPEN			(E_RD_MASK + 0x0072L)
/* error by caller - attempt to close PROTECTION, INTEGRITY, RULE or EVENT
** tuple stream which was not open */
#define         E_RD0073_TUPLE_COUNT			(E_RD_MASK + 0x0073L)
/* error by caller - bad tuple count for qrymod retrieve */
#define                 E_RD0076_OPEN			(E_RD_MASK + 0x0076L)
/* integrity, protection, rule or event table already opened, but
** record access id not available */
#define                 E_RD0077_QEU_OPEN		(E_RD_MASK + 0x0077L)
/* QEU could not open query mod table */
#define                 E_RD0078_QEU_GET		(E_RD_MASK + 0x0078L)
/* QEU could not get data from query mod table */
#define                 E_RD0079_QEU_CLOSE		(E_RD_MASK + 0x0079L)
/* QEU could not close query mod table */
#define		E_RD007A_QEU_DELETE			(E_RD_MASK + 0x007AL)
/* RDF could not qeu-delete partition catalog rows */
#define		E_RD007B_QEU_APPEND			(E_RD_MASK + 0x007BL)
/* RDF could not qeu-append partition catalog rows */


/* RDF internal errors and consistency check begin with 100L */


#define                 E_RD0100_LOG_ERRORS		(E_RD_MASK + 0x0100L)
/* not an error code but a define used to indicate the beginning of an
** error range in which error codes which will cause RDF to log the error
** are to be placed */
#define			E_RD0101_FLUSH_CACHE		(E_RD_MASK + 0x0101L)
/* Consistency check - memory corruption detected via multiple bad checksums.
RDF is flushing its cache. */
#define                 E_RD0102_SYS_CATALOG		(E_RD_MASK + 0x0102L)
/* consistency check - more than one extended system catalog was opened at
** the same time by RDF within the same thread */
#define                 E_RD0103_NOT_OPENED		(E_RD_MASK + 0x0103L)
/* consistency check - attempt to read system catalog which was not opened */
#define                 E_RD0104_BUFFER			(E_RD_MASK + 0x0104L)
/* consistency check - internal buffer for reading extended system catalogs
** not large enough */
#define                 E_RD0105_NO_QDATA		(E_RD_MASK + 0x0105L)
/* consistency check - QEF_QDATA structure not initialized */
#define                 E_RD0106_CELL_COUNT		(E_RD_MASK + 0x0106L)
/* consistency check - iistatistics tuple contains invalid cell count */
#define                 E_RD0107_QT_SEQUENCE		(E_RD_MASK + 0x0107L)
/* consistency check - query tree tuple out of sequence */
#define                 E_RD0108_ADI_FIDESC		(E_RD_MASK + 0x0108L)
/* consistency check - error calling the ADI_FIDESC routine */
#define			E_RD0109_NOT_QTUPLE_ONLY	(E_RD_MASK + 0x0109L)
/* consistency check - rdu_qget is called for multiple reads, 
   but bitmask RDR_QTUPLE_ONLY is not set */
#define			E_RD010A_RQEU_NOT_QTUPLE	(E_RD_MASK + 0x010AL)
/* consistency check - rdu_qclose is called to close a table open for multiple
   reads, but RDR_QTUPLE_ONLY request bitmask is NOT set. */
#define			E_RD010B_NO_QEU_ACC_ID		(E_RD_MASK + 0x010BL)
/* consistency check - rdf was called to close a table.  However that table is
  not open. This was detected because qeucb.qeu_acc_id is null. */
#define                 E_RD010C_HIST_LENGTH		(E_RD_MASK + 0x010CL)
/* consistency check - iistatistics tuple contains invalid hist length */
#define			E_RD010D_BAD_CKSUM		(E_RD_MASK + 0x010DL)
/* Consistency check - checksum error on infoblk indicates potential memory
corruption. */
#define                 E_RD0110_QRYMOD			(E_RD_MASK + 0x0110L)
/* consistency check - internal error processing querymod operation */
#define                 E_RD0111_VIEW			(E_RD_MASK + 0x0111L)
/* consistency check - internal error processing view operation */
#define                 E_RD0112_ATTRIBUTE		(E_RD_MASK + 0x0112L)
/* consistency check - inconsistent attribute cache */
#define                 E_RD0113_KEYS			(E_RD_MASK + 0x0113L)
/* consistency check - inconsistent keying info in attribute cache */
#define                 E_RD0114_HASH			(E_RD_MASK + 0x0114L)
/* consistency check - inconsistent attribute hash info */
#define                 E_RD0116_0INDEX			(E_RD_MASK + 0x0116L)
/* request for 0 index tuples */
#define                 E_RD0117_STREAMID		(E_RD_MASK + 0x0117L)
/* consistency check - cannot find stream id for memory allocation */
#define                 E_RD0118_ULMOPEN		(E_RD_MASK + 0x0118L)
/* consistency check - error calling ULM open */
#define			E_RD0119_QOPEN_INFO		(E_RD_MASK + 0x0119L)
/* Consistency check - wrong self-index in qopen_info array */
#define                 E_RD011A_ULH_RELEASE		(E_RD_MASK + 0x011AL)
/* consistency check - error releasing ULH object */
#define                 E_RD011B_PRIVATE		(E_RD_MASK + 0x011BL)
/* consistency check - private memory stream expected */
#define                 E_RD011C_ULM_CLOSE		(E_RD_MASK + 0x011CL)
/* consistency check - error closing private memory stream */
#define                 E_RD011D_DAPERMIT		(E_RD_MASK + 0x011DL)
/* consistency check - error defining permit all to all */
#define                 E_RD011E_DRPERMIT		(E_RD_MASK + 0x011EL)
/* consistency check - error defining retrieve permit to all */
#define                 E_RD011F_DINTEGRITY		(E_RD_MASK + 0x011FL)
/* consistency check - error defining integrity */
#define                 E_RD0120_DPERMIT		(E_RD_MASK + 0x0120L)
/* consistency check - error defining integrity */
#define                 E_RD0121_DESTROY_VIEW		(E_RD_MASK + 0x0121L)
/* consistency check - error destroying view */
#define                 E_RD0122_DESTROY_PERMIT		(E_RD_MASK + 0x0122L)
/* consistency check - error destroying view */
#define                 E_RD0123_CREATE_VIEW		(E_RD_MASK + 0x0123L)
/* consistency check - error destroying view */
#define                 E_RD0124_DESTROY_INTEGRITY	(E_RD_MASK + 0x0124L)
/* consistency check - error destroying view */
#define                 E_RD0125_EXCEPTION		(E_RD_MASK + 0x0125L)
/* consistency check - unexpected exception in RDF */
#define                 E_RD0126_EXEXCEPTION		(E_RD_MASK + 0x0126L)
/* consistency check - unexpected exception processing exception in RDF */
#define                 E_RD0127_ULM_PALLOC		(E_RD_MASK + 0x0127L)
/* consistency check - error calling ulm_palloc */
#define                 E_RD0128_SERVER_MEMORY		(E_RD_MASK + 0x0128L)
/* consistency check - error releasing server memory to SCF */
#define                 E_RD0129_RDF_STARTUP		(E_RD_MASK + 0x0129L)
/* consistency check - error obtaining memory on RDF startup */
#define                 E_RD012A_ULH_CLOSE		(E_RD_MASK + 0x012AL)
/* consistency check - error shutting down ULH */
#define                 E_RD012B_ULM_CLOSE		(E_RD_MASK + 0x012BL)
/* consistency check - error closing down ULM memory stream for facility */
#define                 E_RD012C_ULH_SEMAPHORE		(E_RD_MASK + 0x012CL)
/* consistency check - cannot get access to ULH semaphore */
#define                 E_RD012D_ULH_ACCESS		(E_RD_MASK + 0x012DL)
/* consistency check - error from ULH ALIAS command */
#define                 E_RD012E_QUERY_TREE		(E_RD_MASK + 0x012EL)
/* consistency check - bad query tree */
#define                 E_RD012F_QUERY_TREE             (E_RD_MASK + 0x012FL)
/* attempt to write out an invalid query tree symbol type */
#define                 E_RD0130_PST_LEN                (E_RD_MASK + 0x0130L)
/* attempt to write out an invalid query tree symbol type - pst_len incorrect
*/
#define                 E_RD0131_QUERY_TREE             (E_RD_MASK + 0x0131L)
/* attempt to write out an invalid query tree header
** - an uninitialized field was found
*/
#define                 E_RD0132_QUERY_TREE             (E_RD_MASK + 0x0132L)
/* consistency check - invalid tree node was read in */
#define                 E_RD0133_OLD_QUERY_TREE		(E_RD_MASK + 0x0133L)
/* consistency check - invalid tree node was read in */

#define                 E_RD0134_INVALID_SECONDARY	(E_RD_MASK + 0x0134L)
/* consistency check - invalid tree node was read in */
#define                 E_RD0135_CREATE_PROCEDURE	(E_RD_MASK + 0x0135L)
/* consistency check - error writing procedure object to catalogs */
#define                 E_RD0136_CONCURRENT_PROC	(E_RD_MASK + 0x0136L)
/* consistency check - invalid concurrent access to procedure object */
#define                 E_RD0137_DUPLICATE_PROCS	(E_RD_MASK + 0x0137L)
/* consistency check - more than one procedure defined with same name */
#define                 E_RD0138_QUERYTEXT		(E_RD_MASK + 0x0138L)
/* consistency check - more than one procedure defined with same name */
#define                 E_RD0139_DROP_PROCEDURE		(E_RD_MASK + 0x0139L)
/* consistency check - cannot drop procedure due to QEF error */
#define                 E_RD013A_UNFIX_PROCEDURE	(E_RD_MASK + 0x013AL)
/* consistency check - cannot unfix procedure */
#define			E_RD013B_BAD_ATTR_COUNT		(E_RD_MASK + 0x013BL)
/* consistency check - bad attribute count of zero for relation */
#define			E_RD013C_BAD_QRYTRE_RNGSIZ	(E_RD_MASK + 0x013CL)
/* consistency check - too many query tree Range Table entries */
#define			E_RD013D_CREATE_COMMENT		(E_RD_MASK + 0x103DL)
/* error creating a comment text tuple (in iidbms_comment) */
#define			E_RD013E_DROP_COMMENT		(E_RD_MASK + 0x103EL)
/* error dropping a comment text tuple -NOT IMPLEMENTED YET, BUT IN FOR
** FUTURE GROWTH */
#define			E_RD013F_MISSING_RELINFO	(E_RD_MASK + 0x013FL)
/* consistency check - caller asked rdf_gdesc for attribute, index, key
**  or attribute name without first getting relation info */
#define			E_RD0140_BAD_RELKEY_REQ		(E_RD_MASK + 0x0140L)
/* consistency check - caller asked rdf_gdesc for key info without getting
**  relation info first. */
#define			E_RD0141_BAD_ATTHASH_REQ	(E_RD_MASK + 0x0141L)
/* consistency check - caller asked rdf_gdesc for attribute name hash table
**  without getting relation info first. */
#define			E_RD0142_ULH_ACCESS		(E_RD_MASK + 0x0142L)
/*  consistency check - error from ULH ACCESS command */
#define			E_RD0143_CREATE_SYNONYM		(E_RD_MASK + 0x0143L)
/* error creating an iisynonym tuple */
#define			E_RD0144_DROP_SYNONYM		(E_RD_MASK + 0x0144L)
/* error destroying an iisynonym tuple */
#define			E_RD0145_ALIAS_MEM		(E_RD_MASK + 0x0145L)
/* ULH returned an error while attempting to define an alias */
#define			E_RD0146_ILLEGAL_TREEVSN	(E_RD_MASK + 0x0146L)
/* Consistency check - invalid tree version.  This tree version is not supported
** for this server. */
#define			E_RD0147_BAD_QTREE_CLASS	(E_RD_MASK + 0x0147L)
/* consistency check - invalid tree type specified for class operation */
#define			E_RD0148_BAD_QTREE_ALIAS 	(E_RD_MASK + 0x0148L)
/* consistency check - invalid tree type specified for alias operation */
#define			E_RD0149_INVALID_TABLETYPE	(E_RD_MASK + 0x0149L)
/* consistency check - invalid tree type requested */
#define                 E_RD014A_NONEXISTENT_SYNONYM    (E_RD_MASK + 0x014AL)
/* Inconsistency in partitioning catalogs */
#define			E_RD014B_BAD_PARTCAT		(E_RD_MASK + 0x014BL)
/* Internal error copying partition definition */
#define			E_RD014C_PCOPY_ERROR		(E_RD_MASK + 0x014CL)

#define                 E_RD0150_CREATE_RULE            (E_RD_MASK + 0x0150L)
/* error creating rule */
#define                 E_RD0151_DROP_RULE              (E_RD_MASK + 0x0151L)
/* error dropping rule */
#define			E_RD0152_CREATE_EVENT		(E_RD_MASK + 0x0152L)
/* error creating event */
#define			E_RD0153_DROP_EVENT		(E_RD_MASK + 0x0153L)
/* error dropping event */
#define			E_RD0154_DROP_SCHEMA		(E_RD_MASK + 0x0154L)
/* error dropping schema */
#define			E_RD0155_CREATE_SEQUENCE	(E_RD_MASK + 0x0155L)
/* error creating a sequence */
#define			E_RD0156_ALTER_SEQUENCE		(E_RD_MASK + 0x0156L)
/* error altering a sequence */
#define			E_RD0157_DROP_SEQUENCE		(E_RD_MASK + 0x0157L)
/* error dropping a sequence */
#define                 E_RD0160_AGROUP                 (E_RD_MASK + 0x0160L)
/* cannot append tuples to iiusergroup */
#define                 E_RD0161_AAPLID                 (E_RD_MASK + 0x0161L)
/* cannot append tuples to iiapplication_id */
#define                 E_RD0162_DGROUP                 (E_RD_MASK + 0x0162L)
/* cannot delete tuples from iiusergroup */
#define                 E_RD0163_DAPLID                 (E_RD_MASK + 0x0163L)
/* cannot delete tuples from iiapplication_id */
#define                 E_RD0164_PGROUP                 (E_RD_MASK + 0x0164L)
/* cannot purge tuples in iiusergroup */
#define                 E_RD0165_RAPLID                 (E_RD_MASK + 0x0165L)
/* cannot replace tuples in iiapplication_id */
#define                 E_RD0166_GDBPRIV                (E_RD_MASK + 0x0166L)
/* cannot place granted database privileges into iidbpriv */
#define                 E_RD0167_RDBPRIV                (E_RD_MASK + 0x0167L)
/* cannot place revoked database privileges into iidbpriv */
#define                 E_RD0168_AUSER                  (E_RD_MASK + 0x0168L)
/* cannot append tuples to iiuser*/
#define                 E_RD0169_DUSER                  (E_RD_MASK + 0x0169L)
/* cannot purge tuples from iiuser*/
#define                 E_RD016A_RUSER                  (E_RD_MASK + 0x016AL)
/* cannot alter tuples in iiuser*/
#define                 E_RD016B_ALOCATION              (E_RD_MASK + 0x016BL)
/* cannot append tuples to iilocation*/
#define                 E_RD016C_DLOCATION              (E_RD_MASK + 0x016CL)
/* cannot delete tuples from iilocation*/
#define                 E_RD016D_RLOCATION              (E_RD_MASK + 0x016DL)
/* cannot replace tuples in iilocation*/
#define                 E_RD016E_RSECSTATE              (E_RD_MASK + 0x016EL)
/* cannot replace tuples to iisecuritystate */
#define			E_RD016F_R_DBP_STATUS		(E_RD_MASK + 0x016FL)
/* cannot update status of a dbproc */

#define			E_RD0180_ALABAUDIT		(E_RD_MASK + 0x0180L)
#define			E_RD0181_DLABAUDIT		(E_RD_MASK + 0x0181L)

#define                 E_RD0182_APROFILE               (E_RD_MASK + 0x0182L)
/* cannot append tuples to iiprofile*/
#define                 E_RD0183_DPROFILE               (E_RD_MASK + 0x0183L)
/* cannot purge tuples from iiprofile*/
#define                 E_RD0184_RPROFILE               (E_RD_MASK + 0x0184L)
/* cannot alter tuples in iiprofile*/

#define                 E_RD0185_ASECALARM               (E_RD_MASK + 0x0185L)
/* cannot append tuples to iiprofile*/
#define                 E_RD0186_DSECALARM               (E_RD_MASK + 0x0186L)
/* cannot purge tuples from iiprofile*/
#define                 E_RD0187_RSECALARM               (E_RD_MASK + 0x0187L)
/* cannot alter tuples in iiprofile*/
#define                 E_RD0188_AROLEGRANT              (E_RD_MASK + 0x0188L)
/* cannot add rolegrant tuples */
#define                 E_RD0189_DROLEGRANT              (E_RD_MASK + 0x0189L)
/* cannot delete rolegrant tuples */
#define                 E_RD018A_EVENT_FAIL              (E_RD_MASK + 0x018AL)
/* call to post RDF cache synchronization event failed */

/* no errors logged for 200 error codes */

#define                 E_RD01FF_RECOVERABLE_ERRORS	(E_RD_MASK + 0x01FFL)
/* not an error code - but a define used to indicate the beginning of a
** range of error codes which are recoverable, and RDF will not log, or
** report the error to the user
*/
#define                 E_RD0200_NO_TUPLES		(E_RD_MASK + 0x0200L)
/* user error - attempt to read query tree tuples but none were found
** for the query ID, probable cause is associated table has been deleted
*/
#define                 E_RD0201_PROC_NOT_FOUND		(E_RD_MASK + 0x0201L)
/* procedure tuple not found */
#define			E_RD0202_ALIAS_DEGRADED		(E_RD_MASK + 0x0202L)
/* RDF unable to define alias to synonym -- this is a warning, not an error */
#define			E_RD0210_ABANDONED_OBJECTS	(E_RD_MASK + 0x0210L)
/*
** user specified restricted revocation but some objects would become abandoned,
** so the statement will not be allowed to proceed
*/
#define			E_RD0211_NONEXISTENT_SCHEMA	(E_RD_MASK + 0x0211L)
/* schema named in DROP SCHEMA statement does not exist */
#define                 E_RD0212_NONEMPTY_SCHEMA	(E_RD_MASK + 0x0212L)
/* schema named in DROP SCHEMA ... RESTRICT statement was non-empty */

/* Titan error code starts at 250 */

#define                 E_RD0250_DD_QEFQUERY		(E_RD_MASK + 0x0250L)
/* send SQL query to QEF */
#define                 E_RD0251_DD_QEFFETCH		(E_RD_MASK + 0x0251L)
/* fetch tuple from QEF */
#define                 E_RD0252_DD_QEFFLUSH		(E_RD_MASK + 0x0252L)
/* send flush to QEF */
#define                 E_RD0253_DD_NOOBJECT		(E_RD_MASK + 0x0253L)
/* no object tuple in iidd_ddb_objects catalog */
#define                 E_RD0254_DD_NOTABLE		(E_RD_MASK + 0x0254L)
/* no local table tuple in iidd_ddb_ldb_tableinfo */
#define                 E_RD0255_DD_COLCOUNT		(E_RD_MASK + 0x0255L)
/* unable to get column count */
#define                 E_RD0256_DD_COLMISMATCH		(E_RD_MASK + 0x0256L)
/* consistency check - mapped column count != local column count */
#define                 E_RD0257_DD_NOLDB		(E_RD_MASK + 0x0257L)
/* ldb desc is not in iidd_ddb_ldbids catalog */
#define                 E_RD0258_DD_NOINGTBL		(E_RD_MASK + 0x0258L)
/* no tuple was found in iidd_ingres_tables */
#define                 E_RD0259_DD_NOPHYTBL		(E_RD_MASK + 0x0259L)
/* no tuple was found in iidd_physical_tables */
#define                 E_RD025A_ULMSTART		(E_RD_MASK + 0x025AL)
/* ulm_start error */
#define                 E_RD025B_ULMSHUT		(E_RD_MASK + 0x025BL)
/* ulm_shutdown error */
#define                 E_RD025C_INVALIDDATE		(E_RD_MASK + 0x025CL)
/* invalid date conversion */
#define                 E_RD025D_INVALID_DATATYPE	(E_RD_MASK + 0x025DL)
/* invalid attribute data type */
#define                 E_RD025E_QEF_LDBINFO		(E_RD_MASK + 0x025EL)
/* qef ldb info */
#define                 E_RD025F_NO_LDBINFO		(E_RD_MASK + 0x025FL)
/* ldb desc did not setup by caller */
#define                 E_RD0260_QEF_DDBINFO		(E_RD_MASK + 0x0260L)
/* rerieve ddbinfo error */
#define                 E_RD0261_QEF_USRSTAT		(E_RD_MASK + 0x0261L)
/* retrieve user stat error */
#define                 E_RD0262_QEF_IIDBDB		(E_RD_MASK + 0x0262L)
/* retrieve iidbdb error */
#define                 E_RD0263_QEF_LDBPLUS		(E_RD_MASK + 0x0263L)
/* retrieve ldbplus error */
#define                 E_RD0264_UNKOWN_OBJECT		(E_RD_MASK + 0x0264L)
/* unknown object type */
#define                 E_RD0265_STORAGE_TYPE_ERROR	(E_RD_MASK + 0x0265L)
/* unknown storeage type */
#define                 E_RD0266_NO_INDEXTUPLE  	(E_RD_MASK + 0x0266L)
/* no index tuple */
#define                 E_RD0267_NO_STATSTUPLE  	(E_RD_MASK + 0x0267L)
/* no statistics tuple was found */
#define                 E_RD0268_LOCALTABLE_NOTFOUND	(E_RD_MASK + 0x0268L)
/* local table does not exist in LDB */
#define                 E_RD0269_OBJ_DATEMISMATCH  	(E_RD_MASK + 0x0269L)
/* alter date in the local table mismatches alter date in CDB */
#define                 E_RD026A_OBJ_INDEXCOUNT 	(E_RD_MASK + 0x026AL)
/* consistency check error on index count */ 
#define                 E_RD026B_UNKNOWN_UPDATEMODE 	(E_RD_MASK + 0x026BL)
/* unknown update mode */ 
#define			E_RD026C_UNKNOWN_USER		(E_RD_MASK + 0x026CL)
/* unauthorized user */
#define			E_RD026D_USER_NOT_OWNER		(E_RD_MASK + 0x026DL)
/* user does not own the local table */
#define                 E_RD026E_RDFQUERY		(E_RD_MASK + 0x026EL)
/* already in tuple fetching mode */ 
#define			E_RD026F_RDFFETCH		(E_RD_MASK + 0x026FL)
/* fetch tuple before sending query */
#define			E_RD0270_RDFFLUSH	    	(E_RD_MASK + 0x0270L)
/* invalid buffer flush request */
#define			E_RD0271_UNKNOWN_STORAGETYPE   	(E_RD_MASK + 0x0271L)
/* unknown storage type for an object */
#define			E_RD0272_UNKNOWN_DDREQUESTS   	(E_RD_MASK + 0x0272L)
/* unknown distributed information requests */
#define			E_RD0273_LDBLONGNAME_NOTFOUND	(E_RD_MASK + 0x0273L)
/* unable to find ldb long name */
#define			E_RD0274_LDBDESC_NOTFOUND	(E_RD_MASK + 0x0274L)
/* unable to find ldb descriptor */
#define			E_RD0275_UNKNOWN_LDBCAPABILITY	(E_RD_MASK + 0x0275L)
/* unknown ldb capability */
#define			E_RD0276_INCONSISTENT_CATINFO   (E_RD_MASK + 0x0276L)
/* error detected in distributed catalog */
#define			E_RD0277_CANNOT_GET_ASSOCIATION (E_RD_MASK + 0x0277L)
/* cannot get ldb association */
#define			E_RD0278_MISMATCH_ARCHITECT_ID  (E_RD_MASK + 0x0278L)
/* qtree was created by different architecture */
#define			E_RD0279_NO_SESSION_CONTROL_BLK (E_RD_MASK + 0x0279L)
/* unable to retrieve rdf session control block from scf */
#define			E_RD027A_GET_ARCHINFO_ERROR     (E_RD_MASK + 0x027AL)
/* error detected when calling qef for architecture info */
#define			E_RD027B_IITABLES_NOTFOUND      (E_RD_MASK + 0x027BL)
/* iitables not found in local database */
#define			E_RD027C_TOOMANY_CLSNODES	(E_RD_MASK + 0x027CL)
/* too many entries in ii_config:clusters.cnf.  To fix, increment rdf_clsnodes*/


/* Define strings that are typically part of errors or warnings, but are
** for whatever reason not a complete message in themselves.
*/
#define S_RDF000_LIST_DEFAULT			(E_RD_MASK + 0xF000L)
#define S_RDF001_ONE_DEFAULT			(E_RD_MASK + 0xF001L)
#define S_RDF002_LIST_EQ			(E_RD_MASK + 0xF002L)
#define S_RDF003_LIST_UNIQUE			(E_RD_MASK + 0xF003L)
#define S_RDF004_RNG_FIRST_LT			(E_RD_MASK + 0xF004L)
#define S_RDF005_RNG_LAST_GT			(E_RD_MASK + 0xF005L)
#define S_RDF006_RANGE_OPER			(E_RD_MASK + 0xF006L)
#define S_RDF007_RANGE_GAP_OVERLAP		(E_RD_MASK + 0xF007L)

/*}
** Name: RDD_DEFAULT - default information for an attribute.
**
** Description:
**      This structure permits an attribute to be referenced to its default
**	value.  The rdf_default points to the default tuple and the 
**	rdf_def_object points to an RDF cache descriptor that RDF requires to
**	build/unfix/destroy default cache objects.
**
** History:
**     09-feb-93 (teresa) 
**	    initial creation for constraints
*/
typedef struct _RDD_DEFAULT
{
	PTR		rdf_def_object;     /* internal to RDF, (is of type
					    ** RDF_DE_ULHOBJECT in RDFINT.H) */
	u_i2		rdf_deflen;	    /* number of characters in the
					    ** default string */
	char		*rdf_default;	    /* ptr to a default tuple */
} RDD_DEFAULT;

/*}
** Name: RDD_QRYMOD - qrymod information of a table.
**
** Description:
**      This structure defines a general structure for integrity/protection
**	information associated with a table.  Currently, query text is only
**      returned for procedures, and query trees are only returned for
**      permits, and integrities.  This is also used for rules.
**
** History:
**     10-apr-86 (ac)
**          written
**     10-nov-86 (seputis)
**          rewritten for caching, resource managment
**     29-aug-88 (seputis)
**          changed to PST_PROCEDURE for lint
**     09-Oct-89 (teg) 
**	    merged titan/terminator.
*/
typedef struct _RDD_QRYMOD
{
	i4		qrym_id;	    /* Integrity/protection/procedure id. */
	struct _PST_PROCEDURE *qry_root_node;   /* The root node of the query 
					    ** tree, integrity/protection only */
	PTR		rdf_qtstreamid;     /* internal to RDF */
	i4		rdf_qtresource;     /* internal to RDF */
	PTR		rdf_ulhptr;	    /* internal to RDF */
	i4		rdf_l_querytext;    /* The length of the query text for
					    ** defining a protection,
					    ** integrity or procedure. */
	char		*rdf_querytext;	    /* The query text for defining a view
					    ** protection, integrity or procedure */
} RDD_QRYMOD;

/*}
** Name: RDD_CLUSTER_INFO - cluster information.
**
** Description:
**      This structure defines a general structure for cluster information.
**
** History:
**     21-mar-89 (mings)
**          created for Titan
*/
typedef struct _RDD_CLUSTER_INFO
{
	struct _RDD_CLUSTER_INFO    *rdd_1_nextnode;	/* point to next node */
	DD_NODE_NAME		    rdd_2_node; 
							/* null terminated node name */
} RDD_CLUSTER_INFO;

/*}
** Name: RDR_DDB_REQ - DDB request information for rdf_call().
**
** Description:
**      This structure provides all necessary DDB request information for RDF.
**
** History:
**     09-aug-88 (mings)
**          created for Titan
**     21-mar-89 (mings)
**          added cluster info support
**     21-jun-89 (mings)
**          added RDR_DD10_OPEN_CDBASSO to force open privilege cdb association.
**	    it is for abf modify association problem.
**     07-aug-90 (teresa)
**	    added RDR_DD11_SYSOWNER for +Y support
**     24-sep-90 (teresa)
**	    added DD_CLUSTERINFO structure.  Idealy this replaces
**	     rdr_d9_cluster_p and rdr_d8_nodes, but we are not sure if we want
**	     to remove those yet or simply duplicate about 40 bytes of info.
**	6-Jul-2006 (kschendel)
**	    Remove d6-ddbid and ddbflush, not used for anything.
*/
typedef struct _RDR_DDB_REQ
{
	RDF_TYPES	rdr_d1_ddb_masks;
#define		RDR_DD1_DDBINFO		0x0001L
				/* DDB information wanted */
#define		RDR_DD2_TABINFO		0x0002L
				/* local table information wanted */
#define		RDR_DD3_LDBPLUS		0x0004L
				/* LDB information wanted */
#define		RDR_DD4_USRSTAT		0x0008L
				/* user access information wanted */
#define		RDR_DD5_IIDBDB 		0x0010L
				/* retrieve control information for 
				** accessing IIDBDB */

	/* 0x0020 not used, available */

#define		RDR_DD7_USERTAB		0x0040L
				/* this mask is used with RDR_DD2_TABINFO
				** by PSF to indicate searching for user
				** table only */
#define		RDR_DD8_USUPER		0x0080L
				/* server wide user status request */
#define		RDR_DD9_CLUSTERINFO	0x0100L
				/* cluster infomation request */

#define		RDR_DD10_OPEN_CDBASSO   0x0200L
				/* force to open privilege cdb association */
#define		RDR_DD11_SYSOWNER	0x0400L
				/* get the owner name of system catalogs */
	DD_1LDB_INFO	*rdr_d2_ldb_info_p;
					/* Pointer to information block
					** (used for RDF_LDB_TABINFO and
					** RDF_LDBINFO), NULL if not used.
					** This pointer also for RDR_DD10_OPEN_CDBASSO
					** for cdb info */
	DD_DDB_DESC	*rdr_d3_ddb_desc_p;
					/* Pointer to DDB descriptor for
					** receiving data(used for
					** RDF_LDBINFO), NULL if not used*/
	DD_USR_DESC	*rdr_d4_usr_desc_p;
					/* Pointer to user descriptor for
					** receiving data(used for RDF_USRSTAT),
					** NULL if not used */
	DD_2LDB_TAB_INFO *rdr_d5_tab_info_p;
					/* Caller's receiving table information
					** block(used for RDR_DD2_TABINFO),
					** NULL if not used */
	i4		 rdr_d7_userstat;
					/* user status */
	i4		 rdr_d8_nodes;  /* number of nodes in the cluster. 
					** info not available if zero */
	RDD_CLUSTER_INFO *rdr_d9_cluster_p;
					/* point to cluster info */
	DD_NODELIST	 *rdr_d10_node_p;
					/* list of node costs */
	DD_NETLIST	 *rdr_d11_net_p;
    					/* list of net costs */ 
	DD_CLUSTER_INFO  rdr_d12_cluster;
					/* contains count and linked list of
					** nodenames for each node in cluster
					*/
} RDR_DDB_REQ;

/*}
** Name: RDF_INSTR - RDF Instruction Definition
**
** Description:
**      This type is used for giving specific instructions to RDF.  These
**	instructions will usually be to RDF_UPDATE, and will usually cause
**	RDF to set particular bits into QEUQ_CB. 
**
**	    RDF_VGRANT_OK - RDF must indicate to QEF that the DBA is 
**			    creating this view.  It does so by setting
**			    bit QEU_DBA_QUEL_VIEW in qeuq_cb.qeuq_flag_mask
**	    RDF_DBA_VIEW  - RDF must indicate to QEF that the owner of this
**			    view also owns all underlaying tables/views, so it
**			    is acceptable for the view owner to do grants on 
**			    this view to other users.  RDF does so by setting
**			    bit QEU_VGRANT_OK in qeuq_cb.qeuq_flag_mask.
**
**	The following bits effect RDF's massaging of data:
**
**	    RDF_TBL_CASE  - RDF will massage table names to the case that the
**			    target database desires, based on a value in
**			    iidbcapabilities.
**	    RDF_OWN_CASE  - RDF will massage owner names to the case that the
**			    target database desires, based on a value in
**			    iidbcapabilities.
**	    RDF_DELIM_TBL - Indicates user delimited LDB table name.  If
**			    set and if the LDB supports mixed case delimited
**			    ids, RDF will *not* massage case of table name.
**			    RDF_DELIM_TBL and RDF_TBL_CASE are mutually
**			    exclusive.
**	    RDF_DELIM_OWN - Indicates user delimited LDB owner name.  If
**			    set and if the LDB supports mixed case delimited
**			    ids, RDF will *not* massage case of owner name.
**			    RDF_DELIM_OWN and RDF_OWN_CASE are mutually
**			    exclusive.
**
**	NOTE:  these are bitmap type of values.  More than 1 instruction may
**	       be set at any given time.
**
** History:
**      21-may-90 (teg)
**          initial creation for FIPS
**	28-apr-92 (teresa)
**	    add RDF_TBL_CASE and RDF_OWN_CASE for star bug 42981.
**	09-jul-91 (andre)
**	    defined RDF_SPLIT_PERM for GRANT WGO project.
**	08-dec-92 (teresa)
**	    added RDF_REGPROC and RDF_FULLSEARCH.
**	07-jan-93 (andre)
**	    defined RDF_USE_CALLER_ULM_RCB
**	01-mar-93 (barbara)
**	    Delimited id support for Star.  Added new rdr_instr masks:
**	    RDF_DELIM_OWN and RDF_DELIM_TBL.	    
*/
typedef i4 RDF_INSTR;

#define	    RDF_NO_INSTR	0x0L	    /* used to initialize */
#define	    RDF_VGRANT_OK       0x01L	    /* create a view owned by the DBA */
#define	    RDF_DBA_VIEW	0x02L	    /* set DMT_VGRANT_OK in relstat */
#define	    RDF_TBL_CASE	0x04L	    /* verify that table name is in the
					    ** correct case for the target db */
#define	    RDF_OWN_CASE	0x08L	    /* verify that owner name is in the
					    ** correct case for the target db */
#define	    RDF_SPLIT_PERM	0x10L	    /* split this permit tuple */
#define	    RDF_REGPROC		0x20L	    /* indicates this is a REGISTERED
					    ** DBPROC (or a "regproc") */
#define	    RDF_FULLSEARCH	0x40L	    /* Indicates RDF should search both
					    ** the table namespace and the
					    ** registered procedure namespace.
					    ** NOTE: this bit should NOT be set
					    **	     if RDF_REGPROC is also set
					    */
#define	    RDF_USE_CALLER_ULM_RCB  0x80L   /* use caller's ULM_RCB when asked
					    ** to pack query tree or query text
					    ** tuples; also for partition copy
					    */
#define	    RDF_DELIM_OWN	0x0100L	    /* User delimited owner name */
#define	    RDF_DELIM_TBL	0x0200L	    /* User delimited table name */

/* Two modifiers for the RDF_PART_COPY operation.  The partition definition
** has an optional partition-name section, and an optional uninterpreted
** value text section (for break values, if used).  The caller can ask for
** either or both.  Normally, both are omitted from the copy (neither names
** nor value texts are needed for runtime operations).
*/
#define	RDF_PCOPY_NAMES		0x0400L	/* Copy the partition names */
#define RDF_PCOPY_TEXTS		0x0800L	/* Copy the raw value texts */


/*}
** Name: RDF_QT_PACK_RCB - request block containing additional info
**				     required for packing query text/tree tuples
**
** Description:
**	This structure contaisn additional information which must be supplied by
**	the caller requesting that RDF pack query gtext/tree into
**	IIQRYTEXT/IITREE format.  In particular, it will contains:
**	
**	    rdf_qt_tuples	    list of tuples which will be returned by RDF
**	    rdf_qt_tuple_count	    number of tuples in the above list
**	    rdf_qt_mode		    query mode to be stored in IIQRYTEXT/IITREE
**				    tuples
** History:
**	07-jan-92 (andre)
**	    written
*/
typedef	struct RDF_QT_PACK_RCB_   RDF_QT_PACK_RCB;

struct RDF_QT_PACK_RCB_
{
    QEF_DATA	    **rdf_qt_tuples;
    i4	    *rdf_qt_tuple_count;
    i4		    rdf_qt_mode;
};

/*}
** Name: RDR_RB - request block for all rdf_call().
**
** Description:
**      This structure defines the request block for requesting different 
**	information(e.g. relation, attribute, protect,etc) of a table.
**	Any combination of information of a table can be requested 
**	by setting the appropriate bits in the rdr_types_mask.
**
**      Many fields that specify operations just on integrities and permits
**      also apply to rules.
**
** History:
**     26-mar-86 (ac)
**          written
**     09-aug-88 (mings)
**          modified for Titan.
**     05-mar-89 (ralph)
**          GRANT Enhancements, Phase 1:
**          Added RDR_GROUP, RDR_APLID to rdr_types_mask flags;
**          Added RDR_REPLACE, RDR_PURGE to rdr_update_op flags;
**          Loaded rdr_qrytuple and rdr_qtuple_count for
**              iiapplication_id/iiusergroup tuples
**     06-mar-89 (neil)
**          Rules: Added RDR_RULE flag and rdr_rule field for rules.
**     31-mar-89 (ralph)
**          GRANT Enhancements, Phase 2:
**          Added RDR_DBPRIV to rdr_types_mask flags;
**          Loaded rdr_qrytuple and rdr_qtuple_count for iidbpriv tuples
**     26-apr-89 (mings)
**          register with refresh
**     17-jul-89 (jennifer)
**          Add in new defintions for manipulating iiuser, iilocation, 
**          iisecuritystate, and iidbaccess.
**      29-sep-89 (ralph)
**          Removed RDR_DBACCESS and related messages.
**     09-Oct-89 (teg)
**	    integrated STAR/TERMINATOR.
**     13-Oct-89 (teg)
**          added RDR_REMOVE bit to fix SYSCON bug where it is not
**          possible to do remove table/view/link command cuz the
**          LDB that the link is to no longer exists
**     17-Oct-89 (teg)
**	    added rdr_2types_mask to rdr_rb because we are running out of bits.
**	24-oct-89 (neil)
**	    Alerters: Added fields to process events (rdr_evname and RDR_EVENT).
**	16-jan-90 (ralph)
**	    Added RDR_SECALM to rdr_types_mask flags in RDR_RB.
**	01-feb-90 (teg)
**	    Added RDR_COMMENT and RDR_SYNONYM to rdr_2types_mask in RDR_RB.
**	28-feb-90 (carol)
**	    Moved RDR_COMMENT to rdr_types_mask in RDR_RB.
**	23-apr-90 (teg)
**	    Renamed RDR_SYNONYM to RDR2_SYNONYM to emphasize that this constant
**	    should be used with rdr_2types_mask instead of rdr_types_mask
**	21-may-90 (teg)
**	    Added RDF_INSTR word to structure.
**	20-sep-91 (teresa)
**	    Pick up ingres63p change 260973:
**	      7-jan-91 (seputis)
**		Added RDR2_TIMEOUT to support the OPF ACTIVE flag.
**	20-dec-91 (teresa)
**	    added RDR2_SYSCAT and RDR2_VALIDATE for SYBIL 
**	17-jan-92 (andre)
**	    added rdr_indep - a pointer to a structure describing objects and
**	    privileges on which a new object or permit depends.
**	16-mar-92 (teresa)
**	    added rdr_rlname to rdr_name, RDR2_ALIAS, RDR2_CLASS and 
**	    RDR2_KILLTREES to rdr_2types_mask, and rdr_sequence for SYBIL.
**	26-jun-92 (andre)
**	    defined RDR_2DBP_STATUS over rdr_2types_mask
**	29-jun-92 (andre)
**	    defined RDR_2INDEP_OBJECT_LIST and RDR_2INDEP_PRIVILEGE_LIST
**	    over rdr_2types_mask
**	22-jul-92 (andre)
**	    undefined RDR_PERMIT_ALL and RDR_RETRIEVE_ALL and defined
**	    RDR_DROP_ALL over rdr_types_mask
**	02-aug-92 (andre)
**	    defined RDR_REVOKE_ALL over rdr_types_mask and RDR_2REVOKE and
**	    RDR_2DROP_CASCADE over rdr_2types_mask
**	13-aug-92 (andre)
**	    added rdr_v2b_col_xlate and rdr_b2v_col_xlate
**	29-sep-92 (andre)
**	    renamed RDR_2* masks over rdr_2types_mask to RDR2_*
**	28-oct-92 (jhahn)
**	    Added RDR2_PROCEDURE_PARAMETERS for FIPS Constraints.
**	07-jan-93 (andre)
**	    We added two new opcodes: RDF_PACK_QTEXT (pack query text into
**	    IIQRYRTEXT tuples) and RDF_PACK_QTREE (pack query tree into IITREE
**	    tuples.)  When called with either of these two opcodes, RDF will
**	    expect to receive a ptr to a (pre-initialized) caller's ULM_RCB +
**	    some miscellaneous information needed to populate IIQRYTEXT/IITREE
**	    tuples and to return them to the caller (contained inside
**	    RDF_QT_PACK_RCB structure.)
**	    Accordingly, I added a pointer to ULM_RCB, a pointer to
**	    RDF_QT_PACK_RCB + defined RDF_USE_CALLER_ULM_RCB over rdr_instr.  
**	08-feb-93 (teresa)
**	    CONSTRAINTS PROJECT: added RDR2_CNS_INTEG, RDR2_KEYS, RDR2_DEFAULT,
**	    RDR2_INVL_DEFAULT to rdr_2types_mask,
**	    added rdr_cnstrname to rdr_name union.
**	12-feb-93 (jhahn)
**	    Added rdr_proc_params and rdr_proc_param_cnt.
**	19-feb-93 (andre)
**	    defined RDR2_SCHEMA
**      13-sep-93 (smc)
**          Made rdr_session_id a CS_SID.
**	21-oct-93 (andre)
**	    Added rdr_synname to rdr_name
**	25-apr-94 (rganski)
**	    Added rdr_histo_type, as part of fix for b62184.
**	16-may-00 (inkdo01)
**	    Added RDR2_COMPHIST for composite histograms.
**	19-dec-2000 (abbjo03)
**	    Add RDR_DATABASE for ALTER DATABASE.
**	7-mar-02 (inkdo01)
**	    Add RDR2_SEQUENCE for sequence manipulation and rdr_seqname 
**	    for later fetching.
**	29-Dec-2003 (schka24)
**	    Rename RDR_BLD_KEY to RDR_BLD_PHYS for partitioned tables.
**	    Add partition info pointer.
**	5-Aug-2004 (schka24)
**	    Add support for new partition/structure checker.
**      22-Mar-2005 (thaju02)
**          Add RDR2_DBDEPENDS to access iidbdepends by key.
**	6-Jul-2006 (kschendel)
**	    Unique dbid should be an i4, clarify db id's.
*/
typedef struct _RDR_RB
{
	i4		rdr_unique_dbid;/* Unique database id (from infodb) */
	PTR		rdr_db_id;	/* DMF database id for session */
	CS_SID		rdr_session_id; /* Session id. */
	DB_TAB_ID	rdr_tabid;	/* Table id. or Schema ID */
	union
	{
	    DB_TAB_NAME	rdr_tabname;	/* Table name. */
	    DB_DBP_NAME	rdr_prcname;	/* procedure name. */
	    DB_NAME	rdr_rlname;	/* rule name */
	    DB_EVENT_NAME rdr_evname;   /* Event_name - used with rdr_owner
					** for event tuple retrieval */
	    DB_NAME	rdr_seqname;	/* sequence name */
	    DB_CONSTRAINT_NAME
			rdr_cnstrname;	/* name of a constraint */
	    DB_SYNNAME	rdr_synname;	/* name of a synonym */
	}	rdr_name;		
	DB_OWN_NAME	rdr_owner;	/* Table/procedure/event owner. */
	RDF_TYPES	rdr_types_mask;	/* Type of information requested. */
#define		RDR_RELATION	    0x0001L /* Retrieve RELATION information. */
#define		RDR_ATTRIBUTES	    0x0002L /* Retrieve ATTRIBUTES information.*/
#define		RDR_INDEXES	    0x0004L /* Retrieve INDEXES information. */
#define		RDR_VIEW	    0x0008L /* Retrieve, append or delete VIEW information. */
#define		RDR_PROTECT	    0x0010L /* Retrieve, append or delete PROTECT information. */
#define		RDR_INTEGRITIES	    0x0020L /* Retrieve, append or delete INTEGRITIES information. **/
#define		RDR_STATISTICS	    0x0040L /* Retrieve IIHISTOGRAM and IISTATISTICS information. */
#define		RDR_BY_NAME	    0x0080L /* Retrieve table information by name. */
#define		RDR_HASH_ATT	    0x0100L /* Request for building hash table of attributes. */
#define		RDR_BLD_PHYS	    0x0200L /* Request for building physical
					    ** storage stuff: the structure-
					    ** key array and the partitioning
					    ** info.  Part of an RDF_GETDESC
					    ** operation.
					    */

					    /*
					    ** drop all permits or
					    ** security_alarms (depending on
					    ** whether RDR_PROTECT or RDR_SECALM
					    ** is set)
					    */  
#define		RDR_DROP_ALL	    0x0400L

					    /*
					    ** user requested that ALL
					    ** [PRIVILEGES] be revoked, i.e.
					    ** privileges were not specified
					    ** explicitly
					    */
#define		RDR_REVOKE_ALL	    0x0800L
/* 0x1000L is now available cuz RDR_QTUPLE_ONLY is removed */
#define		RDR_QTREE	    0x2000L /* Retrieve tree information only. */
#define		RDR_PROCEDURE	    0x4000L /* Retrieve procedure information */
#define		RDR_CHECK_EXIST	    0x8000L /* Indicates drop link action. RDF will only
					       retrieve object information */
#define         RDR_USERTAB	  0x010000L /* Indicates check the local user owner name.
					    ** used by PSF for drop table in distributed mode */
#define		RDR_DST_INFO	  0x020000L /* indicates distributed information was attached. */
#define		RDR_REFRESH	  0x040000L /* indicates register with refresh */
#define		RDR_SET_LOCKMODE  0x080000L /* indicates set lockmode. RDF will not send out
					    ** ldb query for schema checking */
#define         RDR_GROUP	0x0100000L /* Manipulate iiusergroup catalog */
#define         RDR_APLID	0x0200000L /* Manipulate iiapplication_id */
#define         RDR_RULE	0x0400000L /* Process rule information.  Used 
                                        ** with RDR_QTUPLE_ONLY to fetch a rule,
                                        ** and alone to update a rule.
                                        */
#define         RDR_DBPRIV	0x0800000L  /* Manipulate iidbpriv */
#define         RDR_USER	0x1000000L  /* Manipulate iiuser */
#define         RDR_LOCATION	0x2000000L  /* Manipulate iilocation */
#define         RDR_SECSTATE	0x4000000L  /* Manipulate iisecuritystate */
#define         RDR_SECALM	0x8000000L  /* Process security_alarm info --
					    ** Used to drop security alarms. */
#define         RDR_REMOVE      0x10000000L /* Remove command - skip ldb
                                            ** inquiry for this object  */
#define		RDR_EVENT       0x20000000L /* Retrieve/update event info.
					    ** This can be used together with
					    ** RDR_BY_NAME to fetch an event, or
					    ** RDR_QTUPLE_ONLY|RDR_PROTECT to
					    ** fetch event permission.
					    */
#define		RDR_COMMENT	0x40000000L /* manipulate iidbms_comment */

	RDF_TYPES       rdr_2types_mask;    /* Type of info requested -- cont'd.
                                            ** This is a growth word cuz
                                            ** rdr_types_mask is filling up. */
#define		RDR2_SYNONYM	0x00000001L /* manipulate synonyms catalog */
#define		RDR2_TIMEOUT	0x00000002L /* use timeout for all DMF access */
#define         RDR2_NOSYNONYM  0x00000004L /* skip checking the iisynonym
                                            ** catalog */
#define         RDR2_SYSCAT     0x00000008L /* indicates that user has catalog
                                            ** update privilege AND is dropping
                                            ** a system catalog.  STAR does not
                                            ** know if the particular CDB site
                                            ** requires system catalogs to be
                                            ** owned by system catalog owner or
                                            ** by $ingres, so it seems pointless
                                            ** to do consistency checking for
                                            ** the owner of the local catalog
                                            ** IE,  if this bit is set,
                                            ** STAR does NOT do a consistency
                                            ** check to assure that user owns
                                            ** the local object before trying to
                                            ** drop that local object.
                                            */
#define		RDR2_REFRESH    0x00000010L /* is set RDF will refresh the
					    ** cache entry for a requested obj
					    ** by deleting its existing entry
					    ** for that object & recreating it.
					    */
#define		RDR2_ALIAS	0x00000020L /* Build alias to QTREE object to
					    ** invalidate */
#define		RDR2_CLASS	0x00000040L /* Wipe out all objects in specified
					    ** QTREE cache class 
					    */
#define		RDR2_KILLTREES	0x00000080L /* When invalidating an object on
					    ** the relation class, also
					    ** invalidate any assocaited QTREE
					    ** cache objects
					    */
#define		RDR2_QTREE_KILL 0x00000100L /* Invalidate the entire qtree 
					    ** cache, but leave the relation
					    ** cache untouched.
					    */
#define		RDR2_DBP_STATUS	0x00000200L /* enter the list of independent
					    ** objects/privileges into
					    ** IIDBDEPENDS and IIPRIV and/or
					    ** update db_mask1 in IIPROCEDURE
					    */
#define		RDR2_INDEP_OBJECT_LIST 0x0400L /* read a block of IIPRIV tuples
					    ** which belong to a descriptor
					    ** describing privileges on which a
					    ** specified dbproc depends
					    */
#define		RDR2_INDEP_PRIVILEGE_LIST 0x0800L /* read IIDBDEPENDS tuples 
					    ** that describe objects on which a
					    ** specified dbproc depends
					    */
#define		RDR2_REVOKE	  0x01000L  /* processing REVOKE 
					    */
#define		RDR2_DROP_CASCADE 0x02000L  /* processing cascading destruction
					    */
#define		RDR2_INVL_LDBDESC 0x04000L  /* INVALIDATE the LDBdesc cache. */
#define		RDR2_PROCEDURE_PARAMETERS 0x8000L /* read
					    ** II_PROCEDURE_PARAMETER tuples */
#define		RDR2_KEYS	  0x010000L /* Read iikey tuples */
#define		RDR2_CNS_INTEG	  0x020000L /* Get a constraint integrity --
					    ** identified by schema id and 
					    ** constraint name 
					    */
#define		RDR2_DEFAULT	  0x040000L /* get defaults for the attributes*/
#define		RDR2_INVL_DEFAULT 0x080000L /* INVALIDATE the defaults cache */
#define		RDR2_SCHEMA	  0x100000L /* processing related to destruction
					    ** of schemas */
#define		RDR2_PROFILE	  0x1000000L /* User profiles */
#define         RDR2_ROLEGRANT    0x2000000L /* role grants */
#define		RDR2_COMPHIST	  0x4000000L /* loading composite histogram */
#define		RDR2_SEQUENCE	  0x8000000L /* manipulate iisequence catalog */
#define         RDR2_DBDEPENDS    0x10000000L /* retrieve iidbdepends info
                                              ** using part key */
#define         RDR2_COLCOMPARE   0x20000000L /* column comparison info */

/* Indicates info will be used in a read only manner so that a ptr to the
** shared info_blk and be returned
*/
	i4		rdr_hattr_no;	/* OPF uses this field for specifying
					** the attribute number of the requested
					** histogram information. */
	i4		rdr_no_of_attr;	/* OPF uses this field for specifying
					** the total number of attributes in
					** the table. */
	i4		rdr_cellsize;	/* OPF uses this field for specifying
					** cell data size of a histogram. */
	DB_DT_ID	rdr_histo_type; /* OPF uses this field for specifying
					** attribute's histogram datatype */
	i4		rdr_update_op;	/* PSF uses this field for specifying
					** what operation needed to be
					** performed. */
#define		RDR_APPEND	1	/* Define integrity, protect or view. */
#define		RDR_DELETE	2	/* Destroy integrity, protect or view. */
#define		RDR_GETNEXT	3	/* Retrieve the next set of integrity/protect tuples. */
#define		RDR_CLOSE	4	/* Close the iiintegrity or iiprotect table. */
#define		RDR_OPEN	5	/* Open the iiintegrity or iiprotect 
					** table and get first set of tuples */
#define         RDR_REPLACE     6       /* Replace iiapplication_id tuples */
#define         RDR_PURGE       7       /* Purge iiusergroup member tuples */
	PTR		rdr_rec_access_id; /* The record access id for
					   ** retrieving the next set of 
					   ** integrity or protection tuples. 
					   ** This is required for RDR_GETNEXT 
					   ** and RDR_CLOSE. */
	PTR		rdr_qry_root_node;/* The root node of the query tree 
					** that defines the view, integrity or
					** protect. */
	PTR		rdr_qrytuple;	/* Pointer to tuples for update and
					** generic tuple pointer for simple
					** retrievals that do not require or
					** allow tuple caching.
					*/
	i4		rdr_qrymod_id;	/* Protection or integrity number that
					** that wants to be destroyed or
					** retrieved. 0 means destroy
					** all the protection or integrity of a 
					** table. */
	i4		rdr_l_querytext; /* The length of the query text for
					 ** defining a view, protection or 
					 ** integrity. */
	i4		rdr_qtuple_count; /* Number of tuples */
#define	RDF_MAX_QTCNT	    20		/* this limit is based on a QEF limit,
					** and the constant is put here merely
					** for convenience */
	i4		rdr_cnt_base_id;/* Count of the base table ids for 
					** defining a view. */
	PTR		rdr_querytext;	/* The query text for defining a view
					** protection or integrity. */
	PTR		rdr_dmu_cb;	/* A pointer to the DMU_CB for creating 
					** a view. */
	DB_TAB_ID	*rdr_base_id;	/* An array of base table ids for
					** defining a view. */
	DB_TREE_ID	rdr_tree_id;	/* Tree id for retrieving tree tuples
					** of an integrity or protection 
					** definition. */
	DB_QRY_ID	rdr_qtext_id;	/* query text id for destroying a view
					** protection or integrity 
					** definition. */
	PTR             rdr_fcb;        /* A pointer to the internal structure
					** which contains the global memory
					** allocation and caching control
					** information of a facility. */
	i4		rdr_status;	/* status word for view creation */
	RDD_QRYMOD	*rdr_protect;	/* Pointer to protection information. */
	RDD_QRYMOD	*rdr_integrity; /* Pointer to integrity information. */
	RDD_QRYMOD	*rdr_procedure; /* Pointer to procedure information. */
	RDD_QRYMOD      *rdr_rule;      /* Ptr to rule info for rdf_getinfo */
	DB_PART_DEF	*rdr_newpart;	/* Pointer to caller-private copy of
					** new partitioning scheme
					*/
	i4		rdr_newpartsize;  /* Size of new partition copy */
	RDF_INSTR	rdr_instr;	/* Any Special instructions to RDF */
	i2		rdr_sequence;	/* indicates sequence # for a specified
					** permit or integrity */

	PTR		rdr_indep;	/* descriptors of objects and privileges
					** on which new objects (e.g. views,
					** dbprocs) and permits depend
					*/
					/*
					** function pointer to be passed to QEF
					** when processing REVOKE - this
					** function will translate a map of 
					** attributes of an updateable view into
					** a map of attributes of a specified
					** underlying table or view 
					*/
	DB_STATUS	(*rdr_v2b_col_xlate)();
					/*
					** function pointer to be passed to QEF
					** when processing REVOKE - this
					** function will translate a map of
					** attributes of a table or view into a
					** map of attributes of a specified
					** updateable view which is defined on
					** top of that table or view
					*/
	DB_STATUS	(*rdr_b2v_col_xlate)();

					/*
					** ULM_RCB supplied by the caller (must
					** also set RDF_USE_CALLER_ULM_RCB in
					** rdr_instr)
					*/
	PTR 		rdf_fac_ulmrcb;

					/*
					** info required for whan called to pack
					** query text/tree tuples
					*/
	RDF_QT_PACK_RCB	*rdr_qt_pack_rcb;

	/* Titan changes start here */
        DB_DISTRIB      rdr_r1_distrib; /*Star server if DB_DSTYPES,
				        **backend server otherwise,
					**used to start up RDF */	     
	RDR_DDB_REQ	rdr_r2_ddb_req; /*DDB request information */
	i4		rdr_r3_row_width;
					/* row width for the distributed view
					** creation */
	DD_OBJ_TYPE	*rdr_r4_obj_typ_p;
					/* object type array to match
					** rdr_base_id: DD_1OBJ_LINK,
					** DD_2OBJ_TABLE, or DD_3OBJ_VIEW */
       QEF_DATA        *rdr_proc_params;/* Pointer to procedure parameters
                                        ** on create procedure*/
       i4              rdr_proc_param_cnt;
                                       /* Count of procedure parameters
                                       ** on create procedure */
        /* Titan changes end here */ 

	/* The below two things are used for RDF_PART_COPY.  Some callers
	** need fancier memory allocators than just ulm-palloc.  (PSF,
	** for instance, wants a partition def copied into QSF memory.)
	** So, the partition copier wants the address of a caller routine
	** to allocate memory.  The routine takes three parameters:
	** the first parameter is a void * and can be any pointer to
	** caller-convenient state;  the second parameter is the size in
	** bytes of the required area.  The third parameter is where the
	** address of the allocated memory is returned.  The routine can
	** set up error codes/messages anywhere it likes, but should not
	** normally issue any messages itself.
	*/
	DB_STATUS	(*rdr_partcopy_mem)();	/* Memory allocator routine */
	void		*rdr_partcopy_cb;  /* First param passed to above */
	char		*rdr_errbuf;	/* Caller supplied message area */

	/* For partition/struct checker, pass storage struct info here, pass
	** partition definition above in rdr_newpart
	*/
	i4		rdr_storage_type; /* DB_xxx_STORE structure type */
	struct _RDD_KEY_ARRAY *rdr_keys;  /* Caller supplied key list */

}   RDR_RB;


/*}
** Name: RDD_HISTO - RDF version of the histogram information.
**
** Description:
**      This structure is a simplified version of the zopt1stat and
**	zopt2stat tuples. This structure represents the logical rather than
**	the physical definition of a tuple from the zopt1stat and zopt2stat 
**      relations; 
** History:
**	28-mar-86 (ac)
**          written
**	09-nov-92 (rganski)
**	    Added new fields charnunique and chardensity for new character
**	    histogram statistics.
**	18-jan-93 (rganski)
**	    Added sversion and shistlength fields (Character Histogram
**	    Enhancements project).
**	9-jan-96 (inkdo01)
**	    Added f4repf to RDD_HISTO for per-cell repetition factors.
*/
typedef struct _RDD_HISTO
{
    f4              snunique;         /* Number of unique values in the
                                      ** in the column. */
    f4              sreptfact;        /* Repetition factor - the inverse
                                      ** of the number of unique values
                                      ** i.e. number of rows / number
                                      ** of unique values. */
    f4              snull;            /* percentage (fraction of 1.0) of
                                        ** relation which contains NULL for
                                        ** this attribute */
    i2              snumcells;         /* The number of cells in the
                                      ** histogram. */
    i1              sunique;          /* TRUE if all values for that
                                      ** column are unique. */
    i1              scomplete;        /* TRUE - if all values which currently
                                      ** exist in the database for this
                                      ** domain also exists in this attribute.
                                      */
    f4              *f4count;         /* Cell counts for histograms */
    f4              *f4repf; 	      /* Repetition factor array */
    PTR             datavalue;        /* ptr to interval boundaries for
                                      ** histograms. */
    i2              sdomain;          /* - Domain of this attribute.
                                      ** - gives meaning to
                                      ** the db_complete domain flag i.e. the
                                      ** db_domains must be equal and the both
                                      ** attributes of a join complete, if
                                      ** the resultant attribute is to be
                                      ** complete.
                                      */
    i4		    *charnunique;     /* array: number of unique characters
				      ** per character position */
    f4		    *chardensity;     /* array: character set density per
				      ** character position */
    char	    sversion[DB_STAT_VERSION_LENGTH + 1];
                                      /* version number of these statistics */
    i2		    shistlength;      /* Length of boundary values in histogram
				      ** associated with these statistics */
    i2		    reserved;
} RDD_HISTO;


/*}
** Name: RDD_KEY_ARRAY - Structure for primary key information.
**
** Description:
**      This structure defines the information which compose of 
**	the primary key of a table.
**
** History:
**     12-may-86 (ac)
**          written
*/
typedef struct _RDD_KEY_ARRAY
{
	i4	*key_array;	/* Pointer to the key array, each entry
				** of the array is the attribute number
				** which composes of the primary key 
				** associated with the table. The key array
				** starts at entry 0 for key 1, and
				** key_array[key_count] is 0 to terminate
				** the key array. */
	i4	key_count;	/* Number of attributes in the primary key. */
} RDD_KEY_ARRAY;


/*}
** Name: RDD_QDATA - RDF qrymod data block.
**
** Description:
**      This structure defines a general structure for qrymod data block.
** History:
**     10-apr-86 (ac)
**          written
**	19-apr-90 (teg)
**	    fix bug 8011.  The qrym_cnt field was sometimes used to indicate
**	    the amount of memory RDF allocated and was other times used to
**	    indicate the number of tuples that QEF found during a read.
**	    This has been broken into two separate fields.
*/
typedef struct _RDD_QDATA
{
	QEF_DATA	*qrym_data;	    /* The integrity/protection tuples.
					    */
	i4		qrym_cnt;	    /* Number of integrity/protection
					    ** tuples retrieved. */
	i4		qrym_alloc;	    /* number of integrity/protection
					    ** tuples that RDF has allocated
					    ** memory to hold 
					    */
} RDD_QDATA;


/*}
** Name: RDD_VIEW - view information block.
**
** Description:
**      This structure defines the view information block.
** History:
**     10-apr-86 (ac)
**          written
*/
typedef struct _RDD_VIEW
{
	PTR		qry_root_node;	    /* The root node of the query 
					    ** tree. */
} RDD_VIEW;


/*}
** Name: RDD_BUCKET_ATT - hash bucket of attributes information.
**
** Description:
**      This structure defines the hash bucket of attributes information.
**	Each bucket is a linked list of pointer to the corresponding 
**	attribute in the rdr_info.rdr_attr attribute array.
**
** History:
**     12-apr-86 (ac)
**          written
*/
typedef struct _RDD_BUCKET_ATT
{
	DMT_ATT_ENTRY		*attr;
	struct _RDD_BUCKET_ATT	*next_attr;
} RDD_BUCKET_ATT;


/*}
** Name: RDD_ATTHSH - hash table of attributes information.
**
** Description:
**      This structure defines the hash table of attributes information.
**	PSF uses this structure for speeding up range variable attributes 
**	lookup.
**
** History:
**     12-apr-86 (ac)
**          written
*/
typedef struct _RDD_ATTHSH
{
	RDD_BUCKET_ATT	*rdd_atthsh_tbl[RDD_SIZ_ATTHSH];
} RDD_ATTHSH;


/*}
** Name: RDR_INFO - Table information block.
**
** Description:
**      This structure is used for storing all the information of a
**	table. The pointer is NULL if the corresponding information is
**	not requested.
**
** History:
**     26-mar-86 (ac)
**          written
**     12-jan-87 (puree)
**	    modified for cache version.
**     09_aug-88 (mings)
**          modified for Titan.
**      06-mar-89 (neil)
**          Rules: Added rdr_rtuples for rules.
**     26_apr-89 (mings)
**          register with refresh.
**	9-Oct-89 (teg)
**	    Merged STAR/TITAN
**	27-may-92 (teresa)
**	    added rdr_star_obj for sybil.
**	26-nov-93 (robf)
**          added rdr_atuples for security alarm tuples
**	29-Dec-2003 (schka24)
**	    Add partitioned table stuff; remove unused rdr_next.
**	5-jan-06 (dougi)
**	    Add rdr_colcompare to address column comparison stats.
*/
typedef struct _RDR_INFO
{
	DMT_TBL_ENTRY	*rdr_rel;	/* Pointer to table entry. */
	DMT_ATT_ENTRY	**rdr_attr;	/* Pointer to array of pointers to the 
					** attribute entry. */
	PTR		rdr_attr_names;   /* memory allocated for att names */
	DMT_IDX_ENTRY	**rdr_indx;	/* Pointer to array of pointers to index
					** entry array. */
	DMT_PHYS_PART	*rdr_pp_array;	/* Pointer to the physical partition
					** array, if the table has one
					** NOTE: You get this with an
					** RDR_ATTRIBUTE request, rather than
					** only with RDR_BLD_PHYS.  This is
					** wrong, but the latter implies the
					** former, and it can save a dmt-show.
					*/
	DMT_COLCOMPARE	*rdr_colcompare; /* Pointer to column comparison 
					** structure/array */
	RDD_QDATA	*rdr_ituples;	/* Pointer to integrity tuples. */
	RDD_QDATA	*rdr_ptuples;	/* Pointer to protection tuples. */
	RDD_QDATA       *rdr_rtuples;   /* Pointer to rule tuples. */
	RDD_QDATA       *rdr_ktuples;   /* Pointer to key tuples. */
	RDD_QDATA       *rdr_pptuples;  /* Pointer to procedure parameter */
	RDD_QDATA	*rdr_atuples;	/* Pointer to security alarm tuples */
	RDD_VIEW	*rdr_view;	/* Pointer to view information. */
	RDD_HISTO	**rdr_histogram;/* Pointer to array of pointers to RDF
					** revised version of zopt1stat and 
					** zopt2stat tuples. */
	RDD_KEY_ARRAY	*rdr_keys;	/* Pointer to the primary key of a 
					** table. */
	DB_PART_DEF	*rdr_parts;	/* Pointer to partition definition */
	RDD_ATTHSH	*rdr_atthsh;	/* Pointer to the hash table of 
					** attributes information. */
	i4		rdr_no_attr;	/* Number of attributes in the table. */
	i4		rdr_no_index;	/* Number of indexes on the table. */
	i4		rdr_invalid;	/* TRUE if the block was invalidated */
	i4		rdr_attnametot; /* relattnametot */
	RDF_TYPES	rdr_types;	/* Type (contents) of this block */
	RDF_TYPES	rdr_2_types;	/* Type (contents) of this block */
	DB_TAB_TIMESTAMP rdr_timestamp;	/* Time stamp of this table information
					** block. */
	PTR		stream_id;	/* the memory pool stream id where 
					** the table information block is
					** allocated. */
	PTR		rdr_object;	/* Ptr to the RDF_ULHOBJECT for this
					** table information block.  If we
					** are a private infoblk, this points
					** to a private rdf_ulhobject, which
					** will point to the master infoblk,
					** which will point to the master
					** RDF_ULHOBJECT! */
	DB_PROCEDURE	*rdr_dbp;	/* ptr to iiprocedure tuple */
        /* Titan changes start here */
	DD_OBJ_DESC     *rdr_obj_desc;  /* ptr to STAR object descriptor */
	i4	        rdr_ldb_id;	/* ldb id used internal to identify
				        ** a ldb in a cdb. */
	PTR		rdr_ddb_id;	/* ddb id */
	PTR		rdr_star_obj;	/* ptr to the Star cache ULH object for
					** the LDB descriptor associated with
					** this infoblk
					*/
	i4		rdr_local_info; /* mask shows local table schema and stats info */
#define	RDR_L0_NO_SCHEMA_CHANGE	    0x0001L
					/* no change in local schema */
#define	RDR_L1_SUPERSET_SCHEMA	    0x0002L
					/* local schema is a superset */
#define	RDR_L2_SUBSET_SCHEMA	    0x0004L
					/* local schema is a subset */
#define	RDR_L3_SCHEMA_MISMATCH	    0x0008L
					/* local schema is different from old schema */
#define	RDR_L4_STATS_EXIST	    0x0010L
					/* local table statitics is available */
	i4		rdr_checksum;   /* checksum */
        /* Titan changes end here */
}   RDR_INFO;

/*}
** Name: RDF_CB - RDF control block
**
** Description:
**      This structure defines the control block for communication with
**	RDF.  The interface to RDF is done via the rdf_call(op_code, &rdf_cb). 
**
** History:
**     26-mar-86 (ac)
**          written
**     09-aug-88 (mings)
**          modified for Titan.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**      21-Nov-2008 (hanal04) Bug 121248
**          Added caller_ref which is a pointer to a pointer that references
**          the rdf_info_blk in local caller structures. If rdf_info_blk is
**          freed and *caller_ref is equal to the rdf_info_blk freed then
**          RDF will NULL the caller reference to the freed info block.
*/
typedef struct _RDF_CB
{
	struct _RDF_CB	*q_next;	/* Forward pointer. */
	struct _RDF_CB	*q_prev;	/* Backward pointer. */
	SIZE_TYPE	length;		/* Length of the control block. */
	i2		type;		/* Type of the control block. */
#define	    RDF_CB_TYPE		2
	i2		s_reserved;
	PTR		l_reserved;
	PTR    		owner;
	i4		ascii_id;
#define	    RDF_CB_ID	    CV_C_CONST_MACRO('R', 'C', 'B', ' ')
    	DB_ERROR	rdf_error;	/* Operation status. */
	RDR_INFO	*rdf_info_blk;	/* Pointer to the requested table 
					** information block. */
        RDR_INFO	**caller_ref;   /* Address of caller ptr ref
                                        ** to rdf_info_blk */
    	RDR_RB		rdf_rb;		/* Information request block. */
	/* Titan changes start here */
	DB_DISTRIB      rdf_c1_distrib; /* initializing or terminating
					** STAR server if DB_DSTYPES,
					** backend server otherwise */
	/* Titan changes end here */
} RDF_CB;

/*}
** Name: RDF_CCB - Control block for RDF controlling services.
**
** Description:
**      This structure defines the control block for initializing and
**	terminating RDF for the server and facility. This interface to RDF 
**	is done via the rdf_call(op_code, &rdf_ccb). 
**
** History:
**     26-mar-86 (ac)
**          written
**     02-mar-87 (puree)
**	    modified for global pointer to server control block.
**     09-aug-88 (mings)
**	    add rdf_distrib (DB_DISTRIB) for STAR.  
**     17-jan-89 (mings)
**	    add rdf_max_sessions for max number of sessions allowed.  
**     04-apr-89 (mings)
**	    add session control information
**     13-dec-1989 (teg)
**	    add memory size startup parameters:  
**	    rdf_max_tbl, rdf_colavg, rdf_idxavg, rdf_multicache, rdf_cachesize.
**     05-apr-1990 (teg)
**	    added rdf_num_synonym
**     31-aug-1990 (teresa)
**	    added rdf_flags_mask for communicating assorted info.  Defined
**	    1st bit in mask
**     21-jan-92 (teresa)
**	    added rdf_maxddb, rdf_avgldb, rdf_clsnodes, rdf_netcost and
**	    rdf_nodecost for SYBIL
**     29-jul-92 (teresa)
**	    Removed obsolete fields rdf_cdb_p [type = DD_1LDB_INFO *], 
**	      rdf_session_id [type = PTR], rdf_timestamp [type=DB_TAB_TIMESTAMP]
**	      and rdf_curr_mode [type = nat].
**	    Added a new field:	rdf_sesize to return size of RDF session control
**	      block to SCF.
**	    Also moved the following fields to the newly created RDF_SESS_CB:  
**	      rdf_strace->rds_strace.
**	    This was done to handle inconsistency in how 6.4 Star SCF/RDF 
**	    interface fetched the RDF_CCB.  RDF interface has been modified to 
**	    be consistent with other facilities.  This means RDF uses a session
**	    control block instead of piggy-backing on the RDF_CCB.
**	06-apr-93 (ralph)
**	    DELIM_IDENT:
**	    Added rdf_dbxlate and rdf_cat_owner to RDF_CCB.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**	10-Jan-2001 (jenjo02)
**	    Added rdf_rdscb_offset, rdf_adf_cb, rdf_sess_id.
**	1-Dec-2003 (schka24)
**	    Remove obsolete/unref'ed idxavg, clsnodes, netcosts, nodecosts.
*/
typedef struct _RDF_CCB
{
	struct _RDF_CCB *q_next;	/* Forward pointer. */
	struct _RDF_CCB	*q_prev;	/* Backward pointer. */
	SIZE_TYPE	length;		/* Length of the control block. */
	i2		type;		/* Type of the control block. */
#define	    RDF_CCB_TYPE	1
	i2		s_reserved;
	PTR		l_reserved;
	PTR    		owner;
	i4		ascii_id;
#define	    RDF_ID_CCB	    CV_C_CONST_MACRO('R', 'C', 'C', 'B')
	/****************** input parameters *******************************/
	i4		rdf_fac_id;	/* Facility id. */
	i4		rdf_max_tbl;	/* Maximum number of tables allowed in
					** the facility cache. */
	i4		rdf_colavg;	/* Misnomer, it's really the size of
					** the column-defaults cache
					*/
	bool		rdf_multicache; /* flag.  True->user set multicache
					** startup option. */
	SIZE_TYPE	rdf_cachesiz;	/* approx number of bytes of memory rdf
					** cache may occupy. (It will round
					** to nearest 32K) */
	i4             rdf_num_synonym;/* number of synonyms per table */
	PTR		rdf_poolid;	/* Memory pool id. All the table
					** information will be allocated in the
					** memory pool specified by the poolid.
					*/
	SIZE_TYPE	rdf_memleft;	/* Amount of memory left in the memory
					** pool for streams allocation. */
	i4		rdf_maxddb;	/* max number of DDB descriptors allowed
					** on the cache at 1 time. */
	i4		rdf_avgldb;	/* average number of LDBs per DDB. */
	i4		rdf_flags_mask;	/* bitmap to communicate assorted info*/
	u_i4	*rdf_dbxlate;	/* Case translation semantics flags;
					** See <cui.h> for values.
					*/
	DB_OWN_NAME	*rdf_cat_owner;	/* Catalog owner name; one of:
					** "$ingres" if reg ids lower case, or
					** "$INGRES" if reg ids upper case.
					*/
        /* Titan changes start here */
        DB_DISTRIB      rdf_distrib;    /* initializing or terminating
				        ** STAR server if DB_DSTYPES,
				        ** backend server otherwise*/
	i4		rdf_maxsessions;/* max number of sessions allowed */
	PTR		rdf_ddb_id;	/* ddb id */
	i4		rdf_rdscb_offset; /* Offset from SCF's SCB to *RDF_SESS_CB */
	PTR		rdf_adf_cb;	/* Pointer to session's ADF_CB */
	CS_SID		rdf_sess_id;	/* This session's id */
	/* Titan changes end here */
#define	    RDF_CCB_SKIP_SYNONYM_CHECK	0x0001L
	/****************** input or output parameter ***********************/
	PTR		rdf_fcb;	/* A pointer to the internal structure
					** which contains the global memory
					** allocation and caching control
					** information of a facility. */
	/****************** output parameters *******************************/
	PTR		rdf_server;	/* ptr to RDF server control block */
	i4		rdf_sesize;	/* size of RDF's Session Control Block*/
    	DB_ERROR	rdf_error;	/* Operation status. */

} RDF_CCB;

/*}
** Name: RDF_SESS_CB - RDF Session Control block
**
** Description:
**      This structure defines the control block for maintaining session
**	specific RDF variables and data.  The contents are preserved between
**	RDF calls.  They are initialized on the RDF_BGN_SESSION call and are
**	automatically destroyed by SCF when a session terminates.
**
** History:
**     29-jul-92 (teresa)
**	    Initial creation.  Fields rdf_ddb_id and rdf_strace moved here from
**	    RDF_CCB to make the RDF/SCF interface for session specific data 
**	    match the rest of the server.
**	13-aug-92 (teresa)
**	    fix RDF_ID_CCB -> RDFID_SESS
**	30-mar-93 (teresa)
**	    Add rds_special_condt field.
**	06-apr-93 (ralph)
**	    DELIM_IDENT:
**	    Added rds_dbxlate and rds_cat_owner to RDF_SESS_CB.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**	10-Jan-2001 (jenjo02)
**	    Added rds_adf_cb.
*/
typedef struct _RDF_SESS_CB
{
	struct _RDF_SESS_CB *q_next;	/* Forward pointer. */
	struct _RDF_SESS_CB *q_prev;	/* Backward pointer. */
	SIZE_TYPE	length;		/* Length of the control block. */
	i2		type;		/* Type of the control block. */
#define	    RDF_SESS_TYPE	4
	i2		s_reserved;
	PTR		l_reserved;
	PTR    		owner;
	i4		ascii_id;
#define	    RDF_ID_SESS	    CV_C_CONST_MACRO('R', 'S', 'E', 'S')

	bool		rds_distrib;	/* flag: true if distributed session
					**	 false if not distributed sess
					*/
	PTR		rds_ddb_id;	/* ddb id */

	i4		rds_special_condt;  /* this is a bit mask of special
					    ** conditions (usually specified at
					    ** session startup) */
#define	RDS_NO_SPECIAL_CONDIT	0L	    /* no special conditions are set */
#define	RDS_DONT_USE_IISYNONYM	0x0001L	    /* skip checking iisynonym catalog*/
	u_i4	*rds_dbxlate;	/* Case translation semantics flags;
					** See <cui.h> for values.
					*/
	DB_OWN_NAME	*rds_cat_owner;	/* Catalog owner name; one of:
					** "$ingres" if reg ids lower case, or
					** "$INGRES" if reg ids upper case.
					*/
	PTR		rds_adf_cb;	/* Pointer to session's ADF_CB */
	ULT_VECTOR_MACRO(RDF_NB, RDF_NVAO) rds_strace;	
					/* Trace vector for the session. */
} RDF_SESS_CB;
