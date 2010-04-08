/*
** Copyright (c) 1987, 2008 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<cv.h>		/* 6-x_PC_80x86 */
#include	<si.h>
#include	<nm.h>
#include	<lo.h>
#include	<st.h>
#include 	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ug.h>
#include	<cf.h>
#include	"ercf.h"

/**
** Name:	cfcnvt60.c -	Convert formfile from 5.0 to 6.0 format.
**
** Description:
**
**	cf_convto60()	convert intermediate form file to 6.0 format.
**
** History:
**	Revision 6.0  87/04/23  rdesmond
**	Initial revision.
**	05/30/87 (dkh) - Fixed to convert a table field properly.
**	07/13/88 (dkh) - Changed code to avoid bad code generated
**			 by VMS C compiler.
**	07/15/88 (dkh) - Changed _50hdrtype() to pass "buf" instead
**			 of "token" to STequal().
**	09/23/88 (dkh) - Fixed venus bug 3320.
**	01/25/90 (dkh) - Changes for new LO on VMS.
**	10-sep-96 (kch)
**	    In the function cf_convto60(), we now return an error if
**	    field_in[0] is NULL. This prevents an ACCVIO in STcopy().
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**      06-feb-2009 (stial01)
**          Define copyform record buffer with size COPYFORM_MAX_REC
**/

#define OLD_DB_CHAR_MAX		255


/*{
** Name:	cf_convto60 - convert file from 5.0 to 6.0 format.
**
** Description:
**
** Input params:
**	file_in		input file name.
**	file_out	output file name.
**
** Output params:
**	none.
**
** Returns:
**	OK		successful
**	FAIL		not successful
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	23-Apr-87 (rdesmond) written.
**	12-May-87 (rdesmond) fixed conversion of flformat.
**	05/30/87 (dkh) - Fixed to convert a table field properly.
**	10-sep-96 (kch)
**	    Return an error (E_CF0025) if field_in[0] is NULL (field_in[0]
**	    will be NULL if cf_gettok() cannot find a valid token, for example,
**	    if the .frm file contains spaces instead of tabs). This prevents
**	    an ACCVIO when STcopy() is called with field_in[0].
*/
static i4	_50hdrtype();
static STATUS	_50write();

STATUS
cf_convto60 ( file_in, file_out )
char	*file_in;
char	*file_out;
{
	i4			cur_type;
	char		curr_form[FE_MAXNAME+1];

	FILE		*fd_in;
	FILE		*fd_out;
	char		buff_in[COPYFORM_MAX_REC];
	char		*field_out[32];
	LOCATION	loc;
	char		file_locbuf[MAX_LOC + 1];
	i4		jfirst;

	/* start of routine */

	/* Open source file */
	STcopy(file_in, file_locbuf);
	LOfroms(PATH & FILENAME, file_locbuf, &loc);
	if ( SIopen(&loc, ERx("r"), &fd_in) != OK )
	{
	    IIUGerr(E_CF0000_CANNOT_OPEN_FILE, 0, 1, file_in);
	    return FAIL;
	}

	/* Open destination file */
	STcopy(file_out, file_locbuf);
	LOfroms(PATH & FILENAME, file_locbuf, &loc);
	if (SIopen(&loc, ERx("w"), &fd_out) != OK)
	{
	    IIUGerr(E_CF0000_CANNOT_OPEN_FILE, 0, 1, file_out);
	    return FAIL;
	}

	/* Write file header */
	field_out[0] = ERx("COPYFORM:");
	field_out[1] = Cfversion;
	field_out[2] = ERx("now");
	field_out[3] = ERx("Converted from 5.0");
	field_out[4] = NULL;
	_50write(fd_out, field_out, FALSE);
	
	while ( SIgetrec(buff_in, sizeof(buff_in)-1, fd_in) != ENDFILE )
	{ /* read until end of file reached */
		register i4	i;
		i4		rec_type;
		char	*field_in[32];

		static const char	_Empty[1]	= {EOS},
								_Zero[]		= ERx("0");

		/* determine record type of input record */
		/*
		**  The following statements should not be
		**  coalesced together as it causes the VMS C
		**  compiler to generate bad code. (dkh)
		*/
		field_in[0] = cf_gettok(buff_in);
		rec_type = *(field_in[0]) == EOS
			? cur_type : _50hdrtype(field_in[0]);

		/* scan input string */
		for ( i = 0 ; (field_in[i] = cf_gettok((char *)NULL)) != NULL ;
				++i)
			;

		switch (rec_type)
		{
			case RT_FORMHDR:
				field_out[0] = ERx("FORM:");
				field_out[1] = field_in[0];
				field_out[2] = (char *)_Empty;
				field_out[3] = (char *)_Empty;
				field_out[4] = NULL;
				if (field_in[0] != NULL)
				{
				    STcopy(field_in[0], curr_form);
				}
				else
				{
				    IIUGerr(E_CF0025_BADFILE, UG_ERR_FATAL,
					    1, file_in);
				}
				_50write(fd_out, field_out, FALSE);
				cur_type = RT_FORMDET;
				break;
			case RT_FORMDET:
				field_out[0] = field_in[0];
				field_out[1] = field_in[1];
				field_out[2] = field_in[2];
				field_out[3] = field_in[3];
				field_out[4] = field_in[4];
				field_out[5] = field_in[5];
				field_out[6] = field_in[6];
				field_out[7] = ERx("6");
				
				field_out[8] = (char *)_Zero;
				field_out[9] = (char *)_Zero;
				field_out[10] = (char *)_Zero;
				field_out[11] = (char *)_Zero;
				field_out[12] = (char *)_Zero;
				field_out[13] = field_in[7];
				field_out[14] = (char *)_Zero;
				field_out[15] = field_in[9];
				field_out[16] = NULL;
				_50write(fd_out, field_out, TRUE);
				break;
			case RT_FIELDHDR:
				field_out[0] = ERx("FIELD:");
				field_out[1] = NULL;
				_50write(fd_out, field_out, FALSE);
				cur_type = RT_FIELDDET;
				break;
			case RT_FIELDDET:
			{
				i4			fldatatype_in;
				DB_DT_ID	fldatatype_out;
				i4			fllength_in;
				i4			fllength_out;

				char	tb10_1[10];
				char	tb10_2[10];
				char	tb16[61];

				field_out[0] = field_in[0];
				field_out[1] = field_in[1];
				/* determine fldatatype and fllength */
				CVan(field_in[2], &fldatatype_in);
				CVan(field_in[3], &fllength_in);
				switch (fldatatype_in)
				{
					case 0:
						/*
						**  Must be a table field
						**  entry.  Preserve info.
						*/
						fldatatype_out = fldatatype_in;
						fllength_out = fllength_in;
						break;
					case 1:
						fldatatype_out = DB_INT_TYPE;
						fllength_out = fllength_in;
						break;
					case 2:
						fldatatype_out = DB_FLT_TYPE;
						fllength_out = fllength_in;
						break;
					case 3:
						if ( fllength_in <= OLD_DB_CHAR_MAX )
						{ /* C type */
							fldatatype_out =
							    DB_CHR_TYPE;
							fllength_out =
							    fllength_in;
						}
						else
						{ /* Text type */
							fldatatype_out =
							    DB_TXT_TYPE;
							fllength_out = sizeof
							    (DB_TEXT_STRING) +
							    fllength_in - 1;
						}
						break;
				}
				CVna(fldatatype_out, field_out[2] = tb10_1);
				CVna(fllength_out, field_out[3] = tb10_2);
				field_out[4] = (char *)_Zero;
				field_out[5] = field_in[4];
				field_out[6] = field_in[5];
				field_out[7] = field_in[6];
				field_out[8] = field_in[7];
				field_out[9] = field_in[8];
				field_out[10] = field_in[9];
				field_out[11] = field_in[10];
				field_out[12] = field_in[11];
				field_out[13] = field_in[12];
				field_out[14] = field_in[13];
				field_out[15] = field_in[14];
				field_out[16] = field_in[15];
				field_out[17] = field_in[16];
				field_out[18] = (char *)_Zero;
				field_out[19] = (char *)_Zero;
				field_out[20] = (char *)_Zero;
				field_out[21] = field_in[17];
				/* convert flformat */
				if ( fldatatype_out == DB_TXT_TYPE ||
						fldatatype_out == DB_CHR_TYPE )
				{
				    /* ascii type */
				    field_out[22] = (*field_in[18] == '-') ?
					field_in[18] + 1 : field_in[18];
				}
				else if (fldatatype_out == 0)
				{
					/* table field, do nothing. */
					field_out[22] = field_in[18];
				}
				else
				{ /* numeric type */
					if ( *field_in[18] != '+' &&
				    		*field_in[18] != '-' )
					{
						/* add a leading '-' */
						tb16[0] = '-';
						STcopy( field_in[18], &tb16[1] );
						field_out[22] = tb16;
					}
					else
					{
						field_out[22] = field_in[18];
					}
				}
				field_out[23] = field_in[19];
				field_out[24] = field_in[20];
				field_out[25] = field_in[21];
				field_out[26] = field_in[22];
				field_out[27] = NULL;
				_50write(fd_out, field_out, TRUE);
				break;
			} /* end case RT_FIELDDET */
			case RT_TRIMHDR:
				field_out[0] = ERx("TRIM:");
				field_out[1] = NULL;
				_50write(fd_out, field_out, FALSE);
				cur_type = RT_TRIMDET;
				break;
			case RT_TRIMDET:
				field_out[0] = field_in[0];
				field_out[1] = field_in[1];
				field_out[2] = field_in[2];
				field_out[3] = (char *)_Zero;
				field_out[4] = (char *)_Zero;
				field_out[5] = (char *)_Zero;
				field_out[6] = (char *)_Zero;
				field_out[7] = NULL;
				_50write(fd_out, field_out, TRUE);
				break;
			case RT_QBFHDR:
				cur_type = RT_QBFDET;
				break;
			case RT_QBFDET:
			{
				char	tbuf61[61];

				field_out[0] = ERx("QBFNAME:");
				field_out[1] = field_in[0];
				/* construct default short remarks */
				field_out[2] = STprintf(tbuf61,
							ERx("FORM: %s, %s: %s"),
							curr_form,
							( field_in[2] != NULL &&
								*field_in[2] == '1' )
								? ERx("JOINDEF") : ERx("TABLE"),
							field_in[1]
				);

				field_out[3] = (char *)_Empty;
				field_out[4] = NULL;
				_50write(fd_out, field_out, FALSE);
				field_out[0] = field_in[1];
				field_out[1] = curr_form;
				field_out[2] = field_in[2];
				field_out[3] = NULL;
				_50write(fd_out, field_out, TRUE);
				break;
			}
			case RT_JDEFHDR:
				jfirst = TRUE;
				cur_type = RT_JDEFDET;
				break;
			case RT_JDEFDET:
				if (jfirst)
				{
					field_out[0] = ERx("JOINDEF:");
					field_out[1] = field_in[0];
					field_out[2] = (char *)_Empty;
					field_out[3] = (char *)_Empty;
					field_out[4] = NULL;
					_50write(fd_out, field_out, FALSE);
					jfirst = FALSE;
				}
				field_out[0] = field_in[1];
				field_out[1] = field_in[2];
				field_out[2] = field_in[3];
				field_out[3] = field_in[4];
				field_out[4] = field_in[5];
				field_out[5] = NULL;
				_50write(fd_out, field_out, TRUE);
				break;
		} /* end switch */
	} /* end while */

	SIclose(fd_in);
	SIclose(fd_out);

	return OK;
}

/*{
** Name:	_50hdrtype -	Return Header Type of Record in 5.0 Format.
**
** Description:
**	Given a ptr to an input string, returns the record type of the
**	intermediate form file.	 The record type is determined by a pattern
**	at the beginning of the string passed.	If the first character of
**	the input string is a tab, then the record type is determined from the
**	record type from the last call to this procedure.
**
** Input params:
**	token		{char *}  The record header token.
**
** Returns:
**	{nat}	RT_FORMHDR
**			RT_FIELDHDR
**			RT_TRIMHDR
**			RT_QBFHDR
**			RT_JDEFHDR
**			RT_BADTYPE
**
** History:
**	23-Apr-87 (rdesmond) written.
**	14-May-87 (rdesmond) removed check for ":" so 3.0 and 4.0 files are
**				acceptable.
**	06/16/88 (jhw) -- Changed to just return header type.
*/
static i4
_50hdrtype ( token )
char	*token;
{
	char	*cp;
	char	buf[64];

	STcopy( token, buf );
	if ( (cp = STindex( buf, ERx(":"), 0 )) != NULL )
		*cp = EOS;	/* ignore trailing ':' */
	CVupper( buf );

	if ( STequal(buf, ERx("QBFMAP")) )
	{
		return RT_QBFHDR;
	}
	else if ( STequal(buf, ERx("IIQBFINFO")) )
	{
		return RT_JDEFHDR;
	}
	else if ( STequal(buf, ERx("FORM")) )
	{
		return RT_FORMHDR;
	}
	else if ( STequal(buf, ERx("FDFIELDS")) )
	{
		return RT_FIELDHDR;
	}
	else if ( STequal(buf, ERx("FDTRIM")) )
	{
		return RT_TRIMHDR;
	}
	else
	{
		return RT_BADTYPE;
	}
}

/*{
** Name:	_50write - write a record to the form file.
**
** Description:
**	Given a ptr to an array of strings, writes one record, consisting
**	a tab separated list of the strings, to the output file whose
**	file descriptor is passed.  The output record may optionally be
**	preceded by a tab.
**
** Input params:
**	fd		file descriptor of file to write to.
**	field		ptr to an array of strings to write to file.
**	tab		flag indicating whether a preceding tab is desired.
**
** Output params:
**	none.
**
** Returns:
**	OK		successful
**	FAIL		not successful
**
** Exceptions:
**	none.
**
** Side Effects:
**
** History:
**	23-Apr-87 (rdesmond) written.
*/
static STATUS
_50write ( fd, field, tab )
register FILE	*fd;
register char	*field[];
bool			tab;
{
	register char	*lp;
	register i4	i;
	i4		count;			/* number of bytes written */
	char	linebuf[COPYFORM_MAX_REC]; /* output line buffer */

	lp = linebuf;
	if ( tab )
		*lp++ = '\t';

	i = 0;
	STcopy( field[i++], lp );
	lp += STlength(lp);
	while ( field[i] != NULL )
	{
		*lp++ = '\t';
		STcopy( field[i++], lp );
		lp += STlength(lp);
	}

	*lp++ = '\n';
	*lp = EOS;

	return ( SIwrite(lp - linebuf, linebuf, &count, fd) == OK )
				? OK : FAIL;
}
