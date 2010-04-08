/*
** Copyright (c) 1988, 2008 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <ssdef.h>
#include    <er.h>
#include    <pc.h>
#include    <ck.h>
#include    <lo.h>
#include    <starlet.h>
/**
**
**  Name: CKSPAWN.C - Run a CK command of the host operating system
**
**  Description:
**	Run a CK command on the host operating system
**
**	    CK_spawn()    - spawn a CK command and wait
**	    CK_spawnnow() - spawn a CK command and do not wait
**	    CK_is_alive() - is a CK command alive?
**
**
**  History:
**      27-oct-1988 (anton)
**	    Created.
**	03-may-1989 (fredv)
** 	    For some reasons, IBM RT's system() call always gets an interrupt
**	 	signal and returns fail. Use our PCcmdline() works a lot 
**		better. So added a special case for IBM RT.
**	08-may-89 (daveb)
**		If system() doesn't work somewhere, but PCcmdline does,
**		just use PCcmdline everywhere.  Removed the ifdef for
**		rtp_us5 and made if the default.
**
**		It would have been better, in my opinion, to have
**		put this relatively general interface into PC rather
**		than burying it here in CK.  (You would want this
**		style call, because it has a CL_ERR_DESC, unlike
**		PCcmdline.)
**	25-sep-1990 (walt)
**	    (To fix bug 33419)
**	    Removed SYSPRV-setting code from CKrestore, CKsave [ck.c] and moved
**	    it here.  This is being done because CKbegin and CKend need to have
**	    the same functionality.  Rather than replicate the code two more
**	    times there in that file, I'm just moving it to this common place
**	    for them all to use.
**	15-oct-1990 (walt)
**	    Use PCcmdline's additional parameters.
**	18-dec-1990 (walt)
**	    On 25-oct-90 Sandyd integrated a previous version of this file that
**	    updated the old version to use the new PCcmdline parameters that
**	    are in this version.  The version being integrated today (18-dec-90)
**	    is the original file from which that change was taken.  It
**	    also contains many other changed lines.  Rather than merge this
**	    file's many changes into that one - just to preserve the one line
**	    previously integrated - this file is being integrated as a whole
**	    and includes the previously new version of the PCcmdline call.
**      16-jul-93 (ed)
**	    added gl.h
**	14-nov-96 (chech02)
**	    added CK_spawnnow() from UNIX for 2.0 port.
**	25-sep-98 (chash01/kinte01)
**          change CK_spawn() to PCspawn() so that
**          we can have child process running concurrently
**          with parent process. (93552)
**      27-Jun-2000 (horda03)
**          Enable SYSPRV (VMS) privilege before invoking a spawn
**          command. This privilege must be enables in EXEC or KRNL
**          mode. I've plumped for EXEC mode.
**	19-jul-2000 (kinte01)
**	    Correct prototype definitions by adding missing includes
**	01-dec-2000	(kinte01)
**		Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**		from VMS CL as the use is no longer allowed
**	13-mar-2001 (kinte01)
**		Add new routine ck_is_alive
**		Rearranged headers (pc.h before ck.h) to pick up 
**		definition of PID
**/

/*{
** Name: CK_spawn - spawn a CK command and wait
**
** Description:
**	Run the suppled string as a command as ingres which hopefully
**	does a CK operation.
**
** Inputs:
**	command			command to run
**
** Outputs:
**	err_code		machine dependent error return
**
**	Returns:
**	    E_DB_OK
**	    CK_SPAWN		Command would not run or exited failure
**
**	Exceptions:
**	    if enabled
**	    EXCHLDDIED		when command finishs (SIGCHLD)
**
** Side Effects:
**	    none
**
** History:
**	27-oct-1988 (anton)
**	    Created.
**	03-may-1989 (fredv)
** 	    For some reasons, IBM RT's system() call always gets an interrupt
**	 	signal and returns fail. Use our PCcmdline() works a lot 
**		better. So added a special case for IBM RT.
**	08-may-89 (daveb)
**		If system() doesn't work somewhere, but PCcmdline does,
**		just use PCcmdline everywhere.  Removed the ifdef for
**		rtp_us5 and made if the default.
**	25-sep-1990 (walt)
**	    (To fix bug 33419)
**	    Move SYSPRV-setting code here from CKrestore, CKsave [ck.c].
**	15-oct-1990 (walt)
**	    Use PCcmdline's additional parameters.
**	26-apr-1993 (bryanp)
**	    Prototyped this  code for 6.5.
**	26-jul-1993 (bryanp)
**	    No longer require CMKRNL privilege in order to take a checkpoint.
**	    Instead, we now require only that SYSPRV be an authorized privilege.
**	    (Formerly, it was reasonable for CK to assume that the caller had
**		CMKRNL, because LG and LK both required CMKRNL. However, those
**		routines no longer require CMKRNL, so CK should no longer
**		assume it).
**      27-Jun-2000 (horda03)
**          Enable SYSPRV (VMS) privilege before invoking a spawn
**          command. This privilege must be enables in EXEC or KRNL
**          mode. I've plumped for EXEC mode.
**	11-nov-2008 (joea)
**	    Use CL_CLEAR_ERR to initialize CL_ERR_DESC.
*/

/*
**  Definitions of forward static functions.
*/

static STATUS	set_sysprv(
		    i4             sysprv[2],
		    i4             oldprv[2]);
static STATUS	clr_sysprv(
		    i4             sysprv[2],
		    i4             oldprv[2]);

/*
**	26-jul-1993 (bryanp)
**	    No longer require CMKRNL privilege in order to take a checkpoint.
**	    Instead, we now require only that SYSPRV be an authorized privilege.
**	    (Formerly, it was reasonable for CK to assume that the caller had
**		CMKRNL, because LG and LK both required CMKRNL. However, those
**		routines no longer require CMKRNL, so CK should no longer
**		assume it).
**      27-Jun-2000 (horda03)
**          Enable SYSPRV (VMS) privilege before invoking a spawn
**          command. This privilege must be enables in EXEC or KRNL
**          mode. I've plumped for EXEC mode.
*/
STATUS
CK_spawn(
char		*command,
CL_ERR_DESC	*err_code)
{
    STATUS	ret_val = OK;
    i4	status;
    i4	sysprv[2] = { 0x10000000, 0 };
    i4	oldprv[2];
	
    struct
       {
	  i4     arg_count;
	  i4     *sysprv;
	  i4     *oldprv;
       }    sysprv_args = { 2, &sysprv, &oldprv };
    
    CL_CLEAR_ERR(err_code);

    for (;;)
    {
       /*
       **  Give this process SYSPRV so that the spawned process will get
       **  SYSPRV and thus gain access to the DI files. 
       **
       **  In order to acquire this privilege, need to perform the operation
       **  in EXEC mode (or Kernel).
       */

        status = sys$cmexec(set_sysprv, &sysprv_args);

	if ((status & 1) == 0)
	    break;

       if (PCcmdline((LOCATION *) NULL, command, (i4) NULL, (LOCATION *) NULL,
		err_code))
       {
          err_code->error = CK_SPAWN;
	  ret_val = CK_SPAWN;
       }
       break;
    }

    /*  Remove SYSPRV from this process. */

    status = sys$cmexec(clr_sysprv, &sysprv_args);

    if ((status & 1) == 0)
    {
	/* Can't exit and give the user SYSPRV, so trash the process. */

	sys$delprc(0, 0);
    }

    return(ret_val);
}

/*
** Name: set_sysprv	- Add SYSPRV to this process.
**
** Description:
**      This routine gives the current process the authorized privlege of SYSPRV
**	assuming that the caller doesn't have SYSPRV currently but the caller is
**	running the enhanced privilege including CMKRNL.  This gets around a 
**	problem of running VMS BACKUP with SYSPRV so that backup may access the
**	protected files.
**
** Inputs:
**      sysprv                          Mask specifing the privilege to add.
**
** Outputs:
**      oldprv                          The privileges at the time of the request.
**	Returns:
**	    sys$setprv()
**	Exceptions:
**
**
** Side Effects:
**
**
** History:
**      02-nov-1986 (Derek)
**          Revised.
**	25-sep-1990 (Walt)
**	    (To fix bug 33419)
**	    Moved here from ck.c
*/
static STATUS
set_sysprv(
i4             sysprv[2],
i4             oldprv[2])
{
    return (sys$setprv(1, sysprv, 1, oldprv));
}

/*
** Name: clr_sysprv	- Remove SYSPRV from this process.
**
** Description:
**	Remove the authorized SYSPRV that was added by set_sysprv.
**
** Inputs:
**      sysprv                          Mask specifing the privilege to add.
**      oldprv                          The privileges before the set request.
**
** Outputs:
**	Returns:
**	    sys$setprv()
**	Exceptions:
**
**
** Side Effects:
**
**
** History:
**      02-nov-1986 (Derek)
**          Revised.
**	25-sep-1990 (Walt)
**	    (To fix bug 33419)
**	    Moved here from ck.c
*/
static STATUS
clr_sysprv(
i4             sysprv[2],
i4             oldprv[2])
{
	i4		status;

    /*	Check whether the process had SYSPRV before the call. */

    if ((sysprv[0] & oldprv[0]) == 0)
    {
	/*  Remove authorized SYSPRV, but retain image SYSPRV. */

	status = sys$setprv(0, sysprv, 1, 0);
	if (status & 1)
	    status = sys$setprv(1, sysprv, 0, 0);
	return (status);
    }
    return (SS$_NORMAL);
}
/*{
** Name: CK_spawnnow - spawn a CK command and do not wait
**
** Description:
**      Run the suppled string as a command as ingres which hopefully
**      does a CK operation.
**
** Inputs:
**      command                 command to run
**
** Outputs:
**      err_code                machine dependent error return
**
**      Returns:
**          E_DB_OK
**          CK_SPAWN            Command would not run or exited failure
**
**      Exceptions:
**          if enabled
**          EXCHLDDIED          when command finishs (SIGCHLD)
**
** Side Effects:
**          none
**
** History:
**	15-May-1996 (hanch04)
**          Created from CK_spawn.
**      25-sep-1998 (chash01)
**          change CK_spawn() to PCspawn() so that
**          we can have child process running concurrently
**          with parent process. (93552)
**      27-Jun-2000 (horda03)
**          Enable SYSPRV (VMS) privilege before invoking a spawn
**          command. This privilege must be enables in EXEC or KRNL
**          mode. I've plumped for EXEC mode.
*/
 
STATUS
CK_spawnnow(
char            *command,
PID		*pid,
CL_ERR_DESC     *err_code)
{
    STATUS      ret_val = OK;
 
    i4     status;
    i4     sysprv[2] = { 0x10000000, 0 };
    i4     oldprv[2];
        
    struct
        {
                i4     arg_count;
                i4     *sysprv;
                i4     *oldprv;
        }    sysprv_args = { 2, &sysprv, &oldprv };

    CL_CLEAR_ERR(err_code);

    for(;;)
    {

        /* Before executing the command, acquire the SYSPRV privilege.
        ** This must be done in EXEC (or kernel) mode.
        */

        status = sys$cmexec(set_sysprv, &sysprv_args);

        if ((status & 1) == 0) break;

        ret_val = PCspawn((i4)1, &command, (i1)0, NULL, NULL, pid);

        break;
    }

    /* Restore process privileges */

    status = sys$cmexec(clr_sysprv, &sysprv_args);

    if ((status & 1) == 0)
    {
        /* Can't exit and give the user SYSPRV, so trash the process. */

        sys$delprc(0, 0);
    }

    return(ret_val);
}

/*{
** Name: CK_is_alive - is a CK command alive?
**
** Description:
**	This function basically executes PCis_alive() on non-NT platforms
**	and PCisthread_alive() on NT.
**
** Inputs:
**	pid			The PID / Thread ID to check.
**
** Outputs:
**
**      Returns:
**	    Return code from PCis_alive() of PCisthread_alive().
**
**      Exceptions:
**	    none
**
** Side Effects:
**          none
**
** History:
**	10-nov-1999 (somsa01)
**	    Created.
*/

bool
CK_is_alive(
PID	pid)
{
#ifndef NT_GENERIC
    return(PCis_alive(pid));
#else
    return(PCisthread_alive(pid));
#endif /* NT_GENERIC */
}

