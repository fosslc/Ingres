/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<pc.h>
#include	<si.h>
#include	<st.h>
#include	<lo.h>
#include	<si.h>
#include	<ex.h>
#include	<me.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ug.h>
#include	<ooclass.h>
#include	<erm1.h>

#include	<frmindex.h>

/**
** Name:	formindx.c -	Form Index File Edit Program.
**
** Description.
**	Contains the FormIndex command main line.  This program creates and edits
**	the Form Index file that contains all forms used by RT front-ends.  This
**	provides an easily means of configuring releases for different languages.
**
**	A Form Index file consist of an index and the encoded frame structures for
**	the forms.  The Form Index file module provides the methods to access this
**	file for both reading and writing.  The 'IIUFgtfForm()' routine uses this
**	module to load a form at runtime thereby facilitating internationalization
**	of Ingres front-ends.
**
**	Command line syntax:
**
**		formindex filename [-uuser] [-@database] [[-m] formname[.frm]]
**			[-d formname] [-a form[.frm]] [-l] ......
**
**		filename - The Form Index file to be created and/or edited.
**				This filename must have an extension of ".fnx", and this program
**				enforces this by automatically appending this extension to the
**				Form Index filename.
**
**	   Option:
**		[-m] formname[.frm] - Replaces the named form from the database in the
**					Form Index file.  (If it does not exist in the file, it is
**					appended.)  If the optional extension is present, assume
**					it is the name of a CopyForm file that must first be copied
**					into the database before the form can be replaced.
**
**		-d formname - Deletes the name form from the Form Index file.
**
**		-a formname[.frm] - Appends the named form from the database into the
**					Form Index file.  If the optional extension is present,
**					assume it is the name of a CopyForm file that must first be
**					copied into the database before the form can be appended.
**
**		-l - Print the index table information for the Form Index file.
**
** History:
**	Revision 6.0  88/04/01  wong
**	Modified to use Form Index file module, which now inserts
**	forms directly from the database.
**	10-sep-1987 (peter) Add message support.  Also, change semantics of -m flag.
**
**	Revision 5.0K  86/12/20  Kobayashi
**	Initial revision for 5.0 Kanji.
**
**	07/28/88 (dkh) - Put in special hook so that we call cf_rdobj()
**			 to copy a form into the database.  This will
**			 allow us to get the name of the form from
**			 copyform instead of relying on the file name.
**	02/10/89 (dkh) - Integrated change "ingres61b3ug 1050" from cmreview.
**	12/30/89 (dkh) - Removed declarations of Cfversion and Cf_silent
**			 since they are now in copyform!cfglobs.c.
**	04/16/91 (dkh) - Added support to adapt to different formindex
**			 file header sizes.
**	08-jun-92 (leighb) DeskTop Porting Change:
**		Changes 'errno' to 'errnum' cuz 'errno' is a macro in WIN/NT.
**	11/03/92 (dkh) - Changed to not use DB connection for formindex.
**      16-nov-1992 (larrym)
**          Added CUFLIB to NEEDLIBS.  CUF is the new common utility facility.
**	03/25/94 (dkh) - Added call to EXsetclient().
**      11-jan-1996 (toumi01; from 1.1 axp_osf port)
**          Added cast for arg in FEmsg().
**	13-jun-97 (hayke02)
**	    Add dummy calls to force inclusion of libruntime.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**    25-Oct-2005 (hanje04)
**        Add prototype for ArgError() to prevent compiler
**        errors with GCC 4.0 due to conflict with implicit declaration.
**    07-apr-2009 (stial01)
**        Call cf_editobj to fix DB_MAXNAME dependencies in the forms we 
**        add to rtiforms.fnx during build (jam)
**/

/*{
** Name:	main() -	FormIndex Command Main Line and Entry Point.
**
** Description:
**	Parses the command-line arguments calling the appropriate Form Index file
**	object methods to open, edit and write the Form Index file.
**
** Inputs:
**	argc	{nat}  Number of command line arguments.
**	argv	{char **}  Array of references to the arguments.
**
** History:
**	20-Dec-1986 (Kobayashi)
**		Create new for 5.0k
**	10-sep-1987 (peter)
**		Add message support.  Also, change semantics of -m flag
**		to delete only if it exists.  If it does not exist, add it anyway.
**	04/88 (jhw) -- Modified for Form Index file as class.
**      28-sep-1990 (stevet)
**              Add call to IIUGinit.
*/

/*
**	MKMFIN Hints
**
PROGRAM =	formindex

NEEDLIBS =	FORMINDEXLIB COPYFORMLIB OOLIB UFLIB RUNSYSLIB \
		RUNTIMELIB FDLIB FTLIB FEDSLIB \
		UILIB LIBQSYSLIB LIBQLIB UGLIB FMTLIB AFELIB \
		LIBQGCALIB CUFLIB GCFLIB ADFLIB COMPATLIB MALLOCLIB

UNDEFS =	II_copyright
*/

/* local prototypes */
static ArgError (
	STATUS	errnum,
	char	*arg);


#define FIX		ERx(".fnx")

i4
main(argc,argv)
i4	argc;
char	*argv[];
{
	char		*dbname;
	char		*uname;
	STATUS		status;
	EX_CONTEXT	context;
	LOCATION	loc;				/* Form Index file location */
	char		filenm[MAX_LOC+1];	/* Form Index file name */
	FORM_INDEX	*frmindx;
	char		tempform[MAX_LOC+1];	/* Temp Form file name */
        bool		editform=FALSE;
	char		*sp;
	char		*fobj;

	EX	FEhandler();

	/* Tell EX this is an ingres tool. */
	(void) EXsetclient(EX_INGRES_TOOL);

	MEadvise( ME_INGRES_ALLOC );

        /* Add call to IIUGinit to initialize character set table and others */

        if ( IIUGinit() != OK )
	{
                PCexit(FAIL);
        }

	/*
	**  Check if we need to edit maxname dependencies in the form
	*/
	NMgtAt(ERx("II_FORMINDEX_JAM"), &sp);
	if (sp != NULL && STbcompare(sp, 0, ERx("edit_maxname"),
		0, TRUE) == 0)
	{
	    editform=TRUE;
	}

	/* argument check */

	if ( --argc < 2 )
	{
		ArgError( OK, "" );
		PCexit(FAIL);
	}

	/* ready to open index file */

	STcopy( *++argv, filenm );

	if ( STindex(filenm, ERx("."), 0) == (char *)NULL )
	{
		STcat( filenm, FIX );
	}

	/* Set exception handler */
	if ( EXdeclare(FEhandler, &context) != OK )
	{
		EXdelete();
		PCexit(FAIL);
	}

	/* Open Form Index file */

	LOfroms( PATH&FILENAME, filenm, &loc );
	if ((frmindx = IIFDfiOpen( &loc, TRUE, FALSE)) == NULL)
		PCexit( FAIL );

	FEmsg( ERget(S_M10010_Version), FALSE, (PTR)INDEX_VERSION );

	status = OK;

	while ( --argc > 0 && status != FAIL )
	{ /* parse arguments */
		if ((++argv)[0][0] != '-')
		{
			fobj = argv[0];
			if (editform)
			{
			    STcopy("temp.frm", tempform);
			    status = cf_editobj(fobj, tempform);
			    fobj = tempform;
			}
			IIFDfixReplace( frmindx, fobj );
		}
		else
		{ /* flag */

			switch ( argv[0][1] )
			{
			  case 'a':
			  case 'A':
				fobj = (argv[0][2] == EOS) ? (--argc, *++argv) : &(argv[0][2]);
				if ( fobj == NULL || *fobj == '-' || *fobj == EOS )
					ArgError( NOARGVAL, ERx("-a") );
				else
				{
				    if (editform)
				    {
					STcopy("temp.frm", tempform);
					status = cf_editobj(fobj, tempform);
					fobj = tempform;
				    }
				    IIFDfiInsert( frmindx, fobj );
				}
				break;

			  case 'd':
			  case 'D':
				fobj = (argv[0][2] == EOS) ? (--argc, *++argv) : &(argv[0][2]);
				if ( fobj == NULL || *fobj == '-' || *fobj == EOS )
					ArgError( NOARGVAL, ERx("-d") );
				else
					IIFDfiDelete( frmindx, fobj );
				break;

		 	  case 'm':
		 	  case 'M':
				fobj = (argv[0][2] == EOS) ? (--argc, *++argv) : &(argv[0][2]);
				if ( fobj == NULL || *fobj == '-' || *fobj == EOS )
					ArgError( NOARGVAL, ERx("-m") );
				else
				{
					IIFDfixReplace( frmindx, fobj );
				}
				break;

		 	 case 'l':
		 	 case 'L':
				IIFDfiPrint( frmindx );
				break;

			 default :
				ArgError( BADARG, *argv );
				status = FAIL;
				break;
			} /* end switch */
		}
	} /* end while */

	IIFDfiClose( frmindx );

	EXdelete();

	PCexit( status );
}

static
ArgError ( errnum, arg )
STATUS	errnum;
char	*arg;
{
	if ( errnum != OK )
		IIUGfer( stderr, errnum, 0, 1, (PTR)arg);

	IIUGfer( stderr, BADSYNTAX, 0,
		2, (PTR)ERx("formindex"), (PTR)ERget(F_M10001_Usage)
	);
}

static
DbError ( dbname )
char	*dbname;
{
	IIUGfer( stderr, E_M10000_NoDB, 0, 1, (PTR)dbname );
}

/* this function should not be called. Its purpose is to    */
/* force some linkers that can't handle circular references */
/* between libraries to include these functions so they     */
/* won't show up as undefined                               */
dummy1_to_force_inclusion()
{
    IItscroll();
    IItbsmode();
}
