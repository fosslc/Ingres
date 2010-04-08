/*
**  Setu.c
**
** Here is a fragment of code that implements a setuser command.  The function
** setu gets the PCB address from a process global symbol.  The main function,
** setusr, calls the setu function via kernel mode.  This requires VAXC 3.0 or
** above and the link command must include:
**		sys$system:sys.stb/sel,sys$system:sysdef.stb/sel
**
**  History:
**	22-mar-1993 (donj)
**	    Created from testtool!sep!septool_vms!setuser.c.
**  30-mar-1993 (donj)
**          Added tracing feature and ALPHA code changes.
**  13-may-1993 (donj)
**	    Made some changes for Alpha.
**      15-oct-1993 (donj)
**          Make function prototyping unconditional.
**  10-jan-1994 (donj)
**          Moved constant declarations to sep!main!generic!hdr setu.h
**          which will be shared between getu.c and setu.c.
**	13-mar-2001	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from testtool code as the use is no longer allowed
**	15-may-2003 (abbjo03)
**	    In setu(), use STmove to copy and pad the user name, because STlcopy
**	    tacks on a null character.
**	06-jan-2004 (abbjo03)
**	    Provide a return value in setu() to prevent bogus warnings when
**	    qasetuser is invoked. Correct calls to SIprintf() which were
**	    supposed to call SIfprintf().
**      17-jul-2007 (stegr01)
**          remove hardcode pcb$l_jib defs
**          cleanup code
*/
#include    <compat.h>
#include    <er.h>
#include    <si.h>
#include    <st.h>
#include    <setu.h>


#include <ssdef>
#include <stsdef>
#include <prvdef>
#include <psldef>
#include <builtins>
#include <pcbdef>
#include <jibdef>

#include <lib$routines>
#include <starlet>



static i4 setu(i4 length, char *string);

i4
setusr(char	*future_user)
{
    i4   ret_val = SS$_NORMAL;
    u_i4 arglst[3];

    if (tracing)
    {
        char* present_user = getusr();

	SIfprintf(traceptr,
	    "setusr> Inside setusr, present_user = <%s>, future_user = <%s>\n",
	    present_user, future_user);
	SIflush(traceptr);
    }

    arglst[0] = 2;
    arglst[1] = STlength(future_user);
    arglst[2] = (u_i4)future_user;
    ret_val = sys$cmkrnl (setu, arglst);
    if (ret_val != SS$_NORMAL) return (ret_val);

    if (tracing)
    {
        char* present_user = getusr();
	SIfprintf(traceptr,
	    "setusr> Leaving setusr, present_user = <%s>, future_user = <%s>\n",
	    present_user, future_user);
	SIflush(traceptr);
    }

    return (ret_val);
}


static i4 
setu(i4 length, char *string)
{
        extern PCB*  const ctl$gl_pcb;
        extern char* ctl$t_username;

        PCB* pcb = ctl$gl_pcb;
        JIB* jib = pcb->pcb$l_jib;

        char space = ' ';


        /* check we can read the new username */

        u_i2 maxlen = sizeof(jib->jib$t_username);
        u_i2 srclen = min(maxlen, length);
        if (!__PAL_PROBER(string, srclen-1, PSL$C_USER)) return (SS$_ACCVIO);

        /* space fill the JIB and CTL region with the new username */

        lib$movc5(&srclen, (u_i4 *)string, &space, &maxlen, (u_i4 *)jib->jib$t_username);
        lib$movc5(&srclen, (u_i4 *)string, &space, &maxlen, (u_i4 *)&ctl$t_username);

        return (SS$_NORMAL);

}

