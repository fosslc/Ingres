/*
**Copyright (c) 2005 Ingres Corporation
*/
#include    <compat.h>
#include    <gl.h>
#include    <clconfig.h>
#include    <systypes.h>
#include    <pc.h>
#include    <st.h>
#include    <si.h>
#include    <errno.h>
#include    <clsigs.h>

#include <stdlib.h>


/*
** Name: PCexeclp32	    - exec the lp32 version of the running exe
**
** Description:
** 	exec the lp32 version of the running exe
**
** Inputs:
**
** Outputs:
**	none
**
** Returns:
**	STATUS
**
** History:
**	04-Mar-2005 (hanje04)
**	    Created from PCexeclp64
*/
STATUS
PCexeclp32(i4 argc, char **argv)
{
    PCdoexeclp32( argc, argv, TRUE );
}

/*
** Name: PCexeclp32	    - exec the lp32 version of the running exe
**
** Description:
** 	exec the lp32 version of the running exe
**
** Inputs:
**
** Outputs:
**	none
**
** Returns:
**	STATUS
**
** History:
**	04-Mar-2005 (hanje04)
**	    Created PCexeclp64
**	20-Jun-2009 (kschendel) SIR 122138
**	    Hybrid add-on symbol changed, fix here.
*/
STATUS
PCdoexeclp32(i4 argc, char **argv, bool exitonerror)
{
#if ! defined(conf_BUILD_ARCH_64_32) || defined(BUILD_ARCH32)
    return (OK);
#else
    char *exe;
    char exe32[1024];
    char dir64[1024];
    i4	 status=0;
    char *dir;
    char                *debug = NULL;

    PCgetexecname(argv[0],dir64);
    dir = STrchr( dir64, '/' );
    
    if( dir == NULL )
    {
	STprintf(exe32, "lp32/%s", argv[0] );
    }
    else
    {
        exe = dir + 1;
	*dir = '\0';
	STprintf(exe32, "%s/lp32/%s", dir64, exe );
    }
    execvp(exe32, argv);

    /*
    ** Should never get here
    */

    status = errno;

    NMgtAt("II_PC_DEBUGGER", &debug);
    if (debug != (char *)NULL && *debug != EOS)
    {
	/*
	** Can add more debug
	*/
    	SIprintf("execvp() failed for the 32-bit executable: %s\n",argv[0]);
    }

    if( exitonerror )
    {
	PCexit(status);
    }
    return (FAIL);
#endif /* reverse hybrid */
}

/*
** Name: PCspawnlp32	    - exec the lp32 version of the running exe
**
** Description:
** 	spawn the lp32 version of the running exe
**
** Inputs:
**
** Outputs:
**	none
**
** Returns:
**	STATUS
**
** History:
**	04-Mar-2005 (hanje04)
**	    Created from PCspawnlp64
*/
STATUS
PCspawnlp32(i4 argc, char **argv)
{
#if ! defined(conf_BUILD_ARCH_64_32) || defined(BUILD_ARCH32)
    return (OK);
#else
    char *exe;
    char exe32[1024];
    char dir64[1024];
    STATUS	 status=OK;
    PID  pid;

    char *dir;
    char                *debug = NULL;

    PCgetexecname(argv[0],dir64);
    dir = STrchr( dir64, '/' );
    
    if( dir == NULL )
    {
	STprintf(exe32, "lp32/%s", argv[0] );
    }
    else
    {
        exe = dir + 1;
	*dir = '\0';
	STprintf(exe32, "%s/lp32/%s", dir64, exe );
    }
    argv[0] = exe32;
    status = PCspawn(argc, argv, FALSE, NULL, NULL, &pid);

    return (status);
#endif /* reverse hybrid */
}
