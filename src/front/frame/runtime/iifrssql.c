/*
** Copyright (c) 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<cv.h>		/* 6-x_PC_80x86 */
#include	<er.h>
#include	<cm.h>
#include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<adf.h>
#include	<fe.h>
#include	<ft.h>
#include	<fmt.h>
#include	<frame.h>
#include	<menu.h>
#include	<runtime.h>
#include	<eqlang.h>
#include	<iisqlda.h>
#include	<frserrno.h>
# include	<lqgdata.h>
#include	"erfr.h"

/**
**  Name: iifrssql.c - Manages Dynamic SQL/FRS statements.
**
**  Description:
**
**	This files contains the two routines to handle the Dynamic FRS
**	DESCRIBE statement and USING clauses for ESQL/FORMS.
**
**  Defines:
**		IIFRsqDescribe - DESCRIBE a form/table into an SQLDA.
**		IIFRsqExecute  - Process the USING clause with an SQLDA.
**
**  History:
**	25-may-1988	- Created. (ncg)
**	21-dec-88 (bruceb)
**		Handle readonly fields/columns.
**	11/02/90 (dkh) - Replaced IILIBQgdata with IILQgdata().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/


/* External routines */
FUNC_EXTERN i4	IIgetfldio();
FUNC_EXTERN i4	IIputfldio();
FUNC_EXTERN i4	IItcogetio();
FUNC_EXTERN i4	IItcoputio();

FUNC_EXTERN TBSTRUCT	*RTgettbl();

FUNC_EXTERN bool	IIsqdSetSqlvar();	/* In LIBQ */

/* Definitions for modes of DESCRIBE */
# define	SQ_MD_ALL	0
# define	SQ_MD_QRY	1
# define	SQ_MD_UPD	2


/*{
**  Name: IIFRsqDescribe - Describe a form or table field into an SQLDA.
**
**  Description:
**	Given as input an FRS object and a DESCRIBE mode this routine
**	fills the caller's SQLDA with object name and type descriptions.
**	The specified object (form or table field) is found in the FRS
**	and its components are traversed and described to the caller.
**	If the object is not found an error is issued.  The component
**	traversal is done like FORMDATA and TABLEDATA, except that no
**	external objects need to be flagged to control the loop.  The
**	rules for filling the SQLDA are very much like the rules applied
**	by IIsqDescribe (SQL DESCRIBE).
**
**	The names assigned to the SQLDA are the internal names used to
**	name a field or column in an application.  Hidden table field
**	columns are not included.
**
**	The type descriptions are the internal FRS ADF descriptions
**	mapped to external types and lengths, as specified by the Dynamic 
**	SQL DESCRIBE statement:
**
**	1. Length information is converted from the internal representation
**	   to a user representation:
**
**	    Nullable types	   - Subtract 1 byte (length of null byte),
**	    Varying-length strings - Subtract length of count field,
**	    DATE, MONEY		   - Set length to zero.
**
**	2. Some data types are mapped to external SQL data types:
**
**	    Varying Length Char (TXT, LTXT, VCH) - VCH
**	    Fixed Length (CHR, CHA)		 - CHA
**	    Table field				 - TFLD
**
**	The updating of the SQLDA and the inspection of the types is done
**	by the LIBQ routine IIsqdSetSqlvar.
**
**  Inputs:
**	lang		- Language constant for possible SQLDA details.
**	is_form		- Identifies the object being described:
**			  	TRUE	- Form,
**			  	FALSE	- Table field.
**	form_name	- Null-terminated form name.
**	table_name	- Null terminated table field name. If is_form
**			  is TRUE, this argument is ignored (and can
**			  be null).  If is_form is FALSE, this name
**			  must be specified.
**	mode		- Null terminated mode string.  If the mode is blank,
**			  empty or null then the default "ALL" is assumed.
**			  	ALL 	- Describe ALL components,
**				QUERY	- Describe updatable and query-only
**					  fields.
**				UPDATE	- Describe updatable fields.
**			  Only the first letter of the mode is checked.
**			  If the form is currently displayed, the DISPLAY
**			  mode does not effect the DESCRIBE mode results.
**	sqd		- Pointer to caller's SQLDA to fill.
**	    .sqln		Number of "sqlvar" structures within the SQLDA
**				allocated by program.  If sqln < the number 
**				of objects to be described from the FRS then
**				this routine fills in sqln column descriptors
**				in the SQLDA.
**
**  Outputs:
**	sqd		- The SQLDA is filled with the description of the
**			  object as stored in the FRS.
**	    .sqld		The number of objects described.  This may be
**				zero if there are none.  For example, if all
**				fields in a form are read-only then describing
**				the form in UPDATE mode will return zero
**				fields.  If a statement-fatal error occurs
**				this will also be set to zero.
**	    .sqlvar[sqld]	Sqld elements of the array are filled.
**		.sqltype	External data type id of column.
**	    	.sqllen		Length of data in column. (Description above).
**	    	.sqlname	Field/column internal name information
**		    .sqlnamel	Length of internal name.
**	    	    .sqlnamec	Internal name (no EOS termination).
**				Name is blank padded.
**
**	Returns:
**	    VOID - Nothing
**
**	Errors:
**	    RTNOFRM			- No FORMS statement yet.
**	    RTFRIN			- Form not initialized.
**	    TBBAD			- Table not found in form.
**	    E_FR0100_SQLDA_NULL		- FRS SQLDA pointer is null.
**	    E_FR0101_SQ_FORM_TABLE_BAD	- Invalid form or table name.
**	    E_FR0102_SQ_MODE_BAD	- Illegal mode used.
**
**  Preprocessor generated code:
**
**	EXEC FRS DESCRIBE FORM f1 UPDATE INTO :sqlda;
**
**	    IIFRsqDescribe(EQ_C, TRUE, "f1", (char *)0, "u", sqlda);
**
**	EXEC FRS DESCRIBE TABLE f1 :tab_name INTO :sqlda;
**
**	    IIFRsqDescribe(EQ_C, FALSE, "f1", tab_name, (char *)0, sqlda);
**
**  Pseudo Code:
**
**	validate form system and input arguments (sqlda, mode);
**	sqld = 0;
**	if (form)
**	    find form;
**	    for each field based on mode loop
**		check for table type;
**		assign sqlvar name, type and length and update sqld;
**	    endloop;
**	else
**	    find table;
**	    for each column based on mode loop
**		assign sqlvar name, type and length and update sqld;
**	    endloop;
**	endif;
**
**  Side Effects:
**	None - Unlike FORMDATA there are no "current" field or column
**	markers set in the FRS internal data structures.
**	
**  History:
**	25-may-1988	- Created. (ncg)
*/

VOID
IIFRsqDescribe(i4 lang, i4 is_form, char *form_name, char *table_name, char *mode, IISQLDA *sqd)
{
    RUNFRM		*rfrm;			/* Form object */
    FRAME		*frm;
    char		fname[FE_MAXNAME+1];
    TBSTRUCT		*tbl;			/* Table object */
    char		tname[FE_MAXNAME+1];
    FIELD		*fld;			/* Field object */
    FIELD		**fldlist;		/* Field lists */
    i4			numlists;		/* # of fld lists to process */
    i4			limit;			/* # of fields in a list */
    FLDCOL		*fcol;			/* Column of table field */
    i4			md;			/* Local mode */
    DB_DATA_VALUE	dbv;			/* For type processing */
    i4			i;			/* Index */

    if (!IILQgdata()->form_on)			/* Validate form system on */
    {
	IIFDerror(RTNOFRM, 0, (char *)0);
	return;
    }

    if (sqd == (IISQLDA *)0)		/* Validate SQLDA */
    {
	IIFDerror(E_FR0100_SQLDA_NULL, 0, (char *)0);
	return;
    }
    else
    {
	/* Assign initial id */
	STmove(ERx("SQLDA"), ' ', 8, sqd->sqldaid);
        sqd->sqldabc = sizeof(IISQLDA) +
		       (sizeof(sqd->sqlvar[0]) * (sqd->sqln-1));
	sqd->sqld = 0;
    }

    if (mode == (char *)0)			/* Validate mode */
    {
	md = SQ_MD_ALL;
    }
    else
    {
	switch (*mode)
	{
	  case 'a':
	  case 'A':
	    md = SQ_MD_ALL;
	    break;
	  case 'q':
	  case 'Q':
	    md = SQ_MD_QRY;
	    break;
	  case 'u':
	  case 'U':
	    md = SQ_MD_UPD;
	    break;
	  default:
	    IIFDerror(E_FR0102_SQ_MODE_BAD, 1, mode);
	    return;
	}
    } 

    /* Validate form name and get form descriptor */
    if (form_name == (char *)0)
    {
	IIFDerror(E_FR0101_SQ_FORM_TABLE_BAD, 0, (char *)0);
	return;
    }
    _VOID_ IIstrconv(II_CONV, form_name, fname, FE_MAXNAME);
    if ((rfrm = RTfindfrm(fname)) == (RUNFRM *)0)
    {
	IIFDerror(RTFRIN, 1, fname);
	return;
    }

    /* First handle the description of a form */
    if (is_form)
    {
	frm = rfrm->fdrunfrm;

	if (md == SQ_MD_ALL)
	    numlists = 2;				/* Both lists */
	else 
	    numlists = 1;				/* Only frfld */

	/* Process field lists for type and name information */
	for (fldlist = frm->frfld, limit = frm->frfldno;
	     numlists;
	     fldlist = frm->frnsfld, limit = frm->frnsno, numlists--)
	{
	    for (i = 0; i < limit; i++)
	    {
		fld = fldlist[i];

		/* If not requesting all fields then skip readonly */
		if ((fld->fld_var.flregfld->flhdr.fhd2flags & fdREADONLY)
		    && (md != SQ_MD_ALL))
		{
		    continue;
		}
		/* If requesting only update-able then skip query only */
		if (   (fld->fld_var.flregfld->flhdr.fhdflags & fdQUERYONLY)
		    && (md == SQ_MD_UPD))
		{
		    continue;
		}
		/* Describe the table in all modes */
		if (fld->fltag == FTABLE)
		{
		    dbv.db_datatype = DB_TFLD_TYPE;
		    dbv.db_length   = 0;
		    dbv.db_prec	= 0;
		}
		else
		{
		    dbv.db_datatype = fld->fld_var.flregfld->fltype.ftdatatype;
		    dbv.db_length   = fld->fld_var.flregfld->fltype.ftlength;
		    dbv.db_prec	    = fld->fld_var.flregfld->fltype.ftprec;
		}
		_VOID_ IIsqdSetSqlvar(lang, sqd, FALSE, FDGFName(fld), &dbv);
	    }	/* for all fields */
	} /* for all field lists */

	return;				/* Done with form */
    } /* If is form */

    /* Must be a table field, process the name and get the description */
    if (table_name == (char *)0)
    {
	IIFDerror(E_FR0101_SQ_FORM_TABLE_BAD, 0, (char *)0);
	return;
    }
    _VOID_ IIstrconv(II_CONV, table_name, tname, FE_MAXNAME);
    if ((tbl = RTgettbl(rfrm, tname)) == (TBSTRUCT *)0)
    {
	IIFDerror(TBBAD, 2, tname, fname);
	return;
    }

    /* Process all the display columns in the table field */
    for (i = 0; i < tbl->tb_fld->tfcols; i++)
    {
	fcol = tbl->tb_fld->tfflds[i];
	/* Check modes against DESCRIBE mode */
	if (   ((fcol->flhdr.fhdflags & fdtfCOLREAD) && (md != SQ_MD_ALL))
	    || ((fcol->flhdr.fhd2flags & fdREADONLY) && (md != SQ_MD_ALL))
	    || ((fcol->flhdr.fhdflags & fdQUERYONLY) && (md == SQ_MD_UPD))
	   )
	{
	    continue;
	}
	dbv.db_datatype = fcol->fltype.ftdatatype;
	dbv.db_length   = fcol->fltype.ftlength;
	dbv.db_prec	= fcol->fltype.ftprec;
	_VOID_ IIsqdSetSqlvar(lang, sqd, FALSE, FDGCName(fcol), &dbv);
    } /* for all columns */
} /* IIFRsqDescribe */


/*{
**  Name: IIFRsqExecute - Execute a target list of an FRS I/O statement.
**
**  Description:
**	An FRS I/O statement is executed with an SQLDA.  The preprocessor
**	has already generated control flow to manage the actual statement.
**	This routine provides an interface between the SQLDA and the I/O
**	call that would have been generated for the particular statement.
**
**	The input SQLDA may have been described via the FRS DESCRIBE 
**	statement and assigned result/input variables by the caller.
**	The SQLDA types and names may also have been hard coded by the
**	caller.  Determine whether this is a form or table field 
**	operation, and whether this is an input or output call in order
**	to know who to call when the data is processed.  Traverse the
**	SQLDA and for each object call the appropriate routine with the
**	object name, the null indicator, the I/O variable description and
**	address.  The traversal and type conversion is very similar to 
**	the IIsqDaIn routine of Dynamic SQL, except that the names
**	associated with the objects must be passed to the FRS I/O routine.
**
**	Each variable referenced by the SQLDA must have a legal type 
**	identifier and length, some associated data and, if the type
**	is nullable, a null indicator.  This routine loops through 
**	all the the variables pointed at by the sqlda, verifying that the 
**	variable is legal.  Some verification (ie, legal type conversions)
**	are done by the FRS I/O routine that is called. 
**
**	Note that the absolute value of the type is sent to the I/O routine
**	as null types depend on the presence of a null indicator.
**
**	If an error is found at any time during the loop, such as
**	a missing variable or null indicator, the statement is not
**	terminated.  (This follows the same logic as the static case.)
**
**	Unlike the FRS DESCRIBE statement, table field hidden column
**	names and internal state variables may be referenced.  These
**	are not checked by this routine.
**
**	When traversing the objects, some special name and type rules
**	are applied:
**
**	1. If the name begins with "gop(" then the GETOPER interface
**   	   is dynamically generated.
**	2. If the name begins with "pop(" then the PUTOPER interface
**  	   is dynamically generated.  The PUTOPER interface is an
**	   internal/undocumented feature.
**	3. Names read from sqlname are copied and null terminated
**	   before calling the I/O routine.
**	4. Fixed length character types are mapped by this routine to
**	   allow for the different language semantics:
**	    Input:
**	      If language is C map CHA to CHR type and always give length zero
**	      as the semantics are that this must be null terminated.
**	      If language is not C map CHR to CHA type (indicates not null
**	      terminated later on).
**	    Output:
**	      If language is C map CHA to CHR type (leave length).
**	      If language is not C map CHR to CHA type.
**	5. For varying-length character types add the count length before
**	   calling the I/O routine.
**
**  Inputs:
**	lang		- Language constant for SQLDA details.
**	is_form		- Identifies the object owner of the I/O target list:
**			  	TRUE  - Form,
**			  	FALSE - Table field.
**	is_input	- TRUE, if this is input, FALSE if output.  Based
**			  on the value of is_form and is_input the
**			  I/O routine is determined:
**			    TRUE, TRUE		- IIputfldio
**			    TRUE, FALSE		- IIgetfldio
**			    FALSE, TRUE		- IItcoputio
**			    FALSE, FALSE	- IItcogetio
**			  The different I/O calls all have the same input
**			  format.
**	sqd		- Pointer to caller's SQLDA to use.
**	    .sqln		Number of sqlvar elements.
**	    .sqld	  	The number of pertinent sqlvar elements
**				to use for input or output.  Use:
**					min(sqld, sqln)
**	    .sqlvar[sqld]	Array of sqlvar elements to use for input,
**				or to fill on output.
** 	        .sqltype	Type id of input/result variable.
** 	        .sqllen		Length of input/result variable.
**	        .sqldata	If input, address of input variable to assign
**				to FRS.
**	        .sqlind		If input, address of null indicator to send
**				to FRS (or null).
**	    	.sqlname	Field/column internal name information
**		    .sqlnamel	Length of internal name.
**	    	    .sqlnamec	Internal name.
**
**  Outputs:
**	sqd		- If an output target list then some fields of sqlvar
**			  will be assigned into the SQLDA from the I/O routine.
**	    .sqlvar[sqld]	Array of sqlvar elements to fill on output.
**	        .sqldata	If output, address of output variable to
**				retrieve from FRS.
**	        .sqlind		If output, address of null indicator to
**				retrieve from FRS (or null).
**	Returns:
**	    VOID - Nothing
**
**	Errors:
**	    E_FR0100_SQLDA_NULL		- FRS SQLDA pointer is null.
**	    E_FR0103_SQ_NAME_BAD	- Illegal object name.
**	    E_FR0104_SQ_NULL_IND	- Nullable type, but no null indicator.
**	    E_FR0105_SQ_NULL_DATA	- Null data pointer.
**	    Errors reported within the I/O call.
**
**  Preprocessor generated code:
**
**	EXEC FRS PUTFORM :f USING :sqlda;
**
**	    if (IIfsetio(f) != 0) {
**		IIFRsqExecute(EQ_C, TRUE, TRUE, sqlda);
**	    }
**
**	EXEC FRS GETFORM USING :sqlda;
**
**	    if (IIfsetio((char *)0) != 0) {
**		IIFRsqExecute(EQ_C, TRUE, FALSE, sqlda);
**	    }
**
**	EXEC FRS PUTROW :f :t :r USING :sqlda;
**
**	    if (IItbsetio(cmPUTR, f, t, r) != 0) {
**		IIFRsqExecute(EQ_C, FALSE, TRUE, sqlda);
**	    }
**
**	EXEC FRS UNLOADTABLE :f :t USING :sqlda;
**	EXEC FRS BEGIN;
**	    host code;
**	EXEC FRS END;
**
**	    if (IItbact(f, t, UNLOAD) != 0) {
**		while (IItunload() != 0) {
**		    IIFRsqExecute(EQ_C, FALSE, FALSE, sqlda);
**		}
**	    }
**	    IItunend();
**
**  Pseudo Code:
**
**	point at correct i/o routine;
**	for each sqlvar in sqlda loop
**	    pick off indicator, type, length and process for special cases;
**	    pick off name;
**	    if ("pop" and input) or ("gop" and output)
**		set query_function;
**		adjust name;
**	    else
**	        query_function = null;
**	    endif;
**	    if (query_func)
**		query_func(TRUE);
**	    endif;
**	    call i/o routine with name, and i/o data;
**	endloop;
**
**  Side Effects:
**	None
**
**  History:
**	25-may-1988	- Created. (ncg)
*/

VOID
IIFRsqExecute(i4 lang, i4 is_form, i4 is_input, IISQLDA *sqd)
{
    i4			(*routine_io)();	/* I/O routine to call */
    i4			(*query_func)(i4);	/* Extra function - currently
						** only for Query mode */
    struct sqvar	*sqv;			/* User sqlvar */
    i4		col;			/* Index into sqlvar */
    i4			sqlen, sqtype;		/* Mapping to local */
    i2			*sqind;			/* Indicator variable */
    PTR			sqdata;			/* Data variable */
    char		nmbuf[FE_MAXNAME+5+1];	/* Name + "gop()" + null */
    char		*sqname, *nmend;	/* Start and end of name */
    i4			numsq;			/* # of elements to process */

    /* Note, call sequence is different for input vs output, cast to
    ** generic i4 * function pointer.
    */
    if (is_form)
	routine_io = is_input ? ((i4 (*)()) IIputfldio) : ((i4 (*)()) IIgetfldio);
    else
	routine_io = is_input ? ((i4 (*)()) IItcoputio) : ((i4 (*)()) IItcogetio);
	
    if (sqd == (IISQLDA *)0)
    {
	IIFDerror(E_FR0100_SQLDA_NULL, 0, (char *)0);
	return;
    }

    numsq = min(sqd->sqln, sqd->sqld);		/* sqld should be <= sqln */
    for (col = 0, sqv = &sqd->sqlvar[0]; col < numsq; col++, sqv++)
    {
  	if (sqv->sqltype < 0)			/* Indicator required */
	{
	    if ((sqind = sqv->sqlind) == (i2 *)0)
	    {
		IIFDerror(E_FR0104_SQ_NULL_IND, 1, &col);
		continue;
	    }
 	}
	else
	{
	    sqind = (i2 *)0;
	}
	if ((sqdata = sqv->sqldata) == (PTR)0)	/* Validate data */
	{
	    IIFDerror(E_FR0105_SQ_NULL_DATA, 1, &col);
	    continue;
	}

	sqtype = abs(sqv->sqltype);		/* Defaults */
	sqlen = sqv->sqllen;

	/*
	** Process character data as correct host language data type.
	** See rules in interface comment above.
	*/
	if (lang != EQ_C && sqtype == DB_CHR_TYPE)
	{
	    sqtype = DB_CHA_TYPE;
	}
	else if (   lang == EQ_C
		 && (sqtype == DB_CHA_TYPE || sqtype == DB_CHR_TYPE))
	{
	    sqtype = DB_CHR_TYPE;
	    if (is_input)
		sqlen = 0;
	}
	else if (sqtype == DB_VCH_TYPE)
	{
	    sqlen += DB_CNTSIZE;		/* Add in .db_t_count field */
	}

	if (sqv->sqlname.sqlnamel <= 0)		/* Verify user length is ok */
	{
	    CVna(sqv->sqlname.sqlnamel, nmbuf);
	    IIFDerror(E_FR0103_SQ_NAME_BAD, 2, nmbuf, &col);
	    continue;
	}

	_VOID_ STlcopy(sqv->sqlname.sqlnamec, nmbuf,
			min(sqv->sqlname.sqlnamel, FE_MAXNAME));
	CVlower(nmbuf);
	/*
	** Copy the name and lowercase it.  Check for GOP(name) and
	** POP(name) format.  If found skip it and point at the name.
	*/
	if (sqname = STindex(nmbuf, ERx("("), 0))
	{
	    *sqname++ = EOS;
	    if (   (   (is_input  && STcompare(ERx("pop"), nmbuf) == 0)
	            || (!is_input && STcompare(ERx("gop"), nmbuf) == 0)
		   )
		&& (nmend = STindex(sqname, ERx(")"), 0))
	       )
	    {
		*nmend = EOS;
		query_func = is_input ? IIputoper : IIgetoper;
	    }
	    else
	    {
		*--sqname = '(';
		IIFDerror(E_FR0103_SQ_NAME_BAD, 2, nmbuf, &col);
		continue;
	    }
	}
	else					/* No parens */
	{
	    sqname = nmbuf;
	    query_func = NULL;
	}
	if (query_func)				/* Putoper/Getoper */
	    _VOID_ (*query_func)(TRUE);
	if (is_input)
	    _VOID_ (*routine_io)(sqname, sqind, TRUE, sqtype, sqlen, sqdata);
	else
	    _VOID_ (*routine_io)(sqind, TRUE, sqtype, sqlen, sqdata, sqname);
   } /* for each sqlvar */
} /* IIFRsqExecute */
