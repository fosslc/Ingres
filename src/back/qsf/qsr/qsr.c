/*
**Copyright (c) 2004 Ingres Corporation
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <me.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <scf.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <ulm.h>
#include    <qsfint.h>

/**
**
**  Name: QSR.C - QSF's Server Services Module.
**
**  Description:
**      This file contains all of the routines necessary
**      to perform the all of QSF's Server Services op-codes.
**      This includes the opcodes to initialize and terminate 
**      QSF for the server as a whole.
** 
**      This file contains the following routines:
**
**          qsr_startup() - Initialize QSF for the server.
**          qsr_shutdown() - Terminate QSF for the server.
**
**
**  History:
**      06-mar-86 (thurston)    
**          Initial creation.
**	24-jun-86 (thurston)
**	    Modified code to comply with new SCF_CB.  This affected only the
**	    semaphore init and release.
**	02-mar-87 (thurston)
**	    Made changes to switch from GLOBALDEF'ed server CB to a dynamic
**	    memory server CB pointed to by a GLOBALDEF.
**	26-may-87 (thurston)
**	    Added a returned item to QSR_STARTUP: size of session CB's.
**	04-jun-87 (thurston)
**	    Added mechanisms to QSR_STARTUP for caller to set the memory pool
**	    size, either directly, or indirectly based on the max number of
**	    sessions allowed for the server.
**	05-jul-87 (thurston)
**	    In qsr_startup(), we now initialized the new field in server CB,
**	    .qsr_tracing, to be zero.
**	15-sep-87 (thurston)
**	    In QSR_SHUTDOWN:  Fixed the check for `active objects'.  Formerly an
**          active object was considered to be ANY object in QSF's memory.  Now
**          an active object is better defined to be any unnamed object, or any
**          named object with a lock state other than FREE.  Also, the
**          E_QS0005_ACTIVE_OBJECTS and E_QS0006_ACTIVE_SESSIONS errors
**          correctly return E_DB_WARN, not E_DB_ERROR as was previously
**          happening. 
**	28-nov-87 (thurston)
**	    Changed semaphore code to use CS calls instead of going through SCF.
**	25-jan-88 (thurston)
**          Changed qsr_startup() to create and initialize the QSF object hash
**          table, and modified qsr_shutdown() to look through the object hash
**          table instead of using the obsoleted (and now defunct) object queue.
**	13-nov-91 (rog)
**	    Integrated 6.4 changes into 6.5.
**	28-aug-92 (rog)
**	    Prototype QSF.
**	20-oct-1992 (rog)
**	    Initialize new accounting and LRU members of the QSF Server CB.
**	    Include ulf.h before qsf.h.
**	23-Oct-1992 (daveb)
**	    name the semaphore too.
**	12-Mar-1993 (daveb)
**	    also include <me.h>.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	28-Apr-1995 (jenjo02)
**	    Combined CSi|n_semaphore functions calls into single
**	    CSw_semaphore call.
**	23-sep-1996 (canor01)
**	    Move global data definition to qsrdata.c.
**	11-dec-96 (stephenb)
**	    Set qsr_qsscb_offset at startup for use in qsf_session().
**	10-aug-1997 (canor01)
**	    Remove semaphore before releasing memory.
**	19-Aug-1997 (jenjo02)
**	    Distinguish between private and shared ULM streams.
**	    Piggyback ulm_palloc() calls with ulm_openstream() where appropriate.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      7-oct-2004 (thaju02)
**          Define memtot, memleft as SIZE_TYPE for memory pool > 2Gig.
**      23-Nov-2005 (horda03) Bug 115554/INGSRV3518
**          During shutdown, Master Object Header CONDITION memory was not
**          being released, leading to memory access errors.
**	11-Jun-2010 (kiria01) b123908
**	    Init ulm_streamid_p for ulm_openstream to fix potential segvs.
**	29-Oct-2010 (jonj) SIR 120874
**	    Conform to use new uleFormat, DB_ERROR constructs.
**/


/*
** Definition of all global variables owned by this file.
*/

GLOBALREF QSR_CB              *Qsr_scb;	    /* Ptr to QSF's server CB */


/*{
** Name: QSR_STARTUP - Initialize QSF for the server.
**
** Description:
**      This is the entry point to the QSF QSR_STARTUP routine. 
**      This opcode does everything necessary to initialize QSF for the
**      server.  This includes initializing QSF's global SERVER CONTROL
**      BLOCK, allocating QSF's memory POOL, initializing the OBJECT
**      HASH TABLE, and initializing the OBJECT queue.
**
**      Since the memory QSF will be doling out to the other facilities will
**      need to be shared between sessions, the memory POOL QSF gets from
**      SCU_MALLOC will be allocated using the DB_NOSESSION constant as the
**      session id.  This is also why QSF's global SERVER CONTROL BLOCK exists;
**      so that it can keep track of all of this memory this memory on a server
**      basis, not session basis.  Because of this sharing, the SERVER CONTROL
**	BLOCK will be protected by mutex semaphores.  (The memory pool used by
**	QSF will also be protected by mutex semaphores, but this will be done
**	by the ULM routines themselves, and thus transparent to QSF.)
**
** Inputs:
**      QSR_STARTUP                     Op code to qsf_call().
**      qsf_rb                          QSF request block.
**	    .qsf_pool_sz		If this is set to a positive number,
**					then QSF will allocate that many bytes
**					for its memory pool.  If non-positive,
**					then ...
**	    .qsf_max_active_ses		... If this is set to a positive number,
**					take this to be the max number of active
**					sessions allowed for the server, and
**					calculate the memory pool size based on
**					that.  If non-positive, then ...
**					... use an internal constant as the size
**					of the memory pool.
**
** Outputs:
**      qsf_rb                          QSF request block.
**	    .qsf_server			Ptr to QSF's server control block.
**					This must be passed into QSF on each
**					QSS_BGN_SESSION call.
**	    .qsf_scb_sz			Number of bytes required for a QSF
**					session control block.
**	    .qsf_error			Filled in with error if encountered.
**		.err_code		The QSF error that occurred.  One of:
**			E_QS0000_OK
**				    All is fine.
**			E_QS0001_NOMEM
**				    Not enough memory.
**			E_QS9999_INTERNAL_ERROR
**				    Meltdown.
**
**      Returns:
**          E_DB_OK
**          E_DB_WARN
**          E_DB_ERROR
**          E_DB_FATAL
**
**      Exceptions:
**          none
**
** Side Effects:
**          Grabs a large memory pool from SCF (via ULM)
**          that QSF will hold onto until QSF gets
**          terminated (QSR_SHUTDOWN) at server shutdown time.
**
** History:
**      06-mar-86 (thurston)
**          Initial creation.
**	24-jun-86 (thurston)
**	    Modified code to comply with new SCF_CB.  This affected only the
**	    semaphore init.
**	26-may-87 (thurston)
**	    Added a returned item: size of session CB's.
**	04-jun-87 (thurston)
**	    Added mechanisms for caller to set the memory pool size, either
**	    directly, or indirectly based on the max number of sessions allowed
**	    for the server.
**	05-jul-87 (thurston)
**	    Initialized the new field in server CB, .qsr_tracing, to be zero.
**	28-nov-87 (thurston)
**	    Changed semaphore code to use CS calls instead of going through SCF.
**	25-jan-88 (thurston)
**          Changed qsr_startup() to create and initialize the QSF object hash
**          table.
**	07-jul-91 (jrb)
**	    Initialized qsf semaphore using CSi_semaphore instead of just
**	    zeroing out bytes.
**	23-Oct-1992 (daveb)
**	    name the semaphore too.
**	30-nov-1992 (rog)
**	    Initialize new accounting fields in the QSR_CB structure.
**	06-jan-1993 (rog)
**	    Initialize qsr_no_destroyed.
**	07-jan-1993 (rog)
**	    Remove qsr_unique_next initialization.
**	    Added call to qsf_mo_attach();
**	    Initialize the decay factor by calling qsf_set_decay().
**	 7-jan-94 (swm)
**	    Bug #58635
**	    Added PTR cast for qsr_owner which has changed type to PTR.
**	26-Sep-2003(jenjo02)
**	    Deleted qsr_psem_owner, added qsb_sem hash buckets,
**	    qsmo_sem replaces qsmo_crp_sem.
**	22-Feb-2005 (schka24)
**	    Init entire QSR cb to zero.  Raise average QP size (based on very
**	    limited evidence!)
*/

DB_STATUS
qsr_startup( QSF_RCB *qsf_rb )
{
    ULM_RCB             ulm_rcb;
    DB_STATUS           status;
    PTR			poolid;
    SIZE_TYPE		memtot;
    SIZE_TYPE		memleft;
    i4		b;
    i4			i;
    QSO_HASH_BKT	*bucket;

#define	QS0_EST_QP_SIZE		10000	/* Est. size of average named QP */
#define	QS0_MIN_HASH_BUCKETS	 100	/* Smallest # hash buckets allowed */
#define	QS0_MAX_HASH_BUCKETS   MAXI2	/* Largest # hash buckets allowed */

    CLRDBERR(&qsf_rb->qsf_error);

    /* Calculate size of memory pool to ask ULM for */
    /* -------------------------------------------- */
    if (qsf_rb->qsf_pool_sz > 0)
	memtot = qsf_rb->qsf_pool_sz;
    else if (qsf_rb->qsf_max_active_ses > 0)
	memtot = QSF_PBASE_BYTES +
		    ((qsf_rb->qsf_max_active_ses - 1) * QSF_PBYTES_PER_SES);
    else
	memtot = QSF_MEMPOOL_SIZE;


    /* Get memory pool (from ULM) and initialize it */
    /* -------------------------------------------- */
    ulm_rcb.ulm_sizepool = memtot;
    ulm_rcb.ulm_facility = DB_QSF_ID;
    ulm_rcb.ulm_blocksize = 0;		/* Let ULM decide on block size */
    status = ulm_startup(&ulm_rcb);
    if (DB_FAILURE_MACRO(status))
    {
        if (ulm_rcb.ulm_error.err_code == E_UL0005_NOMEM)
        {
	    SETDBERR(&qsf_rb->qsf_error, 0, E_QS0001_NOMEM);
        }
        else
        {
	    SETDBERR(&qsf_rb->qsf_error, 0, E_QS9999_INTERNAL_ERROR);
	    status = E_DB_FATAL;
        }
        return (status);
    }
    poolid = ulm_rcb.ulm_poolid;


    /* Now start a stream and allocate a piece for QSF's server CB */
    /* ----------------------------------------------------------- */
    memleft = memtot;
    ulm_rcb.ulm_memleft = &memleft;
    /* No one allocates from this stream, so make it PRIVATE */
    /* Open stream and alocate QSR_CB with one effort */
    ulm_rcb.ulm_flags = ULM_PRIVATE_STREAM | ULM_OPEN_AND_PALLOC;
    ulm_rcb.ulm_psize = sizeof(QSR_CB);
    ulm_rcb.ulm_streamid_p = NULL;

    status = ulm_openstream(&ulm_rcb);
    if (DB_FAILURE_MACRO(status))
    {
        if (ulm_rcb.ulm_error.err_code == E_UL0005_NOMEM)
        {
	    SETDBERR(&qsf_rb->qsf_error, 0, E_QS0001_NOMEM);
        }
        else
        {
	    SETDBERR(&qsf_rb->qsf_error, 0, E_QS9999_INTERNAL_ERROR);
	    status = E_DB_FATAL;
        }
        return (status);
    }

    qsf_rb->qsf_server = ulm_rcb.ulm_pptr;

    /* Initialize QSF's server CB */
    /* -------------------------- */
    Qsr_scb = (QSR_CB *) ulm_rcb.ulm_pptr;
    MEfill(sizeof(QSR_CB), 0, (PTR) Qsr_scb);	/* Zero it all */
    Qsr_scb->qsr_next = (QSR_CB *) NULL;
    Qsr_scb->qsr_prev = (QSR_CB *) NULL;
    Qsr_scb->qsr_length = sizeof(QSR_CB);
    Qsr_scb->qsr_type = QSRSV_CB;
    Qsr_scb->qsr_owner = (PTR)DB_QSF_ID;
    Qsr_scb->qsr_ascii_id = QSRSV_ASCII_ID;
    Qsr_scb->qsr_qsscb_offset = qsf_rb->qsf_qsscb_offset;

    Qsr_scb->qsr_poolid = poolid;
    Qsr_scb->qsr_streamid = ulm_rcb.ulm_streamid;
    Qsr_scb->qsr_memtot = memtot;
    Qsr_scb->qsr_memleft = memleft;
    Qsr_scb->qsr_tracing = 0;
    ult_init_macro(&Qsr_scb->qsr_trstruct.trvect, 128, 0, 8);


    /* Initialize mutex semaphore for QSF's LRU queue */
    /* ---------------------------------------------- */
    if (CSw_semaphore(&Qsr_scb->qsr_psem, CS_SEM_SINGLE,
    			"QSR sem" ) != OK)
    {
	SETDBERR(&qsf_rb->qsf_error, 0, E_QS0002_SEMINIT);
	return (E_DB_ERROR);
    }

    /* Session control block list inited to empty by MEfill */

    /* Init the next lock id to be MINI2 */
    /* --------------------------------- */
    Qsr_scb->qsr_lk_next = MINI2;

    /* QSF's object hash table inited to zero via MEfill */

    /*
    ** Figure out how many buckets to use:
    **
    **				  (80% of total memory pool)
    **	    Let:	b  =  2 x --------------------------
    **				     (est. size of a QP)
    **
    **	    If:		b  <  (smallest # hash buckets)
    **	    Then, let:	b  =  (smallest # hash buckets)
    **
    **	    If:		b  >  (largest # hash buckets)
    **	    Then, let:	b  =  (largest # hash buckets)
    */

    b = max((2 * (0.80 * memtot) / QS0_EST_QP_SIZE), QS0_MIN_HASH_BUCKETS);
    b = min(b, QS0_MAX_HASH_BUCKETS);
    Qsr_scb->qsr_nbuckets = b;

    /* Get a piece of memory for the hash table */
    /* ---------------------------------------- */
    ulm_rcb.ulm_psize = Qsr_scb->qsr_nbuckets * sizeof(QSO_HASH_BKT);
    status = ulm_palloc(&ulm_rcb);
    if (DB_FAILURE_MACRO(status))
    {
        if (ulm_rcb.ulm_error.err_code == E_UL0005_NOMEM)
        {
	    SETDBERR(&qsf_rb->qsf_error, 0, E_QS0001_NOMEM);
        }
        else
        {
	    SETDBERR(&qsf_rb->qsf_error, 0, E_QS9999_INTERNAL_ERROR);
	    status = E_DB_FATAL;
        }
        return (status);
    }
    
    Qsr_scb->qsr_hashtbl = (QSO_HASH_BKT *) ulm_rcb.ulm_pptr;

    /* Now initialize each bucket */
    /* -------------------------- */
    bucket = Qsr_scb->qsr_hashtbl;
    for (i = 0; i < b; i++)
    {
	/* qsb_sem won't be initialized until bucket is used */
	bucket->qsb_ob1st = NULL;
	bucket->qsb_nobjs = bucket->qsb_mxobjs = 0;
	bucket->qsb_init = FALSE;
	bucket++;
    }


    /* Now initialize the LRU algorithm information */
    /* -------------------------------------------- */
    Qsr_scb->qsr_1st_lru = (QSO_OBJ_HDR *) NULL;
    Qsr_scb->qsr_named_requests = 0;
    qsf_set_decay(QSF_DEFAULT_DECAY);

    /* Miscellaneous statistics inited to zero by MEfill */

    qsf_rb->qsf_scb_sz = sizeof(QSF_CB);

    /* Register QSF objects with the Management Objects facilities */
    /* ----------------------------------------------------------- */
    if (qsf_mo_attach() != OK)
    {
	SETDBERR(&qsf_rb->qsf_error, 0, E_QS0020_QS_INIT_MO_ERROR);
	status = E_DB_FATAL;
    }

    /* "All is well" */
    /* ------------- */
    return (E_DB_OK);
}

/*{
** Name: QSR_SHUTDOWN	- Terminate QSF for the server.
**
** Description:
**      This is the entry point to the QSF QSR_SHUTDOWN routine.
**      This opcode does everything necessary to terminate QSF for 
**      the server.  This includes freeing all memory received from SCF
**	any time during QSF's existence.  Since all OBJECT memory, OBJECT
**	HEADER memory, and OBJECT HASH TABLE memory (i.e.  all memory
**      received from SCF) is allocated out of the memory POOL which QSF
**      gets from SCF (via ULM) at QSR_STARTUP time, this can be done by
**      simply freeing the memory POOL as a whole.
**
** Inputs:
**      QSR_SHUTDOWN                    Op code to qsf_call().
**      qsf_rb                          QSF request block.
**		.qsf_force		If this is non-zero, QSF will force
**					the shutdown even though there are
**					still existing objects and/or active
**					QSF sessions.
**
** Outputs:
**      qsf_rb                          QSF request block.
**		.qsf_error		Filled in with error if encountered.
**			.err_code	The QSF error that occurred.  One of:
**                              E_QS0000_OK
**                                  All is fine.
**                              E_QS0005_ACTIVE_OBJECTS
**                                  QSF still has some objects in its memory.
**				    (This just produces a warning:  E_DB_WARN)
**                              E_QS0006_ACTIVE_SESSIONS
**                                  QSF still has some active sessions.
**				    (This just produces a warning:  E_DB_WARN)
**                              E_QS0007_MEMPOOL_RELEASE
**                                  Error releasing QSF's memory pool.
**                              E_QS000E_SEMFREE
**                                  Error freeing semaphore.
**
**	Returns:
**	    E_DB_OK
**	    E_DB_WARN
**	    E_DB_ERROR
**	    E_DB_FATAL
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    Frees QSF's memory pool (via ULM) previously
**	    grabbed by QSF from ULM.
**
** History:
**	06-mar-86 (thurston)
**          Initial creation.
**	24-jun-86 (thurston)
**	    Modified code to comply with new SCF_CB.  This affected only the
**	    semaphore release.
**	15-sep-87 (thurston)
**	    Fixed the check for `active objects'.  Formerly an active object was
**	    considered to be ANY object in QSF's memory.  Now an active object
**	    is better defined to be any unnamed object, or any named object with
**	    a lock state other than FREE.  Also, the E_QS0005_ACTIVE_OBJECTS and
**	    E_QS0006_ACTIVE_SESSIONS errors correctly return E_DB_WARN, not
**	    E_DB_ERROR as was previously happening.
**	28-nov-87 (thurston)
**	    Changed semaphore code to use CS calls instead of going through SCF.
**	25-jan-88 (thurston)
**	    Modified code to look through the object hash table instead of using
**	    the obsoleted (and now defunct) object queue.
**	07-jul-91 (jrb)
**	    Put code into "ifdef xDEBUG" because it was looking into the
**	    internal contents of a semaphore.
**	30-nov-1992 (rog)
**	    Update the left-over object detection mechanism to reflect the
**	    new hash table definitions.
**	10-aug-1997 (canor01)
**	    Remove semaphore before releasing memory.
**	13-aug-1997 (canor01)
**	    Allow the cleanup to continue even without the force flag, but
**	    don't report errors if the force flag is set.
**	21-Oct-2003 (jenjo02)
**	    Skip masters that have no objects.
**      23-Nov-2005 (horda03) Bug 115554/INGSRV3518
**          Destroy the Condition memory.
*/

DB_STATUS
qsr_shutdown( QSF_RCB *qsf_rb )
{
    ULM_RCB             ulm_rcb;
    i4			*force = &qsf_rb->qsf_force;
    QSO_MASTER_HDR	*master;
    QSO_OBJ_HDR		*obj;
    QSO_HASH_BKT	*bucket;
    i4			i;
    i4		error;

    CLRDBERR(&qsf_rb->qsf_error);


    /* See if object list is empty */
    /* --------------------------- */
    if (Qsr_scb->qsr_nobjs != 0)
    {
	if (Qsr_scb->qsr_no_unnamed && !*force)
	{
	    uleFormat( &qsf_rb->qsf_error, E_QS0005_ACTIVE_OBJECTS, 
	    		(CL_ERR_DESC*)NULL,
	    		ULE_LOG, NULL, NULL,
			(i4) 0, NULL, &error, 0);
	    return (E_DB_WARN);
	}
	else
	{
	    /*
	    ** Don't generate the error if only "un-used,
	    ** named objects" are left in the queue.
	    */
	    for (i = 0; i < Qsr_scb->qsr_nbuckets; i++)
	    {
		bucket = &Qsr_scb->qsr_hashtbl[i];
		for (master = bucket->qsb_ob1st;
		     master != (QSO_MASTER_HDR *) NULL;
		     master = master->qsmo_obnext)
		{
		    /* Skip incomplete or partially deleted masters */
		    if (master->qsmo_type == QSMOBJ_TYPE)
		    {
			obj = master->qsmo_obj_1st;
			while ( obj )
			{
			    if (obj->qso_lk_state != QSO_FREE && !*force)
			    {
				uleFormat( &qsf_rb->qsf_error, E_QS0005_ACTIVE_OBJECTS, 
					    (CL_ERR_DESC*)NULL,
					    ULE_LOG, NULL, NULL,
					    (i4) 0, NULL, &error, 0);
				return (E_DB_WARN);
			    }
			    obj = obj->qso_monext;
			}
			CSr_semaphore(&master->qsmo_sem);
                        CScnd_free( &master->qsmo_cond );
		    }
		}
	    }
	}
    }


    /* See if session control block list is empty */
    /* ------------------------------------------ */
    if (Qsr_scb->qsr_se1st != (QSF_CB *) NULL  &&  !*force)
    {
	uleFormat( &qsf_rb->qsf_error, E_QS0006_ACTIVE_SESSIONS, 
		    (CL_ERR_DESC*)NULL,
		    ULE_LOG, NULL, NULL,
		    (i4) 0, NULL, &error, 0);
	return (E_DB_WARN);
    }

    /* Destroy used hash semaphores */
    for (i = 0; i < Qsr_scb->qsr_nbuckets; i++)
    {
	bucket = &Qsr_scb->qsr_hashtbl[i];
	if ( bucket->qsb_init )
	    CSr_semaphore(&bucket->qsb_sem);
    }


    /* Destroy the semaphore for QSF's LRU queue */
    CSr_semaphore(&Qsr_scb->qsr_psem);

    /* Release memory pool (to ULM) ... Note we do not have  */
    /* to do specific ulm_closestream() calls, since shuting */
    /* down the pool closes all streams within the pool.     */
    /* ----------------------------------------------------- */
    ulm_rcb.ulm_facility = DB_QSF_ID;
    ulm_rcb.ulm_poolid = Qsr_scb->qsr_poolid;
    if (ulm_shutdown(&ulm_rcb) != E_DB_OK)
    {
	SETDBERR(&qsf_rb->qsf_error, 0, E_QS0007_MEMPOOL_RELEASE);
        return (E_DB_ERROR);
    }


    /* "All is well" */
    /* ------------- */
    Qsr_scb = NULL;
    return (E_DB_OK);
}
