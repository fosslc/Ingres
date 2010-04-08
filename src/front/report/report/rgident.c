/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<cv.h>
# include	<gl.h>
# include	<me.h>
# include	<sl.h>
# include	<st.h>
# include	<iicommon.h>
# include	<cm.h> 
# include	<adf.h>
# include	<adfops.h>
# include	<fmt.h>
# include	<fe.h>
# include	<er.h>
# include	<ui.h>
# include	<ug.h>
# include	<rtype.h> 
# include	<rglob.h> 

/*
**   R_G_IDENT - get an identifier string from the internal line.
**		 An identifier may be delimited or regular, but is always
**		 considered NOT to be in normalized form.  Thus, if it
**		 starts with a double quote, then it must end with a double
**		 quote
**
**		 An identifier which exceeds the max allowed size will be
**		 silently truncated (but SREPORT should have caught this!).
**		 In all cases Tokchar will advance to the end of the token.
**
** Input:
**	c_ok	Set to TRUE if compound constructs are valid in the context
**		for which this routine was called.
**
** Output:
**	None.
**
** Returns:
**	ident_ptr
**		Pointer to the un-normalized identifier name, which is in
**		dynamically allocated permanent storage.
**
** History:
**	9-oct-1992 (rdrane)
**		Created.
**	24-nov-92 (rdrane)
**		Correct sizing of ident_buf - it didn't allow for the
**		compound_delim character.
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
**		source line (which SREPORT should have handled ...).
**	8-apr-1993 (rdrane)
**		Replace the main logic loop with a call to IIUGgci_getcid()
**		(which was essentially created from the main logic loop
**		of the SREPORT version of this routine).
**	26-Aug-1993 (fredv)
*8		Included <st.h>.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


char *
r_g_ident(c_ok)
bool	c_ok;
{
	i4	i;
	char	*ident_ptr;
    static FE_GCI_IDENT	ugid;


	_VOID_ IIUGgci_getcid(Tokchar,&ugid,c_ok);

	Tokchar += ugid.b_oset;

	i = STlength(&ugid.name[0]);
	ident_ptr = (char *)MEreqmem((u_i4)0,(u_i4)(i + 1),
				     TRUE,(STATUS *)NULL);
	if  (ident_ptr == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("r_g_ident - ident_ptr"));
	}

	_VOID_ STcopy(&ugid.name[0],ident_ptr);

	return(ident_ptr);
}

