/*
**  Copyright (c) 1995 Ingres Corporation
**
**  Name: ipsetp  - Construct a string that contains a set environment 
**	 	    variable command
**
**  Usage: 
**	ipsetp [variable_name] 
**
**  Description:
** 	Constructs a MS-DOS set environment variable command from a command
**	line argument and standard input.
**
**  Input:
**      Command line - the MS-DOS enviroment variable name
**	Standard input - the string that the MS-DOS enviroment variable name
**		should be set to.
**  Output:
**      Standard output - the complete set environment variable command 
**		or a echo command with a usage message.
** 
**  History:
**	18-jul-1995 (reijo01)
**		Created.  
**	28-Sep-2007 (drivi01)
**		Increase MAXBUF to 8192 since the maximum individual 
**		environment variable size can be 8192 bytes and 4095
**		just isn't enough.
** 
*/

# include <compat.h>
# include <er.h>
# include <si.h>
# include <st.h>

#define 	MAXBUF	8192


void
main(int argc, char *argv[])
{

	char 	buf[ MAXBUF+1 ];
	int	ibuf, ichr;

	/*
	**	Set the set command.
	*/
	STprintf(buf, ERx( "@set " ));

	switch(argc)
	{
		/*
		**	Construct the rest of the command from the 
		** 	command line argument and standard input.
		*/ 
	    case 2:
		ibuf = sizeof(ERx("@set ")) - 1;
		for ( ichr = 0 ;(ibuf < MAXBUF) && argv[1][ichr]; )
		{
			buf[ibuf++] = argv[1][ichr++];
		}	
		buf[ibuf++] = '=';
		/*
		** Set EOS in case there is nothing from standard input.
		*/
		buf[ibuf] = '\0';
		SIgetrec(&buf[ibuf], MAXBUF-ibuf, stdin);
		break;

	    default:
	    	SIprintf(stderr,"echo Usage: ipsetp variable_name"); 
		break;
	}

	/*
	**	Print out the set command.
	*/
	SIprintf("%s\n",buf);
}
