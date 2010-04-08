/*
**  Copyright (c) 1995 Ingres Corporation
**
**  Name: ipchoice  - Construct a string that contains a set environment 
**	 	    variable command
**
**  Usage: 
**	ipchoice [text_string] 
**
**  Description:
**	Mimick the MS-DOS choice command.
**
**  Input:
**      Command line - The choice string.
**
**  Output:
**      Command line - The choice string.
** 
**  Returns:
** 	1 if Yes
**	2 if No
**
**  History:
**	18-jul-1995 (reijo01)
**		Created.  
**	09-aug-1999 (mcgem01)
**		Changed nat to i4.
** 
*/

# include <compat.h>
# include <er.h>
# include <si.h>
# include <st.h>

#define 	MAXBUF	4095


void
main(int argc, char *argv[])
{

	char 	buf[ MAXBUF+1 ];
	i4 	c;
	
	
	if (argc == 1)
		PCexit(FAIL);
	for(;argc > 1 ; --argc)
		SIfprintf(stdout,"%s ",*++argv);
	SIfprintf(stdout,"[Y,N]? ");
	c = SIgetc(stdin);
	if (c == 'Y' || c == 'y')
		PCexit(1);
	else
		PCexit(2);
}
