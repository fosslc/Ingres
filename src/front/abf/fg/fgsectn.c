/*
**  Copyright (c) 1989, 2008 Ingres Corporation
*/

# include	<compat.h>
# include	<st.h>
# include	<cm.h>
# include	<lo.h>
# include	<si.h>
# include	<er.h>
# include	<iicommon.h>
# include	<oslconf.h>

/**
** Name:	fgsectn.c - Output Begin and End section commands
**
** Description:
**	This file defines:
**
**	IIFGbsBeginSection	Output a Begin Section comment
**	IIFGesEndSection	Output an End Section comment
**
** History:
**	9/19/89 (Mike S) Initial Version
**
**	12-dec-1990 (blaise)
**	    Added encode_section(), to translate "\", "/" and " " in
**	    menuitems. Bug numbers 34458, 34869.
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**	25-Aug-2009 (kschendel) 121804
**	    Need iicommon.h to satisfy gcc 4.3.
**/

/* # define's */
/* GLOBALDEF's */
/* extern's */
FUNC_EXTERN VOID IIFGwrj_writeRightJustified();

/* static's */
static const char template[] = 	ERx("%s/*%c %s %s */%s");
static const char _begin[] = 	ERx("BEGIN");
static const char _end[] = 		ERx("END");
static const char _empty[] = 	ERx("");
static const char _semicolon[] = 	ERx(";");

static VOID encode_section();
static VOID output_section();

/*{
** Name:	IIFGbsBeginSection      Output a Begin Section comment
** Name:	IIFGbeEndSection      	Output an End Section comment
**
** Description:
**	Output the Begin (or End) Section header for the specifed section.
**
** Inputs:
**	file		FILE *  file to write to
**	major_sect	char *	major part of the section name
**	minor_sect	char *  minor part of the section name (may be NULL)
**	delimit		bool	whether to add 4GL delimiters to this section
**
** Outputs:
**	none
**
**	Returns:
**		none
**
**	Exceptions:
**		none
**
** Side Effects:
**
** History:
**	9/19/89 (Mike S)	Initial version
*/
VOID
IIFGbsBeginSection(file, major_sect, minor_sect, delimit) 
FILE *file;
char *major_sect;
char *minor_sect;
bool delimit;
{
	output_section(file, _begin, major_sect, minor_sect, _empty, _empty);
}

VOID
IIFGesEndSection(file, major_sect, minor_sect, delimit) 
FILE *file;
char *major_sect;
char *minor_sect;
bool delimit;
{
	output_section(file, _end, major_sect, minor_sect, 
		       (delimit ? _semicolon : _empty), _empty);
}

/*
**	Format the section delimiter, and output it
*/
static VOID
output_section(file, part, major_sect, minor_sect, front_delim, back_delim)
FILE	*file;	/* Output file */
char	*part;	/* BEGIN or END */
char	*major_sect;	/* major part of section name */
char	*minor_sect;	/* Minor part of name; may be NULL */
char	*front_delim;	/* Delimiter preceding comment */
char	*back_delim;	/* Delimiter follwoing comment */
{
	char	minor_encoded[65];
	char	buffer[256];	/* Big buffer -- whole line */
	char	section[100];	/* Small buffer -- section name only */

	/* Format section name */
	if (minor_sect != NULL)
	{
		encode_section(minor_sect, minor_encoded);
		STprintf(section, ERx("%s\\%s"), major_sect, minor_encoded);
	}
	else
		STcopy(major_sect, section);

	/* Format and output whole section delimiter */
	STprintf(buffer, template, front_delim, OSCOMCHAR, part, 
		 section, back_delim);
	IIFGwrj_writeRightJustified(buffer, file);
}

/*
** Translate the section name as follows (bugs 34458, 34869):
**	\	translated to \\
**	space	translated to \_
**	/	translated to \/
** 
*/
static VOID
encode_section(sect, encoded_sect)
char	*sect;		/* section name to be encoded */
char	*encoded_sect;	/* encoded section name */
{
	char	*backslash = ERx("\\");
	char	*underscore = ERx("_");
	char	*space = ERx(" ");
	char	*slash = ERx("/");

	while (*sect != EOS)
	{
		if (CMcmpcase(sect, backslash) == 0)
		{
			/* "\". Translate it to "\\" */
			CMcpychar(sect, encoded_sect);
			CMnext(encoded_sect);
			CMcpyinc(sect, encoded_sect);
		}
		else if (CMcmpcase(sect, space) == 0)
		{
			/* " ". Translate it to "\_" */
			CMcpychar(backslash, encoded_sect);
			CMnext(encoded_sect);
			CMcpychar(underscore, encoded_sect);
			CMnext(encoded_sect);
			CMnext(sect);
		}
		else if (CMcmpcase(sect, slash) == 0)
		{
			/* "/". Translate to "\/" */
			CMcpychar(backslash, encoded_sect);
			CMnext(encoded_sect);
			CMcpyinc(sect, encoded_sect);
		}
		else
		{
			/* copy character from sect to encoded_sect */
			CMcpyinc(sect, encoded_sect);
		}
	}
	*encoded_sect = EOS;
}
