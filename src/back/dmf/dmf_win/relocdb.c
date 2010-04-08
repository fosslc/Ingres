/*
**  Copyright (c) 1995 Ingres Corporation
**
**  Name: relocatedb    - Move journal,dump,checkpoint or default work location
**                        Make a duplicate copy of a database
**  Usage:
**	relocatedb [-new_ckp_location= | -new_dump_location= |
**		    -new_work_location= | -new_jnl_location= |
**		    -new_database= [-location= -new_location= ]]
**		    [+w -w] dbname
**
**  Description:
**	This program provides the INGRES `relocatedb' command by
**	executing the journaling support program, passing along the
**	command line arguments.
** 
**  Exit Status:
**	OK	relocatedb (dmfjsp) succeeded.
**	FAIL	relocatedb (dmfjsp) failed.	
**
**  History:
**	18-jul-1995 (wonst02)
**		Created.  (based upon ingstart.c)
**	12-jan-1998 (somsa01)
**		Run dmfjsp via PCexec_setuid(). (Bug #87751)
** 
*/

/*
PROGRAM = relocatedb 
**
*/

# include <compat.h>
# include <er.h>
# include <pc.h>
# include <st.h>


void
main(int argc, char *argv[])
{
#define 	MAXBUF	4095

	char 	buf[ MAXBUF+1 ];
	int	iarg, ibuf, ichr;

	/*
	 *	Put the program name and first parameter in the buffer.
	 */
	STprintf(buf, ERx( "dmfjsp relocdb" ));

	switch(argc)
	{
	    case 1:
		break;

		/*
		 *	Append any args from the command line, e.g., 
		 *		dmfjsp relocdb -new_ckp_location dbname
		 */
	    default:
		ibuf = sizeof(ERx("dmfjsp relocdb")) - 1;
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

