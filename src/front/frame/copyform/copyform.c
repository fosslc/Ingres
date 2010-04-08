/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<pc.h>		/* 6-x_PC_80x86 */
#include	<ex.h>
#include	<me.h>
#include	<lo.h>
#include	<st.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ug.h>
#include	<cf.h>
#include	<ooclass.h>
#include	<uigdata.h>
#include	<erug.h>
#include	"ercf.h"

/**
** Name:	copyform.c -	CopyForm Main Line and Entry Point Module.
**
** Description:
**	Copyform is a utility for users to copy forms, qbfnames and joindefs
**	between databases and to store those objects on archive devices.
**	There are two phases of operation for the utility. In the first phase,
**	the objects are dumped to an intermediate file.	 In the second phase,
**	the objects are read from the intermediate file and placed in the
**	database.
**	This file defines:
**
**	main()	main procedure for copyform utility
**
** History:
**	Revision 6.1  88/04  wong
**	Added `equel' command-line flag; fixed bug:  `silent' flag not checked;
**	also, must check 'FEingres()' return status and report error.
**
**	Revision 6.0  87/04  rdesmond
**	(rdesmond) - disabled "specify form?" prompt when
**				at least on object on command line.
**	04-apr-87 (rdesmond) - modified for 6.0 (completely rewritten);
**		no longer uses temp files on copy in since quel copy is no
**		longer effective, as unique ids must be generated on input;
**		uses new catalog structure.
**	17-jun-87 (rdesmond) - removed transaction for cf_rdobj.
**	19-jun-87 (rdesmond) - Fixed PCexit bug, #332.
**	08-jul-87 (rdesmond) - changed to be compatible with 5.0
**			syntax.	 Uses -i flag (no longer "in" and "out")
**			and filename must be specified positionally.
**	03/16/88 (dkh) - Added EXdelete call to conform to coding spec.
**	14-apr-88 (bruceb)
**		Added, if DG, a send of the terminal's 'is' string at
**		the very beginning of the program.  Needed for CEO.
**
**	Revision 2.0  83/07  peter
**	Initial revision.
**	07/28/88 (dkh) - Put in special hook so that formindex can
**			 find out the name of the form that was copied in.
**			 Also moved declaration of Cf_silent and Cfversion
**			 to cfrdobj.qsc.
**	08/02/88 (dkh) - Moved Cfversion and Cf_silent back in here.
**	27-nov-89 (sandyd)
**		After consultation with DKH, moved Cfversion and Cf_silent
**		into new "cfglobs.c" file.  On VMS, soft references to those
**		globals (via cf.h) were causing a duplicate main (copyform.c)
**		to be pulled in when linking new programs that use copyform
**		routines.  (Old programs worked around that by hacking in
**		their own GLOBALDEF's of Cfversion and Cf_silent.  Separating
**		the globals into their own file should be a cleaner way to go.)
**	12/23/89 (dkh) - Added support for owner.table.
**	01/02/90 (kenl)- Added support for the +c (connection) flag.
**	19-jan-90 (bruceb)
**		Made 'file' an LO buffer instead of a (char *).
**      28-sep-90 (stevet)
**              Add call to IIUGinit.
**	08/11/91 (dkh) - Fixed bug 39055.
**      16-nov-1992 (larrym)
**          Added CUFLIB to NEEDLIBS.  CUF is the new common utility facility.
**	8/18/93 (dkh) - Changed code to eliminate compile problems on NT.
**	03/25/94 (dkh) - Added call to EXsetclient().
**	18-jun-97 (hayke02)
**	    Add dummy calls to force inclusion of libruntime.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
[@history_template@]...
**/


/*{
** Name:	main - main procedure for copyform
**
** Description:
**	Copyform - main program
**	-----------------------
**
**	COPYFORM is a program which reads in or writes a text file
**	representation of a form, joindef or qbfname to or from a database.
**
**	Calling Sequence:
**		To write out object(s) to a text file, use:
**
**		COPYFORM [-uuser] [-ffile] dbname {form}
**		or
**		COPYFORM -q [-uuser] [-ffile] dbname {qbfname}
**		or
**		COPYFORM -j [-uuser] [-ffile] dbname {joindef}
**		where	-uuser	= alternate username to use.
**			file	= file to write definitions to.
**			dbname	= name of database.
**			{form}	= list of forms to write to file.
**			{qbfname} = list of qbfnames to write to file.
**			{joindef} = list of joindefs to write to file.
**
**
**		To read in a file containing form definitions, use:
**
**		COPYFORM -i [-uuser] [-ffile] [-r] dbname
**		where	-uuser	= alternate username to use.
**			file	= file to write definitions to.
**			-r	= replace existing objects.
**			dbname	= name of database.
**
** History:
**		Revision 30.3  85/11/15	 18:27:08  roger
**		Increased maximum no. of forms on command line
**		to 75, since we use it to create the deliverable
**		forms shipped with the Burroughs international
**		toolkit (61 forms in RTI front-ends, excluding
**		duplicates).
**
**		10/27/85 (prs)	take out cf_prompt.
**
**		7/31/83 (ps)	written.
**
**		29-dec-1986 (peter)	Changed MKMFIN hints for 6.0.
**		2/18/87 (daveb)	 -- Call to MEadvise, added MALLOCLIB hint.
**		04-apr-87 (rdesmond) - completely rewritten
**		08-jul-87 (rdesmond) - changed to be compatible with 5.0
**			syntax.	 Uses -i flag (no longer "in" and "out")
**			and filename must be specified positionally.
**		30-nov-89 (sandyd)
**			Removed unnecessary UNDEFS.  Also removed GLOBALDEFS
**			of Cfversion and Cf_silent, since those have been
**			moved to cfglobs.c.
**		28-aug-1990 (Joe)
**	    		Changed references to IIUIgdata to the UIDBDATA
**			structure returned from IIUIdbdata().
*/

# ifdef DGC_AOS
# define main IICFrsmRingSevenMain
# endif

/*
**	MKMFIN Hints
**
PROGRAM =	copyform

NEEDLIBS =	COPYFORMLIB VIFREDLIB \
		OOLIB UFLIB RUNSYSLIB RUNTIMELIB FDLIB FTLIB FEDSLIB \
		UILIB LIBQSYSLIB LIBQLIB UGLIB FMTLIB AFELIB \
		LIBQGCALIB CUFLIB GCFLIB ADFLIB \
		COMPATLIB MALLOCLIB

UNDEFS =	II_copyright
*/

i4
main(argc, argv)
i4	argc;
char	**argv;
{
    char	*dbname;
    char	*uname = ERx("");
    char	*xflag = ERx("");
    char	file[MAX_LOC + 1];
    char	*with_ptr = ERx("");	/* pointer to with clause */
    bool	replace;
    bool	in;
    i4		inmode;
    char	*obj_arr[MAX_OBJNAMES];
    EX_CONTEXT	context;
    UIDBDATA	*uidbdata;
    i4		max_objs = MAX_OBJNAMES;
    char	**objname;

    EX	FEhandler();

    /* Start of routine */

    /* Tell EX this is an ingres tool. */
    (void) EXsetclient(EX_INGRES_TOOL);

    /* Use the ingres allocator instead of malloc/free default (daveb) */
    MEadvise( ME_INGRES_ALLOC );

    /* Add call to IIUGinit to initialize character set table and others */

    if ( IIUGinit() != OK )
    {
        PCexit(FAIL);
    }


    IIOOinit((OO_OBJECT **)NULL);

    FEcopyright(ERget(F_CF0001_COPYFORM), ERx("1984"));

    /*
    ** Get arguments from command line
    **
    ** This block of code retrieves the parameters from the command
    ** line.  The required parameters are retrieved first and the
    ** procedure returns if they are not.
    ** The optional parameters are then retrieved.
    */
    {
	ARGRET	rarg;
	i4	pos;
	char	*argname;
	i4	fmode;

	if (FEutaopen(argc, argv, ERx("copyform")) != OK)
		PCexit(FAIL);

	in = (FEutaget(ERx("inflag"), 0, FARG_FAIL, &rarg, &pos) == OK);

	/* database name is required */
	if (FEutaget(ERx("database"), 0, FARG_PROMPT, &rarg, &pos) == OK)
	    dbname = rarg.dat.name;
	else
	    PCexit(FAIL);

	/* file name is required */
	if (FEutaget(ERx("file"), 0, FARG_PROMPT, &rarg, &pos) == OK)
	{
	    STcopy(rarg.dat.name, file);
	}
	else
	{
	    PCexit(FAIL);
	}

	if (FEutaget(ERx("connect"), 0, FARG_FAIL, &rarg, &pos) == OK)
	{
	    with_ptr = STalloc(rarg.dat.name);
	    IIUIswc_SetWithClause(with_ptr);
	}

	if (!in)
	{
	    register i4	numobj;
	    bool		obj_on_cl;


# ifdef DGC_AOS
	    if (*with_ptr == EOS)
	    {
	    	with_ptr = STalloc("dgc_mode='reading'");
	    	IIUIswc_SetWithClause(with_ptr);
	    }
# endif

	    /* determine if at least one object name is on the cl */
	    obj_on_cl =
		(FEutaget(ERx("objname"), 0, FARG_FAIL, &rarg, &pos) == OK);

	    /* determine if input mode is specified on the cl */
	    if (FEutaget(ERx("inqbfname"), 0, FARG_FAIL, &rarg, &pos) == OK)
	    {
		inmode = CF_QBFNAME;
	    }
	    else if (FEutaget(ERx("injoindef"), 0, FARG_FAIL, &rarg, &pos)
		== OK)
	    {
		inmode = CF_JOINDEF;
	    }
	    else
	    { /* input mode is not specified on command line */
		if (obj_on_cl)
		{
		    inmode = CF_FORM;
		}
		else
		{
		    _VOID_ FEutaget(ERx("inform"), 0, FARG_PROMPT, &rarg, &pos);
		    if (rarg.dat.flag)
		    {
			inmode = CF_FORM;
		    }
		    else
		    {
			_VOID_ FEutaget(ERx("inqbfname"), 0, FARG_PROMPT,
					&rarg, &pos
			);
			if (rarg.dat.flag)
			{
			    inmode = CF_QBFNAME;
			}
			else
			{
			    _VOID_ FEutaget(ERx("injoindef"), 0, FARG_PROMPT,
					&rarg, &pos
			    );
			    if ( rarg.dat.flag )
			    {
				inmode = CF_JOINDEF;
			    }
			    else
			    {
				PCexit(FAIL);
			    }
			}
		    }
		}
	    }
	    /* determine if at least one object name is on the cl */
	    if (FEutaget(ERx("objname"), 0, FARG_FAIL, &rarg, &pos) == OK)
	    {
		fmode = FARG_FAIL;
		argname = ERx("objname");
	    }
	    else
	    {
		fmode = FARG_OPROMPT;
		/* set argname to display appropriate prompt for objs */
		switch(inmode)
		{
		    case CF_FORM:
			    argname = ERx("form");
			    break;
		    case CF_QBFNAME:
			    argname = ERx("qbfname");
			    break;
		    case CF_JOINDEF:
			    argname = ERx("joindef");
			    break;
		}
	    }

	    /* get object names */
	    objname = &(obj_arr[0]);
	    numobj = 0;
	    while ( FEutaget(argname, numobj, fmode, &rarg, &pos) == OK &&
	      		*rarg.dat.name != EOS )
	    {
		PTR		pointer;
		PTR		oldarray;
		PTR		newarray;
		u_i4	size;
		u_i4	oldsize;
		u_i2		cpsize;

		objname[numobj++] = rarg.dat.name;

		/*
		**  Check to see if we need to allocate a bigger array.
		**  We do it here so that we don't have to do another
		**  check after exiting this loop (due to null termination
		**  of array after we exit).
		*/
		if (numobj >= max_objs)
		{
			oldsize = sizeof(char *) * max_objs;
			size = oldsize + oldsize;
			if ((pointer = (PTR) MEreqmem((u_i4) 0, size,
				(bool) TRUE, NULL)) == NULL)
			{
				/*
				**  Couldn't allocate more memory.  Give
				**  error and exit.
				*/
				IIUGbmaBadMemoryAllocation(ERx("main()"));
				PCexit(FAIL);
			}
			newarray = pointer;
			oldarray = (PTR) objname;

			/*
			**  Now copy old array to new array.
			*/
			while(oldsize > 0)
			{
				cpsize = (oldsize <= MAXI2) ? oldsize : MAXI2;
				MEcopy(oldarray, cpsize, newarray);
				oldsize -= cpsize;
				oldarray = (PTR) ((char *)oldarray + cpsize);
				newarray = (PTR) ((char *)newarray + cpsize);
			}

			/*
			**  Free old array only if was dynamically allocated.
			*/
			if (objname != &(obj_arr[0]))
			{
				MEfree(objname);
			}

			/*
			**  Set new count for maximum objects that
			**  array can hold and reassign pointers.
			*/
			max_objs += max_objs;
			objname = (char **) pointer;
		}
	    }

	    /* terminate the list of objects */
	    objname[numobj] = NULL;
	}

	/* Get optional arguments */

	replace = (FEutaget(ERx("replace"), 0, FARG_FAIL, &rarg, &pos) == OK);
	Cf_silent = (FEutaget(ERx("silent"), 0, FARG_FAIL, &rarg, &pos) == OK);

	if (FEutaget(ERx("user"), 0, FARG_FAIL, &rarg, &pos) == OK)
	    uname = rarg.dat.name;
	if (FEutaget(ERx("equel"), 0, FARG_FAIL, &rarg, &pos) == OK)
	    xflag = rarg.dat.name;

	FEutaclose();
    }

    if ( EXdeclare(FEhandler, &context) != OK )
    {
	EXdelete();
	PCexit(FAIL);
    }

    if ( FEingres(dbname, uname, xflag, (char *)NULL) != OK )
    {
	IIUGerr( E_UG000F_NoDBconnect, UG_ERR_ERROR, 1, (PTR)dbname );
	EXdelete();
	PCexit(FAIL);
    }

    uidbdata = IIUIdbdata();
    if (!in)
    { /* write out the forms or qbfnames */
	cf_dmpobj(inmode, objname, uidbdata->user, uidbdata->dba, file);
    }
    else
    { /* read in the file and process it */
	cf_rdobj(file, uidbdata->user, replace, NULL);
    }

    FEing_exit();

    EXdelete();

    PCexit(OK);
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
