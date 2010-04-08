/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<st.h>
# include	<er.h>
# include	<tm.h>
# include	<lo.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<abfcnsts.h>
# include	<abfcompl.h>
# include	<abclass.h>
# include	<oocat.h>
# include	<metafrm.h>
# include	<dmchecks.h>
# include       <stdprmpt.h>
# include	"abut.h"
# include	"abfglobs.h"
# include	"erab.h"
# include	<abfdbcat.h>

/**
** Name:	abcomchk.c  - Compilation checks
**
** Description:
**	This file defines:
**
**	IIABccsChkCompStatus	Check whether a frame/proc must be recompiled
**	iiabGenCheck		Check whether an Emerald frame must be regen'ed
**
** History:
**	7/90 (Mike S)	Initial version
**	27-mar-91 (rudyw)
**	    Separate declaration and initialization of automatic array Sdstr
**	    since some compilers do not allow it.
**	02-dec-91 (leighb) DeskTop Porting Change:
**		Added routines to pass data across ABF/IAOM boundary.
**	6-Feb-92 (blaise)
**		Added include abfglobs.h.
**	28-feb-1992 (mgw) Bug #41111
**		Replaced calls to S_AB0146_ReturnChange and
**		S_AB0148_FrameChange with calls to S_AB0156_NewReturnChange
**		and S_AB0157_NewFrameChange respectively so these messages
**		will refer to frames or procedures as appropriate instead of
**		just frames.
**	10-jul-92 (leighb) DeskTop Porting Change:
**		Make sure we are in forms mode before prompting user.
**	24-nov-93 (rudyw) Bug 51091
**		Add an additional check in iiabGenCheck to determine if there
**		is truly a custom situation before proceeding as if custom.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**    25-Oct-2005 (hanje04)
**        Add prototype for bad_comp() to prevent compiler
**        errors with GCC 4.0 due to conflict with implicit declaration.
**/

/* # define's */

/* GLOBALDEF's */
/* extern's */
FUNC_EXTERN LOCATION 	*iiabMkLoc();
FUNC_EXTERN LOCATION 	*iiabMkFormLoc();
FUNC_EXTERN STATUS 	iiabGenCheck();

GLOBALREF bool 		IIabVision;
GLOBALREF bool		IIabfmFormsMode;		 

/* static's */
static STATUS file_time();
static STATUS object_file_time();
static STATUS bad_comp(
			USER_FRAME	*comp,
			LOCATION	**srcloc,
			SYSTIME		*srcTime);


/*{
** Name:  IIABccsChkCompStatus    Check whether a frame/proc must be recompiled
**
** Description:
**	Check whether a frame or procedure is out of date with respect to
**	any of the things it depedns on.
**
** Inputs:
**	comp	APPL_COMP *	frame or procedure to check.
**	options	i4		Compilation options
**	genqry	bool		Should we ask about regeneration
**
** Outputs:
**	none
**
**	Returns:
**		STATUS		OK	It's OK -- need not recompile
**				-1	recompile
**				FAIL	failure in regeneration
**				E_NoComp Compilation is undefined
**				E_NoSrcFile No source file found
**				E_NoFrameForm No form found for a frame
**				E_NoFormForm No form found for a compiled form
**
**	Exceptions:
**		none
**
** Side Effects:
**
**	Messages are output to let the user know what we're doing.
**
** History:
**	7/90 (Mike S) Initial version
*/
STATUS
IIABccsChkCompStatus(comp, options, genqry)
APPL_COMP	*comp;
i4		options;
bool		genqry;
{
	LOCATION	*srcloc = NULL;
	char		*srcfile;	/* Source file name */
	SYSTIME		srcTime;
	SYSTIME		formTime;
	SYSTIME		objTime;
	SYSTIME		ILtime;
	char		formDate[OODATESIZE+1];
	char		objDate[OODATESIZE+1];
	OOID		fid;
	DM_DTCHECK_ARG	dtcheck;
        OOID            dummy;
	ER_MSGID	msg;
	ER_MSGID	objtyp = F_AB02FE_Procedure;
	bool		regen = FALSE;
	bool		ismeta = FALSE;
	bool		object = (options & ABF_OBJECTCODE) != 0;
	bool		force = (options & ABF_FORCE) != 0;

	switch (comp->class)
	{
	    case OC_MUFRAME:
	    case OC_APPFRAME:
	    case OC_BRWFRAME:
	    case OC_UPDFRAME:
		if (IIabVision)
		{
			STATUS status;

			ismeta = TRUE;
			IIABdcmDispCompMsg(S_AB0016_Processing, FALSE, 2, 
					   ERget(F_AB02FF_Frame), comp->name);

			/* See if anything must be regenerated */
			status = iiabGenCheck((USER_FRAME *)comp, genqry, 
					      &regen);
			if (status == FAIL)
				return FAIL;	/* Failure; don't recompile */
			else if (status == FE_Cancel)
				return OK;	/* Don't recompile */
			else if (regen)
				return -1;	/* Code or form was regen'ed */
			/* Fall through */
		}

	    case OC_OSLFRAME:
	    case OC_OLDOSLFRAME:
		objtyp = F_AB02FF_Frame;	/* continue falling through */

	    case OC_OSLPROC:
		if (!ismeta)
		{
			IIABdcmDispCompMsg(S_AB0016_Processing, FALSE, 2,
					  ERget(objtyp), comp->name);
		}

		if (!regen && !object && *(comp->compdate) != EOS)
		{
			STATUS status;

			status = bad_comp((USER_FRAME *)comp, &srcloc, &srcTime);
			if (status == E_NoSrcFile)
				return E_NoSrcFile;
			else if (status != OK)
				return status;	/* Don't recompile */
		}

		/* Check invalidation bits */
		if ((comp->flags & APC_RECOMPILE) != 0)
		{
			if ((comp->flags & APC_RETCHANGE) != 0)
			{
				msg = S_AB0156_NewReturnChange;
				IIABdcmDispCompMsg(msg, FALSE, 1, ERget(objtyp));
			}
			else if ((comp->flags & APC_GLOBCHANGE) != 0)
			{
				msg = S_AB0147_GlobChange;
				IIABdcmDispCompMsg(msg, FALSE, 0);
			}
			else 
			{
				msg = S_AB0157_NewFrameChange;
				IIABdcmDispCompMsg(msg, FALSE, 1, ERget(objtyp));
			}
			return -1;
		}

		/* Get IL date */
		if (comp->class == OC_OSLPROC)
			fid = ((_4GLPROC *)comp)->fid;
		else
			fid = ((USER_FRAME *)comp)->fid;

		if (IIAMfeFrmExist(fid, &ILtime) != OK )
		{
			IIABdcmDispCompMsg(S_AB0024_NoFID, FALSE, 0);
			return -1;
		}

		/* Compare IL to form (except for OSL procs, of course) */
		if (comp->class != OC_OSLPROC)
		{
			STATUS status;

			status = IIAMfoiFormInfo(
					((USER_FRAME *)comp)->form->name,
					formDate, &dummy);
			if (status != OK || *formDate == EOS)
			{
				/* No form */
				return E_NoFrameForm;
			}
			IIUGdtsDateToSys(formDate, &formTime);
			if (TMcmp(&ILtime, &formTime) < 0)
			{
				IIABdcmDispCompMsg(S_AB002E_NewForm, FALSE, 0);
				return -1;
			}
		}

		/*
		** Check TYPE OF dependencies vs. IL date
		*/
		dtcheck.recompile = FALSE;
		dtcheck.frame_time = &ILtime;
		IIAMxdsDepSearch(comp, IIAM_zdcDateChange, (PTR)&dtcheck);
		if (dtcheck.recompile)
		{
			IIABdcmDispCompMsg(dtcheck.msg, FALSE, 1, dtcheck.name);
			return -1;
		}

		/* 
		** check IL date against source file date.
		*/
		if (srcloc == NULL)
		{
			/* Get source date, if we haven't */
			srcfile = ((USER_FRAME *)comp)->source;
			srcloc = iiabMkLoc(comp->appl->source, srcfile);
			if (srcloc == NULL)
				return E_NoSrcFile;
			if (file_time(srcloc, &srcTime) != OK)
				return E_NoSrcFile;
		}

		if (TMcmp(&ILtime, &srcTime) <= 0)
		{
			IIABdcmDispCompMsg(S_AB0142_OldIL, FALSE, 0);
			return -1;
		}

		/* If we're not compiling to object, we're done */
		if (!object)
			return OK;

		/* One last check: IL time vs. object time */
		if (object_file_time(srcloc, &objTime) != OK)
		{
			/* No object file */
			IIABdcmDispCompMsg(S_AB0143_NoObject, FALSE, 0);
			return -1;
		}
		if (TMcmp(&objTime, &ILtime) < 0)
		{
			/* IL newer than object */
			IIABdcmDispCompMsg(S_AB0145_OldObject, FALSE, 0);
			return -1;
		}
		return OK;
		break;

	    case OC_AFORMREF:
	    case OC_FORM:
		/* 
		** Form.  Unless we're making an image, there's nothing to do.
		** If anything is out-of-date, we'll both create a compiled for
		** and compile it to object.
		*/
		if (!object)
			return OK;

		IIABdcmDispCompMsg(S_AB0016_Processing, FALSE, 2,
					ERget(F_AB02FA_Form), comp->name);
		if ((srcfile = ((FORM_REF *)comp)->source) == NULL ||
		     *srcfile == EOS)
		{
			/* No source file defined */
			return E_NoComp;
		}
		if ((srcloc = iiabMkFormLoc(srcfile)) == NULL)
			return E_NoSrcFile;     /* No source file */

		if (IIAMfoiFormInfo(comp->name, formDate, &dummy) != OK || 
		    *formDate == EOS)
		{
			/* No form */
			return E_NoFormForm;
		}
		IIUGdtsDateToSys(formDate, &formTime);

		if (object_file_time(srcloc, &objTime) != OK)
		{
			/* No object file */
			IIABdcmDispCompMsg(S_AB0143_NoObject, FALSE, 0);
			return -1;
		}
		if (TMcmp(&objTime, &formTime) < 0)
		{
			/* Form newer than object */
			IIABdcmDispCompMsg(S_AB0149_OldObject, FALSE, 0);
			return -1;
		}
		return OK;	/* We're up-to-date */
		break;

# ifdef COMPILE_REPORTS
	    /* Skip compiling reports until we can:
	    ** 1. Redirect sreport's output into a file.
	    ** 2. Display this file as a listing file on the error screen.
	    ** 3. Decide what to do with an image build when report 
	    ** compilation fails.
	    */
	    case OC_RWFRAME:
		/* 
		** Report frame.  If there's a source file, check it against
		** the report in the database.
		*/
		object = FALSE;
		srcfile = ((REPORT_FRAME *)comp)->source;
		if (srcfile == NULL || *srcfile == EOS)
			return E_NoComp;

		IIABdcmDispCompMsg(S_AB0016_Processing, FALSE,
				2, ERget(F_AB02FF_Frame), comp->name);
		srcloc = iiabMkLoc(comp->appl->source, srcfile);
		if (srcloc == NULL || file_time(srcloc, &srcTime) != OK)
			return E_NoSrcFile;	

		if (IIAMgoiGetObjInfo(((REPORT_FRAME *)comp)->report, OC_REPORT,
					objDate, &dummy) != OK)
			return -1;	/* Something's wrong.Better recompile */
		
		if (*objDate == EOS)
		{
			/* No report in database */
			IIABdcmDispCompMsg(S_AB0140_ForceReport, FALSE, 0);
			return -1;
		}
		IIUGdtsDateToSys(objDate, &objTime);
		if (TMcmp(&objTime, &srcTime) < 0)
		{
			IIABdcmDispCompMsg(S_AB0141_DoReport, FALSE, 0);
			return -1;
		}
		else
			return OK;
# endif

	    case OC_HLPROC:
		/* 3GL proc.  The only test here is source vs. object */
		IIABdcmDispCompMsg(S_AB0016_Processing, FALSE, 2,
				ERget(F_AB02FE_Procedure), comp->name);
		srcfile = ((HLPROC *)comp)->source;
		if (srcfile == NULL || *srcfile == EOS)
		{
			/* It's a library procedure */
			return OK;
		}
		if ((srcloc = iiabMkLoc(comp->appl->source, srcfile)) == NULL)
			return E_NoSrcFile;     /* No source file */
		break;

	    default:
	    /* These types can't compile */
	    	return E_NoComp;
		break;
	}

	/* 
	** The only check left to do is object file vs. source file 
	** We're a 3GL proc.
	*/
	if (file_time(srcloc, &srcTime) != OK)
		return E_NoSrcFile;

	if (object_file_time(srcloc, &objTime) != OK)
	{
		/* No object file */
		IIABdcmDispCompMsg(S_AB0143_NoObject, FALSE, 0);
		return -1;
	}
	if (TMcmp(&objTime, &srcTime) < 0)
	{
		/* Source newer than object */
		IIABdcmDispCompMsg(S_AB0144_OldObject, FALSE, 0);
		return -1;
	}
	return OK;
} 

/*{
** Name:	iiabGenCheck - Check whether an Emerald frame must be regen'ed
**
** Description:
**	See whether the code and/or form must be regen'ed.  This is mostly a
**	mattter of looking at bits; we also do date checks to see whether
**	the existing code should be overwritten, and the form rewritten, fixed
**	up, or left alone.
**
**	Note than FE_Cancel can only be returned if ask is TRUE.
**
** Inputs:
**	comp		USER_FRAME *	Frame to examine
**	ask		bool		Whether an explicit compilation was
**					requested
**
** Outputs:
**	changes		bool *		Was anything regen'ed
**
**	Returns:
**			OK		If nothing failed
**			FAIL		If regeneration failed
**			FE_Cancel	If user chose not to recompile a 
**					custom frame		
**
**	Exceptions:
**		none
**
** Side Effects:
**		Form and/or code may be regen'ed
**
** History:
**	8/90 (Mike S)	Moved from abfcom.c; interface changed
**	10/90 (Mike S)	Make "No" the default for overwriting custom frames
**			and custom menu forms.
**	8/91 (Mike S)	Do the right thing when formgen date and form date
**			are identical.
**	24-nov-93 (rudyw) Bug 51091
**		Check the frame source generation date currently in the
**		database against the source file date in determining if
**		there is a custom source situation.
*/
STATUS
iiabGenCheck(comp, ask, changes)
USER_FRAME *comp;
bool ask;
bool *changes;
{
        LOCATION        floc;
        char            flocbuf[MAX_LOC+1];
        LOINFORMATION   flocinfo;
        SYSTIME         stime;
        SYSTIME         ftime;
        SYSTIME         gtime;
        i4              floiflags;
        METAFRAME       *mf;
        STATUS          rc;
        OOID            dummy;
        ER_MSGID        fupid;
	char		Fdstr[OODATESIZE+1];
	bool		custom;

	/* Assume no changes */
	*changes = FALSE;

        LOcopy(iiabMkLoc( comp->appl->source, comp->source ), flocbuf, &floc);
        mf = comp->mf;

        /* clear "new" flag unconditionally on first gen attempt */
        mf->state &= ~MFST_NEWFR;
 
        /*
        ** if source exists, & more recent than gendate, user
        ** has been editting it.  Unless user is specifically
        ** requesting a compile (ask = TRUE), do nothing.
        ** (if source file doesn't exist, clear gendate so that
        ** we "never generated")
        */
        if (LOexist(&floc) == OK)
        {

                floiflags = LO_I_LAST;
 
                /*
                ** if we can't get file date, assume NOW so that
                ** we don't clobber a user's changes.
                */
                if (LOinfo(&floc, &floiflags, &flocinfo) != OK)
                        TMnow(&(flocinfo.li_last));
 
                IIUGdtsDateToSys(mf->gendate,&stime);

		custom = FALSE;
                if (TMcmp(&stime,&(flocinfo.li_last)) < 0 )
		{
			char buf[ABFARG_SIZE+1];

			custom = TRUE;
			buf[0] = '\0';
			/*
			** Get the current generation date value from the
			** database in case another user generated source
			** after this session started.  Equality between db
			** value and source time signifies process's gendate
			** is out of date rather than custom situation. 51091
			*/
			if ( IIAMggdGetGenDate(comp->appl->ooid,
					             comp->ooid,
					                    buf) == OK )
			{
                		IIUGdtsDateToSys(buf,&gtime);
				if (TMcmp(&gtime,&(flocinfo.li_last)) == 0)
					custom = FALSE;
			}
		}

                if ( custom == TRUE )
                {
			i4	ccans;

                        /* By definiton, a custom frame */
                        comp->flags |= CUSTOM_EDIT;
 
                        if (!ask)
                        {
                                /*
                                ** If DOGEN is set, tell him that VQ chnages
                                ** conflict with user changes
                                */
                                if ((mf->state & MFST_DOGEN) != 0)
                                {
                                        IIABdcmDispCompMsg(
                                                S_AB02F5_CustomVQchanges,
                                                FALSE, 1, ERget(FE_Compile));
                                }
                                return OK;
                        }
 
			ccans = IIUFccConfirmChoice(CONF_GENERIC,
                                        comp->name, ERx(""),
                                        ERget(F_AB02F0_CustRegenHelp),
                                        ERx("abcustrg.hlp"),
                                        F_AB02F1_CustRegenTitle,
                                        F_AB02F3_CustRegenNo,
                                        F_AB02F2_CustRegenYes,
                                        ERx(""), TRUE);

			switch (ccans)
			{
			    case CONFCH_YES:
				return OK;	 	/* Don't regenerate */
			    
			    case CONFCH_CANCEL:
				return FE_Cancel;	/* Don't recompile */

			    default:
				break;			/* Keep going */
			}
                }
        }
        else
        {
                mf->gendate = ERx("");
        }
 
        /*
        ** Now check form date.  This returns an empty string for non-existent
        ** form.
        */
        if (IIAMfoiFormInfo((comp->form)->name, Fdstr, &dummy) != OK)
                return FAIL;
 
        if (Fdstr[0] != EOS)
        {
                fupid = S_AB0276_Form;
 
                if (mf->formgendate == NULL || *(mf->formgendate) == EOS)
                {
                /*
                ** unusual situation - we somehow have a form present without
                ** having generated it.  If happens, fix ourselves up by
                ** simply setting the generation date as the form date.
                **
                ** This is a weird situation, so I'm not going to worry about
                ** recovering the memory.
                */
                        mf->formgendate = STalloc(Fdstr);
                }
        }
        else
        {
                fupid = S_AB00AB_ReForm;
        }
 
        /*
        ** set bits for operations we have to do.  If forced,
        ** we do everything.  Otherwise, we do it only if already
        ** indicated, or if we've never done it.
        */
        if (ask || Fdstr[0] == EOS)
                mf->state |= MFST_DOFORM;
        if (ask || *(mf->gendate) == EOS)
                mf->state |= MFST_DOGEN;
 
        /*
        ** clear the "bad" bits for any operations we are going to do.
        ** If bad bits remain, return fail, since attempts to do anything
        ** will apparently be futile.
        */
        if (mf->state & MFST_DOGEN)
                mf->state &= ~MFST_GENBAD;
        if (mf->state & (MFST_VQEERR | MFST_GENBAD))
        {
                IIUGerr( E_AB027B_BadGen, 0, 1, comp->name );
                return FAIL;
        }
 
        /*
        ** form operation - if form exists, and has a date later than
        ** the form generation date, we fix it up.  Otherwise, we
        ** generate it.  Converting these dates both into SYSTIME's
        ** is probably more wasteful than simply comparing the two
        ** date strings, but the latter would involve ADT calls, too.
        **
        ** "Classic" Menu frames can't be fixed up; we either redo it or leave 
	** it as is.
        */
        if (mf->state & MFST_DOFORM)
        {
                /*
                ** Make sure all the pieces are here.  Assume that setting
                ** undefined bits in second argument doesn't matter.  Also
                ** assumes that FrameFlow / VQ edit leaves catalog up-to-date
                ** or popmask bits set.
                */
                IIAMmpMetaPopulate(mf, ~(mf->popmask));
 
                if (Fdstr[0] != EOS)
                {
                        IIUGdtsDateToSys(Fdstr,&ftime);
                        IIUGdtsDateToSys(mf->formgendate,&stime);
                }
 
		/*
		** On an incredibly fast machine, the form's date may 
		** equal the formgen date for a newly generated form, so
		** we must use '<=' in the test to avoid fixing up an
		** unedited form.
		*/
                if (Fdstr[0] == EOS || TMcmp(&ftime,&stime) <= 0)
                {
                        IIABdcmDispCompMsg(fupid, TRUE, 1, comp->name);
                        rc = IIFGmdf_makeDefaultFrame( mf, FALSE );
			if (rc == OK)
				*changes = TRUE;
                }
                else
                {
                        if (comp->class == OC_MUFRAME && 
			    ((comp->flags & APC_MS_MASK) == APC_MS1LINE))
                        {
				i4	ccans;

				rc = OK;
                                if (ask)
				{
					ccans = 
					    IIUFccConfirmChoice(CONF_GENERIC,
                                            comp->name, ERx(""),
                                            ERget(F_AB02F5_MRegenHelp),
                                            ERx("abmurg.hlp"),
                                            F_AB02F4_MRegenTitle,
                                            F_AB02F7_MRegenNo,
                                            F_AB02F6_MRegenYes,
                                            ERx(""), TRUE);

					switch (ccans)
					{
			    		    case CONFCH_YES:
						/* Don't regenerate */ 
						rc = OK;
						break;
			    
			    		    case CONFCH_CANCEL:
						/* Don't recompile */
						return FE_Cancel;

					    default:
						/* Regenerate form */
                                        	IIABdcmDispCompMsg(fupid, 
							TRUE, 1, comp->name);
                                        	rc = IIFGmdf_makeDefaultFrame(
                                                                mf, FALSE);
						if (rc == OK)
							*changes = TRUE;
					}
                                }
                        }
                        else
                        {
                                IIABdcmDispCompMsg(S_AB00AA_Form, TRUE,
                                                   1, comp->name);
                                rc = IIFGfuf_fixUpForm(mf, FALSE);
				if (rc == OK)
					*changes = TRUE;
                        }
                }
 
                if (rc != OK)
                {
                        return FAIL;
                }
 
                mf->state &= ~MFST_DOFORM;
        }
 
        if (mf->state & MFST_DOGEN)
        {
                char    Sdstr[OODATESIZE+1];
                Sdstr[0]='\0';
 
                IIABdcmDispCompMsg(S_AB0277_Code, TRUE, 1, comp->name);
                /*
                ** If this was done in previous statement, this becomes
                ** a no-op.  We don't simply call it unconditionally to
                ** avoid unneeded access.
                */
                IIAMmpMetaPopulate(mf, ~(mf->popmask));
 
                /* Purge unneeded escape code */
                _VOID_ IIVQpecPurgeEscapeCode(mf, TRUE);
 
                rc = IIFGstart(mf);
                if (rc != OK)
                {
                        return FAIL;
                }
		*changes = TRUE;
                mf->state &= ~MFST_DOGEN;
                comp->flags &= ~CUSTOM_EDIT;
 
                /*
                ** get actual date of new file, so we aren't sensative
                ** to clock differences between host and file server.
                */
                floiflags = LO_I_LAST;
 
                /*
                ** if we can't get file date, simply use host date anyway
                */
                if (LOinfo(&floc, &floiflags, &flocinfo) != OK)
                        TMnow(&(flocinfo.li_last));
 
                IIUGdfsDateFromSys(&(flocinfo.li_last), Sdstr);
                mf->gendate = STtalloc(comp->data.tag, Sdstr);
                comp->compdate = ERx("");
        }
 
        return OK;
}


/*
**	Get the modification time of a file.
*/
static STATUS
file_time(file, time)
LOCATION *file;
SYSTIME	*time;
{
	i4 loflags = LO_I_LAST;
	LOINFORMATION   locinfo;
	STATUS	status;

	status = LOinfo(file, &loflags, &locinfo);
	if (status != OK || (loflags & LO_I_LAST) != LO_I_LAST)
	{
		return FAIL;
	}
	else
	{
		STRUCT_ASSIGN_MACRO(locinfo.li_last, *time);
		return OK;
	}
}

/*
** Get object file time
*/
static STATUS 
object_file_time(srcfile,time)
LOCATION *srcfile;
SYSTIME	*time;
{
	char 	buf[MAX_LOC+1];
	char	basename[MAX_LOC+1];
	char	suffix[MAX_LOC+1];
	char	filename[MAX_LOC+1];
	char	objname[MAX_LOC+1];
	LOCATION obj_file;

	/* Make the object location */
       	if ( LOdetail( srcfile, buf, buf, basename, suffix, buf ) != OK )
                return FAIL;
 
       	_VOID_ STprintf(filename, ERx("%s.%s"), basename, suffix);

       	_VOID_ STprintf(objname, ERx("%s%s"), basename, ABOBJEXTENT);
       	if ( IIABfdirFile(objname, buf, &obj_file) != OK )
       	{
       	 	IIUGerr( E_AB004C_FileLength, 0, 2, filename, objname);
               	return FAIL;
        }     
	
	/* Get its time */
	return file_time(&obj_file, time);
}

/*
** If the source file hasn't changed since a bad compilation, ask whether
** to recompile.
*/
static STATUS
bad_comp(comp, srcloc, srcTime)
USER_FRAME	*comp;		/* Component to check */
LOCATION	**srcloc;	/* Source file location (output) */
SYSTIME		*srcTime;	/* Date on source file (output) */
{
        SYSTIME         cdtime;
        LOCATION        *locptr;

	/* Get the source file's time */
	locptr = iiabMkLoc(comp->appl->source, comp->source);
	if (locptr == NULL)
		return E_NoSrcFile;

	if (file_time(locptr, srcTime) != OK)
		return E_NoSrcFile;
	*srcloc = locptr;

	/* Convert the compilation date */
        IIUGdtsDateToSys(comp->compdate, &cdtime);

	if ((TMcmp(srcTime, &cdtime) < 0) && (IIabfmFormsMode == TRUE))		 
	{
		IIUGerr( E_AB027C_BadComp, 0, 2, comp->name, comp->compdate );

                if (! IIUFver(ERget(S_AB027D_BCPrompt),0))
                {
                        return E_BadComp; 
		} 
	}
	return OK;
}
