/*
** Copyright (c) 1985, 2004 Ingres Corporation
**
** NM.h -- Name header
**
** History: Log:	nmlocal.h,v
**	3-15-83   - Written (jen)
**
**	Revision 1.2  86/05/23  02:51:57  daveb
**	Update for cached symbols.
**
**	Revision 30.2  85/09/17  11:11:52  roger
**	NMgtUserAt() is now a macro.
**		
**	11-Mar-89 (GordonW)
**	    Added extern to NMtime for symbol table mdoification time
**	25-jan-1989 (greg)
**	    Removed obsolete junk.
**	16-aug-95 (hanch04)
**	    Removed NMgtIngAt, now defined in nm.h
**      12-Dec-95 (fanra01)
**          getenv function defined as FUNC_EXTERN.
**          Changed extern to GLOBALREF.
**          Changed extern funtions to FUNC_EXTERN.
**	03-jun-1996 (canor01)
**	    Moved definition of NM_static structure here from nmfiles.c.
**	09-dec-1996 (donc)
**	    Added SYM_VAL to support Optional OpenROAD style symbol lookups.
**	    Added defines for MAX_CONFIG_LINE and SLASH
**	27-may-97 (mcgem01)
**	    Clean up compiler warnings.
**	27-nov-1997 (canor01)
**	    Define manifest constants for quote characters.
**      03-Jul-98 (fanra01)
**          Removed definition of NMgtUserAt for NT as getenv does not return
**          environment from DLL.
**	12-dec-2003 (somsa01)
**	    Added storage of symbol.bak, symbol.log, and symbol.lck locations
**	    in II_NM_STATIC.
**	03-feb-2004 (somsa01)
**	    Backed out last change for now.
**	06-apr-2004 (somsa01)
**	    Added storage of symbol.bak and symbol.log locations in
**	    II_NM_STATIC.
**      17-Nov-2004 (rigka01) bug 112953/INGSRV2955, cross Unix change 472415
**          Lock the symbol table while being updated to avoid corruption or
**	    trunction.
**	11-Feb-2009 (drivi01)
**	    Export NMgtEnv().
*/
 
# include	<tm.h>
# include	<mu.h>
 
# define	MAXLINE		256
# define	MAX_CONFIG_LINE 1024

# ifndef NT_GENERIC
FUNC_EXTERN char	*getenv();

# define	NMgtUserAt(A,B)	(*B = getenv(A))
# endif

typedef struct _SYM
{
	struct _SYM	*s_next;
	char		*s_sym;
	char		*s_val;
	struct _SYM	*s_last;
} SYM;
 
typedef struct _II_NM_STATIC
{
        char            Locbuf[ MAX_LOC + 1 ];  /* for NMloc() */
        bool            ReadFailed;             /* for NMreadsyms */
        LOCATION        Sysloc;                 /* II_SYSTEM */
        char            Sysbuf[ MAX_LOC + 1 ];  /* II_SYSTEM */
        LOCATION        Admloc;                 /* II_ADMIN */
        char            Admbuf[ MAX_LOC + 1 ];  /* II_ADMIN */
	LOCATION        Lckloc;			/* II_CONFIG / II_ADMIN */
	char 		Lckbuf[ MAX_LOC + 1 ];  /* II_CONFIG / II_ADMIN */
	LOCATION        Bakloc;                 /* II_CONFIG / II_ADMIN */
	char            Bakbuf[ MAX_LOC + 1 ];  /* II_CONFIG / II_ADMIN */
	LOCATION        Logloc;                 /* II_CONFIG / II_ADMIN */
	char            Logbuf[ MAX_LOC + 1 ];  /* II_CONFIG / II_ADMIN */
        bool            Init;                   /* NM_initsym called */
	bool		SymIsLocked;	 	/* symbol.lck held */
        MU_SEMAPHORE    Sem;                    /* semaphore */
} II_NM_STATIC;

/* List of symbols, empty until symbol.tbl is read. */
GLOBALREF SYM *s_list;
 
/* Location of the symbol table file, remembered throughout the session */
 
GLOBALREF LOCATION NMSymloc;
GLOBALREF SYSTIME NMtime;
 
/* Function declarations */
FUNC_EXTERN VOID	NMgtEnv();
FUNC_EXTERN FILE	*NMopensyms();
FUNC_EXTERN STATUS	NMaddsym();
FUNC_EXTERN STATUS	NMreadsyms();
FUNC_EXTERN STATUS	NMstIngAt();
FUNC_EXTERN STATUS	NMwritesyms();
FUNC_EXTERN STATUS	NMflushIng();
FUNC_EXTERN STATUS	NM_initsym();
FUNC_EXTERN STATUS 	NMzapIngAt();


# define	NULL_STRING	""
# define	NULL_CHR	1
# define	NULL_NL		2
# define	NULL_NL_NULL	3

#define SLASH		'\\'
#define SLASHSTR	"\\"

#define DQUOTESTR	"\""
#define SQUOTESTR	"\'"
 
typedef struct _sym_val	/* For linked list of symbol-value pairs */
{
    struct _sym_val * next;
    char *	symbol;
    char *	value;
} SYM_VAL;

/* end of private nm.h */
