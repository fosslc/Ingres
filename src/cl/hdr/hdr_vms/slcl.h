/*
** Copyright (c) 1993, 2000 Ingres Corporation
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
**         Allow for 128 categories in Ingres default labels.
**	5-may-94 (robf)
**         Removed UNIX-specific code
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, i4, u_nat, & u_i4
**	    from VMS CL as the use is no longer allowed
*/
/*
** The system specific label type, valid types are in SL.H
**
** This MUST be defined to one of the legal values in SL.H
**
** For non-secure systems define it to SL_LABEL_TY_TEST which is a
** portable generic label implementation.
*/

# define	SL_LABEL_TYPE  SL_LABEL_TY_TEST	/* Test Mac */
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
	# error No security labels are defined in slcl.h. This is an error.
#endif
};

/*
** Maximum external size 
*/
# define	SL_MAX_EXTERNAL   (2000)
