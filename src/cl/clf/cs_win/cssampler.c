/******************************************************************************
**
** Copyright (c) 1995, 2000 Ingres Corporation
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
**	15-may-97 (mcgem01)
**	    Clean up complier warning.
**	16-Oct-1997 (shero03)
**	    Synched the sampler code with csmtmonitor & change 432396.
** 	24-Nov-1998 (wonst02)
** 	    Corrected what looked to be an error in AddLock's hash table code.
** 	07-Jan-1999 (wonst02)
** 	    Add new event types: CS_LIO_MASK (Log I/O), et al.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	24-aug-1999 (somsa01)
**	    Moved the calls to MOattach() and MOdetach() that were in
**	    StartSampler() and StopSampler() (csmonitor.c) to here.
**	    In this way, the MO semaphores would not be locked by the thread
**	    who created this thread (deadlock). Also, this thread now
**	    frees up the CsSamplerBlkPtr and the hCsSamplerSem.
**	10-jan-2000 (somsa01)
**	    In AddMutex(), set the Mutex id. Also, added more statistics.
**	11-oct-2000 (somsa01 for jenjo02)
**	    Break out event wait counts by thread type, keep track
**	    of I/O and transaction rates. Extract Writebehind and
**	    Sort Factotum thread stats.
**	11-oct-2000 (somsa01 for jenjo02)
**	    Backed out calls to LG, prohibited by Ingres encapsulation.
**	    Can't get transaction rates but through LG, so they'll
**	    just show as zero until someone comes up with a better
**	    idea.
**
****************************************************************************/

# include <compat.h>
# include <sp.h>
# include <cs.h>
# include <er.h>
# include <ex.h>
# include <gl.h>
# include <lo.h>
# include <iicommon.h>
# include <me.h>
# include <mo.h>
# include <st.h>
# include <tr.h>
# include <cv.h>
# include <pc.h>
# include <lk.h>
# include <csinternal.h>
# include "cssampler.h"

GLOBALREF CSSAMPLERBLKPTR CsSamplerBlkPtr;
GLOBALREF HANDLE          hCsSamplerSem;
GLOBALREF CS_SYSTEM	  Cs_srv_block;
GLOBALREF char		  CSsamp_index_name[];
GLOBALREF bool		  CSsamp_stopping;

/* Forward References */
static VOID
AddLock( LK_LOCK_KEY	LkKey,
	 i4 		thread_type );

static VOID
AddMutex( CS_SEMAPHORE	*CsSem,
	  i4 		thread_type );

static	CS_SCB	samp_scb;
static	LK_LOCK_KEY	dummy_lock	ZERO_FILL;


VOID
CS_sampler(void)
{
	CS_SCB	*an_scb;
	i4 	sleeptime, elapsed, seconds, event;
	i4	starttime, stoptime;
	i4 	cs_thread_type;
	i4 	cs_state;
	bool	attached = FALSE;
	u_i4	bior, biow;
	u_i4	dior, diork, diow, diowk;
	u_i4	lior, liork, liow, liowk;

  /*
  ** This thread goes into a loop:
  **	1. Lock the sampler block
  **	2. Do sampling
  **	3. Sleep for the specified interval
  ** The thread will exit normally when the sampler block pointer is NULL.
  ** The thread exits abnormally if it cannot lock the block.
  */
  starttime = CS_checktime();
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
    	ExitThread((DWORD)-1);
    }

    if (CsSamplerBlkPtr->shutdown)
    {
	/*
	** Detach the sampler block Managed Object
	*/
	if (attached)
	    MOdetach(CSsamp_index_name, "CsSamplerBlkPtr");

	MEfree((PTR)CsSamplerBlkPtr);
	CsSamplerBlkPtr = NULL;

	CSsamp_stopping = TRUE;
    	UnlockSamplerBlk(hCsSamplerSem);
	CloseHandle(hCsSamplerSem);
	hCsSamplerSem = NULL;

    	ExitThread(0);
    }

    if (!attached)
    {
	/*
	** Attach the sampler block Managed Object
	*/
	MOattach(MO_INSTANCE_VAR, CSsamp_index_name, "CsSamplerBlkPtr",
		 (PTR) CsSamplerBlkPtr);
	attached = TRUE;
    }

    ++CsSamplerBlkPtr->numsamples;   /* Count the number of times we sample */

    /* Loop thru all the SCBs in the server */
    for (an_scb = Cs_srv_block.cs_known_list->cs_next;
    	 an_scb && an_scb != Cs_srv_block.cs_known_list;
	 an_scb = an_scb->cs_next)
    {
	if (an_scb->cs_thread_type >= -1 && 
	    an_scb->cs_thread_type <= MAXSAMPTHREADS - 1)
	    cs_thread_type = an_scb->cs_thread_type;
	else
	    cs_thread_type = MAXSAMPTHREADS - 1; /* use the <invalid> thread */

	/* If Factotum thread, try to isolate which kind */
	if ( cs_thread_type == CS_FACTOTUM )
	{
	    if ( MEcmp((char *)&an_scb->cs_username, " <WriteBehind", 13) == 0 )
		cs_thread_type = CS_WRITE_BEHIND;
	    else if ( MEcmp((char *)&an_scb->cs_username, " <Sort", 6) == 0 )
		cs_thread_type = CS_SORT;
	}

	if (an_scb->cs_state >= 0 &&
	    an_scb->cs_state <= MAXSTATES - 1) 
	    cs_state = an_scb->cs_state;
	else
	    cs_state = MAXSTATES - 1;		/* use the <invalid> state */

	++CsSamplerBlkPtr->Thread[cs_thread_type].numthreadsamples;

	if ( cs_thread_type == CS_NORMAL )
	    ++CsSamplerBlkPtr->totusersamples;
	else
	    ++CsSamplerBlkPtr->totsyssamples;

	switch (cs_state)
	{
	    case CS_COMPUTABLE:		/* Count current facility */
	    {	
	    	i4  facility;

		++CsSamplerBlkPtr->Thread[cs_thread_type].state[cs_state];
		facility = (*Cs_srv_block.cs_facility)(an_scb);
		if (facility >= MAXFACS || facility < 0)
		    facility = MAXFACS - 1;
		++CsSamplerBlkPtr->Thread[cs_thread_type].facility[facility];
		break;
	    }

	    case CS_EVENT_WAIT:		/* Count event types */
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

	    /* Uninteresting states */
	    default:
		++CsSamplerBlkPtr->Thread[cs_thread_type].state[cs_state];
		break;
	} /* switch (cs_state) */
    } /* for */

    /*
    ** If a second or more worth of intervals appear to have elapsed,
    ** compute current and peak per-second I/O, Transaction rates.
    */
    if ( (elapsed += CsSamplerBlkPtr->interval) >= 1000 )
    {
	/* Get the current time; the interval is not reliable! */
	stoptime = CS_checktime();

	if ( (seconds = stoptime - starttime) )
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

	starttime = CS_checktime();
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

    UnlockSamplerBlk(hCsSamplerSem);

    Sleep (sleeptime);
  } /* for (;;) */

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

u_i4 
ElfHash(const unsigned char *name)
{
    u_i4 	h = 0,
    		g;

    while (*name)
    {
    	h = (h << 4) + *name++;
	if (g = h & 0xF0000000)
	    h ^= g >> 24;
	h &= ~g;
    }
    return h;
}


static
VOID
AddMutex( CS_SEMAPHORE *CsSem,
	  i4 		thread_type )
{
    u_i4 	hashnum,
    		hashstart;
    char	*hashname;
    MUTEXES	*hashtable = CsSamplerBlkPtr->Mutex;
    
    ++CsSamplerBlkPtr->nummutexsamples;	/* count the total number */

    if (CsSem->cs_sem_name[0] != '\0')
    {	/* A "named" semaphore */
    	hashnum = ElfHash(CsSem->cs_sem_name) % MAXMUTEXES;
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
	++hashnum;
	if (hashnum >= MAXMUTEXES)		
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
** 	24-Nov-1998 (wonst02)
** 	    Corrected what looked to be an error in AddLock's hash table code.
**
****************************************************************************/

static ulong
LockHash(LK_LOCK_KEY	*LkKey)
{
    ulong	h = 0,
    		g;
    i4 		i;
    uchar	*name = (uchar *)LkKey;
    	
    for (i = sizeof(LK_LOCK_KEY); i > 0; i--)
    {
    	h = (h << 4) + *name++;
	if (g = h & 0xF0000000)
	    h ^= g >> 24;
	h &= ~g;
    }
    return h;
}

	
static VOID
AddLock( LK_LOCK_KEY	LkKey,
	 i4 		thread_type )
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
	    hashtable[hashnum].count[thread_type] = 1;
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

