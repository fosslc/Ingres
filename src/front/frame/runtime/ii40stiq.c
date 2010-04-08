/*
**	ii40stiq.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<cv.h>		 
# include	<st.h>		 
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	<menu.h>
# include	<runtime.h>
# include	"setinq.h"
# include	<fsicnsts.h>
# include	<frserrno.h>
# include	<er.h>
# include	"erfr.h"
# include	<lqgdata.h>
# include	<rtvars.h>

/**
** Name:	ii40stiq.c
**
** Description:
**	Set and inquire command support for 4.0 release and later
**
**	Public (extern) routines defined:
**		IIiqfsio()
**		IIstfsio()
**	Private (static) routines defined:
**
** History:
**	Created - 08/29/85 (dkh)
**	5/2/86	- (mgw, bruceb) bug #8833
**		Added IIset30mode() to support 3.0 version of 'mode <md>' in
**		4.0 release.
**	9/26/86 - (brentw) bug #10028 -- set_frs (mapfile) statement on Unix
**		barfs if filename has upper case letters -- it forces 
**		filename to lower then complains about file not found.
**		Since VMS, IBM and PC don't care about filename case, we can
**		make this as a global change to the system, rather than as a 
**		machine dependent change.  
**	2/27/87 - Modified for ADTs and NULLs (drh)
**	05/02/87 (dkh) - Integrated change bit code.
**	19-jun-87 (bruceb) Code cleanup.
**	07/28/87 (dkh) - Added support for change bit and datatype statements.
**	08/14/87 (dkh) - ER changes.
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
**	09/01/87 (dkh) - Changed <eqforms.h> to "setinq.h"
**	09/01/87 (dkh) - Added explicit include of <frserrno.h>.
**	09/16/87 (dkh) - Integrated table field change bit.
**	08-jan-88 (bruceb)
**		Eliminate assignment of nat's to 'nat *'s (this has the
**		side effect on DG of changing the least significant bit
**		to 0.)  Changes made to IIsimob, IIiqfsio and IIstfsio.
**	11-nov-88 (bruceb)
**		Populate the S_FR0060_TMOUT row of IIfsiinfo.
**		Verify that FRS turned on before accepting any inquire_frs
**		or set_frs statements.
**	16-dec-88 (bruceb)
**		In IIiqfsio, use buffer of size 2*ER_MAX_LEN rather than
**		of size MAXIQSET.  This is because MAXIQSET is (currently)
**		64 and ER_MAX_LEN is (currently) 1000.  Inquiring on the
**		error text requires a longer buffer, and the suggested
**		length is 2*ER_MAX_LEN.
**	20-dec-88 (bruceb)
**		Added rows to IIfsiinfo for frsRDONLY and frsINVIS.
**	06-feb-89 (bruceb)
**		Changed frsFLDFMT to be both set and inquire, not just inquire.
**		(MAXIQSET now valued at 255 for formats).
**	09-mar-89 (bruceb)
**		Added row to IIfsiinfo for frsENTACT.
**	27-mar-89 (bruceb)
**		Don't lowercase the text on a 'set format' statement.
**	24-apr-89 (bruceb)
**		Added row to IIfsiinfo for frsSHELL.
**	08/28/89 (dkh) - Added support for inquriing on frskey that
**			 caused activation.
**	08-sep-89 (bruceb)
**		Invisibility now available for table field columns.
**	27-sep-89 (bruceb)
**		Added row to IIfsiinfo for frsEDITOR.
**	10/06/89 (dkh) - Added support for inquiring on derivation string
**			 and whether a field/column is derived.
**	12/07/89 (dkh) - Added support for inquiring on the precision
**			 and scale of a decimal field type.
**	01/11/90 (esd) - Modified for FT3270 (bracketed by #ifdef's):
**			 Don't allow mapping to control keys.
**	07-feb-90 (bruceb)
**		Added support for Getmessage display/supression.
**		Added support for single character find function.
**	07/24/90 (dkh) - Added support for table field cell highlighting.
**	11/02/90 (dkh) - Replaced IILIBQgdata with IILQgdata().
**	01/11/91 (dkh) - Fixed bug 35227.
**	08/02/91 (dkh) - Fixed bug 38222.
**	04/26/92 (dkh) - Changed definition of FSIINFO to use i2s to fix
**			 ANSI C compilation problems.
**	08/20/92 (dkh) - Added support for set/inquire on existence of
**			 forms objects, dynamic menus and modifying
**			 behavior of "Out of Data" message.
**	12/14/92 (dkh) - Added INTERNAL USE ONLY option to change the number of
**			 rows for a table field.  USERS WON'T TRIP ON THIS
**			 SINCE THE PREPROCESSORS PROHIBIT THE USE OF SET_FRS
**			 WITH THE MAXROW ATTRIBUTE.
**	01/20/93 (dkh) - Added support for the inputmasking toggle.
**	07/24/93 (dkh) - Moved string literals to the message files.
**      03-Feb-96 (fanra01)
**          Extracted data for DLLs on Windows NT.
**	05/08/97 (kitch01)
**			Amend IIstfsio() to set displayonly attribute if MWS is active. This allows
**			the changed FRS attribute to be propogated to Upfront.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

# define        FSIBUFLEN       63
 
# define        FSI_SET         01
# define        FSI_INQ         02
# define        FSI_FRS         04
# define        FSI_FORM        010
# define        FSI_FIELD       020
# define        FSI_TABLE       040
# define        FSI_COL         0100
# define        FSI_ROW         0200
# define        FSI_RC          0400
# define        FSI_MU          01000
 
# define        FSI_SI          (FSI_SET | FSI_INQ)
# define        FSI_OBJS        (FSI_FIELD | FSI_COL | FSI_ROW | FSI_RC)
# define        FSI_OOBJ        (FSI_FIELD | FSI_COL | FSI_FORM | FSI_TABLE)
# define        FSI_FC          (FSI_FIELD | FSI_COL)
# define        FSI_FFRC        (FSI_FORM | FSI_FIELD | FSI_RC)
# define        FSI_RFF         (FSI_ROW | FSI_FORM | FSI_FIELD)
# define        FSI_HTWID       (FSI_FIELD | FSI_COL | FSI_FRS | FSI_FORM)
# define        FSI_FFC         (FSI_FORM | FSI_FIELD | FSI_COL)
 
 
/*
**  Changed "flags" from i2 to i1; "dtype" split into "sdtype" and "idtype".
**  Fix for BUG 7576. (dkh)
*/
 
typedef struct fsistruct
{
        ER_MSGID        name;
        i2              flags;
        i2              sdtype;
        i2              idtype;
        i2              rtcode;
} FSIINFO;

GLOBALREF       FSIINFO IIfsiinfo[];

FUNC_EXTERN	RUNFRM		*RTgetfrm();
FUNC_EXTERN	TBSTRUCT	*RTgettbl();
FUNC_EXTERN	FIELD		*RTgetfld();
FUNC_EXTERN	FLDCOL		*RTgetcol();
FUNC_EXTERN	i4		IIRTsetmu();
FUNC_EXTERN	i4		IIRTinqmu();
FUNC_EXTERN	STATUS		adh_evcvtdb();
FUNC_EXTERN	STATUS		adh_dbcvtev();
FUNC_EXTERN	FIELD		*FDfndfld();
FUNC_EXTERN	i4		(*IIseterr())();

static i4 IIsicpfcnv(char *str, i4 *val);

GLOBALREF       i4      IIsiobj;
GLOBALREF       RUNFRM  *IIsirfrm;
GLOBALREF       FRAME   *IIsifrm;
GLOBALREF       TBLFLD  *IIsitbl;
GLOBALREF       i4      IIsirow;
GLOBALREF       TBSTRUCT        *IIsitbs;

/*
** IIsifld is set for tablefield operations only, and is the FIELD
** structure to use in calculating display area sizes
*/
GLOBALREF       FIELD   *IIsifld;


static	i4	err_to_ignore = 0;	/* error number to ignore */

# define	SI_NOFORM	8152	/* no form found */
# define	SI_NOFLD	8154	/* no table field found */
# define	SI_NOCOL	8155	/* no tf col found */



/*{
** Name:	IIFRceCatchError - Error catcher for inquire_frs.
**
** Description:
**	<comments>
**
** Inputs:
**	errnum		Pointer to error number that triggered this call.
**
** Outputs:
**
**	Returns:
**	0		If we want to ignore this error (set in var
**			err_to_ignore).
**	*errnum		Value pointed to by errnum.
**
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	09/01/92 (dkh) - Initial version.
*/
i4
IIFRceCatchError(errnum)
i4	*errnum;
{
	if (*errnum == err_to_ignore)
	{
		return(0);
	}
	return(*errnum);
}



/*{
** Name:	IIiqobjtype	- get name of object type
**
** Description:
**	Given a code for an object type, return the name to use for the
**	object type ("FRAME", etc.)
**
** Inputs:
**	objtype		code for the object type
**
** Outputs:
**	
**
** Returns:
**	Ptr to the name of the object type
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	???
*/

char *
IIiqobjtype(i4 objtype)
{
	char	*obj;

	switch(objtype)
	{
		case FsiFRS:
			obj = ERx("FRS");
			break;

		case FsiFORM:
			obj = ERx("FORM");
			break;

		case FsiFIELD:
			obj = ERx("FIELD");
			break;

		case FsiTABLE:
			obj = ERx("TABLE");
			break;

		case FsiCOL:
			obj = ERx("COLUMN");
			break;

		case FsiROW:
			obj = ERx("ROW");
			break;

		case FsiRWCL:
			obj = ERx("ROW-COLUMN");
			break;

		case FsiMUOBJ:
			obj = ERx("MENU");
			break;

		default:
			break;
	}
	return(obj);
}

/*{
** Name:	IIiqset		- Set globals for object being set / inquired
**
** Description:
**	Set globals for the object being set/inquired.
**
**	This routine is part of RUNTIME's external interface.
**	
** Inputs:
**	objtype		Code for object being set/inquired about
**	row		Row number
**	formname	Name of the form
**	fieldname	Name of the field
**	term		?? seems to be unused ??
**
** Outputs:
**	
**
** Returns:
**	i4		TRUE
**			FALSE if parms are incorrect
**
** Exceptions:
**	none
**
** Side Effects:
**
**	Sets the globals IIsirfrm, IIsitbl, IIsirow to refer to the frame,
**	tablefield, and row being set or inquired.
**
** Example and Code Generation:
**
**	## inquire_frs form (charvar = field)
**
**	if (IIiqset(FsiFORM, 0, (char *)0, (char *)0) {
**	  IIiqfsio(0,ISREF, DB_CHR_TYPE, 0, charvar, FsiFIELD, (char *)0, 0);
**	}
**
**	## set_frs field form1 (reverse(field2) = 1)
**
**	if (IIiqset(FsiFIELD, 0, "form1", (char *)0) {
**	  IIstfsio(FsiRVVID, "field2", 0, ISVAL, DB_INT_TYPE, sizeof(i4), 1);
**	}
**
** History:
**	?????
*/
/* VARARGS */
i4
IIiqset(objtype, row, formname, fieldname, term)
i4	objtype;
i4	row;
char	*formname;
char	*fieldname;
char	*term;
{
	i4		argcnt;
	RUNFRM		*runf;
	TBSTRUCT	*tblstr;
	char		*objname;
	char		*nmlist[3];
	char		**argv;
	char		buf1[MAXFRSNAME + 1];
	char		buf2[MAXFRSNAME + 1];
	bool		junk;

	argcnt = 0;
	argv = nmlist;

	objname = IIiqobjtype(objtype);

	if (formname != NULL)
	{
		argcnt = 1;
		nmlist[0] = IIstrconv(II_CONV, formname, buf1, MAXFRSNAME);
		if (fieldname != NULL)
		{
			argcnt++;
			nmlist[1] = IIstrconv(II_CONV, fieldname, buf2,
				MAXFRSNAME);
		}
	}

	IIsiobj = objtype;
	switch(objtype)
	{
		case FsiFRS:
		case FsiFORM:
			break;

		case FsiFIELD:
		case FsiTABLE:
		case FsiMUOBJ:
			if ((runf = IIget_stkf(argcnt, 1, objname, argv)) ==
				NULL)
			{
				return(FALSE);
			}
			IIsirfrm = runf;
			IIsifrm = runf->fdrunfrm;
			break;

		case FsiCOL:
		case FsiROW:
		case FsiRWCL:
			if ((runf = IIget_stkf(argcnt, 2, objname, argv)) ==
				NULL)
			{
				return(FALSE);
			}

			/*
			**  Set/inquire on a ROW_COLUMN must be
			**  off of the current form.  Get out of here
			**  if this is not the case.
			*/
			if (objtype == FsiRWCL)
			{
				if (runf != IIstkfrm)
				{
					return(FALSE);
				}
			}

			IIsirfrm = runf;
			IIsifrm = runf->fdrunfrm;
			if ((tblstr = RTgettbl(runf, *++argv)) == NULL)
			{
				IIFDerror(SITBL, 1, *argv);
				return(FALSE);
			}
			IIsitbl = tblstr->tb_fld;
			IIsifld = FDfndfld(IIsifrm,IIsitbl->tfhdr.fhdname,&junk);
			IIsitbs = tblstr;
			if (objtype == FsiRWCL)
			{
				if (row == 0)
				{
					row = IIsitbl->tfcurrow + 1;
				}
				else if (row < 1 || row > IIsitbl->tfrows)
				{
					IIFDerror(SIROW, 0);
					return(FALSE);
				}
			}
			IIsirow = row;
			break;

		default:
		{
			i4	tobj;

			tobj = objtype;
			IIFDerror(E_FI1FF8_8184, 1, &tobj);
			return(FALSE);
		}
	}

	return(TRUE);
}

/*{
** Name:	IIsimobj	-	??
**
** Description:
**	???
**
** Inputs:
**	pobj		Object code
**	attr		No. of the attr being set or inq
**	objname		Name of the object
**	row		Row number
**	RTcode	
**	arg1
**	arg2
**
** Outputs:
**	RTcode	
**	arg1
**	arg2
**
** Returns:
**	i4		TRUE
**			FALSE
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	??????
*/

i4
IIsimobj(i4 pobj, i4 attr, char *objname, i4 row, 
	i4 *RTcode, i4 **arg1, i4 **arg2)
{
	RUNFRM		*runf;
	FIELD		*fld;
	FLDCOL		*fldcol;
	TBSTRUCT	*tbl;
	FSIINFO		*info;
	char		*pobjnm;
	i4		(*oldproc)();

	info = &(IIfsiinfo[attr]);
	pobjnm = IIiqobjtype(pobj);

	switch(pobj)
	{
		case FsiFRS:
			/*
			**  Set and inquires on FRS require
			**  nothing to be set.	But still need to
			**  translate to opcodes that the
			**  RT routines understand.
			*/
			if (!(info->flags & FSI_FRS))
			{
				return(FALSE);
			}
			**arg1 = row;

			/*
			**  Get current form (if any) in case we
			**  are going to handle a last command
			**  request.
			*/
			*arg2 = NULL;
			if (IIstkfrm != NULL)
			{
				*arg2 = (i4 *) IIstkfrm->fdrunfrm;
			}
			break;

		case FsiFORM:
			if (!(info->flags & FSI_FORM))
			{
				return(FALSE);
			}
			if (info->rtcode == frsEXISTS &&
				(objname == NULL || *objname == EOS))
			{
				IIFDerror(E_FI1FF1_8177, 0, NULL);
				return(FALSE);
			}
			err_to_ignore = SI_NOFORM;
			oldproc = IIseterr(IIFRceCatchError);
			if ((runf = IIget_stkf(1, 1, pobjnm, &objname)) == NULL)
			{
				(void) IIseterr(oldproc);

				if (info->rtcode == frsEXISTS)
				{
					*arg1 = NULL;
					*arg2 = NULL;
				}
				else
				{
					return(FALSE);
				}
			}
			else
			{
				(void) IIseterr(oldproc);

				*arg1 = (i4 *) runf;
				*arg2 = (i4 *) runf->fdrunfrm;
			}

			break;

		case FsiFIELD:
			if (!(info->flags & FSI_FIELD))
			{
				return(FALSE);
			}
			if (info->rtcode == frsEXISTS &&
				(objname == NULL || *objname == EOS))
			{
				IIFDerror(E_FI1FF2_8178, 0, NULL);
				return(FALSE);
			}

			err_to_ignore = SI_NOFLD;
			oldproc = IIseterr(IIFRceCatchError);

			if ((fld = RTgetfld(IIsirfrm, objname)) == NULL)
			{
				(void) IIseterr(oldproc);

				if (info->rtcode == frsEXISTS)
				{
					*arg1 = (i4 *) fld;
					*arg2 = NULL;
				}
				else
				{
					IIFDerror(SIFLD, 1, objname);
					return(FALSE);
				}
			}
			else
			{
				(void) IIseterr(oldproc);

				*arg1 = (i4 *) fld;
				*arg2 = (i4 *) fld->fld_var.flregfld;
			}

			break;

		case FsiTABLE:
			if (!(info->flags & FSI_TABLE))
			{
				return(FALSE);
			}

			if ((tbl = RTgettbl(IIsirfrm, objname)) == NULL)
			{
				IIFDerror(SITBL, 1, objname);
				return(FALSE);
			}

			*arg1 = (i4 *) tbl;
			*arg2 = (i4 *) tbl->tb_fld;
			break;

		case FsiCOL:
			if (!(info->flags & FSI_COL))
			{
				return(FALSE);
			}

			err_to_ignore = SI_NOCOL;
			oldproc = IIseterr(IIFRceCatchError);

			if (info->rtcode == frsEXISTS &&
				(objname == NULL || *objname == EOS))
			{
				IIFDerror(E_FI1FF3_8179, 0, NULL);
				return(FALSE);
			}

			if ((fldcol = RTgetcol(IIsitbl, objname)) == NULL)
			{
				(void) IIseterr(oldproc);

				if (info->rtcode == frsEXISTS)
				{
					*arg1 = (i4 *) IIsitbl;
					*arg2 = (i4 *) fldcol;
				}
				else
				{
					IIFDerror(SICOL, 1, objname);
					return(FALSE);
				}
			}
			else
			{
				(void) IIseterr(oldproc);

				*arg1 = (i4 *) IIsitbl;
				*arg2 = (i4 *) fldcol;
			}
			break;

		case FsiROW:
			if (!(info->flags & FSI_ROW))
			{
				return(FALSE);
			}

		/*
			if (row < 1 || row > IIsitbl->tfrows)
			{
				IIFDerror(SIROW, 0);
				return(FALSE);
			}
		*/
			if ((fldcol = RTgetcol(IIsitbl, objname)) == NULL)
			{
				IIFDerror(SIRC, 1, objname);
				return(FALSE);
			}
			*arg1 = (i4 *) IIsitbl;
			*arg2 = (i4 *) fldcol;

			/*
			**  Check if a real row number (!= 0) passed in the
			**  IIiqset().  If so, must be a set/inquire
			**  for change bit.  For a set/inquire for
			**  display attribute, the row number is passed
			**  on the IIiqfsio()/IIstfsio() call, which calls this
			**  routine.  Even if a set/inquire is for
			**  change bit on current row (IIiqset passes
			**  a zero), this check and assignment is still
			**  OK since we just set IIsirow to zero again.
			*/
			if (IIsirow == 0)
			{
				IIsirow = row;
			}
			break;

		case FsiRWCL:
			if (!(info->flags & FSI_RC))
			{
				return(FALSE);
			}

			if ((fldcol = RTgetcol(IIsitbl, objname)) == NULL)
			{
				IIFDerror(SIRC, 1, objname);
				return(FALSE);
			}
			*arg1 = (i4 *) IIsitbl;
			*arg2 = (i4 *) fldcol;
			break;

		case FsiMUOBJ:
			if (!(info->flags & FSI_MU))
			{
				return(FALSE);
			}

			*arg1 = (i4 *) IIsirfrm;
			*arg2 = (i4 *) objname;
			break;

		default:
			return(FALSE);
	}
	*RTcode = info->rtcode;
	return(TRUE);
}

/*{
** Name:	IIstfsio	-	Set an FRS variable
**
** Description:
**	Convert value provided by the user to the type expected by
**	the RT routine that will set the value.	 Call the appropriate
**	RT routine for the type of forms object being set.  Then,
**	if the object is a field or column and the attribute is one of
**	a field's visual attributes (i.e. color, intensity, etc.),
**	call another RT routine to set those (FT?).
**
**	This routine is part of RUNTIME's external interface.
**	
** Inputs:
**	isvar		ignored.
**	type		type of user variable
**	length		no. of bytes in user variable
**	data		ptr to the user variable containing the value to set
**	attr		code for attribute to set
**	name		??
**	row		row number
**
** Outputs:
**	
**
** Returns:
**	i4		TRUE if all ok
**			FALSE if error in parm, or failure to set
**
** Exceptions:
**	none
**
** Example and Code Generation: (See IIiqset)
**
** Side Effects:
**
** History:
**	27-feb-1987	Created with code extracted from IIstfrs, modified
**			for ADTs and NULLs (drh)
**	19-feb-92 (leighb) DeskTop Porting Change:
**		adh_evcvtdb() has only 3 args, bogus 4th one deleted.
**	05-aug-97 (kitch01)
**		For MWS only ensure that we set the displayonly attribute. 
**		This is required by Upfront to alter the appearance of the
**		field depending on this flag.
*/

i4
IIstfsio(i4 attr, char *name, i4 row, i2 *nullind,
	i4 isvar, i4 type, i4 length, PTR tdata)
{
	char			*objname;
	char			*attrname;
	char			*strptr;
	i4			argn;
	i4			*arg1 = NULL;
	i4			*arg2 = NULL;
	i4			RTcode;
	i4			(*func)();
	char			*pobjname;
	FSIINFO			*info;
	char			buf1[MAXFRSNAME + 1];
	DB_EMBEDDED_DATA	edv;
	DB_DATA_VALUE		dbv;
	i4			natval;
	i4			specnat;
	char			specc[ MAXIQSET + 1];
	char			cval[ MAXIQSET + 1];
	char			tname[50];

	if (attr >= FsiATTR_MAX)
	{
		i4	tattr;

		tattr = attr;
		IIFDerror(E_FI1FF7_8183, 1, &tattr);
		return(FALSE);
	}

	if (!IILQgdata()->form_on)
	{
		IIFDerror(RTNOFRM, 0, NULL);
		return(FALSE);
	}

	info = &(IIfsiinfo[attr]);
	pobjname = IIiqobjtype(IIsiobj);
	objname = IIstrconv(II_CONV, name, buf1, MAXFRSNAME);

	/* ensure that *arg1 is a valid location to store data (in IIsimobj) */
	if (IIsiobj == FsiFRS)
		arg1 = &argn;

	if (!IIsimobj(IIsiobj, attr, objname, row, &RTcode, &arg1, &arg2))
	{
		STcopy(ERget(info->name), tname);
		attrname = tname;
		IIFDerror(SIATTR, 1, attrname);
		return(FALSE);
	}

	/*
	**  Renaming or setting the state of a menuitem requires
	**  that the case of the name be preserved.
	*/
	if (RTcode == frsMURNME || RTcode == frsMUONOF)
	{
		objname = IIstrconv(II_TRIM, name, buf1, MAXFRSNAME);
	}

	/*
	**  Build an embedded data value from the caller's parameters
	*/

	edv.ed_type = type;
	edv.ed_length = length;
	edv.ed_data = isvar ? tdata : (PTR) &tdata;
	edv.ed_null = nullind;

	/*
	**  Handle setting frsMAP when the caller has provided a character
	**  string value to set as a special case. ( is this done for
	**  compatability?)
	*/

	if (RTcode == frsMAP && type == DB_CHR_TYPE )
	{
		char	*tcp;

		tcp = IIstrconv(II_CONV, tdata, specc, (i4) FSIBUFLEN);
		if (!IIsicpfcnv(tcp, &specnat ))
		{
			IIFDerror(SIBADCPF, 1, tdata);
			return(FALSE);
		}

		edv.ed_type = DB_INT_TYPE;
		edv.ed_length = sizeof(i4);
		edv.ed_data = (PTR) &specnat;
		edv.ed_null = (i2 *) NULL;
	}

	/*
	**  Convert the embedded data value into a db data value of
	**  the type required by the set operation.
	*/

	switch(info->sdtype)
	{
		case fdINT:
		case fdFLOAT:
			dbv.db_datatype = DB_INT_TYPE;
			dbv.db_length = sizeof( i4  );
			dbv.db_prec = 0;
			dbv.db_data = (PTR) &natval;
			break;

		case fdSTRING:
			dbv.db_datatype = DB_CHR_TYPE;
			dbv.db_length = MAXIQSET;
			dbv.db_prec = 0;
			dbv.db_data = cval;
			break;

		default:
			return(FALSE);
	}

	if ( adh_evcvtdb( FEadfcb(), &edv, &dbv ) != OK )     
	{
		/*  Data conversion error */

		IIFDerror(RTSIERR, 0);
		return( FALSE );
	}



	if ( info->sdtype == fdSTRING )
	{
		/* null terminate the string and trim trailing white */

		cval[ MAXIQSET ] = EOS;
		strptr = cval;
		STtrmwhite( strptr );

		if ( RTcode != frsMAPFILE && RTcode != frsLABEL
			&& RTcode != frsFLDFMT && RTcode != frsMURNME
			&& RTcode != frsMUONOF)
		{
			/* force to lower case */

			CVlower( strptr );
		}
	}

	switch(IIsiobj)
	{
		case FsiFRS:
			func = RTsetsys;
			break;

		case FsiFORM:
			func = RTsetfrm;
			break;

		case FsiFIELD:
			func = RTsetfld;
			break;

		case FsiTABLE:
			func = RTsettbl;
			break;

		case FsiCOL:
			func = RTsetcol;
			break;

		case FsiROW:
			func = RTsetrow;
			break;

		case FsiRWCL:
			func = RTsetrc;
			break;

		case FsiMUOBJ:
			func = IIRTsetmu;
			break;

		default:
		{
			i4	tobj;

			tobj = IIsiobj;
			IIFDerror(E_FI1FF8_8184, 1, &tobj);
			return(FALSE);
		}
	}
	(void) (*func)(arg1, arg2, RTcode, dbv.db_data );

	if (IIsiobj == FsiFIELD || IIsiobj == FsiCOL)
	{
		if (IIsirfrm == IIstkfrm)
		{
			switch(RTcode)
			{
				case frsNORMAL:
				case frsRVVID:
				case frsBLINK:
				case frsUNLN:
				case frsINTENS:
				case frsCOLOR:
					if (IIsiobj == FsiFIELD)
					{
						if (((FIELD *) arg1)->fltag !=
							FREGULAR)
						{
							break;
						}
						RTfldda(IIsifrm, arg1, arg2);
					}
					else
					{
						RTcolda(IIsifrm, arg1, arg2);
					}
					break;

				case frsRDONLY:
				/* If MWS is active try to set attribute.*/
					if (IIMWimIsMws())
					{
						if (IIsiobj == FsiFIELD)
						{
							if (((FIELD *) arg1)->fltag !=
								FREGULAR)
							{
								break;
							}
							RTfldda(IIsifrm, arg1, arg2);
						}
						else
						{
							RTcolda(IIsifrm, arg1, arg2);
						}
					}
					break;
			}
		}
	}

	return(TRUE);
}


/*{
** Name:	IIstfrs -	Set an FRS variable
**
** Description:
**	Sets an FRS variable.  This is a cover routine for pre 6.0
**	compatability.
**
**	This routine is part of RUNTIME's external interface for compatability
**	only.
**	
** Inputs:
**	isvar		ignored.
**	type		type of user variable
**	length		no. of bytes in user variable
**	data		ptr to the user variable containing the value to set
**	attr		code for attribute to set
**	name		??
**	row		row number
**
** Outputs:
**	
**
** Returns:
**	i4		TRUE if all ok
**			FALSE if error in parm, or failure to set
**
** Exceptions:
**	none
**
** Example and Code Generation: (See IIiqset)
**
** Side Effects:
**
** History:
**	????
*/
i4
IIstfrs(i4 attr, char *name, i4 row, i4 isvar, i4 type, i4 length, char *tdata)
{
	return( IIstfsio( attr, name, row, (i2 *) 0, isvar, type, length,
			tdata ) );
}

/*{
** Name:	IIiqfsio	-	Inquire about an FRS attribute
**
** Description:
**
**	This routine is part of RUNTIME's external interface.
**	
** Inputs:
**	nullind
**	isvar
**	type
**	length
**	data
**	attr
**	name
**	row
**
** Outputs:
**
** Returns:
**	i4	TRUE
**		FALSE
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	26-feb-1987	Created from code extracted from IIiqfrs, and modified
**			for ADTs and NULLs (drh).
**	
*/

/* VARARGS4 */
i4
IIiqfsio(i2 *nullind, i4 isvar, i4 type, i4 length, void *data, i4 attr,
	const char *name, i4 row)
{
	char			workbuf[ 2*ER_MAX_LEN + 1 ];
	char			*workptr;
	i4			natval;
	DB_DATA_VALUE		dbv;
	DB_EMBEDDED_DATA	edv;
	char			*pobjname;
	char			*attrname;
	i4			argn;
	i4			*arg1 = NULL;
	i4			*arg2 = NULL;
	i4			RTcode;
	char			*objname;
	i4			(*func)();
	i1			attr_idtype;
	char			buf1[MAXFRSNAME + 1];
	char			tname[50];

	if (attr >= FsiATTR_MAX)
	{
		IIFDerror(SIATTR, 1, ERx("UNKNOWN"));
		return(FALSE);
	}

	if (!IILQgdata()->form_on)
	{
		/*
		** Only inquiries on the error number and the error
		** text are permitted when the FRS is not up.
		*/
		if (attr != FsiERRORNO && attr != FsiERRTXT)
		{
			IIFDerror(RTNOFRM, 0, NULL);
			return(FALSE);
		}
	}

	if (name != NULL)
	{
		objname = IIstrconv(II_CONV, name, buf1, MAXFRSNAME);
	}
	else
	{
		objname = ERx("");
	}
	pobjname = IIiqobjtype(IIsiobj);

	/* ensure that *arg1 is a valid location to store data (in IIsimobj) */
	if (IIsiobj == FsiFRS)
		arg1 = &argn;

	/*
	**  Match the object and attribute to make
	**  sure that the attribute belongs
	**  with the object.
	*/

	if (!IIsimobj(IIsiobj, attr, objname, row, &RTcode, &arg1, &arg2))
	{
		STcopy(ERget(IIfsiinfo[attr].name), tname);
		attrname = tname;
		IIFDerror(SIATTR, 1, attrname);
		return(FALSE);
	}

	/*
	**  Inquring on the state of a menuitem requires
	**  that the case of the name be preserved.
	*/
	if (RTcode == frsMURNME || RTcode == frsMUONOF)
	{
		if (name != NULL)
		{
			objname = IIstrconv(II_TRIM, name, buf1, MAXFRSNAME);
		}
	}

	/*
	**  Build a db_data_value to hold the value from the frs.
	**  The 'type' of the db_data_value can be determined from
	**  the attribute table.
	*/

	attr_idtype = IIfsiinfo[attr].idtype;
	switch(attr_idtype)
	{
		case fdFLOAT:
		case fdINT:
			dbv.db_datatype = DB_INT_TYPE;
			dbv.db_length = sizeof( i4  );
			dbv.db_prec = 0;
			dbv.db_data = (PTR) &natval;
			break;

		case fdSTRING:
			dbv.db_datatype = DB_CHR_TYPE;
			dbv.db_length = 2*ER_MAX_LEN;
			dbv.db_prec = 0;
			dbv.db_data = workbuf;
			break;

		default:
			return(FALSE);
	}

	switch(IIsiobj)
	{
		case FsiFRS:
			func = RTinqsys;
			break;

		case FsiFORM:
			func = RTinqfrm;
			break;

		case FsiFIELD:
			func = RTinqfld;
			break;

		case FsiTABLE:
			func = RTinqtbl;
			break;

		case FsiCOL:
			func = RTinqcol;
			break;

		case FsiROW:
			func = RTinqrow;
			break;

		case FsiRWCL:
			func = RTinqrc;
			break;

		case FsiMUOBJ:
			func = IIRTinqmu;
			break;

		default:
		{
			i4	tobj;

			tobj = IIsiobj;
			IIFDerror(E_FI1FF8_8184, 1, &tobj);
			return(FALSE);
		}
	}

	/*
	**  Call the routine to 'get' the frs attribute requested
	*/

	(*func)(arg1, arg2, RTcode, dbv.db_data );

	/*
	**  Build an embedded-data-value from the callers parameters,
	**  and convert the db-data-value holding the frs attribute
	**  into the caller's variable.
	*/

	/*
	**  Change to fdSTRING until everything gets
	**  converted to DB_*_TYPE standard so arthur
	**  can get running.
	if ( attr_idtype == DB_CHR_TYPE )
	*/
	if (attr_idtype == fdSTRING)
	{
		workptr = workbuf;
		dbv.db_length = STnlength( (i2) 2*ER_MAX_LEN+1, workptr );
	}

	edv.ed_type = type;
	edv.ed_length = length;
	edv.ed_data = data;
	edv.ed_null = nullind;

	if ( adh_dbcvtev( FEadfcb(), &dbv, &edv ) != OK )
	{
		STcopy(ERget(IIfsiinfo[attr].name), tname);
		attrname = tname;
		switch( attr_idtype)
		{
			case fdINT:
			case fdFLOAT:
				IIFDerror(SIATNUM, 1, attrname);
				break;

			case fdSTRING:
				IIFDerror(SIATCHAR, (i4) 1, attrname );
				break;
			default:
				IIFDerror(RTSIERR, 0);
				break;
		}
		return( FALSE);
	}
	else
	{
		return(TRUE);
	}
}


/*{
** Name:	IIsicpfcnv
**
** Description:
**	
**
** Inputs:
**	str		string to convert
**
** Outputs:
**	val		updated with the code value for the string
**
** Returns:
**	i4		TRUE
**			FALSE	if string could not be mapped to a code
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	????
*/
static i4
IIsicpfcnv(char *str, i4 *val)
{
	i4	retval = TRUE;
	i4	stlen;
	i4	pfnum;
	char	*cp;
	char	buf[4096];

	if (str == NULL || *str == EOS)
	{
		return(FALSE);
	}

	STcopy(str, buf);
	stlen = STlength(buf);
	CVlower(buf);

	if (buf[0] == 'p')
	{
		if (buf[1] == 'f')
		{
			cp = &(buf[2]);
			if (CVan(cp, &pfnum) != OK)
			{
				retval = FALSE;
			}
			else
			{
				if (pfnum < 1 || pfnum > Fsi_KEY_PF_MAX)
				{
					retval = FALSE;
				}
				else
				{
					*val = FsiPF_OFST + pfnum - 1;
				}
			}
		}
		else
		{
			retval = FALSE;
		}
	}
# ifndef	FT3270
	else if (buf[0] == 'c')
	{
		if (STbcompare(buf, stlen, ERx("control"), 7, TRUE) == 0)
		{
			cp = &(buf[7]);
			stlen = STlength(cp);
			if (stlen != 1 && stlen != 3)
			{
				retval = FALSE;
			}
			else
			{
				if (stlen == 3)
				{
					if (STcompare(cp, ERx("esc")) == 0)
					{
						*val = FsiESC_CTRL;
					}
					else if (STcompare(cp, ERx("del")) == 0)
					{
						*val = FsiDEL_CTRL;
					}
					else
					{
						retval = FALSE;
					}
				}
				else
				{
					switch(*cp)
					{
						case 'a':
							*val = FsiA_CTRL;
							break;

						case 'b':
							*val = FsiB_CTRL;
							break;

						case 'c':
							*val = FsiC_CTRL;
							break;

						case 'd':
							*val = FsiD_CTRL;
							break;

						case 'e':
							*val = FsiE_CTRL;
							break;

						case 'f':
							*val = FsiF_CTRL;
							break;

						case 'g':
							*val = FsiG_CTRL;
							break;

						case 'h':
							*val = FsiH_CTRL;
							break;

						case 'i':
							*val = FsiI_CTRL;
							break;

						case 'j':
							*val = FsiJ_CTRL;
							break;

						case 'k':
							*val = FsiK_CTRL;
							break;

						case 'l':
							*val = FsiL_CTRL;
							break;

						case 'm':
							*val = FsiM_CTRL;
							break;

						case 'n':
							*val = FsiN_CTRL;
							break;

						case 'o':
							*val = FsiO_CTRL;
							break;

						case 'p':
							*val = FsiP_CTRL;
							break;

						case 'q':
							*val = FsiQ_CTRL;
							break;

						case 'r':
							*val = FsiR_CTRL;
							break;

						case 's':
							*val = FsiS_CTRL;
							break;

						case 't':
							*val = FsiT_CTRL;
							break;

						case 'u':
							*val = FsiU_CTRL;
							break;

						case 'v':
							*val = FsiV_CTRL;
							break;

						case 'w':
							*val = FsiW_CTRL;
							break;

						case 'x':
							*val = FsiX_CTRL;
							break;

						case 'y':
							*val = FsiY_CTRL;
							break;

						case 'z':
							*val = FsiZ_CTRL;
							break;

						default:
							retval = FALSE;
							break;
					}
				}
			}
		}
		else
		{
			retval = FALSE;
		}
	}
# endif
	else
	{
		retval = FALSE;
	}

	return(retval);
}


/*{
** Name:	IIget_stkf	-	Get a stack frame
**
** Description:
**	
**
** Inputs:
**	argcnt
**	minargs
**	object
**	nml
**
** Outputs:
**
** Returns:
**	Ptr to a stack frame
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	????
*/
RUNFRM	*
IIget_stkf(i4 argcnt, i4 minargs, char *object, char **nml)
{
	RUNFRM	*stkf;

	if (argcnt < minargs)
	{
		/* ERROR */
		IIFDerror(SIARGS, 1, object);
		return ((RUNFRM *) NULL);
	}

	if ((stkf = RTgetfrm(*nml)) == NULL)
		IIFDerror(SIFRM, 1, *nml);

	return (stkf);
}

/*{
** Name:	IIset30mode - set form mode, Ingres 3.0 style
**
** Description:
**	contains function equivalent to code produced by eqc for
**		 set_frs form ("mode" = mode)
**	modified to support 'mode <mode>' as it worked in Ingres 3.0.
**
**	Should eqc change to generate new calls, this routine 
**	must change to match!!!	 Only difference should be the
**	TRUE in place of the normal FALSE in call to RTsetmode().
**
** Inputs:
**	mode	string containing name of mode to which the form
**		is to be set
** Outputs:
**	none
**
** Side Effects:
**	Screen is set up as if by display loop; default values
**	are re-displayed.
*/

void
IIset30mode(char *mode)
{
	RUNFRM	*runf;

	if (IIiqset(FsiFORM, 0, (char *)0) != 0)
	{
		if ((runf = RTgetfrm(NULL)) == NULL)
		{
			IIFDerror(SIFRM, 1, ERx(""));
			return;
		}
		RTsetmode(TRUE, runf, mode);

		/*
		**  If mode is fill or query then we'll mark all the
		**  fields with the changed bit since the all the (simple)
		**  field values are being changed anyway (either cleared
		**  or default values).
		*/
		if (*mode == 'f' || *mode == 'q')
		{
			FDschgbit(runf->fdrunfrm, 1, TRUE);
		}
		else
		{

			/*
			**  Just mark fields invalid instead of changed
			**  and invalid.  This will solve some problems
			**  with abf that performs mode changes liberally.
			*/
			IIFDcvbClearValidBit(runf->fdrunfrm);
		}
	}
}

/*{
** Name:	IIiqfrs
**
** Description:
**	This routine is a compatability cover for pre 6.0 EQUEL
**	programs.  It calls IIiqfsio.
**
**	
** Inputs:
**	isvar
**	type
**	length
**	data
**	attr
**	name
**	row
**
** Outputs:
**
** Returns:
**	i4	TRUE
**		FALSE
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	26-feb-1987	Converted to a compatability cover (drh)
**	
*/

i4
IIiqfrs(i4 isvar, i4 type, i4 length, PTR data, i4 attr, char *name, i4 row)
{
	return ( IIiqfsio( (i2 *) NULL, isvar, type, length, data, attr,
			name, row ) );
}



/*{
** Name:	IIgetrtcode - Transforms set/inquire to FRS attribute encoding.
**
** Description:
**	This routine simply transforms a set/inquire attribute encoding
**	to the equivalent FRS encoding.
**
** Inputs:
**	fsitype		Set and inquire attribute to be converted.
**
** Outputs:
**	None.
**
**	Returns:
**		frstype		Internal FRS attribute encoding.
**		0		If routine was passed an invalid s/i code.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	07/24/90 (dkh) - Initial version.
*/
i4
IIgetrtcode(i4 fsitype)
{
	i4	frstype;
	FSIINFO	*info;

	if (fsitype < 0 || fsitype >= FsiATTR_MAX)
	{
		IIFDerror(SIATTR, 1, ERx("UNKNOWN"));
		return(0);
	}
	info = &(IIfsiinfo[fsitype]);
	frstype = info->rtcode;
	return(frstype);
}
