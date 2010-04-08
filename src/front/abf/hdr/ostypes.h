/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/
#define u_i4	long

/**
** Name:	ostypes.h -	OSL Parser Recognized Variables Types.
**
** Description:
**	Contains definitions for the types of variables recognized by the
**	OSL parser.
**
** History:
**	Revision 2.0  87/01/10  13:59:29  john
**	Initial revision
**	
**	Revision 5.1  86/10/17  16:35:33  arthur
**	Code used in 'beta' release to DATEV.
**		
**	Revision 5.0  86/09/22  11:54:58  wong
**	Initial revision (moved from "ostree.h".)
**		
*/

/*
** types
*/
# define	N_UNKNOWN	DB_NODT
# define	N_BOOL		1	/* AD_BOO_TYPE */
# define	N_INT		DB_INT_TYPE
# define	N_FLOAT		DB_FLT_TYPE
# define	N_STRING	DB_CHR_TYPE	/* Char [] */
# define	N_RTSSTR	DB_DYC_TYPE
# define	N_QRY		DB_QRY_TYPE	/* Query structure */
# define	N_SPTR		7	/* Char * */
# define	N_RTSPV		8
# define	N_RTSPR		9	/* ABRTSPR */
# define	N_RTSPRM	10	/* ABRTSPRM */
# define	N_ORTSPRM	11	/* OABRTSPRM */
# define	N_OLRET		12	/* OL_RET */
# define	N_NOTYPE	13	/* Procedure or frame declared as not
						returning a value */
# define	N_QPTR		14	/* Pointer to QRY structure */
