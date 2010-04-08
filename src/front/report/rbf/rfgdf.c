/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<cm.h>
# include	<me.h>
# include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ug.h>
# include	<ui.h>
# include	"rbftype.h"
# include	"rbfglob.h"

/*
**   RFGDF_GETDEFFMTS -
**	Extract the format strings for the header/footer date/time and page
**    	number report variables from their .DECLARE statements, and copy the
**	strings (if any) into their respective RBF variable.
**
**	We have one of three possibilities:
**
**		Nothing found, ren_level < LVL_RBF_PG_FMT -
**			This is a newly created or pre-6.5 feature).  Default
**			Opt.*_inc_fmt to 'y' for all components.
**
**		Nothing found, ren_level >= LVL_RBF_PG_FMT -
**			The user has deleted all three components.  Default
**			Opt.*_inc_fmt to 'n' for all components, and do not
**			look for any additional RBF format variables.
**
**		One or more found -
**			Set the coresponding Opt.*_inc_fmt to 'y'.
**
**    Parameters:
**	None.
**
**    Returns:
**	Nothing
**
**    History:
**	1-jul-1993 (rdrane)
**		Created.
**	12-jul-1993 (rdrane)
**		Correct parameterization of fmt_setfmt() in rFpdf_pdeffmts().
**	08-30-93 (huffman)
**		add <me.h>
**	13-sep-1993 (rdrane)
**		Check report level when setting of the Opt.*_inc_fmt values
**		so that we can differentiate between a new or existing pre-6.5
**		report with no user-specified overide format, and a report
**		where the user has declined to include any of the items.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

	static	VOID	rFpdf_pdeffmts();

VOID
rFgdf_getdeffmts()
{
	DCL	*tdcl;


	/*
	** If the report version is below that which supports
	** the RBF format variables, simply turn all of the
	** options on and return.  En_ren is NULL for newly
	** created reports ...
	*/
	if  ((En_ren == (REN *)NULL) || (En_ren->ren_level < LVL_RBF_PG_FMT))
	{
		Opt.rdate_inc_fmt = ERx('y');
		Opt.rtime_inc_fmt = ERx('y');
		Opt.rpageno_inc_fmt = ERx('y');
		return;
	}

	/*
	** Default the RBF format variables to "not present, then
	** search the DCL chain for any declared variables having
	** the name of the RBF format variables.  If found, then
	** call a sub-routine to parse the .DECLARE text and extract
	** the VALUE portion containing the format string.
	*/
	Opt.rdate_inc_fmt = ERx('n');
	Opt.rtime_inc_fmt = ERx('n');
	Opt.rpageno_inc_fmt = ERx('n');
	tdcl = Ptr_dcl_top;
	while (tdcl != (DCL *)NULL)
	{
		if  (STbcompare(tdcl->dcl_name,0,
				RBF_NAM_DATE_FMT,0,TRUE) == 0)
		{
			rFpdf_pdeffmts(tdcl->dcl_datatype,&Opt.rdate_fmt[0],
				       &Opt.rdate_w_fmt);
			Opt.rdate_inc_fmt = ERx('y');
		}
		else if (STbcompare(tdcl->dcl_name,0,
				   RBF_NAM_TIME_FMT,0,TRUE) == 0)
		{
			rFpdf_pdeffmts(tdcl->dcl_datatype,&Opt.rtime_fmt[0],
				       &Opt.rtime_w_fmt);
			Opt.rtime_inc_fmt = ERx('y');
		}
		else if (STbcompare(tdcl->dcl_name,0,
				   RBF_NAM_PAGENO_FMT,0,TRUE) == 0)
		{
			rFpdf_pdeffmts(tdcl->dcl_datatype,&Opt.rpageno_fmt[0],
				       &Opt.rpageno_w_fmt);
			Opt.rpageno_inc_fmt = ERx('y');
		}
		tdcl = tdcl->dcl_below;
	}

	return;
}


static
VOID
rFpdf_pdeffmts(dcl_str,fmt_str,fmt_width)
	char	*dcl_str;
	char	*fmt_str;
	i4	*fmt_width;
{
	i4	i;
	i4	tk_delim;
	STATUS	status;
	PTR	area;
	u_i4	area_size;
	char	*save_Tokchar;
	char	*tfmt_str;
	FMT	*fmt;
	bool	found;
	char	opt_buf[(MAX_FMTSTR + 1)];


	/*
	** We assume a lot here - no name collision, the
	** user hasn't modified the report outside of RBF,
	** all strings are single quote delimited, etc.
	*/
	found = FALSE;
	i = STlength(dcl_str);
	while ((!found) && (--i >= 0))
	{
		switch(*dcl_str)
		{
		case (ERx('\'')):
			tk_delim = TK_SQUOTE;
			found = TRUE;
			break;

		default:
			CMnext(dcl_str);
			break;
		}
	}

	if  (!found)
	{
		/*
		** This is really an error, but we'll let the default
		** values ride.
		*/
		return;
	}

	/*
	** r_g_string() will double any backslashes as well as
	** backslash double quotes, so be sure that we call
	** r_strpslsh() to get rid of them!
	*/
	save_Tokchar = Tokchar;
	r_g_set(dcl_str);
	tfmt_str = r_g_string(TK_SQUOTE);
	r_strpslsh(tfmt_str);
	Tokchar = save_Tokchar;

	rF_unbstrcpy(&opt_buf[0],tfmt_str);
	MEfree(tfmt_str);

	/*
	** It's really an error also if the string is too long,
	** but once again we'll let the default values ride.
	*/
	if  (STlength(&opt_buf[0]) > MAX_FMTSTR)
	{
		return;
	}

	STcopy(&opt_buf[0],fmt_str);

	/*
	** Now determine the resultant of this width
	** (as well as its validity, which should always
	** be good ...)
	*/
	status = fmt_areasize(Adf_scb,fmt_str,&area_size);
	if  (status != OK)
	{
		FEafeerr(Adf_scb);
	}
	area = MEreqmem((u_i4)0,area_size,TRUE,(STATUS *)NULL);
	status = fmt_setfmt(Adf_scb,fmt_str,area,&fmt,(i4 *)NULL);
	if  (status != OK)
	{
		FEafeerr(Adf_scb);
	}
	*fmt_width = fmt->fmt_width;

	MEfree(area);

	return;
}

