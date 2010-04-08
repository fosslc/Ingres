/*
** Copyright (c) 1985, Ingres Corporation
*/

/**
** Name: CV.H - Header for CV compatilibity library routines.
**
** Description:
**      The file contains the type used by CV and the definition of the
**      CV functions.  These are used for converting data types.
**
** History:
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
**	17-apr-89 (jrb)
**	    Added FUNC_EXTERNs for new CV routines for DECIMAL.
**	20-jul-89 (jrb)
**	    Added new #defs for options to CVpka and changed its return value
**	    from VOID to STATUS.
**	7-jun-93 (ed)
**	    created glhdr version contain prototypes, renamed existing file
**	    to xxcl.h and removed any func_externs
**	22-Jul-1993 (daveb)
**	    add CV_DEC_PTR_SIZE and CV_HEX_PTR_SIZE defines.
**	20-Apr-04 (gordy)
**	    Moved network definitions to cvgcccl.h
**      14-Aug-2009 (horda03) B122474
**          Moved MAXDIGITS from cvfa.c and cvfcvt.c to here
**          and renamed to CV_MAXDIGITS (this was done on
**          UNIX in 1997). Also changed the value to 400
**          (again to mimic the Unix value), as the 308 value
**          was allowing buffer overrun with numbers >1.0e308
**/


/*
**  Forward and/or External function references.
*/


/*
**  Defined constants.
**
**  NOTE: Some of the following statuses are returned by MACRO code in some
**	  CV routines.  If you change any of these you must change the MACRO
**	  routines which return them.
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
** The following values are machine dependant -- may be 64 bits on Alpha.
*/

# ifdef PTR_BITS_64	/* 64 bit pointer values */

# define CV_DEC_PTR_SIZE	22
# define CV_HEX_PTR_SIZE	16

# else			/* assume 32 bit pointer values */

# define CV_DEC_PTR_SIZE	11
# define CV_HEX_PTR_SIZE	8

# endif

