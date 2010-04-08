/*
** Copyright (c) 1990, 2005 Ingres Corporation
**
*/

#include	<compat.h>
#include	<lo.h>
#include	<st.h>
#include	<si.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ooclass.h>
#include	<abclass.h>
#include	<abfcnsts.h>

/**
** Name:	abfcngen.c -	ABF Generate Language Dependent Constants Image.
**
** Description:
**	Contains the routine used to generate the language-dependent constants
**	image files for an application.  Defines:
**
** History:
**	Revision 6.3/03/00  90/07  wong
**	Initial revision.
**
**	Revision 6.5
**	26-oct-92 (davel)
**		We still need this beast for backward compatibilty; it will be
**		phasedout entirely in the release after 6.5.  Modify 
**		iiabGnConstImg() to look for constant values in value[language]
**		instead of value[language-1].
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	07-sep-2005 (abbjo03)
**	    Replace xxx_FILE constants by SI_xxx.
**    25-Oct-2005 (hanje04)
**        Add prototype for write_open() to prevent compiler
**        errors with GCC 4.0 due to conflict with implicit declaration.
*/

/* local prototypes */
static STATUS
write_open (
	i4	language,
	char	*appl,
	char	*datestamp,
	FILE	**imgfile);

/*{
** Name:	iiabGnConstImg() -	Generate Language Dependent Constants
**						Image Files.
** Description:
**	Generates image files for the language-dependent constants for each
**	language variant defined for the application.  *** Remains for
**	backward compatibilty with pre-6.5 releases!  This will become obsolete
**	in the release that follows 6.5.
**
** Inputs:
**	appl		{APPL *}  The application.
**	default_language{nat}  The ER code for the default language for this
**				applicaition.
**	datestamp	{char *} The datestamp for the files
** Returns:
**	{STATUS}  OK if the files were written successfully.
**
** History:
**	07/90 (jhw) -- Written.
**	26-oct-92 (davel)
**		consider languages 1:ER_MAXLANGUAGES; the corresponding value
**		is now saved in value[language] (previously it was saved in 
**		value[language-1]).
*/
STATUS
iiabGnConstImg ( appl, default_language, datestamp )
APPL	*appl;
i4	default_language;
char	*datestamp;
{
	i4	language;

	/* For all ER language codes . . . */
	for ( language = ER_MAXLANGUAGES; language > 0; language--)
	{
		register APPL_COMP	*comps;
		FILE			*img = NULL;

		if ( language == default_language )
			continue;	/* skip the default language */

		for ( comps = (APPL_COMP *)appl->comps ;
				comps != NULL ; comps = comps->_next )
		{
			register CONSTANT	*xconst;
			u_i2			length;
			i4			cnt;
			STATUS			rval;

			if ( comps->class != OC_CONST )
			{ /* skip all but constants . . . */
				continue;
			}

			xconst = (CONSTANT *)comps;
			switch ( xconst->data_type.db_datatype )
			{
			  default:
				continue;	/* skip following ... */
			  case DB_DYC_TYPE:
			  case DB_VCH_TYPE:
			  case DB_TXT_TYPE:
			  case DB_CHR_TYPE:
			  case DB_CHA_TYPE:
				break; /* fall through to after switch ... */
			} /* end switch */
			/* assert: constant is a character type */
			if ( xconst->value[language] == NULL
					|| *xconst->value[language] == EOS )
			{ /* no constants for this language . . . */
				break;
			}
			if ( img == NULL )
			{
				rval = write_open(language, appl->name, 
						  datestamp, &img);
				if ( rval != OK )
					return rval;
			}

			/* write to image file */
			length = (u_i2)STlength(xconst->value[language]);
			if ( (rval = SIwrite((i4)sizeof(u_i2), 
					     (char *)&length, 
					     &cnt, img))
					!= OK
				|| (rval = SIwrite((i4)(length + 1),
						xconst->value[language],
						&cnt, img)) != OK )
			{
				SIclose(img);
				return rval;
			}
		} /* end constant for */

		if ( img != NULL )
			SIclose(img);
	} /* end language for */

	return OK;
}

/*
** Name:	write_open() -	Open Language Constants Image File.
**
** Description:
**	Open an constants image file for the given language type.
**
** Inputs:
**	language	{nat}  The ER code for the language.
**	appl		{char *}  The application name.
**	datestamp	{char *}  The datestamp for the image
**
** Outputs:
**	imgfile		{FILE **}  The file pointer for the open file.
**
** Returns:
**	{STATUS}  OK if the file was opened successfully.
**
** Side Effects:
**	Opens the file for writing, which creates it if it doesn't exist.
**
** History:
**	07/90 (jhw) -- Written.
**	11/90 (Mike S) -- Add header to file:
**			Line 1: Datestamp
*/
static STATUS
write_open ( language, appl, datestamp, imgfile )
i4	language;
char	*appl;
char	*datestamp;
FILE	**imgfile;
{
	STATUS		rval;
	char		lname[ER_MAX_LANGSTR+1];
	LOCATION	img;
	char		ibuf[MAX_LOC+1];
	i4		cnt;

	if ( (rval = ERlangstr(language, lname)) != OK )
		return rval;

	/* open for writing . . . */

	ibuf[0] = EOS;
	_VOID_ LOfroms(PATH, ibuf, &img);
	if ( (rval = IIABsdirSet(lname, &img)) == OK
			|| (rval = IIABcdirCreate(lname, &img)) == OK )
	{ /* language directory exists or was created */
		char	filename[LO_NM_LEN+1];

		_VOID_ STprintf(filename, ABCONFILE, appl);
		if ( (rval = LOfstfile(filename, &img)) == OK )
		{ /* create file for writing */
			rval = SIfopen(&img, "w", SI_VAR, 0, imgfile);
		}
	}
	if (rval == OK)
	{
		_VOID_ LOpurge(&img, 1);
		rval = SIwrite((i4)STlength(datestamp), datestamp, 
				&cnt, *imgfile);
	}
	if (rval == OK)
		rval = SIwrite((i4)1, ERx("\n"), &cnt, *imgfile);

	return rval;
}
