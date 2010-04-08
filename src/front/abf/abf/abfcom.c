/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<lo.h>
# include	<pe.h>		/* 6-x_PC_80x86 */
#include	<st.h>
#include	<nm.h>
#include	<si.h>
#include	<ut.h>
#include	<tm.h>
#include	<er.h>
#include	<ol.h>
#include	<ci.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ug.h>
#include	<adf.h>
#include	<uf.h>
#include	<abclass.h>
#include	<oocat.h>
#include	<abfcnsts.h>
#include	<abftrace.h>
#include	<fdesc.h>
#include	<abfrts.h>
#include	<abfcompl.h>
#include	"abfgolnk.h"
#include	<abfglobs.h>
#include	"erab.h"
#include	"abut.h"
#include       	<stdprmpt.h>
#include	<metafrm.h>
#include        <ft.h>
#include        <fmt.h>
#include        <frame.h>

/**
** Name:	abfcom.c -	ABF Object Compilation Module.
**
** Descriptions:
**	Contains routines that are used in the compilation of the parts of an
**	application.  Defines
**
**	IIABfileCompile() compile a file.
**	iiabAppCompile() compile an application definition.
**	iiabCompile()	compile an application component object if out-of-date.
**	iiabCkExtension() check file extension for language.
**	iiabExtDML()	determine dml for file.
**
** History:
**	Revision 6.3  89/09/30  wong
**	Modified to use new compile options (which includes force and
**	object-code options) for RunTime INGRES support.
**	89/09/29  wong  Make sure suffix is lower-cased on case-insensitive
**			systems before checking.  JupBug #7604.
**
**	Revision 6.3/03/00  90/03/28  Mike S
**	Improve appearance during compiles and links.
**	1. Always produce compilation list files.
**	2. Keep control of the screen during compilations and links.
**
**	15-feb-91 (tom) - implemented vision capability test so that
**		we don't allow imaging of apps with vision frames.
**	8-march-91 (blaise)
**	    Integrated changes for PC porting.
**
**	Revision 6.4
**	03/22/91 (emerson)
**		Fix interoperability bug 36589:
**		Change all calls to abexeprog to remove the 3rd parameter
**		so that images generated in 6.3/02 will continue to work.
**		(Generated C code may contain calls to abexeprog).
**		This parameter was introduced in 6.3/03/00, but all calls
**		to abexeprog specified it as TRUE.  See abfrt/abrtexe.c
**		for further details.
**	20-may-1991 (mgw) Bug #36391
**		Delete the gca save state file if the connection (to UT)
**		was never made (e.g. if the source file doesn't exist).
**	26-jul-1991 (mgw) Bug #38867
**		changed LOdelete(xfloc) to LOdelete(&xfloc) in oslCompile()
**	18-Jan-1993 (fredb) hp9_mpe
**		Various porting changes.  I have described the changes in
**		detail in the individual functions.
**	21-Jan-1993 (fredb)
**		Add code to iiabCompile to prevent program crashes.
**	04-Feb-1993 (fredb)
**		Fix porting change bug in IIABmefMakeErrFile.
**	20-May-1993 (fredb)
**		IFDEF my prior changes to LO buffer sizes.  The relationship
**		between the defined sizes is different between MPE & UNIX.
**		Bug #51560
**      21-mar-94 (smc) Bug #60829
**              Added #include header(s) required to define types passed
**              in prototyped external function calls.
**	15-may-97 (mcgem01)
**		Fix order of includes.
**	15-oct-98 (kitch01)
**		Bug 93617. If the form compiles OK then 0 length er*.tmp files
**		are left in the II_TEMPORARY directory. **
**      21-apr-1999 (hanch04)
**        Replace STrindex with STrchr
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      11-Jun-2004 (hanch04)
**          Removed reference to CI for the open source release.
**	26-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	24-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

char	*UGcat_now();

LOCATION	*iiabMkLoc();
LOCATION	*iiabMkFormLoc();
LOCATION	*iiabMkObjLoc();
char		*IIxflag();

STATUS		IIAMfoiFormInfo();

#ifndef NOVISION
STATUS iiabGenCheck();
#endif

GLOBALREF bool	IIabC50;
GLOBALREF bool	IIabWopt;
GLOBALREF bool	IIabOpen;
GLOBALREF bool	IIabVision;
GLOBALREF char  **IIar_Dbname;
GLOBALREF i2	osNewVersion;

static VOID	make_errlis();

# ifdef VMS     /* Forms are output exclusively in C on Unix and CMS. */
# define USEMACRO  TRUE
# else
# define USEMACRO  FALSE
# endif

/*{
** Name:	IIABfrcCompile() -	Force Compilation for an Object.
**
** Description:
**	Force compilation for an object.
**
** Inputs:
**	app	{APPL *}  The application.
**	obj	{APPL_COMP *}  The application component to be compiled.
**
** Returns:
**	{STATUS}  OK, if compiled successfully.
**		  FAIL otherwise.
**
** History:
**	04/90 (jhw) -- Written.
*/
STATUS
IIABfrcCompile ( app, obj )
APPL		*app;
APPL_COMP	*obj;
{
	STATUS	status = FAIL;
	char	*file;

	if ( obj->class == OC_HLPROC
			&& ( (file = ((HLPROC *)obj)->source) == NULL
				|| *file == EOS ) )
	{ /* a library procedure */
		IIUGerr( E_AB0012_NotLibOper, UG_ERR_ERROR,
				1, ERget(FE_Compile)
		);
	}
	else if ( obj->class == OC_RWFRAME
			&& ( (file = ((REPORT_FRAME *)obj)->source) == NULL
				|| *file == EOS ) )
	{ /* a RBF report frame */
		IIUGerr(E_AB003E_NoRepFile, UG_ERR_ERROR, 0);
	}
	else
	{
		bool		success;
		LOCATION	errors;
		char		buf[MAX_LOC+1];
		bool		hadwarnings;
		bool		haderrors;

		status = IIABmefMakeErrFile(obj, &errors, buf);

		success = iiabCompile( obj, ABF_COMPILE|ABF_FORCE, TRUE,
				( status == OK ) ? &errors : NULL, &status
		);

		hadwarnings = success && status == OK && 
			      ((obj->flags & APC_OSLWARN) != 0);
		haderrors = !success && status != OK;

		if (hadwarnings)
			IIUGerr(S_AB002F_CompWorkedWarnings, 0, 0);
		
		if (hadwarnings || haderrors)
			_VOID_ IIVQdceDisplayCompErrs(app, obj, TRUE);
	}

	return status;
}

/*{
** Name:	iiabAppCompile() -	Compile an Application.
**
** Descripition:
**	Sees if the application needs to be compiled for a definition.
**
** Input:
**	app	{APPL	*}  The application to try and recompile.
**	options	{nat}  Compilation options:
**			ABF_FORCE	- all components should be recompiled.
**			ABF_OBJONLY	- object with no source is OK.
**			ABF_OBJECTCODE	- compile to object-code.
**			ABF_CALLDEPS	- load call dependencies 
**			ABF_NOERRDISP	- supress error display.
**	
** Returns:
**	{bool}  TRUE	successful
**		FALSE	if there is a problem in compiling the application.
**
** Side Effects:
**	May recompile some source files.
**
** History:
**	Written 8/4/82 (jrc)
**	9-jun-1987 (Joe)
**		Added force parameter.
**	3/90 (Mike S)
**		Remove status parameter.
**	9/90 (Mike S)
**		Compile forms only if we're using a compiled form.  Also compile
**		forms last, in case we're in Emerald and they get created or
**		changed as a result of compiling a frame.
*/
bool
iiabAppCompile (app, options)
APPL	*app;
i4	options;
{
	register APPL_COMP	*cp;
	bool			rval = TRUE;
	STATUS			lstat;
	char			buf[MAX_LOC+1];
	LOCATION		errloc;
	FORM_REF		*first_form;	/* First form in list */
	FORM_REF		**formptr;	/* Pointer to next form */
	
# ifdef txCOMP
	if (TRgettrace(ABTCOMPIMAGE, 0))
	{
		abtrcprint( ERx("COMPILE: Compiling the image for %s"),
				app->name,
				(char *) NULL
		);
	}
# endif

	/* Set "Database Form" bit for all forms. Also make a list of forms */
	first_form = NULL;
	formptr = &first_form;
	for ( cp = (APPL_COMP *)app->comps ; cp != NULL ; cp = cp->_next )
	{
		switch (cp->class)
		{

		    case OC_FORM:
	    	    case OC_AFORMREF:
			cp->flags |= APC_DBFORM;
			*formptr = (FORM_REF *)cp;
			formptr = (FORM_REF **)&(cp->dep_on);
			break;
		}
	}
	*formptr = NULL;

	/* 
	** Compile anything that's not a form, and clear "Database Form" 
	** bit for all forms we must compile.
	*/
	for ( cp = (APPL_COMP *)app->comps ; cp != NULL ; cp = cp->_next )
	{
		FORM_REF *formref;
		
		formref = NULL;
		switch (cp->class)
		{
		    case OC_RWFRAME:
			formref = ((REPORT_FRAME *)cp)->form;
			break;

		    /* disallow vision frame types unless we have capability */
		    case OC_MUFRAME:	
		    case OC_APPFRAME:
		    case OC_UPDFRAME:
		    case OC_BRWFRAME:
		    case OC_OSLFRAME:
		    case OC_OLDOSLFRAME:
			formref = ((USER_FRAME *)cp)->form;
			break;
			
		}

		if (formref != NULL && 
		    formref->source != NULL && 
		    *formref->source != EOS)
		{	
			if ((app->flags & APC_DBFORMS) == 0 ||
			    (cp->flags & APC_DBFORM) == 0)
				formref->flags &= ~APC_DBFORM;
		}
			
		if (cp->class  != OC_FORM && cp->class != OC_AFORMREF)
		{
			/* Mot a form; compile it */
			_VOID_ IIABmefMakeErrFile(cp, &errloc, buf);

			_VOID_ iiabCompile( cp, ABF_OBJECTCODE|options, FALSE,
					   &errloc, &lstat);
			rval = rval && (lstat == OK);
		}
	}

	/* Compile any needed forms, if nothing else failed */
	if (rval)
	{	
		FORM_REF *fr;

		for (fr = first_form; fr != NULL; fr = (FORM_REF *)fr->dep_on)
		{
			/* Don't bother with an uncompiled form */
			if ((fr->flags & APC_DBFORM) != 0)
				continue;

			_VOID_ IIABmefMakeErrFile((APPL_COMP *)fr, 
						  &errloc, buf);

			_VOID_ iiabCompile((APPL_COMP *)fr, 
					    ABF_OBJECTCODE|options, FALSE,
			           	    &errloc, &lstat);
			rval = rval && (lstat == OK);

			/* Bug 93617. Remove the tmp files if form compile was OK */
			if (rval)
				_VOID_ LOdelete(&errloc);

		}
	}

	if (!rval)
	{
		/* "Compilation failure" message */
		IIUGerr(COMIMAGE, UG_ERR_ERROR, 0);
	}

	return rval;
}

/*{
** Name:	iiabCompile() -	Compile an Application Component Object.
**
** Description:
**	Compile an application component object.
**
**	Note:
**		The "object_code" flag means exactly that only for 4GL
**		files.  The flag is also used to determine display
**		of status messages - if not going to object_code, we
**		avoid screen erases when able.  Will be set FALSE for
**		everything when compiling for the interpreter.
**
** Inputs:
**	comp	{APPL_COMP *}  The application component object to compile.
**	options	{nat}  Compilation options:
**			ABF_FORCE	- all components should be recompiled.
**			ABF_OBJONLY	- object with no source is OK.
**			ABF_OBJECTCODE	- compile to object-code.
**	genqry		{bool}	Whether to query for metaframe regeneration.
**	errfile		{LOCATION *}	- compilation listing file
**
** Outputs:
**	status		{STATUS *}  Detailed compilation status.
**
** Returns:
**			TRUE if compilation was successful
**			FALSE if it failed, or didn't take place.
**			If it didn't take place because it's unnecessary,
**			status is OK; if because the user didn't wish to
**			retry a previously failed compilation, status is FAIL.
**
** History:
**	03/12/89 (billc) -- Don't print errors for E_CompErrs returned by
**		'oslCompile()'.  JupBug #3327.
**	08/89 (jhw) -- Don't print error for FAIL.  JupBug #7444.  (E_CompErrs
**		case subsumed by this change.)
**	09/89 (jhw) -- Modified to use new compile options and to allow for
**		object with no source.
**	21-feb-91 (blaise)
**		When forcing compilation, test whether the current procedure
**		is a library procedure; if so don't attempt to compile.
**		Bug #35979.
**	25-feb-91 (blaise)
**		Added a further check to the above change; check that the
**		current procedure is a host language procedure.
**	21-Jan-93 (fredb)
**		Added a new check: If iiabMkFormLoc fails, a NULL location
**		pointer is returned which causes LOtos to fail.  Now, if we
**		get a NULL location, we set status to FAIL and return FALSE.
**	21-mar-94 (donc) 59981
**		var_ingres (ABF_OBJONLY) was trying to recompile 4GL code,
**		which as it turns out, is disallowed. What happens now if
**		this is a var_ingres license is that 4GL code will not be
**		preprocessed and compiled. VIFRED forms changes WILL get
**		compiled anf linked in, this allows VAR customers to make
**		forms changes NOT 4GL changes.  All object modules will then
**		be linked together to create the application image.		
*/
static bool	oslCompile();
bool
iiabCompile ( APPL_COMP *comp, i4 options, bool genqry, LOCATION *errfile, STATUS *status )
{
	char	*tdt;
	STATUS	stat;
	STATUS	compcheck;
	bool	compiled = FALSE;
	bool	updframe = FALSE;
	bool	force = (bool)(options & ABF_FORCE);
	bool	doit = force;
	bool	ismeta = FALSE;
	i4	oflags;
	i4	ostate;
	char	*ogdate, *ofdate;
	LOCATION *srcloc;
	ER_MSGID msg;
	char	*srcfile;

	if ( status == NULL )
		status = &stat;

	*status = OK;

	tdt = comp->compdate;
	if (tdt == NULL)
		tdt = comp->compdate = ERx("");

	/* Keep track of things which might change; get source file */
	oflags = comp->flags;
	switch (comp->class)
	{
	    case OC_MUFRAME:
	    case OC_APPFRAME:
	    case OC_UPDFRAME:
	    case OC_BRWFRAME:
		if (IIabVision)
		{
			METAFRAME *m;

			ismeta = TRUE;
			m = ((USER_FRAME *)comp)->mf;
			ostate = m->state;
			ogdate = m->gendate;
			ofdate = m->formgendate;
		}
		/* fall through */

	    case OC_OLDOSLFRAME:
	    case OC_OSLFRAME:
		srcfile = ((USER_FRAME *)comp)->source;
		break;

	    case OC_OSLPROC:
		srcfile = ((_4GLPROC *)comp)->source;
		break;

	    case OC_AFORMREF:
  	    case OC_FORM:
		srcloc = iiabMkFormLoc(((FORM_REF *)comp)->source);
		if (srcloc == NULL)
		{
			srcfile = NULL;		/* just in case */
			*status = FAIL;		/* what else could we use? */
			return (FALSE);
		}
		else
		{
			LOtos(srcloc, &srcfile);
		}
		break;

	    case OC_HLPROC:
		srcfile = ((HLPROC *)comp)->source;
		break;

	    case OC_RWFRAME:
		srcfile = ((REPORT_FRAME *)comp)->source;
		break;
	}

	/* See if we must recompile */
	if (!force)
	{
		compcheck = IIABccsChkCompStatus(comp, options, genqry);
		if (compcheck == OK)
		{
			/* 
			** We issued the "Processing..." message; follow it
			** with a blank line.
			*/
			IIABdcmDispCompMsg(F_AB0101_BlankLine, FALSE, 0);
			return FALSE;	/* No compilation */
		}
		else if (compcheck == E_NoComp)
			return FALSE;	/* No compilation */
		else if (compcheck == -1)
			doit = TRUE;
		else
			*status = compcheck;
	}
	else
	{
		/* Don't compile library procedures */
		if (comp->class == OC_HLPROC &&
					(srcfile == NULL || *srcfile == EOS))
		{
			return FALSE;
		}

		if (IIabVision)
		{
			/* If we were forced, check metaframe generation */
			bool dummy;
			STATUS gencheck;

			switch (comp->class)
			{

			    case OC_MUFRAME:
			    case OC_APPFRAME:
			    case OC_BRWFRAME:
			    case OC_UPDFRAME:

				gencheck = iiabGenCheck((USER_FRAME *)comp,
							genqry, &dummy);
				if (gencheck == FAIL)
				{
					/* The generation failed */
					doit = FALSE;
					*status = FAIL;
				}
				else if (gencheck == FE_Cancel)	
				{
					/*
					** User doesn't want to compile a
					** custom frame
					*/
					return FALSE;
				}
			}
		}
	}
			
	if (doit)
	{
		/* Compile it */
		switch (comp->class)
		{
	        case OC_MUFRAME:
       	   	case OC_APPFRAME:
          	case OC_UPDFRAME:
          	case OC_BRWFRAME:
	  	case OC_OLDOSLFRAME:
	  	case OC_OSLFRAME:
	  	case OC_OSLPROC:
			comp->compdate = ERx("");
			updframe = TRUE;
		        if ( (options & ABF_OBJONLY) )
			{	
			    *status = E_ObjNoSrc;
			    compiled = TRUE;
   			}
			else 
			    compiled = oslCompile( comp, options, errfile, 
						status, ismeta );
			break;

	  	case OC_AFORMREF:
	  	case OC_FORM:
			if ((comp->flags & APC_DBFORM) != 0)
			{
				/* Nothing to do */
				*status = OK;
				break;
			}

			/* make compiled form */
			*status = IIcompfrm(comp->name, (i4 *)NULL,
					((FORM_REF *)comp)->symbol, srcfile, 
					USEMACRO);

			if (*status == OK)
			{
				*status = IIUTcompile((char *)NULL, 
						srcloc,
						S_AB0040_Compiling, 
						(UTARGS *)NULL,
						(LOCATION *)NULL, &compiled);
			}
			break;

		  case OC_HLPROC:
			comp->compdate = ERx("");
			updframe = TRUE;
			*status = IIUTcompile((char *)NULL,
					iiabMkLoc(comp->appl->source, srcfile),
					S_AB0040_Compiling,
					(UTARGS *)NULL, errfile, &compiled
			);
			if (*status != OK)
			{
				comp->flags |= APC_UFERRS;
			}
			break;

		  case OC_RWFRAME:
			IIABdcmDispCompMsg(S_AB0040_Compiling, FALSE, 1,
					   srcfile);
			abexeprog( ERx("sreport"), ERx("file = %L"),
				   1, iiabMkLoc(comp->appl->source, srcfile));
			break;

		  default:
			/* An object type which can't compile */
			*status = OK;
			return FALSE;	
		} /* end switch */
	}

	switch ( *status )
	{
	  ER_MSGID ermsg;

	  case E_ObjNoSrc:
		if ( (options & ABF_OBJONLY) ) {
			*status = OK;
			break;	/* only object is required */
		}
		*status = E_NoSrcFile;	/* requires source */
		/* fall through ... */
	  case E_NoSrcFile:
		if (comp->class == OC_AFORMREF || comp->class == OC_FORM)
			ermsg = E_AB0230_NoCompForm;
		else
			ermsg = E_NoSrcFile;
		IIUGerr(ermsg, UG_ERR_ERROR, 2, 
			comp->appl->source, srcfile);
		comp->compdate = STtalloc(comp->data.tag, UGcat_now());
		updframe = TRUE;
		if (errfile != (LOCATION *)NULL)
			make_errlis(errfile, ermsg, 2, 
				    comp->appl->source, srcfile);
		break;

	  case E_NoFrameForm:
		IIUGerr(E_NoFrameForm, UG_ERR_ERROR, 2, 
			((USER_FRAME *)comp)->form->name, comp->name);
		comp->compdate = STtalloc(comp->data.tag, UGcat_now());
		updframe = TRUE; 
		if (errfile != (LOCATION *)NULL)
			make_errlis(errfile, E_NoFrameForm, 2,
				    ((USER_FRAME *)comp)->form->name, 
				    comp->name); 
		break;

	  case E_NoFormForm:
		IIUGerr(E_NoFormForm, UG_ERR_ERROR, 2, 
			comp->name, comp->appl->name);
		break;

 	  case E_BadComp:
		/* User chose not to recompile something which failed 
		** previously 
		*/
		;	/* Fall through */

 	  case FAIL:
 		/* Compilers return FAIL on error, e.g., OSL (JupBug #3327) or
		** host-language (#7444,) but no error message is associated
		** with FAIL.  E_AB0120 could be printed out in this case, but
		** users have complained about that since it is redundant.
		*/
		comp->compdate = STtalloc(comp->data.tag, UGcat_now());
		updframe = TRUE;
		break;

	  case OK:
		comp->compdate = ERx("");
		comp->flags &= ~APC_INVALID;
		updframe = TRUE;
		break;

	  case UT_CO_SUF_NOT:
		*status = E_AB0134_UnknownExt;
		/* fall through ... */
	  default:
		comp->compdate = STtalloc(comp->data.tag, UGcat_now());
		updframe = TRUE;
		IIUGerr(*status, UG_ERR_ERROR, 1, srcfile);
		break;
	}

	/* Update component, if need be */
	if (ismeta && !updframe)
	{
		METAFRAME *m = ((USER_FRAME *)comp)->mf;

		updframe = m->state != ostate ||
			   STcompare(m->gendate, ogdate) != 0 ||
			   STcompare(m->formgendate, ofdate) != 0;
	}

	updframe = updframe || comp->flags != oflags || 
		   STcompare(comp->compdate, tdt) != 0;	
	if (updframe)
	{
		IIAMufqUserFrameQuick(comp);
	}

	/* Give user trace message */
	if (*status == E_BadComp)
		msg = S_AB001D_BadCompile;
	else if (*status != OK)
		msg = S_AB0013_CompFailed;
	else if (!compiled)
		msg = S_AB0015_NoComp;
	else if (0 == (comp->flags & APC_OSLWARN))
		msg = S_AB0014_CompWorked;
	else
		msg = S_AB002F_CompWorkedWarnings;
	IIABdcmDispCompMsg(msg, FALSE, 0);

	return compiled;
}


STATUS	IIUTlang();
STATUS	IIUTdml();

#define UT_NODML	0
#define UT_QUEL		1
#define UT_SQL		2

/*
** History:
**	11/89 (jhw)  Check for FAIL, which has no error message associated
**			with it.  JupBug #8627.  Also, use PATH&FILENAME
**			to support relative pathnames.
**	25-feb-93 (connie)
**		Use an empty string in the 2nd argument of error 
**		E_AB0136_UnsupportedExt since text returned from ERreport
**		is irrelevant to the status from LOfroms & LOdetail.
**	25-may-93 (blaise)
**		Deleted commented out code.
*/	
bool
iiabCkExtension ( char *file, i4 lang )
{
	STATUS		rval;
	i4		src_lang;
	LOCATION	temp;
	char		buf[MAX_LOC + 1];
	char		lbuf[MAX_LOC + 1];

	if ( file == NULL || *file == EOS )
	{
		IIUGerr( E_AB003D_no_file, UG_ERR_ERROR, 0 );
		return FALSE;
	}
	STcopy(file, lbuf);
	if ( (rval = LOfroms(PATH&FILENAME, lbuf, &temp)) != OK  ||
		    (rval = LOdetail(&temp, buf, buf, buf, buf, buf)) != OK )
	{ /* bad file name, shouldn't happen */
		char	errbuf[ER_MAX_LEN+1];

		errbuf[0] = EOS;
		IIUGerr( E_AB0136_UnsupportedExt, UG_ERR_ERROR,
				2, file, errbuf
		);
		return FALSE;
	}
	else if ( (rval = IIUTlang(file, &src_lang)) != OK )
	{ /* Error */

		if ( rval == UT_CO_SUF_NOT )
		{
			IIUGerr( E_AB0134_UnknownExt, UG_ERR_ERROR, 1, file);
		}
		else
		{
			char	errbuf[ER_MAX_LEN+1];

			if ( rval != FAIL )
				ERreport(rval, errbuf);
			else
				errbuf[0] = EOS;
			IIUGerr( E_AB0136_UnsupportedExt, UG_ERR_ERROR,
					2, file, errbuf
			);
		}
		return FALSE;
	}
	else if ( src_lang != lang )
	{ /* Doesn't match specified language */
		if ( lang == OLOSL )
			IIUGerr( E_AB026A_OSL_extension, UG_ERR_ERROR, 0 );
		else
			IIUGerr( E_AB0135_IllegalExt, UG_ERR_ERROR, 1, file );
		return FALSE;
	}
	return TRUE;
}

/*{
** Name:	iiabExtDML() -	Return the DML of a proc's file.
**
** Description:
**	Given a files, checks the extension and returns the DML for
**	the extension.
**
** Inputs:
**	file	The file.
**
** Returns:
**	{DB_LANG}  The DML for the file.
**
** History:
**	12/16/85 (joe)
**		Written.
**	02/89 (jhw) -- Renamed from 'abcomdml()'.
**	09/89 (jhw) -- Abstracted 'IIUTdml()'.
*/

DB_LANG
iiabExtDML ( file )
char	*file;
{
	i4	dml;

	if ( IIUTdml( file, &dml ) == OK && dml != UT_NODML )
		return dml == UT_SQL ? DB_SQL : DB_QUEL;

	return DB_NOLANG;
}

/*
** Name:    oslCompile()
**
**	Compile an 4GL source file if the new OSL is installed.
**
** History:
**	26-jul-1991 (mgw) Bug #38867
**		changed LOdelete(xfloc) to LOdelete(&xfloc)
*/
static bool
oslCompile ( comp, options, errfile, status, ismeta  )
APPL_COMP	*comp;
i4		options;
LOCATION	*errfile;
STATUS		*status;
bool		ismeta;
{
	bool		compiled;
	char		*file;
	bool		object_code;
	bool		calldeps;
	LOCATION	*srcloc;
	char		arg_tmp[ABBUFSIZE];
	UTARGS		exe;
	LOCATION	*p_serr;
	LOCATION	serr;
	char		sbuf[MAX_LOC+1];
	PTR		xflagptr;	/* in case db connection not made */

	LOCATION	*iiabMkLoc();

	object_code = (bool)(options & ABF_OBJECTCODE);
	calldeps = (bool)(options & ABF_CALLDEPS);

	if ( comp->class == OC_OSLPROC )
	{
		file = ((_4GLPROC *)comp)->source;
	}
	else
	{
		register USER_FRAME	*frm = (USER_FRAME *)comp;

		file = frm->source;

		if ((comp->appl == NULL) ||
			(comp->appl->ooid == 0))    /* 6-x_PC_80x86 */
			return(TRUE);

		if ( frm->class == OC_OLDOSLFRAME )
		{
			frm->class = OC_OSLFRAME;
			frm->data.dirty = TRUE;
		}
	}

	srcloc = iiabMkLoc(comp->appl->source, file);

	/*
	** set up UTexe arguments.
	*/

	exe.cmd_line = arg_tmp;
	exe.arg_cnt = 0;

	/* first 4 args are invariant (plus "file=%L" added by IIUT.) */
	STcopy(ERx("database = %S, equel = %S, appid=%N, frame=%S"), 
		exe.cmd_line);
	exe.args[exe.arg_cnt++] = (PTR) *IIar_Dbname;
	exe.args[exe.arg_cnt++] = xflagptr = (PTR) IIxflag();
	exe.args[exe.arg_cnt++] = (PTR) comp->appl->ooid;
	exe.args[exe.arg_cnt++] = (PTR) comp->name;

	if ( errfile != NULL )
	{ /* Catch error listing. */
		_VOID_ STcat(arg_tmp,ERx(ismeta ?",vqerrors=%L" :",errors=%L"));
		exe.args[exe.arg_cnt++] = (PTR)errfile;
		_VOID_ LOdelete(errfile);
	}
	if ( IIabWopt )
	{ /* turn off OSL warnings */
		_VOID_ STcat(arg_tmp, ERx(", nowarning = %S"));
		exe.args[exe.arg_cnt++] = (PTR) ERx("");
	}

	if ( IIabOpen )
	{ /* turn on warnings about non-Open SQL */
		_VOID_ STcat(arg_tmp, ERx(", open = %S"));
		exe.args[exe.arg_cnt++] = (PTR) ERx("");
	}

	if ( IIabC50 )
	{ /* turn on 5.0 compatibility mode */
		_VOID_ STcat(arg_tmp, ERx(", comp50 = %S"));
		exe.args[exe.arg_cnt++] = (PTR) ERx("");
	}

	/* Redirect stderr/stdout to a temporary file. */
	p_serr = NULL;
	if ( errfile != NULL && NMloc(TEMP, PATH, (char *)NULL, &serr) == OK
			&& LOuniq(ERx("err"), ERx("out"), &serr) == OK )
	{
		LOcopy(&serr, sbuf, &serr);
		p_serr = &serr;
	}

	if ( object_code )
	{
		*status = IIUTcompile((char*)NULL, srcloc, 
				S_AB0040_Compiling, &exe, p_serr, &compiled
		);
	}
	else
	{
		*status = IIUTdbCompile(srcloc, S_AB0040_Compiling, &exe, 
					p_serr, &compiled);
	}

	if (compiled)
	{
		/* 
		** If it compiled successfully, but there's a listing file, 
		** there must have been compilation errors.
		*/
		if ( (OK == *status) && (errfile != NULL) && 
		     (OK == LOexist(errfile)) )
			comp->flags |= APC_OSLWARN;
		else
			comp->flags &= ~APC_OSLWARN;

		/* 
		** Reload the dependencies, since the compiler has written
		** new ones.
		*/
		_VOID_ IIAMdmrReadDeps(comp->appl->ooid, comp->ooid, 
				       !calldeps, (VOID (*)())NULL, comp->appl);
	}
		

	/* Append the stdout/stderr file to the listing file */
	if ( errfile != NULL && p_serr != NULL )
	{
		_VOID_ IIUGafAppendFile(errfile, p_serr, "\n----------\n");
		_VOID_ LOdelete(p_serr);
	}

	/* 
	** If there's no error file, make one.  This should only happen
	** if the 4GL compiler couldn't start up or if it crashed.
	**
	** Bug 36391 - in this case, delete the gca save file since
	** the connection was never made and thus won't need to be
	** restored.
	*/
	if ( *status != OK && errfile != NULL && (LOexist(errfile) != OK) )
	{	
		i4 lstatus = *status;
		LOCATION xfloc;

		LOfroms(PATH&FILENAME, xflagptr, &xfloc);
		LOdelete(&xfloc);
		make_errlis(errfile, S_AB02F4_CompFailure, 1, (PTR)&lstatus);
	}

	if ( errfile != NULL )
		PEworld(ERx("+r+w"), errfile);

	if (compiled && ! ismeta)
	{
		comp->flags &= ~CUSTOM_EDIT;
	}

	if ( *status == FAIL )
	{
		/* Special hack (BUGFIX 3327).  The UT call failed or the
		** compiler returned a non-success status.  We don't know
		** which, we can't guess why.  The message associated with
		** FAIL (associated purely by coincidence!) is totally
		** inappropriate, so we'll try to spruce it up a bit...
		*/

		*status = E_CompErrs;
	}

	return compiled;
}

/*{
** Name:	IIABmefMakeErrFile() -	Make an error file location.
**
** Description:
**
**	Make location for error listing file.  This only really matters
**	for 3GL and 4GL files, but for compatibility with earlier logic, we make
**	sure that a location is filled in.
**
** Inputs:
**	comp	component
**	eloc	location structure to fill in.
**	buf	buffer to associate with eloc.
**
** Outputs:
**	filled in eloc.
**
** Returns:
**	{STATUS}  OK, no errors.
**
** History:
**	18-Jan-93 (fredb) hp9_mpe
**		MPE/iX porting changes: Limited name space and flat 'directory'
**		structure force us to make different LO calls to generate the
**		file location.
**	20-May-1993 (fredb)
**		IFDEF my prior changes to LO buffer sizes.  The relationship
**		between the defined sizes is different between MPE & UNIX.
**		Bug #51560
**	1-jun-1994 (mgw) Bug #57253
**		Don't call iiabMkObjLoc() with an empty fname.
*/

STATUS
IIABmefMakeErrFile(comp,eloc,buf)
APPL_COMP *comp;
LOCATION *eloc;
char *buf;
{
	STATUS		rc;
	char		*fname;
	LOCATION	*oloc;
	LOCATION	tloc;
#ifdef hp9_mpe
	char		*cptr;
	char		path[LO_PATH_MAX+1];
	char		base[LO_FPREFIX_MAX+1];
#else
	char		base[LO_NM_LEN+1];
#endif
	char		junk[MAX_LOC + 1];

	switch ( comp->class )
	{
	  case OC_MUFRAME:
	  case OC_APPFRAME:
	  case OC_UPDFRAME:
	  case OC_BRWFRAME:
	  case OC_OLDOSLFRAME:
	  case OC_OSLFRAME:
		fname = ((USER_FRAME *)comp)->source;
		break;
	  case OC_OSLPROC:
		fname = ((_4GLPROC *)comp)->source;
		break;
	  case OC_HLPROC:
		fname = ((HLPROC *)comp)->source;
		break;
	  default:
		fname = NULL;
		break;
	}

	if (fname != NULL && *fname != EOS)
	{
		oloc = iiabMkObjLoc(fname);
		if (oloc == NULL)
			return FAIL;
#ifdef hp9_mpe
		if ((rc = LOdetail(oloc, junk, path, base, junk, junk)) != OK )
#else
		if ((rc = LOdetail(oloc, junk, junk, base, junk, junk)) != OK )
#endif
			return rc;

#ifdef hp9_mpe
	/*
	** Use 'buf' and 'eloc' instead of temp vars; set a pointer to
	** the account name in "path"; build a file.group.account string;
	** and convert it to a location which will be returned to the
	** caller.
	*/
		if (cptr = STrchr(path, '.'))
		{
			cptr++;		/* advance past "." */
		}
		else
		{
			cptr = path;	/* path was account only */
		}
		_VOID_ STprintf(buf, ERx("%s%s.%s"), base, ABLISEXTENT, cptr);
		if ((rc = LOfroms (PATH & FILENAME, buf, eloc)) != OK)
			return rc;
#else
		_VOID_ STprintf(junk, ERx("%s%s"), base, ABLISEXTENT);
		_VOID_ LOfstfile(junk, oloc);
		LOcopy(oloc,buf,eloc);
#endif
	}
	else
	{
		if ((rc = NMloc(TEMP, PATH, (char *)NULL, &tloc)) != OK ||
			(rc = LOuniq(ERx("er"), ERx("lst"), &tloc)) != OK )
				return rc;
		LOcopy(&tloc, buf, eloc);
	}

	return OK;
}

/*
**	make_errlis
**
**	Construct a listing file from a message.
*/
static VOID
make_errlis(lisfile, errmsg, numargs, a1, a2, a3, a4)
LOCATION *lisfile;	/* Listing file */
ER_MSGID errmsg;	/* Error message */
i4	numargs;	/* Number of arguments for message */
PTR	a1, a2, a3, a4;	/* Message arguments */
{
	FILE *fp;
	char buff[256];

	/* 
	** This is a last-ditch effort to construct a listing file.
	** if anything fails, we'll just give up.
	*/
	if (SIfopen(lisfile, ERx("w"), SI_TXT, (i4)0, &fp) != OK)
		return;
	IIUGfmt(buff, sizeof(buff), ERget(errmsg), numargs, a1, a2, a3, a4);
	_VOID_ SIfprintf(fp, ERx("%s\n"), buff);
	_VOID_ SIclose(fp);
}
