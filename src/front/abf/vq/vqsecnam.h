/*
**	Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	vqsecnam.h - 4GL error section names
**
** Description:
**	Emerald errors occur in sections delimited by special comments.
**	These constants define the section names.  
**
** History:
**	11/13/89 (Mike S) 	Initial version
**	26-aug-92 (blaise)
**		Added EST_LOCPROC (error in local procedure declaration or
**		source code)
**/


/*
**	Major types
*/
# define EST_NONE	0	/* error outside section */
# define EST_HIDDEN	1	/* error in hiden field declaration */
# define EST_CALL	2	/* error in frame call */
# define EST_SELECT	3	/* error in generated SELECT */
# define EST_INSERT	4	/* error in generated INSERT */
# define EST_ESCAPE	5	/* error in escape code */
# define EST_DEFAULT	6	/* error in field default */
# define EST_SOURCE	7	/* error in source code  -- 
				** i.e. user has edited source */
# define EST_LOCPROC	8	/* error in local procedure declaration or 
					source code */

# define EST_UNKNOWN	-1	/* error in unknown section */

/*
**	Minor types for SELECT or INSERT
*/
# define EST_QMASTER	1	/* Error in master section */
# define EST_QDETAIL	2	/* Error in detail section */

/* Minor types for escape are listed in abf!hdr!metafrm.qsh */
