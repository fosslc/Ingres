/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** Name: SLCL.H
**
** Description:
**	This file contains system-specific components of the SL modules
**
** 	It is automatically included in SL.H (which is in GLHDR)
**
** History:
**	24-may-93 (robf)
**	   Created
**	11-aug-93 (robf)
**	   Define SL_LABEL_TYPE to be SL_LABEL_TY_SUN_CMW 
**	9-feb-94 (robf)
**         Allow for 128 categories in Ingres default labels
**	15-feb-1994 (ajc)
**	   Added hp-bls specific entries.
**	22-mar-94 (arc)
**	   <sys/label.h> #defines EXTERNAL on Sun CMW. This causes defns
**	   in front!hdr!hdr!graf.h to be erroneously switched out so
**	   we 'undefine' it.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
/*
** The system specific label type, valid types are in SL.H
**
** This MUST be defined to one of the legal values in SL.H
**
** For non-secure systems define it to SL_LABEL_TY_TEST which is a
** portable generic label implementation.
*/
#ifdef su4_cmw
# define	SL_LABEL_TYPE  SL_LABEL_TY_SUN_CMW	/* CMW MAC */
#else
# if defined(hp8_bls)
# define        SL_LABEL_TYPE  SL_LABEL_TY_HP_BLS      /* HP-BLS MAC */
# else
# define	SL_LABEL_TYPE  SL_LABEL_TY_TEST	/* Test Mac */
# endif
#endif

# include <systypes.h>

#if SL_LABEL_TYPE == SL_LABEL_TY_SUN_CMW
/*
** This symbol is only defined on CMW native cc.
** For base Sun 4.1.1 imported acc, we have to define the symbol
*/
# ifndef SunOS_CMW
# define SunOS_CMW 1
# endif

# undef ulong
# include <sys/label.h>
# undef EXTERNAL
#endif 

#if SL_LABEL_TYPE == SL_LABEL_TY_HP_BLS

# ifndef HP_UX_BLS
# define HP_UX_BLS 1
# endif

# include <mandatory.h>
# endif
/* 
** Security Label defintion, this is system specific. 
*/
struct IISL_LABEL
{
	/* Operating system dependent stuff */
# if SL_LABEL_TYPE == SL_LABEL_TY_TEST
	/*
	** Internal test MAC labels
	*/
	i4	level;
	char    categories[128/BITSPERBYTE];	/* 128 categories */
	char    pad[CL_SLABEL_MAX- sizeof(i4)-sizeof(char[128/BITSPERBYTE])];
#else
# if SL_LABEL_TYPE==SL_LABEL_TY_SUN_CMW
	/*
	** SUN-CMW Labels
	*/
    	bclabel_t	cmw_label;
# elif  SL_LABEL_TYPE==SL_LABEL_TY_HP_BLS
	
	/*
	** HP-BLS Labels
	*/
	mand_ir_t   bls_label;
	/*
	** mand_ir_t is not fixed in size and can grow therefore we
	** have to allocate some space to copy into. Arbitary size might
	** need to change (currently set to 132 bytes)
	*/
	long  mand_spare[33];

#else
	# error No security labels are defined in slcl.h. This is an error.
# endif /* SL_LABEL_TY_SUN_CMW */
#endif
};

/*
** Maximum external size 
*/
# define	SL_MAX_EXTERNAL   (2000)
