# include	<compat.h>
# include	<er.h>
# include	<si.h>
# include	<cm.h>
# include	<lo.h>
# include	<nm.h>
# include	<st.h>
# include	<equtils.h>
# include	<equel.h>
# include	<eqscan.h>
# include	<eqlang.h>
# include	<eqsym.h>
# include	<eqgen.h>
# include	<me.h>
# include	<ereq.h>

/*
+* Filename:	SCINCLUDE.C
** Purpose:	Routines to process Equel 'include' lines.
**
** Defines:	inc_tst_include()	- Test if this is an include line.
**		inc_push_file()		- Include the file.
**		inc_is_nested()		- Are there open include files.
**		inc_pop_file()		- Pop current include file.
**		inc_dump()		- Dump information about include files.
**		inc_parse_name()	- Parse a file name for SQL.
**		inc_inline()		- Force a file to be INLINE. (ESQLA)
**		(*inc_pass2)()		- Entry point to pass 2 EQUEL's (EQP).
**		inc_save()		- Save original parent location.
**		inc_add_inc_dir()	- Add an include ("-h") directory.
** Locals:
**		inc_fileparse()		- Parse a file name into pieces.
**		inc_expand_include()	- Expand included filenames
-*		inc_isdir()		- Is a location a directory?
** Notes:
**  1. Because we support regular and inline inclusion of files, both the
**     routines that open and close the include files must know if the files
**     were inline included or not.
**  2. If we preprocess the same file twice, we do not open the second time
**     for writing - just reading.  We require that INC_INLINE and INC_NOWRITE
**     be mutually exclusive; INLINE overrides NOWRITE and always produces
**     output.
**  3. ADA vs non-ADA - ADA has some problems because it does not have an
**     include facility.  Consequently the following rules are followed:
**
**				           non-ADA		ADA
**			     	       +----------------+---------------
** ## INCLUDE 			       | host_include   | WITH package
** ## INCLUDE INLINE 		       | inline include | inline include
** SQL INCLUDE not in DECLARE SECTION  | host_include   | WITH package
** SQL INCLUDE in DECLARE SECTION      | host_include   | inline include
**
**  4. We support global includes, using <> delimiters.  In eqc and esqlc
**     with the "-c" flag we generate global includes, using <> delimiters.
**     This is the normal case for RT internal code.  If the porting group
**     wishes to run the preprocessor on a master machine and then ship the
**     generated ".c" files to the target machines for compilation, then we
**     don't want to generate #include lines with a full pathname as the real
**     path on the target machine is likely to be different.  On the other
**     hand, we can't just generate <filename.ext>, because only the C
**     compiler supports the "-I" or an equivalent.  The compromise we chose
**     is:
**	  if (language is C) and ("-c" flag specified) then
**	    generate: #include <filename.ext>
**   	  else
**	    generate: #include "full-path-name.ext"
**
**  5. The "-o" flag with no suffix means that we should never generate output
**     for include files.
**
**  6. See cgen.c for a special workaround for double preprocessed files
**     whereby eqc when used with the "-c" and "-o.sh" flags will generate
**     an "exec sql include filename" after seeing ## include filename.
**     (filename may be be a global filename or otherwise.)
**
**  7. We support relative path to local include files. 
**     (global include files are discussed in no. 4 above).
**     We assume that all local include files ("local.qh") reside in the same
**     path as the file that included it. 
**     EXAMPLE:
**     		Assume command was:  ESQLC dir1/main.sc
**     		If main.sc contains "EXEC SQL INCLUDE 'dir2/foo' and
**	  	    foo.sc contains "EXEC SQL INCLUDE 'dir3/foo2'
**	   Include			Generate	
**	   -------			--------
**	  "dir1/dir2/foo.c"          include 'dir2/foo.c'  (in dir1/main.c)
**     "dir1/dir2/dir3/foo2.c"       include 'dir3/foo2.c' (in dir1/dir2/foo.c)
**
** History:
**	10-nov-1984	Rewritten. (mrw)
**	15-apr-1985	Added logical names and no rewriting of same file. (ncg)
**	17-jan-1986	ESQL extensions are like the main extensions. (ncg)
**	01-mar-1986	Added hooks for EQUEL/ADA and ESQL/ADA. See note (ncg)
**	01-dec-1986	Added support for global includes, from code by jhw
**			on 05-feb-1986. (mrw)
**	13-jul-1987 	Changed memory allocation to use [FM]Ereqmem. (bab)
**	15-sep-1988	Added inc_save for inc_fileparse changes and
**			remote include file processing. (ncg)
**	16-nov-1988	Added MVS code to handle data set inclusion. (emerson)
**	11-jan-1989	Added CMS ifdefs again. (wolf)
**	26-jan-1990	Copy buffers because LOfroms is now more strict (bjb)
**	11-jun-92 (seng/fredb)
**			Integrate dcarter's 02-feb-1990 change from 6202p:
**      		   02-feb-1990     Added HP3000 ifdef (hp9_mpe),
**					   which causes output include file
**					   name to be formed correctly
**					   (dcarter)
**	16-Oct-92 (fredb) hp9_mpe
**			Eliminate some of the relative path code for
**			MPE/iX because we don't have relative paths and
**			the resulting filename is seriously hosed.
**	12-Nov-1993 (tad/smc)
**		Bug #56449
**		Replace %x with %p for pointer values where necessary.
**	21-nov-1994 (andyw)
**	    Solaris 2.4/Compiler 3.00 port failed in front!abf!copyapp
**	    then trying to compile caout.sc; this file make an 
**	    EXEC SQL INCLUDE <oodep.h> which is overwritten when
**	    the grammer calls inc_push_file(), this opens for read
**	    then opens for write (no append specified, therefore file
**	    is re-generated). Change to inc_push_file() from write to read
**
**	04-Jan-95 (carly01)
**		Bug 65935
**		Bug fixed on 21-nov-1994 (andyw) was actually caused by 
**		multiple mkmings being done (thus causing the MING file
**		to include multiple references to caout.c, caerror.c, and
**		insrcs.c).  The compile of caout was being done with the
**		erroneous MING file.  This fix restored the open for output:
**		EQ_SI_OPEN_LANG(..., ERx("w"), &ofile).  The MING file was
**		also restored to its correct form (by tutto01).
**      18-jan-1994 (harpa06)
**              Bug #65006
**              When specifying an absolute path to a source file on the
**              command line, and specifying a relative path in the
**              source file using "../" notation, ESQLC would try to look at an
**              invalid directory structure. For example:
**
**              filename: /tmp/esqlc/src/test.sc has
**                      EXEC SQL    INCLUDE "../inc/test.h";
**
**              test.h is located in /tmp/esqlc/inc
**
**              and you specify from the command line:
**                      esqlc /tmp/esqlc/src/test.sc
**
**               from the src directory, ESQLC would look for the file:
**                      /tmp/esqlc/src../inc/test.h
**
**              It now looks for: /tmp/esqlc/src/../inc/test.h
**      12-Dec-95 (fanra01)
**          Added definitions for extracting data for DLLs on windows NT
**	18-mar-1996 (thoda04)
**	    Added #include eqgen.h for out_xxxxx functions.
**	    Added #include eqsym.h for eqgen.h.
**	    Corrected cast of MEreqmem()'s tag_id (1st parm).
**	    Moved inc_add_inc_dir() out of list of locals; it called by eqmain.c.
**	    Defined function prototypes for local functions.
**  29-may-1996 (angusm)
**		Change logic in inc_push_file() so that output of EXEC SQL
**		INCLUDE file in relative path is also in relative path.
**		(bug 76327).
**	24-sep-1996 (kch)
**		In inc_push_file(), added check for null remote path before
**		constructing remote output filename. This change fixes bugs
**		78639, 78423 and 76036.
**	28-jan-1997 (kch)
**		In inc_fileparse(), we now only add a "/" to the remote path
**		if it is not NULL. This change fixes bug 78423 (and not the
**		change described above).
**
** Copyright (c) 2004 Ingres Corporation
**      21-apr-1999 (hanch04)
**        Replace STrindex with STrchr
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
[@history_template@]...
*/

/*
** Each Include file must save the state of the parent file.  The Include
** files are stacked in the order they are referenced. The following
** structures and constants describes all the information saved by the
** Include routines.
*/

/*
** a local structure to hold the components of the include file name
** until we actually include it - inc_tst and inc_push are called independently.
*/

typedef struct finc {
    char	*f_pathfile,	/* path, file and extension only */
		*f_file,	/* file name only */
		*f_ext,		/* file extension */
		*f_more,	/* any qualifiers after file name */
		*f_outname,	/* full output name as generated */
		*f_username,	/* name user gave, with extension munged */
		*f_remotename;	/* Remote include output file name */
} FINC;

/*
** State information saver for nested Includes - most information is from
** the global EQ_STATE.
*/

typedef struct inc_ {
    i4		inc_line;		/* Parent file's input line */
    i4		inc_flags;		/* Current file flags */
    FILE	*inc_infile;		/* Parent input pointer */
    FILE	*inc_outfile;		/* Parent output pointer */
    FINC	inc_finc;		/* Current filename pieces */
    struct inc_ *inc_next;		/* Next (parent) nested file */
} INC_FILE;

/* inc_flags flags */
# define	INC_INLINE	1	/* output goes to parent file */
# define	INC_NOWRITE	2	/* don't write any output */
# define	INC_NOEXPAND	4	/* Don't expand pathname on output */
# define	INC_REMOTE	8	/* Remote output file name used */

/*
** Because inc_tst and inc_push are called independently of each other there
** must be some static information to pass from one routine to the other.
*/
static INC_FILE	inc_tfile ZERO_FILL;	/* name pointers for include */
static LOCATION	f_loc ZERO_FILL;	/* include LOC and associated buffer */
static char	f_locbuf[MAX_LOC+5] ZERO_FILL;	/* Add 4 for SQL extension */

/* A local string table, already implemented as a stack, used to store names */
static STR_TABLE	*inc_fnames = STRNULL;

/*
** Structure to store old file names to prevent writing the same file twice
*/
typedef struct i_o {
    char	*io_name;
    struct i_o	*io_next;
} INC_OLD;
static	INC_OLD		*inc_ofiles ZERO_FILL;

/*
** Stack of Include file pointers
*/
static  INC_FILE	*inc_stack = NULL;	

/*
** Small fixed list of include directory location pointers.
** The list is guaranteed to have a NULL sentinel at the end.
*/
# define	INC_MAX_LOCS	7
static LOCATION		*inc_locs[INC_MAX_LOCS+1] ZERO_FILL;

/* Entry point to EQUEL's that need a second pass into the output files (EQP) */
GLOBALREF	i4	(*inc_pass2)();

/*
** Static location that describes the original parent file.
** This is used by inc_fileparse to allow us to include local files
** from a remote  directory.  This is saved by inc_save if any
** remote directory mapping is required.
*/
static LOCATION p_loc ZERO_FILL;	/* Parent location */
static char	p_locbuf[MAX_LOC+1] ZERO_FILL;

/*
**  Local function prototypes
*/
bool	inc_fileparse(char *namebuf, i4 namebuf_size, FINC *f_inc, i4  pounded, i4  is_global);
i4  	inc_expand_include(LOCATION *f_locp, char *f_locbufp, LOCATION **glob_loc);
bool	inc_isdir(LOCATION *loc);


/*
+* Procedure:	inc_tst_include
** Purpose:	See if the newly read line has an Include directive in it.
**
** Parameters:	None.
** Returns:	i4	- SC_NOINC	- Not include line
**			  SC_INC	- Regular include
**			  SC_INLINE	- Inline code inclusion
** Side Effects:
-*		Scans the input buffer directly.
**
** Notes:
**  1. Used only for EQUEL.
**  2. On regular includes we generate a statement to reflect the include in
**     the preprocessed file.
**     For inline inclusion of code (usually) we just include the code as is
**     and the included source file is preprocessed into the parent output
**     file.
**  3. Names delimited with '<' and '>' are also to be looked for in the
**     global directories, specified with "-h".
*/

i4
inc_tst_include()
{
    char		*cp,
			*fname;
    char		fdelim;	/* Delimiter for included filename */
    bool		in_line;
    bool		n;
    char		savech;		/* To restore the main line buffer */

    cp = SC_PTR;
    if (*cp != '#' || *(cp+1) != '#')	/* Not an Equel line */
    {
	return SC_NOINC;
    } else				/* ## something */
    {
      /* Skip white space up to include */
	for (cp += 2; CMspace(cp) || *cp == '\t'; CMnext(cp))
		;
      /* Cp is pointing at I in a line such as ##<white>Include ... */
    }

  /* we've seen "## " and might get "include" here */
    if (!CMwhite(cp+7) || STbcompare(ERx("include"), 7, cp, 7, TRUE) != 0)
	return SC_NOINC;
  /* Skip "include", and white space till "inline" or first file name delim */
    for (cp += 7; CMspace(cp) || *cp == '\t' ; CMnext(cp))
	    ;
    if (CMwhite(cp+6) && STbcompare(ERx("inline"), 6, cp, 6, TRUE) == 0)
    {
	for (cp += 6; CMspace(cp) || *cp=='\t'; CMnext(cp))
	    ;
	in_line = TRUE;
    } else
	in_line = FALSE;

    /*
    ** we've seen "[##] include [inline]" and expect a filename,
    ** with or without delimiters - if no delimiters then must be alphanumeric
    ** as we do not want to manage things like '[]' '..' '/' of different
    ** systems.
    */
    if (CMnmstart(cp) || CMdigit(cp))	/* filename w/o delimiter */
	fdelim = '\0';
    else				/* must be quoted [path] file  name */
    {
	fdelim = *cp;
	if (fdelim == '<')		/* global includes: "-h" relative */
	{
	    fdelim = '>';
	} else if (fdelim != '"' && fdelim != '\'')
	{
	    char	bcp[3];
	    char	*p = bcp;

	    CMcpyinc( cp, p );
	    *p = '\0';
	    if (bcp[0] == '\n')
		p = ERx("");
	    else
		p = bcp;
	    er_write( E_EQ0211_scINCQUOTE, EQ_ERROR, 1, p );
	    return SC_BADINC;
	}
	CMnext( cp );			/* skip the quote */
    }
    fname = cp;

    /*
    ** 'cp' points to F in a line such as "##<white>include<white>"F.qc".
    ** Filenames start with an alphabetic and end at the delimiter, if any,
    ** or at whitespace if not.  Thus we can accept a line such as
    ** ## include inline falcon::dra1:[cm.21]file.ext;version -- this is comment
    ** or
    ** ## include "<disk>dir/this is a name with embedded spaces"
    ** or
    ** ## include "this char - \" - is an embedded quote"
    */
    for (; *cp && *cp != '\n'; CMnext(cp))
    {
      /* allow escaped delimiters */
	if (*cp == '\\')
	{
	    cp++;		/* next char is never a delimiter */
	    continue;
	}

	if (fdelim != '\0')
	{
	    if (*cp == fdelim)
		break;
	} else if (CMwhite(cp))
	    break;
    }
    savech = *cp;		/* save the source line */
    *cp = '\0';

    /*
    ** in_line is TRUE iff we saw the "inline" modifier,
    ** and fname points to the complete (null-terminated) filename
    */
    STcopy( fname, f_locbuf );			/* copy it for our parent */
    *cp = savech;				/* restore the source line */
    inc_tfile.inc_flags = in_line ? INC_INLINE : 0;

    n = inc_fileparse(f_locbuf, sizeof(f_locbuf), &inc_tfile.inc_finc, TRUE, fdelim == '>');
    if (n == FALSE)
	return SC_BADINC;
    return in_line ? SC_INLINE : SC_INC;
}


/*
+* Procedure:	inc_parse_name
** Purpose:	Parse a passed-in full file name (for SQL).
** Parameters:
**	fname	- char *	- The nul-terminated filename to be parsed.
**	is_global - i4		- File to be parsed as a global name (C only)
** Return Values:
**		i4	- SC_BADINC	- Bad filename
-*			  SC_INC	- Regular include
** Notes:
**	Much like inc_tst_include -- just for SQL.
**
** Imports modified:
**	inc_tfile
**	f_locbuf
*/

i4
inc_parse_name( fname, is_global )
    char	*fname;
    i4		is_global;
{

  /* fname points to the complete (null-terminated) filename */
    STcopy( fname, f_locbuf );			/* copy it for our parent */
    inc_tfile.inc_flags = 0;

    if (inc_fileparse(f_locbuf, sizeof(f_locbuf), &inc_tfile.inc_finc,TRUE,is_global) == FALSE)
	return SC_BADINC;
    return SC_INC;
}

/*
+* Procedure:	inc_inline
** Purpose:	For SQL this file should be made INLINE - used for ADA.
** Parameters:
**	None
** Return Values:
-*	None
** Notes:
**	Now always done for MVS.
**	See the note at top why ADA may be different.
**	We turn off NOWRITE to ensure output for INLINE includes.
**
** History:
**	28-mar-90 	- Put in fix for ADA so declarations in a nested 
**			  include file are no longer included in-line in the 
**			  outermost source file - this caused an ADA compiler
**			  error.  Bug 8975. (teresal)
*/
void
inc_inline()
{
    INC_FILE	*i_f = inc_stack;

    /*
    ** (Bug 8975) set the "in-line" attribute only if you're sure
    ** that there's no parent file or that the parent is generating
    ** output. Otherwise you'll write into an unopened parent.
    */
    if (i_f == NULL || (i_f->inc_flags & INC_NOWRITE) == 0)
    {
    	inc_tfile.inc_flags &= ~INC_NOWRITE;
    	inc_tfile.inc_flags |= INC_INLINE;
    }
}



/*
+* Procedure:	inc_fileparse
** Purpose:	Called to parse an include filename.
**
** Parameters:	namebuf - char *  -  Full file name with extension and
**				     optional extra stuff.
**		f_inc	- FINC *  -  Include file structure to fill.
**		pounded	- i4	  -  Flag to suppress error messages.  Not
**				     suppressed if comes from the ## include
**				     statement.
**		is_global- bool	  -  Flags whether filename is to be parsed
**				     as a global include ("-h").
-* Returns:	bool 	- Was the file parsed okay ?
**
** Note:
**	To make this more useful, and CL compatible the routine first looks
**	for VMS extensions and qualifiers. (ncg)
**	If the "-o" flag was given without an extension then suppress output.
**	Called by inc_tst_include and inc_parse_name.
**
** History:
**	01-nov-1983 	Written (ncg).
**	01-dec-1986	Added support for global includes from jhw,
**			05-feb-1986. (mrw)
**	14-sep-1988	Allowed local files to be in remote directory. (ncg)
**      03-nov-1988     Treat local includes in MVS as specifying a full path
**			and file; treat global includes as specifying only a
**			filename prefix to be found in path-lib. (emerson)
**      30-mar-1989     for MVS, ifndef around logic that adds extension
**                      to directory file-name. (emerson)
**	23-jan-1990	Don't reuse or alter LOfroms buffers. (barbara)
**	19-oct-1990	If ESQL global include: <f_name> does not have 
**			extension, Add default extension. <f_name.default_ext>
**			Previously, only done for local includes.(kathryn)
**	19-oct-1990     Allow relative paths to include files.	(no. 7 above).
**			(kathryn)
**	06-may-1991	Send NULL rather than EOS to LOcompose, when just file
**			name with no path is given.
**	16-Oct-92 (fredb)
**			Eliminate certain sections of code for MPE/ix
**			(hp9_mpe) that are causing the filename to get
**			hosed.  MPE does not have relative paths anyway.
**			
**      18-jan-1994 (harpa06) Bug #65006
**                      When specifying an absolute path to a source file on the
**                      command line, and specifying a relative path in the
**                      source file using "../" notation, ESQLC would try to
**                      look at an invalid directory structure. For example:
**
**                      filename: /tmp/esqlc/src/test.sc has
**                              EXEC SQL    INCLUDE "../inc/test.h";
**
**                      test.h is located in /tmp/esqlc/inc
**
**                      and you specify from the command line:
**                              esqlc /tmp/esqlc/src/test.sc
**
**                       from the src directory, ESQLC would look for the file:
**                              /tmp/esqlc/src../inc/test.h
**
**                      It now looks for: /tmp/esqlc/src/../inc/test.h
**	28-jan-1997 (kch)
**		We now only add a "/" to the remote path if it is not NULL.
**		This change fixes bug 78423 (and not the previous change in
**		inc_push_file()).
**	17-Jun-2004 (schka24)
**	    Avoid buffer overruns.
**	09-Nov-2004 (drivi01)
**	    Modified inc_fileparse function to pass namebuf size as a parameter
**	    to be used in STlcopy function to reflect size of namebuf. 
**	    Fix for bug #113420.
*/

bool
inc_fileparse(namebuf, namebuf_size, f_inc, pounded, is_global)
char	*namebuf;		/* full input file name buffer */
i4  namebuf_size;
FINC	*f_inc;
i4	pounded;
i4	is_global;
{
    char	inc_filename[MAX_LOC+1];/* shadow of namebuf */
    char	dumbuf[MAX_LOC+1];	/* dummy for LOdetail */
    char	f_file[MAX_LOC+1];	/* just file name (no path) */
    char	f_ext[MAX_LOC+1];	/* just input extension */
    char	f_more[MAX_LOC+1];	/* any extra qualifiers */
    char	f_path[MAX_LOC+1];	/* directory name of file */
    char	f_node[MAX_LOC+1];	/* node name of file */
    char	glob_incbuf[MAX_LOC+3];	/* global include munged name */
    char	remote_name[MAX_LOC+1];	/* remote output name */
    char	*logname;
    STATUS	status;
    char 	*p;			/* For special cases */
    char	r_path[MAX_LOC+1];	/* directory name of parent file */
    char	r_node[MAX_LOC+1];	/* node name of parent file */
    char	rel_locbuf[MAX_LOC+1];  /* location buffer of parent file */
    LOCATION	rel_loc;		/* location of parent file */


    f_more[0] = '\0';
    glob_incbuf[0] = '\0';
    remote_name[0] = '\0';

  /* INLINE takes precedence over NOWRITE */

    if ((inc_tfile.inc_flags & INC_INLINE) == 0)
    {
	/*
	** Turn off writing if the user requested, or if this is ADA,
	** as ADA assumes a compile-time library.
	*/
	if (eq_islang(EQ_ADA) || (eq->eq_flags & EQ_INCNOWRT))
	    inc_tfile.inc_flags |= INC_NOWRITE;
    }
	
# ifdef VMS
    /*
    ** There is the possibility of extra qualifiers after extension,
    ** so scan back from end and pick them off for f_more, and leave just
    ** a file specification for LOfroms().
    */
    if (p = STindex(namebuf, ERx("/"), (i2)0))	/* Ie: '/nolist' */
    {
	STlcopy( p, f_more, sizeof(f_more)-1 );
	*p = '\0';
    }
# endif /* VMS */

    logname = (char *)0;
    NMgtAt( namebuf, &logname );		/* Maybe a logical */
    if (logname != (char *)0 && *logname != '\0')
	STlcopy( logname, namebuf, namebuf_size-1 );
    /* namebuf now has the path and file name (without qualifiers) */

    /* namebuf belongs to LOfroms; inc_filename is for local use */
    STlcopy( namebuf, inc_filename, sizeof(inc_filename)-1 );

# ifndef MVS

# ifdef CMS
    status = LOfroms(       FILENAME, namebuf, &f_loc);
# else /* CMS */
    status = LOfroms(PATH & FILENAME, namebuf, &f_loc);
# endif /* CMS */

# else /* MVS */

    if (eq->eq_flags & EQ_COMPATLIB)
	is_global = TRUE;

    if (!is_global)
    {
       status = LOfroms(PATH & FILENAME, namebuf, &f_loc);
    }
    else	/* A global include */
    {
       /*
       ** Make a copy of the user's filename, discard the extension
       ** (if any), and wrap parentheses around it, to indicate
       ** to the MVS LOfroms that the filename is a PDS member-name.
       ** If we found an extension, we require that it be the extension
       ** specified in the -n flag.  This closely approximates
       ** the behavior of the Whitesmith IBM C compiler on #includes.
       */

       glob_incbuf[0] = '(';
       STcopy(inc_filename, glob_incbuf+1);
       p = STrchr(glob_incbuf+1, '.');
       if (p)				/* There is an extension - discard */
       { 
	  /* 
	  ** The MVS C compiler either allows no extension or requires
	  ** a single extension (which is ignored).  The MVS preprocessor
	  ** follows this rule (even if not EQUEL) using the -n flag.
	  */
          if (STbcompare(eq->eq_in_ext, 0, p+1, 0, TRUE) != 0)
          {
             if (pounded)
                er_write(E_EQ0201_scBADINCEXT, EQ_ERROR, 2, inc_filename,
                          eq->eq_in_ext);
             return FALSE;
          }
       } 
       else				/* No extension */
          p = glob_incbuf + STlength(glob_incbuf);
       *p++ = ')';
       *p = EOS;

       /* Get location of #include file-name (viewed as a PDS member-name) */
       STcopy( glob_incbuf, namebuf );
       status = LOfroms(FILENAME, namebuf, &f_loc);
    }

# endif /* MVS */

    if (status != OK)
    {
        /* BUG 4284 - Error only if used ## include (ncg) */
	if (pounded)
	    er_write( E_EQ0207_scILLINCNAM, EQ_ERROR, 1, inc_filename );
	return FALSE;
    }
    f_ext[0] = EOS;
    LOdetail(&f_loc, dumbuf, dumbuf, f_file, f_ext, dumbuf);

# ifndef MVS

    if (dml_islang(DML_EQUEL))
    {
	/* EQUEL requires specific extension */

	if (STbcompare(eq->eq_in_ext, 0, f_ext, 0, TRUE) != 0)
	{
	    if (pounded)
		er_write( E_EQ0201_scBADINCEXT, EQ_ERROR, 2, inc_filename,
			eq->eq_in_ext );
	    return FALSE;
	}
    } else
    {
	if (f_ext[0] == EOS)
	{
	/* ESQL allows any extension or no extension (= default extension) 
	** No Extension on filename so add default estension:
	**	[.sc, .sf, .sb...(whatever the language)] 
	** Re-do LO so f_loc contains "filename.default_ext".
	*/
	    STcat( inc_filename, ERx(".") );

	  /* Might have changed eq_in_ext so use eq_def_in (should be same) */
	    STcat( inc_filename, eq->eq_def_in );
	    STcopy( inc_filename, namebuf );

	  /* Is sure to work as it worked without an extension */
	    LOfroms( PATH & FILENAME, namebuf, &f_loc );
	    LOdetail( &f_loc, dumbuf, dumbuf, f_file, f_ext, dumbuf );
	}
	eq->eq_in_ext = STalloc( f_ext );
    }
# endif /* not MVS */

    /*
    ** If this file should be searched in several include directories,
    ** do so.  Look first in the local dir, then in the global dirs ("-h")
    ** in the order mentioned.  If not found then report an error.
    */
    if (pounded && is_global)
    {
	/*  Include file was specified as <pathname> with angle brackets.
	*/

#ifndef MVS
	if (eq->eq_flags & EQ_COMPATLIB)
	{
	    /*
	    ** This is EQUEL/C, the "-c" flag was specified, and this is a
	    ** global include (#include <pathname>).  We'll generate
	    **		# include	<foo/bar/a.c>
	    ** or whatever -- we'll use the angle brackets, and not prepend
	    ** the directory.
	    **
	    ** Make a copy of the user's filename, and change the extension
	    ** to the correct output extension.  This is because we will
	    ** want to use this name in the generated #include <> line,
	    ** not the name as expanded by inc_expand_include.
	    ** Add the angle brackets in.
	    */

	    glob_incbuf[0] = '<';
	    STcopy( inc_filename, glob_incbuf+1 );
	    p = STrchr( glob_incbuf, '.' );
	    if (p)
		*p = '\0';
	    else
		p = glob_incbuf + STlength(glob_incbuf);
	    STprintf( p, ERx(".%s>"), eq->eq_out_ext );

	    inc_tfile.inc_flags |= INC_NOEXPAND;  /* gen: #include <file.ext> */
	}

#endif /* not MVS */

	/* The -c flag was not issued - expand <pathname> to full path.
	** Append the full directory (one of the directories included with
	** the -h flag).
	*/

	STcopy( inc_filename, namebuf );
	if (!inc_expand_include(&f_loc, namebuf, inc_locs))
	{
	    er_write( E_EQ0226_scINCNOTFOUND, EQ_ERROR, 1, inc_filename );
	    return FALSE;
	}
	STcopy(f_locbuf,inc_filename);
    }
    else
    {
	/*
	** This is a local file so find out if this is included in a remote
	** directory.  If the include file has no directory specification,
	** and the parent file DOES have a directory specification,
	** then assume the local include file is to be found in the parent
	** file's directory.  For example, assume you are not in /tmp, and
	** you issue:
	**	eqc -c -n.qh -o.h /tmp/file.qc
	** And file.qc includes:
	**	## include "foo/local.qh"
	** We should process as though the following INCLUDE was issued:
	**	## include "/tmp/foo/local.qh"
	** but still generate (for the following host compiler):
	**	# include "foo/local.h"
	** The original parent file (/tmp) location was saved by inc_save
	*/


    	LOdetail( &f_loc, f_node, f_path, f_file, f_ext, dumbuf );
    	rel_locbuf[0] = EOS;
    	if (inc_is_nested())
    	{
      		/* This is a nested include file.  
		** Get its parent file location 
		*/
    		STcopy(inc_stack->inc_finc.f_pathfile, rel_locbuf);
    	}
    	else if (p_locbuf[0] != EOS)
    	{
		/* Parent file is Original file and Original file had path */
    		STcopy(p_locbuf,rel_locbuf);
    	}
    	if (LOisfull(&f_loc) || rel_locbuf[0] == EOS)
		STcopy(f_locbuf,inc_filename);
    	else
    	{
# ifndef hp9_mpe
		LOfroms(PATH & FILENAME,rel_locbuf, &rel_loc);
       	 	LOdetail( &rel_loc, r_node, r_path, dumbuf, dumbuf, dumbuf );

		/* if f_loc (foo/local.qh) has path as well as file then add 
		** paths */

		dumbuf[0] = EOS;
		if(f_path[0] != EOS)
		{
/*
** Bug #65006 - check to see if the source include file contains a relative path
** using the "../" notation. If so, concatenate the source file path and the
** include file path correctly
*/
/*
** Bug 78423 - only add a "/" to the reomte path if it is not NULL.
*/
		    if ((f_path[0]=='.')&&(f_path[1]=='.')&&(r_path[0]!='\0'))
			STcat(r_path,"/");
		    STcat(r_path,f_path);

		}
		/* Workaround for LOcompose bug - When path=EOS rather
		** than NULL LOcompose adds "/" to path. So filename with
		** no path has location "/filename" rather than just
		** "filename".
		*/
		if (r_path[0] == EOS)
			LOcompose(r_node,NULL,f_file,f_ext,dumbuf,&rel_loc);
		else
			LOcompose(r_node,r_path,f_file,f_ext,dumbuf,&rel_loc);
		STcopy(rel_locbuf,inc_filename);

		/* rel_loc has path from current working directory to include 
		** file.  
		** remote_name gets filename to be generated (foo/local.h) 
		*/

		STprintf(dumbuf, ERx("%s.%s"), f_file, eq->eq_out_ext );
		LOfstfile(dumbuf, &f_loc);
    		f_inc->f_remotename = str_add(inc_fnames,f_locbuf);

		LOcopy(&rel_loc,f_locbuf,&f_loc);
	
		/* gen #include "foo/local.h" */

		inc_tfile.inc_flags |= INC_REMOTE;
# endif	  /* NOT hp9_mpe */
    	}
    }
    

    /*
    ** inc_filename has full path, file and extension without qualifiers
    ** namebuf has the same, as modified by LOfroms
    ** f_file has just the file name
    ** f_more has any extra stuff after name.
    */
    if (inc_fnames == NULL)
	inc_fnames = str_new( MAX_LOC+1 );
    f_inc->f_pathfile = str_add( inc_fnames, inc_filename);
    f_inc->f_file = str_add( inc_fnames, f_file );
    f_inc->f_more = str_add( inc_fnames, f_more );
    f_inc->f_username = str_add( inc_fnames, glob_incbuf );


    /*
    ** f_pathfile has full input path, file and extension without qualifiers
    ** f_file has just the file name
    ** f_ext is not yet defined
    ** f_more has any extra stuff after name
    ** f_outname is not yet defined.
    ** f_username is {if global compat C include, output name with brackets}
    **		else {is null}
    ** f_remotename is the remote output name to be generated.
    */

    return TRUE;
}

/*
+* Procedure:	inc_push_file
** Purpose:	Change I/O files for a file inclusion after ## include.
**		Saves status of current I/O files and opens appropriate files.
**
** Parameters:	None - there is one static parameter with file information.
-* Returns:	None
** Assumptions:
**	- static variable inc_tfile has the parsed file name to which to switch.
**	- called by grammar after yylex() returns TOK_INCLUDE (just after
**	  inc_tst_include() returned SC_INCLUDE) or from yylex()
**	  when inc_tst_include() returns SC_INLINE.
** History:
**      03-nov-1988 - For MVS, make all includes expand inline. (emerson)
**      21-nov-1994 (andyw)
**          Solaris 2.4/Compiler 3.00 port failed in front!abf!copyapp
**          then trying to compile caout.sc; this file make an 
**          EXEC SQL INCLUDE <oodep.h> which is overwritten when
**          the grammer calls inc_push_file(), this opens for read
**          then opens for write (no append specified, therefore file
**          is re-generated). Change to inc_push_file() from write to read
**	29-may-1996 (angusm)
**		Ensure that we open output file in relative path for
**		preprocessed EXEC SQL INCLUDE file in relative path.
**		(bug 76327).
**	24-sep-1996 (kch)
**		Added check for null remote path before constructing remote
**		output filename and now call LOcompose() with the second
**		argument set to NULL rather than rpath if (*rpath == '\0').
**		This prevents a forward slash ('/') being added to the filename
**		during the call to LOcompose(). A nested include will be marked
**		as being remote, even though it may be in the same directory as
**		the includer. This change fixes bugs 78639, 78423 and 76036.
*/

void
inc_push_file()
{
    INC_FILE		*i_f;
    FINC		*cur_finc;
    FILE		*ifile, *ofile;
    char		tmp_buf[MAX_LOC+1];
    INC_OLD		*io;
    STATUS		opened;
# ifndef UNIX				/* Ignore case on name lookup */
#   define INC_SAME	TRUE
# else					/* Case is significant */
#   define INC_SAME	FALSE
# endif  /* UNIX */

# ifdef MVS      /* In MVS, make all includes inline */
    inc_inline();
# endif  /* MVS */

    cur_finc = &inc_tfile.inc_finc;
  /* Check for existing open include file with same name */
    for (i_f = inc_stack; i_f &&
	 STbcompare(i_f->inc_finc.f_pathfile, 0, cur_finc->f_pathfile, 0,
		    INC_SAME) != 0;
	 i_f = i_f->inc_next)
	;
    if (i_f)
    {
	er_write( E_EQ0209_scINCRCRSVOPN, EQ_ERROR, 1, cur_finc->f_pathfile );
	str_free( inc_fnames, cur_finc->f_pathfile );
	return;
    }
    if (SIopen(&f_loc, ERx("r"), &ifile) != OK)	/* open user's input file */
    {
	er_write( E_EQ0208_scINCINOPN, EQ_ERROR, 1, cur_finc->f_pathfile );
	str_free( inc_fnames, cur_finc->f_pathfile );
	return;
    }

    /* make sure ofile always has a reasonable value */
    ofile = eq->eq_outfile;
    /* Open output file if not inline */
    if (inc_tfile.inc_flags & INC_INLINE)
    {
	cur_finc->f_outname = eq->eq_outname;
    } else
    {
      /* Modify input file name to use output extension */
	if (inc_tfile.inc_flags & INC_REMOTE)   /* bug 76327 */ 
	{
		LOCATION	tl;
		char		r_path[MAX_LOC+1];

		LOfroms(PATH & FILENAME, cur_finc->f_pathfile, &tl);
		LOdetail( &tl, tmp_buf, r_path, tmp_buf, tmp_buf, tmp_buf );
		if (*r_path == '\0')
		{
		    LOcompose(NULL, NULL, cur_finc->f_file, eq->eq_out_ext, NULL, &f_loc);
		}
		else
		{
		    LOcompose(NULL, r_path, cur_finc->f_file, eq->eq_out_ext, NULL, &f_loc);
		}
	}
	else
	{
		STprintf( tmp_buf, ERx("%s.%s"), cur_finc->f_file, eq->eq_out_ext );
# ifdef hp9_mpe
		/* For HP3000, construct new filename from scratch */
		LOfroms( FILENAME, tmp_buf, &f_loc );
# else
		LOfstfile( tmp_buf, &f_loc );
# endif
	}
	LOtos( &f_loc, &cur_finc->f_outname );
	
      /* Check for existing old output file name */
	for (io = inc_ofiles; io &&
	     STbcompare(io->io_name, 0, cur_finc->f_outname, 0, INC_SAME) != 0;
	     io = io->io_next)
	    ;
	if (io == NULL && (inc_tfile.inc_flags & INC_NOWRITE)==0)
	{
	    /*65935 - open for output*/
	    EQ_SI_OPEN_LANG( opened, &f_loc, ERx("w"), &ofile );
            if (opened != OK)
	    {
		er_write(E_EQ0210_scINCOUTOPN, EQ_ERROR, 1, cur_finc->f_outname);
		SIclose( ifile );
		str_free( inc_fnames, cur_finc->f_pathfile );
		return;
	    }
	  /* Remember this name! */
	    if ((io = (INC_OLD *)MEreqmem((u_i2)0, (u_i4)sizeof(INC_OLD),
		TRUE, (STATUS *)NULL)) == NULL)
	    {
		er_write( E_EQ0001_eqNOMEM, EQ_FATAL, 2,
			er_na(sizeof(INC_OLD)), ERx("inc_push_file()") );
		return;
	    }
	    io->io_name = STalloc( cur_finc->f_outname );
	    io->io_next = inc_ofiles;
	    inc_ofiles = io;
	} else
	    inc_tfile.inc_flags |= INC_NOWRITE;	/* We know that INLINE is off */

      /* always set this for upstairs */
	cur_finc->f_outname = str_add( inc_fnames, cur_finc->f_outname );
    } /* not inline */

    /*
    ** ofile points at output file,
    ** f_outname points at full output name
    */

    /* Save the state before switching files */
    if ((i_f = (INC_FILE *)MEreqmem((u_i2)0, (u_i4)sizeof(INC_FILE),
	TRUE, (STATUS *)NULL)) == NULL)
    {
	er_write( E_EQ0001_eqNOMEM, EQ_FATAL, 2, er_na(sizeof(INC_FILE)),
		ERx("inc_push_file()") );
	return;
    }

  /* save parent's state - line number, i/o files */
    i_f->inc_line = eq->eq_line;		/* parent */
    i_f->inc_flags = inc_tfile.inc_flags;	/* us */
    i_f->inc_infile = eq->eq_infile;		/* parent */
    i_f->inc_outfile = eq->eq_outfile;		/* parent */
  /* save parents file names and extension */
    i_f->inc_finc.f_file = eq->eq_filename;	/* parent */
    i_f->inc_finc.f_ext = eq->eq_ext;		/* parent */
    i_f->inc_finc.f_outname = eq->eq_outname;	/* parent */
  /* save pointers for popping */
    i_f->inc_finc.f_pathfile = cur_finc->f_pathfile;    /* us */
    i_f->inc_finc.f_more = cur_finc->f_more;    /* us - not really required */
    i_f->inc_next = inc_stack;		 	/* us */

  /* generate the include statement in the OLD output file */
    if ((i_f->inc_flags & INC_INLINE) == 0)
    {
	if (i_f->inc_flags & INC_NOEXPAND)	/* EQC -c -o -h */
	    gen_include( cur_finc->f_username, (char *)0, cur_finc->f_more );
	else if (i_f->inc_flags & INC_REMOTE)	/* Remote file name */
	    gen_include(cur_finc->f_remotename, (char *)0, cur_finc->f_more);
	else
	    gen_include( cur_finc->f_outname, (char *)0, cur_finc->f_more );
    }
    if (i_f->inc_flags & INC_NOWRITE)
	out_suppress( TRUE );
    else
	out_suppress( FALSE );

    /* Assign new state information for global eq */
    eq->eq_line = 1;
    eq->eq_filename = cur_finc->f_file;
    eq->eq_ext = eq->eq_in_ext;
    eq->eq_outname = cur_finc->f_outname;
    eq->eq_infile = ifile;
    eq->eq_outfile = ofile;
    inc_stack = i_f;
    /*
    ** Some languages (Pascal) require a second pass so so supply entry info.
    ** If this is INLINE then there will be no Pass 2 on Include file.
    */
    if (inc_pass2 && (i_f->inc_flags & INC_INLINE) == 0)
	(*inc_pass2)( TRUE, 0/* Unused */ );	/* Entry */
    if (eq->eq_flags & EQ_SETLINE)	/* EQC -# */
	gen_line( (char *)0 );	/* Note the new source filename in the output */
# ifdef INC_DEBUG
    /*
    ** string table state should be:
    ** f_pathfile	- full input path and file name (no qualifiers)
    ** f_file		- input file name
    ** f_more		- extra qualifiers on input
    ** f_outname	- full output name
    ** f_username	- if global compat C include, output name with brackets
    ** f_remotename	- remote output file name of local file.
    */
    str_dump( inc_fnames, ERx("Pushing") );
# endif /* INC_DEBUG */
}

/*
+* Procedure:	inc_pop_file 	(inc_abort_file)
** Purpose:	Restore previous file environment (called by grammar on EOF).
**		Closes current files, and restores global variable
**		values for old files.
** Parameters:	None
-* Returns:	None
** Notes:	May check inc_stop (set by inc_abort_file) which may have been
**		called when a user hit ^C or a fatal error.
*/

static	i4	inc_stop = FALSE;		/* Abort all include closures */
void		inc_pop_file();

void
inc_abort_file()
{
    inc_stop = TRUE;
    inc_pop_file();
}

void
inc_pop_file()
{
    INC_FILE	*i_f;

    if ((i_f = inc_stack) != NULL)
    {
	SIclose( eq->eq_infile );
	if ((i_f->inc_flags & (INC_INLINE|INC_NOWRITE)) == 0)
	{
	    /* Don't close an inline output file or a non-written file */
	    SIflush( eq->eq_outfile );
	    SIclose( eq->eq_outfile );
	}
	/*
	** Some languages (Pascal) require a second pass on Include file
	** closure. Main files already have a GR_CLOSE call when exiting.
	** eq_outname must still be set correctly.  Don't need Pass 2 on user
	** abort or fatal error, or when the file was included INLINE. When
	** output was suppressed then the Pass 2 mechanism should not try to
	** re-read the output file, but should clean up.
	*/
	if (inc_pass2 && !inc_stop && (i_f->inc_flags & INC_INLINE)==0)
	    (*inc_pass2)( FALSE, ((i_f->inc_flags&INC_NOWRITE)!=0) ); /* Exit */

      /* restore previous environment */
	eq->eq_line = i_f->inc_line;
	eq->eq_outfile = i_f->inc_outfile;
	eq->eq_infile = i_f->inc_infile;
	eq->eq_filename = i_f->inc_finc.f_file;
	eq->eq_ext = i_f->inc_finc.f_ext;
	eq->eq_outname = i_f->inc_finc.f_outname;
	str_free( inc_fnames, i_f->inc_finc.f_pathfile );

	inc_stack = i_f->inc_next;
	if (inc_stack && (inc_stack->inc_flags & INC_NOWRITE))
	    out_suppress( TRUE );
	else
	    out_suppress( FALSE );

	/*
	** If the "-#" flag was given (then this is C)
	** then issue a "#line" directive to sync up
	** the C compiler's error messages.
	**
	** The line number in the parent file won't be updated
	** until the next token is read, so we "adjust" it here
	** to make the line number print correctly.
	** (Can you say "kludge"?  I knew you could).
	*/
	if (eq->eq_flags & EQ_SETLINE)
	{
	    eq->eq_line++;
	    gen_line( (char *)0 );
	    eq->eq_line--;
	}

	MEfree((PTR)i_f);
# ifdef INC_DEBUG
	str_dump( inc_fnames, ERx("Popping") );
# endif /* INC_DEBUG */
    }
}


/*
+* Procedure:	inc_is_nested
** Purpose:	Tell whether we are currently inside an include file.
**
** Parameters:	None
** Returns: 	i4 	- SC_NOINC 	- No include file,
**	          	- SC_INLINE 	- Inline include file,
-*		  	- SC_INC 	- Regular include file.
*/

i4
inc_is_nested()
{
    if (inc_stack == NULL)
	return SC_NOINC;
    return (inc_stack->inc_flags & INC_INLINE) ? SC_INLINE : SC_INC;
}

/*
+* Procedure:	inc_dump
** Purpose:	Dump include status information.
**
** Parameters:	None
-* Returns:	None
*/

void
inc_dump()
{
    INC_FILE	*i_f;
    FINC	*finc;
    i4		i;
    FILE	*df = eq->eq_dumpfile;
    INC_OLD	*io;

    SIfprintf( df, ERx("INC_DUMP: (") );
    if (inc_stack == NULL)
	SIfprintf( df, ERx("no ") );
    SIfprintf( df, ERx("stack)\n") );
    for (i_f = inc_stack, i = 0; i_f != NULL; i_f = i_f->inc_next, i++)
    {
	SIfprintf( df,
	    ERx("  INC[%2d] = 0x%p, line # = %d, flags = %d, next = 0x%p,\n"),
	    i, i_f, i_f->inc_line, i_f->inc_flags, i_f->inc_next );
	finc = &i_f->inc_finc;
	SIfprintf( df, ERx("  f_pathfile(us) = '%s',\n"), finc->f_pathfile );
	SIfprintf( df, ERx("  f_file(parent) = '%s', f_ext(parent) = '%s',\n"),
	    finc->f_file, finc->f_ext );
	SIfprintf( df, ERx("  f_more(us) = '%s', f_outname(parent) = '%s'\n") ,
	    finc->f_more, finc->f_outname );
    }
    /*
    ** String table state should be:
    ** f_pathfile - full input path and file name (no qualifiers)
    ** f_file     - input file name
    ** f_more	  - extra qualifiers on input
    ** f_outname  - full output name
    ** f_username - global output name
    ** f_remotename - remote output file name.
    */
    if (inc_fnames)
    {
	SIfprintf(df,
	    ERx("  String order: f_pathfile, f_file, f_more, f_outname, f_username, f_remotename.\n"));
	str_dump( inc_fnames, ERx("Inc Table") );
	SIfprintf( df, ERx("  Old files:\n") );
	for (io = inc_ofiles; io; io = io->io_next)
	    SIfprintf( df, ERx("    %s\n"), io->io_name );
    } else
	SIfprintf( df, ERx("  Include string table is empty.\n") );
    SIflush( df );
}

/*{
+*  Name: inc_add_inc_dir - Add a directory name to the include list.
**
**  Description:
**	Adds a directory name to the list of directories which will be
**	searched to find include files.  Validates the name first,
**	and issues errors as appropriate.
**
**  Inputs:
**	str		- The name of the directory.
**
**  Outputs:
**	Returns:
**	    Nothing.
**	Errors:
**		EscINCMANY	-- Too many include directories.
**		EeqNOMEM	-- Can't get memory for another LOCATION.
**		EscINCBADDIR	-- Bad include dir name, or not a directory.
-*
**  Side Effects:
**	
**  History:
**	01-dec-1986 - Written. (mrw)
**      03-nov-1988 - Added logic for MVS, to treat certain directory name
**		      strings as specifying a concatenation of locations; this
**		      involved enclosing the entire routine in a loop. (emerson)
*/

void
inc_add_inc_dir( str )
char	*str;
{
    static i4   inc_num_locs = 0;
    LOCATION    *inc_loc;
    char        *locbuf;
    i4          concat_seq_num = 0;

    for (;;)
    {
	if (inc_num_locs >= INC_MAX_LOCS)
	{
	    er_write( E_EQ0223_scINCMANY, EQ_ERROR, 1, er_na(INC_MAX_LOCS) );
	} else if (!str || !*str)
	{
	    if (!str)	/* Plain "-h" is legal, for mkmfin Makefiles */
		er_write( E_EQ0225_scINCNULDIR, EQ_ERROR, 0 );
	} else if ((inc_loc = (LOCATION *)MEreqmem((u_i2)0,
	    (u_i4)sizeof(LOCATION), TRUE, (STATUS *)NULL)) == NULL)
	{
	    er_write( E_EQ0001_eqNOMEM, EQ_FATAL, 2, er_na(sizeof(LOCATION)),
		    ERx("inc_add_inc_dir()") );
	} else if ((locbuf = (char *)MEreqmem((u_i2)0, (u_i4)(MAX_LOC + 1),
	    TRUE, (STATUS *)NULL)) == NULL)
	{
	    er_write( E_EQ0001_eqNOMEM, EQ_FATAL, 2, er_na(MAX_LOC+1),
		    ERx("inc_add_inc_dir()") );
	    MEfree( (PTR)inc_loc );
	} else
	{
#ifndef MVS

	    STcopy( str, locbuf );
	    if (LOfroms(PATH, locbuf, inc_loc) != OK || !inc_isdir(inc_loc))
	    {
		er_write( E_EQ0224_scINCBADDIR, EQ_ERROR, 1, str );
		MEfree((PTR)inc_loc);
		MEfree((PTR)locbuf);
	    }
	    else
		inc_locs[inc_num_locs++] = inc_loc;

#else  /* MVS */

            if (concat_seq_num == 0)    /* first time thru FOR loop */
            {
                STcopy( str, locbuf );
                if (LOfroms(PATH, locbuf, inc_loc) != OK)
		{
                    er_write(E_EQ0224_scINCBADDIR, EQ_ERROR, 1, str);
		    MEfree((PTR)inc_loc);
		    MEfree((PTR)locbuf);
		}
                else
                {
                    inc_locs[inc_num_locs++] = inc_loc;

                    /*
                    ** If a directory name string of the form dd:<ddname>
                    ** (or DD:<ddname>) was specified, <ddname> may specify a
		    ** concatenation of locations.  The MVS LOfroms normally
		    ** builds the first such location.  A subsequent location
		    ** (if any) can be requested by specifying a string of the
		    ** form dd:<ddname>+<n>. Given that the first call to
		    ** LOfroms has succeeded, such a subsequent call should fail
		    ** only when the concatenation of locations has been
		    ** exhausted.
                    */
                    if (STbcompare(str, 3, ERx("DD:"), 3, FALSE) == 0)
                    {
                        concat_seq_num++;
                        continue;               /* Reiterate FOR loop */
                    }
                }
            }
            else                        /* subsequent times thru FOR loop */
            {
                STprintf(locbuf, ERx("%s+%d"), str, concat_seq_num);
                if (LOfroms(PATH, locbuf, inc_loc) == OK)
                {
                    inc_locs[inc_num_locs++] = inc_loc;
                    concat_seq_num++;
                    continue;                   /* Reiterate FOR loop */
                }
		else				/* Locations exhausted */
		{
		    MEfree((PTR)inc_loc);
		    MEfree((PTR)locbuf);
		}
            }
#endif /* MVS */
	}
	return;	/* Loop is normally executed once - MVS may loop on sequences */
    }
}

/*{
+*  Name: inc_expand_include - Expand include filenames with directory prefixes.
**
**  Description:
**	Look through the list of include directories for the specified file.
**	If found, replace the given location by a location for the full name.
**	The intent is that the location will be replaced only in the cases
**	in the table below where "Search Dirs" contains a "Y" (that is,
**	only in <>-includes without an absolute pathname), if the file is found.
**	If the file is not found we do nothing and return FALSE so that
**	inc_push_file won't look in the current directory.
**	Note that the name must be expanded before inc_push_file checks to
**	see if we've already included it -- "foo.qc" and <foo.qc> might
**	be different files.
**
**	===============================================================
**	| Inline  | <> | Path || Create | Write  | Generate  | Search |
**	| Include | "" |      ||  File  | Output | #include  |  Dirs  |
**	|=============================================================|
**	|    0    | "" | Rel  ||   Y    |    Y   |     Y     |   N    |
**	|    0    | "" | Abs  ||   Y    |    Y   |     Y     |   N    |
**	|    0    | "" | None ||   Y    |    Y   |     Y     |   N    |
**	|    0    | <> | Rel  ||   N    |    N   |     Y     |   Y    |
**	|    0    | <> | Abs  ||   N    |    N   |     Y     |   N    |
**	|    0    | <> | None ||   N    |    N   |     Y     |   Y    |
**	|-------------------------------------------------------------|
**	|    1    | "" | Rel  ||   N    |    Y   |     N     |   N    |
**	|    1    | "" | Abs  ||   N    |    Y   |     N     |   N    |
**	|    1    | "" | None ||   N    |    Y   |     N     |   N    |
**	|    1    | <> | Rel  ||   N    |    Y   |     N     |   Y    |
**	|    1    | <> | Abs  ||   N    |    Y   |     N     |   N    |
**	|    1    | <> | None ||   N    |    Y   |     N     |   Y    |
**	===============================================================
**
**  Inputs:
**	f_locp		- Pointer to the location to be expanded
**	f_locbufp	- Pointer to its buffer
**	glob_loc	- Pointer to the array of directory location pointers
**
**  Outputs:
**	The contents of f_locp and f_locbufp are changed to the expanded
**	location, if one matches; if not, they are left unchanged.
**
**	Returns:
**	    TRUE if the location was successfully expanded, else FALSE.
**	Errors:
**	    None.
-*
**  Side Effects:
**	If this is not an INLINE include, flag it as a GLOBAL one
**	(don't create an output file, nor write any output).
**  History:
**	02-dec-1986 - Written. (mrw)
*/

i4
inc_expand_include( f_locp, f_locbufp, glob_loc )
LOCATION	*f_locp;
char		*f_locbufp;
LOCATION	**glob_loc;
{
    LOCATION	test_loc;	/* We build test LOCs here */
    char	buf[MAX_LOC+1];
    char	path_buf[MAX_LOC+1];
    bool	ok = TRUE;
    bool	found = FALSE;
    bool	has_path;

    /* Always try to open an absolute path. */
    if (LOisfull(f_locp))
	return TRUE;

#ifdef CMS
    /*
    ** Use of the -h flag is meaningless on CMS.  If the include fileid
    ** has been incorrectly specified, inc_push_file() will report an
    ** error.  Conflicts between the use of "" and <> delimiters will not
    ** be resolved without further work in this routine (wolf).
    */
	return TRUE;
#endif /* CMS */

    /*
    ** No expansion possible if there are no include directories.
    */
    if (*glob_loc == NULL)
	return FALSE;

    /* Did the user supply a (relative) path? */
    LOdetail( f_locp, buf, path_buf, buf, buf, buf );
    has_path = path_buf[0] != '\0';

    /*
    ** Initialize test_loc so LOaddpath will work
    */
    LOcopy( f_locp, buf, &test_loc );

    /*
    ** We'll see if the file exists in the first dir,
    ** then the next, and so on.  Stop when we find it, or when we run out
    ** of directories to search, or on error.
    ** We assume that if we can find it then we can read it.
    ** Note that we don't look in the current dir (unless that was
    ** specified in a "-h" argument).
    */

    do	/* for each directory in the include list */
    {
	/*
	** If there's no path component then copy in the next prefix,
	** else append it.
	** If LOaddpath succeeds, the result will also have the original
	** filename.  If it fails then drop out.
	** LOaddpath can fail if f_locp is an absolute path,
	** but we already checked for that.
	*/
	if (has_path)
	{
	    if (LOaddpath(*glob_loc, f_locp, &test_loc) != OK)
		ok = FALSE;
	} else
	{
	    LOcopy(*glob_loc, buf, &test_loc);

	    /*
	    ** If we can't set the filename from f_loc, then something
	    ** is wrong, and will continue to be wrong.  Get out of the
	    ** loop.
	    */
	    if (LOstfile(f_locp, &test_loc) != OK)
		ok = FALSE;
	}
	glob_loc++;			/* Skip to next dir */

	if (ok)
	    found = (LOexist(&test_loc) == OK);
    } while (ok && !found && *glob_loc);

    /*
    ** Now if we found the include file in one of the include directories
    ** then test_loc contains a LOCATION that exists,
    ** and which may not be the original location.
    ** Replace the user's location with ours.
    ** Our caller will continue as if nothing ever happened.
    */
    if (ok && found)
    {
	LOcopy( &test_loc, f_locbufp, f_locp );
	return TRUE;
    }
    return FALSE;
}

/*{
+*  Name: inc_isdir - Is this location a directory?
**
**  Description:
**	A pleasant interface to LOisdir that works around a bug in all known
**	implementations.  LOisdir is documented to want a (i4 *); in fact
**	it wants an (i2 *).  This call must correspond to the LOisdir
**	implementation -- be forewarned!
**
**  Inputs:
**	loc		- The location to test.
**
**  Outputs:
**	Returns:
**	    TRUE if this is a directory, else FALSE.
**	Errors:
**	    None.
-*
**  Side Effects:
**	
**  History:
**	02-dec-1986 - Written. (mrw)
*/

bool 
inc_isdir( loc )
LOCATION	*loc;
{
    i2		isdir = 0;

    if (LOisdir(loc, &isdir) != OK)
	return FALSE;

    if (isdir == ISDIR)
	return TRUE;

    return FALSE;
}


/*{
+*  Name: inc_save - Save parent location for remote include file processing.
**
**  Description:
**	Saves parent location passed by eqmain.c when parent location has 
**	path as well as file name.  This is the location of the file which 
**	was given as an argument to the pre-processor. 
**		EX:  "ESQL dir1/dir2/filename.ext"
**	We need to save this location so we can look in this 
**	directory ("dir1/dir2") for local include files.
**	
**  Inputs:
**	loc		- The location to save.
**
**  Outputs:
**	Returns:
**	    Nothing.
**	Errors:
**	    None.
-*
**  Side Effects:
**	Sets p_loc and p_buf if parent has path name.
**	p_loc and p_locbuf remain NULL when only a filename is given as the
**	argument to the pre-processor.
**	
**  History:
**	15-sep-1988 - Written (ncg)
**	19-oct-1990  (kathryn)
**		Always save parent path if there is one. (Not just for eqc -c).
*/

void
inc_save(loc)
LOCATION	*loc;
{
    char	f_path[MAX_LOC +1];
    char	f_node[MAX_LOC +1];
    char	dumbuf[MAX_LOC +1];

    	p_locbuf[0] = EOS;		
	LOdetail(loc, f_node, f_path, dumbuf, dumbuf, dumbuf);

	if (f_node[0] != EOS || f_path[0] != EOS) 
		LOcopy(loc, p_locbuf, &p_loc);	
}
