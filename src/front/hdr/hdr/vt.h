/*
**  VT.h
**
**  Copyright (c) 2004 Ingres Corporation
**
**  static	char	*Sccsid = "@(#)VT.h	30.2	1/1/85";
**
**  History:
**	20-apr-92 (seg)
**		(forward) declare functions that return bool.  Tests
**		against such functions will fail on many ports unless the
**		return type is known to be bool.
**	04/23/92 (dkh) - Declaration for VTUpdPreview() is really a VOID.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


typedef struct fldpos
{
	i4	posy;
	i4	posx;
	i4	posd;
	i4	phsy;
	i4	phsx;
	i4	phsd;
} FLD_POS;

# define	VT_GET_NEW	(i4) 0
# define	VT_GET_EDIT	(i4) 1

# define	VF_NEXT		(i4) 0
# define	VF_PREV		(i4) 1
# define	VF_BEGFLD	(i4) 2
# define	VF_ENDFLD	(i4) 3

# define	VF_SEQ		(i4) 0
# define	VF_NORM		(i4) 1

FUNC_EXTERN	bool	VTnewfrm();
FUNC_EXTERN	bool	VTFormPreview();
FUNC_EXTERN	VOID	VTUpdPreview();
