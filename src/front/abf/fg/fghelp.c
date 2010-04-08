/*
**	Copyright (c) 1989, 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<cm.h>
# include	<lo.h>
# include	<st.h>
# include	<nm.h>
# include       <oocat.h>
# include	<ooclass.h>
# include	<si.h>
# include       <abclass.h>
# include	<metafrm.h>
# include	<erfg.h>
# include	"framegen.h"

/**
** Name:	fghelp.c - generate help_forms statement & copy help file.
**
** Description:
**	This routine processes a ##GENERATE HELP statement, and writes out:
**	  1. one or more help files to the application source directory with
**	the name "source file name" + .hlp, .hlq or .hla. These help files
**	are copies of the help file name listed on the GENERATE HELP statement,
**	and must exist in the II_CONFIG/language directory.
**
**	  2. A HELPFILE statement in the generated 4gl file that points to
**	the above help file in the source directory (each help file written
**	gets its own HELPFILE statement).
**	
**	If a help file already exists, then a new one is not copied.
**	----------------------------------------------------------------
**
**	Anticipated problems with this method of copying help files:
**	
**	  1. If user changes frame type
**	  after codegen, but doesn't change source file name, then code
**	  generator will see that help files already exist for the frame,
**	  and won't copy new ones. But the existing help files for that
**	  frame are the wrong ones!
**
**	  2. If code and help files are generated, and user edits the
**	  help file, and then changes the source file name, then we will
**	  generate new help files and point the HELPFILE statements at
**	  them. User will then have to rename their edited help files.
**
**	There are a couple cases that require us to check for
**	the existance of a help file every time a frame is generated, rather
**	than just the first time. It appears to not be a significant performance
**	hit to generate the help files every time we generate 4gl.
**
**		We base the help file name on the source file name; if
**		the user changes source file name, we need to generate
**		new .hlp files (and we won't know that the source
**		file name has changed). Otherwise, for a previously
**		generated frame, we'll generate a HELPFILE statement
**		pointing to the (presumed) existing .hlp file with that
**		name -- but there won't be one.
**
**		VQ options (esp Insert master allowed) can make
**		entire menus come and go, which could cause the code
**		generator to process a GENERATE HELP statement, that
**		it has never seen before).
**		
**		Worse yet, some menuitems come and go based on VQ options
**		and $vars, such as $insert_master_allowed or
**		$delete_master_allowed. If the user hasn't changed the
**		help file, then we can safely regenerate
**		the file, otherwise not. But, unfortunately, we don't track
**		the date when we last generated a help file. Would be
**		nice if $vars could be used inside the help files to restrict
**		or include sections of the help file. (This should be a
**		stretch goal, as should GENERATE USER_MENUITEM_HELP. In the
**		mean time, we'll generate .hlp files with menuitems 
**		that don't exist on the menuline (user must edit and remove).
**
**  Possible future enhancements:
**	Treat the .osq and .hlp files as one code generation:
**	give .hlp files temp names & keep a stack of them along with
**	.osq file name -- then when all done rename everything in stack.
**
**	Write the date generated into a comment line in the generated help
**	file. Then, generating again, if the help file already exists, compare
**	this internal date/time with that for the help file. If it indicates
**	that the help file has been changed, then do not create a new help
**	file; otherwise create a new help file and clobber the old one.
**
**	This file defines:
**	IIFGhelp
**
** History:
**	5/17/89 (pete)	written.
**	3/15/90 (pete)	change HELP_FORMS to HELPFILE
**	1-may-91 (blaise)
**		Tidied up code: changed a while loop to an if statement to
**		prevent compiler warnings.
**	11-Feb-93 (fredb)
**	   Porting change for hp9_mpe: SIfopen is required for additional
**	   control of file characteristics in IIFGhelp.
**	22-Feb-93 (fredb)
**	   Porting change for hp9_mpe: Look for help files in II_HELPDIR
**	   instead of II_CONFIG.  Lack of directory depth makes ABF/VISION
**	   much more interesting to port ...
**	23-Feb-93 (fredb)
**	   Porting change for hp9_mpe: Major surgery in the way the help
**	   filenames are generated due to the MPE/iX file system and the
**	   limitations of LO.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/* # define's */
/* GLOBALDEF's */

/* extern's */
GLOBALREF FILE *outfp;
GLOBALREF LOCATION IIFGcl_configloc;    /* LOCATION for II_CONFIG/II_LANGUAGE */
					/* NOTE: For hp9_mpe, this is really  */
					/* just FILES.II_SYSTEM               */
GLOBALREF METAFRAME *G_metaframe;

/* static's */
static VOID bldHelpFileName();

STATUS
IIFGhelp(nmbr_words, p_wordarray, p_wbuf, infn, line_cnt)
i4	nmbr_words;
char	*p_wordarray[];
char	*p_wbuf;
char	*infn;		/* name of input file currently processing */
i4	line_cnt;	/* current line number in input file */
{
	USER_FRAME *p_uf = (USER_FRAME *) G_metaframe->apobj ;

	/*
	** NOTE: For hp9_mpe, tmploc_config and tmpbuf_config contain
	** II_HELPDIR instead of II_CONFIG.
	*/
	LOCATION tmploc_config;
	char  tmpbuf_config[MAX_LOC+1]; /* II_CONFIG */
	LOCATION tmploc_source;
	char  tmpbuf_source[MAX_LOC+1]; /* source directory */
	char  srcfn_tmpbuf[MAX_LOC+1];  /* source file name */
	LOCATION tmploc;
	char  helpfn_tmpbuf[MAX_LOC+1];	/* associated with above LOC */
	char  fprefix[MAX_LOC+1];	/* prefix part of source filename */
	char *hlp_path;			/* path of help file in source dir */
	char  dmy[MAX_LOC+1];		/* used in LOdetail call */
	FILE  *helpout;			/* write help file here */

	bool copyError = FALSE;
	bool printErrMsg = FALSE;

        char double_remark[(OOSHORTREMSIZE*2) +1]; /* Buffer with enough
                                                ** room to double any single
                                                ** quotes in short remark.
                                                */
        if (nmbr_words == 0)    /* "##GENERATE HELP" already stripped*/
        {
            IIFGerr (E_FG004D_TooFewArgs, FGCONTINUE, 2, (PTR) &line_cnt,
                        (PTR) infn);
	    copyError = TRUE;
	    printErrMsg = TRUE;
            /* A warning; do not return FAIL -- processing can continue. */
        }
	else if (nmbr_words > 1)
        {
            IIFGerr (E_FG004E_TooManyArgs, FGCONTINUE, 2, (PTR) &line_cnt,
                        (PTR) infn);
            /* A warning; do not return FAIL -- processing can continue. */
        }

	if (nmbr_words > 0)
	{
#ifdef hp9_mpe
	    /* Get value of II_HELPDIR; If undefined, LOCATION will be empty */

	    /* "Borrow" hlp_path pointer for a few lines ... */
	    NMgtAt(ERx("II_HELPDIR"), &hlp_path);

	    tmpbuf_config[0] = EOS;
	    STcopy(hlp_path, tmpbuf_config);
	    LOfroms(PATH, tmpbuf_config, &tmploc_config);
#else
	    /* make copy of II_CONFIG location. */
	    LOcopy (&IIFGcl_configloc, tmpbuf_config, &tmploc_config);
#endif

	    /* make help file name into a LOCATION */
	    STcopy (p_wordarray[0], helpfn_tmpbuf);
	    LOfroms (FILENAME, helpfn_tmpbuf, &tmploc);

	    /* add help file name to II_CONFIG LOCATION */
	    LOstfile (&tmploc, &tmploc_config);

	    /* see if help file exists in II_CONFIG */
	    if (LOexist (&tmploc_config) != OK)
	    {
                IIFGerr (E_FG004F_BadHelpFileName, FGCONTINUE, 3,
			(PTR) &line_cnt, (PTR) infn, (PTR) p_wordarray[0]);

		copyError = TRUE;
	        printErrMsg = TRUE;
	    }
	    else
	    {
        
		/* Assertion: help file exists in II_CONFIG/english.
        	** Location structure, tmploc_config, that points to it is
		** all set.
		**
		** NOTE: For hp9_mpe, we're using II_HELPDIR instead of
		** II_CONFIG.  The english subdir is not even possible
		** with the MPE/iX file system.
		*/
        
#ifdef hp9_mpe
	/*
	** It seems that the first result of creating a number of locations
	** and manipulating them is to isolate the source filename without
	** an extension.  For hp9_mpe, we'll copy the source filename to
	** the final buffer the original code is using and set the end of
	** the filename string at the dot before the extension.
	*/

		char	*cp = NULL;

        	STcopy (p_uf->source, fprefix);
		cp = STindex (fprefix, ERx("."), 0);
		if (cp != NULL)
		   *cp = EOS;

#else
        	/* make source directory into a LOCATION */
        	STcopy (p_uf->appl->source, tmpbuf_source);
        	LOfroms (PATH, tmpbuf_source, &tmploc_source);
    
        	/* make source file name into a (temp) LOCATION */
        	STcopy (p_uf->source, srcfn_tmpbuf);
        	LOfroms (FILENAME, srcfn_tmpbuf, &tmploc);
    
        	/* add (fn.osq) source filename to tmploc_source */
        	LOstfile (&tmploc, &tmploc_source);
    
        	/* extract filename only (no extension) from tmploc_source */
        	LOdetail (&tmploc_source, dmy, dmy, fprefix, dmy, dmy);
#endif
    
        	/* Build help file name, given help file name in II_CONFIG. */
        	bldHelpFileName (p_wordarray[0], fprefix);
        
#ifdef hp9_mpe
	/*
	** Now we want to combine the help file name (now contained in
	** 'fprefix') with the source directory (account) to form a
	** fully qualified filename in a LOCATION.
	*/

		STpolycat (3, fprefix, ERx("."), p_uf->appl->source,
			   tmpbuf_source);
		LOfroms (PATH & FILENAME, tmpbuf_source, &tmploc_source);

#else
        	/* make help file name into a (temp) LOCATION */
        	LOfroms (FILENAME, fprefix, &tmploc);
        
        	/* add fprefix to source LOCATION */
        	LOstfile (&tmploc, &tmploc_source);
#endif
        
        	/* see if help file exists in source directory */
        	if (LOexist (&tmploc_source) == OK)
        	{
      		    /* file already exists; do not copy it. must have
      		    ** been copied the last time code was generated.
        	    ** NOTE: there is no need to LOpurge() the help files, since
        	    ** we never create multiple versions of them.
        	    ** 
        	    */
        	}
        	else	/* file doesn't exist; copy it */
        	{
      		    /* open target help file in source directory. new file. */
#ifdef hp9_mpe
        	    if (SIfopen (&tmploc_source, ERx("w"), SI_TXT, 252, 
				 &helpout) == OK)
#else
        	    if (SIopen (&tmploc_source, ERx("w"), &helpout) == OK)
#endif
        	    {
        		/* copy file from II_CONFIG to source directory. */
        		if (SIcat (&tmploc_config, helpout) != OK)
        		{
        		    SIclose (helpout);
       			    copyError = TRUE;	/* error */
        		}
			else
			{
        		    SIclose (helpout);
			}
        	    }
        	    else
        	    {
			copyError = TRUE;
        	    }
        	}
        	/* get full path name of help file in source directory
        	** to use in HELPFILE statement.
        	*/
        	if (!copyError)
        	{
        	    LOtos(&tmploc_source, &hlp_path);
        	}
	    }
	}

	if ((copyError) && !(printErrMsg))
            IIFGerr (E_FG0050_GenericCopyErr, FGCONTINUE, 3,
		(PTR) &line_cnt, (PTR) infn, (PTR) p_wordarray[0]);

        IIFGdq_DoubleUpQuotes( p_uf->short_remark, double_remark);

	SIfprintf(
		outfp,
	        ERx("%sHELPFILE '%s'\n%s\t'%s';\n"),
	        p_wbuf, double_remark, p_wbuf,
		((nmbr_words == 0) || (copyError)) ? ERx("") : hlp_path
		);

	return OK;
}

/*{
** Name:	bldHelpFileName - Build help file name for source directory.
**
** Description:
** 	Find last character in 'fn'. Concat that character onto ".hl", and
**	tack that string onto 'fn'. 'fn' must be long enough to concat
**	4 more characters onto it.
**
** Inputs:
**	char *help_fn   name of help file on GENERATE HELP stmt.
**	char *fn	name of help file in source directory (with no
**				file extension).
**
** Outputs:
**	'fn' with, for example, ".hlp" concatenated onto it.
**
**	Returns:
**		NONE
**
**	Exceptions:
**		NONE
**
** Side Effects:
**
** History:
**	12/18/89 (pete)	Initial Version.
*/
static VOID
bldHelpFileName (help_fn, fn)
register char *help_fn;
register char *fn;
{
	char last;
	char *p_char;
	char *help_extension = ERx(".hlp");
	i4 i;

	p_char = STindex(help_fn, ERx("."), 0);

	if (p_char > help_fn)
	{
	    /* found a "." after first character */

	    /* back up one and pick off a character */
	    p_char = CMprev (p_char, help_fn);
		
	    last = *p_char;

	    STcat(fn, help_extension);
	    i = STlength (fn);

	    fn[i-1] = last;
	}
	else
	    STcat(fn, help_extension);
}
