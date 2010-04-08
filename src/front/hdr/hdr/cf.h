/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	cf.h -	CopyForm Interface Definitions File.
**
** Description:
**
** History:
**	09-jul-1987 (rdesmond) written.
**	30-Nov-1989 (Mike S) Remove references to IIUIdba and IIUIuser
**	04/08/91 (dkh) - Fixed bug 36573.  Put in better bullet proofing
**			 for corrupted copyform files.
**	17-mar-92 (seg) SIgetrec must be declared (only) in <si.h>
**      07-Dec-95 (fanra01)
**          Changed externs to GLOBALREFs
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      06-feb-2009 (stial01)
**          Added COPYFORM_MAX_REC
**/

/* external declarations */

GLOBALREF		char	Cfversion[];
GLOBALREF		bool	Cf_silent;

STATUS	cf_addobj();
STATUS	cf_adddet();
bool	cf_conflict();
STATUS	cf_convto60();
i4	cf_dmpobj();
bool	cf_objexist();
char	*cf_gettok();
bool	cf_inarray();
i4	cf_rdobj();
i4	cf_rectype();
STATUS	cf_writeobj();
i4	oo_save();

#define	i2ascii_len	7
#define	i4ascii_len	14
#define DATELEN		26
#define	MAX_OBJNAMES	100

# define RT_QBFHDR	1
# define RT_JDEFHDR	2
# define RT_FORMHDR	3
# define RT_FIELDHDR	4
# define RT_TRIMHDR	5
# define RT_QBFDET	6
# define RT_JDEFDET	7
# define RT_FORMDET	8
# define RT_FIELDDET	9
# define RT_TRIMDET	10
# define RT_BADTYPE	11
# define RT_EOF		12

# define CF_QBFNAME	1
# define CF_JOINDEF	2
# define CF_FORM	3

# define FORMDET_COUNT	16	/* # of elements in a form detail line */
# define FIELDDET_COUNT	27	/* # of elements in a field detail line */
# define TRIMDET_COUNT	7	/* # of elements in a trim detail line */

#define COPYFORM_MAX_REC (1024 + FE_MAXNAME)

typedef struct
{
    char    *name;
    i4	    id;
} FORMINFO;
