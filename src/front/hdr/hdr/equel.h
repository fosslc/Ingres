/*
** Copyright (c) 1984, 2008 Ingres Corporation
**
*/

/*
+* Filename:	equel.h
** Purpose:	Define globals for EQUEL.
**
** Defines:
**		EQ_STATE	*eq	- Global EQUEL information.
-*		DML_STATE	*dml	- Global DML (ESQL) information.
** Notes:
**	- Users must include <si.h> first.
**
** History:
**	10-nov-1984		Written	(ncg)
**	15-aug-1985		Added DML layer	(mrw)
**	25-jan-89 (marian)	
**		Added EQ_COMMON for warning messages for non-common 
**		sql commands. 
**	11-Jul-89 		MPE/XL-specific EQ_SI_OPEN_LANG (bryanp)
**	02-oct-89 (neil)
**		Added eq_config, EQ_ADVADS and EQ_ADVAX for dynamic toggle of
**		VAX/VMS and Vads Ada support from eqmain.c.
**      07-feb-1990 (kwatts)
**		Added EQ_COBUNS to eq_config for dynamic control of use
**		of structured MF COBOL
**	17-jul-1990 (gurdeep rahi) - Defined flag to select Alsys-Ada at
**		runtime (see file embed/equel/eqmain.c)
**	20-nov-1992 (Mike S)
**		Add DML code for 4GL.
**	16-dec-1992 (larrym)
**		Added new eq->flags EQ_SQLSTATE and EQ_SQLCODE.  FIPS requires 
**		that we make status information available through global
**		variables called SQLSTATE and SQLCODE.  These flags tell us
**		to generate code to support this.
**	10-jun-1993 (kathryn)
**	    Added eq_flag EQ_ENDIF for FIPS.  Set in sc_scanhost(), currently 
**	    FORTRAN only, it indicates that we must generate code to end an 
**	    IF block ("end if") following the code generated for the current 
**	    SQL stmt. 
**	10-jun-1993 (kathryn)
**	    	Added new eq->flags flag EQ_CHRPAD.  This is set via an ESQL/C 
**	        startup flag, to specify FIPS data assigment rules for fixed 
**		length "C" host variables.  When this flag is set, a pair of 
**		IIsqMods() calls (one to turn the behavior on, and one to turn 
**		it off)  will be generated in around each SQL/FRS/4GL stmt that
**		may assign data to host vars, such as  SELECT, GETROW, etc..
**	23-nov-1993 (teresal)
**		Add new eq->flags flag EQ_CHREOS. This is set for ESQL/C when 
**		"-check_eos" preprocessor flag is given. This causes
**		Ingres to check for a null terminator for character variables
**		(declared with a fixed length buffer) at runtime. EQ_CHREOS
**		causes a call to IIsqMods() before each IIputdomio and
**		IIvarstrio. Bug #55915.
**	28-mar-1994 (larrym)
**	        added EQ_SQLSTA to workaround bug #60322.  Long term solution
**		is to generate reference to sqlstate from symbol table, but 
**		this needs to be fixed asap.
**	28-mar-1994 (teresal)
**		Add new ESQL/C++ preprocessor. Code generated for C++
**		will be generated as C code (the runtime will only see C).
**		Modified eq_islang(), which is used numerous times in other 
**		files to see if this the C preprocessor, to indicate that
**		running C++ is in essence running a C-like preprocessor. 
**      12-Dec-95 (fanra01)
**          Added definitions for referencing data in DLLs on windows NT from
**          code built for static libraries.
**      06-Feb-96 (fanra01)
**          Changed flag from __USE_LIB__ to IMPORT_DLL_DATA.
**      10-Mar-96 (thoda04)
**          Added function prototypes and function prototype arguments.
**	18-Dec-97 (gordy)
**	    Added EQ_MULTI flag for multi-threaded application support.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-feb-2002 (toumi01)
**	    add explicit support for GNAT Ada compiler (initially identical
**	    to Verdix Ada)
**	07-sep-2005 (abbjo03)
**	    Replace xxx_FILE constants by SI_xxx.
**	24-mar-2007 (toumi01)
**	    Add EQ_ATASKBODY flag for tracking scan of Ada "TASK BODY"
**	    so that sqlca declarations at the task level can be added for
**	    multi-thread support for the Ada language.
**	20-Jun-2008 (hweho01)
**	    Add EQ_FOR_DEF64 flag for including 64-bit Fortran definition  
**	    files on hybrid ports.  
**	28-Aug-2009 (kschendel) b121804
**	    Drop the old WIN16 section, misleading and out of date.  Better
**	    to just reconstruct as needed.
**	24-Nov-2009 (frima01) Bug 122490
**	    Added prototypes for yyparse and yyinit to eliminate 
**	    gcc 4.3 warnings.
*/

# ifndef	T_NONE
#    include	<eqrun.h>
# endif		/* T_NONE */

/*
** The following structure should be the only global piece of information 
** in the Equel preprocessor, known to all modules.  It may be a good idea
** to hide the fields of the structure into a file, that can only be
** accessed by a call.  However the current implementation is to have the
** single structure, known to everyone.  The members of the structure should
** correspond only to relevant state information, and not to some minor detail 
** that a particular routine needs.
**
** Important: When checking which language preprocessor is being run,
**	      don't do:  "eq->eq_lang == EQ_C" always use the eq_islang
**	      macro; eq_islang(EQ_C) will return true for a C compatible
**	      language like C++. This way the C++ preprocessor will 
**	      "inherit" C semantics.
*/

typedef struct 
{
    i4		eq_lang;		/* Language being preprocessed */
    i4		eq_line;		/* Current line in input file */
    char	*eq_filename;		/* Short name of preprocessed file */
    char	*eq_ext;		/* Extension of file name */
    char	*eq_outname;		/* Full output name */
    char	*eq_in_ext,		/* Default include input extension */
		*eq_out_ext;		/* Default include output extension */
    char	*eq_def_in,		/* Default main input extension */
		*eq_def_out;		/* Default main output extension */
    FILE	*eq_infile;		/* Input file descriptor */
    FILE	*eq_outfile;		/* Output file descriptor */
    FILE	*eq_listfile,		/* List file descriptor */
		*eq_dumpfile;		/* Dump file descriptor */
    i4	eq_flags;		/* State flags - see below */
    char	eq_sql_quote;		/* The SQL string delimiter */
    char	eq_host_quote;		/* The host language string delimiter */
    char	eq_quote_esc;		/* Char prefix escapes host delimiter */
    i4	eq_config;		/* Dynamic compiler configurations:
					** This field is a flags field that
					** should be set to mutually exclusive
					** values, unless compilers share
					** properties. This field can be used to
					** replace compile-time configuration
					** ifdef's to allow support for multiple
					** compilers on one machine, as well as
					** cross compilation.  The values need
					** not be different across different
					** languages.
					*/
/* ADA options */
/* ----------- */
# define EQ_ADVADS    0x01		/* Verdix/Vads Ada (-avads) */
# define EQ_ADVAX     0x02		/* VAX/VMS Ada (-avax) */
# define EQ_ADALSYS   0x04		/* Alsys-Ada (-aalsys) */
# define EQ_ADGNAT    0x08		/* GNAT Ada (-agnat) */

/* COBOL options */
/* ------------- */
# define EQ_COBUNS    0x01		/* In MF COBOL generate unstructured 
					** code( no END-thing or CONTINUE)
					*/
} EQ_STATE;

# if defined(NT_GENERIC) && defined(IMPORT_DLL_DATA)
GLOBALDLLREF EQ_STATE	*eq;		/* Global state vector known to all */
# else              /* NT_GENERIC && IMPORT_DLL_DATA */
GLOBALREF EQ_STATE	*eq;		/* Global state vector known to all */
# endif             /* NT_GENERIC && IMPORT_DLL_DATA */

/* Macros to access the global Eq */

/*
** eq_islang 	- This macro is used to tell what preprocessor language
**		  is being run, e.g., eq_islang(EQ_COBOL) will
**		  return true if we are running the COBOL preprocessor.
**		  
**		  Note: Because C++ is built on top of C, this macro will
**		  return true if you are asking about C, and you have
**		  a C compatible language like C++. For example:
**
**			eq_islang(EQ_C) = 		true	For ESQL/C
**		  	eq_islang(EQ_C) = 		true	For ESQL/C++
**
**		  C doesn't have the functionality of
**		  C++, so if you are asking about C++, this macro 
**		  returns true only if you are actually running C++:
**
**			eq_islang(EQ_CPLUSPLUS) = 	false	For ESQL/C
**			eq_islang(EQ_CPLUSPLUS) = 	true	For ESQL/C++
*/

# define	eq_islang( l )		( l == EQ_C ? ((eq->eq_lang == EQ_C) \
				       || (eq->eq_lang == EQ_CPLUSPLUS))     \
						    : (eq->eq_lang == l))
/*
** eq_genlang	Macro to determine which language to generate for the
**		runtime calls. For C++, we will generate C as the language
**		for runtime.
*/
# define	eq_genlang( l )		( l == EQ_CPLUSPLUS ? EQ_C : l )

/* Different states for flags field */
/* NOTE: don't forget to update flags in front!embed!equel!eqdump.c */
# define EQ_IIDEBUG	001		/* Runtime debugging */
# define EQ_COMPATLIB	002		/* Compatlib typedefs */
# define EQ_STDIN	004		/* Read from input */
# define EQ_STDOUT	010		/* Generate to output */
# define EQ_LIST	020		/* Produce listing file */
# define EQ_LSTOUT	040		/* Output to listing file too */
# define EQ_SETLINE	0100		/* Generate compiler #line directives */
# define EQ_2POSL	0200		/* Second pass of OSL */
# define EQ_FMTANSI	0400		/* Generate ANSI-std output format */
# define EQ_NOREPEAT	01000		/* REPEAT queries not allowed */
# define EQ_QRYTST	02000		/* Use repeatable query names */
# define EQ_INCNOWRT	04000		/* "-o": Don't write include files */
# define EQ_VERSWARN	010000		/* print version warning messages */
# define EQ_INDECL	020000		/* In an ESQL declaration section */
# define EQ_COMMON	040000		/* Warn about non-open sql commands */
# define EQ_FIPS	0100000		/* Warn about non-fips sql statements */
# define EQ_SQLSTATE	0200000		/* SQLSTATE declared in module */
# define EQ_SQLCODE	0400000		/* SQLCODE declared in module */
# define EQ_NOSQLCODE	01000000	/* ingnore SQLCODE declared in module */
# define EQ_ENDIF       02000000        /* close IF block after this SQL stmt */
# define EQ_CHRPAD      04000000 	/* FIPS assign rules for "C" chr vars */
# define EQ_CHREOS      010000000 	/* FIPS check eos for "C" chr vars */
# define EQ_SQLSTA	020000000	/* SQLSTA declared in FORTRAN module */
# define EQ_MULTI	040000000	/* Multi-threaded application */
# define EQ_ATASKBODY	0100000000	/* Ada "TASK BODY" scanned */
# define EQ_FOR_DEF64 	0200000000	/* 64-bit Fortran definition files*/

# define	eq_ansifmt		(eq->eq_flags & EQ_FMTANSI)

/*
** DML
**	- the Data Manipulation Language structure
**	- contains EQUEL/ESQL/??? specific info
*/

typedef i4	(*func_ptr)();	/* It's a pointer to a func returning a i4  */

typedef struct dml {
    i4		dm_lang;		/* DML_EQUEL or DML_ESQL */
    i4		dm_exec;		/* DML_HOST|DML_DECL|DML_EXEC mode */
    i4		*dm_gentab;		/* really GEN_IIFUNC *; gen table ptr */
    struct {				/* scanner margins for non-C langs */
	i4	dm__left;
	i4	dm__right;
    } dm_margin;
    func_ptr	dm_lex,			/* scanner */
		dm_contin,		/* check for line continue (unused) */
		dm_strcontin,		/* check for string continuation */
		dm_iscomment,		/* checks if line is a comment */
		dm_caarg,		/* add communication arg for II calls */
		dm_cursordmp,		/* dump the cursor data structures */
		dm_sqlcadmp,		/* dump the sqlca stuff */
		dm_sqintodmp,		/* dump sql fetch stuff */
		dm_repeat,		/* hook for sql repeat queries */
		dm_scan_lnum;		/* BASIC hook to scan line numbers */
} DML_STATE;

# define	dml_islang( l )		(dml->dm_lang == l)
# define	dm_left			dm_margin.dm__left
# define	dm_right		dm_margin.dm__right

/* for dm_lang */
# define	DML_EQUEL	1
# define	DML_ESQL	2

# if defined(NT_GENERIC) && defined(IMPORT_DLL_DATA)
GLOBALDLLREF DML_STATE	*dml;		/* Global state vector known to all */
# else              /* NT_GENERIC && IMPORT_DLL_DATA */
GLOBALREF DML_STATE	*dml;		/* Global state vector known to all */
# endif             /* NT_GENERIC && IMPORT_DLL_DATA */

/* States for dml_exec field */
# define DML_HOST	0		/* Host-code state, EQUEL/ESQL */
# define DML_DECL	1		/* In EXEC SQL BEGIN DECLARE SECTION */
# define DML_EXEC	2		/* In EXEC SQL */
# define DML_QUEL	3		/* EQUEL only!  non-host code */

/* Extra flags for use with DML_EXEC */
# define DML__SQL	01000		/* EXEC SQL */
# define DML__FRS	02000		/* EXEC FRS */
# define DML__4GL	04000		/* EXEC 4GL */


/*
** Non-C file opener (for CMS langs require an F 80 file)
** Example:    EQ_SI_OPEN_LANG( opened, &f_loc, "w", &ofile );
*/
#ifndef CMS
#ifdef  hp9_mpe
# define   EQ_SI_OPEN_LANG( result, fileloc, mode, outfile )              \
	    result = SIfopen( fileloc, mode, SI_TXT, 256, outfile );
#else   /* hp9_mpe */
	/* neither CMS, nor MPE/XL: */
# define   EQ_SI_OPEN_LANG( result, fileloc, mode, outfile ) 	  	  \
	    result = SIopen( fileloc, mode, outfile )
#endif  /* hp9_mpe */
#else   /* CMS */
# define   EQ_SI_OPEN_LANG( result, fileloc, mode, outfile ) 	  	  \
	   if (!eq_islang(EQ_C))					  \
	       result = SIfopen( fileloc, mode, SI_TXT, -80, outfile ); \
	   else 							  \
	       result = SIopen( fileloc, mode, outfile )
#endif  /* CMS */

/* prototypes */

FUNC_EXTERN	i4	yyparse();
FUNC_EXTERN	void	yyinit();

/* Error routines */
FUNC_EXTERN	void	er_write(/*ER_MSGID ernum,nat severity,nat parcount,...*/);
FUNC_EXTERN	i4 	er_exit(void);
FUNC_EXTERN	char	*er_na(i4 er_nat);

/* Error reporting constants */
# define  	ER_FILE		stdout       	/* Place to dump our errors */

# define	EQ_ERROR	0
# define	EQ_WARN		1
# define	EQ_FATAL	3
