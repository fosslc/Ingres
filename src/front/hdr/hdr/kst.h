
/*
**	Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	kst.h -	KEYSTRUCT Structure Definitions.
**
** Description:
**	This file contains definitions for the KEYSTRUCT structure
**	for Double Bytes characters.
**
**  History:
**	03/24/87(KY)  -- First written.
**	11/24/89 (dkh) - Added support for goto's from windex.
**	12/27/89 (dkh) - Added support for hot spot trim.
**	01/22/90 (dkh) - Fixed VMS compilation problems.
**	09-mar-90 (bruceb)
**		Added KS_LOCATOR for DECterm support.
**	10/14/90 (dkh) - Integrated round 4 of macws changes.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*
** structure definition for function key, Double Bytes char and single byte char
**  if 'ks_fk' is not zero, it's a function key
**  if 'ks_fk' is zero,     it's a Double Bytes char or single byte char.
*/
struct	    _key_struct
{
	i4	ks_fk;
	u_char	ks_ch[3];
	u_char	ks_align_dummy;	/* dummy field for alignment purposes only. */
	i4	ks_p1;
	i4	ks_p2;
	i4	ks_p3;
};
#define	    KEYSTRUCT	    struct  _key_struct
KEYSTRUCT   *TDgetKS(), *TDsetKS();

	KEYSTRUCT   *FKnxtch();

FUNC_EXTERN	KEYSTRUCT   *FKgetc();

FUNC_EXTERN	KEYSTRUCT   *FKgetinput();

GLOBALREF	KEYSTRUCT   *(*in_func)();

/*
**  If ks_fk is set to KS_GOTO, then we have a goto command from
**  windex.  A KS_MFAST indicates a menu fast path command.
*/

# define	KS_GOTO		-1
# define	KS_MFAST	-2
# define	KS_TURNON	-3
# define	KS_HOTSPOT	-4
# define	KS_LOCATOR	-5
# ifdef DATAVIEW
# define	KS_SCRUP	-6
# define	KS_SCRDN	-7
# define	KS_UP		-8
# define	KS_DN		-9
# define	KS_NXITEM	-10
# define	KS_SCRTO	-11
# endif	/* DATAVIEW */
