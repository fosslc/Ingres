/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<me.h>		
# include	<st.h>	
# include	<te.h>
# include	<si.h>
# include	<er.h>
# include	<gl.h>
# include	<sl.h>
#ifndef NT_GENERIC
#include	<unistd.h>
#endif
# include	<si.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ug.h>
# include	<generr.h>
# include	<gca.h>
# include	<eqrun.h>
# include	<iicgca.h>
# include	<iisqlca.h>
# include	<iilibq.h>
# include	<erlq.h>
# include	<adh.h>

/* {
** Name: iidbprmt.c - Process database prompts
**
** Description:
**	This module contains the routines to handle DBMS prompts
**
** Defines:
**	IILQdbprompt()	- Process a prompt
**
** History:
**	4-nov-93 (robf)	
**	    Created
**	26-jul-95 (duursma)
**	    Added SIflush(stderr) calls in a few places in do_prompt(),
**	    to fix up prompts.  Fixes bug 68699/69765.
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	aug-2009 (stephenb)
**		Prototyping front-ends
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


/* Forward declarations */
typedef struct {
	i4	flags;
#define PROMPT_NOECHO 	0x01
#define PROMPT_TIMEOUT 	0x02
#define PROMPT_PASSWORD 0x04
	i4	status;
#define PROMPT_TIMEDOUT 0x01
	i4	timeout;
	i4	maxlen;
	char	*mesg;
	i4	mesglen;
	char	*reply;
	i4	replen;
} PROMPT_DATA;

static STATUS	unpack_prompt( II_LBQ_CB *, PROMPT_DATA * );
static STATUS	process_prompt( II_THR_CB *, PROMPT_DATA * );
static STATUS	send_reply( II_LBQ_CB *, PROMPT_DATA * );
static VOID	do_prompt(char *, i4, char *,i4);
static STATUS	teget();
static VOID	teclose();

/*{
** Name: IILQdbprompt - Process a DBMS prompt request
**
** Description:
**	This routine calls LIBQGCA to read the values and data associated
**	with an PROMPT that has been received from the DBMS.  It then
**	gets the prompt info, processes it and sends the result back to 
**	the DBMS.
**
**	Ignoring block boundaries, a  prompt message is laid out with
**	the following format:
**
**	    typedef struct {
**		i4		gca_prflags
**		i4		gca_prtimeout
**		i4		gca_prmaxlen
**		i4		gca_l_prmesg
**		char  		gca_prmesg[256]
**	    }
**
**	Note that it is currrently an error if: 
**	    1.  there is more than one GCA_DATA_VALUE for prompt data;
**
**	This restriction will be relaxed in future releases.
**
**
** Inputs:
**	thr_cb		Thread-local-storage control block.
** Outputs:
**	None
**	Errors:
**	    E_LQ00EA_PROMPT_ALLOC - Unable to allocate memory for prompt 
**			handling.
**	    E_LQ00EB_PROMPT_PROTOCOL - Read protocol failure in prompt handling.
** History:
**	4-nov-93 (robf)
**	    Created
*/

VOID
IILQdbprompt( II_THR_CB *thr_cb )
{
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;
    STATUS	status;
    PROMPT_DATA prompt;
    bool	loop;

    /* Don't process prompts if program is disconnecting. */ 
    if (IIlbqcb->ii_lq_curqry & II_Q_DISCON)
	return;

    prompt.flags=0;
    prompt.status=0;
    prompt.mesg=NULL;
    prompt.mesglen=0;
    prompt.reply=NULL;
    prompt.replen=0;

    loop=FALSE;
    do
    {
	/* Get prompt info */
	status=unpack_prompt( IIlbqcb, &prompt );
	if(status==OK)
	{
		/* Prompt user if OK */
		status=process_prompt( thr_cb, &prompt );
	}
	/* Send reply */
	status=send_reply( IIlbqcb, &prompt );
	if(status!=OK)
		break;

    } while(loop);
    /*
    ** Cleanup 
    */
    if(prompt.mesg)
	MEfree(prompt.mesg);

    if(prompt.reply)
	MEfree(prompt.reply) ;

    return;
}

/*
** Name: unpack_prompt
**
** Description:
**	This routine gets a GCA_PROMPT message, unpacks it and fills in
**	the pdata with prompt info
**
** Inputs:
**	IIlbqcb		Current session control block.
**
** Outputs:
**	pdata - prompt data
**
** History:
** 	4-nov-93 (robf)
**	     Created
*/

static STATUS
unpack_prompt( II_LBQ_CB *IIlbqcb, PROMPT_DATA *pdata )
{
    GCA_PROMPT_DATA	prompt;
    IICGC_DESC		*cgc = IIlbqcb->ii_lq_gca;
    DB_DATA_VALUE	db_num;
    DB_DATA_VALUE	db_char;
    i4			type, prec;
    i2			count;
    i4		length;
    i4			i;
    i4			prflags;
    i4			data_count;

    II_DB_SCAL_MACRO(db_num, DB_INT_TYPE, sizeof(i4), 0);
    II_DB_SCAL_MACRO(db_char, DB_CHR_TYPE, 0, 0);

    /* Get flags/timeout/maxlen */
    db_num.db_data=(PTR)&prflags;
    if (IIcgc_get_value(cgc, GCA_PROMPT, IICGC_VVAL, &db_num) != sizeof(i4))	
	goto proto_err;

    if(prflags&GCA_PR_NOECHO)
	pdata->flags|=PROMPT_NOECHO;

    if(prflags&GCA_PR_TIMEOUT)
	pdata->flags|=PROMPT_TIMEOUT;

    if(prflags&GCA_PR_PASSWORD)
	pdata->flags|=PROMPT_PASSWORD;

    db_num.db_data=(PTR)&pdata->timeout;
    if (IIcgc_get_value(cgc, GCA_PROMPT, IICGC_VVAL, &db_num) != sizeof(i4))	
	goto proto_err;

    db_num.db_data=(PTR)&pdata->maxlen;
    if (IIcgc_get_value(cgc, GCA_PROMPT, IICGC_VVAL, &db_num) != sizeof(i4))	
	goto proto_err;

    /* Get the prompt message text */

    db_num.db_data=(PTR)&data_count;
    if (IIcgc_get_value(cgc, GCA_PROMPT, IICGC_VVAL, &db_num) != sizeof(i4))	
	goto proto_err;

    if (data_count == 1)
    {
	/* Read GCA_DATA_VALUE containing prompt */
	db_num.db_data = (PTR)&type;
	if (   IIcgc_get_value(cgc, GCA_PROMPT, IICGC_VVAL, &db_num)
		!= sizeof(i4)
	     || type != DB_CHA_TYPE
	   )
	{
	    goto proto_err;
	}
	db_num.db_data = (PTR)&prec;
	if (IIcgc_get_value(cgc, GCA_PROMPT, IICGC_VVAL, &db_num)!= sizeof(i4))
	    goto proto_err;
	db_num.db_data = (PTR)&length;
	db_num.db_length = sizeof(i4);
	if (IIcgc_get_value(cgc, GCA_PROMPT, IICGC_VVAL, &db_num)
		!= sizeof(i4))
	{
	    goto proto_err;
	}
	count=length;
	/* Allocate for text -- add one for null terminator */
	if ((pdata->mesg = (char *)MEreqmem((u_i4)0, (u_i4)(count +1),
			TRUE, (STATUS *)0)) == (char *)0)
	{
	    IIlocerr( GE_NO_RESOURCE, E_LQ00EA_PROMPT_ALLOC, 
			II_ERR, 0, (char *)0 );
	    return FAIL;
	}
	db_char.db_data = pdata->mesg;
	db_char.db_length = count;
	db_char.db_datatype=type;
	if (IIcgc_get_value(cgc, GCA_PROMPT, IICGC_VVAL, &db_char) != count)
	{
	    goto proto_err;
	}
	pdata->mesg[count] = '\0';
	pdata->mesglen=count;
    }
    else if (data_count != 0)	/* More than one piece of text is err */
    {
	goto proto_err;
    }
    /*
    ** Done  so return OK
    */
    return OK;

proto_err:
    IIlocerr(GE_COMM_ERROR, E_LQ00EB_PROMPT_PROTOCOL, II_ERR, 0, (char *)0);
    return FAIL;
}


/*
** Name: process_prompt
**
** Description:
**	This routine processes a prompt request, getting the input
**	from the user (if possible) and returning the value to the caller.
**
** Inputs:
**	IIlbqcb		Current session control block.
**	pdata - prompt info
**
** Outputs:
**	pdata - prompt reply
**
** History:
** 	4-nov-93 (robf)
**	     Created
*/

static STATUS
process_prompt( II_THR_CB *thr_cb, PROMPT_DATA *pdata )
{
    STATUS	status=OK;
    i4		(*dbprmpt_hdl)() = IIglbcb->ii_gl_hdlrs.ii_dbprmpt_hdl;
    i4		noecho;
    char	*reply;
    i4		replen;
    char	*mesg;
    i4		mesglen;
    bool	loop = FALSE;

    MUp_semaphore( &IIglbcb->ii_gl_sem );

    do
    {
	if(pdata->flags&PROMPT_NOECHO)
		noecho=1;
	else
		noecho=0;
	mesg=pdata->mesg;
	mesglen=pdata->mesglen;
	replen=pdata->maxlen;

	if ((pdata->reply = (char *)MEreqmem((u_i4)0, (u_i4)(replen +1),
			TRUE, (STATUS *)0)) == (char *)0)
	{
	    status=E_LQ00EA_PROMPT_ALLOC;
	    break;
	}
	/*
	** Free up any old saved password if we don't save passwords any more
	*/
        if( ! (IIglbcb->ii_gl_flags & II_G_SAVPWD)  && 
	    IIglbcb->ii_gl_password )
	{
		MEfree( (PTR)IIglbcb->ii_gl_password );
		IIglbcb->ii_gl_password = NULL;
	}
	reply=pdata->reply;
	if(pdata->flags&PROMPT_PASSWORD)
	{
	    /*
	    ** Prompting for password, check if saved 
	    */
      	    if ( IIglbcb->ii_gl_flags & II_G_SAVPWD  && 
		 IIglbcb->ii_gl_password )
	    {
		/*
		** Return saved password
		*/
		STmove( IIglbcb->ii_gl_password, 0 , replen, reply);
		pdata->replen=STlength(reply);
		break;
		
	    }
	}

    	if ( dbprmpt_hdl )
    	{
		_VOID_ IILQsepSessionPush( thr_cb );
		(*dbprmpt_hdl)( mesg, noecho, reply, replen);

		/*
		** Issue error if user has switched sessions in handler and not
		** switched back.
		*/
		_VOID_ IILQscSessionCompare( thr_cb );  /* issues the error */
		IILQspSessionPop( thr_cb );
    	}
	else
	{
		/*
		** Use built-in handler
		*/
		do_prompt(mesg, noecho, reply, replen);
	}
	/*
	** Check the final length
	*/
	pdata->replen=STlength(reply);

        if ( IIglbcb->ii_gl_flags & II_G_SAVPWD  &&
	     pdata->flags & PROMPT_PASSWORD  &&  ! IIglbcb->ii_gl_password )
	{
	    /*
	    ** Save password if requested
	    */
	    if ((IIglbcb->ii_gl_password = (char *)MEreqmem((u_i4)0, 
			(u_i4)(pdata->replen +1),
			TRUE, (STATUS *)0)) == (char *)0)
	    {
	    	status=E_LQ00EA_PROMPT_ALLOC;
	    	break;
	    }
	    STcopy(pdata->reply, IIglbcb->ii_gl_password);
	}
    } while(loop);

    MUv_semaphore( &IIglbcb->ii_gl_sem );

    if ( status != OK )
    {
	IIlocerr( GE_NO_RESOURCE, status, II_ERR, 0, NULL );
    }

    return( status );
}

/*
** Name: send_reply
**
** Description:
**	This routine sends the reply for a prompt back to the partner,
**	essentially  calling the CGC routines to send the data.
**
** Inputs:
**	IIlbqcb		Current session control block.
**	pdata - prompt reply info
**
** Outputs:
**	none	(GCA messages)
**
** History:
** 	4-nov-93 (robf)
**	     Created
*/

static STATUS
send_reply( II_LBQ_CB *IIlbqcb, PROMPT_DATA *pdata)
{
    STATUS status=OK;
    IICGC_DESC		*cgc = IIlbqcb->ii_lq_gca;
    DB_DATA_VALUE	db_num;
    DB_DATA_VALUE	db_char;
    i4			prflags;
    i4			data_count;
    bool		loop;

    loop=FALSE;
    do
    {
	/* Start message */
        IIcgc_init_msg(cgc, GCA_PRREPLY, DB_NOLANG, 0);

	/* Send Status */
	prflags=0;
	if(pdata->status&PROMPT_TIMEDOUT)
		prflags|=GCA_PRREPLY_TIMEDOUT;

	if(pdata->flags&PROMPT_NOECHO)
		prflags|=GCA_PRREPLY_NOECHO;

    	II_DB_SCAL_MACRO(db_num, DB_INT_TYPE, sizeof(i4), 0);
   	db_num.db_data=(PTR)&prflags;
	IIcgc_put_msg(cgc, FALSE, IICGC_VVAL, &db_num);

	/* Reply string, number of reply values */
	data_count=1;
   	db_num.db_data=(PTR)&data_count;
	IIcgc_put_msg(cgc, FALSE, IICGC_VVAL, &db_num);

	II_DB_SCAL_MACRO(db_char, DB_CHR_TYPE, pdata->replen, pdata->reply);
	IIcgc_put_msg(cgc, FALSE, IICGC_VDBV, &db_char);

	/* Complete message and send to the DBMS */
    	IIcgc_end_msg(cgc);

    } while(loop);
    return status;
}

/*
** Name: do_prompt
**
** Description:
**	This is the built-in prompt handler, for when no higher-level
**	handler has been defined.
**
** Inputs:
**	mesg 	- Prompt mesg
**	noecho	- Set to 1 if noecho
**	replen  - Maximum length of reply, excluding NULL 
**
** Outputs:
**	reply	- Where reply goes, NULL terminated
**
** History:
**	4-nov-93 (robf)
** 	   Created
**	26-jul-95 (duursma)
**	   Added SIflush(stderr) calls in a few places to fix up prompts.
**	   Fixes bug 68699/69765.
*/

static VOID
do_prompt( char *mesg, i4  noecho, char *reply, i4  replen )
{
	char	*pptr;
	char	prompt_buf[ER_MAX_LEN+1];


	pptr=mesg;

	if ( noecho )
	{
		TEENV_INFO	junk;

		if ( (noecho = ( SIterminal( stdin ) && TEopen(&junk) == OK )) )
		{
			TErestore( TE_FORMS );
			FEset_exit(teclose);	/* ... on exception */
		}
	}

	for (;;)	
	{
		if( SIterminal( stdin ) )
		{
			SIfprintf( stderr, ERx("%s "), pptr);
			SIflush(stderr);
		}

		MEfill(replen, EOS, (PTR)reply);
		/*
		** Prompt for reply on same line  if possible (nice for
		** short prompts like passwords) otherwise start a new line
		*/
		if(replen+STlength(mesg)>=79)
		{
			SIputc('\n', stderr);
			SIflush(stderr);
		}

		if ( ( noecho ? teget(reply, replen)
				: SIgetrec(reply, replen, stdin) ) != OK )
		{
			break;
		}

		/* strip off trailing blanks */

		(VOID)STtrmwhite(reply);
		/*
		** Done
		*/
		break;
	}
	/*
	** Cleanup in noecho mode
	*/
	if ( noecho )
	{
		FEclr_exit(teclose);
		teclose();
		SIputc('\r', stderr);
		SIflush(stderr);
	}
	return;
}

/*
**
** Name: teget
**
** Description:
**	Read a line of input with no echo.
**
** History:
**	4-nov-93 (robf)
**	   Cloned  from feprompt.c
**	27-jul-95 (duursma)
**	   When we see \n or \r, flush stderr.  This change will eventually
**	   be undone when we fix the VMS CL's so that SIputc() flushes when
**	   it gets a newline (like it would on Unix).
*/
static STATUS
teget ( buf, len )
char		*buf;
register i4	len;
{
	register char	*bp = buf;
	register i4	c;

	while ( len-- > 0 )
	{
		switch ( c = TEget(0) ) 	/* No timeout */
		{
		  case TE_TIMEDOUT:	/* just in case of funny TE CL */
		  case '\n':
		  case '\r':
			SIputc('\n', stderr);
			SIflush(stderr);
			*buf++ = EOS;
			/* fall through ... */
		  case EOF:
			len = 0;
			break;
		  default:
			*buf++ = c;
			break;
		} /* end switch */
	} /* end while */

	return buf == bp ? ENDFILE : OK;
}

/*
**
** Description:
**	Closes the no echo mode for the terminal.  This will be called
**	as part of execption clean-up or once a parameter has been entered.
**
** History:
**	4-nov-93 (robf)
**	   Cloned from  feprompt.c
*/
static VOID
teclose ()
{
	TErestore( TE_NORMAL );
	TEclose();
}

