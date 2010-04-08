/* **************************************************************
**
** getu.c
**
**  History:
**
**  26-mar-1993 (donj)
**          Created out of setuser.c for inclusion in SEP.
**  30-mar-1993 (donj)
**          Added tracing feature. Fixed a bug in setusr().
**          Added ALPHA code changes.
**  13-may-1993 (donj)
**          Made some changes for Alpha.
**   5-jan-1994 (donj)
**          MY_pcb$l_jib should be 252 for ALPHA.
**  10-jan-1994 (donj)
**	    Moved constant declarations to sep!main!generic!hdr setu.h
**	    which will be shared between getu.c and setu.c.
**  11-jan-1994 (donj)
**	    Added check for EOS in getusr() looking for the end
**	    of the username.
**	13-mar-2001	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from testtool code as the use is no longer allowed
**	07-jan-2004 (abbjo03)
**	    Use STmove to copy and pad the username. Fix return values.
**      17-jul-2007 (stegr01)
**          remove hardcode pcb$l_jib defs
**
** **************************************************************
*/
#include    <compat.h>
#include    <er.h>
#include    <cm.h>
#include    <me.h>
#include    <si.h>
#include    <st.h>
#include    <setu.h>


#include <ssdef>
#include <stsdef>
#include <prvdef>
#include <psldef>
#include <builtins>
#include <pcbdef>
#include <phddef>
#include <jibdef>

#include <lib$routines>
#include <starlet>



static i4 getu(i4 length, char *string);

char*
getusr()
{
    char* p;
    i4    sts;
    i4    n = USERNAME_MAX;
    char* cptr  = (char *)MEreqmem(0, n+1, TRUE, (STATUS *)NULL);
    u_i4 arglst[] = {2, n, (u_i4)cptr};

    /*
    ** N.B. getu() does not return a null terminated string
    ** so there is an implicit assumption that
    **
    ** (a) MAX_USERNAME >= sizeof(jib->jib$t_username) [12]
    ** (b) our buffer is prezeroed so we don't run off the end
    **
    */

    sts = sys$cmkrnl (getu, arglst);
    if (sts != SS$_NORMAL) *cptr = EOS;

    for (p = cptr; ((!CMwhite(p))&&(*p != EOS)); CMnext(p));
    *p = EOS;

    return (cptr);
}

static i4 
getu(i4 length, char *string)
{
    /* this function must be executed in KERNEL mode */

    extern PCB*  const ctl$gl_pcb;
    extern PHD*  const ctl$gl_phd;

    PCB* pcb = ctl$gl_pcb;
    PHD* phd = ctl$gl_phd;
    JIB* jib = pcb->pcb$l_jib;

    char space = ' ';

    /* check we can write the user supplied buffer */

    u_i2 srclen = sizeof(jib->jib$t_username);
    if (!__PAL_PROBEW(string, length-1, PSL$C_USER)) return (SS$_ACCVIO);

    lib$movc5(&srclen, (u_i4 *)jib->jib$t_username, &space, &length, (u_i4 *)string);

    return (SS$_NORMAL);
}
