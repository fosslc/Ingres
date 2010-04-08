/*
**	Copyright (c) 2004 Ingres Corporation
*/
# ifndef JF_HDR_INCLUDED
# define JF_HDR_INCLUDED 1
#include    <jfcl.h>

/**CL_SPEC
** Name:	JF.h	- Define JF function externs
**
** Specification:
**
** Description:
**	Contains JF function externs
**
** History:
**	2-jun-1993 (ed)
**	    initial creation to define func_externs in glhdr
**	31-mar-1997 (canor01)
**	    Protect against multiple inclusion of jf.h.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

FUNC_EXTERN STATUS  JFclose(
			JFIO                *jfio,
			CL_SYS_ERR	    *err_code
);

FUNC_EXTERN STATUS  JFcreate(
			JFIO		    *jf_io,
			char		    *device,
			u_i4		    l_device,
			char		    *file,
			u_i4		    l_file,
			u_i4		    bksz,
			i4		    blkcnt,
			CL_SYS_ERR	    *err_code
);

FUNC_EXTERN STATUS  JFdelete(
			JFIO		    *jf_io,
			char                *path,
			u_i4		    l_path,
			char		    *filename,
			u_i4		    l_filename,
			CL_SYS_ERR	    *sys_err
);

FUNC_EXTERN STATUS  JFdircreate(
			JFIO           *f,
			char           *path,
			u_i4          pathlength,
			char           *dirname,
			u_i4          dirlength,
			CL_SYS_ERR     *err_code
);

FUNC_EXTERN STATUS  JFdirdelete(
			JFIO          *f,
			char           *path,
			u_i4          pathlength,
			char           *dirname,
			u_i4          dirlength,
			CL_SYS_ERR     *err_code
);

FUNC_EXTERN STATUS  JFlistfile(
			JFIO	       *f,
			char           *path,
			u_i4          pathlength,
			STATUS         (*func)(),
			PTR            arg_list,
			CL_SYS_ERR     *err_code
);

FUNC_EXTERN STATUS  JFopen(
			JFIO                *jfio,
			char		    *device,
			u_i4		    l_device,
			char		    *file,
			u_i4		    l_file,
			u_i4		    bksz,
			i4		    *eof_block,
			CL_SYS_ERR	    *err_code
);

FUNC_EXTERN STATUS  JFread(
			JFIO                *jfio,
			PTR		    buffer,
			i4		    block,
			CL_SYS_ERR	    *err_code
);

FUNC_EXTERN STATUS  JFwrite(
			JFIO                *jfio,
			PTR		    buffer,
			i4		    block,
			CL_SYS_ERR	    *err_code
);

FUNC_EXTERN STATUS  JFupdate(
			JFIO                *jfio,
			CL_SYS_ERR	    *err_code
);

FUNC_EXTERN STATUS  JFtruncate(
			JFIO		*jfio,
			CL_SYS_ERR	*err_code
);

# endif /* ! JF_HDR_INCLUDED */
