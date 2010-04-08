/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<array.h>
# include	<ds.h>

/**
** Name:	ut.h -	Compatibility Library Utility Interface Definition.
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
**	Sept. 90 - (bobm)
**	   Integrate changes for UTlink / UTcompile enhancements -
**	   warning / fail returns.
**	7-jun-93 (ed)
**	    created glhdr version contain prototypes, renamed existing file
**	    to xxcl.h and removed any func_externs
**	23-may-97 (mcgem01)
**	    Clean up prototypes.
**	05-Dec-1997 (allmi01)
**	    Added selected bypass of prototypes for sgi_us5.
**      24-jul-1998 (rigka01)
** 	    Cross integrate bug 86989 from 1.2 to 2.0 codeline
**          bug 86989: When passing a large CHAR parameter with report,
**          the generation of the report fails due to parm too big
**          Move UTECOM definition from utexe.c to here
**              to allow its use by related items
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	20-Jul-2004 (lakvi01)
**		SIR #112703, Added STATUS as return type to UTcomline.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
**/

/*
**  Forward and/or External function references.
*/

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
# define        	UT_LD_DEF       (E_CL_MASK + E_UT_MASK  + 0x19)
# define       	 	UT_LD_OBJS      (E_CL_MASK + E_UT_MASK  + 0x1A)
# define        	UT_CMP_WARNINGS	(E_CL_MASK + E_UT_MASK  + 0x1B)
# define        	UT_CMP_FAIL     (E_CL_MASK + E_UT_MASK  + 0x1C)
# define        	UT_LNK_WARNINGS (E_CL_MASK + E_UT_MASK  + 0x1D)
# define        	UT_LNK_FAIL     (E_CL_MASK + E_UT_MASK  + 0x1E)

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
	int	ute_err;
	int	ute_cnt;
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
	char			*modName;
	ModDesc			*modDesc;	/* file or function */
	int			modType;
#define					UT_CALL	1
#define					UT_PROCESS	2
#define					UT_OVERLAY	3
	ArrayOf(UTEFORMAL)	modArgs;	/* not array of ptrs*/
} ModDict;

typedef struct {
	int	modDicType;
#define					UT_LOCATION	1
#define					UT_TABLE	2

	/* no union here to allow initialization */
	char			*modDictLoc;	/* file name (LOCATION?) */
	ArrayOf(ModDict *)	modDicTable;
} ModDictDesc;

typedef struct {
        union {
                char    *comline;
                Array   *comargv;
        } result;
        ModDict *module;
} UT_COM;


FUNC_EXTERN UTcomline(
#if defined(CL_PROTOTYPED) && !defined(sgi_us5)
        SH_DESC                 *sh_desc,
        SH_DESC                 *desc_ret,
        i4                      mode,
        PTR                     dict,
        PTR                     dsTab,
        char                    *program,
        char                    *arglist,
        UT_COM                  *com,
        CL_ERR_DESC             *err_code,
        i4                      N,
        PTR			*argn
#endif
);

FUNC_EXTERN VOID UTopentrace(
#ifdef CL_PROTOTYPED
	FILE    **Trace
#endif
);

FUNC_EXTERN VOID UTlogtrace(
#ifdef CL_PROTOTYPED
	LOCATION        *errfile,
	STATUS          status
#endif
);

#define UTECOM 4094

