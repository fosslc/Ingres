/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<cm.h> 
# include	<fe.h>
# include	<ui.h>
# include	<stype.h> 
# include	<sglob.h> 

/*
**   S_G_IDENT - get an identifier string from the internal line.  An
**		 identifier may be delimited or regular, but is always
**		 considered NOT to be in normalized form.  Thus, if it
**		 starts with a double quote, then it must end with a double
**		 quote
**
**		 An identifier which exceeds the size of the static ident
**		 will be silently truncated, and may result in it's being
**		 invalid.  However, in all cases Tokchar will advance to the
**		 end of the token.
**
** Input:
**	inc	TRUE if Tokchar is to be left pointing to the character after
**		the last chararacter of identifier (which may be EOS).
**	type	Address of a i4  to hold the determined type of the
**		identifier - UI_DELIM_ID, UI_REG_ID, or UI_BOGUS_ID.
**	c_ok	TRUE if a compound identifier (["]a["].["]b["]) is
**		allowable in this context.
**
** Output:
**	type	Set to the appropriate identifier type.  Note that this will
**		be misleading when the identifier string is compound.  In that
**		case, a return of UI_BOGUS_ID will not indicate which component
**		is invalid, and a valid return will always reflect the type
**		of the last component.
**
** Returns:
**	ident	Pointer to the un-normalized identifier name, which is in
**		a local static buffer.  Not valid if type == UI_BOGUS_ID. 
**
** History:
**	16-sep-1992 (rdrane)
**		Created.
**	11-nov-1992 (rdrane)
**		Correct sizing of ident_buf - it didn't allow for the
**		compound_delim character.
**	24-nov-92 (rdrane)
**		Enable the c_ok parameter, and use that as the criteria for 
**		recognizing compound constructs rather than St_xns_ok.
**		Allow for whitespace around a compound_delim character, but
**		don't allow for comments.
**	22-jan-1992 (rdrane)
**		Correct sizing of ident_buf - it wasn't allowing for the
**		second component of a compound construct (how did that
**		happen?).
**	18-feb-1993 (rdrane)
**		Disallow leading whitespace around a compound_delim character,
**		as this breaks certain instances of multiple commands per
**		source line.
**	8-apr-1993 (rdrane)
**		Replace the main logic loop with a call to IIUGgci_getcid()
**		(which was esentially created from the main logic loop).
**	1-oct-1993 (rdrane)
**		If any part of a compound identifier is delimited, then
**		consider the entire identifier to be delimited.  Otherwise,
**		validations may fail (cf. s_q_add()).  This turned up while
**		investigating RBF bug 54949.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/



char *
s_g_ident(inc,type,c_ok)
bool	inc;
i4	*type;
bool	c_ok;
{
	char	*save_Tokchar;
	i4	o_type;
	bool	gci_stat;

	/*
	** Buffer for identifier, which may be an
	** un-normalized compound construct.
	*/
    static FE_GCI_IDENT	ugid;


	save_Tokchar = Tokchar;

	gci_stat = IIUGgci_getcid(Tokchar,&ugid,c_ok);

	Tokchar += ugid.b_oset;
	if  (!inc)
	{
		CMprev(Tokchar,save_Tokchar);
	}

	/*
	** Validate the identifier, and don't bother to
	** normalize it.  Note that we can't have a compound
	** construct unless c_ok == TRUE.
	*/
	if  (gci_stat)
	{
		if  (ugid.c_oset != 0)
		{
			/*
			** Validate the individual identifier components.
			** If either is bogus, then the compound identifier
			** is bogus.  If either is delimited, then the
			** compound identifier is delimited.
			*/
			o_type = IIUGdlm_ChkdlmBEobject(&ugid.owner[0],
						       NULL,FALSE);
			if  (o_type != UI_BOGUS_ID)
			{
				*type = IIUGdlm_ChkdlmBEobject(&ugid.object[0],
							       NULL,FALSE);
			}
			/*
			** This is a short cut.  If o_type is not delimited,
			** then we can simply let the previous type
			** determination stand.  If both o_type and type are
			** delimited, then it's a redundant operation.  What
			** we're really trying to handle is the "a".b case.
			*/
			if  (o_type == UI_DELIM_ID)
			{
				*type = o_type;
			}
		}
		else
		{
			*type = IIUGdlm_ChkdlmBEobject(&ugid.object[0],
						       NULL,FALSE);
		}
	}
	else
	{
		/*
		** If the parse failed, we must consider the identifier
		** to be bogus!
		*/
		*type = UI_BOGUS_ID;
	}

	return(&ugid.name[0]);
}



/*
**   S_G_TBL   - get an identifier string from the internal line which may
**		 be an owner.tablename construct.  The owner and table
**		 components may be delimited or regular.  The entire construct
**		 is always considered NOT to be in normalized form.
**
** Input:
**	inc	TRUE if Tokchar is to be left pointing to the character after
**		the last chararacter of identifier (which may be EOS).  This
**		is passed to s_g_ident().
**	rtn_bool
**		Address of a bool.  Set to TRUE if identifier is valid, FALSE
**		if not.
**
** Output:
**	rtn_bool
**		Set appropriately.
**
** Returns:
**	tbl	Pointer to the un-normalized identifier name, which is in
**		a local static buffer.  Not valid if rtn_bool == FALSE. 
**
** History:
**	16-sep-1992 (rdrane)
**		Created.
**	24-nov-1992 (rdrane)
**		Minor corrections to internal variable settings.
*/



char *
s_g_tbl(inc,rtn_bool)
bool	inc;
bool	*rtn_bool;
{
	i4	rtn_char;
	char	owner_buf[(FE_UNRML_MAXNAME + 1)];
	char	name_buf[(FE_UNRML_MAXNAME + 1)];
	FE_RSLV_NAME w_frn;

    static char	tbl_buf[FE_MAXTABNAME];


	/*
	** Parse out whatever identifier we have.  Ignore the value
	** of rtn_char, since it will be misleading for an owner.tablename.
	*/
	w_frn.name = s_g_ident(inc,&rtn_char,TRUE);

	/*
	** Decompose any owner.tablename, and validate the pieces.  Note that
	** we need to get the normalized form of each piece so that
	** FE_unresolve() can later reconstruct the name, and this also
	** saves a later explicit casing.
	*/
	w_frn.owner_dest = &owner_buf[0];
	w_frn.name_dest = &name_buf[0];
	FE_decompose(&w_frn);
	if  (w_frn.owner_spec)
	{
	     rtn_char = IIUGdlm_ChkdlmBEobject(w_frn.owner_dest,
					       w_frn.owner_dest,FALSE);
	    if  ((rtn_char == UI_BOGUS_ID) ||
		 ((rtn_char == UI_DELIM_ID) && (!St_xns_given)))
	    {
		s_error(0x38F,NONFATAL,w_frn.name,NULL);
		*rtn_bool = FALSE;
		return(&tbl_buf[0]);
	    }

	    if  (rtn_char == UI_DELIM_ID)
	    {
		w_frn.owner_dlm = TRUE;
	    }
	    else
	    {
		w_frn.owner_dlm = FALSE;
	    }
	}

	rtn_char = IIUGdlm_ChkdlmBEobject(w_frn.name_dest,
					  w_frn.name_dest,FALSE);
	if  ((rtn_char == UI_BOGUS_ID) ||
	     ((rtn_char == UI_DELIM_ID) && (!St_xns_given)))
	{
		s_error(0x38F,NONFATAL,w_frn.name,NULL);
		*rtn_bool = FALSE;
		return(&tbl_buf[0]);
	}

	if  (rtn_char == UI_DELIM_ID)
	{
		w_frn.name_dlm = TRUE;
	}
	else
	{
		w_frn.name_dlm = FALSE;
	}

	/*
	** Reconstruct the name.  It's now properly cased, but we want it
	** in otherwise un-normalized form.
	*/
	w_frn.name = &tbl_buf[0];
	FE_unresolve(&w_frn);

	*rtn_bool = TRUE;
	return(&tbl_buf[0]);
}

