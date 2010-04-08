/*
**  Copyright (c) 2004 Ingres Corporation
**
**  Name: iiconcat.c -- utility for generating a concatenated string given
**	  several strings.
**
**  Description:
**
**	Usage: iiconcat -s<string1> -s<string2> -s<string3>... <out file>
**
**  History:
**	15-mar-1999 (somsa01)
**          Created.
**	21-apr-2000 (somsa01)
**	    Updated MING hint for program name to account for different
**	    products.
**      17-Nov-2000 (fanra01)
**          Bug 103277
**          Updated to use NMgtAt to include environment variables.
*/

# include <compat.h>
# include <er.h>
# include <lo.h>
# include <nm.h>
# include <pc.h>
# include <si.h>
# include <st.h>

/*
PROGRAM = 	(PROG1PRFX)concat
**
NEEDLIBS = 	COMPATLIB
**
UNDEFS =	II_copyright
**
DEST   =	utility
*/

STATUS
main( argc, argv )
int argc;
char *argv[];
{
    int		i, last_index;
    LOCATION	file_location;
    char	loc_string[MAX_LOC + 1], *p, *value, tmpchr[64];
    FILE	*pStdio=NULL, *pTmpFile=NULL;

    if (argc < 2)
    {
	SIfprintf(stdout, "Usage: iiconcat -s<string1> -s<string2>...");
	SIfprintf(stdout, " <out file>\n");
	PCexit(FAIL);
    }

    /*
    ** Get to the last command line argument without the "-s" flag.
    */
    i = 0;
    while ((++i < argc) && (!STbcompare(argv[i], 2, "-s", 2, FALSE)));

    if (i < argc)
    {
	/*
	** Standard out is to a file.
	*/

	STlcopy(argv[i], loc_string, sizeof(loc_string));
	LOfroms(PATH & FILENAME, loc_string, &file_location);

	if (SIfopen(&file_location, ERx("w"), SI_TXT, SI_MAX_TXT_REC,
		    &pTmpFile) != OK)
        {
            SIfprintf(stderr, "Unable to open file %s\n",argv[i]);
            PCexit(FAIL);
        }
        pStdio = pTmpFile;
	last_index = i;
    }
    else
    {
	/*
	** Normal standard out.
	*/

        pStdio = stdout;
	last_index = argc;
    }

    /*
    ** Cycle through all the "-s" strings and print them out as one
    ** continuous string.
    */
    for (i=1; i < last_index; i++)
    {
	if ((p = STrstrindex(argv[i], "(", 0, FALSE)) != NULL)
	{
	    /*
	    ** We have to unalias an environment variable.
	    */

	    p++;
	    STlcopy(p, tmpchr, STlength(p)-1);
	    NMgtAt(tmpchr, &value);
	    SIfprintf(pStdio, "%s", value);
	}
	else
	{
	    /*
	    ** Just print it as is.
	    */
	    p = &argv[i][2];
	    SIfprintf(pStdio, "%s", p);
	}
    }

    SIfprintf(pStdio, "\n");

    if (pTmpFile != NULL)
    {
        SIclose(pTmpFile);
    }

    PCexit(OK);
}
