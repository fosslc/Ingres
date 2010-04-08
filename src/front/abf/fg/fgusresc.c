/*
**	Copyright (c) 1989, 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<ug.h>
# include	<cm.h>
# include	<lo.h>
# include	<st.h>
# include	<si.h>
# include	<abclass.h>
# include	<metafrm.h>
# include	<erfg.h>
# include	"framegen.h"
# include	<fglstr.h>

/**
** Name:	fgusresc.c -	Process a "generate user_escape" statement.
**
**	This file defines:
**	IIFGusresc	top level routine for user escape processing.
**	find_esc_type	find out what type of user_escape code to write out.
**	gen_usresc	write out code for a given user_escape.
**	gen_fldXact 	write out code for after_field_activates.
**	gen_fldNact 	write out code for field_Entry_activates.
**	ins_exitEsc	update structure of field exit Escape info.
**	fnd_exitEsc	search FIELD_EXIT list for match.
**	upd_exitEsc	update existing FIELD_EXIT item
**	init_exitEsc	initialize FIELD_EXIT item
**	gen_FldExit	generate field exit code from FIELD_EXIT list.
**	fgdmpFExit	dump FIELD_EXIT list. 'setenv CCFLAGS -DDEBUG' to use.
**	parse_actname	parse 'fieldname', or 'tbl.col' string.
**	wrtFldNact	write out a field entry escape.
**	wrtFldXact	write out field exit excape.
** 	fnd_all		find an "all" type entry in FIELD_EXIT list. REMOVED.
**
** History:
**	6/2/89 (pete)	Written.
**	11/14/89 (pete) Change LookUp validation error to message the variable
**			IIinvalidmsg, rather than the error message string.
**	12/22/89 (pete) Change name of escape from "query_next" to
**			"query_new_data".
**	1/26/90 (pete)	Write out field "all" type escapes first.
**	4/13/90 (pete)	Change IIinvalidmsg to IIinvalmsg1 & IIinvalmsg2, to
**			give users complete control over format of the message
**			we generate when enter invalid lookup field value.
**	9/90 (Mike S)	After a tablefield validation fails, reset the
**			change bit to 1.
**	2/91 (Mike S) Add "User Menuitem" escape
**	24-jul-92 (blaise)
**		Added new escape types: Before-Lookup, After-Lookup, On-Timeout,
**		On-Dbevent, Table-Field-Menuitems, Menuline-Menuitems. Changed
**		User-Menuitems to map to Table-Field-Menuitems, which replaces
**		it.
**	11-Feb-93 (fredb)
**	   Porting change for hp9_mpe: SIfopen is required for additional
**	   control of file characteristics in fgdmpFExit.
**	12-mar-93 (blaise)
**		Reinstated code to generate a COMMIT statement if no locks are
**		being held.
**      24-sep-96 (hanch04)
**              Global data moved to data.c
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**    25-Oct-2005 (hanje04)
**        Add prototype for gen_FldExit() to prevent compiler
**        errors with GCC 4.0 due to conflict with implicit declaration.
**/

/* # define's */

# define All	ERx("all")

/* extern's */
FUNC_EXTERN VOID IIFGbsBeginSection ();
FUNC_EXTERN VOID IIFGesEndSection ();

GLOBALREF FILE *outfp;
GLOBALREF METAFRAME *G_metaframe;
LSTR *IIFGals_allocLinkStr();
VOID IIFGols_outLinkStr();
char *IIFGbuf_pad();
STATUS IIFGlupval();

/* static's */
static	TAGID	Esctag ={0};		/* memory tag for FIELD_EXIT alloc'ns*/
static	find_esc_type();
static	STATUS	gen_usresc();
static	STATUS	gen_fldXact();
static	STATUS	gen_fldNact();
static	STATUS	ins_exitEsc();
static	FIELD_EXIT *P_festrt;		/* pointer to start of linked list */
static  FIELD_EXIT *fnd_exitEsc();	/* search FIELD_EXIT list for match */
static	VOID	upd_exitEsc();		/* update existing FIELD_EXIT item*/
static	VOID	init_exitEsc();		/* initialize FIELD_EXIT item */
static	bool	parse_actname();	/* parse 'tbl.col' string */ 
static  STATUS	wrtFldNact();		/* write out a field entry escape */
static  STATUS  wrtFldXact();		/* write out field exit excape */
static	STATUS	gen_FldExit(
			FIELD_EXIT *p_festrt,
			char	*p_wbuf,
			char	*infn,
			i4	line_cnt);

/* Note: entries in esc_types do not have to be in sorted order */
static ESC_TYPE esc_types[] = {
	{ERx("append_start"),   ESC_APP_START, F_FG001F_Append_Start },
	{ERx("append_end"),	ESC_APP_END,   F_FG0020_Append_End },
	{ERx("delete_start"),   ESC_DEL_START, F_FG0023_Delete_Start },
	{ERx("delete_end"),	ESC_DEL_END,   F_FG0024_Delete_End },
	{ERx("form_start"),	ESC_FORM_START,F_FG001A_Form_Start },
	{ERx("form_end"),	ESC_FORM_END,  F_FG001B_Form_End },
	{ERx("query_start"),    ESC_QRY_START, F_FG001C_Query_Start },
	{ERx("query_new_data"),	ESC_QRY_NEW_DATA,  F_FG002A_Query_New_Data },
	{ERx("query_end"),	ESC_QRY_END,   F_FG001E_Query_End },
	{ERx("update_start"),   ESC_UPD_START, F_FG0021_Update_Start },
	{ERx("update_end"),	ESC_UPD_END,   F_FG0022_Update_End },
	{ERx("user_menuitems"), ESC_TF_MNUITM, F_FG0043_Table_Field_Menuitems},
	{ERx("table_field_menuitems"), ESC_TF_MNUITM, F_FG0043_Table_Field_Menuitems},
	{ERx("menuline_menuitems"), ESC_ML_MNUITM, F_FG0044_Menuline_Menuitems},
	{ERx("before_lookup"),	ESC_BEF_LOOKUP, F_FG003E_Before_Lookup},
	{ERx("after_lookup"),	ESC_AFT_LOOKUP, F_FG003F_After_Lookup},
	{ERx("on_timeout"),	ESC_ON_TIMEOUT,	F_FG0041_On_Timeout},
	{ERx("on_dbevent"),	ESC_ON_DBEVENT,	F_FG0042_On_Dbevent}
};
#define num_esc_types (sizeof(esc_types)/sizeof(ESC_TYPE))

/*{
** Name:	IIFGusresc - Process a "generate user_escape" statement.
**
** Description:
**	Process a "generate user_escape" statement like:
**
**		## generate user_escape escape_type
**
**	Where 'escape_type' is one of:	form_start, form_end,
**					query_start, query_new_data, query_end,
**					append_start, append_end,
**					update_start, update_end,
**					delete_start, delete_end,
**					before_field_activates,
**					after_field_activates, user_menuitems,
**					table_field_menuitems,
**					menuline_menuitems, before_lookup,
**					after_lookup, on-timeout, on-dbevent.
**		Note 1:	"before_field_activates" & "after_field_activates"
**			are handled differently from the rest
**		Note 2: From 6.5, user_menuitems has been replaced by table_
**			field_menuitems. Any existing "## generate user_escape
**			user_menuitems" statements will be processed as if
**			they were "## generate user_escape table_field_
**			menuitems"
**
** Inputs:
**	i4	nmbr_words;	** number of words in p_wordarray.
**	char	*p_wordarray[];	** words on command line.
**	char	*p_wbuf;	** whitespace to tack to front of generated code
**	char	*infn;		** name of input file currently processing.
**	i4	line_cnt;	** current line number in input file.
**
** Outputs:
**	Writes generated code to FILE *outfp
**
**	Returns:
**		STATUS
**
**	Exceptions:
**		NONE
**
** Side Effects:
**
** History:
**	6/5/89 (pete)	Written.
*/
STATUS
IIFGusresc(nmbr_words, p_wordarray, p_wbuf, infn, line_cnt)
i4	nmbr_words;	/* number of words in p_wordarray */
char	*p_wordarray[];	/* words on command line. */
char	*p_wbuf;	/* whitespace to tack to front of generated code */
char	*infn;		/* name of input file currently processing */
i4	line_cnt;	/* current line number in input file */
{
	i4 row;
	STATUS stat;

	if (nmbr_words > 1)
	{
	    /*Note: "##generate user_escape" already stripped*/

	    IIFGerr (E_FG002C_WrongNmbrArgs, FGCONTINUE, 2, (PTR) &line_cnt,
			(PTR) infn);

	    /* A warning; do not return FAIL -- processing can continue. */
	}
	else if (nmbr_words <= 0)
	{
	    IIFGerr (E_FG002D_MissingArgs, FGFAIL, 2, (PTR) &line_cnt,
			(PTR) infn);
	}

	CVlower(p_wordarray[0]);


	if (STequal(p_wordarray[0],ERx("before_field_activates")))
	{
	    if (gen_fldNact(G_metaframe, p_wbuf, infn, line_cnt) != OK)
		return FAIL;
	}
	else if (STequal(p_wordarray[0],ERx("after_field_activates")))
	{

	    P_festrt = NULL ;	/* init global ptr to start of FIELD_EXIT list*/
	    Esctag = FEgettag(); /* set global memory tag for escape code */

	    stat = gen_fldXact(G_metaframe, p_wbuf, infn, line_cnt);
	    IIUGtagFree(Esctag);	/*release memory for FIELD_EXIT list*/

	    if (stat != OK)
		return FAIL;
	}
	else if ( (row = find_esc_type(p_wordarray[0], esc_types)) != -1)
	{
	    /* it's a user_escape other than "field_XXXX_activates" */

	    if (gen_usresc(esc_types[row].type, esc_types[row].section,
			G_metaframe, p_wbuf) != OK)
		return FAIL;
	}
	else
	{
	    /* unknown escape type */
	    IIFGerr (E_FG002E_UnknownArg, FGFAIL,
			3, (PTR) &line_cnt, (PTR) infn, (PTR) p_wordarray[0]);
	}

	return OK;
}

/*{
** Name:	gen_fldNact - write out code for before_field_activates.
**
** Description:
**	Write out code for all before_field_activates.
**	NOTE: write out any "all" activates first, before the
**	individual field and column activates.
**
** Inputs:
**	METAFRAME *mf
**	char	*p_wbuf		Whitespace to tack to front of generated code.
**	char	*infn		Name of input file currently processing
**	i4	line_cnt	current line number in input file
**
** Outputs:
**
**	Returns:
**		STATUS
**
**	Exceptions:
**		NONE
**
** Side Effects:
**	NONE.
**
** History:
**	7/13/89 (pete)	Initial Version.
**	1/26/90 (pete)	Write out field & column "all" escapes first.
*/
static STATUS
gen_fldNact(mf, p_wbuf, infn, line_cnt) 
METAFRAME *mf;
char	*p_wbuf;	/* whitespace to tack to front of generated code */
char	*infn;		/* name of input file currently processing */
i4	line_cnt;	/* current line number in input file */
{
	register i4  i;
	register MFESC **p_esc;
	char tblfld_name[FE_MAXNAME+1];
	char fld_name[FE_MAXNAME+1];
	/*
	**	Process all Before-Field activates.
	**	Stop searching when reach the Field Change activates
	**	cause there's no ESC_FLD_ENTRY escapes after that (MFESC
	**	is ordered by (*p_esc)->type ).
	**
	** 	Write out field ALL and tblfld.ALL activates first, because
	**	we want individual field activates to over-ride the 'all'
	**	activate; for that to work, the individual field activates must
	**	come last in the source file.
	*/
	for (i=0, p_esc = mf->escs;
	    (i< mf->numescs) && ((*p_esc)->type < ESC_FLD_CHANGE); i++, p_esc++)
	{
	    if ( (*p_esc)->type == ESC_FLD_ENTRY )
	    {
		/* We've encountered a field entry escape.
		** MFESC.item contains 'tablename.columnname' if the 
		** activate is on a table field column; otherwise, it
		** contains 'fieldname'.
		*/
		_VOID_ parse_actname((*p_esc)->item, tblfld_name, fld_name);
		if (STbcompare (fld_name, 0, All, 0, TRUE) == 0)
		{
		    /* this is an "all" field entry escape. write it out */
		    if (wrtFldNact(p_wbuf, (*p_esc)->item,
			(*p_esc)->text ) != OK)
		        return FAIL;
		}
	    }
	}

	/* write out everything but the "all" activates */
	for (i=0, p_esc = mf->escs;
	    (i< mf->numescs) && ((*p_esc)->type < ESC_FLD_CHANGE); i++, p_esc++)
	{
	    if ( (*p_esc)->type == ESC_FLD_ENTRY )
	    {
		_VOID_ parse_actname((*p_esc)->item, tblfld_name, fld_name);
		if (STbcompare (fld_name, 0, All, 0, TRUE) != 0)
		{
		    if (wrtFldNact(p_wbuf, (*p_esc)->item,
			(*p_esc)->text ) != OK)
		        return FAIL;
		}
	    }
	}
	return OK;
}

/*{
** Name:	wrtFldNact - write out a field Entry activate.
**
** Description:
**
** Inputs:
**	char *p_wbuf;	whitespace to put before generated lines.
**	char *item;	name of field or tbl.col.
**	char *text;	escape code.
**
** Outputs:
**
**	Returns:
**		STATUS
**
**	Exceptions:
**		NONE
**
** Side Effects:
**
** History:
**	1/29/90 (pete)	Initial version.
*/
static STATUS
wrtFldNact(p_wbuf, item, text)
char *p_wbuf;
char *item;
char *text;
{
	SIfprintf (outfp, ERx("\n%sBEFORE FIELD '%s' =\n"),
		p_wbuf, item);

	SIfprintf (outfp, ERx("%sBEGIN\n"), p_wbuf);

	IIFGbsBeginSection (outfp, ERget(F_FG0027_Field_Enter), item, TRUE);

	if (IIFGwe_writeEscCode( text, p_wbuf, PAD4, outfp) != OK)
	    return FAIL ;

	IIFGesEndSection (outfp, ERget(F_FG0027_Field_Enter), item, TRUE);

	SIfprintf (outfp, ERx("%sEND\n"), p_wbuf);

	return OK;
}

/*{
** Name:	gen_fldXact - write out code for after_field_activates.
**
** Description:
**	Write out code for all after_field_activates, which includes code for
**	exit and change activates. Lookup validations
**	also get generated as part of the field-exit and field-change
**	activate.
**
** Inputs:
**	METAFRAME *mf
**	char	*p_wbuf		Whitespace to tack to front of generated code.
**	char	*infn		Name of input file currently processing
**	i4	line_cnt	current line number in input file
**
** Outputs:
**
**	Returns:
**		STATUS
**
**	Exceptions:
**		NONE
**
** Side Effects:
**	Allocates memory.
**
** History:
**	6/7/89 (pete)	Written.
*/
static STATUS
gen_fldXact(mf, p_wbuf, infn, line_cnt) 
METAFRAME *mf;
char	*p_wbuf;	/* whitespace to tack to front of generated code */
char	*infn;		/* name of input file currently processing */
i4	line_cnt;	/* current line number in input file */
{
	register i4  i;
	register MFESC **p_esc = mf->escs;

	/*
	**	Note that there may not be field exit or change activates
	**	to process for this frame, but we need to continue processing
	**	below anyway, because a Lookup may exist. We must generate
	**	a field activate block when a Lookup exists for a field.
	*/

	/*
	**	Build a list of structures describing which fields have
	**	Lookup validations, field_exit activations or
	**	field_change activations (if any of these 3 exist, then
	**	we must generate a field activate block).
	**
	**	First build list of fields that have field_exit or
	**	field_change activations. Note that MFESC array is in
	**	MFESC.type order (although the order isn't used here).
	*/

	for (i=0; i< mf->numescs ; i++, p_esc++)
	{
	    if ( ( (*p_esc)->type == ESC_FLD_EXIT) ||
		 ( (*p_esc)->type == ESC_FLD_CHANGE) )
	    {
		if (ins_exitEsc ( (*p_esc)->item, -1, (*p_esc)->type,
				(*p_esc) ) != OK)
		    return FAIL;
	    }
	}

	/*
	**	Now, add to the list built above, info on fields that need
	**	Lookup validations (Lookup validations are done in the
	**	field exit activate block). Fields that need a Lookup
	**	validation query are fields in the primary master
	**	or primary detail table that have a lookup defined on them.
	*/

	for ( i=0 ; i< mf->numjoins ; i++)
	{
	    if (  (mf->joins[i]->type == JOIN_MASLUP)
	       || (mf->joins[i]->type == JOIN_DETLUP))
	    {
		/* save the info on this Lookup Join in the FIELD_EXIT list */

		/*
		** "tab_1.col_1" is the primary table.col in the lookup join.
		** The field activate, where the lookup occurs, is
		** on the field corresponding to the primary table.col
		** (as opposed to the Lookup table table.col).
		*/
		i4 tabindx = mf->joins[i]->tab_1;
		i4 colindx = mf->joins[i]->col_1;

		char *item = mf->tabs[ tabindx]->cols[colindx]->alias;
		char *activate_name;
		if (  mf->joins[i]->type == JOIN_DETLUP
		   || (G_metaframe->mode & MFMODE) == MF_MASTAB
		   )
		{
		    if ( (activate_name = (char *)FEreqmem((u_i4) Esctag,
				(STlength(TFNAME) + 1 + STlength(item) + 1),
				FALSE, (STATUS *)NULL) ) == NULL )
		    {
		        IIFGerr (E_FG0030_OutOfMemory, FGFAIL, 0);
		    }
		    STpolycat (3, TFNAME, ERx("."), item, activate_name);
		}
		else
		    activate_name = item;

		if (ins_exitEsc ( activate_name, i, -1, (MFESC *)NULL) != OK)
		    return FAIL;
	    }
	}

#ifdef DEBUG
	/* dump FIELD_EXIT linked list to file */
	fgdmpFExit (P_festrt, ERx("field_exit.dmp"));
#endif

	/*
	** FIELD_EXIT list is all ready. Now generate code.
	*/
	return (gen_FldExit(P_festrt, p_wbuf, infn, line_cnt));
}

/*{
** Name:	gen_FldExit - Generate Field Exit code from FIELD_EXIT list.
**
** Description:
**	Process the FIELD_EXIT linked list and SIwrite() code like:
**
**	  field-exit activate
**	  begin
**	    if field-change:
**		field-change escape code
**		LOOKUP validation SELECT & error if invalid value.
**	    endif
**	    field-exit escape code
**	    RESUME NEXT
**	  end
**
** Inputs:
**	FIELD_EXIT *p_fe	pointer to start of FIELD_EXIT list to process
**	char	*p_wbuf		whitespace to tack to front of generated code
**	char	*infn		name of input file currently processing
**	i4	line_cnt	current line number in input file
**
** Outputs:
**	Writes code to FILE *outfp.
**
**	Returns:
**		STATUS
**
**	Exceptions:
**		NONE
**
** Side Effects:
**
** History:
**	6/13/89	(pete)	Written.
**	1/29/90	(pete)	Write out field/col "all" activates first.
*/
static STATUS
gen_FldExit(p_festrt, p_wbuf, infn, line_cnt)
FIELD_EXIT *p_festrt;	/* pointer to start of FIELD_EXIT list to process */
char	*p_wbuf;	/* whitespace to tack to front of generated code */
char	*infn;		/* name of input file currently processing */
i4	line_cnt;	/* current line number in input file */
{
	FIELD_EXIT *p_fe;

	/* write out field "all" activates */
	p_fe = p_festrt;
	while (p_fe != NULL)
	{
	    if (p_fe->fld_all)
	    {
		/* write out "all" activate */
	        if (wrtFldXact(p_wbuf, infn, line_cnt, p_festrt, p_fe) != OK)
		    return FAIL;
	    }

	    p_fe = p_fe->next;

	}	/* while() */

	/* write out everything but field "all" activates */
	p_fe = p_festrt;
	while (p_fe != NULL)
	{
	    if ( !(p_fe->fld_all))
	    {
	        if (wrtFldXact(p_wbuf, infn, line_cnt, p_festrt, p_fe) != OK)
		    return FAIL;
	    }

	    p_fe = p_fe->next;

	}	/* while() */

	return OK;
}

/*{
** Name:	wrtFldXact - write out field exit escape.
**
** Description:
**	Write out field exit escape code, using a FIELD_EXIT structure.
**
** Inputs:
**	char	*p_wbuf;	whitespace to tack to front of generated code
**	char	*infn;		name of input file currently processing
**	i4	line_cnt;	current line number in input file
**	FIELD_EXIT *p_festrt;	pointer to start of FIELD_EXIT list to process
**	FIELD_EXIT *p_fe;	pointer to current entry in FIELD_EXIT list
**
** Outputs:
**
**	Returns:
**		OK if code written out okay. FAIL otherwise.
**
**	Exceptions:
**		NONE
**
** History:
**	1/29/90 (pete)	Initial Version. Write out exit 'all' escape for fields
**			that have a Change escape but no exit escape.
**	12/14/90 (pete) Fix for bug 34961: generate code that skips
**			the lookup validate select when form is in 
**			QUERY mode. So user can enter query value like
**			'>123' or 'abc*'. Side effect (desirable one, I think):
**			Field change user_escapes won't fire when form
**			is in QUERY mode.
**	1/11/91 (pete)	Generate "AFTER FIELD" rather than just "FIELD".
**			Suggestion from training.
*/
static STATUS
wrtFldXact(p_wbuf, infn, line_cnt, p_festrt, p_fe)
char	*p_wbuf;	/* whitespace to tack to front of generated code */
char	*infn;		/* name of input file currently processing */
i4	line_cnt;	/* current line number in input file */
FIELD_EXIT *p_festrt;	/* pointer to start of FIELD_EXIT list to process */
FIELD_EXIT *p_fe;	/* pointer to current entry in FIELD_EXIT list */
{
        LSTR *p_lstr_head;	/* for working with linked list of strings*/
        LSTR *p_lstr;

	char *p_wbuf4;	/* p_wbufn == p_wbuf with 'n' more spaces added on */
	char *p_wbuf8;
	char *p_wbuf12;

	p_wbuf4 = IIFGbuf_pad(p_wbuf, PAD4);
	p_wbuf8 = IIFGbuf_pad(p_wbuf, PAD8);
	p_wbuf12 = IIFGbuf_pad(p_wbuf, PAD12);

	SIfprintf (outfp, ERx("\n%sAFTER FIELD '%s' =\n"), p_wbuf, p_fe->item);

	SIfprintf (outfp, ERx("%sBEGIN\n"), p_wbuf);

	if ( (p_fe->jtindx != -1) || (p_fe->changeesc != NULL) )
	{
		/* We must generate a change block. Then insert either a Lookup
		** validation or user's change activate code, or both.
		*/

		/* Check for field change (diff. syntax for simple field
		** and table field column). Parse p_fe->item into table
		** field name (if any) and field or column name.
		*/
		if (p_fe->table_fld)
		{
		    /* p_fe->item contains a '.' (table field activate) */

		    SIfprintf (outfp,
		      ERx("%sINQUIRE_FORMS ROW '' '' (IIint = CHANGE);\n"),
		      p_wbuf4);
		}
		else
		{
		    /*p_fe->item doesn't contain a '.' (simple field activate)*/

		    SIfprintf (outfp,
		      ERx("%sINQUIRE_FORMS FIELD '' (IIint = CHANGE);\n"),
		      p_wbuf4);
		}

		SIfprintf (outfp,
		    ERx("%sINQUIRE_FORMS FORM (IIobjname = MODE);\n"), p_wbuf4);

		SIfprintf (outfp,
	   ERx("%sIF (IIint = 1) AND (UPPERCASE(IIobjname) != 'QUERY') THEN\n"),
		p_wbuf4);

	    	if (p_fe->changeesc != NULL)
		{
		    /* user entered change escape code. insert it. */

               	    IIFGbsBeginSection (outfp, ERget(F_FG0028_Field_Change),
				        p_fe->item, TRUE);

	 	    if (IIFGwe_writeEscCode( p_fe->changeesc->text, p_wbuf8,
			PAD0, outfp) != OK)
		    {
			return FAIL;
		    }

               	    IIFGesEndSection (outfp, ERget(F_FG0028_Field_Change),
				      p_fe->item, TRUE);
		}

	    	if (p_fe->jtindx != -1)
		{
	    	    /* generate Lookup validation SELECT */

		    if (IIFGlupval(p_wbuf8, infn, line_cnt, p_fe->jtindx) != OK)
		    {
			return FAIL;
		    }

		    /* check query rowcount */
		    SIfprintf (outfp,
		        ERx("%sINQUIRE_SQL (IIrowcount = ROWCOUNT);\n"),
			p_wbuf8);

		    /*
		    ** If frame behaviour not set to hold locks
		    **    release lookup table shared locks by committing work
		    */
		    if ( !(G_metaframe->mode & MF_DBMSLOCK) )
		    {
			SIfprintf (outfp,
		          ERx("%sCOMMIT WORK;\t\t/* release shared locks */\n"),
			  p_wbuf8);
		    }

		    SIfprintf (outfp,
				ERx("%sIF IIrowcount <= 0 THEN\n"), p_wbuf8);

		    /* No rows found. invalid entry on screen */

		    /* sound terminal bell */
		    SIfprintf (outfp,
		      ERx("%sCALLPROC beep();\t/* 4gl built-in function */\n"),
			p_wbuf12);

		    /* since this 'message' string is long, use Mike's
		    ** list string allocation & print routines. (these
		    ** LinkStr routines manage their own memory tag).
		    */
		    if ((p_lstr = IIFGals_allocLinkStr ((LSTR *)NULL,
			LSTR_NONE)) == NULL)
		    {
			return FAIL;
		    }

		    p_lstr_head = p_lstr;   /* save head of list */

		    STcopy (ERx("MESSAGE IIinvalmsg1 + '\"' +"),
			p_lstr->string);

		    if ((p_lstr = IIFGals_allocLinkStr ((LSTR *)p_lstr,
			LSTR_NONE)) == NULL)
		    {
			return FAIL;
		    }

		    /*
		    ** "IIinvalmsg1 & IIinvalmsg2 are 4GL local variables that
		    ** get values assigned to them in the frame INITIALIZE
		    ** block. By default nothing is assigned to IIinvalmsg1,
		    ** and something like " is not a valid value" is assigned
		    ** to IIinvalmsg2. These 2 variables are only declared
		    ** when a lookup table exists.
		    */ 
		    STprintf (p_lstr->string,
			ERx("IFNULL(VARCHAR(:%s), '') + '\"' + IIinvalmsg2"),
			p_fe->item );
/*
		    if ((p_lstr = IIFGals_allocLinkStr ((LSTR *)p_lstr,
			LSTR_NONE)) == NULL)
		    {
			return FAIL;
		    }
		    STprintf (p_lstr->string, ERx("' %s'"),
			ERget(F_FG000C_NotValidValue));
*/

		    if ((p_lstr = IIFGals_allocLinkStr ((LSTR *)p_lstr,
			LSTR_SEMICOLON)) == NULL)
		    {
			return FAIL;
		    }
		    STcopy (ERx("WITH STYLE = POPUP"), p_lstr->string);
		    p_lstr->nl = TRUE;

		    /* now, write LSTR structure built above to 4gl file.
		    */
		    IIFGols_outLinkStr (p_lstr_head, outfp, p_wbuf12);

		    if (p_fe->table_fld)
		    {
			/* Reset the Row change bit */
			SIfprintf (outfp, 
				   ERx("%sSET_FORMS ROW '' '' (CHANGE = 1);\n"),
				   p_wbuf12);
		    }
		    SIfprintf (outfp, ERx("%sRESUME;\n"), p_wbuf12);

		    SIfprintf (outfp, ERx("%sENDIF;\n"), p_wbuf8); /*IIint<=0*/
		}

		SIfprintf (outfp, ERx("%sENDIF;\n"), p_wbuf4); /* IIint=1 */
	}

	if (p_fe->exitesc != NULL)
	{
		/* We need to insert user's exit activate code */

               	IIFGbsBeginSection (outfp, ERget(F_FG0029_Field_Exit),
				    p_fe->item, TRUE);
	        if (IIFGwe_writeEscCode( p_fe->exitesc->text, p_wbuf4,
			PAD0, outfp) != OK)
		{
		    return FAIL;
		}

               	IIFGesEndSection (outfp, ERget(F_FG0029_Field_Exit),
				   p_fe->item, TRUE);
	}
#if 0
	/* this section (temporarily?) removed. What this does:
	** for fields with a change escape, but no exit escape, find
	** and write out the exit escape for "all". This was discussed
	** by Emerald group, and decided not to be used.
	*/
	else
	{
	    if (p_fe->changeesc != NULL)
	    {
		/* Change escape, but no corresponding exit escape.
		** Check for a corresponding (simple or table field)
		** "all" exit escape and insert it here (unless
		** the current field is already an "all" type field).
		*/
		if ( !(p_fe->fld_all))
		{
		    /* current field is not "all" or "tblfld.all" */

		    FIELD_EXIT *p_fetmp;

		    if ((p_fetmp = fnd_all (p_festrt, p_fe->table_fld)) != NULL)
		    {
			/* found appropriate type of 'all' exit escape (simple
			** or table field 'all', depending on second argument
			** to fnd_all().
			*/
			SIfprintf (outfp,
			   ERx("%s%s\n"), p_wbuf4, ERget(F_FG0032_CopyExitAll));

               		IIFGbsBeginSection (outfp, ERget(F_FG0029_Field_Exit),
				    p_fetmp->item, TRUE);
	        	if (IIFGwe_writeEscCode( p_fetmp->exitesc->text,
			    p_wbuf4, PAD0, outfp) != OK)
			{
		    	    return FAIL;
			}

               		IIFGesEndSection (outfp, ERget(F_FG0029_Field_Exit),
			    p_fetmp->item, TRUE);
		    }
		}
	    }
	}
#endif

	SIfprintf (outfp, ERx("%sRESUME NEXT;\n"), p_wbuf4);
	SIfprintf (outfp, ERx("%sEND\n"), p_wbuf);

	return OK;
}

#if 0
/* see comment above, and #if 0, for discussion why this removed */

static  FIELD_EXIT *fnd_all();		/* find an "all" FIELD_EXIT entry */
/*{
** Name:	fnd_all - find a simple or table field "all" escape.
**
** Description:
**
** Inputs:
**	FIELD_EXIT p_fe;	beginning of linked list to search.
**	bool	table_fld;	TRUE if should search for table field "all";
**				FALSE if should search for simple field "all".
**
** Outputs:
**
**	Returns:
**		pointer to appropriate FIELD_EXIT entry; otherwise
**		NULL, if none was found.
**
**	Exceptions:
**		NONE
**
** Side Effects:
**
** History:
**	1/29/90 (pete)	Initial version.
*/
static FIELD_EXIT
*fnd_all (p_fe, table_fld)
FIELD_EXIT *p_fe;
bool	table_fld;
{
	/* loop thru FIELD_EXIT linked list looking for appropriate type
	** type of "all" exit activate.
	*/
	while (p_fe != NULL)
	{
	    if (p_fe->fld_all && (p_fe->exitesc != NULL))
	    {
		/* field "all" type escape exists */
	        if (p_fe->table_fld == table_fld)
	        {
		    /* it is the desired type (simple field or table field) */
		    return p_fe;
	        }
	    }

	    p_fe = p_fe->next;

	}	/* while() */

	return NULL;
}
#endif

/*{
** Name:	parse_actname - parse a 'tblfld.column' string.
**
** Description:
**	Parse 'fieldname' or 'tablefield.fieldname' into parts.
**
**	If 'item' contains a '.' this function:
**
**		Returns TRUE, and
**		STcopy's the table-field-name and field-name to the second
**		  and third arguments respectively (which are presumed to
**		  be long enough).
**
**	If 'item' does not contain a '.', then the function returns
**	FALSE and copies the field-name to both the second and third
**	arguments.
**
** Inputs:
**	char	*item
**	char	*tblfld_name
**	char	*fld_name
**
** Outputs:
**
**	Returns:
**		TRUE if 'item' is a table field; FALSE otherwise.
**
**	Exceptions:
**		NONE
**
** Side Effects:
**
** History:
**	8/14/89	(pete)	Initial version.
*/
static bool
parse_actname(item, tblfld_name, fld_name) 
char	*item;
char	*tblfld_name;	/* this gets written to if 'item' contains a '.' */
char	*fld_name;	/* this gets written to if 'item' contains a '.' */
{
	bool item_istblfld = FALSE;
	char *src;
	char *dest = tblfld_name;

	for (src=item; *src != EOS; CMnext(src))
	{
	    if (*src != '.')
	    {
		CMcpychar(src, dest);
		CMnext(dest);
	    }
	    else
	    {
		/* we're parsing "tblfld.column" */

		*dest = EOS;	  /* table name all set; null terminate it */

		dest = fld_name;  /* column name follows '.'; copy there next */
	        item_istblfld = TRUE;
	    }
	}
	*dest = EOS;	/* null terminate whatever we were copying to
			** above when hit EOS.
			*/
	if (!item_istblfld)
	{
	    /* we never encountered a '.' in item */
	    STcopy (tblfld_name, fld_name);
	    *tblfld_name = EOS;
	}

	return (item_istblfld);
}

/*{
** Name:	ins_exitEsc - update structure of field exit Escape info.
**
** Description:
**	Maintain a linked list of FIELD_EXIT structures. If an item
**	already exists in list, then update that entry with
**	not null arguments. Otherwise, if new, then insert a new
**	entry in the list with these values.
**	Activates needed cause of a lookup don't have a 'esc_type', but will
**	have a 'jtindx' (set to -1) to point to the lookup join.
**	Field exit and change activates will have an 'esc_type', but won't
**	have a 'jtindx' (set to -1).
**
** Inputs:
**	char	*item		name of a field on the form
**	i4	jtindx		MFJOIN index for MASLUP or DETLUP join (or -1)
**	i4	esc_type	escape type (or -1).
**	MFESC	*p_mfesc	pointer to MFESC for this escape (or NULL).
**
** Outputs:
**	Updated or new FIELD_EXIT entry in linked list.
**
**	Returns:
**		An exception is raised if memory allocation fails.
**		Always returns OK.
**
**	Exceptions:
**		none
**
** Side Effects:
**	Allocates dynamic memory. Creates a new tag.
**
** History:
**	6/7/89	(pete)	Written.
*/
static STATUS
ins_exitEsc (item, jtindx, esc_type, p_esc )
char	*item;		/* name of a field on the form */
i4	jtindx;		/* MFJOIN index for MASLUP or DETLUP join (or -1 if no
			MFJOIN applies for this escape; -1 happens for field
			exit and change Escapes) */
i4	esc_type;	/* escape type (or -1 if used didn't enter an escape &
			this activate is needed for a Lookup validation query)*/
MFESC	*p_esc;		/* pointer to MFESC for this escape ;*/
{
	FIELD_EXIT *p_fe ;

	if (P_festrt != NULL)
	{
	    /* List already exists. search it for match. */
	    if ( (p_fe = fnd_exitEsc (P_festrt, item)) != NULL)
	    {
		/* found match. update it. */
		upd_exitEsc (p_fe, jtindx, esc_type, p_esc);
		return OK;
	    }
	}

	/* Either no match above, or empty list (P_festrt == NULL).
	** Add new item to list. reset global ptr.
	*/
	if ( (p_fe = (FIELD_EXIT *)FEreqmem((u_i4) Esctag, sizeof(FIELD_EXIT),
				FALSE, (STATUS *)NULL) ) == NULL )
	{
	    IIFGerr (E_FG0030_OutOfMemory, FGFAIL, 0);
	}

	/* Assertion: new memory all set. Initialize values in new item */
	init_exitEsc (p_fe, item, jtindx, esc_type, p_esc);

	/* add new item to front of list */
	p_fe->next = P_festrt ;
	P_festrt   = p_fe;	/* reset global pointer to new start of list */

	return OK;
}

/*{
** Name:	init_exitEsc - Initialize values in new FIELD_EXIT entry.
**
** History:
**	6/9/89	(pete)	Written.
**	1/29/90 (pete)  Set initial values for fields "table_fld" & "fld_all".
*/
static VOID
init_exitEsc (p_fe, item, jtindx, esc_type, p_esc)
FIELD_EXIT *p_fe;	/* pointer to space allocated for new entry */
char	*item;		/* name of a field on the form */
i4	jtindx;		/* MFJOIN index for MASLUP or DETLUP join (or -1) */
i4	esc_type;	/* escape type */
MFESC	*p_esc;		/* pointer to MFESC for this escape ;*/
{
	char tblfld_name[FE_MAXNAME];
	char fld_name[FE_MAXNAME+1];

	p_fe->item      = item;
	p_fe->table_fld = parse_actname(p_fe->item,
	    				  tblfld_name, fld_name);
	p_fe->fld_all   = (STbcompare(fld_name, 0, All, 0, TRUE) == 0)
			  ? TRUE : FALSE;

	p_fe->jtindx    = jtindx;

	if (esc_type == ESC_FLD_EXIT)
	{
	    p_fe->exitesc = p_esc;
	    p_fe->changeesc = (MFESC *)NULL;
	}
	else if (esc_type == ESC_FLD_CHANGE)
	{
	    p_fe->exitesc = (MFESC *)NULL;
	    p_fe->changeesc = p_esc ;
	}
	else
	    p_fe->exitesc = p_fe->changeesc = (MFESC *)NULL;

	p_fe->next = (FIELD_EXIT *)NULL;
}

/*{
** Name:	upd_exitEsc - update an Escape entry in the FIELD_EXIT.
**
** History:
**	6/9/89	(pete)	Written.
*/
static VOID
upd_exitEsc (p_fe, jtindx, esc_type, p_esc)
FIELD_EXIT	*p_fe;	/* update this guy */
i4	jtindx;		/* MFJOIN index for MASLUP or DETLUP join (or -1) */
i4	esc_type;	/* escape type (MFESC.type) */
MFESC	*p_esc;		/* pointer to MFESC for this escape ;*/
{
	if ( jtindx != -1)	/* -1 means "don't update this" */
	    p_fe->jtindx = jtindx;

	if (esc_type == ESC_FLD_EXIT)
	    p_fe->exitesc = p_esc;

	if (esc_type == ESC_FLD_CHANGE)
	    p_fe->changeesc = p_esc ;
}

/*{
** Name:	fnd_exitEsc - Find a matching escape in the FIELD_EXIT list.
**
** Description:
**	Search the FIELD_EXIT linked list for a matching entry. Match on
**	both item name and table section.
**
** Inputs:
**	FIELD_EXIT *p_fe	where to begin the search.
**	char       *item	item name to look for.
**
** Outputs:
**
**	Returns:
**		pointer to matching FIELD_EXIT or NULL if no match found.
**
**	Exceptions:
**		NONE
**
** Side Effects:
**
** History:
**	6/12/89	(pete)	Written.
*/
static FIELD_EXIT
*fnd_exitEsc (p_fe, item)
FIELD_EXIT *p_fe;
char       *item;
{
	do {
	    if ( STequal(p_fe->item, item) )
	    {
		break;	/* found it. drop out of loop */
	    }
	    else
		p_fe = p_fe->next ;

	} while ( p_fe != NULL );

	return p_fe;
}

/*{
** Name:	gen_usresc - generate code for a simple user_escape.
**
** Description:
**	Write code to the output file for a simple user_escape "type" passed
**	as an argument. These are escapes where the code can simply be
**	written out without having to also generate an activate block.
**
** Inputs:
**	i4 type	MFESC type from METAFRAME.
**	ER_MSGID section 	Section name for ^G comment that brackets
**				user code.
**	METAFRAME *mf	metaframe to search for this type in (search in MFESC).
**	char		*p_wbuf whitespace to tack to front of generated code 
**
** Outputs:
**
**	Returns:
**		STATUS
**
**	Exceptions:
**		NONE
**
** Side Effects:
**
** History:
**	6/6/89	(pete)	Written.
**	2/91 Don't assume that Field escapes come at the end.
*/
static STATUS
gen_usresc(type, section, mf, p_wbuf)
i4  type;		/* type of this MFESC (see METAFRAME description) */
ER_MSGID section;	/* section name for ^G comment that brackets user code*/
METAFRAME *mf;
char	*p_wbuf;	/* whitespace to tack to front of generated code */
{
	register i4  i;
	register MFESC **p_esc = mf->escs;

	/*
	**	Stop searching when reach the Field activate escapes
	**	at end of MFESC array (field activates are the most
	**	numerous ones, and come at the end).
	*/
	for (i=0; i< mf->numescs; i++, p_esc++)
	{
	    if (type == (*p_esc)->type)
	    {
		/* User entered code for this escape. Write it out */

	        IIFGbsBeginSection (outfp, ERget(section), (char *)NULL, TRUE);

		if (IIFGwe_writeEscCode( (*p_esc)->text, p_wbuf,
			PAD0, outfp) != OK)
		    return FAIL ;

		IIFGesEndSection (outfp, ERget(section), (char *)NULL, TRUE);
	    }
	}
	return OK;
}

/*{
** Name:	find_esc_type - find an escape type in ESC_TYPE array.
**
** Description:
**	Search array for matching escape type ("query_start", "update_end", etc)
**
** Inputs:
**	char *word	Word to look for in array.
**	ESC_TYPE *p_esc	Pointer to array to search for word in.
**
** Outputs:
**
**	Returns:
**		Row number in array where match was found
**		or -1 if no match.
**
**	Exceptions:
**		NONE
**
** Side Effects:
**
** History:
**	6/6/89 (pete)	Written.
*/
static
find_esc_type(word, p_esc)
char *word;
ESC_TYPE *p_esc;
{
	i4 i;

	for (i = 0; i < num_esc_types; i++, p_esc++)
	{
	    if (STequal(p_esc->name, word ))
		return i;
	}

	return (-1);
}

#ifdef DEBUG
fgdmpFExit (fe, fn)
FIELD_EXIT *fe;	/* beginning of linked list of FIELD_EXIT structs to dump */
char *fn;	/* file name to write dump to */
{

	LOCATION fedmploc;
	GLOBALREF FILE  *fedmpfile;
	char buf[MAX_LOC+1];

	STcopy (fn, buf);
	LOfroms (FILENAME&PATH, buf, &fedmploc);
#ifdef hp9_mpe
	if (SIfopen (&fedmploc, ERx("w"), SI_TXT, 252, &fedmpfile) != OK)
#else
	if (SIopen (&fedmploc, ERx("w"), &fedmpfile) != OK)
#endif
	{
	    SIprintf(ERx("Error: unable to open FIELD_EXIT dump file: %s"),buf);
	    return;
	}

	SIfprintf (fedmpfile, ERx("\nFIELD_EXIT linked list:\n"));

	while ( fe != NULL )
	{
	    SIfprintf (fedmpfile, ERx("\n(&%d) %s, %d, &%d, &%d, &%d."),
		fe, fe->item, fe->jtindx, fe->changeesc,
		fe->exitesc, fe->next);

	    fe = fe->next ;
	}

	if (fedmpfile != NULL)
	    SIclose(fedmpfile);	
}
#endif
