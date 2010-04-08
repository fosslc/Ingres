/*
** Copyright (c) 1986, 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <nm.h>
#include    <lo.h>
#include    <me.h>
#include    <pc.h>
#include    <st.h>
#include    <tr.h>
#include    <ex.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <adf.h>
#include    <lg.h>
#include    <lk.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <dm.h> 
#include    <dmp.h> 
#include    <dmpp.h> 
#include    <dm1b.h> 
#include    <dm0l.h> 
#include    <dmd.h>
#include    <dmftrace.h>

/**
**
**  Name: DMDCHECK.C - Debug Check.
**
**  Description:
**      This routine can be called to display a message for an 
**      unexpected condition
**	and signal the debugger.
**
**          dmd_check - Display a check message and breakpoint.
**
**
**  History:
**      14-jan-1986 (derek)    
**          Created new for jupiter.
**      22-june-1992 (jnash)
**          Added forced dmd_check tracing.
**	07-july-1992 (jnash)
**	    Add DMF Function prototyping.
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	14-dec-1992 (rogerk)
**	    Re-arrange includes for new log record definitions.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (rogerk)
**	    Include respective CL headers for all cl calls.
**	31-jan-1994 (jnash)
**	    Add trace point DM1316 to bypass dump of dmf memory.
**	03-jun-1996 (canor01)
**	    Remove NMloc() semaphore.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**	12-Apr-2008 (kschendel)
**	    Need to include adf.h now.
**	23-Oct-2008 (jonj)
**	    SIR 120874 Modified to use new DB_ERROR based uleFormat 
**/

/* External variables */

/*
**  Definition of static variables and forward static functions.
*/

#define DMDCHECK_LOGFILE_NAME "iicrash.log"
#define DMDCHECK_FILENAME_LEN ((sizeof(DMDCHECK_LOGFILE_NAME))-1)


/*{
** Name: dmd_check_fcn	- Display check message and signal the debugger.
**
** Description:
**      Display the string for the check message and signal the debugger.
**
** Inputs:
**      text                            The text of the message to display.
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	14-jan-1986 (derek)
**	    Created new for jupiter.
**	22-june-1992 (jnash)
**	    Added forced dmd_check tracing.  If no TRset_file has been
**	    performed previously, force output to DMDCHECK_FILENAME.
**	31-jan-1994 (jnash)
**	    Add trace point DM1316 to bypass dump of dmf memory.
**      12-jun-1995 (canor01)
**          semaphore protect NMloc()'s static buffer
**	03-jun-1996 (canor01)
**	    Remove NMloc() semaphore.
**	25-Apr-2003 (jenjo02) SIR 111028
**	    dmd_check() is now a macro, appending and passing
**	    caller's file name and source line to this
**	    dmd_check_fcn(). Added E_DM9449_DMD_CHECK
**	    to report how we got here.
**	    Added call to CS_dump_stack() as well.
**	28-jan-2004 (penga03)
**	    Changed CSdump_stack() to CS_dump_stack(0,0,0,0).
**	28-jan-2004 (somsa01)
**	    Reversed last change, as CSdump_stack() was added for Windows
**	    as well.
*/
VOID
dmd_check_fcn(
i4	    error_code,
char	    *file,
i4	    line)
{
    i4	    local_err_code;
    STATUS	    stat;
    LOCATION	    loc;
    CL_ERR_DESC 	    sys_err;
    i4		    l_qualname;
    char	    *qualname;
    char	    crash_fname[DMDCHECK_FILENAME_LEN + 2 ];

    /* Report where we came from */
    uleFormat(NULL, E_DM9449_DMD_CHECK, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
			(char *)NULL, (i4)0, (i4 *)NULL, &local_err_code, 2,
			0, file,
			0, line);

    /* ... and how we got here */
    CSdump_stack();

    /* If trace file not defined, force one via TR_NOOVERRIDE_MASK */
    MEmove(DMDCHECK_FILENAME_LEN, DMDCHECK_LOGFILE_NAME, ' ',
            sizeof(crash_fname), crash_fname);
    crash_fname[DMDCHECK_FILENAME_LEN] = 0;

    if ( NMloc(UTILITY, FILENAME, crash_fname, &loc) == OK) 
    {
	LOtos(&loc, &qualname);
	l_qualname = STlength(qualname);
	stat = TRset_file(TR_F_APPEND | TR_NOOVERRIDE_MASK, qualname, 
			l_qualname, &sys_err);
	if ( stat == OK )
	{
	    TRdisplay("%@ DMD_CHECK: Crash log created.\n");
	    uleFormat (NULL, E_DM942B_CRASH_LOG, (CL_ERR_DESC *)NULL, ULE_LOG, 
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, &local_err_code, 
		1, DMDCHECK_FILENAME_LEN, crash_fname);
	}
    }

    uleFormat(NULL, error_code, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL, 
			(i4)0, (i4 *)NULL, &local_err_code, 0);
    /*
    ** If DM1316 set, bypass dump of dmf memory to either II_DBMS_LOG
    ** or to iicrash.log, whichever is being used here.
    */
    if (DMZ_ASY_MACRO(16))
	TRdisplay("DMD_CHECK: DMF memory dump inhibited by trace point.\n");
    else
	dmd_dmp_pool(1);

    EXsignal(EX_DMF_FATAL, 0);
}
