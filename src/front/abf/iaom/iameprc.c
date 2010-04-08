/*
**	Copyright (c) 1986, 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
#include "iamstd.h"
#include <iltypes.h>
#include <fdesc.h>
#include <ade.h>
#include <frmalign.h>
#include <ifrmobj.h>
#include "iamfrm.h"
#include "iamtbl.h"

/**
** Name:	iameprc.c - write operation routines
**
** Description:
**	Encoding procedures pointed to by the Enc_tab drive table.
*/

static STATUS bs_puts();

/*{
** Name:	es_wr - encoded string write
**
** Description:
**	Writes an array of encoded strings to the DB.  Prefix / Suffix
**	strings may be "", indicating that they aren't to be used.
**
** Inputs:
**	str	array of strings
**	len	number of strings
**	prefix	prefix string
**	suffix	suffix string
**	bs	backslash treatment flag
**
** Outputs:
**
**	return:
**		OK		success
**		FAIL		write failure
**
** History:
**	8/86 (rlm)	written
**	Revision 6.5
**	19-jun-92 (davel)
**		changed prefix and suffix to be strings instead of single
**		chars, as part of adding decimal support.  Also cleaned up
**		all of the improper db_printf calls - db_printf only accepts
**		i4 arguments.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	13-May-2005 (kodse01)
**	    replace %ld with %d for old nat and long nat variables.
*/

STATUS
es_wr(str,len,prefix,suffix,bs)
char **str;
i4  len;
char *prefix;
char *suffix;
bool bs;
{
	/*
	** db_puts called with an empty or NULL string does nothing.  Of course,
	** we're causing a little extra work, which some extra logic could
	** save us.  DO NOT use db_printf with %s - it does not support %s.
	*/
	for ( ; len > 0; str++,len--)
	{
		if (db_puts(prefix) != OK)
			return (FAIL);
		if (bs)
		{
			if (bs_puts(*str) != OK)
				return (FAIL);
		}
		else
		{
			if (db_puts(*str) != OK)
				return (FAIL);
		}
		if (db_puts(suffix) != OK)
			return (FAIL);
	}
	return (OK);
}

/*
** put out string with backslash treatment.  Must preserve string.
** backslashes themselves, and doublequotes are backslashed.
** bs_index saves making 2 STindex calls.  Again, the fact that
** db_puts with an empty string does nothing simplifies the logic.
*/
static char *
bs_index(s)
char *s;
{
	if (s == NULL)
		return (NULL);

	for (; *s != '\0'; ++s)
		if (*s == '"' || *s == '\\')
			return(s);
	return (NULL);
}

static STATUS
bs_puts(s)
char *s;
{
	char *out, hold[3];

	hold[0] = '\\';
	hold[2] = '\0';
	for (out = bs_index(s); out != NULL; out = bs_index(s = out+1))
	{
		hold[1] = *out;
		*out = '\0';
		if (db_puts(s) != OK)
			return (FAIL);
		if (db_puts(hold) != OK)
			return (FAIL);
		*out = hold[1];
	}
	if (db_puts(s) != OK)
		return (FAIL);
	return (OK);
}

/*
** ind_wr - write stack indirection section
*/
STATUS
ind_wr (dbd,len)
IL_DB_VDESC *dbd;
i4  len;
{
	for ( ; len > 0; len--,dbd++)
	{
		/*
		** db_data is actually an integer offset
		*/
		if (db_printf(ERx("i%dI%di%dI%d"),
					(i4)dbd->db_datatype,
					(i4)dbd->db_length,
					(i4)dbd->db_prec,
					(i4)dbd->db_offset) != OK)
			return (FAIL);
	}
	return (OK);
}

/*
** sym_wr - write symbol table
*/
STATUS
sym_wr(sym,len)
FDESCV2 *sym;
i4  len;
{
	for ( ; len > 0; len--,sym++)
	{
		char *fname,*tname;
		char *tyname;

		/*
		** Write the two "name" strings.
		** (Write an empty string if a name is missing).
		** Note that these are INGRES names and thus won't
		** contain a quote or backslash, at least not until we
		** support delimitted identifers in the symbol table (sigh).
		*/
		if (db_puts(ERx("\"")) != OK)
			return (FAIL);
		if ((fname = sym->fdv2_name) != NULL)
		{
			if (db_puts(fname) != OK)
				return (FAIL);
		}
		if (db_puts(ERx("\"\"")) != OK)
			return (FAIL);

		if ((tname = sym->fdv2_tblname) != NULL)
		{
			if (db_puts(tname) != OK)
				return (FAIL);
		}
		if (db_puts(ERx("\"")) != OK)
			return (FAIL);

		/*
		** Write the four numbers.
		*/
		if (db_printf(ERx("I%di%di%di%d"), 
					(i4)sym->fdv2_dbdvoff,
					(i4)sym->fdv2_visible, 
					(i4)sym->fdv2_order,
					(i4)sym->fdv2_flags
				 ) != OK)
		{
			return (FAIL);
		}
		if (db_puts(ERx("\"")) != OK)
			return (FAIL);
		if ((tyname = sym->fdv2_typename) != NULL)
		{
			if (db_puts(tyname) != OK)
				return (FAIL);
		}
		if (db_puts(ERx("\"")) != OK)
			return (FAIL);

	}
	return (OK);
}

/*
** alg_wr - write alignment array of i4's
*/
STATUS
alg_wr(a,len)
i4 *a;
i4  len;
{
	for ( ; len > 0; len--,a++)
		if (db_printf(ERx("I%d"),(i4) *a) != OK)
			return (FAIL);
	return (OK);
}

/*
** il_wr - write IL
*/
STATUS
il_wr(il,len)
IL *il;
i4  len;
{
	for ( ; len > 0; len--,il++)
		if (db_printf(ERx("i%d"),(i4) *il) != OK)
			return (FAIL);
	return (OK);
}

/*
** hex_wr - write hexadecimal constants.  The array is
** an array of pointers to DB_TEXT_STRINGs db-data-value like structures.
*/
STATUS
hex_wr(hex,len)
DB_TEXT_STRING **hex;
i4  len;
{
	register DB_TEXT_STRING *dtxt;
	i4 slen;
	u_char *bp;

	for (; len > 0; len--,hex++)
	{
		dtxt = (DB_TEXT_STRING *) *hex;
		slen = dtxt->db_t_count;

		if (db_printf(ERx("X%d'"), (i4)slen) != OK)
			return (FAIL);
		for (bp = dtxt->db_t_text; slen > 0; slen--,bp++)
		{
			if (db_printf(ERx("%02lx"),(i4) *bp) != OK)
				return (FAIL);
		}
		if (db_puts(ERx("'")) != OK)
			return (FAIL);
	}

	return (OK);
}
