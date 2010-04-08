/*
** Copyright (c) 2004 Ingres Corporation
*/

/* 
** The following include is used to establish the platform on  
** which the program will be run.
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

#define MEM_PAGESIZE 8192           /* Machine dependent */

/*
**
**  Name: MEMTEST - UNIX Utility Program to check residency of DMF cache
**
**  Description:
**
**    This program uses the mincore() function to check the residency of
**    pages in the shared DMF cache.  It is designed to be used in
**    conjunction with the Ingres CACHELOCK utility to check the
**    need for and effectiveness of DMF cache locking. 
**
**    The program loops indefinitely, periodically reporting on the 
**    number of resident cache pages.  It must be stopped via a kill
**    command.
**
**    The program can only be used on systems possessing mincore().
**    As with the CACHELOCK utility, the cachelock.h header file must
**    be set appropriate to the platform.
**
**    The Ingres server must be up and running when this program is started,
**    the server must be configured with "shared_cache".  
**
**    The only argument to the program is the name of the shared DMF cache,
**    and can be run as follows:
** 
**           memtest <shared_cache_file_name>
**
**    Inputs (argv):
**        cache_name                      Full pathname of the DMF cache
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
**      20-mar-1992 (jnash)
**          Created.
*/
int
main( argc, argv )
int    argc;
char   *argv[];
{
    caddr_t     addr;
    char        vec[500];
    int         num_pages;
    int		found;
    int		i;
    size_t      len;

#ifdef mmap_exists
    int         file_desc;
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
	"\nUSAGE: MEMTEST <cache_name> \n");
        fprintf (stderr,
	"    The MEMTEST utility is used to check DMF cache residency.\n");
        fprintf (stderr,
	"    The program must be run as Ingres, and the server\n");
        fprintf (stderr,
	"    must be running and configured with 'shared_cache'.\n\n"); 
        exit(10);         /* invalid parameter list */
    }

#ifdef no_mmap
    /* form key from cache name */
    shmem_key = ftok(argv[1], 'I');

    /* Get id of shared memory segment, don't create it.
    ** The length parameter here need not be accurate, as we will find
    ** the real length via shmctl().
    */
    if ( (segid = shmget(shmem_key, 8192, 0)) == -1)
    {
        perror ("memtest shmget()");
        exit(20);         
    }

    /* Get info on shared memory segment */
    if (shmctl(segid, IPC_STAT, &shmds) < 0)
    {
        perror ("memtest shmctl()");
        exit(30);        
    }

    /* Attach to segment */
    addr = (char *)shmat(segid, (char *)0, 0);
    if (addr == (char *)-1L)
    {
        perror ("memtest shmat()");
        exit(40);
    }

    len = shmds.shm_segsz;
#endif

#ifdef mmap_exists
    if( (file_desc = open(argv[1], O_RDWR, 0600 )) < 0 )
    {
        perror ("memtest open()");
        exit(50);         /* cache name invalid */
    }

    /* Length of mmap file = size of cache */
    if ((len = lseek(file_desc, (off_t)0, SEEK_END)) == -1L)
    {
        perror ("memtest lseek()");
        exit(60);
    }

    /* map the memory to the mmap file */
    if ((long) (addr = mmap(addr, len, PROT_READ,
                    MAP_SHARED, file_desc, (off_t)0)) == -1L )
    {
        perror ("memtest mmap()");
        exit(70);
    }
#endif

    num_pages = len / MEM_PAGESIZE;

    for (;;)
    {
        if (mincore(addr, len, vec) == -1)
        {
            perror ("memtest mincore()");
            exit(80);         /* mincore problem */
        }

        found = 0;
        for (i = 0;  i < num_pages; i++)
        {
            if ((vec[i] && 1))
                found++;
        }

        fprintf(stderr, "%d out of %d DMF cache pages in core\n", found, num_pages);
        sleep(10);
    }
}
