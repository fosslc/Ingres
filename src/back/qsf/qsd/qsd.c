/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cm.h>
#include    <cs.h>
#include    <me.h>
#include    <tr.h>
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
**  Name: QSD.C - Debug routines for the Query Storage Facility (QSF).
**
**  Description:
**      This file contains all routines used for debugging the Query Storage
**	Facility. 
**
**      This file includes the following routines:
**          qsd_obj_dump() - Dump a QSF object.
**	    qsd_obq_dump() - Dump QSF's object queue.
**
**
**  History:    $Log-for RCS$
**      05-may-86 (thurston)    
**          Initial creation.
**	24-jun-86 (thurston)
**	    Modified code to comply with new SCF_CB.  This allowed setting the
**	    semaphore in qsd_obq_dump() as a SHARED semaphore, which should help
**	    performance.
**	02-mar-87 (thurston)
**	    GLOBALDEF changes.
**	02-mar-87 (thurston)
**	    Added the SQLDA type to a TRdisplay in qsd_obj_dump().
**	27-may-87 (thurston)
**	    Added the qsd_bo_dump() and qsd_boq_dump() routines.
**	13-oct-87 (thurston)
**	    Converted CH routines to CM routines.
**	28-nov-87 (thurston)
**	    Changed semaphore code to use CS calls instead of going through SCF.
**	16-mar-88 (thurston)
**	    Now generates a memory pool map after object queue dump if trace
**	    point QS9 is set.
**	28-aug-92 (rog)
**	    Prototype QSF.
**	30-nov-92 (rog)
**	    Include <ulf.h> before <qsf.h>.  Include <scf.h> for session info.
**	17-oct-94	(ramra01)
**		Init scb variables in qsd_obq_dump & qsd_boq_dump.
**		The calling function will update this. b65254
**	15-august-95 (angusm)
**		bug 70592 - trace point qs007 causes SEGV in qsd_boq_dump()
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-Sep-2003 (jenjo02)
**	    qsr_psem now only used for LRU queue; each hash bucket,
**	    when in use, has its own qsb_sem.
**	23-Feb-2005 (schka24)
**	   Fix up obj dumper a little;  kill "brief" dumpers.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
*/

static void trdisp_objname(char *str, QSO_OBID *obid);



/*{
** Name: QSD_OBJ_DUMP - Dump a QSF object.
**
** Description:
**      Given the object's handle, this routine format's the object header 
**      and sends it to the trace output file.
**
** Inputs:
**	QSD_OBJ_DUMP			Op code to qsf_call().
**	qsf_rb                          QSF request block.
**		.qsf_obj_id
**			.qso_handle	The handle for the object to dump.
**
** Outputs:
**      qsf_rb                          QSF request block.
**		.qsf_error		Filled in with error if encountered.
**                      .err_code       The QSF error that occurred.  One of:
**				E_QS0000_OK
**				    All is fine.
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
**	    Writes to the trace output file.
**
** History:
**	05-apr-86 (thurston)
**          Initial creation.
**	02-mar-87 (thurston)
**	    Added the SQLDA type to a TRdisplay.
**	13-oct-87 (thurston)
**	    Converted CH routines to CM routines.
**	30-nov-1992 (rog)
**	    Various cleanup in support of the new object header fields.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	 7-jan-94 (swm)
**	    Bug #58635
**	    Made owner PTR for compatibility with qso_owner.
**	01-Oct-2009 (smeke01) b122667
**	    Clearly separated sections for dumping fields common to 
**	    both QSO_OBJ_HDR and QSO_MASTER_HDR, and those that are 
**	    specific to each, using the common field qso_type to 
**	    decide.
*/

DB_STATUS
qsd_obj_dump( QSF_RCB *qsf_rb )
{
    QSO_OBJ_HDR         *obj = (QSO_OBJ_HDR *) qsf_rb->qsf_obj_id.qso_handle;
    QSO_MASTER_HDR	*master;

    /* The first section below prints fields common to QSO_OBJ_HDR and QSO_MASTER_HDR */
    /* Example output:
*
*---------------------------------------
*Dump of QSF object at 0x5FE325E0:
*
*    Start of Standard Header
*       Next object = 00000000            Prev object = 00000000
*       Std. length =      184              Std. type =        1
*        Std. owner = QSF               Std. ascii id =     QSOH
*    End of Standard Header
*
*
    */

    qsf_rb->qsf_error.err_code = E_QS0000_OK;
    TRdisplay("\n%39*-\n");
    TRdisplay("Dump of QSF object at 0x%8x:\n\n", 
	obj);
    TRdisplay("    Start of Standard Header\n");
    TRdisplay("\tNext object = %p\t\t  Prev object = %p\n",
	obj->qso_obnext, obj->qso_obprev);
    TRdisplay("\tStd. length = %8d\t\t    Std. type = %8d\n",
	obj->qso_length, obj->qso_type);
    TRdisplay("\t Std. owner = %8w\t\tStd. ascii id =     %4.4s\n",
	    ",CLF,ADF,DMF,OPF,PSF,QEF,QSF,RDF,SCF,ULF",
	    obj->qso_owner, (char *)&obj->qso_ascii_id);
    TRdisplay("    End of Standard Header\n\n");

    /* This next section prints fields specific to a QSO_OBJ_HDR or a QSO_MASTER_HDR */
    if (obj->qso_type == QSOBJ_TYPE || obj->qso_type == QSOBJ_DBP_TYPE)
    {
	/* We're a QSO_OBJ_HDR */

	/* Example output:
*    Start of QSO_OBJ_HDR specific fields
*       Object type = QP
*       Object name: (40) (3,1) p1
*       Alias name: (76) (0,0) p1 ingres (0x4AC33616)
*       Master header = 0x5FE32AF0
*       Object size =     2772  Lock id =        0      Lock Usage count = 0
*       Real usage =        1   Decay usage =        1  Waiters = 0
*       Streamid = 0x5FE324E0   Root = 0x5FE32698       Session = 0x40342284
*       Status =        Lock status = FREE
*    End of QSO_OBJ_HDR specific fields
	*/

	TRdisplay("    Start of QSO_OBJ_HDR specific fields\n");
	TRdisplay("\tObject type = %6w\n", "??????,QTEXT,QTREE,QP,SQLDA,ALIAS", obj->qso_obid.qso_type);
	trdisp_objname("\tObject name:",&obj->qso_obid);
	TRdisplay("\n");

	if ( (master = obj->qso_mobject) != NULL )
	{
	    trdisp_objname("\tAlias name:",&master->qsmo_aliasid);
	    TRdisplay("\n");
	}
	TRdisplay("\tMaster header = 0x%p\n", master);
	TRdisplay("\tObject size = %8d\tLock id = %8d\tLock Usage count = %d\n",
	    obj->qso_size, 
	    obj->qso_lk_id,
	    obj->qso_lk_cur);
	TRdisplay("\tReal usage = %8d\tDecay usage = %8d\tWaiters = %d\n",
	    obj->qso_real_usage,
	    obj->qso_decaying_usage,
	    obj->qso_waiters);
	TRdisplay("\tStreamid = 0x%p\tRoot = 0x%p\tSession = 0x%p\n", 
	    obj->qso_streamid,
	    obj->qso_root, 
	    obj->qso_session);
	TRdisplay("\tStatus = %v %s\tLock status = %6w\n",
	    "DESTROY,LRU_DESTROY", obj->qso_status,
	    (obj->qso_lk_state == QSO_EXLOCK || !obj->qso_root)
	    ? "(incomplete)" : "",
	    "FREE,SHLOCK,EXLOCK", obj->qso_lk_state);
	TRdisplay("    End of QSO_OBJ_HDR specific fields\n");
    }
    else if (obj->qso_type == QSMOBJ_TYPE)
    {
	/* We're a QSO_MASTER_HDR */

	/* Example output:
*    Start of QSO_MASTER_HDR specific fields
*       Object name: (40) (2,1) p1
*       Alias name: (76) (0,0) p1 ingres (0x4AC4715A)
*       Streamid = 0x5FE329F0   Session = 0x4030BB64
*    End of QSO_MASTER_HDR specific fields
	*/

	master = (QSO_MASTER_HDR *) qsf_rb->qsf_obj_id.qso_handle;

	TRdisplay("    Start of QSO_MASTER_HDR specific fields\n");
	trdisp_objname("\tObject name:",&master->qsmo_obid);
	TRdisplay("\n");
	trdisp_objname("\tAlias name:",&master->qsmo_aliasid);
	TRdisplay("\n");
	TRdisplay("\tStreamid = 0x%p\tSession = 0x%p\n", master->qsmo_streamid, master->qsmo_session);
	TRdisplay("    End of QSO_MASTER_HDR specific fields\n");
    }
    else
    {
	TRdisplay("\tUnrecognised qso_type: %d\n", obj->qso_type);	
    }

    return (E_DB_OK);
}


/* helper routine to TRdisplay an object or alias name as:
** <title> (lname) (i1,i1) string1 owner dbid
** where the "owner dbid" part only happens if lname is long enough.
*/
static void trdisp_objname(char *title, QSO_OBID *obid)
{
    i4			*n1;
    i4			*n2;
    i4			num;
    PTR			ptr;
    i2			len, len1;

    TRdisplay("%s (%d)", title, obid->qso_lname);
    if (obid->qso_lname <= 0)
    {
	TRdisplay(" (unnamed)");
    }
    else
    {
	ptr = (PTR )&(obid->qso_name)[0];
	n1 = (int *)ptr;
	ptr += (sizeof(i4));
	n2 = (int *)ptr;
	ptr += (sizeof(i4));
	len = obid->qso_lname - (2 * sizeof(i4));
	len1 = len;  if (len1 > DB_CURSOR_MAXNAME) len1 = DB_CURSOR_MAXNAME;
	TRdisplay(" (%d,%d) %~t", *n1, *n2, len1, ptr);
	if (len > len1)
	{
	    /* repeat query ID or dbp name -- probably an alias id */
	    /* Probably a dbp name */
	    len -= DB_CURSOR_MAXNAME;
	    ptr += DB_CURSOR_MAXNAME;
	    if (len == sizeof(i4))
	    {
		/* Probably a repeat query id: DB_CURSOR_ID + dbid */
		num = *(i4 *)ptr;
		TRdisplay(" (0x%x)",num);
	    }
	    else
	    {
		/* dbp is DB_CURSOR_ID + owner_text + dbid */
		num = 0;
		if (len > DB_OWN_MAXNAME)
		{
		    len = DB_OWN_MAXNAME;
		    num = *(i4 *) (ptr + DB_OWN_MAXNAME);
		}
		TRdisplay(" %~t (0x%x)",len,ptr,num);
	    }
	}
    }
} /* trdisp_objname */

/*{
** Name: QSD_OBQ_DUMP - Dump the QSF object queue.
**
** Description:
**      This routine will format and dump the entire QSF object queue to the
**      trace output file.
**
** Inputs:
**      QSD_OBQ_DUMP                    Op code to qsf_call().
**      qsf_rb                          Request block.
**
** Outputs:
**      qsf_rb
**		.qsf_error		Filled in with error if encountered.
**			.err_code	The QSF error that occurred.  One of:
**				E_QS0000_OK
**				    Operation succeeded.
**                              E_QS0004_SEMRELEASE
**                                  Error releasing semaphore.
**                              E_QS0008_SEMWAIT
**                                  Error waiting for semaphore.
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
**	    Writes to the trace output file.
**
** History:
**	06-may-86 (thurston)
**          Initial creation.
**	24-jun-86 (thurston)
**	    Modified code to comply with new SCF_CB.  This allowed setting the
**	    semaphore as a SHARED semaphore, which should help performance.
**	28-nov-87 (thurston)
**	    Changed semaphore code to use CS calls instead of going through SCF.
**	16-mar-88 (thurston)
**	    Now produces a memory pool map if trace point QS9 is set when this
**	    routine is invoked.
**	30-nov-1992 (rog)
**	    Various cleanup in support of the new object header fields.
**	17-oct-94	(ramra01)
**		Init var 'scb' to NULL. Will be updated in calling function
*/

DB_STATUS
qsd_obq_dump( QSF_RCB *qsf_rb )
{
    PTR			*handle = &qsf_rb->qsf_obj_id.qso_handle;
    PTR			save_handle = *handle;
    QSO_MASTER_HDR      *obj;
    QSO_HASH_BKT	*bucket;
    DB_STATUS		status;
    i4			n;
    i4			i;
    i4		*err_code = &qsf_rb->qsf_error.err_code;
    QSF_CB		*scb = NULL;


    status = E_DB_OK;
    *err_code = E_QS0000_OK;


    TRdisplay("%79*=\n");
    TRdisplay("Dump of QSF's Object hash table:\n");
    if (Qsr_scb->qsr_nobjs == 0)
    {
	TRdisplay("\n\t( QSF's Object hash table is empty. )\n");
    }
    else
    {
	for (i = 0; i < Qsr_scb->qsr_nbuckets; i++)
	{
	    bucket = &Qsr_scb->qsr_hashtbl[i];
	    /* Skip uninit'd buckets */
	    if ( !bucket->qsb_init )
		continue;
	    CSp_semaphore(TRUE, &bucket->qsb_sem);
	    if (bucket->qsb_nobjs == 0  &&  bucket->qsb_mxobjs == 0)
	    {
		CSv_semaphore(&bucket->qsb_sem);
		continue;
	    }

	    TRdisplay("\n\n\t\t*=*=* BUCKET [%d] *=*=*", i);
	    TRdisplay("     # objs = %d", bucket->qsb_nobjs);
	    TRdisplay("     max # objs ever = %d\n", bucket->qsb_mxobjs);

	    n = 0;
	    obj = bucket->qsb_ob1st;
	    while (obj != (QSO_MASTER_HDR *) NULL)
	    {
		TRdisplay("\n\tObject sequence # (in bucket) %d:", ++n);
		*handle = (PTR) obj;
		if ((status = qsd_obj_dump(qsf_rb)) != E_DB_OK) break;

		obj = obj->qsmo_obnext;
	    }
	    CSv_semaphore(&bucket->qsb_sem);
	}
    }
    TRdisplay("%62*= memleft = %6d\n", Qsr_scb->qsr_memleft);

    if (Qsr_scb->qsr_tracing  &&  qst_trcheck(scb, QSF_009_POOL_MAP_ON_QDUMP))
    {
	ULM_RCB			ulm_rcb;

	ulm_rcb.ulm_poolid = Qsr_scb->qsr_poolid;
	ulm_rcb.ulm_facility = DB_QSF_ID;
	status = ulm_mappool(&ulm_rcb);
    }


    *handle = save_handle;
    return (status);
}


/*{
** Name: QSD_LRU_DUMP - Dump QSF LRU queue.
**
** Description:
**     	Walks QSF's LRU object list, prints objects using
**		qsd_obj_dump and sends it to the trace output file.
**
** Inputs:
**		None
** Outputs:
**		None
**	Returns:
**		None
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**		15-aug-1995 (angusm)
**			Initial version.
**	23-Feb-2005 (schka24)
**	    Use local object dumper; mutex the queue while dumping.
*/
void	qsd_lru_dump()
{
    QSO_OBJ_HDR         *obj;
    QSF_RCB		qsf_rb;

    TRdisplay("\n%79*-\n");
    TRdisplay("Dump of LRU queue\n");
    TRdisplay("\n%79*-\n");

    /* Lock the LRU queue */
    CSp_semaphore(TRUE, &Qsr_scb->qsr_psem);
    for (obj = Qsr_scb->qsr_1st_lru; obj; obj = obj->qso_lrnext)
    {
	qsf_rb.qsf_obj_id.qso_handle = (PTR) obj;
	(void) qsd_obj_dump(&qsf_rb);
    }
    CSv_semaphore(&Qsr_scb->qsr_psem);
    TRdisplay("\n*** Finish displaying LRU queue ***\n");
    return;
}
