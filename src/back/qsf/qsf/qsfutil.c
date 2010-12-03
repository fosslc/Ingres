/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <scf.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <qsf.h>
#include    <qsfint.h>

/**
**
**  Name: QSFUTIL.C - Utilities used by QSF
**
**  Description:
**      This file contains several utility routines used by the various pieces
**	of QSF.
**
**	Externally visible functions defined in this module:
**
**          qsf_clrmem() - Clear obsolete objects from the QSF object queue.
**	    qsf_set_decay() - Sets the decay factor for LRU objects.
**
**	Static functions defined in this module:
**
**	    qs0_clrobj() - Vaporizes an object, resets LRU info, etc.
**
**
**  History:    $Log-for RCS$
**      28-may-87 (thurston)
**          Initial creation.
**	05-jul-87 (thurston)
**	    Added code to qsf_clrmem() to check for server wide tracing before
**	    even attempting to look for a particular trace flag.
**	28-nov-87 (thurston)
**	    Changed semaphore code to use CS calls instead of going through SCF.
**	27-jan-88 (thurston)
**	    Rewrote the qsf_clrmem() routine to use the new LRU list/bit
**	    algorithm, and added the static routine, qs0_clrobj().
**	28-aug-1992 (rog)
**	    Prototype QSF.
**	20-oct-1992 (rog)
**	    Change the LRU algorithm to delete the object with the lowest
**	    usage count.
**	27-oct-1992 (rog)
**	    Added qsf_session().
**	27-oct-1992 (rog)
**	    Added qsf_set_decay().
**      28-jan-1997 (hanch04)
**          Cross integrate porting change
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Jan-2001 (jenjo02)
**	    qsf_session() function replaced with GET_QSF_CB macro,
**	    Added *QSF_CB to qsf_clrmem() prototype.
**      18-Feb-2005 (hanal04) Bug 112376 INGSRV2837
**          Added qsf_clrsesobj() which allows a session to flush any
**          objects it owns in the QSF cache.
**	9-feb-2007 (kibro01) b113972
**	    Added parameter to qso_rmvobj to determine if we have a lock on
**	    qsr_psem, which allows us to update the LRU queue.
**	11-feb-2008 (smeke01) b113972
**	    Backed out above change.
**	29-Oct-2010 (jonj) SIR 120874
**	    Use DB_ERROR instead of err_code
**/


/*
**  Definition of static variables and forward static functions.
*/

static bool qs0_clrobj( bool trace_008, QSF_CB *scb, QSO_OBJ_HDR *obj );


/*{
** Name: qsf_clrmem() - Clear obsolete objects from the QSF object queue.
**
** Description:
**      This routine runs through the QSF object queue clearing out any obsolete
**	objects.
**        
** Inputs:
**      sem_set				A boolean.  If TRUE, QSF's server CB
**					semaphore has already been set by the
**					caller.  If FALSE, it has not been, and
**					therefore must be set by this routine.
**
** Outputs:
**	none
**
**	Returns:
**	    TRUE	Some object was cleared from the queue.
**	    FALSE	No object was cleared from the queue.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	28-may-87 (thurston)
**          Initial creation.
**	05-jul-87 (thurston)
**	    Added code to check for server wide tracing before even attempting
**	    to look for a particular trace flag.
**	28-nov-87 (thurston)
**	    Changed semaphore code to use CS calls instead of going through SCF.
**	23-jan-88 (eric)
**	    Added support for QSO_TRANS_OR_DEFINE alias objects. This mostly
**	    means making sure that no one is waiting for objects before
**	    destroying them. If someone is waiting for an object, via a 
**	    semaphore inside the object, them we shouldn't get rid of it
**	    underneath them (otherwise they may never wake up).
**	27-jan-88 (thurston)
**	    Rewrote this routine to use the new LRU circular list, and the new
**	    LRU bit in the object header.
**	20-oct-1992 (rog)
**	    Change the LRU algorithm to delete the object with the lowest
**	    usage count.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	10-Jan-2001 (jenjo02)
**	    Added *QSF_CB to prototype.
**	26-Sep-2003 (jenjo02)
**	    Function now returns #objects destroyed.
**	    qsr_psem now just blocks the LRU queue, not all of QSF.
**	    Continue destroying objects until 1/2 of QSF memory 
**	    is freed up.
**	21-Oct-2003 (jenjo02)
**	    Corrected "while" predicate, which was not looping as
**	    intended.
**	22-Feb-2005 (schka24)
**	    Avoid race by test/set lru-delete flag while mutex held.
**	11-Jul-2005 (schka24)
**	    Remove inappropriate x-integration for b112813 (fixed by above).
*/

i4
qsf_clrmem( QSF_CB *scb )
{
    QSO_OBJ_HDR         *obj;
    QSO_OBJ_HDR         *victim;
    bool		trace_008 = FALSE;
    i4			decay_amount;
    i4			destroyed = 0;


#ifdef    xDEBUG
    trace_008 = (Qsr_scb->qsr_tracing && qst_trcheck(scb, QSF_008_CLR_OBJ));
    if (trace_008)
    {
	TRdisplay(">>>>> Looking for object to clear from QSF memory ...");
    }
#endif /* xDEBUG */

	
    /* Loop until we find a victim and destroy it */
    do
    {
	/* Freeze the LRU queue */
	CSp_semaphore((i4) TRUE, &Qsr_scb->qsr_psem);

	decay_amount = 0;

	if ( Qsr_scb->qsr_named_requests )
	{
	    if ( Qsr_scb->qsr_no_named * Qsr_scb->qsr_decay_factor )
		decay_amount = 
		 Qsr_scb->qsr_named_requests / 
		     (Qsr_scb->qsr_no_named * Qsr_scb->qsr_decay_factor);

	    if (decay_amount <= 0)
		decay_amount = 1;
	}

	victim = (QSO_OBJ_HDR *) NULL;
	obj = Qsr_scb->qsr_1st_lru;

	while ( obj )
	{
	    if ((!victim
		 || (obj->qso_decaying_usage < victim->qso_decaying_usage))
		&& obj->qso_lk_state == QSO_FREE
		&& obj->qso_waiters == 0
		&& obj->qso_mobject != NULL
		&& (obj->qso_status & QSO_LRU_DESTROY) == 0)
		    victim = obj;
	    
	    if ( (obj->qso_decaying_usage -= decay_amount) < 0 )
		obj->qso_decaying_usage = 0;

	    obj = obj->qso_lrnext;
	}

	/* If we have a victim, mark it while mutexed, so that a
	** simultaneous destroy or clrmem in another session won't pick
	** this object at the same time.
	*/
	if (victim != NULL)
	    victim->qso_status |= QSO_LRU_DESTROY;

	/* Unlock the LRU queue */

	CSv_semaphore(&Qsr_scb->qsr_psem);

	/* Do we have a victim to toss out? */
	if ( victim && qs0_clrobj(trace_008, scb, victim) )
	    destroyed++;

    } while ( victim && Qsr_scb->qsr_memleft < Qsr_scb->qsr_memtot / 2 );

    return (destroyed);
}

/*{
** Name: qsf_clrsesobj() - Clear objects owned by the calling session from 
**      the QSF object queue.
**
** Description:
**      This routine runs through the QSF object queue clearing out any 
**	(named) objects owned by the session.
**        
** Inputs:
**      qsf_rb		QSF request block
**	    qsf_sid	Session ID, to be set by caller.
**	    qsf_scb	QSF session CB, set by qsf-call.
**
** Outputs:
**	none
**
**	Returns:
**	E_DB_OK or error status.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	17-Feb-2005 (hanal04) Bug 112376 INGSRV2837
**          Initial creation.
**      30-Nov-2007 (huazh01)
**          Always hold Qsr_scb->qsr_psem while cleaning
**          up the object.
**          bug 116201. 
**      07-jan-2008 (huazh01)
**          restart from the beginning of the list after
**          destroying an object.
**          bug 119688.
**      10-apr-2008 (huazh01)
**          back out the fix to bug 116201 and 119688. 
**          due to various changes that has been made into
**          QSF, those ingres 2.6 changes are not compatible 
**          on 9.x code line. (bug 120240). 
**	26-May-2010 (kschendel) b123814
**	    Run this via the uncommitted session objects list.
*/

DB_STATUS
qsf_clrsesobj( QSF_RCB *qsf_rb )
{
    QSO_OBJ_HDR         *obj, *prev;
    QSF_CB              *scb = qsf_rb->qsf_scb;
    QSF_RCB		qsf_rcb;
    DB_ERROR		localDBerr;

    for (;;)
    {
	/* Freeze the list so some other session trying a clrmem
	** doesn't sneak in.
	*/
	CSp_semaphore((i4) TRUE, &Qsr_scb->qsr_psem);

	obj = scb->qss_snamed_list;
	prev = NULL;
	while ( obj != NULL )
	{
	    if ((obj->qso_status & (QSO_DEFERRED_DESTROY | QSO_LRU_DESTROY)) == 0)
		break;
	    prev = obj;
	    obj = obj->qso_snamed_list;
	}
	if (obj == NULL)
	    break;
	/* Unlink the victim while the list is frozen */
	if (prev == NULL)
	    scb->qss_snamed_list = obj->qso_snamed_list;
	else
	    prev->qso_snamed_list = obj->qso_snamed_list;
	obj->qso_snamed_list = NULL;

	/* If we have a victim, mark it while mutexed, so that a
	** simultaneous destroy or clrmem in another session won't pick
	** this object at the same time.  This also ensures that if
	** the object can't be removed right now (maybe someone else
	** has already seen / is using it), it won't last long,
	** and won't cause an infinite loop here.
	*/
	obj->qso_status |= QSO_LRU_DESTROY;
	CSv_semaphore(&Qsr_scb->qsr_psem);
	/* Ignore delete error */
	(void) qso_rmvobj(&obj, NULL, &localDBerr);

	/* Loop back to start over, a clrmem session might have taken
	** other stuff off our list.
	*/
    }

    /* Loop exits with psem held if the list is empty (or everything on
    ** it is already targeted for deletion by other sessions).
    */
    scb->qss_snamed_list = NULL;
    CSv_semaphore(&Qsr_scb->qsr_psem);

    return (E_DB_OK);
}

/*{
** Name: qs0_clrobj() - Vaporizes an object, resets LRU info, etc.
**
** Description:
**      This is the internal routine used by qsf_clrmem() to actually get rid of
**	an object that it has decided is the one to burn.
**        
** Inputs:
**	trace_008			<<<  *** ONLY USED FOR xDEBUG ***  >>>
**					The setting of the QSF trace point #8.
**	scb				<<<  *** ONLY USED FOR xDEBUG ***  >>>
**					Ptr to QSF session control block.  This
**					is only used for tracing and can be NULL
**					if not yet known.
**	victim				Ptr to object to be cleared.
**
** Outputs:
**	none
**
**	Returns:
**	    TRUE	Specified object was cleared from QSF memory.
**	    FALSE	Object was NOT cleared.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	27-jan-88 (thurston)
**          Initial creation.
**	06-jan-1993 (rog)
**	   Added the qsr_no_destroyed counter.
*/

static bool
qs0_clrobj( bool trace_008, QSF_CB *scb, QSO_OBJ_HDR *victim )
{
    QSO_OBJ_HDR		*obj = victim;
    DB_ERROR		localDBerr;
#ifdef    xDEBUG
    QSF_RCB		qsf_rb;

    if (trace_008)
    {

	STRUCT_ASSIGN_MACRO(obj->qso_obid, qsf_rb.qsf_obj_id);
	(void) qsd_obj_dump(&qsf_rb);
    }	    
#endif /* xDEBUG */

    if ( qso_rmvobj(&obj, (QSO_MASTER_HDR**)NULL, &localDBerr) )
    {
	/*
	** For some reason, qso_rmvobj() failed.  Instead of bombing
	** out here, just continue on with the loop, and see if we
	** can remove anything else.
	*/
	
#ifdef    xDEBUG
	if (trace_008)
	{
	    TRdisplay("*** ... qso_rmvobj() FAILED with err code 0x%8x ***\n",
			localDBerr.err_code);
	}
#endif /* xDEBUG */
	return(FALSE);
    }

    Qsr_scb->qsr_no_destroyed++;

    return (TRUE);
}

/*{
** Name: qsf_set_decay() - Sets qsr_decay_factor in the QSR_CB structure
**
** Description:
**	This function sets the decay factor for LRU objects by
**	setting the qsr_decay_factor in the QSR_CB structure.
**        
** Inputs:
**	decay_factor			Value to set the decay factor to.
**
** Outputs:
**	None
**
** Returns:
**	OK				Valid value given as input.
**	FAIL				Bad input value.
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**	07-jan-1993 (rog)
**          Initial creation.
*/

STATUS
qsf_set_decay( i4  decay_factor )
{
    if (decay_factor < 1 || decay_factor > QSF_MAX_DECAY)
	return(FAIL);

    Qsr_scb->qsr_decay_factor = decay_factor;

    return(OK);
}
