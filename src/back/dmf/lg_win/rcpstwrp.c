/*
**  Copyright (c) 1998 Ingres Corporation
**
**  Name: rcpstwrp.c   - The wrapper to rcpstat
**
**  Description:
**	This is the wrapper to the Ingres "rcpstat" utility.
** 
**  Exit Status:
**	OK	rcpstat succeeded.
**	FAIL	rcpstat failed.	
**
**  History:
**	12-jan-1998 (somsa01)
**	    Created.
** 
*/

/*
PROGRAM = rcpstat 
**
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
    STcopy("ntrcpst", bufptr);
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
