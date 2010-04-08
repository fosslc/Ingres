/*
**  Copyright (c) 1995 Ingres Corporation
**
**  Name: auditdb - database audit trail
**
**  Usage: 
**    	auditdb [-a] [#c] [-bdd-mmmm-yyyy[:hh:mm:ss]] [-edd-mmmm-yyyy[:hh:mm:ss]]
**		[-iusername] [-inconsistent] [-file[=file1,file2,...]]
**		[-table=tbl1,tbl2,...] [-uusername] [-wait] dbname
**
**  Description:
**	This program provides the INGRES `auditdb' command by
**	executing the journaling support program, passing along the
**	command line arguments.
** 
**  Exit Status:
**	OK	auditdb (dmfjsp) succeeded.
**	FAIL	auditdb (dmfjsp) failed.	
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
	STprintf(buf, ERx( "dmfjsp auditdb" ));

	switch(argc)
	{
	    case 1:
		break;

		/*
		 *	Append any args from the command line, e.g., 
		 *		dmfjsp auditdb dbname -table=tablename
		 */
	    default:
		ibuf = sizeof(ERx("dmfjsp auditdb")) - 1;
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

