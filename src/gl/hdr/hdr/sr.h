/*
**	Copyright (c) 2004 Ingres Corporation
*/
#include    <srcl.h>

/**CL_SPEC
** Name:	SR.h	- Define SR function externs
**
** Specification:
**
** Description:
**	Contains SR function externs
**
** History:
**	2-jun-1993 (ed)
**	    initial creation to define func_externs in glhdr
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	6-Mar-2007 (kschendel) SIR 122512
**	    Add N-page read/write utilities.
**/

FUNC_EXTERN STATUS  SRclose(
			SR_IO		*f,
			i4		delete_flag,
			CL_SYS_ERR	*err_code
);

FUNC_EXTERN STATUS  SRopen(
			SR_IO           *f,
			char            *path,
			u_i4		pathlength,
			char            *filename,
			u_i4	        filelength,
			i4		pagesize,
			u_i4		create_flag,
			i4         n,
			CL_SYS_ERR	*err_code
);

FUNC_EXTERN STATUS  SRreadN(
			SR_IO	        *f,
			i4		npages,
			i4         page,
			char            *buf,
			CL_SYS_ERR	*err_code
);

FUNC_EXTERN STATUS  SRwriteN(
			SR_IO	        *f,
			i4		npages,
			i4         page,
			char            *buf,
			CL_SYS_ERR	*err_code
);

/* Original SRread and SRwrite are single page versions */
#define SRread(f,p,b,e) SRreadN(f,1,p,b,e)
#define SRwrite(f,p,b,e) SRwriteN(f,1,p,b,e)

