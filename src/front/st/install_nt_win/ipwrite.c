/*
**  Copyright (c) 1995 Ingres Corporation
**
**  Name: ipwrite  - Echo a line without a carriage return.
**
**  Usage: 
**	ipwrite [text_string] 
**
**  Description:
**	Echo a line without a carriage return.
**
**  Input:
**      Command line - The line to write.
**
**  Output:
**      Command line - The line to write without a carriage return.
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
}
