/*
**Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<er.h>
# include	<nm.h>
# include	<si.h>
# include	<ci.h>
# include	"cilocal.h"

/**
** Name:     CIunix   - Contains all VMS system dependent routines.
**
** Description:
**          This file by definitions applicable only to Unix.
**	This file defines:
**
**      CImk_logical()	define the global variable, II_AUTH_STRING,
**		inside $ING_HOME/files/symbol.tbl.
**      CIrm_logical()	no-op so this call doesn't have to be ifdefed for VMS.	
**
** History:
 * Revision 1.1  90/03/09  09:14:06  source
 * Initialized as new code.
 * 
 * Revision 1.2  90/02/12  09:38:46  source
 * sc
 * 
 * Revision 1.1  89/05/26  15:43:38  source
 * sc
 * 
 * Revision 1.1  89/05/11  01:03:22  source
 * sc
 * 
 * Revision 1.1  89/01/13  17:11:03  source
 * This is the new stuff.  We go dancing in.
 * 
 * Revision 1.1  88/08/05  13:26:22  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
**      Revision 1.3  87/11/17  15:37:59  mikem
**      changed name of local hdr to cilocal.h from ci.h
**      
**      Revision 1.2  87/11/09  15:08:01  mikem
**      Initial integration of 6.0 changes into 50.04 cl to create 
**      6.0 baseline cl
**      
**      Revision 1.2  87/07/27  11:11:40  mikem
**      Updated to meet jupiter coding standards.
**      
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	14-may-97 (mcgem01)
**	    Clean up compiler warnings.
**	29-Nov-2010 (frima01) SIR 124685
**	    Add include of ci.h neccessary for cilocal.h.
**/

/* # defines */
/* typedefs */
/* forward references */
/* externs */
/* statics */



/*
** Name: CImk_logical() - Defines a global variable in the Ingres environment.
**
** Description:
**	  This routine defines a global variable in the 
**	$ING_HOME/files/symbol.tbl file to be the equivalence string passed
**	to it.
**
** Inputs:
**	log_name			Name of environment var to be defined.
**	equiv_str			Value log_name is to be set to.
**	arg1_dummy			A dummy arg to make call interface
**					compatible.
**
** Side Effects:
**	  Defines an Ingres global variable in the file
**	$ING_HOME/files/symbol.tbl.  If the variable already exists, it is 
**	redefined.
*/

/*ARGSUSED*/
VOID
CImk_logical(log_name, equiv_str, arg1_dummy)
PTR	log_name;	/* name of environment var to be defined */
PTR	equiv_str;	/* value log_name is to be set to */
PTR	arg1_dummy;	/* dummy argument, so call interface is the same */
{
	STATUS	status;
	char	errbuf[ER_MAX_LEN];

	status = NMstIngAt(log_name, equiv_str);
	if (status != OK)
	{
		ERreport(status, errbuf);
		SIprintf("WARNING: CImk_logical:  %s\n", errbuf);
	}
	return;
}



/*
** Name: CIrm_logical() - a no-op on Unix.
**
** Description:
**	  This routine is a no-op on Unix as there is no NM CL routine defined
**	to remove a variable from the file $ING_HOME/files/symbol.tbl.
**
** Inputs:
**	arg1_dummy			Dummy arg to make a call interface
**					compatible.
**	arg2_dummy
*/

/*ARGSUSED*/
VOID
CIrm_logical(arg1_dummy, arg2_dummy)
PTR	arg1_dummy;
PTR	arg2_dummy;
{
}
