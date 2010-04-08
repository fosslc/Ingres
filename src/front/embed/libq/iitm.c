/*
** Copyright (c) 1986, 2009 Ingres Corporation
*/

# include	<compat.h>
# include	<cm.h>
# include	<gl.h>
# include	<sl.h>
# include	<me.h>
# include	<iicommon.h>
# include	<gca.h>
# include	<adf.h>
# include	<iicgca.h>
# include	<iirowdsc.h>
# include	<iisqlca.h>
# include	<iilibq.h>
# include	<cm.h>

/**
+*  Name: iitm.c - Interface file for the Terminal Monitor.
**
**  Defines:
**	IItm		- Set the TM flag (used for RETRIEVE names).
**	IItm_dml	- Set the DML for the TM.
**	IItm_retdesc	- Request for the RETRIEVE description. 
**	IItm_status	- Request for general query status information.
**	IItm_trace	- Set internal trace out function.
**	IItm_adfucolset - Set unicode collation for TM.
-*
**  History:
**	31-oct-1986	- Written. (neil)
**	31-mar-1987	- Added IItm. (neil)
**	21-dec-1987	- Added IItm_trace. (neil)
**	20-jan-1988	- Added IItm_dml. (neil)
**	19-may-1989	- Changed names of globals for multiple connect. (bjb)
**	03-dec-1990 (barbara)
**	    Moved state (for INQUIRE_INGRES) from GLB_CB to LBQ_CB (bug 9160)
**	07-mar-1991 (barbara)
**	    Added comment about backend dependence on IItm_trace.
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**	01-apr-1999 (somsa01)
**	    Slight adjustment to declaration of IItm() to supress building
**	    headaches on some UNIX platforms.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	16-Jun-2005 (gupsh01)
**	    Added IItm_adfucolset to initialize unicode collation for 
**	    coercing unicode data when displaying it on the terminal.
**      05-feb-2009 (joea)
**          Rename CM_ischarsetUTF8 to CMischarset_utf8.
**	19-Aug-2009 (kschendel/stephenb) 121804
**	    Need cm.h for proper CM declarations (gcc 4.3).  Add prototypes.
**/


/*{
+*  Name: IItm - Set/Reset the TM flag (used for RETRIEVE queries).
**
**  Description:
**	This routine is called by the terminal monitor in order to set
**	the names modifier for the next RETRIEVE query.  The call should
**	be issued at the start of the session.
**
**  Inputs:
**    turn_on	- Turn it on or off.
**
**  Outputs:
**	Returns:
**	    Void
**	Errors:
**	    None
-*
**  Side Effects:
**	
**  History:
**	31-mar-1987	- Written (ncg)
**	19-may-1989	- Changed TM flag to be part of II_GLB_CB struct. (bjb)
*/

void
IItm(bool turn_on)
{
    if (turn_on)
	IIglbcb->ii_gl_flags |= II_G_TM;
    else
	IIglbcb->ii_gl_flags &= ~II_G_TM;
}


/*{
+*  Name: IItm_dml - set the DML for the TM.
**
**  Description:
**	This routine is called by the terminal monitor in order to set
**	the DML to DB_QUEL or DB_SQL.  Because the TM implicitly switches
**	between the languages (for HELP and PRINT), it needs to
**	dynamically define the DML and cannot rely on defaults.
**
**  Inputs:
**    dml	- DB_QUEL or DB_SQL.
**
**  Outputs:
**	Returns:
**	    Void
**	Errors:
**	    None
-*
**  Side Effects:
**      1. Modifies ii_lq_dml.
**	
**  History:
**	20-jan-1988	- Written (ncg)
*/

void
IItm_dml(dml)
i4	dml;
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;

    IIlbqcb->ii_lq_dml = dml;
}


/*{
+*  Name: IItm_retdesc - Return descriptor information for RETRIEVE to TM.
**
**  Inputs:
**	desc	- ROW_DESC   ** - Pointer to row descriptor.
**
**  Outputs:
**	desc	- ROW_DESC   ** - Row descriptor for RETRIEVE. This may be NULL
**				  if first RETRIEVE not yet run, or there was
**				  an error.  The field rd_flags should be
**				  checked after this is called in order to
**				  determine if the name information is included.
**	Returns:
**	    Nothing
**	Errors:
**	    None
-*
**  Side Effects:
**	None
**	
**  History:
**	31-oct-1986	- Written (ncg)
**	26-mar-1987	- Modified for new row descriptors. (ncg)
*/

void
IItm_retdesc(desc)
ROW_DESC	**desc;
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;

    *desc = &IIlbqcb->ii_lq_retdesc.ii_rd_desc;
}


/*{
+*  Name: IItm_status - Return status info for Terminal Monitor
**
**  Description:
**	Return general status information from LIBQ to the TM.
**
**  Inputs:
**	rows	- i4  * - Pointer to request rows affected.
**	error	- i4  * - Pointer to request error number.
**	status	- i4  * - Pointer to request query status.
**	libq_connect_chgd - bool * - Pointer to request connect
**				     status.		b55911
**
**  Outputs:
**	rows	- i4  * - Number of rows affected by last query.
**	error	- i4  * - Error number (if any) of last query.
**	status	- i4  * - Status of last query.
**	libq_connect_chgd - bool * - set to TRUE if a DIRECT 
**			[DIS]CONNECT/SET SESSION AUTH. is issued.
** 			i.e. GCA_NEW_EFF_USER_MASK bit is set.
**							b55911
**	Returns:
**	    Nothing
**	Errors:
**	    None
-*
**  Side Effects:
**	None
**	
**  History:
**	31-oct-1986	- Written (ncg)
*/

void
IItm_status(rows, error, status, libq_connect_chgd)
i4	*rows;
i4	*error;
i4	*status;
bool   	*libq_connect_chgd;				/* b55911 */
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;

    *rows = IIlbqcb->ii_lq_rowcount;		/* Is i4 for large rows */
    *status = IIlbqcb->ii_lq_qrystat;
    if (IIlbqcb->ii_lq_qrystat & GCA_NEW_EFF_USER_MASK)	/* b55911 */
	{
	  *libq_connect_chgd = TRUE;
	}
    *error = IIlbqcb->ii_lq_error.ii_er_num;	/* Is i4 for large error nos */
}

/*{
**  Name: IItm_trace - Set trace function pointer.
**
**  Description:
**	The TM may need to set the trace output function.  If the TM is in
**	"script" mode, we may want to print trace info to the script file
**	as well as output.  The EQUEL default is to just print trace info
**	to output.
**
**	Note that IItm_trace is called by the DBMS event monitor program
**	(in back/scf/trace/sceadmin.sc).  This file relies on the visibility
**	of IItm_trace and the callback interface as defined in IItm_trace.
**	If this interface changes the sceadmin.sc file should be changed
**	accordingly.
**
**  Inputs:
**	trace_func	- Function pointer for trace output.  The function will
**			  be called with the following arguments:
**
**			  (*trace_func)(flag, dbv)
**			  	bool flag;	-- Flush trace data.
**				DBV  *dbv;	-- DB data value with trace data
**						   (not null terminated).  If
**					    	   flushing dbv is set to null.
**
**  Outputs:
**	Returns:
**	    Void
**	Errors:
**	    None
**
**  Side Effects:
**	
**  History:
**	21-dec-1987	- Written (ncg)
**	19-may-1989	- ii_er_error is now a part of the II_GLB_CB. (bjb)
**	07-mar-1991 (barbara)
**	    Added comment in header about backend dependence on IItm_trace.
*/

void
IItm_trace(trace_func)
i4	(*trace_func)();
{
    IIglbcb->ii_gl_trace = trace_func;
}

/*{
**  Name: IItm_adfucolset - Set unicode collation information for ADF
**
**  Description:
**	The TM may need to set the unicode collation in use. This is required 
**	TM needs to make call to adf for calling unicode conversion routines.
**
**  Inputs:
**	ADF_CB	*	Input TM_adfscb pointer.
**	
**  Outputs:
**	Returns:
**	    Void
**	    sets the relevant fields in TM_adfscb from the thread control
**	    block.
**	Errors:
**	    None
**
**  Side Effects:
**	
**  History:
**	16-jun-2005 (gupsh01)
**	    Added.
**	11-may-2007 (gupsh01)
**	    Initialize adf_utf8_flag.
*/
void
IItm_adfucolset ( scb)
ADF_CB *scb;
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;
    ADF_CB 	*adf_cb = (ADF_CB *) IIlbqcb->ii_lq_adf;

    if (IIlbqcb->ii_lq_ucode_init)
    {
	scb->adf_ucollation = adf_cb->adf_ucollation;
	if (CMischarset_utf8())
	  scb->adf_utf8_flag = AD_UTF8_ENABLED;
	else
	  scb->adf_utf8_flag = 0;
	scb->adf_unisub_status = adf_cb->adf_unisub_status;
	MEcopy ( (char * )adf_cb->adf_unisub_char, 
		AD_MAX_SUBCHAR, scb->adf_unisub_char);
	scb->adf_uninorm_flag = adf_cb->adf_uninorm_flag;
    }
}
