/*
**Copyright (c) 2004 Ingres Corporation
*/
#include    <sys/types.h>
#include    <stdio.h>
#include    <fcntl.h>
#include    <errno.h> 
extern	int	errno;

/*
** Test to see if there exists the CL_POLL_RESVFD problem
** and at what fd stdio fopen() and open() fail
*/


main( argc, argv)
int argc;
char *argv[];
{
    int	fd_counter ;
    int verbose = 0;
    int	 dont_stdio = 0 ;
    FILE	*fp ;
    int	dummy_fd,last_fd , stdio_fail_fd;


    if (argc == 2)
    if (!strcmp("-v", argv[1]))
          verbose = 1 ;

    for (fd_counter = 0 ; fd_counter < 2000 ; fd_counter++ )
    {
	dummy_fd = open("/etc/passwd",O_RDONLY) ;
	if (dummy_fd < 0)
	{
	    if (verbose)
	        printf("open() failure at last_fd %d errno %d\n",last_fd,errno);
	    break ;
	}
	last_fd = dummy_fd ;
	if (!dont_stdio)
	{
	    if( !( fp = fopen( "/etc/passwd", "r" ) ) )
	    {
		if (verbose)
		    printf("Stdio failure at last_fd %d, errno %d \n", last_fd, errno);
		dont_stdio = 1;
		stdio_fail_fd = last_fd ;
	    }
	    else
	    {
		fclose(fp);
	    }
	}
    }
    if (stdio_fail_fd == last_fd)
    {
	if (last_fd < 256)
	{
	    printf("/*# define xCL_091_BAD_STDIO_FD-can't determine-open() fails at fd=%d */\n", last_fd);
	}
	if (verbose)
	  printf("No stdio problem for this platforn until fd %d\n", stdio_fail_fd); 
    }
    else
    {
	if (verbose)
	  printf("stdio_fail_fd = %d open_fail_fd = %d\n", stdio_fail_fd, last_fd);
	printf("#define xCL_091_BAD_STDIO_FD\n");
    }
}
