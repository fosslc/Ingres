
/*
**	Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	gn.h	-	GN definitions
**
** Description:
**	This header file defines encodes to be used with the GN library
**
** History:
**	4/87 (bobm)	written
**	24-Feb-2010 (frima01) Bug 122490
**	    Add function prototypes as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

/*
** Error statii
*/
#define GNE_CODE 101
#define GNE_EXIST 102
#define GNE_MEM 103
#define GNE_FULL 104
#define GNE_NAME 105
#define GNE_PARM 106

/*
** case rules
*/
#define GN_UPPER 1	/* force upper case */
#define GN_LOWER 2	/* force lower case */
#define GN_MIXED 3	/* preserve user case - still case insensitive */

#define GN_SIGNIFICANT 4	/* case sensitive */

/*
** non-alphanumeric treatments
*/
#define GNA_REJECT 1	/* GNE_NAME if name contains any */
#define GNA_XUNDER 2	/* translate to underscores */
#define GNA_ACCEPT 3	/* accept as is */
#define GNA_XPUNDER 4	/* translate to underscores, except '.' left alone */
			/* this is useful for file names */

/* prototypes */

FUNC_EXTERN STATUS      IIGNssStartSpace();
FUNC_EXTERN STATUS      IIGNgnGenName();
FUNC_EXTERN STATUS      IIGNonOldName();
FUNC_EXTERN STATUS      IIGNesEndSpace();

