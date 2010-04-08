/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<st.h>
#include	<lo.h>
#include	<nm.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ug.h>
#include	<adf.h>
#include	<uf.h>
#include	<abclass.h>
#include	<fdesc.h>
#include	<abfrts.h>
#include	<abfcnsts.h>
#include	<abfcompl.h>
#include	"abfgolnk.h"
#include	"abfglobs.h"
#include	"erab.h"
#include	<metafrm.h>
#include	<dmchecks.h>

/**
** Name:	abtstbld.c -	ABF Application Build for Test Module.
**
** Descriptions:
**	Contains routines that are used to build the application for testing.
**	This may include linking an interpreter.  Defines:
**
**	IIABtestLink()	build the application for running in test mode.
**	iiabRmTest()	clean-up test image.
**
** History:
**	Revision 6.3/03/00  89/11/27  wong
**	Moved from "abfcom.c" and "ablink.c".
**
**	02-dec-91 (leighb) DeskTop Porting Change:
**		Added routines to pass data across ABF/IAOM boundary.
**	6-Feb-92 (blaise)
**		Added include abfglobs.h.
**	28-Dec-92 (fredb)
**		Port specific (hp9_mpe) change in IIABtestLink.
**      09-Feb-93 (fredb)
**		Fix bug in my last change (28-Dec-92).
**	11-Nov-1993 (tad)
**		Bug #56449
**		Replace %x with %p for pointer values where necessary.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**    25-Oct-2005 (hanje04)
**        Add prototype for mark_it() to prevent compiler
**        errors with GCC 4.0 due to conflict with implicit declaration.
**	26-Aug-2009 (kschendel) b121804
**	    Remove function defns now in headers.
**/


/*{
** Name:	IIABtestLink() - Build the Application for Running in Test Mode.
**
** Description:
**	Compiles the application and optionally links an interpreter as
**	appropriate for testing purposes.  Different ways used to exist for
**	an application to be built in test mode.  It depended on which of the
**	two defined constants NODY and IMTINTERP are set.  In any case, the
**	application structure contains a ABLNKTST structure to fill-in with
**	the information about the test image.
**
**	NODY		(defined by <dy.h>)
**		The application is linked as a standalone process.  It is
**		run by calling as a subprocess.
**
**	IMTINTERP
**		The application will be run interpretively.  To link it,
**		only the name of a file containing a DS-encode version of
**		the runtime tables is needed.
**
**	not defined NODY
**		The application is dynamically linked into memory and
**		run by directly calling into it.
**
** Inputs:
**	app	{APPL *}  The application to link.
**	tstimg	{ABLNKTST *}  Reference to test mode image object.
**
** Outputs:
**	tstimg	{ABLNKTST *}  Will be filled in with the information about
**				the test image.
**
** Returns:
**	{STATUS}  OK	success
**		  FAIL	failure
**
** History:
**	Written 7/26/82 (jrc)
**	1-sep-1986 (Joe)  Added the different types of test images.
**	01/89 (bobm)  Interpreter changes.
**	01/89 (jhw)  Added ABLNKTST parameter and renamed from 'ablnkrun()'.
**	28-Dec-92 (fredb)
**		The 'STcopy' of ABIMGEXT and the 'NMloc' call combined
**		create a useless name for MPE/iX due to our strange name
**		name symantics.  The 'bin' portion gets duplicated and then
**		mashed.  What should be 'iiinterp.bin' comes out
**		'iiintebi.bin'.  To avoid this problem, we won't copy the
**		ABIMGEXT.
**	09-Feb-93 (fredb)
**		It turns out that we do need to append the ABIMGEXT when
**		there are HL procs in the application.  I moved the copy
**		of the extension inside the 'if' statement for our port
**		only.
*/
LOCATION	*iiabMkObjLoc();

static bool	_chkHLdates();
static STATUS	_AppCompile();
static STATUS 	compile_comp();
static mark_it();

STATUS
IIABtestLink ( app, tstimg )
register APPL	*app;
ABLNKTST *tstimg;
{
	char		buf[MAX_LOC+1];
	LOCATION	exeloc;
	SYSTIME		exedate;
	LOCATION	nm;
	STATUS		rstatus = OK;
	char		interp[32];

#ifdef txROUT
	if (TRgettrace(ABTROUTENT, -1))
		abtrsrout( "IIABtestLink", "app = 0x%p", (char*)app,
				(char*)NULL
		);
#endif

	/*
	** '_AppCompile()' will set "redo_link" if any
	** HL procedures were recompiled.
	*/
	tstimg->redo_link = FALSE;
	if ( (rstatus = _AppCompile(app, tstimg)) != OK )
		return rstatus;

	/*
	** If there are no HL procedures, the interpreter executable is the
	** generic one living in the INGRES bin.  Otherwise, we need to make
	** one.  If we don't already know that we have to relink, we have to
	** check if the executable already exists, and if it's older than any
	** of the HL procedure object-code files.
	*/
#ifdef hp9_mpe
	_VOID_ STprintf(interp, "iiinterp");
#else
	_VOID_ STprintf(interp, "iiinterp%s", ABIMGEXT);
#endif
	if ( tstimg->hl_count > 0 )
	{
#ifdef hp9_mpe
		_VOID_ STprintf(interp, "iiinterp%s", ABIMGEXT);
#endif
		if ( (rstatus = IIABfdirFile(interp, buf, &exeloc)) == OK )
		{
			STcopy(buf, tstimg->abintexe);
			if ( tstimg->redo_link  ||
					LOlast(&exeloc, &exedate) != OK ||
						_chkHLdates(app, &exedate) )
			{
				rstatus = iiabInterpLink(app, tstimg, &exeloc);
			}
		}
	}
	else if ( (rstatus = NMloc(BIN, FILENAME, interp, &nm)) == OK )
	{ /* ... got "interpreter" */
		tstimg->user_link = FALSE;
		LOcopy(&nm, buf, &exeloc);
		STcopy(buf, tstimg->abintexe );
	}

	return rstatus;
}


/*
** Name:	_AppCompile() -	Compile Application for Testing.
**
** Descripition:
**	Does the compiling needed for "Go".
**
** Input:
**	app	{APPL	*}  The application to try and recompile.
**
**	NOTE:
**		There is some tricky use of the "locsym" item made
**		in here.  Objects come out of this routine with
**		locsym filled in non-NULL to indicate that the compile
**		failed, NULL otherwise.  This allows us to eventually
**		run the interpreter in spite of failed compiles, skipping
**		over some of them when building the run-time table.
**
** Returns:
**	OK	compiled
**	FAIL	compiler errors.
**
** Side Effects:
**	May recompile some source files.
**	May set redo_link flag.
**	counts hl procedures in hl_count.
**
** History:
**	1/89 (bobm) written.
**	11/89 (jwh)  Moved here and renamed from 'iiabGoAppCompile()'.
*/
static STATUS
_AppCompile ( app, tst )
APPL			*app;
register ABLNKTST	*tst;
{
	register APPL_COMP	*obj;

	STATUS		rval = OK;
	STATUS		status;

# ifdef txCOMP
	if (TRgettrace(ABTCOMPIMAGE, 0))
	{
		abtrcprint( ERx("COMPILE: Compiling %s for go"), app->name,
				(char *) NULL
		);
	}
# endif

	tst->failed_comps = FALSE;

	IIABdcmDispCompMsg(S_AB027A_Checking, TRUE, 0);

	tst->hl_count = 0;
	if (!tst->plus_tree)
	{
	    	if (tst->one_frame != NULL)
	    	{
			/* Compile just one frame */
			return compile_comp(tst->one_frame, (PTR)tst);
		}

		/* Recompile the whole application */
		for ( obj = (APPL_COMP *)app->comps ; 
		      obj != NULL ; 
		      obj = obj->_next )
		{
			status = compile_comp(obj, (PTR)tst);
			if (status != OK)
				rval = status;
		}

	}
	else
	{
		DM_COMPTREE_ARG comp_args;
		APPL_COMP	*comp = tst->one_frame;

		/* 
		** Compile the tree down from our start frame.
		** First compile the start frame,  We do this here to be sure
		** that its call dependencies are up-to-date before we begin
		** the traversal. 
		*/
		if (comp->class != OC_HLPROC)
		{
			comp_args.tstlnk = (PTR)tst;
			comp_args.rval = compile_comp(comp, (PTR)tst);
			comp_args.compile = compile_comp;
			IIAMxdsDepSearch(comp, IIAM_zctCompTree, 
					 (PTR)&comp_args);
			rval = comp_args.rval;
		}

		/* Now be sure to compile all the 3GL procs */
		for (obj = (APPL_COMP *)app->comps; 
		     obj != NULL; 
		     obj = obj->_next)
		{
			if (obj->class == OC_HLPROC)
			{
				status = compile_comp(obj, tst);
				if (status != OK)
					rval = status;
			}
		}
	}

	return rval;
}

/*
** Compile a single component 
**
** Returns a failure status if:
**	An unexpected error occurs, e.g. we can't create the LOCATION structure
**	for the error file, or
**	A vital frame can't compile.  "Vital" means either the frame we'd like
**	to start the test with, or a 3GL proc.
*/
static STATUS
compile_comp(obj, arg)
APPL_COMP *obj;		
PTR 	arg;
{
	bool		is_top;
	STATUS		status;
	STATUS		rval = OK;
	LOCATION	errors;
	char		buf[MAX_LOC+1];
	bool		compiled;
	register ABLNKTST *tst = (ABLNKTST *)arg;

	if ((status = IIABmefMakeErrFile(obj,&errors,buf)) != OK)
		return status;

	obj->locsym = NULL;
	is_top = FALSE;

	switch ( obj->class )
	{
	    case OC_OSLFRAME:
	    case OC_OLDOSLFRAME:
	    case OC_MUFRAME:
	    case OC_APPFRAME:
	    case OC_UPDFRAME:
	    case OC_BRWFRAME:
	    case OC_OSLPROC:
	    case OC_HLPROC:
		if (tst->entry_frame != NULL &&
				STequal(tst->entry_frame,obj->name))
		{
			is_top = TRUE;
		}

		if (obj->class == OC_HLPROC)
		{
			++tst->hl_count;
			if ( *(((HLPROC *)obj)->source) == EOS)
				break; /* Library procedure */
		}	

		/* Recompile if needed */
		compiled = iiabCompile(obj, ABF_CALLDEPS,
					FALSE, &errors, &status);
		/*
		** If we recompile a Host language procedures, it
		** implies a need to relink the interpreter executable.
		*/
		if (obj->class == OC_HLPROC && compiled)
		{
			tst->redo_link = TRUE;
		}
		break;
	} /* end switch */

	if ( status != OK )
	{
		obj->locsym = ERx("Bad Compile");
		tst->failed_comps = TRUE;
		if (is_top || obj->class == OC_HLPROC)
			rval = FAIL;
	}
	else if (compiled)
		LOdelete(&errors);

	return rval;
}

/*{
** Name:  IIABmstMarkSubTree - mark all subtree components
**
** Description:
**	recursive descent to mark a frame and all it's children for
**	a metaframe type.  Subtree will be flagged with TREE_MARK.
**	Includes the frame itself.  If this is called with a non-
**	metaframe type, the subtree will simply be that one object.
**	Marking the subtree of a NULL simply clears all TREE_MARK's
**
** Inputs:
**	appl - application.
**	pobj - parent object for subtree.
*/
IIABmstMarkSubTree(appl,pobj)
APPL *appl;
APPL_COMP *pobj;
{
	APPL_COMP *obj;

	for (obj = (APPL_COMP *)appl->comps ; obj != NULL ; obj = obj->_next )
		obj->flags &= ~TREE_MARK;

	mark_it(pobj);
}

static
mark_it(obj)
APPL_COMP *obj;
{
	METAFRAME *m;
	i4 i;

	if (obj == NULL || (obj->flags & TREE_MARK) != 0)
		return;

	obj->flags |= TREE_MARK;

	switch ( obj->class )
	{
	  case OC_MUFRAME:
	  case OC_APPFRAME:
	  case OC_UPDFRAME:
	  case OC_BRWFRAME:
		m = ((USER_FRAME *) obj)->mf;
		for (i=0; i < m->nummenus; ++i)
			mark_it(((m->menus)[i])->apobj);
		break;
	  default:
		break;
	}
}

/*
** check dates on HL procedure objects, and set "redo_link" if newer
** than executable.
*/
static bool
_chkHLdates ( app, exedate )
APPL		*app;
SYSTIME		*exedate;
{
	register APPL_COMP	*comp;

	for ( comp = (APPL_COMP *)app->comps ; comp != NULL ;
			comp = comp->_next )
	{
		if ( comp->class == OC_HLPROC &&
				*((HLPROC *)comp)->source != EOS )
		{
			SYSTIME	odate;
			/*
			** LOlast shouldn't fail.  Let it generate a link error
			** if the object has somehow disappeared.  If HL object
			** is newer than executable, we have to relink.
			*/
			if ( LOlast( iiabMkObjLoc(((HLPROC *)comp)->source),
					&odate ) != OK ||
						TMcmp(&odate, exedate) >= 0 )
			{
				return TRUE;
			}
		}
	}
	return FALSE;
}

/*{
** Name:	iiabRmTest() -	Clean-up Test Image.
**
** Description:
**	Removes any test images.
**
** Input:
**	tstimg	{ABLNKTST *}  Reference to test mode image object.
**
** History:
**	06/89 (jhw) - Written.
*/
VOID
iiabRmTest ( tstimg )
register ABLNKTST	*tstimg;
{
	if ( tstimg->hl_count > 0 && *tstimg->abintexe != EOS )
	{
		LOCATION	loc;
		char		lbuf[MAX_LOC+1];

		STcopy(tstimg->abintexe, lbuf);

		if ( LOfroms(PATH&FILENAME, lbuf, &loc) == OK  &&
				LOdelete(&loc) == OK )
		{
			*tstimg->abintexe = EOS;
		}
	}
}
