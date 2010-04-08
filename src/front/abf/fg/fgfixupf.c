/*
**	Copyright (c) 1989, 2004 Ingres Corporation
*/

# include	<compat.h>
# include       <st.h>
# include       <er.h>
# include       <lo.h>
# include       <nm.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include       <ooclass.h>
# include 	<adf.h>
# include 	<ft.h>
# include 	<fmt.h>
# include 	<frame.h>
# include	<ug.h>
# include       <metafrm.h>
# include	<lqgdata.h>
# include       <abclass.h>
# include 	"framegen.h"
# include       <erfg.h>


/**
** Name:	fgfixupf.c	-	Emerald form fixup
**
** Description:
**	"Form fixup" takes place when:
**		1.	A Visual Query has been modified, and
**		2.	The form produced by the previous Visual Query
**			has been changed with Vifred.
**	If (2) were false, we could generate a default form.  As it is, we
**	will try to merge the VQ changes into the existing form.  The changes 
**	are of 3 types:
**		1.	Delete the fields (and columns) no longer needed by 
**			the VQ.
**		2.	Add fields (and columns) newly needed by the VQ to 
**			the top of the form.
**		3.	Add some trim to tell the user what we've done.
**
**	The actions, based on the settings of VQ column flags and presence
**	of the field (or column) on the form are:
**
**	COL_USED	on form		Action:
**	TRUE		TRUE		Copy the field on the form, so long as
**					the COL_TYPECHANGE bit isn't set ,and
**					the types are consistent.
**					Otherwise, create a new field.
**	TRUE		FALSE		Create a new field.
**	FALSE		TRUE		Ignore field on form;
**					don't create a new one.
**	FALSE		FALSE		Do nothing.
**
**
**	For this release, we've simplified the treatment of some thorny issues.
**		1.	If the user has added field(s) whose names conflict with
**			fields we want to add, we inform him of the conflicts
**			and fail. Documentation should warn him about names
**			likely to cause conflicts.
**		2.	If the form is a popup and no longer fits on
**			the screen after fixup, we'll change it to fullscreen.
**			
**	We also fix up ListChoices-style tablefields.  These have no VQ.  We
**	treat the "command" and "explanation" columns as if they has COL_USED
**	set in some virtual VQ.
**
**	This file defines:
**
**	IIFGfuf_fixUpForm	Perform form fixup
**	IIFGgfGetForm		Read form from database
**
** History:
**	6/30/89 (Mike S)	Initial version
**	8/10/89 (Mike S)	Clear change bits
**	12/19/89 (dkh) - VMS shared lib changes.
**	07/20/90 (dkh) - Put in calls to FDsetparse() to bracket call
**			 on FDwfrcreate() so we can correctly get a
**			 form out of the DB and only build part of the
**			 forms structures.
**	11/02/90 (dkh) - Replaced IILIBQgdata with IILQgdata().
**	3/1/91 (pete)	 changed hash tables to store lower case strings
**			 (bug 36223 -- UPPER CASE gateway bug).
**	08-jun-92 (leighb) DeskTop Porting Change:
**		Changed 'errno' to 'errnumptr' cuz 'errno' is a macro in WIN/NT
**	06-aug-93 (connie) Bug #51364
**		When calculating the new table field rows to display,
**		if there is no change in number of menus, do not subtract 
**		max(prev_nummenus,1) which is to take care the case when 
**		number of menus is 0 where row to display is 1.
**		This should fix the problem of having number of rows
**		decremented by 1 each time to compile a no-child menu frame
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	07-mar-2001 (somsa01)
**	    Changed maxcharwidth from 10000 to FE_FMTMAXCHARWIDTH.
**      03-may-2004 (rodjo04) bug 107649 INGCBT417
**          In typeIsOK(), when we encounter a decimal field we should now 
**          check to see if the value has changed, and also to see if the 
**          precision grew because of call to fmt_ideffmt().
**	23-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

/* # define's */
# define FMTSTRSZ	132	/* Maximum size of formatted string */
# define TFROWS		10	/* Default rows in tablefield */
# define HTSIZE		63	/* Hash table size */
# define DEFWIDTH	80	/* Default screen width */
# define DEFHEIGHT	24	/* Default screen width */


/* GLOBALDEF's */
/* extern's */
FUNC_EXTERN	FRAME 	*FDwfrcreate();
FUNC_EXTERN	char  	*UGcat_now();
FUNC_EXTERN	FRAME	*IIFDofOpenFrame();
FUNC_EXTERN	TBLFLD	*IIFDatfAddTblFld();
FUNC_EXTERN	STATUS	IIFDaetAddExistTrim();
FUNC_EXTERN	STATUS	IIFDaecAddExistCol();
FUNC_EXTERN	STATUS	IIFDaefAddExistFld();
FUNC_EXTERN	STATUS	IIFDaexAddExistTf();
FUNC_EXTERN	STATUS	IIFDffFormatField();
FUNC_EXTERN	STATUS	IIFDfcFormatColumn();
FUNC_EXTERN	STATUS  IIFGflcFmtLcCols();
FUNC_EXTERN	FRAME   *IIFGgfGetForm();
FUNC_EXTERN	STATUS	IIFGmdf_makeDefaultFrame();
FUNC_EXTERN 	i4 (*IIseterr())();
FUNC_EXTERN	STATUS  IIUGheHtabEnter();
FUNC_EXTERN	STATUS  IIUGhfHtabFind();
FUNC_EXTERN	STATUS  IIUGhiHtabInit();
FUNC_EXTERN	VOID    IIUGhrHtabRelease();
FUNC_EXTERN	VOID 	FTterminfo();
FUNC_EXTERN	char	*IIFGlccLowerCaseCopy();




/* static's */
static	PTR	fhtab;		/* Field hash table */
static	PTR	chtab;		/* Column hash table */
static i4	frame_mode;	/* Master-only in simplefields, master-only in
				   a table field, or master-detail */
static bool	made_changes;	/* Have we changed the form ? */
static bool	ismenu;		/* Is this a menu frame? */

/* Pseudo-columns for ListChoices-style menus */
static MFCOL command_col;
static MFCOL expl_col;
static char  command_name[] 	= ERx("command");
static char  expl_name[] 	= ERx("explanation");

static ER_MSGID s_readerr;	/* Error number from reading form */

/* 
** Errors in derived fields resulting from deleting depended-upon fields
** and columns.  We'll ignore them if we're only trying to make a frame
** structure.
*/
static ER_MSGID df_errs[] = 	{8351, 8352, 8363, 8364};
static TAGID 	Tag = 0;	/* Default tag */

static STATUS	add_new_cols();
static STATUS	add_new_flds();
static bool	add_old_cols();
static bool	add_old_flds();
static bool	add_old_trim();
static VOID 	check_popup();
static bool 	check_VQ();
static TBLFLD	*create_vision_tf();
static VOID	make_hashes();
static VOID 	move_fields_down();
static VOID 	move_fields_over();
static VOID	search_form();
static VOID 	set_display_flags();
static bool	typeIsOK();
static i4 	frmreaderr();

/*{
** Name:	IIFGfuf_fixUpForm - perform form fixup
**
** Description:
**	Perform form fixup as described above.
**	The update argument is only used if we generate a whole new form.
**	It should be set TRUE unless the caller will update the catalogs
**	itself.
**
** Inputs:
**	metaframe	METAFRAME *	metaframe describing form to produce
**	upodate		bool		should the catalogs be updated
**
** Outputs:
**	none
**
**	Returns:
**		STATUS	Return status	OK if new form was saved to database
**					FAIL otherwise
**
**	Exceptions:
**		none
**
** Side Effects:
**		Generated form is saved to database, overwriting existing form.
**
** History:
**	6/30/89 (Mike S)	Initial version
**	8/9/89 (Mike S)		New IIFDfxFormat... argument
**	1/3/91 (pete)		Made "tag" a global static.
**	3/91 (Mike S)		Fixup ListChoices-style menus
*/
STATUS 
IIFGfuf_fixUpForm( metaframe, update )
METAFRAME *metaframe;
bool	update;
{
	FRAME 	*oldform;	/* Form from database */
	FRAME	*newform;	/* Form being created */
	char 	*formname;	/* Name of forms */
	STATUS 	retval;		/* Return value */
	bool	opened = FALSE;	/* True if dynamic form is open */
	USER_FRAME *apobj = ((USER_FRAME *)(metaframe->apobj));
	TBLFLD	*oldtblfld;	/* Table field on old form created by Emerald */
	TBLFLD	*newtblfld;	/* Table field on new form created by Emerald */
	TRIM	*separator = NULL; /* Line separating new fields from old */
	i4	tblgrowth;	/* Change in width of tablefield */
	bool	visiontf;	/* Will there be Emerald columns in the tblfld*/
	bool	usertf;		/* Are there user columns in the tablefield */
	bool	needtf;		/* Do we need an "iitf" tablefield 
				   ( == (visiontf || usertf) ) */
	i4	newlines = 0;	/* Lines added at top of new form */
	i4	conflicts = 0;	/* Field and column name conflicts */
	ER_MSGID readerr;	/* Error from reading form */

	STATUS	status;

	/* assume failure */
	retval = FAIL;

	made_changes = FALSE;	/* No changes yet */
	chtab = fhtab = NULL;
	frame_mode = metaframe->mode & MFMODE;
	ismenu = metaframe->apobj->class == OC_MUFRAME;

	/* Open a dynamic frame structure; this also starts a new tag region */
	formname = apobj->form->name;
	if((newform = IIFDofOpenFrame(formname)) == NULL)
		goto memerr;
	Tag = newform->frtag;
	opened = TRUE;

	/* get existing form, ignoring all errors. */
	oldform = IIFGgfGetForm(formname, TRUE, &readerr);
	if (oldform == NULL)
	{
		IIUGerr(E_FG0036_NoFormToFixup, 0, 1, formname);
		goto cleanup;	
	}
	oldform->frtag = Tag;
	
	/* 
	** Hash the columns and fields from the VQ which need to
	** be on the form.
	*/
	make_hashes(metaframe, &visiontf);
	
	/* 
	** Search form for needed fields 
	*/
	search_form(oldform, &oldtblfld, &usertf, &conflicts);

	/* If we had name conflicts, fail */
	if (conflicts > 0)
	{
		IIUGerr(E_FG003C_NameConflicts, 0, 1, formname);
		goto cleanup;
	}

	/* Add new fields to new form */
	if (frame_mode != MF_MASTAB && !ismenu)
	{
		status = add_new_flds(newform, metaframe, &newlines, &separator);
		if (status == FAIL)
			goto memerr;
		else if (status != OK)
			goto cleanup;
	}

	/* 
	** We will add a tablefield, if we need one, i.e. if
	** 1. There are old or new columns to add, or
	** 2. The user had columns in the VQ tablefield.
	*/
	needtf = usertf || visiontf;

	/* Add existing fields to form */
	newtblfld = NULL;
	if (! add_old_flds(metaframe, newform, oldform, oldtblfld, newlines, 
			   needtf, visiontf, &newtblfld))
		goto memerr;

	/* If we need to create the "iitf" tablefield, do it */
	if (needtf && (newtblfld == NULL))
	{
		newtblfld = create_vision_tf(oldtblfld, oldform, newform, 
					     newlines, visiontf, metaframe);
		if (newtblfld == NULL)
			goto memerr;
	}

	if (newtblfld != NULL)
	{
		/* If there's a tablefield, add new columns to it */
		status = add_new_cols(metaframe, newform, newtblfld);
		if (status == FAIL)
			goto memerr;
		else if (status != OK)
			goto cleanup;

		/* Add existing columns to tablefield */
		if (oldtblfld != NULL)
			if (!add_old_cols(newform, newtblfld, oldtblfld))
			goto memerr;
	}

	/* Add existing trim */
	if (!add_old_trim(newform, oldform, newlines))
		goto memerr;

	/* Convert dynamic frame to normal FRAME structure */
	opened = FALSE;
	if (!IIFDcfCloseFrame(newform)) 
		goto memerr;

	/* Move fields and trim over by tablefield growth size */
	if (oldtblfld != NULL && newtblfld != NULL)
	{
		tblgrowth = newtblfld->tfhdr.fhmaxx - oldtblfld->tfhdr.fhmaxx;
		if (tblgrowth > 0)
			move_fields_over(newform, newtblfld, tblgrowth);
		if (tblgrowth != 0)
			made_changes = TRUE;	/* Different TF size */	

		tblgrowth = newtblfld->tfrows - oldtblfld->tfrows;
		if (tblgrowth > 0)
			move_fields_down(newform, newtblfld, tblgrowth);
		if (tblgrowth != 0)
			made_changes = TRUE;	/* Different TF size */	
	}
		
	/* Check if it should be a popup */
	check_popup(oldform, newform, formname);

	/* Give separator line its proper width */
	if (separator != NULL)
	{
		char tsbufr[20];

		STprintf(tsbufr, "1:%d:-1", newform->frmaxx);
		separator->trmstr = FEsalloc(tsbufr);
	}

	/* Save form into database if changes were made */
	if (!made_changes)
	{
		retval = OK;
	}
	else
	{
		retval = IIFGscfSaveCompForm(apobj, newform);
		if (retval == OK)
		{
			/* Save number of menuitems */
			metaframe->state &= ~MFST_MIMASK;
			metaframe->state |= 
			 ((metaframe->nummenus << MFST_MIOFFSET) & MFST_MIMASK);

			/* Now check that the form is OK */
			if (IIFGgfGetForm(formname, FALSE, &readerr) == NULL)
				IIUGerr(E_FG0060_FormProblems, 0, 1, formname);
		}
	}
	goto cleanup;

	/* memory allocation failure */
memerr:	
	IIUGerr(E_FG0037_FixupMemErr, 0, 1, formname);
	retval = FAIL;
	/* Fall through to cleanup */

	/* exit processing; clean up all dynamic memory */
cleanup:	
	if (fhtab != NULL)
		IIUGhrHtabRelease(fhtab);
	if (chtab != NULL)
		IIUGhrHtabRelease(chtab);
	if (opened) IIFDcfCloseFrame(newform);
	if (Tag != 0) FEfree(Tag);

	return(retval);
}


/*
** Make hash tables for quick lookup of the fields and columns on the
** VQ.
*/
static VOID
make_hashes(metaframe, visiontf)
METAFRAME *metaframe;	/* Metaframe */
bool	*visiontf;	/* Will we need a tablefield for VQ columns */
{
	i4 i, j;
	MFTAB *tabptr;
	MFCOL *colptr;
	PTR htab;

	*visiontf = FALSE;	/* Assume no tablefield */

	/* Create field and table field column hash tables */
	_VOID_ IIUGhiHtabInit(HTSIZE, NULL, NULL, NULL, &fhtab);
	_VOID_ IIUGhiHtabInit(HTSIZE, NULL, NULL, NULL, &chtab);

	/* If it's a menu, just add the needed columns */
	if (ismenu)
	{
		command_col.name = command_col.alias = command_name;
		expl_col.name = expl_col.alias = expl_name;
		command_col.flags = expl_col.flags = COL_USED;
		command_col.type.db_datatype = expl_col.type.db_datatype = 
			DB_CHA_TYPE;
		IIUGheHtabEnter(chtab, command_name, (PTR)&command_col);
		IIUGheHtabEnter(chtab, expl_name, (PTR)&expl_col);
		*visiontf = TRUE;
		return;
	}

	/* Fill them with the fields which should appear on the form */
	for (i = 0; i < metaframe->numtabs; i++)
	{
		tabptr = metaframe->tabs[i];
		if (tabptr->tabsect > TAB_MASTER || frame_mode == MF_MASTAB)
			htab = chtab;
		else
			htab = fhtab;

		for (j = 0; j < tabptr->numcols; j++)
		{
			colptr = tabptr->cols[j];
			colptr->flags &= ~COL_FOUND;
			if ((colptr->flags & COL_USED) != 0)
			{
				IIUGheHtabEnter(htab,
				    IIFGlccLowerCaseCopy(colptr->alias, Tag),
				    (PTR)colptr);

				if (htab == chtab)
					*visiontf = TRUE;
			}
		}
	}
}
				
/*
**	Search the form for fields and columns which will appear on the
**	form.
*/
static VOID
search_form(oldform, oldtblfld, usertf, conflicts)
FRAME     *oldform;     /* Form containing fields to mark */
TBLFLD    **oldtblfld;  /* Tablefield created by Emerald (output) */
bool      *usertf;      /* Set to TRUE if oldtblfld has user-created fields */
i4      *conflicts;     /* number of name conflicts (modified) */
{
        register FIELD  *field;
        register FLDHDR *fldhdr;
        FLDCOL  *fldcol;
        i4      fldx;
        i4      i;
	bool	vqlocked;

        /* Assume we won't find the tablefield */
        *oldtblfld = NULL;
        *usertf = FALSE;

        /*
        ** Loop through all fields in form.  
        */
        for (fldx = 0; fldx < oldform->frfldno; fldx++)
        {
            field = oldform->frfld[fldx];
            if (field->fltag == FTABLE)
            {
                /* A tablefield.  See if it's Emerald's */
                fldhdr = &field->fld_var.fltblfld->tfhdr;
                if (STcompare(fldhdr->fhdname, TFNAME) == 0)
                {
                    /* We found the Emerald tablefield */
                    *oldtblfld = field->fld_var.fltblfld;

		    /* Check column names */
		    for (i = 0; i < (*oldtblfld)->tfcols; i++)
	    	    {
			fldcol = (*oldtblfld)->tfflds[i];
			vqlocked = check_VQ(&fldcol->flhdr, &fldcol->fltype,
					 chtab, E_FG003F_ColNameConflict,
					 conflicts);
			if (!vqlocked)
			    *usertf = TRUE;  /* User column in VQ tablefield */
		    }
		}
		else
		{
		    /* It's a user tablefield */
		    _VOID_ check_VQ(fldhdr, (FLDTYPE *)NULL, fhtab, 
				    E_FG003D_TfNameConflict, conflicts);
		}
	    }
	    else
	    {
		/* A simple field */
		_VOID_ check_VQ(&field->fld_var.flregfld->flhdr, 
				&field->fld_var.flregfld->fltype, fhtab,
				E_FG003E_FldNameConflict, conflicts);
	    }
	}
}

/*
** Search the VQ for a field or column name.
**
** Returns TRUE if field (or column) has VQLOCK bit set
*/
static bool
check_VQ(fldhdr, fldtype, htab, msgid, conflicts)
FLDHDR 	*fldhdr;	/* Header structure for field (or column) to check */
FLDTYPE *fldtype;	/* Type structure for field */
PTR	htab;		/* Hash table to search */
ER_MSGID msgid;		/* Message to issue for a conflict */
i4	*conflicts;	/* Number of conflicts (modified) */
{
	PTR 	ptr;
	MFCOL 	*colptr;
	bool 	found;
	bool	vqlocked;	

	found = (IIUGhfHtabFind(htab, fldhdr->fhdname, &ptr) == OK);
	vqlocked = (fldhdr->fhd2flags & fdVQLOCK) != 0;
	if (found)
	{
	    if (!vqlocked)
    	    { 
	        /* Not a VQ field (or column), ergo a name conflict */
		IIUGerr(msgid, 0, 1, fldhdr->fhdname);
		(*conflicts)++;
	    } 
	    else
	    {
	        /* A VQ field (or column) */
		colptr = (MFCOL *)ptr;

		/* 
		** Found it; check whether the type is usable.
		*/
		if ((colptr->flags & COL_TYPECHANGE) == 0 && fldtype != NULL)
		{
		    if (typeIsOK(&colptr->type, fldtype))
		    {
			colptr->flags |= COL_FOUND;	/* Use it */
		    }
		    else
		    {
			/* We can't use the old field; the type changed */
			colptr->flags |= COL_TYPECHANGE;
		    }
		}
	    }
	}
	return vqlocked;
}
			
/*
**	Add the new fields to the new form.
*/
static STATUS
add_new_flds(newform, metaframe, newlines, separator)
FRAME	*newform;	/* Form being created */
METAFRAME *metaframe;	/* Metaframe */
i4	*newlines;	/* Number of lines added to form (output) */
TRIM	**separator;	/* Separator line */
{
	MFTAB	*tabptr;	/* Current table */
	MFCOL	*colptr;	/* Current column */
	i4	tabidx;		/* Current table index */
	i4	colidx;		/* Current column index */
	i4 	curline = 0;	/* Current line on form */
	char	buffer[FMTSTRSZ];	/* Formatting buffer */
	FIELD 	*newfield;	/* New field */
	i4	tabadded = 0;	/* tables for which fields were added */
	bool	hasnew;		/* TRUE if table has new fields */
	STATUS	status;

	/* Loop through all tables, looking for new fields */
	for (tabidx = 0; tabidx < metaframe->numtabs; tabidx++)
	{
		/* Only the master section creates simple fields */
		tabptr = metaframe->tabs[tabidx];
		if (tabptr->tabsect > TAB_MASTER || frame_mode == MF_MASTAB)
			break;

		/* See if this table contains new fields */
		hasnew = FALSE;
		for (colidx = 0; colidx < tabptr->numcols; colidx++)
		{
			colptr = tabptr->cols[colidx];
			if ((colptr->flags & (COL_USED|COL_FOUND)) == COL_USED)
			{
				hasnew = TRUE;
				break;
			}
		}
		if (!hasnew)
			continue;

		/* Add table header */
		if (curline > 0) curline++;
		IIUGfmt(buffer, FMTSTRSZ, ERget(S_FG003A_TableHeaderMsg),
			1, tabptr->name);
		if (IIFDattAddTextTrim(newform, curline++, 0, (i4)0, buffer)
			== NULL)
			return(FAIL);
		made_changes = TRUE;	/* Added simple fields */

		/* Add this table's columns */
		for (colidx = 0; colidx < tabptr->numcols; colidx++)
		{
			/* See if this one gets added */
			colptr = tabptr->cols[colidx];
			if ((colptr->flags & (COL_USED|COL_FOUND)) != COL_USED)
				continue;		/* Not a new field */

			/* Make title */
			STcopy(colptr->name, buffer);
			_VOID_ IIFDftFormatTitle(buffer);
			STcat(buffer, ERx(":"));

			/* Format field */
			status = IIFDffFormatField( 
				IIFGlccLowerCaseCopy(colptr->alias, Tag),
				0, curline++, 
				STlength(buffer) + 1, 0, buffer, 
				FE_FMTMAXCHARWIDTH, (i4)0, &colptr->type,
				0, 0, &newfield);
			if (status != OK) return (status);

			/* Set the field display flags */
			set_display_flags(tabptr, colptr, TRUE, 
					  &newfield->fld_var.flregfld->flhdr);

			/* Add field to form */
			status = IIFDaefAddExistFld(newform, newfield, 0, 0);
			if (status != OK) return (status);
		}

		tabadded++;
	}

	if (tabadded > 0)
	{
		/* Add a short separator line; we'll extend it later */
		curline++;
		*separator = IIFDabtAddBoxTrim(newform, curline, 0, 
					       curline, 1, (i4)0);
		*newlines = curline + 1;
	}
	else
	{
		*newlines = 0;		/* We didn't do anything */
	}
	return(OK);
}

/*	
**	Add the old fields to the new form.
*/
static bool	
add_old_flds(
   metaframe, newform, oldform, oldtblfld, offset, needtf, visiontf, newtblfld)
METAFRAME *metaframe;	/* metaframe */
FRAME	*newform;	/* Form being constructed */
FRAME	*oldform;	/* Old form being cannibalized */
TBLFLD	*oldtblfld;	/* Old Emerald-created tablefield */
i4	offset;		/* The number of lines to move everything down by */
bool	needtf;		/* Should we construct the "iitf" tablefield */
bool	visiontf;	/* Will there be Emerald columns in the tblfld*/
TBLFLD	**newtblfld;	/* Table field on new form created by Emerald (output)*/
{
	FIELD *field;
	register FLDHDR	*hdr;
	i4	i;
	MFCOL	*mfcol;
	bool	vqfld;
	PTR	ptr;
	STATUS	status;
	bool	keep;

	/* 
	** For each field in the old form, retain it if:
	** 1. It was created by the user (fdVQLOCK = 0), or
	** 2. Its COL_FOUND bit is set.
	**
	** If we see the old Emerald tablefield, and we need a tablefield,
	** create it.
	*/
	for (i = 0; i < oldform->frfldno; i++)
	{
		field = oldform->frfld[i];
		if (field->fltag == FTABLE)
		{
			/* Check for  the Emerald tablefield */
			if (field->fld_var.fltblfld == oldtblfld)
			{
				if (needtf)
				{
					/* Create it */
					*newtblfld = 
					   create_vision_tf(oldtblfld, oldform, 
						newform, offset, visiontf,
						metaframe);
					if (*newtblfld == NULL)
						return (FALSE);
				}
				else
				{
					/* Deleted a tablefield */
					made_changes = TRUE;
				}
			}
			else
			{
				if (IIFDaexAddExistTf(newform, field, 0, offset)
						!= OK)
					return (FALSE);
			}
		}
		else
		{
			hdr = &field->fld_var.flregfld->flhdr;
			vqfld = ((hdr->fhd2flags &fdVQLOCK) != 0);
			if (vqfld)
			{
				/* One of our fields. See if should be kept */
				keep = FALSE;
				status = IIUGhfHtabFind(fhtab, hdr->fhdname, 
							&ptr);
				if (status == OK)
				{
					mfcol = (MFCOL *)ptr;
					keep = (COL_USED|COL_FOUND) ==
						   (mfcol->flags & 
						   (COL_USED|COL_FOUND));
				}
				if (!keep)
				{
					/* Deleted a simple field */
					made_changes = TRUE;	
					continue;	/* We don't want it */
				}
			}

			/* Add it */		
			if (IIFDaefAddExistFld(newform, field, 0, offset)
				!= OK)
				return (FALSE);
		}
	}
	return (TRUE);
}

/*
**	Add the new columns to the new form
*/
static STATUS
add_new_cols(metaframe, newform, newtblfld)
METAFRAME *metaframe;	/* Metaframe */
FRAME	*newform;	/* Form being constructed */
TBLFLD	*newtblfld;	/* Tablefield to add to */
{
	i4	tabidx;
	i4	colidx;
	MFTAB	*tabptr;
	register MFCOL *colptr;
	FLDCOL	*fldcol;
	STATUS	status;
	char	buffer[FMTSTRSZ];

	/*
	** For a menu, add the columns if need be
	*/
	if (ismenu)
	{
	    bool add_cmd = (command_col.flags & COL_FOUND) == 0;
	    bool add_expl = (expl_col.flags & COL_FOUND) == 0;
	    FLDCOL *command_fc, *expl_fc;

	    if (add_cmd || add_expl)
	    {
		/* Format columns */
		if (IIFGflcFmtLcCols(&command_fc, &expl_fc) != OK)
			return FAIL;

		if (add_cmd)
		    if (IIFDaecAddExistCol(newform, newtblfld, command_fc)!= OK)
			return FAIL;

		if (add_expl)
		    if (IIFDaecAddExistCol(newform, newtblfld, expl_fc) != OK)
			return FAIL;
	    }

	    return OK;
	}

	/* 
	** For each column in the VQ, create a FLDCOL and add it to the
	** new tablefield 
	*/
	for (tabidx = 0; tabidx < metaframe->numtabs; tabidx++)
	{
	    tabptr = metaframe->tabs[tabidx];
	    if (tabptr->tabsect == TAB_MASTER && frame_mode != MF_MASTAB)
		continue;	/* This table makes simple fields */

	    for (colidx = 0; colidx < tabptr->numcols; colidx++)
	    {
		colptr = tabptr->cols[colidx];
		if ((colptr->flags & (COL_USED|COL_FOUND)) != COL_USED)
			continue;	/* Either not used or on form */

		made_changes = TRUE; /* Adding a column */

		/* Make title */
		STcopy(colptr->name, buffer);
		_VOID_ IIFDftFormatTitle(buffer);

		/* Format column */
		if ((status = IIFDfcFormatColumn(
				IIFGlccLowerCaseCopy(colptr->alias, Tag),
				0, buffer,
				&colptr->type, 0, &fldcol)) != OK)
			return status;

		/* Set the column display flags */
		set_display_flags(tabptr, colptr, FALSE, &fldcol->flhdr);

		/* Add column to tablefield */
		if ((status = IIFDaecAddExistCol(newform, newtblfld, fldcol))
			!= OK)
			return status;
	    }
	}
	
	return(OK);
}


/*
**	Add the existing columns which are still needed to the new form.
*/
static bool
add_old_cols(newform, newtblfld, oldtblfld)
FRAME	*newform;	/* frame being constructed */
TBLFLD	*newtblfld;	/* tablefield being constructed */
TBLFLD	*oldtblfld;	/* Old tablefield */
{
	FLDCOL *fldcol;
	register FLDHDR	*hdr;
	i4	i;
	MFCOL	*mfcol;
	bool	vqcol;
	PTR	ptr;
	STATUS	status;
	bool	keep;

	/* 
	** For each column in the old tablefield, retain it if:
	** 1. It was created by the user (fdVQLOCK = 0), or
	** 2. It's still needed.
	*/
	for (i = 0; i < oldtblfld->tfcols; i++)
	{
		fldcol = oldtblfld->tfflds[i];
		hdr = &fldcol->flhdr;

		vqcol = ((hdr->fhd2flags &fdVQLOCK) != 0);
		if (vqcol)
		{
			/* One of our columns.  See if it's still needed */
			keep = FALSE;
			status = IIUGhfHtabFind(chtab, hdr->fhdname, &ptr);
			if (status == OK)
			{
				mfcol = (MFCOL *)ptr;
				keep = (COL_USED|COL_FOUND) ==
					 (mfcol->flags & (COL_USED|COL_FOUND));
			}
			if (!keep)
			{
				/* Deleted a column */
				made_changes = TRUE;
				continue;	/* We don't want it */
			}
		}
		/* Add it */		
		if (IIFDaecAddExistCol(newform, newtblfld, fldcol) != OK)
			return (FALSE);
	}
	return (TRUE);
}


/*
**	Add the existing trim to the new form.
*/
static bool
add_old_trim(newform, oldform, newlines)
FRAME	*newform;		/* New form */
FRAME	*oldform;		/* Old form */
i4	newlines;		/* Number of lines to move things down by */
{
	i4 i;

	/* Loop through, adding all existing trim */
	for (i = 0; i < oldform->frtrimno; i++)
	{
		if (IIFDaetAddExistTrim(
			newform, oldform->frtrim[i], 0, newlines) != OK)
			return(FALSE);
	}
	return (TRUE);
}

/*
**	If the tablefield has more lines, move everything below it down further.
**	This will make the form longer
*/
static VOID
move_fields_down(newform, newtblfld, tblgrowth)
FRAME *newform;		/* Form to modify */
TBLFLD *newtblfld;	/* Tablefield which has grown */
i4	tblgrowth;	/* Amount tablefield has grown by */
{
	i4	tfleft, tfright, tftop;	/* tablefield coordinates */
	register FLDHDR	*tfhdr = &newtblfld->tfhdr;
	i4	frmlength = newform->frmaxx;
	bool	moved = FALSE;
	i4 i;

	/*
	** Calculate the starting and ending column for the tablefield ,
	** and its top row.
	*/
	tftop = tfhdr->fhposy;
	tfleft = tfhdr->fhposx;
	tfright = tftop + tfhdr->fhmaxx - 1;

	/*
	** We assume that nothing overlapped before we started the fixup,
	** so things have to be moved down if:
	**	1. They start below the tablefield.
	**	2. They begin to the left of the tablefield's right, and
	**	   end to the right of its left.
	**
	** We do the fields first.
	*/
	for (i = 0; i < newform->frfldno; i++)
	{
		FIELD	*fld;
		register FLDHDR *hdr;
		i4 	fldtop;
		i4	fldleft;
		i4 	fldright;

		fld = newform->frfld[i];
		/* Get field header */
		if (fld->fltag == FREGULAR)
		{
			hdr = &fld->fld_var.flregfld->flhdr;
		}
		else
		{
			/* Don't move the tablefield itself */
			if (fld->fld_var.fltblfld == newtblfld)
				continue;
			hdr = &fld->fld_var.fltblfld->tfhdr;
		}	
		/* See if we need to move */
		fldtop = hdr->fhposy;
		fldleft = hdr->fhposx;
		fldright = hdr->fhposx + hdr->fhmaxx -1;
		if (fldtop > tftop && fldleft <= tfright && tfleft <= fldright)
		{
			/* Move it */
			hdr->fhposy += tblgrowth;
			moved = TRUE;
		}
	}

	/* Now we do the trim */
	for (i = 0; i < newform->frtrimno; i++)
	{
		register TRIM	*trim;
		i4 	trimtop;
		i4	trimleft;
		i4	trimright;

		trim = newform->frtrim[i];
		trimtop = trim->trmy;
		trimleft = trim->trmx;

		/* Get last trim column; different for box and regular trim */
		if ((trim->trmflags & fdBOXFLD) == 0)
		{
			trimright = trimleft + STlength(trim->trmstr) - 1;
		}
		else
		{
			i4 dummy;
			i4 trimwidth;

			/* The STscanf shouldn't fail */
			if (STscanf(trim->trmstr, ERx("%d:%d"), 
			    &dummy, &trimwidth) != 2)
				trimwidth = 1;
			trimright = trimleft + trimwidth - 1;
		}
		if (trimleft <= tfright && tfleft <= trimright && trimtop > tftop)
		{
			/* Move it */
			trim->trmy += tblgrowth;
			moved = TRUE;
		}
	}
	/* Adjust the form width */
	if (moved)
		newform->frmaxy += tblgrowth;
}

/*
**	If the tablefield has grown, move everything to its right over.
**	This may widen the form.
*/
static VOID
move_fields_over(newform, newtblfld, tblgrowth)
FRAME *newform;		/* Form to modify */
TBLFLD *newtblfld;	/* Tablefield which has grown */
i4	tblgrowth;	/* Amount tablefield has grown by */
{
	i4	tfstart, tfend, tfleft;	/* tablefield coordinates */
	register FLDHDR	*tfhdr = &newtblfld->tfhdr;
	i4	frmwidth = newform->frmaxx;
	i4 i;

	/*
	** Calculate the starting and ending rows for the tablefield ,
	** and its leftmost column.
	*/
	tfstart = tfhdr->fhposy;
	tfend = tfstart + tfhdr->fhmaxy - 1;
	tfleft = tfhdr->fhposx;

	/*
	** We assume that nothing overlapped before we started the fixup,
	** so things have to be moved over if:
	**	1. They have a row in common with the tablefield, and
	**	2. They begin to the right of the tablefield's left.
	**
	** We do the fields first.
	*/
	for (i = 0; i < newform->frfldno; i++)
	{
		FIELD	*fld;
		register FLDHDR *hdr;
		i4 	fldstart;
		i4 	fldend;
		i4	fldleft;
		i4 	fldright;

		fld = newform->frfld[i];
		/* Get field header */
		if (fld->fltag == FREGULAR)
		{
			hdr = &fld->fld_var.flregfld->flhdr;
		}
		else
		{
			/* Don't move the tablefield itself */
			if (fld->fld_var.fltblfld == newtblfld)
				continue;
			hdr = &fld->fld_var.fltblfld->tfhdr;
		}	
		/* See if we need to move */
		fldstart = hdr->fhposy;
		fldend = fldstart + hdr->fhmaxy - 1;
		fldleft = hdr->fhposx;
		if (fldstart <= tfend && tfstart <= fldend && fldleft > tfleft)
		{
			/* Move it */
			hdr->fhposx += tblgrowth;
			fldright = hdr->fhposx + hdr->fhmaxx;
			frmwidth = max(frmwidth, fldright);
		}
	}

	/* Now we do the trim */
	for (i = 0; i < newform->frtrimno; i++)
	{
		register TRIM	*trim;
		i4 	trimstart;
		i4 	trimend;
		i4	trimheight;
		i4	trimleft;
		i4	trimright;
		i4	trimwidth;

		trim = newform->frtrim[i];
		trimstart = trim->trmy;
		trimleft = trim->trmx;

		/* Get last trim line; different for box and regular trim */
		if ((trim->trmflags & fdBOXFLD) == 0)
		{
			trimend = trimstart;
			trimwidth = STlength(trim->trmstr);
		}
		else
		{
			/* The STscanf shouldn't fail */
			if (STscanf(trim->trmstr, ERx("%d:%d"), 
			    &trimheight, &trimwidth) != 2)
				trimheight = trimwidth = 1;
			trimend = trimstart + trimheight - 1;
		}
		if (trimstart <= tfend && tfstart <= trimend && trimleft > tfleft)
		{
			/* Move it */
			trim->trmx += tblgrowth;
			trimright = trim->trmx + trimwidth;
			frmwidth = max(frmwidth, trimright);	
		}
	}
	/* Adjust the form width */
	newform->frmaxx = frmwidth;
}

/*
**	See if the new form can and should be a popup
*/
static VOID
check_popup(oldform, newform, formname)
FRAME	*oldform;	/* Form being fixed up */
FRAME	*newform;	/* Newly created form */
char	*formname;	/* Name of form */
{
	bool	ispopup, isbordered;
	i4 	bord;	/* Space for border */
	i4	row, col;	/* Form coordinates */
	i4	term_cols, term_lines;	/* Terminal size */

	/* If the form system is active, use current width */
	if (IILQgdata()->form_on)
	{
		FT_TERMINFO	terminfo;

		FTterminfo(&terminfo);
		term_cols = terminfo.cols;
		term_lines = terminfo.lines;
	}
	else
	{
		term_cols = DEFWIDTH;
		term_lines = DEFHEIGHT;
	}

	ispopup = (oldform->frmflags &fdISPOPUP) != 0;
	if (!ispopup)
		return;		/* The old one wasn't a popup */

	/* See if it still fits on the screen */
	isbordered = (oldform->frmflags & fdBOXFR) != 0;
	bord = isbordered ? 2 : 0;
	
	/* If it floats, assume the most lenient position */
	if (oldform->frposy != 0)
	{
		row = oldform->frposy - 1;
                col = oldform->frposx - 1;
        }
        else   
        {
                row = col = 0;
        }

	if (row + newform->frmaxy + bord > term_lines - 1 ||
	    col + newform->frmaxx + bord > term_cols )
	{
		/* Tell user it doesn't fit as a popup */
		IIUGerr(E_FG003B_Back_to_fullscreen, 0, 1, formname);
		made_changes = TRUE;	/* Changed to fullscreen form */
		return;	
	}

	/* Make the new one a popup like the old one */
	newform->frmflags |= (isbordered ? (fdISPOPUP|fdBOXFR) : fdISPOPUP);
	newform->frposx = oldform->frposx;
	newform->frposy = oldform->frposy;
}	

/*
**	Set display flags according to the contents of the MFCOL struct.
*/
static VOID
set_display_flags(tabptr, colptr, regfld, flhdr)
MFTAB	*tabptr;	/* Table structure */
MFCOL	*colptr;	/* Table column structure */
bool	regfld;		/* Regular field (not tablefield column) */
FLDHDR	*flhdr;		/* Form field (or column) header */
{

	/* Mark it as an Emerald field */
	flhdr->fhd2flags |= fdVQLOCK;

	/* Underline regular fields (not columns) */
	if (regfld) flhdr->fhdflags |= fdUNLN;

	/*
	** If the field is non-nullable, has no default, and isn't in a lookup
	** table, it's mandatory.
	*/
	if ((colptr->type.db_datatype > 0) && 
	    ((colptr->flags & COL_DEFAULT) == 0) &&
	    (tabptr->usage < TAB_LOOKUP))
		flhdr->fhdflags |= fdMAND;

	/*
	**	If the field is in a Lookup table, it's query only (in 
	**	a simple field or for masters in a tablefield) or readonly 
	**	(in a detail lookup tablefield column).
	*/
	if (tabptr->usage == TAB_LOOKUP)
	{
		if (regfld || frame_mode == MF_MASTAB)
			flhdr->fhdflags |= fdQUERYONLY;
		else
			flhdr->fhd2flags |= fdREADONLY;
	}

	/*
	** Make master-detail join fields Bold; 
	** make lookup join fields reverse video and mandatory. 
	*/
	if ((colptr->flags & COL_DETJOIN) != 0)
		flhdr->fhdflags |= fdCHGINT;
	if ((colptr->flags & COL_LUPJOIN) != 0)
		flhdr->fhdflags |= (fdRVVID|fdMAND);
}

/* 
**	Create the "iitf" tablefield
**
**	Returns:
**				NULL on a memory allocation error
**				Pointer to new tablefield
*/
static TBLFLD * 
create_vision_tf( oldtblfld, oldform, newform, newlines, visiontf, metaframe)
TBLFLD	*oldtblfld;	/* Old tablefield */
FRAME 	*oldform;	/* Form from database */
FRAME	*newform;	/* frame being constructed */
i4	newlines;	/* Lines added at top of new form */
bool	visiontf;	/* Will there be Emerald columns in the tblfld*/
METAFRAME *metaframe;	/* metaframe */
{
	TBLFLD	*newtblfld;	/* New tablefield */
	i4 	tflin;		/* Lines in tablefield */
	i4 	tfcol;		/* Columns in tablefield */
	i4 	tfrows;		/* Rows in tablefield */
	bool	lines;		/* Separate rows with lines */
	bool	coltitles;	/* Use column titles */
	
	/* 
	** If there was a tablefield, make it the same size and
	** put it at the same place, and use the same attributes
	** otherwise put it at the bottom left, give it a default size,
	** and don't highlight.
	*/
	if (oldtblfld != NULL)	
	{
		tflin = oldtblfld->tfhdr.fhposy + newlines;
		tfcol= oldtblfld->tfhdr.fhposx;
		if (ismenu)
		{
			i4 prev_nummenus;

			/* 
			** We want the same relationship of TF rows to 
			** menuitems which we had previously.
			*/
			prev_nummenus = (metaframe->state & MFST_MIMASK) 
						>> MFST_MIOFFSET;
			tfrows = oldtblfld->tfrows + metaframe->nummenus - 
				 ((prev_nummenus != metaframe->nummenus)?
				 max(prev_nummenus,1):prev_nummenus);

			/* If we got a silly answer, keep it what it was */
			if (tfrows < 1)
				tfrows = oldtblfld->tfrows;
		}
		else
		{
			tfrows = oldtblfld->tfrows;
		}
		lines = (oldtblfld->tfhdr.fhdflags & fdNOTFLINES) == 0;
		coltitles = (oldtblfld->tfhdr.fhdflags & fdNOTFTITLE) == 0;
	}
	else
	{
		made_changes = TRUE; /* Adding a tablefield */

		tflin = oldform->frmaxy + newlines;
		tfcol = 0;
		if (ismenu)
			tfrows = max(metaframe->nummenus, 1);
		else
			tfrows = TFROWS;
		lines = FALSE;
		coltitles = !ismenu;
	}

	/* Add the new tablefield */
	if ((newtblfld = IIFDatfAddTblFld(
					newform, TFNAME, tflin, tfcol, tfrows, 
					coltitles, lines)) == NULL)
	{
		/* A memory error.  We'll let the caller handle it */
		return (NULL);
	}

	if (oldtblfld != NULL)
	{
		/* If there was a tablefield,replicate the attributes */
		newtblfld->tfhdr.fhdflags = oldtblfld->tfhdr.fhdflags;
		newtblfld->tfhdr.fhd2flags = oldtblfld->tfhdr.fhd2flags;
	}

	/* Lock it only if it contains Emerald columns */
	if (visiontf) 
		newtblfld->tfhdr.fhd2flags |= fdVQLOCK;
	else
		newtblfld->tfhdr.fhd2flags &= ~fdVQLOCK;

	/* 
	** Turn off scrolling temporarily; add_new_cols will turn
	** it back on, if need be.
	*/
	newtblfld->tfhdr.fhd2flags &= ~fdSCRLFD;

	return (newtblfld);
}


/*{
** Name:	IIFGgfGetForm	- Read form from database
**
** Description:
**	Read form from database.  We only want to build a frame structure, 
**	not to activate the form, so we only syntax-check the derivation
**	strings.  We may even ignore the errors from the syntax check.
**
** Inputs:
**	formname	char *		Name of form to get
**	ignore_errs	bool		Whether to ignore derivation string 
**					errors.
**
** Outputs:
**	readerr		i4		error from form read.  Valid only if 
**					non-zero, ignore_errs is TRUE, and
**					return value is NULL.
**	
**
**	Returns:
**		FRAME *		Frame structure for form.  NULL if we couldn't
**				get it.
**
**	Exceptions:
**		none
**
** Side Effects:
**	The form is read from the database.  Dynamic memory is allocated.
**
** History:
**	8/90 (Mike S) Initial version.
*/
FRAME *
IIFGgfGetForm(formname, ignore_errs, readerr)
char	*formname;
bool	ignore_errs;
i4	*readerr;
{
	FRAME *form;
        i4         (*oldfunc)();           /* Previous error handler */

	/*
	** Set some flags, retrieve the form, and restore the flags to their
	** previous state.
	*/
	if (ignore_errs)
	{
		IIFDieIgnoreErrs(TRUE);
		s_readerr = 0;
		oldfunc = IIseterr(frmreaderr);
	}

	FDsetparse(TRUE);
	form = FDwfrcreate(formname, FALSE);
	FDsetparse(FALSE);
	if (ignore_errs)
	{
		IIFDieIgnoreErrs(FALSE);
		_VOID_ IIseterr(oldfunc);
		if (form == NULL)
			*readerr = s_readerr;
	}
	return form;
}

/*
**      Error handler for loading form from database.  We stuff the
**      error number into "s_readerr", so we can display it later.
**	We're ignoring errors from non-existent base fields and columns
**	in derived fields.
*/
static i4
frmreaderr(errnumptr)
ER_MSGID        *errnumptr;	/* Error from FRS */		 
{
	register ER_MSGID errnum = *errnumptr;
	register i4  i;

	for (i = 0; i < sizeof(df_errs)/sizeof(df_errs[0]); i++)
	{
		if (errnum == df_errs[i])
		{
			/* Ignore it */
			errnum = 0;
			break;
		}
	}
	if (errnum != 0)
		s_readerr = errnum;

        return((i4)(errnum));
}

/*
** We've found a vqlock'ed field or column on the form; see if its
** internal type will work with the column inthe VQ.
*/
static bool
typeIsOK(dbtype, frmtype)
DB_DATA_VALUE 	*dbtype;
FLDTYPE		*frmtype;
{
    	i4 frmnull;	/* Is form field (or column) nullable */
    	i4 dbnull;	/* Is VQ type nullable */
	DB_DT_ID dbabstype = abs(dbtype->db_datatype);
			/* Absolute value (nullability removed) of VQ type */
	i4 db_prec =0 , db_scale = 0, db_len = 0;

	/* First see if the basic type is the same */
    	if  (abs(frmtype->ftdatatype) != dbabstype)
		return FALSE;	/* Different types */

	/* For character types, that's it; no length check is done */
	switch(dbabstype)
	{
	    case DB_CHR_TYPE:
	    case DB_TXT_TYPE:
	    case DB_CHA_TYPE:
	    case DB_VCH_TYPE:
		return TRUE;
	}

    	/* 
	** Nullabilty is allowed to change; lenght is not. A nullable type 
	** has one extra byte.
	*/
	dbnull = (dbtype->db_datatype < 0); 
	frmnull = (frmtype->ftdatatype < 0);

	if (dbabstype == DB_DEC_TYPE)
	{
		if ((dbtype->db_length - dbnull) != (frmtype->ftlength - frmnull))
		{
		    db_prec = DB_P_DECODE_MACRO(dbtype->db_prec);
		    db_scale = DB_S_DECODE_MACRO(dbtype->db_prec);
		    db_prec++;
		    if (db_scale)
				db_prec++;
			if (db_prec == frmtype->ftwidth)
				return(TRUE);
			else
				return(FALSE);
		}
		else
			return (TRUE);
	}


	return (dbtype->db_length - dbnull) == (frmtype->ftlength - frmnull);
}
