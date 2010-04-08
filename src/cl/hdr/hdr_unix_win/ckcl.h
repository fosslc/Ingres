/*
** Copyright (c) 2004 Ingres Corporation
*/

/**CL_SPEC
** Name: CK.H - Definitions of CK routines.
**
** Description:
**      This file defines the structures and constants associated with
**	the CK routines.
**
** Specification:
**
**  Description:  
**	The CK routines perform backups of DI type files.  The CK
**	routines are separate from the DI routines partly out of
**	past history and partly because of the function.  These routines
**	allow a DI directory of files to be copied to a single DI
**	type file, or a single operating system file, or a tape
**	device.  The directory of files can be restored from the
**	single file image created in the previous step.
**      
**  Intended Uses:
**	The CK routines are intended for the sole use of the 
**	checkpoint and rollforward routines in DMF.
**      
**  Assumptions:
**	While a directory is being saved or restored no other user can attempt
**	to use files in the directory.  The ingres programs which 
**      cause checkpoint saves or restores insure this serialization
**      through the use of INGRES locks.  This same locking mechanism
**      is used when running all other INGRES utilities therefore INGRES
**      insures no other uses can use a directory being saved or 
**      restored.  However, we are not preventing access of these files
**      by the host operating system or a user program accessing them
**      through the host operating system.
**	
**      The directory that the file was saved from can be different from the
**      directory it is restored to.
**	
**	The CL routine must provide the ability to write directly to a tape
**	device.
**
**( Definitions:
**
**	Save file	    -  The output of a CKsave call is a single file
**			       containing the contents of all files in the
**			       DI directory that was saved.  The filenames
**			       are also saved in the save file.  The format
**                             should be whatever works most efficiently for
**                             host operating system.
**)
**  Concepts:
**
**	The CK routines must remember the original name given to each file
**	written to a CK save file.  The file is recreated with the same
**	name by the CK restoration routine.
**
**	The data recorded in the CK file should be redundantly recorded
**	if possible.  The data that is saved in these files is most valuable.
**	Writing multiple copies of the same tape can suffice.
**
** History: 
 * Revision 1.2  88/10/25  13:21:49  roger
 * Integrate coding standard formatting tricks
 * just to keep differences at a minimum.
 * 
**      01-nov-1986 (Derek)
**          Created.
**      16-jun-1987 (mmm)
**          Copied vms version to UNIX.
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	11-may-89 (daveb)
**		Remove log and useless old history.
**	26-apr-1993 (bryanp)
**	    Prototyping CK for 6.5.
**	27-apr-93 (vijay)
**	    arg_list is PTR *, not PTR.
**	27-apr-1993 (bryanp)
**	    Actually, arg_list is a PTR. Fixed ck.c to match.
**	7-jun-93 (ed)
**	    created glhdr version contain prototypes, renamed existing file
**	    to xxcl.h and removed any func_externs
**	15-may-97 (mcgem01)
**	    Add a prototype for CK_write_error.
**	23-jun-1998(angusm)
**	    Add CK_OWNNOTING (bug 82736), for UNIX only.
**	06-oct-1998 (somsa01)
**	    Added CK_COMMAND_ERROR, which designates that an error came
**	    back from the command that was spawned sucessfully.
**	    (SIR #93752)
**	11-jan-1999 (mcgem01)
**	    CK_TEMPLATE_MISSING and CK_OWNNOTING had been assigned the 
**	    same error number - this has been rectified.
**	10-nov-1999 (somsa01)
**	    Added CK_is_alive, and CK_exist_error.
**	16-nov-1999 (somsa01)
**	    CK_is_alive() is of type bool.
**	    
**/

/*
**  Forward and/or External function references.
*/
/*
** These functions are private to the VMS CL, but I'm putting their prototypes
** here because there is no more obvious header file to use. If a cklocal.h
** ever gets created for vms, please move these prototypes there.
*/
FUNC_EXTERN STATUS  CK_spawn(
			char		*command,
			CL_ERR_DESC	*err_code);

FUNC_EXTERN STATUS  CK_subst(
			char	*cline,
			i4	comlen,
			char	oper, 
			char	dev, 
			char	dir,
			u_i4	type, 
			u_i4	locnum, 
			u_i4	di_l_path, 
			char	*di_path,
			u_i4	ckp_l_path, 
			char	*ckp_path,
			u_i4	ckp_l_file,
			char	*ckp_file);

FUNC_EXTERN STATUS  CK_exist_error(
			PID             *pid,
			STATUS		*ret_val,
			u_i4		*errnum,
			i4		*os_ret,
			char		**command);

FUNC_EXTERN STATUS  CK_write_error(
			PID             *pid,
			STATUS		ret_val,
			u_i4		errnum,
			i4		os_ret,
			char		*command);

FUNC_EXTERN bool    CK_is_alive(
			PID		pid);

/*
**  Defines of other constants.
*/

/*
**	CK return values.
*/

#define			CK_BAD_PARAM		(E_CL_MASK + E_CK_MASK + 1)
#define			CK_SPAWN		(E_CL_MASK + E_CK_MASK + 2)
#define			CK_BACKUP		(E_CL_MASK + E_CK_MASK + 3)
#define			CK_CMKRNL		(E_CL_MASK + E_CK_MASK + 4)
#define			CK_NOTFOUND		(E_CL_MASK + E_CK_MASK + 5)
#define			CK_BADDELETE		(E_CL_MASK + E_CK_MASK + 6)
#define			CK_BADPATH		(E_CL_MASK + E_CK_MASK + 7)
#define			CK_ENDFILE		(E_CL_MASK + E_CK_MASK + 8)
#define			CK_BADLIST		(E_CL_MASK + E_CK_MASK + 9)
#define			CK_FILENOTFOUND		(E_CL_MASK + E_CK_MASK + 0x0A)
#define			CK_DIRNOTFOUND		(E_CL_MASK + E_CK_MASK + 0x0B)
#define			CK_BADDIR		(E_CL_MASK + E_CK_MASK + 0x0C)
#define			CK_EXISTS    		(E_CL_MASK + E_CK_MASK + 0x0D)
#define			CK_TEMPLATE_MISSING	(E_CL_MASK + E_CK_MASK + 0x0E)
#define			CK_COMMAND_ERROR	(E_CL_MASK + E_CK_MASK + 0x0F)

#ifndef NT_GENERIC
#define			CK_OWNNOTING		(E_CL_MASK + E_CK_MASK + 0x10)
#endif

/*
**      Type constants for CKsave and CKrestore.
*/
#define                 CK_DISK_FILE    1L
#define			CK_TAPE_FILE	2L
