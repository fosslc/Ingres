/*
** Copyright (c) 2004, Ingres Corporation
*/

/**
** Name:	frmalign.h	-	Structure with version & alignment info
**
** Description:
**	This header file defines a structure which contains information
**	on the version and alignment restrictions of an OSL frame.
**	This information exists in 2 different forms:  for the
**	front-ends and for ADF.
**
** History:
**	26-mar-87 (agh)
**		First written.
**	1-apr-87  (agh)
**		Replaced separate members of the FRMALIGN structure with
**		the AFC_VERALN struct.
**
*/

typedef struct
{
	i2		fe_version;	/* version of frame as known to FEs */
	i2		fe_i1_align;	/* i1 alignment, as known to FEs */
	i2		fe_i2_align;	/* alignment for i2's */
	i2		fe_i4_align;	/* alignment for i4's */
	i2		fe_f4_align;	/* alignment for f4's */
	i2		fe_f8_align;	/* alignment for f8's */
	i2		fe_char_align;	/* alignment for char's */
	i2		fe_max_align;	/* align for FEs' maximum-sized type */
	AFC_VERALN	afc_veraln;	/* version and alignment info for ADF */
} FRMALIGN;
