/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<cv.h>
# include	<pc.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<uigdata.h>
# include	<si.h> 
# include	<lo.h> 
# include	<me.h>
# include	<st.h>
# include	<er.h>
# include	<ui.h>
# include	<ug.h>
# include	<erde.h>

/*
** Name:	deleter.c	Main program for Deleter
**
** Description:
**	DELETER is a program which can be used to delete front
**	end objects (forms, reports, applications, graphs, and
**	joindefs) from a database, without using the CATALOGS
**	menu items of each of the front ends.  This is very
**	useful for deleting objects whether they exist or
**	not.
**
** Inputs:
**	The calling sequence is:
**
**		deleter [-s] [-uuser] [-c] database {-tobjectname}
**
**		where	database = name of database.
**			-s = no status messages.
**			-uuser = alternate username to use.
**			-c = if this is the dba, delete these objects for
**			     all database users.
**			-tobjectname is the prefix for the object
**				type followed by the object name
**				to delete (which can include
**				wildcard characters.)  Valid formats
**				are:
**
**				-fformname - forms (including QBFnames)
**				-rreportname - reports.
**				-ggraphname - graphs (now VIGRAPH).
**				-aapplicationname - applications, including
**					object directories.
**				-jjoindefname - joindefs.
**				-qqbfname - delete a qbfname
**
** Outputs:
**	Exits with appropriate error messages.
**
** History:
**	13-mar-1985 (peter)
**		Written.
**	29-jul-1986 (mgw)
**		Changed abDbname to dabDbname to facilitate 4.0/04 the
**		4.0/04 build. abDbname became defined in frame.exe and
**		so was multiply defined in dlabfdir.c
**	18-nov-1987 (peter)
**		Changes for 6.0 to use object tables.
**	28-jan-1988 (peter)
**		Fix bug 1762.   Username flag was not being set correctly.
**      05-mar-1990 (mgw)
**              Changed #include <erde.h> to #include "erde.h" since this is
**              a local file and some platforms need to make the destinction.
**	28-aug-1990 (Joe)
**	    Changed references to IIUIgdata to the UIDBDATA structure
**	    returned from IIUIdbdata().
**      8-oct-1990 (stevet)
**          Added IIUGinit call to initialize character set attribute table.
**	21-mar-1991 (bobm)
**	    Integrate desktop changes.
**	17-aug-91 (leighb) DeskTop Porting Change: main() must return a nat.
**      16-nov-1992 (larrym)
**          Added CUFLIB to NEEDLIBS.  CUF is the new common utility facility.
**      06-jan-1992 (rdrane)
**              Changed #include "erde.h" back to #include <erde.h> since it's
**              no longer a local file (shared with the front/misc/deleter
**		production DELETER).
**      10-jun-97 (cohmi01)  Add dummy calls to force inclusion of libruntime.
**	18-Dec-97 (gordy)
**	    Added SQLCALIB to NEEDLIBS, now required by LIBQ.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-Sep-2004 (drivi01)
**	    Updated NEEDLIBS to link dynamic library SHQLIB and SHFRAMELIB
**	    to replace its static libraries. 
*/

/*
**	Externals 
*/

/*
**	MKMFIN Hints
**
PROGRAM =	deleter

NEEDLIBS =	DELETERLIB  COMPATLIB SHEMBEDLIB SHFRAMELIB \
		SHQLIB

UNDEFS =	II_copyright 
*/

GLOBALDEF bool IIabVision = FALSE;      /* Resolve references in IAOM */

i4						 
main(argc,argv)
i4	argc;		/* number of parameters */
char	*argv[];	/* addresses of parameter strings */
{
	/* external declarations */

	char	dbname[FE_MAXNAME+1];	/* Database name */
	char	dbdirn[FE_MAXNAME+1];	/* Database directory name */
	bool	silent;			/* TRUE to stay silent (-s set) */
	bool	allusers;		/* TRUE if -c flag set */
	char	userflag[FE_MAXNAME+3];	/* User flag specified on cmd line */
	char	username[FE_MAXNAME+1];		/* user name */
	char	dbaname[FE_MAXNAME+1];		/* dba name */
	char	*c, *c2;			/* ptr to char buffer */
	i4	numobjects;		/* count of number of object
					** flags found on command line */
	char	obj_type[2];		/* Object type code (rfjag) */
	char	obj_name[FE_MAXNAME+1];	/* Object name */
	bool	errfound;		/* TRUE if error found */
	char	*linebuf;		/* used for prompt values */
	i2	i;			/* Used as a counter */

	MEadvise(ME_INGRES_ALLOC);

	/* Call IIUGinit to initialize character set attribute table */
	if ( IIUGinit() != OK)
	{
	        PCexit(FAIL);
	}
	    

	FEcopyright(ERx("DELETER"), ERx("1985"));

	linebuf = MEreqmem((u_i4)0,(u_i4)(DB_MAXSTRING+1),FALSE,(STATUS *)NULL);

	/* initialize some variables */

	silent = FALSE;
	_VOID_ STcopy(ERx(""), userflag);
	allusers = FALSE;
	errfound = FALSE;
	_VOID_ STcopy(ERx(""), dbname);
	numobjects = 0;
	
	/* First process the command line. */

	/* first pull out any flags, and count the number of
	** object flags for later processing */

	for(i=1; (i<argc); i++)
	{	/* next parameter */
		if (*(argv[i]) == '-')
		{	/* flag found. Set and clear out */
			switch(*(argv[i]+1))
			{
				case('c'):
				case('C'):
					allusers = TRUE;
					argv[i] = NULL;
					break;

				case('s'):
				case('S'):
					silent = TRUE;
					argv[i] = NULL;
					break;

				case('u'):
				case('U'):
					_VOID_ STcopy(argv[i], userflag);
					argv[i] = NULL;
					break;

				case('r'):
				case('f'):
				case('j'):
				case('a'):
				case('g'):
				case('q'):
					/* Object specified.  Count now,
					** process later but leave in argv*/

					numobjects++;
					break;

				default:
					IIUGerr(E_DE0003_BadFlag, UG_ERR_ERROR,
						(i4) 1, (PTR) argv[i]);
					PCexit(-1);
			}
		}
		else
		{
			/* Must be a database name */
			_VOID_ STcopy(argv[i],dbname);
			argv[i] = NULL;
		}
	}

	/* now check to see that the database name was specified.
	**  If not, ask for one with a prompt.
	*/
	if (STlength(dbname)<1)
	{	/* no database name.  Prompt for one. */
		FEprompt(ERget(F_DE0001_Database),TRUE,DB_MAXNAME,dbname);
		CVlower(dbname);
	}

	/* Open up the database, and get the correct username
	** to use for processing */

	if (FEingres(dbname, userflag, NULL) != OK)
	{
		IIUGerr(E_DE0004_NoOpen, UG_ERR_ERROR, 0);
		PCexit(-1);
	}

	/* Create the database directory name from the dbname */

	for (c = c2 = dbname; *c; ++c)
	{
		if (*c == ':')
			c2 = c + 1;
	}
	_VOID_ STcopy(c2, dbdirn);
	_VOID_ CVupper(dbdirn);

	/* Now get the values to be used for username and
	** dbaname.  Only DBA's may use the -c flag */

	_VOID_ STcopy(IIUIdbdata()->user, username);
	_VOID_ STcopy(IIUIdbdata()->dba, dbaname);

	if (allusers)
	{	/* -c specified.  Set asterisk for username, if legal */
		if (STcompare(dbaname,username) == 0)
		{	/* This is the DBA.  Legal. */
			_VOID_ STcopy(ERx("*"),username);
		}
		else
		{	/* Not the DBA.  Illegal. */
			IIUGerr(E_DE0002_NotDBA, UG_ERR_ERROR, 0);
			PCexit(-1);
		}
	}

	iiuicw1_CatWriteOn();		/* Enable frontend cats */

	/* Any parameters left in the list must be objects.
	** For each object in the list, process the request.
	** If there are no objects in the list, prompt for
	** them */

	if (numobjects > 0)
	{	/* Command line arguments */
		for (i=1; (i<argc); i++)
		{	/* get next object name from command line */
			if (argv[i]==NULL)
			{
				continue;
			}
			obj_type[0] = *(argv[i]+1);	/* Get object code */
			obj_type[1] = '\0';		/* End the string */
			_VOID_ STcopy(argv[i]+2,obj_name);	/* Copy the object name */
			if (dl_delobj(obj_type,obj_name,username,dbdirn,silent)
					 == FAIL)
			{
				errfound = TRUE;
			}
		}
	}
	else
	{	/* No command line arguments prompt until return */
		for (;;)
		{
			FEprompt(ERget(F_DE0002_Object), FALSE, DB_MAXSTRING,
				linebuf);
			c = linebuf;
			if (STlength(c) <= 0)
			{
				break;
			}
			CVlower(c);
			if (*c == '-')
			{	/* Skip over a leading (optional) dash */
				c++;
			}

			switch (*c)
			{	/* Make sure that it is legal */
				case('f'):
				case('r'):
				case('g'):
				case('j'):
				case('a'):
				case('q'):
					obj_type[0] = *c;
					obj_type[1] = '\0';
					_VOID_ STcopy(++c,obj_name);
					if (dl_delobj(obj_type,obj_name,username,dbdirn,silent) == FAIL)
					{
						errfound = TRUE;
					}
					break;

				default:
					IIUGerr(E_DE0005_BadType, UG_ERR_ERROR,
						(i4) 1, (PTR) c);
					errfound = TRUE;
					break;
			}
		}
	}

	iiuicw0_CatWriteOff();		/* Disable frontend cats */

	FEing_exit();

	PCexit(0);
}

/* this function should not be called. Its purpose is to   */
/* force some linkers that cant handle circular references */
/* between libraries to include these functions so they    */
/* won't show up as undefined                              */
dummy1_to_force_inclusion()
{
    IItscroll();
    IItbsmode();
}

