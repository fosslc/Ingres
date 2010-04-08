/*
** Copyright (c) 1985, 2004 Ingres Corporation
*/

#include <systypes.h>

/**
** Name: CV.H - Header for CV compatilibity library routines.
**
** Description:
**      The file contains the type used by CV and the definition of the
**      CV functions.  These are used for converting data types.
**
** History: 
 * Revision 1.1  88/08/05  13:50:32  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
**      Revision 1.3  87/11/16  10:50:02  mikem
**      add CV_DATE and CV_PARAM
**      
**      Revision 1.2  87/11/11  13:32:39  mikem
**      Initial checkin of 6.0 changes to 50.04 baseline cl
**      
**      Revision 1.2  87/10/16  13:58:12  mikem
**      
**      
**      01-oct-1985 (derek)
**          Created new for 5.0.
**	14-nov-86 (thurston)
**	    Incorporated the CVNET.H file into this hdr file, so the user
**	    will not have to know about including TWO cl hdr files for CV.
**	    The following history records have been brought in from CVNET.H:
**	        31-jul-86 (ericj)
**	            Converted for Jupiter.
**	        16-sep-86 (neil)
**	            Added CV_FLTSZ from 5.0.
**	        13-nov-86 (thurston)
**	            Got rid of CV_FLOATSZ, as it conflicts with CV_FLOAT, and
**	            has been replaced with CV_FLTSZ.
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	19-jun-89 (jrb)
**	    Added FUNC_EXTERNs for decimal CV functions.
**	16-mar-90 (fredp)
**	    Added FUNC_EXTERN for CVuahxl().
**	24-sep-90 (jrb)
**	    CVpka now returns STATUS.  Added #defines needed by new cvpk.c.
**	7-jun-93 (ed)
**	    created glhdr version contain prototypes, renamed existing file
**	    to xxcl.h and removed any func_externs
**	30-Jun-1993 (daveb)
**	    remove old useless history, add CV_DEC_PTR_SIZE and 
**	    CV_HEX_PTR_SIZE defines.
**	25-Apr-1997 (kosma01)
**	    moved MAXDIGITS from cvfa.c and cvfcvt.c to here and
**	    renamed it CV_MAXDIGITS.
**      28-Jul-1998 (wanya01)
**          set CV_MAXDIGITS to 400 in case some platform needs
**          large buffer to hold result from calling fcvt. 
**	27-Jan-2004 (fanra01)
**	    Add include of stdlib for standard C prototype to strtoll.          
**	02-Apr-2004 (somsa01)
**	    Include systypes.h to resolve compile problems on HP-UX.
**	20-Apr-04 (gordy)
**	    Moved network definitions to cvgcccl.h.
**/


/*
**  Forward and/or External function references.
*/
# include <stdlib.h>


/*
**  Defined constants.
*/

#define                 CV_OK           0
#define                 CV_SYNTAX       (E_CL_MASK + E_CV_MASK + 0x01)
#define			CV_UNDERFLOW	(E_CL_MASK + E_CV_MASK + 0x02)
#define			CV_OVERFLOW	(E_CL_MASK + E_CV_MASK + 0x03)
#define			CV_TRUNCATE	(E_CL_MASK + E_CV_MASK + 0x04)


/*
**  Defines of other constants.
*/

/* Options to CVpka function */

#define	CV_PKLEFTJUST		0x01
#define	CV_PKZEROFILL		0x02
#define	CV_PKROUND		0x04

/*
** The maximum number of digits returned 
** in the floating point conversion buffer.
*/
#define CV_MAXDIGITS       400

/*
** The following values are machine dependant.
*/

# ifdef PTR_BITS_64	/* 64 bit pointer values */

# define CV_DEC_PTR_SIZE	22
# define CV_HEX_PTR_SIZE	16

# else			/* assume 32 bit pointer values */

# define CV_DEC_PTR_SIZE	11
# define CV_HEX_PTR_SIZE	8

# endif


FUNC_EXTERN VOID CVexp10(
#ifdef CL_PROTOTYPED
	i4              exp,
	f8              *val
#endif
);

FUNC_EXTERN STATUS CVfcvt(
#ifdef CL_PROTOTYPED
	f8      value,
	char    *buffer,
	i4      *digits,
	i4      *exp,
	i4      prec
#endif
);

FUNC_EXTERN STATUS CVecvt(
#ifdef CL_PROTOTYPED
	f8      value,
	char    *buffer,
	i4      *digits,
	i4      *exp
#endif
);

