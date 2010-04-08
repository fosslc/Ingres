/*
**    Copyright (c) 1987, 2008 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <er.h>

#include    <efndef.h>
#include    <iledef.h>
#include    <iosbdef.h>
#include    <lnmdef.h>
#include    <iodef.h>
#include    <fabdef.h>
#include    <rabdef.h>
#include    <ssdef.h>
#include    <descrip.h>
#include    <cs.h>
#include    <starlet.h>
#include    <vmstypes.h>
#include    <astjacket.h>
/**
**
**  Name: ER.C - Implements the ER compatibility library routines.
**
**  Description:
**      The module contains the routines that implement the ER logging
**	portion	of the compatibility library.
**
**          ERlog     - Send message to ERreceive
**	    ERsend    - Send message to ERreceive	- Now Obsolete
**	    ERreceive - Receive message from  ERsend	- Now Obsolete
**
**  History:    
**      02-oct-1985 (derek)    
**          Created new for 5.0.
**	18-Jun-1986 (fred)
**	    Changed location of errmsg.msg.  Now, if xDEV_TEST is defined,
**	    ERlookup() will look for the errmsg.msg in jpt_files:errmsg.msg
**	    as opposed to simply looking in the current directory.  Users
**	    who wish to keep their own copy of errmsg.msg may, of course,
**	    locally redefine jpt_files to be where they wish it to be.
**	17-Sep-86 (ericj)
**	    Changed null-terminated string formatting in ERlookup().
**	13-mar-87 (peter)
**	    Kanji integration. ERlookup and ERreport are now in
**	    separate files.
**	06-Aug-1987 (fred)
**	    Change name of log file from ii_files:errlog.log to
**	    II_CONFIG:errlog.log.
**      17-apr-89 (Jennifer)
**          Added new flag to ERsend to indicate if error message 
**          or security audit message.  Code ERsend for security audit
**          which writes to a mailbox (or other IPC mechanism).
**          Also coded the ERrecieve routine which was not used before
**          and a mechanism to send messages to an operator.
**	28-sep-1992 (pholman)
**	    Following CL committee of 19-sep-1992, mark ERsend and
**	    ERreceive as obsolete, and introduce ERlog (which is 
**	    similar to ERsend before the new parameter was added.
**      16-jul-93 (ed)
**	    added gl.h
**      11-aug-93 (ed)
**          added missing includes
**	11-nov-97 (kinte01)
**	    manually integrate changes by rosga02 from vms cl
**	    Initialized some static switches, that was assumed to be 0
**	19-jul-2000 (kinte01)
**	    Correct prototype definitions by adding missing includes
**	26-oct-2001 (kinte01)
**	    Clean up compiler warnings
**	05-sep-2003 (abbjo03)
**	    Changes to use VMS headers provided by HP.
**	09-oct-2008 (stegr01/joea)
**	    Replace II_VMS_ITEM_LIST_3 by ILE3.
**	29-oct-2008 (joea)
**	    Use EFN$C_ENF and IOSB when calling a synchronous system service.
**	11-nov-2008 (joea)
**	    Use CL_CLEAR_ERR to initialize CL_ERR_DESC.
**       22-dec-2008 (stegr01)
**          Itanium VMS port.
**          
**/
static	VOID		    rms_resume();   /* Wait for RMS completion. */

/*{
** Name: ERsend	- Send message to ERreceive.
**
** Description:
**      This procedure sends a message that can be read by ERreceive. 
**      Normally there are multiple sender processes and a single receiving 
**      process.  The sending processes would use ERsend and the receiving
**	process would use ERreceive.  It is ok for the receiving process to
**	also send messages.  The first time through ERsend a attachment is
**	made to the ERreceive mailbox, and the mailbox is left open from
**	then on.  If no mailbox currently exists, then error logging is
**	not performed.
**
**      For error message files, ERsend currently uses a write once 
**      sharable file instead of the mechanism described above.  For
**      the security audit messages it will use the described mechanism.
**      
**      For operator messages it will use an operating system interface
**      to send messages to an operator.
** Inputs:
**      flag                            Indicate what type of message
**                                      and must be set to ER_ERROR_MSG,
**                                      ER_AUDIT_MSG, or ER_OPER_MSG.
**      message                         Address of buffer containing the 
**                                      message.
**      msg_length                      Length of the message.
**
** Outputs:
**      err_code                        Operating system error code.
**	Returns:
**	    OK
**	    ER_BADOPEN
**	    ER_BADSEND
**	    ER_BADPARAM
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	02-oct-1985 (derek)
**          Created new for 5.0.
**      17-apr-89 (Jennifer)
**          Added new flag to ERsend to indicate if error message 
**          or security audit message.  Code ERsend for security audit
**          which writes to a mailbox (or other IPC mechanism).  Also
**          added support for writing to an operator.
**	29-sep-1992 (pholman)
**	    The use of ERsend with an extra parameter (flag) was stillborn,
**	    and the function is now obsolete.  Use ERlog instead.
*/
STATUS
ERsend(flag, message, msg_length, err_code)
i4                flag;
char               *message;
i4		   msg_length;
CL_ERR_DESC	   *err_code;
{
    static int		e_er_ifi = 0;
    static int		e_er_isi = 0;
    static int		e_er_open_attempted = 0;
    STATUS		status;
    FABDEF		e_fab;
    RABDEF		e_rab;
    static int		a_er_open_attempted = 0;
    static II_VMS_CHANNEL	a_channel = 0;
    FABDEF		*fab;
    RABDEF		*rab;
    static  i4	request_no;

    CL_CLEAR_ERR(err_code);
    
    /*	Check for bad paramters. */

    if ((message == 0 || msg_length == 0) && flag != ER_AUDIT_MSG)
	return (ER_BADPARAM);

    if ((flag != ER_ERROR_MSG) && (flag != ER_AUDIT_MSG) &&
       ( flag != ER_OPER_MSG))
        return (ER_BADPARAM);

    switch (flag)
    {
    case ER_AUDIT_MSG:
     
	if (a_er_open_attempted == 0 && a_channel == 0)
        {
	    struct dsc$descriptor_s	er_name;
	    char			er_name_buffer[16];
	    ILE3			lnm_item_list[2];
	    unsigned short		inst_len = 0;
	    $DESCRIPTOR			(inst_lnm_dsc, "II_INSTALLATION");
	    $DESCRIPTOR			(file_dev_dsc, "LNM$FILE_DEV");

	    /*  Construct name for AUDIT mailbox. */

	    MEcopy("II_AUDIT_", 9, er_name_buffer);
	    lnm_item_list[0].ile3$w_code = LNM$_STRING;
	    lnm_item_list[0].ile3$w_length = 2;
	    lnm_item_list[0].ile3$ps_retlen_addr = &inst_len;
	    lnm_item_list[0].ile3$ps_bufaddr = &er_name_buffer[9];
	    lnm_item_list[1].ile3$w_code = 0;
	    lnm_item_list[1].ile3$w_length = 0;
	    status = sys$trnlnm(0, &file_dev_dsc, &inst_lnm_dsc, 0,
		lnm_item_list);
	    if (status == SS$_NORMAL && inst_len == 2)
	    {
		MEcopy("_IPC", 4, &er_name_buffer[11]);
		er_name.dsc$w_length = 15;
	    }
	    else
	    {
		MEcopy("IPC", 3, &er_name_buffer[9]);
		er_name.dsc$w_length = 12;
	    }
	    er_name.dsc$a_pointer = er_name_buffer;

	    status = sys$assign(&er_name, &a_channel, 0, 0);
	    if ((status & 1) == 0)
	    {
		err_code->error = status;
		return (ER_BADSEND);
	    }
	}
	if (a_channel)
	{
	    CS_SID	sid;
	    IOSB	iosb;

	    /*	Handle special case to connect only but not send message. */

	    if (msg_length == 0 && message == 0)
		return (OK);

	    CSget_sid(&sid);

#ifdef OS_THREADS_USED

#ifdef ALPHA
            /* Alpha can be Internal or OS threaded */
            if (CS_is_mt())
            {
#endif
               status = sys$qiow(EFN$C_ENF, a_channel, IO$_WRITEVBLK|IO$M_NOW, &iosb,
                                 NULL, 0, message, msg_length, 0, 0, 0, 0);

               if ((status & 1) == 0)
               {
                  err_code->error = status;
                  return (ER_BADSEND);
               }
#ifdef ALPHA
            }
            else
            {
#endif

#endif /* OS_THREADS_USED */

#if defined(ALPHA) || !defined(OS_THREADS_USED)
            
	       status = sys$qio(0, a_channel, IO$_WRITEVBLK|IO$M_NOW, &iosb,
	       	                CSresume, sid, message, msg_length, 0, 0, 0, 0);

	       if ((status & 1) == 0)
	       {
	          err_code->error = status;
		  return (ER_BADSEND);
	       }
	       CSsuspend(0, 0, 0);	
#endif

#if defined(ALPHA) && defined(OS_THREADS_USED)
            }
#endif
	    if ((iosb.iosb$w_status & 1) == 0)
	    {
		err_code->error = iosb.iosb$w_status;
		return (ER_BADSEND);
	    }
	    return (OK);
	}
	return (ER_BADSEND);	

    case ER_ERROR_MSG:

	/*	Make sure that the channel is open. */
	    
	if (e_er_open_attempted == 0 && e_er_ifi == 0)
	{
	    e_er_open_attempted++;
	    fab = &e_fab; 
	    rab = &e_rab;

	    /*  Open I/O channel or file. */

	    MEfill(sizeof(e_fab), 0, (PTR)fab);
	    MEfill(sizeof(e_rab), 0, (PTR)rab);
	    fab->fab$b_bid = FAB$C_BID;
	    fab->fab$b_bln = FAB$C_BLN;
	    fab->fab$l_fna = "II_DBMS_CONFIG:ERRLOG.LOG";
	    fab->fab$b_fns = 25;
	    fab->fab$b_rfm = FAB$C_VAR;
	    fab->fab$b_rat = FAB$M_CR;
	    fab->fab$l_fop = FAB$M_CIF;
	    fab->fab$b_shr = FAB$M_SHRPUT | FAB$M_SHRGET;
	    fab->fab$b_fac = FAB$M_PUT;
	    status = sys$create(fab);
	    if ((status & 1) == 0)
	    {
		err_code->error = status;
		return (ER_BADSEND);
	    }
	    rab->rab$b_bid = RAB$C_BID;
	    rab->rab$b_bln = RAB$C_BLN;
	    rab->rab$l_fab = fab;
	    rab->rab$l_rop = RAB$M_EOF;
	    status = sys$connect(rab);
	    if ((status & 1) == 0)
	    {
		sys$close(fab);
		err_code->error = status;
		return (ER_BADSEND);
	    }
	    e_er_ifi = fab->fab$w_ifi;
	    e_er_isi = rab->rab$w_isi;
	}

	if  (flag == ER_ERROR_MSG && e_er_isi)
	{
	    rab = &e_rab;
	    fab = &e_fab;
	    MEfill(sizeof(e_rab), 0, (PTR)rab);
	    rab->rab$b_bid = RAB$C_BID;
	    rab->rab$b_bln = RAB$C_BLN;
	    rab->rab$l_rbf = message;
	    rab->rab$w_rsz = msg_length;
	    rab->rab$w_isi = e_er_isi;
	    status = sys$put(rab);
	    if ((status & 1) == 0)
	    {
		err_code->error = status;
		return (ER_BADSEND);
	    }
	    status = sys$flush(rab);
	    if ((status & 1) == 0)
	    {
		err_code->error = status;
		return (ER_BADSEND);
	    }
	    return (OK);
	}

	return (ER_BADSEND);	

    case ER_OPER_MSG:
	{	    
	    i4    length;
	    struct
	    {
		    unsigned i4	opc$b_ms_type:8;	
					    /*  Message type. */
#define			    OPC$_RQ_RQST	    3
		    unsigned i4	opc$b_ms_target:24;	
					    /*  Message target. */
#define			    OPC$M_NM_OPER12	    0x800000
		    unsigned i4	opc$l_ms_rqstid;	
					    /*  Request id. */
		    char		opc$b_ms_text[ER_OPR_MAX];	
					    /*  Text of message. */
	    }opc;
	    struct
	    {
		i4	    length;
		char	    *buffer;
	    }opc_desc;

	    opc.opc$b_ms_type = OPC$_RQ_RQST;
	    opc.opc$b_ms_target |= OPC$M_NM_OPER12;
	    opc.opc$l_ms_rqstid = ++request_no;
	    length = msg_length;
	    if (length > sizeof(opc.opc$b_ms_text))
		length = sizeof(opc.opc$b_ms_text);
	    MEcopy(message, length, opc.opc$b_ms_text);
	    opc_desc.length = sizeof(opc) - sizeof(opc.opc$b_ms_text) + length;
	    opc_desc.buffer = (PTR)&opc;
	    sys$sndopr(&opc_desc, 0); 
	    return (OK);

	} /* End case ER_OPER_MSG */

    }  /* End switch */

}

/*{
** Name: ERreceive - Receive message from  ERsend (obsolete).
**
** Description:
**      This procedure receives a message that was sent by ERsend.
**      Normally there are multiple sender processes and a single receiving 
**      process.  The sending processes would use ERsend and the receiving
**	process would use ERreceive.  It is ok for the receiving process to
**	also send messages.  The first time through ERreceive the mailbox
**      or other IPC mechanism is created.  The name is a INGRES environment
**      variable (logical) called II_AUDIT_IPC.  The mailbox is left open from
**	then on.  This mechanism is currently used only for security 
**      auditing.  If no mailbox (or other IPC channel) currently exists, 
**      then security logging cannot be performed.  This is considered 
**      fatal.
**
**      If the recieve buffer is too small to hold the message, then 
**      the message is truncated and ER_TRUNCATED is returned.
** Inputs:
**      flag                            Indicate what type of message
**                                      and must be set to
**                                      ER_AUDIT_MSG.
**      buffer                          Address of buffer to contain the 
**                                      message.
**      buf_length                      Length of the buffer.
**
** Outputs:
**      msg_length                      Pointer to area for length of the 
**                                      message.
**      err_code                        Operating system error code.
**	Returns:
**	    OK
**	    ER_BADOPEN
**          ER_NO_AUDIT
**	    ER_BADRCV
**	    ER_BADPARAM
**          ER_TRUNCATED
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      17-apr-89 (Jennifer)
**          Coded for Orange.
**	28-sep-1992 (pholman)
**	    Security Audit Records will no longer be sent to an external
**	    process via ER, instead this functionality will be performed
**	    via calls to the SXF.
*/
STATUS
ERreceive(flag, buffer, buf_length, msg_length, err_code)
i4                flag;
char               *buffer;
i4                buf_length;
i4		   *msg_length;
CL_ERR_DESC	   *err_code;
{
    static II_VMS_CHANNEL	er_channel = 0;
    STATUS         status;
    IOSB	   er_iosb;
    
    CL_CLEAR_ERR(err_code);
    if (er_channel == 0)
    {
	struct dsc$descriptor_s	er_name;
	char			er_name_buffer[16];
	unsigned short		inst_len = 0;
	$DESCRIPTOR		(inst_lnm_dsc, "II_INSTALLATION");
	$DESCRIPTOR		(file_dev_dsc, "LNM$FILE_DEV");
	ILE3			lnm_item_list[2];

	/*  Construct name for AUDIT mailbox. */

	MEcopy("II_AUDIT_", 9, er_name_buffer);
	lnm_item_list[0].ile3$w_code = LNM$_STRING;
	lnm_item_list[0].ile3$w_length = 2;
	lnm_item_list[0].ile3$ps_retlen_addr = &inst_len;
	lnm_item_list[0].ile3$ps_bufaddr = &er_name_buffer[9];
	lnm_item_list[1].ile3$w_code = 0;
	lnm_item_list[1].ile3$w_length = 0;
	status = sys$trnlnm(0, &file_dev_dsc, &inst_lnm_dsc, 0, lnm_item_list);
	if (status == SS$_NORMAL && inst_len == 2)
	{
	    MEcopy("_IPC", 4, &er_name_buffer[11]);
	    er_name.dsc$w_length = 15;
	}
	else
	{
	    MEcopy("IPC", 3, &er_name_buffer[9]);
	    er_name.dsc$w_length = 12;
	}
	er_name.dsc$a_pointer = er_name_buffer;

        status = sys$crembx(1, &er_channel, ER_MAX_LEN, 
                      ER_MSG_MAX * ER_MAX_LEN, 0,0, &er_name);	
        if ((status & 1) == 0)
	{	    
	    err_code->error = status;
	    return (ER_NO_AUDIT);
        }
	status = sys$delmbx(er_channel);
        if ((status & 1) == 0)
	{	    
	    sys$dassgn(er_channel);
	    err_code->error = status;
	    return (ER_NO_AUDIT);
        }
    }
    status = sys$qiow(EFN$C_ENF,er_channel, IO$_READVBLK, &er_iosb, 
                      0,0, buffer, buf_length, 0,0,0,0);
    if (status & 1)
	status = er_iosb.iosb$w_status;
    if ((status & 1) == 0)
    {
	err_code->error = status;
	return (ER_BADRCV);    
    }
    *msg_length = er_iosb.iosb$w_bcnt;
    if (er_iosb.iosb$w_status == SS$_BUFFEROVF)
	return (ER_TRUNCATED);
    return (OK);
}

/*{
** Name: ERlog	- Send message to the error logger.
**
** Description:
**	This procedure sends a message to the system specific error
**	logger (currently an error log file).
**
** Inputs:
**      message                         Address of buffer containing the message.
**      msg_length                      Length of the message.
**
** Outputs:
**      err_code                        Operating system error code.
**	Returns:
**	    OK
**	    ER_BADOPEN
**	    ER_BADSEND
**	    ER_BADPARAM
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	29-sep-1992 (pholman)
**	    First created, taken from original (6.4) version of ERsend
*/
STATUS
ERlog(message, msg_length, err_code)
char               *message;
i4		   msg_length;
CL_ERR_DESC	   *err_code;
{
    static int		er_ifi = 0;
    static int		er_isi = 0;
    static int		er_open_attempted = 0;
    STATUS		status;
    FABDEF		fab;
    RABDEF		rab;

    CL_CLEAR_ERR(err_code);
    
    /*	Check for bad paramters. */

    if (message == 0 || msg_length == 0)
	return (ER_BADPARAM);

    /*	Make sure that the channel is open. */

    if (er_open_attempted == 0 && er_ifi == 0)
    {
	/*  Dont't try and open every time if the first time failed. */

	er_open_attempted++;

	/*  Open RMS file. */

	MEfill(sizeof(fab), 0, (PTR)&fab);
	MEfill(sizeof(rab), 0, (PTR)&rab);
	fab.fab$b_bid = FAB$C_BID;
	fab.fab$b_bln = FAB$C_BLN;
	fab.fab$l_fna = "II_CONFIG:ERRLOG.LOG";
	fab.fab$b_fns = 20;
	fab.fab$b_rfm = FAB$C_VAR;
	fab.fab$b_rat = FAB$M_CR;
	fab.fab$l_fop = FAB$M_CIF;
	fab.fab$b_shr = FAB$M_SHRPUT | FAB$M_SHRGET;
	status = sys$create(&fab);
	if ((status & 1) == 0)
	    return (ER_BADSEND);
	rab.rab$b_bid = RAB$C_BID;
	rab.rab$b_bln = RAB$C_BLN;
	rab.rab$l_fab = &fab;
	rab.rab$l_rop = RAB$M_EOF;
	status = sys$connect(&rab);
	if ((status & 1) == 0)
	{
	    sys$close(&fab);
	    return (ER_BADSEND);
	}
	er_ifi = fab.fab$w_ifi;
	er_isi = rab.rab$w_isi;
    }

    if (er_isi)
    {
	MEfill(sizeof(rab), 0, (PTR)&rab);
	rab.rab$b_bid = RAB$C_BID;
	rab.rab$b_bln = RAB$C_BLN;
	rab.rab$w_isi = er_isi;
	rab.rab$l_rbf = message;
	rab.rab$w_rsz = msg_length;
	status = sys$put(&rab);
	if ((status & 1) == 0)
	    return (ER_BADSEND);
	status = sys$flush(&rab);
	if ((status & 1) == 0)
	    return (ER_BADSEND);
	return (OK);
    }

    return (ER_BADSEND);	
}
