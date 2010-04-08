/*
**Copyright (c) 2004 Ingres Corporation
*/
# include	<te.h>
# include	<lo.h>
# include	<tc.h>
# include	<st.h>

/*
** TEtest is used to test forms programs.
** in and out are the names of files to open for reading and
** writing.
**
**	history:
**		2/85 (ac) -- made stdin nonbuffered.
**		8/85 (joe) -- Now uses IIfe_tpin for input.
**
**		Revision 30.3  86/02/06  18:27:18  roger
**		We can only reassign stdin to the keystroke file
**		when testing with AT&T curses, which unconditionally
**		reads therefrom.  This means that calls to TEswitch
**		will fail, and in particular, that the Keystroke
**		File Editor cannot be used with AT&T INGRES.
**
**		24-may-89 (mca)
**			Added support for opening TCFILEs, as needed by SEP.
**			While the "out_type" formal parameter was added to
**			the spec, TEtest does not now support opening TCFILEs
**			for output, since it's never called upon to do so
**			and the TE output routines don't support it
**			anyway.
**		
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

int    IIte_use_tc = 0;
TCFILE *IItc_in    = NULL;

TEtest(in, in_type, out, out_type)
char	*in;
i4	in_type;
char	*out;
i4	out_type;
{
	char in_name[MAX_LOC];
	LOCATION in_loc;
	FILE	*stream;

	if (in && *in != '\0' && in_type != TE_NO_FILE)
	{
		switch (in_type)
		{
			case TE_SI_FILE:
				if ((IIte_fpin =
# ifdef ATTCURSES
				     freopen(in, "r", stdin)
# else
				     fopen(in, "r")
# endif
				    ) == NULL)
					return FAIL;
				TEunbuf(IIte_fpin);
				break;
			case TE_TC_FILE:
				STcopy(in, in_name);
				LOfroms(FILENAME, in_name, &in_loc);

				if (TCopen(&in_loc, "r", &IItc_in) != OK)
					return(FAIL);
				IIte_use_tc = 1;
				break;
			default:
				return FAIL;
		}
	}

	if (out && *out != '\0' && out_type != TE_NO_FILE)
	{
		if (out_type != TE_SI_FILE)
			return FAIL;
		stream = freopen(out, "w", stdout);
		if (stream == NULL)
			return FAIL;
	}
	return OK;
}  /* TEtest */
