/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: PCerr.H - Error numbers returned from PC routines
**
** Description:
**      Definitions of possible CL error returns from PC routines
**
** History:
**	27-oct-1997 (canor01)
**	    Convert from old-style error numbers to new ones using
**	    E_CL_MASK and E_PC_MASK that will correspond to messages
**	    in erclf.msg.
*/

# define	 PC_CM_CALL	( E_CL_MASK | E_PC_MASK | 0x01 )	/*  PCcmdline: arguments incorrect  */
# define	 PC_CM_EXEC	( E_CL_MASK | E_PC_MASK | 0x02 )	/*  PCcmdline: cannot execute command specified, bad magic number  */
# define	 PC_CM_MEM	( E_CL_MASK | E_PC_MASK | 0x03 )	/*  PCcmdline: system temporarily out of core or process requires too many segmentation registers  */
# define	 PC_CM_OWNER	( E_CL_MASK | E_PC_MASK | 0x04 )	/*  PCcmdline: must be owner or super-user to execute this command  */
# define	 PC_CM_PATH	( E_CL_MASK | E_PC_MASK | 0x05 )	/*  PCcmdline: part of specified path didn't exist  */
# define	 PC_CM_PERM	( E_CL_MASK | E_PC_MASK | 0x06 )	/*  PCcmdline: didn't have permission to execute  */
# define	 PC_CM_PROC	( E_CL_MASK | E_PC_MASK | 0x07 )	/*  PCcmdline: system process table is temporarily full  */
# define	 PC_CM_REOPEN	( E_CL_MASK | E_PC_MASK | 0x08 )	/*  PCcmdline: couldn't associate argument [in, out]_name with std[in, out]  */
# define	 PC_CM_SUCH	( E_CL_MASK | E_PC_MASK | 0x09 )	/*  PCcmdline: command doesn't exist  */
# define	 PC_CM_BAD	( E_CL_MASK | E_PC_MASK | 0x0a )	/*  PCcmdline: child didn't execute, e.g bad parameters, bad or inacessible date, etc.  */
# define	 PC_CM_TERM	( E_CL_MASK | E_PC_MASK | 0x0b )	/*  PCcmdline: child we were waiting for returned bad termination status  */
# define	 PC_END_PIPE	( E_CL_MASK | E_PC_MASK | 0x0c )	/*  PCendpipe: had trouble closing PIPE argument passed in  */
# define	 PC_SP_CALL	( E_CL_MASK | E_PC_MASK | 0x0d )	/*  PC[f][s]spawn: arguments incorrect  */
# define	 PC_SP_EXEC	( E_CL_MASK | E_PC_MASK | 0x0e )	/*  PC[f][s]spawn: cannot execute command specified, bad magic number  */
# define	 PC_SP_MEM	( E_CL_MASK | E_PC_MASK | 0x0f )	/*  PC[f][s]spawn: system temporarily out of core or process requires too many segmentation registers  */
# define	 PC_SP_OWNER	( E_CL_MASK | E_PC_MASK | 0x10 )	/*  PC[f][s]spawn: must be owner or super-user to execute this command  */
# define	 PC_SP_PATH	( E_CL_MASK | E_PC_MASK | 0x11 )	/*  PC[f][s]spawn: part of specified path didn't exist  */
# define	 PC_SP_PERM	( E_CL_MASK | E_PC_MASK | 0x12 )	/*  PC[f][s]spawn: didn't have permission to execute  */
# define	 PC_SP_PROC	( E_CL_MASK | E_PC_MASK | 0x13 )	/*  PC[f][s]spawn: system process table is temporarily full  */
# define	 PC_SP_SUCH	( E_CL_MASK | E_PC_MASK | 0x14 )	/*  PC[f][s]spawn: command doesn't exist  */
# define	 PC_SP_CLOSE	( E_CL_MASK | E_PC_MASK | 0x15 )	/*  PC[f]sspawn: had trouble closing unused sides of pipes  */
# define	 PC_SP_DUP	( E_CL_MASK | E_PC_MASK | 0x16 )	/*  PC[f]sspawn: had trouble associating, dup()'ing, the pipes with child's std[in, out]  */
# define	 PC_SP_PIPE	( E_CL_MASK | E_PC_MASK | 0x17 )	/*  PC[f]sspawn: couldn't establish communication pipes  */
# define	 PC_SP_OPEN	( E_CL_MASK | E_PC_MASK | 0x18 )	/*  PCfsspawn: had trouble associating child's std[in,out] with passed in FILE pointers  */
# define	 PC_SP_REOPEN	( E_CL_MASK | E_PC_MASK | 0x19 )	/*  PCspawn: couldn't associate argument [in, out]_name with std[in, out]  */
# define	 PC_RD_CALL	( E_CL_MASK | E_PC_MASK | 0x1a )	/*  PCread: one of the passed in arguments is NULL  */
# define	 PC_RD_CLOSE	( E_CL_MASK | E_PC_MASK | 0x1b )	/*  PCread: stream corrupted, should never happen  */
# define	 PC_RD_OPEN	( E_CL_MASK | E_PC_MASK | 0x1c )	/*  PCread: trouble establishing communication pipes  */
# define	 PC_SD_CALL	( E_CL_MASK | E_PC_MASK | 0x1d )	/*  PCsend: invalid exception value  */
# define	 PC_SD_NONE	( E_CL_MASK | E_PC_MASK | 0x1e )	/*  PCsend: No process corresponding to the pid (process argument) exists  */
# define	 PC_SD_PERM	( E_CL_MASK | E_PC_MASK | 0x1f )	/*  PCsend: Can't signal specified process.  Effective uid of caller isn't "root" and doesn't match real uid of receiving process  */
# define	 PC_WT_BAD	( E_CL_MASK | E_PC_MASK | 0x20 )	/*  PCwait: child didn't execute, e.g bad parameters, bad or inacessible date, etc.  */
# define	 PC_WT_NONE	( E_CL_MASK | E_PC_MASK | 0x21 )	/*  PCwait: no children of this process are currently alive  */
# define	 PC_WT_TERM	( E_CL_MASK | E_PC_MASK | 0x22 )	/*  PCwait: child we were waiting for returned bad termination status  */
# define	PC_WT_INTR	( E_CL_MASK | E_PC_MASK | 0x23 )	/* PCwait: child was interrupted by a signal */
# define	PC_WT_EXEC	( E_CL_MASK | E_PC_MASK | 0x28 )	/* PCwait: child is executing */
