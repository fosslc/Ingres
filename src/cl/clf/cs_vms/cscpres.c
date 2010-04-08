/*
** Copyright (c) 2009, 2008 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <er.h>
#include    <ex.h>
#include    <me.h>
#include    <nm.h>
#include    <pc.h>
#include    <st.h>
#include    <tr.h>
#include    <iosbdef.h>
#include    <descrip.h>
#include    <exhdef.h>
#include    <iodef.h>
#include    <efndef.h>
#include    <psldef.h>
#include    <stsdef.h>
#include    <ssdef.h>
#include    <ints.h>
#include    <chfdef.h>
#include    <cmbdef.h>
#include    <agndef.h>
#include    <libicb.h>
#include    <lib$routines.h>
#include    <starlet.h>
#include    <vmstypes.h>

#include    <csinternal.h>
#include    <astjacket.h>

/*
** Name: CSCPRES.C	- Cross-process resume support
**
** Description:
**	This file contains the routines which implement cross-process resume.
**
**	Cross-process resume is used by the Logging and Locking components of
**	DMF.
**
**	    CScp_resume	    - Resume (a thread in) a process
**
**	VMS Implementation:
**
**	    Each process which calls CSinitiate() establishes a mailbox named
**	    II_CPRES_xx_pid, and queues a READ on that mailbox. It then
**	    immediately deletes the mailbox, so that it'll disappear when the
**	    process dies.
**
**	    If another process wishes to resume (a thread in) this process, that
**	    other process prepares a "wakeup message" containing the session ID
**	    of the thread to awaken, assigns a channel to this process's
**	    mailbox if it hasn't yet done so, and sends the wakeup message to
**	    this process.
**
**	    When the read completes, the completion routine (running in a user
**	    mode AST) calls CSresume on behalf of the indicated thread and
**	    queues the next READ on the mailbox.
**
**	    Internal-to-the-CL subroutines:
**		CS_cp_mbx_create	- mailbox creation & initialization
**		cpres_mbx_assign	- get channel to other process's mbox,
**					  assigning the channel if needed.
**		cpres_mbx_read_complete	- QIO read completion handler.
**		cpres_mbx_write_compete	- QIO write completion handler.
**		cpres_q_read_io		- queue a read IO.
**
**	TO DO: when a sibling process dies, we must de-assign its mailbox and
**		recover the space in our remembered channels array.
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	9-oct-1992 (bryanp)
**	    Using an IOSB on the stack is not safe for an async qio call,
**	    because the function may return before the i/o completes,
**	    and later when the I/O completes it will write into that
**	    iosb. So use a global variable for the IOSB.
**	19-oct-1992 (bryanp)
**	    CSresume expects to be called at AST level. Oblige it by invoking it
**	    via sys$dclast().
**	20-oct-1992 (bryanp)
**	    Back out the DCLAST change; CSresume can now be called at normal
**	    level.
**	14-dec-1992 (bryanp)
**	    ERsend calls should be ERlog calls. If caller gone, log it but don't
**	    die; this is not a fatal error.
**      16-jul-93 (ed)
**	    added gl.h
**	29-sep-1993 (walt)
**	    In Cscp_resume get an event flag number from lib$get_ef rather than
**	    use event flag zero in the sys$qio call.  
**	18-oct-1993 (rachael)
**	    Call lib$signal(SS$_DEBUG) only when compiled with xDEBUG flag.
**	    Modified TRdisplay statement to reflect calling function.
**	23-may-1994 (bryanp) B60736
**	    Added clean_channels() routine and called it periodically to
**		ensure that a process doesn't accumulate channels to dead
**		processes indefinitely.
**	09-mar-1998 (kinte01)
**	    Cross-integrate change 433580 from VAX CL into AXP VMS CL
**	16-Nov-1998 (jenjo02)
**	    CScp_resume() prototype changed to pass CS_CPID * instead of PID, SID.
**	30-dec-1999 (kinte01)
**	   Add missing includes for ANSI C compiler
**	01-dec-2000	(kinte01)
**		Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**		from VMS CL as the use is no longer allowed
**	28-may-2003 (horda03) Bug 109272
**	    When the RCP actions a Cross-Process resume an ACCVIO can
**	    occur.
**	28-aug-2003 (abbjo03)
**	    Changes to use VMS headers provided by HP.
**    18-aug-2004 (horda03) Bug 112983/INGSRV3009
**        Create the mailbox with sufficient space to handle simultaneous resumes
**        for a minimum of 5 sessions (or 1 tenth of the number of sessions).
**        Thus preventing other processes entering RWMBX state.
**	02-may-2007 (jonj/joea)
**	    Use CS_TAS/CS_ACLR instead of CSp/v_semaphore for the
**	    cpres_channels_sem.
**	04-Apr-2008 (jonj)
**	    Add an AST for the write QIO so that the IOSB status can be
**	    checked; NOREADER is delivered to the IOSB, not qio status.
**	    Rearranged handling of NOREADER.
**	11-nov-2008 (joea)
**	    Use CL_CLEAR_ERR to initialize CL_ERR_DESC.
**      22-dec-2008 (strge01)
**         Itanium support 
**         Get rid of event flags - even the async qio calls release 
**         the efn before the i/o is known to have completed
**         store ICB on stack for Itanium 
*/

GLOBALREF CS_SYSTEM           Cs_srv_block;

typedef struct
{
    CS_SID	wakeup_sid;

    /* horda03 - Added to track Cross Process problem */
    PID         wakeup_pid;
    CS_SID      from_sid;
    PID         from_pid;
    i4          sender_in_ast;
} CS_CP_WAKEUP_MSG;


typedef	struct 
{
    PID		pid; 
    i4		state;
#define			CPchanIsAlive	0
#define			CPchanIsDying	1
#define			CPchanIsDead	2
    II_VMS_CHANNEL chan;
} CP_CHANNEL;


static	II_VMS_CHANNEL	    cpres_mbx_chan;
static	IOSB		    cpres_iosb;
static	CS_CP_WAKEUP_MSG    cpres_msg_buffer;
static	CS_ASET		    cpres_channels_sem;
static	i4		    cpres_num_channels_assigned;

static	CP_CHANNEL	    cpres_channels[1000];

static	void	cpres_q_read_io(void);
static	void	cpres_mbx_write_complete( CS_CPID *cpid );
static	void	cpres_mbx_read_complete( PTR astprm );
static	STATUS	clean_channels(void);
static	STATUS	cpres_mbx_assign(PID pid,  CP_CHANNEL **CPchan);

/*
** Name: CS_cp_mbx_create	- mailbox creation and initialization
**
** Description:
**	This subroutine is called from CSinitiate().
**
**	It does the following:
**	    a) establishes a mailbox, with the name II_CPRES_xx_pid, where
**		xx is the (optional) installation code, and pid is the
**		process ID in hex.
**	    b) queues a read on the mailbox, with completion routine set to
**		CS_cp_mbx_complete
**	    c) deletes the mailbox, so it'll go away when the process dies.
**
** Inputs:
**	num_sessions    - Number of sessions for the process.
**
** Outputs:
**	sys_err		- reason for error
**
** Returns:
**	OK, !OK
**
** Side Effects:
**	Sets cpres_mbx_chan to the mailbox's channel
**	Defines the system-wide logical name II_CPRES_xx_pid
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	08-Nov-2007 (jonj)
**	    Use of "num_sessions" is totally bogus. CS_cp_mbx_create() is called
**	    before the startup parms are determined from config.dat (where we'd
**	    find "connect_limit"), so SCD hard-codes num_sessions = 32, resulting
**	    in CS_CP_MIN_MSGS == 5 always being used, which is way too small.
**	    Instead, default to the (configurable) VMS sysgen parameter
**	    DEFMBXBUFQUO.
**	    Also, create mailbox as read-only. Writers will assign write-only
**	    channels.
*/
STATUS
CS_cp_mbx_create(i4 num_sessions, CL_ERR_DESC	*sys_err)
{
    struct	dsc$descriptor_s    name_desc;
    i4		vms_status;
    char	mbx_name[100];
    char	*inst_id;
    PID		pid;

    CL_CLEAR_ERR(sys_err);
    /*
    ** Build the mailbox logical name:
    */
    PCpid(&pid);
    NMgtAt("II_INSTALLATION", &inst_id);
    if (inst_id && *inst_id)
	STprintf(mbx_name, "II_CPRES_%s_%x", inst_id, (i4)pid);
    else
	STprintf(mbx_name, "II_CPRES_%x", (i4)pid);

    name_desc.dsc$a_pointer = mbx_name;
    name_desc.dsc$w_length = STlength(mbx_name);
    name_desc.dsc$b_dtype = DSC$K_DTYPE_T;
    name_desc.dsc$b_class = DSC$K_CLASS_S;

    vms_status = sys$crembx(
		1,		    /* Mailbox is "permanent" */
		&cpres_mbx_chan,    /* where to put channel */
		(i4)sizeof(CS_CP_WAKEUP_MSG),
				    /* maximum message size */
		0,		    /* buffer quota (DEFMBXBUFQUO) */
		0,		    /* prot mask = all priv */
		PSL$C_USER,	    /* acmode */
		&name_desc,	    /* logical name descriptor */
		CMB$M_READONLY,	    /* flags */
		0);		    /* nullarg */

    if ( vms_status != SS$_NORMAL )
    {
	sys_err->error = vms_status;
	if (vms_status == SS$_NOPRIV)
	    return (E_CS00F8_CSMBXCRE_NOPRIV);
	else
	    return (E_CS00F7_CSMBXCRE_ERROR);
    }

    /* Hang a read */
    cpres_q_read_io();

    /* Mark for deletion, so it disappears when we exit. */

    sys$delmbx(cpres_mbx_chan);

    cpres_channels_sem = 0;

    cpres_num_channels_assigned = 0;

    return (OK);
}

/*
** Name: CScp_resume	- Resume (a thread in) a process
**
** Description:
**	This routine resumes the indicated thread in the indicated process.
**
**	If the indicated process is this process, then this is a simple
**	CSresume operation. If the indicated process is another process, then
**	that process must be notified that it should CSresume the indicated
**	thread.
**
** Inputs:
**	cpid		pointer to CS_CPID with
**	   .pid		- the indicated process
**	   .sid		- the indicated session
**	   .iosb	- a thread-safe IOSB
**	   .data	- where we'll place a pointer
**			  to the CPchan written to.
**
** Outputs:
**	None
**
** Returns:
**	void
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	9-oct-1992 (bryanp)
**	    Use global IOSB, not stack IOSB, so that when QIO completes
**	    some time from now it will not overwrite arbitrary stack stuff.
**	19-oct-1992 (bryanp)
**	    CSresume expects to be called at AST level. Oblige it by invoking it
**	    via sys$dclast().
**	20-oct-1992 (bryanp)
**	    Back out the DCLAST change; CSresume can now be called at normal
**	    level.
**	14-dec-1992 (bryanp)
**	    ERsend() calls should be ERlog() calls.
**	29-sep-1993 (walt)
**	    Get an event flag number from lib$get_ef rather than use event flag
**	    zero in the sys$qio call.  
**	18-oct-1993 (rachael)
**	    Call lib$signal(SS$_DEBUG) only when compiled with xDEBUG flag.
**	16-Nov-1998 (jenjo02)
**	    Prototype changed to pass CS_CPID * instead of PID, SID.
**	08-Nov-2007 (jonj)
**	    Write with IO$M_NOW and IO$M_READERCHECK, check for dead reader
**	    process (NOREADER), mark this PID/channel as dead for subsequent
**	    resumers to ignore. IO$M_NOW does not wait for the reader to
**	    read.
**	04-Apr-2008 (jonj)
**	    Embed IOSB in CS_CPID and reinstate lib$get_ef() to assure
**	    thread-safeness.
**	    Disable/reenable ASTs to prevent seen duplicate reads on the
**	    other end.
**	    Supply cpres_mbx_write_complete() AST to check IOSB status
**	    for NOREADER.
*/
void
CScp_resume( CS_CPID *cpid )
{
    i4		    	vms_status;
    CS_CP_WAKEUP_MSG	wakeup_msg;
    i4		    	mbox_chan;
    char		msg_buf[100];
    CL_ERR_DESC		local_sys_err;
    CP_CHANNEL		*CPchan;
    II_VMS_EF_NUMBER	efn;
    i4			ReenableASTs;

    if (cpid->pid == Cs_srv_block.cs_pid)
    {
	CSresume(cpid->sid);
    }
    else
    {
	/* Disable AST delivery */
	ReenableASTs = (sys$setast(0) == SS$_WASSET);

	/* Initialize to success */
	vms_status = SS$_NORMAL;

	if ( cpres_mbx_assign(cpid->pid, &CPchan) == OK )
	{
	    /* If reader is not alive, do nothing */
	    if ( CPchan->state == CPchanIsAlive )
	    {
		/* The SID of the session to resume */
		wakeup_msg.wakeup_sid = cpid->sid;

		/* horda03 - Fill in details to help track Cross-Process
		**           ACCVIO problem.
		*/
		wakeup_msg.wakeup_pid    = cpid->pid;
		wakeup_msg.from_pid      = Cs_srv_block.cs_pid;

		/* If from AST, "from_sid" is meaningless */
		if ( (wakeup_msg.sender_in_ast = lib$ast_in_prog()) )
		    wakeup_msg.from_sid = 0;
		else
		    wakeup_msg.from_sid = (CS_SID)Cs_srv_block.cs_current;

		/*
		** Plunk message, don't wait for reader to read it.
		**
		** Use IOSB embedded in CS_CPID, pass CS_CPID* to
		** AST completion.
		*/

		/* Set CPchan in the CS_CPID for AST's use */
		cpid->data = (PTR)CPchan;

		vms_status = sys$qio(EFN$C_ENF, CPchan->chan, 
				    IO$_WRITEVBLK | IO$M_NOW | IO$M_READERCHECK,
				    &cpid->iosb,
				    cpres_mbx_write_complete, 
				    cpid, 
				    &wakeup_msg, sizeof(wakeup_msg),
				    0, 0, 0, 0);

		if ( vms_status != SS$_NORMAL )
		{
		    STprintf(msg_buf, "[%x.%x] Error (%x) queueing write to %x on channel %d",
			    wakeup_msg.from_pid,
			    wakeup_msg.from_sid,
			    vms_status, CPchan->pid, CPchan->chan);
		    ERlog(msg_buf, STlength(msg_buf), &local_sys_err);
		}
	    }
	}
	else
	{
	    STprintf(msg_buf, "Unable to assign channel to %x", cpid->pid);
	    ERlog(msg_buf, STlength(msg_buf), &local_sys_err);

	    STprintf(msg_buf, "Ignoring error in assigning mailbox for PID %x", 
			cpid->pid);
	    ERlog(msg_buf, STlength(msg_buf), &local_sys_err);
	    /*
	    ** The process we were going to send this message to will probably
	    ** "hang", which at least allows some sort of diagnosis. Killing
	    ** ourselves at this point is less useful, since it tends to crash
	    ** the entire installation.
	    */
	}
	
	if ( vms_status != SS$_NORMAL )
	{
	    STprintf(msg_buf, "CScp_resume QIO to %x failed with status %x",
		    cpid->pid, vms_status);
	    ERlog(msg_buf, STlength(msg_buf), &local_sys_err);
#ifdef xDEBUG
	    lib$signal(SS$_DEBUG);
#endif
	    PCexit(FAIL);
	}

	if ( ReenableASTs )
	    sys$setast(1);
    }
    return;
}

/*
** Name: cpres_mbx_write_complete	- QIO write completion handler.
**
** Description:
**	This subroutine is called as a user-mode AST routine when the write I/O
**	on the cross-process resume mailbox completes.
**
**	It checks the IOSB for NOREADER and if true
**	sets the channel status to CPchanDying.
**
** Inputs:
**	cpid			- *CS_CPID of writer containing
**		iosb			IOSB used for QIO
**		data			Pointer to CPchan
**
** Outputs:
**	None
**
** Returns:
**	void
**
** Side Effects:
**	Channel may be taken out of service.
**
** History:
**	04-Apr-2008 (jonj)
**	    Created.
*/
static void
cpres_mbx_write_complete( CS_CPID *cpid )
{
    CP_CHANNEL	*CPchan;

    /* If reader has died, shutdown the channel */
    if ( cpid->iosb.iosb$w_status == SS$_NOREADER )
    {
	CPchan = (CP_CHANNEL*)cpid->data;

	/* Loop until we get the sem */
	while ( !CS_TAS(&cpres_channels_sem) );

	/* If not yet noticed, mark channel as dying */
	if ( CPchan->state == CPchanIsAlive )
	    CPchan->state = CPchanIsDying;

	CS_ACLR(&cpres_channels_sem);
    }
}

/*
** Name: cpres_q_read_io	- queue a read I/O on our CP mailbox
**
** Description:
**	This subroutine queues an I/O on this process's CPRES mailbox.
**
** Inputs:
**	None
**
** Outputs:
**	None
**
** Returns:
**	void
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	18-oct-1993 (rachael)
**	    Call lib$signal(SS$_DEBUG) only when compiled with xDEBUG flag.
**	    Modified TRdisplay statement to reflect calling function.
**	04-Apr-2008 (jonj)
**	    Use EFN to assure thread-safeness rather than defaulting
**	    to EFN zero.
*/
static void
cpres_q_read_io(void)
{
    i4			vms_status;

    vms_status = sys$qio(EFN$C_ENF, cpres_mbx_chan, IO$_READVBLK, &cpres_iosb,
	    cpres_mbx_read_complete, (__int64)&cpres_iosb, &cpres_msg_buffer,
	    sizeof(CS_CP_WAKEUP_MSG), 0, 0, 0, 0);

    if ( vms_status != SS$_NORMAL )
    {
	TRdisplay("cpres_q_read_io QIO failed, status %x\n", vms_status);
#ifdef xDEBUG
	lib$signal(SS$_DEBUG);
#endif
	PCexit(FAIL);
    }
    return;
}

/*
** Name: cpres_mbx_assign	- get channel to target process, assign if need
**
** Description:
**	This subroutine looks up the channel to the target process. If we have
**	not yet assigned a channel to the target process, then we assign one
**	and remember it. The resulting channel is returned.
**
** Inputs:
**	pid			- the target process's pid
**
** Outputs:
**	CPchan			- Pointer to CP_CHANNEL of process.
**
** Returns:
**	OK, !OK
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	14-dec-1992 (bryanp)
**	    ERsend() calls should be ERlog() calls.
**	23-may-1994 (bryanp) B60736
**	    Added clean_channels() routine and called it periodically to
**		ensure that a process doesn't accumulate channels to dead
**		processes indefinitely.
**	02-May-2007 (jonj)
**	    Use of CSp/v_semaphore is prohibited from within an AST as
**	    it corrupts cs_inkernel and the ready queues. Use CS_ASET/ACLR
**	    instead.
**	08-Nov-2007 (jonj)
**	    Changed function to return pointer to CP_CHANNEL instead
**	    of channel number, initialize IsDead to FALSE.
**	04-Apr-2008 (jonj)
**	    Remove disabling of ASTs here, caller will have already done
**	    that.
**	    Check if PID has been deadified by write AST; if so,
**	    cancel any pending I/O, deassign channel, mark CPchan
**	    as unusuable (dead).
*/
static STATUS
cpres_mbx_assign(PID pid, CP_CHANNEL **CPchan)
{
    i4	    	i;
    struct	dsc$descriptor_s    name_desc;
    i4		vms_status;
    char	mbx_name[100];
    char	*inst_id;
    char	msg_buf[250];
    STATUS	status;
    STATUS	cl_status;
    PID		my_pid;
    CL_ERR_DESC	local_sys_err;
    CP_CHANNEL	*NewCPchan;

    /* Loop until we get the sem */
    while ( !CS_TAS(&cpres_channels_sem) );

    for (i = 0; i < cpres_num_channels_assigned; i++)
    {
	if (cpres_channels[i].pid == pid)
	{
	    /*
	    ** If write completion AST noticed that
	    ** the reading process has died or gone away,
	    ** mark its PID as dead, cancel any leftover
	    ** I/O and deassign the channel.
	    */
	    if ( cpres_channels[i].state == CPchanIsDying )
	    {
		sys$cancel(cpres_channels[i].chan);
		sys$dassgn(cpres_channels[i].chan);

		cpres_channels[i].state = CPchanIsDead;

		STprintf(msg_buf, "%x PID %x on channel %d is dead, deassigned",
				Cs_srv_block.cs_pid,
				cpres_channels[i].pid, 
				cpres_channels[i].chan);
		ERlog(msg_buf, STlength(msg_buf), &local_sys_err);
	    }

	    /* Return channel, dead or alive */
	    *CPchan = &cpres_channels[i];
	    CS_ACLR(&cpres_channels_sem);
	    return (OK);
	}
    }

    if ( status = clean_channels() )
    {
	CS_ACLR(&cpres_channels_sem);
	return (status);
    }

    if ( cpres_num_channels_assigned <
		    (sizeof(cpres_channels)/sizeof(CP_CHANNEL)) )
    {
	/*
	** New process, and room remains in the channel array, so assign a
	** channel.
	*/
	NewCPchan = &cpres_channels[cpres_num_channels_assigned];

	NMgtAt("II_INSTALLATION", &inst_id);
	if (inst_id && *inst_id)
	    STprintf(mbx_name, "II_CPRES_%s_%x", inst_id, (i4)pid);
	else
	    STprintf(mbx_name, "II_CPRES_%x", (i4)pid);

	name_desc.dsc$a_pointer = mbx_name;
	name_desc.dsc$w_length = STlength(mbx_name);
	name_desc.dsc$b_dtype = DSC$K_DTYPE_T;
	name_desc.dsc$b_class = DSC$K_CLASS_S;

	vms_status = sys$assign(&name_desc,	/* devname */
			    &NewCPchan->chan,	/* channel */
			    0, 			/* access_mode */
			    0,			/* mbxnam */
			    AGN$M_WRITEONLY);	/* flags */

	if ( vms_status == SS$_NORMAL )
	{
	    NewCPchan->pid = pid;
	    NewCPchan->state = CPchanIsAlive;
	    *CPchan = NewCPchan;
	    cpres_num_channels_assigned++;
	    CS_ACLR(&cpres_channels_sem);
	    return (OK);
	}
	else
	{
	    STprintf(msg_buf,
		    "%x cpres_mbx_assign: Error (%x) assigning channel to %s",
		    Cs_srv_block.cs_pid, vms_status, mbx_name);
	    ERlog(msg_buf, STlength(msg_buf), &local_sys_err);
	}
    }
    else
    {
	/*
	** No room left in channels array. One possibility is to go through
	** the array looking for assigned channels to mailboxes of dead
	** processes and clean those up.
	*/
	STcopy("PANIC! No room left in channels array!", msg_buf);
	ERlog(msg_buf, STlength(msg_buf), &local_sys_err);
    }

    CS_ACLR(&cpres_channels_sem);
    return (FAIL);
}

/*
** Name: cpres_catch      - Catch any exceptions.
**
** Description:
**       This routine mirrors the action of EXcatch(). Because
**       cpres_mbx_complete() functions in an AST it is not
**       possible to use the normal EX routines to trap exceptions.
**       In an AST, there may not be a "cs_current" session, and
**       the so the EXdeclare() call will ACCVIO, causing the
**       server to enter an infinite loop.
**
**       This routine is envoked when an ACCIO occurs trying to resume
**       a session, it will wind back the thread to the point where
**       the exception handler was declared.
**
**       Because the Cross Process resume is done via an AST, and is
**       thus synchronous, the AST has exclusive access to the static
**       variable cpres_context.
**
**       For more information on the steps being performed to return
**       to the CSCPdeclare() call, see EXcatch().
**
** Inputs:
**       chf$signal_array
**       chf$mech_array
**
** Outputs:
**       EXDECLARE
**
** Side effects:                          
**       Control returned to point where exception occured.
**
** History:
**       20-aug-2003 (horda03)
**            Created.
*/
static i4
cpres_catch( struct chf$signal_array *sigs, struct chf$mech_array *mechs)
{
   EX_CONTEXT cpres_context;
   int64  establisher_fp      = mechs->chf$q_mch_frame & 0xffffffff;
   INVO_CONTEXT_BLK  *jmpbuf  = &cpres_context.iijmpbuf;
   uint64 invo_value          = EX_DECLARE;
   i4     sts;

#if defined(ALPHA)
   uint64 invo_mask           = 0x0FFFCul | ((uint64) 0X03FCul << (BITS_IN(uint64) / 2));
   int    establishers_handle = lib$get_invo_handle( jmpbuf );
   
   lib$put_invo_registers( establishers_handle, jmpbuf, &invo_mask );

   /* Return to CSCPdeclare() call, but EX_DECLARE returned */

   sys$goto_unwind(&establishers_handle, jmpbuf+2, &invo_value, 0);

#elif defined(IA64)
    uint64  establishers_handle;
    i4      gr_mask[4];
    i4      fr_mask[4];
    i4      msk;
    i4      k;

    sts = lib$i64_get_invo_handle(jmpbuf, &establishers_handle);
    if (!(sts & STS$M_SUCCESS)) /* returns 0 == fail, 1==success */
    {
        /* lib$signal (SS$_DEBUG); */
        /* if we carry on after this we'll just get an accvio ... so resignal */
        return (SS$_RESIGNAL);
    }

    for (k=0; k<4; k++) gr_mask[k] = jmpbuf->libicb$o_gr_valid[k];
    fr_mask[0] = jmpbuf->libicb$l_fr_valid;
    msk = (jmpbuf->libicb$ph_f32_f127) ? 0xFFFFFFFF : 0;
    for (k=1; k<4; k++) fr_mask[k] = msk;

    sts = lib$i64_put_invo_registers( establishers_handle, jmpbuf, gr_mask, fr_mask );

    /* Return to CSCPdeclare() call, but EX_DECLARE returned */

    /* (target_invo, target_pc, new_retval1, new_retval2) */

    sys$goto_unwind_64(&establishers_handle, &jmpbuf->libicb$ih_pc, &invo_value, 0);
#else
#error "cpres_catch:: missing code for this platform"
#endif
   
   /* Should never reach here */

   TRdisplay( "%@ cscpres_catch:: SYS$GOTO_UNWIND failed\n\n");

   PCexit( FAIL );

   return SS$_RESIGNAL;
}

/*
** Name: cpres_mbx_read_complete	- QIO read completion handler.
**
** Description:
**	This subroutine is called as a user-mode AST routine when the read I/O
**	on the cross-process resume mailbox completes.
**
**	It calls CSresume on behalf of the indicated session ID and schedules
**	the next read I/O on the mailbox.
**
** Inputs:
**	astprm			- ast parameter
**
** Outputs:
**	None
**
** Returns:
**	void
**
** Side Effects:
**	A session is resumed
**	Another I/O is queued.
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**      28-may-2003 (horda03) Bug 109272
**          Set up an exception handler, to catch any ACCVIO's caused
**          by invalid SIDs.
*/
static void
cpres_mbx_read_complete( PTR astprm )
{
    CS_CPID	cpid;

    if (cpres_msg_buffer.wakeup_pid != Cs_srv_block.cs_pid)
    {
       TRdisplay( "%@ cpres::Wakeup received for wrong process. W_PID = %08x W_SID = %08x, S_PID = %08x, S_SID = %08x, S_AST = %d\n",
                  cpres_msg_buffer.wakeup_pid, cpres_msg_buffer.wakeup_sid,
                  cpres_msg_buffer.from_pid, cpres_msg_buffer.from_sid,
                  cpres_msg_buffer.sender_in_ast );

       TRdisplay( "cpres::Attempting to resume correct session.\n");

	cpid.pid = cpres_msg_buffer.wakeup_pid;
	cpid.sid = cpres_msg_buffer.wakeup_sid;
	CScp_resume(&cpid);
    }
    else
    {
       i4 sts = 0; /* In the followin context 0 == success */
       EX_CONTEXT cpres_context;

       /* Bug 109272; catch any exceptions.
       **   CSCPdeclare() is similar to EXdeclare(), but as we are in an AST
       **   there may not be a current session (Cs_srv_block.cs_current == 0)
       **   which would lead to an infinite ACCVIO loop. So instead use a
       **   static EX_CONTEXT block (cpres_context) to store the return address
       **   and let the EXCEPTION call the local function cpres_catch, which
       **   will return to the "if" statement below with EX_DECLARE if an
       **   exception occurs.
       */
#if defined(ALPHA)

#define CSCPdeclare( y ) ( VAXC$ESTABLISH( cpres_catch ), \
                           EXgetctx( (y)->iijmpbuf))

       if (CSCPdeclare( &cpres_context ) ) sts = 1;

#elif defined(IA64)

       /* init returns 0 as a failure, get_curr always returns 0 as success */

       sts = lib$i64_init_invo_context(&cpres_context.iijmpbuf, LIBICB$K_INVO_CONTEXT_VERSION, 0);
       sts = (sts) ? lib$i64_get_curr_invo_context(&cpres_context.iijmpbuf) : 1;
#endif
       if (sts)
       {
          TRdisplay( "%@ cpres::Exception occured for W_PID = %08x W_SID = %08x, S_PID = %08x, S_SID = %08x, S_AST = %d\n",
                     cpres_msg_buffer.wakeup_pid, cpres_msg_buffer.wakeup_sid,
                     cpres_msg_buffer.from_pid, cpres_msg_buffer.from_sid,
                     cpres_msg_buffer.sender_in_ast );
       }
       else
           CSresume(cpres_msg_buffer.wakeup_sid);

#define CSCPdelete() (VAXC$ESTABLISH( NULL ))

       CSCPdelete();
    }

    /* Hang another read */
    cpres_q_read_io();
}

/*
** Name: clean_channels		- garbage collect channels to dead processes
**
** Description:
**	Called whenever we're about to assign a channel to a new process.
**	Before assigning the new channel, we compact the existing channels
**	array by reclaiming any channels to old, dead processes.
**
** Inputs:
**	None
**
** Outputs:
**	None
**
** Returns:
**	STATUS
**
** History:
**	23-may-1994 (bryanp)
**	    Created as part of resolving B60736.
**	08-Nov-2007 (jonj)
**	    Check that PID is already known to be dead.
*/
static STATUS
clean_channels(void)
{
    i4		i;
    i4		j;
    i4		s;
    char	msg_buf[250];
    CL_ERR_DESC	local_sys_err;
    CP_CHANNEL	*CPchan;

    for (i = 0; i < cpres_num_channels_assigned; i++)
    {
	CPchan = &cpres_channels[i];

	if ( CPchan->state == CPchanIsDead || !(PCis_alive(CPchan->pid)) )
	{
	    TRdisplay("%@ cscpres: garbage collect channel to %x\n",
			CPchan->pid);
	    /*
	    ** What cleanup is required:
	    **	    1) cancel any outstanding I/O on this channel
	    **	    2) deassign the channel
	    **	    3) shift all the other channels down one in the array
	    **		(ugh, but we don't do this very often)
	    **	    4) back 'i' off by one so that we don't skip any
	    **		channels in the loop
	    **	    5) back cpres_num_channels_assigned off by one to
	    **		reflect the cleaned-up channel.
	    */

	    /*
	    ** If already known to be dead, we've cancelled/deassigned
	    ** already.
	    */
	    if ( CPchan->state != CPchanIsDead )
	    {
		CPchan->state = CPchanIsDead;

		s = sys$cancel(CPchan->chan);
		if ( s != SS$_NORMAL )
		{
		    STprintf(msg_buf, "Error %x cancelling channel %x",
				s, CPchan->chan);
		    ERlog(msg_buf, STlength(msg_buf), &local_sys_err);
		    TRdisplay("%s\n", msg_buf);
		    return (FAIL);
		}
		s = sys$dassgn(CPchan->chan);
		if ( s != SS$_NORMAL )
		{
		    STprintf(msg_buf, "Error %x releasing channel %x",
				s, CPchan->chan);
		    ERlog(msg_buf, STlength(msg_buf), &local_sys_err);
		    TRdisplay("%s\n", msg_buf);
		    return (FAIL);
		}
	    }

	    for (j = i + 1; j < cpres_num_channels_assigned; j++)
	    {
		cpres_channels[j - 1].pid  = cpres_channels[j].pid;
		cpres_channels[j - 1].state  = cpres_channels[j].state;
		cpres_channels[j - 1].chan = cpres_channels[j].chan;
	    }

	    i--;
	    cpres_num_channels_assigned--;
	}
    }
    return (OK);
}

/*
** Name: CS_cpres_event_handler         - handle any cross process resumes
**
** Description:
**      This routine is called by the CS event system to handle any
**      cross-process resumes which are sent to this process from other
**      processes. It in turn calls back to the event system to do most of
**      the work.
**
** Inputs:
**      none
**
** Outputs:
**      none
**
** Returns:
**      OK
**
** History:
**      3-jul-1992 (bryanp)
**          created.
*/
STATUS
CS_cpres_event_handler(void)
{
#ifdef OS_THREADS_USED
    CS_handle_wakeup_events(Cs_srv_block.cs_pid);
#endif

    return (OK);
}
