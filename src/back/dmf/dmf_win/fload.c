/*
**  Copyright (c) 1995, 2001 Ingres Corporation
**
**  Name: fastload   
**
**  Usage:
**	fastload dbname -file=<filename> -table=<tablename>
**
**  Description:
**	This program provides the INGRES `fastload' command by
**	executing the journaling support program, passing along the
**	command line arguments.
** 
**  Exit Status:
**	OK	fastload (dmfjsp) succeeded.
**	FAIL	fastload (dmfjsp) failed.	
**
**  History:
**	25-mar-1997 (mcgem01)
**		Created.  (based upon relocdb.c)
**      03-oct-2001 (mcgem01)
**		Run dmfjsp via PCexec_suid() so that the command
**		may be run from accounts other than 'ingres'.
**      23-apr-2004 (rigka01) bug#112185, INGDBA279
**		fastload cannot access the shared memory segment when
**              run under user ID that did not start ingres when 
**              ingres is started as a service
**      31-aug-2004 (sheco02)
**          X-integrate change 468064 to main.
** 
*/

/*
PROGRAM = fastload
**
*/
# include <compat.h>
# include <er.h>
# include <lo.h>
# include <pc.h>
# include <st.h>


/*
**  Execute a command, blocking during its execution.
*/
static STATUS
execute( char *cmd )
{
	CL_ERR_DESC err_code;

	return( PCcmdline( (LOCATION *) NULL, cmd, PC_WAIT,
		(LOCATION *) NULL, &err_code) );
}


void
main(int argc, char *argv[])
{
#define 	MAXBUF	4095

	char 	buf[ MAXBUF+1 ];
	int		iarg, ibuf, ichr;

	/*
	 *	Put the program name and first parameter in the buffer.
	 */
	STprintf(buf, ERx( "dmfjsp fastload" ));

	switch(argc)
	{
	    case 1:
		break;

		/*
		 *	Append any args from the command line, e.g., 
		 *		dmfjsp fastload dbname
		 */
	    default:
		ibuf = sizeof(ERx("dmfjsp fastload")) - 1;
		for (iarg = 1; (iarg < argc) && (ibuf < MAXBUF); iarg++) {
		    buf[ibuf++] = ' ';
		    for (ichr = 0; 
		    	 (argv[iarg][ichr] != '\0') && (ibuf < MAXBUF); 
		    	 ichr++, ibuf++)
		    	buf[ibuf] = argv[iarg][ichr];
		}
		buf[ibuf] = '\0';
	}

	/*
	 *	Execute the command.
	 */
	if( PCexec_suid(&buf) != OK )
	    PCexit(FAIL);

	PCexit(OK);
}

