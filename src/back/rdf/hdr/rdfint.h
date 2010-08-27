/*
**Copyright (c) 2004 Ingres Corporation
**
*/

/*}
** Name: RDFINT.H - Typedefs for RDF internal data structures.
**
** Description:
**      This file contains the structure typedefs and definitions of
**	symbolic constants for RDF internal uses.  It defines the relationships
**	of the various RDF cache structures.  These are depected in pictorial
**	form to assist in understanding the RDF cache:
**
**
**	THE RELATION CACHE: (relcache)
**	===================
**
**	     RDR_INFO			    RDF_ULHOBJECT
**	    :--------------:		   :----------------:<----------+
**	    :   :  :       :<--------------: rdf_sysinfoblk :		:
**	    : rdr_rel	   :		   : rdf_ulhptr     :-------+	:
**	    :  etc         :		   : rdf_state      :	    :	:
**	    : rdr_timestamp:		   :	:  :	    :	    :	:
**	    : stream_id    :		   :		    :	    :	:
**	    : rdr_object   :-------------->:        	    :	    :	:
**	    :   :  :       :		   :    	    :	    :	:
**	    :--------------:		   :----------------:	    :	:
**			 					    :	:
**								    :	:
**	     CS_SEMAPHORE	     ULH_OBJECT			    :	:
**	    :------------:	    :-----------------------:	    :	:
**	    :		 :	    :			    :<------+	:
**	    : (used to 	 :	    : ulh_id		    :		:
**	    : get	 :	    : ulh_objname	    :		:
**	    : semaphores :	    : ulh_namelen	    :		:
**	    : from SCF)  :<---------: ulh_usem		    :		:
**	    :		 :	    : ulh_uptr		    :-----------+
**	    :------------:	    :-----------------------:
**
** Note: there are private RDR_INFO's and RDF_ULHOBJECT's too!  The
** ULH_OBJECT header always points to the master.  Private infoblks and
** rdf_ulhobjects point at the master, and use the ULH_OBJECT's semaphore.

**	THE LDBDESC CACHE:
**	==================
**
**	     RDR_INFO			    RDF_ULHOBJECT
**	    :-----------------:		   :--------------------:<----------+
**	    :   :  :          :<-----------: rdf_sysinfoblk	:	    :
**	    : rdr_rel	      :		   : rdf_state		:	    :
**	    :  etc            :		   :   :    :		:	    :
**	+---: rdr_obj_desc    :		   : rdf_ulhptr         :-------+   :
**	:   : rdr_timestamp   :		   :   :    :           :       :   :
**	:   : stream_id       :		   : rdf_shared_sessions:	:   :
**	:   : rdr_object      :----------->:   :    :	        :       :   :
**	:   : rdr_ldb_id      :		   : rdf_starptr        :---+	:   :
**	:   : rdr_ddb_id      :		   : 			:   :	:   :
**	:   : rdr_star_obj    :-+	   :			:   :	:   :
**	:   : rdr_checksum    :	:	   :			:   :	:   :
**	:   :   :  :          :	:	   :			:   :	:   :
**	:   :-----------------:	:	   :--------------------:   :	:   :
**	:		 	:				    :	:   :
**	:    DD_OBJ_DESC	:				    :	:   :
**	:-->:----------------:	:	   RDF_DULHOBJECT	    :	:   :
**	    :dd_o9_tab_info. :  :	   :---------------:	    :	:   :
**	+---: dd_t9_ldb_p    :  +--------->:		   :<-------+	:   :
**	:   :----------------:             :		   :		:   :
**	:				   : rdf_state	   :		:   :
**	:   DD_1LDB_INFO		   : rdf_ulhptr    :----+	:   :
**	:   :------------:		   : rdf_pstream_id:	:	:   :
**	+-->: LDBDESC    :		   : rdr_ddb_id    :	:	:   :
**	    : cache Entry:		   : rdr_ldb_id    :	:	:   :
**	    :            :<----------------: rdd_ldbdesc   :	:	:   :
**	    :	         :		   :		   :	:	:   :
**	    :------------:		   :---------------:	:	:   :
**							        :	:   :
**								:	:   :
**	     CS_SEMAPHORE	    ULH_OBJECT			:	:   :
**	    :------------:	    :-----------------------:	:	:   :
**	    :		 :	    : ulh_id		    :<--+	:   :
**	    : (used to 	 :	    : ulh_objname	    :<----------+   :
**	    : get	 :	    : ulh_namelen	    :		    :
**	    : semaphores :<---------: ulh_usem		    :		    :
**	    : from SCF)	 :	    : ulh_uptr		    :---------------+
**	    :------------:	    :-----------------------:
**
**	NOTE: RDR_INFO.rdr_star_obj does not appear to be initialized...
**	      [ 11-nov-93 (teresa) ]

**	THE DEFAULTS CACHE:
**	===================
**
**	     RDR_INFO			    RDF_ULHOBJECT
**	    :-----------:		   :----------------:
**	    :   :  :    :<-----------------: rdf_sysinfoblk :
**	    : rdr_object:----------------->:    :    :	    :
**	    : rdr_attr  :---+		   : rdf_ulhptr     :--------------:
**	    :   :  :    :   :		   :    :    :	    :		   :
**	    :-----------:   :		   :----------------:		   :
**			    :						   :
**	    DMT_ATT_ENTRY   :						   :
**	    :-----------:   :						   :
**	    :   :  :    :<--+						   :
**	+---:att_default:						   :
**	:   :-----------:						   :
**	:								   :
**	:     RDD_DEFAULT		     RDF_DE_ULHOBJECT		   :
**	:   :----------------:		    :---------------:		   :
**	+-->:		     :		    : rdf_state	    :		   :
**	    : rdf_def_object :------------->:		    :<----------+  :
**	    : rdf_deflen     :		    : rdf_ulhptr    :-------+	:  :
**	    :		     :		    : rdf_pstream_id:	    :	:  :
**	    :                :<-------------: rdf_attdef    :	    :	:  :
**	    :		     :		    : rdf_ldef	    :	    :	:  :
**	+---: rdf_default    :		    : rdf_defval    :---+   :	:  :
**	:   :----------------:		    :---------------:	:   :	:  :
**	:							:   :	:  :
**	:	    :-------------------------------:		:   :	:  :
**	+---------->: var length character string   :<----------+   :	:  :
**		    :(contains ASCII representation :		    :	:  :
**		    : of default value)		    :		    :	:  :
**		    :-------------------------------:		    :	:  :
**								    :	:  :
**	     CS_SEMAPHORE	     ULH_OBJECT			    :	:  :
**	    :------------:	    :-----------------------:	    :	:  :
**	    :		 :	    : ulh_id		    :<------+	:  :
**	    : (used to 	 :	    : ulh_objname	    :		:  :
**	    : get	 :	    : ulh_namelen	    :		:  :
**	    : semaphores :<---------: ulh_usem		    :		:  :
**	    : from SCF)	 :	    : ulh_uptr		    :-----------+  :
**	    :	         :	    :			    :	           :
**	    :	         :	    :			    :<-------------+
**	    :------------:	    :-----------------------:
**
**
** History: 
**     02-apr-86 (ac)
**          written
**     12-jan-87 (puree)
**	    modify RDI_FCB for ULH hash table id for the info blocks.
**	    add definition for ULH call and Max # of table info blocks in
**	     a hash table.
**     02-mar-87 (puree)
**	    modify for global pointer to server control block.
**     09-aug-88 (mings)
**	    add qef_rcb, qef_cb to RDF_GLOBAL for Titan.
**          add rdv_distrib (DB_DISTRIB) to RDI_SVCB for Titan.
**     06-mar-89 (neil)
**          Rules: Modified CB's and added flags for rules processing.
**     23-jun-89 (mings)
**	    add rdf_shared_sessions in RDF_ULHOBJECT to record number of sessions
**	    currently using the object
**     06-Oct-1989 (teg)
**	    Merge Star and Local versions or RDFINT.H.  (used ifdef DISTRIBUTED
**	    to keep out things that we dont yet want in for local.)  This merge
**	    is necessary NOW to support new RDF caching needed to resolve bug
**	    5446.
**	27-oct-89 (neil)
**          Rules: Modified names of fields for lint.
**	    Alerters: Added new fields to RDI_FCB.
**    13-dec-89	 (teg)
**	    defined constants to calculate local cache size and also to 
**	    calculate hash table density.  Defined desired hash table density
**	    as 150 buckets.
**    09-feb-90 (teg)
**	    brought over two new constants from titan to keep hdrs in
**	    synch:  RDF_INTERNAL_ERROR, RDF_MTRDF
**    05-apr-90 (teg)
**	    defined RDF_SYNCNT and added synonym support
**    05-feb-91 (teresa)
**	    pick up statistics chagnes from 6.3
**    28-feb-91 (teresa)
**	    moved RDF_CHILD and RDF_QTREE to rdftree.h
**    07-nov-91 (teresa)
**	    Picked up INGRES63P changes 263297 and ~
**	    13-Aug-91 (dchan)
**		Removed duplicate #define for RDF_INTERNAL_ERROR and
**		RDF_MTRDF.  It was causing too many warnings at compile time.
**	    21-jan-91 (seputis)
**		added define for default timeout
**	21-jan-91 (teresa)
**	    SYBIL changes, includeing: 
**		renamed: RDF_NETNODES->RDF_NETCOST; RDF_NUMCPUS->RDF_NODECOST;
**		added RDF_AVGLDBS, RDF_STARBUCKETS
**		Removed: RDF_DENSITY, RDU_INFO_DUMP, RDU_REL_DUMP, RDU_ATTR_DUMP
**			 RDU_INDX_DUMP, RDU_ATTHSH_DUMP, RDU_KEYS_DUMP,
**			 RDU_HIST_DUMP, RDU_ITUPLE_DUMP, RDU_PTUPLE_DUMP
**	30-mar-92 (teresa)
**	    remove references to ME and ST routines from Forward and/or External
**	    function references section.
**	07-jan-93 (teresa)
**	    add rdf_fulmcb to RDU_GLOBAL to hold an initialized ULM_RB from 
**	    another facility.  If the caller supplies this in RDF_RB, then RDF 
**	    will use the caller's ulm_cb for memory allocation instead of 
**	    allocating from RDF memory.
**	08-apr-93 (ralph)
**	    DELIM_IDENT:
**	    Introduce RDF_FCB.rdi_utiddesc, which is the same as rdi_tiddesc,
**	    but has an upper-cased att_name ("TID").  Yuck.
**	07-jul-93 (teresa)
**	    Moved prototype of rdf_trace to rdf.h so SCF can see it as well as
**	    RDF.
**	11-nov-93 (teresa)
**	    Updated description section of this file to give a pictorial 
**	    relationship of the structures that comprise the RDF cache. 
**	    Also added RDF_INVAL_FIXED to RDF_RTYPES, and added field 
**	    rdf_fix_toinvalidate to RDF_GLOBAL to fix bug 56471.
**	29-nov-93 (robf)
**          Added security alarm support, new rdf_ssecalarm and rds_X20_atuple
**	    fields
**	15-Aug-1997 (jenjo02)
**	    Call CL directly for semaphores instead of going thru SCF.
**	    Removed rdf_fscfcb, rdf_scfcb as a result.
**	23-Sep-1997 (jenjo02)
**	    Changed type of rdv_global_sem to CS_SEMAPHORE from SCF_SEMAPHORE
**	    for consistency.
**	07-nov-1998 (nanpr01)
**	    Added RDF_SNOTINIT define to initializze the control block with 
**	    this value.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Jan-2001 (jenjo02)
**	    (Re)Added GLOBALDEF/REF for *RDI_SVCB, added GET_RDF_SESSCB
**	    macro, rds_adf_cb, deprecating rdu_gcb(), rdu_adfcb(),
**	    rdu_gsvcb() functions.
**	22-feb-2001 (somsa01)
**	    Changed rdv_rdscb_offset to be of type SIZE_TYPE.
**	3-Dec-2003 (schka24)
**	    Be more liberal with hash buckets.
**	29-Dec-2003 (schka24)
**	    RDF support for partitioned tables.
**	    Better doc for a few structure members.
**	    (For jenjo02) fix RDF timeout, it's in seconds, not millisec.
**	2-Feb-2004 (schka24)
**	    Remove duplicate prototype of rdf-call.
**      12-Apr-2004 (stial01)
**          Define length as SIZE_TYPE.
**      7-oct-2004 (thaju02)
**          Define rdv_pool_size, rdv_in0_memory, rdv_memleft as SIZE_TYPE.
**	6-Feb-2008 (kibro01) b114324
**	    Added rdu_drop_attachments
**	10-Sep-2008 (jonj)
**	    SIR 120874: Rename rdu_ierror() to rduIerrorFcn() invoked by
**	    rdu_ierror macro. Rename rdu_warn() to rduWarn() invoked by
**	    rdu_warn macro. Replace facilities err_code with its DB_ERROR
**	    pointer in rdu_ferror() prototype.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**      20-aug-2010 (stial01)
**          rdu_mrecover() Added ULM_RCB param.
*/

/* Macro to return pointer to session's RDF_SESS_CB directly from SCF's SCB */
#define	GET_RDF_SESSCB(sid) \
	*(RDF_SESS_CB**)((char *)sid + Rdi_svcb->rdv_rdscb_offset)
/*
**	Miscellaneous defines
*/
#define	    RDF_TIMEOUT	30	/* default lock timeout is 30 seconds */

#define	RDF_INTERNAL_ERROR (0L)		/* RDF_INTERNAL_ERROR used by error 
					** handling mechanism to indicate an 
					** internal error detected was by RDF */

    /* 
    **  these constants are used to define LOCAL and STAR RDF cache size and 
    **  also number of hashbuckets in local RDF CACHES 
    ** The hash bucket counts could be parameterized but for now I'll
    ** content myself with increasing them.  (schka24).
    */

#define RD_NOTSPECIFIED	-1		/* indicates a startup parameter was not
					** specified */
#define RDF_BUCKETS	501		/* desired max number of hash buckets
					** for ulh to allocate/use for RDF
					** caches */
#define RDF_STARBUCKETS 51		/* max number of hash buckes for ULH to
					** allocate/use for DDB_OBJECT cache */
#define RDF_BLKSIZ	(32 * 1024)	/* rdf cache block size = 32K bytes  */
					/* (relatively meaningless) */
#define RDF_MEMORY	340 * (32*1024) /* default memory size is 
					**	340 * RDF_BLKSIZ */
#define RDF_MIN_MEMORY	(128 * 1024)	/* RDF cannot do even minimal 
					** functionality with less than this
					** amount of memory 
					*/
#define RDF_MTRDF	300		/* default number of objects in cache */
#define	RDF_COLDEFS	15		/* default num. of columns defaults */
#define RDF_SYNCNT      1		/* default number of synonyms / table */
#define RDF_AVGLDBS	5		/* default number of LDBs per DDB */

/* Operation codes to request dumps -- currently not called by anyone but
** supported in RDF code */
#define	RDU_INFO_DUMP	    20
#define	RDU_REL_DUMP	    21
#define	RDU_ATTR_DUMP	    22
#define	RDU_INDX_DUMP	    23
#define	RDU_ATTHSH_DUMP	    24
#define	RDU_KEYS_DUMP	    25
#define	RDU_HIST_DUMP	    26
#define	RDU_ITUPLE_DUMP	    27
#define	RDU_PTUPLE_DUMP	    28


/* distrubuted session wide trace starts at 50 */
#define RDU_ADDOBJ_DUMP	    59
#define RDU_DELOBJ_DUMP	    60

/* server wide trace starts at 90 */

#define RDU_DIAGNOSIS	    90
#define RDU_FLUSH_ALL	    92
#define RDU_LDB_DESCRIPTOR  94
#define RDU_SUMMARY	    95
#define RDU_HELP	    99

/*}
** Name: RDF_SVAR - internal cache hit statistics type
**
** Description:
**      This type is used for gathering information on internal RDF operations
[@comment_line@]...
**
** History:
**      23-nov-87 (seputis)
**          initial creation
*/
typedef i4 RDF_SVAR;

/*}
** Name: RDF_TABLE - Identifiers for table operations
**
** Description:
**      This type is used to identify tables RDF must open, access and close
**
** *Note* There is a array in rdfgetinfo.c that is indexed by an RDF_TABLE
** number.  If you change any of the #define assignments here, or add new
** ones, make sure you find and fix that array.
**
** History:
**      11-sep-92 (teresa)
**          initial creation
**	18-feb-93 (teresa)
**	    added RD_IIDEFAULT
**	05-mar-93 (teresa)
**	    added RD_IISCHEMA, RD_IIINTEGRITYIDX
**	18-mar-02 (inkdo01)
**	    Added RD_IISEQUENCE.
**	1-Jan-2004 (schka24)
**	    Added partitioning catalogs.
**	5-jan-06 (dougi)
**	    Added RD_IICOLCOMPARE.
*/
typedef i4 RDF_TABLE;
#define			RD_IIDBDEPENDS		1
#define 		RD_IIDBXDEPENDS		2
#define 		RD_IIEVENT		3
#define 		RD_IIHISTOGRAM		4
#define 		RD_IIINTEGRITY		5
#define 		RD_IIPRIV		6
#define 		RD_IIPROCEDURE		7
#define 		RD_IIPROTECT		8
#define 		RD_IIQRYTEXT		9
#define 		RD_IIRULE		10
#define 		RD_IISTATISTICS		11
#define 		RD_IISYNONYM		12
#define 		RD_IITREE		13
#define 		RD_IIXEVENT		14
#define 		RD_IIXPROCEDURE		15
#define			RD_IIKEY		16
#define                 RD_IIPROCEDURE_PARAMETER 17
#define                 RD_IIDEFAULT		18
#define                 RD_IISCHEMA		19
#define                 RD_IIINTEGRITYIDX	20
#define			RD_IISECALARM		21
#define			RD_IISEQUENCE		22
#define			RD_IIDISTSCHEME		23
#define			RD_IIDISTCOL		24
#define			RD_IIDISTVAL		25
#define			RD_IIPARTNAME		26
#define			RD_IICOLCOMPARE		27
/* RDF doesn't need to deal with the iipartition catalog */

#define			RD_MAX_TABLE		27  /* Keep this up-to-date */
						/* Did you fix rdfgetinfo.c? */

/*}
** Name: RDF_BUILD - internal build type
**
** Description:
**      This type is used for gathering information on how requests get into
**	rdf cache
** History:
**      31-jan-91 (teresa)
**          initial creation
*/
typedef i4  RDF_BUILD;

/* requested rdf object was not on cache and not found in catalogs */
#define	RDF_NOTFOUND		0

/* requested rdf object was already in the RDF cache */
#define	RDF_ALREADY_EXISTS	1

/* requested rdf object was built from dbms catalogs and placed into cache */
#define	RDF_BUILT_FROM_CATALOGS 2

/*}
** Name: RDI_FCB - A facility control block for all rdf_call() of the facility.
**
** Description:
**      This is a internal structure used by the calling facilities to inform
**	the global memory allocation and caching control information of a 
**	facility to RDF.  There is one RDI_FCB per facility.  This block
**	is shared by all sessions (within the facility).  Most of the elements
**	in this structure are for read only except the rdi_memleft counter and
**	rdi_infoq pointer.  Mutaul exclusion for rdi_memleft is done by ULM
**	itself.  Mutual exclusion for rdi_infoq is through the rdi_sem
**	semaphore.
**
** History:
**     02-apr-86 (ac)
**          written
**     12-jan-87 (puree)
**	    modify RDI_FCB for ULH hash table id for the info blocks.
**     24-mar-87 (puree)
**	    add rdi_infoq.
**      06-mar-89 (neil)
**          Rules: Added rdi_rlxxx fields.
**	06-oct-89 (teg)
**	    Merged Local/Distributed RDF.
**	27-oct-89 (neil)
**          Rules: Modified names of rdi_rlt_xxx fields rdi_rt_xxx.
**	    Alerters: Added new fields to RDI_FCB.
**	14-dec-89 (teg)
**	    add a pointer to RDF server control block to eleminate rdf using
**	    GLOBALDEF Rdi_svcb.
**	09-apr-90 (teg)
**	    add counters for synonyms requests/hits
**	01-feb-91 (teresa)
**	    removed RDF_SVAR statistic counters from this structure (and placed
**	    new ones in RDF_STATISTCS, which is in RDF_SVCB.
**	23-jan-92 (teresa)
**	    SYBIL:  removed the following elements:  rdi_poolid; rdi_memleft; 
**		    rdi_alias; rdi_hashid, rdi_qthashid
**	08-apr-93 (ralph)
**	    DELIM_IDENT:
**	    Introduce RDF_FCB.rdi_utiddesc, which is the same as rdi_tiddesc,
**	    but has an upper-cased att_name ("TID").  Yuck.
**	10-Jan-2001 (jenjo02)
**	    Deleted rdi_svcb.
**	    
*/
typedef struct _RDI_FCB
{
	PTR	rdi_streamid;		/* stream id associated with the facility
					** control block */
	DMT_ATT_ENTRY	*rdi_tiddesc;	/* description of the attribute, tid,
					** for PSF attribute lookup. */
	DMT_ATT_ENTRY	*rdi_utiddesc;	/* description of the attribute, TID,
					** for PSF attribute lookup. */
	i4	rdi_fac_id;		/* facility id */
	i4	reserved;
} RDI_FCB;

/*}
** Name: RDD_HMAP - map of histograms which have been looked
**
** Description:
**      This is a map of histograms which have been looked up by RDF, and is
**      used by RDF to avoid lookups of histograms which to not exist.  In this
**      case the histogram ptr is always NULL so some indicator is needed to
**      differenciate between this and the case in which the histogram has not
**      been looked up.
**
** History:
**      6-nov-87 (seputis)
**          initial creation
[@history_template@]...
*/
typedef char RDD_HELEMENT;
typedef RDD_HELEMENT RDD_HMAP[DB_MAX_COLS];

/*}
** Name: RDF_TRTYPE - tree type for iitree catalog lookup
**
** Description:
**      This is the type of the attribute in the iitree catalog which
**      describes the tree type, i.e. view, integrity, permit
[@comment_line@]...
**
** History:
**      15-nov-87 (seputis)
**          initial creation
[@history_line@]...
[@history_template@]...
*/
typedef i2 RDF_TRTYPE;

/*}
** Name: RDF_STATE - describes state of object
**
** Description:
**      Determines the state of an object in terms of which thread use it
**
** History:
**      6-nov-87 (seputis)
**          initial creation
**	20-may-92 (teresa)
**	    added RDF_SBAD for sybil.
[@history_template@]...
*/
typedef i4  RDF_STATE;
#define                 RDF_SUPDATE     1
/* object is currently being updated */
#define                 RDF_SSHARED     2
/* object is not being modified and is available for shared access */
#define                 RDF_SPRIVATE    3
/* object is private and will not be accessed by other threads, it should be
** deleted when the table is unfixed */
#define                 RDF_SNOACCESS   4
/* object is not being accessed and is available to be locked for update
** mode but cannot be shared */
#define			RDF_SBAD	5
/* there was an error while creating this object, so concurrent threads should
** not try to use it. */
#define                 RDF_SNOTINIT    6
/* object not initialized */

/*}
** Name: RDF_DULHOBJECT - structure defining a distributed ULH object for rdf
**
** Description:
**      This structure defines the distributed object stored in ULH.  It is  
**	the header info for the distributed ULH object.
**
** History:
**	18-may-92 (teresa)
**	    Initial creation for Sybil (replaces RDD_LDB_DESC structure)
[@history_template@]...
*/
typedef struct _RDF_DULHOBJECT
{

    /*
    ** housekeeping info 
    */
    RDF_STATE       rdf_state;          /* state of access to object, this
                                        ** variable must be updated and looked
                                        ** under protection of a ULH semaphore
                                        */
    ULH_OBJECT      *rdf_ulhptr;        /* this is the ulh object used to
					** release the stream */
    PTR		    rdf_pstream_id;	/* the private memory pool stream id 
					** if LDB descriptor is allocated from 
					** private memory.  Currently there are
					** not private descriptors and the
					** cache object's streamid is copied
					** here.
					*/
    /*
    ** the unique identifier for this LDB descriptor is comprised of a
    ** DDB id and a LDB id
    */
    i4		rdf_ddbid;                  /* unique identifier for DDB */
    i4		rdf_ldbid;                  /* unique identifier for LDB */

    /*
    ** rdd_ldbdesc is the portion of this structure that is returned to the
    ** caller and contains the information the caller is interested in.
    */
    DD_1LDB_INFO    *rdd_ldbdesc;               /* ptr to ldb descriptor */

}   RDF_DULHOBJECT;

/*}
** Name: RDF_DE_ULHOBJECT - structure defining a defaults ULH object for rdf
**
** Description:
**      This structure defines the defaults object stored in ULH.  It is  
**	the header info for the defaults ULH object.
**
** History:
**	08-feb-93 (teresa)
**	    Initial creation for constraints.
[@history_template@]...
*/
typedef struct _RDF_DE_ULHOBJECT
{

    /*
    ** housekeeping info 
    */
    RDF_STATE       rdf_state;          /* state of access to object, this
                                        ** variable must be updated and looked
                                        ** under protection of a ULH semaphore
                                        */
    ULH_OBJECT      *rdf_ulhptr;        /* this is the ulh object used to
					** release the stream */
    PTR		    rdf_pstream_id;	/* the private memory pool stream id 
					** if defdescriptor is allocated from 
					** private memory.  Currently there are
					** not private descriptors and the
					** cache object's streamid is copied
					** here.
					*/
    RDD_DEFAULT	    *rdf_attdef;	/* ptr to RDD_DEFAULT structure */
    u_i2	    rdf_ldef;		/* length of default value */    
    char	    *rdf_defval;	/* contains default value as char str */

}   RDF_DE_ULHOBJECT;

/*}
** Name: RDF_ULHOBJECT - structure defining a ULH object for rdf
**
** Description:
**	This structure is what a ULH hash entry points to for relcache
**	entries and PROCEDURE qtree cache entries.
**	It points to the corresponding master RDR_INFO infoblk that
**	contains or points to most of the actual table or dbproc info.
**	Note that when a private copy of an infoblk is made, it
**	gets a private copy of the RDF_ULHOBJECT as well.
**
** History:
**      16-oct-87 (seputis)
**          initial creation
**      06-mar-89 (neil)
**          Rules: Added rdf_srule.
**      23-jun-89 (mings)   
**	    add rdf_shared_sessions to record number of session use this
**	    object
**	6-Oct-89 (teg)
**	    Merged Local/Distributed RDF.
**	20-may-92 (teresa)
**	    added rdf_starptr for sybil.
**	08-feb-93 (teresa)
**	    added defaults ptr for constraints
[@history_template@]...
*/
typedef struct _RDF_ULHOBJECT
{
    RDR_INFO        *rdf_sysinfoblk;    /* ptr to master copy */
    RDF_STATE       rdf_state;          /* state of access to object, this
                                        ** variable must be updated and looked
                                        ** under protection of a ULH semaphore
                                        */
    RDF_STATE       rdf_sintegrity;     /* state of access to the integrity
                                        ** relation */
    RDF_STATE       rdf_sprotection;    /* state of access to the protection
                                        ** relation */
    RDF_STATE       rdf_ssecalarm;      /* state of access to the secalarm
                                        ** relation */
    RDF_STATE       rdf_srule;          /* state of access to iirule */
    RDF_STATE       rdf_skeys;          /* state of access to iikeys */
    RDF_STATE       rdf_sprocedure_parameter; /* state of access to
					** iiprocedure_parameter */
    ULH_OBJECT      *rdf_ulhptr;        /* this is the ulh object used to
					** release the stream */
    RDD_QRYMOD	    *rdf_procedure;     /* ptr to procedure text, only
					** used for db procedure interface */
    i4		    rdf_shared_sessions;
					/* number of sessions share this object */
    RDF_DULHOBJECT  *rdf_starptr;	/* ptr to star cache ulh object */
    RDF_DE_ULHOBJECT *rdf_defaultptr;	/* ptr to defaults cache ulh object */
}   RDF_ULHOBJECT;

/*}
** Name: RDF_RTYPES - type used to indicate resources held in RDF
**
** Description:
**      Bit mask describing resources held in RDF
**
** History:
**      28-oct-87 (seputis)
**          initial creation
**      28-apr-89 (mings)
**          Added RDF_BAD_CHECKSUM, RDF_CLEANUP
**	06-oct-1989 (teg)
**	    Merged Star/Local and converted bitflags to HEX.
**	20-may-92 (teresa)
**	    added RDF_DULH, RDF_D_RUPDATE, RDF_BLD_LDBDESC, RDF_BAD_LDBDESC
**	    for Sybil.
**	11-Nov-93 (teresa)
**	    Added field RDF_INVAL_FIXED to fix bug 56471.
[@history_template@]...
*/
typedef i4  RDF_RTYPES;
#define RDF_RSEMAPHORE	    0x00000001L	/* set if RDF semaphore is held on
					** the ULH table element */
#define RDF_RULH	    0x00000002L	/* TRUE if ULH object has been created
					** or fixed by this thread */
#define RDF_RPRIVATE	    0x00000004L	/* TRUE if ULM temporary memory stream
					** has been created by this thread due
					** to concurrent update attempt */
#define RDF_RUPDATE	    0x00000008L	/* TRUE if master copy of ULH object
					** descriptor as been reserved for update
					** by this thread */
#define RDF_RQEU	    0x00000010L	/* set if RDF facility currently has an
					** extended system catalog opened
					** which must be closed */
#define RDF_RINTEGRITY	    0x00000020L	
#define RDF_RPROTECTION	    0x00000040L	
#define RDF_RQTUPDATE	    0x00000080L	/* set if a ULH query tree object is
					** being obtained */
#define RDF_RMULTIPLE_CALLS 0x00000100L	/* used to indicate that a querymod
					** operations will progress over
					** multiple calls to RDF so that the
					** QEF file needs to remain open */
#define RDF_REFRESH	    0x00000200L /* indicates object descriptor in the 
					** cache should not be reused. Go to 
					** cdb and ldb for all information */
#define RDF_BAD_CHECKSUM    0x00000400L /* indicates bad check sum in the info
					** block. A new info block will used &
					** the info will be retrieved from cdb*/
#define RDF_CLEANUP	    0x00000800L /* indicates the cached object is no 
					** longer valid. Invalidate the object 
					** when got a chance. */
#define RDF_RRULE	    0x00001000L /* set if iirule is still open. */
#define RDF_DULH	    0x00002000L	/* TRUE if Distributed ULH object has 
					** been created/fixed by this thread */
#define RDF_D_RUPDATE	    0x00004000L	/* TRUE if Star ULH object descriptor 
					** has been reserved for update
					** by this thread */
#define RDF_BLD_LDBDESC	    0x00008000L /* TRUE while RDF is building LDBdesc 
					** cache objects, FALSE othewise */
#define RDF_BAD_LDBDESC	    0x00010000L /* TRUE if there was an error while
					** building the LDBdesc cache object,
					** indicating the data is not complete*/
#define RDF_QTUPLE_ONLY	    0x00020000L /* indicates RDF is reading tuples from
					** catalogs on behalf of caller */
#define RDF_RPROCEDURE_PARAMETER 0x00040000L /* iiprocedure_parameter cat is open */
#define RDF_RKEYS	    0x00080000L	/* iikeys catalog is open */
#define RDF_DEULH	    0x00100000L	/* TRUE if Default ULH object is
					** been created/fixed by this thread */
#define RDF_DE_RUPDATE	    0x00200000L	/* TRUE if Defaults ULH object descriptor 
					** has been reserved for update
					** by this thread */
#define RDF_DEF_DESC	    0x00400000L /* TRUE while RDF is building defaults
					** cache objects, FALSE othewise */
#define	RDF_BAD_DEFDESC	    0x00800000L /* TRUE if there was an error while
					** building the Defaults cache object,
					** indicating the data is not complete*/
#define RDF_DSEMAPHORE	    0x01000000L	/* set if RDF semaphore is held on
					** the Default ULH table element */
#define	RDF_ATTDEF_BLD	    0x02000000L /* set while RDF is in the process of
					** updating the DMT_ATTR entry with
					** defaults cache info */
#define	RDF_INVAL_FIXED	    0x04000000L /* RDF invalidate has fixed an infoblk
					** in order to obtain info from it so it
					** can invalidate the infoblk which was
					** referenced by table id instead of by
					** address  (this is in field 
					**    global->rdf_fix_toinvalidate
					*/
#define RDF_RSECALARM       0x10000000L /* Security Alarm */

/*}
** Name: RDF_ITYPES - describes which RDF structures have been initialized
**
** Description:
**      Each mask represents a particular structure which needs to be
**      initialized prior to use.  Since most structures are not used
**      in any one RDF call, this field provides a quick way to indicate
**      this.
**
** History:
**      5-nov-87 (seputis)
**          initial creation
**	20-may-92 (teresa)
**	    defined RDF_D_IULH and RDF_D_IULM for sybil.
**	15-feb-93 (teresa)
**	    added RDF_DE_IULH, RDF_DE_IULM for constraints.
[@history_template@]...
*/
typedef i4  RDF_ITYPES;
#define	RDF_IULM    0x0001	/* TRUE - ULM control block initialized to 
				** allow ulm_palloc from a private memory stream
				*/
#define	RDF_ISCF    0x0002	/* TRUE - SCF control block initialized to 
				** allow semaphore calls on the ULH object */
#define	RDF_ISHWDMF 0x0004	/* TRUE - DMF control block initialized to 
				** allow show calls */
#define	RDF_ITBL    0x0008	/* TRUE - if tmp_rel has been used to do a 
				** lookup by name instead of table ID
				** so that it has been initialized */
#define RDF_INAME   0x0010	/* TRUE - ulh_namelen and ulh_objname have been 
				** initialized */
#define RDF_ILOCK   0x0020	/* TRUE - if update lock has been object on the 
				** table descriptor */
#define RDF_IULH    0x0040	/* TRUE - ULH control has been inited */
#define RDF_IQDATA  0x0080	/* TRUE if qefcb.output field is initialized*/
#define RDF_IQEU    0x0100	/* TRUE if qefcb.output field is initialized*/
#define	RDF_D_IULH  0x0200	/* TRUE if Distributed ULHCB is initialized */
#define	RDF_D_IULM  0x0400	/* TRUE if Distributed ULMCB is initialized */
#define RDF_DE_IULH 0x0800	/* TRUE if Defaults ULHCB is initialized */
#define	RDF_DE_IULM 0x01000	/* TRUE if Defaults ULMCB is initialized */

/*}
** Name: RDF_GLOBAL - state of RDF processing
**
** Description:
**      This structure describes the state of RDF processing and the
**      current resource usage
**
** History:
**      16-oct-87 (seputis)
**          initial creation
**	07-nov-91 (teresa)
**	    picked up ingres63p change 260973
**	    03-feb-91 (teresa)
**		pre-integrated seputis change to add rfd_timeout to this structure
**	20-may-92 (teresa)
**	    added rdf_d_ulhobject, rdf_dulmcb, rdf_dist_ulhcb for sybil.
**	22-sep-92 (teresa)
**	    add a ptr to the session control block
**	15-Oct-92 (teresa)
**	    add rd_syn_name and rd_syn_own.
**	07-jan-93 (teresa)
**	    add rdf_fulmcb to RDU_GLOBAL to hold an initialized ULM_RB from 
**	    another facility.  If the caller supplies this in RDF_RB, then RDF 
**	    will use the caller's ulm_cb for memory allocation instead of 
**	    allocating from RDF memory.
**	15-feb-93 (teresa)
**	    added support for defaults cache.
**	20-may-93 (teresa)
**	    changed rdf_de_ulhcb to rdf_def_ulhcb because it was not unique
**	    to 8 characters with rdf_de_ulhobject.  Also added rdf_deulmcb
**	11-nov-93 (teresa)
**	    added field rdf_fix_toinvalidate to fix bug 56471.
**	10-Jan-2001 (jenjo02)
**	    Added rdf_sess_id, deleted rdf_svcb.
[@history_template@]...
*/
typedef struct _RDF_GLOBAL
{
    i4		    rdf_operation;      /* caller's operation code */
    RDF_CB          *rdfcb;             /* current rdf control block */
    RDF_CCB         *rdfccb;            /* current rdf control block */
                                        /* one of rdfccb or rdfcb is NULL */
    RDF_SESS_CB	    *rdf_sess_cb;	/* ptr to session control block */
    CS_SID	    rdf_sess_id;	/* This session's id */
    RDF_RTYPES	    rdf_resources;      /* NON-ZERO if any resources are held */
    RDF_ITYPES	    rdf_init;           /* used to indicate which control blocks
                                        ** have been initialized */
    QEU_CB	    rdf_qeucb;		/* used to make calls to QEU */
    RDF_ULHOBJECT   *rdf_ulhobject;     /* Pointer to cache object being worked
					** on.  This might be a master
					** rdf_ulhobject or a private copy */
    RDF_DULHOBJECT  *rdf_d_ulhobject;   /* ptr to star cache object to be updated */
    RDF_DE_ULHOBJECT *rdf_de_ulhobject; /* ptr to defaults cache object to be updated */
    ULM_RCB         rdf_ulmcb;          /* used for ULM calls */
    ULM_RCB         rdf_dulmcb;         /* used for distributed ULM calls */
    ULM_RCB         rdf_deulmcb;         /* used for distributed ULM calls */
    ULM_RCB	    *rdf_fulmcb;	/* ULM CB initialized by calling 
					** facility -- used for those operations
					** where RDF needs to format something
					** that goes into a QSF object
					*/
    ULH_RCB	    rdf_ulhcb;          /* relcache hash or qtree hash
					** ULH request block */
    ULH_RCB	    rdf_dist_ulhcb;	/* used for ULH calls */
    ULH_RCB	    rdf_def_ulhcb;	/* defaults cache - used for ulh calls*/

    DMT_SHW_CB	    rdf_dmtshwcb;       /* used for DMF show calls */
    DMT_CHAR_ENTRY  rdf_timeout;        /* used for DMF show calls for timeout */
    DMT_TBL_ENTRY   rdf_trel;           /* temporary DMF table descriptor
                                        ** used in lookup by table name */
    i4		    rdf_namelen;	/* length of ULH object name for table*/
    char	    rdf_objname[ULH_MAXNAME]; /* ULH object name for table */
    DMR_ATTR_ENTRY  rdf_keys[3];	/* array of descriptors used to key
					** into system catalogs */
    DMR_ATTR_ENTRY  *rdf_keyptrs[3];	/* ptr to descriptor elements used to
                                        ** key into system catalogs */
    DB_SYNNAME      rd_syn_name;	/* scratchpad to build a synonym name
					** since synonym names are currently
					** 32 and DB_MAXNAME is currently 24
					*/
    DB_SYNOWN       rd_syn_own;		/* scratchpad to build a synonym owner
					** name since synonym owner names are 
					** currently 32 and DB_MAXNAME is 
					** currently 24
					*/
    RDR_INFO	    *rdf_fix_toinvalidate; /* pointer to a relation infoblk
					** that has been fixed by rdf_invalidate
					** so it could obtain info about permit,
					** integrity, tree or secondary indexes.
					** the contents of this field is only
					** meaningful when bit RDF_INVAL_FIXED
					** in rdf_resources (RDF_TYPES) is set.
					*/

    /* Titan changes start here */
    i4	    rdf_local_info;     /* shows local table schema and stats 
					** info for RDR_REFRESH request.  We 
					** should not put the info here.  But 
					** PSF requires RDF to return this info 
					** in RDR_INFO and we no other place to 
					** put. */
    QEF_RCB         rdf_qefrcb;         /* Used to call qef */
    DD_OBJ_DESC	    rdf_tobj;           /* temporary object desc */
    RDD_RCB	    rdf_ddrequests;	/* distributed info request block */	
					/* in tuple fetching operation */
    /* Titan changes end here */
}   RDF_GLOBAL;

/*}
** Name: RDF_STATISTICS - RDF Statistics
**
** Description:
**      This structure keeps running statistics for this server. There
**	statistics are reported by rdfshut.c at server shutdown, providing
**	ii_dbms_log is defined.
**
**	The intention of these statistics is to permit analysis that will
**	allow the user to better tune RDF startup parameters based on the
**	what RDF is doing.   It should also give information to permit 
**	improved development.  The following are examples of development
**	types of things that statistics should be considered for:
**	    1) we probably want to better tune the RDF memory allocation
**	       algorythm and also define more user specifiable startup
**	       parameters.  Current algorythm is tuned solely on core catalog
**	       types of things (#relations, #attributes, #indexes) and does
**	       not consider QRYMOD types of things: #permits, #integrities, etc.
**	    2) RDF cache invalidation.  It is suspected that the RDF cache is
**	       invalidiated much more than desired.  After keeping a running
**	       count of cache invalidations, it may become necessary to revisit
**	       the whole cache invalidation algorythm.  Perhaps we should chain
**	       related objects together and have a clear chain option, etc...
**
**	NOTE:  any developer is strongly encouraged to add any new statistics
**	       that they feel are useful for customer tuning or for gathering
**	       statistics that will lead to better development.
** History:
**     31-jan-91 (teresa)
**	    initial creation and editorial.
**     07-jul-92 (teresa)
**	    new statistics for sybil.
**	16-sep-92 (teresa)
**	    added rds_i8_cache_invalid.
**	04-feb-93 (teresa)
**	    added rdf_n12_default_claim, rds_r18_pptups, rds_b18_pptups,
**	    rds_c18_pptups, rds_m18_pptups, rds_z18_pptups, rds_r19_ktups,
**	    rds_b19_ktups, rds_c19_ktups, rds_m19_ktups, rds_z19_ktups
**	    for FIPS constraints.
**	1-dec-93 (robf)
**	    Added rds_?20_alarm for security alarms
**	19-mar-02 (inkdo01)
**	    Added elements for sequences.
**	6-may-02 (inkdo01)
**	    Added more for sequence protection retrievals.
*/

typedef struct _RDF_STATISTICS
{
    /* 
    ** conventions:
    **	append tup to catalog:	rds_a	(ie, requests to rdfupdate)
    **	build for cache ctrs:	rds_b	(ie, by rdfgetdsec, rdfgetinfo)
    **	already on cache ctrs:	rds_c	(ie, by rdfgetdsec, rdfgetinfo)
    **	delete tup from catalog:rds_d	(ie, requests to rdfupdate)
    **	fix (build) object ctrs:rds_f
    **	lock cache object:	rds_l	(ie, plan to update it)
    **	invalidation counters:  rds_i	(ie, requests to rdfinvalid)
    **  general counters:	rds_n
    **	request counters:	rds_r	(ie, requests to rdfgetdesc, rdfgetinfo)
    **	unfix object counters:	rds_u
    **  refresh counters	rds_x	
    **  not-found counters:	rds_z
    **
    ** Notes:
    **	The old style Hit counters are replaces with rds_c and rds_b counters,
    **  to indicate when we find the object already on the cache or when we
    **  build the object and place it on the cache.
    ** 
    **  Almost counter can overflow if the server runs long enough.  If the
    **  statistics appear meaningless, they may be victums of overflow.  In that
    **  case, bring up a server and run for about 1 day and bring it back down,
    **  and look at those statistics.
    **
    **  II_DBMS_LOG must be defined for RDF to output statistics at server
    **	shutdown.
    */

    /*******************/
    /* EXISTANCE CHECK */
    /*******************/
    RDF_SVAR	rds_r0_exists;	    /* number of times PSF checks only 
				    ** for existance */
    RDF_SVAR	rds_c0_exists;	    /* number of times that object requested
				    ** by PSF for existance only is on cache
				    */
    RDF_SVAR	rds_b0_exists;	    /* number of times that object requested
				    ** for existance only is built and put
				    ** on cache */
    RDF_SVAR	rds_z0_exists;	    /* existance check did not find object */

    /* NOTE: the number of EXIST CHECK objects that do not exist can be 
    **	     calcualted as:
    **		rds_r0_core_exists - (rds_c0_core_exists + rds_b0_core_exists)
    */

    /*************************/
    /* GET CORE CATALOG INFO */
    /*************************/
    RDF_SVAR	rds_r1_core;	    /* number of times RDF is asked to get
				    ** info from core catalogs for object*/
    RDF_SVAR	rds_c1_core;	    /* number of times requested core cat
				    ** info is already on cache */
    RDF_SVAR	rds_b1_core;	    /* number of times that requested core
				    ** catalog info must be partially or fully
				    ** built -- ie, relation info may already 
				    ** be on cache from previous existance check
				    ** call, but attribute info is not on cache.
				    */
    RDF_SVAR	rds_z1_core;	    /* object does not exist */

    /****************************/
    /* non-core catalog objects */
    /****************************/
    RDF_SVAR	rds_r2_permit;	    /* req. for permission info */
    RDF_SVAR	rds_c2_permit;	    /* permission info is already on cache */
    RDF_SVAR	rds_b2_permit;	    /* build permission info & put on cache */
    RDF_SVAR	rds_a2_permit;	    /* append permission info to catalogs */
    RDF_SVAR	rds_d2_permit;	    /* delete permission info from catalogs */

    RDF_SVAR	rds_r3_integ;	    /* req. for integrity info */
    RDF_SVAR	rds_c3_integ;	    /* integrity info is already on cache */
    RDF_SVAR	rds_b3_integ;	    /* build integrity info & put on cache */
    RDF_SVAR	rds_a3_integ;	    /* append integrity info to catalogs */
    RDF_SVAR	rds_d3_integ;	    /* delete integrity info from catalogs */

    RDF_SVAR	rds_r4_view;	    /* req. for view info */
    RDF_SVAR	rds_c4_view;	    /* view info is already on cache */
    RDF_SVAR	rds_b4_view;	    /* build view info & put on cache */
    RDF_SVAR	rds_a4_view;	    /* append view info to catalogs */
    RDF_SVAR	rds_d4_view;	    /* delete view info from catalogs */

    RDF_SVAR	rds_r5_proc;	    /* req. for procedure info */
    RDF_SVAR	rds_c5_proc;	    /* procedure info is already on cache */
    RDF_SVAR	rds_b5_proc;	    /* build procedure info & put on cache */
    RDF_SVAR	rds_a5_proc;	    /* append procedure info to catalogs */
    RDF_SVAR	rds_d5_proc;	    /* delete procedure info from catalogs */
    
    RDF_SVAR	rds_r6_rule;	    /* req. for rule info */
    RDF_SVAR	rds_c6_rule;	    /* rule info is already on cache */
    RDF_SVAR	rds_b6_rule;	    /* build rule info & put on cache */
    RDF_SVAR	rds_a6_rule;	    /* append rule info to catalogs */
    RDF_SVAR	rds_d6_rule;	    /* delete rule info from catalogs */
    
    RDF_SVAR	rds_r7_event;	    /* req. for event info */
    RDF_SVAR	rds_b7_event;	    /* build event info (this is never cached)*/
    RDF_SVAR	rds_a7_event;	    /* append event info to catalogs */
    RDF_SVAR	rds_d7_event;	    /* delete event info from catalogs */

    /* all to all */
    RDF_SVAR	rds_a8_alltoall;    /* add all to all permission */
    RDF_SVAR	rds_d8_alltoall;    /* remove all to all permission */
    
    /* read to all */
    RDF_SVAR	rds_a9_rtoall;	    /* add all to all permission */
    RDF_SVAR	rds_d9_rtoall;	    /* remove all to all permission */

    /* dbdb catalog operations */
    RDF_SVAR	rds_a10_dbdb;	    /* append tuple to iidbdb catalog */
    RDF_SVAR	rds_d10_dbdb;	    /* detele tuple from iidbdb catalog */
    RDF_SVAR	rds_o10_dbdb;	    /* replace/purge tuple in iidbdb catalog */

    /* statistics info */
    RDF_SVAR	rds_r11_stat;	    /* request statistics */
    RDF_SVAR	rds_c11_stat;	    /* statistic info is already on cache */
    RDF_SVAR	rds_b11_stat;	    /* build statistic info and put on cache */

    /* synonyms info */
    RDF_SVAR	rds_r16_syn;	    /* number of times RDF looks for synonyms */
    RDF_SVAR	rds_b16_syn;	    /* number of times RDF finds a synonym in
				    ** catalog and places info on cache.*/
    RDF_SVAR	rds_a16_syn;	    /* add tuple to iisynonym catalog */
    RDF_SVAR	rds_d16_syn;	    /* delete tuple from iisynonym catalog */
    RDF_SVAR	rds_z16_syn;	    /* number times synonym not defined */

    /* comments info */
    RDF_SVAR	rds_a17_comment;    /* add tuple to iicomment catalog */
    RDF_SVAR	rds_d17_comment;    /* delete tuple from iicomment catalog
				    ** currently not used. */


    RDF_SVAR	rds_r20_alarm;	    /* req. for alarm info */
    RDF_SVAR	rds_c20_alarm;	    /* alarm info is already on cache */
    RDF_SVAR	rds_b20_alarm;	    /* build alarm info & put on cache */
    RDF_SVAR	rds_a20_alarm;	    /* append alarm info to catalogs */
    RDF_SVAR	rds_d20_alarm;	    /* delete alarm info from catalogs */

    /* sequences info */
    RDF_SVAR	rds_r21_seqs;	    /* req. for sequence info */
    RDF_SVAR	rds_b21_seqs;	    /* build sequence info (this is never cached)*/
    RDF_SVAR	rds_a21_seqs;	    /* append sequence info to catalogs */
    RDF_SVAR	rds_d21_seqs;	    /* delete sequence info from catalogs */

    /*****************************/
    /* query tuple read requests */
    /*****************************/
    RDF_SVAR	rds_r12_itups;	    /* request integrity tuples */
    RDF_SVAR	rds_b12_itups;	    /* build integrity tuples on cache */
    RDF_SVAR	rds_c12_itups;	    /* integrity tuples already on cache */
    RDF_SVAR	rds_m12_itups;	    /* integrity tuples require multiple reads*/
    RDF_SVAR	rds_z12_itups;	    /* # times integrity tuples not found */

    RDF_SVAR	rds_r13_etups;	    /* request event permit tuples */
    RDF_SVAR	rds_b13_etups;	    /* event permits obtained on single rdf 
				    ** call (these are never cached) */
    RDF_SVAR	rds_m13_etups;	    /* event permit tuples require multiple
				    ** RDF calls */
    RDF_SVAR	rds_z13_etups;	    /* # times event permit tuples not found */

    RDF_SVAR	rds_r14_ptups;	    /* request permit tuples */
    RDF_SVAR	rds_b14_ptups;	    /* build permit tuples on cache */
    RDF_SVAR	rds_c14_ptups;	    /* permit tuples already on cache */
    RDF_SVAR	rds_m14_ptups;	    /* permit tuples require multiple reads */
    RDF_SVAR	rds_z14_ptups;	    /* # times permit tuples not found */

    RDF_SVAR	rds_r15_rtups;	    /* request rule tuples */
    RDF_SVAR	rds_b15_rtups;	    /* build rule tuples on cache */
    RDF_SVAR	rds_c15_rtups;	    /* rule tuples already on cache */
    RDF_SVAR	rds_m15_rtups;	    /* rule tuples require multiple reads */
    RDF_SVAR	rds_z15_rtups;	    /* # times rule tuples not found */

   RDF_SVAR    rds_r18_pptups;      /* request proc param tuples */
   RDF_SVAR    rds_b18_pptups;      /* build proc param tuples on cache */
   RDF_SVAR    rds_c18_pptups;      /* proc param tuples already on cache */
   RDF_SVAR    rds_m18_pptups;      /* proc param tuples require multiple
				    ** reads */
   RDF_SVAR    rds_z18_pptups;      /* # times proc param tuples not found */

   RDF_SVAR    rds_r19_ktups;	    /* request iikey tuples */
   RDF_SVAR    rds_b19_ktups;	    /* build iikey tuples on cache */
   RDF_SVAR    rds_c19_ktups;	    /* iikey tuples already on cache */
   RDF_SVAR    rds_m19_ktups;	    /* iikey tuples require multiple
				    ** reads */
   RDF_SVAR    rds_z19_ktups;	    /* # times iikey tuples not found */

   RDF_SVAR	rds_r20_atups;	    /* request alarm tuples */
   RDF_SVAR	rds_b20_atups;	    /* build alarm tuples on cache */
   RDF_SVAR	rds_c20_atups;	    /* alarm tuples already on cache */
   RDF_SVAR	rds_m20_atups;	    /* alarm tuples require multiple reads */
   RDF_SVAR	rds_z20_atups;	    /* # times alarm tuples not found */

   RDF_SVAR	rds_r22_stups;	    /* request sequence permit tuples */
   RDF_SVAR	rds_b22_stups;	    /* sequence permits obtained on single rdf 
				    ** call (these are never cached) */
   RDF_SVAR	rds_m22_stups;	    /* sequence permit tuples require multiple
				    ** RDF calls */
   RDF_SVAR	rds_z22_stups;	    /* # times sequence permit tuples not found */

   /*************************/
    /* Invalidation counters */
    /*************************/
    RDF_SVAR	rds_i1_obj_invalid; /* running count of the number of times a
				    ** single object in the rdf cache is 
				    ** invalidated. */
    RDF_SVAR	rds_i2_p_invalid;   /* running count of # times a procedure
				    ** object is invalidated from rdf cache */
    RDF_SVAR	rds_i3_flushed;	    /* running count of times the object to be
				    ** invalidated is already flushed off of the
				    ** cache -- indicator of extreme concurrancy
				    ** conditions */
    RDF_SVAR	rds_i4_class_invalid; /* invalidate all protect or integrity or
				    ** rule trees in this class */
    RDF_SVAR	rds_i5_tree_invalid;/*invalidate a protect/integrity/rule tree*/
    RDF_SVAR	rds_i6_qtcache_invalid; /* running count of the number of times
				    ** the rdf QT cache is invalidated. */
    RDF_SVAR	rds_i7_cache_invalid; /* running count of the number of times
				    ** the whole rdf cache is invalidated. */
    RDF_SVAR	rds_i8_cache_invalid; /* running count of the number of times
				    ** the ldbdesc cache is invalidated. */

    /****************/
    /* general info */
    /****************/
    i4		rds_n0_tables;	    /* running count of #tables on cache.  This
				    ** will include multiple instances of the
				    ** same table.  IE, table is cleared from
				    ** cache and then cache entry is rebuilt will
				    ** count twice.  This is just used to calc
				    ** avg # columns */
    float	rds_n1_avg_col;	    /* average number of columns per table. This
				    ** is calucated as:  
		**  ( (rds_n1_avg_col * rds_n0_tables) + num_attrs_this_tbl ) /
		**		    (rds_n0_tables + 1)
		*/
    float	rds_n2_avg_idx;	    /* average number of secondary indexes per 
    				    ** table.  This is calucated as:  
		**  ( (rds_n2_avg_idx * rds_n0_tables) + num_idxs_this_tbl ) /
		**		    (rds_n0_tables + 1)
		*/
    RDF_SVAR	rds_n7_exceptions;  /* running count of # exceptions RDF has
				    ** encountered */
    RDF_SVAR	rds_n8_QTmem_claim; /* running counter of number of times RDF
				    ** runs out of memory on the cache and
				    ** reclaims it from the Query Tree cache. */
    RDF_SVAR	rds_n9_relmem_claim;/* running counter of number of times RDF
				    ** runs out of memory on the cache and
				    ** reclaims it from the Relation cache */
    RDF_SVAR	rds_n10_nomem;	    /* running counter of number of times RDF
				    ** runs out of memory on the cache and
				    ** is unable to reclaim it from either the
				    ** query tree or the relation cache.  If this
				    ** count is nonzero, user MUST tune rdf
				    ** cache to increase memory size.
				    */
    RDF_SVAR	rds_n11_LDBmem_claim; /* running counter for number of times RDF
				    ** runs out of memory on the cache and
                                    ** reclaims it from the LDBdesc cache */
    RDF_SVAR	rdf_n12_default_claim; /* running counter for number of times 
				    ** RDF runs out of memory on the cache and
                                    ** reclaims it from the defaults cache */
    

    /****************************/
    /* cache refresh infomation */
    /****************************/
    RDF_SVAR	rds_x1_relation;    /* number of times a relation infoblk is
				    ** refreshed on behalf of caller's request*/
    RDF_SVAR	rds_x2_qtree;	    /* number of times a qtree cache object is
				    ** refreshed on behalf of caller's request*/


    /*******************************/
    /* CACHE FIX/UNFIX INFORMATION */
    /*******************************/
    /* this section will give a feeling for concurent activity and also for
    ** errors.  We do not expect rds_u8_invalid or rds_u9_fail to be
    ** hit in a healthy environment.  The number of Private Vs shared
    ** objects built and released will give a feel for how much concurent 
    ** activity is going on.  (I have broken out statistis on private/shared 
    ** protect, integrity, procedure and rule objects because they may be 
    ** useful in the future, when RDF has more tuning available.)
    ** There is a chance that these counters may overflow if the server is up
    ** for a long time because alot of objects are fixed and unfixed in the
    ** normal operation of the DBMS server.  However, for trouble shooting, this
    ** may provide useful information . */
    i4	rds_u0_protect;	    /* # times shared protect obj is unfixed */
    i4	rds_u1_integrity;   /* # times shared integrity obj is unfixed*/
    i4	rds_u2_procedure;   /* # times shared procedure obj is unfixed*/
    i4	rds_u3_rule;	    /* # times shared rule obj is unfixed */
    i4	rds_u4p_protect;    /* # times private protect obj is unfixed */
    i4	rds_u5p_integrity;  /* # times private integrity obj is unfixed*/
    i4	rds_u6p_procedure;  /* # times private procedure obj is unfixed*/
    i4 	rds_u7p_rule;	    /* # times private rule obj is unfixed */
    i4	rds_u8_invalid;	    /* # times invalid qrymod obj unfix req. */
    i4	rds_u9_fail;	    /* # times RDR_INFO unfix request failed */
    i4	rds_u10_private;    /* # times private RDR_INFO is unfixed */
    i4 	rds_u11_shared;	    /* # times shared RDR_INFO is unfixed */
    i4	rds_f0_relation;    /* # times relation object is fixed */
    i4	rds_f1_integrity;   /* # times integrity obj is fixed*/
    i4	rds_f2_procedure;   /* # times procedure obj is fixed*/
    i4	rds_f3_protect;	    /* # times protect obj is fixed */
    i4	rds_f4_rule;	    /* # times rule obj is fixed */
    i4	rds_l0_private;     /* this is a running counter of each time RDF makes
			    ** a private RDR_INFO object because another thread 
			    ** has an update lock on the object that this thread
			    ** wants to update.  This is an indicator of RDF
			    ** concurency.	    */
    i4	rds_l1_private;	    /* overflow ctr for rds_f1_private */
    i4	rds_l2_supdate;     /* this is a running counter of each time that a 
			    ** thread updates a shared RDR_INFO object. */
    i4	rds_l3_supdate;	    /* overflow ctr for rds_f2_supdate */

} RDF_STATISTICS;

/*}
** Name: RDI_SVCB - Server control block for RDF
**
** Description:
**      This structure defines the server control block for RDF.
**	There will be one of these for each database server.
**	This structure holds items that are shared between sessions, such
**	as trace vectors, hash table info, memory info (poolid, amount of
**	memory left, etc).
**
**	This used to just hold trace vector information and was a global
**	variable.  However, INGRES coding standards (and some platforms that
**	we port to) do not allow global variables, so this is being tied to
**	the RDF_CCB via a pointer in RDF_CCB.
**
** History:
**     15-apr-86 (ac)
**          written
**     02-mar-87 (puree)
**	    modify for global pointer to server control block.
**     09-aug-88 (ms)
**          modified for Titan.  
**     06-Oct-89 (teg)
**	    merge titan/local (RDF_NB, RDF_NAVO, RDF_NVP added to local, but
**	    adder did NOT create a history record)
**     13-dec-89 (teg)
**	    changed description section to accurately reflect what RDI_SVCB
**	    is and how it is used.
**     05-feb-91 (teresa)
**	    added rdvstat to structure for statistics reporting.
**	23-jan-92 (teresa)
**	    major changes for SYBIL.
**	04-feb-93 (teresa)
**	    add new default cache info
**	30-mar-93 (teresa)
**	    remove RDV_DONT_USE_IISYNONYM from rdv_special_condt
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with
**	    DM0M_OBJECT.
**	17-feb-94 (teresa)
**	    Added new counter for invalid checksums: rdv_bad_cksum_ctr.
**	10-Jan-2001 (jenjo02)
**	    Added rsv_rdscb_offset.
**	22-feb-2001 (somsa01)
**	    Changed rdv_rdscb_offset to be of type SIZE_TYPE.
*/


typedef struct _RDI_SVCB
{
	struct _RDI_SVCB	*q_next;	/* Forward pointer. */
	struct _RDI_SVCB	*q_prev;	/* Backward pointer. */
	SIZE_TYPE	length;		/* Length of the control block. */
	i2		type;		/* Type of the control block. */
#define	    RDI_CB_TYPE	4
	i2		s_reserved;
	PTR		l_reserved;
	PTR    		owner;
	i4		ascii_id;
#define	    RDI_SCB_ID	    CV_C_CONST_MACRO('S', 'V', 'C', 'B')

	i4		rdv_special_condt;  /* this is a bit mask of special
					    ** conditions (usually specified at
					    ** session startup) */
#define	RDV_NO_SPECIAL_CONDIT	0L	    /* no special conditions are set */

	RDF_STATISTICS	rdvstat;	  /* Running RDF statistics for this
					  ** server */

	/* memory management */
	i4		rdv_state;	    /* server initialization state */
#define RDV_1_RELCACHE_INIT	0x0001L	    /* relation description cache is
					    ** allocated */
#define RDV_2_LDBCACHE_INIT	0x0002L	    /* ldb description cache is 
					    ** allocated */ 
#define RDV_3_RELHASH_INIT	0x0004L	    /* relation hash table is created */
#define RDV_4_QTREEHASH_INIT	0x0008L	    /* qtree hash table is created */	
#define RDV_5_DEFAULTS_INIT	0x0010L	    /* DEFAULT hash table is created */
	SIZE_TYPE	rdv_pool_size;	    /* Size of rdf cache pool in bytes*/
	i4		rdv_cnt0_rdesc;	    /* # objects allowed on rel cache */
	i4		rdv_cnt1_qtree;	    /* # objs allowed on QTREE cache */
	i4		rdv_cnt2_dist;	    /* # objs allowed on LDBdesc cache*/
	i4		rdv_cnt3_default;   /* # objs allowed on default cache*/
	SIZE_TYPE	rdv_in0_memory;		/* input tuning parameter */
	i4		rdv_in1_max_tables;	/* input tuning parameter */
	i4		rdv_in2_table_columns;	/* input tuning parameter */
	i4		rdv_in3_table_synonyms;	/* input tuning parameter */
	i4		rdv_in4_cache_ddbs;	/* input tuning parameter */
	i4		rdv_in5_average_ldbs;	/* input tuning parameter */
	PTR		rdv_poolid;	    /* memory pool id */
	SIZE_TYPE	rdv_memleft;	    /* remaining memory in pool */
	PTR		rdv_rdesc_hashid; /* ulh hash table id of the relation descriptors */
	PTR		rdv_qtree_hashid;   /* ulh hash table id of the qtree */
	PTR		rdv_def_hashid;	 /* ulh hash table id of default cache*/
	PTR		rdv_dist_hashid;    /* ulh hash table for distributed
					    ** access cache */
	CS_SEMAPHORE	rdv_global_sem;	    /* rdf global semaphore */

        /* Titan changes start here */
        DB_DISTRIB      rdv_distrib;      /* this valus is a bit mask and 
					  ** initialized at server startup time.
					  ** DB_1_LOCAL_SVR is used to indicate
					  ** the server supports local sessions
					  ** and DB_2_DISTRIB_SVR is used to 
					  ** indicate the server supports 
					  ** distributed sessions. */

	i4		rdv_max_sessions; /* maximum sessions allowed in a server */
	i4		rdv_cnt;	  /* num of ddb cached */
	PTR		rdv_cls_streamid; /* stream id for cluster info space */
	i4		rdv_num_nodes;     /* number of nodes in the cluster. cluster info not
					  ** available if zero */
	RDD_CLUSTER_INFO *rdv_cluster_p;  /* point to cluster info. Null if not applicable */
	DD_NODELIST	 *rdv_node_p;	  /* point to cpu cost list */
	DD_NETLIST	 *rdv_net_p;	  /* point to net cost list */
	i4		 rdv_cost_set;    /* indicate whether cpu/net cost are set */
#define RDV_0_COST_NOT_SET	   (i4)0
#define RDV_1_COST_SET		   (i4)1 
        /* Titan changes end here */

	i4		rdv_bad_cksum_ctr; /* Ctr for num of bad cksums*/
#define	RDV_MAX_BAD_CKSUMS	5
	/* server wide trace points and their mapping to session trace points */
	ULT_VECTOR_MACRO(RDF_NB, RDF_NVAO) rdf_trace;	
	    /* 
	    **  #defines to map server trace points to session trace points go
	    **  here 
	    */
	SIZE_TYPE	rdv_rdscb_offset;	/* Offset from SCF's SCB
						** to RDF_SESS_CB
						*/
} RDI_SVCB;

GLOBALREF	RDI_SVCB	*Rdi_svcb;


/*}
** Name: RDF_CALLER - Self-Identifier for a calling routine.
**
** Description:
**      This type is used to allow one routine to identify itself when calling
**	another routine that must know who was calling it.
**
** History:
**      17-sep-92 (teresa)
**          initial creation for Sybil trace points.
*/
/* these codes define the calling routine for the master trace routine */
typedef	    i4	    RDF_CALLER;
#define	    RDFGETDESC	1
#define	    RDFGETINFO	2
#define	    RDFUNFIX	3
#define	    RDFINVALID	4

/*
** Name: RDF_CALL_INFO - RDF call operation info table
**
** Description:
**	This table has one entry per RDF-CALL operation code (indexed
**	by same).  It gives some useful info about the RDF operation
**	such as what kind of request control block is passed in, etc.
**
**	The actual data table is statically defined in rdfdata.c.
**
** History:
**	25-Mar-2004 (schka24)
**	    Globalize to eliminate magic switch stmts in rdfutil.
*/

typedef struct
{
	i2	cb_type;	/* RDF_CB_TYPE or RDF_CCB_TYPE */
	bool	need_sess_cb;	/* TRUE if session ID needed */
	bool	caller_ulm_cb;	/* TRUE if caller can pass ulm rcb */
} RDF_CALL_INFO;


/*}
** Name: RDF_SYVAR - internal synonyms information
**
** Description:
**      This type is used for reporting information on internal RDF use of
**	synonyms, which is transparent to the rest of the server.
**
** History:
**      09-apr-90 (teg)
**          initial creation for SYNONYMS
*/
typedef i4 RDF_SYVAR;

#define		RDU_1SYN	0x00000001  /*requested name was a synonym */
#define		RDU_2CACHED	0x00000002  /*base table that synonym resolves*/

/*}
**
** Description:
**      Function Prototype definitions
**
** History:
**      16-jul-92 (teresa)
**          initial creation
**	12-aug-92 (teresa)
**	    rdf_release is no longer static and local only to rdfcall, so 
**	    put prototype definition here.
**	16-sep-92 (teresa)
**	    arguments to rdt_clear_cache() changed to handle additional tracepts
**	02-mar-93 (teresa)
**	    added prototype for rdd_defsearch()
**	6-Jul-2006 (kschendel)
**	    Remove a couple things local to rdfinvalid.c
*/

/*
** Prototypes for RDEFAULT. ---------------------------------------------------
*/
FUNC_EXTERN DB_STATUS rdd_defsearch(	
		RDF_GLOBAL         *global,
		DMT_ATT_ENTRY	   *col_ptr,
		RDD_DEFAULT	   **def_ptr);

/*
** Prototypes for RDFBEGIN. ---------------------------------------------------
*/

FUNC_EXTERN DB_STATUS rdf_bgn_session(
    RDF_GLOBAL	    *global,
    RDF_CCB	    *rdf_ccb);

/*
** Prototypes for RDFCALL ------------------------------------------------------
**/

/* rdf_release	- release any resources that RDF has on table */
FUNC_EXTERN VOID rdf_release( 
    RDF_GLOBAL *global,
    DB_STATUS  *status);

/*
** Prototypes for RDFDDB ------------------------------------------------------
**/

/* rdf_open_cdbasso -  force open privilege cdb association. */
FUNC_EXTERN DB_STATUS rdf_open_cdbasso(RDF_GLOBAL *global);

/* rdf_clusterinfo - cluster information request. */
FUNC_EXTERN DB_STATUS rdf_clusterinfo(RDF_GLOBAL *global);

/* rdf_usuper - retrieve server wide user status */
FUNC_EXTERN DB_STATUS rdf_usuper(RDF_GLOBAL *global);

/* rdf_netnode_cost - retrieve net cost info from iidbdb */
FUNC_EXTERN DB_STATUS rdf_netnode_cost(RDF_GLOBAL *global);

/* rdf_iidbdb - call QEF to retrieve control information for accessing database 
** IIDBDB. */
FUNC_EXTERN DB_STATUS rdf_iidbdb(RDF_GLOBAL *global);

/* rdf_usrstat	- get user's DB access status. */
FUNC_EXTERN DB_STATUS rdf_usrstat(RDF_GLOBAL *global);

/* rdf_ldbplus	-  call QEF to retrieve LDB's dba name and capabilities. */
FUNC_EXTERN DB_STATUS rdf_ldbplus(RDF_GLOBAL *global);

/* rdf_userlocal - get the user name from ldb. */
FUNC_EXTERN DB_STATUS rdf_userlocal(	RDF_GLOBAL         *global,
					DD_OWN_NAME	   user_name);

/* rdf_localtab	- retrieve local table information. */
FUNC_EXTERN DB_STATUS rdf_localtab(	RDF_GLOBAL         *global,
					char		   *sys_owner);

/* rdf_sysowner	- retrieve ldb system catalog owner name. */
FUNC_EXTERN DB_STATUS rdf_sysowner(	RDF_GLOBAL         *global,
					char		   *sys_table,
					char		   *sys_owner);

/* rdf_gldbinfo	- Retrieve LDB description. */
FUNC_EXTERN DB_STATUS rdf_gldbinfo(RDF_GLOBAL *global);


/* rdf_ldb_tabinfo - Request description of a table. */
FUNC_EXTERN DB_STATUS rdf_tabinfo(RDF_GLOBAL *global);

/* rdf_ddbinfo - call QEF to get access status of DDB, the CDB information. */
FUNC_EXTERN DB_STATUS rdf_ddbinfo(RDF_GLOBAL *global);

/* rdf_ddb    - Retrieve iidbdb, ddb/cdb, and ldb information. */
FUNC_EXTERN DB_STATUS rdf_ddb(  RDF_GLOBAL  *global,
				RDF_CB	    *rdfcb);
/*
** Prototypes for RDFEND -------------------------------------------------------
**/

/* rdf_end_session - End a RDF session */
FUNC_EXTERN DB_STATUS rdf_end_session(  RDF_GLOBAL  *global,
				        RDF_CCB	    *rdf_ccb);
/*
** Prototypes for module RDFGETDESC -------------------------------------------
*/

/* rdu_gsemaphore - get a semaphore on the ULH object */
FUNC_EXTERN DB_STATUS rdu_gsemaphore( RDF_GLOBAL *global);

/* rdu_rsemaphore - release semaphore on ULH object */
FUNC_EXTERN DB_STATUS rdu_rsemaphore( RDF_GLOBAL  *global);

/* rdu_shwdmfcall - call DMF show call routine for table */
FUNC_EXTERN DB_STATUS rdu_shwdmfcall(	RDF_GLOBAL         *global,
					i4		    dmf_mask,
					RDF_SYVAR	   *flagmask,
					DB_TAB_ID	   *table_id,
					bool		   *refreshed);

/* rdf_gdesc - Request description of a table. */
FUNC_EXTERN DB_STATUS rdf_gdesc(  RDF_GLOBAL	*global,
				  RDF_CB	*rdfcb);
/*
** Prototypes for RDFGETINFO ---------------------------------------------------
*/

/* rdd_streequery - send inquiry query to QEF for iidd_ddb_tree catalog. */
FUNC_EXTERN DB_STATUS rdd_streequery(RDF_GLOBAL *global);

/* rdd_gtreetuple - retrieve tree tuples from iidd_ddb_tree. */
FUNC_EXTERN DB_STATUS rdd_gtreetuple(	RDF_GLOBAL         *global,
					bool		   *eof_found);

/* rdu_qopen	- routine to open a QEF file */
FUNC_EXTERN DB_STATUS rdu_qopen(  
	    RDF_GLOBAL	    *global,
	    RDF_TABLE       table,
	    char	    *data,
	    i4		    extcattupsize,
	    PTR		    datap,
	    i4		    datasize);

/* rdu_qget - get the next set of tuples from extended catalog */
FUNC_EXTERN DB_STATUS rdu_qget(   
	    RDF_GLOBAL	*global,
	    bool	*eof_found);

/* rdu_qclose - close the extended system catalog */
FUNC_EXTERN DB_STATUS rdu_qclose(RDF_GLOBAL *global);

/* rdr_tuple - read data from IITREE catalog tuples 
**  NOTE: defined in rdftree because of variable RDF_TSTATE 
**	    FUNC_EXTERN DB_STATUS rdr_tuple(  
**	    	    RDF_GLOBAL	*global,
**	    	    i4		length,
**	    	    PTR		buffer,
**	    	    RDF_TSTATE  *tstate)
*/

/* rdu_qtclass - get class name for query tree hash table */
FUNC_EXTERN VOID rdu_qtclass(
		RDF_GLOBAL  *global,
		u_i4	    id,
		i4	    tree_type,
		char	    *name,
		i4	    *len);

/* rdu_qtalias - get alias name for query tree hash table object */
FUNC_EXTERN VOID rdu_qtalias(	
		RDF_GLOBAL  *global,
		i4	    tree_type,
		char	    *name,
		i4	    *len);

/* rdu_qmclose - close query mod table */
FUNC_EXTERN DB_STATUS rdu_qmclose(
		RDF_GLOBAL      *global,
		RDF_STATE       *usr_state,
		RDF_STATE	*sys_state);

/* rdu_txtcopy - copy iiqrytext data from read buffer to cache */
FUNC_EXTERN VOID  rdu_txtcopy(	u_char  *from,
				u_i2	len,
				u_char  *to);

/* rdf_colcompare - retrieve column comparison stats for a table. */
FUNC_EXTERN DB_STATUS rdu_colcompare(
	    RDF_GLOBAL	    *global);

/* rdf_getinfo - Request qrymod and statistics information of a table. */
FUNC_EXTERN DB_STATUS rdf_getinfo(  RDF_GLOBAL	*global,
				    RDF_CB	*rdfcb);
/*
** Prototypes for module RDFGETOBJ --------------------------------------------
*/

/* rdd_caps   - setup ldb capabilities. */
FUNC_EXTERN DB_STATUS rdd_caps(   RDF_GLOBAL	    *global,
				  DD_CAPS	    *cap_p,
				  RDC_DBCAPS	    *caps);

/* rdd_gldbdesc	- retrieve LDB description from CDB. */
FUNC_EXTERN DB_STATUS rdd_gldbdesc( RDF_GLOBAL         *global,
				    DD_1LDB_INFO	*ldb_p);

/* rdd_ldbsearch - search ldb description by ldb id. */
FUNC_EXTERN DB_STATUS rdd_ldbsearch(	RDF_GLOBAL         *global,
					DD_1LDB_INFO	   **desc_p,
				    	i4		   ddb_id,
					i4		   ldb_id);

/* rdd_cvtype	- convert data type name to data type id. */
FUNC_EXTERN DB_STATUS rdd_cvtype(   RDF_GLOBAL	*global,
				    char	*string,
				    DB_DT_ID	*tp,
				    ADF_CB	*adfcb);

/* rdd_cvstorage - convert storage type to storage id. */
FUNC_EXTERN DB_STATUS rdd_cvstorage(	RDD_16CHAR	storage,
					i4		*tp);

/* rdd_cvobjtype - convert object/table type. */
FUNC_EXTERN VOID rdd_cvobjtype(	char		*object_type,
				DD_OBJ_TYPE	*tp);

/* rdd_tab_init - initialize DMT_TBL_ENTRY */
FUNC_EXTERN VOID rdd_tab_init(DMT_TBL_ENTRY  *tbl_p);

/* rdd_rtrim - trim the blanks on the right and null terminate. */
FUNC_EXTERN VOID rdd_rtrim( PTR		tab_name,
			    PTR		tab_owner,
			    RDD_OBJ_ID	*id_p);

/* rdd_query - send a query to qef. */
FUNC_EXTERN DB_STATUS rdd_query(RDF_GLOBAL *global);

/* rdd_fetch  - call qef to fetch a tuple. */
FUNC_EXTERN DB_STATUS rdd_fetch( RDF_GLOBAL *global);

/* rdd_flush - call qef to flush. */
FUNC_EXTERN DB_STATUS rdd_flush( RDF_GLOBAL *global);

/* rdd_vdada - send a query with db data values to qef. */
FUNC_EXTERN DB_STATUS rdd_vdata( RDF_GLOBAL *global);

/* rdd_begin - send a notice to qef for a series of operations. */
FUNC_EXTERN DB_STATUS rdd_begin( RDF_GLOBAL *global);

/* rdd_username - retrieve local username and compare with local object name. */
FUNC_EXTERN DB_STATUS rdd_username(RDF_GLOBAL *global);

/* rdd_tableupd - update num of rows and pages in iidd_tables. */
FUNC_EXTERN DB_STATUS rdd_tableupd(	RDF_GLOBAL         *global,
					RDD_OBJ_ID         *id_p,
					i4		   numrows,
					i4		   numpages,
					i4		   overpages);

/* rdd_stampupd - update time stamp in iidd_ddb_tableinfo. */
FUNC_EXTERN DB_STATUS rdd_stampupd(	RDF_GLOBAL         *global,
					u_i4		   stamp1,
					u_i4		   stamp2);

/* rdd_baseviewchk - check views on an object */
FUNC_EXTERN DB_STATUS rdd_baseviewchk( RDF_GLOBAL *global);

/* rdd_schemachk - schema consistency check. */
FUNC_EXTERN DB_STATUS rdd_schemachk(	RDF_GLOBAL         *global,
					RDD_OBJ_ID	   *obj_id_p,
					DD_ATT_NAME	   *map_p,
					ADF_CB		   *adfcb);

/* rdd_alterdate - retrieve table alternation date LDB. */
FUNC_EXTERN DB_STATUS rdd_alterdate(	RDF_GLOBAL         *global,
					RDD_OBJ_ID	   *obj_id_p,
					DD_ATT_NAME	   *mapped_names,
					ADF_CB              *adfcb);

/* rdd_getcolno - retrieve attribute count of an objects from CDB. */
FUNC_EXTERN DB_STATUS rdd_getcolno(	RDF_GLOBAL         *global,
					RDD_OBJ_ID	   *id_p);

/* rdd_localstats - check whether statistics is available. */
FUNC_EXTERN DB_STATUS rdd_localstats(	RDF_GLOBAL *global );

/* rdd_idxcount	- retrieve index count. */
FUNC_EXTERN DB_STATUS rdd_idxcount(	RDF_GLOBAL         *global,
					RDD_OBJ_ID	   *id_p);

/* rdd_ldbtab - retrieve ldb table info from CDB. */
FUNC_EXTERN DB_STATUS rdd_ldbtab(   RDF_GLOBAL     *global,
				    RDD_OBJ_ID	   *id_p);

/* rdd_objinfo	- retrieve object information from CDB. */
FUNC_EXTERN DB_STATUS rdd_objinfo(  RDF_GLOBAL	*global,
				    RDD_OBJ_ID	*id_p);

/* rdd_relinfo	- retrieve object information from CDB. */
FUNC_EXTERN DB_STATUS rdd_relinfo(  RDF_GLOBAL	*global,
				    RDD_OBJ_ID	*id_p);

/* rdd_gobjinfo - inquire object info from CDB. */
FUNC_EXTERN DB_STATUS rdd_gobjinfo( RDF_GLOBAL *global);

/* rdd_gmapatt - build mapped column descriptor for object. */
FUNC_EXTERN DB_STATUS rdd_gmapattr( RDF_GLOBAL *global);

/* rdd_gattr - build attributes descriptor for object. */
FUNC_EXTERN DB_STATUS rdd_gattr( RDF_GLOBAL  *global);

/* rdd_gindex	- build index descriptor for an object. */
FUNC_EXTERN DB_STATUS rdd_gindex(RDF_GLOBAL *global);

/* rdd_gstats - retrieve object information from CDB. */
FUNC_EXTERN DB_STATUS rdd_gstats(RDF_GLOBAL *global);

/* rdd_ghistogram - retrieve histogram information from iidd_histogram in CDB */
FUNC_EXTERN DB_STATUS rdd_ghistogram( RDF_GLOBAL *global);

/* rdd_commit - commit to release all the locks gotten on the CDB catalogs */
FUNC_EXTERN DB_STATUS rdd_commit( RDF_GLOBAL *global);

/* rdd_gobjbase - read object base from iidd_ddb_object_base in CDB */
FUNC_EXTERN DB_STATUS rdd_gobjbase( RDF_GLOBAL *global);

/* rdd_getobjinfo - retrieve distributed object information. */
FUNC_EXTERN DB_STATUS rdd_getobjinfo( RDF_GLOBAL *global);

/*
** Prototypes for RDFINIT -----------------------------------------------------
*/

FUNC_EXTERN DB_STATUS rdf_initialize( RDF_GLOBAL      *global,
				      RDF_CCB         *rdf_ccb);
/*
** prototypes for RDFINVALID --------------------------------------------------
*/

/* rdf_invalidate - Invalidate a table information block currently in
**		    the cache.
*/
FUNC_EXTERN DB_STATUS rdf_invalidate(	
		RDF_GLOBAL	*global,
		RDF_CB		*rdfcb);

/* rdr_killtree - kill a tree class */
FUNC_EXTERN DB_STATUS rdr_killtree(
	RDF_GLOBAL	*global,
        i4		tree_type,
        u_i4		id);


/* invalidate_relcache - invalidate a relation cache entry & any 
**			 associated objects
*/
FUNC_EXTERN DB_STATUS invalidate_relcache(
	    RDF_GLOBAL	*global,
	    RDF_CB	*rdfcb,
	    RDR_INFO	*infoblk,
	    DB_TAB_ID	*obj_id);
/*
** prototypes for RDFPART ------------------------------------------------------
*/
/* rdp_build_partdef -- build partitioned table definition */
FUNC_EXTERN DB_STATUS rdp_build_partdef(
	RDF_GLOBAL	*global);

/* rdp_copy_partdef -- Copy a partition definition for caller */
FUNC_EXTERN DB_STATUS rdp_copy_partdef(
	RDF_GLOBAL	*global,
	RDF_CB		*rdfcb);

/* rdp_update_partdef -- store a partitioned table definition into the
** system catalogs
*/
FUNC_EXTERN DB_STATUS rdp_update_partdef(
	RDF_GLOBAL	*global,
	RDF_CB		*rdfcb);

/* rdp_finish_partdef -- finalize and check a partitioned table definition
** created manually, eg by the parser.
*/
FUNC_EXTERN DB_STATUS rdp_finish_partdef(
	RDF_GLOBAL	*global,
	RDF_CB		*rdfcb);

/* rdp_part_compat -- Check partition / storage structure compatibility */
FUNC_EXTERN DB_STATUS rdp_part_compat(
	RDF_GLOBAL	*global,
	RDF_CB		*rdfcb);

/*
** prototypes for RDFSHUT ------------------------------------------------------
*/

/* rdf_shutdown - Shut down RDF for the server. */
FUNC_EXTERN DB_STATUS rdf_shutdown(
		RDF_GLOBAL	*global,
		RDF_CCB		*rdf_ccb);

/* print RDF statistics */
FUNC_EXTERN VOID rdf_report(RDF_STATISTICS *stat);

/*
** prototypes for RDFSTART ----------------------------------------------------
*/
/* rdf_startup - Start up RDF for the server. */
FUNC_EXTERN DB_STATUS rdf_startup(RDF_GLOBAL	*global,
				  RDF_CCB	*rdf_ccb);
/*
** prototypes for RDFTERM ------------------------------------------------------
*/

/* rdf_terminate - Terminate RDF for a facility. */
FUNC_EXTERN DB_STATUS rdf_terminate( 
		RDF_GLOBAL      *global,
                RDF_CCB         *rdf_ccb);

/*
** prototypes for RDFTRACE -----------------------------------------------------
*/

/* rdf_trace - Call RDF trace operation. 
-- moved to RDF.H so SCF can see, but also left here for documentation
FUNC_EXTERN DB_STATUS rdf_trace(DB_DEBUG_CB *debug_cb);
*/

/* rdt_clear_cache - Clear RDF cache */
FUNC_EXTERN DB_STATUS rdt_clear_cache(i4     trace_pt);

/*
** prototypes for RDFUNFIX -----------------------------------------------------
*/
FUNC_EXTERN DB_STATUS rdf_unfix(  RDF_GLOBAL  *global,
				  RDF_CB      *rdf_cb);
/*
** prototypes for RDFUPDATE ----------------------------------------------------
*/

/* rdf_update - append/delete qrymod information of a table */
FUNC_EXTERN DB_STATUS rdf_update( RDF_GLOBAL      *global,
				  RDF_CB          *rdfcb);

/* rdu_treenode_to_tuple - query tree to tuples conversion routine. */
FUNC_EXTERN DB_STATUS rdu_treenode_to_tuple(RDF_GLOBAL  *global,
					    PTR		root,
					    DB_TAB_ID   *tabid,
					    i4		mode,
					    RDD_QDATA   *data);

/* rdu_qrytext_to_tuple - query text to tuples conversion routine. */
FUNC_EXTERN DB_STATUS rdu_qrytext_to_tuple( RDF_GLOBAL  *global,
					    i4		mode,
					    RDD_QDATA   *data);

/*
** prototypes for RDFQTPACK ----------------------------------------------------
*/

FUNC_EXTERN DB_STATUS rdf_qtext_to_tuple(   RDF_GLOBAL      *global,
					    RDF_CB          *rdfcb);

FUNC_EXTERN DB_STATUS rdf_qtree_to_tuple(   RDF_GLOBAL      *global,
					    RDF_CB          *rdfcb);

/*
** prototypes for RDFUTIL ------------------------------------------------------
*/

/* rdu_atthsh_dump - RDF hash table of attributes information dump. */
#ifdef xDEV_TEST
FUNC_EXTERN VOID rdu_atthsh_dump(   RDR_INFO	*info);
#endif

/* rdu_attr_dump - RDF attributes informatin dump. */
#ifdef xDEV_TEST
FUNC_EXTERN VOID rdu_attr_dump(	    RDR_INFO	*info);
#endif

/* rdu_com_checksum - execute checksum on infoblock */
FUNC_EXTERN i4 rdu_com_checksum(RDR_INFO	*infoblk);

/* Name: rdu_cstream - create a new memory stream */
FUNC_EXTERN DB_STATUS rdu_cstream( RDF_GLOBAL	*global);

/* rdu_currtime	- get current time */
FUNC_EXTERN VOID rdu_currtime( 	DB_TAB_TIMESTAMP    *time_stamp);

/* rdu_dstream	- delete a memory stream */
FUNC_EXTERN DB_STATUS rdu_dstream(  RDF_GLOBAL	*global,
				    PTR		stream_id,
				    RDF_STATE	*state);

/* rdu_r_dulh	- unfix a distributed ulh object */
FUNC_EXTERN DB_STATUS rdu_r_dulh(   RDF_GLOBAL	*global);

/* rdf_exception - handle unknown exception for RDF */
FUNC_EXTERN STATUS rdf_exception(   EX_ARGS	*args);

/* rdu_ferror - RDF error handling routine */
FUNC_EXTERN VOID rdu_ferror(	RDF_GLOBAL	*global,
				DB_STATUS	status,
				DB_ERROR	*ferror,
				RDF_ERROR	interror,
				RDF_ERROR       suppress);

/* rdu_his_dump - RDF histogram information dump. */
#ifdef xDEV_TEST
FUNC_EXTERN VOID rdu_hist_dump(	    RDR_INFO	*info,
				    i4		att_start,
				    i4		att_end);
#endif

/* rdu_i_dulh	- invalidate a ulh distributed object */
FUNC_EXTERN DB_STATUS rdu_i_dulh(   RDF_GLOBAL	*global);

/* rdu_indx_dump - RDF index informatin dump. */
#ifdef xDEV_TEST
FUNC_EXTERN VOID rdu_indx_dump(	    RDR_INFO	*info);
#endif

/* rdu_ierror	- this routine will report the RDF error */
FUNC_EXTERN VOID rduIerrorFcn(	RDF_GLOBAL	   *global,
				DB_STATUS	   status,
				RDF_ERROR          local_error,
				PTR		   FileName,
				i4		   LineNumber);
#define rdu_ierror(global,status,local_error) \
    rduIerrorFcn(global,status,local_error,__FILE__,__LINE__)

/* rdu_info_dump - RDF table information block dump. */
#ifdef xDEV_TEST
FUNC_EXTERN VOID rdu_info_dump(	    RDR_INFO	*info);
#endif

/* rdu_init_info - Initilaize table information block. */
FUNC_EXTERN VOID rdu_init_info( RDF_GLOBAL	*global,
				RDR_INFO	*rdf_info_blk,
				PTR             streamid);

/* rdu_ituple_dump - RDF integrity tuples dump. */
#ifdef xDEV_TEST
FUNC_EXTERN VOID rdu_ituple_dump(   RDR_INFO	*info);
#endif

/* rdu_iulh	- invalidate a ulh object */
FUNC_EXTERN DB_STATUS rdu_iulh(	    RDF_GLOBAL	*global);

/* rdu_keys_dump - RDF primary key informatin dump. */
#ifdef xDEV_TEST
FUNC_EXTERN VOID rdu_keys_dump(	    RDR_INFO	*info);
#endif

/* rdu_malloc - Allocate memory */
FUNC_EXTERN DB_STATUS rdu_malloc(   RDF_GLOBAL	*global,
				    i4		psize,
				    PTR         *pptr);

/* rdu_mrecover - garbage-collect RDF memory */
FUNC_EXTERN DB_STATUS rdu_mrecover(RDF_GLOBAL *global,
				   ULM_RCB   *ulm_rcb,
				   DB_STATUS orig_status,
				   RDF_ERROR errcode);

/* rdd_setsem	- get a semaphore on ldb or ddb descriptor. */
FUNC_EXTERN DB_STATUS rdd_setsem(RDF_GLOBAL *global,
				 i4    sem_type);

/* rdd_resetsem	- release semaphore. */
FUNC_EXTERN DB_STATUS rdd_resetsem(RDF_GLOBAL *global,
				   i4    sem_type);

/* rdu_master_infodump - Figure if there are any requests to dump RDF table 
**			 information then do so
*/
#ifdef xDEV_TEST
FUNC_EXTERN VOID rdu_master_infodump(RDR_INFO	*info,
				     RDF_GLOBAL	*global,
				     RDF_CALLER caller,
				     RDF_SVAR 	request);
#endif

/* rdu_private	- create private sys_infoblk */
FUNC_EXTERN DB_STATUS rdu_private( RDF_GLOBAL	*global,
				   RDR_INFO	**infoblkpp);

/* rdu_ptuple_dump - RDF protection tuples dump. */
#ifdef xDEV_TEST
FUNC_EXTERN VOID rdu_ptuple_dump(   RDR_INFO	*info);
#endif

/* rdu_rel_dump - RDF relation informatin dump. */
#ifdef xDEV_TEST
FUNC_EXTERN VOID rdu_rel_dump(	    RDR_INFO	*info);
#endif

/* rdu_rtuple_dump - RDF rules tuple dump. */
#ifdef xDEV_TEST
FUNC_EXTERN VOID rdu_rtuple_dump(   RDR_INFO	*info);
#endif

/* rdu_rulh	- unfix a ulh object */
FUNC_EXTERN DB_STATUS rdu_rulh(	    RDF_GLOBAL	*global);

/* rdu_sccerror - output an RDF error message via SCF */
FUNC_EXTERN DB_STATUS rdu_sccerror(DB_STATUS	    status,
			      DB_SQLSTATE   *sqlstate,
			      RDF_ERROR     local_error,
			      char          *msg_buffer,
			      i4            msg_length);


/* rdu_setname	- create a name for the ULH hash object */
FUNC_EXTERN VOID rdu_setname(	RDF_GLOBAL         *global,
				DB_TAB_ID          *table_id);

/* rdu_view_dump - RDF view information dump. */
#ifdef xDEV_TEST
FUNC_EXTERN VOID rdu_view_dump(	    RDR_INFO	*info);
#endif

/* rdu_warn	- this routine will report an RDF warning. */
FUNC_EXTERN VOID rduWarnFcn( RDF_GLOBAL	  *global,
			     RDF_ERROR     warn,
			     PTR	   FileName,
			     i4	   	   LineNumber);
#define rdu_warn(global,warn) \
    rduWarnFcn(global,warn,__FILE__,__LINE__)

/* rdu_xlock - mark object as locked or create new private object */
FUNC_EXTERN DB_STATUS rdu_xlock(    RDF_GLOBAL	*global,
				    RDR_INFO	**infoblkpp);

    /* rdu_xor - compute checksum for a structure */
    FUNC_EXTERN VOID rdu_xor(	    i4	*chksum,
				    char	*iptr,
				    i4	isize);

/* rdu_drop_attachments	- unfix attachments from a ulh object */
FUNC_EXTERN STATUS rdu_drop_attachments (RDF_GLOBAL *global,
					 RDR_INFO *infoblk,
					 i4 sessions_reqd);

/* the following routines are not used now but may be useful later, they are
** lifted from the TITAN codeline (6.4 star) 
*/

    /* rdu_help - print trace help information */
#ifdef xDEV_TEST
    FUNC_EXTERN VOID rdu_help(	    i4	where_to_print);
#endif 

    /* rdu_object_trace - print relation descriptor info */
#ifdef xDEV_TEST
    FUNC_EXTERN VOID rdu_object_trace(  RDF_CCB	*scb_p,
					RDR_INFO	*rdf_info_blk,
					i4	where_to_print,
					i4	tr_type);
#endif 

    /* rdu_scftrace - call scc_trace to display trace information */
#ifdef xDEV_TEST
    FUNC_EXTERN VOID rdu_scftrace(  i4	*where_p,
				    u_i4	length,
				    char	*msg_buf);
#endif 
