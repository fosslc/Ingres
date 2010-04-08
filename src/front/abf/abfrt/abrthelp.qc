/*
** Copyright (c) 1991, 2008 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<cv.h>		 
#include	<st.h>
#include	<lo.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<uf.h>
#include	<abfcnsts.h>
#include	<fdesc.h>
#include	<abfrts.h>
#include	"keyparam.h"

/**
** Name:	abrthelp.qc -	ABF Run-time System Help Procedures.
**
** Description:
**	Contains the routines that access the FRS help facilities.  Defines:
**
**	IIARhelp()	display text file as help.
**	IIARhlpMenu()	display help sub-menu.
**	IIARfieldHelp()	display help on field or column.
**
** History:
**	Revision 6.4
**	03/11/91 (emerson)
**		Integrated another set of DESKTOP porting changes.
**
**	Revision 6.3/03/00  89/11  wong
**	Initial revision.
**
**	02-dec-91 (leighb) DeskTop Porting Change: Added include of <cv.h>.
**	08-jun-92 (leighb) DeskTop Porting Change:
**		Changed 'errno' to 'errnum' cuz 'errno' is a macro in WIN/NT
**	19-dec-96 (chech02)
**		changed iiARmsgDir to GLOBALREF.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
*/

GLOBALREF LOCATION	iiARmsgDir;

static const
	char	_subject[]	= ERx("subject"),
		_filename[]	= ERx("filename");

#define _NoHelpFileName		0
#define _BadHelpFileName	1

/*{
** Name:	IIARhelp() -	Display Text File as Help.
**
** Description:
**	Displays the text contained in a file as a help frame (using
**	##helpfile.)  The 'filename' parameter is a required keyword parameter
**	whose path is assumed to be specified by the application message
**	directory.  But the 'subject' keyword parameter is optional. The
**	recognized parameters are as follows:
**
**		subject		Any string type.  The subject line.
**		filename	Any string type.  The name of the file
**							containing the help
**							text.
** Inputs:
**	prm	{ABRTSPRM *}  The parameter structure for the call.
**	name	{char *}  Procedure name
**	proc	{ABRTSPRO *}  The procedure object.
**
** History:
**	10/89 (jhw) -- Written.
*/
PTR
IIARhelp ( prm, name, proc )
ABRTSPRM	*prm;
char		*name;
ABRTSPRO	*proc;
##{
	register char  		**fp;		/* pointer to formal name */
	register ABRTSPV	*ap;		/* pointer to actuals */
	register i4		cnt;

	KEYWORD_PRM	prm_desc;
##	char		subject[ABBUFSIZE+1];
	char		file_name[MAX_LOC+1];

	if ( prm == NULL )
	{
		iiarUsrErr(_NoHelpFileName, 1, name);
		return NULL;
	}

	/* assert((prm->pr_version > 1);	calling frame >= 6.0 */

	subject[0] = EOS;
	file_name[0] = EOS;

	prm_desc.parent = name;
	prm_desc.pclass = F_AR0006_procedure;
	for ( fp = prm->pr_formals, ap = prm->pr_actuals, cnt = prm->pr_argcnt ;
			fp != NULL && --cnt >= 0 ; ++fp, ++ap )
	{
		char	prm_name[FE_MAXNAME+1];

		if ( *fp == NULL )
			iiarUsrErr(E_AR0004_POSTOOSL, 1,ERget(prm_desc.pclass));

		prm_desc.name = *fp;
		prm_desc.value = (DB_DATA_VALUE *)ap->abpvvalue;
		STlcopy(*fp, prm_name, (u_i4)sizeof(prm_name) - 1);
		CVlower(prm_name);
		if ( STequal(prm_name, _subject) )
		{
			iiarGetValue( &prm_desc, (PTR)subject, DB_CHR_TYPE,
					sizeof(subject) - 1
			);
		}
		else if ( STequal(prm_name, _filename) )
		{
			iiarGetValue( &prm_desc, (PTR)file_name, DB_CHR_TYPE,
					sizeof(file_name) - 1
			);
		}
		else
		{
			iiarUsrErr( _UnknownPrm,
				3, *fp, name, ERget(F_AR0006_procedure)
			);
		}
	} /* end for */

	if ( STtrmwhite(file_name) <= 0 )
		iiarUsrErr(_NoHelpFileName, 1, name);
	else
	{
		char		tbuf[MAX_LOC+1];
		LOCATION	tmp;

		LOcopy(&iiARmsgDir, tbuf, &tmp);	 
		if ( LOfstfile(file_name, &tmp) != OK )
			iiarUsrErr(_BadHelpFileName, 2, name, file_name);
		else
##		{
##			char	*fn;

			LOtos(&tmp, &fn);
##			HELPFILE subject fn;
##		}
	}

	return NULL;
##}

/*{
** Name:	IIARhlpMenu() -	Display Help Sub-Menu.
**
** Description:
**	Displays the FRS help sub-menu, which displays the text in the specified
**	help file as one of its menu items (using ##help_frs.)  The 'filename'
**	parameter is a required keyword parameter whose path is assumed to be
**	specified by the application message directory.  The recognized keyword
**	parameters are as follows:
**
**		subject		Any string type.  The subject line; optional
**		filename	Any string type.  The name of the file
**							containing the help
**							text.
** Inputs:
**	prm	{ABRTSPRM *}  The parameter structure for the call.
**	name	{char *}  Procedure name
**	proc	{ABRTSPRO *}  The procedure object.
**
** History:
**	10/89 (jhw) -- Written.
*/
PTR
IIARhlpMenu ( prm, name, proc )
ABRTSPRM	*prm;
char		*name;
ABRTSPRO	*proc;
##{
	register char  		**fp;		/* pointer to formal name */
	register ABRTSPV	*ap;		/* pointer to actuals */
	register i4		cnt;

	KEYWORD_PRM	prm_desc;
##	char		subject[ABBUFSIZE+1];
	char		file_name[MAX_LOC+1];

	if ( prm == NULL )
	{
		iiarUsrErr(_NoHelpFileName, 1, name);
		return NULL;
	}

	/* assert((prm->pr_version > 1);	calling frame >= 6.0 */

	subject[0] = EOS;
	file_name[0] = EOS;

	prm_desc.parent = name;
	prm_desc.pclass = F_AR0006_procedure;
	for ( fp = prm->pr_formals, ap = prm->pr_actuals, cnt = prm->pr_argcnt ;
			fp != NULL && --cnt >= 0 ; ++fp, ++ap )
	{
		char	prm_name[FE_MAXNAME+1];

		if ( *fp == NULL )
			iiarUsrErr(E_AR0004_POSTOOSL, 1,ERget(prm_desc.pclass));

		prm_desc.name = *fp;
		prm_desc.value = (DB_DATA_VALUE *)ap->abpvvalue;
		STlcopy(*fp, prm_name, (u_i4)sizeof(prm_name) - 1);
		CVlower(prm_name);
		if ( STequal(prm_name, _subject) )
		{
			iiarGetValue( &prm_desc, (PTR)subject, DB_CHR_TYPE,
					sizeof(subject) - 1
			);
		}
		else if ( STequal(prm_name, _filename) )
		{
			iiarGetValue( &prm_desc, (PTR)file_name, DB_CHR_TYPE,
					sizeof(file_name) - 1
			);
		}
		else
		{
			iiarUsrErr( _UnknownPrm,
				3, *fp, name, ERget(F_AR0006_procedure)
			);
		}
	} /* end for */

	if ( STtrmwhite(file_name) <= 0 )
		iiarUsrErr(_NoHelpFileName, 1, name);
	else
	{
		char		tbuf[MAX_LOC+1];
		LOCATION	tmp;

		LOcopy(&iiARmsgDir, tbuf, &tmp);	 
		if ( LOfstfile(file_name, &tmp) != OK )
			iiarUsrErr(_BadHelpFileName, 2, name, file_name);
		else
##		{
##			char	*fn;

			LOtos(&tmp, &fn);
##			HELP_FRS ( SUBJECT = subject, FILE = fn );
##		}
	}

	return NULL;
##}

/*{
** Name:	IIARfieldHelp() -	Display Help for Field or Column.
**
** Description:
**	Displays the FRS field/column help frame for a field or column.  This
**	will be either a pop-up selection form of the valid values for the
**	field or column, or a description of the field or column.
**
**	By default, if no parameters are specified, the current field or column
**	is used.  The recognized keyword parameters are as follows:
**
**		form		Any string type.  The form name; optional.
**		field		Any string type.  The field name; optional.
**		column		Any string type.  The column name; optional.
** Inputs:
**	prm	{ABRTSPRM *}  The parameter structure for the call.
**	name	{char *}  Procedure name
**	proc	{ABRTSPRO *}  The procedure object.
**
** Returns:
**	{PTR}  NULL always.
**
** History:
**	11/89 (jhw) -- Written.
*/

STATUS	(*IIseterr())();

# define	FRMNOINIT	0x00b51fc0
# define	RTRFFL		0x00b51faa
# define	TBBAD		0x00b5203b
# define	TBBADCOL	0x00b52041

/*
** Name:	_frsError() -	EQL/FRS Error Handler.
**
** Description:
**	Handles certain FRS errors.  Note:  The SIATTR error will often occur
**	as the result of other errors and must be handled as well.  In these
**	cases, test for both the original error and SIATTR from 'FEinqferr()'.
*/
#define _SIFRM	8152
#define _SIFLD	8154
#define _SICOL	8155
#define _SIATTR	8156

static STATUS
_frsError ( err )
STATUS	*err;
{
	if ( *err == _SIFRM || *err == _SIFLD || *err == _SICOL )
		return 0;
	else if ( *err == _SIATTR )
		return 0;	/* ignore derivative error */
	else
		return *err;
	/*NOT REACHED*/
}

PTR
IIARfieldHelp ( prm, name, proc )
ABRTSPRM	*prm;
char		*name;
ABRTSPRO	*proc;
##{
	register char  		**fp;		/* pointer to formal name */
	register ABRTSPV	*ap;		/* pointer to actuals */
	register i4		cnt;
	STATUS			(*user_errh)();	/* User Error Handler */
	STATUS			errnum;		 
	i4			row;
	DB_DATA_VALUE		retvalue;

##	i4		istable;
##	char		formname[FE_MAXNAME+1];
##	char		fldname[FE_MAXNAME+1];
##	char		colname[FE_MAXNAME+1];
	KEYWORD_PRM	prm_desc;
	FLD_VALUE	fldv;

	row = 0;
	retvalue.db_data = (PTR)&row;
	retvalue.db_datatype = DB_INT_TYPE;
	retvalue.db_length = sizeof(row);

	IIARrvlReturnVal( &retvalue, prm, name, ERget(F_AR0006_procedure) );

	if ( prm == NULL )
	{
		IIUFfieldHelp();
		return NULL;
	}

	/* assert((prm->pr_version > 1);	calling frame >= 6.0 */

	fldv._form = formname;
	fldv._field = fldname;
	fldv._column = colname;

	formname[0] = EOS;
	fldname[0] = EOS;
	colname[0] = EOS;

	prm_desc.parent = name;
	prm_desc.pclass = F_AR0006_procedure;
	for ( fp = prm->pr_formals, ap = prm->pr_actuals, cnt = prm->pr_argcnt ;
			fp != NULL && --cnt >= 0 ; ++fp, ++ap )
	{
		char	prm_name[FE_MAXNAME+1];

		if ( *fp == NULL )
			iiarUsrErr(E_AR0004_POSTOOSL, 1,ERget(prm_desc.pclass));

		prm_desc.name = *fp;
		prm_desc.value = (DB_DATA_VALUE *)ap->abpvvalue;
		STlcopy(*fp, prm_name, (u_i4)sizeof(prm_name) - 1);
		CVlower(prm_name);
		if ( STequal(prm_name, ERx("form")) )
		{
			iiarGetValue( &prm_desc, (PTR)formname, DB_CHR_TYPE,
					sizeof(formname) - 1
			);
		}
		else if ( STequal(prm_name, ERx("field")) )
		{
			iiarGetValue( &prm_desc, (PTR)fldname, DB_CHR_TYPE,
					sizeof(fldname) - 1
			);
		}
		else if ( STequal(prm_name, ERx("column")) )
		{
			iiarGetValue( &prm_desc, (PTR)colname, DB_CHR_TYPE,
					sizeof(colname) - 1
			);
		}
		else
		{
			iiarUsrErr( _UnknownPrm,
				3, *fp, name, ERget(F_AR0006_procedure)
			);
		}
	} /* end for */


	if ( formname[0] == EOS )
	{
##		INQUIRE_FRS FORM ( formname = NAME );
		if ( FEinqferr() != OK || formname[0] == EOS )
			return NULL;
	}

	user_errh = IIseterr(_frsError);
	if ( fldname[0] == EOS )
	{
##		INQUIRE_FRS FORM ( fldname = FIELD(formname) );
		if ( (errnum = FEinqferr()) != OK || fldname[0] == EOS )
		{
	    		_VOID_ IIseterr(user_errh);
			if ( errnum == _SIFRM || errnum == _SIATTR )
	    			IIFDerror(FRMNOINIT, 1, formname);
			return NULL;
		}
	}

##	INQUIRE_FRS FIELD formname ( istable = TABLE(fldname) );
	if ( (errnum = FEinqferr()) != OK )
	{ /* ... likely not a field on the form */
	    	_VOID_ IIseterr(user_errh);
		if ( errnum == _SIFRM )
	    		IIFDerror(FRMNOINIT, 1, formname);
		else if ( errnum == _SIFLD || errnum == _SIATTR )
	    		IIFDerror(RTRFFL, 2, formname, fldname);
		return NULL;
	}

	if ( colname[0] != EOS && istable == 0 )
	{ /* not a table field, but a column was specified */
	    	_VOID_ IIseterr(user_errh);
	    	IIFDerror(TBBAD, 2, fldname, formname);
		return NULL;
	}
	else if ( istable == 1 )
	{
		if ( colname[0] == EOS )
		{
##			INQUIRE_FRS TABLE formname (colname = COLUMN(fldname));
			if ( FEinqferr() != OK || colname[0] == EOS )
			{ /* shouldn't happen */
	    			_VOID_ IIseterr(user_errh);
				return NULL;
			}
		}
		else
		{ /* Check for column existence */
			/* As a side-effect, this will report an error if
			** the named column is not actually a column on the
			** table field.
			*/
##			INQUIRE_FRS COLUMN formname fldname (
##					istable = NUMBER(colname)
##			);
			if ( (errnum = FEinqferr()) != OK )
			{
	    			_VOID_ IIseterr(user_errh);
				if ( errnum == _SICOL || errnum == _SIATTR )
					IIFDerror(TBBADCOL, 2, fldname, colname);
				return NULL;
			}
		}
	}
	_VOID_ IIseterr(user_errh);

	row = IIUFfldHelp(&fldv);
	IIARrvlReturnVal( &retvalue, prm, name, ERget(F_AR0006_procedure) );

	return NULL;
##}
