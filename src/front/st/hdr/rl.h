/*
**  Copyright (c) 2004 Ingres Corporation
**
** rl.h -- header for rl.c
**
**  History:
**    06-Mar-2004 (hanje04)
**	  Created history.
**    08-Mar-2004 (hanje04)
**        SIR 107679
**        Improve syscheck functionality for Linux:
**        Define more descriptive error codes so that ingstart can make a
**        more informed decision about whether to continue or not. (generic)
**        Extend linux functionality to check shared memory values.
**        Add -c flag and the ability to write out kernel parameters needed
**        by the system in order to run Ingres.
**        NOTE: This is really not a complete solution, the whole syscheck
**        / rl utility needs rewriting as it is very difficult to follow
**        and rather out of date.
**    14-Nov-2006 (bonro01)
**        On linux retrieve shminfo values from /proc/sys/kernel files
**        instead of using sysctl().  Create 64bit _shminfo to save
**        the values.
**    17-Oct-2007 (hanje04)
**	    BUG 114907
**	    Defined _shminfo for OSX too as it's needed on 10.4
**	12-Feb-2008 (hanje04)
**	    SIR S119978
**	    Replace mg5_osx with generic OSX
*/

/* maximum length of string returned by RLcheck() */ 
# define RL_MAX_LEN	512	

/*
** WARNING: syscheck relies on each hard limit identifier being
** 	smaller than the corresponding soft limit id.
*/
# define RL_PROC_FILE_DESC_HARD		0
# define RL_PROC_FILE_DESC_SOFT		1
# define RL_PROC_FILE_SIZE_HARD		2
# define RL_PROC_FILE_SIZE_SOFT		3
# define RL_PROC_RES_SET_HARD		4
# define RL_PROC_RES_SET_SOFT		5
# define RL_PROC_CPU_TIME_HARD		6
# define RL_PROC_CPU_TIME_SOFT		7
# define RL_PROC_DATA_SEG_HARD		8
# define RL_PROC_DATA_SEG_SOFT		9
# define RL_PROC_STACK_SEG_HARD		10
# define RL_PROC_STACK_SEG_SOFT		11
# define RL_SYS_SHARED_SIZE_HARD	12
# define RL_SYS_SHARED_SIZE_SOFT	13
# define RL_SYS_SHARED_NUM_HARD		14
# define RL_SYS_SHARED_NUM_SOFT		15
# define RL_SYS_SEMAPHORES_HARD		16
# define RL_SYS_SEMAPHORES_SOFT		17
# define RL_SYS_SEM_SETS_HARD		18
# define RL_SYS_SEM_SETS_SOFT		19
# define RL_SYS_SEMS_PER_ID_HARD	20
# define RL_SYS_SEMS_PER_ID_SOFT	21
# define RL_SYS_SWAP_SPACE_HARD		22
# define RL_SYS_SWAP_SPACE_SOFT		23
/*
** syscheck also relies on RL_END_OF_LIST being the largest identifier.
*/ 
# define RL_END_OF_LIST			24

/*
** Define syscheck return codes.
*/

# define RL_KMEM_OPEN_FAIL         1
# define RL_FILE_DESC_FAIL         2
# define RL_FILE_SIZE_FAIL         3
# define RL_RES_SET_FAIL           4
# define RL_CPU_TIME_FAIL          5
# define RL_DATA_SEG_FAIL          6
# define RL_STACK_SEG_FAIL         7
# define RL_SHARED_SIZE_FAIL        8
# define RL_SHARED_NUM_FAIL         9
# define RL_SEMAPHORES_FAIL         10
# define RL_SEM_SETS_FAIL           11
# define RL_SEMS_PER_ID_FAIL        12
# define RL_SWAP_SPACE_FAIL         13

FUNC_EXTERN STATUS RLcheck();

#if defined(LNX) || ( defined(OSX) && ! defined(_shminfo) )
/* Define shared memory structures */
struct _shminfo {
        i8 shmmax;
        i8 shmmni; 
        i8 shmall;
} ;
#endif
