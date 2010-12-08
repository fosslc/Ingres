/*
**    Include file
*/

#include <compat.h>
#include <si.h>
#include <st.h>
#include <cm.h>
#include <lo.h>
#include <er.h>

#include <termdr.h>

#define comments_c

#include <sepdefs.h>
#include <sepfiles.h>
#include <septxt.h>
#include <seperrs.h>
#include <termdrvr.h>

#include <fndefs.h>

/*
**  History:
**      ##-###-#### (XXXX)
**	    Created.
**	30-nov-1992 (DonJ)
**	    Did some minor housekeeping.
**	01-feb-1993 (donj)
**	    MPE can't handle SIopen (). Needs SIfopen to create
**	    SI_TXT files.
**      26-apr-1993 (donj)
**	    Finally found out why I was getting these "warning, trigrah sequence
**	    replaced" messages. The question marks people used for unknown
**	    dates. (Thanks DaveB).
**	 5-may-1993 (donj)
**	    Add function prototypes, init some pointers.
**	 7-may-1993 (donj)
**	    Add more prototyping.
**      25-may-1993 (donj)
**	    Use a common msg[] char array.
**       2-jun-1993 (donj)
**          Isolated Function definitions in a new header file, fndefs.h. Added
**          definition for module, and include for fndefs.h.
**      15-oct-1993 (donj)
**          Make function prototyping unconditional.
**	30-dec-1993 (donj)
**	    Added GLOBALREF's for mainW, screenLine and paging. They're used by the
**	    disp_line() macros.
**	 4-jan-1994 (donj)
**	    Added some parens, re-formatted some things qa_c complained about. Simp-
**	    lified form_comment() function by modularizing, creating new functions
**	    read_fm_command() and read_in_comments().
**	10-may-1994 (donj)
**	    Took out some stuff that duplicated what fndefs.h did. also changed
**	    some statically allocated structures and made them dynamically allo-
**	    cated. Implemented a Create_stf_file() function for the creation of
**	    and file of the form {prefix}{pid#}.stf. Here it's the comment file
**	    ct{pid#}.stf.
**	24-may-1994 (donj)
**	    Took out two unused GLOBALREF's.
**	31-may-1994 (donj)
**	    Took out #include for sepparam.h. QA_C was complaining about all
**	    the unecessary GLOBALREF's.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	21-May-2003 (bonro01)
**	    Add support for HP Itanium (i64_hpu)
**	1-Dec-2010 (kschendel)
**	    (Serious) compiler warning fix in create-comment.
*/

/*
    Reference global variables
*/

GLOBALREF    bool          batchMode ;

GLOBALREF    WINDOW       *mainW ;

GLOBALREF    i4            fromLastRec ;
GLOBALREF    i4            sep_state ;
GLOBALREF    i4            sep_state_old ;

GLOBALREF    LOCATION     *cmtLoc ;

GLOBALREF    char          canonAns ;
GLOBALREF    char          holdBuf [] ;
GLOBALREF    char          buffer_1 [] ;
GLOBALREF    char         *cmtFile ;
GLOBALREF    char         *cmtName ;
GLOBALREF    char         *ErrC ;
GLOBALREF    char         *ErrOpn ;
GLOBALREF    char         *ErrF ;
GLOBALREF    char          msg [TEST_LINE] ;

/*
**    Reference to external subroutines
*/

STATUS                     read_fm_command (FILE *aFile) ;
STATUS                     read_in_comments (FILE *aFile, FILE *comPtr) ;

/*
**    Local module data.
*/

bool                       commentNameBuilt = FALSE ;

char                      *comment = ERx(" comments from test script") ;
char                      *cfile   = ERx(" comments file") ;

/*
**  skip_comment
**
**  Description:
**	Ignore a sep comment, dumping the lines into the log file.
**
**	If preprint is false, get_comment has already dumped the last 'command'
**	into the log file which is just the first line of the comment.  Dump the
**	rest of the comment and the actual command into the log file.
**
**  History:
**	30-Dec-2008 (wanfr01)
**	    Bug 121443 
**	    Comment start may have been printed by get_command already
**	    
*/

STATUS
skip_comment(FILE *aFile,char *cbuff,char preprint)
{
    STATUS                 ret_status = OK ;

    if ((preprint == FALSE) && (cbuff != NULL))
    {
	append_line(cbuff,0);
    }

    do
    {
        if ((ret_status = read_fm_command(aFile)) == OK)
        {
	    if ((sep_state&IN_COMMENT_STATE)||(sep_state_old&IN_COMMENT_STATE))
	    {
	        append_line(buffer_1,0);
	    }
	}
    } while ((sep_state_old&IN_COMMENT_STATE)&&(ret_status == OK));

    if (preprint == TRUE)
        append_line(buffer_1,0);

    return (ret_status);

} /* end of skip_comment */

/*
**	form_comment
*/
STATUS
form_comment(FILE *aFile)
{
    STATUS                 ret_status ;
    FILE                  *comPtr  = NULL ;

    /*
    ** Find beginning of commment.
    */

    do
    {
        ret_status = read_fm_command(aFile);

    } while ((ret_status == OK)&&
	     ((sep_state & BLANK_LINE_STATE)||(sep_state & IN_ENDING_STATE)));

    if (ret_status == OK)
    {
	if ((sep_state & IN_COMMENT_STATE) == 0)
	{
	    fromLastRec = 1;
	    STcopy(buffer_1, holdBuf);
	    ret_status = FAIL;
	}
    }

    /*
    ** create comments file
    */

    if (ret_status == OK)
    {
	if ((ret_status = bld_cmtName(msg)) != OK)
	{
	    disp_line(msg,0,0);
	}
	else if ((ret_status = SIfopen(cmtLoc, ERx("w"), SI_TXT, TERM_LINE,
				       &comPtr)) != OK)
	{
	    disp_line(STpolycat(2,ErrOpn,cfile,msg),0,0);
	}
    }

    /*
    ** read in comments
    */

    if (ret_status == OK)
    {
	if ((ret_status = read_in_comments(aFile, comPtr)) == OK)
	{
	    /*
	    ** close comments file
	    */

	    if (SIclose(comPtr)!= OK)
	    {
		STpolycat(3,ErrC,ERx("close"),cfile,msg);
		disp_line(msg,0,0);
		ret_status = FAIL;
	    }
	}
    }

    return(ret_status);

} /* end of form_comment */

STATUS
read_in_comments(FILE *aFile, FILE *comPtr)
{
    STATUS                 ret_status = OK ;

    do
    {
	if ((ret_status = read_fm_command(aFile)) == OK)
	{
	    if (sep_state&IN_COMMENT_STATE)
	    {
		SIputrec(buffer_1,comPtr);
	    }
	}
    } while ((sep_state&IN_COMMENT_STATE)&&(ret_status == OK));

    return(ret_status);
}

/*
** Name: read_fm_command
**
** History:
**	17-Oct-2008 (wanfr01)
**	    Bug 121090
**	    Added new parameter to get_commmand
*/
STATUS
read_fm_command(FILE *aFile)
{
    STATUS                 ioerr ;
    STATUS                 ret_status = OK ;

    ioerr = get_command(aFile, NULL,FALSE);
    if ((ioerr == FAIL)||(ioerr == ENDFILE))
    {
	if (ioerr == FAIL)
        {
	    STpolycat(3,ErrF,ERx("reading"),comment,msg);
	}
	else
	{
            STpolycat(3,ErrF,ERx("reading (EOF)"),comment,msg);
	}
	disp_line(msg,0,0);
	ret_status = FAIL;
    }
    return (ret_status);
}

/*
**  bld_cmtName ()
*/
STATUS
bld_cmtName ( char *errmsg )
{
    STATUS                 ret_val = OK ;

    if (!commentNameBuilt)
    {
	if ((ret_val = Create_stf_file(SEP_ME_TAG_NODEL, ERx("ct"),
				       &cmtFile, &cmtLoc, errmsg)) != OK)
	{
	    disp_line(msg,0,0);
	}
	else
	{
	    cmtName = STtalloc(SEP_ME_TAG_NODEL, cmtFile);
	    commentNameBuilt = TRUE;
	}
    }

    return (ret_val);
}

/*
**  create_comment ()
*/
STATUS
create_comment ()
{
    STATUS                 ret_val ;
    FILE                  *tmpf = NULL ;

    if ((ret_val = bld_cmtName(msg)) != OK)
    {
	disp_line(msg, 0, 0);
    }
    else
    {
	if (SIfopen(cmtLoc, ERx("a"), SI_TXT, TERM_LINE, &tmpf) == OK)
	{
	    SIclose(tmpf);
	}
	edit_file(cmtName);
	fix_cursor();
	TDclear(curscr);
	TDtouchwin(mainW);
	TDrefresh(mainW);
    }

    return (ret_val);

} /* end of create_comment */

/*
**  insert_comment ()
*/
STATUS
insert_comment ()
{
    STATUS                 ret_val ;
    OFFSET_TYPE            size ;

    if ((ret_val = bld_cmtName(msg)) != OK)
    {
	disp_line(msg, 0, 0);
    }
    else
    if (SEP_LOexists(cmtLoc))
    {
	LOsize(cmtLoc, &size);
	if (size > 0)
	{
	    append_line(OPEN_COMMENT, 1);
	    append_file(cmtName);
	    append_line(CLOSE_COMMENT, 1);
	}
	del_floc(cmtLoc);
    }

    return (ret_val);
} /* end of insert_comment */

/*
**  do_comments ()
*/
char
do_comments()
{
    char                   menu_ans ;

    do
    {
	if (canonAns == EOS)
	{
	    disp_prompt(NEW_CANON,&menu_ans,NEW_CANON_A);
	}
	else
	{
	    menu_ans = canonAns;
	}

	if ((menu_ans == 'c')||(menu_ans == 'C'))
	{
	    create_comment();
	}

    } while ((menu_ans == 'c')||(menu_ans == 'C'));

    insert_comment();
    return(menu_ans);
} /* end of do_comments () */
