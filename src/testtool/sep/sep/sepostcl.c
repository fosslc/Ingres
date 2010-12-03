/*
    Include files
*/
#include <compat.h>
#include <cm.h>
#include <er.h>
#include <lo.h>
#include <me.h>
#include <mh.h>
#include <ex.h>
#include <pc.h>
#include <si.h>
#include <st.h>
#include <tc.h>
#include <te.h>
#include <cv.h>
#include <termdr.h>

#define sepostcl_c

#include <sepdefs.h>
#include <sepfiles.h>
#include <septxt.h>
#include <termdrvr.h>
#include <token.h>
#include <fndefs.h>

/*
** History:
**	16-aug-1989 (boba)
**		Use global, not local, tc.h.
**	17-aug-1989 (eduardo)
**		Fix some mis-named temporary files.
**	29-Aug-1989 (eduardo)
**		Added new parameter to getLocation (type of location
**		being parsed)
**	6-Nov-1989 (Eduardo)
**		Added performance improvement
**	7-Feb-1990 (Eduardo)
**		Fixed bug by which 'setenv VAR newval' did not overwrite
**		the old value for VAR
**	19-jul-1990 (rog)
**		Change include files to use <> syntax because the headers
**		are all in testtool/sep/hdr.
**	06-aug-1990 (rog)
**		Add two more sepset variables and the unsepset command.
**	24-jun-1991 (donj)
**		Move defines of *_CMMD and FILL_PROMPT to sepdefs.h so they
**		are accessible to other modules to implement conditional
**		execution.
**	30-aug-91 (donj)
**		Added 2nd argument to sepset_cmmd() and unsepset_cmmd() to
**		implement "[UN]SEPSET TRACE [TOKENS|LEX]>". When tracing flag
**		is flipped on from off state, trace_nr is incremented and
**		tracing file is opened, using trace_nr as part of filename.
**		So, if you turn tracing on and off and on, you'll get two
**		trace files, TRACE1.STF and TRACE2.STF.
**		Also implemented "[UN]SEPSET PAGING" that mitigates the scroll-
**		problem by clearing the page and resetting the screenLine var.
**		Tried setting mainW->_flags |= _SCROLLWIN to get scrolling
**		working right, but to no avail.
**	14-jan-1992 (donj)
**	    Reformatted variable declarations to one per line for clarity.
**	    Modified all quoted string constants to use the ERx("") macro.
**	    Added explicit declarations for all submodules called. Added
**	    sepset command, "sepset diff_numerics". This turns on numeric
**	    diffing for A_NUMBER (INTEGER) and A_FLOAT token types. Added
**	    "@FILE(" capabilities to the SEP OS/TCL commands: delete, fill,
**	    cd, type, exists, setenv. Replaced separate code with the sub-
**	    routines to translate sepparams.
**	27-jan-1992 (donj)
**	    Add tracing support for sepparam debugging; TRACE_PARM.
**      28-jan-1992 (jefff)
**          Initialized 'Free_mem' to NULL in sep_ostcl().  Can't assume
**          that automatic vars get initialized to NULL.
**      29-jan-1992 (donj)
**          Added fuzz_factor for diff_numerics diffing precision.
**	31-jan-1992 (donj)
**	    Took out TRACE_PARM from the default for SEPSET TRACE. If you want
**	    all trace features, you have to say SEPSET TRACE ALL. UNSEPSET
**	    will still remove all tracing.
**	24-feb-1992 (donj)
**	    Added tracing for SEP to SEPSON dialog to sepset command.
**	02-apr-1992 (donj)
**	    Removed unneeded references. Changed all refs to '\0' to EOS. Changed more char pointer
**          increments (c++) to CM calls (CMnext(c)). Changed a "while (*c)"
**          to the standard "while (*c != EOS)". Implement other CM routines
**          as per CM documentation.
**	10-jun-1992 (donj)
**	    Free_mem not needed anymore. Using Tagged memory blocks
**	05-jul-1992 (donj)
**	    Added cl ME memory tags to use MEtfree() to make memory handling
**	    simpler. Added process dialog and SEP CM wrapper tracing.
**	26-jul-1992 (donj)
**	    Switch MEreqmem() with SEP_MEalloc().
**	 1-sep-1992 (donj)
**	    Added tracing for SEP's grammar2 functions to sepset command.
**	30-nov-1992 (donj)
**	    Elaborate on SEPSET command for the SED masking enhancements.
**	    Add sepset (and unsepset) grammar (sed|lex|cm) [(results|canon)]
**	01-dec-1992 (donj)
**	    Sun4 needed a FUNC_EXTERN for SEP_CMlastchar().
**	    Moved typedefs for FILLSTACK to sepdefs.h. Created GLOBALREFS
**	    for fillstack pointers here, and GLOBALDEFS in septool.c.
**	    Rewrote fill_cmmd() and del_fillfiles(). Also added constant
**	    definitions for return codes for SIopen(), SI_BAD_SYNTAX,
**	    and SI_CANT_OPEN.
**	04-sep-1993 (donj)
**	    Took out sepset stuff for GRAMMAR_CM (grammar2 functions).
**      15-feb-1993 (donj)
**          Add a ME_tag arg to SEP_LOfroms.
**	09-mar-1993 (donj)
**	    Set loctype to NULL to let SEP_LOfroms() determine what type
**	    the string is.
**	24-mar-1993 (donj)
**	    Alter fuzz factor to new format.
**	26-mar-1993 (donj)
**	    Change exp10() call to pow() call. exp10() doesn't exist on
**	    VAX/VMS.
**      26-mar-1993 (donj)
**          Change pow() call to MHpow() call. Use the CL, Use the CL, Use
**          the CL, Use the CL, Use the CL.
**	26-mar-1993 (donj)
**	    Protect VMS from call to CVan for a "1e-01" type float.
**	 5-may-1993 (donj)
**	    Add function prototypes, init some pointers.
**	 7-may-1993 (donj)
**	    Add more prototyping.
**       7-may-1993 (donj)
**          Fix SEP_MEalloc().
**      25-may-1993 (donj)
**          Add initializations that QA_C complained about.
**       2-jun-1993 (donj)
**          Isolated Function definitions in a new header file, fndefs.h. Added
**          definition for module, and include for fndefs.h.
**      26-aug-1993 (donj)
**          Fix a problem with abort_if_matched feature.
**	 9-sep-1993 (donj)
**	    Add an unsetenv command.
**      15-oct-1993 (donj)
**	    Make function prototyping unconditional. Add functionality to
**	    sepset_cmmd() and unsepset_cmmd(). To manipulate the TokenID
**	    bit masks so user can turn on and off some types of masking.
**      30-dec-1993 (donj)
**          Added GLOBALREF for screenLine. They're used by the disp_line() macros.
**	28-jan-1994 (donj)
**	    Added new A_PRODUCT token to mask for matching "INGRES" and "OpenINGRES"
**	13-apr-1994 (donj)
**	    Fixed an bad parameter being passed to SIclose().
**	    Was passing an _iobuf ** instead of an _iobuf *.
**	20-apr-1994 (donj)
**	    Added another token type, A_COMPILE, for those pesky ABF msgs,
**	    "Compiling 100012.c".
**	10-may-1994 (donj)
**	    Fixed a typo.
**	31-may-1994 (donj)
**	    Took out #include for sepparam.h. QA_C was complaining about all
**	    the unecessary GLOBALREF's.
**	24-Jul-1996 (somsa01)
**	    On line 725, the second argument to LOtos should be "&cptr", not
**	    "cptr".
**	28-Jul-1998 (lamia01)
**		Added new version identifiers, Computer Associates as the company name,
**		and NT directory masking. Version identifier also works now with DBL.
**		New tokens - Database ID, Server ID, Alarm ID, Transaction ID and
**		Checkpoint ID introduced to avoid unnecessary diffs.
**	28-Apr-1999 (johco01)
**		Added checks into sepset_cmmd and unsepset_cmmd so when 
**		processing sepset commands pointed to by SEP_SET no attempt
**		is made to write to stdout, which was causing sep to core
**		dump.
**		The core dump occured because sepResults does not exist at
**		this point and an attempt to write to it is made.
**	05-Mar-1999 (ahaal01)
**		Moved "include ex.h" after "include mh.h".  The check for
**		"ifdef EXCONTINUE" in ex.h does no good if sys/context.h
**		(via mh.h) is included after ex.h.
**	25-jul-1999 (popri01)
**	    Now using sep's sed feature for checkpoint id, which is specific to LAR.
**	    (See grammar.lex for additional explanation.)
**	    Also, add support for "@file" notation for sed files so that we don't
**	    need to copy them to the current directory.
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      05-Sep-2000 (hanch04)
**          replace nat and longnat with i4
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Include cv.h for function defintions
**	1-Dec-2010 (kschendel)
**	    Fix compiler warnings.
*/

/*
** Global variables
*/

GLOBALREF    WINDOW       *mainW ;
GLOBALREF    WINDOW       *statusW ;

GLOBALREF    char          buffer_1 [SCR_LINE] ;
GLOBALREF    char         *lineTokens [SCR_LINE] ;
GLOBALREF    char          cont_char[] ;
GLOBALREF    char          SEPpidstr [] ;

GLOBALREF    i4            screenLine ;
GLOBALREF    i4            tracing ;

GLOBALREF    FILE         *traceptr ;

GLOBALREF    LOCATION     *SED_loc ;

GLOBALREF    bool          grammar_methods ;
GLOBALREF    bool          diff_numerics ;
GLOBALREF    bool          shellMode ;
GLOBALREF    bool          batchMode ;
GLOBALREF    bool          updateMode ;
GLOBALREF    bool          paging ;

GLOBALREF    f8            fuzz_factor ;

GLOBALREF    FILLSTACK    *fill_stack ;
GLOBALREF    FILLSTACK    *fill_stack_root ;
GLOBALREF    FILLSTACK    *fill_temp ;

GLOBALREF    long          A_always_match ;
GLOBALREF    long          A_word_match ;

GLOBALREF    SEPFILE      *sepResults ;


static bool                nodelete = FALSE ;
i4                         dummynat = 0 ;

/*
**	SI errors that are found in cl!clf!si SI.h
*/

# define SI_BAD_SYNTAX	(E_CL_MASK | E_SI_MASK | 1)
# define SI_CANT_OPEN	(E_CL_MASK | E_SI_MASK | 4)


STATUS
sep_ostcl(FILE  *testFile,char **argv,i4 cmmdID)
{
    char                  *nargv = NULL ;
    char                  *cptr  = NULL ;

    STATUS                 syserr ;
    STATUS                 to_diff = OK ;

    register i4            i ;
    i4                     len ;

    if (argv[0] == NULL || *argv[0] == EOS)
    {
	disp_line(ERx("ERROR: OS/TCL command is missing from command line"),0,0);
	return(FAIL);
    }
    /* check for SEP parameters and strings enclosed  by " */

    nargv = (cmmdID == SETENV_CMMD ? argv[2] : argv[1]);

    if (IS_SEPparam(nargv))
    {
	if ((syserr = Trans_SEPparam(&nargv,TRUE,SEP_ME_TAG_NODEL)) != OK)
	    return(syserr);
    }
    else
    {
	len  = STlength(nargv);
	cptr = nargv;
	for (i=1; i<len; i++)
	    CMnext(cptr);

	if (nargv&&(CMcmpcase(nargv,DBL_QUO) == 0)&&
	    (CMcmpcase(cptr,DBL_QUO) == 0))
	{
	    *cptr = EOS;
	    CMnext(nargv);
	}
    }

    /* rewind results file */

    SEPrewind(sepResults, FALSE);

    /* identify and process OC/TCL command */

    to_diff = FAIL;
    switch(cmmdID)
    {
       case TYPE_CMMD:     to_diff = type_cmmd(nargv);			  break;
       case EXISTS_CMMD:   to_diff = exists_cmmd(nargv);		  break;
       case DELETE_CMMD:   to_diff = do_delete_cmmd(nargv);		  break;
       case FILL_CMMD:     fill_cmmd(testFile,nargv);			  break;
       case CD_CMMD:       to_diff = cd_cmmd(nargv);			  break;
       case SETENV_CMMD:   to_diff = setenv_cmmd(argv[1],nargv, TRUE);    break;
       case UNSETENV_CMMD: to_diff = unsetenv_cmmd(argv[1], TRUE);        break;
       case SEPSET_CMMD:   to_diff = sepset_cmmd(nargv, argv[2], argv[3]);break;
       case UNSEPSET_CMMD: to_diff = unsepset_cmmd(nargv, argv[2], argv[3]);
									  break;

       default:	disp_line(ERx("ERROR: unknown OS/TCL command"),0,0);
    }

    MEtfree(SEP_ME_TAG_PARAM);

    /* signal end of file and flush results file */

    SEPputc(SEPENDFILE, sepResults);
    SEPrewind(sepResults, TRUE);

    /* diff output obtained if appropiate */

    syserr = OK;

    if (to_diff == OK)
    {
	if (shellMode)
	    create_canon(sepResults);
	else
	{
	    syserr = diffing(testFile,1,0);
	}
    }

    return(syserr);
}


STATUS
do_delete_cmmd(char *filenm)
{
    LOCATION               aLoc ;
    char                  *fileBuf = NULL ;
    LOCTYPE                aLocType = 0 ;
    bool                   is_ok ;

    fileBuf = SEP_MEalloc(SEP_ME_TAG_NODEL, MAX_LOC+1, TRUE, (STATUS *)NULL);
    if (is_ok = IS_FileNotation(filenm))
	if (Trans_SEPfile(&filenm,NULL,TRUE,SEP_ME_TAG_NODEL) != OK)
	    is_ok = FALSE;

    STcopy(filenm, fileBuf);
    SEP_LOfroms(aLocType,fileBuf,&aLoc, SEP_ME_TAG_NODEL);

    if (!SEP_LOexists(&aLoc))
    {
	STcopy(ERx("file was not found\n"),buffer_1);
	SEPputrec(buffer_1, sepResults);
    }
    else
    {
	if (LOdelete(&aLoc) == OK)
	{
	    STcopy(ERx("file was deleted\n"),buffer_1);
	    SEPputrec(buffer_1, sepResults);
#ifdef VMS
	    while (SEP_LOexists(&aLoc))
		LOdelete(&aLoc);
#endif
	}
	else
	{
	    STcopy(ERx("file was not deleted\n"),buffer_1);
	    SEPputrec(buffer_1, sepResults);
	}
    }
    MEfree(fileBuf);
    return(OK);
}





STATUS
exists_cmmd(char *filenm)
{
    LOCATION               aLoc ;
    char                  *fileBuf = NULL ;
    LOCTYPE                aLocType = 0 ;
    bool                   is_ok ;

    fileBuf = SEP_MEalloc(SEP_ME_TAG_NODEL, MAX_LOC+1, TRUE, (STATUS *)NULL);
    if (is_ok = IS_FileNotation(filenm))
	if (Trans_SEPfile(&filenm,NULL,TRUE,SEP_ME_TAG_NODEL) != OK)
	    is_ok = FALSE;

    STcopy(filenm, fileBuf);
    SEP_LOfroms(aLocType,fileBuf,&aLoc, SEP_ME_TAG_NODEL);

    if (SEP_LOexists(&aLoc))
    {
	STcopy(ERx("file was found\n"),buffer_1);
	SEPputrec(buffer_1, sepResults);
    }
    else
    {
	STcopy(ERx("file was not found\n"),buffer_1);
	SEPputrec(buffer_1, sepResults);
    }
    MEfree(fileBuf);
    return(OK);

}





STATUS
cd_cmmd(char *directory)
{
    LOCATION               aLoc ;
    char                   dirBuf [MAX_LOC] ;
    char                  *dirPtr  = NULL ;
    i2                     isadir ;
    LOCTYPE                aLocType = 0 ;
    bool                   is_ok ;

    if (is_ok = IS_FileNotation(directory))
	if (Trans_SEPfile(&directory,NULL,TRUE,SEP_ME_TAG_NODEL) != OK)
	    is_ok = FALSE;

    STcopy(directory, dirBuf);

    if (!is_ok)
    {
	CVupper(dirBuf);
	NMgtAt(dirBuf, &dirPtr);
	if (dirPtr == NULL || *dirPtr == EOS)
	{
	    disp_line(ERx("Illegal format for directory name\n"),0,0);
	    return(FAIL);
	}
	else
	{
	    STcopy(dirPtr, dirBuf);
	}
    }
    SEP_LOfroms(aLocType, dirBuf, &aLoc, SEP_ME_TAG_NODEL);

    LOisdir(&aLoc, &isadir);
    if (SEP_LOexists(&aLoc) && (isadir == ISDIR))
    {
	if (LOchange(&aLoc) == OK)
	    SEPputrec(ERx("moved to new directory\n"), sepResults);
	else
	    SEPputrec(ERx("could not move to new directory\n"), sepResults);
    }
    else
	SEPputrec(ERx("directory was not found\n"), sepResults);

    return(OK);
}

STATUS
unsetenv_cmmd(char *logical, bool bflag)
{
    STATUS                 ret_val = OK ;

    ret_val = undefine_env_var(logical);
    if (bflag == TRUE)
    {
        if (ret_val != OK)
            STcopy(ERx("environment variable could not be undefined\n"),buffer_1);
        else
            STcopy(ERx("environment variable was undefined\n"),buffer_1);

        SEPputrec(buffer_1, sepResults);
    }

    return (ret_val);
}

STATUS
setenv_cmmd(char *logical,char *value,bool bflag)
{
    STATUS                 ret_val ;
    char                  *thevalue = NULL ;
    char                   valueBuf [MAX_LOC] ;
    bool                   is_ok ;

    thevalue = value;
    if (IS_SEPparam(thevalue))
	if (Trans_SEPparam(&thevalue,TRUE,SEP_ME_TAG_NODEL) != OK)
	    value = NULL;

    if (logical == NULL || *logical == EOS)
    {
	disp_line(ERx("ERROR: name of environment variable is missing from command line"),0,0);
	return(FAIL);
    }

    if (is_ok = IS_FileNotation(thevalue))
	if (Trans_SEPfile(&thevalue,NULL,TRUE,SEP_ME_TAG_NODEL) != OK)
	    is_ok = FALSE;

    STcopy(thevalue, valueBuf);

    ret_val = define_env_var(logical, valueBuf);
    if (bflag == TRUE)
    {
	if (ret_val != OK)
	    STcopy(ERx("environment variable could not be defined\n"),buffer_1);
	else
	    STcopy(ERx("environment variable was defined\n"),buffer_1);

	SEPputrec(buffer_1, sepResults);
    }
    return(OK);
}


STATUS
type_cmmd(char *filenm)
{
    STATUS                 ioerr ;
    LOCATION               aLoc ;
    FILE                  *aPtr = NULL ;
    char                   lastchar = EOS ;
    char                  *fileBuf = NULL ;
    LOCTYPE                aLocType = 0 ;
    i2                     isitafile ;
    bool                   is_ok ;

    fileBuf = SEP_MEalloc(SEP_ME_TAG_NODEL, MAX_LOC+1, TRUE, (STATUS *)NULL);
    if (is_ok = IS_FileNotation(filenm))
	if (Trans_SEPfile(&filenm,NULL,TRUE,SEP_ME_TAG_NODEL) != OK)
	    is_ok = FALSE;

    STcopy(filenm, fileBuf);
    SEP_LOfroms(aLocType,fileBuf,&aLoc, SEP_ME_TAG_NODEL);
    LOisdir(&aLoc, &isitafile);

    if (SEP_LOexists(&aLoc) && (isitafile == ISFILE))
    {
	if (SIopen(&aLoc,ERx("r"),&aPtr) != OK)
	{
	    STcopy(ERx("file could not be opened"),buffer_1);
	    SEPputrec(buffer_1, sepResults);
	}
	else
	{
/*
**	    Eduardo 28/12/89
**	    Changed size from TERM_LINE to SCR_LINE
*/
	    while ((ioerr = SIgetrec(buffer_1,SCR_LINE,aPtr)) == OK)
	    {
		SEPputrec(buffer_1, sepResults);
		lastchar = buffer_1[STlength(buffer_1) - 1];
	    }
	    if (ioerr != ENDFILE)
	    {
		STcopy(ERx("ERROR: failed while reading in file"),buffer_1);
		SEPputrec(buffer_1, sepResults);
	    }
	    if (lastchar != '\n')
		SEPputc('\n', sepResults);
	    SIclose(aPtr);
	}
    }
    else
    {
	STcopy(ERx("file was not found\n"),buffer_1);	
	SEPputrec(buffer_1, sepResults);
    }
    MEfree(fileBuf);
    return(OK);
}


STATUS
SEP_SET_read_envvar(char *envvar)
{
    STATUS                 ret_val ;
    LOCATION               sepsetloc ;
    FILE                  *sepsetptr = NULL ;
    char                  *set_buffer = NULL ;
    char                  *tcptr1 = NULL ;
    char                  *tcptr2 = NULL ;
    char                  *tcptr3 = NULL ;
    char                  *tcptr4 = NULL ;

    ret_val = OK;

    if (envvar == NULL || *envvar == EOS)
	return (ret_val);

    if (CMcmpnocase(envvar, ERx("@")) == 0)
    {
	if ((ret_val = SEP_LOfroms(FILENAME & PATH, ++envvar, &sepsetloc, SEP_ME_TAG_NODEL))
	    == OK)
	    if ((ret_val = SIopen(&sepsetloc,ERx("r"),&sepsetptr)) == OK)
	    {
		set_buffer = SEP_MEalloc(SEP_ME_TAG_NODEL, TEST_LINE, TRUE,
					 (STATUS *) NULL);
		while ((ret_val = SIgetrec(set_buffer, TEST_LINE, sepsetptr))
		       == OK)
		{
		    tcptr1 = SEP_CMlastchar(set_buffer,0);
		    *tcptr1 = EOS;
		    if ((ret_val = SEP_SET_read_envvar(set_buffer)) != OK)
			break;
		}
		SIclose(sepsetptr);
		if (ret_val == ENDFILE)
		    ret_val = OK;
		MEfree(set_buffer);
	    }
    }
    else
    {
	tcptr1 = envvar;
	tcptr2 = tcptr3 = tcptr4 = NULL;

	if ((tcptr2 = STindex(tcptr1,ERx(" "),0)) != NULL)
	{
	    *tcptr2 = EOS;
	    tcptr2++;
	    if ((tcptr3 = STindex(tcptr2,ERx(" "),0)) != NULL)
	    {
		*tcptr3 = EOS;
		tcptr3++;
		if ((tcptr4 = STindex(tcptr3,ERx(" "),0)) != NULL)
		{
		    *tcptr4 = EOS;
		    tcptr4++;
		}
	    }
	}

	if (tcptr2 == NULL)
	    ret_val = sepset_cmmd(tcptr1, tcptr2, tcptr3);
	else
	{
	    if (STbcompare(tcptr1,0,ERx("setenv"),0,TRUE) != 0)
	    {
		if (STbcompare(tcptr1,0,ERx("sepset"),0,TRUE) != 0)
		    ret_val = sepset_cmmd(tcptr1, tcptr2, tcptr3);
		else
		    ret_val = sepset_cmmd(tcptr2, tcptr3, tcptr4);
	    }
	    else
	    {
		if (tcptr3 == NULL)
		    ret_val = FAIL;
		else
		    ret_val = setenv_cmmd(tcptr2, tcptr3, (bool)FALSE);
	    }
	}
    }
    return (ret_val);
}

STATUS
set_or_clear_mask_bit( bool set_it, char *a, char *ebuf )
{
    STATUS                 ret_val = OK ;
    i4                     l = 0 ;
    long                   f = 0 ;

    /* **************************************************************** *
     A_HEXA A_FLOAT A_NUMBER A_WORD A_OCTAL A_SPACE A_DELIMITER A_CONTROL
     A_RETCOLUMN A_UNKNOWN A_DBLWORD        A_ALWAYS_MATCH  A_WORD_MATCH
     * **************************************************************** */

    l = STlength(a);
    if      (STbcompare(ERx("date"),     4,a,l,TRUE)==0) f = A_DATE;
    else if (STbcompare(ERx("time"),     4,a,l,TRUE)==0) f = A_TIME;
    else if (STbcompare(ERx("drop"),     4,a,l,TRUE)==0) f = A_DROP;
    else if (STbcompare(ERx("rule"),     4,a,l,TRUE)==0) f = A_RULE;
    else if (STbcompare(ERx("user"),     4,a,l,TRUE)==0) f = A_USER;
    else if (STbcompare(ERx("table"),    5,a,l,TRUE)==0) f = A_TABLE;
    else if (STbcompare(ERx("owner"),    5,a,l,TRUE)==0) f = A_OWNER;
    else if (STbcompare(ERx("group"),    5,a,l,TRUE)==0) f = A_GROUP;
    else if (STbcompare(ERx("column"),   6,a,l,TRUE)==0) f = A_COLUMN;
    else if (STbcompare(ERx("banner"),   6,a,l,TRUE)==0) f = A_BANNER;
    else if (STbcompare(ERx("dbname"),   6,a,l,TRUE)==0) f = A_DBNAME;
    else if (STbcompare(ERx("product"),  7,a,l,TRUE)==0) f = A_PRODUCT;
    else if (STbcompare(ERx("version"),  7,a,l,TRUE)==0) f = A_VERSION;
    else if (STbcompare(ERx("destroy"),  7,a,l,TRUE)==0) f = A_DESTROY;
    else if (STbcompare(ERx("compname"), 8,a,l,TRUE)==0) f = A_COMPNAME;
    else if (STbcompare(ERx("directory"),9,a,l,TRUE)==0) f = A_DIRECTORY;
    else if (STbcompare(ERx("copyright"),9,a,l,TRUE)==0) f = A_COPYRIGHT;
    else if (STbcompare(ERx("procedure"),9,a,l,TRUE)==0) f = A_PROCEDURE;
    else if (STbcompare(ERx("compile"),  7,a,l,TRUE)==0) f = A_COMPILE;
    else if (STbcompare(ERx("databsid"), 8,a,1,TRUE)==0) f = A_DATABSID;
    else if (STbcompare(ERx("serverid"), 8,a,1,TRUE)==0) f = A_SERVERID;
    else if (STbcompare(ERx("alarmid"),  7,a,1,TRUE)==0) f = A_ALARMID;
    else if (STbcompare(ERx("tranid"),   6,a,1,TRUE)==0) f = A_TRANID;
    else
    {
	STprintf(ebuf,ERx("ERROR: Illegal value for mask operation <%s>\n"),a);
	SEPputrec(ebuf,sepResults);
	return (FAIL);
    }

    if (set_it)
    {
	A_always_match |=  f;
	A_word_match   &= ~f;
    }
    else
    {
	A_always_match &= ~f;
	A_word_match   |=  f;
    }

    return (ret_val);
}

STATUS
sepset_cmmd (char *argv,char *argv2,char *argv3)
{
    STATUS                 set_or_clear_mask_bit ( bool set_it, char *a, char *ebuf ) ;

    register i4            i ;

    i4                     slen ;
    i4                     arg_len ;
    i4                     arg2_len ;
    i4                     arg3_len ;
    i4                     tmp_flag ;
    i4                    *iptr = NULL ;

    bool                   all_digits ;

    char                  *cptr = NULL ;
    char                  *cptr2 = NULL ;
    char                  *s_buffer = NULL ;

    arg_len = STlength(argv);
    s_buffer = SEP_MEalloc(SEP_ME_TAG_NODEL, TEST_LINE, TRUE, (STATUS *)NULL);

    if (STbcompare(ERx("nodelete"), 8, argv, arg_len, TRUE) == 0)
    {
	nodelete = TRUE;
	SEPputrec(ERx("SEP internal variable 'nodelete' is set.\n"),sepResults);
    }
    else if (STbcompare(ERx("sed"), 3, argv, arg_len, TRUE) == 0)
    {
	if (argv2 != NULL && *argv2 != EOS)
	{
	    if (STbcompare(ERx("results"), 7, argv2, 7, TRUE) == 0)
		tmp_flag = GRAMMAR_SED_R;
	    else if (STbcompare(ERx("canon"), 5, argv2, 5, TRUE) == 0)
		tmp_flag = GRAMMAR_SED_C;
	    else
		tmp_flag = 0;

	    if (tmp_flag == 0)
		cptr2 = argv2;
	    else
		cptr2 = argv3;

	    if (cptr2 != NULL && *cptr2 != EOS)
	    {
		if (SED_loc != NULL)
		   	LOtos(SED_loc,&cptr);
                else
                {
                    cptr = SEP_MEalloc(SEP_ME_TAG_NODEL, sizeof(LOCATION), TRUE,
                                    (STATUS *)NULL);
		    SED_loc = (LOCATION *)cptr;
                    cptr = SEP_MEalloc(SEP_ME_TAG_NODEL, MAX_LOC+1, TRUE,
                      	            (STATUS *)NULL);
                }
		if (IS_FileNotation(cptr2))
		{
        		if (Trans_SEPfile(&cptr2,NULL,TRUE,SEP_ME_TAG_NODEL) == OK)
				STcopy(cptr2, cptr);
			else
				STcopy("", cptr);
		}
		else
			STcopy(cptr2, cptr);
		SEP_LOfroms(FILENAME & PATH, cptr, SED_loc, SEP_ME_TAG_NODEL);
		if (SEP_LOexists(SED_loc))
		{
		    switch (tmp_flag)
		    {
		       case 0:
			    grammar_methods |= (GRAMMAR_SED_R|GRAMMAR_SED_C);
			    break;

		       case GRAMMAR_SED_R:
			    grammar_methods &= ~GRAMMAR_SED_C;
			    grammar_methods |=  GRAMMAR_SED_R;
			    break;

		       case GRAMMAR_SED_C:
			    grammar_methods &= ~GRAMMAR_SED_R;
			    grammar_methods |=  GRAMMAR_SED_C;
			    break;
		    }
		}
		else
		{
		    grammar_methods &= ~(GRAMMAR_SED_R|GRAMMAR_SED_C);
		    MEfree(cptr);
		    MEfree((char *)SED_loc);
		    SED_loc = NULL;
		}
	    }
	}
    }
    else if (STbcompare(ERx("scroll"), 6, argv, arg_len, TRUE) == 0)
    {
	mainW->_scroll = TRUE;
	mainW->_flags |= _SCROLLWIN;
	SEPputrec(ERx("SEP internal variable 'scroll' is set.\n"),sepResults);
    }
    else if (STbcompare(ERx("paging"), 6, argv, arg_len, TRUE) == 0)
    {
	paging = TRUE;
    }
    else if (STbcompare(ERx("mask"), 4, argv, arg_len, TRUE) == 0)
    {
	if (argv2 == NULL || *argv2 == EOS)
	    SEPputrec(ERx("ERROR: No value for sepset mask.\n"), sepResults);
	else
	    set_or_clear_mask_bit(TRUE, argv2, s_buffer);
    }
    else if (STbcompare(ERx("nomask"), 4, argv, arg_len, TRUE) == 0)
    {
        if (argv2 == NULL || *argv2 == EOS)
            SEPputrec(ERx("ERROR: No value for sepset nomask.\n"), sepResults);
        else
            set_or_clear_mask_bit(FALSE, argv2, s_buffer);
    }
    else if (STbcompare(ERx("diff_numerics"), 6, argv, arg_len, TRUE) == 0)
    {
	diff_numerics = TRUE;
	if (argv2 == NULL || *argv2 == EOS)
	    fuzz_factor = 0;
	else
	{
	    for (all_digits = TRUE, cptr = argv2;
		 (*cptr)&&(all_digits); all_digits = CMdigit(cptr++));

	    if (all_digits)
	    {
		if (CVan(argv2,iptr) == OK)
		{
		    if (*iptr > 0)
			fuzz_factor = MHpow(10,(-1)*(fuzz_factor));
		    else
			fuzz_factor = MHpow(10,fuzz_factor);
		}
		else
		{
		    STprintf(s_buffer, ERx("ERROR: Illegal value for fuzz_factor %s.\n")
			     ,argv2);
		    SEPputrec(s_buffer, sepResults);
		    fuzz_factor = 0;
		}
	    }
	    else
	    {
		if (CVaf(argv2,'.',&fuzz_factor) != OK)
		{
		    STprintf(s_buffer, ERx("ERROR: Illegal value for fuzz_factor %s.\n")
			     ,argv2);
		    SEPputrec(s_buffer, sepResults);
		    fuzz_factor = 0;
		}
	    }
	}
    }
    else if (STbcompare(ERx("trace"), 5, argv, arg_len, TRUE) == 0)
    {
	if (tracing == 0)
	    SEP_Trace_Open();

	SEP_Trace_Set(argv2);
    }
    else
    {
	char	*null_it = ERx("\n") ;

	slen = arg_len;
	if (slen > 9)
	{
	    cptr = argv;
	    for (i=0; i<9; i++) CMnext(cptr);
	}
	else
	{
	    cptr = null_it;
	}

	if ((STbcompare(ERx("continue="), 9, argv, 9, TRUE) == 0)
	    && (*cptr != '\n') && (*cptr != EOS))
	{
	    cont_char[0] = *cptr;
	    cont_char[1] = EOS;
	    STprintf(s_buffer,
		     ERx("SEP internal variable 'continue' is set to %s.\n"),
		     cont_char);
	    SEPputrec(s_buffer, sepResults);
	}
	else
	{
	    STprintf(s_buffer, ERx("sepset: unknown variable '%s'.\n"), argv);
	    SEPputrec(s_buffer, sepResults);
	}
    }
    MEfree(s_buffer);
    return(OK);
}


STATUS
unsepset_cmmd (char *argv,char *argv2,char *argv3)
{
    STATUS                 set_or_clear_mask_bit ( bool set_it, char *a, char *ebuf ) ;

    char                  *s_buffer = NULL ;
    i4                     arg_len ;
    i4                     arg2_len ;
    i4                     arg3_len ;

    s_buffer = SEP_MEalloc(SEP_ME_TAG_NODEL, TEST_LINE, TRUE, (STATUS *)NULL);
    arg_len = STlength(argv);

    if (STbcompare(ERx("nodelete"), 8, argv, arg_len, TRUE) == 0)
    {
	nodelete = FALSE;
	SEPputrec(ERx("SEP internal variable 'nodelete' has been unset.\n"),
		  sepResults);
    }
    else if (STbcompare(ERx("sed"), 3, argv, arg_len, TRUE) == 0)
    {
	if (argv2 == NULL || *argv2 == EOS)
	{
	    grammar_methods &= ~(GRAMMAR_SED_R|GRAMMAR_SED_C);
	}
	else
	{
	    arg_len = STlength(argv2);
	    if (STbcompare(ERx("results"), 7, argv2, arg_len, TRUE) == 0)
		grammar_methods &= ~GRAMMAR_SED_R;
	    else if (STbcompare(ERx("canon"), 5, argv2, arg_len, TRUE) == 0)
		grammar_methods &= ~GRAMMAR_SED_C;
	}
    }
    else if (STbcompare(ERx("scroll"), 6, argv, arg_len, TRUE) == 0)
    {
	mainW->_scroll = FALSE;
	mainW->_flags &= ~_SCROLLWIN;
	SEPputrec(ERx("SEP internal variable 'scroll' has been unset.\n"),
		  sepResults);
    }
    else if (STbcompare(ERx("paging"), 6, argv, arg_len, TRUE) == 0)
    {
	paging = FALSE;
    }
    else if (STbcompare(ERx("mask"), 4, argv, arg_len, TRUE) == 0)
    {
        if (argv2 == NULL || *argv2 == EOS)
            SEPputrec(ERx("ERROR: No value for unsepset mask.\n"), sepResults);
        else
            set_or_clear_mask_bit(FALSE, argv2, s_buffer);
    }
    else if (STbcompare(ERx("nomask"), 4, argv, arg_len, TRUE) == 0)
    {
        if (argv2 == NULL || *argv2 == EOS)
            SEPputrec(ERx("ERROR: No value for unsepset nomask.\n"),sepResults);
        else
            set_or_clear_mask_bit(TRUE, argv2, s_buffer);
    }
    else if (STbcompare(ERx("diff_numerics"), 6, argv, arg_len, TRUE) == 0)
    {
	diff_numerics = FALSE;
    }
    else if (STbcompare(ERx("trace"), 5, argv, arg_len, TRUE) == 0)
    {
	if (tracing != 0)
	    SIclose(traceptr);

	SEP_Trace_Clr(argv2);
    }
    else
    {
	if (STbcompare(ERx("continue="), 9, argv, 9, TRUE) == 0)
	{
	    cont_char[0] = '-';
	    cont_char[1] = EOS;
	    STprintf(s_buffer,
		     ERx("SEP internal variable 'continue' is set to %s.\n"),
		     cont_char);
	    SEPputrec(s_buffer, sepResults);
	}
	else
	{
	    STprintf(s_buffer, ERx("unsepset: unknown variable '%s'.\n"), argv);
	    SEPputrec(s_buffer, sepResults);
	}
    }
    MEfree(s_buffer);
    return(OK);
}

/*
** Name: fill_cmmd
**
** History:
**	17-Oct-2008 (wanfr01)
**	    Bug 121090
**	    Added new parameter to get_commmand
*/

STATUS
fill_cmmd(FILE *testFile,char *filenm)
{
    STATUS                 ioerr = OK ;
    FILE                  *aPtr = NULL ;
    char                  *fileBuf = NULL ;
    char                  *cptr = NULL ;
    LOCTYPE                aLocType = 0 ;
    bool                   is_ok ;
    bool                   to_editor = TRUE ;

    fileBuf = SEP_MEalloc(SEP_ME_TAG_NODEL, MAX_LOC+1, TRUE, (STATUS *)NULL);
    if (is_ok = IS_FileNotation(filenm))
	if (Trans_SEPfile(&filenm,NULL,TRUE,SEP_ME_TAG_NODEL) != OK)
	    is_ok = FALSE;

    STcopy(filenm, fileBuf);
    cptr = SEP_MEalloc(SEP_ME_TAG_FILL, sizeof(FILLSTACK), TRUE, (STATUS *) NULL);
    fill_temp = (FILLSTACK *)cptr;

    cptr = SEP_MEalloc(SEP_ME_TAG_FILL, sizeof(LOCATION), TRUE, (STATUS *) NULL);
    fill_temp->fs_loc = (LOCATION  *)cptr;

    SEP_LOfroms(aLocType,fileBuf,fill_temp->fs_loc, SEP_ME_TAG_FILL);

    if (shellMode)
    {
	if (SEP_LOexists(fill_temp->fs_loc))
	{
	    get_answer(ERx("file was found. Do you want to edit it ? (y/n) "),
		       buffer_1);
	    if (buffer_1[0] == 'n' || buffer_1[0] == 'N')
		to_editor = FALSE;
	}
	if (to_editor == TRUE)
	{
	    edit_file(fileBuf);
	    TEwrite(CL,STlength(CL));
	    TEflush();
	    fix_cursor();
	    TDtouchwin(mainW);
	    TDtouchwin(statusW);
	    TDrefresh(mainW);
	}
	append_line(FILL_PROMPT,1);
	append_file(fileBuf);
	append_line(FILL_PROMPT,1);
    }
    else
    {
	switch (ioerr = SIopen(fill_temp->fs_loc,ERx("w"),&aPtr))
	{
	   case OK:
		if ((ioerr = get_command(testFile,FILL_PROMPT,FALSE)) != OK)
		    disp_line(ERx("ERROR: failed while reading in FILL file"),0,0);
		else
		{
		    if (updateMode)
			append_line(FILL_PROMPT,1);

		    while ((ioerr = get_command(testFile,NULL,FALSE)) == OK)
		    {
			if (lineTokens[0] &&
			    (STcompare(lineTokens[0],FILL_PROMPT) == 0))
			    break;

			SIputrec(buffer_1,aPtr);
			if (updateMode)
			    append_line(buffer_1,0);
		    }
		    if (ioerr != OK)
			disp_line(ERx("ERROR: failed while reading in FILL file"),0,0);
		    else
		    {
			LOtos(fill_temp->fs_loc, &fill_temp->fs_buf);
			fill_temp->fs_next = fill_stack;
			fill_stack = fill_temp;

			if (fill_stack_root == NULL)
			    fill_stack_root = fill_temp;

			if (updateMode)
			    append_line(FILL_PROMPT,1);
			SIclose(aPtr);
		    }
		}
		break;
	   case SI_BAD_SYNTAX:
		disp_line(ERx("ERROR: could not open file, Bad Syntax."),0,0);
		break;
	   case SI_CANT_OPEN:
		disp_line(ERx("ERROR: could not open file, Can't Open."),0,0);
                       break;
	   default:
		disp_line(ERx("ERROR: could not open file, Unknown Error."),0,0);
	}
    }
    MEfree(fileBuf);
    return(ioerr);

}
VOID
del_fillfiles ()
{
    while (fill_stack)
    {
	if (!nodelete)
	    LOdelete(fill_stack->fs_loc);

	fill_stack = fill_stack->fs_next;
    }
    MEtfree(SEP_ME_TAG_FILL);
}
