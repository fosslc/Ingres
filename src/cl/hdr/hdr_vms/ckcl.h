/*
** Copyright (c) 1986, 2001 Ingres Corporation
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
**	The CK module performs "back-ups" and "restore" of INGRES databases.  
**	INGRES databases consist of directories of DI files. The backup and
**	restore may be performed to either disk or tape devices.
**      
**  Intended Uses:
**	The CK routines are intended for the sole use of the 
**	checkpoint and rollforward routines in DMF.
**      
**  Assumptions:
**	The ingres programs which cause checkpoint saves or restores insure 
**	the required serialization through the use of INGRES locks.  
**	The implementation of CK must nost prevent concurrent access of the 
**	database (online backup is implemented by a combination of CK, LG
**	and JF).
**	
**      The physical disk location that the database was saved from can be 
**	different from the location it is restored to.
**	
**  Definitions:
**
**(	Save file	    -  The output of a CKsave call is conceptually a
**			       single file containing the contents of all 
**			       files in the DI directory that was saved.  
**			       Enough information, e.g. the filenames on the
**			       directory, to restore the database location
**			       must be contained in the save file. The format
**                             should be whatever works most efficiently for
**                             host operating system. A database backup will
**			       consist of a save file for each INGRES location
**			       the database resides on.
**)
**  Concepts:
**
**	The CK routines must "remember" the original name given to each file
**	written to a CK save file.  The file is recreated with the same
**	name by the CK restoration routine.
**
**	The data recorded in the CK file should be redundantly recorded
**	if possible.  The data that is saved in these files is most valuable.
**	Writing multiple copies of the same tape can suffice.
**
** History: 
**      01-nov-1986 (Derek)
**          Created.
**	26-apr-1993 (bryanp)
**	    Added prototypes for 6.5.
**	26-apr-1993 (bryanp)
**	    arg_list is a PTR, not a (PTR *).
**	7-jun-93 (ed)
**	    created glhdr version contain prototypes, renamed existing file
**	    to xxcl.h and removed any func_externs
**	31-mar-1999 (kinte01)
**	    cross integrate Unix change 439756 by mcgem01
**          CK_TEMPLATE_MISSING moved here from ck.h
**          Also added definition for CK_COMMAND_ERROR.
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	13-mar-2001 (kinte01)
**	    Add CK_is_alive
**
[@history_template@]...
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
#define                 CK_TEMPLATE_MISSING     (E_CL_MASK + E_CK_MASK + 0x0E)
#define                 CK_COMMAND_ERROR        (E_CL_MASK + E_CK_MASK + 0x0F)
/*
**      Type constants for CKsave and CKrestore.
*/
#define                 CK_DISK_FILE    1L
#define			CK_TAPE_FILE	2L
