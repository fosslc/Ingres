/*
**  Copyright (c) 1998 Ingres Corporation
**
**  Name: lckstwrp.c   - The wrapper to lockstat
**
**  Description:
**	This is the wrapper to the Ingres "lockstat" utility.
** 
**  Exit Status:
**	OK	lockstat succeeded.
**	FAIL	lockstat failed.	
**
**  History:
**	12-jan-1998 (somsa01)
**	    Created.
** 
*/
/*
**
PROGRAM = lockstat
*/
# include <compat.h>
# include <pc.h>
# include <st.h>


void
main(int argc, char *argv[])
{
    char	buf[256];
    char	*bufptr = (char *)buf;
    int		iarg;

    /*
    ** Put the program name and its parameters in the buffer.
    */
    STcopy("ntlockst", bufptr);
    for (iarg = 1; iarg<argc; iarg++)
    {
	STcat(bufptr, " ");
	STcat(bufptr, argv[iarg]);
    }

    /*
    ** Execute the command.
    */
    if( PCexec_suid(&buf) != OK )
	PCexit(FAIL);

    PCexit(OK);
}
