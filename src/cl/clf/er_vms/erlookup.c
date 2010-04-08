/*
** Copyright (c) 1985, 2008 Ingres Corporation
*/
 
#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <st.h>
#include    <cm.h>
#include    <er.h>
#include    <ex.h>
#include    <tm.h>
#include    <cs.h>	    /* Needed for "erloc.h" */
#include    <cv.h>
#include    "erloc.h"
 
/**
**
**  Name: ERlookup.C - Add ERslookup function on the ER compatibility library
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
**	07-oct-88 (thurston)
**	    Added the static routine er_exhandler() to catch exceptions that
**	    happen if there are missing parameters for a message.
**	01-may-89 (jrb)
**	    Changed interface for generic error return.  Also changed call to
**	    cer_sstr for new generic_error parameter.
**	23-oct-92 (andre)
**	    Changed interface to return SQLSTATE status code.  cer_sstr()'s
**	    intertface was changed to return SQLSTATE status code
**
**	    Upon request by the CL committee, renamed ERlookup() to ERslookup()
**	    and left ERlookup() around for a short while (until people who need
**	    to convert are done ceonverting)
**      16-jul-93 (ed)
**	    added gl.h
**      11-aug-93 (ed)
**          added missing includes
**	07-jun-95 (albany, from wolf, for hanch04)
**	    Added sttrmnwhite call to remove white space from end of table/db 
**	    name. (69072)
**	05-feb-98 (kinte01)
**	    Cross integrate change from VAX VMS CL to Alpha CL
**          30-aug-96 (nick)
**            Remove '  0x' from the output of hexadecimal parameters ; this
**            isn't what is supposed to happen and makes it impossible to print
**            out 64 bit values such as transaction ids.
**	11-jun-1998 (kinte01)
**	    Cross integrate change 435890 from oping12
**          18-may-1998 (horda03)
**            X-Integrate Change 427896 to enable Evidence Sets.
**	31-Mar-98 (gordy)
**	    Support one byte integers as parameters.
**	18-jun-1999 (canor01)
**	    Add ER_NOPARAM for no parameter substitution.
**	19-jul-2000 (kinte01)
**	    Correct prototype definitions by adding missing includes
**	19-jul-2000 (kinte01)
**	   Add missing external function references
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	15-oct-2001 (kinte01)
**	   Correct compiler warnings
**	08-Oct-2002 (hanch04)
**	    On systems that support 64-bit longs, allow the printing of
**	    8 byte longs.
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

FUNC_EXTERN VOID EXdump();
FUNC_EXTERN EX_CONTEXT * i_EXtop();
FUNC_EXTERN EX_CONTEXT * i_EXnext();
FUNC_EXTERN VOID i_EXpop();
 

/*{
** Name: ERslookup	- Lookup an error message in the error file.
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
**	     Added ER_TEXTONLY flag, which prevents the message name
**	    from being retrieved, just the text.  This is like ERget(),
**	    except that parameter substitution is still done.  It is
**	    provided so the caller can concatentate two or more messages,
**	    specifically, an INGRES and an OS error message.
**	20-jul-1988 (roger)
**	    Added semantic description for this horrid routine.
**	    Don't bother keeping msg_length current; set once on success.
**	    Removed unused variables, moved others to enclosing scopes.
**	07-oct-88 (thurston)
**	    Added code to declare an exception handler { er_exhandler() } to
**	    catch exceptions that happen if there are missing parameters for a
**	    message.
**	07-feb-1989 (roger)
**	    Brought VMS implementation into conformance with specification:
**	    ignore `param' argument if `clerror' is non-null.  Also redefined
**	    (clerror->error == 0) as "no error", as a result of removing all
**	    tests for this case from mainline code, which should not examine
**	    the internals of `clerror'.
**	01-may-1989 (jrb)
**	    Added generic_error argument to this function.
**	23-oct-92 (andre)
**	    Added sqlstate argument to this function.
**
**	    Upon request by the CL committee, renamed ERlookup() to ERslookup()
**	    and left ERlookup() around for a short while (until people who need
**	    to convert are done ceonverting)
**	31-Mar-98 (gordy)
**	    Support one byte integers as parameters.
**	18-jun-1999 (canor01)
**	    Add ER_NOPARAM for no parameter substitution.
*/

STATUS
ERslookup(
i4		    msg_number,
CL_ERR_DESC	    *clerror,
i4		    flags,
char		    *sqlstate,
char		    *msg_buf,
i4		    msg_buf_size,
i4		    language,
i4		    *msg_length,
CL_ERR_DESC	    *err_code,
i4		    num_param,
ER_ARGUMENT	    *param)
{
    i4			erindex;	/* index of ERmulti table */
    i4			status;
    i4			length = 0;
    ER_ARGUMENT		*p;
    ER_ARGUMENT		nullarg;    /* substitute for `param' if (clerror) */
    char		tempbuf[ER_MAX_LEN+ER_MAX_NAME+2];
    i4			templen;
    char		*p_msg_buf;
    char		*p_tempbuf;
    SYSTIME		stime;
    char		langbuf[ER_MAX_LANGSTR];
    EX_CONTEXT		context;
 
#define			    D_WIDTH	    12
#define			    F_WIDTH	    20
#define			    X_WIDTH	    12
 
    /*	Validate the parameters. */
 
    CL_CLEAR_ERR(err_code);
    if (msg_buf == 0 || msg_buf_size == 0 || msg_length == 0)
    {
	return (ER_BADPARAM);
    }
    if (language != -1 && ERlangstr(language,langbuf) != OK)
    {
	return (ER_BADLANGUAGE);
    }
 
    if(!(flags & ER_NAMEONLY))
    {
        EXdump(msg_number);
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
 
    if (clerror && !msg_number)
    {
	if (clerror->error == 0)
	{
	    *msg_length = 0;    /* no-op; toss out timestamp if present */
	}
	else
	{
            DESCRIPTOR		msg_desc;
 
    	    msg_desc.desc_length = msg_buf_size - length;
	    msg_desc.desc_value = &msg_buf[length];
	    if ((status = cer_sysgetmsg(clerror->error, msg_length, &msg_desc,
	        err_code)) != OK)
	    {
	        return (status);
	    }
	    *msg_length += length;
	}
	msg_buf[*msg_length] = EOS;
	return (OK);
    }
 
    /*
    **	Check if error message file is already opened or not yet. 
    **  First see if the language is initialized.  If not, initialize
    **  it and the message files.
    **	If it is already opened, cer_fndindex function returns the index of
    **	ERmulti table that internal language code is parameter 'language'.
    **  If not yet, it returns '-1'.
    */
 
    if ((erindex = cer_fndindex(language)) == -1)
    {
	if ((status = cer_nxtindex(language,&erindex)) != OK)
	{   /* Error in initializing the language */
	    return (status);
	}
    }
 
    /*  If the error message file is not opened, open the message file. */
 
    if (!cer_isopen(erindex, ER_SLOWSIDE))
    {
        if ((status = cer_sinit(language, msg_number, erindex, err_code)) != OK)
        {
            return (status);
        }
    }
 
    /*	If not open then just return. */
    if (!cer_isopen(erindex, ER_SLOWSIDE))
    {
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
 
    if ((status = cer_sstr(msg_number, sqlstate, tempbuf,
	                   msg_buf_size - length, erindex, err_code,
			   flags & ER_TEXTONLY? ER_GET : ER_LOOKUP))
	!= OK)
    {
	return (status);
    }
 
    /*
    **	Format the text with parameters into the caller's buffer.
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

    /* If we only want the message name - return now */
 
    if(flags & ER_NAMEONLY)
    {
        *msg_length = p_msg_buf - msg_buf;
        *p_msg_buf = EOS;
        return(OK);
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
	res_len = STlcopy(ERx("*** ERslookup() ERROR: Missing or bad parameter \
 for this message. ***"), p_msg_buf, bytes_left_in_buf - 1);
	p_msg_buf += res_len;
	*msg_length = (i4)(p_msg_buf - msg_buf);
	EXdelete();
	return (OK);
    }
 
    if (clerror)
    {
	/*
	** Presumably someone outside of the CL is passing message
	** parameters for a CL error message, which is a no-no.
	** We are ignoring `param' in this case, substituting this
	** null message parameter instead.
	*/
	nullarg.er_value = 0;
	nullarg.er_size = ER_PTR_ARGUMENT;
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
	if (*p_tempbuf < '0' || *p_tempbuf > '9')
	    continue;
	pnum = *p_tempbuf - '0';
	if (!clerror && pnum >= num_param)
	{
		EXdelete();
		return(ER_BADPARAM);
	}
	p = clerror? &nullarg : &param[pnum];
	CMnext(p_tempbuf);
	switch (clerror? 'c' : *p_tempbuf)
	{
	  case 'd':
	    /* Convert an integer into the buffer with width D_WIDTH */
 
	    if (p->er_size == ER_PTR_ARGUMENT) /* this is ptr to i4 */
		number = *(i4 *)p->er_value;
	    else if (p->er_size == 0)   /* this is a i4 */
		number = (i4)p->er_value;
	    else if (p->er_size == 1)
		number = *(i1 *)p->er_value;
	    else if (p->er_size == 2)
		number = *(i2 *)p->er_value;
	    else if (p->er_size == 4)
		number = *(i4 *)p->er_value;
	    else if (p->er_size == 8)
		number = *(long *)p->er_value;
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
	    MEcopy(p->er_value, p->er_size, p_msg_buf);
	    p->er_size = (i4)STtrmnwhite(p_msg_buf, p->er_size);
	    p_msg_buf += p->er_size;
	    continue;
 
	  case 'x':
	    /* Convert an integer into the buffer with width D_WIDTH */
 
	    if (p->er_size == ER_PTR_ARGUMENT)
		unumber = *(u_i4 *)p->er_value;
	    else if (p->er_size == 0)
		unumber = (u_i4)p->er_value;
	    else if (p->er_size == 1)
		unumber = *(u_i1 *)p->er_value;
	    else if (p->er_size == 2)
		unumber = *(u_i2 *)p->er_value;
	    else if (p->er_size == 4)
		unumber = *(u_i4 *)p->er_value;
 
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
char		    *msg_buf,
i4		    msg_buf_size,
i4		    language,
i4		    *msg_length,
CL_ERR_DESC	    *err_code,
i4		    num_param,
ER_ARGUMENT	    *param)
{
    STATUS	stat;
    char	sqlstate[5];

    CL_CLEAR_ERR(err_code);
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
er_exhandler(
EX_ARGS	    *ex_args)
{
    if (    ex_args->exarg_num == EXSEGVIO
	||  ex_args->exarg_num == EXBUSERR
       )
	return (EXDECLARE);
    else
	return (EXRESIGNAL);
}
