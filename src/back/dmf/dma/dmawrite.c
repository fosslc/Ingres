/*
**Copyright (c) 2004 Ingres Corporation
*/
 
#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <tr.h>
#include    <er.h>
#include    <me.h>
#include    <pc.h>
#include    <tm.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <adf.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <scf.h>
#include    <sxf.h>
#include    <lg.h>
#include    <lk.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmxcb.h>
#include    <dml.h>
#include    <dma.h>

/*{
** Name: DMAWRITE.C - Format and write security audit records for DMF
**
**      This file contains:
**
**	dma_write_audit()	write audit record, no accompanying text
**	dma_write_audit1()	write audit record w. accompanying text.
**
** History:
**	17-nov-1992 (robf)
**		Created.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**		Include <lg.h> to get logging system type definitions.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Include <lk.h> to get locking system type definitions.
**	    Include <pc.h> before <lk.h> to pick up PID definition
**	26-jul-1993 (rogerk)
**	    Include respective CL headers for all cl calls.
**	08-sep-93 (swm)
**	    Changed sid type from i4  to CS_SID to match recent CL
**	    interface revision. Removed casts of session ids to PTR.
**	13-oct-93 (robf)
**          Fix protype problems.
**	30-dec-93 (robf)
**          Set SXF session id/scb to NULL. It was wrong before and ignored
**	    by SXF.
**	9-mar-94 (stephenb)
**	    add new parameter dbname and use it in sxfrcb.
**	14-mar-94 (stephenb)
**	    Add new parameter to allow us to audit one database while
**	23-june-1998(angusm)
**	    Clone dma_write_audit1 from dma_write_audit to allow
**	    detailed audit of OS interface of ckpdb/rollforwarddb
**	13-Sep-1999 (jenjo02)
**	    Return immediately if neither C2 nor B1 security.
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**	12-Apr-2008 (kschendel)
**	    Need to include adf.h now.
**	08-Dec-2008 (jonj)
**	    SIR 120874: dma_? auditing functions converted to DB_ERROR *,
**	    use new form uleFormat.
**	02-Nov-2010 (jonj) SIR 124685 Prototype Cleanup
**	    Add include of dma.h
*/
/* Name: dma_write_audit()
**
** Description:
**      The routine dma_write_audit() is the interface from DMF to SXF
**	allowing audit records to be formatted and written.
**      If the message cannot be sent, this is considered a 
**      severe error.
**
** Inputs:
**      SXF_EVENT           type,          Type of event 
**      SXF_ACCESS          access,        Type of access 
**      char                *obj_name,     Object name 
**      i4                  obj_len,       Object name length
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
**	17-nov-1992 (robf)
**		Created.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**		Include <lg.h> to get logging system type definitions.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Include <lk.h> to get locking system type definitions.
**	    Include <pc.h> before <lk.h> to pick up PID definition
**	26-jul-1993 (rogerk)
**	    Include respective CL headers for all cl calls.
**	08-sep-93 (swm)
**	    Changed sid type from i4  to CS_SID to match recent CL
**	    interface revision. Removed casts of session ids to PTR.
**	13-oct-93 (robf)
**          Fix protype problems.
**	30-dec-93 (robf)
**          Set SXF session id/scb to NULL. It was wrong before and ignored
**	    by SXF.
**	9-mar-94 (stephenb)
**	    add new parameter dbname and use it in sxfrcb.
**	14-mar-94 (stephenb)
**	    Add new parameter to allow us to audit one database while
**	    connected to another.
**	13-Sep-1999 (jenjo02)
**	    Return immediately if neither C2 nor B1 security.
*/
DB_STATUS
dma_write_audit (
       SXF_EVENT           type,         /* Type of event */
       SXF_ACCESS          access,       /* Type of access */
       char                *obj_name,    /* Object name */
       i4                  obj_len,      /* Object len */
       DB_OWN_NAME         *obj_owner,   /* Object owner */
       i4             msgid,        /* Message identifier */
       bool                force_write,   /* Force write to disk */
       DB_ERROR            *dberr,     	 /* Error info */
       DB_DB_NAME	   *dbname	  /* Database name */
) 
{
    i4         local_error;
    CL_ERR_DESC	    syserr;
    DB_STATUS       status;
    SXF_RCB     sxfrcb;
    CS_SID	    sid;

    CLRDBERR(dberr);

    /* Do nothing if security auditing is not enabled */
    if ( (dmf_svcb->svcb_status & (SVCB_C2SECURE)) == 0 )
	return(E_DB_OK);

    /*
    **	Map QEF message ids to SXF message ids.
    */
    if((msgid >= DB_QEF_ID*4096L) && 
       (msgid <  (DB_QEF_ID+1)*4096L))
    {
	msgid-= DB_QEF_ID*4096;
	msgid+= DB_SXF_ID*4096;
    }

    /*
    **	Set up the SXF call 
    */
    MEfill(sizeof(sxfrcb), NULLCHAR, (PTR)&sxfrcb);
    sxfrcb.sxf_cb_type    = SXFRCB_CB;
    sxfrcb.sxf_length     = sizeof(sxfrcb);

    sxfrcb.sxf_sess_id    = (CS_SID)0;
    sxfrcb.sxf_scb        = NULL;
    sxfrcb.sxf_objname    = obj_name;
    sxfrcb.sxf_objowner   = obj_owner;
    sxfrcb.sxf_objlength  = obj_len;
    sxfrcb.sxf_auditevent = type;
    sxfrcb.sxf_accessmask = access;
    sxfrcb.sxf_msg_id     = msgid;
    sxfrcb.sxf_db_name	  = dbname;

    if ( force_write==TRUE)
	sxfrcb.sxf_force = 1;
    else
	sxfrcb.sxf_force = 0;

    status = sxf_call(SXR_WRITE, &sxfrcb);

    if (status != E_DB_OK) 
    {
	(VOID) uleFormat(&sxfrcb.sxf_error, 0, &syserr, ULE_LOG, NULL,
	    (char *)NULL, 0L, (i4 *)NULL, &local_error, 0);
	SETDBERR(dberr, 0, E_DM0126_ERROR_WRITING_SECAUDIT);
	return (E_DB_ERROR);
    }

    return (E_DB_OK);
}

/* 
** Name: dma_write_audit1()
**
** Description: 
**      The routine dma_write_audit1() is the interface from DMF to SXF
**	allowing audit records plus text. to be formatted and written.
**
** Inputs:
**      SXF_EVENT           type,          Type of event 
**      SXF_ACCESS          access,        Type of access 
**      char                *obj_name,     Object name 
**      i4                  obj_len,       Object name length
**      DB_OWN_NAME         *obj_owner,    Object owner 
**      PTR                 detail_txt,    detail text 
**      i4             detail_int,    detail integer 
**      i4             detail_len,    length of detail text 
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
**	20-oct-1997(angusm)
**	Cloned from dma_write_audit().
**	13-Sep-1999 (jenjo02)
**	    Return immediately if neither C2 nor B1 security.
*/
DB_STATUS
dma_write_audit1 (
       SXF_EVENT           type,         /* Type of event */
       SXF_ACCESS          access,       /* Type of access */
       char                *obj_name,    /* Object name */
       i4                  obj_len,      /* Object len */
       DB_OWN_NAME         *obj_owner,   /* Object owner */
       i4             msgid,        /* Message identifier */
       bool                force_write,   /* Force write to disk */
       DB_ERROR            *dberr,       /* Error info */
       DB_DB_NAME	   *dbname,	  /* Database name */
       PTR 		   detail_txt,   /* detail text */
       i4		   detail_len,   /* length of detail text */
       i4		   detail_int   /* detail integer */
) 
{
    i4         local_error;
    CL_ERR_DESC	    syserr;
    DB_STATUS       status;
    SXF_RCB     sxfrcb;
    CS_SID	    sid;

    CLRDBERR(dberr);

    /* Do nothing if security auditing is not enabled */
    if ( (dmf_svcb->svcb_status & (SVCB_C2SECURE)) == 0 )
	return(E_DB_OK);

    /*
    **	Map QEF message ids to SXF message ids.
    */
    if((msgid >= DB_QEF_ID*4096L) && 
       (msgid <  (DB_QEF_ID+1)*4096L))
    {
	msgid-= DB_QEF_ID*4096;
	msgid+= DB_SXF_ID*4096;
    }

    /*
    **	Set up the SXF call 
    */
    MEfill(sizeof(sxfrcb), NULLCHAR, (PTR)&sxfrcb);
    sxfrcb.sxf_cb_type    = SXFRCB_CB;
    sxfrcb.sxf_length     = sizeof(sxfrcb);

    sxfrcb.sxf_sess_id    = (CS_SID)0;
    sxfrcb.sxf_scb        = NULL;
    sxfrcb.sxf_objname    = obj_name;
    sxfrcb.sxf_objowner   = obj_owner;
    sxfrcb.sxf_objlength  = obj_len;
    sxfrcb.sxf_detail_txt = detail_txt;
    sxfrcb.sxf_detail_int = detail_int;
    sxfrcb.sxf_detail_len = detail_len;
    sxfrcb.sxf_auditevent = type;
    sxfrcb.sxf_accessmask = access;
    sxfrcb.sxf_msg_id     = msgid;
    sxfrcb.sxf_db_name	  = dbname;

    if ( force_write==TRUE)
	sxfrcb.sxf_force = 1;
    else
	sxfrcb.sxf_force = 0;

    status = sxf_call(SXR_WRITE, &sxfrcb);

    if (status != E_DB_OK) 
    {
	(VOID) uleFormat(&sxfrcb.sxf_error, 0, &syserr, ULE_LOG, NULL,
	    (char *)NULL, 0L, (i4 *)NULL, &local_error, 0);
	SETDBERR(dberr, 0, E_DM0126_ERROR_WRITING_SECAUDIT);
	return (E_DB_ERROR);
    }

    return (E_DB_OK);
}
