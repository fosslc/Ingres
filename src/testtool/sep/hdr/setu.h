/*
** setu.h
**
**	This is a VMS (VAX and ALPHA) specific file.
**
**  10-jan-1994 (donj)
**	Created this file. Moved constant declarations to
**	sep!main!generic!hdr setu.h which will be shared between
**	getu.c and setu.c.
**	07-jan-2004 (abbjo03)
**	    Change arg to u_i4 because sys$cmkrnl expects unsigned ints.
*/
#ifdef VMS

#ifdef ALPHA

#define     MY_pcb$l_jib    252

#else

#pragma     builtins
#define     MY_pcb$l_jib    124

#endif /* ALPHA */

#define	    USERNAME_MAX    64

GLOBALREF    i4            tracing ;
GLOBALREF    i4            trace_nr ;
GLOBALREF    FILE         *traceptr ;

FUNC_EXTERN  char          *getusr () ;

u_i4                        arg [4] ;
char                        err_buffer [600] ;

#endif /* VMS */
