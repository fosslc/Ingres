/*
**  Copyright (c) 1995 Ingres Corporation
**
**  Name: infodb  - Show information about the database from the config file
**
**  Usage: 
**	infodb [-uusername] {dbname}
**
**  Description:
**	Show information stored in the database.  Information printed includes:
**		locations, checkpoints, journal, database status, dump
**		information, ...
**
**	This program provides the INGRES `infodb' command by
**	executing the journaling support program, passing along the
**	command line arguments.
** 
**  Exit Status:
**	OK	infodb (dmfjsp) succeeded.
**	FAIL	infodb (dmfjsp) failed.	
**
**  History:
**	18-jul-1995 (wonst02)
**		Created.  (based upon ingstart.c)
**	12-jan-1998 (somsa01)
**		Run dmfjsp via PCexec_suid(). (Bug #87751)
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
	STprintf(buf, ERx( "dmfjsp infodb" ));

	switch(argc)
	{
	    case 1:
		break;

		/*
		 *	Append any args from the command line, e.g., 
		 *		dmfjsp infodb -u$ingres dbname
		 */
	    default:
		ibuf = sizeof(ERx("dmfjsp infodb")) - 1;
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

