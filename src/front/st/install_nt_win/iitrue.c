/*
**  Copyright (c) 2000 Ingres Corporation
**
**  Name: iitrue  - Simple program to return true to emulate the
**	 	    UNIX command "true"
**
**  Usage: 
**	iitrue 
**
**  Description:
**	Mimick the UNIX true command.
**
**  Input:
**      None
**
**  Output:
**      None
** 
**  Returns:
** 	1
**
**  History:
**	03-May-2000 (cucjo01)
**		Created.  
** 
*/

# include <compat.h>
# include <pc.h>


void
main(int argc, char *argv[])
{
	PCexit(OK);
}
