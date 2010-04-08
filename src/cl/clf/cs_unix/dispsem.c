/*
**Copyright (c) 2004 Ingres Corporation
*/
/**
**
**  Name: DISPSEM.C - Display Semaphores for an installation.
**
**  Description:
**      This program will display semaphore values given an argument list of
**	semaphore ids.
**
**          main() - The dispsem main program
**
**
**  History:
**      ???
**	    Created.
**      15-apr-89 (arana)
**	    Removed ifdef xCL_002 around struct semun.  Defined struct semun
**	    in clipc.h ifndef xCL_002.  Fixed first semctl call to pass
**	    pointer to semid_ds struct instead of vals array.
**	5-june-90 (blaise)
**	    Integrated changes from ingresug:
**		Add xCL_075 around SysV shared memory calls.
**	30-dec-92 (mikem)
**	    su4_us5 port.  Added cast to comparison with semstat.sem_nsems to
**	    eliminate su4_us5 acc warning: "semantics of "<" change in ANSI C".
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**/

/* Don't grep system header directly, use RTI headers (fredv) */
#include    <compat.h>
#include    <gl.h>
#include    <clconfig.h>
#include    <systypes.h>
#include    <clipc.h>

main(argc, argv)
int	argc;
char	**argv;
{
# ifdef xCL_075_SYS_V_IPC_EXISTS
    while (--argc && *++argv)
	reptsem(*argv);
    exit(0);
}

reptsem(sid)
char	*sid;
{
    int	id, i;
    struct	semid_ds	semstat;
    u_short	vals[64];
    union semun arg;

    id = atoi(sid);
    printf("sem %d:\n", id);
    arg.buf = &semstat;
    semctl(id, 0, IPC_STAT, arg);
    printf("nsem %d:", semstat.sem_nsems);
    arg.array = vals;
    semctl(id, 0, GETALL, arg);
    for (i = 0; i < (int) semstat.sem_nsems; i++)
	printf(" %d", vals[i]);
    printf("\n");
# else
    exit(0);
# endif
}
