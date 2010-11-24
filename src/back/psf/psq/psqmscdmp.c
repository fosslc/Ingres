/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <qu.h>
#include    <tr.h>
#include    <bt.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <adf.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <scf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <uld.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <psftrmwh.h>

/**
**
**  Name: PSQMSCDMP.C - Miscellaneous dump routines
**
**  Description:
**      This file contains miscellaneous dump routines, for printing out 
**      commonly used structures.
**
**          psq_headdmp - Dump the header info that appears at the beginning
**			    of each control block
**          psq_siddmp - Dump a session id
**          psq_lngdmp - Dump a language specification
**          psq_decdmp - Dump a decimal marker specification
**          psq_dstdmp - Dump a distributed/non-distributed specification
**          psq_booldmp - Dump a boolean
**          psq_modedmp - Dump a query mode number
**          psq_tbiddmp - Dump a table id
**          psq_ciddmp - Dump a cursor id
**          psq_errdmp - Dump an error block
**	    psq_str_dmp - Dump a string
**
**
**  History:    $Log-for RCS$
**      10-jan-86 (jeff)    
**          written
**	14-nov-88 (stec)
**	    Initialize ADF_CB on the stack.
**	6-apr-1992 (bryanp)
**	    New query modes: PSQ_DGTT, PSQ_DGTT_AS_SELECT
**	03-nov-92 (jrb)
**	    Changed PSQ_SEMBED in trace message to PSQ_SWORKLOC since I'm
**	    reusing this query mode.
**	06-nov-1992 (rog)
**	    <ulf.h> must be included before <qsf.h>.
**	10-feb-93 (rblumer)
**	    Added PSQ_DISTCREATE and other new query modes to psq_modedmp.
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**	11-oct-93 (tad)
**	    Bug #56449
**	    Changed TRdisplay format args from %x to %p where necessary to
**	    display pointer types. We need to distinguish this on, eg 64-bit
**	    platforms, where sizeof(PTR) > sizeof(int).
**	10-Jan-2001 (jenjo02)
**	    ADF_CB* is available in PSS_SESBLK, remove SCU_INFORMATION
**	    call to get it, use psf_sesscb() instead.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
i4 psq_headdmp(
	PSQ_CBHEAD *cb_head);
i4 psq_siddmp(
	CS_SID *sessid);
i4 psq_lngdmp(
	DB_LANG qlang);
i4 psq_decdmp(
	DB_DECIMAL *decimal);
i4 psq_dstdmp(
	DB_DISTRIB distrib);
i4 psq_booldmp(
	i4 boolval);
i4 psq_modedmp(
	i2 mode);
i4 psq_tbiddmp(
	DB_TAB_ID *tabid);
i4 psq_ciddmp(
	DB_CURSOR_ID *cursid);
i4 psq_errdmp(
	DB_ERROR *errblk);
i4 psq_dtdump(
	DB_DT_ID dt_id);
i4 psq_tmdmp(
	DB_TAB_TIMESTAMP *timestamp);
i4 psq_jrel_dmp(
	PST_J_MASK *inner_rel,
	PST_J_MASK *outer_rel);
i4 psq_upddmp(
	i4 updtmode);
i4 psq_str_dmp(
	unsigned char *str,
	i4 len);

/*{
** Name: psq_headdmp	- Dump a control block header
**
** Description:
**      This function dumps the header information that appears at the 
**      beginning of each control block.
**
** Inputs:
**      cb_head                         Pointer to header information
**
** Outputs:
**      NONE
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_FATAL			Function failed due to some internal
**					problem.  Error status not in control
**					blocks.
**	Exceptions:
**	    none
**
** Side Effects:
**	    Sends output to terminal and/or log file.
**
** History:
**	10-jan-86 (jeff)
**          written
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
*/
DB_STATUS
psq_headdmp(
	PSQ_CBHEAD         *cb_head)
{
    if (TRdisplay("\tControl Block Header:\n") != TR_OK)
	return (E_DB_ERROR);
    if (TRdisplay("\t\tnext:\t0x0%p\n", cb_head->psq_cbnext) != TR_OK)
	return (E_DB_ERROR);
    if (TRdisplay("\t\tprev:\t0x0%p\n", cb_head->psq_cbprev) != TR_OK)
	return (E_DB_ERROR);
    if (TRdisplay("\t\tlength:\t%d\n", cb_head->psq_cblength) != TR_OK)
	return (E_DB_ERROR);
    if (TRdisplay("\t\ttype:\t%d\n", cb_head->psq_cbtype) != TR_OK)
	return (E_DB_ERROR);
    if (TRdisplay("\t\ts_reserved:\t%d\n", cb_head->psq_cb_s_reserved) != TR_OK)
	return (E_DB_ERROR);
    if (TRdisplay("\t\tl_reserved:\t%d\n", cb_head->psq_cb_l_reserved) != TR_OK)
	return (E_DB_ERROR);
    if (TRdisplay("\t\towner:\t%d\n", cb_head->psq_cb_owner) != TR_OK)
	return (E_DB_ERROR);
    if (TRdisplay("\t\tascii_id:\t%4.4s\n", (char *) &cb_head->psq_cb_ascii_id)
	!= TR_OK)
	return (E_DB_ERROR);

    return (E_DB_OK);
}

/*{
** Name: psq_siddmp	- Dump a session id
**
** Description:
**      This function dumps a session id.
**
** Inputs:
**      sessid                          Pointer to a session id
**
** Outputs:
**      NONE
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_FATAL			Failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    Sends output to terminal and/or log file
**
** History:
**	10-jan-86 (jeff)
**          written
**	08-sep-93 (swm)
**	    Changed sessid parameter to psq_siddmp() function to CS_SID
**	    to reflect recent CL interface revision.
*/
DB_STATUS
psq_siddmp(
	CS_SID          *sessid)
{
    /*
    ** A CS_SID must be able to hold an SCB_CB pointer or a nat; output
    ** with %p since its definition is exported by the CL. This will avoid
    ** value truncation on platforms where sizeof(PTR) > sizeof(i4).
    */
    if (TRdisplay("%p", (PTR)*sessid) != TR_OK)
	return (E_DB_ERROR);
    return (E_DB_OK);
}

/*{
** Name: psq_lngdmp	- Dump a query language id
**
** Description:
**      This function dumps a query language id.
**
** Inputs:
**      qlang                           A query language id
**
** Outputs:
**      NONE
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_FATAL			Failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    Sends output to terminal and/or log file
**
** History:
**	10-jan-86 (jeff)
**          written
*/
DB_STATUS
psq_lngdmp(
	DB_LANG            qlang)
{
    STATUS              status;

    if (qlang == DB_NOLANG)
	status = TRdisplay("No Query Language");
    else
	status = TRdisplay("%v", "QUEL,SQL", qlang);

    if (status == TR_OK)
	return (E_DB_OK);
    else
	return (E_DB_ERROR);
}

/*{
** Name: psq_decdmp	- Dump a decimal marker description
**
** Description:
**      This function dumps a decimal marker description.  If the decimal 
**      marker is not specified, it says so.
**
** Inputs:
**      decimal                         Pointer to decimal marker description
**
** Outputs:
**      NONE
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_FATAL			Failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    Sends output to terminal and/or log file
**
** History:
**	10-jan-86 (jeff)
**          written
*/
DB_STATUS
psq_decdmp(
	DB_DECIMAL         *decimal)
{
    STATUS              status;

    if (decimal->db_decspec)
	status = TRdisplay("Decimal Marker = '%c'", decimal->db_decimal);
    else
	status = TRdisplay("Decimal Marker Not Specified");

    if (status == TR_OK)
	return (E_DB_OK);
    else
	return (E_DB_ERROR);
}

/*{
** Name: psq_dstdmp	- Dump Distributed/Non-distributed Specifier
**
** Description:
**      This function dumps a distributed/non-distributed specifier
**
** Inputs:
**      distrib                         The distrib/non-distrib specifier
**
** Outputs:
**      NONE
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_FATAL			Failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    Sends output to terminal and/or log file
**
** History:
**	10-jan-86 (jeff)
**          written
*/
DB_STATUS
psq_dstdmp(
	DB_DISTRIB         distrib)
{
    if (TRdisplay("%w", "Not Specified,Distributed,Not Distributed",
	distrib) != TR_OK)
	return (E_DB_ERROR);
    return (E_DB_OK);
}

/*{
** Name: psq_booldmp	- Dump a boolean
**
** Description:
**      This function dumps a boolean.
**
** Inputs:
**      boolval                         The boolean to dump
**
** Outputs:
**      NONE
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_FATAL			Failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    Sends output to terminal and/or log file
**
** History:
**	10-jan-86 (jeff)
**          written
*/
DB_STATUS
psq_booldmp(
	i4                boolval)
{
    STATUS              status;

    if (boolval)
	status = TRdisplay("TRUE");
    else
	status = TRdisplay("FALSE");

    if (status != TR_OK)
	return (E_DB_ERROR);

    return (E_DB_OK);
}

/*{
** Name: psq_modedmp	- Dump a query mode
**
** Description:
**      This function dumps a query mode.
**
** Inputs:
**      mode                            The query mode
**
** Outputs:
**      NONE
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_FATAL			Failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    Sends output to terminal and/or log file
**
** History:
**	10-jan-86 (jeff)
**          written
**	21-oct-87 (stec)
**	    Changed mode definition from i4  to i2.
**	6-apr-1992 (bryanp)
**	    Brought function up to date (it was very old).
**	    New query modes: PSQ_DGTT, PSQ_DGTT_AS_SELECT
**	10-feb-93 (rblumer)
**	    Added PSQ_DISTCREATE and other new query modes.
**	24-May-206 (kschendel)
**	    Grossly out of date, rather than fix use %d (name) form
**	    and ask for name from uld.
*/
DB_STATUS
psq_modedmp(
	i2	mode)
{
    TRdisplay("%d (%s)",mode,uld_psq_modestr((i4) mode));
    return (E_DB_OK);
}

/*{
** Name: psq_tbiddmp	- Dump a table id
**
** Description:
**      This function dumps a table id
**
** Inputs:
**      tabid                           Pointer to the table id
**
** Outputs:
**      NONE
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_FATAL			Failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    Sends output to terminal and/or log file
**
** History:
**	10-jan-86 (jeff)
**          written
*/
DB_STATUS
psq_tbiddmp(
	DB_TAB_ID          *tabid)
{
    if (TRdisplay("db_tab_base:\t%d, db_tab_index:\t%d", tabid->db_tab_base,
	tabid->db_tab_index) != TR_OK)
	return (E_DB_ERROR);

    return (E_DB_OK);
}

/*{
** Name: psq_ciddmp	- Dump a cursor id
**
** Description:
**      This function dumps a cursor id
**
** Inputs:
**      cursid                          Pointer to the cursor id
**
** Outputs:
**      NONE
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_FATAL			Failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    Sends output to terminal and/or log file
**
** History:
**	10-jan-86 (jeff)
**          written
*/
DB_STATUS
psq_ciddmp(
	DB_CURSOR_ID	*cursid)
{
    if (TRdisplay("\t\tdb_cursor_id:\t%d,%d\n\t\tdb_curs_name:%#s",
	cursid->db_cursor_id[0], cursid->db_cursor_id[1], 
	sizeof (cursid->db_cur_name),
	cursid->db_cur_name) != TR_OK)
	return (E_DB_ERROR);

    return (E_DB_OK);
}

/*{
** Name: psq_errdmp	- Dump an error block
**
** Description:
**      This function dumps a standard error block.
**
** Inputs:
**      errblk                          Pointer to the error block
**
** Outputs:
**      NONE
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_FATAL			Failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    Sends output to terminal and/or log file
**
** History:
**	10-jan-86 (jeff)
**          written
*/
DB_STATUS
psq_errdmp(
	DB_ERROR         *errblk)
{
    if (TRdisplay("\t\t\t err_code:%d err_data:%d\n", errblk->err_code,
	errblk->err_data) != TR_OK)
    {
	return (E_DB_ERROR);
    }

    return (E_DB_OK);
}

/*{
** Name: psq_dtdump	- Dump a datatype id
**
** Description:
**      This function dumps a datatype id in human-readable form.
**
** Inputs:
**      dt_id                           The datatype id to dump
**
** Outputs:
**      None
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic failure
**	    E_DB_FATAL			Catastrophic failure
**	Exceptions:
**	    None
**
** Side Effects:
**	    Sends output to terminal and/or log file
**
** History:
**	29-may-86 (jeff)
**          written
**	14-nov-88 (stec)
**	    Initialize ADF_CB on the stack.
**	10-Jan-2001 (jenjo02)
**	    Use ADF_CB* from session control block
**	    instead of SCU_INFORMATION.
*/
DB_STATUS
psq_dtdump(
	DB_DT_ID           dt_id)
{
    ADI_DT_NAME         adt_name;
    PSS_SESBLK		*sess_cb;
    ADF_CB	        *adf_cb;
    PTR			ad_errmsgp;

    /* Get ADF_CB* from session control block */
    sess_cb = psf_sesscb();

    adf_cb = (ADF_CB*)sess_cb->pss_adfcb;

    /* Preserve and restore ad_errmsgp */
    ad_errmsgp = adf_cb->adf_errcb.ad_errmsgp;
    adf_cb->adf_errcb.ad_errmsgp = NULL;
    if (adi_tyname(adf_cb, dt_id, &adt_name) == E_DB_OK)
    {
	TRdisplay("%#s", sizeof (adt_name.adi_dtname), adt_name.adi_dtname);
    }
    else
    {
	TRdisplay("Bad datatype id %d", dt_id);
    }

    adf_cb->adf_errcb.ad_errmsgp = ad_errmsgp;

    return (E_DB_OK);
}

/*{
** Name: psq_tmdmp	- Dump a timestamp
**
** Description:
**      This function dumps a timestamp in human-readable form
**
** Inputs:
**      timestamp                       Pointer to the timestamp
**
** Outputs:
**      None
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic failure
**	    E_DB_FATAL			Catastrophic failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    Sends output to terminal and/or log file.
**
** History:
**	02-jun-86 (jeff)
**          written
*/
DB_STATUS
psq_tmdmp(
	DB_TAB_TIMESTAMP   *timestamp)
{
    if (TRdisplay("db_tab_high_time: %d, db_tab_low_time: %d",
	timestamp->db_tab_high_time, timestamp->db_tab_low_time) != TR_OK)
    {
	return (E_DB_ERROR);
    }

    return (E_DB_OK);
}

/*{
** Name: psq_jrel_dmp	- Dump inner and outer relation mask
**
** Description:
**      This function dumps inner and outer relation mask in a somewhat
**	readable form.
**
** Inputs:
**	inner_rel		    ptr to the inner relation mask
**      outer_rel		    ptr to the outer relation mask
**
** Outputs:
**      None
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic failure
**	    E_DB_FATAL			Catastrophic failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    Sends output to terminal and/or log file.
**
** History:
**	19-sep-89 (andre)
**          written
*/
DB_STATUS
psq_jrel_dmp(
	PST_J_MASK	*inner_rel,
	PST_J_MASK	*outer_rel)
{
    char	    inner_buf[sizeof(PST_J_MASK) * BITSPERBYTE + 1];
    char	    outer_buf[sizeof(PST_J_MASK) * BITSPERBYTE + 1];
    register i4     i;
    register char   *cur_inner = inner_buf;
    register char   *cur_outer = outer_buf;

    for (i = sizeof(PST_J_MASK) * BITSPERBYTE - 1;
         i >= 0;
	 i--, cur_inner++, cur_outer++)
    {
	*cur_inner = (BTtest(i, (char *) inner_rel)) ? '1' : '0';
	*cur_outer = (BTtest(i, (char *) outer_rel)) ? '1' : '0';
    }
    *cur_inner = *cur_outer = '\0';

    if (TRdisplay("\n\t\t\tInner relation mask: %s", inner_buf) != TR_OK)
    {
	return (E_DB_ERROR);
    }

    if (TRdisplay("\n\t\t\tOuter relation mask: %s", outer_buf) != TR_OK)
    {
	return (E_DB_ERROR);
    }

    return (E_DB_OK);
}

/*{
** Name: psq_upddmp	- Dump an update mode
**
** Description:
**      This function dumps a "for update" mode for debugging purposes.  There
**	are three choices: direct update, deferrred update, and readonly.
**
** Inputs:
**      updtmode                        The update mode
**
** Outputs:
**      None
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic failure
**	    E_DB_FATAL			Catastrophic failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    Sends output to terminal and/or log file.
**
** History:
**	02-jun-86 (jeff)
**          written
*/
DB_STATUS
psq_upddmp(
	i4                updtmode)
{
    if (TRdisplay("%w", "DIRECT UPDATE,DEFERRED UPDATE,READ ONLY", updtmode)
	!= TR_OK)
	return (E_DB_ERROR);
    return (E_DB_OK);
}

/*{
** Name: psq_str_dmp	- Dump a name
**
** Description:
**      This function dumps a string for debugging purposes.  Non-zero length
**	implies that the string may have some trailing blanks which will be
**	eliminated before pritning; otherwise it is assumed to be
**	NULL-terminated.
**
** Inputs:
**	str				string to print
**      len				length; if non-zero, trailing blanks, if
**					any will be eliminated; otherwise, it
**					will be assumed to be NULL-terminated
**
** Outputs:
**      None
**	
** Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Non-catastrophic failure
**	    E_DB_FATAL			Catastrophic failure
**
** Exceptions:
**	    none
**
** Side Effects:
**	    Sends output to terminal and/or log file.
**
** History:
**	26-nov-91 (andre)
**          written
*/
DB_STATUS 
psq_str_dmp(
	u_char	    *str,
	i4	    len)
{
    STATUS		    status;

    if (len > 0)
    {
	status = TRdisplay("%.#s", (i4) psf_trmwhite(len, (char *) str), str);
    }
    else if (len == 0)
    {
	status = TRdisplay("%s", str);
    }
    else
    {
	return(E_DB_ERROR);
    }

    return((status == TR_OK) ? E_DB_OK : E_DB_ERROR);
}
