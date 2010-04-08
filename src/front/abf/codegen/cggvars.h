/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:    cggvars.h -		Code Generator Global Environment Declarations.
**
** Description:
**	Declares global variables used by the code generator which
**	produces C code based on the OSL intermediate language.
**
** History:
**	Revision 6.4
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Added #defines for new FRMINFO fields: dbd, num_dbd, stacksize.
**		Also added routine_name.
**
**	Revision 6.0  87/06  arthur
**	Initial revision.
*/

/*}
** Name:    CG_GLOBS -	Code Generator Environment Description:
**
** Description:
**	Structure containing global information used throughout the code
**	generator.  The information is either passed on the command line or
**	returned from the ILRF.
*/
typedef struct 
{
	char	*frame_name;	/* name of the frame, from -f flag */
	char	*form_name;	/* name of the form, from -m flag */
	char	*symbol_name;	/* symbol for this OSL frame, from -s flag */
	char	*routine_name;	/* name of curent frame, proc, or local proc */
	char	*ofile_name;	/* name of output file, from -o flag */
	FILE	*ofile_ptr;	/* output file pointer */
	i4	fid;		/* frame identifier, from -# flag */
	FRMINFO	frminfo;	/* structure for interface with ILRF.
				   Contains pointers to FDESC, IL, and struct
				   with version and alignment info */
	bool	f_static;	/* form is 'static' if true */
} CG_GLOBS;

extern CG_GLOBS	IICGgloGlobs;

/*
** Definitions for names used for elements of the code generator's
** global structure.
*/
# define CG_frame_name		IICGgloGlobs.frame_name
# define CG_form_name		IICGgloGlobs.form_name
# define CG_symbol_name		IICGgloGlobs.symbol_name
# define CG_routine_name	IICGgloGlobs.routine_name
# define CG_of_name		IICGgloGlobs.ofile_name
# define CG_of_ptr		IICGgloGlobs.ofile_ptr
# define CG_fid			IICGgloGlobs.fid
# define CG_frminfo		IICGgloGlobs.frminfo
# define CG_fdesc		IICGgloGlobs.frminfo.symtab
# define CG_il_stmt		IICGgloGlobs.frminfo.il
# define CG_frmalign		IICGgloGlobs.frminfo.align
# define CG_frm_dbd		IICGgloGlobs.frminfo.dbd
# define CG_frm_num_dbd		IICGgloGlobs.frminfo.num_dbd
# define CG_frm_stacksize	IICGgloGlobs.frminfo.stacksize
# define CG_static		IICGgloGlobs.f_static

/*
** Internal flags.
*/
extern bool	IICGdonFrmDone;		/* Finished with all IL for frame? */
