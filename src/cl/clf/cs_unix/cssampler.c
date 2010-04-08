/******************************************************************************
**
**Copyright (c) 2004 Ingres Corporation
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
**	13-dec-1996 (canor01)
**	    Treat special case where cs_state is COMPUTABLE, but cs_mask
**	    is MUTEX_MASK.
**	06-Oct-1998 (jenjo02)
**	    Changed MAXTHREADS to MAXSAMPTHREADS, brought up to date with
**	    csmtsampler.
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
****************************************************************************/

# include <compat.h>
# include <sp.h>
# include <cs.h>
# include <er.h>
# include <ex.h>
# include <lo.h>
# include <me.h>
# include <st.h>
# include <tr.h>
# include <cv.h>
# include <pc.h>
# include <csinternal.h>
# include "cssampler.h"

GLOBALREF CSSAMPLERBLKPTR CsSamplerBlkPtr;
GLOBALREF CS_SYSTEM	  Cs_srv_block;

/* Forward References */
VOID
AddMutex( CS_SEMAPHORE	*CsSem,
	  i4		thread_type );

static	CS_SCB	samp_scb;


VOID
CS_sampler(void)
{
	CS_SCB	*an_scb;
	i4	sleeptime;
	i4	cs_thread_type;
	i4	cs_state;

  /*
  ** This thread goes into a loop:
  **	1. Lock the sampler block
  **	2. Do sampling
  **	3. Sleep for the specified interval
  ** The thread will exit normally when the sampler block pointer is NULL.
  ** The thread exits abnormally if it cannot lock the block.
  */

  for (;;)
  {
    CSget_scb(&an_scb);
    if ( CsSamplerBlkPtr == NULL || 
         (an_scb->cs_mask & CS_DEAD_MASK) ||
         (an_scb->cs_mask & CS_IRPENDING_MASK))
    {
	return;
    }

    ++CsSamplerBlkPtr->numsamples;   /* Count the number of times we sample */

    /* Loop thru all the SCBs in the server */
    for (an_scb = Cs_srv_block.cs_known_list->cs_next;
    	 an_scb && an_scb != Cs_srv_block.cs_known_list;
	 an_scb = an_scb->cs_next)
    {
	/* skip the sampler thread and the monitor thread */
	if (an_scb == &samp_scb ||
	    (an_scb->cs_mask & CS_MNTR_MASK) )
	    continue;

	if (an_scb->cs_thread_type >= -1 && 
	    an_scb->cs_thread_type <= MAXSAMPTHREADS - 1)
	    cs_thread_type = an_scb->cs_thread_type;
	else
	    cs_thread_type = MAXSAMPTHREADS - 1;	/* use the <invalid> type */

	if (an_scb->cs_state >= 0 &&
	    an_scb->cs_state <= MAXSTATES - 1) 
	    cs_state = an_scb->cs_state;
	else
	    cs_state = MAXSTATES - 1;

	switch (cs_state)
	{
	    i4  facility;

	    case CS_COMPUTABLE:
		if ( an_scb->cs_mask == CS_MUTEX_MASK )
		{
		    /* really waiting on a mutex */
                    ++CsSamplerBlkPtr->
                      Thread[cs_thread_type].numthreadsamples;
                    ++CsSamplerBlkPtr->             /* count current state */
                      Thread[cs_thread_type].state[CS_MUTEX];
                    AddMutex( ((CS_SEMAPHORE *)an_scb->cs_sync_obj),
                                    cs_thread_type );
		}
		else
		{
		    ++CsSamplerBlkPtr->
		      Thread[cs_thread_type].numthreadsamples;
		    ++CsSamplerBlkPtr->		/* count current state */
		      Thread[cs_thread_type].state[cs_state];
		    facility = (*Cs_srv_block.cs_facility)(an_scb);
		    if (facility >= MAXFACS || facility < 0)
		        facility = MAXFACS - 1;
		    ++CsSamplerBlkPtr->		/* count current facility */
		      Thread[cs_thread_type].facility[facility];
		}
		break;

	    case CS_FREE:
	    case CS_STACK_WAIT:
	    case CS_UWAIT:
		++CsSamplerBlkPtr->
		  Thread[cs_thread_type].numthreadsamples;
		++CsSamplerBlkPtr->		/* count current state */
		  Thread[cs_thread_type].state[cs_state];
		break;

	    case CS_EVENT_WAIT:
		++CsSamplerBlkPtr->
		  Thread[cs_thread_type].numthreadsamples;
		++CsSamplerBlkPtr->		/* count current state */
		  Thread[cs_thread_type].state[cs_state];
		switch (cs_thread_type)
		{
		    case CS_USER_THREAD:
			++CsSamplerBlkPtr->numusereventsamples;
			++CsSamplerBlkPtr->		/* count event type */
		  	  userevent[(an_scb->cs_memory & CS_DIO_MASK ?
				      0 :
				     an_scb->cs_memory & CS_BIO_MASK ?
				      1 :
				     an_scb->cs_memory & CS_LOCK_MASK ?
		    		      2 :
		    		     an_scb->cs_memory & CS_LOG_MASK ?
				      3 :
		    		     an_scb->cs_memory & CS_LGEVENT_MASK ?
				      4 :
		    		     an_scb->cs_memory & CS_LKEVENT_MASK ?
				      5 :
				     /* else it is ...   unknown */
		    		      6)];
			break;
		    default:
			++CsSamplerBlkPtr->numsyseventsamples;
			++CsSamplerBlkPtr->		/* count event type */
		  	  sysevent[(an_scb->cs_memory & CS_DIO_MASK ?
				      0 :
				    an_scb->cs_memory & CS_BIO_MASK ?
				      1 :
				    an_scb->cs_memory & CS_LOCK_MASK ?
		    		      2 :
		    		    an_scb->cs_memory & CS_LOG_MASK ?
				      3 :
		    		    an_scb->cs_memory & CS_LGEVENT_MASK ?
				      4 :
		    		    an_scb->cs_memory & CS_LKEVENT_MASK ?
				      5 :
				    /* else it is ...   unknown */
		    		      6)];
			break;
		} /* switch (cs_thread_type) */
		break;

	    case CS_MUTEX:
		++CsSamplerBlkPtr->
		  Thread[cs_thread_type].numthreadsamples;
		++CsSamplerBlkPtr->		/* count current state */
		  Thread[cs_thread_type].state[cs_state];
		AddMutex( ((CS_SEMAPHORE *)an_scb->cs_sync_obj),
				cs_thread_type );
		break;

	    case CS_CNDWAIT:
		++CsSamplerBlkPtr->
		  Thread[cs_thread_type].numthreadsamples;
		++CsSamplerBlkPtr->		/* count current state */
		  Thread[cs_thread_type].state[cs_state];
		break;

	    default:
		++CsSamplerBlkPtr->
		  Thread[cs_thread_type].numthreadsamples;
		++CsSamplerBlkPtr->		/* count "invalid" state */
		  Thread[cs_thread_type].state[MAXSTATES - 1];
		break;
	} /* switch (cs_state) */
    } /* for */


  sleeptime = CsSamplerBlkPtr->interval;

  CS_realtime_update_smclock();

  CSms_thread_nap( sleeptime );
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
    u_i4	h = 0,
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


VOID
AddMutex( CS_SEMAPHORE *CsSem,
	  i4		thread_type )
{
    u_i4	hashnum,
    		hashstart;
    char	*hashname;
    MUTEXES	*hashtable = CsSamplerBlkPtr->Mutex;
    
    ++CsSamplerBlkPtr->nummutexsamples;	/* count the total number */

    if (CsSem->cs_sem_name[0] != '\0')
    {	/* A "named" semaphore */
    	hashnum = ElfHash((const unsigned char *)CsSem->cs_sem_name) % MAXMUTEXES;
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
