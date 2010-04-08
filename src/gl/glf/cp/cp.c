/*
** Copyright (c) 1984, 2008 Ingres Corporation
**
*/
# include	 <compat.h>
# include	 <gl.h>
#ifdef VMS
#include <starlet.h>
#include <efndef.h>
#include <iledef.h>
#include <iosbdef.h>
#include <jpidef.h>
#include <ssdef.h>
#include <gen64def.h>
#include <vmstypes.h>
#else
# include	 <clconfig.h>
#endif
# include	 <lo.h>
# include	 <si.h>
# include	 <cp.h>

/*
**
**	CPopen	-	open or create a file as if you are the user (i.e. the front-end process)
**
**	Parameters:
**	    loc		- pointer to the location to open or create
**	    file_mode	- "r" for read, "a" for append, "w" for write
**	    file_type	- SI_TXT for text files, SI_BIN for binary files,
**			  SI_VAR for variable files
**	    rec_length	- record length (only used with binary files)
**	    user_file	- pointer to the copy file descriptor (pointer to a
**			  FILE * on VMS)
**
**	Returns:
**		OK if file is opened
**		FAIL if not
**
**	History:
**		02/06/84 (lichtman) -- written
**		2-oct-84 (dreyfus) -- use privileges of parent process if 
**			there is one. The privileges are those at the time
**			of the process creation or image invocation.
**		28-mar-89 (russ)
**			Added setuid() calls for machines which don't have
**			setreuid().  setuid() suffices for System V machines,
**			we don't need docopy.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	16-oct-2002 (abbjo03)
**	    Add #ifdef'd includes for VMS.
**	28-aug-2003 (abbjo03)
**	    Changes to use VMS headers provided by HP.
**	20-Jul-2004 (lakvi01)
**		SIR# 112703, cleaned-up warnings.
**	23-Jul-2004 (schka24)
**	    Fix unix breakage from above.
**	07-sep-2005 (abbjo03)
**	    Replace xxx_FILE constants by SI_xxx.
**	09-oct-2008 (stegr01/joea)
**	    Replace II_VMS_ITEM_LIST_3 by ILE3.
**	28-oct-2008 (joea)
**	    Use EFN$C_ENF and IOSB when calling a synchronous system service.
*/

#ifndef NT_GENERIC
STATUS
CPopen(
	LOCATION	*loc,
	char		*file_mode,
	i2		file_type,
	i4		rec_length,
	CPFILE		*user_file )
{
	STATUS		ret_val;
#ifdef	VMS
	long		ast_value;
	GENERIC_64	procprv;
	GENERIC_64	priv_mask;
	II_VMS_FILE_PROTECTION	protmask = 0xff00;
	ILE3		itemlist[3];
	IOSB		iosb;

	itemlist[0].ile3$w_length = sizeof(procprv);
	itemlist[0].ile3$w_code = JPI$_PROCPRIV;
	itemlist[0].ile3$ps_bufaddr = &procprv;
	itemlist[0].ile3$ps_retlen_addr = NULL;

	itemlist[1].ile3$w_length = sizeof(priv_mask);
	itemlist[1].ile3$w_code = JPI$_CURPRIV;
	itemlist[1].ile3$ps_bufaddr = &priv_mask;
	itemlist[1].ile3$ps_retlen_addr = NULL;
 
	itemlist[2].ile3$w_length = NULL;
	itemlist[2].ile3$w_code = NULL;
	itemlist[2].ile3$ps_bufaddr = NULL;
	itemlist[2].ile3$ps_retlen_addr = NULL;

	ret_val = sys$getjpiw(EFN$C_ENF, 0, 0, &itemlist, &iosb, 0, 0);
	if (ret_val & 1)
	    ret_val = iosb.iosb$w_status;
	if (ret_val != SS$_NORMAL)
	{
		return (FAIL);
	}

	/*	open file with user's (front-end's) protections		*/
	ast_value = sys$setast(0);
	priv_mask.gen64$q_quadword = ~procprv.gen64$q_quadword &
		priv_mask.gen64$q_quadword;
	sys$setprv(0, &priv_mask, 0, 0);
	sys$setdfprot(&protmask, 0);
#else
	int	eu, ru;
	
	ru = getuid();
	eu = geteuid();
# ifdef xCL_044_SETREUID_EXISTS
	setreuid(eu, ru);
# else
	setuid(ru);
# endif
#endif

	ret_val = SIfopen(loc, file_mode, file_type, rec_length, user_file);


	/*	The file is now opened or it has failed, 
	**	so set the privileges back	
	*/

#ifdef	VMS
	sys$setprv(1, &priv_mask, 0, 0);
	sys$setdfprot(0, 0);
	if (ast_value == SS$_WASSET)
		sys$setast(1);
#else
# ifdef xCL_044_SETREUID_EXISTS
	setreuid(ru, eu);
# else
	setuid(eu);
# endif
#endif
	return (ret_val);
}
#endif
