/*
** Copyright (c) 2004 Ingres Corporation
** All rights reserved.
*/


#include	<erit.h>


/**
** Name:	ILERROR.h	-	Header file for error numbers
**
** Description:
**	Header file for error numbers.
**
*/

/*
** Error levels.
** An ILE_EQUEL error is caught by EQUEL or the Forms Runtime System.
** All other types of errors are flagged by the interpreter itself.
** The comments indicate the type of action taken.
*/
#define	ILE_EQUEL	01	/* proceed as EQUEL would */
#define	ILE_STMT	02	/* skip to next OSL statement */
#define	ILE_FRAME	04	/* return to calling frame */
#define	ILE_FATAL	010	/* exit from the application */
#define	ILE_SYSTEM	020	/* a system (not user) error */

/*
** Error numbers
*/
#define	ILE_COMLINE	E_IT0000_comline	/* Too many or few arguments */
#define	ILE_FLAGBAD	E_IT0001_flagbad	/* Bad flag on command line
						   (flag) */
#define	ILE_FAIL	E_IT0002_fail		/* Fail returned by some
						   routine */
#define	ILE_ARGBAD	E_IT0003_argbad		/* Bad argument passed to
						   routine */
#define	ILE_FILOPEN	E_IT0004_filopen	/* Could not open file
						   (filename) */
#define	ILE_ILORDER	E_IT0005_ilorder	/* IL statement found out of
						   order */
#define	ILE_ILMISSING	E_IT0006_ilmissing	/* Expected IL statement not
						   found (operator expected) */
#define	ILE_EOFIL	E_IT0007_eofil		/* Unexpected EOF reached in
						   IL file */
#define	ILE_TRFILE	E_IT0008_trfile		/* Could not open trace file
						   (file name) */
#define	ILE_FORMNAME	E_IT0009_formname	/* No form name for this frame
						   (frame name) */
#define	ILE_CLIENT	E_IT000A_client		/* Not ILRF client */
#define	ILE_FIDBAD	E_IT000B_fidbad		/* Bad frame identifier */
#define	ILE_CFRAME	E_IT000C_cframe		/* No current frame */
#define	ILE_FRDWR	E_IT000D_frdwr		/* Frame read/write failure */
#define	ILE_RADBAD	E_IT000E_radbad		/* Bad return address block */
#define	ILE_FRMDISP	E_IT000F_frmdisp	/* Could not display form
						   (form name) */
#define	ILE_JMPBAD	E_IT0010_jmpbad		/* Bad address for IL
						   statement */
#define	ILE_EXPBAD	E_IT0011_expbad		/* Bad compiled expression */
#define	ILE_DIVZERO	E_IT0012_divzero	/* Divide by 0 in an
						   expression */
#define	ILE_NUMOVF	E_IT0013_numovf		/* Numeric overflow or
						   underflow */
#define	ILE_FLTBAD	E_IT0014_fltbad		/* General floating point
						   exception */
#define	ILE_IMGBAD	E_IT0015_imgbad		/* Bad image file */
#define	ILE_CALBAD	E_IT0016_calbad		/* Failed call to frame or
						   procedure */
#define	ILE_DUPARG	E_IT0017_duparg		/* Duplicate argument on command
						   line (argument name) */
#define	ILE_NOMEM	E_IT0018_nomem		/* Ran out of memory */
#define	ILE_SYCALL	E_IT0019_sycall		/* Could not call system
						   (command line) */
#define	ILE_NOOBJ	E_IT001A_noobj		/* Specified ABF object
						   non-existent */
