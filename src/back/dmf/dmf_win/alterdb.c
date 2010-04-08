/*
**  Copyright (c) 1995 Ingres Corporation
**
**  Name: alterdb.c -- alter database state or characteristics
**
**  Usage:
** 	alterdb [-disable_journaling] [-delete_oldest_ckp]
**         [-init_jnl_blocks=nnn] [-jnl_block_size=nnn] [-target_jnl_blocks=nnn]
**         dbname 
**
**  Description:
**	This program provides the INGRES `alterdb' command by executing 
** 	the journaling support program (dmfjsp), passing along the
**	command line arguments.
** 
**  Exit Status:
**	OK	alterdb (dmfjsp) succeeded.
**	FAIL	alterdb (dmfjsp) failed.	
**
**  History:
**	17-jul-1995 (wonst02)
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
	STprintf(buf, ERx( "dmfjsp alterdb" ));

	switch(argc)
	{
	    case 1:
		break;

		/*
		 *	Append any args from the command line, e.g., 
		 *		dmfjsp alterdb dbname -jnl_block_size=4096
		 */
	    default:
		ibuf = sizeof(ERx("dmfjsp alterdb")) - 1;
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

