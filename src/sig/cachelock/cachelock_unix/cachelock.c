/*
** Copyright (c) 2004 Ingres Corporation
*/

/* 
** The following include is used to establish the platform on  
** which the program will be run.  The appropriate version
** must be copied to cachelock.h prior to program compilation.
*/
# include "cachelock.h"

# include <stdio.h>
# include <string.h>
# include <sys/types.h>
# include <fcntl.h>
# include <unistd.h>

# ifdef mmap_exists
# include <sys/mman.h>
# endif 

# ifdef no_mmap
# include <sys/ipc.h>
# include <sys/shm.h>
# endif

/*
**
**  Name: CACHELOCK - UNIX Utility Program to lock the shared DMF cache 
**
**  Description:
**
**    This program locks the entire shared DMF cache in processor memory.  
**    The program cannot be used to lock non-shared caches, nor 
**    to lock part of the cache.  It is not usable on all
**    UNIX platforms.  
**
**    Read the "read.me" file before running this program. 
**
**    CACHELOCK is designed for use in systems where it is either
**    undesirable or impossible to lock the cache via the preferred
**    "dmf.lock_cache" server startup parameter.  The program
**    must be run setuid root, and to be effective must remain alive 
**    while the system is up.  
**
**    "mmap_exists" and "no_mmap" ifdefs select code appropriate to
**    either mmap/mlock or older SYS V shmat/shmctl usage. The 
**    "cachelock.h" header file must contain the appropriate #define.
**
**    The server must be running prior to starting this program
**    and must employ the same memory management facility
**    (mmap() or shmctl()) used by this program.  Furthermore, the 
**    server must be configured to run with "shared_cache".
**
**    The only program argument is the name of the shared DMF cache,
**    as follows:
**
**           cachelock <shared_cache_file_name>
**
**    The program runs until stopped by a kill command, or if run
**    interactively, by Control-C.  It will be paged out after it locks 
**    the memory, so imposes little system overhead.
**
**    The program may need to be modified to work properly on
**    some UNIX platforms.
**
**    NOTE:  Locking memory is potentially VERY disruptive to system 
**    performance, so this program should be used with UTMOST CAUTION!!!  
**    Closely monitor system performance after starting the program.
**	  	
**    Inputs (argv):
**        cache_name                      Full pathname of the cache to lock
**                           (typically $II_SYSTEM/ingres/files/cach_def.dat)
**
**    Outputs (exit status):
**         0                              Normal exit
**         Nonzero    			  Error exit code
**
**    Exceptions:
**	   None.
**
**    Side Effects:
**         None. 
**
** History:
**
**      20-mar-1992 (jnash)
**          Created.
**	16-may-1995 (morayf)
**	    Changed shmat return value check from long to (char *) as
**	    this is how it is declared in <sys/shmat.h>.
**	10-may-1999 (walro03)
**	    Remove obsolete version string odt_us5.
*/
int
main( argc, argv )
int    argc;
char   *argv[];
{
    caddr_t     addr;

#ifdef mmap_exists
    int         file_desc;
    size_t      len;
#endif 

#ifdef no_mmap
    int         segid;
    struct shmid_ds shmds;
    key_t       shmem_key;
#endif 

    addr = (caddr_t)0;    /* Let OS choose virtual address */

    if (argc != 2)
    {
        fprintf (stderr, 
	"\nUSAGE: CACHELOCK <cache_name>\n\n");
        fprintf (stderr,
	"    This Ingres utility is used to lock the DMF cache.\n");
        fprintf (stderr,
	"    The program must be run by ROOT, and the Ingres server\n");
        fprintf (stderr,
	"    must be running and configured with 'shared_cache'.\n\n"); 
	fprintf (stderr,
	"    <cache_name> is the full pathname of the file \n");
        fprintf (stderr,
	"                 mapped to the shared DMF cache.\n\n");
        exit(10);         /* invalid parameter list */
    }

#ifdef no_mmap
    /* form key from cache name */
    shmem_key = ftok(argv[1], 'I');

    /* Get id of shared memory segment, don't create it.
    ** The length parameter here need not be accurate, as we'll find
    ** the real length via shmctl().
    */
    if ( (segid = shmget(shmem_key, 8192, 0)) < 0)
    {
        perror ("cachelock shmget()");
        exit(20);         
    }

    /* Get info on shared memory segment */
    if (shmctl(segid, IPC_STAT, &shmds) < 0)
    {
        perror ("cachelock shmctl()");
        exit(30);        
    }

    /* Attach to segment */
    if (shmat(segid, (char *)0, 0) == (void *)-1)
    {
        perror ("cachelock shmat()");
        exit(40);
    }

    /* lock the memory */
    if (shmctl(segid, SHM_LOCK, 0) < 0)
    {
	perror("cachelock shmctl()");
	exit(50);
    }
#endif

#ifdef mmap_exists
    if( (file_desc = open(argv[1], O_RDWR, 0600 )) < 0 )
    {
        perror ("cachelock open()");
        exit(60);         /* cache name invalid? */
    }

    /* length of mmap file = size of cache */
    if ((long) (len = (size_t) lseek(file_desc, (off_t)0, SEEK_END)) == -1L)
    {
        perror ("cachelock lseek()");
        exit(70);
    }

    /* map the memory to mmap file */
    if ((long) (addr = mmap(addr, len, PROT_READ,
                    MAP_SHARED, file_desc, (off_t)0)) == -1L )
    {
        perror ("cachelock mmap()");
        exit(80);
    }

    /* lock all memory in shared segment */
    if (mlock(addr, len) < 0)
    {
        perror ("cachelock mlock()");
        exit(90);
    }
#endif

    pause();        /* wait until killed */

    exit(0);        
}


