/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<cm.h>
# include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ug.h>
# include	<ui.h>


	static	bool	rif_forms_active = FALSE;


/*
**   R_INIT_FRMS -
**	Cover to call FEforms(), and track whether or not this has been done
**    	so that we can explicitly FEendforms() without causing r_exit() to
**	fail on issuing a subsequent FEendforms().
**
**    Parameters:
**	None.
**
**    Returns:
**	STATUS - 	The status of the FEforms() call or OK if the
**			forms system is already active.
**
**    History:
**	12-jul-1993 (rdrane)
**		Created.
**	15-apr-1994 (rdrane)
**		Conditionalize setting SmartLook override to support the
**		Alex Technologies, Ltd. SIRIUS product.
*/

STATUS
r_init_frms()
{
	STATUS	status;


	/*
	** If forms are active, just return OK
	*/
	if  (rif_forms_active)
	{
		return(OK);
	}

	/*
	** Start the froms system.  If successful, reflect the state
	** in the module's static variable .  Always return the
	** status from FEforms().
	*/
#ifdef DATAVIEW
	IIMWomwOverrideMW();
#endif /* DATAVIEW */
	status = FEforms();
	if  (status == OK)
	{
		rif_forms_active = TRUE;
	}

	return(status);

}



/*
**   R_END_FRMS -
**	Cover to call FEendforms(), and track whether or not this has been done
**    	so that we prevent r_exit()from failing on issuing a subsequent
**	FEendforms().
**
**    Parameters:
**	None.
**
**    Returns:
**	Nothing
**
**    History:
**	12-jul-1993 (rdrane)
**		Created.
*/

VOID
r_end_frms()
{


	/*
	** If forms are inactive, just return.
	*/
	if  (!rif_forms_active)
	{
		return;
	}

	/*
	** End the froms system, and reflect the state
	** in the module's static variable.  Since no status
	** is returned, we must assume that it worked ...
	*/
	FEendforms();

	rif_forms_active = FALSE;

	return;
}

