/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<me.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	 <rtype.h> 
# include	 <rglob.h> 

/*
**   R_NEW_TCMD - set up a new TCMD structure, with an error code
**	in the code spot to start.  
**
**	Parameters:
**		*otcmd	if specified, this is the TCMD structure in
**			the linked list above this one.  Fill in the
**			link in that structure.  If null, ignore.
**
**	Returns:
**		ntcmd address which is made here.
**
**	Trace Flags:
**		2.0.
**
**	History:
**		1/5/81 (ps)	written.
**		8/16/84	(gac)	added sequence number to TCMD structure.
*/



TCMD *
r_new_tcmd(otcmd)
register TCMD	*otcmd;
{
	/* internal declarations */

	register TCMD	*ntcmd;		/* fast ptr to new structure */

	/* start of routine */


	ntcmd = (TCMD *) MEreqmem(0,sizeof(TCMD),TRUE,(STATUS *) NULL);
	ntcmd->tcmd_code = P_ERROR;
	ntcmd->tcmd_seqno = Sequence_num;

	if (otcmd != NULL)
	{	/* fill in old pointer */
		otcmd->tcmd_below = ntcmd;
	}

	return(ntcmd);
}
