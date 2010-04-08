/*
**  Copyright (c) 1995 Ingres Corporation
**
**  Name: ipreadp  - Read a line from standard input and write to standard 
**			output.
**
**  Usage: 
**	ipreadp 
**
**  Description:
** 	Reads a line from standard input and writes to standard output.
**
**  Input: from standard input.
**
**  Output: outputs whatever was received from standard input to standard output
** 
**  History:
**	18-jul-1995 (reijo01)
**		Created.  
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
	
	SIflush(stdin);
	gets(buf);
	/*	SIgetrec(buf, MAXBUF, stdin); */
	SIputrec(buf, stdout);

}
