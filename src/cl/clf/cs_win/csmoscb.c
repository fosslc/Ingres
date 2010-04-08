/*
** Copyright (C) 1987, 2004 Ingres Corporation.  All Rights Reserved.
*/

# include <compat.h>
# include <sys\types.h>
# include <sp.h>
# include <pc.h>
# include <cs.h>
# include <mo.h>
# include <tr.h>
# include <csinternal.h>

# include "cslocal.h"

# include "csmgmt.h"

/*
** Name:	csmoscb.c	- MO interface to thread SCBs
**
** Description:
**	 MO interfaces for NTX CS SCBs.
**
** Functions:
**
**	CS_scb_attach		- make an SCB known to MO/IMA
**	CS_detach_scb		- make an SCB unknown to MO/IMA
**	CS_scb_index		- MO index functions for known CS SCB's
**	CS_scb_self_get		- MO get function for cs_self field
**	CS_state_get		- MO get function for cs_state field
**	CS_memory_get		- MO get function for cs_memory field
**	CS_thread_id_get	- MO get function for cs_thread_id field
**	CS_thread_type_get	- MO get function for cs_thread_type field
**	CS_scb_mask_get		- MO get function for cs_ mask fields.
**	CS_scb_cpu_get		- MO get function for cs_cputime field.
**	CS_scb_inkernelget	- MO get function for cs_inkernel field.
**	CSmo_bio_done_get	- MO get function for sum of BIO r/w done.
**	CSmo_bio_waits_get	- MO get function for sum of BIO r/w waits.
**	CSmo_bio_time_get	- MO get function for sum of BIO r/w time.
**	CSmo_dio_done_get	- MO get function for sum of DIO r/w done.
**	CSmo_dio_waits_get	- MO get function for sum of DIO r/w waits.
**	CSmo_dio_time_get	- MO get function for sum of DIO r/w time.
**	CSmo_lio_done_get	- MO get function for sum of LIO r/w done.
**	CSmo_lio_waits_get	- MO get function for sum of LIO r/w waits.
**	CSmo_lio_time_get	- MO get function for sum of LIO r/w time.
**	CS_scb_bio_get		- MO get function for sum of BIO r/w.
**	CS_scb_dio_get		- MO get function for sum of DIO r/w.
**	CS_scb_lio_get		- MO get function for sum of LIO r/w.
**	CSmo_num_active_get	- MO get function for active sessions.
**	CSmo_hwm_active_get	- MO get function for high water mark.
**
** History:
**	26-Oct-1992 (daveb)
**	    Add checks/logging in attach/detach routines.  Was getting
**	    segv's after re-attaching an SCB that had it's links zeroed
**	    out.
**	1-Dec-1992 (daveb)
**	    Use unsigned long out for SCB instance values, because they
**	    might be negative as pointer values.
**	13-Jan-1993 (daveb)
**	    Rename MO objects to include a .unix component.
**	18-jan-1993 (mikem)
**	    Initialized str to point to buf in the CS_scb_memory_get() routine.
**	    It was not initialized in the CS_MUTEX case.
**	31-aug-1995 (shero03)
**	    Calculate cpu time for each thread.
**	12-sep-1995 (shero03)
**	    Added cs_sync_obj
**      12-Dec-95 (fanra01)
**          Extracted data for DLLs on windows NT.
**	27-may-97 (mcgem01)
**	    Cleaned up compiler warnings.
**      13-Feb-98 (fanra01)
**          Modified methods for index, attach and detach to use SID as
**          instance for searching but still return the scb.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	24-jul-2000 (somsa01 for jenjo02)
**	    Added exp.clf.unix.cs.scb_thread_id and CS_thread_id_get
**	    method to return OS thread id.
**	10-oct-2000 (somsa01)
**	    Deleted cs_bio_done, cs_bio_time, cs_bio_waits, cs_dio_done,
**	    cs_dio_time, cs_dio_waits from Cs_srv_block, cs_dio, cs_bio
**	    from CS_SCB. Kept the MO objects for them and added MO
**	    methods to compute their values.
**	13-oct-2003 (somsa01)
**	    Use MOsidout() instead of MOptrout() in CS_scb_self_get().
**	14-oct-2003 (somsa01)
**	    Use MOptrout() instead of MOulongout() in CS_scb_index().
**	23-jan-2004 (somsa01)
**	    Added CSmo_num_active_get(), CSmo_hwm_active_get().
**	22-jul-2004 (somsa01)
**	    Removed unnecessary include of erglf.h.
**	09-Feb-2009 (smeke01) b119586
**	    Brought CS_scb_cpu_get() & CS_scb_inkernel_get() into line 
**	    with Linux & Solaris developments. 
**      09-Feb-2010 (smeke01) b123226, b113797 
**	    MOlongout/MOulongout now take i8/u_i8 parameter.
*/


/*
**  Defines of other constants.
*/
 
/* number of 100-nano-seconds in 1 second */
#define		HNANO	10000000
 
/* number of 100-nano-seconds in 1 milli-second */
#define		HNANO_PER_MILLI	10000
 
/* number of milli-seconds in 1 second */
#define		MILLI	1000
/* number of seconds in the lo word  */
#define		SECONDS_PER_DWORD 0xFFFFFFFF / HNANO_PER_MILLI


typedef struct {
	i4              bit;
	char           *str;
}               MASK_BITS;


GLOBALREF CS_SEMAPHORE Cs_known_list_sem;
GLOBALREF MO_CLASS_DEF CS_scb_classes[];

MASK_BITS       mask_bits[] =
{
	{CS_TIMEOUT_MASK, "TIMEOUT"},
	{CS_INTERRUPT_MASK, "INTERRUPT"},
	{CS_BAD_STACK_MASK, "BAD_STACK"},
	{CS_DEAD_MASK, "DEAD"},
	{CS_IRPENDING_MASK, "IRPENDING"},
	{CS_NOXACT_MASK, "NOXACT"},
	{CS_MNTR_MASK, "MNTR"},
	{CS_FATAL_MASK, "FATAL"},
	{CS_MID_QUANTUM_MASK, "MID_QUANTUM"},
	{CS_EDONE_MASK, "EDONE_MASK"},
	{CS_IRCV_MASK, "IRCV"},
	{CS_TO_MASK, "TIMEOUT"},
	{0, 0}
};



/*{
** Name:	CS_scb_attach	- make an SCB known to MO/IMA
**
** Description:
**	Links the specified SCB into the known thread tree.  Logs
**	error to server log if it's already present (it shouldn't).
**
** Re-entrancy:
**	no.  Called with inkernel set, presumabely.
**
** Inputs:
**	scb		the thread to link in.
**
** Outputs:
**	scb		cs_spblk is updated.
**
** Returns:
**	none.
**
** Side Effects:
**	May TRdisplay debug information.
**
** History:
**	26-Oct-1992 (daveb)
**	    documented.
**      13-Feb-98 (fanra01)
**          Modified to use the SID as the key.
**	02-oct-2007 (thaju02)
**	    Take cs_scb_mutex before accessing cs_scb_tree. (B119243)
*/
void
CS_scb_attach(CS_SCB * scb)
{
	SPBLK           node;
	SPBLK          *sp;

	CS_synch_lock( &Cs_srv_block.cs_scb_mutex );
	node.key = (PTR) scb->cs_self;
	sp = SPlookup(&node, Cs_srv_block.cs_scb_ptree);
	if (sp != NULL) {
		TRdisplay("CS_scb_attach: attempt to attach existing SCB %x!!!\n",
			  scb);
	} else {
		scb->cs_spblk.uplink = NULL;
		scb->cs_spblk.leftlink = NULL;
		scb->cs_spblk.rightlink = NULL;
		scb->cs_spblk.key = (PTR) scb->cs_self;
		SPinstall(&scb->cs_spblk, Cs_srv_block.cs_scb_ptree);
	}
	CS_synch_unlock( &Cs_srv_block.cs_scb_mutex );
}




/*{
** Name:	CS_detach_scb	- make an SCB unknown to MO/IMA
**
** Description:
**	Unlinks the specified SCB from the known thread tree.  Best
**	called just before de-allocating the block.  Logs a message
**	to the server log with TRdisplay if the block isn't there
**	(it should be).
**
** Re-entrancy:
**	no.  Called with inkernel set, presumabely.
**
** Inputs:
**	scb		the thread to zap.
**
** Outputs:
**	scb		cs_spblk is updated.
**
** Returns:
**	none.
**
** Side Effects:
**	May TRdisplay debug information.
**
** History:
**	26-Oct-1992 (daveb)
**	    documented.
**      13-Feb-98 (fanra01)
**          Modified to use the SID as the key.
**	02-oct-2007 (thaju02)
**	    Take cs_scb_mutex before accessing cs_scb_tree. (B119243)
*/

void
CS_detach_scb(CS_SCB * scb)
{
	SPBLK           node;
	SPBLK          *sp;

	CS_synch_lock( &Cs_srv_block.cs_scb_mutex );
	node.key = (PTR) scb->cs_self;
	sp = SPlookup(&node, Cs_srv_block.cs_scb_ptree);
	if (sp == NULL) {
		TRdisplay("CS_detach_scb: attempt to detach unknown SCB %x\n",
			  scb);
	} else {
		SPdelete(&scb->cs_spblk, Cs_srv_block.cs_scb_ptree);
	}
	CS_synch_unlock( &Cs_srv_block.cs_scb_mutex );
}

/* ---------------------------------------------------------------- */

/*{
** Name:	CS_scb_index	- MO index functions for known CS SCB's
**
** Description:
**	Standard MO index function to get at the nodes in the known
**	scb tree.
**
** Re-entrancy:
**	NO; this may be a problem in real MP environments.  FIXME.
**
** Inputs:
**	msg		the index op to perform
**	cdata		the class data, ignored.
**	linstance	the length of the instance buffer.
**	instance	the input instance
**
** Outputs:
**	instance	on GETNEXT, updated to be the one found.
**	idata		updated to point to the SCB in question.
**
** Returns:
**	standard index function returns.
**
** History:
**	26-Oct-1992 (daveb)
**	    documented.
**	1-Dec-1992 (daveb)
**	    Use MOulongout, 'cause it might be "negative" as a pointer.
**      13-Feb-98 (fanra01)
**          Modified to use SID as instance but returns the scb as the
**          instance data to be passed to methods.  Incurs a lookup performance
**          penalty.
**	13-oct-2003 (somsa01)
**	    Use MOptrout() instead of MOulongout().
**	02-oct-2007 (thaju02)
**	    Take cs_scb_mutex before accessing cs_scb_tree. (B119243)
*/

STATUS
CS_scb_index(i4  msg,
	     PTR cdata,
	     i4  linstance,
	     char *instance,
	     PTR * instdata)
{
	STATUS          stat = OK;
	PTR             ptr;

	CS_synch_lock( &Cs_srv_block.cs_scb_mutex );
	switch (msg) {
	case MO_GET:
		if (OK == (stat = CS_get_block(instance,
					       Cs_srv_block.cs_scb_ptree,
					       &ptr)))
			*instdata = (PTR) CS_find_scb((CS_SID) ptr);
		break;

	case MO_GETNEXT:
		if (OK == (stat = CS_nxt_block(instance,
					       Cs_srv_block.cs_scb_ptree,
					       &ptr))) {
			*instdata = (PTR) CS_find_scb((CS_SID) ptr);
			stat = MOptrout(MO_INSTANCE_TRUNCATED,
                                        ptr,
					linstance,
					instance);
		}
		break;

	default:
		stat = MO_BAD_MSG;
		break;
	}
	CS_synch_unlock( &Cs_srv_block.cs_scb_mutex );
	return (stat);
}

/* ---------------------------------------------------------------- */


/*{
** Name:	CS_scb_self_get	- MO get function for cs_self field
**
** Description:
**	A nice interpretation of the cs_self fields, special
**	casing CS_DONT_CARE and CS_ADDER_ID.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		offset into the SCB, ignored.
**	objsize		size of the cs_self field, ignored.
**	object		the CS_SCB in question.
**	luserbuf	length of output buffer
**
**
** Outputs:
**	userbuf		written with string, one of "CS_DONT_CARE",
**			"CS_ADDER_ID", or decimal value of the block.
**
** Returns:
**	standard get function returns.
**
** History:
**	26-Oct-1992 (daveb)
**	    documented.  Use MOlongout instead of CVla.
**	13-oct-2003 (somsa01)
**	    Use MOsidout() instead of MOptrout().
**	14-oct-2003 (somsa01)
**	    Increase buf to 24.
*/

STATUS
CS_scb_self_get(i4  offset,
		i4  objsize,
		PTR object,
		i4  luserbuf,
		char *userbuf)
{
	char            buf[24];
	char           *str = buf;
	CS_SCB         *scb = (CS_SCB *) object;

	if (scb->cs_self == CS_DONT_CARE)
		str = "CS_DONT_CARE";
	else if (scb->cs_self == CS_ADDER_ID)
		str = "CS_ADDER_ID";
	else
		(void) MOsidout(0, (PTR)scb->cs_self, sizeof(buf), buf);

	return (MOstrout(MO_VALUE_TRUNCATED, str, luserbuf, userbuf));
}



/*{
** Name:	CS_state_get	- MO get function for cs_state field
**
** Description:
**	A nice interpretation of the cs_state fields.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		offset into the SCB, ignored.
**	objsize		size of the cs_self field, ignored.
**	object		the CS_SCB in question.
**	luserbuf	length of output buffer
**
**
** Outputs:
**	userbuf		written with nice state string.
**
** Returns:
**	standard get function returns.
**
** History:
**	26-Oct-1992 (daveb)
**	    documented.
*/
STATUS
CS_scb_state_get(i4  offset,
		 i4  objsize,
		 PTR object,
		 i4  luserbuf,
		 char *userbuf)
{
	CS_SCB         *scb = (CS_SCB *) object;
	char           *str;

	switch (scb->cs_state) {
	case CS_FREE:
		str = "CS_FREE";
		break;
	case CS_COMPUTABLE:
		str = "CS_COMPUTABLE";
		break;
	case CS_STACK_WAIT:
		str = "CS_STACK_WAIT";
		break;
	case CS_UWAIT:
		str = "CS_UWAIT";
		break;
	case CS_EVENT_WAIT:
		str = "CS_EVENT_WAIT";
		break;
	case CS_MUTEX:
		str = "CS_MUTEX";
		break;
	case CS_CNDWAIT:
		str = "CS_CNDWAIT";
		break;

	default:
		str = "<invalid>";
		break;
	}

	return (MOstrout(MO_VALUE_TRUNCATED, str, luserbuf, userbuf));
}




/*{
** Name:	CS_memory_get	- MO get function for cs_memory field
**
** Description:
**	A nice interpretation of the cs_memory field.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		offset into the SCB, ignored.
**	objsize		size of the cs_self field, ignored.
**	object		the CS_SCB in question.
**	luserbuf	length of output buffer
**
**
** Outputs:
**	userbuf		written with string.
**
** Returns:
**	standard get function returns.
**
** History:
**	26-Oct-1992 (daveb)
**	    documented.
**	18-jan-1993 (mikem)
**	    Initialized str to point to buf in the CS_scb_memory_get() routine.
**	    It was not initialized in the CS_MUTEX case.
*/
STATUS
CS_scb_sync_obj_get(i4  offset,
		    i4  objsize,
		    PTR object,
		    i4  luserbuf,
		    char *userbuf)
{
	STATUS          stat;
	char            buf[80];
	char           *str = buf;

	CS_SCB         *scb = (CS_SCB *) object;

	switch (scb->cs_state) {
	case CS_EVENT_WAIT:

		str = scb->cs_memory & CS_DIO_MASK ?
			"DIO" :
			scb->cs_memory & CS_BIO_MASK ?
			"BIO" :
			scb->cs_memory & CS_LOCK_MASK ?
			"LOCK" :
			"LOG-IO";
		stat = MOstrout(MO_VALUE_TRUNCATED, str, luserbuf, userbuf);
		break;

	case CS_MUTEX:

		buf[0] = 'x';
		buf[1] = ' ';
		stat = MOptrout(0, scb->cs_sync_obj, sizeof(buf), &buf[2]);
		stat = MOstrout(MO_VALUE_TRUNCATED, str, luserbuf, userbuf);
		break;

	case CS_CNDWAIT:

		stat = MOptrout(MO_VALUE_TRUNCATED,
				scb->cs_sync_obj, luserbuf, userbuf);
		break;

	default:

		stat = MOptrout(MO_VALUE_TRUNCATED, scb->cs_sync_obj,
				  luserbuf, userbuf);

		break;
	}
	return (stat);
}



/*{
** Name:	CS_thread_type_get	- MO get function for cs_thread_type field
**
** Description:
**	A nice interpretation of the cs_thread_type.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		offset into the SCB, ignored.
**	objsize		size of the cs_self field, ignored.
**	object		the CS_SCB in question.
**	luserbuf	length of output buffer
**
**
** Outputs:
**	userbuf		written with string, one of "internal", "user",
**			or decimal value of the block.
**
** Returns:
**	standard get function returns.
**
** History:
**	26-Oct-1992 (daveb)
**	    documented.  Use MOlongout instead of CVla.
*/

STATUS
CS_scb_thread_type_get(i4  offset,
		       i4  objsize,
		       PTR object,
		       i4  luserbuf,
		       char *userbuf)
{
	CS_SCB         *scb = (CS_SCB *) object;
	char            buf[80];
	char           *str = buf;

	if (scb->cs_thread_type == CS_INTRNL_THREAD)
		str = "internal";
	else if (scb->cs_thread_type == CS_USER_THREAD)
		str = "user";
	else
		(void) MOlongout(0, scb->cs_thread_type, sizeof(buf), buf);

	return (MOstrout(MO_VALUE_TRUNCATED, str, luserbuf, userbuf));
}



/*{
** Name:	CS_scb_mask_get	- MO get function for cs_ mask fields.
**
** Description:
**	A nice interpretation of mask fields.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		offset into the SCB of the mask field.
**	objsize		size of the mask field, ignored.
**	object		the CS_SCB in question.
**	luserbuf	length of output buffer
**
** Outputs:
**	userbuf		written with string, one of "CS_DONT_CARE",
**			"CS_ADDER_ID", or decimal value of the block.
**
** Returns:
**	standard get function returns.
**
** History:
**	26-Oct-1992 (daveb)
**	    documented.
*/

STATUS
CS_scb_mask_get(i4  offset,
		i4  objsize,
		PTR object,
		i4  luserbuf,
		char *userbuf)
{
	char            buf[80];
	i4              mask = *(i4 *) ((char *) object + offset);
	MASK_BITS      *mp;
	char           *p = buf;
	char           *q;

	for (mp = mask_bits; mp->str != NULL; mp++) {
		if (mask & mp->bit) {
			if (p != buf)
				*p++ = ',';
			for (q = mp->str; *p++ = *q++;)
				continue;
		}
	}
	*p = EOS;
	return (MOstrout(MO_VALUE_TRUNCATED, buf, luserbuf, userbuf));
}

/* ---------------------------------------------------------------- */

/*{
** Name:	CS_scb_cpu_get	- MO get function for cs_cpu field.
**
** Description:
**	Obtain the cpu time this thread has used from the OS.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		offset into the SCB of the mask field.
**	objsize		size of the mask field, ignored.
**	object		the CS_SCB in question.
**	luserbuf	length of output buffer
**
** Outputs:
**	userbuf		The total amount of cpu time in milliseconds
**			used by this thread.
**
** Returns:
**	Standard get function returns. Note that the aim is NOT to 
**	return an error if at all possible. This is because returning an 
**	error in IMA for even just one value in one row causes no data 
**	to be returned for ANY rows in the query, with no error message 
**	to the front end client. An acceptable alternative is to return 
**	return zero as the value. 
**
** History:
**	31-Aug-1995 (shero03)
**	    created.
**	09-Feb-2009 (smeke01) b119586
**	    Brought into line with Linux & Solaris developments. Put
**	    guts of original CS_scb_cpu_get() / CS_scb_inkernel_get() 
**	    into one separate function, allowing the possibility of 
**	    calling it from elsewhere. 
*/
STATUS
CS_scb_cpu_get(i4  offset,
		i4  objsize,
		PTR object,
		i4  luserbuf,
		char *userbuf)
{
    i4		totalcpu = 0;
    CS_SCB	*pSCB = (CS_SCB *) object;
    CS_THREAD_STATS_CB	thread_stats_cb;

    thread_stats_cb.cs_thread_handle = pSCB->cs_thread_handle; 
    thread_stats_cb.cs_stats_flag = CS_THREAD_STATS_CPU; 
    if (CSMT_get_thread_stats(&thread_stats_cb) == OK)
	totalcpu = thread_stats_cb.cs_usercpu + thread_stats_cb.cs_systemcpu;

    return (MOlongout(MO_VALUE_TRUNCATED, totalcpu, luserbuf, userbuf));
}

/* ---------------------------------------------------------------- */

/*{
** Name:	CS_scb_inkernel_get - MO get function for cs_inkernel field.
**
** Description:
**	Obtain the kernel cpu time this thread has used from the OS.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		offset into the SCB of the mask field.
**	objsize		size of the mask field, ignored.
**	object		the CS_SCB in question.
**	luserbuf	length of output buffer
**
** Outputs:
**	userbuf		The total amount of kernel cpu time in milliseconds
**			used by this thread.
**
** Returns:
**	Standard get function returns. Note that the aim is NOT to 
**	return an error if at all possible. This is because returning an 
**	error in IMA for even just one value in one row causes no data 
**	to be returned for ANY rows in the query, with no error message 
**	to the front end client. An acceptable alternative is to return 
**	return zero as the value. 
**
** History:
**	31-Aug-1995 (shero03)
**	    created.
**	09-Feb-2009 (smeke01) b119586
**	    Brought into line with Linux & Solaris developments. Put
**	    guts of original CS_scb_cpu_get() / CS_scb_inkernel_get() 
**	    into one separate function, allowing the possibility of 
**	    calling it from elsewhere. 
*/

STATUS
CS_scb_inkernel_get(i4  offset,
		i4  objsize,
		PTR object,
		i4  luserbuf,
		char *userbuf)
{
    i4		kernelcpu = 0;
    CS_SCB	*pSCB = (CS_SCB *) object;
    CS_THREAD_STATS_CB	thread_stats_cb;

    thread_stats_cb.cs_thread_handle = pSCB->cs_thread_handle; 
    thread_stats_cb.cs_stats_flag = CS_THREAD_STATS_CPU; 
    if (CSMT_get_thread_stats(&thread_stats_cb) == OK)
	kernelcpu = thread_stats_cb.cs_systemcpu;

	return (MOlongout(MO_VALUE_TRUNCATED, kernelcpu, luserbuf, userbuf));
}

/*{
** Name:	CS_thread_id_get    - MO get function for cs_thread_id field
**
** Description:
**	Returns actual OS thread id when OS_THREADS_USED,
**	zero otherwise.
**
**	Written as a get function to provide for cs_thread_id's
**	that may be structures.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		offset into the SCB, ignored.
**	objsize		size of the cs_thread_id field, ignored.
**	object		the CS_SCB in question.
**	luserbuf	length of output buffer
**
**
** Outputs:
**	userbuf		Decimal value of cs_thread_id.
**
** Returns:
**	standard get function returns.
**
** History:
**	17-Nov-1999 (jenjo02)
**	    Added.
*/

STATUS
CS_thread_id_get(i4 offset,
		i4 objsize,
		PTR object,
		i4 luserbuf,
		char *userbuf)
{
    char buf[ 20 ];
    char *str = buf;
    CS_SCB *scb = (CS_SCB *)object;

    /*
    ** If CS_THREAD_ID is a structure, this will need to be
    ** modified to fit!
    */
    (void) MOlongout( 0, scb->cs_thread_id, sizeof(buf), buf );

    return( MOstrout( MO_VALUE_TRUNCATED, str, luserbuf, userbuf ));
}


/*{
** Name:	CSmo_bio_????_get -- MO get methods for sum of BIO
**				     read/write time, waits, done.
**
** Description:
**	Returns the sum of BIO read/write time, waits, done.
**
** Inputs:
**	offset		ignored.
**	size		ignored.
**	object		ignored.
**	lsbuf		length of output buffer.
**	sbuf		output buffer.
**
** Outputs:
**	sbuf		written with BIO sum, unsigned integer.
**	
**
** Returns:
**	OK
**	MO_VALUE_TRUNCATED
**
** History:
**	20-Sep-2000 (jenjo02)
**	    created.
*/
STATUS
CSmo_bio_time_get(i4 offset,
		  i4 size,
		  PTR object,
		  i4 lsbuf,
		  char *sbuf)
{
    return (MOulongout( MO_VALUE_TRUNCATED,
		(u_i8)(Cs_srv_block.cs_wtstatistics.cs_bior_time +
		        Cs_srv_block.cs_wtstatistics.cs_biow_time),
		lsbuf, sbuf));
}
STATUS
CSmo_bio_waits_get(i4 offset,
		   i4 size,
		   PTR object,
		   i4 lsbuf,
		   char *sbuf)
{
    return (MOulongout( MO_VALUE_TRUNCATED,
		(u_i8)(Cs_srv_block.cs_wtstatistics.cs_bior_waits +
		        Cs_srv_block.cs_wtstatistics.cs_biow_waits),
		lsbuf, sbuf));
}
STATUS
CSmo_bio_done_get(i4 offset,
		  i4 size,
		  PTR object,
		  i4 lsbuf,
		  char *sbuf)
{
    return (MOulongout( MO_VALUE_TRUNCATED,
		(u_i8)(Cs_srv_block.cs_wtstatistics.cs_bior_done +
		        Cs_srv_block.cs_wtstatistics.cs_biow_done),
		lsbuf, sbuf));
}

/*{
** Name:	CSmo_dio_????_get -- MO get methods for sum of DIO
**				     read/write time, waits, done.
**
** Description:
**	Returns the sum of DIO read/write time, waits, done.
**
** Inputs:
**	offset		ignored.
**	size		ignored.
**	object		ignored.
**	lsbuf		length of output buffer.
**	sbuf		output buffer.
**
** Outputs:
**	sbuf		written with DIO sum, unsigned integer.
**	
**
** Returns:
**	OK
**	MO_VALUE_TRUNCATED
**
** History:
**	20-Sep-2000 (jenjo02)
**	    created.
*/
STATUS
CSmo_dio_time_get(i4 offset,
		  i4 size,
		  PTR object,
		  i4 lsbuf,
		  char *sbuf)
{
    return (MOulongout( MO_VALUE_TRUNCATED,
		(u_i8)(Cs_srv_block.cs_wtstatistics.cs_dior_time +
		        Cs_srv_block.cs_wtstatistics.cs_diow_time),
		lsbuf, sbuf));
}
STATUS
CSmo_dio_waits_get(i4 offset,
		   i4 size,
		   PTR object,
		   i4 lsbuf,
		   char *sbuf)
{
    return (MOulongout( MO_VALUE_TRUNCATED,
		(u_i8)(Cs_srv_block.cs_wtstatistics.cs_dior_waits +
		        Cs_srv_block.cs_wtstatistics.cs_diow_waits),
		lsbuf, sbuf));
}
STATUS
CSmo_dio_done_get(i4 offset,
		  i4 size,
		  PTR object,
		  i4 lsbuf,
		  char *sbuf)
{
    return (MOulongout( MO_VALUE_TRUNCATED,
		(u_i8)(Cs_srv_block.cs_wtstatistics.cs_dior_done +
		        Cs_srv_block.cs_wtstatistics.cs_diow_done),
		lsbuf, sbuf));
}

/*{
** Name:	CSmo_lio_????_get -- MO get methods for sum of LIO
**				     read/write time, waits, done.
**
** Description:
**	Returns the sum of LIO read/write time, waits, done.
**
** Inputs:
**	offset		ignored.
**	size		ignored.
**	object		ignored.
**	lsbuf		length of output buffer.
**	sbuf		output buffer.
**
** Outputs:
**	sbuf		written with LIO sum, unsigned integer.
**	
**
** Returns:
**	OK
**	MO_VALUE_TRUNCATED
**
** History:
**	20-Sep-2000 (jenjo02)
**	    created.
*/
STATUS
CSmo_lio_time_get(i4 offset,
		  i4 size,
		  PTR object,
		  i4 lsbuf,
		  char *sbuf)
{
    return (MOulongout( MO_VALUE_TRUNCATED,
		(u_i8)(Cs_srv_block.cs_wtstatistics.cs_lior_time +
		        Cs_srv_block.cs_wtstatistics.cs_liow_time),
		lsbuf, sbuf));
}
STATUS
CSmo_lio_waits_get(i4 offset,
		   i4 size,
		   PTR object,
		   i4 lsbuf,
		   char *sbuf)
{
    return (MOulongout( MO_VALUE_TRUNCATED,
		(u_i8)(Cs_srv_block.cs_wtstatistics.cs_lior_waits +
		        Cs_srv_block.cs_wtstatistics.cs_liow_waits),
		lsbuf, sbuf));
}
STATUS
CSmo_lio_done_get(i4 offset,
		  i4 size,
		  PTR object,
		  i4 lsbuf,
		  char *sbuf)
{
    return (MOulongout( MO_VALUE_TRUNCATED,
		(u_i8)(Cs_srv_block.cs_wtstatistics.cs_lior_done +
		        Cs_srv_block.cs_wtstatistics.cs_liow_done),
		lsbuf, sbuf));
}

/*{
** Name:	CS_scb_bio_get 	-- MO get methods for sum of BIO
**				     read/writes.
**
** Description:
**	Returns the sum of BIO read/writes.
**
** Inputs:
**	offset		ignored.
**	objsize		ignored.
**	object		the CS_SCB in question.
**	luserbuf	length of output buffer
**
** Outputs:
**	userbuf		Sum of reads+writes.
**	
**
** Returns:
**	OK
**	MO_VALUE_TRUNCATED
**
** History:
**	20-Sep-2000 (jenjo02)
**	    created.
*/
STATUS
CS_scb_bio_get(i4 offset,
	       i4 objsize,
	       PTR object,
	       i4 luserbuf,
	       char *userbuf)
{
    CS_SCB *scb = (CS_SCB *)object;

    return (MOulongout( MO_VALUE_TRUNCATED,
		(u_i8)(scb->cs_bior + scb->cs_biow),
		luserbuf, userbuf));
}

/*{
** Name:	CS_scb_dio_get 	-- MO get methods for sum of DIO
**				     read/writes.
**
** Description:
**	Returns the sum of DIO read/writes.
**
** Inputs:
**	offset		ignored.
**	objsize		ignored.
**	object		the CS_SCB in question.
**	luserbuf	length of output buffer
**
** Outputs:
**	userbuf		Sum of reads+writes.
**	
**
** Returns:
**	OK
**	MO_VALUE_TRUNCATED
**
** History:
**	20-Sep-2000 (jenjo02)
**	    created.
*/
STATUS
CS_scb_dio_get(i4 offset,
	       i4 objsize,
	       PTR object,
	       i4 luserbuf,
	       char *userbuf)
{
    CS_SCB *scb = (CS_SCB *)object;

    return (MOulongout( MO_VALUE_TRUNCATED,
		(u_i8)(scb->cs_dior + scb->cs_diow),
		luserbuf, userbuf));
}

/*{
** Name:	CS_scb_lio_get 	-- MO get methods for sum of LIO
**				     read/writes.
**
** Description:
**	Returns the sum of LIO read/writes.
**
** Inputs:
**	offset		ignored.
**	objsize		ignored.
**	object		the CS_SCB in question.
**	luserbuf	length of output buffer
**
** Outputs:
**	userbuf		Sum of reads+writes.
**	
**
** Returns:
**	OK
**	MO_VALUE_TRUNCATED
**
** History:
**	20-Sep-2000 (jenjo02)
**	    created.
*/
STATUS
CS_scb_lio_get(i4 offset,
	       i4 objsize,
	       PTR object,
	       i4 luserbuf,
	       char *userbuf)
{
    CS_SCB *scb = (CS_SCB *)object;

    return (MOulongout( MO_VALUE_TRUNCATED,
		(u_i8)(scb->cs_lior + scb->cs_liow),
		luserbuf, userbuf));
}

/*{
** Name:	CSmo_num_active_get
**		CSmo_hwm_active_get
**				    -- MO get methods for active
**				       sessions and the high water
**				       mark.
**
** Description:
**
**	Returns the number of active threads, or the
**	high water mark of active threads.
**
** Inputs:
**	offset		ignored.
**	size		ignored.
**	object		ignored.
**	lsbuf		length of output buffer.
**	sbuf		output buffer.
**
** Outputs:
**	sbuf		written with appropriate value.
**	
**
** Returns:
**	OK
**	MO_VALUE_TRUNCATED
**
** History:
**	22-May-2002 (jenjo02)
**	    Created.
*/
STATUS
CSmo_num_active_get(
    i4 offset, i4 size, PTR object, i4 lsbuf, char *sbuf )
{
    return (MOulongout( MO_VALUE_TRUNCATED,
		(u_i8)CS_num_active(),
		lsbuf, sbuf));
}
STATUS
CSmo_hwm_active_get(
    i4 offset, i4 size, PTR object, i4 lsbuf, char *sbuf )
{
    return (MOulongout( MO_VALUE_TRUNCATED,
		(u_i8)CS_hwm_active(),
		lsbuf, sbuf));
}

/*{
** Name:	CS_num_active
**		CS_hwm_active
**
**	External functions to compute and return the count of
**	user/iimonitor sessions in CS_COMPUTABLE state.
**
** Description:
**
**	Computes and returns cs_num_active as the number of
**	CS_COMPUTABLE, CS_USER_THREAD and CS_MONITOR thread 
**	types. Also updates cs_hwm_active, the high-water
**	mark.
**
**	cs_num_active, cs_hwm_active are of no intrinsic use
**	to the server, so we computed them only when requested
**	by MO or IIMONITOR.
**
** Inputs:
**	none
**
** Outputs:
**	cs_num_active   Current number of CS_COMPUTABLE user+iimonitor
**			threads.
**	cs_hwm_active	HWM of that value.
**	
**
** Returns:
**	CS_num_active : The number of active threads, as defined above.
**	CS_hwm_active : The hwm of active threads.
**
** History:
**	22-May-2002 (jenjo02)
**	    Created to provide a consistent and uniform
**	    method of determining these values for both
**	    MT and Ingres threaded servers.
*/
i4	
CS_num_active(void)
{
    CS_SCB	*scb;
    i4		num_active = 0;

    CSp_semaphore(1, &Cs_known_list_sem);

    for (scb = Cs_srv_block.cs_known_list->cs_prev;
	 scb != Cs_srv_block.cs_known_list;
	 scb = scb->cs_prev)
    {
	if ( scb->cs_state == CS_COMPUTABLE &&
	    (scb->cs_thread_type == CS_USER_THREAD ||
	     scb->cs_thread_type == CS_MONITOR) )
	{
	    num_active++;
	}
    }

    if ( (Cs_srv_block.cs_num_active = num_active)
	    > Cs_srv_block.cs_hwm_active )
    {
	Cs_srv_block.cs_hwm_active = num_active;
    }

    CSv_semaphore(&Cs_known_list_sem);

    return(num_active);
}

i4	
CS_hwm_active(void)
{
    /* First, recompute active threads, hwm */
    CS_num_active();
    return(Cs_srv_block.cs_hwm_active);
}

