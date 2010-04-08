/*
** Copyright (c) 2004, 2009 Ingres Corporation
*/

# include	<compat.h>
# include	<er.h>
# include	<st.h>
# include	<cv.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<gca.h>
# include	<generr.h>
# include	<iicgca.h>

/**
+*  Name: cgcutils.c - This file contains utility routines for GCA.
**
**  Description:
**
**  Defines:
**
**	IIcc1_util_error      -	Error message generator.
**	IIcc2_util_type_name  -	Return string for type name.
**	IIcc3_util_status     - Return status string for OS errors.
-*
**  History:
**	16-sep-1987	- Written (ncg)
**	10-feb-1988	- Added IIcc3_util_status (ncg)
**	10-may-1989	- Introduced generic comm error, although cgc_handle
**			  doesn't yet take advantage of it.
**	01-dec-1989	- Updated for Phoenix/Alerters. (bjb)
**	30-jan-1990 (barbara)
**	    In IIcc3_util_status check for presence of OS error before
**	    calling ERlookup.
**      10-dec-1992 (nandak,mani,larrym)
**          Added GCA_XA_SECURE and GCA_XA_RESTART
**	23-aug-1993 (larrym)
**	    Added support for tracing GCA1_INVPROC, needed for owner.procedure
**	16-Nov-95 (gordy)
**	    Added GCA1_FETCH message.
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	 1-Oct-09 (gordy)
**	    Added support for tracing GCA2_INVPROC.
**/


/*{
**  Name: IIcc1_util_error - Issue the GCA service error.
**
**  Description:
**	Issues the GCA originated errors.  Fills a handle structure and
**	calls the user supplied handler (in cgc).
**
**  Inputs:
**	cgc		- Client general communications descriptor.
**	user		- Is this a user error or GCA error.  If this is a user
**			  error the "generic" error number is set to 
**			  GE_SYNTAX_ERROR.
**	errorno		- Local directory error number.  If this is a user
**			  error this gets put into h_local, otherwise h_errorno.
**	numargs		- Number of parameters.
**	a1 - a4		- Up to 4 null-terminated arguments.
**
**  Outputs:
**	Returns:
**		VOID
**	Errors:
**		None
**
**  Side Effects:
**	None
**
**  History:
**	16-sep-1987		Written for GCA. (ncg)
**	18-jan-1989		Hard codes the generic error for user errors.
**				This may be changed in the future if there are
**				more user errors added. (ncg)
*/

VOID
IIcc1_util_error(cgc, user, errorno, numargs, a1, a2, a3, a4)
IICGC_DESC	*cgc;
bool		user;
ER_MSGID	errorno;
i4		numargs;
char		*a1, *a2, *a3, *a4;
{
    IICGC_HARG	handle_arg;

    if (user)
    {
	handle_arg.h_errorno = (i4)GE_SYNTAX_ERROR;
	handle_arg.h_local   = (i4)errorno;
    }
    else
    {
	handle_arg.h_errorno = (i4)GE_COMM_ERROR;
	handle_arg.h_local   = (i4)errorno;
    }
    handle_arg.h_numargs = numargs;
    handle_arg.h_data[0] = a1;
    handle_arg.h_data[1] = a2;
    handle_arg.h_data[2] = a3;
    handle_arg.h_data[3] = a4;
    _VOID_ (*cgc->cgc_handle)(user ? IICGC_HUSERROR : IICGC_HGCERROR,
			      &handle_arg);
} /* IIcc1_util_error */

/*{
**  Name: IIcc2_util_type_name - Given a message type return a name.
**
**  Description:
**	On error messages or debugging the caller may need the type name.
**
**  Inputs:
**	message_type	- Message type.
**	buffer		- Buffer for temp storage, mininum of 30 bytes.
**
**  Outputs:
**	Returns:
**		Pointer to string name, or "GCA_MSG_<hex number>" for
**		undefined name.
**	Errors:
**		None
**
**  Side Effects:
**	None
**
**  History:
**	16-sep-1987		Written for GCA. (ncg)
**	09-may-1988		Added DB procedure messages. (ncg)
**	13-mar-1989		Added GCA_RESTART msg. (bjb)
**	02-aug-1991		Add GCA1_C_INTO and GCA1_C_FROM (teresal) 
**      10-dec-1992             Added GCA_XA_SECURE,GCA_XA_RESTART
**                                 (nandak,mani,larrym)
**	24-may-1993 (kathryn)
**	    Add case for new GCA1_DELETE message.
**	23-aug-1993 (larrym)
**	    added GCA1_INVPROC case.
**	8-nov-93 (robf)
**          added GCA_PROMPT, GCA_PRPRELY messages
**	16-Nov-95 (gordy)
**	    Added GCA1_FETCH message.
**	13-feb-2007 (dougi)
**	    Added GCA2_FETCH message.
**	 1-Oct-09 (gordy)
**	    Added GCA2_INVPROC message.
*/

char *
IIcc2_util_type_name( i4  message_type, char *buffer )
{
    char	*gca_str;

    /* This switch should be a lookup table, but the defines keep changing */
    switch (message_type)
    {
      case GCA_MD_ASSOC:
	gca_str = ERx("GCA_MD_ASSOC");
	break;
      case GCA_ACCEPT:
	gca_str = ERx("GCA_ACCEPT");
	break;
      case GCA_REJECT:
	gca_str = ERx("GCA_REJECT");
	break;
      case GCA_RELEASE:
	gca_str = ERx("GCA_RELEASE");
	break;
      case GCA_Q_BTRAN:
	gca_str = ERx("GCA_Q_BTRAN");
	break;
      case GCA_S_BTRAN:
	gca_str = ERx("GCA_S_BTRAN");
	break;
      case GCA_Q_ETRAN:
	gca_str = ERx("GCA_Q_ETRAN");
	break;
      case GCA_S_ETRAN:
	gca_str = ERx("GCA_S_ETRAN");
	break;
      case GCA_ABORT:
	gca_str = ERx("GCA_ABORT");
	break;
      case GCA_ROLLBACK:
	gca_str = ERx("GCA_ROLLBACK");
	break;
      case GCA_SECURE:
	gca_str = ERx("GCA_SECURE");
	break;
      case GCA_XA_SECURE:
	gca_str = ERx("GCA_XA_SECURE");
	break;
      case GCA_DONE:
	gca_str = ERx("GCA_DONE");
	break;
      case GCA_REFUSE:
	gca_str = ERx("GCA_REFUSE");
	break;
      case GCA_COMMIT:
	gca_str = ERx("GCA_COMMIT");
	break;
      case GCA_QUERY:
	gca_str = ERx("GCA_QUERY");
	break;
      case GCA_DEFINE:
	gca_str = ERx("GCA_DEFINE");
	break;
      case GCA_INVOKE:
	gca_str = ERx("GCA_INVOKE");
	break;
      case GCA_FETCH:
	gca_str = ERx("GCA_FETCH");
	break;
      case GCA1_FETCH:
	gca_str = ERx("GCA1_FETCH");
	break;
      case GCA2_FETCH:
	gca_str = ERx("GCA2_FETCH");
	break;
      case GCA_DELETE:
	gca_str = ERx("GCA_DELETE");
	break;
      case GCA1_DELETE:
	gca_str = ERx("GCA1_DELETE");
	break;
      case GCA_CLOSE:
	gca_str = ERx("GCA_CLOSE");
	break;
      case GCA_ATTENTION:
	gca_str = ERx("GCA_ATTENTION");
	break;
      case GCA_NP_INTERRUPT:
	gca_str = ERx("GCA_NP_INTERRUPT");
	break;
      case GCA_QC_ID:
	gca_str = ERx("GCA_QC_ID");
	break;
      case GCA_TDESCR:
	gca_str = ERx("GCA_TDESCR");
	break;
      case GCA_TUPLES:
	gca_str = ERx("GCA_TUPLES");
	break;
      case GCA_C_INTO:
	gca_str = ERx("GCA_C_INTO");
	break;
      case GCA_C_FROM:
	gca_str = ERx("GCA_C_FROM");
	break;
      case GCA1_C_INTO:
	gca_str = ERx("GCA1_C_INTO");
	break;
      case GCA1_C_FROM:
	gca_str = ERx("GCA1_C_FROM");
	break;
      case GCA_CDATA:
	gca_str = ERx("GCA_CDATA");
	break;
      case GCA_ACK:
	gca_str = ERx("GCA_ACK");
	break;
      case GCA_IACK:
	gca_str = ERx("GCA_IACK");
	break;
      case GCA_RESPONSE:
	gca_str = ERx("GCA_RESPONSE");
	break;
      case GCA_ERROR:
	gca_str = ERx("GCA_ERROR");
	break;
      case GCA_TRACE:
	gca_str = ERx("GCA_TRACE");
	break;
      case GCA_Q_STATUS:
	gca_str = ERx("GCA_Q_STATUS");
	break;
      case GCA_CREPROC:
	gca_str = ERx("GCA_CREPROC");
	break;
      case GCA_DRPPROC:
	gca_str = ERx("GCA_DRPPROC");
	break;
      case GCA_INVPROC:
	gca_str = ERx("GCA_INVPROC");
	break;
      case GCA1_INVPROC:
        gca_str = ERx("GCA1_INVPROC");
        break;
      case GCA2_INVPROC:
        gca_str = ERx("GCA2_INVPROC");
	break;
      case GCA_RETPROC:
	gca_str = ERx("GCA_RETPROC");
	break;
      case GCA_RESTART:
	gca_str = ERx("GCA_RESTART");
	break;
      case GCA_XA_RESTART:
	gca_str = ERx("GCA_XA_RESTART");
	break;
      case GCA_EVENT:
	gca_str = ERx("GCA_EVENT");
	break;
      case GCA_PROMPT:
	 gca_str = ERx("GCA_PROMPT");
	 break;
      case GCA_PRREPLY:
	 gca_str = ERx("GCA_PRREPLY");
	 break;
      default:
	STprintf(buffer, ERx("GCA_MSG_0X%04x"), message_type);
	CVupper(buffer);
	gca_str = buffer;
    }
    return gca_str;
} /* IIcc2_util_type_name */

/*{
**  Name: IIcc3_util_status - Build an OS status string.
**
**  Description:
**	S-prints the status and if there is an OS error also copies that
**	using ERlookup.  Result status string is:
**
**	    GCA status # -- GCA status message string [- OS status]
**
**  Inputs:
**	gca_stat	- Status returned from GCA.
**	os_error	- OS status from GCA.
**
**  Outputs:
**	stat_buf	- Place to put string results.  Must be declared
**			  with length >= IICGC_ELEN + 1.
**	Returns:
**		VOID
**	Errors:
**		None
**
**  Side Effects:
**	None
**
**  History:
**	10-feb-1988	- Written (ncg)
**	07-dec-1988	- Modified to provide the GCA message string as well.
**	26-jan-1989	- Modified interface for ERlookup for OS errors (ncg)
**	08-feb-1989	- Added parameter count to call to ERlookup (bjb)
**	16-may-1989	- Added parameter genericerror (null) to ERlookup (bjb)
**	30-jan-1990 (barbara)
**	    New GCF requires that we check gca_stat for the presence of an
**	    OS error before calling ERlookup -- otherwise ERlookup turns
**	    'oserror' into a garbage error.
*/

VOID
IIcc3_util_status(gca_stat, os_error, stat_buf)
STATUS		gca_stat;
CL_SYS_ERR	*os_error;
char		*stat_buf;	/* IICGC_ELEN + 1 */
{
    i4		elen, slen;			/* Error and string lengths */
    CL_SYS_ERR	err_code;			/* Ignored */
    char	os_buf[IICGC_ELEN + 1];		/* Buffer for OS message */
    STATUS	er_stat;			/* Return from ERlookup */
    char	*gc_msg;			/* GCA message string */
    /*
    ** Use language 1 which is guaranteed to be ok.  The system will return
    ** OS errors in a system dependent way, regardless of our language.
    */
    i4		language = 1;
#   define	E_GC_I2MASK	0xFFFF		/* Low bits of GCA status */

    STprintf(stat_buf, ERx("E_GC%04x"), gca_stat & E_GC_I2MASK);
    if ((gc_msg = ERget((i4)gca_stat)) != (char *)0)
    {
	STcat(stat_buf, ERx(" -- "));		/* Add to buffer */
	slen = STlength(stat_buf);
	elen = STlength(gc_msg);
	STlcopy(gc_msg, stat_buf + slen, min(IICGC_ELEN - slen, elen));
    } /* If read GCA status */

    elen = IICGC_ELEN;

    if (CLERROR(gca_stat))
    {
	er_stat = ERlookup((i4)0, os_error, 0, NULL,
		        os_buf, sizeof(os_buf) - 1, language,
		        &elen, &err_code, 0, (ER_ARGUMENT *)0);
	if (elen > 0)
	{
	    STcat(stat_buf, ERx(" - "));		/* Add to buffer */
	    slen = STlength(stat_buf);
	    STlcopy(os_buf, stat_buf + slen, min(IICGC_ELEN - slen, elen));
	} /* If there was an OS error */
    }

} /* IIcc3_util_status */
