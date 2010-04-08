/*
**	Copyright (c) 1989, 2008 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<rms.h>
# include	<descrip.h>
# include	<iledef.h>
# include	<lnmdef.h>
# include	<starlet.h>
# include	<st.h>
# include	<cv.h>
# include	<lo.h>
# include	<er.h>
# include	"lolocal.h"

static void syntax_parse();
static STATUS translate_logical();
static STATUS translate_name();

/* Logical name tables to search */
static $DESCRIPTOR(table_list, "LNM$FILE_DEV");

/*{
** Name:	LOsearchpath	- Create a LOCATION which includes a search path
**
** Description:
**
**	Our goal is to get the LOCATIONS's string into "device:[directory]"
**	format if possible, without losing the search list.   If it's already
**	in this form, we'll leave it alone.  Otherwise we'll try to do logical
**	name translation, and hope there's only one equivalence string.
**
** Inputs:
**	fnb		u_i4		nam$l_fnb result of SYS$PARSE
**	loc		LOCATION *	Location to parse
**
** Outputs:
**	loc		LOCATION *	Location to parse
**
**	Returns:
**		None.
**
**	Exceptions:
**		none.
**
** Side Effects:
**
** History:
**	10/17/89 (Mike S)	Initial version
**      16-jul-93 (ed)
**	    added gl.h
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes and
**	   forward function references
**	26-oct-2001 (kinte01)
**	   Clean up compiler warnings
**      16-sep-2002 (horda03) Bug 108702
**          The use of logical which references another logical
**          which is a search list, the location string is
**          corrupt;
**             e.g  DEFINE AAA II_SYSTEM:[TOOLS],II_SYSTEM:[INGRES]
**                  DEFINE BBB AAA
**
**                  BBB:THE.FILE ==> AAATHE.FILE
**
**          Need to retain the :, if the value of BBB doesn't contain
**          a : nor a ].
**	06-may-2003 (abbjo03)
**	    Correct 10/26/01 change. In translate_name, result_len should be an
**	    unsigned short (u_i2).
**	16-oct-2003 (abbjo03)
**	    Correct 9/16/02 change. In translate_name, comparisons to CBRK and
**	    CABRK assume they are single characters, but they are strings.
**	09-oct-2008 (stegr01/joea)
**	    Replace II_VMS_ITEM_LIST_3 by ILE3.
*/
void
LOsearchpath(
u_i4		fnb,
LOCATION 	*loc)
{
	char	*obracket;	/* Opening bracket of directory */
	char	*colon;		/* Colon preceding device */


	/* 
	** If we don't have brackets, try logical name translation.
	*/
	CVupper(loc->string);	/* Since logical names are case-sensitive */
	for ( ; ; )
	{
		if ((obracket = STindex(loc->string, OBRK, 0)) == NULL)
			obracket = STindex(loc->string, OABRK, 0);
		if (obracket != NULL) break;

		/* Exit the loop if logical name translation fails */
		if (translate_name(loc->string) != OK) break;
	}

	/*
	** If at this point, we have neither brackets nor a colon, but
	** the fnb tells us that device or directory was specified, append
	** a colon and treat the LOCATION as a device.
	*/
	colon = STindex(loc->string, COLON, 0);
	if ( (colon == NULL) && (obracket == NULL) && 
	     ((fnb & (NAM$M_EXP_DEV|NAM$M_EXP_DIR)) != 0) )
	{
		STcat(loc->string, COLON);
	}

	/* Do a purely syntactic parse */
	syntax_parse(loc);
	return;
}

/*
**	Do logical name translation of the device part of the string; 
**	i.e. either the part preceding the colon, or all of it, if 
**	there's no colon.
*/
static STATUS
translate_name(
char	*string)	/* String to translate (input and output) */
{
	char	*colon;		/* Colon preceding device */
	$DESCRIPTOR(lnm_desc, ERx("")); /*Logical name buffer */
	char	result[MAX_LOC+1];/* Logical name translation buffer */
	u_i2	result_len;
	ILE3	itemlist[4];	/* Logical name itemlist */
	i4	max_index;	/* greatest equivalence string index */
	u_i4	attribs;	/* logical name attributes */
	STATUS	status;
	char	buffer[MAX_LOC+1];
	char	last_ch;

	lnm_desc.dsc$a_pointer = string;
	colon = STindex(string, COLON, 0);
	if (colon == NULL)
		lnm_desc.dsc$w_length = STlength(string);
	else
		lnm_desc.dsc$w_length = colon - string;
		
	/* Set up itemlist */
	itemlist[0].ile3$w_code = LNM$_STRING;
	itemlist[0].ile3$w_length = MAX_LOC;
	itemlist[0].ile3$ps_bufaddr = result;
	itemlist[0].ile3$ps_retlen_addr = &result_len;

	itemlist[1].ile3$w_code = LNM$_MAX_INDEX;
	itemlist[1].ile3$w_length = sizeof(i4);
	itemlist[1].ile3$ps_bufaddr = (PTR)&max_index;
	itemlist[1].ile3$ps_retlen_addr = NULL;

	itemlist[2].ile3$w_code = LNM$_ATTRIBUTES;
	itemlist[2].ile3$w_length = sizeof(i4);
	itemlist[2].ile3$ps_bufaddr = (PTR)&attribs;
	itemlist[2].ile3$ps_retlen_addr = NULL;

	itemlist[3].ile3$w_code = itemlist[3].ile3$w_length = 0;
		
	status = sys$trnlnm(NULL, &table_list, &lnm_desc, NULL,
			    itemlist);
		
	/* 
	** We only proceed further if:
	** 1. The translation succeeded, and
	** 2. The logical wasn't a search path, and
	** 3. The logical wasn't a concealed device.
	*/
	if (((status & 1) == 0) || 
	    (max_index > 0) || 
	    ((attribs & LNM$M_CONCEALED) != 0))
	{
		return (FAIL);
	}

	/* Create the new string */
	result[result_len] = EOS;
	if (colon)
	{
		/* If the result is another logical
		** i.e. doesn't contain a ':' nor a
                ** closing bracket as the last character
		** then need to include the ':' from the
		** initial string.
		*/
		last_ch = result[result_len - 1];

		if ( (last_ch == ':') || (last_ch == *CBRK) ||
		     (last_ch == *CABRK) )
			colon++;
		STcopy(colon, buffer);
		STpolycat(2, result, buffer, string);
	}
	else
	{
		STcopy(result, string);
	}
	return (OK);
}

/* 
** Assign the LOCATION pointers via a syntactical parse of the LOCATION 
** string .
*/
static void
syntax_parse(
LOCATION 	*loc)
{
	char	*p;
	char	*dot;		/* Dot separating name from type */
	char	*semi;		/* Semicolon (or dot) preceding version */

	/* Look for node */
	loc->nodeptr = loc->string;
	p = STindex(loc->nodeptr, COLON, 0);
	if (p != NULL && p[1] == *COLON)
		loc->nodelen = p - loc->string + 2;
	else
		loc->nodelen = 0;

	/* Look for device */
	loc->devptr = loc->nodeptr + loc->nodelen;
	p = STindex(loc->devptr, COLON, 0);
	if (p != NULL)
		loc->devlen = p - loc->devptr + 1;
	else
		loc->devlen = 0;


	/* Look for directory */
	loc->dirptr = loc->devptr + loc->devlen;
	p = STindex(loc->dirptr, CBRK, 0);
	if (p == NULL) p = STindex(loc->dirptr, CABRK, 0);
	if (p != NULL)
	      	loc->dirlen = p - loc->dirptr + 1;
	else
		loc->dirlen = 0;

	/* Look for filename */
	loc->nameptr = loc->dirptr + loc->dirlen;
	dot = STindex(loc->nameptr, DOT, 0);
	semi = STindex(loc->nameptr, SEMI, 0);
	if (dot != NULL && semi == NULL)
		semi = STindex(dot+1, DOT, 0);

	if (dot != NULL)
		loc->namelen = dot - loc->nameptr;
	else if (semi != NULL)
		loc->namelen = semi - loc->nameptr;
	else
		loc->namelen = STlength(loc->nameptr);

	/* Look for type */
	loc->typeptr = loc->nameptr + loc->namelen;
	if (dot != NULL)	
	{
		if (semi != NULL)
			loc->typelen = semi - dot;
		else
			loc->typelen = STlength(loc->typeptr);
	}
	else
	{
		loc->typelen = 0;
	}				

	/* Look for version */
	loc->verptr = loc->typeptr + loc->typelen;
	if (semi != NULL)
		loc->verlen = STlength(loc->verptr);
	else
		loc->verlen = 0;
}
