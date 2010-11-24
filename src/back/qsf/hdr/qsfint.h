/*
**Copyright (c) 2004 Ingres Corporation
**
*/

/**
** Name: QSFINT.H - All definitions needed only inside QSF.
**
** Description:
**      This file contains all of the typedefs and defines that are
**      needed internally to QSF.  This file depends on the following
**	files already being #included:
**		dbms.h
**		qsf.h (for QSO_OBID, at least)
**		cs.h	(for the CS_SEMAPHOREs)
**
**      This file defines the following typedefs:
**          QSO_OBJ_HDR	   - Object header.
**          QSO_MASTER_HDR - Master object header.
**          QSO_HASH_TAB   - Object hash table.
**          QSR_CB	   - QSF server control block.
**
** History: $Log-for RCS$
**      14-mar-86 (thurston)
**          Initial creation.
**	23-apr-86 (thurston)
**	    Major overhaul.
**      02-mar-87 (thurston)
**	    Added the standard structure header, and the .qsr_streamid member
**	    to the QSR_CB typedef.  Also removed the .qsr_inited member.
**	13-may-87 (thurston)
**	    Upped the QSF_MEMPOOL_SIZE to be 1/4 Mb, instead of 64 Kb.
**	19-may-87 (thurston)
**	    Added group of #defines for QSF trace points.
**	29-may-87 (thurston)
**	    Changed the block size of QTREE's and QP's from 4K to 1K.
**	04-jun-87 (thurston)
**	    Added the constants QSF_PBASE_BYTES and QSF_PBYTES_PER_SES used
**	    in calculating the size of QSF's memory pool based on max number
**	    of active sessions.
**	05-jul-87 (thurston)
**	    Added the .qsr_tracing member to the QSR_CB typedef.
**	28-nov-87 (thurston)
**	    Changed the SCF_SEMAPHORE to a CS_SEMAPHORE in the QSR_CB typedef,
**	    to enable easier usage of CS{p,v}_semaphore() routines instead of
**	    going through SCF.
**	25-mar-88 (thurston)
**	    Added QSF info request trace point QSF_510_POOLMAP to generate a
**	    ULM Memory Pool Map in the log file for QSF's memory pool.
**	31-aug-1989 (sandyh)
**	    Change QSF_PBYTES_PER_SES from 40K to 80K to lessen QSF memory
**	    errors. This is a stop gap measure until more work is done to
**	    figure out the reason QSF is not freeing long term objs correctly.
**	08-Jan-1992 (fred)
**	    Added associated external object list to object header.  This done
**	    in support of peripheral object (aka large object) support.
**	20-mar-92 (rog)
**	    Added QSF info request trace point QSF_506_LRU_FLUSH to flush
**	    all LRU-able objects from the QSF memory pool.
**	28-aug-92 (rog)
**	    Prototype QSF.
**	16-oct-92 (rog)
**	    Modify structures to support the new pieces added by the
**	    FIPS constraints project.
**	19-oct-92 (rog)
**	    Added the QSO_MASTER_HDR structure to support multiple query plans
**	    per semantically-equivalent query.
**	03-nov-1992 (rog)
**	    Removed the QSS_MEM structure.
**	16-nov-1992 (rog)
**	    Added the QSF_507_SIZE_STATS trace point to display max sizes
**	    allocated/requested.
**	30-nov-1992 (rog)
**	    Added the QSF_DECAY_FACTOR #define to set the default decay factor
**	    for LRU objects.  Moved QSF_TRSTRUCT out of this file and into qsf.h
**	15-jan-1993 (rog)
**	    Added the QSF_MAX_ORPHANS #define to set maximum number of times
**	    we iterate through the orphan detection code.
**	12-mar-1993 (rog)
**	    Added a prototype for the qso_destroyall() function.
**	23-dec-1993 (rog)
**	    Added a prototype for the qso_sandd() function.
**	28-mar-1994 (rog)
**	    Added qso_size to the QSO_OBJ_HDR structure.
**	27-apr-1994 (rog)
**	    Added prototypes for qsf_attach_inst() and qsf_detach_inst().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Jan-2001 (jenjo02)
**	    Added GET_QSF_CB macro to replaces qsf_session() function.
**	    Added *QEF_CB parm to qsf_clrmem() prototype.
**	26-Sep-2003 (jenjo02)
**	    Added QSF_508_LRU_DISPLAY trace point, reworked mutexing
**	    scheme to eliminate qso_psem bottleneck.
**      12-Apr-2004 (stial01)
**          Define length as SIZE_TYPE.
**      7-oct-2004 (thaju02)
**          Define qsr_memtot and qsr_memleft as SIZE_TYPE for memory pool > 2Gig.
**      18-Feb-2005 (hanal04) Bug 112376 INGSRV2837
**          Added prototype for qsf_clrsesobj().
**	21-Feb-2005 (schka24)
**	    Get rid of separate alias object, put FE alias right in the
**	    master.  This is necessary to eliminate lookup vs delete races,
**	    besides being simpler.  Fix up comments a little.
**      10-Jul-2006 (hanal04) Bug 116358
**          Added QSF_511_ULM_PRINT to call ulm_print_pool for QSF.
**	9-feb-2007 (kibro01) b113972
**	    Add parameter to qso_destroy, qso_rmvobj and qso_unlock
**	    so that in qso_rmvobj we know if we have locked qsr_psem
**	11-feb-2008 (smeke01) b113972
**	    Backed out above cross.
**      17-feb-09 (wanfr01) b121543
**	    Added parameter to qsf_attach_inst and qsf_detach_inst
**	29-Oct-2010 (jonj) SIR 120874
**	    qso_rmvindex err_code param changed to *DB_ERROR
*/


/*
**      This group of #defines are for the server as a whole.
*/
#define		    QSF_MEMPOOL_SIZE    262144L	/* # bytes in QSF's mem pool  */
						/* if we must use an internal */

#define		    QSF_PBASE_BYTES     100000L	/* # bytes needed for the 1st */
						/* session.  Used in calculat-*/
						/* QSF's memory pool size.    */

#define		    QSF_PBYTES_PER_SES   80000L	/* # bytes for each additional*/
						/* session.  Used in calculat-*/
						/* QSF's memory pool size.    */

#define		    QSF_DEFAULT_DECAY	     2  /* Amount to multiply # of    */
						/* objects before calculating */
						/* amount to decay usage      */
						/* factor.		      */
#define		    QSF_MAX_DECAY	   100  /* Maximum amount that the    */
						/* LRU decay factor can be    */
						/* set to.		      */
#define		    QSF_MAX_ORPHANS	  1000  /* Maximum number of orphans  */
						/* to look for.		      */


/*
**      These are the defined session specific trace points for QSF:
*/

#define	QSS_MINTRACE			1   /* Smallest session trace point.  */

#define	    QSF_001_ENDSES_OBJQ         1   /* Dump the object queue on       */
					    /* session termination.           */

#define	    QSF_002_CALLS               2   /* Announce entry and exit of all */
					    /* QSF calls except QSR_STARTUP,  */
					    /* QSR_SHUTDOWN, & QSS_BGN_SESSION*/

#define	    QSF_003_CHK_SCB_LIST        3   /* Do consistency checks on       */
					    /* session CB list.               */

#define	    QSF_004_AFT_CREATE_OBJQ     4   /* Dump the object queue after    */
					    /* each object has been created.  */

#define	    QSF_005_CHK_OBJQ            5   /* Do consistency checks on       */
					    /* QSF's object queue.            */

#define	    QSF_006_OBJQ_LONG_FMT       6   /* When set, all object queue     */
					    /* dumps will be in long format.  */

#define	    QSF_007_AFT_CREATE_NAMED    7   /* Dump the object queue ONLY     */
					    /* after a named obj is created.  */

#define	    QSF_008_CLR_OBJ             8   /* Show info when trying to clear */
					    /* an object from the object queue*/

#define	    QSF_009_POOL_MAP_ON_QDUMP   9   /* Generate a mem pool map when-  */
					    /* ever the obj queue is dumped.  */

#define	QSS_MAXTRACE			9   /* Largest session trace point.   */


/*
**      These are the defined server wide trace points for QSF:
*/

#define	QSR_MINTRACE		     1001   /* Smallest server trace point.   */

#define	    QSF_1001_LOG_INFO_REQS   1001   /* Send all info requests to the  */
					    /* log file as well as the user   */

#define	    QSF_1002_HASH_BKT	     1002   /* For each call to qso_hash()    */
					    /* for named objects show the obj */
					    /* ID and hash bucket             */

#define	    QSF_1003_HASH_BKT_ALL    1003   /* For ALL calls to qso_hash()    */
					    /* show the obj ID and hash bucket*/

#define	QSR_MAXTRACE		     1003   /* Largest server trace point.    */


/*
**      These are quasi-trace points; they don't really get set, but are instead
**	special requests for information to be displayed to the user through a
**	ule_format() call.
*/

#define	QSQ_MINTRACE		      501   /* Smallest info trace point.     */

#define	    QSF_501_MEMLEFT           501   /* Display memory left stats.     */

#define	    QSF_502_NUM_OBJS          502   /* Display #'s of objs in QSF.    */

#define	    QSF_503_NUM_BKTS          503   /* Display hash bucket stats.     */

#define	    QSF_504_NUM_SESS          504   /* Display # session stats.       */

#define	    QSF_505_LRU               505   /* Display LRU stats.             */

#define	    QSF_506_LRU_FLUSH         506   /* Flush the LRU queue.	      */

#define	    QSF_507_SIZE_STATS        507   /* Display allocation size stats. */
#define	    QSF_508_LRU_DISPLAY       508   /* Display LRU queues */

#define	    QSF_510_POOLMAP           510   /* Generate mem-pool map.         */

#define     QSF_511_ULM_PRINT         511   /* ulm_print_pool the QSF pool    */

#define	QSQ_MAXTRACE		      511   /* Largest info trace point.      */


/*
**  Defines of other constants.
*/

/* Macro to extract session's *QSF_CB directly from SCF's SCB */
#define	GET_QSF_CB(sid) \
	*(QSF_CB**)((char *)sid + Qsr_scb->qsr_qsscb_offset)


/*}
** Name: QSO_OBJ_HDR - Object header.
**
** Description:
**      This structure defines a QSF object header.  One of these structures
**	will be created and placed at the front of the object's memory stream
**      for each object that gets created in QSF's memory via a call to
**      QSO_CREATE. Its purpose is to provide a mechanism to chain all objects
**      that exists for the server as a whole, and to act as a repository for
**      information about its corresponding object.  This includes the object
**      name, if it has one, the object's lock state, the "root" for the object,
**      the streamid, and several other pieces of accounting information about
**      the object. 
**
**	Object headers come in two flavors:  named and un-named.  Un-named
**	objects are purely private to a specific session, and are
**	manipulated solely by object handle.  (Which QSF knows internally
**	is the object header address.)
**
**	Named objects are a bit more complicated.  Named objects are always
**	QP objects (although QSF doesn't depend on that, I hope).  A
**	named object will point to an object master which is used to
**	control access to the object(s) of the given name.  Rarely, more
**	than one object can have the same name -- this happens with set-
**	input DB procedures, where we allow multiple QP's (possibly
**	differing based on input temp table size) for the same DBP.
**
** History:
**	14-mar-86 (thurston)
**          Initial creation.
**	23-apr-86 (thurston)
**	    Major overhaul.	
**	26-jan-88 (thurston)
**	    Added .qso_lrnext and .qso_lrprev.
**	08-Jan-1992 (fred)
**	    Added .qso_aeo_next & .qso_aeo_prev.  These form the linked list
**	    of Associated External Objects (QSF_AEO's -- in qsf.h) which allow
**	    QSF to inform external object managers that its base object is about
**	    to be destroyed.
**	19-oct-1992 (rog)
**	    Modified old members and added new ones to support the integrity
**	    constraints (FIPS) project.  These structures will not be part of
**	    any hash buckets, so the standard header pointers will be re-used
**	    to hold pointers to next/previous linked list of session specific
**	    objects for unshared objects.  They will be unused by shared
**	    objects.  Added qso_monext, qso_moprev, qso_mobject, qso_psem_owner,
**	    qso_real_usage, and qso_decaying_usage.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**	26-Sep-2003 (jenjo02)
**	    Deleted qso_psem, qso_psem_owner, added qso_excl_owner.
**	    Replaced qso_aeos structure with n/p pointers to QSF_AEO
**	    structures.
**	21-Feb-2005 (schka24)
**	    Deleted unused external-object stuff.
**	17-Feb-2009 (wanfr01)
**          Bug 121543
**	    Add QSOBJ_DBP_TYPE
**	26-May-2010 (kschendel) b123814
**	    Add qso-snamed-list, list of uncommitted objects owned by a
**	    session.  The list is used for deleting rolled-back objects.
*/
typedef struct _QSO_OBJ_HDR
{
/********************   START OF STANDARD HEADER   ********************/

    struct _QSO_OBJ_HDR	*qso_obnext;    /* Ptr to next object in linked list
					** of objects owned by each session.
					** Unused by shared objects except while
					** being created.
                                        */
    struct _QSO_OBJ_HDR	*qso_obprev;    /* Ptr to previous object in session's
					** object linked list.
					** Unused by shared objects except while
					** being created.
                                        */
    SIZE_TYPE       qso_length;         /* Length of this structure: */
    i2              qso_type;		/* Type of structure.  For now, 1320 */
#define                 QSOBJ_TYPE      1320	/* for no particular reason */
#define                 QSOBJ_DBP_TYPE  1	/* Database procedure */	
    i2              qso_s_reserved;	/* Possibly used by mem mgmt. */
    PTR             qso_l_reserved;	/*    ''     ''  ''  ''  ''   */
    PTR             qso_owner;          /* Owner for internal mem mgmt. */
    i4              qso_ascii_id;       /* So as to see in a dump. */
					/*
					** Note that this is really a char[4]
					** but it is easer to deal with as an i4
					*/
#define                 QSOBJ_ASCII_ID  CV_C_CONST_MACRO('Q', 'S', 'O', 'H')

/*********************   END OF STANDARD HEADER   *********************/

    struct _QSO_OBJ_HDR	*qso_lrnext;    /* Ptr to next object in the list of all
					** objects that are LRU'able.  This list
					** will be a doubly linked list that
					** will contain only those objects that
					** "MAY" be able to be LRU'ed out of
					** memory.  This list will not be used
					** for other objects, including all
					** unnamed objects.
					*/
    struct _QSO_OBJ_HDR	*qso_lrprev;    /* Ptr to prev object in the list of all
					** objects that are LRU'able.
                                        */
    i4		    qso_status;		/* status bits for this object */
#define	    QSO_DEFERRED_DESTROY  ((i4)1)  /* This bit is set if the object  */
					    /* is to be destroyed when the use*/
					    /* count (.qso_lk_cur) reaches 0. */
#define	    QSO_LRU_DESTROY	0x0002	/* Set under QSR sem mutex protection
					** when we decide to destroy an
					** object, to prevent multi-delete race
					*/
    struct _QSO_OBJ_HDR	*qso_monext;    /* Ptr to next object in the list of all
					** semantically identical query plans.
					** Each Master object will have a linked
					** list of these objects.
					** Note that this only happens for
					** set-input DBP's, in all other
					** cases a master should only have one
					** QP object, and this should be NULL
					*/
    struct _QSO_OBJ_HDR	*qso_moprev;    /* Ptr to prev object in the list of all
					** semantically identical query plans.
                                        */
    struct _QSO_MASTER_HDR *qso_mobject;/* Master object that owns this obj. */
    struct _QSO_OBJ_HDR *qso_snamed_list; /* Ptr to next object in list of
					** named QSF objects that belong to
					** this session, and haven't been
					** committed yet.
					*/
    QSO_OBID        qso_obid;           /* Id for the object.
					*/
    PTR             qso_streamid;       /* The ULM memory stream id for
                                        ** the object's memory.
                                        */
    PTR             qso_root;           /* Root address of the object.
					** QSF doesn't really care, but a null-
					** root says "this object is still
					** being created".  The root is more
					** likely to be useful to the outside
					** world (eg parse tree root, etc).
                                        */
    QSF_CB	    *qso_session;	/* Session that created this object.
					** Nulled out for persistent (shareable,
					** named) objects after the creating
					** session exits.
					** It will be valid for at least until
					** qso_root is set (ie if qso_root is
					** null, qso_session will be set).
					** qso_session will also be set for
					** an object on an snamed_list.
					*/
    i4              qso_lk_state;       /* Lock state the object is in.  This
                                        ** will always be one of the following
                                        ** symbolically defined constants (these
                                        ** are defined in QSF.H): 
					**	QSO_FREE   - Not locked.
					**	QSO_SHLOCK - Locked for sharing.
					**	QSO_EXLOCK - Exclusively locked.
					*/
    i4              qso_lk_cur;         /* Current # of locks on the object.
					** Only relevant when shared locked,
					** pay no attention if exclusive locked
                                        */
    i4		    qso_lk_id;		/* Lock id, only used when locked
					** exclusively.  QSF hands out a
                                        ** different lock id each time any
                                        ** object within the server is locked
                                        ** exclusively (via QSO_CREATE or
                                        ** QSO_LOCK), and requires that lock id
                                        ** to be supplied to QSO_UNLOCK before
                                        ** the object can be unlocked. 
                                        */
    i4		    qso_waiters;	/* Current # of sessions waiting to
					** get at this object.
					*/
    QSF_CB	   *qso_excl_owner;	/* Session holding QSO_EXLOCK */
    i4	    	    qso_real_usage;	/* Number of times this object has been
					** used.  Only relevant for named objs.
					*/
    i4	    	    qso_decaying_usage;	/* Usage counter used by the LRU
					** algorithm to keep more recently used
					** objects longer.
					*/
    i4	    	    qso_size;		/* Size of the object (total size of
					** all of the QSO_PALLOC pieces added
					** together.
					*/
}   QSO_OBJ_HDR;


/*}
** Name: QSO_MASTER_HDR - Master object header.
**
** Description:
**	Unnamed objects belong strictly to one session, and that session
**	tracks the objects by their handles.  Unnamed objects just sort
**	of float around loose inside QSF, and require no special tracking.
**	They do not participate in the QSF hash table, they aren't LRU'ed
**	out, there's no master, etc.
**
**	A named object has a master object header (a.k.a an "index" object)
**	for keeping track of the object.  The master holds the object and
**	object alias names (BE and FE names), and is used to hold the
**	object's position on a hash chain.  The object is always a QP
**	object, we don't share other kinds of objects (although QSF doesn't
**	really care about the object type much).  If the shared thing is a
**	set-input DB procedure, there can be multiple QP's connected to the
**	same master object.  (A set-input DBP QP is sensitive to the
**	size of the passed-in temp table, and QEF is able to choose the
**	most size-appropriate QP from multiple QP's belonging to the DBP.)
**	All the QP objects are created with the same object ID, and
**	one master is used to connect them all together.
**
**	Note that it's possible to have a non-shareable named object,
**	although it is rare.  (Named v5-style QUEL queries are not shared,
**	and some forms of repeated QUEL queries can't be shared.)
**
**	Mutex ordering:  master mutex > hash mutex > QSR mutex.
**
** History:
**	19-oct-1992 (rog)
**	    Created.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**	26-Sep-2003 (jenjo02)
**	    Added qsmo_busy, qsmo_waiters, qsmo_sem, qsmo_cond, qsmo_bucket,
**	    deleted qsmo_crp_sem, qsmo_crp_sem_owner,
**	    replaced qsmo_aeos structure with n/p pointers to
**	    QSF_AEO structures.
**	21-Feb-2005 (schka24)
**	    Move FE alias objid here to eliminate separate alias index
**	    object.  Eliminate alias pointer, master waiters count.
*/
typedef struct _QSO_MASTER_HDR
{
/********************   START OF STANDARD HEADER   ********************/

    struct _QSO_MASTER_HDR *qsmo_obnext; /* Ptr to next object in hash bucket.
					 ** Named objects will be truly hashed,
					 ** while unnamed objects will all be
					 ** grouped into one special bucket
					 ** which is separate from the hash
					 ** table.
                                         */
    struct _QSO_MASTER_HDR *qsmo_obprev; /* Ptr to prev object in hash bucket.*/
    SIZE_TYPE       qsmo_length;        /* Length of this structure: */
    i2              qsmo_type;		/* Type of structure.  For now, 1320 */
#define                 QSMOBJ_TYPE	2571	/* for no particular reason */
    i2              qsmo_s_reserved;	/* Possibly used by mem mgmt. */
    PTR             qsmo_l_reserved;	/*    ''     ''  ''  ''  ''   */
    PTR             qsmo_owner;         /* Owner for internal mem mgmt. */
    i4              qsmo_ascii_id;      /* So as to see in a dump. */
					/*
					** Note that this is really a char[4]
					** but it is easer to deal with as an i4
					*/
#define                 QSMOBJ_ASCII_ID  CV_C_CONST_MACRO('Q', 'S', 'M', 'H')

/*********************   END OF STANDARD HEADER   *********************/

    QSO_OBID        qsmo_obid;          /* QP (backend) object ID for this
					** named object.  (the qsf_obj_id)
					*/
    QSO_OBID	    qsmo_aliasid;	/* FE (alias) object ID for this named
					** object, if any.  All shareable
					** masters have some sort of alias.
					*/
    PTR             qsmo_streamid;      /* The ULM memory stream id for
                                        ** this object's memory.  Used only
					** to allocate this header.
                                        */
    QSF_CB	   *qsmo_session;	/* Session that created this object. */
    CS_SEMAPHORE    qsmo_sem;		/* Mutex to protect Master */
    CS_CONDITION    qsmo_cond;		/* Condition to wait on while
					** QP is being built
					*/
    QSO_OBJ_HDR	   *qsmo_obj_1st;	/* First QP owned by this master.  */
    QSO_OBJ_HDR	   *qsmo_obj_last;	/* Last QP owned by this master.
					** Not used by aliases.
					*/
    struct _QSO_HASH_BKT    *qsmo_bucket; /* Master's hash bucket */
}   QSO_MASTER_HDR;


/*}
** Name: QSO_HASH_BKT - QSF object hash bucket.
**
** Description:
**      This structure defines a hash bucket used for hashing QSF objects.
**	An array of these hash buckets will be allocated at server startup time
**	to serve as the hash table.  The number of buckets in the table will be
**	dynamically chosen based on the amount of memory being used for the QSF
**	memory pool.
**
**      All named objects will be hashed into the server wide hash table.
**	Only named objects are hashed;  unnamed objects are purely private
**	to their owning session, and are manipulated by handle alone.
**	Named objects are typically shareable, and are the only objects
**	that can persist beyond end-of-query.
**
**	The hash bucket mutex is used to prevent create and delete
**	races and link foul-ups.
**	Mutex ordering:  master mutex > hash mutex > QSR mutex.
**
** History:
**     22-jan-88 (thurston)
**          Initial creation.
**     19-oct-1992 (rog)
**          Deleted the qsb_oblast member because we don't need it if we
**	    always add to the beginning of the chain.
**	26-Sep-2003 (jenjo02)
**	    Added qsb_init, qsb_sem.
*/

typedef struct _QSO_HASH_BKT
{
    QSO_MASTER_HDR *qsb_ob1st;         /* Ptr to 1st object in bucket.	     */
    i4              qsb_nobjs;          /* # of objects currently in bucket. */
    i4              qsb_mxobjs;         /* Max # of objects ever in bucket.  */
    i4		    qsb_init;		/* qsb_sem initialized? */
    CS_SEMAPHORE    qsb_sem;		/* Mutex for hash list */
}   QSO_HASH_BKT;


/*}
** Name: QSR_CB - QSF's server control block.
**
** Description:
**      This structure defines the QSF server control block which gets
**      created as QSF is coming to life for the server.  There will be
**	exaclty one of these structures for the entire server, and will
**	be visable to all QSF sessions.  Therefore, it will be protected
**	against corruption by using semaphores whenever its information is
**	modified, or any lists rooted here are modified.  It maintains
**      information that is server wide, and thus all sessions will share
**      this single structure.  Some of the information in this control
**      block is:  The root for the list of all objects currently stored
**	in QSF, the poolid for QSF's memory pool, and a pointer to the
**      QSF object hash table.
**
** History:
**      14-mar-86 (thurston)
**          Initial creation.
**      23-apr-86 (thurston)
**	    Major overhaul.
**      02-mar-87 (thurston)
**	    Added the standard structure header, and the .qsr_streamid member.
**	    Also removed the .qsr_inited member.
**	05-jul-87 (thurston)
**	    Added the .qsr_tracing member.
**	28-nov-87 (thurston)
**	    Changed the SCF_SEMAPHORE to a CS_SEMAPHORE, to enable easier usage
**	    of CS{p,v}_semaphore() routines instead of going through SCF.
**	26-jan-88 (thurston)
**	    Removed the .qsr_ob1st and .qsr_oblast, replaced them with the hash
**	    table, .qsr_hashtbl, and a few other associated fields:
**	    .qsr_nbuckets, .qsr_object_lru, Also added some more statistical
**	    fields:  .qsr_memtot, .qsr_mxobjs, .qsr_bmaxobjs, .qsr_mxsess,
**	    .qsr_bkts_used, .qsr_mxbkts_used.
**	19-oct-1992 (rog)
**	    Removed qsr_se1st and added qsr_1st_lru, qsr_last_lru,
**	    qsr_no_unnamed, qsr_mx_unnamed, and qsr_psem_owner for better
**	    statistics, performance, and reliability.  Also added qsr_no_named,
**	    qsr_mx_named, qsr_no_index, qsr_mx_index, qsr_mx_size, and
**	    qsr_mx_rsize.
**	06-jan-1993 (rog)
**	    Added qsr_no_destroyed to count the number of objects destroyed by
**	    the LRU algorithm.
**	08-jan-1993 (rog)
**	    Remove qsr_unique_next.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**	10-dec-96 (stephenb)
**	    Add qsr_qsscb_offset for qsf_session().
**	26-Sep-2003 (jenjo02)
**	    Deleted qsr_psem_owner. qsr_psem is now only used to
**	    mutex LRU queue.
*/

typedef struct _QSR_CB
{
/********************   START OF STANDARD HEADER   ********************/

    struct _QSR_CB  *qsr_next;          /* Forward ptr */
    struct _QSR_CB  *qsr_prev;          /* Backward ptr */
    SIZE_TYPE       qsr_length;         /* Length of this structure */
    i2              qsr_type;		/* Type of structure.  For now, 1957 */
#define                 QSRSV_CB        1957	/* for no particular reason */
    i2              qsr_s_reserved;	/* Possibly used by mem mgmt. */
    PTR             qsr_l_reserved;	/*    ''     ''  ''  ''  ''   */
    PTR             qsr_owner;          /* Owner for internal mem mgmt. */
    i4              qsr_ascii_id;       /* So as to see in a dump. */
					/*
					** Note that this is really a char[4]
					** but it is easer to deal with as an i4
					*/
#define                 QSRSV_ASCII_ID  CV_C_CONST_MACRO('Q', 'S', 'V', 'B')

/*********************   END OF STANDARD HEADER   *********************/

    QSF_CB	    *qsr_se1st;         /* Ptr to 1st session control block in
					** list of all active sessions in QSF.
					*/
    PTR             qsr_poolid;         /* Memory pool id for QSF.
					*/
    PTR             qsr_streamid;       /* Memory stream id for QSF's server CB.
					*/
    SIZE_TYPE       qsr_memtot;         /* Total amount of memory to use for
					** QSF's memory pool.  (i.e. The pool
					** size request to ULM.)
					*/
    QSO_OBJ_HDR    *qsr_1st_lru;	/* Points to the first object in the
					** LRU list.  New named objects get
					** placed here.
					*/
    i4		    qsr_named_requests;	/* Number of times we requested a named
					** object since the last time we cleared
					** objects out of QSF memory.
					*/
    i4		    qsr_decay_factor;	/* Amount to adjust the number of
					** objects to lower the point at which
					** we set objects decaying usage to 0.
					*/
    SIZE_TYPE       qsr_memleft;        /* Memory limit counter for all of
					** QSF's memory pool.  Initially QSF
					** will not attempt to limit memory
					** for any entity or group of entities
					** other than the entire pool.
					*/
    i4		    qsr_qsscb_offset;	/* Offset of QSF_CB in SCB */
    i4              qsr_nsess;          /* # QSF sessions currently active.
					*/
    i4              qsr_mxsess;         /* Max # QSF sessions ever active.
					*/
    i4              qsr_lk_next;	/* Next lock id to give out for an
                                        ** exclusive lock of an object.
					*/
    CS_SEMAPHORE    qsr_psem;		/* Mutex semaphore to avoid corruption.
					** of QSR_CB and LRU queue 
					*/
    i4              qsr_tracing;	/* This will be incremented every time
                                        ** a QSF trace point is set, and decre-
                                        ** mented every time one is cleared.
                                        ** Therefore, if this is zero, then
                                        ** there are no QSF trace points set
                                        ** in the entire server.
					*/
    QSF_TRSTRUCT    qsr_trstruct;       /* QSF's server trace vector struct.
					*/
    i4         qsr_nobjs;          /* # objects currently stored in QSF.
					*/
    i4         qsr_mxobjs;         /* Max # objects ever in QSF.
					*/
    i4              qsr_bmaxobjs;       /* Max # objects ever in any one given
					** bucket.
					*/
    i4              qsr_bkts_used;      /* # of hash buckets that actually have
					** at least one object in them.
					*/
    i4              qsr_mxbkts_used;    /* Max # of hash buckets that, at any 
					** given moment, had at least one object
					** in them.
					*/
    i4              qsr_nbuckets;       /* # of hash buckets in table.
					*/
    QSO_HASH_BKT    *qsr_hashtbl;       /* Ptr to the QSF object hash table.
					** The table will actually be a
					** contiguous array of QSO_HASH_BKT's,
					** and used as an array.  It may not,
					** however, be contiguous with this
					** server CB.  The number of buckets in
					** the table will be dynamically set at
					** server startup time.
                                        */
    i4	    	    qsr_no_unnamed;	/* current # of unnamed objects.  */
    i4	    	    qsr_mx_unnamed;	/* Max # ever of unnamed objects. */
    i4	    	    qsr_no_named;	/* current # of named objects.    */
    i4	    	    qsr_mx_named;	/* Max # ever of named objects.   */
    i4	    	    qsr_no_index;	/* current # of master objects.   */
    i4	    	    qsr_mx_index;	/* Max # ever of master objects.  */
    i4              qsr_mx_size;	/* Largest size stored.	          */
    i4              qsr_mx_rsize;	/* Largest size requested.	  */
    i4	    	    qsr_no_destroyed;	/* Objects LRU-destroyed */
    i4	    	    qsr_no_trans;	/* Number TRANSlated */
    i4	    	    qsr_no_defined;	/* Number DEFINEd */
    i4		    qsr_hash_spins;	/* Spins waiting for hash delete OK */
    i8		    qsr_sum_size[QSO_MAX_OBJ+1];  /* sum of obj size for
					** computing an average, by obj type
					** Note! we maintain the average at
					** delete time, for convenience.
					** Avg won't include named objs that
					** haven't been destroyed.  Do a
					** QS506 first if you want the most
					** accurate average-size stats.
					*/
    i4		    qsr_num_size[QSO_MAX_OBJ+1];  /* # of sizes summed */
}   QSR_CB;


/*
**      This group of #defines gives the possible
**      block sizes used for all QSF objects.
*/
#define                 QSO_BQTEXT_BLKSZ  512	/* This is just a guess */
#define                 QSO_BQTREE_BLKSZ 1024	/* This is just a guess */
#define                 QSO_BQP_BLKSZ    1024	/* This is just a guess */
#define                 QSO_BSQLDA_BLKSZ  512	/* This is just a guess */
#define                 QSO_BMASTER_BLKSZ sizeof(QSO_MASTER_HDR)


/*
** References to globals variables declared in other C files.
*/

GLOBALREF QSR_CB     *Qsr_scb;		/* Ptr to QSF's Server Control Block */


/*
**  Forward and/or External function references.
*/

/* Check a QSF tracepoint */
FUNC_EXTERN bool	qst_trcheck		( QSF_CB *scb, i4  xtp );

FUNC_EXTERN STATUS	qsf_attach_inst		( QSO_OBID obid, i2 type );
FUNC_EXTERN i4		qsf_clrmem		( QSF_CB *scb );
FUNC_EXTERN DB_STATUS	qsf_clrsesobj		( QSF_RCB *qsf_rb );
FUNC_EXTERN STATUS	qsf_detach_inst		( QSO_OBID obid, i2 type );
FUNC_EXTERN STATUS	qsf_mo_attach		( void );
FUNC_EXTERN STATUS	qsf_decay_set		( i4  decay_factor );

FUNC_EXTERN DB_STATUS	qsd_obj_dump		( QSF_RCB *qsf_rb );
FUNC_EXTERN DB_STATUS	qsd_obq_dump		( QSF_RCB *qsf_rb );
FUNC_EXTERN void	qsd_lru_dump		(void );

FUNC_EXTERN DB_STATUS	qsr_shutdown		( QSF_RCB *qsf_rb );
FUNC_EXTERN DB_STATUS	qsr_startup		( QSF_RCB *qsf_rb );

FUNC_EXTERN DB_STATUS	qss_bgn_session		( QSF_RCB *qsf_rb );
FUNC_EXTERN DB_STATUS	qss_end_session		( QSF_RCB *qsf_rb );

FUNC_EXTERN DB_STATUS	qso_chtype		( QSF_RCB *qsf_rb );
FUNC_EXTERN DB_STATUS	qso_create		( QSF_RCB *qsf_rb );
FUNC_EXTERN DB_STATUS	qso_destroy		( QSF_RCB *qsf_rb );
FUNC_EXTERN DB_STATUS	qso_destroyall		( QSF_RCB *qsf_rb );
FUNC_EXTERN DB_STATUS	qso_gethandle		( QSF_RCB *qsf_rb );
FUNC_EXTERN DB_STATUS	qso_getnext		( QSF_RCB *qsf_rb );
FUNC_EXTERN DB_STATUS	qso_info		( QSF_RCB *qsf_rb );
FUNC_EXTERN DB_STATUS	qso_just_trans		( QSF_RCB *qsf_rb );
FUNC_EXTERN DB_STATUS	qso_lock		( QSF_RCB *qsf_rb );
FUNC_EXTERN DB_STATUS	qso_palloc		( QSF_RCB *qsf_rb );
FUNC_EXTERN DB_STATUS	qso_rmvobj		( QSO_OBJ_HDR **qso_obj,
						  QSO_MASTER_HDR **master,
						  DB_ERROR *dberr );
FUNC_EXTERN DB_STATUS	qso_ses_commit		( QSF_RCB *qsf_rb );
FUNC_EXTERN DB_STATUS	qso_setroot		( QSF_RCB *qsf_rb );
FUNC_EXTERN DB_STATUS	qso_trans_or_define	( QSF_RCB *qsf_rb );
FUNC_EXTERN DB_STATUS	qso_unlock		( QSF_RCB *qsf_rb );
