/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** NO_OPTIM = ris_u64 i64_aix
*/

#include    <compat.h>
#include    <gl.h>
#include    <st.h>
#include    <ut.h>
#include    <systypes.h>
#include    <stdarg.h>

/*
**      Defines the maximum number of substitution strings
**	that can be passed to II_subst(). 
*/

#define ARRAYSIZE 100


/*
** Name: II_subst()	- perform string substitutions
**
** Description:
**      II_subst() copies characters from input_str to output_str.
**      If the magic character is encountered in the input string,
**      the next character selected from the input string is
**      used for a search through the values of 'chr' in the
**	zero-terminated list of 'chr'/'sub_str' pairs passed to
**      II_subst() on the command line (varargs is used).  If
**      a match is found, then the corresponding value of
**      sub_str is substituted in the output string instead of
**      the instance of <magic_char><chr> found in the input
**      string. If the magic char is found, but the following
**      character does not have a match in the list of
**      'chr'/'sub_str' pairs, then the two characters (the magic
**      char and the following char) are copied to the output string
**      unchanged.  If two instances of the magic char occur together,
**      then a single copy of the magic char is written to the output
**      string.
**      
** Inputs:
**      magic_char -	The magic character.  When this is found
**			in the input string, a string substitution
**			is performed based on the next character in
**			the input string.
**      input_str -	The input string.
**      size_ptr -	Points to the maximum size of the output string.
**	chr -		Key character identifying a specific substitution
**			string (sub_str).
**	sub_str -	A substitution string.
**
** Outputs:
**      output_str -	The output string.
**      size_ptr -	Upon returning, the area pointed to by size_ptr
**			is set to the number of characters written to the
**			output string.
**
**	Returns:
**	    none
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      02-dec-87 (russ)
**          Function initially created.
**	21-mar-91 (seng)
**	    Add <systypes.h> to clear <types.h> on AIX 3.1.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**      16-jul-99 (hweho01)
**          Turned off optimization for AIX 64-bit (ris-u64) to fix
**          run-time problem. Symptom :  E_AB0020 link failed during
**          front end sep test (abf13.sep).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	16-aug-2001 (toumi01)
**	    speculative i64_aix NO_OPTIM change for beta xlc_r - FIXME !!!
**      14-Jan-2004 (hanje04)
**          Change functions using old, single argument va_start (in
**          varargs.h), to use newer (ANSI) 2 argument version (in stdargs.h).
**          This is because the older version is no longer implemented in
**          version 3.3.1 of GCC.
**	05-Feb-2004 (hanje04)
**	    Remove first 4 calls to va_arg from II_subst as these arguments
**	    are now declared explicitly in the function.
**	29-Nov-2010 (frima01) SIR 124685
**	    Added include of ut.h for prototype include from utcl.h.
*/

/* VARARGS4 */
void
II_subst(i4 magic_char, 
	char *input_str, 
	i4 *size_ptr,
	char *output_str, ...)
{
	i4	size;
	va_list ap;

	struct {
		char chr;
		char *str;
	} list[ARRAYSIZE];

	register char *is;
	register char *os;
	register char *ts;

	register i4  i,n;


	va_start(ap, output_str);

	size = *size_ptr - 1;

	/*
	** Build up list of key chars and their corresponding
	** substitution strings using varargs.
	*/

	i = 0;
	while ((list[i].chr = va_arg(ap, i4)) != 0)
	{
		if ((list[i].str = va_arg(ap, char *)) == (char *) 0)
		{
			list[i].chr = 0;
			break;
		}
		if (++i == ARRAYSIZE)
			break;
	}

	/*
	** Make sure that ARRAYSIZE was not exceded.
	*/

	if (i == ARRAYSIZE && va_arg(ap, i4) != 0)
	{
		STcopy("II_subst: array size exceded",output_str);
		*size_ptr = 0;
		return;
	}

	/*
	** Copy input string to output string, performing
	** string substitutions where necessary.
	*/

	n = 0;
	is = input_str;
	os = output_str;
	while (*is != EOS)
	{
		if (*is == magic_char)
		{
			is++;
			/* search list of key chars */
			for (i = 0; list[i].chr != 0; i++)
				if (list[i].chr == *is)
					break;
			if (list[i].chr == 0)	/* no match */
			{
				if (*is == magic_char)
					is++;
				if (n == size)
					break;
				*os++ = magic_char;
				n++;
			}
			else			/* match */
			{
				ts = list[i].str;
				while (*ts != EOS) {
					if (n == size)
						break;
					*os++ = *ts++;
					n++;
				}
				is++;
			}
		}
		else
		{
			if (n == size)
				break;
			*os++ = *is++;
			n++;
		}
	} /* while */

	/* null terminate output string */
	*os = EOS;

	/* set size_ptr to number of chars written to output string */
	*size_ptr = n;

	va_end(ap);
}
