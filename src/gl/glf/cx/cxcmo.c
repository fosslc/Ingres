/*
** Copyright (c) 2001, 2004 Ingres Corporation
**
**
*/

#include    <compat.h>
#include    <pc.h>
#include    <cs.h>
#include    <er.h>
#include    <lk.h>
#include    <cx.h>
#include    <cxprivate.h>
#include    <me.h>

/**
**
**  Name: CXCMO.C - 'C'luster 'M'emory 'O'bject functions.
**
**  Description:
**
**      This module contains routines which implement a means to
**	read and atomically update variables visible across an
**	entire Ingres cluster. 
**
**	At present there are THREE styles of implementation
**	for this.   One 'DLM' used the value blocks associated
**	with distributed locks to store and control access to
**	the CMO.   This has the advantage of being universally
**	implementable, but has the drawback of potentially
**	being a performance drag, and introducing the possibility
**	of the value being invalidated if an updater dies at an
**	inopportune moment.   The 'IMC' style uses a
**	gigabit Internal Memory Channel to manage the CMO.
**	However this is specific to Tru64.  Finally using OpenDLM
**      under Linux, we can exploit the dlm_scnop routine to
**      perform quick atomic READ, WRITE, and SEQUENCE operations
**      without the overhead of acquiring and releasing locks.
**      One defect in this implementation is that UPDATE may
**      cause the other operations to "block" and return a
**      failure code.  In actual usage, no single CMO faces
**      the possibility of a simultanious READ & UPDATE.
**
**	Since the primary usage of a CMO is to maintain a
**	cluster-wide consistent data item for maintaining
**	SEQUENCE numbers such as the LSN, a specialized API function
**      is provided that will exploit any facility which allows
**      bumping a global sequence number more efficiently than the
**      general generic UPDATE function. 
**
**      Arbitrary user defined updates are performed by passing
**      the address of a callback function which has as one of it's
**      parameters the current value of the CMO.   The update
**      function can then safely calculate a new value knowing that
**      the current value will not change during this calculation.
**      However since the update function is called with the CMO
**      locked, it must never block, and should execute as fast as
**      possible. 
**
**
**	Public routines
**
**          CXcmo_read		- Read the current value of a CMO.
**	    CXcmo_write		- Replace the value of a CMO.
**          CXcmo_update	- Update the value of a CMO.
**          CXcmo_sequence	- Obtain the next value for a global counter.
**          CXcmo_synch 	- Synchronize with global counter..
**
**	    CXcmo_lsn_gsynch	- Synchronize Local Log Sequence # (LSN)
**				  to cluster wide LSN.
**	    CXcmo_lsn_lsynch	- Synchronize Local LSN, against passed
**				  value.
**	    CXcmo_lsn_next	- Bump local LSN & return val to caller.
**
**	Internal routines
**
**	    cx_cmo_support	- Guts of the CMO logic.
**	    cx_cmo_read		- internal CX_CMO_FUNC to implement read.
**	    cx_cmo_write	- internal CX_CMO_FUNC to implement write.
**	    cx_cmo_sequence	- internal CX_CMO_FUNC to implement seq #
**                                generation if not using CX_CMO_SCN.
**	    cx_cmo_synch	- internal CX_CMO_FUNC to implement 
**                                synchronization with a global counter
**                                if not using CX_CMO_SCN.
**	    
[@func_list@]...
**
**
**  History:
**      26-feb-2001 (devjo01)
**          Created.
**	02-apr-2001 (devjo01)
**	    Add CXcmo_write, cx_cmo_write.
**	24-oct-2003 (kinte01)
**	    Add missing me.h header file
**	06-jan-2004 (devjo01)
**	    Linux support.
**      20-sep-2004 (devjo01)
**	    Cumulative CX changes for cluster beta.
**          - Add CX_CMO_SCN & CMO_OP_SEQUENCE, CMO_OP_SYNCH support
**	    which allows us to exploit specialized vendor provided
**	    mechanisms for maintaining cluster wide sequence numbers.
**	    - tuff.
**	02-nov-2004 (devjo01)
**	    Add CX_F_NO_DOMAIN to CXdlm_request calls to keep CMO
**	    locks out of the recovery domain.
**/


/*
**	OS independent declarations
*/

# define        CMO_OP_READ     0
# define        CMO_OP_WRITE    1
# define        CMO_OP_UPDATE   2
# define        CMO_OP_SEQUENCE 3
# define        CMO_OP_SYNCH    4

GLOBALREF	CX_PROC_CB	CX_Proc_CB;
GLOBALREF	CX_STATS	CX_Stats;

GLOBALREF	CX_CMO_DLM_CB	CX_cmo_data[CX_MAX_CMO][CX_LKS_PER_CMO];


static STATUS cx_cmo_read( CX_CMO *oldval, CX_CMO *poutval, 
			PTR punused, bool invalidin );
static STATUS cx_cmo_write( CX_CMO *oldval, CX_CMO *pnewval, 
			PTR punused, bool invalidin );
static STATUS cx_cmo_sequence( CX_CMO *oldval, CX_CMO *pnewval, 
			PTR punused, bool invalidin );
static STATUS cx_cmo_synch( CX_CMO *oldval, CX_CMO *pnewval, 
			PTR punused, bool invalidin );
static STATUS cx_cmo_support( u_i4 cmoindex, u_i4 opcode, CX_CMO_FUNC *pfunc, 
		             CX_CMO *pcmo, PTR pdata );


/*{
** Name: CXcmo_read          - Read the current value of a CMO.
**
** Description:
**
**      This routine returns the value of a CMO at the moment it is
**	called.   There is no guarantee that the value will not
**	change immediately after it has been read.
**
**	If CMO value is not set, a zero filled CMO is returned.
**
** Inputs:
**      cmoindex	- Index number of CMO [ 0 .. CX_MAX_CMO - 1 ]
**	pcmo		- Pointer to CMO to be filled.
**
** Outputs:
**	*pcmo		- Filled in CMO
**
**	Returns:
**		OK			- Normal successful completion.
**		E_CL2C08_CX_W_SUCC_IVB	- CMO value marked invalid.
**		E_CL2C10_CX_E_NOSUPPORT - Unsupported CMO style this OS.
**		E_CL2C11_CX_E_BADPARAM  - cmoindex out of range.
**		E_CL2C17_CX_E_BADCONFIG - Invalid CMO style.
**		E_CL2C1B_CX_E_BADCONTEXT - CXconnect not yet called.
**		Other codes possible if DLM used.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	26-feb-2001 (devjo01)
**	    Created.
*/
STATUS
CXcmo_read( u_i4 cmoindex, CX_CMO *pcmo )
{
    CX_Stats.cx_cmo_reads++;
    return cx_cmo_support( cmoindex, CMO_OP_READ, cx_cmo_read, pcmo, NULL );
} /* CXcmo_read */



/*{
** Name: CXcmo_write          - Write the value of a CMO.
**
** Description:
**
**      This routine relaces the current value of a CMO with the passed
**	contents.   Major difference from CXcmo_update, is that new
**	value cannot be derived from existing value.
**
** Inputs:
**      cmoindex	- Index number of CMO [ 0 .. CX_MAX_CMO - 1 ]
**	pcmo		- Pointer to new CMO value.
**
** Outputs:
**	none
**
**	Returns:
**		OK			- Normal successful completion.
**		E_CL2C10_CX_E_NOSUPPORT - Unsupported CMO style this OS.
**		E_CL2C11_CX_E_BADPARAM  - cmoindex out of range.
**		E_CL2C17_CX_E_BADCONFIG - Invalid CMO style.
**		E_CL2C1B_CX_E_BADCONTEXT - CXconnect not yet called.
**		Other codes possible if DLM used.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	02-Apr-2001 (devjo01)
**	    Created.
*/
STATUS
CXcmo_write( u_i4 cmoindex, CX_CMO *pcmo )
{
    CX_Stats.cx_cmo_writes++;
    return cx_cmo_support( cmoindex, CMO_OP_WRITE, cx_cmo_write, pcmo, NULL );
} /* CXcmo_write */



/*{
** Name: CXcmo_update	- Atomically update a CMO.
**
** Description:
**
**      Atomically update a CMO by getting exclusive access to the
**	CMO value, then passing this value to a user provided update
**	function, which will calculate the new CMO value before
**	returning control to CX to complete the update, and release the
**	exclusive hold on the CMO.
**	
**	User provided function must not update the old value passed
**	to it.   If it needs to return additional information, or
**	additional information needs to be passed to the update function
**	to perform its work, then this additional data should be made
**	reachable through 'pdata'.
**	
** Inputs:
**
**      cmoindex	- Index number of CMO [ 0 .. CX_MAX_CMO - 1 ]
**
**	pcmo		- Pointer to CMO to hold updated value.
**
**	pfunc		- address of user callback routine with the
**			  following parameters:
**
**		poldval		- Ptr to current CMO value;	
**		
**		pnewval		- Ptr to buffer to hold new CMO value;	
**		
**		pdata		- Pointer to auxillary data.
**		
**		invaliddata	- bool value set to true if old CMO value is
**				  invalid.
**
**	pdata		- Pointer to auxillary data used by callback.
**
** Outputs:
**
**	*pcmo		- Filled in CMO
**
**	Returns:
**		OK			- Normal successful completion.
**		E_CL2C08_CX_W_SUCC_IVB	- CMO value marked invalid.
**		E_CL2C10_CX_E_NOSUPPORT - Unsupported CMO style this OS.
**		E_CL2C11_CX_E_BADPARAM  - cmoindex out of range.
**		E_CL2C17_CX_E_BADCONFIG - Invalid CMO style.
**		E_CL2C1B_CX_E_BADCONTEXT - CXconnect not yet called.
**		E_CL2C36_CX_E_OS_MISC_ERR - IMC spin time-out.
**		E_CL2C37_CX_E_OS_MISC_ERR - IMC update isolation error.
**		Other codes possible if DLM used.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	26-feb-2001 (devjo01)
**	    Created.
*/
STATUS
CXcmo_update( u_i4 cmoindex, CX_CMO *pcmo, CX_CMO_FUNC *pfunc, PTR pdata )
{
    CX_Stats.cx_cmo_updates++;
    return cx_cmo_support( cmoindex, CMO_OP_UPDATE, pfunc, pcmo, pdata );
} /* CXcmo_update */



/*{
** Name: CXcmo_sequence	- Atomically update a CMO.
**
** Description:
**
**      Atomically obtain the next value for a global sequence number
**	associated with this CMO.   Sequence number is always a 64 bit
**      unsigned integer value.   Initial value may be set using
**      CXcmo_write, or queried without forcing an increment via CXcmo_read.
**	
** Inputs:
**
**      cmoindex	- Index number of CMO [ 0 .. CX_MAX_CMO - 1 ]
**
**	pseq		- Pointer to sequence buffer hold updated value.
**
** Outputs:
**
**	*pseq		- Filled in with new counter value.
**
**	Returns:
**		OK			- Normal successful completion.
**		E_CL2C08_CX_W_SUCC_IVB	- CMO value marked invalid.
**		E_CL2C10_CX_E_NOSUPPORT - Unsupported CMO style this OS.
**		E_CL2C11_CX_E_BADPARAM  - cmoindex out of range.
**		E_CL2C17_CX_E_BADCONFIG - Invalid CMO style.
**		E_CL2C1B_CX_E_BADCONTEXT - CXconnect not yet called.
**		E_CL2C36_CX_E_OS_MISC_ERR - IMC spin time-out.
**		E_CL2C37_CX_E_OS_MISC_ERR - IMC update isolation error.
**		Other codes possible if DLM used.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	15-Sep-2004 (devjo01)
**	    Created from CXcmo_update.
*/
STATUS
CXcmo_sequence( u_i4 cmoindex, CX_CMO_SEQ *pseq )
{
    CX_Stats.cx_cmo_seqreqs++;

    return cx_cmo_support( cmoindex, CMO_OP_SEQUENCE, cx_cmo_sequence,
                           (CX_CMO *)pseq, NULL );
} /* CXcmo_sequence */


/*{
** Name: CXcmo_synch	- Atomically update a CMO.
**
** Description:
**
**      Atomically compare an 64 bit sequence value with a global 64 bit
**      sequence value, and if the current sequence value is lower update
**      it to the value passed.  Passed CMO is updated to reflect the final
**      value.
**	
** Inputs:
**
**      cmoindex	- Index number of CMO [ 0 .. CX_MAX_CMO - 1 ]
**
**	pseq		- Pointer to sequence buffer hold updated value.
**
** Outputs:
**
**	*pseq		- Final value of counter after synch op.
**
**	Returns:
**		OK			- Normal successful completion.
**		E_CL2C08_CX_W_SUCC_IVB	- CMO value marked invalid.
**		E_CL2C10_CX_E_NOSUPPORT - Unsupported CMO style this OS.
**		E_CL2C11_CX_E_BADPARAM  - cmoindex out of range.
**		E_CL2C17_CX_E_BADCONFIG - Invalid CMO style.
**		E_CL2C1B_CX_E_BADCONTEXT - CXconnect not yet called.
**		E_CL2C36_CX_E_OS_MISC_ERR - IMC spin time-out.
**		E_CL2C37_CX_E_OS_MISC_ERR - IMC update isolation error.
**		Other codes possible if DLM used.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	15-Sep-2004 (devjo01)
**	    Created from CXcmo_update.
*/
STATUS
CXcmo_synch( u_i4 cmoindex, CX_CMO_SEQ *pseq )
{
    CX_Stats.cx_cmo_synchreqs++;

    return cx_cmo_support( cmoindex, CMO_OP_SYNCH, cx_cmo_synch,
			   (CX_CMO *)pseq, NULL );
} /* CXcmo_synch */


/*{
** Name: cx_cmo_support          - guts of the CMO module.
**
** Description:
**
**      This routine performs all of the real work of the CMO sub-sub-facility.
**	
**	If DLM style is in use, it attempts to perform its work using
**	one of a set of previously allocated DLM locks.  If all DLM
**	locks reserved for this CMO are busy, then an ad-hoc lock is
**	obtained and discarded within this routine.  
**
**	If IMC style is in use, then global memory segment is locked,
**	and callback function is called with value read from read-only
**	mapping to global memory segment.  If lkmode is EX, then an
**	new value is written to write-only mapping to global memory
**	segment, and IMC "lock" is not released until propigation of
**	update to all nodes is confirmed.
**	
** Inputs:
**	
**      cmoindex	- Index number of CMO [ 0 .. CX_MAX_CMO - 1 ]
**	
**	lkmode		- LK_N for read access, LK_X for update access.
**	
**	pfunc		- address of callback routine.
**	
**	pcmo		- Pointer to CMO to be filled.
**	
**	pdata		- Pointer to auxillary data used by callback.
**	
** Outputs:
**	
**	*pcmo		- Filled in CMO
**
**	Returns:
**		OK			- Normal successful completion.
**		E_CL2C08_CX_W_SUCC_IVB	- CMO value marked invalid.
**		E_CL2C10_CX_E_NOSUPPORT - Unsupported CMO style this OS.
**		E_CL2C11_CX_E_BADPARAM  - cmoindex out of range.
**		E_CL2C17_CX_E_BADCONFIG - Invalid CMO style.
**		E_CL2C1B_CX_E_BADCONTEXT - CXconnect not yet called.
**		E_CL2C36_CX_E_OS_MISC_ERR - IMC spin time-out.
**		E_CL2C37_CX_E_OS_MISC_ERR - IMC update isolation error.
**		Other codes possible if DLM used.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    Changes to IMC memory if IMC used.  Changes to satistics in
**	    CX_Proc_CB.
**
** History:
**	26-feb-2001 (devjo01)
**	    Created.
**      15-sep-2004 (devjo01)
**          Add CX_CMO_SCN & CMO_OP_SEQUENCE stuff, 2nd paramer is
**          now an opcode rather than a lock mode.
**	20-Oct-2004 (hanje04)
**	    Wrap cx_cmo_support() in #ifdef conf_CLUSTER_BUILD and 
**          return E_CL2C10_CX_E_NOSUPPORT if we're not
*/
STATUS
cx_cmo_support( u_i4 cmoindex, u_i4 opcode, CX_CMO_FUNC *pfunc, 
		CX_CMO *pcmo, PTR pdata )
{
# ifdef conf_CLUSTER_BUILD 
    STATUS			 status, ustatus;
    i4				 i, getit;
    u_i4                         lkmode;
    CX_CMO_DLM_CB		*pcmolk;
    CX_REQ_CB			 lkreq, *plkreq;

# ifdef CX_PARANOIA
    if ( cmoindex >= CX_MAX_CMO )
	return E_CL2C11_CX_E_BADPARAM;
    if ( CX_ST_UNINIT == CX_Proc_CB.cx_state )
	return E_CL2C1B_CX_E_BADCONTEXT;
# endif /*CX_PARANOIA*/

    switch ( CX_Proc_CB.cx_cmo_type )
    {
    case CX_CMO_SCN:
# if defined(LNX)

        if ( CMO_OP_SEQUENCE == opcode ||
             CMO_OP_SYNCH == opcode )
        {
            /* Find free lock */
            for ( i = 0; i < CX_LKS_PER_CMO; i++ )
            {
                pcmolk = &CX_cmo_data[cmoindex][i];
                if ( !CS_ISSET( &pcmolk->cx_cmo_busy ) &&
                     CS_TAS( &pcmolk->cx_cmo_busy ) )
                {
                    break;
                }
            }
            if ( CX_LKS_PER_CMO == i )
            {
                /* Couldn't find a free lock, allocate a private lock */
                lkreq.cx_key = pcmolk->cx_cmo_lkreq.cx_key;
                lkreq.cx_user_func = NULL;
                plkreq = &lkreq;
                getit = TRUE;
            }
            else
            {
                plkreq = &pcmolk->cx_cmo_lkreq;
                if ( CX_NONZERO_ID( &plkreq->cx_lock_id ) )
                {
                    getit = FALSE;
                }
                else 
                {
                    getit = TRUE;
                }
            }

            if ( getit )
            {
                /* Request lock if not already held. */
                plkreq->cx_new_mode = LK_N;
                status = CXdlm_request( CX_F_WAIT | CX_F_PCONTEXT | 
                                        CX_F_NO_DOMAIN | CX_F_NODEADLOCK,
					plkreq, NULL );
            }
            else
            {
                status = OK;
            }

            if ( OK == status )
            {
                switch ( opcode )
                {
                case CMO_OP_READ:	/* Can't get here for now */
                    status = DLM_SCNOP( plkreq->cx_lock_id, SCN_CUR,
                      CX_CMO_VALUE_SIZE * 8, (char*)(&plkreq->cx_value), 
                      (char*)pcmo );
                    break;

                case CMO_OP_WRITE:	/* Can't get here for now */
                    status = DLM_SCNOP( plkreq->cx_lock_id, SCN_SET,
                      CX_CMO_VALUE_SIZE * 8, (char*)pcmo,
                      (char*)(&plkreq->cx_value) );
                    break;

                case CMO_OP_SEQUENCE:
                    status = DLM_SCNOP( plkreq->cx_lock_id, SCN_INC,
                      CX_CMO_VALUE_SIZE * 8, (char*)(&plkreq->cx_value), 
                      (char*)pcmo );
                    break;

                case CMO_OP_SYNCH:
                    status = DLM_SCNOP( plkreq->cx_lock_id, SCN_ADJ,
                      CX_CMO_VALUE_SIZE * 8, (char*)pcmo, (char*)pcmo );
                    break;

                default:
                    status = E_CL2C11_CX_E_BADPARAM;
                }
            }

            if ( plkreq == &lkreq )
            {
                /* Request was locally allocated, should be locally freed. */
                (void)CXdlm_release( 0, plkreq );
            }
            else
            {
                CS_ACLR( &pcmolk->cx_cmo_busy );
            }

            if ( OK == status )
            {
                break;
            }

            /* Drop into DLM case if blocked */
            TRdisplay( "CMO[%d] DLM_BLOCKED: op=%d, status=0x%x, plkreq = %p\n",
                        cmoindex, opcode, status, plkreq );
        }

# endif /* LNX */

        /* Drop into next case */

    case CX_CMO_DLM:

        if ( CMO_OP_READ == opcode )
            lkmode = LK_N;
        else
            lkmode = LK_X;

	/* Find free lock */
	for ( i = 0; i < CX_LKS_PER_CMO; i++ )
	{
	    pcmolk = &CX_cmo_data[cmoindex][i];
	    if ( !CS_ISSET( &pcmolk->cx_cmo_busy ) &&
		 CS_TAS( &pcmolk->cx_cmo_busy ) )
	    {
		break;
	    }
	}
	if ( CX_LKS_PER_CMO == i )
	{
	    /* Couldn't find a free lock, allocate a private lock */
	    lkreq.cx_key = pcmolk->cx_cmo_lkreq.cx_key;
	    lkreq.cx_user_func = NULL;
	    plkreq = &lkreq;
	    getit = TRUE;
	}
	else
	{
	    plkreq = &pcmolk->cx_cmo_lkreq;
	    if ( CX_NONZERO_ID( &plkreq->cx_lock_id ) )
	    {
		getit = FALSE;
		plkreq->cx_new_mode = lkmode;
		status = CXdlm_convert( CX_F_WAIT | CX_F_NODEADLOCK,
		 plkreq );
		if ( OK != status )
		{
		    if ( E_CL2C25_CX_E_DLM_NOTHELD == status )
			getit = TRUE;
		    else if ( E_CL2C08_CX_W_SUCC_IVB != status )
		    {
			/* Unexpected error */
			CS_ACLR( &pcmolk->cx_cmo_busy );
			break;
		    }
		}
	    }
	    else 
	    {
		getit = TRUE;
	    }
	}

	if ( getit )
	{
	    /* Request lock if not already held. */
	    plkreq->cx_new_mode = lkmode;
	    status = CXdlm_request( CX_F_WAIT | CX_F_PCONTEXT | 
				    CX_F_NO_DOMAIN | CX_F_NODEADLOCK,
				    plkreq, NULL );
	}

	if ( OK == status )
	{
	    ustatus = (*pfunc)( (CX_CMO *)plkreq->cx_value, 
				  pcmo, pdata, FALSE ); 
	}
        else if ( E_CL2C08_CX_W_SUCC_IVB == status )
	{
	    ustatus = (*pfunc)( (CX_CMO *)plkreq->cx_value, 
				  pcmo, pdata, TRUE ); 
	}
	else
	{
	    /* Unexpected error */
	    if ( plkreq != &lkreq ) CS_ACLR( &pcmolk->cx_cmo_busy );
	    break;
	}

	if ( OK == ustatus )
	{
	    (void)MEcopy( pcmo, sizeof(CX_CMO), (CX_CMO *)plkreq->cx_value );
	    status = OK;
	}

	if ( plkreq == &lkreq )
	{
	    /* Request was locally allocated, should be locally freed. */
	    (void)CXdlm_release( 0, plkreq );
	}
	else if ( LK_N != lkmode )
	{
	    /* Downgrade lock to NL (should always be synchronously granted). */
	    plkreq->cx_new_mode = LK_N;
	    (void)CXdlm_convert( CX_F_WAIT | CX_F_NODEADLOCK, 
	      &pcmolk->cx_cmo_lkreq );
	}
        if ( plkreq != &lkreq ) CS_ACLR( &pcmolk->cx_cmo_busy );
	break;

    case CX_CMO_IMC:
# if defined(axp_osf)
	/*
	**	Tru64 implementation
	*/
	{
	i4		 imcstat;
	i4		 lockindex;
	i4		 errcount;
	i4		 retry, spincnt;
	u_i4		 oldpid, oldseq;
	CXAXP_CMO_IMC_CB *prxa, *ptxa;

	if ( CX_Proc_CB.cxaxp_imc_lk_cnt > 1 )
	    lockindex = cmoindex % CX_Proc_CB.cxaxp_imc_lk_cnt;
	else
	    lockindex = 0;

	while ( 1 )
	{
	    imcstat = IMC_LKACQUIRE( CX_Proc_CB.cxaxp_imc_lk_id, lockindex, 0,
				     IMC_LOCKWAIT );
	    if ( IMC_LOCKPRIOR == imcstat )
	    {
		/* Running OS threads, yield CPU and try again. */
		CS_yield_thread();
		continue;
	    }
	    break;
	}

	if ( imcstat )
	{
	    status = cx_translate_status( imcstat );
	    break;
	} /* end axp_osf block */

	prxa = CX_Proc_CB.cxaxp_cmo_rxa + cmoindex;
	ptxa = CX_Proc_CB.cxaxp_cmo_txa + cmoindex;

	status = OK;
	ustatus = (*pfunc)( &prxa->cx_cmo_value, pcmo, pdata, FALSE );

	if ( (ustatus == OK) && (CMO_OP_READ != opcode) )
	{
	    do 
	    {
		/* Get current error count.  Note negative number is returned
                   in the extremely unlikely case of a simultaneous update. */
		while ( ( errcount =
			IMC_RDERRCNT_MR( CX_Proc_CB.cx_imc_rail ) ) < 0 )
		    /* Boy are we "lucky".  Try again */
		    ;
		oldseq = prxa->cx_cmo_seq;
		oldpid = prxa->cx_cmo_pid;
		do
		{
		    ptxa->cx_cmo_errcnt = errcount;
		    ptxa->cx_cmo_seq = oldseq + 1;
		    ptxa->cx_cmo_pid = CX_Proc_CB.cx_pid;
		    (void)MEcopy( pcmo, sizeof(CX_CMO),  ptxa->cx_cmo_value );

		    /* Flush memory */
		    (void)MB();

		    /* If error occurs, retransmit */
		} while ( ( imcstat = IMC_CKERRCNT_MR( &errcount,
			    CX_Proc_CB.cx_imc_rail ) ) != IMC_SUCCESS );
		
		for ( spincnt = retry = 0;
		      oldpid == prxa->cx_cmo_pid &&
		      oldseq == prxa->cx_cmo_seq;
		      spincnt++ )
		{
		    if ( !( ++spincnt % CXAXP_CMO_IMC_SPINCHK ) )
		    {
			if ( IMC_SUCCESS != IMC_CKERRCNT_MR(&errcount,0))
			{
			    /* bump stat, don't bother protecting. */
			    CX_Proc_CB.cxaxp_imc_retries++;
			    retry = 1;
			    break;
			}
			if ( spincnt >= CXAXP_CMO_IMC_SPINMAX )
			{
			    /* bump failure stat, don't bother protecting. */
			    CX_Proc_CB.cxaxp_imc_spinfails++;
			    status = E_CL2C36_CX_E_OS_MISC_ERR;
			    break;
			}
		    }
		}
		if ( spincnt > CX_Proc_CB.cxaxp_imc_maxspin )
		{
		    /* Statistical information, don't bother protecting */
		    CX_Proc_CB.cxaxp_imc_maxspin = spincnt;
		}
		if ( (oldseq+1) != prxa->cx_cmo_seq ||
		     CX_Proc_CB.cx_pid != prxa->cx_cmo_pid )
		{
		    /*
		    ** This should be impossible, since we have
		    ** the IMC "lock" for this piece of memory.
		    **
		    ** Bump failure stat, don't bother protecting. 
		    */
		    CX_Proc_CB.cxaxp_imc_isofails++;
		    status = E_CL2C37_CX_E_OS_MISC_ERR;
		}
	    } while ( retry );
	} /*if LK_X*/

	imcstat = IMC_LKRELEASE( CX_Proc_CB.cxaxp_imc_lk_id, lockindex );
	if ( OK == status )
	{
	    status = cx_translate_status( imcstat );
	}
	} /* end axp_osf block */
# else /*axp_osf (Tru64)*/
	/*
	**	All other platforms
	*/
	status = E_CL2C10_CX_E_NOSUPPORT;
# endif /*axp_osf (Tru64)*/
	break;

    default:
	status = E_CL2C17_CX_E_BADCONFIG;
    }
    return status;
# else /* conf_CLUSTER_BUILD */
    return  E_CL2C10_CX_E_NOSUPPORT;
# endif /* conf_CLUSTER_BUILD */
} /*cx_cmo_support*/


/*{
** Name: cx_cmo_read          - NOP update function to implement read.
**
** Description:
**
**      This routine is used by CXcmo_read as the update function
**	passed to cx_cmo_support.   It simply copies the in value
**	to the outbuffer.
**	
** Inputs:
**	
**	oldval		- Current value for CMO.
**
**	poutval		- Where to copy CMO value.
**
**	punused		- unused.
**
**	invalidin	- unused.
**	
** Outputs:
**	
**	*poutval	- CMO value copied from pinval.
**
**	Returns:
**		OK			- Normal successful completion.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	26-feb-2001 (devjo01)
**	    Created.
*/
STATUS
cx_cmo_read( CX_CMO *oldval, CX_CMO *poutval, PTR punused, bool invalidin ) 
{
    (void)MEcopy( oldval, sizeof(CX_CMO), poutval );
    return OK;
} /*cx_cmo_read*/


/*{
** Name: cx_cmo_write         - internal function to implement write.
**
** Description:
**
**      This routine is used by CXcmo_write as the update function
**	passed to cx_cmo_support.   It simply copies the user data
**	to the outbuffer.
**	
** Inputs:
**	
**	oldval		- Current value for CMO. (ignored)
**
**	pnewval		- Already holds new value, so ignored.
**
**	punused  	- unused.
**
**	invalidin	- unused.
**	
** Outputs:
**	
**	none
**
**	Returns:
**		OK			- Always
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	02-Apr-2001 (devjo01)
**	    Created.
*/
STATUS
cx_cmo_write( CX_CMO *oldval, CX_CMO *pnewval, PTR punused, bool invalidin ) 
{
    /* Do nothing, buffer already holds new value */
    return OK;
} /*cx_cmo_write*/


/*{
** Name: cx_cmo_sequence         - internal function to implement sequence.
**
** Description:
**
**      This routine is used by CXcmo_sequence as the update function
**	passed to cx_cmo_support if no OS provided mechanism exists to
**      support sequence numbers.
**	
** Inputs:
**	
**	oldval		- Current value for 64 bit sequence counter.
**
**	pnewval		- Buffer to hold new counter value.
**
**	punused  	- unused.
**
**	invalidin	- unused.
**	
** Outputs:
**	
**	none
**
**	Returns:
**		OK			- Always
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	15-Sep-2004 (devjo01)
**	    Created from cx_cmo_write.
*/
STATUS
cx_cmo_sequence( CX_CMO *oldval, CX_CMO *pnewval, PTR punused, bool invalidin ) 
{
    /* Increment 8 byte counter passed to us */
    ((CX_LSN *)pnewval)->cx_lsn_high = ((CX_LSN *)oldval)->cx_lsn_high;
    if ( 0 == ( ((CX_LSN *)pnewval)->cx_lsn_low =
                ((CX_LSN *)oldval)->cx_lsn_low + 1 ) )
        ((CX_LSN *)pnewval)->cx_lsn_high++;
    return OK;
} /*cx_cmo_sequence*/

/*{
** Name: cx_cmo_synch         - internal function to implement synch.
**
** Description:
**
**      This routine is used by CXcmo_synch as the update function
**	passed to cx_cmo_support if no OS provided mechanism exists to
**      support atomic synchronization of sequence numbers.
**	
** Inputs:
**	
**	oldval		- Current value for 64 bit synch counter.
**
**	pnewval		- Buffer to hold new counter value.
**
**	punused  	- unused.
**
**	invalidin	- unused.
**	
** Outputs:
**	
**	none
**
**	Returns:
**		OK			- Always
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	15-Sep-2004 (devjo01)
**	    Created from cx_cmo_write.
*/
STATUS
cx_cmo_synch( CX_CMO *oldval, CX_CMO *pnewval, PTR punused, bool invalidin ) 
{
    /* Synchronize oldval & new value to MAX(old,new) */
    if ( ( ((CX_LSN *)pnewval)->cx_lsn_high <
           ((CX_LSN *)oldval)->cx_lsn_high ) ||
         ( ( ((CX_LSN *)pnewval)->cx_lsn_high ==
             ((CX_LSN *)oldval)->cx_lsn_high ) &&
           ( ((CX_LSN *)pnewval)->cx_lsn_low <
             ((CX_LSN *)oldval)->cx_lsn_low ) ) )
	*(CX_LSN *)pnewval = *(CX_LSN *)oldval;
    return OK;
} /*cx_cmo_synch*/


/*{
** Name: CXcmo_lsn_gsynch - Synchronize local & global LSN value.
**
** Description:
**
**      Atomically compare locally maintained LSN value with global
**	(cluster-wide) LSN value, and set both to the greater value.
**	
** Inputs:
**
**      lsnbuf		- Optional pointer to 64 bit structure to
**		          remit new internal "raw" LSN.
**
** Outputs:
**
**	*lsnbuf		- Filled with new LSN.  Note this LSN is
**			  uncontrolled, and may be instantly
**			  obsolete.  However, assuming success,
**			  it IS guaranteed to be at least as high
**			  as any gsynch'ed LSN issued anywhere in 
**			  the cluster at the time this function is
**			  entered.
**
**	Returns:
**		OK			- Normal successful completion.
**		E_CL2C08_CX_W_SUCC_IVB	- CMO value marked invalid.
**		E_CL2C10_CX_E_NOSUPPORT - Unsupported CMO style this OS.
**		E_CL2C11_CX_E_BADPARAM  - cmoindex out of range.
**		E_CL2C17_CX_E_BADCONFIG - Invalid CMO style.
**		E_CL2C1B_CX_E_BADCONTEXT - CXconnect not yet called.
**		E_CL2C36_CX_E_OS_MISC_ERR - IMC spin time-out.
**		E_CL2C37_CX_E_OS_MISC_ERR - IMC update isolation error.
**		Other codes possible if DLM used.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    Local, and/or Global LSN will change.
**
** History:
**	20-Sep-2004 (devjo01)
**	    Initial version.
*/
STATUS
CXcmo_lsn_gsynch( CX_LSN *lsnbuf )
{
    STATUS		status;
    union {
	CX_LSN		lsn;
	CX_CMO		cmo;
    }			buf;
    CX_LSN		locallsn;
	
    CX_Stats.cx_cmo_lsn_gsynchreqs++;

    MEfill( sizeof(buf), '\0', (PTR)&buf );
    
    status = CXcmo_lsn_lsynch( &buf.lsn );
    while ( OK == status )
    {
	locallsn = buf.lsn;
	status = cx_cmo_support( CX_LSN_CMO, CMO_OP_SYNCH, cx_cmo_synch,
                           &buf.cmo, NULL );
	if ( OK != status ) break;

	if (lsnbuf)
	{
	    *lsnbuf = buf.lsn;
	}

	if ( buf.lsn.cx_lsn_low != locallsn.cx_lsn_low ||
	     buf.lsn.cx_lsn_high != locallsn.cx_lsn_high )
	{
	    status = CXcmo_lsn_lsynch( &buf.lsn );
	}
	break;
    }
    return status;
} /* CXcmo_lsn_gsynch */



/*{
** Name: CXcmo_lsn_lsynch - Synchronize local LSN  with passed value.
**
** Description:
**
**      Atomically compare locally maintained LSN value with value
**	passed by caller, and set both to the greater value.
**	
** Inputs:
**
**      *lsnbuf		- Pointer to 64 bit structure holding 
**			  "proposed" LSN.
**
** Outputs:
**
**	*lsnbuf		- Filled with higher LSN value.
**
**	Returns:
**		OK			- Normal successful completion.
**		Other codes possible if CSp_semaphore fail possible.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    Local LSN may change.
**
** History:
**	20-Sep-2004 (devjo01)
**	    Initial version.
*/
STATUS
CXcmo_lsn_lsynch( CX_LSN *lsnbuf )
{
    STATUS	status;

    status = CSp_semaphore( TRUE, &CX_Proc_CB.cx_ncb->cx_lsn_sem );
    if ( OK == status )
    {
	CX_Stats.cx_cmo_lsn_lsynchreqs++;

	if ( ( lsnbuf->cx_lsn_high > CX_Proc_CB.cx_ncb->cx_lsn.cx_lsn_high ) ||
	     ( ( lsnbuf->cx_lsn_high ==
	         CX_Proc_CB.cx_ncb->cx_lsn.cx_lsn_high ) &&
	         ( lsnbuf->cx_lsn_low >
		   CX_Proc_CB.cx_ncb->cx_lsn.cx_lsn_low ) ) )
	{
	    CX_Proc_CB.cx_ncb->cx_lsn = *lsnbuf;
	}
	else
	{
	    *lsnbuf = CX_Proc_CB.cx_ncb->cx_lsn;
	}
	(void)CSv_semaphore( &CX_Proc_CB.cx_ncb->cx_lsn_sem );
    }
    else
	CX_Stats.cx_severe_errors++;
    return status;
} /* CXcmo_lsn_lsynch */



/*{
** Name: CXcmo_lsn_next - Increment local LSN and return value.
**
** Description:
**
**      Atomically increment locally maintained LSN value and return
**	this new value to caller.
**	
** Inputs:
**
**      *lsnbuf		- Pointer to buffer to receive new LSN.
**
** Outputs:
**
**	*lsnbuf		- Filled with LSN value.
**
**	Returns:
**		OK			- Normal successful completion.
**		Other codes possible if CSp_semaphore fail possible.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    Local LSN incremented.
**
** History:
**	20-Sep-2004 (devjo01)
**	    Initial version.
*/
STATUS
CXcmo_lsn_next( CX_LSN *lsnbuf )
{
    STATUS	status;

    status = CSp_semaphore( TRUE, &CX_Proc_CB.cx_ncb->cx_lsn_sem );
    if ( OK == status )
    {
	CX_Stats.cx_cmo_lsn_nextreqs++;

	if ( 0 == ++CX_Proc_CB.cx_ncb->cx_lsn.cx_lsn_low )
	    ++CX_Proc_CB.cx_ncb->cx_lsn.cx_lsn_high;

	*lsnbuf = CX_Proc_CB.cx_ncb->cx_lsn;
	(void)CSv_semaphore( &CX_Proc_CB.cx_ncb->cx_lsn_sem );
    }
    else
	CX_Stats.cx_severe_errors++;
    return status;
} /* CXcmo_lsn_next */


/* EOF: CXCMO.C */
