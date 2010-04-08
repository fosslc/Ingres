/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cm.h>
#include    <cs.h>
#include    <me.h>
#include    <sl.h>
#include    <st.h>
#include    <tr.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <scf.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <ulm.h>
#include    <qsfint.h>
#include    <hshcrc.h>

/**
**
**  Name: QSO.C - QSF's Object Services Module.
**
**  Description:
**      This file contains all of the routines necessary
**      to perform the all of QSF's Object Services op-codes.
**      This includes the opcodes to create and destroy objects
**	from QSF's memory, lock and unlock objects, allocate
**	pieces for objects, set the root address of an object, etc.
** 
**      This file contains the following routines:
**
**          qso_create() - Create a QSF object.
**          qso_destroy() - Destroy a QSF object (by handle).
**          qso_destroyall() - Destroy all members of a multiple QSF object.
**          qso_lock() - Lock a QSF object exclusively or shared.
**          qso_unlock() - Unlock a QSF object.
**          qso_setroot() - Set the root address of a QSF object.
**          qso_info() - Get information about a QSF object.
**          qso_palloc() - Allocate memory pieces for a QSF object.
**          qso_gethandle() - Retrieve the handle for a named QSF object.
**          qso_getnext() - Retrieve the next handle of a multiple QSF object.
**	    qso_trans_or_define() - Translate or define an alias atomically
**	    qso_just_trans() - Translate an alias (but don't define it).
**          qso_chtype() - Change type of a QSF object.
**	    qso_rmvobj() - Remove an object from the object list.
**
**      This file also contains the following internal routines, which are
**	declared "static" and are therefore not visible outside of this file:
**
**	    qso_rmvindex() - Remove an index master object.
**	    qso_wait_for() - Wait for an object in the process of being created.
**	    qso_mkobj() - Make an object, ie. low level of qso_create().
**	    qso_lknext() - Returns the next available lock id for QSF.
**	    qso_mkindex() - Add a master to the hash table.
**	    qso_find() - Find a QSO object with the given name and type.
**	    qso_lkset() - Actually locks the object.
**	    qso_hash() - Returns ptr to appropriate hash bucket for an obj ID.
**	    qso_nextbyhandle() - Internal function to find the next multiple QP
**				 based on a handle.
**
**
**  History:
**      06-mar-86 (thurston)    
**          Initial creation.
**      02-jun-86 (thurston)    
**	    Corrected bug in qso_setroot() where the routine always returned
**	    E_DB_ERROR even when there was no error.
**	24-jun-86 (thurston)
**	    Modified code to comply with new SCF_CB.  This affected all calls
**	    to SCU_WAIT and SCU_RELEASE.
**	23-feb-87 (thurston)
**	    Reversed all STRUCT_ASSIGN_MACRO's to conform with CL spec.
**	02-mar-87 (thurston)
**	    GLOBALDEF changes.
**	02-mar-87 (thurston)
**	    Added support for new type of QSF object: SQLDA.
**	16-mar-87 (stec)
**	    Added new interface qso_chtype.
**	12-may-87 (thurston)
**	    Added an xDEBUG section to qso_create() that dumps the QSF object
**	    queue whenever a request to create a named object is recieved.  This
**	    should ultimately become a trace point.
**	05-jul-87 (thurston)
**	    Added code to qso_create(), qso_rmvobj(), and qso_addobj() to check
**	    for server wide tracing before even attempting to look for a
**	    particular trace flag.
**	05-aug-87 (thurston)
**	    Changed interface to QSO_GETHANDLE to support object locking at
**	    get_handle time.  Added new low level object locker and modified
**	    QSO_LOCK to use it.
**	21-aug-87 (thurston)
**	    Speeded up the search for QSF object names in qso_find() by looking
**	    at first 8 bytes of name as two i4's.  This will avoid excessive
**	    calls to MEcmp(). 
**	28-nov-87 (thurston)
**	    Changed semaphore code to use CS calls instead of going through SCF.
**	20-jan-88 (eric)
**	    Added support for the QSO_TRANS_OR_DEFINE opcode.
**	29-jan-88 (thurston)
**	    Added support for the QSF hash table.
**	19-feb-88 (thurston)
**	    Checks in qso_lock() and qso_gethandle() for EXLOCKs on a QP have
**	    been modified to check for an EXLOCK on a "sharable" QP.
**	24-may-88 (eric)
**	    Added support for the QSO_JUST_TRANS opcode
**	20-dec-89 (ralph)
**	    QUANTUM BUG FIXES:
**	    Don't remove object if we have the only shared lock, but some
**	    other thread is waiting on the QP to be built in qso_just_trans
**	    or qso_trans_or_define (qso_destroy).
**	    Don't add alias objects to the LRU queue (qso_addobj).
**	    Return WASDEFINED if object is removed (qso_trans_or_define).
**	    Return UNKNOWN_OBJ if object is removed (qso_just_trans).
**	    Provide interface to lock QSF object in qso_trans_or_define and
**	    qso_just_trans.
**	06-jun-90 (andre)
**	    Made changes to allow creation of multiple aliases on QP objects.
**	    Added a new routine to create a new alias on an existing QP object
**	    (this will be different from TRANS_OR_DEFINE since we will expect
**	    the object to exist)
**	    Made changes to qso_rmvobj() to remove aliases iteratively, rather
**	    than recursively.
**	13-nov-91 (rog)
**	    Integrated 6.4 changes into 6.5.
**	28-aug-1992 (rog)
**	    Prototype QSF and silence compiler warnings.
**	08-Jan-1992 (fred)
**	    Added qso_associate() in support of peripheral object management.
**	30-nov-1992 (rog)
**	    Major changes in support of the multiple QP objects (FIPS), and new
**	    tuning and supportability improvements.  Include ulf.h before qsf.h.
**	08-mar-1993 (rog)
**	    Changed the interface to qso_mkobj() so that the scb is passed in
**	    to qso_mkobj(), which means that the function doesn't have to waste
**	    time getting the scb.
**	29-jun-1993 (rog)
**	    Include <tr.h> for the xDEBUGed TRdisplay()s.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	 7-jan-94 (swm)
**	    Bug #58635
**	    Added PTR cast for qso_owner (which has changed type to PTR) in
**	    assignments and comparisons.
**	30-mar-94 (rickh)
**	    Object usage counts were pointing at an uninitialized area.
**	16-may-1994 (rog)
**	    Attach and detach named QP's to MO structures.
**	28-Apr-1995 (jenjo02)
**	    Combined CSi|n_semaphore functions calls into single
**	    CSw_semaphore call.
**	09-may-1996 (kch)
**	    In the function qso_hash(), we now check for DB_MAXNAME (32)
**	    characters in the object name as well as the first blank. This
**	    change fixes bug 75638.
**	09-jan-1997(angusm)
**	    Diagnostic and other changes relating to bug 76278.
**	    These treat the symptoms, not the root cause.
**	15-Aug-1997 (jenjo02)
**	    Changed ulm_streamid from (PTR) to (PTR*) so that ulm
**	    can destroy those handles when the memory is freed.
**	19-Aug-1997 (jenjo02)
**	    Distinguish between private and shared ULM streams.
**	    Piggyback ulm_palloc() calls with ulm_openstream() where appropriate.
**	19-nov-1998 (somsa01)
**	    In qso_trans_or_define() and qso_just_trans(), increment the
**	    wait status of an object that is found to let others know that
**	    the current session is using it.  (Bug #94216)
**	30-nov-1998 (somsa01)
**	    Removed previous change from qso_just_trans(); it is not needed
**	    there. Also, moved decrementation of qso_wait to AFTER the check
**	    of qso_psem_owner and qso_psem.
**	03-dec-1998 (somsa01)
**	    In qso_rmvobj(), if we are just removing the object from the
**	    list of objects attached to the master object, we need to also
**	    update the alias' pointer to the first object (if there is one).
**	    Also, moved the decrement of qso_wait to BEFORE the releasing
**	    of the qso_psem semaphore in qso_destroy(), qso_destroyall(),
**	    qso_unlock(), and qso_wait_for(). Also, removed some uneeded
**	    variables.  (Bug #94216)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Jan-2001 (jenjo02)
**	    qsf_session() function replaced with GET_QSF_CB macro,
**	    callers of QSF now must supply session's CS_SID in QSF_RCB
**	    (qsf_sid). qsf_call() guarantees that qsf_rb->qsf_scb
**	    contains QSF_CB* when these functions are invoked.
**	    Added QSF_CB* parameter to qsf_clrmem().
**	04-sep-2003 (abbjo03)
**	    Add missing declarations in xDEBUG sections.
**	26-Sep-2003 (jenjo02)
**	    Wholesale redesign to eliminate qsr_psem single point of
**	    contention. Hash lists and individual master objects are now
**	    mutexed. "waits" are done using CS_CONDITION objects
**	    instead of mutex blocks.
**	21-Feb-2005 (schka24)
**	    Remove deadwood.  Eliminate alias as a separate index object,
**	    caused races / segv's because find would find the alias
**	    on the hash chain just as destroy deleted master.  (Despite
**	    an andre-comment above, only one alias per master is allowed.)
**	    Prevent clrmem/clrmem and clrmem/destroyall races, and
**	    a rare create/create race.  Combine named destroy's into
**	    common routine. Attempt to update comments with recently
**	    hard-won knowledge.
**	    It's mildly amusing to note that these races were uncovered
**	    by a completely abusive named-object create/delete load,
**	    amounting to thousands per second among 16 sessions -- caused
**	    by a bug I put into DMF a year ago!
**	9-feb-2007 (kibro01) b113972
**	    Add parameter to qso_rmvobj to state whether we have a
**	    lock on the LRU queue
**	11-feb-2008 (smeke01) b113972
**	    Back out above change.
**	14-Nov-2008 (kiria01) b120998
**	    Handle GETHANDLE of proto-creates objects better - detect the
**	    oddity and the owner state & allow for it to be destroyed.
**	    Also added debugging stall point to aid multi-thread debugging.
**	22-May-2009 (kibro01) b122104
**	    Pass in scb so we can compare an unfinished object's scb against
**	    the excl_owner field to cater for the case where this session
**	    created the object, stopped before producing a query plan, and is
**	    now coming along with another object with the same text using
**	    dynamic query caching.
*/


/*
**  Definition of static variables and forward static functions.
*/

static DB_STATUS	qso_destroy_one(QSO_OBJ_HDR *obj,
					QSO_MASTER_HDR **master_p,
					QSF_RCB *qsf_rb,
					i4 *err_code);
static DB_STATUS	qso_mkindex	( QSO_OBID *qso_obid,
					  QSO_MASTER_HDR **master, 
					  QSF_CB *scb,
					  QSO_HASH_BKT **hash,
					  i4 *err_code );
static QSO_OBJ_HDR	*qso_find	( QSO_OBID *qso_obid,
					  QSO_MASTER_HDR **master,
					  QSO_HASH_BKT **hash);
static QSO_HASH_BKT	*qso_hash	( QSO_OBID *objid );
static i4		qso_lknext	( void );
static DB_STATUS	qso_lkset	( QSO_OBJ_HDR *obj, i4  lock_req,
					  i4  *lock_id, PTR *root,
					  QSF_CB *scb,
					  i4 *err_code );
static DB_STATUS	qso_mkobj	( QSO_OBJ_HDR **out_obj,
					  QSO_OBID *new_objid, i4  blksz,
					  QSF_CB *scb,
					  QSO_HASH_BKT **hash,
					  QSO_MASTER_HDR **master,
					  i4 *err_code );
static DB_STATUS	qso_nextbyhandle( QSO_OBJ_HDR *obj, QSF_RCB *qsf_rb,
					  QSF_CB *scb, 
					  QSO_MASTER_HDR **master,
					  i4 *err_code );
static DB_STATUS	qso_wait_for	( QSO_OBJ_HDR **qso_obj, 
					  QSO_MASTER_HDR **master,
					  QSF_CB *scb,
					  i4 *err_code );
static DB_STATUS	qso_rmvindex	( QSO_MASTER_HDR **index,
					  i4 *err_code );

#ifdef QSO_DBG_HOLD
/* If QSO_DBG_HOLD defined externally, stall points are
** compiled in to facilitate debugging. Define as 0 to
** build as latent and set to 1 to quietly stall the dbms
** thread involved.
** The commented out stall points are left in place to
** mark that they are not forgotten but are usuall code
** paths */
volatile i4 onHOLD = QSO_DBG_HOLD;
volatile QSF_RCB *rcbHOLD;
volatile QSF_CB *cbHOLD;
void DBGSTALL(QSF_RCB *rb) {while(onHOLD && rcbHOLD != rb) sleep(1);}
void DBGSTALL2(QSF_CB *rb) {while(onHOLD && cbHOLD != rb) sleep(1);}
#else
#define DBGSTALL(v)
#define DBGSTALL2(v)
#endif



/*{
** Name: QSO_CREATE - Create a QSF object.
**
** Description:
**      This is the entry point to the QSF QSO_CREATE routine. 
**	An object is created in QSF's memory space.  The object can
**	be named or unnamed.  An unnamed object is private to the
**	calling session, and indeed QSF adds little or no value for
**	unnamed objects.  Named objects however can be shared among
**	sessions, and can have a lifetime beyond one query lifetime.
**	The object that's created is marked with the QSF exclusive
**	lock, so that nobody else will be able to use it or delete it
**	until the object is unlocked (with QSO_UNLOCK).
**
**	You might expect that named objects are actually created here.
**	As it turns out, you would be wrong.  Certainly, named objects
**	CAN be created here, but they usually aren't.  Since the rest
**	of Ingres distinguishes between an internal (back-end) name
**	and an external (front-end or alias) name, what almost always
**	creates a shareable, named object is QSO_TRANS_OR_DEFINE.
**	Once the named object is created (via define), the creating
**	session will end up here, in QSO_CREATE.  What we'll do for
**	that specific case is exclusive-lock the object, as advertised
**	above, and simply hand it back to the caller.
**
**	At this point, there are only four types of objects that QSF will
**	store:  QTEXT (query text), QTREE (query tree), QP (query plan), and
**	SQLDA (used for `describe').
**
** Inputs:
**      QSO_CREATE                      Op code to qsf_call().
**      qsf_rb                          Request block.
**		.qsf_obj_id		Object id for the object to be
**					created.
**			.qso_type	The type of object this will be.
**					Must be one of the following:
**					    QSO_QTEXT_OBJ - query text
**					    QSO_QTREE_OBJ - query tree
**					    QSO_QP_OBJ    - query plan
**					    QSO_SQLDA_OBJ - SQLDA used for SQL's
**							    `describe' stmt.
**			.qso_lname	Length in bytes of the object name
**					given in .qso_name.  If zero, this
**					is an un-named object.
**			.qso_name	Name for the object (optional).
**                                      For named objects, the object id
**                                      must be unique and is defined by
**                                      the name and type of object.
**                                      Therefore, there can be up to
**                                      4 objects with the same name:
**                                      1 QTEXT, 1 QTREE, 1 QP, 1 SQLDA. 
**
** Outputs:
**      qsf_rb
**		.qsf_obj_id
**			.qso_handle	This will be the only part of
**                                      the object id used for the rest
**                                      of the QSF calls that work with
**                                      this object (except QSO_GETHANDLE,
**                                      which will return this).  This
**                                      is actually a pointer to the
**                                      object header, but should only
**                                      be used as an abstraction by the
**                                      calling routines. 
**		.qsf_lk_id		Since CREATE implies locking the object
**					exclusively, this will be set to the
**					lock id QSF assigns to the object.
**					This will be required whenever any form
**                                      of object modification is done through
**                                      the QSF routines (e.g. QSO_PALLOC,
**                                      QSO_SETROOT, etc.).
**		.qsf_error		Filled in with error if encountered.
**			.err_code	The QSF error that occurred.  One of:
**				E_QS0000_OK
**				    Operation succeeded.
**				E_QS0001_NOMEM
**				    Out of memory.
**				E_QS0009_BAD_OBJTYPE
**				    Illegal object type.
**				E_QS000A_OBJ_ALREADY_EXISTS
**				    An object of this name and type
**				    already exists.
**				E_QS000B_BAD_OBJNAME
**				    Illegal object name.
**                              E_QS000C_ULM_ERROR
**                                  Unexpected ULM error.
**                              E_QS000D_CORRUPT
**                                  QSF's internal structures got crunched.
**                              E_QS001C_EXTRA_OBJECT
**				    A QP object was added to a chain of QP
**				    objects.
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
**          Other facilities and sessions will now "see" this
**	    object just created, since an object header will
**	    be set up for it and entered into the object queue and
**          the object hash table.  QSF will guarantee, however, that
**	    other callers will not be able to get access to this object
**	    via QSO_LOCK until the proper QSO_UNLOCK has been done.
**
** History:
**	06-mar-86 (thurston)
**          Initial creation.
**	24-jun-86 (thurston)
**	    Modified code to comply with new SCF_CB.  This affected the call
**	    to SCU_WAIT and SCU_RELEASE.
**	02-mar-87 (thurston)
**	    Added support for new type of QSF object: SQLDA.
**	12-may-87 (thurston)
**	    Added an xDEBUG section that dumps the QSF object queue whenever
**	    a request to create a named object is recieved.  This should
**	    ultimately become a trace point.
**	05-jul-87 (thurston)
**	    Added code to check for server wide tracing before even attempting
**	    to look for a particular trace flag.
**	28-nov-87 (thurston)
**	    Changed semaphore code to use CS calls instead of going through SCF.
**	20-jan-88 (eric)
**	    Took the guts out and made into qso_mkobj(). This allows 
**	    qso_trans_or_define() to use the same code.
**	27-dec-88 (jrb)
**	    Added code to check return status from qso_mkobj to avoid accvio
**	    when ulm fails in it.
**	28-aug-1992 (rog)
**	    Change for(;;) to do/while(0) to silence compiler warning.
**	30-nov-1992 (rog)
**	    Changes to support master objects and multiple QP's.  Also, set the
**	    owner of the QSF semaphore.  Add new objects to the session's CB so
**	    that orphaned objects can be tracked.
**	06-mar-1996 (nanpr01)
**	    Change the nested if to a switch statement.
**	24-Feb-2005 (schka24)
**	    Handle object being created while mkobj is reclaiming.
*/

DB_STATUS
qso_create( QSF_RCB *qsf_rb )
{
    DB_STATUS		status = E_DB_OK;
    i4			blksz;
    i4           	*err_code = &qsf_rb->qsf_error.err_code;
    QSO_OBJ_HDR		*obj, *nobj;
    QSO_MASTER_HDR	*master;
    QSO_OBID		*objid = &qsf_rb->qsf_obj_id;
    i4			objtyp = qsf_rb->qsf_obj_id.qso_type;
    QSF_CB		*scb = qsf_rb->qsf_scb;
    bool		extra_qp;
    QSO_HASH_BKT	*hash;

    /* Now do some error checking */

    switch (objtyp)
    {
	case QSO_QTEXT_OBJ : 
	  blksz = QSO_BQTEXT_BLKSZ;
	  break;
	case QSO_QTREE_OBJ :
	  blksz = QSO_BQTREE_BLKSZ;
	  break;
	case QSO_QP_OBJ :
	  blksz = QSO_BQP_BLKSZ;
	  break;
	case QSO_SQLDA_OBJ :
	  blksz = QSO_BSQLDA_BLKSZ;
	  break;
	default :
	DBGSTALL(qsf_rb);
	  *err_code = E_QS0009_BAD_OBJTYPE;
	  return(E_DB_ERROR);
    }

    if (objid->qso_lname < 0 || objid->qso_lname > sizeof(objid->qso_name))
    {
	DBGSTALL(qsf_rb);
	*err_code = E_QS000B_BAD_OBJNAME;
	return(E_DB_ERROR);
    }

    *err_code = E_QS0000_OK;
    master = (QSO_MASTER_HDR*)NULL;

    /* Faster-path for unnamed creates */
    if (objid->qso_lname == 0)
    {
	status = qso_mkobj(&obj, objid, blksz, scb,
				&hash, &master, err_code);
	if (DB_SUCCESS_MACRO(status))
	{
	    qsf_rb->qsf_lk_id = obj->qso_lk_id;
	    /* Add this object to the session's private object list. */
	    if (scb->qss_obj_list != (QSO_OBJ_HDR *) NULL)
	    {
		scb->qss_obj_list->qso_obprev = obj;
		obj->qso_obnext = scb->qss_obj_list;
	    }
	    scb->qss_obj_list = obj;
	}
	return (status);
    }
	
    /* Trying to create a named object.  We'll see if we can find it
    ** lying around already.
    ** Outer loop deals with object being found during mkobj.
    */
    extra_qp = FALSE;
    for (;;)
    {
	if ( obj = qso_find(objid, &master, &hash) )
	{
	    /* Found, master's sem is locked */
	    if (master->qsmo_aliasid.qso_lname == 0
		&& master->qsmo_session != scb)
	    {
		/* Non-shareable QP being accessed by a different session */
		/* (If this is even still possible, it's a quel thing */
		DBGSTALL(qsf_rb);
		*err_code = E_QS000A_OBJ_ALREADY_EXISTS;
		status = E_DB_ERROR;
		break;
	    }

	    /* Either this is a shared object, or the session who owns it
	    ** is trying to create another.
	    */
	    for ( obj = master->qsmo_obj_1st; obj; obj = obj->qso_monext )
	    {
		if (obj->qso_root == NULL || obj->qso_lk_state == QSO_EXLOCK)
		{
		    /* If this is a shareable object, and it's root
		    ** has not been set yet, and we are the creator, 
		    ** then return as if the object was created,
		    ** even though the QSO_TRANS_OR_DEFINE operation
		    ** did the create.  What's probably happening here is
		    ** that the real QP for a repeat query or DBP is being
		    ** created by OPF.  Just hand OPF the handle that was
		    ** created at trans-or-define time.
		    */
		    if ( obj->qso_session == scb )
		    {
			status = qso_lkset(obj, QSO_EXLOCK, &qsf_rb->qsf_lk_id,
				       &qsf_rb->qsf_root, scb, err_code);
			break;
		    }
		}
	    }

	    if ( obj )
		break;	/* We found an object in the process of being created.*/

	    /* There's already a QP attached to this master object, so let
	    ** the caller know that he is creating more than one QP for this
	    ** query.
	    */
	    extra_qp = TRUE;

	}

	/* If master found, and we're adding another QP, master is nonnull
	** and we hold the master mutex.  If no master found, we're holding
	** the hash mutex, and master is null.
	** Returns holding the master mutex.
	*/
	status = qso_mkobj(&obj, objid, blksz, scb, 
				&hash, &master, err_code);
	if (status == E_DB_INFO)
	{
	    /* This means that mkobj had to reclaim memory, and the
	    ** object appeared while we were doing it.  Start the whole
	    ** mess over so that we can decide what to do.  (e.g. extra
	    ** qp?  or obj already exists?  or just return it??)
	    */
	    CSv_semaphore(&master->qsmo_sem);
	    continue;
	}
	    
	if (DB_SUCCESS_MACRO(status))
	{
	    qsf_rb->qsf_lk_id = obj->qso_lk_id;

	    if ( master->qsmo_aliasid.qso_lname == 0 )
	    {
		/* Add named nonshareable object to session's private list.
		** (when is such a thing is possible?  quel maybe?)
		*/
		if (scb->qss_obj_list != (QSO_OBJ_HDR *) NULL)
		{
		    scb->qss_obj_list->qso_obprev = obj;
		    obj->qso_obnext = scb->qss_obj_list;
		}
		scb->qss_obj_list = obj;
	    }
	    else if (extra_qp)
	    {
		/* Creating multiple QP's under one master.  Count up the
		** waiter count, just like trans-or-define does for the
		** very first one, because the eventual unlock is going to
		** count the waiter count back down!  Sheesh.
		*/
		++obj->qso_waiters;
	    }
	}
	break;
    };	/* for */

    /* Release master's semaphore */
    if ( master )
	CSv_semaphore(&master->qsmo_sem);

    if (extra_qp)
    {
	DBGSTALL(qsf_rb);
	*err_code = E_QS001C_EXTRA_OBJECT;
	status = E_DB_ERROR;
    }

#ifdef    xDEBUG
    if (    Qsr_scb->qsr_tracing
	&&  (	qst_trcheck(scb, QSF_004_AFT_CREATE_OBJQ)
	     || (   objid->qso_lname
		 && qst_trcheck(scb, QSF_007_AFT_CREATE_NAMED)
		)
	    )
       )
    {
	/* Dump QSF obj queue after creating an object */
	/* ------------------------------------------- */
	DB_STATUS		ignore;
	i4			save_err = *err_code;

	TRdisplay("<<< Dumping QSF object queue after creating object >>>\n");
	ignore = qsd_obq_dump(qsf_rb);

	*err_code = save_err;
    }
#endif /* xDEBUG */

    return (status);
}


/*{
** Name: QSO_DESTROY - Destroy a QSF object.
**
** Description:
**      This is the entry point to the QSF QSO_DESTROY routine. 
**      This opcode is used to destroy a QSF object from QSF's
**      memory space.  The object is specified by its handle (address),
**	so either named or unnamed objects can be destroyed.
**	In either case, though, some sort of lock has to be in place
**	on the object, or an error occurs.  Typically it will be a
**	shared lock for a named object, and it must be an exclusive
**	lock for an unnamed object.
**
**      The memory freed up by this action will go back into QSF's
**      memory pool to be recycled again when another object gets
**      created or needs more memory.  This memory will not get released
**      to SCF again (via ULM), until QSF is terminated for the
**      server.
**
** Inputs:
**      QSO_DESTROY                     Op code to qsf_call().
**      qsf_rb                          Request block.
**              .qsf_obj_id             Object id for the object to be
**                                      destroyed.
**                      .qso_handle     Handle for the object.
**					The remainder of scf_obj_id is
**					irrelevant.
**              .qsf_lk_id              The lock id QSF assigned to this object
**					when it was exclusively locked.
**					(Not needed if it's a named object
**					which is share-locked.)
**
** Outputs:
**      qsf_rb                          Request block.
**              .qsf_error		Filled in with error if encountered.
**                      .err_code	The QSF error that occurred.  One of:
**                              E_QS0000_OK
**				    Operation succeeded.
**                              E_QS000F_BAD_HANDLE
**				    Supplied handle does not refer to an object.
**				E_QS0010_NO_EXLOCK
**				    You don't have exclusive access to object.
**				E_QS0011_ULM_STREAM_NOT_CLOSED
**				    ULM would not close the memory stream for
**				    this session.
**				E_QS001B_NO_SESSION_CB
**				    Could not find the current session.
**				E_QS001D_NOT_OWNER
**				    Session tried destroy an object that it
**				    does not own.
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
**          Other facilities and sessions will no longer be able to
**          find the object that was destroyed.
**
** History:
**      06-mar-86 (thurston)
**          Initial creation.
**	24-jun-86 (thurston)
**	    Modified code to comply with new SCF_CB.  This affected the call
**	    to SCU_WAIT and SCU_RELEASE.
**	28-nov-87 (thurston)
**	    Changed semaphore code to use CS calls instead of going through SCF.
**	20-jan-87 (eric)
**	    Changed to allow callers to destroy objects with only a shared
**	    lock on it. This means support for deferred destroys elsewhere.
**	    This is needed by the QSO_TRANS_OR_DEFINE opcode, since we want
**	    users to wait until shared QP's are finished being built.
**	31-mar-89 (jrb)
**	    Fixed two bugs here: we were allowing objects to be destroyed when
**	    the caller had a mismatching lock id and when the object was not
**	    locked at all.
**	20-dec-89 (ralph)
**	    Don't remove object if we have the only shared lock, but some
**	    other thread is waiting on the QP to be built in qso_just_trans
**	    or qso_trans_or_define.
**	06-jun-90 (andre)
**	    make sure that the object being destroyed is NOT an alias;
**	28-aug-1992 (rog)
**	    Change for(;;) to do/while(0) to silence compiler warning.
**	30-oct-1992 (rog)
**	    Added the owner to the semaphore, and reworked aliases.  Added
**	    support for master objects.
**	03-mar-1994 (rog)
**	    If we mark an object for deferred destroy, then we need to destroy
**	    the object's alias.
**	16-may-1994 (rog)
**	    When destroying a named object, detach it from MO, too.
**	03-dec-1998 (somsa01)
**	    Moved decrementation of qso_wait to BEFORE releaseing the qso_psem
**	    semaphore.
**	22-Feb-2005 (schka24)
**	    Extract named-object destroyer.
**      17-feb-09 (wanfr01) b121543
**          Add QSOBJ_DBP_TYPE as valid qso_type
*/

DB_STATUS
qso_destroy( QSF_RCB *qsf_rb )
{
    QSO_OBJ_HDR         *obj = (QSO_OBJ_HDR *) qsf_rb->qsf_obj_id.qso_handle;
    i4			*err_code = &qsf_rb->qsf_error.err_code;
    DB_STATUS		status;
    QSO_MASTER_HDR	*master;

    /* Now, make sure there is an object to destroy */

    if ( !obj
	||  !((obj->qso_type == QSOBJ_TYPE) ||
	      (obj->qso_type == QSOBJ_DBP_TYPE))
	||  (obj->qso_ascii_id != QSOBJ_ASCII_ID)
	||  (obj->qso_owner != (PTR)DB_QSF_ID)
	||  (obj->qso_length != sizeof(QSO_OBJ_HDR))
       )
    {
	DBGSTALL(qsf_rb);
	*err_code = E_QS000F_BAD_HANDLE;
	return(E_DB_ERROR);
    }

    *err_code = E_QS0000_OK;
    status = E_DB_OK;

    /* If object is named, might be shareable, ask for named object destroy */
    if (obj->qso_obid.qso_lname > 0)
    {
	status = qso_destroy_one(obj, NULL, qsf_rb, err_code);
	return (status);
    }

    /* If object is unnamed, just destroy it, no shenanigans.
    ** Unnamed objects are purely private, can't be waited on by
    ** multiple sessions, etc.
    */

    /* The object is supposed to be locked, but it's no big deal.
    ** Being unnamed, there's no danger in just destroying the object.
    ** (So the retry loop in e.g. ops_qsfdestroy is not really needed, at
    ** least not for named objects.)
    ** Make sure the caller isn't a complete screw-up though.
    */
    if (obj->qso_lk_state == QSO_EXLOCK
      && obj->qso_lk_id != qsf_rb->qsf_lk_id)
    {
	DBGSTALL(qsf_rb);
	*err_code = E_QS0013_BAD_LOCKID;
	return (E_DB_ERROR);
    }

    master = NULL;
    status = qso_rmvobj(&obj, &master, err_code);
    return (status);
}

/*{
** Name: QSO_DESTROYALL - Destroy all multiple QSF objects.
**
** Description:
**      This is the entry point to the QSF QSO_DESTROYALL routine. 
**      This opcode is used to all of the objects attached to a single
**	master object from QSF's memory space.  Unlike QSO_DESTROY, the
**	caller does not need to have a lock on the objects to be destroyed.
**	The object(s) destroyed are *always* named objects.
**	Unlike regular destroy-by-handle, no locks need to exist on any
**	of the objects destroyed.
**
**      The memory freed up by this action will go back into QSF's
**      memory pool to be recycled again when another object gets
**      created or needs more memory.  This memory will not get released
**      to SCF again (via ULM), until QSF is terminated for the
**      server.
**
** Inputs:
**      QSO_DESTROYALL			Op code to qsf_call().
**      qsf_rb                          Request block.
**              .qsf_obj_id             Object id for the object to be
**                                      destroyed.
**			.qso_type	The type of object to look for.
**					Currently only named QSO_QP_OBJ's.
**			.qso_lname	Length in bytes of the object name
**					given in .qso_name.
**			.qso_name	Name for the object.
**
** Outputs:
**      qsf_rb                          Request block.
**              .qsf_error		Filled in with error if encountered.
**                      .err_code	The QSF error that occurred.  One of:
**                              E_QS0000_OK
**				    Operation succeeded.
**				E_QS0009_BAD_OBJTYPE
**				    Illegal object type.
**				E_QS000B_BAD_OBJNAME
**				    Illegal object name.
**				E_QS0011_ULM_STREAM_NOT_CLOSED
**				    ULM would not close the memory stream for
**				    this session.
**				E_QS0019_UNKNOWN_OBJ
**				    No object of this name and type exists.
**				E_QS001B_NO_SESSION_CB
**				    Could not find the current session.
**
** Returns:
**	E_DB_OK
**	E_DB_WARN
**	E_DB_ERROR
**	E_DB_FATAL
**
** Exceptions:
**	None
**
** Side Effects:
**	Other facilities and sessions will no longer be able to find the
**	objects that were destroyed.
**
** History:
**	11-mar-1993 (rog)
**	    Created.
**	03-mar-1994 (rog)
**	    If we mark an object for deferred destroy, then we need to destroy
**	    the object's alias.
**	07-mar-1994 (rog)
**	    If an object isn't free, than we have to do more than just mark
**	    it for deferred destruction.
**	16-may-1994 (rog)
**	    When destroying a named object, detach it from MO, too.
**	03-dec-1998 (somsa01)
**	    Moved decrementation of qso_wait to BEFORE releaseing the qso_psem
**	    semaphore.
**	19-Oct-2000 (jenjo02)
**	    Don't take semaphore until AFTER all error checking
**	    has been done!
**	22-Feb-2005 (schka24)
**	    Revise to call single-named-object destroyer.
*/

DB_STATUS
qso_destroyall( QSF_RCB *qsf_rb )
{
    DB_STATUS		status;
    i4             	*err_code = &qsf_rb->qsf_error.err_code;
    QSO_OBID		*objid = &qsf_rb->qsf_obj_id;
    i4			objtyp = qsf_rb->qsf_obj_id.qso_type;
    QSO_OBJ_HDR         *obj;
    QSO_MASTER_HDR	*master;
    QSO_MASTER_HDR	*alias;
    QSF_CB		*scb = qsf_rb->qsf_scb;

    /* Now do some error checking */

    switch ( objtyp )
    {
	case QSO_QTEXT_OBJ:
	case QSO_QTREE_OBJ:
	case QSO_QP_OBJ:
	case QSO_SQLDA_OBJ:
	    break;
	default:
	    DBGSTALL(qsf_rb);
	    *err_code = E_QS0009_BAD_OBJTYPE;
	    return(E_DB_ERROR);
    }

    if (objid->qso_lname <= 0 || objid->qso_lname > sizeof(objid->qso_name))
    {
	DBGSTALL(qsf_rb);
	*err_code = E_QS000B_BAD_OBJNAME;
	return(E_DB_ERROR);
    }

    master = (QSO_MASTER_HDR*)NULL;
    *err_code = E_QS0000_OK;
    while ( (obj = qso_find(objid, &master, (QSO_HASH_BKT **)NULL)) != NULL)
    {
	/* Got an object, master is locked.
	** See qso_destroy_one for a discussion of concurrency issues.
	*/

	status = qso_destroy_one(obj, &master, qsf_rb, err_code);
	if (DB_FAILURE_MACRO(status))
	    break;

	/* If master was deleted as well, we're done.  Don't keep
	** looking in case of crappy object naming, someone might
	** create a new object of the same name.  (Should NOT happen,
	** but might as well stop now.)
	*/
	if (master == NULL)
	    break;
    }

    return (status);
}

/*
** Name: qso_destroy_one -- Destroy one named object
**
** Description:
**	This routine does the common work for QSO_DESTROY and
**	QSO_DESTROYALL when applied to named objects.  (QSO_DESTROY
**	is a destroy-by-handle, QSO_DESTROYALL is in effect a destroy-
**	by-name.)
**
**	If the object being destroyed is still in use by some other
**	session, or is being waited on by another session, it's
**	marked for deferred deletion and we just exit (after counting
**	off an unlock for ourselves).
**
**	Destroying an object requires care if races are to be avoided.
**	The objects we're dealing with here are (typically) shareable,
**	and so multiple sessions might try to delete the same object at
**	the same time.  The interesting cases are:
**
**	1) two sessions both using a shared object, both decide to
**	destroy it using DESTROY.
**	This situation is detected because before an object can be
**	destroyed (by handle), it has to be share-locked.  If some
**	object is locked by multiple sessions, or if there are sessions
**	still waiting for the object to be completed, we just mark the
**	object for deferred destroy.  The last session looking at the object
**	does the actual remove.
**
**	2) Session A decides to DESTROY an object, session B is in the
**	middle of memory reclamation (clrmem).
**	This actually isn't a problem, because clrmem refuses to consider an
**	object that is locked (or has waiters).  As long as we're careful
**	to not make the object look unlocked until we set the LRU-destroy
**	flag, we're ok.
**
**	3) Sessions A and B are both doing clrmem and they manage to consider
**	the same object simultaneously.
**	Won't happen, because clrmem makes its choice while holding the QSF
**	mutex.  When it picks an object, it turns on an LRU-destroy flag
**	that prevents the other session from picking the object.
**
**	4) Session A is doing a DESTROYALL while session B is in clrmem.
**	This one is a bit tricky.  DESTROYALL basically does "lookup by
**	object ID, then delete" repeatedly until the object ID isn't found
**	any more.  We have to plug races at several points.
**	First, two mechanisms prevent races between qso_find and clrmem.
**	When qso_find considers a master, it mutexes, it, and verifies
**	its type.  Object removal mutexes the master and clears its type
**	before it unmutexes it.  qso_find also increments a hash bucket
**	indicator while it's searching.  Object removal can't actually
**	delete the master until the indicator is cleared.  Taken together,
**	find is prevented from finding an object undergoing removal, and
**	remove is prevented from completing the removal until we're sure
**	that nobody is in the middle of trying to find it.
**
**	Second, once DESTROYALL chooses an object, it sets and checks the
**	same LRU-destroy flag that clrmem uses.  If B has already picked
**	the object for deletion, A won't delete it also.  (And if B is
**	sufficiently far along with deletion, A won't find the object at
**	all, viz. the prior paragraph.)
**
**	One other QSF strangeness to handle here is the case of a QP
**	object that was created with QSO_TRANS_OR_DEFINE, but the
**	definition was never completed.  To prevent premature culling,
**	trans-or-define marks the object as waiting, in a funny unlocked
**	state.  We'll recognize that state and do a sort of pseudo-unlock,
**	so that we don't wait for some nonexistent other session to do
**	the destroy.
**
** Inputs:
**	obj			QSO_OBJ_HDR object to be destroyed
**	master_p		QSO_MASTER_HDR ** passed as NULL if caller is
**				destroy-by-handle.  If non-null, caller
**				is DESTROYALL, and the master is mutexed.
**	qsf_rb			QSF_RCB original destroy request block
**	err_code		Pointer to error code (caller presets to OK)
**
** Outputs:
**	Returns usual DB_STATUS.
**	master_p		Updated to NULL if we removed the master;
**				if master still there, is unmutexed.
**	err_code		May be updated if there's a screw-up
**
** Side Effects:
**
** History:
**	22-Feb-2005 (schka24)
**	    Extract from destroy/destroyall;  more delete-race squashing.
*/

static DB_STATUS
qso_destroy_one(QSO_OBJ_HDR *obj, QSO_MASTER_HDR **master_p,
	QSF_RCB *qsf_rb, i4 *err_code)
{
    DB_STATUS status;
    QSO_MASTER_HDR *master;	/* Master index header */

    master = NULL;
    if (master_p != NULL) master = *master_p;

    status = E_DB_OK;
    do	/* Something to break out of */
    {
	/* Mutex the master if we didn't have it passed in (the
	** destroy-by-handle case).
	*/
	if (master_p == NULL)
	{
	    master = obj->qso_mobject;
	    CSp_semaphore(TRUE, &master->qsmo_sem);
	}

	/* If this is a shared QP that we created, and it's not finished,
	** trans-or-define marks the object waiting with the excl-owner
	** == us.  Apparently we've decided to destroy the object before
	** finishing it, so adjust the waiter count and wake up anyone
	** else who might have noticed that the object is there.
	*/

	if (obj->qso_obid.qso_type == QSO_QP_OBJ
	    && master->qsmo_aliasid.qso_lname > 0
	    && (obj->qso_lk_state == QSO_EXLOCK || obj->qso_root == NULL))
	{
	    if (obj->qso_excl_owner != qsf_rb->qsf_scb)
	    {
		/* A session is still creating this shareable object,
		** and it is not the one trying to destroy it.
		*/
		DBGSTALL(qsf_rb);
		*err_code = E_QS001D_NOT_OWNER;
		status = E_DB_ERROR;
		break;
	    }

	    obj->qso_waiters--;
	    obj->qso_excl_owner = (QSF_CB *) NULL;
	}

	/* Wake up any other waiters on this object.  They'll
	** see the deferred-delete flag that we're going to set
	** (before unmutexing the master), and one of them will
	** end up actually doing the delete.
	*/
	if ( obj->qso_waiters )
	    CScnd_broadcast(&master->qsmo_cond);

	/* If other people might be using the object, we can't delete
	** it just now.  Mark it so that nobody finds it, and let the
	** last person using the object destroy it.
	*/

	if ((obj->qso_lk_state == QSO_SHLOCK
	    && (obj->qso_lk_cur > 1 || obj->qso_waiters)
	    )
	    || (obj->qso_lk_state == QSO_EXLOCK
		&& obj->qso_lk_id == qsf_rb->qsf_lk_id
		&& obj->qso_waiters
	       )
	   )
	{
	    obj->qso_status |= QSO_DEFERRED_DESTROY;
	    obj->qso_lk_cur--;
	}
	else
	{
	    /* We're really going to get rid of the object.
	    ** If this is destroy-by-handle, the object has to be
	    ** locked -- make sure.
	    */
	    if (master_p == NULL)
	    {
		if (obj->qso_lk_state == QSO_EXLOCK
		    && obj->qso_lk_id != qsf_rb->qsf_lk_id
		   )
		{
		    DBGSTALL(qsf_rb);
		    *err_code = E_QS0013_BAD_LOCKID;
		    status = E_DB_ERROR;
		    break;
		}

		if (obj->qso_lk_state == QSO_FREE )
		{
		    DBGSTALL(qsf_rb);
		    *err_code = E_QS0012_OBJ_NOT_LOCKED;
		    status = E_DB_ERROR;
		    break;
		}
	    }

	    /* Make sure that this object hasn't just been selected
	    ** for LRU delete by some other session, and make sure that LRU
	    ** delete doesn't pick this object while we're smashing it.
	    ** (LRU delete will typically not choose our object because it
	    ** is usually locked, but not necessarily -- if destroyall.)
	    */
	    CSp_semaphore(TRUE, &Qsr_scb->qsr_psem);
	    if (obj->qso_status & QSO_LRU_DESTROY)
	    {
		/* Someone is doing a clrmem, they picked this object */
		CSv_semaphore(&Qsr_scb->qsr_psem);
		if (master != NULL)
		    CSv_semaphore(&master->qsmo_sem);
		return (E_DB_OK);
	    }
	    /* Don't let clrmem pick this object. */
	    obj->qso_status |= QSO_LRU_DESTROY;
	    CSv_semaphore(&Qsr_scb->qsr_psem);

	    /* Note that this call can remove the master as well */
	    status = qso_rmvobj(&obj, &master, err_code);
	}
    } while (0);

    /* If the master is still around, un-mutex it. */
    if ( master )
	CSv_semaphore(&master->qsmo_sem);

    /* If destroy-by-name, tell caller whether master's still around */
    if (master_p != NULL)
	*master_p = master;

    return(status);

} /* qso_destroy_one */

/*{
** Name: QSO_LOCK - Lock a QSF object exclusively or shared.
**
** Description:
**      This is the entry point to the QSF QSO_LOCK routine. 
**      This opcode is used to lock a QSF object for either exclusive or
**	shared access.  If the object is to be exclusively locked, QSF will
**	assign, and hand back to the caller, a lock id that will be required
**      whenever a QSO_PALLOC, QSO_SETROOT, QSO_UNLOCK, etc. is used.
**
**	This routine also returns the root address of the locked object to the
**	caller so they can find their way about the object.
**
**	While you might imagine from the name that QSO_LOCK waits for
**	the appropriate access to the object, you would be mistaken.
**	QSO_LOCK simply sets the desired lock state into the object
**	header.  If the object is not in a state in which the lock
**	request can be satisfied, an error is returned.
**
**	Named, shareable QP objects may not be exclusively locked.
**	In fact, using QSO_LOCK for a shareable object is probably a
**	bad idea, unless the object is already marked as non-grabbable in
**	some other manner.  The proper way to get a shareable object handle
**	is with QSO_GETHANDLE or _GETNEXT, both of which have the
**	ability to lock the object as it's fetched.
**
** Inputs:
**      QSO_LOCK                        Op code to qsf_call().
**      qsf_rb                          Request block.
**		.qsf_obj_id		Object id.
**			.qso_handle	Handle for the object to lock.
**		.qsf_lk_state		Type of lock being asked for.  One of:
**					    QSO_SHLOCK - Lock for sharing.
**					    QSO_EXLOCK - Lock exclusively.
**
** Outputs:
**      qsf_rb
**		.qsf_root 		The root address for the locked object.
**		.qsf_lk_id		If object is being locked for shared
**                                      access, this will not be touched.  If
**					locking exclusively, this will be set to
**					the lock id QSF assigns to the object.
**					This will be required whenever any form
**                                      of object modification is done through
**                                      the QSF routines (e.g. QSO_PALLOC,
**                                      QSO_SETROOT, etc.).
**		.qsf_error		Filled in with error if encountered.
**			.err_code	The QSF error that occurred.  One of:
**				E_QS0000_OK
**				    Operation succeeded.
**                              E_QS000D_CORRUPT
**				    QSF's object store has been corrupted.
**                              E_QS000F_BAD_HANDLE
**				    Supplied handle does not refer to an object.
**                              E_QS0014_EXLOCK
**                                  Object is already locked exclusively.
**                              E_QS0015_SHLOCK
**                                  Object is already locked shared.
**                              E_QS0016_BAD_LOCKSTATE
**                                  Illegal lock state; must be QSO_EXLOCK or
**                                  QSO_SHLOCK.
**				E_QS001B_NO_SESSION_CB
**				    Could not find the current session.
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
**          If locking an exclusively locked object, the object will be
**          taken out of the QSO_FREE state and placed in the QSO_EXLOCK state
**          so that no other clients will be able to get either shared or
**          exclusive locks on the object.  If locking an object for shared
**          access, the number of share locks on that object will be
**          incremented.  If this number had been zero, the object will be taken
**          out of the QSO_FREE state and placed in the QSO_SHLOCK state,
**          meaning that other clients can get share locks on the object, but
**          cannot get an exclusive lock. 
**
** History:
**	06-mar-86 (thurston)
**          Initial creation.
**	24-jun-86 (thurston)
**	    Modified code to comply with new SCF_CB.  This affected the call
**	    to SCU_WAIT and SCU_RELEASE.
**	05-aug-87 (thurston)
**	    Modified to use new low level object locker.
**	28-nov-87 (thurston)
**	    Changed semaphore code to use CS calls instead of going through SCF.
**	28-jan-88 (eric)
**	    Added check to make sure that no one locks QP exclusivly. This
**	    is because QPs can now be shared, so locking QPs exclusivly is
**	    dangerous and, because of changes to QSO_DESTROY, unnessary.
**	19-feb-88 (thurston)
**	    Check for EXLOCKs on a QP has been modified to be an EXLOCK on a
**	    "sharable" QP.
**	28-aug-1992 (rog)
**	    Defensive programming: make sure object contains something before
**	    accessing through it.
**	30-oct-1992 (rog)
**	    Added the owner to the semaphore.
**      17-feb-09 (wanfr01) b121543
**          Add QSOBJ_DBP_TYPE as valid qso_type
**	22-oct-09 (toumi01)
**	    Wrap the "QSO_LOCK of shareable named object" debug warning
**	    in xDEBUG to avoid lots of dbms log noise running with
**	    cache_dynamic.
*/

DB_STATUS
qso_lock( QSF_RCB *qsf_rb )
{
    DB_STATUS		status = E_DB_OK;
    i4			*err_code = &qsf_rb->qsf_error.err_code;
    QSO_OBJ_HDR         *obj = (QSO_OBJ_HDR *) qsf_rb->qsf_obj_id.qso_handle;
    QSO_MASTER_HDR	*master;

    /* Sanity check the object */

    if (!obj
        ||  !((obj->qso_type == QSOBJ_TYPE) ||
              (obj->qso_type == QSOBJ_DBP_TYPE))
	|| (obj->qso_ascii_id != QSOBJ_ASCII_ID)
	|| (obj->qso_owner != (PTR)DB_QSF_ID)
	|| (obj->qso_length != sizeof(QSO_OBJ_HDR))
       )
    {
	DBGSTALL(qsf_rb);
	*err_code = E_QS000F_BAD_HANDLE;
	return(E_DB_ERROR);
    }

    if ( master = obj->qso_mobject )
    {
	CSp_semaphore(TRUE, &master->qsmo_sem);
	if (master->qsmo_aliasid.qso_lname > 0)
	{
#ifdef xDEBUG
	    TRdisplay("%@ QSO_LOCK of shareable named object %p (master %p) in state %d, lock req %d\n",
		obj, master, obj->qso_lk_state, qsf_rb->qsf_lk_state);
#endif

	    /* Not allowed to EXLOCK a shareable object via QSO_LOCK */
	    if (qsf_rb->qsf_lk_state == QSO_EXLOCK)
	    {
		DBGSTALL(qsf_rb);
		CSv_semaphore(&master->qsmo_sem);
		*err_code = E_QS0016_BAD_LOCKSTATE;
		return (E_DB_ERROR);
	    }
	}
    }

    status = qso_lkset(	obj,
			qsf_rb->qsf_lk_state,
			&qsf_rb->qsf_lk_id,
			&qsf_rb->qsf_root,
			qsf_rb->qsf_scb,
			err_code
		      );

    if ( master )
	CSv_semaphore(&master->qsmo_sem);

    return (status);
}


/*{
** Name: QSO_UNLOCK - Unlock a QSF object.
**
** Description:
**      This is the entry point to the QSF QSO_UNLOCK routine. 
**      This opcode is used to unlock a QSF object that has either a
**	shared lock, or an exclusive lock on it.  If the object was locked
**	exclusively, the caller is required to supply the lock id that was
**      assigned at QSO_LOCK or QSO_CREATE time.
**
**	If the object is shareable, and there are other sessions waiting
**	for the object (waiting for QP construction to complete),
**	those sessions are waked up.  It may also be that the object
**	has been marked for destruction;  if we are the last session
**	locking such an object, we'll destroy it.
**
** Inputs:
**      QSO_UNLOCK                      Op code to qsf_call().
**      qsf_rb                          Request block.
**		.qsf_obj_id		Object id.
**			.qso_handle	Handle for the object to unlock.
**		.qsf_lk_id		If object is locked for shared access,
**					this will be ignored.  If locked
**					exclusively, this is the lock id that
**					QSF assigned when the object was locked.
**
** Outputs:
**      qsf_rb
**		.qsf_error		Filled in with error if encountered.
**			.err_code	The QSF error that occurred.  One of:
**				E_QS0000_OK
**				    Operation succeeded.
**                              E_QS000D_CORRUPT
**				    QSF's object store has been corrupted.
**                              E_QS000F_BAD_HANDLE
**				    Supplied handle does not refer to an object.
**                              E_QS0012_OBJ_NOT_LOCKED
**                                  Object was not locked.
**                              E_QS0013_BAD_LOCKID
**				    The lock id you supplied does not match
**				    the one assigned to the object when it was
**				    exclusively locked.
**				E_QS001B_NO_SESSION_CB
**				    Could not find the current session.
**				E_QS001D_NOT_OWNER
**				    A session tried to unlock an object that
**				    it does not hold own.
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
**          If unlocking an exclusively locked object, the object will be
**          taken out of the QSO_EXLOCK state and placed in the QSO_FREE state
**          so that other clients will be able to get shared or exclusive locks
**          on the object.  If unlocking a share locked object, the number of
**          share locks on that object will be decremented.  If this number
**          reaches zero, the object will be taken out of the QSO_SHLOCK state
**          and placed in the QSO_FREE state. Also, If the last lock is being
**	    taken off of an object that is mark as deferred destroy, then
**	    we will destroy it at this time.
**
** History:
**	06-mar-86 (thurston)
**          Initial creation.
**	24-jun-86 (thurston)
**	    Modified code to comply with new SCF_CB.  This affected the call
**	    to SCU_WAIT and SCU_RELEASE.
**	28-nov-87 (thurston)
**	    Changed semaphore code to use CS calls instead of going through SCF.
**	22-jan-87 (eric)
**	    Added code to destroy deferred destroy objects if there are no more
**	    locks on them.
**	28-aug-1992 (rog)
**	    Change for(;;) to do/while(0) to silence compiler warning.
**	30-oct-1992 (rog)
**	    Added the owner to the semaphore, and support for master objects.
**	30-nov-1998 (somsa01)
**	    Moved decrementation of qso_wait to AFTER the check of
**	    qso_psem_owner and qso_psem.
**	03-dec-1998 (somsa01)
**	    Moved decrementation of qso_wait to BEFORE releaseing the qso_psem
**	    semaphore.
**	24-Feb-2005 (schak24)
**	    Don't pick up variables in declaration section, have to wait
**	    until after we mutex the master!
**	5-nov-2007 (dougi)
**	    Decrement wait count for QTEXT, too. For cached dynamic queries.
**      17-feb-09 (wanfr01) b121543
**          Add QSOBJ_DBP_TYPE as valid qso_type
*/

DB_STATUS
qso_unlock( QSF_RCB *qsf_rb )
{
    DB_STATUS		status;
    i4             	*err_code = &qsf_rb->qsf_error.err_code;
    QSO_OBJ_HDR         *obj = (QSO_OBJ_HDR *) qsf_rb->qsf_obj_id.qso_handle;
    i4			*lk_state;
    i4			*lk_cur;
    i4			*lk_id;
    QSO_MASTER_HDR	*master;

    /* Now do some error checking */

    if (!obj
        ||  !((obj->qso_type == QSOBJ_TYPE) ||
              (obj->qso_type == QSOBJ_DBP_TYPE))
	|| (obj->qso_ascii_id != QSOBJ_ASCII_ID)
	|| (obj->qso_owner != (PTR)DB_QSF_ID)
	|| (obj->qso_length != sizeof(QSO_OBJ_HDR))
       )
    {
	DBGSTALL(qsf_rb);
	*err_code = E_QS000F_BAD_HANDLE;
	return(E_DB_ERROR);
    }

    *err_code = E_QS0000_OK;

    /* If master, lock its sem */
    if ( master = obj->qso_mobject )
	CSp_semaphore(TRUE, &master->qsmo_sem);

    lk_state = &obj->qso_lk_state;
    lk_cur = &obj->qso_lk_cur;
    lk_id = &obj->qso_lk_id;
    switch (*lk_state)
    {
	case QSO_FREE:
	    DBGSTALL(qsf_rb);
	    if (*lk_cur != 0)
		*err_code = E_QS000D_CORRUPT;
	    else
		*err_code = E_QS0012_OBJ_NOT_LOCKED;
	    break;
	case QSO_EXLOCK:
	    if (*lk_cur != 1)
	    {
		DBGSTALL(qsf_rb);
		*err_code = E_QS000D_CORRUPT;
	    }
	    else
	    {
		if (qsf_rb->qsf_lk_id != *lk_id)
		{
		    DBGSTALL(qsf_rb);
		    *err_code = E_QS0013_BAD_LOCKID;
		}
		else
		{
		    /* All OK ... unlock the exclusively locked object */
		    /* ----------------------------------------------- */

		    if ( (obj->qso_obid.qso_type == QSO_QP_OBJ
			 || obj->qso_obid.qso_type == QSO_QTEXT_OBJ)
			 && master
			 && master->qsmo_aliasid.qso_lname > 0
		       )
		    {

			/* A shareable object will have an EXCL lock
			** if it was created by QSO_TRANS_OR_DEFINE, and
			** then "re-created" by QSO_CREATE.  The trans-
			** or-define operation bumps up the waiters count
			** to hide the object from the memory reclaimer.
			** Now's the time to fix that.
			*/
			if (obj->qso_excl_owner != qsf_rb->qsf_scb)
			{
			    /* A session has an EXCL lock on this
			    ** object, and it is not the one trying to
			    ** unlock it.
			    */
			    DBGSTALL(qsf_rb);
			    *err_code = E_QS001D_NOT_OWNER;
			    break;
			}

			obj->qso_waiters--;
			obj->qso_excl_owner = (QSF_CB *) NULL;
		    }

		    *lk_cur = 0;
		    *lk_state = QSO_FREE;
		    *lk_id = 0;
		}
	    }
	    break;
	case QSO_SHLOCK:
	    if (*lk_cur < 1)
		*err_code = E_QS000D_CORRUPT;
	    else
	    {
		/* All OK ... unlock the share locked object */
		/* ----------------------------------------- */

		if (--(*lk_cur) == 0)
		    *lk_state = QSO_FREE;
	    }
	    break;
	default:
	    DBGSTALL(qsf_rb);
	    *err_code = E_QS000D_CORRUPT;
	    break;
    }

    /* Wake up anyone waiting on this object */
    if ( master && obj->qso_waiters )
	CScnd_broadcast(&master->qsmo_cond);

    if ( *err_code == E_QS0000_OK &&
	    obj->qso_status & QSO_DEFERRED_DESTROY &&
	    obj->qso_lk_cur == 0 &&
	    obj->qso_waiters == 0
	)
    {
	/* if the object is marked to be destroyed later and there are no
	** more locks on it, then lets destroy it.
	*/
	status = qso_rmvobj(&obj, &master, err_code);
    }

    /* If master remains, unmutex it */
    if ( master )
	CSv_semaphore(&master->qsmo_sem);

    return ( (*err_code) ? E_DB_ERROR : E_DB_OK );
}


/*{
** Name: QSO_SETROOT - Set the root address of a QSF object.
**
** Description:
**      This is the entry point to the QSF QSO_SETROOT routine. 
**      This opcode is used to assign an address as the "root" address
**	for a QSF object.  This address should be a pointer to some form
**	of root node, or other structure, from which, one can traverse
**	the entire object. In other words, this "root" is the "entrance"
**	to all memory "tagged" by the object id.
**
**	The root of an object will be given out to callers by the
**	QSO_INFO opcode, which is how different facilities and
**	sessions will be able to share QSF objects.
**
**      This opcode will only be legal while the object is exclusively locked,
**	and the caller can supply the proper lock id.
**
**	Note that QSF itself doesn't care much about the object root,
**	although it does use the lack of a root to mean that the object
**	is not yet completely constructed (and therefore can be considered
**	to be exclusively locked -- even if it isn't!)
**
** Inputs:
**      QSO_SETROOT                     Op code to qsf_call().
**      qsf_rb                          Request block.
**		.qsf_obj_id		Object id.
**			.qso_handle	The object's handle.
**		.qsf_root		The address to set as the root
**					address to this object.
**		.qsf_lk_id		The lock id QSF assigned to this object
**					when you locked it exclusively.
**
** Outputs:
**      qsf_rb                          Request block.
**		.qsf_error		Filled in with error if encountered.
**			.err_code	The QSF error that occurred.  One of:
**                              E_QS0000_OK
**				    Operation succeeded.
**                              E_QS000F_BAD_HANDLE
**				    Supplied handle does not refer to an object.
**				E_QS0010_NO_EXLOCK
**				    You don't have exclusive access to object.
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
**	    All other facilities and sessions requesting information about this
**          object will now be given the new root address.
**
** History:
**	06-mar-86 (thurston)
**          Initial creation.
**	02-jun-86 (thurston)
**	    Corrected bug where routine always returned E_DB_ERROR even when
**	    there was no error.
**	24-jun-86 (thurston)
**	    Modified code to comply with new SCF_CB.  This affected the call
**	    to SCU_WAIT and SCU_RELEASE.
**	28-nov-87 (thurston)
**	    Changed semaphore code to use CS calls instead of going through SCF.
**	28-aug-1992 (rog)
**	    Defensive programming: make sure object contains something before
**	    accessing through it.
**	30-oct-1992 (rog)
**	    Removed aliases from this function.
**      17-feb-09 (wanfr01) b121543
**          Add QSOBJ_DBP_TYPE as valid qso_type
*/

DB_STATUS
qso_setroot( QSF_RCB *qsf_rb )
{
    QSO_OBJ_HDR         *obj = (QSO_OBJ_HDR *) qsf_rb->qsf_obj_id.qso_handle;
    i4			*err_code = &qsf_rb->qsf_error.err_code;


    /* First, make sure there is an object at the given handle */

    if (!obj
        ||  !((obj->qso_type == QSOBJ_TYPE) ||
              (obj->qso_type == QSOBJ_DBP_TYPE))
	|| (obj->qso_ascii_id != QSOBJ_ASCII_ID)
	|| (obj->qso_owner != (PTR)DB_QSF_ID)
	|| (obj->qso_length != sizeof(QSO_OBJ_HDR))
       )
    {
	DBGSTALL(qsf_rb);
	*err_code = E_QS000F_BAD_HANDLE;
	return (E_DB_ERROR);
    }


    /* Make sure object is locked by caller, then set its root */

    if (obj->qso_lk_state != QSO_EXLOCK || obj->qso_lk_id != qsf_rb->qsf_lk_id)
    {
	DBGSTALL(qsf_rb);
	*err_code = E_QS0010_NO_EXLOCK;
	return (E_DB_ERROR);
    }

    obj->qso_root = qsf_rb->qsf_root;
    *err_code = E_QS0000_OK;
    return (E_DB_OK);
}


/*{
** Name: QSO_INFO - Get information about a QSF object.
**
** Description:
**      This is the entry point to the QSF QSO_INFO routine. This opcode is used
**      to retrieve various information kept internally by QSF for the specified
**      object.  The information includes the object's name (if it has one) and
**      type, the "root" address of the object, the object's current lock state,
**      and the number of current locks on the object. 
**
**	Note, that if the object's status is QSO_EXLOCK, and the caller is not
**	the one who has the exclusive lock on the object, the object could be
**	in the process of being modified by someone else.  Therefore, in this
**	case, the information returned by this routine may not be accurate.  If
**	the caller requires absolutely accurate information about the object
**	that caller should first get a read lock (shared lock) on the object by
**	using the QSO_LOCK opcode with a lock state of QSO_SHLOCK.
**
** Inputs:
**      QSO_INFO                        Op code to qsf_call().
**      qsf_rb                          Request block.
**		.qsf_obj_id		Object id.
**			.qso_handle	The object's handle.
**
** Outputs:
**      qsf_rb                          Request block.
**		.qsf_obj_id		Object id.
**			.qso_type	The type of object this is;
**					Guaranteed to be one of the following:
**					    QSO_QTEXT_OBJ - query text
**					    QSO_QTREE_OBJ - query tree
**					    QSO_QP_OBJ    - query plan
**					    QSO_SQLDA_OBJ - SQLDA used for SQL's
**							    `describe' stmt.
**			.qso_lname	Length in bytes of the object name
**					given in .qso_name.  If zero, this
**					is an un-named object.
**			.qso_name	Name of the object, if it has one.
**		.qsf_root		The root address of this object.
**					This will be NULL if a root has
**					not yet been set.
**		.qsf_lk_state           Current lock state of this object.
**					This will be one of the following:
**					    QSO_FREE   - Not locked.
**					    QSO_SHLOCK - Locked for sharing.
**					    QSO_EXLOCK - Exclusively locked.
**		.qsf_lk_cur             Current number of locks on this object.
**		.qsf_error		Filled in with error if encountered.
**			.err_code	The QSF error that occurred.  One of:
**                              E_QS0000_OK
**				    Operation succeeded.
**                              E_QS000D_CORRUPT
**				    QSF's object store has been corrupted.
**                              E_QS000F_BAD_HANDLE
**				    Supplied handle does not refer to an object.
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
**	    none
**
** History:
**	06-mar-86 (thurston)
**          Initial creation.
**	02-mar-87 (thurston)
**	    Added support for new type of QSF object: SQLDA.
**	20-nov-91 (rog)
**	    Make sure obj isn't null before accessing through it.
**	30-oct-1992 (rog)
**	    Removed aliases from this function.
**      17-feb-09 (wanfr01) b121543
**          Add QSOBJ_DBP_TYPE as valid qso_type
*/

DB_STATUS
qso_info( QSF_RCB *qsf_rb )
{
    QSO_OBJ_HDR         *obj = (QSO_OBJ_HDR *) qsf_rb->qsf_obj_id.qso_handle;
    i4		*err_code = &qsf_rb->qsf_error.err_code;


    /* First, make sure there is an object at the given handle */

    if (!obj
        ||  !((obj->qso_type == QSOBJ_TYPE) ||
              (obj->qso_type == QSOBJ_DBP_TYPE))
	|| (obj->qso_ascii_id != QSOBJ_ASCII_ID)
	|| (obj->qso_owner != (PTR)DB_QSF_ID)
	|| (obj->qso_length != sizeof(QSO_OBJ_HDR))
       )
    {
	DBGSTALL(qsf_rb);
	*err_code = E_QS000F_BAD_HANDLE;
	return (E_DB_ERROR);
    }

    /* Check to make sure object handle is the object's address */

    if (qsf_rb->qsf_obj_id.qso_handle != obj->qso_obid.qso_handle)
    {
	DBGSTALL(qsf_rb);
	*err_code = E_QS000D_CORRUPT;
	return (E_DB_ERROR);
    }


    /* Now give back all of the info about the object */

    STRUCT_ASSIGN_MACRO(obj->qso_obid, qsf_rb->qsf_obj_id);
    qsf_rb->qsf_root = obj->qso_root;
    qsf_rb->qsf_lk_state = obj->qso_lk_state;
    qsf_rb->qsf_lk_cur = obj->qso_lk_cur;


    *err_code = E_QS0000_OK;
    return (E_DB_OK);
}


/*{
** Name: QSO_PALLOC - Allocate memory pieces for a QSF object.
**
** Description:
**      This is the entry point to the QSF QSO_PALLOC routine. 
**      This opcode is used to allocate pieces of contiguous memory
**	for a QSF object in QSF's memory space to be shared by other
**	facilities, and shared between sessions.  All pieces allocated
**	to the object will be "tagged" (internal to QSF) with the
**	object id.
**
**      This opcode will only be legal while the object is exclusively locked,
**	and the caller can supply the proper lock id.
**
**	Each allocated piece will be guaranteed to be contiguous and
**	aligned on a sizeof(ALIGN_RESTRICT) byte boundary.
**
**	
** Inputs:
**      QSO_PALLOC                      Op code to qsf_call().
**      qsf_rb                          Request block.
**		.qsf_obj_id		Object id.
**			.qso_handle	The object's handle.
**		.qsf_lk_id              The lock id QSF assigned to the object
**                                      when you locked it exclusively.
**		.qsf_sz_piece		The number of contiguous bytes
**					to allocate.
**
** Outputs:
**      qsf_rb                          Request block.
**		.qsf_piece		Ptr to the piece allocated.  This
**                                      piece is guaranteed to be as large as
**                                      requested (if no error), contiguous, and
**                                      aligned on a sizeof(ALIGN_RESTRICT) byte
**                                      boundary. 
**		.qsf_error		Filled in with error if encountered.
**			.err_code	The QSF error that occurred.  One of:
**                              E_QS0000_OK
**				    Operation succeeded.
**				E_QS0001_NOMEM
**				    Out of memory.
**				E_QS000C_ULM_ERROR
**				    An error occurred in the ULM when
**				    attempting to allocate a piece from
**				    the object's memory stream.
**                              E_QS000F_BAD_HANDLE
**				    Supplied handle does not refer to an object.
**				E_QS0010_NO_EXLOCK
**				    You don't have exclusive access to object.
**				E_QS0018_BAD_PIECE_SIZE
**				    The requested piece size is < 1.
**				E_QS001B_NO_SESSION_CB
**				    Could not find the current session.
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
**	    none
**
** History:
**	06-mar-86 (thurston)
**          Initial creation.
**	28-aug-1992 (rog)
**	    Defensive programming: make sure object contains something before
**	    accessing through it.
**	30-oct-1992 (rog)
**	    Removed aliases, added the owner to the semaphore, and added 
**	    accounting statistics.  See the note below about the reliability
**	    of the statistics during highly concurrent activity.
**	28-mar-1994 (rog)
**	    Increment the total size of the object.
**	23-Feb-2005 (schka24)
**	    Simplify bass-ackwards if-then-else style.
**      17-feb-2009 (wanfr01) b121543
**          Add QSOBJ_DBP_TYPE as valid qso_type
**	    Add Object to approrpiate IMA instance list via parameter
*/

DB_STATUS
qso_palloc( QSF_RCB *qsf_rb )
{
    QSO_OBJ_HDR         *obj = (QSO_OBJ_HDR *) qsf_rb->qsf_obj_id.qso_handle;
    i4			*err_code = &qsf_rb->qsf_error.err_code;
    ULM_RCB		ulm_rcb;
    QSF_CB		*scb = qsf_rb->qsf_scb;


    /* First, make sure there is an object at the given handle */

    if (!obj
        ||  !((obj->qso_type == QSOBJ_TYPE) ||
              (obj->qso_type == QSOBJ_DBP_TYPE))
	|| (obj->qso_ascii_id != QSOBJ_ASCII_ID)
	|| (obj->qso_owner != (PTR)DB_QSF_ID)
	|| (obj->qso_length != sizeof(QSO_OBJ_HDR))
       )
    {
	DBGSTALL(qsf_rb);
	*err_code = E_QS000F_BAD_HANDLE;
	return (E_DB_ERROR);
    }

    /* Check for illegal piece size */

    if (qsf_rb->qsf_sz_piece < 1)
    {
	DBGSTALL(qsf_rb);
	*err_code = E_QS0018_BAD_PIECE_SIZE;
	return (E_DB_ERROR);
    }

    *err_code = E_QS0000_OK;

    /* Make sure object is locked by caller, then allocate the piece */

    if (obj->qso_lk_state != QSO_EXLOCK || obj->qso_lk_id != qsf_rb->qsf_lk_id)
    {
	DBGSTALL(qsf_rb);
	*err_code = E_QS0010_NO_EXLOCK;
	return (E_DB_ERROR);
    }
    ulm_rcb.ulm_facility = DB_QSF_ID;
    ulm_rcb.ulm_streamid_p = &obj->qso_streamid;
    ulm_rcb.ulm_psize = qsf_rb->qsf_sz_piece;
    ulm_rcb.ulm_memleft = &Qsr_scb->qsr_memleft;

    while ( ulm_palloc(&ulm_rcb) != E_DB_OK )
    {
	/* We hold no semaphores, so simply retry 
	** after qsf_clrmem()
	*/
	if (ulm_rcb.ulm_error.err_code != E_UL0005_NOMEM)
	{
	    DBGSTALL(qsf_rb);
	    *err_code = E_QS000C_ULM_ERROR;
	    return (E_DB_ERROR);
	}
	/* try to clear the obj queue */
	if ( !qsf_clrmem(scb) )
	{
	    /* Nothing left to clear */
	    DBGSTALL(qsf_rb);
	    *err_code = E_QS0001_NOMEM;
	    return (E_DB_ERROR);
	}
    }

    qsf_rb->qsf_piece = ulm_rcb.ulm_pptr;
    obj->qso_size += qsf_rb->qsf_sz_piece;

    /* Save the largest size obtained */
    if ( qsf_rb->qsf_sz_piece > Qsr_scb->qsr_mx_size )
	Qsr_scb->qsr_mx_size = qsf_rb->qsf_sz_piece;

    if (qsf_rb->qsf_sz_piece > Qsr_scb->qsr_mx_rsize)
	Qsr_scb->qsr_mx_rsize = qsf_rb->qsf_sz_piece;

    return ( E_DB_OK );
}


/*{
** Name: QSO_GETHANDLE - Retrieve the handle for a named QSF object.
**
** Description:
**      This is the entry point to the QSF QSO_GETHANDLE routine. 
**      This opcode is used to retrieve the handle for a named QSF object
**	when all you know is that object's name.  The handle is the method of
**	telling all of the other QSF routines what object you are referring to.
**
**	In addition to retrieving the object handle, we'll lock the
**	object according to the lock type indicated by the caller.
**	This is necessary, as unlocked named objects are subject to
**	reclaim at any time by other sessions.  It would be unfortunate
**	to have the object vanish out from under the caller.
**	The correct lock type is always or nearly always QSO_SHLOCK,
**	asking for a shared lock.  (Exclusive locks on sharable QP
**	objects are not allowed, except when the object is being
**	created -- and that's not the case here.)
**
**	Note that a GETNEXT call with no initial object handle is equivalent
**	to a GETHANDLE call asking for a shared lock.
**
**	This routine does not wait for the shared QP to finish being
**	created.  I think we get away with this because if this isn't the
**	creating session, the only way another session is supposed to know
**	the object ID is through a translation, and both the the translating
**	entry points do a wait-for.  Kinda scary though. (schka24)
**
** Inputs:
**      QSO_GETHANDLE                   Op code to qsf_call().
**      qsf_rb                          Request block.
**		.qsf_obj_id		Object id.
**			.qso_type	The type of object to look for.
**					Must be one of the following:
**					    QSO_QTEXT_OBJ - query text
**					    QSO_QTREE_OBJ - query tree
**					    QSO_QP_OBJ    - query plan
**					    QSO_SQLDA_OBJ - SQLDA used for SQL's
**							    `describe' stmt.
**					(Although the above is correct in
**					theory, in practice only QP objects
**					are ever named.)
**			.qso_lname	Length in bytes of the object name
**					given in .qso_name.
**			.qso_name	Name for the object.
**                                      For named objects, the object id
**                                      must be unique and is defined by
**                                      the name and type of object.
**                                      Therefore, there can be up to
**                                      4 objects with the same name:
**                                      1 QTEXT, 1 QTREE, 1 QP, 1 SQLDA.
**		.qsf_lk_state		Type of lock being asked for.  One of:
**					    QSO_SHLOCK - Lock for sharing.
**					    QSO_EXLOCK - Lock exclusively.
**						(probably a Bad Idea)
**
** Outputs:
**      qsf_rb                          Request block.
**		.qsf_obj_id		Object id.
**			.qso_handle	The object's handle.
**		.qsf_root 		The root address for the locked object.
**		.qsf_lk_id		If object is being locked for shared
**                                      access, this will not be touched.  If
**					locking exclusively, this will be set to
**					the lock id QSF assigns to the object.
**					This will be required whenever any form
**                                      of object modification is done through
**                                      the QSF routines (e.g. QSO_PALLOC,
**                                      QSO_SETROOT, etc.).
**		.qsf_error		Filled in with error if encountered.
**			.err_code	The QSF error that occurred.  One of:
**                              E_QS0000_OK
**				    Operation succeeded.
**				E_QS0009_BAD_OBJTYPE
**				    Illegal object type.
**				E_QS000B_BAD_OBJNAME
**				    Illegal object name.
**                              E_QS000D_CORRUPT
**				    QSF's object store has been corrupted.
**                              E_QS0014_EXLOCK
**                                  Object is already locked exclusively.
**                              E_QS0015_SHLOCK
**                                  Object is already locked shared.
**                              E_QS0016_BAD_LOCKSTATE
**                                  Illegal lock state; must be QSO_EXLOCK
**                                  or QSO_SHLOCK.
**				E_QS0019_UNKNOWN_OBJ
**				    No object of this name and type exists.
**				E_QS001B_NO_SESSION_CB
**				    Could not find the current session.
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
**	    none
**
** History:
**	06-mar-86 (thurston)
**          Initial creation.
**	02-mar-87 (thurston)
**	    Added support for new type of QSF object: SQLDA.
**	05-aug-87 (thurston)
**	    Changed interface to support object locking at get_handle time.
**	29-jan-88 (eric)
**	    Now returns `BAD_LOCK_STATE' if the caller attempts to EXLOCK a QP.
**	    This is an enforcement for shared QP's.
**	19-feb-88 (thurston)
**	    Check for EXLOCKs on a QP has been modified to be an EXLOCK on a
**	    "sharable" QP.
**	28-aug-1992 (rog)
**	    Change for(;;) to do/while(0) to silence compiler warning.
**	30-oct-1992 (rog)
**	    Added the owner to the semaphore, removed aliases, and added
**	    support for master objects.
**	30-mar-94 (rickh)
**	    Object usage counts were pointing at an uninitialized area.
**	22-Feb-2005 (schka24)
**	    Disallow calls that don't request a lock.  Fortunately, nobody
**	    does that, as it would be wrong anyway.
*/

DB_STATUS
qso_gethandle( QSF_RCB *qsf_rb )
{
    DB_STATUS		status;
    i4             	*err_code = &qsf_rb->qsf_error.err_code;
    QSO_OBID		*objid = &qsf_rb->qsf_obj_id;
    i4			objtyp = qsf_rb->qsf_obj_id.qso_type;
    QSO_OBJ_HDR         *obj;
    QSO_MASTER_HDR	*master = (QSO_MASTER_HDR*)NULL;


    /* Now do some error checking */

    switch ( objtyp )
    {
	case QSO_QTEXT_OBJ:
	case QSO_QTREE_OBJ:
	case QSO_QP_OBJ:
	case QSO_SQLDA_OBJ:
	    break;
	default:
	    DBGSTALL(qsf_rb);
	    *err_code = E_QS0009_BAD_OBJTYPE;
	    return(E_DB_ERROR);
    }

    if (objid->qso_lname <= 0 || objid->qso_lname > sizeof(objid->qso_name))
    {
	DBGSTALL(qsf_rb);
	*err_code = E_QS000B_BAD_OBJNAME;
	return(E_DB_ERROR);
    }
    if (qsf_rb->qsf_lk_state != QSO_SHLOCK && qsf_rb->qsf_lk_state != QSO_EXLOCK)
    {
	DBGSTALL(qsf_rb);
	*err_code = E_QS0016_BAD_LOCKSTATE;
	return (E_DB_ERROR);
    }

    *err_code = E_QS0000_OK;

    if ( (obj = qso_find(objid, &master, (QSO_HASH_BKT**)NULL)) == NULL)
    {
	/*DBGSTALL(qsf_rb);*/
	*err_code = E_QS0019_UNKNOWN_OBJ;
	return (E_DB_ERROR);
    }
    /* Object found, master mutex is held */

    /* Shareable QP objects may not be exclusively locked */
    if ( qsf_rb->qsf_lk_state == QSO_EXLOCK &&
	 objtyp == QSO_QP_OBJ &&
	 master->qsmo_aliasid.qso_lname > 0 )
    {
	DBGSTALL(qsf_rb);
	*err_code = E_QS0016_BAD_LOCKSTATE;
    }
    else if ( (status = qso_lkset( obj,
				qsf_rb->qsf_lk_state,
				&qsf_rb->qsf_lk_id,
				&qsf_rb->qsf_root,
				qsf_rb->qsf_scb,
				err_code) ) == E_DB_OK )
    {
	obj->qso_real_usage++;
	obj->qso_decaying_usage++;
	Qsr_scb->qsr_named_requests++;
    }
    else if (*err_code == E_QS0014_EXLOCK &&
	    !qsf_rb->qsf_root &&
	    qsf_rb->qsf_scb == obj->qso_excl_owner &&
	    qsf_rb->qsf_scb == obj->qso_session)
    {
	/* incomplete block that belongs to us -
	** allow handle to be returned */
	*err_code = E_QS0000_OK;
    }

    /* Release master mutex */
    CSv_semaphore(&master->qsmo_sem);

    return ( (*err_code) ? E_DB_ERROR : E_DB_OK );
}

/*{
** Name: QSO_GETNEXT - Retrieve the handle for the next named QSF object.
**
** Description:
**
**      This is the entry point to the QSF QSO_GETNEXT function. 
**	This opcode takes as input the handle of the query plan that the
**	requestor is looking at and returns the next query plan related to
**	this query if it exists.  Otherwise, it returns a warning status
**	with the error code set to E_QS0019_UNKNOWN_OBJ.
**
**	If the handle is NULL, then QSO_GETNEXT returns the first query plan
**	with the given object type/name, using shared locking.
**
**	The object will be locked with a shared lock, and the root address
**	will be returned.  Objects that are exclusively locked or whose root
**	has not been set will be "invisible" to QSO_GETNEXT.
**	After the object to be returned is locked, the shared lock on the
**	previous object (whose handle was passed in) is released.  This order
**	is important to prevent objects from being deleted out from under
**	the requesting session.
**
**	Most named objects only have one object per name.  The exception case
**	is set-input database procedures, which can have multiple QP's
**	attached to the same master index object, all with the same name.
**
**	Note that we do a wait-for on the object to be returned.  This
**	is actually somewhat logical, if awkward.  Normally we only need
**	to wait-for an object when we first access it, via an alias
**	translation (trans-or-define or just-trans).  (That is why the
**	gethandle operation doesn't wait-for.)  Now, though, we're
**	going after the next QP in the list, and we haven't waited-for
**	that one yet.  So we had better do so.
**
** Inputs:
**
**	QSO_GETNEXT			Op code to qsf_call().
**	qsf_rb				Request block.
**		.qsf_obj_id		Object id.
**					Either handle, or type/lname/name is
**					used, but not both.
**			.qso_handle	The current object's handle.
**					(Can be null)
**			.qso_type	The type of object to look for.
**					Currently must be:
**					    QSO_QP_OBJ    - query plan
**			.qso_lname	Length in bytes of the object name
**					given in .qso_name.
**			.qso_name	Name for the object.
**	
** Outputs:
**	qsf_rb				Request block.
**		.qsf_obj_id		Object id.
**			.qso_handle	The next object's handle.
**			.qsf_root	The root address of the object.
**		.qsf_error		Filled in with error if encountered.
**			.err_code	The QSF error that occurred.  One of:
**				E_QS0000_OK
**			 		Operation succeeded.
**				E_QS0009_BAD_OBJTYPE
**			 		Illegal object type.
**				E_QS000B_BAD_OBJNAME
**					Illegal object name.
**				E_QS000D_CORRUPT
**					QSF's object store has been corrupted.
**				E_QS0019_UNKNOWN_OBJ
**					No object of this name and type exists
** Returns:
**	E_DB_OK
**	E_DB_WARN
**	E_DB_ERROR
**	E_DB_FATAL
**	
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**	17-nov-1992 (rog)
**          Initial creation.
**      22-nov-2009 (stephenb)
**          If qso_gethandle returns E_QS0014_EXLOCK we should ignore
**          the object (see comments above).
*/
DB_STATUS
qso_getnext(QSF_RCB *qsf_rb)
{
    DB_STATUS		status = E_DB_OK;
    i4             *err_code = &qsf_rb->qsf_error.err_code;
    QSO_OBID		*objid = &qsf_rb->qsf_obj_id;
    QSO_OBJ_HDR         *obj;
    QSF_CB		*scb = qsf_rb->qsf_scb;
    QSO_MASTER_HDR	*master;

    if (objid->qso_handle == NULL)
    {
	/* Same as QSO_GETHANDLE case */
	qsf_rb->qsf_lk_state = QSO_SHLOCK;
	status = qso_gethandle(qsf_rb);
	if (status != E_DB_OK && *err_code == E_QS0014_EXLOCK)
	    /* 
	    ** (stephenb) we should ignore exclusively locked objects 
	    ** (see comments at head of function).
	    ** Also see the schka24 "kinda scary" comment at the head of 
	    ** qso_gethandle. Since exclock is supposed to imply that the object
	    ** is being created, how do we know its ID, since we should have
	    ** waited in translation. The implication is that there is a timing
	    ** window here that we are not completely handling, where an object
	    ** bay be "good" when we translate, and then perhaps is destroyed 
	    ** and is in the process of being re-created when we try to do 
	    ** something with it using the ID. This happens because we
	    ** often do not lock the object between translating it and using it
	    ** (or even in some parts of QSF itself).
	    ** The DBMS copes with the "destroyed from under our feet" case
	    ** by just asking for the object to be re-created (in SCF), but
	    ** if someone else already started that process, we could end up here,
	    ** where we have the object ID, but it's already exclusively locked
	    ** because someone else is already re-creating it.
	    ** I am handling this possibility here by simply returning QS0019
	    ** which will cause QEF to mark the plan invalid, which in turn will
	    ** cause SCF to re-try...but as Karl says... "kinda scary".
	    ** It's an architectural problem in QSF that we should fix some time.
	    */
	    *err_code = E_QS0019_UNKNOWN_OBJ;
	return (status);
    }

    obj = (QSO_OBJ_HDR *) objid->qso_handle;

    /* The current object must have a shared lock. */
    if ( obj->qso_lk_state != QSO_SHLOCK )
    {
	DBGSTALL(qsf_rb);
	*err_code = E_QS0016_BAD_LOCKSTATE;
	return (E_DB_ERROR);
    }

    if ( master = obj->qso_mobject )
	CSp_semaphore(TRUE, &master->qsmo_sem);

    /* Get next object, skip dead ones, wait-for the one we want.
    ** Next object handle set into qsf_rb's qsf_obj_id.
    */
    status = qso_nextbyhandle(obj, qsf_rb, scb, &master, err_code);

    /* Unlock the current object. */

    if ( obj->qso_lk_cur < 1 )
    {
	DBGSTALL(qsf_rb);
	*err_code = E_QS000D_CORRUPT;
	status = E_DB_FATAL;
    }
    else
    {
	/* All OK ... unlock the share-locked object */
	/* ----------------------------------------- */

	if ( --obj->qso_lk_cur == 0 )
	    obj->qso_lk_state = QSO_FREE;
    }

    if (status == E_DB_OK
	&& obj->qso_status & QSO_DEFERRED_DESTROY
	&& obj->qso_lk_cur == 0
	&& obj->qso_waiters == 0
       )
    {
	/*
	** If the object is marked to be destroyed later, and
	** there are no more locks on it, then let's destroy it.
	*/
	status = qso_rmvobj(&obj, &master, err_code);
    }

    if ( master )
	CSv_semaphore(&master->qsmo_sem);

    return (status);
}

/*{
** Name: QSO_TRANS_OR_DEFINE - Translate or define an alias to an object.
**
** Description:
**      This is the entry point to the QSF QSO_TRANS_OR_DEFINE routine. 
**      This opcode is used to either translate or define an alias to 
**	a QSF QP object in QSF's memory space to be shared by other facilities, 
**	and shared between sessions.  If an object with the given alias
**	already exists, we'll return the real object id.  If no such
**	object exists, we'll create one using the real object ID passed in.
**
**	An alias is not an object.  It's another name for a QSF object.
**	(It's not entirely clear to me why QSF callers need two names
**	for the same thing, but there you are.)
**	The QSO_ALIAS_OBJ object type never applies to a real object,
**	it's only used to distinguish alias ID's from real ID's.
**
**	If the caller does not pass a lock request (i.e. if QSO_FREE
**	is passed), and a translation is found (as opposed to a new
**	definition), be aware that the handle returned (if found) MAY NOT
**	be touched!  The *only* part of the returned info that is valid
**	on a successful QSO_FREE translation is the name info (type, lname,
**	and the name itself).  The QP object that the alias pointed to
**	may be fair game for reclamation, and might be gone even before
**	we make it out of this routine.
**
**	If we find a translation, and it looks like the object is still
**	being constructed, we'll wait for it to be available.
**
**	If a new object is defined, even if the caller indicates QSO_FREE,
**	we hack up the newly defined object so that it won't be reclaimed:
**	waiters is incremented, and the "exclusive owner" is set to us.
**	For this case, it is OK to use the returned object handle.
**	What will ultimately happen in the normal course of events is a
**	QSO_CREATE asking for the object ID;  create will find our defined
**	object and just hand it back, after marking its lock state EXCL.
**	Then, assuming all goes well, someone will QSO_UNLOCK the object
**	which is where the waiter count is decremented back to normal.
**	If something bad happens, the QSO_DESTROY routines know about this
**	weird object state (unlocked with nonzero waiter), and deal with
**	it properly.
**
** Inputs:
**	QSO_TRANS_OR_DEFINE             Op code to qsf_call().
**      qsf_rb                          Request block.
**		.qsf_feobj_id		The FE Object id to be used as the 
**					source of the translation.
**			.qso_type	The type of object this will be.
**					It must be:
**					    QSO_ALIAS_OBJ - An alias.
**			.qso_lname	Length in bytes of the object name
**					given in .qso_name.
**			.qso_name	Name for the object which consists of
**					a string and two i4's. The esql
**					compilier must ensure that this is
**					unique.
**		.qsf_obj_id		The DBMS Object id to be used as the
**					destination for the translation if one
**					is defined.
**			.qso_type	The type of object this will be.
**					It must be:
**					    QSO_QP_OBJ    - query plan
**			.qso_lname	Length in bytes of the object name
**					given in .qso_name. 
**			.qso_name	Name for the object.
**                                      For named objects, the object id
**                                      must be unique and is defined by
**                                      the name and type of object.
**                                      Therefore, there can be up to
**                                      4 objects with the same name:
**                                      1 QTEXT, 1 QTREE, 1 QP, 1 SQLDA. 
**		.qsf_lk_state		Type of lock being asked for.  One of:
**					    QSO_FREE   - No lock requested.
**					    QSO_SHLOCK - Lock for sharing.
**					    QSO_EXLOCK - Lock exclusively.
**					      (not allowed for shareable objs)
**					Note: The object is locked only if
**					WASTRANSLATED is returned.
**
** Outputs:
**      qsf_rb
**		.qsf_obj_id	If a translation for the qsf_feobj_id
**				exists then this will hold the destination
**				of the translation. If a translation needs to
**				be defined then this will remain unchanged. In
**				either case, qsf_obj_id will hold the DBMS id
**				that qsf_feobj_id translates to after this
**				operation is complete.
**			.qso_type	The type of object this will be.
**					It must be:
**					    QSO_QP_OBJ    - query plan
**			.qso_lname	Length in bytes of the object name
**					given in .qso_name. 
**			.qso_name	Name for the object.
**                                      For named objects, the object id
**                                      must be unique and is defined by
**                                      the name and type of object.
**                                      Therefore, there can be up to
**                                      4 objects with the same name:
**                                      1 QTEXT, 1 QTREE, 1 QP, 1 SQLDA. 
**			.qso_handle	The object's handle.  Valid only
**					if WASTRANSLATED is returned and
**					qsf_lk_state was not set to QSO_FREE.
**		.qsf_t_or_d		This tells the caller whether a
**					translation or definition happened.
**					It will hold QSO_WASTRANSLATED or
**					QSO_WASDEFINED.
**		.qsf_error		Filled in with error if encountered.
**			.err_code	The QSF error that occurred.  One of:
**				E_QS0000_OK
**				    Operation succeeded.
**				E_QS0001_NOMEM
**				    Out of memory.
**				E_QS0009_BAD_OBJTYPE
**				    Illegal object type.
**				E_QS000B_BAD_OBJNAME
**				    Illegal object name.
**                              E_QS000C_ULM_ERROR
**                                  Unexpected ULM error.
**                              E_QS000D_CORRUPT
**                                  QSF's internal structures got crunched.
**                              E_QS0014_EXLOCK
**                                  Object is already locked exclusively.
**                              E_QS0015_SHLOCK
**                                  Object is already locked shared.
**                              E_QS0016_BAD_LOCKSTATE
**                                  Illegal lock state; must be QSO_EXLOCK,
**                                  QSO_SHLOCK or QSO_FREE.
**				E_QS001B_NO_SESSION_CB
**				    Could not find the current session.
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
**          Other facilities and sessions will now "see" this
**	    alias just created, since an object header will
**	    be set up for it and entered into the object queue and
**          the object hash table.  QSF will guarantee, however, that
**	    other callers will not be able to get access to this object
**	    via QSO_TRANS_OR_DEFINE until the proper QSO_UNLOCK has been done
**	    on the underlying QP.
**
** History:
**	18-jan-88 (eric)
**          Initial creation.
**	03-jan-90 (ralph)
**	    Corrected quantum bugs:
**		- Allow lock to be set if WASTRANSLATED returned
**		- Return WASDEFINED if object was translated but removed
**	28-aug-1992 (rog)
**	    Change for(;;) to do/while(0) to silence compiler warning.
**	30-oct-1992 (rog)
**	    Reworked aliases, and added the owner to the semaphores.  Added
**	    support for master objects.
**	03-mar-1994 (rog)
**	    If qso_wait_for() says that the object was destroyed, we need
**	    to recheck the hash table to look for the object in case someone
**	    is rebuilding the QP before we got a chance to.
**	16-may-1994 (rog)
**	    Attach new aliases to the MO structures and save the first QP's
**	    handle in the alias's QSO_OBID.
**	19-nov-1998 (somsa01)
**	    If we find the object, bump up it's wait status to let others
**	    know that this session is using it.  (Bug #94216)
**	03-dec-1998 (somsa01)
**	    Removed previous changes; they have been reworked for bug 94216.
**	31-oct-2007 (dougi)
**	    trans_or_define() also works on text objects (for cached dynamic).
*/
DB_STATUS
qso_trans_or_define( QSF_RCB *qsf_rb )
{
    DB_STATUS		status = E_DB_OK;
    i4             	*err_code = &qsf_rb->qsf_error.err_code;
    QSO_OBID		*alias_objid = &qsf_rb->qsf_feobj_id;
    i4			alias_objtyp = qsf_rb->qsf_feobj_id.qso_type;
    QSO_OBJ_HDR		*qp_obj;
    QSO_OBID		*qp_objid = &qsf_rb->qsf_obj_id;
    i4			qp_objtyp = qsf_rb->qsf_obj_id.qso_type;
    QSF_CB		*scb = qsf_rb->qsf_scb;
    QSO_OBJ_HDR         *obj;
    QSO_MASTER_HDR	*master;
    QSO_MASTER_HDR	*alias;
    QSO_HASH_BKT	*hash;
    /* Now do some error checking */

    if (alias_objtyp != QSO_ALIAS_OBJ || (qp_objtyp != QSO_QP_OBJ &&
		qp_objtyp != QSO_QTEXT_OBJ))
    {
	DBGSTALL(qsf_rb);
	*err_code = E_QS0009_BAD_OBJTYPE;
	return(E_DB_ERROR);
    }

    if (alias_objid->qso_lname <= 0 || alias_objid->qso_lname > sizeof(alias_objid->qso_name)
      || qp_objid->qso_lname <= 0 || qp_objid->qso_lname > sizeof(qp_objid->qso_name))
    {
	DBGSTALL(qsf_rb);
	*err_code = E_QS000B_BAD_OBJNAME;
	return(E_DB_ERROR);
    }

    /* Doesn't usually loop, but can loop if we start a define, and
    ** object turns up while we're off reclaiming memory.
    */
    for (;;)
    {
	/* We've passed all of the initial error checks, so reset the errcode */
	*err_code = E_QS0000_OK;
	master = (QSO_MASTER_HDR*)NULL;
	alias = (QSO_MASTER_HDR*)NULL;
	hash = (QSO_HASH_BKT*)NULL;

	/* Look for an object under the alias */
	while ( obj = qso_find(alias_objid, &master, &hash) )
	{
	    /* Object found, we're holding the master mutex */
	    qp_obj = master->qsmo_obj_1st;

	    /* Wait for any QP's in the process of being built.
	    ** If it disappears while we wait, try again.
	    */
	    status = qso_wait_for(&qp_obj, &master, scb, err_code);
	    if (qp_obj || status != E_DB_OK)
		break;
	}

	if (status != E_DB_OK)
	    break;

	if ( obj )
	{
	    /* Fill in the output values now that we know them. */
	    qsf_rb->qsf_t_or_d = QSO_WASTRANSLATED;
	    Qsr_scb->qsr_no_trans++;
	    STRUCT_ASSIGN_MACRO(obj->qso_obid, *qp_objid);

	    /* Lock the object if requested */
	    if (qsf_rb->qsf_lk_state != QSO_FREE)
	    {
		if (qsf_rb->qsf_lk_state == QSO_EXLOCK &&
		    master->qsmo_aliasid.qso_lname > 0
		   )
		{
		    DBGSTALL(qsf_rb);
		    *err_code = E_QS0016_BAD_LOCKSTATE;
		    status = E_DB_ERROR;
		    break;
		}

		if (status = qso_lkset( obj,
					qsf_rb->qsf_lk_state,
					&qsf_rb->qsf_lk_id,
					&qsf_rb->qsf_root,
					scb,
					err_code
				      )
		   )
		{
		    break;
		}
	    }
	}
	else
	{
	    /*
	    ** We didn't find an object, so lets create a new one.
	    **
	    ** The appropriate hash qsb_mutex is held (by qso_find())
	    */
	    /* Create master index object and QP object */
	    status = qso_mkobj(&qp_obj, qp_objid, QSO_BQP_BLKSZ, scb, 
				&hash, &master, err_code);
	    if (status == E_DB_INFO)
	    {
		/* The object popped up while we were reclaiming memory.
		** Start over from the top, it's probably a "translate" now.
		*/
		CSv_semaphore(&master->qsmo_sem);
		continue;
	    }

	    if (DB_FAILURE_MACRO(status))
	    {
		DBGSTALL(qsf_rb);
		break;
	    }

	    /* Master's semaphore is also held */

	    if (qsf_rb->qsf_flags & QSO_REPEATED_OBJ) 	
	        qp_obj->qso_type = QSOBJ_TYPE;
	    else if (qsf_rb->qsf_flags & QSO_DBP_OBJ) 	
	    	qp_obj->qso_type = QSOBJ_DBP_TYPE;

	    qsf_rb->qsf_t_or_d = QSO_WASDEFINED;
	    Qsr_scb->qsr_no_defined++;

	    /*
	    ** Fill in the alias object ID in the master.
	    ** Also fill in the alias object handle.  It's meaningless and
	    ** bogus, but MO needs something.
	    */
	    STRUCT_ASSIGN_MACRO(qsf_rb->qsf_feobj_id, master->qsmo_aliasid);
	    alias_objid->qso_handle = (PTR) qp_obj;
	    master->qsmo_aliasid.qso_handle = (PTR) qp_obj;

	    /* Attach the alias to the MO structures. */
	    status = qsf_attach_inst(*alias_objid,qp_obj->qso_type);

	    if (DB_FAILURE_MACRO(status))
	    {
#ifdef xDEBUG
		TRdisplay("QSF MO Attach failure : status = (%d) \n",status);
		TRdisplay("objid = %x, qso_type = %d, qso_lname = %d \n",
			  alias_objid->qso_handle, alias_objid->qso_type,
			  alias_objid->qso_lname);
#endif
		status = E_DB_OK;
	    }

	    /* Now lets unlock the QP object, since other people will want
	    ** to lock it, but set the wait flag to make sure that others
	    ** know that the plan is waiting to be completed.
	    ** That will also prevent memory reclaim from grabbing it.
	    */
	    qp_obj->qso_lk_state = QSO_FREE;
	    qp_obj->qso_lk_cur = 0;
	    qp_obj->qso_lk_id = 0;
	    qp_obj->qso_waiters++;
	    qp_obj->qso_excl_owner = scb;
	}
	break;

    };  /* for */

    /* If master is still locked, release its semaphore */
    if ( master )
	CSv_semaphore(&master->qsmo_sem);


#ifdef    xDEBUG
    if (    Qsr_scb->qsr_tracing
	&&  (	qst_trcheck(scb, QSF_004_AFT_CREATE_OBJQ)
	     || (   alias_objid->qso_lname
		 && qst_trcheck(scb, QSF_007_AFT_CREATE_NAMED)
		)
	    )
       )
    {
	/* Dump QSF obj queue after creating an object */
	/* ------------------------------------------- */
	DB_STATUS		ignore;
	i4			save_err = *err_code;

	TRdisplay("<<< Dumping QSF object queue after creating object >>>\n");
	ignore = qsd_obq_dump(qsf_rb);

	*err_code = save_err;
    }
#endif /* xDEBUG */

    return (status);
}

/*{
** Name: QSO_JUST_TRANS - Translate an alias to an object (but don't define).
**
** Description:
**      This is the entry point to the QSF QSO_JUST_TRANS routine. 
**      This opcode is used to translate an alias to 
**	a QSF QP object in QSF's memory space to be shared by other facilities, 
**	and shared between sessions. If the alias does not exist, then
**	an error is returned rather than defining the alias.
**
**	An alias is not an object.  It's another name for a QSF object.
**	The QSO_ALIAS_OBJ object type never applies to a real object,
**	it's only used to distinguish alias ID's from real ID's.
**
**	If the caller does not pass a lock request (i.e. if QSO_FREE
**	is passed), be aware that the handle returned (if found) MAY NOT
**	be touched!  The *only* part of the returned info that is valid
**	on a successful QSO_FREE translation is the name info (type, lname,
**	and the name itself).  The QP object that the alias pointed to
**	may be fair game for reclamation, and might be gone even before
**	we make it out of this routine.
**
**	If we find a translation, and it looks like the object is still
**	being constructed, we'll wait for it to be available.
**
** Inputs:
**	QSO_JUST_TRANS			Op code to qsf_call().
**      qsf_rb                          Request block.
**		.qsf_feobj_id		The FE Object id to be used as the 
**					source of the translation.
**			.qso_type	The type of object this will be.
**					It must be:
**					    QSO_ALIAS_OBJ - An alias.
**			.qso_lname	Length in bytes of the object name
**					given in .qso_name.
**			.qso_name	Name for the object which consists of
**					a string and two i4's. The esql
**					compilier must ensure that this is
**					unique.
**		.qsf_lk_state		Type of lock being asked for.  One of:
**					    QSO_FREE   - No lock requested.
**					    QSO_SHLOCK - Lock for sharing.
**					    QSO_EXLOCK - Lock exclusively.
**					Note: The object is locked only if
**					WASTRANSLATED is returned.
**
** Outputs:
**      qsf_rb
**		.qsf_obj_id	If a translation for the qsf_feobj_id
**				exists then this will hold the destination
**				of the translation. If a translation needs to
**				be defined then this will remain unchanged. In
**				either case, qsf_obj_id will hold the DBMS id
**				that qsf_feobj_id translates to after this
**				operation is complete.
**			.qso_type	The type of object this will be.
**					It must be:
**					    QSO_QP_OBJ    - query plan
**			.qso_lname	Length in bytes of the object name
**					given in .qso_name. 
**			.qso_name	Name for the object.
**                                      For named objects, the object id
**                                      must be unique and is defined by
**                                      the name and type of object.
**                                      Therefore, there can be up to
**                                      4 objects with the same name:
**                                      1 QTEXT, 1 QTREE, 1 QP, 1 SQLDA.  
**			.qso_handle	The object's handle.  Valid only
**					if WASTRANSLATED is returned and
**					qsf_lk_state was not set to QSO_FREE.
**		.qsf_t_or_d		This tells the caller that a 
**					translation or happened.
**					It will hold QSO_WASTRANSLATED.
**		.qsf_error		Filled in with error if encountered.
**			.err_code	The QSF error that occurred.  One of:
**				E_QS0000_OK
**				    Operation succeeded.
**				E_QS0001_NOMEM
**				    Out of memory.
**				E_QS0009_BAD_OBJTYPE
**				    Illegal object type.
**				E_QS000B_BAD_OBJNAME
**				    Illegal object name.
**                              E_QS000C_ULM_ERROR
**                                  Unexpected ULM error.
**                              E_QS000D_CORRUPT
**                                  QSF's internal structures got crunched.
**                              E_QS0014_EXLOCK
**                                  Object is already locked exclusively.
**                              E_QS0015_SHLOCK
**                                  Object is already locked shared.
**                              E_QS0016_BAD_LOCKSTATE
**                                  Illegal lock state; must be QSO_EXLOCK,
**                                  QSO_SHLOCK or QSO_FREE.
**				E_QS0019_UNKNOWN_OBJ
**				    The Alias is not known.
**				E_QS001B_NO_SESSION_CB
**				    Could not find the current session.
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
**
** History:
**	18-jan-88 (eric)
**          Initial creation.
**	03-jan-89 (ralph)
**	    Corrected quantum bugs:
**		- Allow lock to be set if WASTRANSLATED returned
**		- Return UNKNOWN_OBJ if object was translated but removed
**	28-aug-1992 (rog)
**	    Change for(;;) to do/while(0) to silence compiler warning.
**	30-oct-1992 (rog)
**	    Removed aliases, and added the owner to the semaphores.
**	03-mar-1994 (rog)
**	    If qso_wait_for() says that the object was destroyed, we need
**	    to recheck the hash table to look for the object in case someone
**	    is rebuilding the QP before we got a chance to.
**	19-nov-1998 (somsa01)
**	    If we find the object, bump up it's wait status to let others
**	    know that this session is using it.  (Bug #94216)
**	30-nov-1998 (somsa01)
**	    Removed previous change from this function. It is not needed
**	    here. Furthermore, it caused a memory leak.
**      11-nov-2007 (huazh01)
**          Initialize qsf_rb->qsf_t_or_d to -1. This prevent qsf_t_or_d
**          get a random value if no object is found. 
**          This fixes b119495. 
*/

DB_STATUS
qso_just_trans( QSF_RCB *qsf_rb )
{
    DB_STATUS           status;
    i4      	        *err_code = &qsf_rb->qsf_error.err_code;
    QSO_OBID		*alias_objid = &qsf_rb->qsf_feobj_id;
    i4			alias_objtyp = qsf_rb->qsf_feobj_id.qso_type;
    QSO_OBJ_HDR		*qp_obj;
    QSO_OBID		*qp_objid = &qsf_rb->qsf_obj_id;
    i4			qp_objtyp = qsf_rb->qsf_obj_id.qso_type;
    QSF_CB		*scb = qsf_rb->qsf_scb;
    QSO_OBJ_HDR         *obj;
    QSO_MASTER_HDR	*master = (QSO_MASTER_HDR*)NULL;

    /* Now do some error checking */

    if (alias_objtyp != QSO_ALIAS_OBJ)
    {
	DBGSTALL(qsf_rb);
	*err_code = E_QS0009_BAD_OBJTYPE;
	return(E_DB_ERROR);
    }

    if (alias_objid->qso_lname <= 0 || alias_objid->qso_lname > sizeof(alias_objid->qso_name))
    {
	DBGSTALL(qsf_rb);
	*err_code = E_QS000B_BAD_OBJNAME;
	return(E_DB_ERROR);
    }

    *err_code = E_QS0000_OK;
    status = E_DB_OK;
    qsf_rb->qsf_t_or_d = -1;

    while ( obj = qso_find(alias_objid, &master, (QSO_HASH_BKT**)NULL) )
    {
	/* Found, holding the master mutex */
	/* Wait for any QP's in the process of being built. */
	qp_obj = master->qsmo_obj_1st;

	status = qso_wait_for(&qp_obj, &master, scb, err_code);
	if (qp_obj || status != E_DB_OK)
	    break;
    }

    if ( status == E_DB_OK )
    {
	if ( obj == NULL )
	{
	    /*DBGSTALL(qsf_rb);*/
	    *err_code = E_QS0019_UNKNOWN_OBJ;
	}
	else
	{
	    /* Fill in the output values now that we know them. */
	    qsf_rb->qsf_t_or_d = QSO_WASTRANSLATED;
	    Qsr_scb->qsr_no_trans++;
	    STRUCT_ASSIGN_MACRO(obj->qso_obid, *qp_objid);

	    /* Lock the object if requested */
	    if (qsf_rb->qsf_lk_state != QSO_FREE)
	    {
		if ( qsf_rb->qsf_lk_state == QSO_EXLOCK &&
		     master->qsmo_aliasid.qso_lname > 0 )
		{
		    DBGSTALL(qsf_rb);
		    *err_code = E_QS0016_BAD_LOCKSTATE;
		}
		else
		{
		    status = qso_lkset( obj,
					qsf_rb->qsf_lk_state,
					&qsf_rb->qsf_lk_id,
					&qsf_rb->qsf_root,
					scb,
					err_code
				      );
		}
	    }
	}
    }

    /* If master's still around, release its semaphore */
    if ( master )
	CSv_semaphore(&master->qsmo_sem);

    return ( (*err_code) ? E_DB_ERROR : E_DB_OK );
}


/*{
** Name: QSO_CHTYPE - Change type of a QSF object.
**
** Description:
**      This is the entry point to the QSF QSO_CHTYPE routine. 
**      This opcode is used to change the type of a QSF object.
**
**      This opcode will only be legal while the object is exclusively locked,
**	and the caller can supply the proper lock id.
**
**	The only (current) usage of this operation is to change a
**	QP object into a QTREE object, for CREATE TABLE AS SELECT and
**	the dgtt variant.  When CREATE parsing starts, the parser thinks
**	that a typical non-QP DDL statement is coming, but to allow the
**	subselect to be properly optimized, the parse result has to be
**	a QTREE object.  In no case do we change the type of a named object.
**
** Inputs:
**      QSO_CHTYPE			Op code to qsf_call().
**      qsf_rb                          Request block.
**		.qsf_obj_id		Object id.
**			.qso_handle	The object's handle.
**			.qso_type	The new type for the object; must
**					be one of the following:
**					    QSO_QTEXT_OBJ - query text
**					    QSO_QTREE_OBJ - query tree
**					    QSO_QP_OBJ    - query plan
**					    QSO_SQLDA_OBJ - SQLDA used for SQL's
**							    `describe' stmt.
**			.qso_lname	Length of the object name.  Must be
**					zero, as only unnamed objects may have
**					their type changed.
**		.qsf_lk_id		The lock id QSF assigned to this object
**					when you locked it exclusively.
**
** Outputs:
**      qsf_rb                          Request block.
**		.qsf_error		Filled in with error if encountered.
**			.err_code	The QSF error that occurred.  One of:
**                              E_QS0000_OK
**				    Operation succeeded.
**				E_QS0009_BAD_OBJTYPE
**				    Supplied object type is invalid.
**				E_QS000A_OBJ_ALREADY_EXISTS
**				    Object with spec'd name,type already exists.
**                              E_QS000F_BAD_HANDLE
**				    Supplied handle does not refer to an object.
**				E_QS0010_NO_EXLOCK
**				    You don't have exclusive access to object.
**
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    None.
**
** History:
**	16-mar-87 (stec)
**          Initial creation.
**	28-nov-87 (thurston)
**	    Changed semaphore code to use CS calls instead of going through SCF.
**	28-aug-1992 (rog)
**	    Change for(;;) to do/while(0) to silence compiler warning.
**	30-oct-1992 (rog)
**	    Added the owner to the semaphores, and removed unnamed objects from
**	    the bucket moving code.
**	06-mar-1996 (nanpr01)
**	    List is getting corrupted here. 
**	21-Feb-2005 (schka24)
**	    Unnamed objects only.
**      17-feb-09 (wanfr01) b121543
**          Add QSOBJ_DBP_TYPE as valid qso_type
*/

DB_STATUS
qso_chtype( QSF_RCB *qsf_rb )
{
    QSO_OBID		*objid = &qsf_rb->qsf_obj_id;
    i4			objtyp = objid->qso_type;
    QSO_OBJ_HDR         *obj = (QSO_OBJ_HDR *) objid->qso_handle;
    i4			*err_code = &qsf_rb->qsf_error.err_code;

    /*
    ** First, make sure there is an object at the given handle.
    ** Make sure the object ID is for an unnamed object.
    */
    if (!obj
        ||  !((obj->qso_type == QSOBJ_TYPE) ||
              (obj->qso_type == QSOBJ_DBP_TYPE))
	|| (obj->qso_ascii_id != QSOBJ_ASCII_ID)
	|| (obj->qso_owner != (PTR)DB_QSF_ID)
	|| (obj->qso_length != sizeof(QSO_OBJ_HDR))
	|| objid->qso_lname > 0
       )
    {
	DBGSTALL(qsf_rb);
	*err_code = E_QS000F_BAD_HANDLE;
	return(E_DB_ERROR);
    }

    /*
    ** Verify that the new type is valid.
    */
    switch ( objtyp )
    {
	case QSO_QTEXT_OBJ:
	case QSO_QTREE_OBJ:
	case QSO_QP_OBJ:
	case QSO_SQLDA_OBJ:
	    break;
	default:
	    DBGSTALL(qsf_rb);
	    *err_code = E_QS0009_BAD_OBJTYPE;
	    return(E_DB_ERROR);
    }

    /*
    ** Make sure object is locked by caller.
    */
    if ( obj->qso_lk_state != QSO_EXLOCK ||
	 obj->qso_lk_id != qsf_rb->qsf_lk_id )
    {
	DBGSTALL(qsf_rb);
	*err_code = E_QS0010_NO_EXLOCK;
	return(E_DB_ERROR);
    }

    *err_code = E_QS0000_OK;

    obj->qso_obid.qso_type = objtyp;

    return (E_DB_OK);
}


/*{
** Name: qso_rmvobj - Remove an object from QSF.
**
** Description:
**	Removes the object from QSF, and, if necessary, destroys the master
**	object that owns the object.
**
**	There are three slight call variants:
**	master_p == NULL - this is a call to delete a named or unnamed
**	object from the outside (qs0_clrmem) (currently, always a named
**	object).  Need to mutex the object master and make sure that the
**	object is still available for deleting.
**
**	master_p != NULL but *master_p == NULL - delete an unnamed object
**
**	master_p != NULL and *master_p != NULL - delete a named object,
**	we already hold the master mutex and it's OK to go ahead with the
**	deletion.  If the master doesn't disappear, the master mutex
**	remains held throughout and at exit.
**
**	If we remove a master, before releasing the master mutex,
**	the master header type is cleared so that qso_find won't see
**	this master after we release the mutex.  Also, if there's someone
**	in qso_find on the master's hash bucket, the actual delete
**	of the master will stall until the finder is out of qso_find.
**	These two precautions (actually, taken in qso_rmvindex) ensure
**	that there are no fatal find-vs delete races.
**
** Inputs:
**      qso_obj			Pointer to the QSO_OBJ_HDR.
**	master_p		QSO_MASTER_HDR **, pointer to master.
**				This is nulled out if we remove the master.
**	errcode			An output
**
** Outputs:
**	errcode			Detailed error code if any
**
**	Returns:
**	    E_DB_OK, E_DB_FATAL
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    Removes the object from QSF, so all others will no longer see it.
**
** History:
**	29-apr-86 (thurston)
**          Initial creation.
**	05-jul-87 (thurston)
**	    Added code to check for server wide tracing before even attempting
**	    to look for a particular trace flag.
**	22-jan-88 (eric)
**	    Added support for QSO_TRANS_OR_DEFINE. This involved automatically
**	    destroying alias' when QP's are destroyed. Also, this routine now
**	    destroys the objects ULM memory stream.
**	08-jan-1992 (fred)
**	    Added support for destroying associated objects as part of
**	    destroying the base object.
**	30-oct-1992 (rog)
**	    Reworked support for aliases and simplified the function.  Also
**	    added support for master objects.
**	15-jan-1993 (rog)
**	    Make sure to remove all objects from the session's orphan list.
**	09-jul-1993 (rog)
**	    Call CSr_semaphore() to release object's the semaphore.
**	09-jan-1997(angusm)
**	    Diagnostic and other changes relating to bug 76278.
**	    Since AEO functionality is not used, prevent AEO cleanup
**	06-mar-1996 (nanpr01)
**	    Initialize the pointers to null after freeing the memory. 
**	03-dec-1998 (somsa01)
**	    If we are just removing the object from the list of objects
**	    attached to the master object, we need to also update the
**	    alias' pointer to the first object (if there is one).
**	    (Bug #94216)
**	21-Oct-2003 (jenjo02)
**	    When called externally from qs0_clrobj() (master_p == NULL),
**	    sanity check the object, recheck it for
**	    removability after acquiring Master's mutex.
**	24-Feb-2005 (schka24)
**	    Maintain object size stats here.
**	2-Mar-2005 (schka24)
**	    Avoid dereferencing an obsolete session list pointer, it can't
**	    be meaningful unless the object is private to the session.
**      13-apr-2005 (huazh01)
**          Modify the fix for b111006, INGSRV2535. After including the
**          fix for b112813, INGSRV2931, it is no longer necessary to 
**          request Qsr_scb->qsr_psem because its caller routine, 
**          qsf_clrmem(), already holds such mutex. 
**          This fixes b114290, INGSRV3245.
**      17-feb-09 (wanfr01) b121543
**          Remove the object from the appropriate IMA list as determined by
**	    the QSF object's qso_type
*/

DB_STATUS
qso_rmvobj( 
QSO_OBJ_HDR 	**qso_obj,
QSO_MASTER_HDR	**master_p,
i4 		*err_code )
{
    QSO_OBJ_HDR		*obj = *qso_obj;
    QSO_MASTER_HDR	*master, *alias;
    ULM_RCB		ulm_rcb;
    DB_STATUS		status;
    i4			obj_size;
    bool		shareable;

    /* If Master passed, caller has mutexed it */
    if ( master_p )
	master = *master_p;
    else
    {
	/* Called externally from qs0_clrobj(): */
	if ( !((obj->qso_type == QSOBJ_TYPE) ||
              (obj->qso_type == QSOBJ_DBP_TYPE)) ||
	     obj->qso_ascii_id != QSOBJ_ASCII_ID ||
	     obj->qso_owner != (PTR)DB_QSF_ID ||
	     obj->qso_length != sizeof(QSO_OBJ_HDR) )
	{
	    return(E_DB_ERROR);
	}

	if ( master = obj->qso_mobject )
	{
	    CSp_semaphore(TRUE, &master->qsmo_sem);

	    /* If no longer removable, don't try, tell caller */
	    if ( obj->qso_lk_state != QSO_FREE || obj->qso_waiters
	      || master->qsmo_type != QSMOBJ_TYPE )
	    {
		CSv_semaphore(&master->qsmo_sem);
		return(E_DB_ERROR);
	    }
	}
    }

    *err_code = E_QS0000_OK;
    status = E_DB_OK;
    shareable = FALSE;

    obj_size = obj->qso_size;

    /* First, remove it from the appropriate bucket or linked list */
    if (obj->qso_obid.qso_lname == 0)
    {
	/* Update the statistics */
	Qsr_scb->qsr_no_unnamed--;
    }
    else
    {
	/* remove named object from the LRU queue, if it was there. */
	CSp_semaphore(TRUE, &Qsr_scb->qsr_psem);
	if ( obj->qso_lrnext )
	    obj->qso_lrnext->qso_lrprev = obj->qso_lrprev;
	if ( obj->qso_lrprev )
	    obj->qso_lrprev->qso_lrnext = obj->qso_lrnext;
	else if (Qsr_scb->qsr_1st_lru == obj)
	{
	    /* Update the pointer to the head of the list. */
	    Qsr_scb->qsr_1st_lru = obj->qso_lrnext;
	}
	/* Now, update the statistics */
	Qsr_scb->qsr_no_named--;
	CSv_semaphore(&Qsr_scb->qsr_psem);

	obj->qso_lrnext = (QSO_OBJ_HDR *) NULL;
	obj->qso_lrprev = (QSO_OBJ_HDR *) NULL;

	/* Note whether shareable for later on */
	if (master->qsmo_aliasid.qso_lname > 0)
	    shareable = TRUE;

	if (obj->qso_monext == (QSO_OBJ_HDR *) NULL
	    && obj->qso_moprev == (QSO_OBJ_HDR *) NULL)
	{
	    /*
	    ** This is the only QP attached to the master object, so 
	    ** destroy the master, too.
	    */

	    /* If there's an alias object ID, detach it from IMA */
	    if (master->qsmo_aliasid.qso_lname > 0)
	    {
		STATUS	stat;

		stat = qsf_detach_inst(master->qsmo_aliasid, obj->qso_type);
		if (stat)
		{
#ifdef xDEBUG
		    TRdisplay("QSF MO detach failure : status = (%d) \n",
			      stat);
		    TRdisplay("objid = %x, qso_type = %d, qso_lname = %d \n",
		       alias->qsmo_obid.qso_handle, 
		       alias->qsmo_obid.qso_type,
		       alias->qsmo_obid.qso_lname);
#endif
		    if (DB_FAILURE_MACRO(stat)) 
			TRdisplay("@ qsf_detach_inst(2) failure, stat %d\n", stat);
		}

	    }
	    /* Destroy the master */
	    status = qso_rmvindex(&master, err_code);
	    if ( master_p )
		*master_p = (QSO_MASTER_HDR *) NULL;
	    if (DB_FAILURE_MACRO(status))
	    {
		TRdisplay("@ qso_rmvindex failure(3), status %d\n", status);
	    }
	    /* Include master header in object size */
	    obj_size += sizeof(QSO_MASTER_HDR);
	}
	else
	{
	    /*
	    ** Otherwise, just remove it from the list of objects
	    ** attached to the master object.
	    */
	    if ( obj->qso_monext )
		obj->qso_monext->qso_moprev = obj->qso_moprev;
	    else
		master->qsmo_obj_last = obj->qso_moprev;
	    if ( obj->qso_moprev )
		obj->qso_moprev->qso_monext = obj->qso_monext;
	    else
	    {
		master->qsmo_obj_1st = obj->qso_monext;
	    }

	    obj->qso_moprev = (QSO_OBJ_HDR *) NULL;
	    obj->qso_monext = (QSO_OBJ_HDR *) NULL;
	}

    }

    /* Remove the object from the session's orphan list.  Shareable
    ** named objects are never put on the list, don't try to take
    ** them off (qso_session may be completely obsolete).
    */

    if (!shareable)
    {
	if ( obj->qso_obnext )
	    obj->qso_obnext->qso_obprev = obj->qso_obprev;
	if ( obj->qso_obprev )
	    obj->qso_obprev->qso_obnext = obj->qso_obnext;
	else if (obj->qso_session->qss_obj_list == obj)
	    obj->qso_session->qss_obj_list = obj->qso_obnext;
    }

    /* Now, update more statistics.
    ** This is a simple place to accumulate object totals for average
    ** size info.  We know that it won't get any bigger.  :-)
    ** (setroot might work too, but what the heck.)
    */
    Qsr_scb->qsr_nobjs--;
    ++ Qsr_scb->qsr_num_size[obj->qso_obid.qso_type];
    Qsr_scb->qsr_sum_size[obj->qso_obid.qso_type] += obj_size;
    
    /* Make the object look deleted */
    obj->qso_type = 0;
    obj->qso_ascii_id = 0;
    obj->qso_owner = 0;
    obj->qso_length = 0;

    /* Finally, close its memory stream */
    ulm_rcb.ulm_facility = DB_QSF_ID;
    ulm_rcb.ulm_streamid_p = &obj->qso_streamid;
    ulm_rcb.ulm_memleft = &Qsr_scb->qsr_memleft;
    if ( status = ulm_closestream(&ulm_rcb) )
	*err_code = E_QS0011_ULM_STREAM_NOT_CLOSED;

    /*
    ** If master is still around and we mutexed it here,
    ** release its mutex.
    */
    if ( master_p == NULL && master != NULL )
	CSv_semaphore(&master->qsmo_sem);
    *qso_obj = (QSO_OBJ_HDR*)NULL;
    return (status);
}

/*{
** Name: qso_rmvindex - Remove an index object from QSF.
**
** Description:
**	Removes a master index object from QSF.
**	The caller must enter holding the master mutex.
**
**	See qso_rmvobj for info on race prevention.
**
** Inputs:
**      index_p			Pointer * to the QSO_MASTER_HDR.
**
** Outputs:
**	index_p			Nulled out upon return
**
** Returns:
**	E_DB_OK, E_DB_FATAL
**
** Exceptions:
**	none
**
** Side Effects:
**	Removes the index object from QSF, so all others will no longer see it.
**
** History:
**	30-nov-1992 (rog)
**          Initial creation.
**	14-jan-1993 (rog)
**          Added a "return(E_DB_OK)" at the end of the function.
**	09-jul-1993 (rog)
**	    Call CSr_semaphore() to release object's the semaphore.
**	09-jan-1997(angusm)
**	    Diagnostic and other changes relating to bug 76278.
**	    Since AEO functionality is not used, prevent AEO cleanup
*/

static DB_STATUS
qso_rmvindex(
QSO_MASTER_HDR 	**index_p,
i4		*err_code )
{
    QSO_MASTER_HDR	*index = *index_p;
    ULM_RCB		ulm_rcb;
    QSO_HASH_BKT	*bucket;

    /* Caller must hold "index" qsmo_sem */

    /* Remove index from hash chain */
    bucket = index->qsmo_bucket;
    index->qsmo_type = 0;		/* Make this master invisible */

    CSv_semaphore(&index->qsmo_sem);

    CSp_semaphore(TRUE, &bucket->qsb_sem);

    /*
    ** Wait for any threads searching this hash list
    ** to be done before removing this entry.  This ensures that we
    ** don't delete a master out from underneath someone who's searching.
    ** FIXME, this is a pure spin, if contention develops make it a
    ** condition wait.
    */
    while ( bucket->qsb_init > 1 )
    {
	CSv_semaphore(&bucket->qsb_sem);
	Qsr_scb->qsr_hash_spins++;
	/* This is evil, but you can't call CS_swuser in OS-thread mode,
	** and CSswitch doesn't really do the job, and there's no better
	** interface.  Without this, internal threads just eat cpu and
	** make no progress.  FIXME
	*/
	if ( !CS_is_mt())
	    CS_swuser();
	CSp_semaphore(TRUE, &bucket->qsb_sem);
    }

    if (index == bucket->qsb_ob1st)
	bucket->qsb_ob1st = index->qsmo_obnext;
    else
	index->qsmo_obprev->qsmo_obnext = index->qsmo_obnext;
    if ( index->qsmo_obnext )
	index->qsmo_obnext->qsmo_obprev = index->qsmo_obprev;

    /* Now, update the statistics */
    if (--bucket->qsb_nobjs == 0)
	Qsr_scb->qsr_bkts_used--;
    Qsr_scb->qsr_nobjs--;
    Qsr_scb->qsr_no_index--;
    
    CSv_semaphore(&bucket->qsb_sem);

    /* Destroy the master's condition and semaphore */
    CScnd_free(&index->qsmo_cond);
    CSr_semaphore(&index->qsmo_sem);

    /* Close the index object's memory stream */
    ulm_rcb.ulm_facility = DB_QSF_ID;
    ulm_rcb.ulm_streamid_p = &index->qsmo_streamid;
    ulm_rcb.ulm_memleft = &Qsr_scb->qsr_memleft;
    if ( ulm_closestream(&ulm_rcb) )
	*err_code = E_QS0011_ULM_STREAM_NOT_CLOSED;
    else
	*err_code = E_QS0000_OK;

    *index_p = (QSO_MASTER_HDR*)NULL;

    return ( (*err_code) ? E_DB_ERROR : E_DB_OK );
}

/*{
** Name: qso_wait_for - Wait for a QP in the process of being created
**
** Description:
**	Waits for a shareable, named QP that is in the process of
**	being created.  Presumably we've acquired the QP handle
**	as part of translating a query alias.
**
**	We assume that it's not the session that is actually creating
**	the object.  No check to that effect is made.
**
**
** Inputs:
**	obj_p		Pointer to QSO_OBJ_HDR pointer
**			Nulled out on return if object destroyed because
**			someone wanted it destroyed, and we were the last
**			waiter for the object.
**	master_p	Pointer to QSO_MASTER_HDR pointer for named obj
**			(Must be named object, or we never wait)
**			Must enter holding the master mutex, and we exit
**			with it held.  (it may be released in between.)
**			It's also possible that we exit with *master_p = NULL
**			if the object and its master are deleted.
**	err_code	An output
**
** Outputs:
**	obj_p		May be nulled out if object deleted
**	master_p	May be nulled out if object and master deleted
**	err_code	The QSF error that occurred.  One of:
**                              E_QS0000_OK
**				    Operation succeeded.
**
** Returns:
**	E_DB_OK, E_DB_FATAL
**
** Exceptions:
**	None
**
** Side Effects:
**	May destroy the object, and even the master that points to it.
**
** History:
**	20-nov-1992 (rog)
**          Initial creation.
**	27-aug-1993 (rog)
**	    If we can't get the object semaphore, we still have to try to
**	    get the QSF semaphore before exiting, or we will break the above
**	    expected side effects.
**	12-jan-1993 (rog)
**	    If deferred destroy is set, but the caller is not the last one 
**	    waiting on this object, we need to exit with destroyed set to TRUE.
**	03-dec-1998 (somsa01)
**	    Moved decrementation of qso_wait to BEFORE releaseing the qso_psem
**	    semaphore.
**	24-Feb-2005 (schka24)
**	    Just cuz we wake up doesn't mean that the condition is satisfied.
**	    Loop around until object is really ready.
**	17-Dec-2008 (smeke01) b121394
**	    Make sure that we don't wait for QPs that are marked for 
**	    destruction.
**	22-May-2009 (kibro01) b122104
**	    Pass in scb so we can compare an unfinished object's scb against
**	    the excl_owner field to cater for the case where this session
**	    created the object, stopped before producing a query plan, and is
**	    now coming along with another object with the same text using
**	    dynamic query caching.
*/
static DB_STATUS
qso_wait_for(
QSO_OBJ_HDR 	**obj_p,
QSO_MASTER_HDR	**master,
QSF_CB		*scb,
i4		*err_code )
{
    QSO_OBJ_HDR *obj = *obj_p;
    DB_STATUS	status;

    status = E_DB_OK;
    *err_code = E_QS0000_OK;
    if (*master == NULL)
	return (E_DB_OK);

    /* if (the QP is not marked for destruction, AND is in the middle of being built) 
    ** then wait for it to be finished. 
    */
    while (!(obj->qso_status & (QSO_DEFERRED_DESTROY | QSO_LRU_DESTROY)) 
	&& (obj->qso_lk_state == QSO_EXLOCK || obj->qso_root == (PTR)NULL))
    {
	if (obj->qso_root == (PTR)NULL && obj->qso_excl_owner == scb)
	{
	    /* This is ours, which only happens when we've prepared it
	    ** and gone on to something else without producing a query plan.
	    ** The safe thing is to mark it for destruction and start a new one.
	    */
	    obj->qso_status |= QSO_DEFERRED_DESTROY;
	    *obj_p = NULL;
	    return (status);
	}
	/* let anyone that is trying to destroy this query plan know
	** that we are waiting to use it.
	*/
	obj->qso_waiters++;

	CScnd_wait(&(*master)->qsmo_cond, &(*master)->qsmo_sem);

	obj->qso_waiters--;
    }

    /*
    ** This part is tricky.  If QSO_DEFERRED_DESTROY is set, then we
    ** should not see the object.  And, the last session to attempt
    ** to see the object must destroy it.
    */
    if ( obj->qso_status & QSO_DEFERRED_DESTROY &&
	 obj->qso_lk_cur == 0 && obj->qso_waiters == 0 )
    {
	/* Destroy the object, also, perhaps, master. */
	status = qso_rmvobj(obj_p, master, err_code);
    }

    return(status);
}

/*{
** Name: qso_nextbyhandle - Get the next query plan in the chain after the
**			    one specified.
**
** Description:
**	Gets the next QP after the one specified by handle.  The specified
**	object must be share-locked.  More importantly, the current
**	object's master must be mutexed.
**
**	If there is indeed a next object belonging to the same master,
**	we do a wait-for to ensure that it's completely build.  (See
**	commentary for qso_getnext.)  If the object vanishes instead,
**	we'll inform the caller by nulling out the passed-in master ptr.
**
**	This function does not unlock the current object; it expects its
**	caller to do so.
**
** Inputs:
**      obj			The current object
**	qsf_rb			QSF_RCB request block, an output
**	scb			Pointer to the current QSF session CB.
**	master_p		QSO_MASTER_HDR ** ptr to mutexed master ptr
**	err_code		an output
**
** Outputs:
**	qsf_rb			qsf_obj_id, qsf_root, qsf_lk_id filled in
**				with info for next object if any
**	master_p		Can be nulled out if deferred destroy
**	err_code		The QSF error that occurred.  One of:
**                              E_QS0000_OK
**				    Operation succeeded.
**                              E_QS0016_BAD_LOCKSTATE
**                                  Illegal lock state; must be QSO_SHLOCK.
**
** Returns:
**	E_DB_OK, E_DB_ERROR
**
** Exceptions:
**	None
**
** Side Effects:
**	Can release and re-take the master mutex if we wait for
**	another object.  Entire master can disappear if deferred destroy
**	requested while we wait.
**
** History:
**	04-dec-1992 (rog)
**          Initial creation.
*/
static DB_STATUS
qso_nextbyhandle(
QSO_OBJ_HDR 	*obj,
QSF_RCB 	*qsf_rb,
QSF_CB 		*scb,
QSO_MASTER_HDR	**master,
i4		*err_code )
{
    QSO_OBJ_HDR		*new_obj;
    QSO_OBID		*objid = &qsf_rb->qsf_obj_id;
    DB_STATUS		status = E_DB_OK;

    /* The current object must have a shared lock. */
    if (obj->qso_lk_state != QSO_SHLOCK)
    {
	*err_code = E_QS0016_BAD_LOCKSTATE;
	return (E_DB_ERROR);
    }

    new_obj = obj->qso_monext;
    while ( new_obj )
    {
	/* Ignore objects that are set for deferred destruction. */
	if ((new_obj->qso_status & (QSO_DEFERRED_DESTROY | QSO_LRU_DESTROY)) == 0)
	{
	    QSO_OBJ_HDR		*next_obj = new_obj->qso_monext;

	    /* Wait for any QP's in the process of being built. */
	    status = qso_wait_for(&new_obj, master, scb, err_code);
	    if (status != E_DB_OK)
		return(status);
	    
	    /*
	    ** If qso_wait_for() destroyed it instead of waiting for it,
	    ** we need to set the next object in the list correctly and
	    ** continue.
	    ** Because we have the current object share-locked there's no
	    ** danger of the entire list disappearing.
	    */
	    if ( !new_obj )
	    {
		new_obj = next_obj;
		continue;
	    }

	    status = qso_lkset(new_obj, QSO_SHLOCK, &qsf_rb->qsf_lk_id,
			       &qsf_rb->qsf_root, scb, err_code);
	    if (status == E_DB_OK)
		break;

	    DBGSTALL(qsf_rb);
	    /*
	    ** Reset status; we won't report errors from qso_lkset()
	    ** at this point.
	    */
	    status = E_DB_OK;

	}
	new_obj = new_obj->qso_monext;
    }

    if ( new_obj )
    {
	STRUCT_ASSIGN_MACRO(new_obj->qso_obid, *objid);
	new_obj->qso_real_usage++;
	new_obj->qso_decaying_usage++;
	Qsr_scb->qsr_named_requests++;
	*err_code = E_QS0000_OK;
    }
    else
    {
	/* We didn't find a usable next object. */
	/*DBGSTALL(qsf_rb);*/
	*err_code = E_QS0019_UNKNOWN_OBJ;
	status = E_DB_ERROR;
    }

    return (status);
}

/**
** Name: qso_mkobj - Make an object and add it to the object list.
**
** Description:
**	This makes an object and adds it to the list of all objects in QSF, 
**	and increments the number of objects in that list.
**	The object is initialized into EXLOCK lock state, so that the
**	caller has exclusive control at the beginning.  (And, so that the
**	memory grabber won't reclaim it prematurely.)
**
**	There are various cases:
**	Creating an unnamed object: *hash is irrelevant, *master should
**	be null.
**
**	Creating a new named object (no existing master):  *hash should
**	be supplied, and hash bucket should be mutexed.  *master should
**	be null upon entry, will be supplied (and mutexed) at exit.
**
**	Adding another object to an existing master:  *hash should be null,
**	and *master should be supplied.  Master should be mutexed at entry.
**
**	If we had to reclaim memory, and we're creating a named object,
**	it may turn out that someone else created the object while we
**	were off doing the reclaim.  When this happens, we return E_DB_INFO
**	to notify the caller, who may need to take special action.
**	Aside from the E_DB_INFO, all the other returned stuff is as
**	normal (out_obj, master filled in, master mutex held).
**
** Inputs:
**	new_objid	This is the object id for the new object to be created.
**	blksz		block size for the QSF object to be created
**	hash		QSO_HASH_BKT ** pointer to hash bucket ptr where we'll
**			hang the object's master.  Only relevant if it's a
**			brand new named object (no existing master).
**	master		QSO_MASTER_OBJ ** pointer to master ptr for named
**			objects, see above.
** Outputs:
**	out_obj		This will hold the pointer to the new object.
**	err_code	If qso_mkobj() does not return E_DB_OK then err_code
**			will hold an error code for explaination. If qso_mkobj()
**			returns E_DB_OK then err_code will remain unchanged.
**
**	Returns:
**	    E_DB_OK, E_DB_INFO, E_DB_FATAL
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    Adds the object to the object list, so all others will now see it.
**
** History:
**	18-jan-88 (eric)
**          Initial creation, stolen from qso_create actually.
**	29-jan-88 (thurston)
**	    Now clears the LRU bit on an object when it is locked.
**	08-Jan-1992 (fred)
**	    Added initialization of the qso_aeo_* fields.
**	30-nov-1992 (rog)
**	    Added support for master objects and the new accounting info.
**	08-mar-1993 (rog)
**	    Added the scb argument as passed in to qso_mkobj() for better 
**	    performance.
**	09-jul-1993 (rog)
**          Initialize semaphore by calling CSi_semaphore() instead of just
**	    zeroing out bytes.  Also, name the semaphore.
**	28-mar-1994 (rog)
**          Initialize qso_size.
**	22-Feb-2005 (schka24)
**	    Need to re-search after clrmem if named insert.  Deal with
**	    situation where mkindex had to clrmem and then found the
**	    desired object.  Don't set master's qsmo_type until after we
**	    get past all the clrmem poop and are safely holding the master
**	    mutex again.
*/
static DB_STATUS
qso_mkobj( 
QSO_OBJ_HDR	**out_obj, 
QSO_OBID	*new_objid, 
i4		blksz, 
QSF_CB		*scb,
QSO_HASH_BKT	**hash_p,
QSO_MASTER_HDR	**master_p,
i4		*err_code )
{
    bool		new_master;
    ULM_RCB             ulm_rcb;
    DB_STATUS		status;
    QSO_MASTER_HDR	*master;
    QSO_OBJ_HDR		*new_obj;

    for (;;)
    {
	new_master = FALSE;
	/*
	** If we're creating a named object, create/get the master object first
	** to make sure there's enough memory for it.
	*/
	if (new_objid->qso_lname > 0 && *master_p == (QSO_MASTER_HDR*)NULL)
	{
	    new_objid->qso_handle = NULL;
	    status = qso_mkindex(new_objid, master_p, scb, hash_p, err_code);
	    if (status != E_DB_OK)
	    {
		if (status == E_DB_INFO)
		    *out_obj = (QSO_OBJ_HDR *) new_objid->qso_handle;
		return(status);
	    }
	    new_master = TRUE;
	}
	master = *master_p;

	/* Master retrieved successfully; get a memory stream for the new object */

	ulm_rcb.ulm_facility = DB_QSF_ID;
	ulm_rcb.ulm_poolid = Qsr_scb->qsr_poolid;
	ulm_rcb.ulm_blocksize = blksz;
	ulm_rcb.ulm_memleft = &Qsr_scb->qsr_memleft;
	ulm_rcb.ulm_streamid_p = &ulm_rcb.ulm_streamid;
	ulm_rcb.ulm_psize = sizeof(QSO_OBJ_HDR);
	/*
	** If a named object, open a SHARED stream,
	** otherwise, get a PRIVATE stream.
	*/
	if ( master )
	    ulm_rcb.ulm_flags = ULM_SHARED_STREAM | ULM_OPEN_AND_PALLOC;
	else
	    ulm_rcb.ulm_flags = ULM_PRIVATE_STREAM | ULM_OPEN_AND_PALLOC;

	if ( (status = ulm_openstream(&ulm_rcb)) == E_DB_OK )
	    break;		/* It worked */

	/* Release master's semaphore while we purge for memory.
	** We'll have to re-find after reclaiming.
	*/
	if ( master )
	{
	    /* Kill off any new master.  We'll try again after looping.
	    ** If new object on existing master, just un-mutex.
	    */
	    if (new_master)
		(void) qso_rmvindex(master_p, err_code);
	    else
		CSv_semaphore(&master->qsmo_sem);
	}
	master = *master_p = (QSO_MASTER_HDR*)NULL;

	if ( ulm_rcb.ulm_error.err_code != E_UL0005_NOMEM )
	{
	    DBGSTALL2(scb);
	    *err_code = E_QS000C_ULM_ERROR;
	    break;
	}
	/* try to clear the obj queue */
	if ( !qsf_clrmem(scb))
	{
	    /* Nothing left to clear */
	    DBGSTALL2(scb);
	    *err_code = E_QS0001_NOMEM;
	    break;
	}
	/* Since we let go of the master, someone else might have
	** snuck in with a create, so we have to look again.
	** (and take either hash or master mutex again.)
	*/
	if (new_objid->qso_lname > 0
	  && qso_find(new_objid,master_p,hash_p) != NULL)
	{
	    /* One popped up from elsewhere, return it. */
	    *out_obj = (QSO_OBJ_HDR *) new_objid->qso_handle;
	    return (E_DB_INFO);
	}
    };

    if (status == E_DB_OK)
    {	/* We have a good stream and QSO_OBJ_HDR ... fill it in */
	    /******   Start of standard header   *******/
	*out_obj = new_obj = (QSO_OBJ_HDR *) ulm_rcb.ulm_pptr;
	new_obj->qso_obnext = (QSO_OBJ_HDR *) NULL;
	new_obj->qso_obprev = (QSO_OBJ_HDR *) NULL;
	new_obj->qso_length = sizeof(QSO_OBJ_HDR);
	new_obj->qso_type = QSOBJ_TYPE;
	new_obj->qso_owner = (PTR)DB_QSF_ID;
	new_obj->qso_ascii_id = QSOBJ_ASCII_ID;
	new_obj->qso_s_reserved = 0;
	new_obj->qso_l_reserved = 0;
	    /*******   End of standard header   ********/

	new_obj->qso_status = 0;
	new_objid->qso_handle = ulm_rcb.ulm_pptr;
	STRUCT_ASSIGN_MACRO(*new_objid, new_obj->qso_obid);
	new_obj->qso_streamid = ulm_rcb.ulm_streamid;
	new_obj->qso_root = (PTR) NULL;
	new_obj->qso_lk_state = QSO_EXLOCK;
	new_obj->qso_excl_owner = scb;
	new_obj->qso_lk_cur = 1;
	new_obj->qso_lk_id = qso_lknext();
	new_obj->qso_waiters = 0;
	new_obj->qso_real_usage = 0;
	new_obj->qso_decaying_usage = 0;
	new_obj->qso_size = 0;
	new_obj->qso_lrprev = (QSO_OBJ_HDR *) NULL;

	if (new_obj->qso_obid.qso_lname > 0)
	{
	    new_obj->qso_mobject = master;
	    /* It's OK to find this master now */
	    if (new_master)
		master->qsmo_type = QSMOBJ_TYPE;

	    if (master->qsmo_obj_1st == NULL)
	    {
	        master->qsmo_obj_1st = master->qsmo_obj_last = new_obj;
		new_obj->qso_moprev = NULL;
	    }
	    else
	    {
		new_obj->qso_moprev = master->qsmo_obj_last;
		master->qsmo_obj_last->qso_monext = new_obj;
		master->qsmo_obj_last = new_obj;
	    }

	    /* Add this object to the LRU list. */
	    CSp_semaphore(TRUE, &Qsr_scb->qsr_psem);
	    if ( Qsr_scb->qsr_1st_lru )
		Qsr_scb->qsr_1st_lru->qso_lrprev = new_obj;
	    new_obj->qso_lrnext = Qsr_scb->qsr_1st_lru;
	    Qsr_scb->qsr_1st_lru = new_obj;

	    if (++Qsr_scb->qsr_no_named > Qsr_scb->qsr_mx_named)
		Qsr_scb->qsr_mx_named = Qsr_scb->qsr_no_named;
	    CSv_semaphore(&Qsr_scb->qsr_psem);
	}
	else
	{
	    /* Not LRU'able */
	    new_obj->qso_lrnext = (QSO_OBJ_HDR *) NULL;
	    new_obj->qso_mobject = (QSO_MASTER_HDR *) NULL;
	    new_obj->qso_moprev = (QSO_OBJ_HDR *) NULL;
	    if (++Qsr_scb->qsr_no_unnamed > Qsr_scb->qsr_mx_unnamed)
		Qsr_scb->qsr_mx_unnamed = Qsr_scb->qsr_no_unnamed;
	}
	new_obj->qso_monext = (QSO_OBJ_HDR *) NULL;

	new_obj->qso_session = scb;

	/* Maintain some statistics */
	if (++Qsr_scb->qsr_nobjs > Qsr_scb->qsr_mxobjs)
	    Qsr_scb->qsr_mxobjs = Qsr_scb->qsr_nobjs;
    }

    return (status);
}

/**
** Name: qso_lknext - Returns the next available lock id for QSF.
**
** Description:
**      This routine looks at the global QSF SERVER CONTROL BLOCK to get the 
**      next available lock id, and then increments this counter.  If the 
**      counter would overflow, it is recycled to the lowest possible value.
**      Note that there is a chance that someone could get the same lock id, 
**      therefore, but even if this happens, all it means is that they could 
**      accidently unlock someone elses exclusively locked object.  Since 
**      choosing a random number could do the same thing, it is felt that this 
**      method is sufficient.
**
**
** Inputs:
**      none
**
** Outputs:
**      none
**
**	Returns:
**	    Returns a i4  which is the lock id.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    Increments the .qsr_lk_next field of QSF's global SERVER CONTROL
**          BLOCK, which everybody sees.
**
** History:
**	29-apr-86 (thurston)
**          Initial creation.
**	22-Feb-2005 (schka24)
**	    No big deal, but increment the counter thread-safely.
*/

static i4
qso_lknext( void )
{
    i4		lockid = 0;

    /* Avoid zero as a lockid */
    while (lockid == 0)
    {
	lockid = CSadjust_counter(&Qsr_scb->qsr_lk_next,1);
	if (lockid >= MAXI2)
	    (void) CSadjust_counter(&Qsr_scb->qsr_lk_next, MINI2 - MAXI2);
    }

    return (lockid);
}


/**
** Name: qso_mkindex - Create a new stub master header
**
** Description:
**	A new master index header is created and linked into the hash
**	chain.  The master is partially initialized, although the
**	standard header "type" is zeroed out.  The caller has to fix
**	qsmo_type eventually.  Reason:  the caller may have to do a
**	clrmem, necessitating dropping mutexes, and we don't want anyone
**	to find this master until the caller is ready.
**
**	The caller specifies the hash bucket to link the new master to,
**	and is holding the hash mutex.  If we need to reclaim memory,
**	we'll drop the hash mutex and reverify that the named master
**	still doesn't exist before trying again.  Upon exit, the
**	master mutex is locked.
**
**	If we have to reclaim memory, and someone else manages to create
**	the object ID that we were originally looking for, this
**	routine returns E_DB_INFO with the object handle filled into
**	the object id.  (and the master mutex held.)
**
** Inputs:
**	objid		Pointer to the object ID that we need a master for.
**			Can be an output.
**      scb		Session Control Block that is requesting the master.
**	hash		QSO_HASH_BKT ** pointer to ptr to hash
**			bucket where we will insert the master.
**			Must enter with hash mutex locked.
**			(Will drop the mutex.)
**
** Outputs:
**	objid		If E_DB_INFO is returned, the qso_handle points to
**			the object that was created while we were off
**			reclaiming memory
**	master		This will contain the master object that will
**			own any QP's created with the objid object id.
**			The master will be locked.
**	err_code	If qso_mkindex() returns E_DB_ERROR then
**			err_code will hold an error code for explaination.
**			If qso_mkindex() returns E_DB_OK then err_code will
**			remain unchanged.
**
**	Returns:
**	    E_DB_OK, E_DB_INFO, E_DB_ERROR/FATAL
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    Adds the object to the object list for one of the hash buckets, so
**	    all others will now see it.
**
** History:
**	30-nov-1992 (rog)
**          Initial creation.
**	09-jul-1993 (rog)
**          Initialize semaphore by calling CSi_semaphore() instead of just
**	    zeroing out bytes.  Also, name the semaphore.
**	15-oct-1993 (rog)
**          Initialize qsmo_alias to NULL in case this is a master object with
**	    no aliases.
**	21-Oct-2003 (jenjo02)
**	    qso_find() may find index but no object attached to it yet;
**	    that counts as "already exists".
**	22-Feb-2005 (schka24)
**	    Don't fill in object ID here, caller may drop this master if
**	    clrmem needed.  Let caller do it.  Invent INFO return so caller
**	    can handle situation where the object appeared while we were
**	    reclaiming.
*/

static DB_STATUS
qso_mkindex( 
QSO_OBID	*qso_obid, 
QSO_MASTER_HDR	**index, 
QSF_CB		*scb,
QSO_HASH_BKT	**hash,
i4		*err_code )
{
    char		sem_name[CS_SEM_NAME_LEN];
    QSO_HASH_BKT	*bucket;
    QSO_MASTER_HDR	*new_index;
    ULM_RCB		ulm_rcb;
    DB_STATUS		status;

    *index = (QSO_MASTER_HDR*)NULL;
    
    for (;;)
    {
	/* qsb_sem is mutexed */
	bucket = *hash;

	/* We need to create a new index object. */

	ulm_rcb.ulm_facility = DB_QSF_ID;
	ulm_rcb.ulm_poolid = Qsr_scb->qsr_poolid;
	ulm_rcb.ulm_blocksize = sizeof(QSO_MASTER_HDR);
	ulm_rcb.ulm_memleft = &Qsr_scb->qsr_memleft;
	ulm_rcb.ulm_streamid_p = &ulm_rcb.ulm_streamid;
	ulm_rcb.ulm_flags = ULM_SHARED_STREAM | ULM_OPEN_AND_PALLOC;
	ulm_rcb.ulm_psize = sizeof(QSO_MASTER_HDR);

	if ( (status = ulm_openstream(&ulm_rcb)) == E_DB_OK )
	    break;			/* It worked */

	/*
	** We can't hold the hash semaphore while
	** clearing objects from the LRU queue.
	** Release it, try to clear memory, and
	** retry from the top; another thread
	** may have created this index while
	** we were off clearing objects...
	*/
	CSv_semaphore(&bucket->qsb_sem);
	*hash = (QSO_HASH_BKT*)NULL;

	if (ulm_rcb.ulm_error.err_code != E_UL0005_NOMEM)
	{
	    DBGSTALL2(scb);
	    *err_code = E_QS000C_ULM_ERROR;
	    break;
	}
	/* try to clear the obj queue */
	if ( !qsf_clrmem(scb))
	{
	    DBGSTALL2(scb);
	    /* Nothing left to clear */
	    *err_code = E_QS0001_NOMEM;
	    break;
	}
	/* Since we let go of the hash mutex, someone else might have
	** snuck in with a create, so we have to look again.
	** (and take the hash mutex again.)
	*/
	if (qso_find(qso_obid,index,hash) != NULL)
	    return (E_DB_INFO);		/* obid handle filled in */
    };

    if ( status )
    {
	DBGSTALL2(scb);
	return(status);
    }

    *index = new_index = (QSO_MASTER_HDR *) ulm_rcb.ulm_pptr;

    /* Initialize mutex semaphore for this index object -- do now, so
    ** that error cleanup is simplest, also it is double protection
    ** against someone seeing the master in a hash chain with an
    ** un-initialized mutex.
    */
    STprintf(sem_name, "QSF Master %p", new_index);
    if ( CSw_semaphore(&new_index->qsmo_sem, CS_SEM_SINGLE, sem_name) )
    {
	/* Oops.  Clean up the mess. */
	CSv_semaphore(&bucket->qsb_sem);
	ulm_rcb.ulm_facility = DB_QSF_ID;
	ulm_rcb.ulm_streamid_p = &new_index->qsmo_streamid;
	ulm_rcb.ulm_memleft = &Qsr_scb->qsr_memleft;
	ulm_closestream(&ulm_rcb);
	*index = NULL;

	*err_code = E_QS0002_SEMINIT;
	return (E_DB_ERROR);
    }
    
    /* Link the master into the hash chain, but don't make it look
    ** like anything valid yet.  (Hence the zero type.)  The zero
    ** type tells qso_find that this master's no good.
    */
    new_index->qsmo_obj_1st = (QSO_OBJ_HDR *) NULL;
    new_index->qsmo_obj_last = (QSO_OBJ_HDR *) NULL;
    new_index->qsmo_type = 0;
    new_index->qsmo_bucket = bucket;

    /* New index object goes at the front of the chain;  this is
    ** essential so that we don't disturb any qso_find's in progress.
    */

    new_index->qsmo_obprev = (QSO_MASTER_HDR *) NULL;
    new_index->qsmo_obnext = bucket->qsb_ob1st;
    if (bucket->qsb_ob1st)
	bucket->qsb_ob1st->qsmo_obprev = new_index;
    else if (++Qsr_scb->qsr_bkts_used > Qsr_scb->qsr_mxbkts_used)
	    Qsr_scb->qsr_mxbkts_used = Qsr_scb->qsr_bkts_used;
    bucket->qsb_ob1st = new_index;

    /* Release hash semaphore */
    CSv_semaphore(&bucket->qsb_sem);

	/******   Start of standard header   *******/
    new_index->qsmo_length = sizeof(QSO_MASTER_HDR);
    new_index->qsmo_owner = (PTR)DB_QSF_ID;
    new_index->qsmo_ascii_id = QSMOBJ_ASCII_ID;
    new_index->qsmo_s_reserved = 0;
    new_index->qsmo_l_reserved = 0;
	/* qsmo_type skipped on purpose */
	/*******   End of standard header   ********/
    new_index->qsmo_streamid = ulm_rcb.ulm_streamid;

    CSp_semaphore(TRUE, &new_index->qsmo_sem);

    CScnd_init(&new_index->qsmo_cond);

    /* Now that we have it mutexed, fill in stuff that qso_find is
    ** interested in.  Leave the alias wiped, someone else will fill that in.
    */

    STRUCT_ASSIGN_MACRO(*qso_obid, new_index->qsmo_obid);
    MEfill(sizeof(QSO_OBID), 0, &new_index->qsmo_aliasid);
    new_index->qsmo_session = scb;

    /* Maintain some statistics */
    if (++bucket->qsb_nobjs > bucket->qsb_mxobjs)
	bucket->qsb_mxobjs = bucket->qsb_nobjs;
    if (bucket->qsb_mxobjs > Qsr_scb->qsr_bmaxobjs)
	Qsr_scb->qsr_bmaxobjs = bucket->qsb_mxobjs;
    if (++Qsr_scb->qsr_no_index > Qsr_scb->qsr_mx_index)
	Qsr_scb->qsr_mx_index = Qsr_scb->qsr_no_index;
    if (++Qsr_scb->qsr_nobjs > Qsr_scb->qsr_mxobjs)
	Qsr_scb->qsr_mxobjs = Qsr_scb->qsr_nobjs;

    /* Return with index's qsmo_sem held */
    *hash = (QSO_HASH_BKT*)NULL;
    return (E_DB_OK);
}


/**
** Name: qso_find - Find a QSO object with the given name and type.
**
** Description:
**      This routine will return TRUE if a QSF object exists with the supplied
**      name and type (these are supplied in a QSO_OBID structure), and FALSE
**	if not.  If an object is found, the handle portion of the supplied
**	object id will be filled in.
**
**	If the object is not found, the caller can request that we
**	return the hash bucket pointer where the object ID hashes to,
**	along with holding the hash mutex.  This should be done if the
**	caller will create a new object if not found.  Callers that
**	are just searching should pass NULL for the hash_p argument.
**
** Inputs:
**      qso_obid                        Object id.
**		.qso_lname		Length in bytes of the object name
**					given in .qso_name.  If zero, this
**					is an un-named object, and thus will
**					not be found.
**		.qso_name		Name of object.
**		.qso_type		Type of object.
**					If the type is QSO_ALIAS_OBJ, we are
**					to match against the master alias ID.
**					For any other type, we match against
**					the master regular ID (qsmo_obid).
**	master_p			An output
**	hash_p				If NULL, tells qso_find to not try
**					to return any hash info (lookup
**					only, not going to create later).
**
** Outputs:
**      qso_obid                        Object id.
**		.qso_handle		Handle for the object, if an object
**					is found.
**	master_p			Master object for the QP, if one is
**					found.
**	hash_p				Where to put hash bucket pointer, if
**					no object is found;  hash mutex
**					will be held (if hash_p != NULL).
**					If object IS found, *hash_p is set
**					to NULL.
**
**	Returns:
**	    object address if found
**	    NULL if not found
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	29-apr-86 (thurston)
**          Initial creation
**	21-aug-87 (thurston)
**	    Speeded up the search by looking at first 8 bytes of name as two
**	    i4's.  This will avoid excessive calls to MEcmp().  
**	29-jan-88 (thurston)
**	    Modified to use the qso_hash() routine and the hash buckets, which
**	    have replaced the single object queue in QSF.
**	30-nov-1992 (rog)
**	    Reworked function to find master objects in the hash table and to
**	    find the object based on those master objects.
**	15-oct-1993 (rog)
**	    We should not find objects marked for deferred destruction.
**	21-Oct-2003 (jenjo02)
**	    Watch for masters in the process of being constructed which
**	    as yet have no objects to avoid SEGV.
**	29-Jan-2004 (jenjo02)
**	    Plug race conditions in which a Master was in the
**	    process of being deleted from the hash list while
**	    we are looking at it.
**	31-Mar-2004 (jenjo02)
**	    Skip master's which have no qsmo_obj_1st.
**	22-Feb-2005 (schka24)
**	    Alias is no longer a separate object, find alias in the
**	    master index header.  Be slightly pickier with hash mutexing.
*/

static QSO_OBJ_HDR *
qso_find( 
QSO_OBID 	*qso_obid,
QSO_MASTER_HDR	**master_p,
QSO_HASH_BKT	**hash)
{
    QSO_HASH_BKT	*bucket;
    QSO_MASTER_HDR	*master, *nmaster, *fmaster;
    QSO_MASTER_HDR	*alias_master;
    QSO_OBJ_HDR		*obj;
    i4			lname, type;

    /* 
    ** If master is found, it is returned mutexed.
    **
    ** If not found and "hash" is provided (create),
    ** hash is returned mutexed.
    */

    *master_p = (QSO_MASTER_HDR*)NULL;
    if ( hash ) 
	*hash = (QSO_HASH_BKT*)NULL;

    /* Unnamed objects don't hash */
    lname = qso_obid->qso_lname;
    if ( lname <= 0 )
	return (NULL);

    /* if this type is QSO_ALIAS_OBJ we are supposed to match
    ** against the alias ID, otherwise match against the object ID.
    */
    type = qso_obid->qso_type;

    bucket = qso_hash(qso_obid);

    CSp_semaphore(TRUE, &bucket->qsb_sem);
    /* Prevent deletions while we read the list */
    bucket->qsb_init++;

    /* Outer loop retries search if nomatch and something got inserted
    ** at the front.  Inner loop searches the hash chain.
    */

    do
    {
	fmaster = bucket->qsb_ob1st;
	CSv_semaphore( &bucket->qsb_sem);
	/* the hash chain may now change, but only at the front;
	** we can search beyond fmaster without fear.
	*/
	for ( master = fmaster; 
	      master; 
	      master = nmaster )
	{
	    nmaster = master->qsmo_obnext;

	    /* Skip those being created or destroyed (type = 0) */
	    if ( master->qsmo_type == QSMOBJ_TYPE &&
		 master->qsmo_obj_1st)
	    {
		/* Check out object or alias ID depending on what we're
		** looking for.
		*/
		if (type == QSO_ALIAS_OBJ)
		{
		    if (master->qsmo_aliasid.qso_type != type
		      || master->qsmo_aliasid.qso_lname != lname
		      || MEcmp((PTR) master->qsmo_aliasid.qso_name,
			       (PTR) qso_obid->qso_name, lname) != 0)
			continue;
		}
		else
		{
		    if (master->qsmo_obid.qso_type != type
		      || master->qsmo_obid.qso_lname != lname
		      || MEcmp((PTR) master->qsmo_obid.qso_name,
			       (PTR) qso_obid->qso_name, lname) != 0)
			continue;
		}
		/* Preliminary match.  Mutex the master and re-check.
		** Someone could be deleting this master, but they can't
		** finalize the delete until we're out of here.
		*/
		CSp_semaphore(TRUE, &master->qsmo_sem);
		if ( master->qsmo_type == QSMOBJ_TYPE &&
		     (obj = master->qsmo_obj_1st) != NULL)
		{
		    if (type == QSO_ALIAS_OBJ)
		    {
			if (master->qsmo_aliasid.qso_type != type
			  || master->qsmo_aliasid.qso_lname != lname
			  || MEcmp((PTR) master->qsmo_aliasid.qso_name,
				   (PTR) qso_obid->qso_name, lname) != 0)
			{
			    CSv_semaphore(&master->qsmo_sem);
			    continue;
			}
		    }
		    else
		    {
			if (master->qsmo_obid.qso_type != type
			  || master->qsmo_obid.qso_lname != lname
			  || MEcmp((PTR) master->qsmo_obid.qso_name,
				   (PTR) qso_obid->qso_name, lname) != 0)
			{
			    CSv_semaphore(&master->qsmo_sem);
			    continue;
			}
		    }

		    /*
		    ** We found the right master header, and it has at
		    ** least one object belonging to it (so it's not being
		    ** created or destroyed.)  Now we need to make sure that
		    ** there's an object belonging to the master that isn't
		    ** marked for destruction.
		    */

		    while ( obj && obj->qso_status & (QSO_DEFERRED_DESTROY | QSO_LRU_DESTROY) )
			obj = obj->qso_monext;

		    if (obj == NULL)
		    {
			/* We got the right header, but its objects are
			** all marked for destruction.
			** Pretend there ain't nothing there.
			*/
			CSv_semaphore(&master->qsmo_sem);
			break;
		    }

		    /* This is the one! */
		    qso_obid->qso_handle = (PTR) obj;
		    *master_p = master;

		    /* We're done, let deletes proceed */
		    CSp_semaphore(TRUE, &bucket->qsb_sem);
		    bucket->qsb_init--;
		    CSv_semaphore(&bucket->qsb_sem);

		    /* Return with master's qsmo_sem locked */
		    return(obj);
		}
		/* Master isn't any good, keep looking */
		CSv_semaphore(&master->qsmo_sem);
	    }
	} /* for */
	/* If we fell/broke out of the search loop, we didn't find it.
	** If stuff got inserted at the front, try again.
	*/
	CSp_semaphore(TRUE, &bucket->qsb_sem);
    } while (bucket->qsb_ob1st != fmaster);

    /* We're done, nomatch, let deletes proceed */
    bucket->qsb_init--;

    /* If wanted, return with hash semaphore (create) */
    if ( hash )
	*hash = bucket;
    else
	CSv_semaphore(&bucket->qsb_sem);

    return (NULL);
}

/**
** Name: qso_lkset - Lock an object.
**
** Description:
**      This routine is the low level object locker.  If it's a
**	named object, the object's master's mutex should be held
**	by the caller to prevent conflicts.
**
** Inputs:
**      obj				Ptr to object header.
**	lock_req			Requested lock state.
**
** Outputs:
**      lock_id				Lock ID.
**      root				Root address of locked object.
**	err_code			The QSF error that occurred.  One of:
**				E_QS0000_OK
**				    Operation succeeded.
**                              E_QS000D_CORRUPT
**				    QSF's object store has been corrupted.
**                              E_QS000F_BAD_HANDLE
**				    Supplied handle does not refer to an object.
**                              E_QS0014_EXLOCK
**                                  Object is already locked exclusively.
**                              E_QS0015_SHLOCK
**                                  Object is already locked shared.
**                              E_QS0016_BAD_LOCKSTATE
**                                  Illegal lock state; must be QSO_EXLOCK or
**                                  QSO_SHLOCK.
**
**	Returns:
**	    E_DB_OK
**	    E_DB_WARN
**	    E_DB_ERROR
**	    E_DB_FATAL
**	    
**
**	Exceptions:
**	    none
**
** Side Effects:
**          If locking an exclusively locked object, the object will be
**          taken out of the QSO_FREE state and placed in the QSO_EXLOCK state
**          so that no other clients will be able to get either shared or
**          exclusive locks on the object.  If locking an object for shared
**          access, the number of share locks on that object will be
**          incremented.  If this number had been zero, the object will be taken
**          out of the QSO_FREE state and placed in the QSO_SHLOCK state,
**          meaning that other clients can get share locks on the object, but
**          cannot get an exclusive lock. 
**
** History:
**	05-aug-87 (thurston)
**          Initial creation
**	28-aug-1992 (rog)
**	    Change for(;;) to do/while(0) to silence compiler warning.
**	30-oct-1992 (rog)
**	    No need to set the LRU bit anymore.
*/

static DB_STATUS
qso_lkset(
QSO_OBJ_HDR 	*obj,
i4		lock_req,
i4  		*lock_id,
PTR 		*root,
QSF_CB		*scb,
i4 		*err_code )
{
    i4			*lk_state = &obj->qso_lk_state;
    i4			*lk_cur = &obj->qso_lk_cur;
    i4			*lk_id = &obj->qso_lk_id;
    i4			*obj_status = &obj->qso_status;

    *err_code = E_QS0000_OK;

    switch (lock_req)	/* switch on requested lock state */
    {
	case QSO_EXLOCK:
	    switch (*lk_state)	    /* switch on object's */
	    {			    /* actual lock state  */
		case QSO_EXLOCK:
		    DBGSTALL2(scb);
		    *err_code = E_QS0014_EXLOCK;
		    break;
		case QSO_SHLOCK:
		    DBGSTALL2(scb);
		    *err_code = E_QS0015_SHLOCK;
		    break;
		case QSO_FREE:

		    /* All OK ... lock the object exclusively */
		    /* -------------------------------------- */

		    obj->qso_excl_owner = scb;
		    *lk_state = QSO_EXLOCK;
		    *lk_cur = 1;
		    *lk_id = *lock_id = qso_lknext();
		    *root = obj->qso_root;
		    break;
		default:
		    DBGSTALL2(scb);
		    *err_code = E_QS000D_CORRUPT;
		    break;
	    }
	    break;
	case QSO_SHLOCK:
	    switch (*lk_state)
	    {
		case QSO_EXLOCK:
		    /*DBGSTALL2(scb);*/
		    *err_code = E_QS0014_EXLOCK;
		    break;
		case QSO_SHLOCK:
		case QSO_FREE:

		    /* All OK ... lock the object shared */
		    /* --------------------------------- */

		    *lk_state = QSO_SHLOCK;
		    (*lk_cur)++;
		    *root = obj->qso_root;
		    break;
		default:
		    DBGSTALL2(scb);
		    *err_code = E_QS000D_CORRUPT;
		    break;
	    }
	    break;
	default:
	    DBGSTALL2(scb);
	    *err_code = E_QS0016_BAD_LOCKSTATE;
	    break;
    }

    return( (*err_code) ? E_DB_ERROR : E_DB_OK );
}


/**
** Name: qso_hash() - Returns ptr to appropriate hash bucket for an obj ID.
**
** Description:
**      This is the hashing function for QSF's hash table of objects.  It hashes
**	an object ID into one of the available QSF hash buckets.
**
**	Only named, master objects get entered into the hash table, so
**	qso_hash() should never be called with an lname == 0.
**	
**	It is essential that object ID's and object alias ID's hash to
**	the same hash bucket.  In order for this to work, we can only
**	hash a limited subset of the object name;  specifically, the
**	text part of the DB_CURSOR_ID part of the name.  That's the
**	only part of an object ID that is guaranteed to match its alias.
**
** Inputs:
**      objid				Object ID to be hashed.
**	    .qso_lname
**	    .qso_name
**
** Outputs:
**      none
**
**	Returns:
**	    Ptr to appropriate hash bucket.
**	    If an error is detected, it will return NULL.
**	    
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    Initializes qsb_sem on first hash to bucket.
**
** History:
**	29-jan-88 (thurston)
**          Initial creation
**	30-nov-1992 (rog)
**          Allowed function to hash into bucket 0.
**	03-aug-1993 (rog)
**	    Replace the hash function with one that spreads things out more
**	    evenly.  (This new function is based on a function from the section
**	    on hashing in the red dragon compiler book.)
**	21-sep-1993 (rog)
**	    Add definitions for variables used inside some xDEBUG code.
**	13-may-1994 (rog)
**	    Speed up the hashing function by only hashing on the name.
**	09-may-1996 (kch)
**	    We now check for DB_MAXNAME (32) characters in the object name
**	    as well as the first blank, by incrementing the counter n each
**	    time name is incremented, and then checking that n is less
**	    than DB_MAXNAME. This change fixes bug 75638.
**	26-Sep-2003 (jenjo02)
**	    Hash using HSH_char(), the Ingres standard.
**	    On first use of hash bucket, init its semaphore.
*/

static QSO_HASH_BKT *
qso_hash( QSO_OBID *objid )
{
    PTR			name = (char *) &objid->qso_name[0];
    i4			length;
    QSO_HASH_BKT	*hash;

    /*
    ** This is ugly, but let's skip the two i4's that are at the beginning
    ** of a DB_CURSOR_ID and just hash on the name.
    */
    /*
    ** Bug 75638 - We now only hash DB_MAXNAME characters of the
    **		   name -- omit any DBP owner or db id from the hash.
    **
    ** Throw anything less than 8 bytes into bucket zero.
    */
    if ( (length = objid->qso_lname - 2 * sizeof(i4)) > 0 )
    {
	hash = Qsr_scb->qsr_hashtbl +
		(u_i4)((HSH_char(name + 2 * sizeof(i4),
		     (length > DB_MAXNAME) ? DB_MAXNAME 
					   : length))
		    % Qsr_scb->qsr_nbuckets);
    }
    else
	hash = Qsr_scb->qsr_hashtbl;

    /* Init semaphore on first use of hash bucket */
    if ( !hash->qsb_init )
    {
	/* Block concurrent hashers */
	CSp_semaphore(1, &Qsr_scb->qsr_psem);
	if ( !hash->qsb_init )
	{
	    char	sem_name[CS_SEM_NAME_LEN];
	    CSw_semaphore(&hash->qsb_sem, CS_SEM_SINGLE,
	       STprintf(sem_name, "QSF hash %p", (PTR)hash));
	    hash->qsb_init = TRUE;
	}
	CSv_semaphore(&Qsr_scb->qsr_psem);
    }
    return(hash);
}
