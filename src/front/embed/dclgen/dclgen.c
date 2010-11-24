/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<pc.h>		/* 6-x_PC_80x86 */
# include	<cm.h>
# include	<er.h>
# include	<ex.h>
# include	<lo.h>
# include	<me.h>
# include	<si.h>
# include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<adf.h>		/* Needed by afe.h */
# include	<fe.h>
# include	<afe.h>
# include	<ui.h>
# include	<ug.h>
# include	<adh.h>		/* For external date lengths */
# include	<eqcob.h>
# include	"erdc.h"

/**
+* Filename:	dclgen.c
** Purpose:	Generate host-language structure based on the format in the 
**		database table.
**
** Defines:	
**	main()		Entry point; calls other routines
**	dc_dumpdesc()	Gets table desc from ingres and dumps to file
**	dc_savedesc()	Saves column descriptions into structure	
**	dc_getargs()	Processes command line arguments
**	dc_getalias()	Get alias for a column name with invalid chars
**	dc_getlang()	Gets language argument from user
**	dc_getdbname()	Gets database name argument from user
**	dc_gettabname()	Gets table name argument from user
**	dc_getfilename()Gets output file name from user
**	dc_getstrucname()Gets data structure name from user
**	dc_namechk()	Checks database/table name for validity
**	dc_usagerr()	Outputs usage message and terminates program execution
**	dc_chklang()	Checks language name for validity
**	dc_ada()	Language dependent code generator
**	dc_basic()	  "	    "	    "	   "
**	dc_c()		  "	    "	    "	   "
**	dc_cobol()	  "	    "	    "	   "
**	dc_fortran()	  "	    "	    "	   "
**	dc_fortran()	  "	    "	    "	   "
**	dc_pascal()	  "	    "	    "	   "
**	ex_handle()	Cleans up after interrupt
**
** Usage:
**		dclgen language database table file structure
**
**		Generates a table description and a structure mapping of the 
**		named database table and dumps it into the given file.  The 
**		file default name is "table.ext".
-*
** Examples (VMS):
**		$ dclgen fortran db tab tab.for tabstruct 
**		
** Notes:
**		F77 Fortran and CMS Fortran do not allow structures.  The
**		5th argument to DCLGEN, the structure arg, is replaced with
**		a "prefix" arg.  The prefix is appended to the start of
**		each variable name (equivalent to each structure member on
**		VMS).  This helps ensure that variable names will be unique
**		even when names are shared between different db tables.
**
** History:
**	21-jun-1985 (barbara)	- Written.
**	26-oct-1985 (peter)	- Added MKMFIN Hints.
**	02-apr-1986 (neil)	- Added ADA declarations.
**	27-jun-1986 (neil)	- Modified PASCAL code.
**	13-may-1987 (brent)	- Fix bug 11479 -- uppercase relnames aren't
**					found.  See dc_tbnamechk() for details.
**	30-jul-1987 (barbara)	- Updated for 6.0. (No direct access to system
**				 	catalogs)
**	03/16/88 (dkh) 		- Added EXdelete call to conform to coding spec.
**	13-mar-89 (sylviap)	- Passes dbname to DBMS as entered.  Does not 
**				  check for illegal characters.  If error,
**				  DBMS will emit error.
**				- Call IIUGlbo_lowerBEobject to handle case 
**				  sensitive tablenames.
**				- DG changes:
**					Change underscore to dash.
**					DG comment indicator is '*' in col. 1.
**	10-oct-89 (neil)	- Added dc_config for Ada configuration.  This
**				  can be modified using the "-a" flag.  Other
**				  config changes for other languages can use
**				  other flags.
**	28-dec-89 (kenl)
**				- Added handling of the +c 
**				  (connection/with clause) flag.
**	27-mar-90 (teresal)	- Modified dc_fortran to generate 5 spaces
**				  rather than a tab before line continuation
**				  character on UNIX. Fix for Bug 8250.
**	06-sep-1990 (barbara)
**	    Convert underscores in column names to dashes when outputting
**	    COBOL record elements on Unix.
**      19-sep-1990 (pete)
**          Change FEingres() to FEningres(). No dictionary check required.
**	14-sep-1990 (teresal)	- Modified to support decimal data type.
**      10-oct-1990 (stevet)
**          Added IIUGinit call to initialize character set attribute table.
**	20-dec-1990 (kathryn)
**	    Added -f77 flag which indicates no structures.
**	29-nov-91 (seg)
**	    ex_handle needs to return STATUS, not void.  Also, leave
**	    logic for exiting in calling func.
**	16-jun-92 (seng/fredb)
**	    Integrate dcarter's 20-jun-90 changes from 6202p:
**		20-jun-90 (dcarter)
**        	   - ifdef'd in call to SIfopen for HP3000 instead of SIopen
**		     call.  We need to be able to specify file type for
**		     compiler-edible code.
**        	   - Copied UNIX ifdefs to declare floating point items as
**		     packed decimal for HP3000 COBOL.
**        	   - Copied sylviap's mod to change underscores to dash for
**		     HP3000 COBOL also.
**      16-nov-1992 (larrym)
**          Added CUFLIB to NEEDLIBS.  CUF is the new common utility facility.
**	17-nov-1992 (lan)
**	    Added long varchar support.
**	15-jan-1993 (lan)
**	    Added owner.tablename and delimited identifier support.
**	16-sep-1993 (sandyd)
**	    Added byte, byte varying, long byte support.  For non-C languages,
**	    we just use a string variable and warn them that byte inserts
**	    might get garbled over hetnet unless they use an SQLDA with
**	    genuine byte datatypes in their sqlvar entries.  For C, we generate
**	    a VARBYTE embedded struct.
**	    Also fixed a bug in the handling of the E_DC000A_BADCHRSTR warning.
**	    (It was outside the loop, so it would only get displayed if the
**	    long type was the last column in the table.)
**	03/25/94 (dkh) - Added call to EXsetclient().
**	21-apr-1993 (geri)
**	    Bug 54017:  added support for delimited ids in column names by
**	    replacing "illegal" characters such as spaces with a legal 
**	    character. Also put quotes around delimited column names in
**	    the DECLARE TABLE statement.
**	20-dec-93 (robf)
**          Added security_label support. These are returned into 
**	    a C VARBYTE structure in internal format initially, and 
**	    a regular string for other languages.
**	25-apr-1994 (johnst)
**	    Bug #60014
**          When generating C structure declaration for case LgenSTRUCT in 
**	    dc_c(), do not unconditionally map DB_INT_TYPE to C long type for
**	    i4's. This causes problems on 64-bit platforms like Dec Alpha OSF/1
**          where long's are 64-bit while int's are 32-bit quantities. Use 
**	    sizeof() test instead to determine whether "long" or "int" should 
**	    be used.
**      11-jan-1996 (toumi01; from 1.1 axp_osf port)
**          Added kchin's change for axp_osf
**          21-feb-94 (kchin)
**          When generating ADA structure declaration for DEC ADA on DEC
**          OSF/1, map the following INGRES type to DEC ADA type:
**          INGRES type     DEC ADA type
**          ===========     ============
**          integer1        short_short_integer
**          float4          float
**          float           long_float
**	20-sep-1996 (thoda04)
**	    Ported to Windows 3.1.  Cleaned up warnings.
**	21-jun-1999 (kitch01)
**      Bug 97524. On NT floats/money fields were being declared as COMP-1
**	    or COMP-2 for COBOL dclgen's. These were then thrown out as
**	    invalid by esqlcbl preprocessor. COMP-1/2 is only allowed if
**	    COB_FLOATS has been defined, I changed the ifdefs to use this flag.
**      19-sep-2000 (ashco01)
**          Bug: 102697. When generating BASIC structure declaration for ALPHA 
**          VMS BASIC map ingres FLOAT and MONEY datatypes to new SFLOAT
**          & TFLOAT AlphaVMS BASIC 1.4 Datatypes.
**              INGRES type             ALPHA VMS BASIC type
**              ===========             ====================
**              float4                  sfloat (previously 'single')
**              float8                  tfloat (previously 'double')
**              money                   tfloat (previously 'double')
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	19-mar-2001 (somsa01)
**	    Updated MING hint for program name to account for different
**	    product builds.
**      01-jun-2001 (bolke01)
**          Bug: 104828. When generating COBOL structure declaration for ALPHA 
**          VMS COBOL map ingres FLOAT and MONEY datatypes are set to COMP-3 which
** 	    should be COMP-1/COMP-2. Included the header file eqcob.h to resolve 
**          missing declaration of COB_FLOATS for VMS 
**	13-aug-2001 (kinte01)
**	    Remove their version/your version comments from previous integration
**	12-oct-2001 (gupsh01)
**	    Added support for unicode data types ie, nchar, nvarchar and 
**	    long nvarchar. Since unicode support is at present only for 
**	    c /cpp language only these have been modified. Procedures dc_c 
**	    and dc_savedesc and dc_adftosq are updated.
**	25-nov-2004 (komve01)
**	    Added support for bigint SQL data type. Procedures dc_c,dc_cobol,dc_fortran
**	    dc_pascal, dc_pli, dc_basic and dc_savedesc and dc_adftosq are updated.
**	25-jul-2008 (toumi01)
**	    Modify 1994 fix for i4's long vs. int decision to prefer int.
**      28-jan-2009 (stial01)
**          Use DB_MAXNAME for database objects, LNAME_MAX for language
**	16-Jun-2009 (thich01)
**	    Treat GEOM type the same as LBYTE.
**	20-Aug-2009 (thich01)
**	    Treat all spatial types the same as LBYTE.
**      18-feb-2010 (joea)
**          Add support for DB_BOO_TYPE.  Remove obsolete PL/I code.
*/


static		i4	dc_dblang = DB_SQL;	/* default db lang for DCLGEN */
static		FILE	*dc_outf = NULL;
/*
** Various buffers that are used:
**	database name, table name, "-X" flag, filename and structure name 
*/
# define	DC_ARGLEN	64
# define	DC_COMMSIZE	2*DB_GW1_MAXNAME + 60
# define	LNAME_MAX	32  /* programming language name size max */
static	char	dc_dbname[DB_GW1_MAXNAME+1]	 ZERO_FILL;
static	char	dc_tabname[DB_GW1_MAXNAME+1] ZERO_FILL;
static	char	dc_xflag[MAX_LOC+1]	 ZERO_FILL;
static		char	dc_filename[MAX_LOC+1] 	ZERO_FILL;
static		char	dc_strucname[DC_ARGLEN+1]	ZERO_FILL;
static		bool	dc_iningres = FALSE;
static		char	dc_msg[MAX_LOC+1]	ZERO_FILL;
static		char	dc_lname[LNAME_MAX+1]	ZERO_FILL;  /* which language,
							    ** fortran, 
							    ** cobol, etc */
static		char	*dc_pp = ERx("");	/* EQUEL ## */
static		i4	nostruct = FALSE;	/* See "Notes" above */
static		char	*valid_char = ERx("_");	/* valid char in column names */
/* 
** Dynamic configuration flag for languages that allow command line
** compiler generation.
*/
#define		DC_ADVAX	0x01		/* Vax Ada */
#define		DC_ADVADS	0x02		/* Vads Ada */
#ifdef VMS
static		i4	dc_config = DC_ADVAX;
#else /* UNIX */
static		i4	dc_config = DC_ADVADS;
#endif

/*
** DC_ATT - Stores information for later structure dumping. Relies on the
**	    fact that at most MAXDOM columns are in any table.
*/
typedef struct	{
    char	atname[DB_GW1_MAXNAME+1];
    i4		atadftype;	/* ADF type code */
    i4		atexsize;	/* External size */	
} DC_ATT;
static	DC_ATT	dc_attab[DB_GW2_MAX_COLS] ZERO_FILL;   /* for holding attributes */
static	i4	dc_attcnt 	ZERO_FILL;

/* Language dependent entries into dc_lang */
# define	LgenCOMMENT 	(i4)0
# define	LgenMARGIN  	(i4)1
# define	LgenCONTINUE	(i4)2
# define	LgenTERM    	(i4)3
# define	LgenSTRUCT  	(i4)4

# define	L_ALL	1
# define	L_NONE	0
#ifdef VMS
# define	L_VMS	1	/* Support on VMS only */
#else 
# define	L_VMS	0
#endif

void	dc_c(), dc_fortran(), dc_pascal(), dc_cobol(), dc_ada(), 
	dc_basic();
GLOBALDEF void	(*dc_lang)() = NULL;
void	dc_getargs(i4, char**);
void	dc_dumpdesc(void);
i4  	dc_adftosq(i4, i4, i4, char*);
void	dc_savedesc(char *, i4, i4, i4);
void	dc_getlang(void);
void	dc_getdbname(char *);
void	dc_gettabname(char *);
void	dc_getfilename(char *);
void	dc_getstrucname(char *);
void    dc_usagerr(void);
bool	dc_namechk(char *);
bool	dc_chklang(char *);
bool	dc_getalias(char *, char *);
static STATUS ex_handle();
void	FEing_exit(void);

FE_RSLV_NAME	rslvname;
char		tablename[DB_MAXNAME + 1];
char		ownername[DB_MAXNAME + 1];
	    	

/*{
+* Procedure:	main
** Purpose:	The main routine of DCLGEN
** Parameters:
**	argc	- i4	- command line arg count
**	argv	- **char - command line arguments
** Return Values:
-*	Returns OK or FAIL to the caller
*/

/*
**	MKMFIN Hints.
**

PROGRAM =	(PROG0PRFX)dclgen

NAME = dclgen	

NEEDLIBS =	UFLIB RUNTIMELIB FDLIB FTLIB FEDSLIB UILIB \
		LIBQLIB UGLIB FMTLIB AFELIB LIBQGCALIB CUFLIB \
		GCFLIB ADFLIB COMPATLIB MALLOCLIB

UNDEFS =	II_copyright
*/


main( argc, argv )
i4	argc;
char	*argv[];
{
    LOCATION		loc;
    FILE		*outf;
    EX_CONTEXT		context;

    /* Tell EX this is an ingres tool. */
    (void) EXsetclient(EX_INGRES_TOOL);

    /* Use the ingres allocator instead of malloc/free default (daveb) */
    MEadvise( ME_INGRES_ALLOC );

    /* Call IIUGinit to initialize character set attribute table */
    if ( IIUGinit() != OK)
    {
	PCexit(FAIL);
    }

    FEcopyright(ERx("DCLGEN"), ERx("1985"));
    dc_getargs( argc, argv );
    if (EXdeclare( ex_handle, &context ) != OK)
    {
	EXdelete();
	if (dc_iningres)
	{
	    FEing_exit();
	}
	if (dc_outf)
	    SIclose( dc_outf );
	PCexit( FAIL );
    }

    SIprintf( ERx("%s\n\n"), ERget(S_DC00A1_WORK) );
    SIflush( stdout );
    if (FEningres((char *)NULL, (i4)0, dc_dbname, dc_xflag, NULL) != OK)
    {
	IIUGfer( stdout, E_DC0001_OPENDB, 0, 1, dc_dbname );
	PCexit( FAIL );
    }
    dc_iningres = TRUE;

    /* DG tablenames can be mixed case, so do not CVlower */
    /*IIUGlbo_lowerBEobject(dc_tabname);*/
    rslvname.name = dc_tabname;
    rslvname.name_dest = tablename;
    rslvname.owner_dest = ownername;
    rslvname.is_nrml = FALSE;
    if (FE_fullresolve(&rslvname) != TRUE)
    {
	IIUGfer( stdout, E_DC0002_NOTABLE, 0, 2, dc_tabname, dc_dbname );
	FEing_exit();
    	PCexit( FAIL );
    }

    LOfroms( PATH & FILENAME, dc_filename, &loc );
#ifdef hp9_mpe
    if (SIfopen(&loc, ERx("w"), SI_TXT, 256, &outf) != OK)
#else
    if (SIopen(&loc, ERx("w"), &outf) != OK)
#endif /* hp9_mpe */
    {
	IIUGfer( stdout, E_DC0003_FILE, 0, 1, dc_filename );
	FEing_exit();
    	PCexit( FAIL );
    }
    dc_outf = outf;
    dc_dumpdesc();
    FEing_exit();
    SIclose( dc_outf );
    EXdelete();
    PCexit( OK );
}

/*
+* Procedure:	dc_dumpdesc
** Purpose:	Gets table description from Ingres and dumps into file
** Parameters:
**	None.
** Return Values:
-*	None
** Notes:
** 	What is dumped:
** 1.   A comment (not for QUEL)
** 2.   SQL-type description of database table (not for QUEL)
** 3.   A language-dependent data structure definition.
*/

void
dc_dumpdesc()
{
    FE_ATT_QBLK	qblk;
    FE_ATT_INFO atinfo;
    char	sqltype[DB_TYPE_SIZE];
    char	commbuf[DC_COMMSIZE];
    i4		rval;
    bool	first = TRUE;
    register FILE	*df = dc_outf;
    char	 temp_name[DB_MAXNAME + 1];
    char	 alias_column[DB_MAXNAME + 1];
    char	*name_ptr;
    i4	adflength;	
    DB_DT_ID	datatype;
    bool	DC000A_already_displayed = FALSE;
    bool	DC000B_already_displayed = FALSE;

    if (FEatt_open( &qblk, tablename, ownername) != OK)
    {
	IIUGfer( stdout, E_DC0004_OPNTABLE, 0, 1, dc_tabname );
	FEing_exit();
	SIclose( df );
	PCexit( FAIL );
    }

    SIprintf( ERx("%s\n\n"), 
	IIUGfmt(dc_msg, MAX_LOC, ERget(S_DC00A2_DESC), 1, dc_tabname) );
    SIflush( stdout );

    if (dc_dblang == DB_SQL)	    /* Emit table description for SQL only */
    {
	IIUGfmt( commbuf, DC_COMMSIZE, ERget(S_DC00A3_COMMENT), 2,
	    dc_tabname, dc_dbname );
	(*dc_lang)( LgenCOMMENT, commbuf );
	(*dc_lang)( LgenMARGIN, NULL );
	SIfprintf( df, ERx("EXEC SQL DECLARE %s TABLE"), dc_tabname );
	(*dc_lang)( LgenCONTINUE, ERx("(") );
    }

    /* Fetch column descriptions, one by one */
    while (FEatt_fetch( &qblk, &atinfo) == OK)
    {
	/* For decimal data type, use precision rather the length in bytes */
	datatype = abs(atinfo.adf_type);
	if (datatype == DB_DEC_TYPE
	    || datatype == DB_LVCH_TYPE
	    || datatype == DB_LBYTE_TYPE
	    || datatype == DB_GEOM_TYPE
	    || datatype == DB_POINT_TYPE
	    || datatype == DB_MPOINT_TYPE
	    || datatype == DB_LINE_TYPE
	    || datatype == DB_MLINE_TYPE
	    || datatype == DB_POLY_TYPE
	    || datatype == DB_MPOLY_TYPE
	    || datatype == DB_GEOMC_TYPE
	    || datatype == DB_LBIT_TYPE)
	    adflength = atinfo.extern_length;
	else
	    adflength = atinfo.intern_length;	
	if (dc_dblang == DB_SQL)
	{
	    if (!first)
	    {
		SIfprintf( df, ERx(",") );
		(*dc_lang)( LgenCONTINUE, NULL );
	    }
	    first = FALSE;
	    rval = dc_adftosq(atinfo.adf_type, adflength,
				(i4) atinfo.scale, sqltype);
	    if (rval != OK)
	    {
		i4	adftype = atinfo.adf_type;
		IIUGfer( stdout, E_DC0005_SQLTYPE, 0, 2,
			&adftype, &adflength );
		FEing_exit();
		SIclose( df );
		PCexit( FAIL );
	    }
	    if (IIUGdlm_ChkdlmBEobject(atinfo.column_name, NULL, TRUE) 
	    	== UI_DELIM_ID)
	    	SIfprintf( df, ERx("\"%s\"\t%s"), atinfo.column_name, sqltype );
	    else
	    	SIfprintf( df, ERx("%s\t%s"), atinfo.column_name, sqltype );
	}
	if (dc_getalias(atinfo.column_name, alias_column) == TRUE)
	    IIUGfer( stdout, E_DC0011_COLNAME, 0, 1, atinfo.column_name );

# if defined(DGC_AOS) || defined(hp9_mpe)
	STcopy (alias_column, temp_name);
	if (STcompare(dc_lname, ERx("cobol")) == 0)
	{
		/*  
		** Change underscore to dash for DG COBOL only.  Underscore
		** is an illegal character. (Also HP3000 Cobol)
		*/
		name_ptr = &temp_name;
		while  (*name_ptr != NULL)
		{
			if (*name_ptr == '_')
				*name_ptr = '-';
			name_ptr++;
		}
	}
	dc_savedesc( temp_name, atinfo.adf_type, adflength, 
		(i4) atinfo.scale );
# else
	dc_savedesc( alias_column, atinfo.adf_type, adflength,
		(i4) atinfo.scale );
# endif /* DGC_AOS hp9_mpe */

	/*
	** Warn that zero-length string buffers have been generated for
	** "unlimited" length datatypes, and will require user editing 
	** before these declarations will compile.
	*/
	if (   !DC000A_already_displayed
	    && (   datatype == DB_LVCH_TYPE 
		|| datatype == DB_LBYTE_TYPE
		|| datatype == DB_LBIT_TYPE
		|| datatype == DB_LNVCHR_TYPE))
	{
	    IIUGfer( stdout, E_DC000A_BADCHRSTR, 0, 1, dc_tabname );
	    DC000A_already_displayed = TRUE;
	}

	/*
	** For non-C languages, warn that the SQLDA must be used for 
	** insert/upate of "byte" types across hetnet.
	*/
	if (   !DC000B_already_displayed
	    && (   datatype == DB_BYTE_TYPE
		|| datatype == DB_VBYTE_TYPE
		|| datatype == DB_LBYTE_TYPE)
	    && (STcompare(dc_lname, ERx("c")) != 0))
	{
	    IIUGfer( stdout, E_DC000B_BYTETYPE, 0, 1, dc_tabname );
	    DC000B_already_displayed = TRUE;
	}

    } /* while */

    if (dc_dblang == DB_SQL)
    {
	SIfprintf( df, ERx(")") );
	(*dc_lang)( LgenTERM, NULL );
	SIfprintf( df, ERx("\n") );
    }
    (*dc_lang)( LgenSTRUCT, NULL );

}

/*{
+*  Name: dc_adftosq - Convert ADF type to SQL type
**
**  Description:
**	Given an ADF type code and length, convert into a character
** 	representation of the equivalent SQL type.
**
**  Inputs:
**	adf_type	- Type code (may be negative)
**	adf_size	- Internal size of adf_type data or Precision for
**			  a decimal data type.
**	adf_scale	- Scale info used for decimal data type.
**
**  Outputs:
**	sqlbuf		- Character string representing SQL type
**	Returns:
**	    OK
**	    FAIL
**	Errors:
**	    None.
-*
**  Side Effects:
**	None
**	
**  History:
**	28-jul-1987 - written (bjb)
**	11-oct-2001 (gupsh01)
**	    Added support for unicode data types.
**	03-aug-2006 (gupsh01)
**	    Added support for ANSI date/time types.
*/
i4
dc_adftosq(adf_type, adf_size, adf_scale, sqlbuf)
i4	adf_type, adf_size, adf_scale;
char	*sqlbuf;
{
    DB_DT_ID	type;

    *sqlbuf = '\0';
    /*
    ** Don't adjust size for a nullable decimal data type as 'adf_size' 
    ** contains precision not number of bytes.
    */	
    type = abs(adf_type);
    if (adf_type <= 0 && (type != DB_DEC_TYPE)
		      && (type != DB_LVCH_TYPE)
		      && (type != DB_LBYTE_TYPE)
		      && (type != DB_LBIT_TYPE))  /* Nullable type? */
	adf_size--;

    switch (abs(adf_type))
    {
      case DB_DTE_TYPE:
	STcopy( ERx("ingresdate"), sqlbuf );
	break;
      case DB_ADTE_TYPE:
	STcopy( ERx("ansidate"), sqlbuf );
	break;
      case DB_TMWO_TYPE:
	STcopy( ERx("time without time zone"), sqlbuf );
	break;
      case DB_TMW_TYPE:
	STcopy( ERx("time with time zone"), sqlbuf );
	break;
      case DB_TME_TYPE:
	STcopy( ERx("time with local time zone"), sqlbuf );
	break;
      case DB_TSWO_TYPE:
	STcopy( ERx("timestamp without time zone"), sqlbuf );
	break;
      case DB_TSW_TYPE:
	STcopy( ERx("timestamp with time zone"), sqlbuf );
	break;
      case DB_TSTMP_TYPE:
	STcopy( ERx("timestamp with local time zone"), sqlbuf );
	break;
      case DB_INYM_TYPE:
	STcopy( ERx("interval year to month"), sqlbuf );
	break;
      case DB_INDS_TYPE:
	STcopy( ERx("interval day to second"), sqlbuf );
	break;
      case DB_MNY_TYPE:
	STcopy( ERx("money"), sqlbuf );
	break;
      case DB_BOO_TYPE:
        STcopy("boolean", sqlbuf);
        break;
      case DB_INT_TYPE:
	switch (adf_size)
	{
	  case 1:
	    STcopy( ERx("integer1"), sqlbuf );
	    break;
	  case 2:
	    STcopy( ERx("smallint"), sqlbuf );
	    break;
	  case 4:
	    STcopy( ERx("integer"), sqlbuf );
	    break;
	  case 8:
	    STcopy( ERx("bigint"), sqlbuf );
	    break;
	}
	break;
      case DB_FLT_TYPE:
	switch (adf_size)
	{
	  case 4:
	    STcopy( ERx("float4"), sqlbuf );
	    break;
	  case 8:
	    STcopy( ERx("float"), sqlbuf );
	    break;
	}
	break;
      case DB_DEC_TYPE:
	STprintf( sqlbuf, ERx("decimal(%d,%d)"), adf_size, adf_scale );
	break;
      case DB_CHA_TYPE:
	STprintf( sqlbuf, ERx("char(%d)"), adf_size );
	break;
      case DB_CHR_TYPE:
	STprintf( sqlbuf, ERx("c%d"), adf_size );
	break;
      case DB_NCHR_TYPE:
	STprintf( sqlbuf, ERx("nchar(%d)"), adf_size/sizeof(UCS2) );
	break;
      case DB_VCH_TYPE:
	STprintf( sqlbuf, ERx("varchar(%d)"), adf_size -DB_CNTSIZE );
	break;
      case DB_NVCHR_TYPE:
	STprintf( sqlbuf, ERx("nvarchar(%d)"), (adf_size -DB_CNTSIZE)/sizeof(UCS2) );
	break;
      case DB_TXT_TYPE:
	STprintf( sqlbuf, ERx("vchar(%d)"), adf_size -DB_CNTSIZE );
	break;
      case DB_LVCH_TYPE:
	STprintf( sqlbuf, ERx("long varchar") );
	break;
      case DB_LNVCHR_TYPE:
	STprintf( sqlbuf, ERx("long nvarchar") );
	break;
      case DB_LBIT_TYPE:
	STprintf( sqlbuf, ERx("long bit") );
	break;
      case DB_BYTE_TYPE:
	STprintf( sqlbuf, ERx("byte(%d)"), adf_size );
	break;
      case DB_VBYTE_TYPE:
	STprintf( sqlbuf, ERx("byte varying(%d)"), adf_size -DB_CNTSIZE );
	break;
      case DB_LBYTE_TYPE:
	STprintf( sqlbuf, ERx("long byte") );
	break;
    }
    if (*sqlbuf == '\0')
	return FAIL;
    if (adf_type > 0)
	STcat( sqlbuf, ERx(" not null") );
    return OK;
}


/*
+* Procedure:	dc_savedesc
** Purpose:	Saves data descriptions of columns of a database table
** Parameters:
**	name	- *char	- name of column
**	atype	- i4	- adt data type of column
**	atsize	- i4	- size of data
**	atscale - i4	- scale of data (used for decimal data type)
** Return Values:
-*	None
** History:
**	03-aug-2006 (gupsh01)
**	    Added support for ANSI date/time types.
*/

void
dc_savedesc( name, atype, atsize, atscale )
char	*name;
i4	atype;
i4	atsize;
i4	atscale;
{
    DC_ATT	*da;

    da = &dc_attab[dc_attcnt++]; 
    STtrmwhite( name );
    STcopy( name, da->atname );
    da->atadftype = abs(atype);
    /*	
    ** Don't adjust size for a nullable decimal data type as 'atsize'
    ** contains precision not number of bytes.
    */
    if (atype < 0 && (da->atadftype != DB_DEC_TYPE)
		  && (da->atadftype != DB_LVCH_TYPE)
		  && (da->atadftype != DB_LBYTE_TYPE)
		  && (da->atadftype != DB_LBIT_TYPE)
		  && (da->atadftype != DB_LNVCHR_TYPE))  /* Nullable type? */
	atsize--;

    switch (da->atadftype)
    {
      case DB_VCH_TYPE:
      case DB_TXT_TYPE:
      case DB_VBYTE_TYPE:
	atsize -= DB_CNTSIZE;
	break;
	  case DB_NVCHR_TYPE:
    atsize = (atsize - DB_CNTSIZE)/sizeof(UCS2);	
	break;
	  case DB_NCHR_TYPE:
	atsize /= sizeof(UCS2);
	break;
      case DB_DTE_TYPE:
	atsize = ADH_EXTLEN_DTE;
        break;
      case DB_ADTE_TYPE:
	atsize = ADH_EXTLEN_ADTE;
        break;
      case DB_TMWO_TYPE:
	atsize = ADH_EXTLEN_TMWO;
        break;
      case DB_TMW_TYPE:
	atsize = ADH_EXTLEN_TMW;
        break;
      case DB_TSWO_TYPE:
	atsize = ADH_EXTLEN_TSWO;
        break;
      case DB_TSTMP_TYPE:
	atsize = ADH_EXTLEN_TSTMP;
        break;
      case DB_INYM_TYPE:
	atsize = ADH_EXTLEN_INTYM;
        break;
      case DB_INDS_TYPE:
	atsize = ADH_EXTLEN_INTDS;
        break;
      case DB_DEC_TYPE:
	/* Store encoded precision and scale for a decimal data type */
	atsize = (i4) DB_PS_ENCODE_MACRO(atsize, atscale);
        break;
      case DB_LVCH_TYPE:
      case DB_LBYTE_TYPE:
      case DB_LBIT_TYPE:
	  case DB_LNVCHR_TYPE:
	atsize = 0;
	break;
    }
    da->atexsize = atsize;
}


/*
+* Procedure:	dc_getargs
** Purpose:	User-friendly processor of command line arguments
** Parameters:
**	argc	- i4	- Command line argument count
**	argv	- **char - Arguments
** Return Values:
-*	None
** Notes:
** 1.  Any arguments preceded by a dash are expected before the
**	five regular arguments.  (Dash arguments may be there
**	if DCLGEN is called from another INGRES frontend).
** 2.  The five regular arguments are given a preliminary validity
**	check and the user is prompted for each one if necessary.
** 3.  If too few arguments are provided, the user is prompted for
**	the remainder.
** 4.  Arguments are put into global variables
** Imports modified:
**	(dc_lang)(), dc_dblang, dc_dbname, dc_tabname,
**	dc_filename, dc_strucname, dc_config
*/

void
dc_getargs( argc, argv )
i4	argc;
char	**argv;
{
    i4    	argcnt = 0;		/* Count number of non-dash args */
    char	*cp;
    char	tempbuf[DC_ARGLEN+1];	/* temp place for some args */
    char	*with_ptr = ERx("");	/* connection parameter pointer */

    argc--;
    argv++;				/* Skip over "dclgen" command */
    while (argc)			/* Get '-' arguments first */
    {
	cp = *argv;
	if (*cp == '-')
	{
	    switch(*(cp+1))
	    {
	        case 'X':
	        case 'x':
	          STlcopy( cp, dc_xflag, MAX_LOC );
	          break;
	        case 'Q':
	        case 'q':
	          dc_dblang = DB_QUEL;
	          dc_pp = ERx("## ");
	          break;
	        case 'A':
	        case 'a':
	          if (STbcompare(cp+2, 0, ERx("vax"), 0, TRUE) == 0)
		      dc_config = DC_ADVAX;
	          else if (STbcompare(cp+2, 0, ERx("vads"), 0, TRUE) == 0)
		      dc_config = DC_ADVADS;
	          else
	          {
		  IIUGfer( stdout, E_DC0006_FLAG, 0, 1, cp );
		      dc_usagerr();
	          }
	          break;
		case 'F':
		case 'f':
		  if (STbcompare(cp+2, 0, ERx("77"), 0, TRUE) == 0)
		  	nostruct = TRUE;	
		  else
		  {
			IIUGfer( stdout, E_DC0006_FLAG, 0, 1, cp );
			dc_usagerr();
		  }
		  break;
	        default:
	          IIUGfer( stdout, E_DC0006_FLAG, 0, 1, cp );
	          dc_usagerr();
	          break;
	    }
	}
	else if (*cp  == '+')
	{
	    switch(*(cp+1))
	    {
	        case 'C':	/* connection parameters */
	        case 'c':
		  /* set up WITH clause for CONNECT */
		  with_ptr = STalloc(cp + 2);
		  IIUIswc_SetWithClause(with_ptr);
		  break;

	        default:
	          IIUGfer( stdout, E_DC0006_FLAG, 0, 1, cp );
	          dc_usagerr();
	          break;
	    }
	}
	else
	{
	    break;
	}
	argc--;
	argv++;
    }

# ifdef DGC_AOS
    if (*with_ptr == EOS)
    {
    	with_ptr = STalloc(ERx("dgc_mode='reading'");
    	IIUIswc_SetWithClause(with_ptr);
    }
# endif

    /*
    ** Do a preliminary check on validity of "regular" arguments
    */
    while (argc--)
    {
	cp = *argv++;
	argcnt++;
	switch (argcnt)
	{
	    case 1:
		if (!dc_chklang(cp))
		    dc_getlang();
		break;

	    case 2:			/* Database name */
		STcopy( cp, tempbuf );
		/* 
		** FE will pass database name as entered.  Will not do any
		** prior checking.  If any error, will get a DBMS error, not FE
		** error..
		*/
		if (*tempbuf == EOS)   
		    dc_getdbname( tempbuf );
		STcopy( tempbuf, dc_dbname );
		break;

	    case 3:			/* Table name */
		STcopy ( cp, tempbuf );
		if (*tempbuf == EOS)   
		    dc_gettabname( tempbuf );
		STcopy( tempbuf, dc_tabname );
		break;

	    case 4:
		STcopy( cp, dc_filename );
		break;

	    case 5:
		STcopy( cp, dc_strucname );
		STtrmwhite( dc_strucname );
		break;

	    default:
		break;
	}

    }
    if (argcnt > 5) {
	IIUGfer( stdout, E_DC0007_MANYARGS, 0, 1, &argcnt );
	dc_usagerr();
    }

    /* 
    ** Now prompt for any arguments that are missing
    */
    switch (argcnt)
    {
	case 0:
	    dc_getlang();

	case 1:
	    dc_getdbname( tempbuf );
	    STcopy( tempbuf, dc_dbname );

	case 2:
	    dc_gettabname( tempbuf );
	    STcopy( tempbuf, dc_tabname );

	case 3:
	    dc_getfilename( dc_filename );

	case 4:
	    dc_getstrucname( dc_strucname );
	    STtrmwhite( dc_strucname );
	    break;
    }
}

/*
+* Procedure:	dc_getlang
** Purpose:	Prompt user for valid language for emitting of data structure
** Parameters:
**	None
** Return Values:
-*	None
** Notes:
** 1.	dc_chklang() is called which will cause (dc_lang)() to be set.
*/

void
dc_getlang()
{
    char	buf[20];

    do {
	if (FEprompt(ERget(S_DC00A6_ARGLANG), TRUE, 20, buf) != OK)
	    PCexit( FAIL );
    } while (dc_chklang(buf) == FALSE);
}

/*
+* Procedure:	dc_getdbname
** Purpose:	Prompt user for "reasonable" database name
** Parameters:
**	dbname	- *char	- To hold database name obtained from user
** Return Values:
-*	None
*/

void
dc_getdbname( dbname )
char	*dbname;
{
    do {  
	if (FEprompt(ERget(S_DC00A7_ARGDB), TRUE, DC_ARGLEN, dbname) != OK)
	    PCexit( FAIL );
    } while (dbname == NULL);  
}


/*
+* Procedure:	dc_gettabname
** Purpose:	Prompt user for "reasonable" table name
** Parameters:
**	tabname - *char - to hold table name obtained from user
** Return Values:
-*	None
*/

void
dc_gettabname( tabname )
char	*tabname;
{
    do {
	if (FEprompt(ERget(S_DC00A8_ARGTABLE), TRUE, DC_ARGLEN, tabname) != OK)
	    PCexit( FAIL );
    } while (tabname == NULL);
}

/*
+* Procedure:	dc_getfilename
** Purpose:	Prompt user for file name
** Parameters:
**	filenam	- *char	- To hold file name
** Return Values:
-*	None
*/

void
dc_getfilename( filename )
char	*filename;
{
    if (FEprompt(ERget(S_DC00A9_ARGFILE), TRUE, MAX_LOC, filename) != OK)
	PCexit( FAIL );
}

/*
+* Procedure:	dc_usagerr
** Purpose:	Upon a usage error (eg. too many arguments or incorrect 
**		flags), dc_usagerr is called to output a correct usage
**		message and terminate program execution.
** Parameters:
**	None
** Return Values:
-*	None
*/

void
dc_usagerr()
{
	SIprintf( ERx("%s\n"),
	  IIUGfmt(dc_msg, MAX_LOC, ERget(S_DC00A4_USAGE), 0) );
	SIprintf( ERx("%\t%s\n"), 
	  IIUGfmt(dc_msg, MAX_LOC, ERget(S_DC00A5_ARGS), 0) );
	PCexit( FAIL );
}


/*
+* Procedure:	dc_getstrucname
** Purpose:	Prompt user for structure name
** Parameters:
**	filenam	- *char	- To hold structure name
** Return Values:
-*	None
** Notes:	For non-VMS FORTRAN users, prompt for a prefix name
**		instead of a structure name.  See notes at top of file.
*/

void
dc_getstrucname( strucname )
char 	*strucname;
{
# ifndef VMS
    if (nostruct)
    {
        if (FEprompt(ERget(S_DC00AC_ARGPREFIX), TRUE, DC_ARGLEN, strucname) 
	    	          != OK)
	    PCexit( FAIL );
    }
    else
# endif /* VMS */
    if (FEprompt(ERget(S_DC00AA_ARGSTRUCT), TRUE, DC_ARGLEN, strucname) != OK)
	PCexit( FAIL );
}

/*
+* Procedure:	dc_namechk
** Purpose:	Checks database/table name string for validity
** Parameters:
**	objname	- *char	- Database name to be checked
** Return Values:
-*	TRUE: object name is reasonable; FALSE otherwise
*/

bool
dc_namechk( objname )
char	*objname;
{

    if (FEchkname(objname) != OK)
    {
	IIUGfer( stdout, E_DC0008_BADNAME, 0, 1, objname );
	return FALSE;
    }
    return TRUE;
}


/*
+* Procedure:	dc_chklang
** Purpose:	Check host language being used.
** Parameters:
**	lang	- char * - String of users language input.
** Return Values:
-*	TRUE if language was found and is supported.
**
** Imports modified:
**	Points global dc_lang to specific function.
** History:
**    10-aug-1993 (kathryn)   Bug 54046
**	  Changed ln_sys for basic to L_ALL. Dclgen will generate correct output
**	  for each language regardless what machine it is run on.
**    19-apr-1994 (teresal)
**	  Added 'cc' as a language option for C++.  Note: for now 'cc'
**	  will generate the same thing as 'c' does.
*/

bool
dc_chklang( lang )
char	*lang;
{
    static struct _langs
    {
	char	*lm_name;	/* Language name */
	i4	lm_uniq;	/* Required unique letters */
	i4	lm_sys;		/* On which system */
	void (*lm_func)();	/* Language dependent routine */
    } lang_map[] = { 
	{ERx("ada"),		1,	L_ALL,	dc_ada},
	{ERx("basic"),		1,	L_ALL,	dc_basic},
	{ERx("c"),		0,	L_ALL,	dc_c},
	{ERx("cc"),		0,	L_ALL,	dc_c},
	{ERx("cobol"),		2,	L_ALL,	dc_cobol},
	{ERx("fortran"),	1,	L_ALL,	dc_fortran},
	{ERx("pascal"),		2,	L_ALL,	dc_pascal},
	{(char *)0,		0,	0,	0}
    };
    register struct _langs	*l;

    for (l = lang_map; 
	 l->lm_name && STbcompare(lang, l->lm_uniq, l->lm_name, 0, TRUE) != 0;
	 l++)
	 ;
    if (l->lm_name == (char *)0 || !l->lm_sys)
    {
	IIUGfer( stdout, E_DC0009_BADLANG, 0, 1, lang );
	return FALSE;
    }
    dc_lang = l->lm_func;
    STcopy (l->lm_name, dc_lname);
    return TRUE;
}

/*
+* Procedure:	dc_getalias
** Purpose:	Checks if name contains invalid characters, and
**		replaces all invalid characters
** Parameters:
**	in	- *char	- name to be checked for invalid chars
**	out	- *char - string with possibly replaced characters
** Return Values:
-*	TRUE: invalid chars were replaced; FALSE: no invalid chars found
*/

bool
dc_getalias( in, out )
char	*in;
char	*out;
{

    i4  i;
    bool replaced = FALSE;

    for (i = 0; i < DB_MAXNAME && *in != EOS; CMnext(in))
    {
	CMbyteinc(i, in);

	if (i <= DB_MAXNAME)
	{
	    if (CMnmchar(in))
	    {
		CMcpychar(in, out);
		CMnext(out);
	    }
	    else
	    {
		replaced = TRUE;
		CMcpychar(valid_char, out);
		CMnext(out);
	    }
	}
	else 
	    break;
    }
    *out = EOS;

    return replaced;
}

/*
+* Procedure:	dc_lang
** Purpose:	Language dependent code generators.
** Defines:	dc_ada, dc_basic, dc_c, dc_cobol, dc_fortran, dc_pascal
**		all of which accept the same arguments.
** Parameters:
**	flag	- i4	- What to generate
**	str	- char *- Possible string to dump too.
** Return Values:
-*	None
** Notes:
**	Possible entry points are:
**	 LgenCOMMENT	- Comment before SQL statement,
**	 LgenMARGIN	- Prefix SQL statement with margin,
**	 LgenCONTINUE	- Continue an SQL statement according to ESQL rules,
**	 LgenTERM	- Statement terminator,
**	 LgenSTRUCT	- Dump host structure.
** History:
**	03-aug-2006 (gupsh01)
**	    Added support for ANSI date/time types.
*/


void
dc_ada( flag, str )
i4	flag;
char	*str;
{
    register	DC_ATT	*da;
    register	FILE	*df = dc_outf;
    i4			i;
    i4			len;
    char		*ada_i1_str;
    char		*ada_f4_str;
    char		*ada_f8_str;

    if (dc_config & DC_ADVAX)
    {
	ada_i1_str = ERx("short_short_integer");
	ada_f4_str = ERx("float");
	ada_f8_str = ERx("long_float");
    }
    else
    {
#if defined(axp_osf)
        ada_i1_str = ERx("short_short_integer");
        ada_f4_str = ERx("float");
        ada_f8_str = ERx("long_float");
#else /* axp_osf */
	ada_i1_str = ERx("tiny_integer");
	ada_f4_str = ERx("short_float");
	ada_f8_str = ERx("float");
#endif /* axp_osf */
    }

    switch (flag)
    {
      case LgenCOMMENT:
	SIfprintf( df, ERx("-- %s\n"), str );
	return;
      case LgenMARGIN:
	SIfprintf( df, ERx("  ") );
	return;
      case LgenCONTINUE:
	if (str)
	    SIfprintf( df, ERx("\n\t%s"), str );
	else
	    SIfprintf( df, ERx("\n\t ") );
	return;
      case LgenTERM:
	SIfprintf( df, ERx(";\n") );
	return;
      case LgenSTRUCT:
	/* Record head */
	SIfprintf( df, ERx("%stype %s_rec is\n"), dc_pp, dc_strucname );
	SIfprintf( df, ERx("%s\trecord\n"), dc_pp );
	for (i = 0, da = dc_attab; i < dc_attcnt; i++, da++)
	{
	    SIfprintf( df, ERx("%s\t\t%s:"), dc_pp, da->atname );
	    for (len = DB_GW1_MAXNAME - STlength(da->atname) +1; len > 0; len--)
		SIfprintf( df, ERx(" ") );
	    switch (da->atadftype)
	    {
              case DB_BOO_TYPE:
                SIfprintf(df, "Boolean");
                break;
	      case DB_INT_TYPE:
		if (da->atexsize == sizeof(i4))
		    SIfprintf( df, ERx("integer"));
		else if (da->atexsize == sizeof(i2))
		    SIfprintf( df, ERx("short_integer") );
		else if (da->atexsize == sizeof(i8))
		    SIfprintf( df, ERx("long_long") );
		else
		    SIfprintf( df, ada_i1_str );
		break;
	      case DB_FLT_TYPE:
		if (da->atexsize == sizeof(f8))
		    SIfprintf( df, ada_f8_str );
		else
		    SIfprintf( df, ada_f4_str );
		break;
	      case DB_CHR_TYPE:
	      case DB_CHA_TYPE:
	      case DB_TXT_TYPE:
	      case DB_VCH_TYPE:
	      case DB_DTE_TYPE:
	      case DB_ADTE_TYPE:
	      case DB_TMWO_TYPE:
	      case DB_TMW_TYPE:
	      case DB_TME_TYPE:
	      case DB_TSWO_TYPE:
	      case DB_TSW_TYPE:
	      case DB_TSTMP_TYPE:
	      case DB_INDS_TYPE:
	      case DB_INYM_TYPE:
	      case DB_BYTE_TYPE:
	      case DB_VBYTE_TYPE:
	      case DB_LBYTE_TYPE:
	      case DB_LVCH_TYPE:
	      case DB_LBIT_TYPE:
		SIfprintf( df, ERx("string(1..%d)"), da->atexsize );
		break;
	      case DB_DEC_TYPE:
	      case DB_MNY_TYPE:
		SIfprintf( df, ada_f8_str );
		break;
	    }
	    SIfprintf( df, ERx(";\n") );
	}
	SIfprintf( df, ERx("%s\tend record;\n"), dc_pp );
	SIfprintf( df, ERx("%s%s: %s_rec;\n"),dc_pp,dc_strucname,dc_strucname);
	return;
    }
}

void
dc_basic( flag, str )
i4	flag;
char	*str;
{
    register	DC_ATT	*da;
    register	FILE	*df = dc_outf;
    i4			i;
    i4  		prec, scale;
    bool	isstring;

    switch (flag)
    {
      case LgenCOMMENT:
	SIfprintf( df, ERx("\t! %s\n"), str );
	return;
      case LgenMARGIN:
	SIfprintf( df, ERx("\t  ") );
	return;
      case LgenCONTINUE:
	if (str)
	    SIfprintf( df, ERx("\t&\n\t\t%s"), str );
	else
	    SIfprintf( df, ERx("\t&\n\t\t") );
	return;
      case LgenTERM:
	SIfprintf( df, ERx("\n") );
	return;
      case LgenSTRUCT:
	/* Structure head -- a tag */
	SIfprintf( df, ERx("%s\t  record %s_\n"), dc_pp, dc_strucname );
	for (i = 0, da = dc_attab; i < dc_attcnt; i++, da++)
	{
	    isstring = FALSE;
	    SIfprintf( df, ERx("%s\t\t"), dc_pp );
	    switch (da->atadftype)
	    {
              case DB_BOO_TYPE:
                SIfprintf(df, "byte");
                break;
	      case DB_INT_TYPE:
		if (da->atexsize == sizeof(i4))
		    SIfprintf( df, ERx("long") );
		else if (da->atexsize == sizeof(i2))
		    SIfprintf( df, ERx("word") );
		else
		    SIfprintf( df, ERx("byte") );
		break;
	      case DB_FLT_TYPE:
		if (da->atexsize == sizeof(double))
#if defined(VMS)
                    SIfprintf( df, ERx("tfloat") );
                else
                    SIfprintf( df, ERx("sfloat") );
#else
		    SIfprintf( df, ERx("double") );
		else
		    SIfprintf( df, ERx("single") );
#endif
		break;
	      case DB_CHR_TYPE:
	      case DB_CHA_TYPE:
	      case DB_TXT_TYPE:
	      case DB_VCH_TYPE:
	      case DB_DTE_TYPE:
	      case DB_ADTE_TYPE:
	      case DB_TMWO_TYPE:
	      case DB_TMW_TYPE:
	      case DB_TME_TYPE:
	      case DB_TSWO_TYPE:
	      case DB_TSW_TYPE:
	      case DB_TSTMP_TYPE:
	      case DB_INDS_TYPE:
	      case DB_INYM_TYPE:
	      case DB_BYTE_TYPE:
	      case DB_VBYTE_TYPE:
	      case DB_LBYTE_TYPE:
	      case DB_LVCH_TYPE:
	      case DB_LBIT_TYPE:
		SIfprintf( df, ERx("string") );
		isstring = TRUE;
		break;
	      case DB_MNY_TYPE:
#if defined(VMS)
                SIfprintf( df, ERx("tfloat") );
#else
		SIfprintf( df, ERx("double") );
#endif
		break;
	      case DB_DEC_TYPE:
		prec = (i4) DB_P_DECODE_MACRO((i2) da->atexsize);
		scale = (i4) DB_S_DECODE_MACRO((i2) da->atexsize);
		SIfprintf( df, ERx("decimal(%d,%d)"), prec, scale );
		break;
 	    }
	    SIfprintf( df, ERx("\t%s"), da->atname );
	    if (isstring)
		SIfprintf( df, ERx(" = %d"), da->atexsize );
	    SIfprintf( df, ERx("\n") );
	}
	SIfprintf( df, ERx("%s\t  end record %s_\n"), dc_pp, dc_strucname );
	SIfprintf( df, ERx("%s\t  declare %s_ %s\n"), dc_pp, dc_strucname, 
	    dc_strucname );
	return;
    }
}

void
dc_c( flag, str )
i4	flag;
char	*str;
{
    register	DC_ATT	*da;
    register	FILE	*df = dc_outf;
    i4			i;
    bool		isstring;

    switch (flag)
    {
      case LgenCOMMENT:
	SIfprintf( df, ERx("/* %s */\n"), str );
	return;
      case LgenMARGIN:
	SIfprintf( df, ERx("  ") );
	return;
      case LgenCONTINUE:
	if (str)
	    SIfprintf( df, ERx("\n\t%s"), str );
	else
	    SIfprintf( df, ERx("\n\t ") );
	return;
      case LgenTERM:
	SIfprintf( df, ERx(";\n") );
	return;
      case LgenSTRUCT:
	/* Structure head -- a tag */
	SIfprintf( df, ERx("%s  struct %s_ {\n"), dc_pp, dc_strucname );
	for (i = 0, da = dc_attab; i < dc_attcnt; i++, da++)
	{
	    isstring = FALSE;
	    SIfprintf( df, ERx("%s\t"), dc_pp );
	    switch (da->atadftype)
	    {
              case DB_BOO_TYPE:
                if (STcompare(dc_lname, "cc") == 0)
                    SIfprintf(df, "bool");
                else
                    SIfprintf(df, "char");
                break;
	      case DB_INT_TYPE:
		if (da->atexsize == sizeof(i4))
		    /* 
		    ** Determine whether i4's should really be defined as 
		    ** either native int or long types on this platform.
		    */
		    if (sizeof(i4) == sizeof(int))
		        SIfprintf( df, ERx("int") );
		    else
		        SIfprintf( df, ERx("long") );
		else if(da->atexsize == sizeof(i8))
			# ifdef LP64
				# ifdef WIN64
					SIfprintf( df, ERx("LONG64") );
				# else
					SIfprintf( df, ERx("long") );
				# endif /* WIN64 */
			# else  /* LP64 */
				# ifdef WIN32
					SIfprintf( df, ERx("__int64") );
				# else
                                        #if defined(VMS)
						SIfprintf( df, ERx("__int64") );
					# else
						SIfprintf( df, ERx("long long") );
					#endif
				# endif /* WIN64 */
			# endif /* LP64 */
		else
		    SIfprintf( df, ERx("short") );
		break;
	      case DB_FLT_TYPE:
		if (da->atexsize == sizeof(double))
		    SIfprintf( df, ERx("double") );
		else
		    SIfprintf( df, ERx("float") );
		break;
	      case DB_CHR_TYPE:
	      case DB_CHA_TYPE:
	      case DB_TXT_TYPE:
	      case DB_VCH_TYPE:
              case DB_DTE_TYPE:
	      case DB_ADTE_TYPE:
	      case DB_TMWO_TYPE:
	      case DB_TMW_TYPE:
	      case DB_TME_TYPE:
	      case DB_TSWO_TYPE:
	      case DB_TSW_TYPE:
	      case DB_TSTMP_TYPE:
	      case DB_INDS_TYPE:
	      case DB_INYM_TYPE:
		SIfprintf( df, ERx("char") );
		isstring = TRUE;
		break;
              case DB_NCHR_TYPE:
		  case DB_NVCHR_TYPE:
		SIfprintf( df, ERx("wchar_t") );
		isstring = TRUE;
		break;
	      case DB_LVCH_TYPE:
	      case DB_LBIT_TYPE:		  
		SIfprintf( df, ERx("char") );
		da->atexsize--;
		isstring = TRUE;
		break;
	      case DB_LNVCHR_TYPE:
		SIfprintf( df, ERx("wchar_t") );
		da->atexsize--;
		isstring = TRUE;
		break;
	      case DB_DEC_TYPE:
	      case DB_MNY_TYPE:
		SIfprintf( df, ERx("double") );
		break;
	      case DB_BYTE_TYPE:
	      case DB_LBYTE_TYPE:
	      case DB_VBYTE_TYPE:
		SIfprintf( df, ERx("varbyte struct {\n") );
		SIfprintf( df, ERx("  \t\t\tshort\tlen;\n") );
		SIfprintf( df, ERx("  \t\t\tchar\tbuf[%d];\n"), da->atexsize );
		SIfprintf( df, ERx("  \t} ") );
		break;
	    }
	    SIfprintf( df, ERx("\t%s"), da->atname );
	    if (isstring == TRUE)
		SIfprintf( df, ERx("[%d]"), da->atexsize+1 );
	    SIfprintf( df, ERx(";\n") );
	}
	/* Structure tail */
	SIfprintf( df, ERx("%s  } %s;\n"), dc_pp, dc_strucname );
	return;
    }
}

void
dc_cobol( flag, str )
i4	flag;
char	*str;
{
    register	DC_ATT	*da;
    register	FILE	*df = dc_outf;
    i4			i;
    i4  		prec, scale;
#ifdef UNIX
    char		*cp;	/* Ptr for converting underscores to dashes */
#endif /* UNIX */
# ifndef VMS
		/*	  12345678901	*/
    char        *cob_s = ERx("      ");           /* Sequence area (col 1-6)*/
    char     	*cob_a = ERx("       ");          /* Area A (column 8-11) */
    char     	*cob_b = ERx("           ");      /* Area B (column 12 up) */
# ifdef DGC_AOS		
    char	*cob_c = ERx("*");	  	  /* DG Comment indicator(col 1) */
# else 
    char	*cob_c = ERx("      *");	  /* Comment indicator(col 7) */
# endif /*DGC_AOS */
# else
    char	*cob_s = ERx("");
    char	*cob_a = ERx("\t");
    char	*cob_b = ERx("\t  ");
    char	*cob_c = ERx("*     *");	  /* VMS comments(ansi/trm'l) */
# endif /* VMS */

     
    switch (flag)
    {
      case LgenCOMMENT:
	SIfprintf( df, ERx("%s %s\n"), cob_c, str );
	return;
      case LgenMARGIN:
	SIfprintf( df, ERx("%s"), cob_b );
	return;
      case LgenCONTINUE:
	if (str)
	    SIfprintf( df, ERx("\n%s  %s"), cob_b, str );
	else
	    SIfprintf( df, ERx("\n%s   "), cob_b );
	return;
      case LgenTERM:
	SIfprintf( df, ERx("\n%sEND-EXEC.\n"), cob_b );
	return;
      case LgenSTRUCT:
	/* Structure head */
	CVupper( dc_strucname );
	SIfprintf( df, ERx("%s%s01 %s.\n"), dc_pp, cob_a, dc_strucname );
	for (i = 0, da = dc_attab; i < dc_attcnt; i++, da++)
	{
	    CVupper( da->atname );
#ifdef UNIX
	    for(cp = da->atname; *cp; CMnext(cp))
	    {
		if (*cp == '_')
		    *cp = '-';
	    }
#endif /* UNIX */
	    SIfprintf( df, ERx("%s%s02  %s  "), dc_pp, cob_b, da->atname );
	    switch (da->atadftype)
	    {
              case DB_BOO_TYPE:
                SIfprintf(df, "PICTURE 9");
                break;
	      case DB_INT_TYPE:
		SIfprintf( df, ERx("PICTURE ") );
		if (da->atexsize == sizeof(i4))
		    SIfprintf( df, ERx("S9(9)") );
		else if (da->atexsize == sizeof(i8))
		    SIfprintf( df, ERx("S9(18)") );
		else
# ifdef UNIX
		    SIfprintf( df, ERx("S9(5)") );
# else
		    SIfprintf( df, ERx("S9(4)") );
# endif /* UNIX */
		SIfprintf( df, ERx(" USAGE COMP") );
		break;
	      case DB_FLT_TYPE:
# ifndef COB_FLOATS
		SIfprintf( df, "PICTURE S9(10)V9(8) USAGE COMP-3" );
# else
		 
		SIfprintf( df, ERx("USAGE ") );
		if (da->atexsize == sizeof(f8))
		    SIfprintf( df, ERx("COMP-2") );
		else
		    SIfprintf( df, ERx("COMP-1") );
# endif /* COB_FLOATS */
		break;
	      case DB_CHR_TYPE:
	      case DB_CHA_TYPE:
	      case DB_TXT_TYPE:
	      case DB_VCH_TYPE:
	      case DB_DTE_TYPE:
	      case DB_ADTE_TYPE:
	      case DB_TMWO_TYPE:
	      case DB_TMW_TYPE:
	      case DB_TME_TYPE:
	      case DB_TSWO_TYPE:
	      case DB_TSW_TYPE:
	      case DB_TSTMP_TYPE:
	      case DB_INDS_TYPE:
	      case DB_INYM_TYPE:
	      case DB_BYTE_TYPE:
	      case DB_VBYTE_TYPE:
	      case DB_LBYTE_TYPE:
	      case DB_LVCH_TYPE:
	      case DB_LBIT_TYPE:
		SIfprintf( df, ERx("PICTURE X(%d)"), da->atexsize );
		break;
	      case DB_MNY_TYPE:
# ifndef COB_FLOATS
		SIfprintf( df, "PICTURE S9(10)V9(8) USAGE COMP-3" );
# else
		SIfprintf( df, ERx("USAGE COMP-2") );
# endif /* COB_FLOATS */
		break;
	      case DB_DEC_TYPE:
		prec = (i4) DB_P_DECODE_MACRO((i2) da->atexsize);
		scale = (i4) DB_S_DECODE_MACRO((i2) da->atexsize);
		/*
		** Maximum precision for COBOL is 18, notify the user and
		** use a default size (18,8) if precision > 18
		*/
		if (prec > 18)
		{
		    IIUGfer( stdout, E_DC0010_BADCOBPREC, 0, 2, &prec, 
				da->atname );
		    prec = 18;
		    scale = 8;	
		}
		prec -= scale;	/* Number of digits to left of the decimal */
		if (scale == 0)
		    SIfprintf( df, ERx("PICTURE S9(%d) USAGE COMP-3"), prec );
		else if (prec == 0)
		    SIfprintf( df, ERx("PICTURE SV9(%d) USAGE COMP-3"), scale );
		else
		    SIfprintf( df, ERx("PICTURE S9(%d)V9(%d) USAGE COMP-3"),
				prec, scale);
		break;
	    }
	    SIfprintf( df, ERx(".\n") );
	}
	return;
    }
}

void
dc_fortran( flag, str )
i4	flag;
char	*str;
{
    register	DC_ATT	*da;
    register	FILE	*df = dc_outf;
    i4			i;

# ifdef UNIX
    char		*cont_str = "     1";
# else
    char		*cont_str = "\t1";
# endif /* UNIX */

    switch (flag)
    {
      case LgenCOMMENT:
	SIfprintf( df, ERx("C %s\n"), str );
	return;
      case LgenMARGIN:
	SIfprintf( df, ERx("\t") );
	return;
      case LgenCONTINUE:
	if (str)
	    SIfprintf( df, ERx("\n%s %s"), cont_str, str );
	else
	    SIfprintf( df, ERx("\n%s  "), cont_str );
	return;
      case LgenTERM:
	SIfprintf( df, ERx("\n") );
	return;
      case LgenSTRUCT:
# ifndef VMS
	if (nostruct)
	    ;
	else
# endif /* VMS */
	/* Structure head */
	    SIfprintf( df, ERx("%s\tstructure /%s_/\n"), dc_pp, dc_strucname );	
	for (i = 0, da = dc_attab; i < dc_attcnt; i++, da++)
	{
# ifndef VMS
	    if (nostruct)
		SIfprintf( df, ERx("%s\t"), dc_pp );
	    else
# endif /* VMS */
		SIfprintf( df, ERx("%s\t\t"), dc_pp );
	    switch (da->atadftype)
	    {
              case DB_BOO_TYPE:
                SIfprintf(df, "logical");
                break;
	      case DB_INT_TYPE:
		if (da->atexsize == sizeof(i4))
		    SIfprintf( df, ERx("integer*4") );
		else if (da->atexsize == sizeof(i8))
		    SIfprintf( df, ERx("integer*8") );
		else
		    SIfprintf( df, ERx("integer*2") );
		break;
	      case DB_FLT_TYPE:
		if (da->atexsize == sizeof(f8))
		    SIfprintf( df, ERx("real*8") );
		else
		    SIfprintf( df, ERx("real*4") );
		break;
	      case DB_CHR_TYPE:
	      case DB_CHA_TYPE:
	      case DB_TXT_TYPE:
	      case DB_VCH_TYPE:
	      case DB_DTE_TYPE:
	      case DB_ADTE_TYPE:
	      case DB_TMWO_TYPE:
	      case DB_TMW_TYPE:
	      case DB_TME_TYPE:
	      case DB_TSWO_TYPE:
	      case DB_TSW_TYPE:
	      case DB_TSTMP_TYPE:
	      case DB_INDS_TYPE:
	      case DB_INYM_TYPE:
	      case DB_BYTE_TYPE:
	      case DB_VBYTE_TYPE:
	      case DB_LBYTE_TYPE:
	      case DB_LVCH_TYPE:
	      case DB_LBIT_TYPE:
		SIfprintf( df, ERx("character*%d"), da->atexsize );
		break;
	      case DB_DEC_TYPE:
	      case DB_MNY_TYPE:
		SIfprintf( df, ERx("real*8") );
		break;
	    }
# ifndef VMS
	    if (nostruct)		/* Use "structname" as a prefix */
		SIfprintf( df, ERx("\t%s%s\n"), dc_strucname, da->atname );
	    else
# endif /* VMS */
		SIfprintf( df, ERx("\t%s\n"), da->atname );
	}
	/* Structure tail */
# ifndef VMS
	if (!nostruct)
	{
# endif /* VMS */
	    SIfprintf( df, ERx("%s\tend structure\n"), dc_pp );
	    SIfprintf( df, ERx("%s\trecord /%s_/ %s\n"), dc_pp, dc_strucname, 
		dc_strucname );	
# ifndef VMS
	}
# endif /* VMS */
	break;
    }
    return;
}

void
dc_pascal( flag, str )
i4	flag;
char	*str;
{
    register	DC_ATT	*da;
    register	FILE	*df = dc_outf;
    i4			i;

    switch (flag)
    {
      case LgenCOMMENT:
	SIfprintf( df, ERx("{%s}\n"), str );
	return;
      case LgenMARGIN:
	SIfprintf( df, ERx("  ") );
	return;
      case LgenCONTINUE:
	if (str)
	    SIfprintf( df, ERx("\n\t%s"), str );
	else
	    SIfprintf( df, ERx("\n\t ") );
	return;
      case LgenTERM:
	SIfprintf( df, ERx(";\n") );
	return;
      case LgenSTRUCT:
	/* Structure head */
	SIfprintf( df, ERx("%stype %s_rec = record\n"), dc_pp, dc_strucname );
	for (i = 0, da = dc_attab; i < dc_attcnt; i++, da++)
	{
	    SIfprintf( df, ERx("%s\t%s : "), dc_pp, da->atname );
	    switch (da->atadftype)
	    {
              case DB_BOO_TYPE:
                SIfprintf(df, "Boolean");
                break;
	      case DB_INT_TYPE:
		if (da->atexsize == sizeof(i4))
		    SIfprintf( df, ERx("Integer") );
		else if (da->atexsize == sizeof(i2))
		    SIfprintf( df, ERx("[word] %d..%d"), MINI2, MAXI2 );
		else if (da->atexsize == sizeof(i8))
		    SIfprintf( df, ERx("Comp") );
		else
		    SIfprintf( df, ERx("[byte] %d..%d"), MINI1, MAXI1 );
		break;
	      case DB_FLT_TYPE:
		if (da->atexsize == sizeof(f8))
		    SIfprintf( df, ERx("Double") );
		else
		    SIfprintf( df, ERx("Real") );
		break;
	      case DB_BYTE_TYPE:
	      case DB_CHR_TYPE:
	      case DB_CHA_TYPE:
	      case DB_DTE_TYPE:
	      case DB_ADTE_TYPE:
	      case DB_TMWO_TYPE:
	      case DB_TMW_TYPE:
	      case DB_TME_TYPE:
	      case DB_TSWO_TYPE:
	      case DB_TSW_TYPE:
	      case DB_TSTMP_TYPE:
	      case DB_INDS_TYPE:
	      case DB_INYM_TYPE:
		SIfprintf( df, ERx("packed array[1..") );
		SIfprintf( df, ERx("%d] of Char"), da->atexsize );
		break;
	      case DB_TXT_TYPE:
	      case DB_VCH_TYPE:
	      case DB_VBYTE_TYPE:
	      case DB_LBYTE_TYPE:
	      case DB_LVCH_TYPE:
	      case DB_LBIT_TYPE:
		SIfprintf( df, ERx("varying[") );
		SIfprintf( df, ERx("%d] of Char"), da->atexsize );
		break;
	      case DB_DEC_TYPE:
	      case DB_MNY_TYPE:
		SIfprintf( df, ERx("Double") );
		break;
	    }
	    SIfprintf( df, ERx(";\n") );
	}
	SIfprintf( df, ERx("%send;\n"), dc_pp );
	SIfprintf( df, ERx("%svar %s : %s_rec;\n"),
	    dc_pp, dc_strucname, dc_strucname );
	return;
    }
}

/*
** Procedure:	ex_handle
** Purpose:	Clean things up after an exception has occurred
** Parameters:
**	ex	- *EX_ARGS	- Info on exception
** Return Values:
**	EXDECLARE
** History:
**	29-nov-91 (seg)
**	    ex_handle needs to return STATUS, not void.  Also, leave
**	    logic for exiting in calling func.
*/

static STATUS
ex_handle( ex )
EX_ARGS	*ex;
{
    if (ex->exarg_num == EXINTR )
    {
	SIprintf( ERx("%s\n"), ERget(S_DC00AB_ABORT) );
	SIflush( stdout );
    }
    return EXDECLARE;
}
