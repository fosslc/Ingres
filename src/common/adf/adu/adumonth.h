/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: ADUMONTHTAB.H -    Make read-only data defined in MONTHTAB.ROC known
**			    to other modules.
**
** Description:
**	used by monthchk.c and monthtab.roc
**
** History: $Log-for RCS$
**      14-apr-86 (ericj)
**	    Modified from 4.0/02 code for Jupiter.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	17-mar-87 (thurston)
**	    Added typedef name ADU_MONTHTAB instead of just using a struct name.
**	15-jun-89 (jrb)
**	    Changed GLOBALREF into GLOBALCONSTREF.
**      31-dec-1992 (stevet)
**          Added function prototypes.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

typedef struct _ADU_MONTHTAB
{
    char    *code;
    i4	    month;
}   ADU_MONTHTAB;

GLOBALCONSTREF	    ADU_MONTHTAB	    Adu_monthtab[];

GLOBALCONSTREF	    i4			    Adu_ii_dmsize[];

FUNC_EXTERN bool adu_1monthcheck(char    *input,
				 i4	*output);
FUNC_EXTERN i4 adu_2monthsize(i4  month,
			      i4  year);
