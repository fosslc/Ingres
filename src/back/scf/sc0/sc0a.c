/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <er.h>
#include    <ex.h>
#include    <pc.h>
#include    <tm.h>
#include    <me.h>
#include    <ci.h>

#include    <dbms.h>
#include    <iicommon.h>

#include    <dbdbms.h>
#include    <ddb.h>
#include    <ulf.h>
#include    <tm.h>
#include    <cs.h>
#include    <cv.h>
#include    <st.h>
#include    <scf.h>
#include    <qsf.h>
#include    <adf.h>
#include    <gca.h>

/* added for scs.h prototypes, ugh! */
#include <dudbms.h>
#include <ulm.h>
#include <dmf.h>
#include <dmccb.h>
#include <dmrcb.h>
#include <copy.h>
#include <qefrcb.h>
#include <qefqeu.h>
#include <qefcopy.h>

#include    <sxf.h>
#include    <sc.h>
#include    <scc.h>
#include    <scs.h>
#include    <scd.h>
#include    <sc0m.h>
#include    <scfcontrol.h>
#include    <sc0a.h>
#include    <sc0e.h>
/*
**	SC0A.C - SCF Audit routines.
**
**	This file contains routines for security auditing, which may be
**	called within the SCF. They provide an interface to SXF
**
**	Routines include:
**		sc0a_write_audit()	 - Write an audit record
**		sc0a_flush_audit()	 - Flush audit records to disk.
**		sc0a_qrytext()		 - Audit query text
**		sc0a_qrytext_param()	 - Audit query text params
**		sc0a_qrytext_data()	 - Audit query text data
**
**	History:
**	30-nov-1992  (robf)
**		Created.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	29-Jun-1993 (daveb)
**	    Add include of <me.h>
**	13-sep-93 (swm)
**	    Include cv.h to pickup CV_HEX_PTR_SIZE define which is now
**	    needed by scs.h.
**	22-mar-94 (robf)
**          Only build query text audit records if session has query_text
**	    auditing enabled. This avoids the overhead of building the
**	    audit record everytime only the discard it in SXF when the
**	    filter is disabled.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Sep-2008 (jonj)
**	    SIR 120874: include sc0e.h to get function prototypes.
**	    Use new forms of sc0ePut(), uleFormat().
*/
GLOBALREF SC_MAIN_CB	*Sc_main_cb;
/*
** Forward declarations
*/
static DB_STATUS
write_audit (
SCD_SCB		    *scb,	  /* SCB */
SXF_EVENT           type,         /* Type of event */
SXF_ACCESS          access,       /* Type of access */
char                *obj_name,    /* Object name */
i4		    obj_len,	  /* Length of object */
DB_OWN_NAME         *obj_owner,   /* Object owner */
i4             msgid,        /* Message identifier */
bool                force_write,   /* Force write to disk */
char		    *detailtext,
i4		    detaillen,
i4		    detailnum,
i4		    userprivs,	   /* User privileges */
DB_ERROR             *err_code     /* Error info */
) ;

/*{
** Name: sc0a_flush_audit - Flush security audit buffers
**
** Description:
**      This routine flushes the SXF security audit buffers for the current
**	session to disk. This must done prior to any information being 
**	returned to the frontend.
**
** Inputs:
**      None
**
** Outputs:
**
**	Returns:
**	    DB_STATUS
**	    error code.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-nov-1992 (robf)
**          Created.
[@history_template@]...
*/
DB_STATUS
sc0a_flush_audit(SCD_SCB *scb, i4 *cur_error)
{
	SXF_RCB sxf_rcb;
	DB_STATUS  status;
        DB_STATUS (*sxfptr)();
	
	if (!(Sc_main_cb->sc_capabilities & SC_C_C2SECURE))
	{
		/*
		**	This is not a C2 server so can just return
		*/
		return E_DB_OK;
	}
	if (!(scb->scb_sscb.sscb_facility & (1 << DB_SXF_ID)))
	{
		/*
		**	SXF not initialized yet so quietly return,
		*/
		return E_DB_OK;
	}
        sxf_rcb.sxf_ascii_id=SXFRCB_ASCII_ID;
        sxf_rcb.sxf_length = sizeof (sxf_rcb);
        sxf_rcb.sxf_cb_type = SXFRCB_CB;
	/*
	**	This holds a pointer to SXF_CALL
	*/
        sxfptr= (DB_STATUS (*)())Sc_main_cb->sc_sxf_cptr;

        status = sxfptr(SXR_FLUSH, &sxf_rcb);
	
	if (status!=E_DB_OK)
	{
		sc0ePut(&sxf_rcb.sxf_error, 0, 0, 0);
		*cur_error = E_SC023D_SXF_BAD_FLUSH;
		return status;
	}
	return E_DB_OK;
}

/*
** Name: sc0a_write_audit(), write an audit record
**
** Description:
**	This routine is an interface to SXF to write audit records
**	from SCF. 
**
** Inputs:
**      SXF_EVENT           type,          Type of event 
**      SXF_ACCESS          access,        Type of access 
**      char                *obj_name,     Object name 
**	i4		    obj_len	   Length of object name
**      DB_OWN_NAME         *obj_owner,    Object owner 
**      i4             msgid,         Message identifier 
**      bool                force_write    Force write to disk 
**      i4             *err_code      Error info 
**
** Outputs:
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	27-nov-1992 (robf)
**		Created.
**	08-sep-93 (swm)
**	    Changed sid type from i4  to CS_SID to match recent CL
**	    interface revision.
**	25-oct-93 (vijay)
**	    remove wrong type cast.
**	12-oct-93 (swm)
**	    Bug #56447
**	    Eliminated unnecessary PTR cast for sid.
**	13-jan-94 (robf)
**           put back missing code to check for SXF initiated.
*/
DB_STATUS
sc0a_write_audit (
SCD_SCB 	    *scb,	  /* SCF info */
SXF_EVENT           type,         /* Type of event */
SXF_ACCESS          access,       /* Type of access */
char                *obj_name,    /* Object name */
i4		    obj_len,	  /* Length of object */
DB_OWN_NAME         *obj_owner,   /* Object owner */
i4             msgid,        /* Message identifier */
bool                force_write,   /* Force write to disk */
i4		    user_priv,	   /* User privilege  mask */
DB_ERROR             *err_code     /* Error info */
) 
{
	i4         local_error;
	CL_SYS_ERR	syserr;
	DB_STATUS       status;
	SXF_RCB         sxfrcb;
	CS_SID	        sid;

	if (!(Sc_main_cb->sc_capabilities & SC_C_C2SECURE))
	{
		/*
		**	This is not a C2 server so can just return
		*/
		return E_DB_OK;
	}
	if (!(scb->scb_sscb.sscb_facility & (1 << DB_SXF_ID)))
	{
		/*
		**	SXF not initialized yet so quietly return,
		*/
		return E_DB_OK;
	}
	return (write_audit(scb,  type,  access, 
			obj_name, obj_len, obj_owner, 
			msgid,     force_write,
			NULL, 0, 0, user_priv,
			err_code ));
}

/*
** Name: sc0a_qrytext(), write query text in one or more segments
**	 to the audit log.
**
** Description:
**	This routine is an interface to SXF to write audit records
**	from SCF. 
**
** Inputs:
**	scb		SCB
**
**	qrytext		Query text to be written
**
**	qrylen		Length of qrytext in bytes
**
**	ops		Operations (start/end/more)
**
** Outputs:
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	6-jul-93 (robf)
**		Created.
**	23-dec-93 (robf)
**              Changed QUERY_TEXT  to QUERYTEXT for  consistency with
**	        other messages.
*/
DB_STATUS
sc0a_qrytext(
	SCD_SCB *scb,
	char	*qrytext,
	i4 qrylen,
	i4	ops
)
{
	SXF_RCB sxf_rcb;
	DB_STATUS  status=E_DB_OK;
        DB_STATUS (*sxfptr)();
	SCS_QTACB  *qtacb = &scb->scb_sscb.sscb_qtacb;
	char	   *tptr;
	char	   *bufptr;
	DB_ERROR   err_code;
	CL_SYS_ERR syserr;
	i4	   local_error;
	
	
	if (!(Sc_main_cb->sc_capabilities & SC_C_C2SECURE))
	{
		/*
		**	This is not a C2 server so can just return
		*/
		return E_DB_OK;
	}
	if (!(scb->scb_sscb.sscb_facility & (1 << DB_SXF_ID)))
	{
		/*
		**	SXF not initialized yet so quietly return,
		*/
		return E_DB_OK;
	}
	if (!(scb->scb_sscb.sscb_ics.ics_rustat&DU_UAUDIT_QRYTEXT))
	{
		/*
		** Session doesn't have query text auditing so don't audit
		*/
		return E_DB_OK;
	}
	/*
	** If starting reset the audit sequence
	*/
	if(ops& SC0A_QT_START)
	{
		qtacb->qta_seq=1;
		qtacb->qta_len=0;
	}
	/*
	** If a new record is wanted flush the current one if any
	*/
	if((ops&SC0A_QT_NEWREC) && qtacb->qta_len>0)
	{
		status=write_audit(scb, SXF_E_QRYTEXT,  
			SXF_A_QRYTEXT|SXF_A_SUCCESS, 
			"QUERYTEXT", sizeof("QUERYTEXT")-1,
			scb->scb_sscb.sscb_ics.ics_eusername, 
			I_SX2729_QUERY_TEXT, FALSE,
			qtacb->qta_buff,
			qtacb->qta_len,
			qtacb->qta_seq,
			0,
			&err_code );
		if(status!=E_DB_OK)
		{
			_VOID_ uleFormat(&err_code, 0, &syserr, 
				ULE_LOG, NULL,
			    (char *)0, 0L, (i4 *)0, &local_error, 0);
			_VOID_ uleFormat(NULL, E_SC023F_QRYTEXT_WRITE, NULL, ULE_LOG, NULL,
			    (char *)0, 0L, (i4 *)0, &local_error, 0);
		}
		/*
		** And reset length for new record, increment sequence count
		*/
		qtacb->qta_len=0;
		qtacb->qta_seq++;
	}
	/*
	** Build the text into the audit buffer
	*/
	tptr=qrytext;
	bufptr=qtacb->qta_buff+qtacb->qta_len;
	while(qrylen>0)
	{
		/*
		** Fill the current buffer
		*/
		if(qtacb->qta_len<SCS_QTA_BUFSIZE)
		{
			u_i2 copy_bytes;

			if(qtacb->qta_len+qrylen>SCS_QTA_BUFSIZE)
				copy_bytes=SCS_QTA_BUFSIZE-qtacb->qta_len;
			else
				copy_bytes=qrylen;
			/*
			** Put the data  in the buffer
			*/
			MEcopy(tptr,copy_bytes,bufptr);
			qrylen-=copy_bytes;
			qtacb->qta_len+=copy_bytes;
			bufptr+=copy_bytes;
			tptr+=copy_bytes;
		}
		/*
		** Check if buffer is full
		*/
		if(qtacb->qta_len==SCS_QTA_BUFSIZE)
		{
			/*
			** Buffer is full
			*/
			status=write_audit(scb, SXF_E_QRYTEXT,  
				SXF_A_QRYTEXT|SXF_A_SUCCESS, 
				"QUERYTEXT", sizeof("QUERYTEXT")-1,
				scb->scb_sscb.sscb_ics.ics_eusername, 
				I_SX2729_QUERY_TEXT, FALSE,
				qtacb->qta_buff,
				qtacb->qta_len,
				qtacb->qta_seq,
				0,
				&err_code );
			if(status!=E_DB_OK)
			{
				_VOID_ uleFormat(&err_code, 0, &syserr, 
					ULE_LOG, NULL,
				    (char *)0, 0L, (i4 *)0, &local_error, 0);
				_VOID_ uleFormat(NULL, E_SC023F_QRYTEXT_WRITE, NULL, ULE_LOG, NULL,
				    (char *)0, 0L, (i4 *)0, &local_error, 0);
				break;
			}
			/*
			** Reset for next chunk
			*/
			qtacb->qta_seq++; /* Next sequence */
			qtacb->qta_len=0; /* Start of buffer */
			bufptr=qtacb->qta_buff;

		}
	}
	/*
	** At end flush any remaining buffer
	*/
	if(ops& SC0A_QT_END)
	{
		if(qtacb->qta_len)
		{
			/*
			** Need to flush data
			*/
			status=write_audit(scb, SXF_E_QRYTEXT,  
				SXF_A_QRYTEXT|SXF_A_SUCCESS, 
				"QUERYTEXT", sizeof("QUERYTEXT")-1,
				scb->scb_sscb.sscb_ics.ics_eusername, 
				I_SX2729_QUERY_TEXT, FALSE,
				qtacb->qta_buff,
				qtacb->qta_len,
				qtacb->qta_seq,
				0,
				&err_code );
			if(status!=E_DB_OK)
			{
				_VOID_ uleFormat(&err_code, 0, &syserr, 
					ULE_LOG, NULL,
				    (char *)0, 0L, (i4 *)0, &local_error, 0);
				_VOID_ uleFormat(NULL, E_SC023F_QRYTEXT_WRITE, NULL, ULE_LOG, NULL,
				    (char *)0, 0L, (i4 *)0, &local_error, 0);
			}
		}
		/*
		** And reset
		*/
		qtacb->qta_seq=1;
		qtacb->qta_len=0;
	}	
	return status;
}

/*
** Name: sc0a_qrytext_data(), write query text parameters in one or more 
**	segments to the audit log.
**
** Description:
**	This routine is an interface to SXF to write audit records
**	from SCF. 
**
** Inputs:
**	scb		SCB
**
**	num_data	Number of data elements
**
**	qrydata		query data to be audited
**
** Outputs:
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	6-jul-93 (robf)
**		Created.
*/
DB_STATUS
sc0a_qrytext_data(
	SCD_SCB *scb,
	i4	num_data,
	DB_DATA_VALUE **qrydata
)
{
	i4		i;
	DB_DATA_VALUE   *dbv;
	char		namebuf[DB_MAXNAME+1];
	DB_STATUS	status=E_DB_OK;
	
	if (!(Sc_main_cb->sc_capabilities & SC_C_C2SECURE))
	{
		/*
		**	This is not a C2 server so can just return
		*/
		return E_DB_OK;
	}
	if (!(scb->scb_sscb.sscb_facility & (1 << DB_SXF_ID)))
	{
		/*
		**	SXF not initialized yet so quietly return,
		*/
		return E_DB_OK;
	}
	if (!(scb->scb_sscb.sscb_ics.ics_rustat&DU_UAUDIT_QRYTEXT))
	{
		/*
		** Session doesn't have query text auditing so don't audit
		*/
		return E_DB_OK;
	}
	/*
	** Loop over all data items formatting as parameters
	*/
	for( i=0; i<num_data; i++)
	{
		dbv=qrydata[i];
		STprintf(namebuf,"Param %d",i+1);
		status=sc0a_qrytext_param(scb, 
				namebuf, 
				STlength(namebuf),
				dbv);
		if(status!=E_DB_OK)
			break;
	}
	return status;
}

/*
** Name: sc0a_qrytext_param(), write query text parameters in one or more 
**	segments to the audit log.
**
** Description:
**	This routine is an interface to SXF to write audit records
**	from SCF. 
**
** Inputs:
**	scb		SCB
**
**	name		Name of data
**
**	name_len	Length of data
**
**	qrydata		query data to be audited
**
** Outputs:
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	6-jul-93 (robf)
**	     Created.
**	30-dec-93 (robf)
**           Add support for LONG BIT, and include name length in
**	     size calculation.
**	22-mar-94 (robf)
**           Some formats may include extra trim, so allow for this
**	     in the formatting. 
*/
DB_STATUS
sc0a_qrytext_param(
	SCD_SCB *scb,
	char	*name,
	i4	name_len,
	DB_DATA_VALUE *qrydata
)
{
	DB_STATUS  status=E_DB_OK;
	char	   qrytext[256];
	i4	   bdt;
	
	if (!(Sc_main_cb->sc_capabilities & SC_C_C2SECURE))
	{
		/*
		**	This is not a C2 server so can just return
		*/
		return E_DB_OK;
	}
	if (!(scb->scb_sscb.sscb_facility & (1 << DB_SXF_ID)))
	{
		/*
		**	SXF not initialized yet so quietly return,
		*/
		return E_DB_OK;
	}
	if (!(scb->scb_sscb.sscb_ics.ics_rustat&DU_UAUDIT_QRYTEXT))
	{
		/*
		** Session doesn't have query text auditing so don't audit
		*/
		return E_DB_OK;
	}

	/* Format data as name:value */
	status=sc0a_qrytext(scb, name, name_len, SC0A_QT_NEWREC);
	status=sc0a_qrytext(scb, ":", 1, SC0A_QT_MORE);

	bdt=abs(qrydata->db_datatype); /* Allow for nullable types */

	if(bdt==DB_LVCH_TYPE || bdt==DB_LBYTE_TYPE  ||
	   bdt==DB_LBIT_TYPE
	   || qrydata->db_length+name_len+15>sizeof(qrytext))
	{
		STcopy("Long Varchar/Byte/Bit Value", qrytext);
	}
	else
	{
		adu_prvalarg(STprintf, qrytext, qrydata);
	}
	status=sc0a_qrytext(scb, qrytext, STlength(qrytext), SC0A_QT_MORE);
	return status;
}

/*
** Name: write_audit(), write an audit record
**
** Description:
**	This routine is the low-level interface which writes audit record
**	from SCF. 
**
** Inputs:
**      SXF_EVENT           type,          Type of event 
**      SXF_ACCESS          access,        Type of access 
**      char                *obj_name,     Object name 
**	i4		    obj_len	   Length of object name
**      DB_OWN_NAME         *obj_owner,    Object owner 
**      i4             msgid,         Message identifier 
**      bool                force_write    Force write to disk 
**	char		    *detailtext	   Detail text/len/num
**	i4		    detaillen
**	i4		    detailnum
**	i4		    userprivs
**      i4             *err_code      Error info 
**
** Outputs:
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	7-jul-93 (robf)
**		Created.
**	11-oct-93 (robf)
**         Prototype corrections to MEcopy
**	30-dec-93 (robf)
**	   Rework SXF session id, if passed from the caller's SCB.
*/
static DB_STATUS
write_audit (
SCD_SCB		    *scb,	  /* Option SCB */
SXF_EVENT           type,         /* Type of event */
SXF_ACCESS          access,       /* Type of access */
char                *obj_name,    /* Object name */
i4		    obj_len,	  /* Length of object */
DB_OWN_NAME         *obj_owner,   /* Object owner */
i4             msgid,        /* Message identifier */
bool                force_write,   /* Force write to disk */
char		    *detailtext,
i4		    detaillen,
i4		    detailnum,
i4		    userprivs,
DB_ERROR             *err_code     /* Error info */
) 
{
	i4         local_error;
	CL_SYS_ERR	syserr;
	DB_STATUS       status;
	SXF_RCB         sxfrcb;


	MEfill(sizeof(sxfrcb), NULLCHAR, (PTR)&sxfrcb);
	sxfrcb.sxf_cb_type    = SXFRCB_CB;
	sxfrcb.sxf_length     = sizeof(sxfrcb);

	if(scb && scb->scb_sscb.sscb_sxscb)
		sxfrcb.sxf_scb = (PTR)scb->scb_sscb.sscb_sxscb;
	else
		sxfrcb.sxf_scb = NULL;

	sxfrcb.sxf_sess_id    = (CS_SID) 0;
	sxfrcb.sxf_objname    = obj_name;
	sxfrcb.sxf_objowner   = obj_owner;
	sxfrcb.sxf_objlength  = obj_len;
	sxfrcb.sxf_auditevent = type;
	sxfrcb.sxf_accessmask = access;
	sxfrcb.sxf_msg_id     = msgid;
	sxfrcb.sxf_detail_txt = detailtext;
	sxfrcb.sxf_detail_len = detaillen;
	sxfrcb.sxf_detail_int = detailnum;
	sxfrcb.sxf_ustat      = userprivs;
	if ( force_write==TRUE)
		sxfrcb.sxf_force = 1;
	else
		sxfrcb.sxf_force = 0;

	status = (*Sc_main_cb->sc_sxf_cptr)(SXR_WRITE, &sxfrcb);

	if (status != E_DB_OK) 
	{
		_VOID_ uleFormat(&sxfrcb.sxf_error, 0, &syserr, ULE_LOG, NULL,
		    (char *)0, 0L, (i4 *)0, &local_error, 0);
		_VOID_ uleFormat(NULL, E_SC023E_SXF_BAD_WRITE, NULL, ULE_LOG, NULL,
		    (char *)0, 0L, (i4 *)0, &local_error, 0);
		/*
		**	Pass error back to caller
		*/
		*err_code = sxfrcb.sxf_error;
		return (status); /* Distinguish between fatal/error */
	}
	return (E_DB_OK);
}
