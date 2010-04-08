/*
** Copyright (c) 1983, 2008 Ingres Corporation
*/
# include	<compat.h>  
# include	<gl.h>
# include	<pc.h>  
# include	"pclocal.h"
# include	"pcerr.h"
# include	<lo.h>  
# include	<nm.h>
# include	<st.h>  
# include	<si.h>  
# include	<descrip.h>
# include	<ssdef.h>
# include	<lib$routines.h>

/**
 *	Name:
 *		PCspawn.c
 *
 *	Function:
 *		PCspawn
 *
 *	Arguments:
 *		i4		argc;
 *		char		**argv;
 *		LOCATION	*in_name;
 *		LOCATION	*out_name;
 *		PID		*pid;
 *
 *	Result:
 *		Spawn a DCL subprocess.
 *
 *		create a new process with its standard input and output linked 
 *		to LOCATIONs named by the caller.  Null LOCATION means use 
 *		current stdin or stdout.
 *
 *		Returns:
 *			OK		-- success
 *			PC_SP_CALL	-- arguments incorrect
 *			PC_CM_BAD	-- access violation in argument list
 *
 *	Side Effects:
 *		None
 *
 *	History:
 *		03/83	--	(gb)
 *			written
 *		9/83 (wood) -- adapted to VAX/VMS environmemt.
 *
 *
**      16-jul-93 (ed)
**	    added gl.h
**      11-aug-93 (ed)
**          added missing includes
**      02-oct-1996 (rosga02)
**          Bug 74980: add processing for VMS 6.0 captive accounts,
**          to allow spawning to be /TRUSTED if II_SPAWN_TRUSTED
**          is set.
**      28-sep-98 (chash01/kinte01)
**          change the size of cmdline from 80 to 256 (80 is too short.)
**          Also if waitflg arg is set (means subprocess wait), then set 
**          spflag = 0 (means no wait to VMS), if waitflg is not set,
**          then set spflag |= 0x1 (set the first bit means no wait) (93552)
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes
**	02-sep-2003 (abbjo03)
**	    Changes to use VMS headers provided by HP.
**	27-jul-2004 (abbjo03)
**	    Rename pc.h to pclocal.h to avoid conflict with CL hdr!hdr file.
**	12-nov-2008 (joea)
**	    Fully initialize the string descriptors.
*/



STATUS
PCspawn(
i4		argc,
char		**argv,
i1		wait,
LOCATION	*in_name,
LOCATION	*out_name,
PID		*pid)
{
	char		*in_fname;
	char		*out_fname;
	struct dsc$descriptor_s	calldsc, infdsc, outfdsc,
			*infdscp, *outfdscp;
	i4		spflag = 0, stat = 0;
	char		cmdline[256];
	register char	*ct;
	register i4	i, arglen, cmdlen = 0;
	char            *cp;	

	PCstatus = OK;

	if ((argc < 1) || (argv[0] == NULL))
		return(PCstatus = PC_SP_CALL);

	infdscp = outfdscp = NULL;

 	if (in_name != NULL)
	{
		LOtos(in_name, &in_fname);
	/*	lib$spawn takes care of opening in_fname, out_fname */
		infdsc.dsc$w_length = STlength(in_fname);
		infdsc.dsc$a_pointer = in_fname;
		infdsc.dsc$b_dtype = DSC$K_DTYPE_T;
		infdsc.dsc$b_class = DSC$K_CLASS_S;
		infdscp = &infdsc;
	}
	if (out_name != NULL)
	{
		LOtos(out_name, &out_fname);
	/*	lib$spawn takes care of opening in_fname, out_fname */
		outfdsc.dsc$w_length = STlength(out_fname);
		outfdsc.dsc$a_pointer = out_fname;
		outfdsc.dsc$b_dtype = DSC$K_DTYPE_T;
		outfdsc.dsc$b_class = DSC$K_CLASS_S;
		outfdscp = &outfdsc;
	}

	calldsc.dsc$a_pointer = ct = cmdline;
	for (i = 0; i < argc; i++)
	{
		STcopy(argv[i], ct);
		arglen = STlength(argv[i]);
		cmdlen += arglen;
		ct += arglen;
	}
	calldsc.dsc$w_length = cmdlen;
	calldsc.dsc$b_dtype = DSC$K_DTYPE_T;
	calldsc.dsc$b_class = DSC$K_CLASS_S;

	if (!wait) 
		spflag = 1;

	/*
	** bug 74980 - allow spawns to be considered 'trusted'
	*/
	NMgtAt("II_SPAWN_TRUSTED", &cp);
	if (cp != NULL && *cp != '\0')
		spflag |= CLI$M_TRUSTED;
  
	PCstatus = lib$spawn(&calldsc, infdscp, outfdscp, &spflag, 0, pid, 0, PCEVENTFLGP, 0, 0);

	/*	Either spawn completed successfully and subproc is gone or we got an error.	*/

	switch (PCstatus)
	{
	  case SS$_ACCVIO:
		return(PCstatus = PC_CM_BAD);

	  case SS$_NORMAL:
		return(PCstatus = OK);
	}

	return(PCstatus);
}
