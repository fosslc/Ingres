/*
**  Copyright (c) 2004 Ingres Corporation
**
**
**  Name: cr.h -- header for cr.c
**
**  Description:
**	This file contains essential definitions and prototypes for
**	functions defined in front!st!config!cr.c which compute changes
**	to INGRES configuration resources by interpreting configuration
**	rules.
**
**  History:
**	15-dec-92 (tyler)
**		Created.
**	11-feb-93 (tyler)
**		Added bool argument to CRsetPMval() and CRvalidate()
**		prototypes.
**	29-jan-93 (tyler)
**		Added char ** argument to CRvalidate() prototype.
**	22-jun-93 (tyler)
**		Changed bool argument to FILE * in CRsetPMval() and
**		CRvalidate() prototypes.  Change i4  argument to
**		PM_CONTEXT * in CRinit() and CRsetPMcontext() prototypes.  
**	23-jul-93 (tyler)
**		Added bool argument to CRsetPMval() prototype.  Added
**		missing header and past History.
**	04-aug-93 (tyler)
**		Added bool member "reference" to CR_NODE structure
**		to enable heuristic optimization to be performed within
**		CRsort().  Allows quick identification of unreferenced
**		symbols.  Added prototype for CRdependsOn().  Changed
**		PMsymToCRidx() to CRidxFromPMsym().
**	19-oct-93 (tyler)
**		Added CR_NO_RULES_ERR status and modified other
**		status symbol names.  Modified other error status
**		symbols.
**	31-oct-95 (nick)
**		Redefine all symbols beginning with '_' - using '_anything'
**		is a no-no and clashes in this case with _VOID.
**	27-nov-1995 (canor01)
**		Added EXEC_TEXT_STR type.
**	26-feb-1996 (canor01)
**		Added DIRECTORY and FILE constraint types.
**	07-mar-1996 (canor01)
**		Add prototype for CRpath_constraint(), which returns a
**		pointer to a function to do pathname validation.
**	06-jun-1996 (thaju02)
**		Modified prototype CRsetPMval() to return char pointer.
**      15-apr-2003 (hanch04)
**          Changed CRsetPMval to take an extra parameter.
**          CRsetPMval currently will print out to a file or
**          stdout if that is passed.  CRsetPMval show always be passwd the
**          config.log file to write to.  It may allow tee the output to 
**          stdout if TRUE is passed as the new flag.
**          This change fixes bug 100206.
**	24-June-2003 (wansh01) 	
** 	    Moved cr.h from front/st/config to front/st/hdr 	
**	15-March-2004 (gorvi01)
**	    Added ND_DECIMAL, ND_SIGNED_INT
**	    for inclusion of float and negative value constraints.
**	21-Aug-2009 (kschendel) 121804
**	    Add CRtype, fix CRsetPMval type.
**	24-Nov-2009 (frima01) Bug 122490
**	    Added prototype for CRresetcache to eliminate gcc 4.3 warnings.
*/


typedef struct node
{
	u_i4		type;
	char		*symbol;
	char		*str_val;	
	f8		num_val;
	struct node	*sym_val;
	struct node	*child0;
	struct node	*child1;
	struct node	*list;
	struct node	*constraint;
	bool		test;
	i4		index;
	bool		reference;

} CR_NODE;

# define CR_VOID	0
# define CR_INTEGER	1
# define CR_STRING	2
# define CR_BOOLEAN	3
# define CR_UNDEFINED	4


# define ND_VOID	0
# define ND_SYM_VAL	1 
# define ND_NUM_VAL	2
# define ND_STR_VAL	3
# define ND_BOOL_VAL	4
# define ND_UNDEFINED	5
# define ND_MATH_OP	6
# define ND_COMPARE_OP	7
# define ND_LOGIC_OP	8
# define ND_COND1	9
# define ND_COND2	10
# define ND_EXEC_TEXT	11
# define ND_MIN		12
# define ND_MAX		13
# define ND_SUM		14
# define ND_SET_MEMBER	15
# define ND_PRIME	16
# define ND_POWER2	17
# define ND_REQUIRES	18
# define ND_EXEC_TEXT_STR	19
# define ND_DIRECTORY	20
# define ND_FILESPEC	21
# define ND_DECIMAL	22  
# define ND_SIGNED_INT	23
# define ND_SIZETYPE	24

# define CR_NO_RULES_ERR	1 
# define CR_RULE_LIMIT_ERR	2 
# define CR_RECURSION_ERR	3 

typedef bool (BOOLFUNC)();

FUNC_EXTERN STATUS	CRcompute( i4, char ** );
FUNC_EXTERN bool	CRdependsOn( i4, i4  );
FUNC_EXTERN bool	CRderived( char * );
FUNC_EXTERN void	CRinit( PM_CONTEXT * );
FUNC_EXTERN STATUS	CRload( char *, i4  * );
FUNC_EXTERN char	*CRsymbol( i4  );
FUNC_EXTERN CR_NODE	*CRnode( void );
FUNC_EXTERN char	*CRsetPMval( char *, char *, FILE *, bool, bool );
FUNC_EXTERN void	CRsetPMcontext( PM_CONTEXT * );
FUNC_EXTERN void	CRsort( i4  *, i4  );
FUNC_EXTERN u_i4	CRtype( i4 );
FUNC_EXTERN STATUS 	CRvalidate( char *, char *, char **, FILE * );
FUNC_EXTERN i4		CRidxFromPMsym( char * );
FUNC_EXTERN BOOLFUNC	*CRpath_constraint( CR_NODE *, u_i4 );
FUNC_EXTERN bool	unary_constraint( CR_NODE *, u_i4);
FUNC_EXTERN STATUS 	CRresetcache();

# ifdef VMS
# define TEMP_DIR ERx( "sys$scratch:" ) 
# else /* VMS */
# define TEMP_DIR ERx( "/tmp/" )
# endif /* VMS */

