/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<cm.h>
# include	<er.h>
# include	<si.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<eqrun.h>
# include	<equel.h>
# include	<eqstmt.h>
# include	<fsicnsts.h>
# include	<eqfrs.h>
# include	<ereq.h>

/*
+* Filename:	eqfrs.c
** Purpose:	Interpret FRS names and statements.
** Defines:	
**		frs_inqset	- Set global mode (SET or INQUIRE).
**		frs_object	- Initialize object-type.
**		frs_parentname	- Add parent-names.
**		frsck_constant	- Check and return values for frs-constant.
**		frs_iqsave	- Save reference for INQUIRE.
**		frsck_valmap	- Check and return values for map-constant.
**		frsck_mode	- Check the validity of object mode.
**		frsck_key	- Check and return values for frs-key.
** Locals:
**		frs_getword	- Find a pseudo-keyword.
**	
** History:	05-aug-1986	- Abstracted interpretive routines and moved
**				  generation routines to "eqgenfrs.c". (jhw)
**		28-aug-1985	- Written to support the new format of the
**				  INQUIRE/SET FRS statements. (ncg)
**		04-dec-1985	- Added COLO[U]R and ACTIVATE (ncg)
**		22-may-1988	- Added VENUS support  (marge)
**		16-jun-1988	- Bug fixed: switched rows, columns, in 
**				  frs__constab.  (marge)
**		07-jul-1988	- Added ERRTXT to frs__constab.  (marge)
**		05-may-1989	- Added support for entry activation 
**				  by adding BEFORE to frs__actcoms(teresa)
**		03-jul-1989	- Added support for shell frs command by adding
**				  SHELL to the map and label objects table
**				  and to the frs constants table. (teresa)
**		07-aug-1989	- Add support for mapping of arrow keys.
**				  (teresa)	
**		10-aug-1989	- Add support for inquiring of last_frskey.
**				  (teresa)
**		07-nov-1989	- Add support for enabling/disabling editor and
**				  inquiring on derived fields. (teresa)
**		02-feb-1990	- Add error message suppression, i.e.,  new frs 
**				  constant: getmessages. (teresa)
**		09-aug-1990 (barbara)
**		    Changes for supporting per value display attributes
**		    in a table field.  1) The WITH clause on LOADTABLE/INSERTROW
**		    now uses this module.  2) Setting/Inquiring on FRS-constant
**		    values of ROW object type may use a character object name
**		    (ie, a column name).  This allows setting one cell in a
**		    given row.
**		05-aug-1991 (teresal)
**		    Fix FRS constants table for "derived" and 
**		    "derivation_string" so that an object name is
**		    optional and not required. Bug 38905.
**		07-oct-1992 (lan)
**		    Added FRS object type menu to table frs__objtab and
**		    FRS constants rename, active, exists, outofdatamessage
**		    to table frs__consttab.
**		03-dec-1992 (lan)
**		    Fixed a bug where the use of a variable for object-name
**		    in the statement "set_frs menu..." was not accepted.
**		08-jan-1993 (lan)
**		    Added FRS constants decimal_scale and decimal_precision
**		    to table frs__consttab.
**		14-jan-1993 (lan)
**		    Added FRS constant decimal_format to table frs__consttab.
**		24-feb-1993 (sylviap)
**		    Added FRS constant inputmasking to table frs__consttab.
**              12-Dec-95 (fanra01)
**                  Added definitions for extracting data for DLLs on windows
**                  NT
**              06-Feb-96 (fanra01)
**                  Made data extraction generic.
**		14-mar-1996 (thoda04)
**		    Added function prototype for static frs_getword().
**		    Upgraded frsck_constant() to ANSI style func proto for 
**		    Windows 3.1.
**		12-Jun-1996 (mckba02)
**		    Changed function frsck_constant, #73178.
**		18-dec-1996 (chech02)
**		    Fixed redefined symbols error: frs.
**
** Notes:
**
** 1. These routines are the interpretive part of the "EQFRS" module.  Code
**    generation routines are in "eqgenfrs.c".
**
** 2. Naming conventions used in this file are described by the following syntax
**    and example:
**
**	INQUIRE_FRS object-type [parent-names(s)] 
**		(var = frs-const[(object-name)], ...)
**	SET_FRS object-type [parent-names(s)] 
**		(frs-const[(object-name)] = var_or_val, ...)
**	Example:
**		INQUIRE_FRS field form1 (var = format(field1))
**		SET_FRS frs (menumap = 1)
**
**	0.  The sequence "INQUIRE_FRS field form1" is a header (frs_head()).
** 	1.  The occurance of the word "field" is an FRS object-type (FRS_OBJ).
**	2.  "form1" is an object parent-name (FRS_NAME).
** 	3.  The occurance of the word "format" is an FRS constant (FRS_CONST).
**	4.  "field1" is an object-name (FRS_NAME).
**
** 3. New WITH clause syntax handled through these routines:
**
**	[WITH (ATTR(colname) = ATTR_VAL, {ATTR(colname) = ATTR_VAL})];
**
**    This syntax may appear on LOADTABLE and INSERTROW statements.
**    Example:
**	LOADTABLE form1 table1 (col1 = val) WITH (reverse(col1) = 1);
**
**	0.  The implied object type is column
**	1.  No parent-names on WITH clause (run-time has this info from main
**	    part of the LOADTABLE/INSERTROW statement).
**	2.  ATTR (e.g. 'reverse')  is an FRS constant
**	3.  'colname' (e.g. col1)  is an object-name.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
GLOBALREF FRS__	frs;


/*
** frs__objtab[] - Table of master FRS objects-type. Order in terms of most 
**		   common objects, because of linear search.
*/
static FRS_OBJ	frs__objtab[] = {
    /* Name			Define		Args	Flag	    Old */
    {ERx("field"),		FsiFIELD,	1,	_fFLD,	    TRUE},
    {ERx("form"),		FsiFORM,	0,	_fFRM,	    TRUE},
    {ERx("frame"),		FsiFORM,	0,	_fFRM,	    TRUE},
    {ERx("table"),		FsiTABLE,	1,	_fTAB,	    TRUE},
    {ERx("column"),		FsiCOL,		2,	_fCOL,	    TRUE},
    {ERx("frs"),		FsiFRS,		0,	_fFRS,	    FALSE},
    {ERx("row"),		FsiROW,		2,	_fROW,	    FALSE},
    {ERx("row_column"),		FsiRWCL,	2,	_fRCL,	    FALSE},
    {ERx("menu"),		FsiMUOBJ,	1,	_fMEN,	    FALSE},
    {(char *)0,		0,		0,	0,	    FALSE}
};


/* frs__consttab[] - Table of FRS constants */
static FRS_CONST	frs__consttab[] = {
  /* Name		Define	     {Inqtyp, Settyp}	Obj-name  Flags */
  {ERx("terminal"),	FsiTERMINAL, {T_CHAR, T_NONE},	_fOBNUL, _fFRS},
  {ERx("errorno"),	FsiERRORNO,  {T_INT,  T_NONE},	_fOBNUL, _fFRS},
  {ERx("errortext"),	FsiERRTXT,   {T_CHAR, T_NONE},	_fOBNUL, _fFRS},
  {ERx("validate"),	FsiVALDATE,  {T_INT,  T_INT},	_fOBREQ, _fFRS},
  {ERx("activate"),	FsiACTIVATE, {T_INT,  T_INT},	_fOBREQ, _fFRS},
  {ERx("menumap"),	FsiMENUMAP,  {T_INT,  T_INT},	_fOBNUL, _fFRS},
  {ERx("mapfile"),	FsiMAPFILE,  {T_CHAR, T_CHAR},	_fOBNUL, _fFRS},
  {ERx("pcstyle"),	FsiPCSTYLE,  {T_INT,  T_INT},	_fOBNUL, _fFRS},
  {ERx("statusline"),FsiSTATLN,   {T_INT,  T_INT},	_fOBNUL, _fFRS},
  {ERx("map"),	FsiMAP,      {T_CHAR, T_UNDEF},	_fOBREQ, _fFRS}, /* See later */
  {ERx("label"),	FsiLABEL,    {T_CHAR, T_CHAR},	_fOBREQ, _fFRS},
  {ERx("command"),	FsiLCMD,     {T_INT,  T_NONE},  _fOBNUL, _fFRS},
  {ERx("timeout"),	FsiTIMEOUT,  {T_INT,  T_INT},	_fOBNUL, _fFRS},
  {ERx("normal"),	FsiNORMAL,   {T_INT,  T_INT}, _fOBOPT, _fFLD|_fCOL|_fROW|_fRCL},
  {ERx("reverse"),	FsiRVVID,    {T_INT,  T_INT}, _fOBOPT, _fFLD|_fCOL|_fROW|_fRCL},
  {ERx("blink"),	FsiBLINK,    {T_INT,  T_INT}, _fOBOPT, _fFLD|_fCOL|_fROW|_fRCL},
  {ERx("underline"),	FsiUNLN,     {T_INT,  T_INT}, _fOBOPT, _fFLD|_fCOL|_fROW|_fRCL},
  {ERx("intensity"),	FsiCHGINT,   {T_INT,  T_INT}, _fOBOPT, _fFLD|_fCOL|_fROW|_fRCL},
  {ERx("color"),	FsiCOLOR,    {T_INT,  T_INT}, _fOBOPT, _fFLD|_fCOL|_fROW|_fRCL},
  {ERx("colour"),	FsiCOLOR,    {T_INT,  T_INT}, _fOBOPT, _fFLD|_fCOL|_fROW|_fRCL},
  {ERx("change"),	FsiCHG,      {T_INT,  T_INT}, _fOBOPT, _fFRM|_fFLD|_fROW|_fRCL},
  {ERx("mode"),	FsiFMODE,    {T_CHAR, T_CHAR}, _fOBOPT,_fFRM|_fTAB|_fFLD|_fCOL},
  {ERx("name"),	FsiNAME,     {T_CHAR, T_NONE}, _fOBOPT,_fFRM|_fTAB|_fFLD|_fCOL},
  {ERx("field"),	FsiFLD,      {T_CHAR, T_NONE},	_fOBOPT, _fFRM},
  {ERx("number"),	FsiNUMBER,   {T_INT,  T_NONE},	_fOBOPT, _fFLD|_fCOL},
  {ERx("length"),	FsiLENGTH,   {T_INT,  T_NONE},	_fOBOPT, _fFLD|_fCOL},
  {ERx("type"),		FsiTYPE,     {T_INT,  T_NONE},	_fOBOPT, _fFLD|_fCOL},
  {ERx("datatype"),	FsiDATYPE,   {T_INT,  T_NONE},	_fOBOPT, _fFLD|_fCOL},
  {ERx("format"),	FsiFORMAT,   {T_CHAR, T_CHAR},	_fOBOPT, _fFLD|_fCOL},
  {ERx("valid"),	FsiVALID,    {T_CHAR, T_NONE},	_fOBOPT, _fFLD|_fCOL},
  {ERx("table"),	FsiFLDTBL,   {T_INT,  T_NONE},	_fOBOPT, _fFLD},
  {ERx("rowno"),	FsiROWNO,    {T_INT,  T_NONE},	_fOBOPT, _fTAB},
  {ERx("maxrow"),	FsiMAXROW,   {T_INT,  T_NONE},	_fOBOPT, _fTAB},
  {ERx("lastrow"),	FsiLASTROW,  {T_INT,  T_NONE},	_fOBOPT, _fTAB},
  {ERx("datarows"),	FsiDATAROWS, {T_INT,  T_NONE},	_fOBOPT, _fTAB},
  {ERx("maxcol"),	FsiMAXCOL,   {T_INT,  T_NONE},	_fOBOPT, _fTAB},
  {ERx("column"),	FsiTBLCOL,   {T_CHAR, T_NONE},	_fOBOPT, _fTAB},
  {ERx("startrow"),	FsiITSROW,   {T_INT,  T_NONE},	_fOBOPT, _fFLD|_fROW},
  {ERx("startcolumn"),	FsiITSCOL,   {T_INT,  T_NONE},	_fOBOPT, _fFLD|_fROW},
  {ERx("rows"),		FsiITHEIGHT, {T_INT,  T_NONE},	_fOBOPT, _fFLD|_fCOL|_fFRS|_fFRM},
  {ERx("columns"),	FsiITWIDTH,  {T_INT,  T_NONE},	_fOBOPT, _fFLD|_fCOL|_fFRS|_fFRM},
  {ERx("cursorrow"),	FsiCROW,     {T_INT,  T_NONE},	_fOBOPT, _fFRS},
  {ERx("cursorcolumn"),	FsiCCOL,     {T_INT,  T_NONE},	_fOBOPT, _fFRS},
  {ERx("displayonly"),	FsiRDONLY,   {T_INT,  T_INT},	_fOBOPT, _fFLD|_fCOL},
  {ERx("invisible"),	FsiINVIS,    {T_INT,  T_INT},	_fOBOPT, _fFLD|_fCOL},
  {ERx("shell"),	FsiSHLKEY,   {T_INT,  T_INT},	_fOBNUL, _fFRS},
  {ERx("last_frskey"),	FsiLFRSKEY,  {T_INT,  T_NONE},	_fOBNUL, _fFRS},
  {ERx("editor"),	FsiEDITKEY,  {T_INT,  T_INT},	_fOBNUL, _fFRS},
  {ERx("derived"),	FsiDRVED,    {T_INT,  T_NONE},	_fOBOPT, _fFLD|_fCOL},
  {ERx("derivation_string"),FsiDRVSTR,{T_CHAR,T_NONE},	_fOBOPT, _fFLD|_fCOL},
  {ERx("getmessages"),	FsiGMSGS,    {T_INT,  T_INT},	_fOBNUL, _fFRS},
  {ERx("rename"),	FsiMURENAME, {T_CHAR, T_CHAR},	_fOBREQ, _fMEN},
  {ERx("active"),	FsiMUACTIVE, {T_INT,  T_INT},	_fOBREQ, _fMEN},
  {ERx("exists"),	FsiEXISTS,   {T_INT,  T_NONE},	_fOBREQ, _fFRM|_fFLD|_fCOL},
  {ERx("outofdatamessage"),FsiODMSG, {T_INT,  T_INT},	_fOBNUL, _fFRS},
  {ERx("decimal_precision"),FsiDEC_PREC,{T_INT,  T_INT},_fOBREQ, _fFLD|_fCOL},
  {ERx("decimal_scale"),FsiDEC_SCL,  {T_INT,  T_INT},	_fOBREQ, _fFLD|_fCOL},
  {ERx("decimal_format"),FsiDEC,     {T_INT,  T_INT},	_fOBREQ, _fFLD|_fCOL},
  {ERx("inputmasking"), FsiEMASK,    {T_INT,  T_INT},	_fOBREQ, _fFLD|_fCOL},
  {(char *)0,	0, 	     {T_NONE, T_NONE},	0,	 0}
};


/*
** FRS_WORD - Simple structure for fast lookups to find corresponding values
**	      of pseudo-keywords.
*/
typedef	struct {
    char	*fw_name;		/* Name of word */
    i4		fw_def;			/* Integer value */
} FRS_WORD;

static FRS_WORD	*frs_getword(FRS_WORD tab[], char *word);

/*
** frs__cmmands - Table of commands and their values. Used by MAP and LABEL.
**
** Note: This table is interdependent with runtime/fehkeys.qsc. Do not add new 
**	 command names without adding them there too. (ncg)
*/
static FRS_WORD frs__cmmands[] = {
	{ERx("nextfield"),		FsiNEXTFLD},
	{ERx("previousfield"),	FsiPREVFLD},
	{ERx("nextword"),		FsiNXWORD},
	{ERx("previousword"),	FsiPRWORD},
	{ERx("mode"),		FsiMODE},
	{ERx("redraw"),		FsiREDRAW},
	{ERx("deletechar"),		FsiDELCHAR},
	{ERx("rubout"),		FsiRUBOUT},
	{ERx("editor"),		FsiEDITOR},
	{ERx("leftchar"),		FsiLEFTCH},
	{ERx("rightchar"),		FsiRGHTCH},
	{ERx("downline"),		FsiDNLINE},
	{ERx("upline"),		FsiUPLINE},
	{ERx("newrow"),		FsiNEWROW},
	{ERx("clear"),		FsiCLEAR},
	{ERx("clearrest"),		FsiCLREST},
	{ERx("menu"),		FsiMENU},
	{ERx("scrollup"),		FsiSCRLUP},
	{ERx("scrolldown"),		FsiSCRLDN},
	{ERx("scrollleft"),		FsiSCRLLT},
	{ERx("scrollright"),		FsiSCRLRT},
	{ERx("duplicate"),		FsiDUP},
	{ERx("printscreen"),		FsiPRSCR},
	{ERx("nextitem"),		FsiNXITEM},
	{ERx("shell"),			FsiSHELL},
	{(char *)0,		0}
};

/*
** frs__valcoms - Table of validate command values.
*/
static FRS_WORD frs__valcoms[] = {
	{ERx("nextfield"),		FsiNXFLDVAL},
	{ERx("previousfield"),	FsiPRFLDVAL},
	{ERx("menu"),		FsiMENUVAL},
	{ERx("keys"),		FsiKEYSVAL},
	{ERx("menuitem"),		FsiANYMUVAL},
	/* "frskeyN" is handled separately */
	{(char *)0,		0}
};

/*
** frs__actcoms - Table of activate command values.
*/
static FRS_WORD frs__actcoms[] = {
	{ERx("nextfield"),		FsiNXACT},
	{ERx("previousfield"),	FsiPRACT},
	{ERx("menu"),		FsiMUACT},
	{ERx("menuitem"),		FsiMIACT},
	{ERx("keys"),		FsiKYACT},
	{ERx("before"),		FsiENTACT},
	{(char *)0,		0}
};

/*
** frs__ctrl - Table of control values.  Note we cannot just use the
**	       expression c - 'a' + SI_A_CTRL because we may not be using
** ASCII ordered character sets.  At least we can look for the prefix
** "control" before we start searching.
*/
static FRS_WORD frs__ctrl[] = {
	{ERx("a"),		FsiA_CTRL},
	{ERx("b"),		FsiB_CTRL},
	{ERx("c"),		FsiC_CTRL},
	{ERx("d"),		FsiD_CTRL},
	{ERx("e"),		FsiE_CTRL},
	{ERx("f"),		FsiF_CTRL},
	{ERx("g"),		FsiG_CTRL},
	{ERx("h"),		FsiH_CTRL},
	{ERx("i"),		FsiI_CTRL},
	{ERx("j"),		FsiJ_CTRL},
	{ERx("k"),		FsiK_CTRL},
	{ERx("l"),		FsiL_CTRL},
	{ERx("m"),		FsiM_CTRL},
	{ERx("n"),		FsiN_CTRL},
	{ERx("o"),		FsiO_CTRL},
	{ERx("p"),		FsiP_CTRL},
	{ERx("q"),		FsiQ_CTRL},
	{ERx("r"),		FsiR_CTRL},
	{ERx("s"),		FsiS_CTRL},
	{ERx("t"),		FsiT_CTRL},
	{ERx("u"),		FsiU_CTRL},
	{ERx("v"),		FsiV_CTRL},
	{ERx("w"),		FsiW_CTRL},
	{ERx("x"),		FsiX_CTRL},
	{ERx("y"),		FsiY_CTRL},
	{ERx("z"),		FsiZ_CTRL},
	{ERx("del"),		FsiDEL_CTRL},
	{ERx("esc"),		FsiESC_CTRL},
	{(char *)0,	0}
};

static FRS_WORD frs__arrow[] = {
	{ERx("uparrow"),	FsiUPARROW},
	{ERx("downarrow"),	FsiDNARROW},
	{ERx("rightarrow"),	FsiRTARROW},
	{ERx("leftarrow"),	FsiLTARROW},
	{(char *)0,		0}
};

/*
+* FRS Header Parsing:
**
** Source Code:
**	INQUIRE_FRS column form1 table1 (target-list)
**
**	WITH clause on LOADTABLE/INSERTROW:
**		... WITH (ATTR(colname) = ATTR_VAL)...
**
** Internal Calls:
**    INQUIRE:	frs_inqset( FRSinq );
**		frs_object( "column" );
**		frs_parentname( form1, T_CHAR, (SYM *)0 );
**		frs_parentname( table1, T_CHAR, (SYM *)0 );
**		frs_head();
**
**    WITH:	frs_inqset( FRSload );
**		frs_object( "row" );
** Generates:
**    INQUIRE:	if (IIiqset(FsiCOLUMN, 0, "form1", "table1", (char *)0) != 0)
**
**    WITH clause:
**		nothing (LOADTABLE/INSERTROW code generation serves as
**		the "header" code generation).
*/


/*
+* Procedure:	frs_inqset
** Purpose:	Set global mode of statement
** Parameters:
**	flag	-  i4  	- FRSinq or FRSset.
** Return Values:
-*	None
*/

void
frs_inqset( flag )
i4	flag;
{
    MEfill( (u_i2)sizeof(FRS__), '\0', (char *)&frs );
    frs.f_mode = flag;
}

/*
+* Procedure:	frs_object
** Purpose:	Set the object-type for this statement.
** Parameters:
**	objtype	- char * - Object-type as a name.
** Return Values:
-*	None
** Side Effects:
**	Initializes the global frs structre.
*/

void
frs_object( objtype )
register char	*objtype;
{
    register FRS_OBJ	*fo;

    /* Enter a new object-type and make sure it is okay */
    for (fo = frs__objtab; 
	 fo->fo_name && STbcompare(objtype, 0, fo->fo_name, 0, TRUE) != 0; 
	 fo++)
	 ;
    if (fo->fo_name == (char *)0)	/* Leave it pointing at it */
    {
	frs_error( E_EQ0151_fsBAD, EQ_ERROR, 1, objtype );
	frs.f_err = TRUE;
	return;
    }
    frs.f_obj = fo;
    frs.f_row.fn_name = FRSrowDEF;	/* Reset row number for current row */
}

/*
+* Procedure:	frs_parentname
** Purpose:	Add a parent-name for the current object.
** Parameters:
**	nm	- char *- Parent-name "name".
**	type	- i4	- Type (T_INT for rows or T_CHAR).
**	var	- PTR	- Is this a variable.
** Return Values:
-*	None
** Notes:
**	Stashes them away in the global array of parent names.
*/

void
frs_parentname( nm, typ, var )
char	*nm;
i4	typ;
PTR	var;
{
    register FRS_OBJ		*fo;
    register FRS_NAME	*fn;

    if (frs.f_err)
	return;
    if (frs.f_argcnt >= FRSmaxNAMES)
    {
	frs.f_err = TRUE;
	frs_error( E_EQ0155_fsMAXPAR, EQ_ERROR, 1, er_na(FRSmaxNAMES) );
	return;
    }
    fo = frs.f_obj;
    /*
    ** Handle special case of ROW[_COLUMN] which allows an integer row number
    ** to be a parent-name.  The optional row number MUST be the last
    ** argument added.  Add the row name into the current stash.
    */
    fn = &frs.f_pnames[frs.f_argcnt];
    if (typ == T_INT)
    {
	if (   (fo->fo_def != FsiROW && fo->fo_def != FsiRWCL)
	    || frs.f_argcnt != fo->fo_args) 	/* form+tab */
	{
	    frs_error( E_EQ0066_grSTR, EQ_ERROR, 1, nm );
	    var = (PTR)NULL;		/* Make sure it comes out as string */
	}
	else
	{
	    fn = &frs.f_row;
	    frs.f_argcnt--;		/* Not adding to f_pnames[] */
	}
    }
    fn->fn_name = nm;
    fn->fn_var = var;
    frs.f_argcnt++;
}


/*
+* FRS Constant Parsing:
**
** Source Code:	
**		INQUIRE ... (var1 = format(field1))	
**		SET ... (menumap = "file.map")
** Internal Calls:
**	INQUIRE:	frs_iqvar( var1, vartype, varsym );
**			frs_constant( "format", "field1", T_CHAR, (SYM *)0 );
**	SET:		frs_constant( "menumap", (char *)0, T_NONE, (SYM *)0 );
**			frs_setval( "file.map", T_CHAR, (SYM *)0 );
** Generates:
**	INQUIRE:	
**		IIiqfrs(ISVAR, vartype, varlen, var1, FsiFORMAT, "field1", 0);
**	SET:
-*		IIstfrs(FsiMENUMAP, (char *)0, 0, NOVAR, T_CHAR, 0, "file.map");
*/


/*
+* Procedure:	frsck_constant
** Purpose:	Check and return values for frs-constant
** Parameters:
**	constname	- char *	- Frs-constant name.
**	objname		- char *	- Object name.
**	type		- i4		- Type of object name.
**	var		- bool		- Type is variable?
**	value		- i4  *		- Object value, 0 if error,
**						< 0 if indeterminable,
**						> 0 otherwise.
** Return Values:
-*	i4	- Frs constant value.
** Notes:
**   1. Most of the verification is done via table lookup, but some special
**	cases are handled individually.
**   2.  In case of INQUIRE, check type, in case of SET saves frs-constant
**	 info for frs_setval.
**
** History:
**	09-aug-1990 (barbara)
**	    Removed the restriction that set_frs row could take a char
**	    object-name only if frs-constant was CHANGE.
**	12-Jun-1996 (mckba02)
**	    Bug #73178. 
**		An error was being generated for frs_constants that
**		required an object-name, when the object was a variable.
**		When an frs_constant requires an object-name the
**		object name should not be NULL, unless the object 
**		type is a variable (var == 1).
**
*/

i4
frsck_constant( char *constname, char *objname, i4  type, bool var, 
                register i4  *value )
{
/* Special types of local errors */
#   define	_fERNUL		0		/* No error */
#   define	_fERREG		1		/* Regular error */
#   define	_fERMSG		2		/* Message alrady printed */

    register FRS_CONST	*fc;
    register FRS_WORD	*fw;
    i4			fcdef;
    i4			err = _fERNUL;

    *value = 0;
    if (frs.f_err)
	return FsiBAD;

    frs.f_consterr = FALSE;
    /* Find the frs-constant in the table */
    for (fc = frs__consttab ; fc->fc_name != NULL ; ++fc)
    {
	 if (STbcompare(constname, 0, fc->fc_name, 0, TRUE) == 0) 
		break;
    }
    /* Did we find the frs-constant */
    if (fc->fc_name == (char *)NULL)
    {
	frs_error( E_EQ0151_fsBAD, EQ_ERROR, 1, constname );
	err = _fERMSG;
    }
    /* Is the frs-constant in conflicting "set" mode */
    else if (frs.f_mode == FRSset && !fc_canset(fc))
    {
	frs_error( E_EQ0156_fsNOSET, EQ_ERROR, 1, constname );
	err = _fERMSG;
    }
    /* Is this constant allowed in the master object-type */
    else if ((fc->fc_oflag & frs.f_obj->fo_flag) == 0)
    {
	frs_error( E_EQ0159_fsOBJCONST, EQ_ERROR, 2, constname, 
		frs.f_obj->fo_name );
	err = _fERMSG;
    }
    else if (objname != (char *)NULL || var)	/* Object-name used */
    {
	register FRS_OBJ	*fo = frs.f_obj;

	/* Is an object-name allowed */
	if (fc->fc_oname == _fOBNUL)
	{
	    frs_error( E_EQ0154_fsNOTOBJ, EQ_ERROR, 1, constname );
	    err = _fERMSG;
	}
        /* If not using the current object-name (null) then check types */
        else if (type == T_INT && fo->fo_def != FsiROW)
        {
            frs_error( var ? E_EQ0067_grSTRVAR : E_EQ0066_grSTR, EQ_ERROR, 1,
                        objname );
            err = _fERMSG;
	}
    }
    else if (objname == (char *)NULL && !var && fc->fc_oname == _fOBREQ)
    {
	frs_error( E_EQ0153_fsREQOBJ, EQ_ERROR, 1, constname );
	err = _fERMSG;
    }

    /* Interpret object name w/r to constant */
    if (err == _fERNUL)
    {
	if (fc->fc_oname == _fOBREQ && (!var && objname == (char *)NULL) )
	    err = _fERREG;
	else switch (fcdef = fc->fc_def)
	{
	  case FsiMAP:
	  case FsiLABEL:
	    /* Check for map(frskeyN) */
	    if ((*value = frsck_key(objname)) != 0)
	    {
		if (*value < 0)
		{
		    frs_error( E_EQ0157_fsNUM, EQ_ERROR, 2, ERx("FRSKEY"), 
			    er_na(Fsi_KEY_PF_MAX) );
		    err = _fERMSG;
		}
		else
		    *value += FsiFRS_OFST - 1;
	    }
	    /* Check for a menu # - map(menuN) - Is digit? (menu is also command) */
	    else if ((STbcompare(objname, 4, ERx("menu"), 0, TRUE) == 0) &&
		    CMdigit(objname+4))
	    {
		if ((CVan(objname+4, value) != OK) || !fc_murange(*value))
		{
		    frs_error( E_EQ0157_fsNUM, EQ_ERROR, 2, ERx("MENU"), 
			    er_na(Fsi_MU_MAX) );
		    err = _fERMSG;
		}
		else
		    *value += FsiMU_OFST - 1;
	    }
	    /* Check for a command - nextfield */
	    else
	    {
		fw = frs_getword( frs__cmmands, objname );
		if (fw->fw_name == (char *)NULL)
		    err = _fERREG;
		else
		    *value = fw->fw_def;
	    }
	    break;
    
	  case FsiVALDATE:
	    /* Check for frskeyN */
	    if (STbcompare(objname, 6, ERx("frskey"), 0, TRUE) == 0)
	    {
		if ((CVan(objname+6, value) != OK) || !fc_keyrange(*value))
		{
		    frs_error( E_EQ0157_fsNUM, EQ_ERROR, 2, ERx("FRSKEY"), 
		    	    er_na(Fsi_KEY_PF_MAX) );
		    err = _fERMSG;
		}
		else
		{
		    fcdef = FsiFRSKVAL;
		    *value += FsiFRS_OFST - 1;
		}
	    }
	    /* Check for validate command */
	    else
	    {
		fw = frs_getword( frs__valcoms, objname );
		if (fw->fw_name == (char *)NULL)
		    err = _fERREG;
		else
		    fcdef = fw->fw_def;
	    }
	    break;
    
	  case FsiACTIVATE:
	    /* Check for activate command */
	    fw = frs_getword( frs__actcoms, objname );
	    if (fw->fw_name == (char *)NULL)
		err = _fERREG;
	    else
		fcdef = fw->fw_def;
	    break;
    
	  default:
	    *value = -1;	/* did not determine object value */
	    break;
	} /* end switch */

	if (err == _fERREG)
	    frs_error( E_EQ0152_fsCONSTSYN, EQ_ERROR, 2, constname, objname );
    }

    if (err != _fERNUL)
    {
	fcdef = FsiBAD;
	*value = 0;	/* reset to 0 after switch */
    }

    if (frs.f_mode == FRSinq)	/* Check types on INQUIRE */
    {
	register FRS_VAR	*fv = &frs.f_var;

	if (fv->fv_type != fc->fc_type[FRSinq] && !err)
	{
	    /* DBV is always OK -- runtime must deal with it. */
	    if (fv->fv_type != T_DBV)
		frs_error( fc->fc_type[FRSinq] == T_INT ? E_EQ0061_grINTVAR : 
			E_EQ0067_grSTRVAR, EQ_ERROR, 1, fv->fv_name );
	}
    }
    else 	/* Save error status and frs-constant for SET values */
    {
	frs.f_const = fc;
	frs.f_consterr = (err != _fERNUL);
    }

    return fcdef;
}

/*
+* Procedure:	frs_iqsave
** Purpose:	Save reference for INQUIRE.
** Parameters:
**	name	- char *- Variable name.
**	type	- i4	- Variable type.
** Return Values:
-*	None
** Notes:
**	Saves the information away for frs_constant.
*/

void
frs_iqsave( name, type )
char			*name;
i4			type;
{
    register FRS_VAR	*fv = &frs.f_var;

    /* Save information for later frs-constant type checking */
    fv->fv_name = name;
    fv->fv_type = type;
}

/*
+* Procedure:	frsck_valmap
** Purpose:	Check and return value for map-constant.
** Parameters:
**	name	- char *- Variable name.
** Return Values:
-*	i4 - The map value.
*/

i4
frsck_valmap (name)
register char	*name;
{
    register FRS_WORD	*fw;
    i4		value = 0;

    /* Check for a controlX */
    if (STbcompare(name, 7, ERx("control"), 0, TRUE) == 0)
    {
	fw = frs_getword( frs__ctrl, name+7 );
	if (fw->fw_name == (char *)NULL)
	    frs_error( E_EQ0160_fsPFCTRL, EQ_ERROR, 1, name );
	else
	    value = fw->fw_def;
    }
    /* Check for a PF number */
    else if (STbcompare(name, 2, ERx("pf"), 0, TRUE) == 0)
    {
	if ((CVan(name+2, &value) != OK) || !fc_keyrange(value))
	    frs_error( E_EQ0157_fsNUM, EQ_ERROR, 2, ERx("PF key"), 
		    er_na(Fsi_KEY_PF_MAX) );
	else
	    value += FsiPF_OFST - 1;
    }
    /* Check for an arrow key */
    else
    {
	fw = frs_getword( frs__arrow, name );
	if (fw->fw_name == (char *)NULL)
	    frs_error( E_EQ0160_fsPFCTRL, EQ_ERROR, 1, name );
	else
	    value = fw->fw_def;
    }
    return value;
}

/*
+* Procedure:	frs_getword
** Purpose:	Utility to search for keyword in table for passed word.
** Parameters:	
**	tab 	- FRS_WORD[] 	- Word table to use,
**	word	- char *	- Caller's word.
** Returns Values:	
-*	FRS_WORD * - Pointer to entry found.
*/

static FRS_WORD	*
frs_getword( tab, word )
FRS_WORD	tab[];
register char	*word;
{
    register FRS_WORD	*fw;

    for (fw = tab; 
	 fw->fw_name && STbcompare(word, 0, fw->fw_name, 0, TRUE) != 0; 
	 ++fw)
	 ;
    return fw;
}


/*
+* Procedure:	frsck_mode
** Purpose:	Check out mode of object.
** Parameters:
**	stmt	- char *- Statement name (in case of error).
**	mode	- char *- Mode being set (cannot be in a variable).
** Return Values:
-*	char * - Pointer to valid mode.
** Notes:
**	Checks the input mode against a list of valid modes.
**
** History:
**	10/90 (jhw) - Modified to return "FILL" mode on error.  Bug #34260.
*/

char *
frsck_mode (stmt, mode)
char		*stmt;
register char	*mode;
{
    static char	*modes[] = {
		    ERx("read"), ERx("update"), ERx("fill"), 
		    ERx("query"), (char *)0
    };

    register char	**cp;

    for (cp = modes ; *cp != NULL ; ++cp)
	if (STbcompare(*cp, 0, mode, 0, TRUE) == 0)
	    return *cp;
    /* fall through on error ... */
    frs_error( E_EQ0069_grBADWORD, EQ_ERROR, 3, mode, stmt, ERx("FILL") );
    return ERx("f");
}

/*
+* Procedure:	frsck_key
** Purpose:	Check FRS key specification.
** Parameters:
**	spec	- char * - Key specification.
** Return Values:
-*	i4	- Key value, if > 0,
**			not FRS key if = 0,
**			bad key value, if < 0.
*/

i4
frsck_key (spec)
char	*spec;
{
    i4	keynum;

    if (spec == (char*)NULL || STbcompare(spec, 6, ERx("frskey"), 6, TRUE) != 0)
	return 0;
    else if (CVan(spec+6, &keynum) != OK || !fc_keyrange(keynum))
	return -1;
    else
	return keynum;
}
