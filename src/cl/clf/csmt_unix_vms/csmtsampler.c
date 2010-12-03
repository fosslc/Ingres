/******************************************************************************
**
** Copyright (c) 1995, 2008 Ingres Corporation
**
******************************************************************************/

/****************************************************************************
**
** Name: CS_sampler - The monitor's sampling thread
**
** Description:
**      This routine is started as a separate thread by CSmonitor.
**      It looks at the global ptr for its block to decide what to do.
**	It sleeps for the sampling period, then awakens, takes a look at
**	what each thread is doing, and puts the thread's current state into
**	one of the counting buckets. When it wakes up and finds the global
**	ptr NULL, it quits.
**
** Inputs:
**	The GLOBALREF PTR CS_samplerBlkPtr - Pointer to the Sampler Block.
**
** Outputs:
**	All statistics are in the Sampler BLock pointed to by CS_samplerBlkPtr.
**	
** Returns:
** 
** Exceptions:
** 
** Side Effects:
**      Sleeps for interval specified in the Sampler Block, then wakes and
**	counts current states of the various threads.
**
**  History:
**	08-sep-1995 (wonst02)
**	    Original.
**	21-sep-1995 (shero03)
**	    Count the mutexes by thread.
**	27-sep-1995 (wonst02)
**	    Validate cs_thread_type and cs_state before using as array index.
**	04-oct-1995 (wonst02)
**	    Get the address of scs_facility from the CS_SYSTEM block.
**	15-nov-1995 (canor01)
**	    The facility number as stored in the CS_SYSTEM block may be
**	    invalid.
**	08-feb-1996 (canor01)
**	    Add LGEVENT_MASK and LKEVENT_MASK and explicit LOG_MASK.  LOG_MASK
**	    is no longer the default.
**	09-sep-1996 (canor01)
**	    Hold the Cs_known_list_sem while traversing the known list chain.
**	    Do not allocate the SCB on the stack; stack is local in Solaris.
**	13-sep-1996 (canor01)
**	    Skip the sampler thread itself when gathering statistics.
**	16-sep-1996 (canor01)
**	    Save the mutex id (its address) as well as its name, for use
**	    by "show mutex" command.
**	18-sep-1996 (canor01)
**	    To make statistics more meaningful, do not collect samples on
**	    either the sampler thread or the monitor thread.
**	17-dec-1996 (wonst02)
**	    Add lock resource information.
**	24-jan-1997 (wonst02)
**	    Count numtlsslots & numtlsprobes for new CS tls hash table.
**      18-feb-1997 (hanch04)
**          As part of merging of Ingres-threaded and OS-threaded servers,
**          rename file to csmtsampler.c and moved all calls to CSMT...
**      07-mar-1997 (canor01)
**          Make all functions dummies unless OS_THREADS_USED is defined.
**	11-apr-1997 (canor01)
**	    Make CS_tls statistics dependent on inclusion of local tls
**	    functions.
**	14-apr-1997 (kosma01)
**	    For POSIX threads place the thread_id in a pthread_t type
**	    variable.
**	11-Mar-1998 (jenjo02)
**	    Changed MAXTHREADS to MAXSAMPTHREADS. 
**	20-Mar-1998 (jenjo02)
**	    Removed cs_access_sem, utilizing cs_event_sem instead to 
**	    protect cs_state and cs_mask.
**	02-Jun-1998 (jenjo02)
**	    Set sampler thread's cs_thread_type to CS_SAMPLER so that
**	    scs_iformat() can weed them out. They have no SCF component
**	    and give scs_iformat() fits because of the lack.
**	24-Sep-1998 (jenjo02)
**	    Standardize cs_self = (CS_SID *)scb, session id's are always 
**	    a pointer to the thread's SCB, never OS thread_id, which is 
**	    contained in cs_thread_id.
**	16-Nov-1998 (jenjo02)
**	    Support newly-defined event wait masks, IdleThread-less
**	    server.
**
**	04-Apr-2000 (jenjo02)
**	    Added getrusage stats to sampler output, when available.
**	    CSMT_tls_measure() passed cs_thread_id, not cs_self, as argument.
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-Sep-2000 (jenjo02)
**	    Break out event wait counts by thread type, keep track
**	    of I/O and transaction rates. Extract Writebehind and
**	    Sort Factotum thread stats.
**	11-Oct-2000 (jenjo02)
**	    Backed out calls to LG, prohibited by Ingres encapsulation.
**	    Can't get transaction rates but through LG, so they'll
**	    just show as zero until someone comes up with a better
**	    idea.
**	08-dec-2008 (joea)
**	    VMS port:  exclude Unix event system code.
**	21-Aug-2009 (kschendel) 121804
**	    Declare CSMTset_scb.
**	11-Nov-2010 (kschendel) SIR 124685
**	    Prototype / include fixes.
****************************************************************************/

# include <compat.h>
# include <clconfig.h>
# ifdef OS_THREADS_USED
# include <gl.h>
# include <sp.h>
# include <cs.h>
# include <er.h>
# include <ex.h>
# include <lo.h>
# include <iicommon.h>
# include <me.h>
# include <st.h>
# include <tr.h>
# include <cv.h>
# include <pc.h>
# include <lk.h>
# include <csev.h>
# include <csinternal.h>
# include <rusage.h>
# include "csmtlocal.h"
# include "csmtsampler.h"

GLOBALREF CSSAMPLERBLKPTR CsSamplerBlkPtr;
GLOBALREF CS_SYNCH        hCsSamplerSem;
GLOBALREF CS_SEMAPHORE    Cs_known_list_sem;

/* Forward References */
static VOID
AddLock( LK_LOCK_KEY	LkKey,
	 i4		thread_type );

static VOID
AddMutex( CS_SEMAPHORE	*CsSem,
	  i4		thread_type );

static	CS_SCB	samp_scb;
static	LK_LOCK_KEY	dummy_lock	ZERO_FILL;


/* The screwy looking return type is dictated by create-thread. */

void *
CSMT_sampler(void *dummy)
{
	CS_SCB	*an_scb;
	i4	sleeptime, elapsed, seconds, event;
	SYSTIME starttime, stoptime;
	i4	cs_thread_type;
	i4	cs_state;
	bool	shutdown = FALSE;
	u_i4	bior, biow;
	u_i4	dior, diork, diow, diowk;
	u_i4	lior, liork, liow, liowk;

  /* The sampler thread needs a minimal SCB to be able to sleep */
    samp_scb.cs_type = CS_SCB_TYPE;
    samp_scb.cs_client_type = 0;
    samp_scb.cs_owner = (PTR)CS_IDENT;
    samp_scb.cs_tag = CS_TAG;
    samp_scb.cs_length = sizeof(CS_SCB);
    samp_scb.cs_stk_area = NULL;
    samp_scb.cs_stk_size = 0;
    samp_scb.cs_state = CS_COMPUTABLE;
    samp_scb.cs_priority = CS_LIM_PRIORITY;    /* highest priority */
    samp_scb.cs_thread_type = CS_SAMPLER;
    samp_scb.cs_pid = Cs_srv_block.cs_pid;
    samp_scb.cs_self = (CS_SID)&samp_scb;
    MEmove( 18,
            " <sampler thread> ",
            ' ',
            sizeof(samp_scb.cs_username),
            samp_scb.cs_username);

    CS_thread_id_assign(samp_scb.cs_thread_id, CS_get_thread_id());

#if !defined(VMS)
    /* Allocate and init an event wakeup block */
    if ( CSMT_get_wakeup_block(&samp_scb) )
	return NULL;
#endif

    /* Set the Sampler's SCB pointer in thread local storage */
    CSMTset_scb( &samp_scb );

    CSMTp_semaphore( TRUE, &Cs_known_list_sem );
    samp_scb.cs_next = Cs_srv_block.cs_known_list;
    samp_scb.cs_prev = Cs_srv_block.cs_known_list->cs_prev;
    Cs_srv_block.cs_known_list->cs_prev->cs_next = &samp_scb;
    Cs_srv_block.cs_known_list->cs_prev          = &samp_scb;
    CSMTv_semaphore( &Cs_known_list_sem );

  /*
  ** This thread goes into a loop:
  **	1. Lock the sampler block
  **	2. Do sampling
  **	3. Sleep for the specified interval
  ** The thread will exit normally when the sampler block pointer is NULL.
  ** The thread exits abnormally if it cannot lock the block.
  */
  TMnow(&starttime);
  elapsed = 0;

  /* Prime the local I/O, Transaction rate counters */
  bior  = Cs_srv_block.cs_wtstatistics.cs_bior_done;
  biow  = Cs_srv_block.cs_wtstatistics.cs_biow_done;
  dior  = Cs_srv_block.cs_wtstatistics.cs_dior_done;
  diork = Cs_srv_block.cs_wtstatistics.cs_dior_kbytes;
  diow  = Cs_srv_block.cs_wtstatistics.cs_diow_done;
  diowk = Cs_srv_block.cs_wtstatistics.cs_diow_kbytes;
  lior  = Cs_srv_block.cs_wtstatistics.cs_lior_done;
  liork = Cs_srv_block.cs_wtstatistics.cs_lior_kbytes;
  liow  = Cs_srv_block.cs_wtstatistics.cs_liow_done;
  liowk = Cs_srv_block.cs_wtstatistics.cs_liow_kbytes;

  /* Transaction rates cannot be determined */
  CsSamplerBlkPtr->txn[CURR] = 0;
  CsSamplerBlkPtr->txn[PEAK] = 0;

  for (;;)
  {
				 
    if (LockSamplerBlk(&hCsSamplerSem) != OK)
    {
	shutdown = TRUE;
    }

    if (CsSamplerBlkPtr == NULL)
    {
    	UnlockSamplerBlk(&hCsSamplerSem);
	shutdown = TRUE;
    }
    if ( shutdown == TRUE )
    {
	CSMTp_semaphore( 1, &Cs_known_list_sem );     /* exclusive */
	samp_scb.cs_next->cs_prev = samp_scb.cs_prev;
	samp_scb.cs_prev->cs_next = samp_scb.cs_next;
	CSMTv_semaphore( &Cs_known_list_sem );

#if !defined(VMS)
	/* Free the event wakeup block */
	CSMT_free_wakeup_block(&samp_scb);
#endif

	return NULL;
    }

    ++CsSamplerBlkPtr->numsamples;   /* Count the number of times we sample */

    CSMTp_semaphore( 0, &Cs_known_list_sem );

    /* Loop thru all the SCBs in the server */
    for (an_scb = Cs_srv_block.cs_known_list->cs_next;
    	 an_scb && an_scb != Cs_srv_block.cs_known_list;
	 an_scb = an_scb->cs_next)
    {
	/* skip the sampler thread and the monitor thread */
	if (an_scb == &samp_scb ||
	    an_scb->cs_thread_type == CS_MONITOR )
	    continue;

	if (an_scb->cs_thread_type >= -1 && 
	    an_scb->cs_thread_type <= MAXSAMPTHREADS - 1)
	    cs_thread_type = an_scb->cs_thread_type;
	else
	    cs_thread_type = MAXSAMPTHREADS - 1; /* use the <invalid> type */

	/* If Factotum thread, try to isolate which kind */
	if ( cs_thread_type == CS_FACTOTUM )
	{
	    if ( MEcmp((char *)&an_scb->cs_username, " <WriteBehind", 13) == 0 )
		cs_thread_type = CS_WRITE_BEHIND;
	    else
	    if ( MEcmp((char *)&an_scb->cs_username, " <Sort", 6) == 0 )
		cs_thread_type = CS_SORT;
	}
	    
	if (an_scb->cs_state >= 0 &&
	    an_scb->cs_state <= MAXSTATES - 1) 
	    cs_state = an_scb->cs_state;
	else
	    cs_state = MAXSTATES - 1;

	++CsSamplerBlkPtr->Thread[cs_thread_type].numthreadsamples;

	if ( cs_thread_type == CS_NORMAL )
	    ++CsSamplerBlkPtr->totusersamples;
	else
	    ++CsSamplerBlkPtr->totsyssamples;

	switch (cs_state)
	{
	    case CS_COMPUTABLE:
	    {
	    	i4 facility;

		++CsSamplerBlkPtr->Thread[cs_thread_type].state[cs_state];
		facility = (*Cs_srv_block.cs_facility)(an_scb);
		if (facility >= MAXFACS || facility < 0)
		    facility = MAXFACS - 1;
		++CsSamplerBlkPtr->		/* count current facility */
		  Thread[cs_thread_type].facility[facility];
		break;
	    }

	    case CS_EVENT_WAIT:
		if ( an_scb->cs_memory & CS_BIO_MASK )
		    ++CsSamplerBlkPtr->Thread[cs_thread_type].evwait[EV_BIO];
		else if ( an_scb->cs_memory & CS_DIO_MASK )
		    ++CsSamplerBlkPtr->Thread[cs_thread_type].evwait[EV_DIO];
		else if ( an_scb->cs_memory & CS_LIO_MASK )
		    ++CsSamplerBlkPtr->Thread[cs_thread_type].evwait[EV_LIO];
		else if ( an_scb->cs_memory & CS_LOG_MASK )
		    ++CsSamplerBlkPtr->Thread[cs_thread_type].evwait[EV_LOG];
		else if (an_scb->cs_memory & CS_LOCK_MASK)
		{
		    ++CsSamplerBlkPtr->Thread[cs_thread_type].evwait[EV_LOCK];
		    AddLock( an_scb->cs_sync_obj ?
		    		*((LK_LOCK_KEY *)an_scb->cs_sync_obj) :
		    		dummy_lock,
			     cs_thread_type );
		}
		else
		    ++CsSamplerBlkPtr->Thread[cs_thread_type].state[cs_state];
		event = (an_scb->cs_memory & CS_DIO_MASK ?
			  an_scb->cs_memory & CS_IOR_MASK ?
			  0 : 1
			 :
			 an_scb->cs_memory & CS_LIO_MASK ?
			  an_scb->cs_memory & CS_IOR_MASK ?
			  2 : 3
			 :
			 an_scb->cs_memory & CS_BIO_MASK ?
			  an_scb->cs_memory & CS_IOR_MASK ?
			  4 : 5
			 :
			 an_scb->cs_memory & CS_LOG_MASK ?
			  6 :
			 an_scb->cs_memory & CS_LOCK_MASK ?
			  7 :
			 an_scb->cs_memory & CS_LGEVENT_MASK ?
			  8 :
			 an_scb->cs_memory & CS_LKEVENT_MASK ?
			  9 :
			 /* else it is ...   unknown */
			  10);

		switch (cs_thread_type)
		{
		    case CS_USER_THREAD:
			++CsSamplerBlkPtr->numusereventsamples;
			++CsSamplerBlkPtr->userevent[event]; /* count event type */
			break;
		    default:
			++CsSamplerBlkPtr->numsyseventsamples;
			++CsSamplerBlkPtr->sysevent[event]; /* count event type */
			break;
		} /* switch (cs_thread_type) */
		break;

	    case CS_MUTEX:
		++CsSamplerBlkPtr->Thread[cs_thread_type].state[cs_state];
		AddMutex( ((CS_SEMAPHORE *)an_scb->cs_sync_obj),
				cs_thread_type );
		break;

	    default:
		++CsSamplerBlkPtr->Thread[cs_thread_type].state[cs_state];
		break;
	} /* switch (cs_state) */

# ifndef xCL_094_TLS_EXISTS
	++CsSamplerBlkPtr->numtlssamples;
	CsSamplerBlkPtr->numtlsreads = 0;
	CsSamplerBlkPtr->numtlsdirty = 0;
	CsSamplerBlkPtr->numtlswrites = 0;
	CSMT_tls_measure(an_scb->cs_thread_id, &CsSamplerBlkPtr->numtlsslots, 
		&CsSamplerBlkPtr->numtlsprobes, &CsSamplerBlkPtr->numtlsreads, 
		&CsSamplerBlkPtr->numtlsdirty, &CsSamplerBlkPtr->numtlswrites );
# endif /* xCL_094_TLS_EXISTS */
    } /* for */

    CSMTv_semaphore( &Cs_known_list_sem );

    /*
    ** If a second or more worth of intervals appear to have elapsed,
    ** compute current and peak per-second I/O, Transaction rates.
    */
    if ( (elapsed += CsSamplerBlkPtr->interval) >= 1000 )
    {
	/* Get the current time; the interval is not reliable! */
	TMnow(&stoptime);

	if ( (seconds = stoptime.TM_secs - starttime.TM_secs) )
	{
	    if ( (CsSamplerBlkPtr->bior[CURR] =
		   (Cs_srv_block.cs_wtstatistics.cs_bior_done - bior)
		       / seconds)
		    > CsSamplerBlkPtr->bior[PEAK] )
		CsSamplerBlkPtr->bior[PEAK] = CsSamplerBlkPtr->bior[CURR];

	    if ( (CsSamplerBlkPtr->biow[CURR] =
		   (Cs_srv_block.cs_wtstatistics.cs_biow_done - biow)
		       / seconds)
		    > CsSamplerBlkPtr->biow[PEAK] )
		CsSamplerBlkPtr->biow[PEAK] = CsSamplerBlkPtr->biow[CURR];

	    if ( (CsSamplerBlkPtr->dior[CURR] =
		   (Cs_srv_block.cs_wtstatistics.cs_dior_done - dior)
		       / seconds)
		    > CsSamplerBlkPtr->dior[PEAK] )
		CsSamplerBlkPtr->dior[PEAK] = CsSamplerBlkPtr->dior[CURR];

	    if ( (CsSamplerBlkPtr->diork[CURR] =
		   (Cs_srv_block.cs_wtstatistics.cs_dior_kbytes - diork)
		       / seconds)
		    > CsSamplerBlkPtr->diork[PEAK] )
		CsSamplerBlkPtr->diork[PEAK] = CsSamplerBlkPtr->diork[CURR];

	    if ( (CsSamplerBlkPtr->diow[CURR] =
		   (Cs_srv_block.cs_wtstatistics.cs_diow_done - diow)
		       / seconds)
		    > CsSamplerBlkPtr->diow[PEAK] )
		CsSamplerBlkPtr->diow[PEAK] = CsSamplerBlkPtr->diow[CURR];

	    if ( (CsSamplerBlkPtr->diowk[CURR] =
		   (Cs_srv_block.cs_wtstatistics.cs_diow_kbytes - diowk)
		       / seconds)
		    > CsSamplerBlkPtr->diowk[PEAK] )
		CsSamplerBlkPtr->diowk[PEAK] = CsSamplerBlkPtr->diowk[CURR];

	    if ( (CsSamplerBlkPtr->lior[CURR] =
		   (Cs_srv_block.cs_wtstatistics.cs_lior_done - lior)
		       / seconds)
		    > CsSamplerBlkPtr->lior[PEAK] )
		CsSamplerBlkPtr->lior[PEAK] = CsSamplerBlkPtr->lior[CURR];

	    if ( (CsSamplerBlkPtr->liork[CURR] =
		   (Cs_srv_block.cs_wtstatistics.cs_lior_kbytes - liork)
		       / seconds)
		    > CsSamplerBlkPtr->liork[PEAK] )
		CsSamplerBlkPtr->liork[PEAK] = CsSamplerBlkPtr->liork[CURR];

	    if ( (CsSamplerBlkPtr->liow[CURR] =
		   (Cs_srv_block.cs_wtstatistics.cs_liow_done - liow)
		       / seconds)
		    > CsSamplerBlkPtr->liow[PEAK] )
		CsSamplerBlkPtr->liow[PEAK] = CsSamplerBlkPtr->liow[CURR];

	    if ( (CsSamplerBlkPtr->liowk[CURR] =
		   (Cs_srv_block.cs_wtstatistics.cs_liow_kbytes - liowk)
		       / seconds)
		    > CsSamplerBlkPtr->liowk[PEAK] )
		CsSamplerBlkPtr->liowk[PEAK] = CsSamplerBlkPtr->liowk[CURR];

	    /* Transaction rate cannot be determined */
	}

	TMnow(&starttime);
	elapsed = 0;
        bior  = Cs_srv_block.cs_wtstatistics.cs_bior_done;
        biow  = Cs_srv_block.cs_wtstatistics.cs_biow_done;
        dior  = Cs_srv_block.cs_wtstatistics.cs_dior_done;
        diork = Cs_srv_block.cs_wtstatistics.cs_dior_kbytes;
        diow  = Cs_srv_block.cs_wtstatistics.cs_diow_done;
        diowk = Cs_srv_block.cs_wtstatistics.cs_diow_kbytes;
        lior  = Cs_srv_block.cs_wtstatistics.cs_lior_done;
	liork = Cs_srv_block.cs_wtstatistics.cs_lior_kbytes;
	liow  = Cs_srv_block.cs_wtstatistics.cs_liow_done;
	liowk = Cs_srv_block.cs_wtstatistics.cs_liow_kbytes;
    }

    sleeptime = CsSamplerBlkPtr->interval;

    UnlockSamplerBlk(&hCsSamplerSem);

    CSms_thread_nap( sleeptime );
  } /* for (;;) */

    /* NOTREACHED */

} /* CS_sampler */


/****************************************************************************
**
** Name: AddMutex - keeps track of which mutexes (semaphores) are used.
**
** Description:
**      This routine uses a simple hash to maintain a table of semaphore names.
**	When called, it finds the name of the semaphore in the hash table, or
**	adds it if not already there. Then the routine increments the count
**	for this semaphore.
**
**	The hash routine is borrowed from the UNIX System V algorithm by ELF
**	(executable and linking format) to hash strings.
**
** Inputs:
**	Pointer to a CS_SEMAPHORE block.
**	Thread type.
**	The GLOBALREF PTR CS_samplerBlkPtr - Pointer to the Sampler Block.
**
** Outputs:
**	All statistics are in the Sampler BLock pointed to by CS_samplerBlkPtr.
**	
** Returns:
** 
** Exceptions:
** 
** Side Effects:
**
**  History:
**	12-sep-95 (wonst02)
**	    Original.
**
****************************************************************************/

static VOID
AddMutex( CS_SEMAPHORE *CsSem,
	  i4		thread_type )
{
    ulong	hashnum,
    		hashstart;
    char	*hashname;
    MUTEXES	*hashtable = CsSamplerBlkPtr->Mutex;
    
    ++CsSamplerBlkPtr->nummutexsamples;	/* count the total number */

    if (CsSem && CsSem->cs_sem_name[0] != '\0')
    {	/* A "named" semaphore */
    	hashnum = CSsamp_elf_hash((const unsigned char *)CsSem->cs_sem_name)
		% MAXMUTEXES;
	hashname = CsSem->cs_sem_name;
    }
    else
    {   /* All "unnamed" semaphores use the last slot */
    	hashnum = MAXMUTEXES;
	hashname = "<unnamed>";
    }

    hashstart = hashnum;
    do 
    {
	if (hashtable[hashnum].name[0] == '\0')
	{   /* Slot available: add semaphore entry to the table */
	    STcopy(hashname, hashtable[hashnum].name);
	    hashtable[hashnum].id = (char *) CsSem;
	    hashtable[hashnum].count[thread_type] = 1;
	    break;
	}
	if (STcompare(hashname, hashtable[hashnum].name) == 0)
	{   /* Found the correct semaphore entry...increment the count. */
	    ++hashtable[hashnum].count[thread_type];
	    hashtable[hashnum].id = (char *) CsSem;
	    break;
	}
	/* This is a hash collision...see if the next slot is available. */
	/* If past the end of the table, wrap around to beginning.	 */
	if (++hashnum >= MAXMUTEXES)		
	    hashnum = 0;		
    } while (hashnum != hashstart);
    /*
    ** If we fall out of the 'do' loop without finding a slot (the table is
    ** full), just ignore this semaphore.
    */
} /* AddMutex() */

/****************************************************************************
**
** Name: AddLock - keeps track of which locks are being waited on.
**
** Description:
**      This routine uses a simple hash to maintain a table of lock keys.
**	When called, it finds the lock key in the hash table or adds it
**	if not already there. Then the routine increments the count for
**	this lock.
**
** Inputs:
**	Copy of a LK_LOCK_KEY block.
**	Thread type.
**	The GLOBALREF PTR CS_samplerBlkPtr - Pointer to the Sampler Block.
**
** Outputs:
**	All statistics are in the Sampler BLock pointed to by CS_samplerBlkPtr.
**	
** Returns:
** 
** Exceptions:
** 
** Side Effects:
**
**  History:
**	17-dec-96 (wonst02)
**	    Original.
**
****************************************************************************/

static ulong
LockHash(LK_LOCK_KEY	*LkKey)
{
    ulong	h = 0,
    		g;
    i4		i;
    uchar	*name = (uchar *)LkKey;
    	
    for (i = sizeof(LK_LOCK_KEY); i > 0; i--)
    {
    	h = (h << 4) + *name++;
	if ((g = (h & 0xF0000000)) != 0)
	    h ^= g >> 24;
	h &= ~g;
    }
    return h;
}

	
static VOID
AddLock( LK_LOCK_KEY	LkKey,
	 i4		thread_type )
{
    ulong	hashnum,
    		hashstart;
    LOCKS	*hashtable = CsSamplerBlkPtr->Lock;
    
    ++CsSamplerBlkPtr->numlocksamples;	/* count the total number */

    if (LkKey.lk_type)
    {	/* A lock key */
    	hashnum = LockHash(&LkKey) % MAXLOCKS;
    }
    else
    {   /* All "unnamed" locks use the last slot */
    	hashnum = MAXLOCKS;
    }

    hashstart = hashnum;
    do 
    {
	if (hashtable[hashnum].lk_key.lk_type == 0)
	{   /* Slot available: add lock entry to the table */
	    MEcopy((PTR)&LkKey, sizeof(LK_LOCK_KEY), 
	    	   (PTR)&hashtable[hashnum].lk_key);
	    ++hashtable[hashnum].count[thread_type];
	    break;
	}
	if (MEcmp((PTR)&LkKey, (PTR)&hashtable[hashnum].lk_key,
		  sizeof(LK_LOCK_KEY)) == 0)
	{   /* Found the correct lock entry...increment the count. */
	    ++hashtable[hashnum].count[thread_type];
	    break;
	}
	/* This is a hash collision...see if the next slot is available. */
	/* If past the end of the table, wrap around to beginning.	 */
	if (++hashnum >= MAXLOCKS)
	    hashnum = 0;		
    } while (hashnum != hashstart);
    /*
    ** If we fall out of the 'do' loop without finding a slot (the table is
    ** full), just ignore this lock.
    */
} /* AddLock() */
# endif /* OS_THREADS_USED */
