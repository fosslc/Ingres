/*
**  Copyright (c) 2001 Ingres Corporation
**
**  Name: rcpcfgwrp.c	- The wrapper to rcpconfig
**
**  Description:
**	This is the wrapper to the Ingres "rcpconfig" utility.
** 
**  Exit Status:
**	OK	rcpconfig succeeded.
**	FAIL	rcpconfig failed.	
**
**  History:
**	29-jan-2001 (somsa01)
**	    Created.
**	22-Jun-2007 (drivi01)
**	    On Vista, this binary should only run in elevated mode.
**	    If user attempts to launch this binary without elevated
**	    privileges, this change will ensure they get an error
**	    explaining the elevation requriement.
** 
*/
/*
**
PROGRAM = rcpconfig 

NEEDLIBS = COMPATLIB GCFLIB
*/

# include <compat.h>
# include <gl.h>
# include <iicommon.h>
# include <er.h>
# include <id.h>
# include <pc.h>
# include <pm.h>
# include <si.h>
# include <st.h>
# include <gca.h>
# include <ep.h>


void
main(int argc, char *argv[])
{
    char	buf[256];
    char	*bufptr = (char *)buf;
    int		iarg;
    char	userbuf[GL_MAXNAME];
    char	*host, *user = userbuf;

    /*
    ** Put the program name and its parameters in the buffer.
    */
    STcopy("ntrcpcfg", bufptr);
    for (iarg = 1; iarg<argc; iarg++)
    {
	STcat(bufptr, " ");
	STcat(bufptr, argv[iarg]);
    }

    /*
    **  Make sure the user has the SERVER_CONTROL privilege.
    */
    PMinit();
    switch(PMload((LOCATION *) NULL, (PM_ERR_FUNC *) NULL))
    {
	case OK:
	    /* loaded successfully */
	    break;

	case PM_FILE_BAD:
	    SIprintf("RCPCONFIG: Syntax error detected.\n");
	    break;

	default:
	    SIprintf("RCPCONFIG: Unable to open data file.\n");
	    PCexit(FAIL);
    }

    PMsetDefault(0, "ii");
    host = PMhost();
    PMsetDefault(1, host);
    PMsetDefault(2, ERx("dbms"));
    PMsetDefault(3, ERx("*"));

    IDname(&user);
    if (gca_chk_priv(user, GCA_PRIV_SERVER_CONTROL) != OK)
    {
	SIprintf(
	    "RCPCONFIG: User does not have the SERVER_CONTROL privilege.\n");
	PCexit(FAIL);
    }
    /*
    ** Check if this is Vista, and if user has sufficient privileges
    ** to execute.
    */
    ELEVATION_REQUIRED();

    /*
    ** Execute the command.
    */
    if( PCexec_suid(buf) != OK )
	PCexit(FAIL);

    PCexit(OK);
}
