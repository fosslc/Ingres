/*
**	Copyright (c) 2004 Ingres Corporation
*/
# ifndef PC_HDR_INCLUDED
# define PC_HDR_INCLUDED 1

/* for LOCATION definition */
#include    <lo.h>
/* for EX definition */
#include    <ex.h>
#include    <si.h>
#include    <pccl.h>

/**CL_SPEC
** Name:	PC.h	- Define PC function externs
**
** Specification:
**
** Description:
**	Contains PC function externs
**
** History:
**	2-jun-1993 (ed)
**	    initial creation to define func_externs in glhdr
**	13-jun-1993 (ed)
**	    change PCpid to be VOID
**	14-Nov-96 (gordy)
**	    Added PCtid().
**	31-mar-1997 (canor01)
**	    Protect against multiple inclusion of pc.h.
**      18-jun-1997 (harpa06)
**          Added PCcmdline95()
**	23-jan-1998 (somsa01)
**	    Added PCexec_suid().
**	10-nov-1999 (somsa01)
**	    Added PCisthread_alive().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	15-aug-2002 (hanch04)
**	    Added PCexeclp64().
**	06-dec-2006 (drivi01)
**	    Add PCadjust_SeDebugPrivilege to windows CL.
**      10-Jan-2007 (rajus01)
**         Changed the return type for PCtid() to u_i4. Added PCtidx(),
**         a cover for PCtid() to return Pseudo Thread IDs on those OS
**         platforms that do not re-use the thread IDs.
**      26-Apr-2007 (hanal04) Bug 118196
**         Define format for printing pids in a common place.
**	25-Jul-2007 (drivi01)
**	   Added PCisAdmin() function for Vista port.
**	   Checks if the user running the application has
**	   Administrative prpivileges
**	18-Sep-2007 (drivi01)
**	   PCisAdmin can be NT_GENERIC now that we only use it in a
**	   macro is epcl.h.
**	24-Nov-2009 (frima01) Bug 122490
**	   Added prototype for PCreap to eliminate gcc 4.3 warnings.
**	1-Dec-2010 (kschendel) SIR 124685
**	    Kill CL_PROTOTYPED (always on now).
**      06-Dec-2010 (horda03) SIR 124685
**          Fix VMS build problems, 
**/

VOID PCatexit(
	    VOID	(*func)()
);

FUNC_EXTERN STATUS  PCcmdline(
	    LOCATION	*interpreter, 
	    char	*cmdline,
	    i4		wait, 
	    LOCATION	*err_log, 
	    CL_ERR_DESC *err_code
);

FUNC_EXTERN STATUS  PCcmdline95(
	    LOCATION	*interpreter, 
	    char	*cmdline,
	    i4		wait, 
	    LOCATION	*err_log, 
	    CL_ERR_DESC *err_code
);

FUNC_EXTERN VOID PCcrashself(
	    void
);

FUNC_EXTERN VOID    PCexit(
	    STATUS	status
);

#ifdef NT_GENERIC
FUNC_EXTERN STATUS PCexec_suid(
	    char	*cmdbuf
);
FUNC_EXTERN STATUS PCadjust_SeDebugPrivilege(
	    BOOL bEnablePrivilege,
	    CL_ERR_DESC *err_desc	    
);
FUNC_EXTERN BOOL PCisAdmin( 
void 
);
#endif /* NT_GENERIC */




#ifndef PCpid
FUNC_EXTERN VOID    PCpid(
	    PID		*pid
);
#endif

#ifdef VMS
#define PIDFMT "%-10x"
#else
#define PIDFMT "%-10d"
#endif

#ifndef PCtid
FUNC_EXTERN u_i4 PCtid( VOID );
#endif

#ifndef PCtidx
FUNC_EXTERN u_i4 PCtidx(VOID);
#endif

FUNC_EXTERN VOID    PCsleep(
	    u_i4	msec
);

FUNC_EXTERN VOID    PCunique(
	    char	*uniquestr
);

FUNC_EXTERN STATUS  PCforce_exit(
	    PID		pid, 
	    CL_ERR_DESC *err_desc
);

FUNC_EXTERN bool    PCis_alive(
	    PID		pid
);

FUNC_EXTERN bool    PCisthread_alive(
	    PID		pid
);

FUNC_EXTERN STATUS  PCexeclp64(
	    i4 argc,
	    char **argv
);

FUNC_EXTERN i4 PCreap(void);
STATUS	PCread(char *, char *);
STATUS	PCfspawn(i4, char **, bool, FILE **, FILE **, PID *);
#ifdef xPURIFY
VOID	purify_override_PCexit(i4);
#endif
STATUS	PCspawnlp64(i4 argc, char **argv);
STATUS	PCexeclp32(i4 argc, char **argv);
STATUS	PCspawnlp32(i4 argc, char **argv);
STATUS	PCendpipe(PIPE);
void	PCcleanup(EX);

# endif /* ! PC_HDR_INCLUDED */
