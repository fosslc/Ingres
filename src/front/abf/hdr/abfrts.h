/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<qgv0.h>
# include	<qg.h>
# include	<abfosl.h>

/**
** Name:	abfrts.h -	ABF Application RunTime Definitions File.
**
** Description:
**	Contains the declarations of the objects used in the ABF runtime system.
**
** History:
**	Revision 6.4  23-mar-1992 (mgw) Bug #36595 a.k.a. 41188
**		Changed ABFILSIZE from 32 to 48 to match the size of the
**		field on the form that is used to fill the arrays defined
**		as [ABFILSIZE+1].
**
**	Revision 6.3  89/11  wong
**	Added default application DBMS connection session ID.
**
**	Written 7/20/82 (jrc)
**	Modified 8/23/84 (jrc)   Added ABFRQDEF
**
**	8/85 (jhw) -- Moved to front-end headers; moved definitions
**		for ABF RTS object table names and member sizes from
**		"abf/abfconsts.h".
**	2/86 (prs)	Added constants for VIGRAPH.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*
** Name:	ABRT_SESSION -	Application DBMS Connection Session ID.
**
** Description:
**	Default ABF application DBMS connection session ID.
**
** History:
**	11/89 (jhw) -- Defined.
*/
#define ABRT_SESSION	-1


# define	ABCOMSIZE	30	/* the size of a command line */
# define	ABFILSIZE	48	/* The size of a file name */

/*
** Each of the different frame types has a unique runtime
** structure which is used to call the frame.
**
** !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
** NB
**	If these structures are changed, then abftab must be
**	changed so it produces the right code at runtime.
**	Also, since these are external structures, they must
**	be long word aligned.
**
** ALSO - these structures are passed to the interpreter through DS.
**	If modified, the DS templates used in ioi (ioirtt.c) must be modified
**	to reflect those changes.
*/

/*
** For user calls we need the menu table which has the name of the menu
** item and the procedure to call
*/
typedef struct {
	PTR	abrmenu;
	DB_DATA_VALUE	abrrettype;		/* One of OL_I4, ... */
} ABRTSUSER;

typedef struct {
	i4	*(*abrfunc)();
	DB_DATA_VALUE	abrrettype;		/* One of OL_I4, ... */
	i4	abrfid;
	i4	abrstatic;			/* is frame static? */
} ABRTSNUSER;

/*
** For QBF calls we need to know the relation name and the form name
** If it is a ABFRQDEF, then the name in relid is a QDEF
*/
typedef struct {
	char	abrqfrelid[FE_MAXNAME+1];
	char	abrqfcomline[ABCOMSIZE+1];
	i4	abrqfjdef;
} ABRTSQBF;

typedef struct {
	char	abrrwname[FE_MAXNAME+1];
	char	abrrwcomline[ABCOMSIZE+1];
	char	abrrwoutput[ABFILSIZE+1];
} ABRTSRW;

typedef struct {
	char	abrgfname[FE_MAXNAME+1];
	char	abrgfcomline[ABCOMSIZE+1];
	char	abrgfoutput[ABFILSIZE+1];
} ABRTSGBF;

/*
** A frame structure includes the name of the frame, its type
** and the calling structure used.
**
** This structure is used in a table by the runtime system of
** ABF to find each frame's definition.
*/
typedef	union abrvarstr {
	ABRTSQBF	abrfrvqbf;
	ABRTSUSER	abrfrvuser;
	ABRTSRW		abrfrvrw;
	ABRTSGBF	abrfrvgbf;
	ABRTSNUSER	abrfrvnuser;
} ABRTSVAR;

/*
** form information
*/
typedef struct {
	char	*abrfoname;
	PTR	abrfoaddr;
	i4	abrfopos;
	i4	abrfosource;
	i4	abrforef;
} ABRTSFO;

/* form source definitions */
#define ABFOSIF 1	/* image file */
#define ABFOSC 2	/* compiled form */
#define ABFOSDB 3	/* database */

typedef struct {
	char		*abrfrname;
	i4		abrfrtype;	/* one of the type enumerations */
	ABRTSFO		*abrform;
	ABRTSVAR	*abrfrvar;
	i4		abrnext;	/* Index of next */
	i4		abrdml;
} ABRTSFRM;

typedef struct {
	char		*abrgname;
	char		*abrgtname;	/* name of the datatype */
	DB_DATA_VALUE	abrgdattype;	
	i4		abrgnext;	/* Index of next */
	i4		abrgtype;	/* global or const */
} ABRTSGL;

typedef struct _a_attr {
	char		*abraname;	/* name of the attribute */
	char		*abratname;	/* name of the datatype */
	DB_DATA_VALUE	abradattype;
	i4		abraoff;	/* offset in data area */
} ABRTSATTR;

typedef struct {
	char		*abrtname;	/* name of the datatype */
	ABRTSATTR	*abrtflds;	/* list of fields */
	i4		abrtcnt;	/* # of dbdvs */
	i4		abrtsize;	/* size of data area */
	i4		abrtnext;	/* Index of next typedef */
} ABRTSTYPE;

/*
** some # defs for the union 
*/
# define	abrfrqbf	abrfrvar->abrfrvqbf
# define	abrfruser	abrfrvar->abrfrvuser
# define	abrfrrw		abrfrvar->abrfrvrw
# define	abrfrgbf	abrfrvar->abrfrvgbf
# define	abrfrnuser	abrfrvar->abrfrvnuser

/*
** returns
*/
# define	ABRRETURN		-1
# define	ABREXIT			-2

/*
** FRAME STACK
** the frame call stack is implemented with the following structure.
*/
typedef struct abrstktype
{
	char			*abrfsname;
	i4			abrfsusage;
	ABRTSFRM		*abrfsfrm;
	ABRTSPRM		*abrfsprm;
	FDESC			*abrfsfdesc;
	i4			abrfsdml;	/* Saved dml setting */
	struct abrstktype	*abrfsnext;
} ABRTSSTK;

/*
** PROC TABLE
** Contains the name and language of all the procedures that
** the application knows about.
*/
typedef struct abrproctype
{
	char	*abrpname;
	i4	abrplang;

	DB_DATA_VALUE	abrprettype;

	VOID	(*abrpfunc)();
	i4	abrpfid;
	i4	abrpnext;
	i4	abrpdml;
} ABRTSPRO;

/*}
** Name:	ABRTSOBJ	- An rts structure.
**
** Description:
**	This structures pulls together all the information needed
**	by the runtime system into a single place so it can
**	be encoded and decoded by DS.
**
** History:
**	10/89 (jhw) -- Added support for natural language.
**	12/89 (jhw) -- Added support for role ID and password flags.
**	03/90 (jhw) -- Made version 2.
**	10/90 (jhw) -- Should be version 3 from 6.3/03/00.  Bug #34284.
*/
typedef struct {
	i4		abroversion;	/* A version number */
# define ABRT_VERSION	3		/* Current version */
	char		*abrodbname;	/* The database name */
	char		*abrosframe;	/* The default starting frame */
	i2		abdml;		/* The application DML */
	i1		start_proc;	/* Whether to start with a procedure */
	i1		batch;		/* Whether application is batch */
	i4		abrohcnt;	/* Number of element in hash table */
	i4		*abrofhash;	/* The frame hash table */
	i4		*abrophash;	/* The procedure hash table */
	i4		abrofcnt;	/* The number of frames in abroftab. */
	i4		abropcnt;	/* The number of procs in abroptab. */
	i4		abrofocnt;	/* The number of forms in abrofotab. */
	ABRTSFRM	*abroftab;	/* The frame table */
	ABRTSPRO	*abroptab;	/* The procedure table */
	ABRTSFO		*abrofotab;	/* The form table */
	i4             language;       /* Natural language for application */
	char		*abroleid;	/* role ID (and password) */
	char		*abpassword;	/* user password (interpreter only) */
	i4		*abroghash;	/* The globals hash table */
	i4		*abrothash;	/* The typedefs hash table */
	i4		abroglcnt;	/* The number of globals in abrogltab */
	i4		abrotycnt;	/* The number of typedefs in abrotytab*/
	ABRTSGL		*abrogltab;	/* The globals table */
	ABRTSTYPE	*abrotytab;	/* The typedefs table */
	char		*abcndate;	/* Datestamp for constants file */
	char		*abroappl;	/* Application name */
} ABRTSOBJ;

/*}
** Name:	ABRTSFUNCS	- Name/Address Function Map Entry.
**
** Description:
**	Defines an entry that maps between the name of an application object and
**	its function address.  In particular, this structure structure is used
**	to link HL procs in the interpreter executable.
**
** History:
**	11/88 (joe) -- Written.
**	23-Aug-2009 (kschendel) 121804
**	    Since almost everything in the (system) tables return PTR, re-cast
**	    the function call arg.
*/
typedef struct {
	char	*name;
	PTR	(*addr)();
} ABRTSFUNCS;
