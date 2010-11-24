/*
**Copyright (c) 2004 Ingres Corporation
*/

# include    <compat.h>
# include    <gl.h>
# include    <sl.h>
# include    <iicommon.h>
# include    <dbdbms.h>
# include    <cs.h>
# include    <st.h>
# include    <er.h>
# include    <ex.h>
# include    <tr.h>
# include    <me.h>
# include    <adf.h>
# include    <scf.h>
# include    <tm.h>
# include    <dmf.h>
# include    <dmrcb.h>
# include    <dmtcb.h>
# include    <ulf.h>
# include    <ulm.h>
# include    <sxf.h>

# include    <gwf.h>
# include    <gwfint.h>
# include    <gwftrace.h>
# include    "gwfsxa.h" 
GLOBALREF  ULM_RCB gwsxa_ulm; 
i4		 gwsxa_acctype_to_msgid(SXF_ACCESS acctype);
i4		 gwsxa_evtype_to_msgid(SXF_EVENT evtype);
VOID		 gwsxa_msgid_default(i4 msgid,DB_DATA_VALUE  *desc);

static DB_STATUS gwsxa_alloc_msgdesc(SXA_MSG_DESC **m);
static DB_STATUS gwsxa_add_msgid(i4 msgid, DB_DATA_VALUE  *desc);
static VOID 	 gwsxa_priv_to_str(i4 priv,DB_DATA_VALUE *pstr,bool term);
static STATUS   sxa_cvt_handler( EX_ARGS *ex_args );
static STATUS 	sxa_check_adferr( ADF_CB *adf_scb );

GLOBALREF CS_SEMAPHORE GWsxamsg_sem ; /* Semaphore to protect 
					       message list*/
GLOBALREF CS_SEMAPHORE GWsxa_sem ; /* Semaphore to protect general increments*/
GLOBALREF GW_FACILITY	*Gwf_facility;
GLOBALREF GWSXASTATS  GWsxastats;
/*
**	This is the root of the message id cache
*/
static SXA_MSG_DESC *mroot=NULL;
/*
** Name: GWSXCVT.C	- Conversion routines for SXA Gateway
**
** Description:
**	This file contains the routines which do the conversion between
**	SXF data and the output ingres records
**
** History:
**	17-sep-92 (robf)
**	    Created
**	20-nov-92 (robf)
**	    Added handling for objectowner column. 
**	17-dec-92 (robf)
**	    Updated error handling.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	11-oct-93 (swm)
**	    Bug #56448
**	    Made gwf_display() portable. The old gwf_display()
**	    took an array of variable-sized arguments. It could
**	    not be re-written as a varargs function as this
**	    practice is outlawed in main line code.
**	    The problem was solved by invoking a varargs function,
**	    TRformat, directly.
**	    The gwf_display() function has now been deleted, but
**	    for flexibilty gwf_display has been #defined to
**	    TRformat to accomodate future change.
**	    All calls to gwf_display() changed to pass additional
**	    arguments required by TRformat:
**	        gwf_display(gwf_scctalk,0,trbuf,sizeof(trbuf) - 1,...)
**	    this emulates the behaviour of the old gwf_display()
**	    except that the trbuf must be supplied.
**	    Note that the size of the buffer passed to TRformat
**	    has 1 subtracted from it to fool TRformat into NOT
**	    discarding a trailing '\n'.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      24-Feb-2004 (kodse01)
**          Removed gwxit.h inclusion which is not required.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**	03-Nov-2010 (jonj) SIR 124685 Prototype Cleanup
**	    Delete local scf_call() declaration.
*/


/*
** Name: gwsxa_sxf_to_ingres - Convert an SXF audit record to a
**	 format that ingres can understand.
**
** Inputs:
**	gwx_rcb		Extended RCB for the gateway
**	sxf_record	The SXF record that needs to be converted
**
** Outputs:
**	ingres_buffer - converted buffer
**
** Returns:
**	E_DB_OK if sucessful
**	E_DB_ERROR if something went wrong
**	E_DB_FATAL on fatal error.
**
** History:
**	17-sep-92 (robf)
**	    Created
**	30-nov-92 (robf)
**	    Updated error handling to allow for E_DB_FATAL from message
**	    conversion routines.
**	1-jul-93 (robf)
**	    Added support for object security label. This can be returned
**	    as either a security label or seclabel_id.
**	7-jul-93 (robf)
**	    Added support for sessionid, returned as a text string.
**	23-dec-93 (robf)
**          Added support for querytext_sequence. This is the  same as the
**	    detailnum except that its only non-zero for query text audit
**	    events.  (Per LRC decision)
**		
*/
DB_STATUS
gwsxa_sxf_to_ingres(GWX_RCB *gwx_rcb, SXF_AUDIT_REC *sxf_record)
{
    GW_RSB      *rsb = (GW_RSB *)gwx_rcb->xrcb_rsb;
    i4     *error = &gwx_rcb->xrcb_error.err_code;
    GWSXA_RSB   *sxa_rsb = (GWSXA_RSB *)rsb->gwrsb_internal_rsb;
    GW_TCB      *tcb = rsb->gwrsb_tcb;
    GWSXA_XREL  *xrel = (GWSXA_XREL*)tcb->gwt_xrel.data_address;
    DB_STATUS   	status=E_DB_OK;
    char		trbuf[GWF_MAX_MSGLEN + 1]; /* last char for `\n' */

    PTR		    ingres_buffer   = gwx_rcb->xrcb_var_data1.data_address;
    i4	    ing_buf_len	    = gwx_rcb->xrcb_var_data1.data_in_size;
    DB_TAB_ID	    *db_tab_id	    = gwx_rcb->xrcb_tab_id;

    ADF_CB	    *adfcb;
    GW_ATT_ENTRY    *gw_attr;
    DMT_ATT_ENTRY   *ing_attr;
    GWSXA_XATT	    *sxa_attr;
    DB_DATA_VALUE   ing_value;
    DB_DATA_VALUE   sxa_value;
    i4		    column_count;
    i4		    i;
    i4	    ulecode;
    i4	    msglen;
    EX_CONTEXT	    context;
    i4	    bas_rec_length;
    char	    tmpdesc[SXA_MAX_DESC_LEN+1];
    char	    tmpobjtype[DB_TYPE_MAXLEN+1]; /* objecttype char(24)*/
    char	    tmpacctype[DB_TYPE_MAXLEN+1]; /* auditevent char(24) */
    char	    tmppriv[SXA_MAX_PRIV_LEN+1];
    char	    audittmstr[26];
    i4	    ing_record_length;
    i4	    tmpi4;
    bool	    aligned;
    PTR	            align_save;
    /*
    **	aligned output buffer big enough for any column in iiaudit
    */
    ALIGN_RESTRICT  alignbuffer[256/sizeof(ALIGN_RESTRICT) + DB_OBJ_MAXNAME];

    adfcb = rsb->gwrsb_session->gws_adf_cb;
    
    column_count = tcb->gwt_table.tbl_attr_count;
    gw_attr = tcb->gwt_attr;
    ing_record_length = 0;

    /*
    ** Plant an exception handler so that if any lower-level ADF routine
    ** aborts, we can attempt to trap the error and "do the right thing".
    */
    if (EXdeclare( sxa_cvt_handler, &context) != OK)
    {
	/*
	** We get here if an exception occurs and the handler longjumps
	** back to us (returns EXDECLARE). In this case, ADF has asked us
	** to abort the query, so we return with an 'error converting record'
	** error. We can distinguish between "conversion error" and "fatal
	** ADF internal error" by checking the ADF control block. This is
	** important because our caller must be able to distinguish these
	** errors.
	*/
	EXdelete();
	adfcb = rsb->gwrsb_session->gws_adf_cb;
        *error = E_GW4065_SXA_TO_INGRES_ERROR;
	return (E_DB_ERROR);
    }
    /*
    ** Loop over all columns in iiaudit doing the conversion
    */
    for (i = 0; i < column_count; i++, gw_attr++)
    {
	ing_attr = &gw_attr->gwat_attr;
	sxa_attr = (GWSXA_XATT*)gw_attr->gwat_xattr.data_address;	

	/* Create the data values needed for conversion */
	ing_value.db_datatype = ing_attr->att_type;
	ing_value.db_prec = ing_attr->att_prec;
	ing_value.db_length = ing_attr->att_width;
	ing_value.db_data = ing_attr->att_offset + (char *)ingres_buffer;
	if (GWF_SXA_SESS_MACRO(9))
	{
		gwf_display(gwf_scctalk, 0, trbuf, sizeof(trbuf) - 1,
		    gwf_scctalk, 0, trbuf, sizeof(trbuf) - 1,
		    "SXA: Processing attribute %d, %s\n", i,
		    sxa_attr->audname);
	}
	/*
	**	Check if output buffer is aligned, if not then we
	**	need to use a temp (aligned) buffer and copy it
	*/
	if (ing_value.db_data != ME_ALIGN_MACRO(ing_value.db_data,
			sizeof(ALIGN_RESTRICT)))
	{
		aligned=FALSE;
		/*
		**	Use a temp buffer
		*/
		align_save=ing_value.db_data;
		ing_value.db_data= (PTR)alignbuffer;
		if (ing_value.db_length>sizeof(alignbuffer))
			ing_value.db_length=sizeof(alignbuffer);
	}
	else
	{
		aligned=TRUE;
	}
	if ((ing_attr->att_offset + ing_attr->att_width) > ing_buf_len)
	{
	    /*
	    ** This Ingres column is off the end of the Ingres buffer. This is
	    ** a coding error, so report it as such:
	    */
	    status = E_DB_ERROR;
	    *error = E_GW4065_SXA_TO_INGRES_ERROR;
	    break;
	}
	/*
 	**	Set up the data for the SXF conversion
	*/	
	sxa_value.db_prec=0;
	switch(sxa_attr->audid)
	{
	case SXA_ATT_AUDITTIME: 
		/*
		**	AUDITTIME needs to be converted to a date-format
		**	string so it can be coerced to whatever the
		**	user wants (like a DATE). 
		**
		**	If the Ingres type is integer DONT do the 
		**	conversion, since presumably they want the
		**	actual binary value.
		*/

		if (ing_value.db_datatype == DB_INT_TYPE)
		{
			/*
			**	This loses milliseconds (on systems
			**	that have the resolution) but there is no
			**	place to put it.
			*/
			sxa_value.db_datatype=DB_INT_TYPE;
			sxa_value.db_length=sizeof(sxf_record->sxf_tm.TM_secs);
			tmpi4=sxf_record->sxf_tm.TM_secs;
			sxa_value.db_data= (PTR) &tmpi4;
		}
		else
		{
			/*
			**	There are several other ways to do this,
			**	but this seems relatively simple - break
			**	into components and convert into a standard
			**	ingres date DD-MMM-YY HH:MM:SS
			**/
			struct TMhuman tmh;
			_VOID_ TMbreak(&(sxf_record->sxf_tm),&tmh);
			STprintf(audittmstr,"%s-%s-%s %s:%s:%s",
				tmh.day,
				tmh.month,
				tmh.year,
				tmh.hour,
				tmh.mins,
				tmh.sec
				);
			if (GWF_SXA_SESS_MACRO(9))
			{
				gwf_display(gwf_scctalk, 0, trbuf, sizeof(trbuf) - 1,
				    "SXA: Audittime days='%s', months='%s', years='%s', hr='%s', min='%s', sec='%s'\n",
				tmh.day, tmh.month, tmh.year, tmh.hour,
				tmh.mins, tmh.sec);
				gwf_display(gwf_scctalk, 0, trbuf, sizeof(trbuf) - 1,
				    "SXA: Audittime string value is '%s'\n",
					audittmstr);
			}
			sxa_value.db_datatype = DB_CHA_TYPE;
			sxa_value.db_length = sizeof(audittmstr);
			sxa_value.db_data = audittmstr;
		}
		break;
	case SXA_ATT_USER_NAME:
		sxa_value.db_datatype = DB_CHA_TYPE;
		sxa_value.db_length = sizeof(sxf_record->sxf_euserid.db_own_name);
		sxa_value.db_data = sxf_record->sxf_euserid.db_own_name;
		break;
	case SXA_ATT_SESSID:
		sxa_value.db_datatype = DB_CHA_TYPE;
		sxa_value.db_length = SXF_SESSID_LEN;
		sxa_value.db_data = sxf_record->sxf_sessid;
		break;
	case SXA_ATT_REAL_NAME:
		sxa_value.db_datatype = DB_CHA_TYPE;
		sxa_value.db_length = sizeof(sxf_record->sxf_ruserid.db_own_name);
		sxa_value.db_data = sxf_record->sxf_ruserid.db_own_name;
		break;
	case SXA_ATT_DATABASE:
		sxa_value.db_datatype = DB_CHA_TYPE;
		sxa_value.db_length = sizeof(sxf_record->sxf_dbname.db_db_name);
		sxa_value.db_data = sxf_record->sxf_dbname.db_db_name;
		break;
	case SXA_ATT_USERPRIVILEGES:
		if (ing_value.db_datatype == DB_INT_TYPE)
		{
			sxa_value.db_datatype = DB_INT_TYPE;
			sxa_value.db_length=sizeof(sxf_record->sxf_userpriv);
			tmpi4=sxf_record->sxf_userpriv;
			sxa_value.db_data= (PTR) &tmpi4;
		}
		else
		{
			sxa_value.db_datatype = DB_CHA_TYPE;
			sxa_value.db_length = sizeof(tmppriv)-1;
			sxa_value.db_data = tmppriv;
			_VOID_ gwsxa_priv_to_str(sxf_record->sxf_userpriv,&sxa_value,FALSE);
		}
		break;
	case SXA_ATT_OBJPRIVILEGES:
		if (ing_value.db_datatype == DB_INT_TYPE)
		{
			sxa_value.db_datatype = DB_INT_TYPE;
			sxa_value.db_length=sizeof(sxf_record->sxf_objpriv);
			tmpi4=sxf_record->sxf_objpriv;
			sxa_value.db_data= (PTR) &tmpi4;
		}
		else
		{
			sxa_value.db_datatype = DB_CHA_TYPE;
			sxa_value.db_length = sizeof(tmppriv)-1;
			sxa_value.db_data = tmppriv;
			_VOID_ gwsxa_priv_to_str(sxf_record->sxf_objpriv, &sxa_value,FALSE);
		}
		break;
	case SXA_ATT_AUDITSTATUS:
		sxa_value.db_datatype = DB_CHA_TYPE;
		sxa_value.db_length = 1;
		if (sxf_record->sxf_auditstatus==OK)	
			sxa_value.db_data="Y"; /* Success */
		else
			sxa_value.db_data="N"; /* Failure */
		break;
	case SXA_ATT_AUDITEVENT:
		/*
		**	Need to convert from a message id to 
		**	a string here. Make sure there is somewhere to
		**	put data.
		*/
		sxa_value.db_data = tmpacctype;
		sxa_value.db_datatype = DB_CHA_TYPE;
		status= gwsxa_msgid_to_desc(
			      gwsxa_acctype_to_msgid(sxf_record->sxf_accesstype), 
				&sxa_value);
		break;
	case SXA_ATT_OBJECTTYPE:
		/*
		**	Need to convert from a message id to 
		**	a string here. Make sure there is somewhere to
		**	put data.
		*/
		sxa_value.db_data = tmpobjtype;
		sxa_value.db_datatype = DB_CHA_TYPE;
		status=gwsxa_msgid_to_desc(
			      gwsxa_evtype_to_msgid(sxf_record->sxf_eventtype), 
				&sxa_value);
		break;
	case SXA_ATT_OBJECTNAME:
		sxa_value.db_datatype = DB_CHA_TYPE;
		sxa_value.db_length = sizeof(sxf_record->sxf_object);
		sxa_value.db_data = sxf_record->sxf_object;
		break;

	case SXA_ATT_DETAILINFO:
		sxa_value.db_datatype = DB_CHA_TYPE;
		if (sxf_record->sxf_detail_txt && sxf_record->sxf_detail_len)
		{
			sxa_value.db_data = sxf_record->sxf_detail_txt;
			sxa_value.db_length = sxf_record->sxf_detail_len;
		}
		else
		{
			sxa_value.db_data = " ";
			sxa_value.db_length = 1;
		}
		break;
		
	case SXA_ATT_DETAILNUM:
		sxa_value.db_datatype = DB_INT_TYPE;
		sxa_value.db_length=sizeof(tmpi4);
		tmpi4=sxf_record->sxf_detail_int;
		sxa_value.db_data= (PTR) &tmpi4;
		break;

	case SXA_ATT_QRYTEXTSEQ:
		/*
		** querytext_sequence is the detailint element, 
		** but is set to 0 except for QUERYTEXT audit records.
		*/
		if(sxf_record->sxf_eventtype==SXF_E_QRYTEXT)
			tmpi4=sxf_record->sxf_detail_int;
		else
			tmpi4=0;
		sxa_value.db_datatype = DB_INT_TYPE;
		sxa_value.db_length=sizeof(tmpi4);
		sxa_value.db_data= (PTR) &tmpi4;
		break;

	case SXA_ATT_OBJECTOWNER:
		sxa_value.db_datatype = DB_CHA_TYPE;
		sxa_value.db_data = sxf_record->sxf_objectowner.db_own_name;
		sxa_value.db_length = sizeof(sxf_record->sxf_objectowner.db_own_name);
		/*
		**	Allow for empty object owner, since ADF converts
		**	odd characeters in this case.
		*/
		if (sxf_record->sxf_objectowner.db_own_name[0]=='\0')
		{
			MEfill(sxa_value.db_length,' ', sxa_value.db_data);
		}

		break;
	case SXA_ATT_DESCRIPTION:
		/*
		**	Need to convert from a message id to 
		**	a string here. Make sure there is somewhere to
		**	put data. Allow user to retrieve integer msgid if
		**	necessary.
		*/
		if (ing_value.db_datatype == DB_INT_TYPE)
		{
			sxa_value.db_datatype = DB_INT_TYPE;
			sxa_value.db_length=sizeof(tmpi4);
			tmpi4=sxf_record->sxf_messid;
			sxa_value.db_data= (PTR) &tmpi4;
		}
		else
		{
			sxa_value.db_data = tmpdesc;
			sxa_value.db_datatype = DB_CHA_TYPE;
			status=gwsxa_msgid_to_desc(sxf_record->sxf_messid, &sxa_value);
		}
		break;
	default:
		/*
		**	We should never get here, indicates either
		**	a corrupted control block or a new type was added
		**	but the switch tables wasn't updated.
		**	Either way, log an error
		*/
	         gwsxa_error(gwx_rcb,E_GW4077_SXA_BAD_ATTID,
			 SXA_ERR_INTERNAL,2,
			sizeof(sxa_attr->audid),    (PTR)&sxa_attr->audid,	
			sizeof(sxa_attr->audname),  (PTR)&sxa_attr->audname
		    );
		status=E_DB_ERROR;
	};

	if (status!=E_DB_OK)
	{
		/*
		**	This is mainly here for two cases:
		**	- Bad column name
		**	- Semaphore failure while converting msgid to desc.
		*/
		*error=E_GW4065_SXA_TO_INGRES_ERROR;
		return status;
	}
	if ((status = adc_cvinto(adfcb, &sxa_value, &ing_value)) != E_DB_OK)
	{
		i4 tmp;
		char *attname;
	    /*
	    ** We failed to convert an SXF record into Ingres format. This
	    ** means that some of the SXF data was un-parseable. For now, we
	    ** will log the error in exhaustive detail. We may later wish to
	    ** change the error handling so that the user can request that
	    ** conversion errors be 'ignored' without generating error
	    ** messages. In that case, we would wish to continue rather than to
	    ** break.
	    **
	    ** Note that many, even most, datatype conversions are handled
	    ** by ADF with exceptions rather than with normal error returns...
	    **
	    ** Also, the ad_errcode in the adfcb often requires parameters to
	    ** be formatted correctly, and we don't have those parameters
	    ** available here, so if possible we use the already-formatted
	    ** message provided to us in ad_ersxagp instead.
	    */
	    if (adfcb->adf_errcb.ad_emsglen > 0)
		_VOID_ ule_format( (i4)0, (CL_ERR_DESC *)0,
			    ULE_MESSAGE, (DB_SQLSTATE *) NULL,
			    adfcb->adf_errcb.ad_errmsgp,
			    adfcb->adf_errcb.ad_emsglen,
			    &msglen, &ulecode, 0 );
	    else
		_VOID_ ule_format( adfcb->adf_errcb.ad_errcode,
			    (CL_ERR_DESC *)0, 
			    ULE_LOG, (DB_SQLSTATE *) NULL, (char *)0,
			    (i4)0, &msglen, &ulecode, 0 );

		/*
		E_GW4066_SXA_CVT_ERROR:NO_GE
		An error occurred converting a SAL record to Ingres format.\n
		The column which could not be converted was column %0d, name '%1c'.\n
		The Ingres datatype is %2d, SAL datatype was %3d"
		*/
	    _VOID_ gwsxa_error(NULL,E_GW4066_SXA_CVT_ERROR,
			 SXA_ERR_LOG|SXA_ERR_USER,4,
			sizeof(i),		(PTR)&i,
			sizeof(sxa_attr->audname),     sxa_attr->audname,
			sizeof(ing_attr->att_type),(PTR)&ing_attr->att_type,
			sizeof(sxa_value.db_datatype),(PTR)&sxa_value.db_datatype
		    );
			
	    if ((status = sxa_check_adferr( adfcb )) != E_DB_OK)
	    {
		if (adfcb->adf_errcb.ad_errclass == ADF_USER_ERROR)
		    *error = E_GW050D_DATA_CVT_ERROR;
		else
		    *error = E_GW4065_SXA_TO_INGRES_ERROR;
		break;
	    }
	}

	/*
	**	If not aligned then copy the data back
	*/
	if (aligned==FALSE)
	{
	    MECOPY_VAR_MACRO((PTR)alignbuffer, ing_value.db_length, align_save);
	}

	/*
	** Accumulate the Ingres record length and see if a buffer overflow
	** just occurred (such an overflow would be a SERIOUS coding bug)
	*/

	ing_record_length += ing_value.db_length;
	if (ing_record_length > ing_buf_len)
	{
	    status = E_DB_ERROR;
	    *error = E_GW4065_SXA_TO_INGRES_ERROR;
	    break;
	}
    }


    EXdelete();

    return (status);
}

/*
** Name: sxa_cvt_handler	- exception handler for conversion errors
**
** Description:
**	ADF may raise exceptions during data conversion. This should be
**	quite rare when converting records from SXF, but better safe than
**	sorry.
**	When an exception is raised, the EX facility calls us. We, in turn, 
**	ask ADF what it thinks about this exception. ADF tells us whether 
**	the exception is to be
**	    1) ignored
**	    2) warned about
**	    3) treated as a query-fatal error.
**
**	Right now, we don't handle warnings correctly. Instead, both warnings
**	and ignored conditions are quietly ignored, and result in an EXCONTINUE.
**	query fatal errors result in a EXDECLARE back to the EXdeclare'd
**	routines which do something useful like abort the query.
**
** Inputs:
**	ex_args		    - Exception arguments, as called by EX
**
** Outputs:
**	None
**
** Returns:
**	EXCONTINUES	    - Please continue with the next column.
**	EXDECLARE	    - Please abort this query
**	EXRESIGNAL	    - We don't know what to do, ask the next guy.
**
** Exceptions and Side Effects:
**	The EXDECLARE return causes an abrupt return to a higher routine.
**
** History:
**	9-sep-92   (robf)
**	    Created from GWRMS ADF handler
**	26-oct-92 (andre)
**	    in ADF_ERROR, ad_generic_error has been replaced with ad_sqlstate
**	    in SCF_CB, scf_generic_error has been replaced with scf_sqlstate
**	08-sep-93 (swm)
**	    Bug #56447
**	    Changed sid type from i4  to CS_SID to match recent CL
**	    interface revision.
**      18-Feb-1999 (hweho01)
**          Changed EXCONTINUE to EXCONTINUES to avoid compile error of
**          redefinition on AIX 4.3, it is used in <sys/context.h>.
*/
static STATUS
sxa_cvt_handler( EX_ARGS *ex_args )
{
    SCF_CB	scf_cb;
    ADF_CB	*adf_cb;
    DB_STATUS	adf_status;
    DB_STATUS	status;
    i4	msglen;
    i4	ulecode;
    char	emsg_buf [ER_MAX_LEN];
    SCF_SCI	sci_list;
    CS_SID	sid;

    for (;;)
    {
	/* Get the ADF_CB for this session */
	scf_cb.scf_type = SCF_CB_TYPE;
	scf_cb.scf_length = sizeof(SCF_CB);
	scf_cb.scf_facility = DB_ADF_ID;
	scf_cb.scf_session = DB_NOSESSION;
	scf_cb.scf_ptr_union.scf_sci = (SCI_LIST*)&sci_list;
	scf_cb.scf_len_union.scf_ilength = 1;
	sci_list.sci_length = sizeof(PTR);
	sci_list.sci_code = SCI_SCB;
	sci_list.sci_aresult = (PTR)(&adf_cb);
	sci_list.sci_rlength = NULL;
	if ((status = scf_call(SCU_INFORMATION, &scf_cb)) != E_DB_OK)
	{
	    gwf_error(scf_cb.scf_error.err_code, GWF_INTERR, 0);
	    gwf_error(E_GW0302_SCU_INFORMATION_ERROR, GWF_INTERR, 0);
	    break;
	}

	/*
	** We'd like an error message, so if there isn't already a buffer,
	** find one.
	*/
	if (adf_cb->adf_errcb.ad_ebuflen == 0)
	{
	    adf_cb->adf_errcb.ad_ebuflen = sizeof(emsg_buf);
	    adf_cb->adf_errcb.ad_errmsgp = emsg_buf;
	}

	/*
	** Now ask ADF to decipher the error which occurred
	*/

	adf_status = adx_handler( adf_cb, ex_args );

	switch ( adf_status )
	{
	    case E_DB_OK:
		/*
		** The exception is to be ignored.
		*/
		return (EXCONTINUES);

	    case E_DB_WARN:
		if (adf_cb->adf_errcb.ad_errcode == E_AD0116_EX_UNKNOWN)
		{
		    /*
		    ** OOPS, ADF doesn't recognize this. Resignal up the chain
		    */
		    return (EXRESIGNAL);
		}
		else
		{
		    /*
		    ** The exception deserves a warning. Unfortunately, we
		    ** aren't currently set up to do this...
		    */
		    return (EXCONTINUES);
		}

	    case E_DB_ERROR:
		/*
		** If the exception is to be query-fatal, ad_errcode is set
		** to an error numbewr in the range of E_AD1120 to E_AD1170,
		** or if an internal error occurred in the ADF error analysis,
		** ad_errcode is set to some internal error code.
		*/
		if (adf_cb->adf_errcb.ad_errcode == E_AD0116_EX_UNKNOWN)
		{
		    /*
		    ** OOPS, ADF doesn't recognize this. Resignal up the chain
		    */
		    return (EXRESIGNAL);
		}
		else
		{
		    switch ( adf_cb->adf_errcb.ad_errcode )
		    {
			case E_AD1120_INTDIV_ERROR:
			case E_AD1121_INTOVF_ERROR:
			case E_AD1122_FLTDIV_ERROR:
			case E_AD1123_FLTOVF_ERROR:
			case E_AD1124_FLTUND_ERROR:
			case E_AD1125_MNYDIV_ERROR:
			    /*
			    ** This was an ADF exception and I have handled
			    ** it for you by formatting an error message for
			    ** you to issue to the user and, after you have
			    ** done this, control should resume at the
			    ** instruction immediately after the one which
			    ** declared you. The code there should cause the
			    ** routine to clean up, if necessary, and return
			    ** with an appropriate error condition.
			    */
			    /*
			    ** Issuing to the user...
			    */
			    CSget_sid( &sid );
			    scf_cb.scf_type = SCF_CB_TYPE;
			    scf_cb.scf_length = sizeof(SCF_CB);
			    scf_cb.scf_facility = DB_GWF_ID;
			    scf_cb.scf_session = sid; /* was: DB_NOSESSION */
			    scf_cb.scf_nbr_union.scf_local_error = 
					    adf_cb->adf_errcb.ad_errcode;
			    STRUCT_ASSIGN_MACRO(adf_cb->adf_errcb.ad_sqlstate,
				scf_cb.scf_aux_union.scf_sqlstate);
			    scf_cb.scf_len_union.scf_blength =
					    adf_cb->adf_errcb.ad_emsglen;
			    scf_cb.scf_ptr_union.scf_buffer =
					    adf_cb->adf_errcb.ad_errmsgp;
			    status = scf_call(SCC_ERROR, &scf_cb);
			    if (status != E_DB_OK)
			    {
				gwf_error(scf_cb.scf_error.err_code,
						GWF_INTERR, 0);
				gwf_error(E_GW0303_SCC_ERROR_ERROR,
						GWF_INTERR, 0);

				return (EXRESIGNAL); /* ??? */
			    }

			    return (EXDECLARE);

			default:
			    /*
			    ** adx_handler() puked.
			    */
			    _VOID_ ule_format( adf_cb->adf_errcb.ad_errcode,
				    (CL_ERR_DESC *)0, ULE_LOG,
				    (DB_SQLSTATE *) NULL, (char *)0, (i4)0,
				    &msglen, &ulecode, 0 );

			    return (EXRESIGNAL);
		    }
		}

	    default:
		/*
		** adx_handler() puked.
		*/
		_VOID_ ule_format( adf_cb->adf_errcb.ad_errcode,
			    (CL_ERR_DESC *)0, 
			    ULE_LOG, (DB_SQLSTATE *)NULL, (char *)0, (i4)0,
			    &msglen, &ulecode, 0 );

		return (EXRESIGNAL);
	}

	break;
    }
    /*
    ** Getting here is a bug...
    */
    return (EXRESIGNAL);
}

/*
** Name: gwf_check_adferr()	- better error handling for ADF error.
**
** Description:
**	ADF's handling of data conversion errors is 2-fold: some error result
**	in exceptions being raised. Others result in a non-zero return code
**	being returned from adc_cvinto(). This routine handles those errors:
**
**	If the error was an internal ADF error, we abort the query.
**	    THERE IS ONE EXCEPTION TO THIS. See history comments re: AD2009
**	If the error was a data conversion error, we check the ADF "-x" setting
**	    1) If it says 'ignore error', we just return OK
**	    2) If it says 'issue warning', we increment ADF's warning counter
**		and return OK.
**	    3) If it says 'abort query', we return E_DB_ERROR.
**
**	Most of this code should really be in ADF, but I'm in a rush and John
**	isn't here to talk to...
**
** Inputs:
**	adf_scb	    - the ADF control block from the adc_cvinto() call.
**
** Outputs:
**	adf_scb	    - may have its warning counts incremented.
**
** Returns:
**	E_DB_OK	    - continue with query
**	E_DB_ERROR  - abort query.
**		      If the adf control block's ad_errclass says ADF_USER_ERROR
**		      then the error message has already been sent to the
**		      front-end and the caller should not treat this as an
**		      'internal' error, but rather as a standard conversion err.
**
** History:
**	17-sep-1992 (robf)
**		Created from the GWRMS checker.
**	26-oct-92 (andre)
**	    replaced calls to ERlookup() with calls to ERslookup();
**	    instead of generic error, ERslookup() returns sqlstate
**
**	    in ADF_ERROR, ad_generic_error has been replaced with ad_sqlstate
**	    in SCF_CB, scf_generic_error has been replaced with scf_sqlstate
**	08-sep-93 (swm)
**	    Bug #56447
**	    Changed sid type from i4  to CS_SID to match recent CL
**	    interface revision.
*/
static STATUS
sxa_check_adferr( ADF_CB *adf_scb )
{
    SCF_CB	scf_cb;
    DB_STATUS	status;
    CS_SID	sid;
    DB_STATUS	return_status;
    STATUS	tmp_status;
    CL_ERR_DESC	sys_err;

    if (adf_scb->adf_errcb.ad_errclass != ADF_USER_ERROR)
    {
	/*
	** Internal ADF error. Abort query, UNLESS this is the AD2009, which
	** indicates "no coercion exists". For AD2009, let's report the no
	** coercion message to the frontend instead of returning "SC0206". We
	** 'downgrade' this message from an ADF internal error to an ADF
	** user error, format it, and report it to the user. 
	*/
	if (adf_scb->adf_errcb.ad_errcode == E_AD2009_NOCOERCION)
	{
	    tmp_status  = ERslookup(adf_scb->adf_errcb.ad_errcode, 0,
				ER_TIMESTAMP,
				adf_scb->adf_errcb.ad_sqlstate.db_sqlstate,
				adf_scb->adf_errcb.ad_errmsgp,
				adf_scb->adf_errcb.ad_ebuflen, -1,
				&adf_scb->adf_errcb.ad_emsglen,
				&sys_err, (i4)0,
				(ER_ARGUMENT *)0);

	    if (tmp_status != OK)
	    {
		return (E_DB_ERROR);
	    }
	    else
	    {
		/*
		** else we drop through to send the message to the frontend, and
		** return with an 'abort this query' error. We downgrade this
		** message to USER error so that the layers above us will think
		** it is a 'normal' conversion error. We do NOT, however, place
		** it under the control of "-x". This error is unconditionally
		** query-fatal.
		*/
		adf_scb->adf_errcb.ad_errclass = ADF_USER_ERROR; 
		return_status = E_DB_ERROR;
	    }
	}
	else
	{
	    return (E_DB_ERROR);
	}
    }
    else
    {
	/*
	** Check the ADF control block  to
	** decide whether the user requested ignore errors, warn errors, or
	** abort query errors. The only tricky one is warn errors, which
	** require some inspection of the particular error to determine which
	** error code to increment.
	*/

	/* Set up Warning or Error in the internal message buffer */

	if (adf_scb->adf_exmathopt == ADX_IGN_MATHEX)
	{
		return (E_DB_OK);
	}
	else if (adf_scb->adf_exmathopt == ADX_WRN_MATHEX)
	{
	    switch (adf_scb->adf_errcb.ad_errcode)
	    {
	      case E_GW7001_FLOAT_OVF:
		adf_scb->adf_warncb.ad_fltovf_cnt++;
		return (E_DB_OK);

	      case E_GW7002_FLOAT_UND:
		adf_scb->adf_warncb.ad_fltund_cnt++;
		return (E_DB_OK);

	      case E_GW7000_INTEGER_OVF:
		adf_scb->adf_warncb.ad_intovf_cnt++;
		return (E_DB_OK);

	      default:
		return_status = E_DB_OK;
		break;	/* send the  error  to the user, then continue query */
	    }
	}   /* end of warning processing */
	else
	{
	    /*
	    ** "-xf" == abort query. Do nothing here; drop through to common
	    ** code which sends message to front-end. Then return ERROR,
	    ** which will cause our caller  to abort  the query.
	    */
	    return_status = E_DB_ERROR;
	}
    }
    /*
    ** We get here in the case that this error will abort the query, but it
    ** is a 'user' error, which means that it is NOT an internal error and
    ** should result in a friendly user message rather than an unfriendly
    ** 'internal error' message. The friendly user message is, we hope,
    ** already formatted in the ADF control block. We package it up and send
    ** it to the user, then return E_DB_ERROR to cause our caller to break
    ** out of his processing. Our caller can distinguish this return from
    ** internal error returns because of the ADF_USER_ERROR value in the
    ** ADF control block...
    */
    CSget_sid( &sid );

    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_length = sizeof(SCF_CB);
    scf_cb.scf_facility = DB_GWF_ID;
    scf_cb.scf_session = sid;	    /* was: DB_NOSESSION */
    scf_cb.scf_nbr_union.scf_local_error = adf_scb->adf_errcb.ad_errcode;
    STRUCT_ASSIGN_MACRO(adf_scb->adf_errcb.ad_sqlstate,
	scf_cb.scf_aux_union.scf_sqlstate);
    scf_cb.scf_len_union.scf_blength = adf_scb->adf_errcb.ad_emsglen;
    scf_cb.scf_ptr_union.scf_buffer = adf_scb->adf_errcb.ad_errmsgp;
    status = scf_call(SCC_ERROR, &scf_cb);
    if (status != E_DB_OK)
    {
	gwf_error(scf_cb.scf_error.err_code, GWF_INTERR, 0);
	gwf_error(E_GW0303_SCC_ERROR_ERROR, GWF_INTERR, 0);

	/* upgrade to 'internal error' */
	adf_scb->adf_errcb.ad_errclass = ADF_INTERNAL_ERROR;
    }

    return (return_status);
}

/*
** Name: gwsxa_msgid_to_desc - converts a message id to text
**
** Description:
**	The main reason for this routine is to provide caching of
**	commonly used messages to increasing performance (we could use
**	"fast" messages in ER but initially it was decided to do the
**	caching in the GWF instead.)
**
**	If the message id can't be found an error is issued ONCE, then
**	a default message is provided for subsequent usage
**
**	This uses the GWsxamsg_sem semaphore to make sure that
**	another session doesn't change the underlying tree while walking
**	or updating it.
**
** Inputs:
**	msgid	The SXF message id that needs to be converted
**
** Outputs:
**	desc	The converted description
**
** Returns:
**	E_DB_OK if sucessful
**
** History:
**	17-sep-92 (robf)
**	    Created
*/
DB_STATUS
gwsxa_msgid_to_desc(i4 msgid, DB_DATA_VALUE *desc)
{
	DB_STATUS status=E_DB_OK;
	STATUS    semstat;
	i4 local_error;

	/*
	**	Get exclusive access to the message heap
	*/
	if (semstat = CSp_semaphore(TRUE, &GWsxamsg_sem))
	{
		ule_format(semstat, (CL_ERR_DESC*) NULL,
			ULE_LOG, NULL, 0, 0, 0, &local_error, 0);
		return E_DB_FATAL;
	}
	/*
	**	Increment lookup counter
	*/
	gwsxa_incr(&GWsxa_sem, &GWsxastats.sxa_num_msgid_lookup);
	/*
	**	First try to lookup the message id in the cache
	*/
	status=gwsxa_find_msgid(msgid,desc);

	if (status!=E_DB_OK)
	{
		status=gwsxa_add_msgid(msgid,desc);
	}
	/*
	**	Release semaphore
	*/
	if (semstat = CSv_semaphore( &GWsxamsg_sem))
	{
		ule_format(semstat, (CL_ERR_DESC*) NULL,
			ULE_LOG, NULL, 0, 0, 0, &local_error, 0);
		return E_DB_FATAL;
	}
	return E_DB_OK;
}

/*
** Name: gwsxa_find_msgid - finds a message id in the cache.
**
** Description:
**	Searches the cache for a message id, if found fills in desc
**	with the description of the message.
**
** Inputs:
**	msgid	The SXF message id that needs to be converted
**
** Outputs:
**	desc	The converted description
**
** Returns:
**	E_DB_OK    if sucessful
**	E_DB_WARN if not found
**
** History:
**	17-sep-92 (robf)
**	    Created
*/
DB_STATUS
gwsxa_find_msgid(i4 msgid,DB_DATA_VALUE *desc)
{
	SXA_MSG_DESC *m;

	m=mroot;
	while(m!=NULL)
	{
		if (m->msgid==msgid)
			break;
		else if (msgid<m->msgid)
			m=m->left;
		else 
			m=m->right;
	}
	if (m==NULL)
	{
		/*
		**	Not found, so return
		*/
		return E_DB_WARN;
	}
	/*
	**	Message Id found in the cache so convert the
	**	data.
	*/
	desc->db_length=m->db_length;
	MEcopy(m->db_data,m->db_length,desc->db_data);
	return E_DB_OK;
}

/*
** Name: gwsxa_add_msgid - adds a message id to the cache.
**
** Description:
**	Adds a new message id to the cache. If ERslookup can't find
**	the message id, then it issues a single diagnostic and creates
**	a default message for later usage.
**
** Inputs:
**	msgid	The SXF message id that needs to be converted
**
** Outputs:
**	desc	The converted description
**
** Returns:
**	E_DB_OK    if sucessful
**	E_DB_WARN if not found
**
** History:
**	17-sep-92 (robf)
**	    Created
**	26-oct-92 (andre)
**	    replaced calls to ERlookup() with calls to ERslookup();
**	    instead of generic error, ERslookup() returns sqlstate
*/
static DB_STATUS
gwsxa_add_msgid(i4 msgid, DB_DATA_VALUE  *desc)
{
	SXA_MSG_DESC *m;
	SXA_MSG_DESC *mnew;
	char	msgdesc[81];
	VOID gwsxa_msg_defaut();
	i4		ulecode;
	i4		msglen;
	DB_STATUS	status;
	STATUS	        erstatus;
	CL_ERR_DESC sys_err;
        char		trbuf[GWF_MAX_MSGLEN + 1]; /* last char for `\n' */

	/*
	**	First walk the tree to find the place where the
	**	data would go
	*/
	m=mroot;
	while(m!=NULL)
	{
		if (m->msgid==msgid)
			break;
		else if (msgid<m->msgid)
		{
			if (m->left==NULL)
				break;
			else
				m=m->left;
		}
		else 
		{
			if (m->right==NULL)
				break;
			else
				m=m->right;
		}
	}
	/*
	**	Sanity check to see if found already
	*/
	if (m!=NULL && m->msgid==msgid)
	{
		desc->db_length=m->db_length;
		desc->db_data=m->db_data;
		return E_DB_OK;
		
	}
	/*
	**	Look up the message text
	*/
	erstatus = ERslookup(msgid, NULL,
		ER_TEXTONLY, (char *) NULL,
		msgdesc, sizeof(msgdesc), -1,
		&msglen,&sys_err,0,NULL);

	/*
	** If ERslookup failed, get a default text for the message
	** and continue. Always issue a diagnostic just in case.
	*/
	if (erstatus != OK)
	{
		TRdisplay("SXA Gateway: Unable to lookup message id %d (X%x), using default.\n",msgid,msgid);
		gwsxa_msgid_default(msgid,desc);
		MEcopy(desc->db_data,sizeof(msgdesc),msgdesc);
	        gwsxa_incr(&GWsxa_sem, &GWsxastats.sxa_num_msgid_fail);
	}
	else
	{
		if (GWF_SXA_SESS_MACRO( 8))
		{
			gwf_display(gwf_scctalk, 0, trbuf, sizeof(trbuf) - 1,
			    "SXA Gateway: Mapped message id %d to '%s'\n",
				msgid, msgdesc);
		}
	}
	gwsxa_incr(&GWsxa_sem, &GWsxastats.sxa_num_msgid);
	/*
	**	Allocate space for new message
	*/
	status=gwsxa_alloc_msgdesc(&mnew);
	if ( status!=E_DB_OK)
	{
		/*
		**	No more memory right now.
		**	so what we do is fill in a "default" value in the
		**	data and return
		*/
		gwsxa_msgid_default(msgid,desc);
		return E_DB_OK;
	}
	/*
	**	Save the data in the new pointer and hook into the
	**	tree.
	*/
	MEcopy(msgdesc, sizeof(mnew->db_data), mnew->db_data);
	mnew->msgid=msgid;
	mnew->db_length=STtrmwhite(mnew->db_data);
	mnew->left=NULL;
	mnew->right=NULL;
	if (mroot==NULL)
	{
		mroot=mnew;
	}
	else if (msgid<m->msgid)
	{
		/*
		**	Goes on the left
		*/
		m->left=mnew;
	}
	else 
	{
		/*
		**	Goes on the right
		*/
		m->right=mnew;
	}
	/*
	**	Return the new saved value
	*/
	desc->db_length=mnew->db_length;
	MEcopy(mnew->db_data,desc->db_length,desc->db_data);
	return E_DB_OK;
}
/*
** Name: gwsxa_alloc_msgdesc - allocates memory for a new message
**	 description.
**
** Outputs:
**	m 	points to the new memory.
**
** Returns:
**	E_DB_OK    if sucessful
**	E_DB_ERROR if something went wrong
**
** History:
**	17-sep-92 (robf)
**	    Created
*/
static DB_STATUS
gwsxa_alloc_msgdesc(SXA_MSG_DESC **m)
{
	DB_STATUS status;
        char	trbuf[GWF_MAX_MSGLEN + 1]; /* last char for `\n' */

	/*
	**	Allocate a new value from the pool
	*/
	gwsxa_ulm.ulm_psize=sizeof(SXA_MSG_DESC);
	if (GWF_SXA_SESS_MACRO(5))
	{
		gwf_display(gwf_scctalk, 0, trbuf, sizeof(trbuf) - 1,
		    "SXA_ALLOC_MSG: Allocating %d bytes for new message \n",
			gwsxa_ulm.ulm_psize);
	}
	status=ulm_palloc(&gwsxa_ulm);

	if (status != E_DB_OK)
	{
		i4 tmp=sizeof(SXA_MSG_DESC);
		gwsxa_error(NULL,E_GW0314_ULM_PALLOC_ERROR,
			SXA_ERR_USER|SXA_ERR_LOG,
			4,
			sizeof(gwsxa_ulm.ulm_memleft), (PTR)&gwsxa_ulm.ulm_memleft,
			sizeof(gwsxa_ulm.ulm_sizepool),(PTR)&gwsxa_ulm.ulm_sizepool,
			sizeof(gwsxa_ulm.ulm_blocksize),(PTR)&gwsxa_ulm.ulm_blocksize,
			sizeof(tmp), (PTR)&tmp
			);
		return E_DB_ERROR;

	}
	*m= (SXA_MSG_DESC*)gwsxa_ulm.ulm_pptr;
	return E_DB_OK;
}

/*
** Name: gwsxa_msgid_default - Generates a default message for
**       a message id
**	 description.
**
** Description:
**	Generates a substitute message text when for some reason the
**	message description is not available.
**
** Inputs:
**	msgid message id
** Outputs:
**	desc formatted output.
**
** Returns:
**	None
**
** History:
**	17-sep-92 (robf)
**	    Created
*/
VOID
gwsxa_msgid_default(i4 msgid,DB_DATA_VALUE  *desc)
{
	char cmsgid[25];
	STprintf(desc->db_data,"Audit description id %d (X%x)",msgid,msgid);
}


/*
** Name: gwsxa_evtype_to_msgid - converts an SXF event type to a msgid for
**	 text string lookup.
**
** Description:
**	This takes an SXF event type and returns an appropriate
**	msgid which can then be used as a key to get a textual
**	description of the event
**
** Inputs:
**	evtype 
**
** Returns:
**	Message ID
**
** History:
**	21-sep-92 (robf)
**	    Created
*/
i4
gwsxa_evtype_to_msgid(SXF_EVENT evtype)
{
	i4 i;
        char		trbuf[GWF_MAX_MSGLEN + 1]; /* last char for `\n' */
	static struct {
		SXF_ACCESS evtype;
		i4    msgid;
	} evtypes[]= {
		SXF_E_DATABASE,	I_SX2901_DATABASE,
		SXF_E_APPLICATION,	I_SX2902_APPLICATION,
		SXF_E_PROCEDURE,	I_SX2903_PROCEDURE,
		SXF_E_TABLE,	I_SX2904_TABLE,
		SXF_E_LOCATION,	I_SX2905_LOCATION,
		SXF_E_VIEW,	I_SX2906_VIEW,
		SXF_E_RECORD,	I_SX2907_RECORD,
		SXF_E_SECURITY,	I_SX2908_SECURITY,
		SXF_E_USER,	I_SX2909_USER,
		SXF_E_LEVEL,	I_SX2910_LEVEL,
		SXF_E_ALARM,	I_SX2911_ALARM,
		SXF_E_RULE,	I_SX2912_RULE,
		SXF_E_EVENT,	I_SX2913_EVENT,
		SXF_E_ALL,	I_SX2914_ALL,
		SXF_E_INSTALLATION,	I_SX2915_INSTALLATION,
		SXF_E_RESOURCE,		I_SX2916_RESOURCE,
		SXF_E_QRYTEXT,		I_SX2917_QRYTEXT,
		0,		I_SX2900_BAD_EVENT_TYPE
		
	};
	for (i=0; evtypes[i].evtype!= 0; i++)
	{
		if ( evtypes[i].evtype==evtype)
			break;
	}
	if (GWF_SXA_SESS_MACRO(50) && evtypes[i].msgid==I_SX2900_BAD_EVENT_TYPE)
	{
		gwf_display(gwf_scctalk, 0, trbuf, sizeof(trbuf) - 1,
		    "SXA: Bad/unknown Event type %d found\n",evtype);
	}
	return evtypes[i].msgid;
}

/*
** Name: gwsxa_acctype_to_msgid - converts an SXF object access type 
**	 to a msgid for text string lookup.
**
** Description:
**	This takes an SXF object access type and returns an appropriate
**	msgid which can then be used as a key to get a textual
**	description of the event
**
** Inputs:
**	evtype 
**
** Returns:
**	Message ID
**
** History:
**	21-sep-92 (robf)
**	    Created
**	27-nov-92 (robf)
**	    Added handling for SXF_A_SETUSER 
**	16-jun-93 (robf)
**	     Conditionally include old accesstypes if defined in SXF
**	14-feb-94 (robf)
**           Map SXF_A_CONNECT/DISCONNECT
*/
i4
gwsxa_acctype_to_msgid(SXF_ACCESS acctype)
{
	i4 i;
        char		trbuf[GWF_MAX_MSGLEN + 1]; /* last char for `\n' */
	static struct {
		SXF_ACCESS acctype;
		i4    msgid;
	} acctypes[]= {
		SXF_A_SELECT, 	I_SX2801_SELECT,
		SXF_A_INSERT,	I_SX2802_INSERT,
		SXF_A_DELETE,	I_SX2803_DELETE,
		SXF_A_UPDATE,	I_SX2804_UPDATE,
		SXF_A_COPYINTO,	I_SX2805_COPYINTO,
		SXF_A_COPYFROM,	I_SX2806_COPYFROM,
		SXF_A_SCAN,	I_SX2807_SCAN,
		SXF_A_ESCALATE,	I_SX2808_ESCALATE,
		SXF_A_EXECUTE,	I_SX2809_EXECUTE,
		SXF_A_EXTEND,	I_SX2810_EXTEND,
		SXF_A_CREATE,	I_SX2811_CREATE,
		SXF_A_MODIFY,	I_SX2812_MODIFY,
		SXF_A_DROP,	I_SX2813_DROP,
		SXF_A_INDEX,	I_SX2814_INDEX,
		SXF_A_ALTER,	I_SX2815_ALTER,
		SXF_A_RELOCATE,	I_SX2816_RELOCATE,
		SXF_A_CONTROL,	I_SX2817_CONTROL,
		SXF_A_AUTHENT,	I_SX2818_AUTHENT,
# ifdef SXF_A_READ
		SXF_A_READ,	I_SX2819_READ,
		SXF_A_WRITE,	I_SX2820_WRITE,
		SXF_A_READWRITE,	I_SX2821_READWRITE,
# endif
		SXF_A_TERMINATE,	I_SX2822_TERMINATE,
#ifdef SXF_A_DOWNGRADE
		SXF_A_DOWNGRADE,	I_SX2823_DOWNGRADE,
#endif
		SXF_A_CONNECT,  I_SX2835_CONNECT,
		SXF_A_DISCONNECT,  I_SX2836_DISCONNECT,
		SXF_A_BYTID,	I_SX2824_BYTID,
		SXF_A_BYKEY,	I_SX2825_BYKEY,
		SXF_A_MOD_DAC,	I_SX2826_MOD_DAC,
		SXF_A_REGISTER,	I_SX2827_REGISTER,
		SXF_A_REMOVE,	I_SX2828_REMOVE,
		SXF_A_RAISE,	I_SX2829_RAISE,
		SXF_A_SETUSER,	I_SX282A_SETUSER,
		SXF_A_REGRADE,	I_SX282B_REGRADE,	
		SXF_A_WRITEFIXED,I_SX282C_WRITEFIXED,	
		SXF_A_QRYTEXT,	I_SX2830_QRYTEXT,	
		SXF_A_SESSION,	I_SX2831_SESSION,	
		SXF_A_QUERY,	I_SX2832_QUERY	,	
		SXF_A_LIMIT,	I_SX2833_LIMIT	,	
		SXF_A_USERMESG,	I_SX2834_USERMESG,	
		0,		I_SX2800_BAD_ACCESS_TYPE
	};
	for (i=0; acctypes[i].acctype!= 0; i++)
	{
		if ( acctypes[i].acctype==acctype)
			break;
	}
	if (GWF_SXA_SESS_MACRO(50) && acctypes[i].msgid==I_SX2800_BAD_ACCESS_TYPE)
	{
		gwf_display(gwf_scctalk, 0, trbuf, sizeof(trbuf) - 1,
		    "SXA: Bad/unknown Access type %d found\n",acctype);
	}
	return acctypes[i].msgid;
}
/*
** Name: gwsxa_priv_to_string - converts a privilege mask to a string
**       of Y and -
**
** Description:
**	This takes an internal priv mask and converts it bit-by-bit
**	to a privilege string (Y & -). Currently there is no knowledge
**	of the "meaning" of each bit. 
**      
**	It is assumed that pstr points to something sensible
**
**	If 'term' is set to TRUE then a NULL terminator is added,
**	otherwise not.
**
** Inputs:
**	priv	the internal privilege mask 
**
** Outputs:
**	pstr    string holding the privileges
**
** History:
**	21-sep-92 (robf)
**	    Created
*/
static VOID
gwsxa_priv_to_str( i4 priv, DB_DATA_VALUE *pstr, bool term)
{
	int i;
	for (i=0;i<pstr->db_length;i++)
	{
		if (priv& (1<<i))
			pstr->db_data[i]='Y';
		else
			pstr->db_data[i]='-';
	}
	if(term==TRUE)
		pstr->db_data[pstr->db_length]=EOS;
}

