/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#ifdef i64_aix
#define MAX 403
#define MIN 405
#endif
#include    <gl.h>
#include    <cs.h>
#include    <cv.h>
#include    <st.h>
#include    <cm.h>
#include    <qu.h>
#include    <me.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    "pslsgram.h"
#include    <yacc.h>

/**
**
**  Name: PSLYERROR.C - YACC Error Handling Function
**
**  Description:
**	This file contains the error handling function for yacc.
**
**          psl_yerror - Handle a yacc error
**	    psl_yerr_buf - Extract recent token(s) for error messages.
**          psl_sx_error - Report a syntax error for which user
**			   has specified error handling.
**
**
**  History:    $Log-for RCS$
**      05-feb-86 (jeff)    
**          written
**      07-apr-87 (stec)
**          Add SQL error handling.
**	10-dec-87 (stec)
**	    Added psl_sx_error.
**	21-jan-1990 (greg)
**	    bug 8902 : CVfa requires a decimal_char parameter
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	16-mar-93 (rblumer)
**	    6.5 extends SQL CREATE TABLE to handle constraints & user-defined
**	    defaults, so remapped syntax error to new error message.
**	29-jun-93 (andre)
**	    #included CV.H
**	13-aug-93 (andre)
**	    fixed causes of acc warnings
**	15-sep-93 (swm)
**	    Added cs.h include above other header files which need its
**	    definition of CS_SID.
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	11-jun-2001 (somsa01)
**	    In psl_yerror() and psl_sx_error(), to save stack space,
**	    dynamically allocate space for buf.
**	29-jul-2001 (toumi01)
**	    Early definition of MIN/MAX for i64_aix to avoid conflict with
**	    system header macros with the same names.
**	1-Feb-2004 (schka24)
**	    Extract last-token finder into utility for partition def to use.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
void psl_yerror(
	i4 errtype,
	PSS_SESBLK *sess_cb,
	PSQ_CB *psq_cb);
void psl_sx_error(
	i4 error,
	PSS_SESBLK *sess_cb,
	PSQ_CB *psq_cb);
void psl_yerr_buf(
	PSS_SESBLK *sess_cb,
	char *buf,
	i4 buf_len);

/*{
** Name: psl_yerror	- Handle a yacc error.
**
** Description:
**      The QUEL grammar calls this function when it finds something wrong, 
**      for example, a syntax error or a stack overflow.  Note that all
**	errors here are treated as user errors, even when it would seem more
**	appropriate to treat them as internal errors; this is so they will be
**	reported to the user and we'll be able to figure out what's going on.
**
** Inputs:
**      errtype                         YACC constant saying what the error is
**	sess_cb				The session control block
**	  .pss_prvtok			  Beginning point of current token in
**					  query buffer.
**	  .pss_endbuf			  End of current query buffer.
**	  .pss_nxtchar			  Beginning of next token in query
**					  buffer.
**	  .pss_yacc			  yacc control block
**	psq_cb				The control block used to call the
**					parser facility.
**	  .psq_error			  Place to put the error info.
**
** Outputs:
**      psq_cb                          The control block used to call the
**					parser facility
**	  .psq_error			  Filled in with error information.
**	Returns:
**	    NONE
**	Exceptions:
**	    none
**
** Side Effects:
**	    Formats an error message and send it to the user.
**
** History:
**	05-feb-86 (jeff)
**          written
**      07-apr-87 (stec)
**          Add SQL error handling.
**	21-sep-88 (andre)
**	    Increase range of syntax errors to have SQL errors in
**	    [2500-2599,2800-3899] and QUEL errors in [2600-2699,3900-3999]
**	13-aug-92 (andre)
**	    6.5 needs a new syntax error message for GRANT and REVOKE; since we
**	    are using the same error message file for 6.4 and 6.5, I am forced
**	    to define new syntax error message numbers for 6.5, so the usual
**	    computattion will no longer yield the right message number.
**	    Therefore, I added code to treat GRANT and REVOKE syntax error
**	    messages separately
**	12-jan-93 (tam)
**	    6.5 extends REGISTER/REMOVE syntax to handle registered procedures.
**	    Remapped old error messages to new error messages for PSQ_REG_LINK,
**	    PSQ_REMOVE and PSQ_REG_REMOVE
**	16-mar-93 (rblumer)
**	    6.5 extends SQL CREATE TABLE to handle constraints & user-defined
**	    defaults, so remapped syntax error to new error message.
**	30-mar-93 (rblumer)
**	    map error in CREATE SET-input PROCEDURE to same error as for
**	    CREATE PROCEDURE; take out code to remap CREATE TABLE since now
**	    have swapped PSQ_DISTCREATE and PSQ_CREATE mode numbers (so that
**	    we can use old 6.4 error message for PSQ_DISTCREATE and a new error
**	    number for PSQ_CREATE, without needing to remap either one).
**	    Also need to remap QUEL error message for CREATE to 6.4 message #.
**	13-apr-93 (rblumer)
**	    change remapping of set-input procedure to change local query mode,
**	    instead of global psq_cb->psq_mode.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	30-jul-93 (robf)
**	    Add handling for new Secure 2.0 functions.
**	22-mar-94 (robf)
**          Add handling for GRANT/REVOKE ROLE
**      22-jul-96 (ramra01)
**          Add handling for ALTER TABLE ADD or DROP COLUMN
**	11-jun-2001 (somsa01)
**	    To save stack space, dynamically allocate space for buf.
**	1-Feb-2004 (schka24)
**	    Nobody wants to look at a 32K error msg anyway.  Use a
**	    smaller buf on the stack.
**	19-Oct-2005 (penga03)
**	    Added PSQ_ATBL_ALTER_COLUMN to the conditionals that test
**	    alter table (add/drop/alter column).
*/
VOID
psl_yerror(
	i4                errtype,
	PSS_SESBLK	   *sess_cb,
	PSQ_CB		   *psq_cb)
{
    i4             err_code;
    char		buf[DB_ERR_SIZE+10];
    i4		lineno = sess_cb->pss_lineno;
    i4		err_no;
    register i4	qmode = psq_cb->psq_mode;


    switch (errtype)
    {

    case 1:
      psf_error(E_PS030F_BACKUP, 0L, PSF_USERERR, &err_code,
	&psq_cb->psq_error, 0);
      break;

    case 2:
      psf_error(2800L, 0L, PSF_USERERR, &err_code,
	&psq_cb->psq_error, 1, (i4) sizeof(lineno), &lineno);
      break;

    case 3:
	psl_yerr_buf(sess_cb, buf, DB_ERR_SIZE);

      /*								    
      ** Note that SQL errors are in [2500-2599,3800-3899], while QUEL	    
      ** errors are in [2600-2699,3900-3999]
      **
      ** Because we use the same error message files for 6.4 and 6.5,
      ** and the syntax has changed for several commands & their error messages
      ** in 6.5, we have to have separate syntax messages for certain commands.
      ** The 6.5 messages for these commands now have error numbers outside the
      ** special 'syntax' range, and are processed specially below.
      ** These commands include:
      ** 	SQL  GRANT & REVOKE
      ** 	STAR REG_LINK, REMOVE, REG_REMOVE
      */
      if (psq_cb->psq_qlang == DB_SQL)
      {
	if (qmode == PSQ_GRANT)
	{
	    err_no = 4109L;
	}
	else if (qmode == PSQ_REVOKE)
	{
	    err_no = 4110L;
	}
	else if (qmode == PSQ_REG_LINK)
	{
	    err_no = E_PS1206_REGISTER;
	}
	else if (qmode == PSQ_REMOVE)
	{
	    err_no = E_PS1207_REMOVE;
	}
	else if (qmode == PSQ_REG_REMOVE && 
		sess_cb->pss_distrib & DB_3_DDB_SESS)
	{
	    err_no = E_PS1207_REMOVE;
	}
	else if (qmode == PSQ_CRESETDBP)
	{
	    /* create procedure and create set-input procedure
	    ** use the same syntax error message (for now)
	    */
	    qmode = PSQ_CREDBP;
	}
	else if (qmode == PSQ_ALTAUDIT)
	{
		/* ALTER SECURITY_AUDIT */
		err_no=4120;
	}
	else if (qmode == PSQ_SET_SESSION)
	{
		/* SET SESSION ... */
		err_no=4121;
	}
	else if (qmode == PSQ_CPROFILE) 
	{
		/* CREATE PROFILE */
		err_no=4122;
	}
	else if (qmode == PSQ_APROFILE) 
	{
		/* ALTER PROFILE */
		err_no=4123;
	}
	else if (qmode == PSQ_DPROFILE) 
	{
		/* DROP PROFILE */
		err_no=4124;
	}
	else  if (qmode == PSQ_MXSESSION)
	{
		/* SET [NO]MAX nnn for sessions */
		err_no=4125;
	}
	else if (qmode == PSQ_CALARM)
	{
		/* CREATE SECURITY_ALARM */
		err_no=4127;
	}
	else if (qmode == PSQ_KALARM)
	{
		/* DROP SECURITY_ALARM */
		err_no=4128;
	}
	else if (qmode == PSQ_SETROLE)
	{
		/* SET ROLE */
		err_no=4129;
	}
	else if (qmode == PSQ_RGRANT)
	{
		/* GRANT role*/
		err_no=4130;
	}
	else if (qmode == PSQ_RREVOKE)
	{
		/* REVOKE role */
		err_no=4131;
	}
	else
	{
	    if ( (qmode == PSQ_ATBL_ADD_COLUMN) ||
		 (qmode == PSQ_ATBL_DROP_COLUMN) ||
		 (qmode == PSQ_ATBL_ALTER_COLUMN) )
	    {
		/* ALTER TABLE ADD or DROP COLUMN */
		qmode = PSQ_ALTERTABLE;
	    }
	    err_no = ((qmode < 100) ? (2500 + qmode)
				    : (3700 + qmode));
	}
      }
      else
      {
	if (qmode == PSQ_CREATE)
	{
	    err_no = 2607L;
	}
	else 
	{
	    err_no = ((qmode < 100) ? (2600 + qmode)
				    : (3800 + qmode));
	}
      }
      psf_error(err_no, 0L, PSF_USERERR, &err_code,
	&psq_cb->psq_error, 2, (i4) sizeof(lineno), &lineno,
	0, buf);
      break;

    default:
      psf_error(E_PS0311_YACC_UNKNOWN, 0L, PSF_USERERR, &err_code,
	&psq_cb->psq_error, 0);
      break;
    }

}

/*{
** Name: psl_sx_error	- Handle specific syntax errors for which user has
**			  specified error handling code.
**
** Description:
**      Parser calls this routine when a syntax error has been discovered and
**	the user has specified `error' production to handle the condition.
**	The action for this production calls this routine and normally
**	YYABORTs to stop processing.
**
** Inputs:
**      error                           Error number describing syntax error
**	sess_cb				The session control block
**	psq_cb				The control block used to call the
**					parser facility
** Outputs:
**      psq_cb                          The control block used to call the
**					parser facility
**	  .psq_error			  Filled in with error information.
** Returns:
**	None
** Exceptions:
**	None
**
** Side Effects:
**	Formats an error message and sends it to the user.
**
** History:
**      10-dec-87 (stec)
**          Written.
**	11-jun-2001 (somsa01)
**	    To save stack space, dynamically allocate space for buf.
**	1-Feb-2004 (schka24)
**	    Extract token finder into separate routine.
*/
VOID
psl_sx_error(
	i4	    error,
	PSS_SESBLK  *sess_cb,
	PSQ_CB	    *psq_cb)
{
    i4             err_code;
    /* Should be long enough for the biggest token, in practice */
    /* This is pretty big, make it a max error size. */
    char		buf[DB_ERR_SIZE + 10];
    i4		lineno = sess_cb->pss_lineno;

    psl_yerr_buf(sess_cb, buf, DB_ERR_SIZE);
    
    psf_error(error, 0L, PSF_USERERR, &err_code,
	&psq_cb->psq_error, 2, (i4) sizeof(lineno), &lineno,
	0, buf);

}

/*
** Name: psl_yerr_buf -- Extract token context for error messages
**
** Description:
**	Generic syntax error messages, as well as some specific
**	ones, may want to include the current input context with
**	the message.  This routine uses the lex input position
**	to construct some context in the caller supplied buffer.
**	The context string is asciz (null terminated.)
**
**	The caller's buffer can be any convenient length, but the
**	length checking done here is rather sloppy!  In practice, the
**	buffer should be at least a couple dozen characters.  Regardless
**	of length, the buffer should be a bit longer than buf-len,
**	by at least 1 char (for the null), and 5-10 is better.
**	(There wasn't any length checking at all in the original.)
**
** Inputs:
**	sess_cb		PSS_SESBLK parser session control block
**	buf		Where to put the context string
**	buf_len		How much room there is in buf
**
** Outputs:
**	None
**	Context string built up in caller's buf
**
** History:
**	1-Feb-2004 (schka24)
**	    extracted from original psl-sx-error.
*/

void
psl_yerr_buf(PSS_SESBLK *sess_cb, char *buf, i4 buf_len)
{

    i4			buf_cnt = 0;
    i4			i;
    u_char		*cur_char;
    YACC_CB		*yacc_cb = (YACC_CB*) sess_cb->pss_yacc;
    YYTOK_LIST		*tok_ptr;
    u_char		*end_char;

    /*
    ** Skip over whitespace, because that couldn't have caused the problem.
    */

    /* this token may be a stacked value */
    /* **** Currently this code doesn't do anything because nobody
    ** (at least in the backend) ever stacks tokens this way!
    */
    if (yacc_cb->yyr_cur || yacc_cb->yyr_pop != &yacc_cb->yyr)
    {
	i = yacc_cb->yyr_cur - 1;
	/* if not in current stack, get previous */
	if (i < 0)
	{
	    for (tok_ptr = &yacc_cb->yyr; 
		tok_ptr->yy_next != yacc_cb->yyr_pop;)
		tok_ptr = tok_ptr->yy_next;
	    i = YYMAXDEPTH - 1;
	}
	else
	    tok_ptr = yacc_cb->yyr_pop;
	cur_char = (u_char *)tok_ptr->yyr[i].err_val;
	end_char = (u_char *)tok_ptr->yyr[i].err_end;
    }
    else
    {
	cur_char = sess_cb->pss_prvtok;
	end_char = sess_cb->pss_nxtchar;
    }
    while (*cur_char == ' ' || *cur_char == '\t' || *cur_char == '\n' ||
	   *cur_char == '\r')
    {
	cur_char++;
    }

    /*
    ** It shouldn't be possible for cur_char to be greater than pss_endbuf,
    ** but just in case...
    */
    if (cur_char >= sess_cb->pss_endbuf || cur_char >= end_char)
    {
	STcopy("EOF", buf);
	return;
    }
    else
    {
	switch (*(cur_char++))
	{
	  i2	    i2align;
	  i4	    i4align;
	  f8	    f8align;
	  i4	    count;
	  char      ch;
	  i4	    slength;
	  char      num_buf[256];
	  u_char    *straddr;
	  i2	    res_width;

	case PSQ_HVI2:
	  MEcopy((PTR) cur_char, sizeof(i2align), (PTR) &i2align);
	  CVla((i4) i2align, (char *) buf);
	  break;

	case PSQ_HVI4:
	  MEcopy((PTR) cur_char, sizeof(i4align), (PTR) &i4align);
	  CVla((i4) i4align, (char *) buf);
	  break;

	case PSQ_HVF8:
	  MEcopy((PTR) cur_char, sizeof(f8align), (PTR) &f8align);
	  CVfa(f8align, 10, 3, 'n', (char) sess_cb->pss_decimal, (char *) buf,
	       &res_width);
	  break;

	case PSQ_HVSTR:
	  slength = STlength((char *) cur_char);
	  straddr = cur_char;
	  for (count = 0; count < slength; count++)
	  {
	    ch = straddr[count];
	    if (CMprint(&ch))
	    {
		buf[buf_cnt++] = ch;
	    }
	    else
	    {
		i4align  = (i4) ch;
		STprintf(num_buf, "\\%o", i4align);
		STcopy(num_buf, (char *) &buf[buf_cnt]);
		buf_cnt += STlength(num_buf);
	    }
	    if (buf_cnt >= buf_len)
	    {
		buf[buf_cnt] = '\0';
		break;
	    }
	  }
	  break;

	case '\0':
	  break;

	default:
	  cur_char--;
	  count = end_char - cur_char;
	  if (count > buf_len) count = buf_len;
	  STncpy(buf, (char *) cur_char, count);
	  buf[ count ] = '\0';
	  break;
	}
    }
    STtrmwhite(buf);
} /* psl_yerr_buf */
