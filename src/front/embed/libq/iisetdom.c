/*
** Copyright (c) 1987, 2009 Ingres Corporation
*/

# include	<compat.h>
# include	<cm.h>
# include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<generr.h>
# include	<adf.h>
# include	<gca.h>
# include	<iicgca.h>
# include	<eqrun.h> 		/* Preprocessor/Runtime common */
# include	<iisqlca.h>
# include	<iilibq.h>
# include	<adh.h>
# include	<erlq.h>
# include	<me.h>
# include	<fe.h>
# include	<cm.h>

/**
+*  Name: iisetdom.c - Send variables to the back end.
**
**  Description:
**	IIputdomio is used to write the contents of a host language variable to 
**	the Quel parser. The variable must represent a constant in a Quel 
**	expression.
**
**  Defines:
**	IIputdomio
**	IIsetdom
**
**  Notes:
**	None.
-*
**  History:
**	26-aug-1987 	- Modified to use GCA. (ncg)
**	24-oct-1988	- Added IILQdtDbvTrim cover. (wong)
**	19-may-1989	- Changed global var names for multiple connects. (bjb)
**	02-jul-1991	- Initialized dbf struct using MEfill, and included
**			  me.h for the MEfill macro.  char types leave integer
**			  attribute fields un-initialized.
**	01-nov-1992 (kathryn)
**	    Updated to handle large objects.  Added isnull and tmp_len
**	    variables.  If variable is large object then it must be
**	    transmitted in segments (call IILQlpd_LoPutData).
**	23-nov-1992 	- Added fix so decimal datatype won't fall into code
**		      	  used for handling large objects. (teresal)
**	10-apr-1993  (kathryn)
**	    Add call to IILQlcc_LoCbClear to clear large object data structure 
**	    after sending large object.
**	20-jul-1993  (kathryn)
**	    Set handler flags for large objects.
**	10-jan-1993 (teresal)
**	    Put in support for checking non-null terminated C strings
**	    (FIPS). Bug # 55915.
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      12-sep-2001 (stial01)
**          IIputdomio() fixed hostlen and seglen args to IILQlpd_LoPutData
**      05-feb-2009 (joea)
**          Rename CM_ischarsetUTF8 to CMischarset_utf8.
**	19-Aug-2009 (kschendel/stephenb) 121804
**	    Need cm.h for proper CM declarations (gcc 4.3).  Add prototypes.
**/

/*{
+*  Name: IIputdomio - Write a variable to the back end.
**
**  Description:
**	IIputdomio is used to write the contents of a host language variable to 
**	the Quel parser. The variable must represent a constant in a Quel 
**	expression.
**
**  Inputs:
**	indaddr		- The address of the null indicator variable
**	isvar		- TRUE/FALSE: is a variable/is a constant
**	type		- The EQUEL datatype
**	length		- The length of the variable (0 for strings)
**	addr		- The address of the variable (value for constants)
**
**  Outputs:
**	Returns:
**	    Nothing.
**	Errors:
**	    <error codes and description>
-*
**  Side Effects:
**	Sends data to the backend parser.
**	
**  Notes:
**   1. The processing of the variable is dependent on its type and the 
**	current host language. 
**   2.	Floats are converted to doubles first.
**   3. If the data consists of a 1-byte integer, it's moved into a 
**	local 2-byte integer before sending to the DBMS.  If we ever get an 
**	i1 by value we're in deep trouble.  The code assumes that the first 
**	byte of the value is the i1 itself, which simply isn't true on some 
**	machines.  On the other hand, the preprocessor will never generate 
**	code that does this.  This conversion is done to be backward compatible
**	with 5.0 repeat query generation, where it "knew" that the parameter
**	was specified as "$digit is i2".
**
**  Example:
**		##  # define I 12
**		##  short ind;
**		##  i4    i;
**
**		## append to tab (col1 = i:ind, col2 = I)
**
**  Generates (in part):	
**		# define I 12
**		short ind;
**		i4   i;
**		   
**		IIputdomio( &ind, 1, T_INT, sizeof(i4), &i );
**		IIputdomio( (short *)0, 0, T_INT, sizeof(i4), I );
**  History:
**	06-feb-1985 - Rewritten (ncg)
**	04-mar-1985 - Changed IIpipe to pointer. (lin)
**	27-sep-1986 - Added conversion type parameter to pb_put calls
**		      to support inter-architecture networking. (mmb)
**	09-oct-1986 - Modified for 6.0 global removal. (mrw)
**	25-oct-1986 - Modified to use DB_EMBEDDED_DATA for host var info.(bjb)
**	12-dec-1986 - Updated interface to adh_ routines. (bjb)
**	26-feb-1987 - Updated interface for null indicator support (bjb)
**	31-mar-1987 - Modified for II_Q_COMMA and II_Q_NULIND semantics. (ncg)
**	26-aug-1987 - Modified to use GCA. (ncg)
**	30-aug-1988 - Bug 3045: in C a user may have a null buffer and a
**		      null indicator set. (ncg)
**	12-dec-1988 - Generic error processing. (ncg)
**	23-nov-1992 - Fix for decimal to avoid large object code. (teresal)
**	09-dec-1991 - Modified ADF control block reference to be session 
**		      specific instead of global (part of decimal changes).
**		      (teresal)
**	15-jan-1993 (kathryn)
**	    Send long types to adh_evtodb to determine actual length.
**	    Adh will not change the datatype as a long datatype is different
**	    than just a CHAR with a long length, it includes an ADP_PERIPHERAL.
**	    Note that ADH does not recognize a long datatype (DB_LVCH_TYPE,
**	    etc..) so long datatypes should not be sent to adh_*cvt* routines.
**	    Only DB_DATA_VALUES may have long types, as there is no way
**	    for a user application to declare them.
**	18-aug-1993 (kathryn)
**	    Any value longer than DB_MAXSTRING gets sent as ADP_PERIPHERAL,
**	    For versions 6.5 and greater.  Gateways will change to accept
**	    ADP_PERIPHERAL type even for lengths < 4096.
**	01-oct-1993 (kathryn)
**	    Change value of first parameter in call to IILQlpd_LoPutdata().
**	4-may-00 (inkdo01)
**	    Fix local data error (i1's copied to i2's can be overlaid)
**	25-Feb-05 (karbh01)(B113961)(INGCBT::557)
**	 A hard coded value passed from embedded cobol program. Compiler defines that with i8 size.
**	 Store value into i4 variable and pass the address of this variable.
**	28-Sep-2007 (gupsh01)
**	    For UTF8 installations put a value of 16000 bytes instead.
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
**
*/
/*VARARGS4*/
void
IIputdomio( indaddr, isvar, type, length, addr )
i2		*indaddr;
i4		isvar;
i4		type;
i4		length;
char		*addr;
{
    II_THR_CB		*thr_cb = IILQthThread();
    II_LBQ_CB		*IIlbqcb = thr_cb->ii_th_session;
    DB_DATA_VALUE	dbv, dvbuf;
    DB_EMBEDDED_DATA	edv;
			/* +1 for null byte if conversion is needed */
    f8			setbuf[(DB_GW4_MAXSTRING + DB_CNTSIZE)/sizeof(f8) +1];
    i4			qflags = IIlbqcb->ii_lq_curqry;
    DB_STATUS		stat;
    i4		locerr;
    i4		generr;
    i4			max_len = 0;
    i2		local_i2;
    bool		isnull = FALSE;
    MEfill(sizeof(dbv), 0, &dbv);
    if (qflags & II_Q_QERR)
	return;

    if (type == DB_DBV_TYPE)
	isnull = ADH_ISNULL_MACRO((DB_DATA_VALUE *)addr);
    else
    	isnull = (indaddr && *indaddr == DB_EMB_NULL);

    if (isvar && addr == (char *)0)
    {
	/* If NULL constant, allow null address too - point it at blanks */
	if (isnull)
	{
	    addr = ERx("    ");
	}
	else
	{
	    IIlocerr(GE_SYNTAX_ERROR, E_LQ0010_EMBNULLIN, II_ERR, 0, (char *)0);
	    IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	    return;
	}
    }

    /*
    ** If we have a large object, then do not go thru ADH.
    **
    ** Note: Decimal length appears large because it contains an 
    ** encoded precision and scale, so make sure this is NOT a decimal type.
    */


    /* Transfer host variable information to an DB_EMBEDDED_DATA */
    edv.ed_type = type;
    if (length <=  MAXI2) 	    /* could be for BLOB */
        edv.ed_length = length;
    else
    {
	dbv.db_length = length;
	edv.ed_length = 0;
    }
    edv.ed_null = indaddr;
    /* 
    ** If the null indicator does not need to be preserved (ie, type
    ** compatibility for REPEAT queries), and the null indicator is not
    ** null-valued, set the indicator pointer to null so that ADH does
    ** not explicitly convert (and require extra storage).
    */
    if (!(qflags & II_Q_NULIND) && edv.ed_null && *edv.ed_null != DB_EMB_NULL)
	    edv.ed_null = (i2 *)0;
    if (type==DB_INT_TYPE && length==sizeof(i1))	/* Convert to i2 */
    {
	edv.ed_length = sizeof(i2);
	if (isvar)
	    local_i2 = I1_CHECK_MACRO(*(i1 *)addr);
	else
	    local_i2 = I1_CHECK_MACRO(*(i1 *)&addr);
	edv.ed_data = (PTR)&local_i2;
    } 
    else			/* Not an i1 */
    {
	if (isvar)		/* Point at the callers address */
	    edv.ed_data = (PTR)addr;
	else
	{
#if defined(any_aix) && defined (LP64)
          if (IIlbqcb->ii_lq_gca->cgc_version <= GCA_PROTOCOL_LEVEL_64)
          {
              /* i8 is not supported by GCA */
               i4 tmp;
                tmp = *(i8 *)&addr;
                edv.ed_data = (PTR)&tmp;
          }
          else
#endif
              edv.ed_data = (PTR)&addr;
	}
    }

    dvbuf.db_length = sizeof(setbuf);
    dvbuf.db_data = (PTR)setbuf;
    stat = adh_evtodb( IIlbqcb->ii_lq_adf, &edv, &dbv, &dvbuf, TRUE );

    /* Do we have a BLOB */
    if (CMischarset_utf8())
      max_len =  DB_UTF8_MAXSTRING
	       + (ADH_NULLABLE_MACRO((DB_DATA_VALUE *)&dbv) ? 1 : 0)
               + (IIDB_VARTYPE_MACRO((DB_DATA_VALUE *)&dbv) ? DB_CNTSIZE : 0);
    else

      max_len =  DB_MAXSTRING  
	       + (ADH_NULLABLE_MACRO((DB_DATA_VALUE *)&dbv) ? 1 : 0)
               + (IIDB_VARTYPE_MACRO((DB_DATA_VALUE *)&dbv) ? DB_CNTSIZE : 0);

    if (  (stat == E_DB_OK) && !isnull &&  (dbv.db_length > max_len) 
       && (type != DB_DEC_TYPE)
       && (IIlbqcb->ii_lq_gca->cgc_version >=  GCA_PROTOCOL_LEVEL_60))
    {
	i4	hostlen = dbv.db_length;
	i4	seglen = dbv.db_length; 

	/*
	** adh_evtodb calculated db internal length (ucs2 length of data)
	** IILQlpd_LoPutData wants something entirely different:
	** seglen = #wchars and hostlen
	*/
	if (type == DB_NVCHR_TYPE)
	{
	    seglen = (dbv.db_length - DB_CNTSIZE) / sizeof(UCS2);
	    hostlen = (seglen * sizeof(wchar_t)) + DB_CNTSIZE;
	}
	else if (type == DB_NCHR_TYPE)
	{
	    seglen = dbv.db_length / sizeof(UCS2);
	    hostlen = seglen * sizeof(wchar_t);
	}

	IIlbqcb->ii_lq_lodata.ii_lo_flags |= (II_LO_START | II_LO_PUTHDLR);
	IIlbqcb->ii_lq_flags |= II_L_DATAHDLR;	
	IILQlpd_LoPutData(II_DATSEG|II_DATLEN ,type, hostlen,
				edv.ed_data, seglen, 1);
	IILQlcc_LoCbClear( IIlbqcb );
	IIlbqcb->ii_lq_flags &= ~II_L_DATAHDLR;
	return;
    }
    if (stat == E_DB_OK)
    {
	stat = adh_evcvtdb( IIlbqcb->ii_lq_adf, &edv, &dbv, FALSE );
    }

    if (stat != E_DB_OK)
    {
	switch (IIlbqcb->ii_lq_adf->adf_errcb.ad_errcode)
	{
	  case E_AD1010_BAD_EMBEDDED_TYPE:
	  case E_AD1011_BAD_EMBEDDED_LEN:
	    locerr = E_LQ0014_DBVAR;
	    generr = GE_DATA_EXCEPTION + GESC_TYPEINV;
	    break;
	  case E_AD1012_NULL_TO_NONNULL:
	    locerr = E_LQ0016_DBNULL;
	    generr = GE_DATA_EXCEPTION + GESC_NEED_IND;
	    break;
	  case E_AD1013_EMBEDDED_NUMOVF:
	    locerr = E_LQ0015_DBOVF;
	    generr = GE_DATA_EXCEPTION + GESC_FIXOVR;
	    break;
	  /* Temporarily map ADF internal error to unterminated C string
          ** while we are waiting to get a ADF error message defined for this.
          */
          /* case E_ADXXXX_UNTERM_C_STR */
          case E_AD9999_INTERNAL_ERROR:
            locerr = E_LQ00EC_UNTERM_C_STRING;
            generr = GE_DATA_EXCEPTION + GESC_ASSGN;
            break;
	  case E_AD2009_NOCOERCION:
	  case E_AD2004_BAD_DTID:
	  default:
	    locerr = E_LQ0017_DBCONV;
	    generr = GE_DATA_EXCEPTION + GESC_ASSGN;
	    break;
	}
	IIlocerr(generr, locerr, II_ERR, 0, (char *)0);
	IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	return;
    }
# ifdef II_DEBUG
    if (IIdebug)
       SIprintf(ERx("IIputdomio: type %d, length %d\n"),
                dbv.db_datatype, dbv.db_length);
# endif /* II_DEBUG */

    /*
    ** In dynamic SQL queries, (with the USING clause) a list of variables
    ** may be sent without separating commas.  Currently, there is no
    ** special block for these, but we want to allow it in the future so
    ** we generate the commas here.  Q_1COMMA is set before the first put data,
    ** while Q_2COMMA is set before subsequent puts.  The initial flag is
    ** set in IIsqExStmt and IIcsOpen (both may have the USING clause).
    */
    if (qflags & II_Q_2COMMA)		/* Not first one */
    {
	DB_DATA_VALUE	dbcomma;

	II_DB_SCAL_MACRO(dbcomma, DB_QTXT_TYPE, 1, ERx(","));
	IIcgc_put_msg(IIlbqcb->ii_lq_gca, FALSE, IICGC_VQRY, &dbcomma);
    }
    else if (qflags & II_Q_1COMMA)	/* First item - need commas on next */
        IIlbqcb->ii_lq_curqry |= II_Q_2COMMA;

    IIcgc_put_msg(IIlbqcb->ii_lq_gca, TRUE, IICGC_VDBV, &dbv);
}

/*{
+*  Name: IIsetdom - Cover routine for IIputdomio
**
**  Description:
**	This routine is here for backward compatibility, so that old
**	programs may be relinked and still work.  It merely calls
**	the newer data-sending routine, IIputdomio.
**
**  Inputs:
**	isvar	- TRUE/FALSE: is a variable/is a constant
**	hosttyp - The embedded datatype
**	length  - The length of the variable (may be 0 for strings)
**	addr	- The address of the variable (value for constants)
**
**  Outputs:
**	Returns:
**	    Nothing.	
**	Errors:
**	    <error codes and description>
-*
**  History:
**	26-feb-1987 - written (bjb)
*/
void
IIsetdom( isvar, type, length, addr )
i4		isvar;
i4		type;
i4		length;
char		*addr;
{
    IIputdomio( (i2 *)0, isvar, type, length, addr );    
}

/*{
** Name:	IILQdtDbvTrim() -	Send Trimmed Value to DBMS.
**
** Description:
**	A special cover for 'IIputdomio()' that trims white space from
**	DB_CHR_TYPE and DB_CHA_TYPE data values.  Needed because whitespace
**	is not significant in these types, but the type information is not
**	sent to the DBMS (which seems them as TEXT or VARCHAR.)
**
** Input:
**	dbv	{DB_DATA_VALUE *}  The data value to send to the DBMS.
**
** Called by:
**	'IIQG_generate()' and 4GL-generated C code.
**
** History:
**	10/88 (jhw) -- Written.
**	03-aug-1989	- Bug #6961.  Nullable types must call IIputdomio 
**			  with a null ind. (bjb)
*/
VOID
IILQdtDbvTrim ( dbv )
register DB_DATA_VALUE	*dbv;
{
    i2		null_ind = 0;

    /* If C or CHAR and not null */
    if ((   dbv->db_datatype == DB_CHR_TYPE || dbv->db_datatype == -DB_CHR_TYPE
	 || dbv->db_datatype == DB_CHA_TYPE || dbv->db_datatype == -DB_CHA_TYPE)
	&& !(ADH_ISNULL_MACRO(dbv))
       )
    {
	IIputdomio(dbv->db_datatype > 0 ? (i2 *)0 : &null_ind,
		   TRUE, DB_CHA_TYPE,
		   dbv->db_datatype > 0 ? dbv->db_length : (dbv->db_length-1),
		   dbv->db_data);
    }
    else
    {
	    IIputdomio((i2 *)0, TRUE, DB_DBV_TYPE, 0, (char *)dbv);
    }
}
