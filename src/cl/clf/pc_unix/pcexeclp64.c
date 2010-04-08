/*
**Copyright (c) 2004 Ingres Corporation
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
** Name: PCexeclp64	    - exec the lp64 version of the running exe
**
** Description:
** 	exec the lp64 version of the running exe
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
**	13-aug-2002 (hanch04)
**	    Created
**	10-sep-2002 (hanch04)
**	    Added PCspawnlp64 to spawn the 64-bit program and continue to run the 32-bit one.
**	28-Oct-2002 (hanch04)
**	    Print debug output only if II_PC_DEBUGGER is set
**	20-Jun-2009 (kschendel) SIR 122138
**	    Hybrid add-on symbol changed, fix here.
*/
STATUS
PCexeclp64(i4 argc, char **argv)
{
    PCdoexeclp64( argc, argv, TRUE );
}

/*
** Name: PCexeclp64	    - exec the lp64 version of the running exe
**
** Description:
** 	exec the lp64 version of the running exe
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
**	13-aug-2002 (hanch04)
**	    Created
*/
STATUS
PCdoexeclp64(i4 argc, char **argv, bool exitonerror)
{
#if ! defined(conf_BUILD_ARCH_32_64) || defined(BUILD_ARCH64)
    return (OK);
#else
    char *exe;
    char exe64[1024];
    char dir32[1024];
    i4	 status=0;
    char *dir;
    char                *debug = NULL;

    PCgetexecname(argv[0],dir32);
    dir = STrchr( dir32, '/' );
    
    if( dir == NULL )
    {
	STprintf(exe64, "lp64/%s", argv[0] );
    }
    else
    {
        exe = dir + 1;
	*dir = '\0';
	STprintf(exe64, "%s/lp64/%s", dir32, exe );
    }
    execvp(exe64, argv);

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
    	SIprintf("execvp() failed for the 64-bit executable: %s\n",argv[0]);
    }

    if( exitonerror )
    {
	PCexit(status);
    }
    return (FAIL);
#endif /* hybrid */
}

/*
** Name: PCspawnlp64	    - exec the lp64 version of the running exe
**
** Description:
** 	spawn the lp64 version of the running exe
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
**	30-aug-2002 (hanch04)
**	    Created
*/
STATUS
PCspawnlp64(i4 argc, char **argv)
{
#if ! defined(conf_BUILD_ARCH_32_64) || defined(BUILD_ARCH64)
    return (OK);
#else
    char *exe;
    char exe64[1024];
    char dir32[1024];
    STATUS	 status=OK;
    PID  pid;

    char *dir;
    char                *debug = NULL;

    PCgetexecname(argv[0],dir32);
    dir = STrchr( dir32, '/' );
    
    if( dir == NULL )
    {
	STprintf(exe64, "lp64/%s", argv[0] );
    }
    else
    {
        exe = dir + 1;
	*dir = '\0';
	STprintf(exe64, "%s/lp64/%s", dir32, exe );
    }
    argv[0] = exe64;
    status = PCspawn(argc, argv, FALSE, NULL, NULL, &pid);

    return (status);
#endif /* hybrid */
}
