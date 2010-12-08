/*
**	Copyright (c) 2004 Ingres Corporation
*/
# ifndef CK_HDR_INCLUDED
# define CK_HDR_INCLUDED

#include    <pc.h>
#include    <ckcl.h>

/**CL_SPEC
** Name:	CK.h	- Define CK externs
**
** Specification:
**
** Description:
**	Contains CK function externs
**
** History:
**	2-jun-1993 (ed)
**	    initial creation to define func_externs in glhdr
**	23-may-1994 (bryanp)
**	    Add define for E_CK030E_TEMPLATE_FILE_MISSING error message.
**	30-sep-1996 (canor01)
**	    Protect against multiple inclusion of ck.h.
**	12-jan-1998 (mcgem01)
**	    CK_TEMPLATE_MISSING should be in ckcl.h along with all of
**	    the other CK return values.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-Nov-2010 (frima01) SIR 124685
**	    Added prototype for CK_spawnnow.
**      06-Dec-2010 (horda03) SIR 124685
**          Fix VMS build problems
**/

FUNC_EXTERN STATUS  CKbegin(
			u_i4	    type,
			char	    oper,
			char	    *ckp_path,
			u_i4	    ckp_l_path,
			u_i4	    numlocs,
			CL_ERR_DESC *sys_err);
FUNC_EXTERN STATUS  CKdelete(
			char                *path,
			i4		    l_path,
			char		    *filename,
			i4		    l_filename,
			CL_ERR_DESC	    *sys_err);
FUNC_EXTERN STATUS  CKdircreate(
			char           *path,
			u_i4          pathlength,
			char           *dirname,
			u_i4          dirlength,
			CL_ERR_DESC    *err_code);
FUNC_EXTERN STATUS  CKdirdelete(
			char           *path,
			u_i4          pathlength,

			char           *dirname,
			u_i4          dirlength,
			CL_ERR_DESC     *err_code);
FUNC_EXTERN STATUS  CKend(
			u_i4	    type,
			char	    oper,
			char	    *ckp_path,
			u_i4	    ckp_l_path,
			CL_ERR_DESC *sys_err);
FUNC_EXTERN STATUS  CKlistfile(
			char           *path,
			u_i4          pathlength,
			STATUS         (*func)(),
			PTR            arg_list,
			CL_ERR_DESC     *err_code);
FUNC_EXTERN STATUS  CKrestore(
			char                *ckp_path,
			u_i4		    ckp_l_path,
			char		    *ckp_file,
			u_i4		    ckp_l_file,
			u_i4		    type,
			char		    *di_path,
			u_i4		    di_l_path,
			CL_ERR_DESC	    *sys_err);
FUNC_EXTERN STATUS  CKsave(
			char		    *di_path,
			u_i4		    di_l_path,
			u_i4		    type,
			char                *ckp_path,
			u_i4		    ckp_l_path,
			char		    *ckp_file,
			u_i4		    ckp_l_file,
			CL_ERR_DESC	    *sys_err);
FUNC_EXTERN STATUS  CK_spawnnow(
			char		    *command,
			PID		    *pid,
			CL_ERR_DESC	    *err_code);

# endif /* CK_HDR_INCLUDED */
