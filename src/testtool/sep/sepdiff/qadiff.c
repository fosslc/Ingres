#include <compat.h>
#include <er.h>
#include <lo.h>
#include <me.h>
#include <pc.h>
#include <si.h>
#include <st.h>

#include <sepdefs.h>
#include <sepfiles.h>

#include <token.h>
#include <edit.h>

/*
**  NO_OPTIM = int_lnx int_rpl ibm_lnx sqs_ptx i64_aix i64_lnx
*/

/*
** History:
**	16-aug-1989 (boba)
**		Include compat.h to get UNIX define.
**	05-oct-1989 (mca)
**		Create an array of pointers to tokens, which will be used
**		by the output routine to avoid much pointer-chasing.
**		Add debugging output, ifedef'd by the LEX_DEBUG symbol.
**		Do a better job of reusing memory structures, rather than
**		always allocating and freeing.
**	19-oct-1989 (eduardo)
**		undef extern definition for VMS sake & added support for
**		SEPFILES
**	01-dec-1989 (eduardo)
**		Added memory cleanup if memory limit was reached
**	06-dec-1989 (mca)
**		Remove a call to calloc() that was made redundant when the
**		size_tokarray() function was added.
**	23-apr-1990 (seng)
**		Shorten file name of lex log file.  System V limits names
**		to 14 characters.
**	11-may-1990 (bls)
**		Merged with porting codeline (shorten filenames; intent
**		of porting code preserved, but exact changes not made).
**	16-may-1990 (rog)
**		Fix problems from the above integration.
**	11-jun-1990 (rog)
**		External functions should be FUNC_EXTERN, not GLOBALREF.
**	11-jun-1990 (rog)
**		Make sure external variables are declared correctly.
**	13-jun-1990 (rog)
**		Add some register declarations.
**	13-jun-1990 (rog)
**		More performance improvements.
**	19-jul-1990 (rog)
**		Change include files to use <> syntax because the headers
**		are all in testtool/sep/hdr.
**	17-feb-1991 (seiwald)
**		Support for performance mods: no more HaveUnput; a single
**		call to free_tokens() clears both token lists.
**	30-aug-91 (donj)
**		Took out #IFDEFs for LIST_TOKENS and replaced with trace
**		feature set by "SEPSET TRACE TOKENS" or "SEPSET TRACE"
**		commands. Also compressed trace code into one new function,
**		Trace_Token_List().
**	14-jan-1992 (donj)
**	    Modified all quoted string constants to use the ERx("") macro.
**	    Reformatted variable declarations to one per line for clarity.
**	    Added explicit declarations for all submodules called. Simplified 
**	    and clarified some control loops.
**      02-apr-1992 (donj)
**          Added A_DBNAME token type. Moved static nat's out of QAdiff().
**	26-aug-1992 (donj)
**	    Try using C-coded version of yylex(), SEP_GRdo_file().
**	30-nov-1992 (donj)
**	    Add SED masking enhancements (grammar_methods).
**	18-jan-1993 (donj)
**	    Wrap LOcompose() with SEP_LOcompose() to fix a UNIX bug.
**	04-feb-1993 (donj)
**	    Added tracing cases for A_DBLWORD, A_DESTROY, A_DROP, A_RULE,
**	    A_PROCEDURE, A_TABLE, A_OWNER, A_USER, A_COLUMN.
**	04-feb-1993 (donj)
**	    Rip out C-coded version of yylex(), SEP_GRdo_file(). It was too
**	    slow. Will work with LEX source code and grammar.lex to create
**	    a doublebyte compatible yylex().
**	15-feb-1993 (donj)
**	    Clean up in Run_SED_on_file() wasn't deleting *.sdr and *.sed
**	    files. Removed ampersands.
**	25-mar-1993 (donj)
**	    Skip SED processing if the Results or Canon files are empty.
**	 4-may-1993 (donj)
**	    NULLed out some char ptrs, Added prototyping of functions.
**	 7-may-1993 (donj)
**	    Add more prototyping.
**	25-aug-1993 (donj)
**	    Made the trace dump of the token lists more efficient by
**	    condensing duplicate entries.
**      26-aug-1993 (donj)
**          Change STprintf() to IISTprintf(), STmove() to IISTmove() and
**          MEcopy() to IIMEcopy() due to changes in CL.
**	 9-sep-1993 (donj)
**	    Fixed problem with SED file usage. Wasn't allocating enough
**	    space for the LOCATIONs. I was allocating sizeof(LOCATION *)
**	    rather than sizeof(LOCATION). Wasn't a problem on VMS. The
**	    VMS compiler is very forgiving.
**	15-oct-1993 (donj)
**	    Make function prototypes unconditional. Take a useless, unused
**	    parameter out of G_do_miller(). Add a A_GROUP token type.
**	28-jan-1994 (donj)
**	    Added new A_PRODUCT token to mask for matching "INGRES" and "OpenINGRES"
**      20-apr-1994 (donj)
**          Added support for new token type, A_COMPILE. For those pesky ABF
**          "Compiling 10104.c" messages that always diff.
**	10-may-1994 (donj)
**	    Use del_floc() rather than LOdelete(). del_floc() takes care of
**	    multiple versions on VMS systems whereas LOdelete() doesn't.
**	31-may-1994 (donj)
**	    Took out #include for sepparam.h. QA_C was complaining about all
**	    the unecessary GLOBALREF's.
**	07-jun-1994 (donj)
**	    Simplified some string constants  and printf statements.
**	31-Jan-1996 (somsa01)
**	    Used LOdelete() instead of del_floc() for NT_GENERIC platforms when
**	    "Recreating SEP File". del_floc() would go into an endless loop
**	    when trying to delete a ca*.stf file.
**	28-Jul-1998 (lamia01)
**		Added new version identifiers, Computer Associates as the company name,
**		and NT directory masking. Version identifier also works now with DBL.
**		New tokens - Database ID, Server ID, Alarm ID, Transaction ID and
**		Checkpoint ID introduced to avoid unnecessary diffs.
**	14-may-1999 (toumi01)
**	    Add NO_OPTIM for Linux to stop calls to QAdiff from writing
**	    many blank lines to stdout.  When running automated sep, these
**	    show up as thousands of blank lines in be*.out and fe*.out.
**	21-may-1999 (walro03)
**		No-optimize to fix sep diff errors on Sequent (sqs_ptx).
**	25-may-1999 (somsa01)
**	    Include st.h .
**	14-jun-1999 (somsa01)
**	    Added missing comment begin character to correct build problem.
**	25-Jul-1999 (popri01)
**	    To minimize sep resource requirements, and because checkpoint id is 
**	    specific to LAR, implement Iain's checkpoint ID masking with sep's sed 
**	    feature, rather than here. (See grammar.lex for additional explanation.)
**	06-oct-1999 (toumi01)
**	    Change Linux config string from lnx_us5 to int_lnx.
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	22-aug-2000 (somsa01)
**	    Add NO_OPTIM for ibm_lnx to stop calls to QAdiff from writing
**	    many blank lines to stdout.  When running automated sep, these
**	    show up as thousands of blank lines in be*.out and fe*.out.
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      22-sep-2000 (mcgem01)
**          More nat and longnat to i4 changes.
**	21-aug-2001 (toumi01)
**	    NO_OPTIM i64_aix to fix sep diff error (extra blank lines)
**	2-Jun-2006 (bonro01)
**	    Check return code from SED command.
**       6-Nov-2006 (hanal04) SIR 117044
**          Add int.rpl for Intel Rpath Linux build.
*/

FUNC_EXTERN                yylex() ;
FUNC_EXTERN  E_edit        G_do_miller( i4  m, i4  n, i4  max_d, i4  *merr ) ;
FUNC_EXTERN  VOID          O_output () ;
FUNC_EXTERN  VOID          size_tokarray () ;
FUNC_EXTERN  VOID          free_tokens () ;
FUNC_EXTERN  VOID          clean_edit_blocks () ;
FUNC_EXTERN  char         *SEP_MEalloc () ;

GLOBALREF    LOCATION      SEPresloc ;
GLOBALREF    LOCATION      SEPcanloc ;
GLOBALREF    LOCATION     *SED_loc ;

GLOBALDEF    FILE_TOKENS  *list_head = NULL ;
GLOBALDEF    FILE_TOKENS  *list_tail = NULL ;
GLOBALDEF    FILE_TOKENS  *list1 ;
GLOBALDEF    FILE_TOKENS  *list2 ;
GLOBALREF    FILE_TOKENS **tokarray1 ;
GLOBALREF    FILE_TOKENS **tokarray2 ;

GLOBALDEF    SEPFILE      *currSEPptr ;
GLOBALREF    FILE         *traceptr ;

GLOBALREF    i4            tracing ;
GLOBALDEF    i4            no_tokens ;
GLOBALDEF    i4            totaldiffs ;
GLOBALDEF    i4            failMem ;
GLOBALDEF    i4            numtokens1 ;
GLOBALDEF    i4            numtokens2 ;

GLOBALREF    bool          grammar_methods ;

static i4                  no_res = 0;
static i4                  no_cans = 0;

    i4                     Trace_Token_List ( char *list_label, i4  l_nr,
                                              i4  num_of_tokens,
                                              FILE_TOKENS **tokarray ) ;
    STATUS                 Run_SED_on_file  ( SEPFILE **Sep_file,
                                              LOCATION *Sep_Loc ) ;

int
QAdiff(SEPFILE **canFile,SEPFILE **resFile,SEPFILE *diffFile,i4 max_d)
{
    char                  *toktype = NULL ;

    register i4            i ;
    i4                     memerr ;

    STATUS                 ret_val ;

    register FILE_TOKENS  *t = NULL ;
    E_edit                 ed_res ;


    totaldiffs = no_tokens = failMem = 0;
    list_head = list_tail = NULL;
    SEPrewind(diffFile, TRUE);

    if (grammar_methods&GRAMMAR_SED_C)
    {
	if( (ret_val = Run_SED_on_file(canFile,&SEPcanloc)) != OK )
	{
	    char               tempbuffer[TEST_LINE+1];

	    clean_token_blocks();
	    IISTmove(ERx("ERROR: SED command failed"),' ',
		   TEST_LINE, tempbuffer);
	    tempbuffer[TEST_LINE] = '\0';
	    SEPputrec(tempbuffer, diffFile);
	    return(1);
	}
    }
    if (grammar_methods&GRAMMAR_LEX)
    {
	SEPrewind(*canFile, TRUE);
	currSEPptr = *canFile;
	yylex();
    }

    if (failMem)
    {
	char               tempbuffer[TEST_LINE+1];

	clean_token_blocks();
	IISTmove(ERx("ERROR: lack of memory while lexing"),' ',
	       TEST_LINE, tempbuffer);
	tempbuffer[TEST_LINE] = '\0';
	SEPputrec(tempbuffer, diffFile);
	return(1);
    }
    list1 = list_head;
    numtokens1 = no_tokens;
    size_tokarray(1, numtokens1+1);
    for (i = 1, t = list1; t != NULL; i++, t = t->next)
	tokarray1[i] = t;

    if (tracing & TRACE_TOKENS)
	Trace_Token_List(ERx("Canon"), ++no_cans, numtokens1, tokarray1);

    failMem = no_tokens = 0;
    list_head = list_tail = NULL;

    if (grammar_methods&GRAMMAR_SED_R)
    {
	if( (ret_val = Run_SED_on_file(resFile,& SEPresloc)) != OK )
	{
	    char               tempbuffer[TEST_LINE+1];

	    clean_token_blocks();
	    IISTmove(ERx("ERROR: SED command failed"),' ',
		   TEST_LINE, tempbuffer);
	    tempbuffer[TEST_LINE] = '\0';
	    SEPputrec(tempbuffer, diffFile);
	    return(1);
	}
    }
    if (grammar_methods&GRAMMAR_LEX)
    {
	SEPrewind(*resFile, FALSE);
	currSEPptr = *resFile;
	yylex();
    }

    if (failMem)
    {
	char               tempbuffer[TEST_LINE+1];

	clean_token_blocks();
	IISTmove(ERx("ERROR: lack of memory while lexing"),' ',
	       TEST_LINE, tempbuffer);
	tempbuffer[TEST_LINE] = '\0';
	SEPputrec(tempbuffer, diffFile);
	return(1);
    }
    list2 = list_head;
    numtokens2 = no_tokens;
    size_tokarray(2, numtokens2+1);
    for (i = 1, t = list2 ; t != NULL ; i++, t = t->next)
	tokarray2[i] = t;

    if (tracing & TRACE_TOKENS)
	Trace_Token_List(ERx("Result"), ++no_res, numtokens2, tokarray2);

    ed_res = G_do_miller(numtokens1,numtokens2,max_d,&memerr);
    if (memerr)
    {
	char               tempbuffer[TEST_LINE+1];

	clean_edit_blocks();
	IISTmove(ERx("ERROR: lack of memory while diffing"),' ',
	       TEST_LINE, tempbuffer);
	tempbuffer[TEST_LINE] = '\0';
	SEPputrec(tempbuffer, diffFile);
	return(1);
    }
    if (totaldiffs)
	O_output(ed_res, 0, totaldiffs, diffFile);

    free_tokens();
    SEPrewind(*canFile, FALSE);
    SEPrewind(*resFile, FALSE);

    return(totaldiffs);
}

i4
Trace_Token_List(char *list_label,i4 l_nr,i4 num_of_tokens,FILE_TOKENS **tokarray)
{
    i4                     i, j, k ;
    char                  *toktype  = NULL ;
    char                  *otoktype = NULL ;
    bool                   skip_it  = FALSE ;
    char                  *ARRY_LINE_FMT
			   = ERx(" %5d   %-40s   %5d    A_%-10s\n") ;
    char                  *ARRY_BANNER
			   = ERx("-------------------------------------------------------------------------\n") ;

    SIfprintf(traceptr,ERx("\n                       %6s #%d,  %d Tokens.\n\n"),
	      list_label, l_nr, num_of_tokens );

    if (num_of_tokens == 0) return (OK);

    SIputrec(ARRY_BANNER,traceptr);
    SIputrec(ERx(" Token |            First 40 Chars  "), traceptr);
    SIputrec(ERx("              |  Line  |    Token\n"),  traceptr);
    SIputrec(ERx("  Nr   |              of Token "), traceptr);
    SIputrec(ERx("           |   Nr   |    Type\n"), traceptr);
    SIputrec(ARRY_BANNER,traceptr);

    for (i=1, j=0; i <= num_of_tokens; i++)
    {
	if ((j == 0)||(tokarray[i]->tokenID != A_UNKNOWN)||
	    (STcompare(tokarray[i]->token,tokarray[j]->token) != 0))
	{
	    if (skip_it == TRUE)
	    {
		if ((k = tokarray[i]->token_no - tokarray[j]->token_no) == 2)
		{
		    SIfprintf(traceptr, ARRY_LINE_FMT, tokarray[j]->token_no,
				        tokarray[j]->token, tokarray[j]->lineno,
			                otoktype );
		}
		else
		{
		    SIfprintf(traceptr, ERx("\t\t - (%d) duplicate tokens -\n"),
					k-1);
		}

		skip_it = FALSE;
	    }

	    j = i;
	}
	else
	{
	    otoktype = toktype;
	    skip_it = TRUE;
	    continue;
	}

	switch (tokarray[i]->tokenID)
	{
	    case A_NUMBER:	toktype = ERx("NUMBER");	break;
	    case A_OCTAL:	toktype = ERx("OCTAL");		break;
	    case A_HEXA:	toktype = ERx("HEXA");		break;
	    case A_FLOAT:	toktype = ERx("FLOAT");		break;
	    case A_WORD:	toktype = ERx("WORD");		break;
	    case A_DBLWORD:     toktype = ERx("DBLWORD");       break;
	    case A_DATE:	toktype = ERx("DATE");		break;
	    case A_TIME:	toktype = ERx("TIME");		break;
	    case A_DELIMITER:	toktype = ERx("DELIMITER");	break;
	    case A_CONTROL:	toktype = ERx("CONTROL");	break;
	    case A_UNKNOWN:	toktype = ERx("UNKNOWN");	break;
	    case A_RETCOLUMN:	toktype = ERx("RETCOLUMN");	break;
	    case A_DIRECTORY:	toktype = ERx("DIRECTORY");	break;
	    case A_BANNER:	toktype = ERx("BANNER");	break;
	    case A_PRODUCT:	toktype = ERx("PRODUCT");	break;
	    case A_VERSION:	toktype = ERx("VERSION");	break;
	    case A_ALARMID:	toktype = ERx("ALARMID");	break;
	    case A_COMPNAME:	toktype = ERx("COMPNAME");	break;
	    case A_COPYRIGHT:	toktype = ERx("COPYRIGHT");	break;
	    case A_DBNAME:	toktype = ERx("DBNAME");	break;
	    case A_DESTROY:     toktype = ERx("DESTROY");       break;
	    case A_DROP:        toktype = ERx("DROP");          break;
	    case A_RULE:        toktype = ERx("RULE");          break;
	    case A_PROCEDURE:   toktype = ERx("PROCEDURE");     break;
	    case A_SPACE:	toktype = ERx("SPACE");		break;
	    case A_DATABSID:	toktype = ERx("DATABSID"); 	break;
	    case A_TRANID: 	toktype = ERx("TRANID"); 	break;
	    case A_SERVERID:	toktype = ERx("SERVERID"); 	break;
	    case A_TABLE:       toktype = ERx("TABLE");         break;
	    case A_OWNER:       toktype = ERx("OWNER");         break;
          case A_USER:        toktype = ERx("USER");          break;
	    case A_COLUMN:      toktype = ERx("COLUMN");        break;
	    case A_GROUP:       toktype = ERx("GROUP");         break;
	    case A_COMPILE:     toktype = ERx("COMPILE");       break;
	    default:		toktype = ERx("(error)");	break;
	}
	SIfprintf(traceptr, ARRY_LINE_FMT, tokarray[i]->token_no,
			    tokarray[i]->token, tokarray[i]->lineno, toktype );
    }
    SIfprintf(traceptr, ERx("(%5d)\n"), num_of_tokens);
    return (OK);
}

STATUS
Run_SED_on_file(SEPFILE **Sep_file,LOCATION *Sep_Loc)
{
    STATUS                 ret_val ;
    CL_ERR_DESC            cl_err;

    LOCATION              *sedtmploc = NULL ;
    LOCATION              *filtmploc = NULL ;

    char                  *SEDbuffer = NULL ;
    char                  *sedtmp = NULL ;
    char                  *filtmp = NULL ;
    char                  *cmd_line = NULL ;
    char                  *sed_dev = NULL ;
    char                  *sed_path = NULL ;
    char                  *sed_name = NULL ;
    char                  *sed_type = NULL ;
    char                  *sed_vers = NULL ;
    char                  *fptr1 = NULL ;
    char                  *fptr2 = NULL ;

    FILE                  *SEDfptr = NULL ;
    FILE                  *fptr = NULL ;

    bool                   Sep_file_empty ;

    sed_dev  = SEP_MEalloc(SEP_ME_TAG_SED, LO_DEVNAME_MAX+1,  TRUE, NULL);
    sed_path = SEP_MEalloc(SEP_ME_TAG_SED, LO_PATH_MAX+1,     TRUE, NULL);
    sed_name = SEP_MEalloc(SEP_ME_TAG_SED, LO_FPREFIX_MAX+1,  TRUE, NULL);
    sed_type = SEP_MEalloc(SEP_ME_TAG_SED, LO_FSUFFIX_MAX+1,  TRUE, NULL);
    sed_vers = SEP_MEalloc(SEP_ME_TAG_SED, LO_FVERSION_MAX+1, TRUE, NULL);

    SEDbuffer = SEP_MEalloc(SEP_ME_TAG_SED, SCR_LINE+1, TRUE, NULL);

    /*
    ** Create Sed output file names.
    */
    sedtmploc = (LOCATION *)SEP_MEalloc(SEP_ME_TAG_SED, sizeof(LOCATION),
					TRUE, NULL);
    filtmploc = (LOCATION *)SEP_MEalloc(SEP_ME_TAG_SED, sizeof(LOCATION),
					TRUE, NULL);
    sedtmp    = SEP_MEalloc(SEP_ME_TAG_TOKEN, MAX_LOC+1, TRUE, NULL);
    filtmp    = SEP_MEalloc(SEP_ME_TAG_TOKEN, MAX_LOC+1, TRUE, NULL);

    LOtos(Sep_Loc, &fptr2);
    STcopy(fptr2, sedtmp);
    STcopy(fptr2, filtmp);
    LOfroms(FILENAME & PATH, sedtmp, sedtmploc);
    LOfroms(FILENAME & PATH, filtmp, filtmploc);
    LOdetail(sedtmploc, sed_dev, sed_path, sed_name, sed_type, sed_vers);
    STcopy(ERx("sed"),sed_type);
    SEP_LOcompose(sed_dev, sed_path, sed_name, sed_type, sed_vers, sedtmploc);
    STcopy(ERx("sdr"),sed_type);
    SEP_LOcompose(sed_dev, sed_path, sed_name, sed_type, sed_vers, filtmploc);

    /*
    ** Copy Sep SI_RACC file to a SI_TXT file.
    */
    SEPrewind(*Sep_file, FALSE);
    if ((ret_val = SIfopen(filtmploc,ERx("w"), SI_TXT, SCR_LINE, &fptr))
	!= OK)
	return(ret_val);

    for (Sep_file_empty = TRUE;
	 (ret_val = SEPgetrec(SEDbuffer, *Sep_file)) == OK;
	 SIputrec(SEDbuffer, fptr))
	if (Sep_file_empty)
	    Sep_file_empty = FALSE;

    SIclose(fptr);

    if (Sep_file_empty == FALSE)
    {   /*
	** Create command line for sed command.
	*/
	LOtos(SED_loc, &fptr1);
	LOtos(filtmploc, &fptr2);
	cmd_line = SEP_MEalloc(SEP_ME_TAG_TOKEN, MAX_LOC+1, TRUE, NULL);
	IISTprintf(cmd_line, "sed -f %s %s", fptr1, fptr2);

	/*
	** Close Sep file and run the SED.
	*/
	SEPclose(*Sep_file);
	ret_val = PCcmdline(NULL, cmd_line, PC_WAIT, sedtmploc, &cl_err);
	if(ret_val != OK)
	{
	    SEPopen(Sep_Loc, SCR_LINE, Sep_file);
	    del_floc(sedtmploc);
	    del_floc(filtmploc);
	    MEtfree(SEP_ME_TAG_SED);
	    return(1);
	}

	/*
	** Recreate Sep file.
	*/
#ifdef NT_GENERIC
	LOdelete(Sep_Loc);
#else
	del_floc(Sep_Loc);
#endif
	if ((ret_val = SIfopen(Sep_Loc, ERx("w"), SI_RACC, SCR_LINE, &fptr))
	    != OK)
	    return(ret_val);
	SIclose(fptr);
	SEPopen(Sep_Loc, SCR_LINE, Sep_file);

	/*
	** Open Sed output file.
	*/
	if ((ret_val = SIfopen(sedtmploc, ERx("r"), SI_TXT, SCR_LINE, &SEDfptr))
	    != OK)
	    return(ret_val);

	/*
	** Copy the Sed output file to the Sep file.
	*/
	while((ret_val = SIgetrec(SEDbuffer, SCR_LINE, SEDfptr)) == OK)
	    SEPputrec(SEDbuffer, *Sep_file);

	SEPclose(*Sep_file);
	SIclose(SEDfptr);

	/*
	** Reopen the sep file.
	*/
	SEPopen(Sep_Loc, SCR_LINE, Sep_file);

	/*
	** Clean up.
	*/
	del_floc(sedtmploc);
    }
    del_floc(filtmploc);
    MEtfree(SEP_ME_TAG_SED);
    return(OK);
}

