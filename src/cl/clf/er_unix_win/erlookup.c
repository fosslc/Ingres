/*
**Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <st.h>
#include    <cm.h>
#include    <er.h>
#include    <tm.h>
#include    <cs.h>
#include    <ex.h>
#include    <cv.h>
#include    "erloc.h"

/**
**  Name:	erlookup.c -	ER Module Look-up Message Routine.
**
**  Description:
**      The module contains the routines that add ERslookup function on the ER
**	routine of the compatibility library.
**
**	The following externally visible routines are defined in this file:
**
**          ERslookup() - Lookup error message text.
**
**	The following static routines are also defined in this file:
**
**	    er_exhandler() - Exception handler for message parameter processing.
**
**  History:
**      02-oct-1985 (derek)    
**          Created new for 5.0.
**	18-Jun-1986 (fred)
**
**	01-Oct-1986 (kobayashi)
**	    Change for KANJI function.
**	02-apr-1987 (peter)
**	    Add support for pointers using -1 in parameters.
**	20-jul-1988 (roger)
**	    Implemented UNIX system-dependent error reporting.
**	07-oct-88 (thurston)
**	    Added the static routine er_exhandler() to catch exceptions that
**	    happen if there are missing parameters for a message.
**	12-jun-89 (jrb)
**	    Added generic error argument to ERlookup.
**	25-jul-89 (rogerk)
**	    Added generic error argument to ERlookup calls.
**	23-jan-1990 (daveb)
**	    Fix logic for test for params to be < 0 || > 9 instead of
**	    < 0 && > 9, which can never be true.  Discovered by ICL.
**	01-Oct-1990 (anton)
**	    Include cs.h for reentrancy support
**	    Add semaphore calls to protect access to message file.
**	23-oct-92 (andre)
**	    Changed interface to return SQLSTATE status code.  cer_sstr()'s
**	    intertface was changed to return SQLSTATE status code
**
**	    Upon request by the CL committee, renamed ERlookup() to ERslookup()
**	    and left ERlookup() around for a short while (until people who need
**	    to convert are done ceonverting)
**	30-jun-93 (shailaja)
**	    Fixed compiler warnings.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**      01-sep-93 (smc)
**          Commented lint pointer truncation warning.
**      19-may-1995 (canor01)
**          changed errno to errnum for reentrancy
**	07-jun-95 (hanch04)
**	    Bug #69072  added sttrmnwhite to remove white space
**	    from end of table/db name.
**	22-feb-1996 (prida01)
**	    Add cal to EXdump for server diagnostics
**	28-aug-96 (nick)
**	    Change processing of hexadecimal parameters to remove the '  0x'
**	    prefix. #70996
**      27-may-97 (mcgem01)
**          Clean up compiler warnings.
**	07-aug-1997 (canor01)
**	    Allow '%%' to be interpreted as literal '%', and a '%' by
**	    any non-substitution character to be literal.
**	31-Mar-98 (gordy)
**	    Support one byte integers as parameters.
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	18-jun-1999 (canor01)
**	    Add ER_NOPARAM flag for no parameter substitution.
**      18-Oct-99 (mosjo01/hweho01) SIR 97564
**          Moved include of ex.h to after cs.h to avoid compile
**          error " __jmpbuf is not a member of struct ex_context"
**          on AIX 4.3.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	15-feb-2001 (somsa01)
**	    Corrected compiler warnings.
**	21-sep-2001 (somsa01)
**	    Added code to handle the case where we are using the MU
**	    semaphore routines.
**      08-Oct-2002 (hanch04)
**          On systems that support 64-bit longs, allow the printing of
**	    8 byte longs.
**	20-Jul-2004 (lakvi01)
**		SIR #112703, cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**/


/*
**
**			RUN TIME MESSAGE FILE STRUCTURE
**
** * Slow message file
**
**     +------ index_count (i4) Number of index entries.
**    /
** +-/--+----+----+----+----------------+ --+
** |0003|0050|0100|0160|                |   |
** +----+-\--+----+----+----------------+   |
** |       +-------------------------------------- index_key[511] (i4)
** |		        	        |   |        Highest message in
** |	   	            	        |   |        corresponding bucket.
** |		 	                |   +-- INDEX_PAGE part
** |		 		        |   |	(2048byte)
** |			 	        |   |
** |			 	        |   |
** |		    	                |   |
** |				        |   |
** +--+--+----+--+-----+----------------+ --+
** |36|07|0000|15|50000|QBF0000_SUCCESS-|   |
** +--+--+----+--+-----+----------------|   |
** |success000|			        |   |   +-- MESSAGE_ENTRY part
** +----------+-------------------------+   |	(2048 * ? byte)
** |                                    |   |   message_entry is structure.
** |                                    |   |    me_length (i2) Length of the
** |			                |   |        message entry.
** |			                |   |    me_text_length (i2) Length of
** |			                |   |        the text portion.
** +------------------------------------+ --+    me_msg_number (i4) The message
**						     number for this message.
**						 me_name_length (i2) Length of 
**						     the name field.
**						 me_sqlstate (char[5]).
**						     SQLSTATE status code
**						 me_name_text[me_name_length+me_
**						     text_length+1] (char) Area
**						     for message name and
**						     message text.
**
** In the above example, the name and text of QBF0000_SUCCESS are separated
** by '-'.  All message lengths are a multiple of 4.
**
*/


/*}
** Name: DESCRIPTOR - A message string descriptor.
**
** Description:
**      A structure describing a variable length item in a message string system
**	call.
**
** History:
**     02-oct-1985 (derek)
**          Created new for 5.0.
*/
typedef struct _DESCRIPTOR
{
    int             desc_length;        /* Length of the item. */
    char            *desc_value;        /* Pointer to item. */
}   DESCRIPTOR;


/*
**  Definition of static variables and forward static functions.
*/

static  STATUS      er_exhandler();     /* Exception handler to catch any
					** exceptions caused by message 
					** parameter processing.  This can
					** happen when the message thinks
					** there are more parameters than
					** are actually passed in.
					*/


/*{
** Name:	ERslookup() -	Lookup an error message in the error file.
**
** Description:
**      Look up the text of the INGRES message associated with `msg_number',
**	substituting values passed in `param' for placeholders in the message
**	text.  If `clerror' is non-null, `param' is ignored.  If, in addition,
**	`msg_number' is 0, a system-dependent message about the CL error
**	described by `clerror' is retrieved instead of an INGRES message.
**	It is an error for both `msg_number' and `clerror' to be null.
**	The resulting message is returned in `msg_buf', which should point
**	to a buffer whose size is `msg_buf_size'. The actual message size is
**	placed in `msg_length'.   If the `SQLSTATE' pointer was non-null,
**	the SQLSTATE status code is filled in here.  `Language' specifies the
**	desired message language.
**
**	`Flags' can include:
**
**	    ER_TIMESTAMP    Affix a time stamp to the message.
**	    ER_TEXTONLY	    Omit the message name/number from INGRES messages.
**	    ER_NOPARAM	    Do not perform parameter substitution.
**
**
** Inputs:
**      msg_number                      The number of the message to look up.
**	clerror				(Poss. null) ptr to CL error descriptor.
**      flags                           Formatting options.
**	msg_buf				Pointer to message buffer.
**      msg_buf_size                    Size of the message buffer.
**	language			Language code for this session
**					(as found by ERlangcode).
**	num_param			Length of param.
**	param				ER_ARGUMENT vector.
**
** Outputs:
**	sqlstate			The SQLSTATE status code for this error.
**	err_code			CL error descriptor.
**      msg_length                      Resulting message text length.
**	err_code			OS system error, if status != OK.
**
** Returns:
**	    OK
**	    ER_NOT_FOUND
**	    ER_NO_FILE
**	    ER_BADOPEN
**	    ER_BADPARAM
**	    ER_BADLANGUAGE
**	    ER_TRUNCATED
**	    ER_TOOSMALL
**	    ER_BADREAD
**	    ER_DIRERR
**	    ER_NOALLOC
**
** Exceptions: 
**	    none
**
** Side Effects:
**	    none
**
** History:
**	04-oct-1985 (derek)
**          Created new for 5.0.
**	3-Apr-86 (fred)
**	    Modified code to correctly print %#x constants.
**	17-Sep-1986 (ericj)
**	    Fixed null-terminated string formatting. Made returned message
**	    null-terminated also.
**	17-sep-1986 (peter)
**	    Added language parameter.
**	01-Oct-1986 (kobayashi)
**	    Change source code for KANJI.
**	02-apr-1987 (peter)
**	    Add support for ER_PTR_ARGUMENT in ER_ARGUMENT length to indicate
**	    pointers for int, hex and float. This is for use by
**	    the frontends in order to pass variable argument
**	    lists instead of control blocks. If the er_argument
**	    is set to ER_PTR_ARGUMENT, the corresponding value is a pointer.
**	    This applies to all of the formats. By definition, if
**	    the format is 'd' the pointer is to a i4, if it
**	    is 'f' it is a pointer to a double, if it is a 'c' it is 
**	    a pointer to a NULL terminated string. if it is an 'x'
**	    it is a pointer to an u_i4.
**	25-jun-1987 (peter)
**	    Add support for the test mode of files.  If this is set
**	    on, then the file is closed after each message, using 
**	    ERclose, and the message file for the class is used.
**	    See comments in ERinit for details.
**	08-jul-1988 (roger)
**	     Added ER_UNIXERROR generic message for formatting UNIX
**	    system errors and changed this function to look it up
**	    by calling itself, passing CL_ERR_DESC fields as message
**	    parameters.
**	     Added ER_TEXTONLY flag, which prevents the message name
**	    from being retrieved, just the text.  This is like ERget(),
**	    except that parameter substitution is still done.  It is
**	    provided so the caller can concatentate two or more messages,
**	    specifically, an INGRES and an OS error message.  Necessitated
**	    by the addition of the generic message above, which is used
**	    to format all UNIX system errors.
**	20-jul-1988 (roger)
**	     Provided for passing system-dependent data in CL_ERR_DESC to
**	    be used as message parameters.  If a message is looked up
**	    and the `clerror' argument is non-null, `param' is reset to
**	    reference a "hidden" ER_ARGUMENT block used to pass data in
**	    clerror->moreinfo.  Alternatively, if clerror->intern is set,
**	    parameters for the corresponding error message can be passed
**	    in `moreinfo'.
**	    -Added semantic description for this horrid routine.
**	    -Don't bother keeping msg_length current; set once on success.
**	    -Removed unused variables, moved others to enclosing scopes.
**	07-oct-88 (thurston)
**	    Added code to declare an exception handler { er_exhandler() } to
**	    catch exceptions that happen if there are missing parameters for a
**	    message.
**	24-jan-1989 (roger)
**	    Bit o' defensive programming: added array bounds checking macro
**	    ERNAME(i) to index ERnames[] table, in case we're looking up
**	    garbage from auto space.
**	10-mar-1989 (roger)
**	    Added num_param argument per CL Committee.
**	12-jun-89 (jrb)
**	    Added generic_error argument to this function.
**	23-oct-92 (andre)
**	    Added sqlstate argument to this function.
**
**	    Upon request by the CL committee, renamed ERlookup() to ERslookup()
**	    and left ERlookup() around for a short while (until people who need
**	    to convert are done ceonverting)
**	13-Mch-1996 (prida01)
**	    Add extra parameter to EXdump
**	28-aug-96 (nick)
**	    Remove '  0x' from the output of hexadecimal parameters ; this
**	    isn't what is supposed to happen and makes it impossible to print
**	    out 64 bit values such as transaction ids.
**	31-Mar-98 (gordy)
**	    Support one byte integers as parameters.
**	18-jun-1999 (canor01)
**	    Add ER_NOPARAM flag for no parameter substitution.
**	21-Feb-2005 (thaju02)
**	    Added support of unsigned param.
**	16-Jun-2006 (kschendel)
**	    Do i8's properly.
*/

STATUS
ERslookup(
    i4		    msg_number,
    CL_ERR_DESC	    *clerror,
    i4		    flags,
    char	    *sqlstate,
    char	    *msg_buf,
    i4		    msg_buf_size,
    i4		    language,
    i4		    *msg_length,
    CL_ERR_DESC	    *err_code,
    i4		    num_param,
    ER_ARGUMENT	    *param )
{
    i4			erindex;	/* index of ERmulti table */
    i4		status;
    i4			length = 0;
    ER_ARGUMENT		*p;
    ER_ARGUMENT		hidden[CLE_INFO_ITEMS];	/* to access info in clerror */
    char		tempbuf[ER_MAX_LEN+ER_MAX_NAME+2];
    i4			templen;
    char		*p_msg_buf;
    char		*p_tempbuf;
    SYSTIME		stime;
    char		langbuf[ER_MAX_LANGSTR];
    EX_CONTEXT		context;
    ER_SEMFUNCS		*sems;

#define			    D_WIDTH	    23
#define			    F_WIDTH	    20
#define			    X_WIDTH	    18


    /*	Validate the parameters. */

    if (msg_buf == 0 || msg_buf_size == 0 || msg_length == 0)
    {
	return (ER_BADPARAM);
    }
    if (language != -1 && ERlangstr(language,langbuf) != OK)
    {
	return (ER_BADLANGUAGE);
    }
 
    if (!(flags & ER_NAMEONLY))
    {
        EXdump(msg_number,0);
    }
 

    /*	Insert timestamp if requested. */

    if (flags & ER_TIMESTAMP)
    {
	if (msg_buf_size < 21)
	{
	    return (ER_TOOSMALL);
	}
	TMnow(&stime);
	TMstr(&stime,msg_buf);

	length = (i4)STlength(msg_buf);	
	msg_buf[length++] = ' ';
    }

    /*
    **    if (clerror && msg_number)
    **        look up msg_number, optional parameters in clerror->moreinfo
    **    else if (clerror)
    **    {
    **        if (clerror->intern)
    **            look up clerror.intern, optional params in clerror->moreinfo
    **        if (clerror->callid)
    **            look up system error message
    **    }
    */
    if (clerror)
    {
	if (msg_number)	/* Look up message after system error */
	{
	    /*
	    ** Set up an ER_ARGUMENT that references system-dependent
	    ** information in `clerror', and point `param' at it.
	    */
	    i4  i;

	    for (i = 0; i < CLE_INFO_ITEMS; ++i)
	    {
	        /* "...all of whose members begin at offset 0..." (K&R) */

	        hidden[i].er_value = (PTR)&clerror->moreinfo[i].data._i4;
	        hidden[i].er_size = clerror->moreinfo[i].size;
	    }

	    param = &hidden[0];
	    num_param = CLE_INFO_ITEMS;
	}
        else		/* retrieve system-dependent error messages */
	{
	    i4	        len;
	    ER_ARGUMENT	argv[3];

	    if (clerror->intern)    /* look up internal CL error */
	    {
	        i4  i;

	        for (i = 0; i < CLE_INFO_ITEMS; ++i)
	        {
	            argv[i].er_value = (PTR)&clerror->moreinfo[i].data._i4;
	            argv[i].er_size = clerror->moreinfo[i].size;
	        }

	        /*
	        ** Don't timestamp on recursive call, since it's been done
	        ** already (if requested).
	        */
	        if ((status = ERslookup((i4) clerror->intern,
		    (CL_ERR_DESC*) NULL, flags & ~ER_TIMESTAMP | ER_TEXTONLY,
		    NULL, &msg_buf[length], msg_buf_size-length, language, &len,
		    err_code, CLE_INFO_ITEMS, argv)) != OK)
	        {
	            return (status);
	        }
	        length += len;

		if (clerror->callid)
		    msg_buf[length++] = '\n';
	    }

	    if (clerror->callid)    /* look up system error message text */
	    {
                DESCRIPTOR	msg_desc;

    	        msg_desc.desc_length = sizeof(tempbuf) - 1;
	        msg_desc.desc_value = tempbuf;
	        if ((status = cer_sysgetmsg(clerror, &len, &msg_desc, err_code))
		    != OK)
	        {
	            return(status);
	        }

	        argv[0].er_size = argv[1].er_size = argv[2].er_size =
		    ER_PTR_ARGUMENT;

	        argv[0].er_value = (PTR)&clerror->errnum;
	        argv[1].er_value = (PTR)ERNAME((i4) clerror->callid);
	        argv[2].er_value = (PTR)tempbuf;

	        if ((status = ERslookup(ER_UNIXERROR, (CL_ERR_DESC*) NULL,
	                flags & ~ER_TIMESTAMP | ER_TEXTONLY, NULL,
			&msg_buf[length], msg_buf_size - length, language,
			&len,err_code, 3, argv))
		    != OK)
	        {
	            return (status);
	        }
	        length += len;
	    }

	    msg_buf[*msg_length = length] = EOS;
	    return (OK);
        }
    }


    /*
    **	Check if error message file is already opened or not yet. 
    **  First see if the language is initialized.  If not, initialize
    **  it and the message files.
    **	If it is already opened, cer_fndindex function returns the index of
    **	ERmulti table that internal language code is parameter 'language'.
    **  If not yet, it returns '-1'.
    */

    if (cer_issem(&sems))
    {
	if (((sems->sem_type & MU_SEM) ?
	    (*sems->er_p_semaphore)(&sems->er_mu_sem) :
	    (*sems->er_p_semaphore)(1, &sems->er_sem)) != OK)
	{
	    sems = NULL;
	}
    }

    if ((erindex = cer_fndindex(language)) == -1)
    {
	if ((status = cer_nxtindex(language,&erindex)) != OK)
	{   /* Error in initializing the language */
	    if (sems)
	    {
		if (sems->sem_type & MU_SEM)
		    _VOID_ (*sems->er_v_semaphore)(&sems->er_mu_sem);
		else
		    _VOID_ (*sems->er_v_semaphore)(&sems->er_sem);
	    }
	    return (status);
	}
    }

    /*  If the error message file is not opened, open the message file. */

    if (!cer_isopen(erindex,ER_SLOWSIDE))
    {
        if ((status = cer_sinit(language,msg_number,erindex,err_code)) != OK)
        {
	    if (sems)
	    {
		if (sems->sem_type & MU_SEM)
		    _VOID_ (*sems->er_v_semaphore)(&sems->er_mu_sem);
		else
		    _VOID_ (*sems->er_v_semaphore)(&sems->er_sem);
	    }
            return (status);
        }
    }

    /*	If not open then just return. */
    if (!cer_isopen(erindex,ER_SLOWSIDE))
    {
	if (sems)
	{
	    if (sems->sem_type & MU_SEM)
		_VOID_ (*sems->er_v_semaphore)(&sems->er_mu_sem);
	    else
		_VOID_ (*sems->er_v_semaphore)(&sems->er_sem);
	}
	/*
	**  As internal file id is '0', openning file will fail.
	**  In her,return status 'ER_BADOPEN' to show open fail.
	*/
	return (ER_BADOPEN);
    }
	
    /*
    **  Search message string from file and set to buffer.
    **	Error status on system call set to 'err_code'.
    */

    status = cer_sstr(msg_number, sqlstate, tempbuf,
			msg_buf_size - length, erindex, err_code,
			flags & ER_TEXTONLY? ER_GET : ER_LOOKUP);
    if (sems)
    {
	if (sems->sem_type & MU_SEM)
	    _VOID_ (*sems->er_v_semaphore)(&sems->er_mu_sem);
	else
	    _VOID_ (*sems->er_v_semaphore)(&sems->er_sem);
    }

    if (status != OK)
    {
	return (status);
    }

    /*
    **	Format the text with parameters into the callers buffer.
    **  The message is truncated if it will not fit.
    */

    /*  Insert part of name from temporary buffer to buffer */

    status = OK;
    templen = (i4)STlength(tempbuf);
    p_msg_buf = &msg_buf[length];
    p_tempbuf = tempbuf;
    if (!(flags & ER_TEXTONLY))
    {
        while(*p_tempbuf != '\t')
        {
	    CMcpyinc(p_tempbuf,p_msg_buf);
        }
        CMcpyinc(p_tempbuf,p_msg_buf);
    }
    

    /* ============================================ */
    /* Copy text to message substituting arguments. */
    /* -------------------------------------------- */
    /* (But first, declare an exception handler to  */
    /*  catch bad params that may access violate.)  */
    /* ============================================ */

    if (EXdeclare(er_exhandler, &context))
    {
	u_i4	res_len;
	u_i4	bytes_left_in_buf;

	bytes_left_in_buf = (u_i4)(msg_buf_size - (p_msg_buf - msg_buf));
	res_len = STlen( STncpy(p_msg_buf,
   ERx("*** ERslookup() ERROR: Missing or bad parameter for this message. ***"),
	bytes_left_in_buf ));
	p_msg_buf[ bytes_left_in_buf - 1 ] = EOS;
	p_msg_buf += res_len;
	*msg_length = (i4)(p_msg_buf - msg_buf);
	EXdelete();
	return (OK);
    }


    for( ;p_tempbuf - tempbuf < templen; CMnext(p_tempbuf))
    {
        long		number;
        u_long    	unumber;
        double		fnumber;
	i4		i;
	i4		pnum;
 
	if ( (*p_tempbuf != '%') || (flags & ER_NOPARAM) )
	{
	    if ((p_msg_buf - msg_buf) >= msg_buf_size)
		break;
	    CMcpychar(p_tempbuf,p_msg_buf);
	    CMnext(p_msg_buf);
	    continue;
	}
	if (p_tempbuf - tempbuf + 2 >= templen)
	    continue;
	CMnext(p_tempbuf);
	if (*p_tempbuf == '!')
	{
	    if ((p_msg_buf - msg_buf) + 3 >= msg_buf_size)
		continue;
	    CMcpychar(ERx("\r"),p_msg_buf);
	    CMnext(p_msg_buf);
	    CMcpychar(ERx("\n"),p_msg_buf);
	    CMnext(p_msg_buf);
	    CMcpychar(ERx("\t"),p_msg_buf);
	    CMnext(p_msg_buf);
	    continue;
	}
	/*
	** Only works for up to 10 parameters, and makes character set
	** assumptions - should be fixed.
	*/
	if ( *p_tempbuf < '0' || *p_tempbuf > '9' )
	{
	    /* treat any other character as a literal */
	    if ((p_msg_buf - msg_buf) >= msg_buf_size)
		break;
	    if ( *p_tempbuf != '%' )
	    {
	        CMcpychar("%",p_msg_buf);
	        CMnext(p_msg_buf);
	    }
	    CMcpychar(p_tempbuf,p_msg_buf);
	    CMnext(p_msg_buf);
	    continue;
	}
	pnum = *p_tempbuf - '0';
	if (pnum >= num_param)
	{
		EXdelete();
		return(ER_BADPARAM);
	}
	p = &param[pnum];
	CMnext(p_tempbuf);
	switch (*p_tempbuf)
	{
	  case 'd':
	    /* Convert an integer into the buffer with width D_WIDTH */

	    if (p->er_size == ER_PTR_ARGUMENT) /* this is ptr to i4 */
		number = *(i4 *)p->er_value;
	    else if (p->er_size == 0)   /* this is a i4 */
		number = (i4)(SCALARP)p->er_value;
	    else if (p->er_size == 1)
		number = *(i1 *)p->er_value;
	    else if (p->er_size == 2)
		number = *(i2 *)p->er_value;
	    else if (p->er_size == 4)
		number = *(i4 *)p->er_value;
	    else if (p->er_size == 8)
		number = *(i8 *)p->er_value;
	    else
		continue;

	    if (p_msg_buf - msg_buf + D_WIDTH >= msg_buf_size)
	        continue;

	    if (p->er_size == 8)
	    {
	        CVla8(number, p_msg_buf);
	    }
	    else
	    {
	        CVla((i4)number, p_msg_buf);
	    }
	    while (*p_msg_buf)
		CMnext(p_msg_buf);
	    continue;

	  case 'u':
	    /* Convert an integer into the buffer with width D_WIDTH */

	    if (p->er_size == ER_PTR_ARGUMENT) /* this is ptr to u_i4 */
		number = *(u_i4 *)p->er_value;
	    else if (p->er_size == 0)   /* this is a u_i4 */
		number = (u_i4)(SCALARP)p->er_value;
	    else if (p->er_size == 1)
		number = *(u_i1 *)p->er_value;
	    else if (p->er_size == 2)
		number = *(u_i2 *)p->er_value;
	    else if (p->er_size == 4)
		number = *(u_i4 *)p->er_value;
	    else if (p->er_size == 8)
		number = *(u_i8 *)p->er_value;
	    else
		continue;

	    if (p_msg_buf - msg_buf + D_WIDTH >= msg_buf_size)
	        continue;

	    if (p->er_size == 8)
	    {
	        CVula8(number, p_msg_buf);
	    }
	    else
	    {
	        CVula((u_i4)number, p_msg_buf);
	    }
	    while (*p_msg_buf)
		CMnext(p_msg_buf);
	    continue;

	  case 'f':
	  {
	    i2	    res_width;

	    /* Convert a float into the buffer with width F_WIDTH */

	    if (p->er_size == ER_PTR_ARGUMENT) /* Pointer to a double */
		fnumber = *(double *)p->er_value;
	    else if (p->er_size == 4)
		fnumber = *(f4 *)p->er_value;
	    else if (p->er_size == 8)
		fnumber = *(f8 *)p->er_value;
	    else
		continue;

	    if (p_msg_buf - msg_buf + F_WIDTH >= msg_buf_size)
		continue;

	    /* Always convert to 'e' format. */

	    CVfa(fnumber, (i4) 20, (i4) 5, 'e', '.', p_msg_buf, &res_width);
	    p_msg_buf += F_WIDTH;
	    continue;
	  }
	  case 'c':
	    /* Convert a character array into buffer. */

	    if (p->er_value == 0)
		p->er_value = (PTR)ERx("<missing>");
	    if ((p->er_size == 0) || (p->er_size == ER_PTR_ARGUMENT))
	    {
		for (i = 0; ((char *)p->er_value)[i]; i++)
		    ;
		p->er_size = i;
	    }
             
	    if (p_msg_buf - msg_buf + p->er_size >= msg_buf_size)
		continue;

	    if (p->er_size > msg_buf_size - (p_msg_buf - msg_buf))
		p->er_size = (i4)(msg_buf_size - (p_msg_buf - msg_buf));
/*	    p->er_size=STtrmwhite(p_msg_buf);*/
	    MEcopy(p->er_value, p->er_size, p_msg_buf);
	    p->er_size = (i4)STtrmnwhite(p_msg_buf, p->er_size);
	    p_msg_buf += p->er_size;
	    continue;

	  case 'x':
	    /* Convert an integer into the buffer with width D_WIDTH */

	    if (p->er_size == ER_PTR_ARGUMENT)
		unumber = *(u_i4 *)p->er_value;
	    else if (p->er_size == 0)
		unumber = (u_i4)(SCALARP)p->er_value;
	    else if (p->er_size == 1)
		unumber = *(u_i1 *)p->er_value;
	    else if (p->er_size == 2)
		unumber = *(u_i2 *)p->er_value;
	    else if (p->er_size == 4)
		unumber = *(u_i4 *)p->er_value;
	    else if (p->er_size == 8)
		unumber = *(u_i8 *)p->er_value;

	    if (p_msg_buf - msg_buf + X_WIDTH >= msg_buf_size)
		continue;

	    for (i = 8; --i >= 0; )
	    {
		/* {@fix_me@}
		** This is *NOT* machine independent.  This relys on an
		** ASCII-like character set, where the digits '0'-'9' are
		** contiguous and sequential, and the characters 'A'-'F'
		** are contiguous and sequential.  Both ASCII and EBCDIC
		** happen to be this way.
		*/
		if ((*(p_msg_buf + i) = (unumber & 0x0f) + '0') > '9')
		    *(p_msg_buf + i) += 'A' - '9' - 1;
		unumber >>= 4;
	    }
	    p_msg_buf += 8;
	    continue;

	default:
	    continue;
	}
    }
    *msg_length = (i4)(p_msg_buf - msg_buf);
    *p_msg_buf = EOS;
    EXdelete();
    return (OK);
}

/*
** this is a temporary stub for ERlookup().  It will go away as of the next
** integration.
*/ 
STATUS
ERlookup(
    i4		    msg_number,
    CL_ERR_DESC	    *clerror,
    i4		    flags,
    i4		    *generic_error,
    char	    *msg_buf,
    i4		    msg_buf_size,
    i4		    language,
    i4		    *msg_length,
    CL_ERR_DESC	    *err_code,
    i4		    num_param,
    ER_ARGUMENT	    *param )
{
    STATUS	stat;
    char	sqlstate[5];

    stat = ERslookup(msg_number, clerror, flags, sqlstate, msg_buf,
	msg_buf_size, language, msg_length, err_code, num_param, param);

    if (stat != OK || generic_error == (i4 *) NULL)
	return(stat);

    /*
    ** now convert SQLSTATE to a generic error
    **
    ** here we are using a (AWK-filtered) copy of the routine converting
    ** SQLSTATEs to generic errors (I am only doing it [making a copy of an ADF
    ** function] because this stub has a month and a half to live)
    */

    {
	i4        ss_class, ss_subclass;
	static struct
	{
	    i4		dbmserr;
	    i4		generr;
	} numeric_exception[] =
	{
	    { /* E_AD1127_DECOVF_ERROR */	0x021127L,	0x9D0B },
	    { /* E_GW7015_DECIMAL_OVF */	0x0F7015L,	0x9D0B },
	    { /* E_AD1013_EMBEDDED_NUMOVF */	0x021013L,	0x9D1C },
	    { /* E_CL0503_CV_OVERFLOW */	0x010503L,	0x9D1C },
	    { /* E_GW7000_INTEGER_OVF */	0x0F7000L,	0x9D1C },
	    { /* E_US1068_4200 */		0x001068L,	0x9D1C },
	    { /* E_GW7001_FLOAT_OVF */		0x0F7001L,	0x9D1D },
	    { /* E_GW7004_MONEY_OVF */		0x0F7004L,	0x9D1D },
	    { /* E_US106A_4202 */		0x00106AL,	0x9D1D },
	    { /* E_CL0502_CV_UNDERFLOW */	0x010502L,	0x9D21 },
	    { /* E_GW7002_FLOAT_UND */		0x0F7002L,	0x9D22 },
	    { /* E_GW7005_MONEY_OVF */		0x0F7005L,	0x9D22 },
	    { /* E_US106C_4204 */		0x00106CL,	0x9D22 },
	    { -1L, -1L},
	};
    
	static struct
	{
	    i4         dbmserr;
	    i4         generr;
	} division_by_zero[] =
	{
	    { /* E_US1069_4201 */		0x001069L,	0x9D1E },
	    { /* E_US106B_4203 */		0x00106BL,	0x9D1F },
	    { /* E_AD1126_DECDIV_ERROR */	0x021126L,	0x9D20 },
	    { -1L, -1L},
	};
    
	/*
	** convert SQLSTATE class (treated as a base 36 number) to a base 10 integer
	*/
	if (sqlstate[0] > '0' && sqlstate[0] <= '9')
	    ss_class = (sqlstate[0] - '0') * 36;
	else if (sqlstate[0] >= 'A' && sqlstate[0] <= 'Z')
	    ss_class = (sqlstate[0] - 'A' + 10) * 36;
	else
	    ss_class = 0;
    
	if (sqlstate[1] > '0' && sqlstate[1] <= '9')
	    ss_class += sqlstate[1] - '0';
	else if (sqlstate[1] >= 'A' && sqlstate[1] <= 'Z')
	    ss_class += sqlstate[1] - 'A' + 10;
    
	/*
	** convert SQLSTATE subclass (treated as a base 36 number) to a base 10
	** integer
	*/
	if (sqlstate[2] > '0' && sqlstate[2] <= '9')
	    ss_subclass = 36 * 36 * (sqlstate[2] - '0');
	else if (sqlstate[2] >= 'A' && sqlstate[2] <= 'Z')
	    ss_subclass = 36 * 36 * (sqlstate[2] - 'A' + 10);
	else
	    ss_subclass = 0;
    
	if (sqlstate[3] > '0' && sqlstate[3] <= '9')
	    ss_subclass += 36 * (sqlstate[3] - '0');
	else if (sqlstate[3] >= 'A' && sqlstate[3] <= 'Z')
	    ss_subclass += 36 * (sqlstate[3] - 'A' + 10);
	
	if (sqlstate[4] > '0' && sqlstate[4] <= '9')
	    ss_subclass += sqlstate[4] - '0';
	else if (sqlstate[4] >= 'A' && sqlstate[4] <= 'Z')
	    ss_subclass += sqlstate[4] - 'A' + 10;
    
	switch (ss_class)
	{
	    case 120:
	    {
		switch (ss_subclass)
		{
		    default:
		    {
			*generic_error = 0x9DD0L;
			break;
		    }
		}
    
		break;
	    }
	    case 73:
	    {
		switch (ss_subclass)
		{
		    default:
		    {
			*generic_error = 0x9CA4L;
			break;
		    }
		}
    
		break;
	    }
	    case 8:
	    {
		switch (ss_subclass)
		{
		    case 3:
		    {
			*generic_error = 0x80E8L;
			break;
		    }
		    case 6:
		    {
			*generic_error = 0x9088L;
			break;
		    }
		    case 2:
		    {
			*generic_error = 0x80E8L;
			break;
		    }
		    case 1:
		    {
			*generic_error = 0x98BCL;
			break;
		    }
		    case 4:
		    {
			*generic_error = 0x94D4L;
			break;
		    }
		    case 7:
		    {
			*generic_error = 0x9088L;
			break;
		    }
		    case 6480:
		    {
			*generic_error = 0x75BCL;
			break;
		    }
		    default:
		    {
			*generic_error = 0x98BCL;
			break;
		    }
		}
    
		break;
	    }
	    case 74:
	    {
		switch (ss_subclass)
		{
		    case 73:
		    {
			*generic_error = 0x9D08L;
			break;
		    }
		    case 8:
		    {
			*generic_error = 0x9D0EL;
			break;
		    }
		    case 38:
		    {
			i4    i;
    
			for (i = 0;
			     (   division_by_zero[i].dbmserr != -1L
			      && division_by_zero[i].dbmserr != msg_number);
			     i++)
			;
    
			if (division_by_zero[i].dbmserr == -1L)
			    *generic_error = 0x9D24L;
			else
			    *generic_error = division_by_zero[i].generr;
    
			break;
		    }
		    case 5:
		    {
			*generic_error = 0x9D0CL;
			break;
		    }
		    case 74:
		    {
			*generic_error = 0x9D11L;
			break;
		    }
		    case 41:
		    {
			*generic_error = 0x9D0FL;
			break;
		    }
		    case 44:
		    {
			*generic_error = 0x7918L;
			break;
		    }
		    case 7:
		    {
			*generic_error = 0x9D0FL;
			break;
		    }
		    case 45:
		    {
			*generic_error = 0x7918L;
			break;
		    }
		    case 77:
		    {
			*generic_error = 0x7918L;
			break;
		    }
		    case 75:
		    {
			*generic_error = 0x9D08L;
			break;
		    }
		    case 9:
		    {
			*generic_error = 0x9D0FL;
			break;
		    }
		    case 2:
		    {
			*generic_error = 0x9D0AL;
			break;
		    }
		    case 3:
		    {
			i4    i;
    
			for (i = 0;
			     (   numeric_exception[i].dbmserr != -1L
			      && numeric_exception[i].dbmserr != msg_number);
			     i++)
			;
    
			if (numeric_exception[i].dbmserr == -1L)
			    *generic_error = 0x9D24L;
			else
			    *generic_error = numeric_exception[i].generr;
    
			break;
		    }
		    case 78:
		    {
			*generic_error = 0x9D08L;
			break;
		    }
		    case 1:
		    {
			*generic_error = 0x9D09L;
			break;
		    }
		    case 37:
		    {
			*generic_error = 0x80E8L;
			break;
		    }
		    case 79:
		    {
			*generic_error = 0x7918L;
			break;
		    }
		    case 76:
		    {
			*generic_error = 0x98BCL;
			break;
		    }
		    case 6480:
		    {
			*generic_error = 0x9D17L;
			break;
		    }
		    default:
		    {
			*generic_error = 0x9D08L;
			break;
		    }
		}
    
		break;
	    }
	    case 83:
	    {
		switch (ss_subclass)
		{
		    default:
		    {
			*generic_error = 0x7D00L;
			break;
		    }
		}
    
		break;
	    }
	    case 7:
	    {
		switch (ss_subclass)
		{
		    case 3:
		    {
			*generic_error = 0x7D00L;
			break;
		    }
		    case 8:
		    {
			*generic_error = 0x7D00L;
			break;
		    }
		    case 9:
		    {
			*generic_error = 0x7D00L;
			break;
		    }
		    case 5:
		    {
			*generic_error = 0x7D00L;
			break;
		    }
		    case 6:
		    {
			*generic_error = 0x7D00L;
			break;
		    }
		    case 1:
		    {
			*generic_error = 0x7D00L;
			break;
		    }
		    case 2:
		    {
			*generic_error = 0x7D00L;
			break;
		    }
		    case 4:
		    {
			*generic_error = 0x7D00L;
			break;
		    }
		    case 7:
		    {
			*generic_error = 0x7D00L;
			break;
		    }
		    case 6480:
		    {
			*generic_error = 0x98BCL;
			break;
		    }
		    default:
		    {
			*generic_error = 0x7D00L;
			break;
		    }
		}
    
		break;
	    }
	    case 10:
	    {
		switch (ss_subclass)
		{
		    case 1:
		    {
			*generic_error = 0x98BCL;
			break;
		    }
		    case 6480:
		    {
			*generic_error = 0x79E0L;
			break;
		    }
		    default:
		    {
			*generic_error = 0x98BCL;
			break;
		    }
		}
    
		break;
	    }
	    case 75:
	    {
		switch (ss_subclass)
		{
		    default:
		    {
			*generic_error = 0x9D6CL;
			break;
		    }
		}
    
		break;
	    }
	    case 80:
	    {
		switch (ss_subclass)
		{
		    default:
		    {
			*generic_error = 0xA028L;
			break;
		    }
		}
    
		break;
	    }
	    case 121:
	    {
		switch (ss_subclass)
		{
		    default:
		    {
			*generic_error = 0x98BCL;
			break;
		    }
		}
    
		break;
	    }
	    case 84:
	    {
		switch (ss_subclass)
		{
		    default:
		    {
			*generic_error = 0x7918L;
			break;
		    }
		}
    
		break;
	    }
	    case 113:
	    {
		switch (ss_subclass)
		{
		    default:
		    {
			*generic_error = 0x7D00L;
			break;
		    }
		}
    
		break;
	    }
	    case 86:
	    {
		switch (ss_subclass)
		{
		    default:
		    {
			*generic_error = 0x797CL;
			break;
		    }
		}
    
		break;
	    }
	    case 112:
	    {
		switch (ss_subclass)
		{
		    default:
		    {
			*generic_error = 0x75A8L;
			break;
		    }
		}
    
		break;
	    }
	    case 76:
	    {
		switch (ss_subclass)
		{
		    default:
		    {
			*generic_error = 0x9DD0L;
			break;
		    }
		}
    
		break;
	    }
	    case 123:
	    {
		switch (ss_subclass)
		{
		    default:
		    {
			*generic_error = 0x797CL;
			break;
		    }
		}
    
		break;
	    }
	    case 111:
	    {
		switch (ss_subclass)
		{
		    default:
		    {
			*generic_error = 0x75BCL;
			break;
		    }
		}
    
		break;
	    }
	    case 78:
	    {
		switch (ss_subclass)
		{
		    default:
		    {
			*generic_error = 0x75B2L;
			break;
		    }
		}
    
		break;
	    }
	    case 77:
	    {
		switch (ss_subclass)
		{
		    default:
		    {
			*generic_error = 0x9E34L;
			break;
		    }
		}
    
		break;
	    }
	    case 85:
	    {
		switch (ss_subclass)
		{
		    default:
		    {
			*generic_error = 0x9E34L;
			break;
		    }
		}
    
		break;
	    }
	    case 2:
	    {
		switch (ss_subclass)
		{
		    default:
		    {
			*generic_error = 0x0064L;
			break;
		    }
		}
    
		break;
	    }
	    case 647:
	    {
		switch (ss_subclass)
		{
		    default:
		    {
			*generic_error = 0x9088L;
			break;
		    }
		}
    
		break;
	    }
	    case 0:
	    {
		switch (ss_subclass)
		{
		    default:
		    {
			*generic_error = 0x0000L;
			break;
		    }
		}
    
		break;
	    }
	    case 146:
	    {
		switch (ss_subclass)
		{
		    case 6480:
		    {
			*generic_error = 0x7594L;
			break;
		    }
		    case 6481:
		    {
			*generic_error = 0x759EL;
			break;
		    }
		    case 6482:
		    {
			*generic_error = 0x75F8L;
			break;
		    }
		    case 6483:
		    {
			*generic_error = 0x84D0L;
			break;
		    }
		    case 6484:
		    {
			*generic_error = 0x75A8L;
			break;
		    }
		    case 6485:
		    {
			*generic_error = 0x75B2L;
			break;
		    }
		    case 6486:
		    {
			*generic_error = 0x797CL;
			break;
		    }
		    case 6487:
		    {
			*generic_error = 0x797CL;
			break;
		    }
		    default:
		    {
			*generic_error = 0x7918L;
			break;
		    }
		}
    
		break;
	    }
	    case 82:
	    {
		switch (ss_subclass)
		{
		    case 6480:
		    {
			*generic_error = 0x7594L;
			break;
		    }
		    case 6481:
		    {
			*generic_error = 0x759EL;
			break;
		    }
		    case 6482:
		    {
			*generic_error = 0x75F8L;
			break;
		    }
		    case 6483:
		    {
			*generic_error = 0x84D0L;
			break;
		    }
		    case 6484:
		    {
			*generic_error = 0x75A8L;
			break;
		    }
		    case 6485:
		    {
			*generic_error = 0x75B2L;
			break;
		    }
		    case 6486:
		    {
			*generic_error = 0x797CL;
			break;
		    }
		    case 6487:
		    {
			*generic_error = 0x797CL;
			break;
		    }
		    default:
		    {
			*generic_error = 0x7918L;
			break;
		    }
		}
    
		break;
	    }
	    case 115:
	    {
		switch (ss_subclass)
		{
		    case 6480:
		    {
			*generic_error = 0x7594L;
			break;
		    }
		    case 6481:
		    {
			*generic_error = 0x759EL;
			break;
		    }
		    case 6482:
		    {
			*generic_error = 0x75F8L;
			break;
		    }
		    case 6483:
		    {
			*generic_error = 0x84D0L;
			break;
		    }
		    case 6484:
		    {
			*generic_error = 0x75A8L;
			break;
		    }
		    case 6485:
		    {
			*generic_error = 0x75B2L;
			break;
		    }
		    case 6486:
		    {
			*generic_error = 0x797CL;
			break;
		    }
		    case 6487:
		    {
			*generic_error = 0x797CL;
			break;
		    }
		    default:
		    {
			*generic_error = 0x7918L;
			break;
		    }
		}
    
		break;
	    }
	    case 144:
	    {
		switch (ss_subclass)
		{
		    case 2:
		    {
			*generic_error = 0x9D6CL;
			break;
		    }
		    case 1:
		    {
			*generic_error = 0xC2ECL;
			break;
		    }
		    case 3:
		    {
			*generic_error = 0x9088L;
			break;
		    }
		    default:
		    {
			*generic_error = 0x98BCL;
			break;
		    }
		}
    
		break;
	    }
	    case 79:
	    {
		switch (ss_subclass)
		{
		    default:
		    {
			*generic_error = 0x9EFCL;
			break;
		    }
		}
    
		break;
	    }
	    case 1:
	    {
		switch (ss_subclass)
		{
		    case 1:
		    {
			*generic_error = 0x0032L;
			break;
		    }
		    case 2:
		    {
			*generic_error = 0x0032L;
			break;
		    }
		    case 8:
		    {
			*generic_error = 0x0032L;
			break;
		    }
		    case 5:
		    {
			*generic_error = 0x0032L;
			break;
		    }
		    case 3:
		    {
			*generic_error = 0x0032L;
			break;
		    }
		    case 7:
		    {
			*generic_error = 0x0032L;
			break;
		    }
		    case 6:
		    {
			*generic_error = 0x0032L;
			break;
		    }
		    case 10:
		    {
			*generic_error = 0x0032L;
			break;
		    }
		    case 9:
		    {
			*generic_error = 0x0032L;
			break;
		    }
		    case 4:
		    {
			*generic_error = 0x0032L;
			break;
		    }
		    case 6480:
		    {
			*generic_error = 0x0032L;
			break;
		    }
		    case 6481:
		    {
			*generic_error = 0x0032L;
			break;
		    }
		    default:
		    {
			*generic_error = 0x0032L;
			break;
		    }
		}
    
		break;
	    }
	    case 148:
	    {
		switch (ss_subclass)
		{
		    default:
		    {
			*generic_error = 0x7D00L;
			break;
		    }
		}
    
		break;
	    }
	    case 180:
	    {
		switch (ss_subclass)
		{
		    case 1:
		    {
			*generic_error = 0x7602L;
			break;
		    }
		    case 2:
		    {
			*generic_error = 0x8CA0L;
			break;
		    }
		    case 3:
		    {
			*generic_error = 0x8D04L;
			break;
		    }
		    case 4:
		    {
			*generic_error = 0x8D68L;
			break;
		    }
		    case 5:
		    {
			*generic_error = 0x9470L;
			break;
		    }
		    case 6:
		    {
			*generic_error = 0x9858L;
			break;
		    }
		    case 7:
		    {
			*generic_error = 0x9E98L;
			break;
		    }
		    case 8:
		    {
			*generic_error = 0xA0F0L;
			break;
		    }
		    case 9:
		    {
			*generic_error = 0xA154L;
			break;
		    }
		    case 10:
		    {
			*generic_error = 0x7D00L;
			break;
		    }
		    case 11:
		    {
			*generic_error = 0x98BCL;
			break;
		    }
		    case 12:
		    {
			*generic_error = 0x9D0DL;
			break;
		    }
		    case 13:
		    {
			*generic_error = 0x9D12L;
			break;
		    }
		    case 14:
		    {
			*generic_error = 0xA21CL;
			break;
		    }
		    case 15:
		    {
			*generic_error = 0x98BCL;
			break;
		    }
		    case 16:
		    {
			*generic_error = msg_number;
			break;
		    }
		    case 17:
		    {
			*generic_error = 0x75BCL;
			break;
		    }
		    case 18:
		    {
			*generic_error = 0x98BCL;
			break;
		    }
		    case 19:
		    {
			*generic_error = 0x98BCL;
			break;
		    }
		    case 20:
		    {
			*generic_error = 0x98BCL;
			break;
		    }
		    case 21:
		    {
			*generic_error = 0x9088L;
			break;
		    }
		    case 22:
		    {
			*generic_error = 0x9088L;
			break;
		    }
		    default:
		    {
			*generic_error = 0x98BCL;
			break;
		    }
		}
    
		break;
	    }
	    default:
	    {
		*generic_error = 0x98BCL;
		break;
	    }
	}
    
    }

    return(OK);
}

/*
** Name: er_exhandler - Exception handler for message parameter processing.
**
** Description:
**      Catches exceptions incurred while processing message parameters.  This
**	can happen when the message thinks there are more parameters than the
**	caller of ERslookup() provided.
**
** Inputs:
**      ex_args                         Standard EX argument structure,
**					containing the exception that ocurred.
**
** Outputs:
**      none
**
** Returns:
**	    EXDECLARE	    If exception was either EXSEGVIO (AV's) or EXBUSERR.
**	    EXRESIGNAL	    Otherwise.
**
** Exceptions: 
**	    none
**
** Side Effects:
**	    none
**
** History:
**	07-oct-88 (thurston)
**          Created.
*/

static STATUS
er_exhandler( EX_ARGS *ex_args )
{
    if (    ex_args->exarg_num == EXSEGVIO
	||  ex_args->exarg_num == EXBUSERR
       )
	return (EXDECLARE);
    else
	return (EXRESIGNAL);
}
