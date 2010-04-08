/*
**  Copyright (c) 2004 Ingres Corporation
**  All Rights Reserved.
*/
# ifndef PM_HDR_INCLUDED
# define PM_HDR_INCLUDED 1

/*
**
**  Name: pm.h -- header for the General Libraray PM functions
**
**  Description:
**	The header must be included to use functions contained in pm.c
**
**  History:
**	20-oct-92 (tyler)
**		Created.
**	14-dec-92 (tyler)
**		Added FUNC_EXTERN to external function prototypes.	
**	18-jan-93 (tyler)
**		Added PM_NO_INIT return status. 
**	24-may-93 (tyler)
**		Added prototype for PMexpToRegExp().
**	27-may-93 (tyler)
**		Added PM_NO_ENV return status.
**	21-jun-93 (tyler)
**		Added declarations for PM_LIST_REC and PM_CONTEXT required
**		to allow use of caller allocated control blocks in calls
**		to (non-GL) multiple context PM functions.  Increased the
**		PM hash table size from 97 to 503 to improved performance
**		on large data sets.
**	23-jun-93 (tyler)
**		Changed PM_HASH_SIZE back to 97 (from 503) and PM_MAX_ELEM
**		back to 10 (from 32) since the larger values made the
**		PM_CONTEXT structure too large and didn't seem warranted.
**	25-jun-93 (tyler)
**		Added PM_INVALID return status.
**	26-jul-93 (tyler)
**		Added PM_NO_MEMORY return status.  Added prototypes for
**		multiple context PM functions approved by the CL committee
**		on 9-jul-93.
**	18-oct-93 (tyler)
**		Added prototypes for PMhost() and PMmHost().
**	16-dec-93 (tyler)
**		Fixed bogus error STATUS values.
**	27-may-97 (thoda04)
**		Added prototype for PMnumElem().
**	31-mar-1997 (canor01)
**	    Protect against multiple inclusion of pm.h.
**	27-aug-1997 (canor01)
**	    Add prototypes for PMwrite() and PMmWrite().
**	24-sep-97 (mcgem01)
**	    Add prototype for PMlowerOn().
**      22-nov-00 (loera01) bug 103318
**          Added PM_DUP_INIT.
**	21-jan-1999 (canor01)
**	    Allow removing dependency on generated erglf.h file.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	09-feb-2001 (somsa01)
**	    Changed mem_free type to SIZE_TYPE in PM_CONTEXT.
**	02-aug-2001 (somsa01)
**	    Added new boolean to PM_CONTEXT which is true if data
**	    has been loaded into this context.
**	19-apr-2001 (devjo01)
**	    Add missing declarations for PMgetElem() & PMmGetElem().
**	17-dec-2003 (somsa01)
**	    Added PMmLowerOn() prototype.
**	03-feb-2004 (somsa01)
**	    Backed out last change for now.
**	6-Dec-2004 (schka24)
**	    Add missing decls for ExpandRequest's.
**	    64-bit callers were not amused.
**	    Aside from PMmLowerOn (see above) I think decls are all there now.
**	17-Dec-2009 (whiro01)
**	    Add back PMmLowerOn.  It's used in various places and the prototype
**	    is already in "pmutil.h".
*/

/*
** STATUS values of various functions, from <erglf.h>
*/
# define E_GL_MASK              0xD50000
# define E_PM_MASK              0x6000
# define PM_SYNTAX_ERROR	(E_GL_MASK + E_PM_MASK + 0x00)
# define PM_OPEN_FAILED		(E_GL_MASK + E_PM_MASK + 0x01)
# define PM_BAD_FILE		(E_GL_MASK + E_PM_MASK + 0x02)
# define PM_FILE_BAD		PM_BAD_FILE
# define PM_BAD_INDEX		(E_GL_MASK + E_PM_MASK + 0x03)
# define PM_FOUND		(E_GL_MASK + E_PM_MASK + 0x04)
# define PM_NOT_FOUND		(E_GL_MASK + E_PM_MASK + 0x05)
# define PM_BAD_REGEXP		(E_GL_MASK + E_PM_MASK + 0x06)
# define PM_NO_INIT		(E_GL_MASK + E_PM_MASK + 0x07)
# define PM_NO_II_SYSTEM	(E_GL_MASK + E_PM_MASK + 0x08)
# define PM_BAD_REQUEST		(E_GL_MASK + E_PM_MASK + 0x09)
# define PM_NO_MEMORY		(E_GL_MASK + E_PM_MASK + 0x0a)
# define PM_DUP_INIT       	(E_GL_MASK + E_PM_MASK + 0x0b)

/*
** NSUBEXP and RE_EXP should be moved to re.h - if and when RE becomes
** a GL module.
*/

# define NSUBEXP  10

typedef struct regexp
{
	char	*startp[ NSUBEXP ];
	char	*endp[ NSUBEXP ];
	char	regstart;		/* Internal use only. */
	char	reganch;		/* Internal use only. */
	char	*regmust;		/* Internal use only. */
	i4	regmlen;		/* Internal use only. */
	char	program[1];		/* Unwarranted compiler chumminess */

} RE_EXP;

# define PM_MAX_ELEM	10	/* Maximum number of name elements */

# define PM_HASH_SIZE	97	/* Number of hash buckets for PM structure */

/* PM list structure */

typedef struct rec
{
	char		*name;
	char		*elem;
	char		*value;
	struct rec	*owner;
	struct rec	*next;

} PM_LIST_REC;

/* PM control block */

typedef struct {
	u_i2		memory_tag;
	bool		force_lower;
	char		*mem_block;
	SIZE_TYPE	mem_free;
	long		total_mem;
	i4		num_name;
	PM_LIST_REC	*htab[ PM_MAX_ELEM ][ PM_HASH_SIZE ];
	PM_LIST_REC	**list;
	i4		list_len;
	char		*def_elem[ PM_MAX_ELEM ];
	i4		high_idx;
	char		*request_cache;
	char		*expand_cache;
	RE_EXP		*load_restriction;
	i4		magic_cookie;
	bool		data_loaded;

} PM_CONTEXT;

typedef struct
{
	RE_EXP	*expbuf;
	i4	last; 

} PM_SCAN_REC;

typedef VOID PM_ERR_FUNC( STATUS, i4, ER_ARGUMENT * );

FUNC_EXTERN STATUS	PMdelete( char * );
FUNC_EXTERN STATUS	PMmDelete( PM_CONTEXT *, char * );
FUNC_EXTERN char	*PMexpandRequest( char * );
FUNC_EXTERN char	*PMmExpandRequest( PM_CONTEXT *, char * );
FUNC_EXTERN char	*PMexpToRegExp( char * );
FUNC_EXTERN char	*PMmExpToRegExp( PM_CONTEXT *, char * );
FUNC_EXTERN void	PMfree( void );
FUNC_EXTERN void	PMmFree( PM_CONTEXT * );
FUNC_EXTERN STATUS	PMget( char *, char ** );
FUNC_EXTERN STATUS	PMmGet( PM_CONTEXT *, char *, char ** );
FUNC_EXTERN char	*PMgetDefault( i4  );
FUNC_EXTERN char	*PMmGetDefault( PM_CONTEXT *, i4  );
FUNC_EXTERN char	*PMgetElem( i4 idx, char *name );
FUNC_EXTERN char	*PMmGetElem( PM_CONTEXT *,i4 idx, char *name );
FUNC_EXTERN char	*PMhost( void );
FUNC_EXTERN char	*PMmHost( PM_CONTEXT * );
FUNC_EXTERN STATUS	PMinit( void );
FUNC_EXTERN STATUS	PMmInit( PM_CONTEXT ** );
FUNC_EXTERN STATUS	PMinsert( char *, char * );
FUNC_EXTERN STATUS	PMmInsert( PM_CONTEXT *, char *, char * );
FUNC_EXTERN STATUS	PMload( LOCATION *, PM_ERR_FUNC * );
FUNC_EXTERN STATUS	PMmLoad( PM_CONTEXT *, LOCATION *, PM_ERR_FUNC * );
FUNC_EXTERN i4    	PMmNumElem( PM_CONTEXT *, char * );
FUNC_EXTERN i4    	PMnumElem( char * );
FUNC_EXTERN STATUS	PMmRestrict( PM_CONTEXT *, char * );
FUNC_EXTERN STATUS	PMrestrict( char * );
FUNC_EXTERN STATUS	PMscan( char *, PM_SCAN_REC *, char *, char **,
				char ** );
FUNC_EXTERN STATUS	PMmScan( PM_CONTEXT *, char *, PM_SCAN_REC *,
				char *, char **, char ** );
FUNC_EXTERN STATUS	PMsetDefault( i4, char * );
FUNC_EXTERN STATUS	PMmSetDefault( PM_CONTEXT *, i4, char * );
FUNC_EXTERN STATUS      PMmWrite( PM_CONTEXT *context, LOCATION *loc );
FUNC_EXTERN STATUS      PMwrite( LOCATION *loc );
FUNC_EXTERN void        PMlowerOn( void );
FUNC_EXTERN void	PMmLowerOn( PM_CONTEXT *context );



# endif /* ! PM_HDR_INCLUDED */
