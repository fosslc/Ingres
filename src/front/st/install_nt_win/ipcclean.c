/*
** Copyright (c) 2004 Ingres Corporation
**
*/

/*
**  Name: ipcclean.c
**
**  Description:
**	Check that the user has SERVER_CONTROL privileges and if so,
**	remove the shared memory files.
**
**
NEEDLIBS = GCFLIB
**
**  History:
**      18-jul-95 (emmag)
**          Created.
**	05-sep-95 (emmag)
**	    Removed copyright banner.
**	11-oct-95 (tutto01)
**	    Declared buffer to pass to idname.  Fixes crash on Alpha.
**	20-mar-97 (mcgem01)
**	    Generecise the code for Jasmine.
**	05-feb-1999 (somsa01)
**	    Save off ii_system before the next call to NMgtAt (which is
**	    inside IDname()). The next call could screw up the memory
**	    location of ii_system.
**	09-aug-1999 (mcgem01)
**	    Changed nat to i4.
**      21-Apr-2004 (rigka01) bug#112179, INGINS235
**          Do not delete the shared memory directory after deleting
**          each of the shared memory files 
**      31-aug-2004 (sheco02)
**          X-integrate change 468063 to main.
**	08-oct-2004 (somsa01)
**	    Due to the fix for bug 112179 (rigka01), if the shared memory
**	    does not exist it will not create it. This now happens.
**	25-Jul-2007 (drivi01)
**	    This program will only run for the user with elevated 
**	    privileges, it will exit for users with restricted token.
*/

# include <stdio.h>
# include <compat.h>
# include <ep.h>
# include <er.h>
# include <id.h>
# include <lo.h>
# include <me.h>
# include <nm.h>
# include <pm.h>
# include <si.h>
# include <st.h>
# include <gl.h>
# include <sl.h>
# include <iicommon.h>
# include <gca.h>

VOID
main (int argc, char *argv[])
{
	LOCATION loc;
	char *   ii_system;
	char *   user;
	char	 user_buf[256];
	char *	 host;
	char	 temp_buf [MAX_LOC + 1];
	i4	 len = 0;
	STATUS status=0;
	LO_DIR_CONTEXT lc;
	char 	*locstring;

	/*
	** Check if this is Vista, and if user has sufficient privileges
	** to execute.
	*/
	ELEVATION_REQUIRED();	

        NMgtAt( ERx( SystemLocationVariable ), &ii_system );
        if( ii_system == NULL || *ii_system == EOS )
	{
	    SIprintf( "%s has not been set in your environment.\n", 
				SystemLocationVariable );
	    PCexit (FAIL);
	}

	ii_system = STalloc (ii_system);
	STcopy(ERx ( "" ), temp_buf);
	STcopy(ii_system, temp_buf);

        /*
        **  Make sure user has SERVER_CONTROL privilege before allowing
	**  them to remove shared memory files.
        */

	user = user_buf;
        IDname(&user);

	/*
	** Initialize PM functions.
	*/

	(void) PMinit();
        switch(PMload((LOCATION *) NULL, (PM_ERR_FUNC *) NULL))
        {
            case OK:
                 /* loaded successfully */
                 break;

            case PM_FILE_BAD:
                 SIprintf("IPCCLEAN: Syntax error detected.\n");
                 PCexit(FAIL);

            default:
                 SIprintf("IPCCLEAN: Unable to open data file.\n");
                 PCexit(FAIL);
        }

        PMsetDefault(0, SystemCfgPrefix);
        host = PMhost();
        PMsetDefault(1, host);
        PMsetDefault(2, ERx("dbms"));
        PMsetDefault(3, ERx("*"));


        if (gca_chk_priv(user, GCA_PRIV_SERVER_CONTROL) != OK)
        {
           SIprintf("User does not have the SERVER_CONTROL privilege.\n");
           PCexit(FAIL);
        }

	/*
	** If II_SYSTEM has a trailing \ we need to remove it
	** before appending the shared memory location to it.
	*/

	len = strlen (temp_buf);
	if (temp_buf [len - 1] == '\\')
	    temp_buf [len - 1] = '\0';
	STcat (temp_buf,  "\\");
	STcat (temp_buf,  SystemLocationSubdirectory);
	STcat (temp_buf,  "\\files\\memory");

	LOfroms (PATH, temp_buf, &loc);



	/*
	** Remove the shared memory location, if it exists and recreate
        ** it.
	*/

	if (LOexist (&loc) == OK)
	{
	        /* Delete the shared memory files */
 		if (LOwcard (&loc, NULL, NULL, NULL, &lc) != OK)
		{
		  /* the directory is probably empty so abort the search*/
		  _VOID_ LOwend(&lc); 
		  SIprintf ("\nShared memory files do not exist.\n");
		  PCexit (OK);
		}
		else
		{
		  if (LOdelete(&loc) != OK)
	          {
		    LOtos(&loc,&locstring); 
		    SIprintf ("Unable to remove shared memory file:\n");
		    SIprintf ("\t\t%s\n", locstring);
		    SIprintf ("The server may still be running.\n");
		    PCexit (FAIL);
		  }
		}
	  	while ((status = LOwnext(&lc, &loc)) == OK) 
	        {
		  if (LOdelete(&loc) != OK)
		  {
		    LOtos(&loc,&locstring); 
		    SIprintf ("Unable to remove shared memory file:\n");
		    SIprintf ("\t\t%s\n", locstring);
		    SIprintf ("The server may still be running.\n");
		    PCexit (FAIL);
		  }
	        }
		if (status == ENDFILE)
		{
		    /* terminate the search since EOF was reached */
		    _VOID_ LOwend(&lc); 
		    SIprintf ("\nShared memory files successfully removed.\n");
		}
		else
		{
		    SIprintf ("The server may still be running.\n");
		    PCexit (FAIL);
		}
	}
	else if (LOcreate (&loc) != OK)
	{
	    SIprintf ("Unable to create shared memory location:\n\t\t%s\n",
		      loc.string);
	    PCexit (FAIL);
	}

	PCexit (OK);
}
