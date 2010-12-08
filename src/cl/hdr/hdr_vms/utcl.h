/*
** Copyright (c) 1985, Ingres Corporation
*/

#ifndef UTCL_DEFINED
#define UTCL_DEFINED

/**
** Name: UT.H - Global definitions for the UT compatibility library.
**
** Description:
**      The file contains the type used by UT and the definition of the
**      UT functions.  These are used for utility library support.
**
** History:
**      01-oct-1985 (jennifer)
**          Updated to codinng standard for 5.0.
**	9-sep-1986 (Joe)
**	   Updated to 5.0 spec.
**	5-feb-1987 (Joe)
**	   Put back nested include of array.h since these
**	   are now legal, and this is an example of where
**	   their use is important.
**	7-jun-93 (ed)
**	    created glhdr version contain prototypes, renamed existing file
**	    to xxcl.h and removed any func_externs
**      10/13/92 (dkh) - Changed the ModDict and ModDictDesc structures
**                       to no longer require the ArrayOf construct.
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**      06-Dec-2010 (horda03) SIR 124685
**          Fix VMS build problems.
**/

# include	<array.h>
# include       <ds.h>


/*
**  Forward and/or External function references.
*/
FUNC_EXTERN STATUS  UTcommand(); 


/* 
** Defined Constants
*/

/* UT return status codes. */

# define                UT_OK           0
# define		UT_CO_FILE	(E_CL_MASK + E_UT_MASK  + 0x01)
# define		UT_CO_IN_NOT	(E_CL_MASK + E_UT_MASK  + 0x02)
# define		UT_CO_LINES	(E_CL_MASK + E_UT_MASK  + 0x03)
# define		UT_CO_CHARS	(E_CL_MASK + E_UT_MASK  + 0x04)
# define		UT_CO_SUF_NOT	(E_CL_MASK + E_UT_MASK  + 0x05)
# define		UT_ED_CALL	(E_CL_MASK + E_UT_MASK  + 0x06)
# define		UTENOEQUAL	(E_CL_MASK + E_UT_MASK  + 0x07)
# define		UTENOPERCENT	(E_CL_MASK + E_UT_MASK  + 0x08)
# define		UTENOSPEC	(E_CL_MASK + E_UT_MASK  + 0x09)
# define		UTEBADARG	(E_CL_MASK + E_UT_MASK  + 0x0A)
# define		UTENOARG	(E_CL_MASK + E_UT_MASK  + 0x0B)
# define		UTEBIGCOM	(E_CL_MASK + E_UT_MASK  + 0x0C)
# define		UTEBADSPEC	(E_CL_MASK + E_UT_MASK  + 0x0D)
# define		UTENOBIN	(E_CL_MASK + E_UT_MASK  + 0x0E)
# define		UTENOPROG	(E_CL_MASK + E_UT_MASK  + 0x0F)
# define		UT_PR_CALL	(E_CL_MASK + E_UT_MASK  + 0x10)
# define		UTENoInit	(E_CL_MASK + E_UT_MASK  + 0x11)
# define		UTENoWriteFunc	(E_CL_MASK + E_UT_MASK  + 0x12)
# define		UTENoReadFunc	(E_CL_MASK + E_UT_MASK  + 0x13)
# define		UTENoModuleType	(E_CL_MASK + E_UT_MASK  + 0x14)
# define		UTENOSYM	(E_CL_MASK + E_UT_MASK  + 0x15)
# define		UTELATEEXE	(E_CL_MASK + E_UT_MASK  + 0x16)
# define		UTEBADTYPE	(E_CL_MASK + E_UT_MASK  + 0x17)
# define		UT_EXE_DEF	(E_CL_MASK + E_UT_MASK  + 0x18)

#define	UT_WAIT		0	/* normal */
#define	UT_NOWAIT	01
#define	UT_STRICT	02

# define	UT_KEEP	010

/*
** New for 4.0
**
** A struct to take an error and its parameters.
** And a constant for the maximum number of errors.
*/
# define UTEMAXERR	3
typedef struct
{
	i4	ute_err;
	i4	ute_cnt;
	char	*ute_argv[UTEMAXERR];
} UTERR;

typedef struct
{
	char	*ute_arg;	/* Formal argument's name */
	char	*ute_line;	/* OS dependent line to replace it with */
} UTEFORMAL;

typedef struct {		/* using struct to allow initialization */
	char	*modLoc;	/* file name (should be LOCATION?) */
	int	(*modFunc)();
} ModDesc;

typedef struct {
        int             size;   /* number of elements in array */
        UTEFORMAL       *array; /* pointer to array of elements */
} FormalArray;

typedef struct {
        char                    *modName;
        ModDesc                 *modDesc;       /* file or function */
        int                     modType;
#define                                 UT_CALL 1
#define                                 UT_PROCESS      2
#define                                 UT_OVERLAY      3
        FormalArray             modArgs;        /* not array of ptrs*/
} ModDict;

typedef struct {
        int     modDictType;
#define                                 UT_LOCATION     1
#define                                 UT_TABLE        2

        /* no union here to allow initialization */
        char                    *modDictLoc;    /* file name (LOCATION?) */
} ModDictDesc;

#endif
