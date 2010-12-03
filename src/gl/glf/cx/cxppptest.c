
/*
** Copyright (c) 2005 Ingres Corporation
**
*/

/*
** Name: cxppptest.c - Test program for CXPPP.
**
** Description:
**     This breath taking hack, includes all of CXPPP.C as the body
**     of an independent test program.  This program was done in haste,
**     and is cruder than cxtest, and the other test programs for CX. 
**    
**     This verifies the basic functioning of the CXPPP facility.
**    
**     At least one instance should be started with parameter "read"
**     Then one or more instances can be started with parameter "write",
**     and the host name of the listener to converse with.
**    
**     Listener will verify that all messages are received.
**    
**     To exit listener, hit cntl-C.
**
** History:
**	16-may-2005 (devjo01)
**	    Created.   
**	31-may-2005 (devjo01)
**	    Fix compiler warnings missed in VMS build.
**      03-nov-2010 (joea)
**          Declare local functions static.
*/

# define INCLUDED_FROM_CXPPPTEST
# include <cxppp.c>

static void
usage( char *text )
{
    if ( text )
    {
	printf( "Bad parameter! (%s)\n\n", text );
    }
    printf("Usage: cxppptest [ read | write <host> [iterations] ]\n");
    exit (1);
}

main(int argc, char **argv)
{
#   define	REC_SIZE	16
#   define 	REC_FMT		"[%07.7d-%06.6d]"
#   define	REC_CNT		 8
#   define	ITERATIONS	10000
#   define      MAX_PARTNERS	16

    void	*ctx;
    bool write = FALSE;
    STATUS	 status;
    char	*datap;
    char	*hostname;
    i4		 fatal;
    i4		 datacount, iter, total, iterations;
    i4		 nextpartner, partner;
    struct
    {
	i4	pid;
	i4	data;
    } 		 partners[MAX_PARTNERS];
    PID		 cpid;

    if (argc < 2)
    {
	usage( NULL );
    }

    if( !STbcompare( argv[1], 0, "write", 0, TRUE )  )
    {
	iterations = ITERATIONS;
        write = TRUE;
	if (argc < 3 )
	{
	    usage( "missing host name" );
	}
	hostname = argv[2];
	if ( ( argc == 4 ) && ( 1 != sscanf(argv[3], "%d", &iterations) ) )
	{
	    usage( argv[3] );
	}
    }
    else if ( !STbcompare( argv[1], 0, "read", 0, TRUE )  )
    {
        write = FALSE;
    }
    else
    {
	usage( argv[1] );
    }

    do
    {
	if (write)
	{
	    i4		i, cnt, pid;
	    char	*bp, buf[512];

	    /*
	    ** Create unique process ID by mangling PID with time.
	    ** On VMS, each program run from the same session will
	    ** have the same PID.
	    */
	    PCpid( &pid );
	    pid = ( (pid << 16) | (0x0FFFF & time(NULL)) ) & 0x03FFFFF;

	    printf("*** Writing %d records to %s with ID %d ***\n\n",
	     iterations, hostname, pid );
	    printf("Allocating context.\n");
	    status = CXppp_alloc( &ctx, NULL, 0, 0 , FALSE);
	    if ( status ) break;

	    printf("Establishing connection\n");
	    status = CXppp_connect( ctx, hostname, "CXPPP_TEST" );
	    if ( status ) break;
	    printf("Established contection.\n");


	    iter = 0;
	    while ( iter < iterations )
	    {
#		if  0
		    /* Write multiple blocks */
		    cnt = 1 + rand() % 5;
		    if ( iter + cnt > iterations ) cnt = iter - 100;
		    for ( i = 0, bp = buf; i++ < cnt; )
		    {
			sprintf( bp, REC_FMT, pid, iter + i );
			bp += REC_SIZE;
		    }
#		else
		    cnt = 1;
		    sprintf( buf, REC_FMT, pid, iter );
#		endif

		if ( 1 == cnt )
		    printf("Writing data rec %d.\n", iter );
		else
		    printf("Writing data recs %d-%d.\n", iter, iter+cnt-1 );

		status = CXppp_write( ctx, buf, REC_SIZE, cnt );
		if ( status ) break;

		iter += cnt;
	    }

	    printf("Disconnecting.\n");
	    status = CXppp_disconnect( ctx );
	    if ( status ) break;

	    printf("closed connection.\n");
	}
	else
	{
	    printf("*** Establishing CXPPP_TEST listener ***\n\n" );
	    printf("Allocating context.\n");
	    status = CXppp_alloc( &ctx, "CXPPP_TEST", 2,
	             100 * sizeof(ALIGN_RESTRICT) * REC_SIZE, FALSE );
	    if ( status ) break;

	    printf("Listening for a connection\n");
	    status = CXppp_listen( ctx );
	    if ( status ) break;

	    printf("Waiting for data\n");
	    total = fatal = nextpartner = 0;
	    while ( !fatal )
	    {
		status = CXppp_read( ctx, REC_SIZE, 8, 0, 
				     &datap, &datacount );
		if ( status ) break;

		total += datacount;
		printf("%7d Read %d record(s) at %x\n", total, 
		  datacount, datap );
		while ( !fatal && datacount-- )
		{
		    if ( 2 != sscanf(datap, "[%d-%d]",  &cpid, &iter ) )
		    {
			printf("*** Fail to parse: %16.16s ***\n", datap );
			fatal=1;
			break;
		    }
		    partner = nextpartner;
		    while ( partner-- > 0 )
		    {
			if ( cpid == partners[partner].pid )
			{
			    /* Found partner, check if iteration is correct */
			    if ( iter != partners[partner].data + 1 )
			    {
				printf("*** Iteration mismatch "
				"found %d, expected %d for %d\n",
				partners[partner].data, iter, cpid );
				fatal=1;
				break;
			    }
			    partners[partner].data = iter;
			    break;
			}
		    }
		    if ( partner < 0 )
		    {
			if ( nextpartner >= MAX_PARTNERS )
			{
			    partner = MAX_PARTNERS;
			    while ( partner-- > 0 )
			    {
				if ( partners[partner].data == iterations )
				{
				    /* reuse slot */
				    break;
				}
			    }
			}
			else
			{
			    partner = nextpartner++;
			}

			if ( partner < 0 )
			{
			    /* No slots free, abort test. */
			    printf("*** All %d partner slots in use"
			           "stopping test\n" );
			    fatal = 1;
			    break;
			}
			partners[partner].pid = cpid;
			partners[partner].data = iter;
		    }
		    datap += REC_SIZE;
		}
	    }

	    printf("Closing Listen\n");
	    status = CXppp_close( ctx );

        }
	printf("Freeing context\n");
	status = CXppp_free(&ctx);
    } while (0);
    printf( "Status = %d\n", status );
    PCexit(0);
}

