/*
**Copyright (c) 2004 Ingres Corporation
*/
/* static char Sccsid[]= "@(#)IDname.c	3.1	9/30/83";	*/

# include	<compat.h>
# include	<gl.h>
# include       <clconfig.h>
# include	<systypes.h>
# include	<pwd.h>
# include       <diracc.h>
# include	<handy.h>
# include	<dl.h>
# include	<st.h>
# include	<stdio.h>
# include       <er.h>
# include	<pm.h>

/*
	IDname
		set the passed in argument 'name' to contain the name
			of the user who is executing this process.
**
** History:
**	17-may-90 (blaise)
**	    Integrated changes from ingresug:
**		Make sure the passwd pointer was set.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	12-mar-1997 (canor01)
**	    Change literal "ingres" to SystemAdminUser for portability.
**	16-jun-1997 (canor01)
**	    Use thread-safe version of getpwuid() if available.
**	16-aug-1997 (popri01)
**	    Add stdio hdr. It's included via the system pwd hdr for some
**	    Unixes, but not for all (e.g., Unixware). Also included the
**	    Ingres st hdr, since we're now using STcopy.
**	02-sep-1997 (mosjo01)
**	    Add include <dl.h> to resolve BUFSIZ.
**	30-aug-2000 (hanch04)
**	    Added logic for embed_installation.
**	06-Sep-2000 (hanch04)
**	    Changed embed_installation to embed_user
**	13-Apr-2005 (hanje04)
**	    BUG 112987 
**	    Add IDename, which returns the effective user name of the running
**	    process.
*/

/*
** This is the value of embed_installation from config.dat .
*/
static i4 embed_installation = -1;
static i4 recursive_check = -1;

VOID
IDname(name)
char	**name;
{
    struct	passwd	*p_pswd, pwd;
    char		pwuid_buf[BUFSIZ];
    int			size = BUFSIZ;
    char                *host;
    char                *value = NULL;
    char                config_string[256];
    static bool         TNGInstallation;

    if (embed_installation == -1)
    {
        char    config_string[256];
        char    *value, *host;

        embed_installation = 0;
	recursive_check = 1;
        /*
        ** Get the host name, formally used iipmhost.
        */
        host = PMhost();

        /*
        ** Build the string we will search for in the config.dat file.
        */
        STprintf( config_string,
                  ERx("%s.%s.setup.embed_user"),
                  SystemCfgPrefix, host );

        if( (PMinit() == OK) &&
            (PMload( NULL, (PM_ERR_FUNC *)NULL ) == OK) )
        {
            if( PMget( config_string, &value ) == OK )
                if ((value[0] != '\0') &&
                    (STbcompare(value,0,"on",0,TRUE) == 0) )
                    embed_installation = 1;
        }
	recursive_check = 0;
    }

   if (recursive_check || embed_installation)
    {
        STcopy("ingres", *name);
    }
    else
    {
	p_pswd = iiCLgetpwuid(getuid(), &pwd, pwuid_buf, size);
	if( p_pswd == NULL )
	    STcopy(SystemAdminUser, *name );
	else
	    STcopy(p_pswd->pw_name, *name);
    }
}

/*
** Name:
**	IDename
**
** Description:
**	Sets argument 'name' to the user name of the euid of the running 
**	process.
**
** Inputs:
**	None
**
** Output:
**	name  - Effective user name of running process
**
** History:
**	13-Apr-2005 (hanje04)
**	    Created.
*/

VOID
IDename(name)
char **name;
{
    struct	passwd	*p_pswd, pwd;
    char		pwuid_buf[BUFSIZ];
    int			size = BUFSIZ;

    /* Use iiCLgetpwuid to obtain username info from euid */
    p_pswd = iiCLgetpwuid(geteuid(), &pwd, pwuid_buf, size);
    if( p_pswd == NULL )
	STcopy(SystemAdminUser, *name );
    else
	STcopy(p_pswd->pw_name, *name);

}
