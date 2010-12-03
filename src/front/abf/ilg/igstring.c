/*
**	Copyright (c) 1984, 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<ex.h>
#include	<st.h>
#include	<me.h>
#ifndef MEalloc
#define MEalloc(size, status) MEreqmem(0, (u_i4)(size), FALSE, (status))
#endif
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<adf.h>		/* unused */
#include	<fe.h>
#include	<afe.h>
#include	<lt.h>

/**
** Name:	igstring.c -	OSL Interpreted Frame Object Generator
**					String Storage Module.
** Description:
**	Contains the routine that manages the string table for OSL.  All strings
**	are entered into the string table.  Those that have the same value point
**	to the same entry in the string table.  Thus, routines within OSL that
**	compare strings need only compare the string addresses.  (This reduces
**	the number of calls to 'STcompare()'.)  Defines:
**
**	iiIG_string()	return reference to string.
**	iiIG_vstr()	return reference to variable-length string.
**
** History:
**	Revision 6.2  89/06  wong
**	Corrected allocation support for zero-length variable length strings.
**	JupBug #6786.
**
**	Revision 6.0  87/05  wong
**	Added support for variable-length strings.
**
**	Revision 5.1  86/12  wong
**	Modified to allocate large strings seperately and correctly.
**
**	Revision 2.0  83/11  joe
**	Initial revision.
**
**
**	31-aug-1993 (mgw)
**		Fix some casting problems for MEcmp et. al. for prototyping.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	2-Dec-2010 (kschendel) SIR 124685
**	    Fix a couple erroneous u_i2 mecopy casts while fixing prototypes.
*/

typedef struct _strtab {
    i4		st_kind;	/* Whether text of char * */
# define	STCHAR		0
# define	STTEXT		1
    union {
	char		*st_char;
	DB_TEXT_STRING	*st_text;
    }st_ptr;
} STRTAB;

# define	OSLSTR		1

static LIST	**strhash();
static char	*stralloc();

/*{
** Name:	iiIG_string() -	Return Reference to String.
**
** Description:
**	Searchs string table for the string value, entering the string if it's
**	not found.  Returns reference to the string in the table.
**
** Input:
**	str	{char *}  The string to enter into the string table.
**
** Returns:
**	{char *}	A reference to the string.
**
** History:
**	Written 11/16/83 (jrc)
**	12/86 (jhw)  Modified to allocate large strings seperately and
**			correctly.
**	05/87 (jhw)  Abstracted hash and allocation to support variable-length
**			strings.
*/

char *
iiIG_string (str)
register char	*str;
{
    register LIST	*lt;
    LIST		**ent;
    i4			size = 0;
    STRTAB		*st;
    STRTAB		newstring;

    ent = strhash(str, 0, &size);

    for (lt = *ent ; lt != NULL ; lt = lt->lt_next)
    {
	st = (STRTAB *) lt->lt_rec;
	if (st->st_kind == STCHAR
	    && (st->st_ptr.st_char == str
		|| STequal(st->st_ptr.st_char, str)))
	{
	    return (char *)st->st_ptr.st_char;
	}
    }

    /* String was not in table so it must be added */

    newstring.st_kind = STCHAR;
    newstring.st_ptr.st_char = str;
    return stralloc(&newstring, size, sizeof(*str), ent);
}

/*{
** Name:    iiIG_vstr() -	Return Reference to Variable-Length String.
**
** Description:
**	Searchs string table for the string value, entering the string if it's
**	not found.  Returns reference to the string in the table.
**
** Input:
**	str	{char *}  The string to enter into the string table.
**	len	{nat}  String length.
**
** Returns:
**	{char *}	A reference to the string.
**
** History:
**	05/87 (jhw)  Written.
*/

char *
iiIG_vstr (str, len)
register char	*str;
i4		len;
{
    register LIST			*lt;
    LIST				**ent;
    AFE_DCL_TXT_MACRO(DB_GW4_MAXSTRING)	tmp;
    DB_TEXT_STRING			*vstr = (DB_TEXT_STRING *)&tmp;
    STRTAB		*st;
    STRTAB		newstring;

    ent = strhash(str, len, (i4 *)NULL);

    for (lt = *ent ; lt != NULL ; lt = lt->lt_next)
    {
	st = (STRTAB *) lt->lt_rec;
	if (st->st_kind == STTEXT
	    && st->st_ptr.st_text->db_t_count == len
	    && MEcmp((PTR)str, (PTR)st->st_ptr.st_text->db_t_text, len)
	             == 0
	    )
	{
	    return (char *)st->st_ptr.st_text;
	}
    }

    /* String was not in table so it must be added */

    vstr->db_t_count = len;
    MEcopy((PTR) str, len, (PTR) vstr->db_t_text);

    newstring.st_kind = STTEXT;
    newstring.st_ptr.st_text = vstr;
    return stralloc(&newstring, len + sizeof(vstr->db_t_count),
						sizeof(vstr->db_t_count), ent);
}

/*
** Name:    stralloc() -	Allocate String in Block and Hash Table.
**
** Description:
**	Allocates a hash table entry for a string including space in the
**	shared string block and fills it in.
**
** Input:
**	str	{STRTAB *}  The string.
**	size	{nat}  The space needed for the string (including EOS.)
**	align	{nat}  Alignment padding for string.
**	ent	{LIST **}  Hash table entry for string.
**
** Returns:
**	{char *}  Reference to string in block.
**
** History:
**	12/89 (jhw) -- Corrected test for available size.  JupBug #9110.
**      01-dec-93 (smc)
**	    Bug #58882
**          Commented lint pointer truncation warning.
**	1-Dec-2010 (kschendel)
**	    Fix the warning.
*/

static char	*strblock = NULL;
static i4	strleft = 0;

static char *
stralloc (str, size, align, ent)
STRTAB	*str;
i4	size;
i4	align;
LIST	**ent;
{
    register LIST   *lt;
    register i4     pad;
    char	    *strval;
    PTR		    value;
    STATUS	    status;
    STRTAB	    *st;

    value = ( str->st_kind == STCHAR )
		? (PTR) str->st_ptr.st_char : (PTR) str->st_ptr.st_text;
    /* Calculate pad for string length (u_i2) */
    pad = ME_ALIGN_MACRO(strblock, align) - strblock;
    /*
    ** See if enough space is left in the current block for
    ** the string plus any pad.  If not, allocate more.
    */
    if ( strleft <= size + pad )
    {
#define		STRBLOCK 1024

	/* Special case:  Large strings are allocated separately to avoid
	** wasting space in the string buffer.  (The definition of what is
	** large is somewhat ad. hoc. and depends on the amount of space left.)
	*/
	if (size > STRBLOCK || (size > STRBLOCK/16 && strleft > 16))
	{
	    register char    *strblock;

	    if ((strblock = (char *)MEalloc(size, &status)) == NULL)
		EXsignal(EXFEMEM, 1, ERx("iiIG_string()"));
	    MEcopy(value, size, (PTR) strblock);
	    return strblock;
	}

	if ((strblock = (char *)MEalloc(STRBLOCK, &status)) == NULL)
	    EXsignal(EXFEMEM, 1, ERx("iiIG_string()"));
	strleft = STRBLOCK;
    }
    pad = ME_ALIGN_MACRO(strblock, align) - strblock;
    strval = strblock + pad;
    MEcopy(value, size, (PTR) strval);
    strblock += size + pad;
    strleft -= size + pad;
    /*
    ** Enter string in buffer into the hash table.
    */
    if ((st = (STRTAB *)MEalloc(sizeof(*st), &status)) == NULL)
	EXsignal(EXFEMEM, 1, ERx("iiIG_string()"));
    MEcopy((PTR) str, sizeof(*st), (PTR) st);
    if (str->st_kind == STCHAR)
	st->st_ptr.st_char = strval;
    else
	st->st_ptr.st_text = (DB_TEXT_STRING *) strval;
    lt = LTalloc(OSLSTR);
    lt->lt_rec = (PTR)st;
    lt->lt_next = *ent;
    *ent = lt;

    return strval;
}

/*
** Name:    strhash() -	Return Hash Table Entry for String.
**
** Description:
**	Hashs string and returns entry in hash table.
**
** Input:
**	str	{char *}  The string.
**	size	{nat}  Size of variable-length string.
**	length	{nat *}  Length of string for fixed length strings, if not NULL.
**
** Output:
**	length	{nat *}  Size of string including EOS, if not variable-length.
**
** Returns:
**	{LIST **}  Hash table entry for string.
**
** History:
**	05/87 (jhw) -- Abstracted from 'osstring()' and modified
**			to support variable-length strings.
**	06/89 (jhw) -- Added separated 'size' and 'length' to support allocation
**		of zero-length variable-length strings.
*/

#define		HASHSIZE 71

static LIST	*strtab[HASHSIZE] = {NULL};

static LIST **
strhash (str, size, length )
char	*str;
i4	size;
i4	*length;
{
    register char	*strp;
    register i4	hashvalue = 0;

    if ( length == NULL )
    {
	register i4	len = size;

	for (strp = str ; --len >= 0 ; ++strp)
	    hashvalue += (hashvalue << 2) + *strp;
    }
    else
    {
	for (strp = str ; *strp != EOS ; ++strp)
	    hashvalue += (hashvalue << 2) + *strp;
    }

    if ((hashvalue %= HASHSIZE) < 0)
	hashvalue = 0;

    if ( length != NULL )
    	*length = (strp - str) + 1;

    return &(strtab[hashvalue]);
}
