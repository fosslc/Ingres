/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<er.h>
# include	<st.h> 
# include	<si.h> 
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<generr.h>
# include	<adf.h>
# include	<gca.h>
# include	<iisqlca.h>
# include	<iicgca.h>
# include	<eqrun.h>
# include	<iilibq.h>
# include	<erlq.h>
# include	<cm.h>
# include	<eqlang.h>
# include	<fe.h>

/**
+*  Name: iiinqset.c - Define routines to support INQUIRE and SET_EQUEL.
**
**  Defines:
**	IILQisInqSqlio  - INQUIRE_SQL routine.
**	IIeqiqio 	- Cover for IILQisInqSqlio for backward compatibility
**	IIeqinq	 	- Cover routine for IIeqiqio for backward compatibility
**	IILQssSetSqlio  - SET_SQL routine.
**	IIeqstio 	- Cover for IILQssSetSqlio for backward compatibility
**	IIeqset	 	- Cover routine for IIeqstio for backward compatibility
**	IILQshSetHandler- SET_SQL user defined handlers.
**	IItuples 	- Return the row count (old call).
**	IIerrnum 	- Return/Set error number.
**
**  Example:
**	## int	err;
**	## int	rno;
**	## short ind;
**
**	## inquire_equel (err:ind = errorno, rno = rowcount)
**	## set_equel (errordisp = 1)
**  Generates:
**	int err;
**	int rno;
**	short ind;
**	IIeqiqio(&ind,1,30,4,&err,"errorno");
**	IIeqiqio((short *)0,1,30,4,&rno,"rowcount");
**	
**	IIeqstio("errordisp",(short *)0,0,30,4,1);
-*
**  History:
**	06-aug-1983     - Written. (jen)
**	14-oct-1983	- Added IIeqset. (ncg)
**	26-feb-1987	- Modified to support null indicators and DBVs. (bjb)
**	29-jun-87 (bab)	Code cleanup.  Cast parm to IIconvert from PTR to
**		(i4 *) to help quiet lint.
**	12-dec-1988	- Generic error processing. (ncg)
**	19-may-1989	- Changed global names for multiple connections. (bjb)
**	06-dec-1989	- Changed for Phoenix/Alerters. (bjb)
**	30-apr-1990	- Added SET for "prefetchrows" & "printcsr". (ncg)
**	14-jun-1990 (barbara)
**	    Added SET and INQUIRE for "programquit".  This is for backwards
**	    compatibility after bug fix 21832.
**	26-jul-1990 (barbara)
**	    Support for decimal.
**      27-jul-1990 (barbara)
**	    Added INQUIRE for "prefetchrows".
**	03-dec-1990 (barbara)
**	    Moved state (for INQUIRE_INGRES) from GLB_CB to LBQ_CB (bug 9160)
**	15-mar-1991 (kathryn)
**	    Added :   IILQisInqSqlio - IILQssSetSqlio - IILQshSetHandler
**	    Changed : IIeqiqio to cover routine for IILQisInqSqlio
**		  and IIeqstio to cover routine for IILQssSetSqlio
**      14-mar-1991 (teresal)
**          Add support for SET and INQUIRE "savequery" and
**          INQUIRE "querytext".  Also changed the behavior
**          of SET_INGRES for "printqry" and "printgca"
**          so it can be issued anywhere in an embedded program.
**	13-mar-1991 (barbara)
**	    Added PRINTTRACE for directing server trace messages to a file.
**      20-mar-1991 (teresal)
**          Add INQUIRE for logical keys, i.e., "table_key" and
**          "object_key".
**	25-mar-1991 (teresal)
**	    Clean up IIeqstio and IILQssSetSqlio so argument declarations 
**	    are in the correct order.
**	25-mar-1991 (barbara)
**	    Added COPYHANDLER support to IILQshSetHandler.
**	01-apr-1991 (kathryn)
**	    Added message E_LQ00A4_ATTVALTYPE which is more appropriate now
**	    that attributes are checked at pre-process time.
**	01-nov-1992 (kathryn)
**	    Added support for INQUIRE_SQL "columntype" and "columnname".
**	10-nov-1992 (davel)
**	    Added support for SET_SQL "session" from 4GL (i.e. passed as
**	    DB_DATA_VALUEs) in IILQssSetSqlio().
**	09-dec-1992 (teresal)
**	    Modified ADF control block reference to be session specific
**	    instead of global (this is part of the decimal changes).
**	07-jan-1993 (teresal)
**	    Call LIBQ routine IILQasAdfcbSetup() to allocate an 
**	    ADF control block - this allows a ADF CB for each session.
**	02-mar-1995 (canor01)
**	    DBMSERROR - lbqerr->ii_er_other can be reset by cascading
**	    local errors. Should follow documentation to be sqlerrd(1)
**	    (B66970).
**	11-apr-1995 (canor01)
**	    Above fix will fail when libq functions are called 
**	    directly and there is no sqlca.
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	12-feb-2003 (abbjo03)
**	    Remove PL/1 as a supported language.
**  	03-Dec-2003 (gilta01)
**	    Added #ifdef statement to enable the fix for b110944 to
**          be compatible with 32 bit Ingres.
**          The code within the #ifdef and #endif is only required for 
**          platforms that support 64-bit Ingres.
**	16-Jun-2009 (thich01)
**	    Treat GEOM type the same as LBYTE.
**	24-Aug-2009 (stephenb/kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**	15-Oct-2010 (thich01)
**	    Treat all spatial types the same as LBYTE.
**/

# define	II__OBJLEN	20	/* Length of object name */

/* Error defines taken from runtime.h (forms system) */
# define	SIATTR		(i4)8156
# define	SIATNUM		(i4)8157
# define	SIATCHAR	(i4)8158

/* External data */
FUNC_EXTERN i4	IIret_err();		/* Standard error printing */
FUNC_EXTERN i4	IIno_err();		/* No error printing */

/*{
+*  Name: IILQisInqSqlio - Inquires about information maintained by LIBQ.
**
**  Description:
**	This routine allows callers to inquire about some information 
**	maintained by LIBQ.  Currently users can inquire about:
**
**	errorno - The INGRES/generic errorno (or zero) will be assigned
**		  to the user variable.
**	dbmserror - The DBMS-specific error number.
**	errortext - The text of the last occurring error.
**	errorseverity - Status of error (valid if errornumber is set).
**	rowcount - The number of rows RETRIEVED will be assigned.
**	endquery - The values 1 or 0 will be assigned.
**	messagenumber - Database procedure message number.
**	messagetext - Database procedure message text.
**	iiret - Internal object generated for status of database procedure.
**	high|lowstamp - QA objects for concurrency testing.
**	savestate - Internal state saved from parent process.
**	transaction - In or out of a transaction.
**	errortype - Is default error reporting generic or local?
**	session - What is session id of current session?
**	programquit - Program set to quit on association failures?
**	prefetchrows - Number of cursor rows returned on prefetch.  This
**		number is only defined right after open cursor.
**      querytext - If savequery is on, returns last query text sent to DBMS.
**      savequery - Returns if savequery feature is turned on (1) or off (0)
**      table_key - What is the value of the last logical table_key
**              added via an INSERT statement.
**      object_key - What is the value of the last logical object_key
**              added via an INSERT statement.
**	dbeventname - Name of the last consumed event.
**	dbeventowner - Owner of the last consumed event.
**	dbeventdatabase - Database name for the last consumed event.
**	dbeventtime - Time of the last consumed event.
**	dbeventtext - Text of the last consumed event.
**	connection_target - [vnode::]dbname[/gateway] from command line
**	columnname - Column name - valid only from within a GET datahandler.
**	columntype - Column datatype - valid only from within a GET datahandler.
**	
**	
**	All these objects require a user variable to hold the result of
**	the INQUIRE.  For internal use, a DB_DATA_VALUE (pointing to variable) 
**	is also legal.  If the caller passes in a variable of the wrong type,
**	adh_dbcvtev will return an error.  
**
**      Note that this routine is set up to support null indicators,
**      but most objects being inquired about can never be null
**      valued.  The exception is "querytext", "table_key" and
**      "object_key"; if LIBQ cannot return a valid value, the
**      null indicator is set (and the user's variable is not
**      touched) to indicate no value is returned.
**
**  Inputs:
**	inaddr	- i2 *  - Address of null indicator
**	isvar	- i4	- Is this a variable (better be).
**	type	- i4	- Host type of variable.
**	len	- i4	- Lenth of variable.
**	addr	- char *- Address of variable.
**	attrib 	- i4    - Integer representing attribute name.
**
**  Outputs:
**	Returns:
**	    Nothing
**	Errors:
**	    SIAT errors from the form system, flagging bad atributes on
**	    SET/INQUIRE.
**	    SIATTR	- Bad attribute used.
**		        - Wrong type (still to be done)
-*
**  Side Effects:
**	
**  History:
**	06-aug-1983     - Written. (jen)
**	26-feb-1987	- Extended to support null indicators and DBVs. (bjb)
**	03-mar-1987	- Added commit stamp retrieval. (ncg)
**	11-may-1988	- Added procedure status and user message
**			  retrieval. (ncg)
**	01-sep-1988	- Added saved state retrieval. (ncg)
**	27-sep-1988	- Added transaction state retrieval. (ncg)
**	12-dec-1988	- Generic error retrieval. (ncg)
**	18-apr-1989	- Added DIFFARCH retrieval. (bjb)
**	11-may-1989	- Added ERRORTYPE object. (bjb)
**	19-may-1989	- Modified names of globals for multiple connects
**			  and added SESSION object. (bjb)
**	06-dec-1989	- Added EVENTASYNC retrieval for Phoenix/Alerters. (bjb)
**	14-jun-1990 (barbara)
**	    Added PROGRAMQUIT for symmetry with same feature on set_ingres.
**	26-jul-1990 (barbara)
**	    Added decimal support.  Set prec field to zero.
**	27-jul-1990 (barbara)
**	    Added PREFETCH for cursor prefetch.
**	12-oct-1990 (kathryn)
**	    INQUIRE_INGRES(ERRORTEXT) - Edit the error message which has been
**	    formatted by IIUGfemFormatError. Add "\r" for each "\n" in message.
**          In VMS non-C languages, printing of "\n" outputs only a line-feed.
**	    An additional "\r" (carriage return) is needed to output the
**	    desired new-line. We edit the message here because Inquire_Ingres 
**          is the only time we pass the message back to the user for possible
**	    printing from NON-C language. Tests indicate the added "\r" is OK 
**	    on UNIX.
**	15-oct-1990 (barbara)
**	    On INQUIRE dbmserror, no need to check ii_er_num  for zero.
**	    Informational messages set dbmserror only and we want to pick
**	    these up.
**	23-oct-1990 (kathryn)
**	    Only add "\r" if host language ("ii_lg_host") is NON-C.
**	20-nov-1990 (kathryn)
**	    Change above fix to:  ifdef VMS  - This is for VMS only.
**	    1) Fortran OR PL1 then remove "\n" from formatted message.
**	    2) Ada, Basic, Cobol, and Pascal add "\r" for each "\n".
**	15-mar-1991 (kathryn)
**	    Moved functionality from IIeqiqio to IILQisInqSqlio
**	    and made IIeqiqio a cover to this routine.  The pre-processor now 
**	    evaluates attributes, and generates calls to this routine with an
**	    integer constant representing the attribute rather than the
**	    attribute name as a string.
**	14-mar-1991 (teresal)
**	    Add "savequery" and "querytext" features.	
**      20-mar-1991 (teresal)
**          Add support for inquiring on logical key values.
**	01-may-1991 (teresal)
**	    Fixed EVENTTEXT to return actual length of the text.
**	18-jul-1991 (teresal)
**	    Fix eventname, eventowner, eventdatabase to return correct
**	    length.  Also, fixed errortext, savestate, messagetext, and
**	    event info to return a blank string with out nulls
**	    if no data was found. Bugs 38507 and 38680.
**	20-jul-1991 (teresal)
**	    Changed EVENT to DBEVENT.
**	01-nov-1992 (kathryn)
**	    Added isCOLTYPE and isCOLNAME for support of  INQUIRE_SQL
**	    of "columtype" and "columnname" for use within datahandlers.
**	21-dec-1992 (larrym)_
**	    Added support for inquire_sql (CONNECTION_TARGET)
**	07-jan-1993 (teresal)
**	    Call LIBQ routine to set up ADF control block.
**	03-mar-1993 (larrym)
**	    added support for inquire_sql (CONNECTION_NAME)
**	
*/


/*VARARGS5*/
void
IILQisInqSqlio(indaddr, isvar, type, len, addr, attrib)
i2	*indaddr;			/* address of null indicator */
i4	isvar;				/* flag - variable? unused */
i4	type;				/* variable data type */
i4	len;				/* length of data in bytes */
PTR	addr;				/* address of var */
i4	attrib;				/* name of field to return */
{
    II_THR_CB		*thr_cb = IILQthThread();
    II_LBQ_CB		*IIlbqcb = thr_cb->ii_th_session;
    DB_DATA_VALUE	dbiq;	
    DB_EMBEDDED_DATA	edv;
    DB_STATUS		stat;
    i4			itemp;
    II_ERR_DESC		*lbqerr;
    char		fmterr[ER_MAX_LEN +1];
    char		*eptr;
    char		*fptr;


    lbqerr = &IIlbqcb->ii_lq_error;
    dbiq.db_prec = 0;

    /* Get name of attribute and build a dbv from the appropriate object */

    switch (attrib)
    {
    	case siROWCNT:		/* rowcount */
		dbiq.db_datatype = DB_INT_TYPE;
		dbiq.db_length = sizeof(i4);
		dbiq.db_data = (PTR)&IIlbqcb->ii_lq_rowcount;
		break;

	case siERNO  :		/* errorno */
	case isMSGNO :		/* messagenumber */
		dbiq.db_datatype = DB_INT_TYPE;
		dbiq.db_length = sizeof(i4);
		dbiq.db_data = (attrib == siERNO) ?
			(PTR)&lbqerr->ii_er_num : (PTR)&lbqerr->ii_er_mnum;
   		break; 

    	case isENDQRY:		/* endquery */
		dbiq.db_datatype = DB_INT_TYPE;
		dbiq.db_length = sizeof(i4);
		itemp = ((i4)IIlbqcb->ii_lq_qrystat & GCA_END_QUERY_MASK) != 0;
		dbiq.db_data = (PTR)&itemp;
		break;

    	case siDBMSER:		/* dbmserror */
	 	dbiq.db_datatype = DB_INT_TYPE;
	 	dbiq.db_length = sizeof(i4);
                /* 02-mar-1995 (canor01)
                ** lbqerr->ii_er_other can be reset by cascading local
		** errors, while dbms error was saved in sqlerrd(1).
		*/
                if ( IIlbqcb->ii_lq_sqlca != NULL )
	 	    dbiq.db_data = (PTR)&IIlbqcb->ii_lq_sqlca->sqlerrd[0];
                else
		    dbiq.db_data = (PTR)&lbqerr->ii_er_other;
		break;

    	case isIIRET:		/* iiret */
		dbiq.db_datatype = DB_INT_TYPE;
		dbiq.db_length = sizeof(i4);
		dbiq.db_data = (PTR)&IIlbqcb->ii_lq_rproc;
    		break;

    	case isERTEXT: 		/* errortext */
		/* If LIBQ has no error text saved return a blank */
		dbiq.db_datatype = DB_CHR_TYPE;
		if (lbqerr->ii_er_sbuflen == 0)
		{
	    	    dbiq.db_data = (PTR)ERx("");
	    	    dbiq.db_length = 0;
		}
		else
		{
		    dbiq.db_data = lbqerr->ii_er_smsg;
		    dbiq.db_length = STlength(lbqerr->ii_er_smsg);

	            /*
	            ** Add carriage return in case we are on 
	            ** VMS called from non-C language
	            */
#ifdef VMS
	            if (IIlbqcb->ii_lq_host == EQ_FORTRAN)
	            {
		        while ((fptr = STindex(dbiq.db_data,ERx("\n"),0)) 
				 != NULL)
		    	*fptr = ' ';
	            }
	            else if(IIlbqcb->ii_lq_host != EQ_C)
	            {
	                eptr = lbqerr->ii_er_smsg;
	                fptr = fmterr;
	                while (*eptr != EOS)
	                {
	    	            if (*eptr == '\n')
	    	            {
		    	        *fptr = '\r';
			        fptr++;
		            }
		            CMcpyinc(eptr,fptr);
	                }
	                CMcpychar(eptr,fptr);
	                dbiq.db_data = fmterr;
	                dbiq.db_length = STlength(fmterr);
	            }

#endif /* VMS */
	        }
		break;

    	case isERSEV :		/* errorseverity */
		/*
		** Errorseverity is currently copied to the user variable.
		** It is not confirmed to be of any documented value.
		** When this is done, we should modify iierror.c to set the
		** correct error status.
		*/
		dbiq.db_datatype = DB_INT_TYPE;
		dbiq.db_length = sizeof(i4);
		dbiq.db_data = (PTR)&lbqerr->ii_er_estat;
		break;

    	case isMSGTXT:		/* messagetext */
		dbiq.db_datatype = DB_CHR_TYPE;
		if (lbqerr->ii_er_mbuflen == 0 || lbqerr->ii_er_msg[0] == '\0')
		{
	    	    dbiq.db_data = (PTR)ERx("");
	    	    dbiq.db_length = 0;
		}
	        else
	        {
	            dbiq.db_data = lbqerr->ii_er_msg;
	            dbiq.db_length = STlength(lbqerr->ii_er_msg);
	        }
		break;

    	case isHISTMP:		/* highstamp */
		dbiq.db_datatype = DB_INT_TYPE;
		dbiq.db_length = sizeof(i4);
		dbiq.db_data = (PTR)&IIlbqcb->ii_lq_stamp[0];
		break;

    	case isLOSTMP:		/* lowstamp */
		dbiq.db_datatype = DB_INT_TYPE;
		dbiq.db_length = sizeof(i4);
		dbiq.db_data = (PTR)&IIlbqcb->ii_lq_stamp[1];
		break;
    	case siSAVEST:		/* savestate */
		dbiq.db_datatype = DB_CHR_TYPE;
		if (IIlbqcb->ii_lq_gca && IIlbqcb->ii_lq_gca->cgc_savestate)
		{
	    	    dbiq.db_data = (PTR)IIlbqcb->ii_lq_gca->cgc_savestate;
	    	    dbiq.db_length = STlength((char *)dbiq.db_data);
		}
		else
		{
	    	    dbiq.db_data = (PTR)ERx("");
	    	    dbiq.db_length = 0;
		}
		break;

    	case isTRANS:		/* transaction */
		dbiq.db_datatype = DB_INT_TYPE;
		dbiq.db_length = sizeof(i4);
		itemp = IIlbqcb->ii_lq_flags & II_L_XACT ? 1 : 0;
		dbiq.db_data = (PTR)&itemp;
		break;

    	case isDIFARCH:		/* diffarch */
		dbiq.db_datatype = DB_INT_TYPE;
		dbiq.db_length = sizeof(i4);
		if (IIlbqcb->ii_lq_gca)
	    	    itemp = IIlbqcb->ii_lq_gca->cgc_g_state & IICGC_HET_MASK ? 
			1 : 0;
		else
		    itemp = 0;
		dbiq.db_data = (PTR)&itemp;
		break;

    	case siERTYPE:		/* errortype */
		dbiq.db_datatype = DB_CHR_TYPE;
		dbiq.db_data = IIlbqcb->ii_lq_error.ii_er_flags & II_E_DBMS ? 
			       ERx("dbmserror") :
			       ERx("genericerror");    /* generic is default */
		dbiq.db_length = STlength(dbiq.db_data);
		break;

    	case siSESSION:		/* session */
		dbiq.db_datatype = DB_INT_TYPE;
		dbiq.db_length = sizeof(i4);
		itemp = IIlbqcb->ii_lq_sid;
		dbiq.db_data = (PTR)&itemp;
		break;

    	case isDBEVASY:		/* dbeventasync */
		dbiq.db_datatype = DB_INT_TYPE;
		dbiq.db_length = sizeof(i4);
		itemp = IIlbqcb->ii_lq_event.ii_ev_flags & II_EV_ASYNC ? 1 : 0;
		dbiq.db_data = (PTR)&itemp;
		break;

    	case siPRQUIT:		/* programquit */
		dbiq.db_datatype = DB_INT_TYPE;
		dbiq.db_length = sizeof(i4);
		itemp = (IIglbcb->ii_gl_flags & II_G_PRQUIT) ? 1 : 0;
		dbiq.db_data = (PTR)&itemp;
		break;
		
    	case siPFROWS:		/* prefetchrows */
		dbiq.db_datatype = DB_INT_TYPE;
		dbiq.db_length = sizeof(i4);
		itemp = IIlbqcb->ii_lq_csr.css_pfcgrows;
		dbiq.db_data = (PTR)&itemp;
		break;

    	case siSAVEQRY:		/* savequery */
		dbiq.db_datatype = DB_INT_TYPE;
		dbiq.db_length = sizeof(i4);
		itemp = (IIglbcb->ii_gl_flags & II_G_SAVQRY) ? 1 : 0;
		dbiq.db_data = (PTR)&itemp;
		break;

    	case siSAVEPASSWD:		/* savepassword */
		dbiq.db_datatype = DB_INT_TYPE;
		dbiq.db_length = sizeof(i4);
		itemp = IIglbcb->ii_gl_flags & II_G_SAVPWD ? 1 : 0;
		dbiq.db_data = (PTR)&itemp;
		break;

    	case isQRYTXT:		/* querytext */
		dbiq.db_datatype = DB_CHR_TYPE;
		if ( ! (IIglbcb->ii_gl_flags & II_G_SAVQRY)  ||
            	     IIdbg_getquery( IIlbqcb, &dbiq ) != OK )
        	{
            	    /* If savequery text feature is not on or there is nothing
            	    ** in the query buffer, set the null indicator
            	    ** and don't change the host variable data.
            	    */
            	    if (indaddr)
                	*indaddr = -1;	 /* Set null indicator to on */
            	    return;
        	}
        	if (indaddr)
            	    *indaddr = 0;	/* Got query text, set null ind off */
		break;

	case isOBJKEY:          /* logical key value - object_key */
		if (IIlbqcb->ii_lq_logkey.ii_lg_flags & II_LG_OBJKEY)
                {
                    dbiq.db_datatype = DB_CHA_TYPE;
                    dbiq.db_length = DB_LEN_OBJ_LOGKEY;
                    dbiq.db_data = IIlbqcb->ii_lq_logkey.ii_lg_key;
                    if (indaddr)
                        *indaddr = 0;          /* Set null indicator off */
		    /*
		    ** If user variable is C-type null terminated
		    ** string, modify type and length of user's variable
		    ** so it will hold logical key data which can 
	   	    ** contain embedded null data.
		    */
		    if (type == DB_CHR_TYPE)
		    {
			type = DB_CHA_TYPE;
			if (len == 0)		/* Just a string pointer */
                    	    len = DB_LEN_OBJ_LOGKEY;	/* Copy whole key */
			else
			    ++len;		/* Data won't be null term */
		    }
                }
                else
                {
                    if (indaddr)
                        *indaddr = -1;          /* Set null indicator on */
                    return;
                }
                break;

	case isTABKEY:          /* logical key value - table_key */
                if (IIlbqcb->ii_lq_logkey.ii_lg_flags & II_LG_TABKEY)
                {
                    dbiq.db_datatype = DB_CHA_TYPE;
                    dbiq.db_length = DB_LEN_TAB_LOGKEY;
                    dbiq.db_data = IIlbqcb->ii_lq_logkey.ii_lg_key;
                    if (indaddr)
                        *indaddr = 0;          /* Set null indicator off */
		    /*
		    ** If user variable is C-type null terminated
		    ** string, modify type and length of user's variable
		    ** so it will hold logical key data which can 
	   	    ** contain embedded null data.
		    */
		    if (type == DB_CHR_TYPE)
		    {
			type = DB_CHA_TYPE;
			if (len == 0)		/* Just a string pointer */
                    	    len = DB_LEN_TAB_LOGKEY;	/* Copy whole key */
			else
			    ++len;		/* Data won't be null term */
		    }
                }
                else
                {
                    if (indaddr)
                        *indaddr = -1;          /* Set null indicator on */
                    return;
                }
                break;

    	case isDBEVNAME:	/* dbeventname */
		dbiq.db_datatype = DB_CHR_TYPE;
		if (IIlbqcb->ii_lq_event.ii_ev_list == (II_EVENT *)0 ||
		    !(IIlbqcb->ii_lq_event.ii_ev_flags & II_EV_CONSUMED))
		{
		    /* Return a blank name if no consumed event on queue */  
	    	    dbiq.db_data = (PTR)ERx("");
	    	    dbiq.db_length = 0;
		}
		else
		{
		    dbiq.db_data = IIlbqcb->ii_lq_event.ii_ev_list->iie_name;
		    dbiq.db_length = STlength(IIlbqcb->ii_lq_event.ii_ev_list->iie_name);
		}
		break;

    	case isDBEVOWNER:	/* dbeventowner */
		dbiq.db_datatype = DB_CHR_TYPE;
		if (IIlbqcb->ii_lq_event.ii_ev_list == (II_EVENT *)0 ||
		    !(IIlbqcb->ii_lq_event.ii_ev_flags & II_EV_CONSUMED))
		{
		    /* Return a blank owner if no consumed event on queue */  
	    	    dbiq.db_data = (PTR)ERx("");
	    	    dbiq.db_length = 0;
		}
		else
		{
		    dbiq.db_data = IIlbqcb->ii_lq_event.ii_ev_list->iie_owner;
		    dbiq.db_length = STlength(IIlbqcb->ii_lq_event.ii_ev_list->iie_owner);
		}
		break;

    	case isDBEVDB:		/* dbeventdatabase */
		dbiq.db_datatype = DB_CHR_TYPE;
		if (IIlbqcb->ii_lq_event.ii_ev_list == (II_EVENT *)0 ||
		    !(IIlbqcb->ii_lq_event.ii_ev_flags & II_EV_CONSUMED))
		{
		    /* Return a blank database if no consumed event on queue */ 
	    	    dbiq.db_data = (PTR)ERx("");
	    	    dbiq.db_length = 0;
		}
		else
		{
		    dbiq.db_data = IIlbqcb->ii_lq_event.ii_ev_list->iie_db;
		    dbiq.db_length = STlength(IIlbqcb->ii_lq_event.ii_ev_list->iie_db);
		}
		break;

    	case isDBEVTIME:	/* dbeventtime */
		if (IIlbqcb->ii_lq_event.ii_ev_list == (II_EVENT *)0 ||
		    !(IIlbqcb->ii_lq_event.ii_ev_flags & II_EV_CONSUMED))
		{
		    /* Return a blank time if no consumed event on queue */  
		    dbiq.db_datatype = DB_CHR_TYPE;
	    	    dbiq.db_data = (PTR)ERx("");
	    	    dbiq.db_length = 0;
		}
		else
		{
		    dbiq.db_datatype = DB_DTE_TYPE;
		    dbiq.db_data = 
			IIlbqcb->ii_lq_event.ii_ev_list->iie_time.db_date;
		    dbiq.db_length = DB_DTE_LEN;
		}
		break;

    	case isDBEVTEXT:	/* dbeventtext */
		dbiq.db_datatype = DB_CHR_TYPE;
		if (IIlbqcb->ii_lq_event.ii_ev_list == (II_EVENT *)0 ||
		    !(IIlbqcb->ii_lq_event.ii_ev_flags & II_EV_CONSUMED))
		{
		    /* Return blank text if no consumed event on queue */ 
	    	    dbiq.db_data = (PTR)ERx("");
	    	    dbiq.db_length = 0;
		}
		else
		{
		    /* Also, return blank text if no event text.
		    ** Warning: there is no way the user can
		    ** tell the difference between no event and
		    ** an event with no text.   Use must look at
		    ** the event name to distinguish this.
		    */
		    if (IIlbqcb->ii_lq_event.ii_ev_list->iie_text == (char *)0)
		    {
			dbiq.db_data = (PTR)ERx("");
			dbiq.db_length = 0;
		    }
		    else
		    {
		    	dbiq.db_data = IIlbqcb->ii_lq_event.ii_ev_list->iie_text;
		    	dbiq.db_length = 
			    STlength(IIlbqcb->ii_lq_event.ii_ev_list->iie_text);
		    }
		}
		break;

	case isCOLNAME:		/* columnname */
		dbiq.db_datatype = DB_CHR_TYPE;
                if (IIlbqcb->ii_lq_lodata.ii_lo_name[0] == EOS)
		{
		    dbiq.db_data = (PTR)ERx("");
		    dbiq.db_length = 0;
		}
		else
		{
		    dbiq.db_data = IIlbqcb->ii_lq_lodata.ii_lo_name;
		    dbiq.db_length = STlength(IIlbqcb->ii_lq_lodata.ii_lo_name);
		}
		break;

        case isCOLTYPE:        /* columntype */
                dbiq.db_datatype = DB_INT_TYPE;
                dbiq.db_length = sizeof(i4);
		if (IIlbqcb->ii_lq_lodata.ii_lo_flags & II_LO_GETHDLR)
                	itemp = IIlbqcb->ii_lq_lodata.ii_lo_datatype;
		else
			itemp = 0;
                dbiq.db_data = (PTR)&itemp;
                break;

	case isCONTARG:		/* connection_target */
		dbiq.db_datatype = DB_CHR_TYPE;
                if (IIlbqcb->ii_lq_con_target == EOS)
		{
		    dbiq.db_data = (PTR)ERx("");
		    dbiq.db_length = 0;
		}
		else
		{
		    dbiq.db_data = IIlbqcb->ii_lq_con_target;
		    dbiq.db_length = STlength(IIlbqcb->ii_lq_con_target);
		}
		break;
	case siCONNAME:		/* connection_name */
		dbiq.db_datatype = DB_CHR_TYPE;
                if (IIlbqcb->ii_lq_con_target == EOS)
		{
		    dbiq.db_data = (PTR)ERx("");
		    dbiq.db_length = 0;
		}
		else
		{
		    dbiq.db_data = IIlbqcb->ii_lq_con_name;
		    dbiq.db_length = STlength(IIlbqcb->ii_lq_con_name);
		}
		break;

    	default:
		return;
    }

    /* There may not be an ADF_CB set up */
    if (IIlbqcb->ii_lq_adf == NULL)
	if (IILQasAdfcbSetup( thr_cb ) != OK)
	    return;

    /* Build an EDV from caller's embedded variable information */
    edv.ed_type = (i2)type;
    edv.ed_length = (i2)len;
    edv.ed_data = (PTR)addr;
    edv.ed_null = indaddr;

    stat = adh_dbcvtev(IIlbqcb->ii_lq_adf, &dbiq, &edv);
    /* EQUEL - Does not check attributes at pre-process time */
    if (stat != E_DB_OK)
    {
	IIlocerr((i4)(GE_DATA_EXCEPTION + GESC_ASSGN), 
		 E_LQ00A4_ATTVALTYPE, II_ERR, 0, (char *)0);
    }
}

/*{
+*  Name: IIeqiqio - Cover routine for IILQisInqSqlio
**
**  Description:
**      This routine is here for backward compatibility, so that old
**      programs may be relinked and still work.  It merely calls
**      the newer INQUIRE routine, IILQisInqSqlio().
**
**  Inputs:
**      inaddr  - i2 *  - Address of null indicator
**      isvar   - i4    - Is this a variable (better be).
**      type    - i4    - Host type of variable.
**      len     - i4    - Lenth of variable.
**      addr    - char *- Address of variable.
**      name    - char *- Name of attribute.
**
**  Outputs:
**      Returns:
**          VOID
**  History:
**	15-mar-1991 (kathryn)
**		Written.
**	22-apr-1991 (teresal)
**		Add event information.  Although events are SQL only, need
**		to add these event keywords here because 4GL is still using
**		the old interface. 
**	15-mar-1993 (kathryn)
**	    Added columnname and columntype.
*/

void
IIeqiqio(indaddr, isvar, type, len, addr, name)
i2      *indaddr;                       /* address of null indicator */
i4      isvar;                          /* flag - variable? unused */
i4      type;                           /* variable data type */
i4      len;                            /* length of data in bytes */
PTR     addr;                           /* address of var */
char    *name;                          /* name of field to return */
{
    register char       *nm;
    char                nmbuf[II__OBJLEN+1];
    i4			attrib;
    

    if ((nm = IIstrconv(II_CONV, name, nmbuf, II__OBJLEN)) == (char *)0)
	    return;
    
    if (STcompare(nm, ERx("rowcount")) == 0)
	attrib = siROWCNT;
    else if (STcompare(nm, ERx("errorno")) == 0)
	attrib = siERNO;
    else if (STcompare(nm, ERx("messagenumber")) == 0)
	attrib = isMSGNO;
    else if (STcompare(nm, ERx("endquery")) == 0)
	attrib = isENDQRY;
    else if (STcompare(nm, ERx("dbmserror")) == 0)
	attrib = siDBMSER;
    else if (STcompare(nm, ERx("iiret")) == 0)	/* Internally generated */
	attrib = isIIRET;
    else if (STcompare(nm, ERx("errortext")) == 0)
	attrib = isERTEXT;
    else if (STcompare(nm, ERx("errorseverity")) == 0)
	attrib = isERSEV;
    else if (STcompare(nm, ERx("messagetext")) == 0)
	attrib = isMSGTXT;
    else if (STcompare(nm, ERx("highstamp")) == 0)
	attrib = isHISTMP;
    else if (STcompare(nm, ERx("lowstamp")) == 0)
	attrib = isLOSTMP;
    else if (STcompare(nm, ERx("savestate")) == 0)
	attrib = siSAVEST;
    else if (STcompare(nm, ERx("transaction")) == 0)
	attrib = isTRANS;
    else if (STcompare(nm, ERx("diffarch")) == 0)
	attrib = isDIFARCH;
    else if (STcompare(nm, ERx("errortype")) == 0)
	attrib = siERTYPE;
    else if (STcompare(nm, ERx("session")) == 0)
	attrib = siSESSION;
    else if (STcompare(nm, ERx("dbeventasync")) == 0)
	attrib = isDBEVASY;
    else if (STcompare(nm, ERx("programquit")) == 0)
	attrib = siPRQUIT;
    else if (STcompare(nm, ERx("prefetchrows")) == 0)
	attrib = siPFROWS;
    else if (STcompare(nm, ERx("savequery")) == 0)
	attrib = siSAVEQRY;
    else if (STcompare(nm, ERx("querytext")) == 0)
	attrib = isQRYTXT;
    else if (STcompare(nm, ERx("table_key")) == 0)
        attrib = isTABKEY;
    else if (STcompare(nm, ERx("object_key")) == 0)
        attrib = isOBJKEY;
    else if (STcompare(nm, ERx("dbeventname")) == 0)
        attrib = isDBEVNAME;
    else if (STcompare(nm, ERx("dbeventowner")) == 0)
        attrib = isDBEVOWNER;
    else if (STcompare(nm, ERx("dbeventdatabase")) == 0)
        attrib = isDBEVDB;
    else if (STcompare(nm, ERx("dbeventtime")) == 0)
        attrib = isDBEVTIME;
    else if (STcompare(nm, ERx("dbeventtext")) == 0)
        attrib = isDBEVTEXT;
    else if (STcompare(nm, ERx("columnname")) == 0)
	attrib = isCOLNAME;
    else if (STcompare(nm, ERx("columntype")) == 0)
	attrib = isCOLTYPE;
    else if (STcompare(nm, ERx("connection_target")) == 0)
        attrib = isCONTARG;
    else if (STcompare(nm, ERx("connection_name")) == 0)
        attrib = siCONNAME;
    else
    	IIlocerr(GE_SYNTAX_ERROR, E_LQ00A0_ATTNOINQ, II_ERR, 1, nm);
    IILQisInqSqlio(indaddr, isvar, type, len, addr, attrib);
}

/*{
+*  Name: IIeqinq - Cover routine for IIeqiqio
**
**  Description:
**	This routine is here for backward compatibility, so that old
**	programs may be relinked and still work.  It merely calls
**	the newer INQUIRE routine, IIeqiqio().
**
**  Inputs:
**	isvar		- i4	- Is this a variable (better be).
**	type		- i4	- Host type of variable.
**	len		- i4	- Length of variable.
**	addr		- char *- Address of variable.
**	name		- char *- Name of attribute.
**
**  Outputs:
**	Returns:
**	    Nothing
-*
**  Side Effects:
**	
**  History:
**	27-feb-1987 - written (bjb)
*/

void
IIeqinq(isvar, type, len, addr, name)
i4	isvar;				/* flag - variable? unused */
i4	type;				/* variable data type */
i4	len;				/* length of data in bytes */
PTR	addr;				/* address of var */
char	*name;				/* name of field to return */
{
    IIeqiqio( (i2 *)0, isvar, type, len, addr, name );
}

/*{
+*  Name: IILQssSetSqlio - Sets information maintained by LIBQ.
**
**  Description:
**	This routine allows callers to set some information maintained by LIBQ.
**	It is easily expandable to set more information.
**
**	Currently only integer and string objects can be set by users.
**
**	NOTE:  This routine is not set up in quite the same way as other
**	INPUT routines in that it does not build an EDV and convert to a
**	DBV.  Why?  Because the objects to be set are not (yet) represented
**	as DB_DATA_VALUES.  However, we may expect DBVs as input to this
**	routine, so in order to use the data referenced by a DBV, we map 
**	the DBV into a local EDV (whose .ed_data field we may "legally" 
**	use).
**
**	Although the interface supports null indicators, they are
**	currently ignored.
**
**  Inputs:
**	attrib 	- i4    - Integer representing attribute name.
**	indaddr - i2 *  - Address of indicator variable (unused)
**	isvar	- i4	- Is this by reference or by value.
**	type	- i4	- Host type of variable.
**	len	- i4	- Lenth of variable.
**	data	- char *- Address of variable or value.
**
**  Outputs:
**	Returns:
**	    Nothing
**	Errors:
**	    SIAT errors from the form system, flagging bad atributes on
**	    SET/INQUIRE.
**	    SIATTR	- Bad attribute used.
**	    SIATNUM	- Number expected, but character string used.
**	    SIATCHAR	- Character string expected, but number used.
-*
**  Side Effects:
**	
**  History:
**	14-oct-1983	- Written. (ncg)
**	27-feb-1987	- Modified interface (bjb)
**	01-sep-1988	- Added saved state setting. (ncg)
**	06-sep-1988	- Added printqry setting. (ncg)
**	09-sep-1988	- Added printgca setting. (ncg)
**	11-may-1989	- Added errortype (a user setting). (bjb)
**	19-may-1989	- Modified names of global vars and added SESSION for
**			  multiple connects. (bjb)
**	06-dec-1989	- Added EVENTDISPLAY for Phoenix/Alerters. (bjb)
**	30-apr-1990	- Added prefetchrows & printcsr setting. (ncg)
**	14-jun-1990	(barbara)
**	    Added PROGRAMQUIT for backwards compatibilty after bug fix 21832.
**	    LIBQ now continues after network failures, but setting this
**	    flag causes it to quit as before.
**      15-mar-1991 (kathryn)
**          Moved functionality from IIeqstio to IILQssSetSqlio
**          and made IIeqstio a cover to this routine.  The pre-processor now
**          evaluates attributes, and generates calls to this routine with an
**          integer constant representing the attribute rather than the
**          attribute name as a string. 
**      14-mar-1991 (teresal)
**          Add support for SET "savequery".  Also, fixed "printqry"
**          and "printgca" so SET_INGRES can be issued outside
**          of a connected session.
**	13-mar-1991 (barbara)
**	    Added PRINTTRACE for directing server trace messages to a file.
**	10-apr-1991 (kathryn)
**	    Added:
**		GCAFILE file name for gca trace messages (printgca).
**		QRYFILE file name for query trace messages (printqry).
**		TRCFILE file name for server trace messages (printtrace).
**	09-may-1991 (kathryn)
**	    Build a correct DB_EMBEDDED_DATA type from DB_DATA_VALUE
**	    (DB_DBV_TYPE).  Add check to ensure that the DB_DATA_VALUE data 
**	    type is correct for the attribute. Datatype of DB_DATA_VALUE is not 
**	    known until runtime, so cannot be checked at pre-process time.
**      01-nov-1991  (kathryn)
**          Trace file information (printqry,printgca,printtrace) now stored 
**	    in new IILQ_TRACE structure of IIglbcb rather than directly in 
**	    IIglbcb.  
**	15-dec-1991 (kathryn)
**	    Correct handling of trace flags when setting "printqry" on.
**	10-nov-92 (davel)
**	    Added siSESSION (for session) to the switch statement for 
**	    DB_DATA_VALUEs, as this can be set from 4GL.
**	17-nov-92 (larrym)
**	    Added "default" to switch statement to check for attribute that 
**	    can't be set by a DB_DATA_VALUE and print an error.  
**	15-jun-1993 (larrym)
**	    cleaned up a lot of the session switching code (for bug 50756)
**	    the code that actually does the session switching has been
**	    moved into iilqsess.c, which this now calls.  Error
**	    reporting for sessions that don't exist has been moved from 
**	    iilqsess.c into here.    See iilqsess.c for more info.
**	15-sep-1993 (kathryn)
**	    Add set_sql columntype. This statement is valid only when issued
**	    from within large object PUT datahandlers.
**	18-Sep-2003 (bonro01)
**	    For i64_hpu added a length check for 8-byte integers.
**	06-oct-2003 (penga03)
**	    Added #ifdef LP64...#endif surrounded the statements using i8.
**	26-jan-2004 (sheco02)
**	    Since i8 is defined for all platforms in main now, remove the pre-
**	    vious #ifdef/#endif. Remove the extra #endif too.
**	23-Jun-2004 (karbh01) (b112481) (p2859)
**	    Cast char pointer to (i8 *).
**	25-May-2005 (karbh01) (b114571) (INGEMB141)
**	    remove changes for b112481. Correct change in cobgen.c.
**	13-Jul-2005 (schka24)
**	    x-integration screwed up above, fix.
**      11-Jun-2008 (horda03) Bug 112481
**          In a 64bit application when the "data" parameter is passed
**          as an integer values (isvar FALSE), then casting the
**          address of data as an i4 pointer to otain the value may
**          not retrieve the value passed; as the referenced value
**          could be the high order bits of the I8 parameter. This
**          has been observed on RS4 and SU9 platforms. Typically
**          occurs when COBOL embedded applications uses EXEC SQL
**          SET_SQL(SESSION=1).
*/

void
IILQssSetSqlio(attrib, indaddr, isvar, type, len, data)
i4      attrib;                         /* name of field to set */
i2      *indaddr;                       /* Address of null indicator - unused */
i4      isvar;                          /* flag - variable? */
i4      type;                           /* variable data type */
i4      len;                            /* length of data in bytes */
PTR     data;                           /* union of data type ptrs */
{
    II_THR_CB		*thr_cb = IILQthThread();
    II_LBQ_CB		*IIlbqcb = thr_cb->ii_th_session;
    DB_EMBEDDED_DATA    edv;
    DB_STATUS           stat;
    register char       *nm;
    i4                  tmpnat;
    i4			rtype;
    i4			savtype;
    DB_TEXT_STRING	*txtptr;
    char                *tmpchar;
    IILQ_TRACE		*tmptrace;
    DB_DATA_VALUE       dbv;
    struct {                            /* Temp buffer for char conversions */
        i2      txlen;
        char    txbuf[IICGC_SV_LEN+1];
    } txt; 
    II_LBQ_CB           *sw_state;      /* SESSION switched to */
    char		ebuf[20];	/* error attribute buffer */

    /* There may not be an ADF_CB set up */
    if (IIlbqcb->ii_lq_adf == NULL)
	if (IILQasAdfcbSetup( thr_cb ) != OK)
	    return;	

    edv.ed_type = (i2)type;
    edv.ed_length = (i2)len;
    edv.ed_null = indaddr;          /* Not used, actually */
    if (isvar)
        edv.ed_data = (PTR)data;
    else	
	edv.ed_data = (PTR)&data;

    if (isvar && type == DB_DBV_TYPE)
    {
	savtype = abs(((DB_DATA_VALUE *)data)->db_datatype);

	/* Ensure that DB_DBV_TYPE is correct datatype for attribute.
	** DB_DBV_TYPES cannot be checked at pre-process time.
	*/
	switch (attrib)
	{
	    case isCOLTYPE:
	    case ssERMODE:
	    case ssERDISP:
	    case siROWCNT:
	    case ssPRTQRY:
	    case ssPRTGCA:
	    case ssDBEVDISP:
	    case siPFROWS:
	    case ssPRTCSR:
	    case siPRQUIT:
	    case siERNO:
	    case siDBMSER:
	    case siSAVEQRY:
	    case ssPRTTRC:    
	    case siSESSION:    
		if (savtype != DB_INT_TYPE)
		{
		    IIlocerr((i4)(GE_DATA_EXCEPTION + GESC_ASSGN),
		            E_LQ00A4_ATTVALTYPE, II_ERR, 0, (char *)0);
		    return;
		}
		adh_dbtoev(IIlbqcb->ii_lq_adf,(DB_DATA_VALUE *)data,&edv);
		edv.ed_data = ((DB_DATA_VALUE *)data)->db_data;
		rtype = DB_INT_TYPE;
		break;

	    case siSAVEST:
	    case siERTYPE:
	    case ssGCAFILE:
	    case ssQRYFILE:
	    case ssTRCFILE:
	    case siCONNAME:    
		rtype = DB_CHR_TYPE;
		break;
	    default:
		CVna(attrib,ebuf);
		IIlocerr((i4)(GE_DATA_EXCEPTION + GESC_ASSGN),
		            E_LQ00A1_ATTNOSET, II_ERR, 1, ebuf);
		return;
	}
    }
    else
    {
	rtype = edv.ed_type;
    }
    if (rtype != DB_INT_TYPE)
    {
	    txt.txlen = 0;
	    II_DB_SCAL_MACRO(dbv, DB_TXT_TYPE, sizeof(txt)-1, &txt);
	    stat = adh_evcvtdb(IIlbqcb->ii_lq_adf, &edv, &dbv, FALSE);
	    if (stat != OK)
	    {
	        IIlocerr((i4)(GE_DATA_EXCEPTION + GESC_ASSGN), 
		         E_LQ00A4_ATTVALTYPE, II_ERR, 0, (char *)0);
	        return;
	    }
	txt.txbuf[txt.txlen] = EOS;
	tmpchar = txt.txbuf;
	
    }

    /* Make sure we're accessing the right bytes in following comparisons */
    else
    {
#ifdef LP64
        /* This is a 64 bit application, so if "data" does not reference a
        ** variable (isvar == FALSE) then the numeric value is probably in
        ** i8 format; so need to ensure don't reference the wrong part of
        ** the value supplied.
        */
        if (!isvar)
        {
           tmpnat = (i4) data;
        }
        else
#endif

	switch (edv.ed_length)
	{
	  case 1:
	    tmpnat = *(i1 *)edv.ed_data;
	    break;
	  case 2:
	    tmpnat = *(i2 *)edv.ed_data;
	    break;
	  case 4:
	    tmpnat = *(i4 *)edv.ed_data;
	    break;
	  case 8:
	    tmpnat = *(i8 *)edv.ed_data;
	    break;
	}
    }

    switch (attrib)
    {
      case isCOLTYPE:
	if (IIlbqcb->ii_lq_lodata.ii_lo_flags & II_LO_PUTHDLR)
	{
	    switch (abs(tmpnat))
	    {
		case DB_LBYTE_TYPE:
		case DB_GEOM_TYPE:
		case DB_POINT_TYPE:
		case DB_MPOINT_TYPE:
		case DB_LINE_TYPE:
		case DB_MLINE_TYPE:
		case DB_POLY_TYPE:
		case DB_MPOLY_TYPE:
		case DB_GEOMC_TYPE:
		case DB_LVCH_TYPE:
		case DB_LBIT_TYPE:
			IIlbqcb->ii_lq_lodata.ii_lo_datatype = tmpnat;
			break;
		default:
		    CVna( tmpnat, ebuf );
		    IIlocerr( GE_SYNTAX_ERROR, E_LQ00A3_ATTSETDATA, II_ERR, 2,
		   	ERx("columntype"), ebuf );
		    break;
	    }
	}
	else
	{
	    IIlocerr(GE_SYNTAX_ERROR, E_LQ00E5_PUTHDLRONLY,
				II_ERR, 1, ERx("SET_SQL(columntype)"));
	}
	return;
	      
      case ssERMODE:	/* Silence / normalize error messages */
	if (tmpnat > 0)
	    IIglbcb->ii_gl_seterr = IIret_err;
	else
	    IIglbcb->ii_gl_seterr = IIno_err;
	return;

      case ssERDISP:	/* Print last error message */
	/*
	** Temporarily turn on error messages, and restore 
	** after printing.
	*/
	if (tmpnat > 0 && IIlbqcb->ii_lq_error.ii_er_num != IIerOK &&
	    IIlbqcb->ii_lq_error.ii_er_smsg)
	{
	    IImsgutil(IIlbqcb->ii_lq_error.ii_er_smsg);
	}
	return;
	    
      case siROWCNT:	/* Set rowcount - may be overwritten by user */
	IIconvert((i4 *)edv.ed_data, &IIlbqcb->ii_lq_rowcount,
		  edv.ed_type, edv.ed_length, DB_INT_TYPE, sizeof(i4));
	return;

      case ssPRTQRY:	/* Toggle "printqry" debugging */
	if (tmpnat)
	    IIdbg_toggle( IIlbqcb, IITRC_ON );
	else
	    IIdbg_toggle( IIlbqcb, IITRC_OFF );
	return;

      case ssPRTGCA:	/* Toggle "printgca" tracing */
	if ( tmpnat )
	    IILQgstGcaSetTrace( IIlbqcb, IITRC_ON );
	else
	    IILQgstGcaSetTrace( IIlbqcb, IITRC_OFF );
        return;

      case siSAVEST:	/* Set GCA save state for spawning */
	if (IIlbqcb->ii_lq_gca)
	{
	    if (IIlbqcb->ii_lq_gca->cgc_savestate != (char *)0)
	    {
	        MEfree(IIlbqcb->ii_lq_gca->cgc_savestate);
	        IIlbqcb->ii_lq_gca->cgc_savestate = (char *)0;
	    }
	    if ((IIlbqcb->ii_lq_gca->cgc_savestate = STalloc(tmpchar))
					== (char *)0)
	    {
		IIlocerr(GE_NO_RESOURCE, E_LQ00A2_ATTBADTYPE, II_ERR, 1, 
		ERx("savestate"));
	    }
	}
	return;

      case siERTYPE:	/* Set default for error reporting */
	if (STbcompare(tmpchar, 0, ERx("dbmserror"), 9, TRUE) == 0)
	{
	    IIlbqcb->ii_lq_error.ii_er_flags |= II_E_DBMS; 
	    IIlbqcb->ii_lq_error.ii_er_flags &= ~II_E_GENERIC; 
	}
	else if (STbcompare(tmpchar, 0, ERx("genericerror"), 12, TRUE) == 0)
	{
	    IIlbqcb->ii_lq_error.ii_er_flags |= II_E_GENERIC; 
	    IIlbqcb->ii_lq_error.ii_er_flags &= ~II_E_DBMS; 
	}
	else
	    IIlocerr( GE_SYNTAX_ERROR, E_LQ00A3_ATTSETDATA, II_ERR, 2, 
			ERx("errortype"), tmpchar );
	return;

	case siSESSION:	/* Switch to another session using session_id */
	    /* note, XA checking is done in IILQfsSessionHandle */
	    if ( tmpnat == IIsqsetNONE )
		sw_state = NULL;
	    else  if ( ! (sw_state = IILQfsSessionHandle( tmpnat )) )
	    {
	        CVna( tmpnat, ebuf );
	        IIlocerr( GE_SYNTAX_ERROR, 
			  E_LQ00BE_SESSNOTFOUND, II_ERR, 1, ebuf );
		return;
	    }

	    IILQssSwitchSession( thr_cb, sw_state );  
	    return;

	case siCONNAME:	/* Switch to another connection using connection name */
	      /* note, XA checking is done in IILQscConnNameHandle */
	      sw_state = IILQscConnNameHandle( tmpchar );
	      if (sw_state == NULL)
	      {
		  IIlocerr(GE_SYNTAX_ERROR, E_LQ0042_CONNAMENOTFOUND, II_ERR, 
			   1, tmpchar);
		  return;
	      }
	      IILQssSwitchSession( thr_cb, sw_state );  
	      return;

	case ssDBEVDISP:	/* Print/don't print event info as it arrives */
	  if (tmpnat > 0)
	    IIlbqcb->ii_lq_event.ii_ev_flags |= II_EV_DISP;
	  else
	    IIlbqcb->ii_lq_event.ii_ev_flags &= ~II_EV_DISP;
	  return;

      case siPFROWS:	/* Set cursor pre-fetch row # */
	IIlbqcb->ii_lq_csr.css_pfrows = tmpnat;
	return;

      case siSAVEPASSWD:	/* Save password in default dbprompt */
	MUp_semaphore( &IIglbcb->ii_gl_sem );
        if (tmpnat)
	    IIglbcb->ii_gl_flags |= II_G_SAVPWD;
	else
	    IIglbcb->ii_gl_flags &= ~II_G_SAVPWD;
	MUv_semaphore( &IIglbcb->ii_gl_sem );
	return;

      case ssPRTCSR:	/* Set cursor tracing on/off */
	MUp_semaphore( &IIglbcb->ii_gl_sem );
        if (tmpnat)
	    IIglbcb->ii_gl_flags |= II_G_CSRTRC;
	else
	    IIglbcb->ii_gl_flags &= ~II_G_CSRTRC;
	MUv_semaphore( &IIglbcb->ii_gl_sem );
	return;

      case siPRQUIT:	/* Set cursor tracing on/off */
	MUp_semaphore( &IIglbcb->ii_gl_sem );
	if (tmpnat)
	    IIglbcb->ii_gl_flags |= II_G_PRQUIT;
	else
	    IIglbcb->ii_gl_flags &= ~II_G_PRQUIT;
	MUv_semaphore( &IIglbcb->ii_gl_sem );
	return;

      case siERNO:
        IIlbqcb->ii_lq_error.ii_er_num = tmpnat;
	return;

      case siDBMSER:
	IIlbqcb->ii_lq_error.ii_er_other = tmpnat;
	return;

      case siSAVEQRY:	/* Toggle savequery on or off */
	if ( tmpnat )
	    IIdbg_save( IIlbqcb, IITRC_ON );
	else
	    IIdbg_save( IIlbqcb, IITRC_OFF );
        return;

      case ssPRTTRC:	/* Toggle printtrace on or off */
	if ( tmpnat )
	    IILQsstSvrSetTrace( IIlbqcb, IITRC_ON );
	else
	    IILQsstSvrSetTrace( IIlbqcb, IITRC_OFF );
        return;

      case ssGCAFILE:
	STtrmwhite(tmpchar);
	tmptrace = &IIglbcb->iigl_msgtrc;
	MUp_semaphore( &tmptrace->ii_tr_sem );

	if ( tmptrace->ii_tr_file )		/* file already open */
	    IIUGerr( E_LQ00A5_TRACEFILE, 0, 2, tmpchar,tmptrace->ii_tr_fname );
	else
	{
	    if ( tmptrace->ii_tr_fname )  MEfree( tmptrace->ii_tr_fname );
	    if ( ! (tmptrace->ii_tr_fname = STalloc( tmpchar )) )
               IIlocerr( GE_NO_RESOURCE, 
		         E_LQ00A2_ATTBADTYPE, II_ERR, 1, ERx("gcafile") );
	}

	MUv_semaphore( &tmptrace->ii_tr_sem );
	return;

      case ssQRYFILE:
	STtrmwhite( tmpchar );
	tmptrace = &IIglbcb->iigl_qrytrc;
	MUp_semaphore( &tmptrace->ii_tr_sem );

	if ( tmptrace->ii_tr_file )		/* file already open */
	    IIUGerr( E_LQ00A5_TRACEFILE, 0, 2, tmpchar,tmptrace->ii_tr_fname );
	else
	{
	    if ( tmptrace->ii_tr_fname )  MEfree( tmptrace->ii_tr_fname );
	    if ( ! (tmptrace->ii_tr_fname = STalloc( tmpchar )) )
		IIlocerr( GE_NO_RESOURCE, 
			  E_LQ00A2_ATTBADTYPE, II_ERR, 1, ERx("qryfile") );
	}

	MUv_semaphore( &tmptrace->ii_tr_sem );
	return;

      case ssTRCFILE:
	STtrmwhite( tmpchar );
	tmptrace = &IIglbcb->iigl_svrtrc;
	MUp_semaphore( &tmptrace->ii_tr_sem );

	if ( tmptrace->ii_tr_file )	/* file already open */
	    IIUGerr(E_LQ00A5_TRACEFILE, 0, 2, tmpchar, tmptrace->ii_tr_fname);
	else
	{
	    if ( tmptrace->ii_tr_fname )  MEfree( tmptrace->ii_tr_fname );
	    if ( ! (tmptrace->ii_tr_fname = STalloc( tmpchar )) )
               IIlocerr( GE_NO_RESOURCE, 
		         E_LQ00A2_ATTBADTYPE, II_ERR, 1, ERx("tracefile") );
	}

	MUv_semaphore( &tmptrace->ii_tr_sem );
	return;
    }
}

/*{
+*  Name: IIeqstio - desc
**
**  Description:
**	This routine is here for backward compatibility, so that old
**	programs may be relinked and still work.  It merely calls
**	the newer SET routine, IILQssSetSqlio().
**
**
**  Inputs:
**	name 	- char *- Name of attribute.
**	isvar	- i4	- Is this by reference or by value.
**	type	- i4	- Host type of variable.
**	len	- i4	- Lenth of variable.
**	data	- char *- Address of variable or value.
**
**  Outputs:
**	Returns:
**	    Nothing
**
**  Side Effects:
**	
**  History:
**	15-mar-1991 (kathryn)
**		Written. 
**	16-mar-1991 (barbara)
**	    Added PRINTTRACE.
**	01-apr-1991 (kathryn)
**	    Check attribute type for EQUEL - only ESQL checks attribute value
**	    types at pre-process time.
**	10-apr-1991 (kathryn)
**	    Added GCAFILE, QRYFILE, and TRACEFILE.
**	09-may-1991 (kathryn)
**	    Correct the datatype/attribute check for non-DB_DATA_VALUE types.
**	    Pass DB_DATA_VALUES directly to IILQssSetSqlio , the datatype
**          will be checked in IILQssSetSqlio.	
**	15-sep-1993 (kathryn)
**	    Add COLTYPE as valid SET_SQL attribute.
*/

/*VARARGS5*/
void
IIeqstio(name, indaddr, isvar, type, len, data)
char	*name;				/* name of field to set */
i2	*indaddr;			/* Address of null indicator - unused */
i4	isvar;				/* flag - variable? */
i4	type;				/* variable data type */
i4	len;				/* length of data in bytes - unused */
PTR	data;				/* union of data type ptrs */
{
    i4			setmode;
    i4			rtype;
    DB_EMBEDDED_DATA    edv;
    register char	*nm;
    char		nmbuf[II__OBJLEN+1];

	int	stat;

    /*
    **	Get attribute name to inquire and the pointer to the
    **	current FRS object defined in IIsetinq.c.
    */
    if ((nm = IIstrconv(II_CONV, name, nmbuf, II__OBJLEN)) == (char *)0)
	return;

    /* Get name of attribute and look in the appropriate object */
    if (STcompare(nm, ERx("errormode")) == 0)
    {
	setmode = ssERMODE;
	rtype = DB_INT_TYPE;
    }
    else if (STcompare(nm, ERx("errordisp")) == 0)
    {
	setmode = ssERDISP;
	rtype = DB_INT_TYPE;
    }
    else if (STcompare(nm, ERx("rowcount")) == 0)
    {
	setmode = siROWCNT;
	rtype = DB_INT_TYPE;
    }
    else if (STcompare(nm, ERx("savestate")) == 0)
    {
	setmode = siSAVEST;
	rtype = DB_CHR_TYPE;
    }
    else if (STcompare(nm, ERx("printqry")) == 0)
    {
	setmode = ssPRTQRY;
	rtype = DB_INT_TYPE;
    }
    else if (STcompare(nm, ERx("printgca")) == 0)
    {
	setmode = ssPRTGCA;
	rtype = DB_INT_TYPE;
    }
    else if (STcompare(nm, ERx("errortype")) == 0)
    {
	setmode = siERTYPE;
	rtype = DB_CHR_TYPE;
    }
    else if (STcompare(nm, ERx("session")) == 0)
    {
	setmode = siSESSION;
	rtype = DB_INT_TYPE;
    }
    else if (STcompare(nm, ERx("dbeventdisplay")) == 0)
    {
	setmode = ssDBEVDISP;
	rtype = DB_INT_TYPE;
    }
    else if (STcompare(nm, ERx("prefetchrows")) == 0)
    {
	setmode = siPFROWS;
	rtype = DB_INT_TYPE;
    }
    else if (STcompare(nm, ERx("printcsr")) == 0)
    {
	setmode = ssPRTCSR;
	rtype = DB_INT_TYPE;
    }
    else if (STcompare(nm, ERx("programquit")) == 0)
    {
	setmode = siPRQUIT;
	rtype = DB_INT_TYPE;
    }
    else if (STcompare(nm, ERx("errorno")) == 0)
    {
	setmode = siERNO;
	rtype = DB_INT_TYPE;
    }
    else if (STcompare(nm, ERx("dbmserror")) == 0)
    {
	setmode = siDBMSER;
	rtype = DB_INT_TYPE;
    }
    else if (STcompare(nm, ERx("savequery")) == 0)
    {
	setmode = siSAVEQRY;
	rtype = DB_INT_TYPE;
    }
    else if (STcompare(nm, ERx("printtrace")) == 0)
    {
	setmode = ssPRTTRC;
	rtype = DB_INT_TYPE;
    }
    else if (STcompare(nm, ERx("gcafile")) == 0)
    {
	setmode = ssGCAFILE;
	rtype = DB_CHR_TYPE;
    }
    else if (STcompare(nm, ERx("qryfile")) == 0)
    {
	setmode = ssQRYFILE;
	rtype = DB_CHR_TYPE;
    }
    else if (STcompare(nm, ERx("tracefile")) == 0)
    {
	setmode = ssTRCFILE;
	rtype = DB_CHR_TYPE;
    }
    else if (STcompare(nm, ERx("connection_name")) == 0)
    {
	setmode = siCONNAME;
	rtype = DB_CHR_TYPE;
    }
    else if (STcompare(nm, ERx("columntype")) == 0)
    {
	setmode = isCOLTYPE;
	rtype = DB_INT_TYPE;
    }
    else
    {
	IIlocerr(GE_SYNTAX_ERROR, E_LQ00A1_ATTNOSET, II_ERR, 1, nm);
	return;
    }

    if (type != DB_DBV_TYPE)
    {
      if (rtype == DB_CHR_TYPE)
      {
	switch (type)
	{
	    case DB_CHA_TYPE:
	    case DB_CHR_TYPE:
	    case DB_TXT_TYPE:
	    case DB_VCH_TYPE:
			break;
		default:
			IIlocerr((i4)(GE_DATA_EXCEPTION + GESC_ASSGN),
			E_LQ00A2_ATTBADTYPE, II_ERR, 1, nm);
			return;
	}
      }
      else if (rtype == DB_INT_TYPE && type != rtype)
      {
	IIlocerr((i4)(GE_DATA_EXCEPTION + GESC_ASSGN),
		E_LQ00A2_ATTBADTYPE, II_ERR, 1, nm);
	return;
      }
   } 

    IILQssSetSqlio(setmode, indaddr, isvar, type, len, data);
}

/*{
+*  Name: IIeqset - desc
**
**  Description:
**	This routine is here for backward compatibility, so that old
**	programs may be relinked and still work.  It merely calls
**	the newer SET routine, IIeqstio().
**
**
**  Inputs:
**	name 	- char *- Name of attribute.
**	isvar	- i4	- Is this by reference or by value.
**	type	- i4	- Host type of variable.
**	len	- i4	- Lenth of variable.
**	data	- char *- Address of variable or value.
**
**  Outputs:
**	Returns:
**	    Nothing
**
**  Side Effects:
**	
**  History:
**	27-feb-1987	- Written. (bjb)
*/
void
IIeqset(name, isvar, type, len, data)
char	*name;				/* name of field to set */
i4	isvar;				/* flag - variable? */
i4	type;				/* variable data type */
i4	len;				/* length of data in bytes - unused */
PTR	data;				/* union of data type ptrs */
{
    IIeqstio( name, (i2 *)0, isvar, type, len, data );
}


/*{
+*  Name: IItuples - Return the row count.
**
**  Inputs:
**	None
**
**  Outputs:
**	Returns:
**	    Number of rows affected by last query.
**	Errors:
**	    None
-*
**  Side Effects:
**	
**  History:
**	dd-mmm-yyyy - text (init)
*/

i4
IItuples()
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;

    return IIlbqcb->ii_lq_rowcount;
}


/*{
+*  Name: IIerrnum - Return or Set the error number.
**
**  Inputs:
**	setting - i4	- True if setting.
**	errval	- i4	- Error number if setting.
**  Outputs:
**	Returns:
**	    i4 - Error number (if you've just set it, that's what you'll get).
**	Errors:
**	    None
-*
**  Side Effects:
**	1. If called to set the error number, then this will "forget" the
**	   current error number (if there was one).
**	
**  History:
**	05-dec-1986 - Written (ncg)
*/

i4
IIerrnum(setting, errval)
i4	setting;
i4	errval;
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;

    if (setting)
        IIlbqcb->ii_lq_error.ii_er_num = errval;
    return IIlbqcb->ii_lq_error.ii_er_num;
}

/*{
+*  Name: IILQshSetHandler - Set user defined handler.
**
**  Description:
**  	Set/Unset a user defined handler.  This call is generated by
**	Set_SQL(handler_name = function_ptr) where handler name may be:
**		errorhandler
**		dbeventhandler
**		messagehandler
**		dbprompthandler
**	And function_ptr may be a pointer to any host language function.
**
**  Inputs:
**      hdlr     - i4    	- Integer representing handler name.
**      funcptr  - FUNC_PTR    	- Function pointer to user handler.
**				  OR '0' to "unset".
**  Outputs:
**      Returns:
**          VOID.
**      Errors:
**          None
-*
**  History:
**      15-mar-1991 - (kathryn) Written.
**	25-mar-1991 (barbara)
**	    Added support for COPYHANDLER.
**	9-nov-93 (robf)
**          Added support for DBPROMPTHANDLER
*/

void
IILQshSetHandler(hdlr,funcptr)
i4	hdlr;
i4	(*funcptr)();
{
    switch (hdlr)
    {
	case siERHDLR:
	    	IIglbcb->ii_gl_hdlrs.ii_err_hdl = funcptr;	
		break;
	case siDBEVHDLR:
		IIglbcb->ii_gl_hdlrs.ii_evt_hdl = funcptr;
		break;
	case siMSGHDLR:
		IIglbcb->ii_gl_hdlrs.ii_msg_hdl = funcptr;
		break;
	case siCPYHDLR:
		IIglbcb->ii_gl_hdlrs.ii_cpy_hdl = funcptr;
		break;
	case siDBPRMPTHDLR:
		IIglbcb->ii_gl_hdlrs.ii_dbprmpt_hdl = funcptr;
		break;
	default:
		break;
    }
}
